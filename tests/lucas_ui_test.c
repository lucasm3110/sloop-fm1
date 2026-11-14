/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
static uint32_t host_slots[4u*0x14000u/4u];
#define SMP_USER_XIP(k) ((const uint8_t*)host_slots+(k)*SMP_USER_SIZE)
#define LUCAS_FM 1
#define UI_TEST_MAIN upstream_ui_test_main
#include "ui_pages_test.c"
#define PROJ_HOST 1
#include "../firmware/src/project.c"
static uint32_t lf_dirty(void) {return 1;}
static void block(unsigned count) {int32_t b[2*CTL];while(count--)mix_block(b,CTL);}
static void load_pack(const char *path) {
    FILE *f=fopen(path,"rb");assert(f);
    for(unsigned k=0;k<4;k++) {assert(!fseek(f,SMP_USER_OFF(k),SEEK_SET));assert(fread((uint8_t*)host_slots+k*SMP_USER_SIZE,1,SMP_USER_SIZE,f)==SMP_USER_SIZE);smp_user_scan(k);}
    fclose(f);lf_library_scan();for(unsigned k=0;k<10;k++)assert(lf_library[k].valid);
}
static void chords(void) {
    static const uint8_t major[7]={0,2,4,5,7,9,11},third[7]={4,3,3,4,4,3,3},fifth[7]={7,7,7,7,7,7,6};
    track_t *t=&trk[0];t->p[P_SCALE]=1;t->p[P_QUANT]=2;t->p[P_CHORD]=1;t->p[P_TRANS]=0;song.octave=0;
    for(unsigned root=0;root<12;root++) {t->p[P_ROOT]=root;
        for(unsigned d=0;d<7;d++) {uint8_t notes[4];unsigned n=kb_map(t,key_of_white(d+4));assert(n==60+root+major[d]);assert(chord_notes(t,n,notes)==3);assert(notes[1]-n==third[d] && notes[2]-n==fifth[d]);}}
    t->p[P_ROOT]=2;
    fm1_in.notes=1u<<7;block(100);assert(lf_note[0]==62);fm1_in.notes=0;block(100);
    lf_mode_set(0);for(unsigned k=0;k<27;k++)assert(kb_map(t,k)==53+k);
    lf_mode_set(2);t->p[P_AHOLD]=1;
    fm1_in.notes=1u<<7;block(100);fm1_in.notes=0;block(100);assert(kh_any(t));
    lf_key_change(P_ROOT,1);block(100);assert(!kh_any(t));
    fm1_in.notes=1u<<7;block(100);fm1_in.notes=0;block(100);assert(kh_any(t));
    lf_mode_set(0);block(100);assert(!kh_any(t));
    t->p[P_AHOLD]=0;lf_release(15);block(2000);
    puts("PASS: 12-root diatonic chords, chromatic mapping, held note release on root/mode changes");
}
static void loops(void) {
    unsigned i;int32_t l[CTL],r[CTL];
    trk[0].p[P_ROOT]=2;song.g[G_BPM]=72;
    for(i=0;i<4;i++)assert(lf_loop_toggle(i)==0);
    assert(lf_loop_toggle(4)==3);
    for(i=0;i<CTL;i++)l[i]=r[i]=0;lf_loops_mix(l,r,CTL);
    {unsigned count=0;for(unsigned t=0;t<NPART;t++)for(unsigned v=0;v<NVOICE;v++)count+=trk[t].v[v].active!=0;assert(count<=NVOICE);}
    lf_loops_stop();for(i=0;i<256;i++){memset(l,0,sizeof l);memset(r,0,sizeof r);lf_loops_mix(l,r,CTL);}
    for(i=0;i<4;i++)assert(!lf_layers[i].active);
    assert(lf[LF_FLAGS+6]&1);trk[0].p[P_ROOT]=3;assert(lf_loop_toggle(6)==2);
    assert(lf_loop_toggle(0)==0); /* texture stays available at any key/tempo */
    lf_loops_stop();for(i=0;i<256;i++)lf_loops_mix(l,r,CTL);
    song.playing=1;clk_beat=1;lf_loops_mix(l,r,CTL);assert(lf_loop_toggle(9)==0);
    assert(lf_layers[0].queued);clk_beat=4;lf_loops_mix(l,r,CTL);assert(!lf_layers[0].queued);
    lf_loops_stop();song.playing=0;for(i=0;i<256;i++)lf_loops_mix(l,r,CTL);
    puts("PASS: ten resident imports, four-layer cap, texture pitch, key guard, queued rhythmic start, faded stop");
}
static void effects(void) {
    int32_t l[CTL],r[CTL];unsigned i,j,type;int wide;
    lf_defaults();memset(lf_smooth,0,sizeof lf_smooth);
    for(i=0;i<CTL;i++){l[i]=1234;r[i]=-3456;}lf_master_fx(l,r,CTL);
    for(i=0;i<CTL;i++)assert(l[i]==1234 && r[i]==-3456);
    for(type=0;type<4;type++) {
        lf[LF_MOD]=type;lf[LF_AMOUNT]=127;lf[LF_DEPTH]=80;lf[LF_WIDTH]=0;
        memset(lf_modbuf,0,sizeof lf_modbuf);memset(lf_phaser,0,sizeof lf_phaser);lf_smooth[LF_WIDTH]=0;
        for(j=0;j<3000;j++){for(i=0;i<CTL;i++)l[i]=r[i]=(int32_t)osc_sine((j*CTL+i)*9739152u)/3;lf_master_fx(l,r,CTL);for(i=0;i<CTL;i++)assert(l[i]==r[i]);}
        wide=0;lf[LF_WIDTH]=127;
        for(j=0;j<1000;j++){for(i=0;i<CTL;i++)l[i]=r[i]=(int32_t)osc_sine((j*CTL+i)*9739152u)/3;lf_master_fx(l,r,CTL);for(i=0;i<CTL;i++)wide|=l[i]!=r[i];}
        assert(wide);
    }
    lf[LF_GMIX]=lf[LF_FEEDBACK]=lf[LF_SCATTER]=lf[LF_DENSITY]=127;
    for(j=0;j<20000;j++){for(i=0;i<CTL;i++)l[i]=r[i]=j<1000?(int32_t)osc_sine((j*CTL+i)*9739152u)/2:0;lf_master_fx(l,r,CTL);for(i=0;i<CTL;i++)assert(abs(l[i])<100000 && abs(r[i])<100000);}
    assert(lf_pitch_offset(0,2)==0);puts("PASS: exact dry bypass, four modulation modes mono/wide, grain feedback/headroom bounded");
}
static void persistence(void) {
    project_t a,b;unsigned i;
    for(i=0;i<LF_COUNT;i++)lf[i]=(uint8_t)((i*3)%128);
    lf[LF_EQ]=1;lf[LF_MOD]=2;lf[LF_PITCH]=1;for(i=0;i<10;i++)lf[LF_FLAGS+i]&=3;
    proj_capture(&a);assert(proj_ok(&a));memset(lf,0,sizeof lf);proj_apply(&a,1);proj_capture(&b);
    for(i=0;i<LF_COUNT;i++)assert(lf[i]==((uint16_t)pt_get(&a.t[3],LF_STORE[i/2+1])>>(8*(i&1))&255));
    assert(sizeof a<=3840);a.sum^=1;assert(!proj_ok(&a));
    printf("PASS: extension round trip, damaged checksum rejected; project %zu/3840 bytes\n",sizeof a);
}
int main(int argc,char **argv) {
    host_tracks_init();panel_init();settings_init();layers_init();lf_open(LF_HOME);
    for(unsigned p=0;p<=LF_LOOPINFO;p++){lf_open(p);lf_ui_draw();}
    for(unsigned type=0;type<7;type++)for(unsigned page=0;page<2;page++){lf_open(LF_EFFECTS);lf_effect=type;lf_fxpage=page;lf_ui_draw();}
    tap(B_HOME);assert(lf_page==LF_HOME);tap(B_LFO);assert(lf_page==LF_LOOPS);tap(B_HOME);tap(B_ARP);assert(lf_page==LF_PLAY);
    tap(B_SCL);assert(lf_page==LF_KEY);tap(B_HOME);assert(lf_page==LF_HOME);
    chords();if(argc>1){load_pack(argv[1]);loops();}effects();persistence();
    puts("PASS: all Lucas navigation and knob pages rendered within 240x240");return 0;
}
