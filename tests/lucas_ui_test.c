/* SPDX-License-Identifier: GPL-3.0-only */
#include <stdint.h>
static uint32_t host_slots[4u*0x14000u/4u];
#define SMP_USER_XIP(k) ((const uint8_t*)host_slots+(k)*SMP_USER_SIZE)
#define LUCAS_FM 1
#define UI_TEST_MAIN upstream_ui_test_main
#include "ui_pages_test.c"
#define PROJ_HOST 1
#include "../firmware/src/project.c"
static void lf_save_scene(void){saves++;ui_message("saved");}
static void turn(unsigned role,int delta){encs[panel.enc[role]]=delta;lf_ui_input(0);}
static int64_t render(unsigned frames){int64_t energy=0;int32_t b[2*CTL];for(unsigned f=0;f<frames;f+=CTL){mix_block(b,CTL);for(unsigned i=0;i<2*CTL;i++)energy+=abs(b[i]);}return energy;}
int main(int argc,char **argv){
    host_tracks_init();panel_init();settings_init();layers_init();lf_defaults();song.g[G_BPM]=72;lf_open(LF_HOME);
    assert(argc>1);FILE *f=fopen(argv[1],"rb");assert(f);
    for(unsigned k=0;k<4;k++){assert(!fseek(f,SMP_USER_OFF(k),SEEK_SET));assert(fread((uint8_t*)host_slots+k*SMP_USER_SIZE,1,SMP_USER_SIZE,f)==SMP_USER_SIZE);smp_user_scan(k);}fclose(f);
    lf_library_scan();for(unsigned k=0;k<10;k++)assert(lf_library[k].valid);
    assert(render(2048)==0);tap(B_PLAY);assert(lf_running);assert(render(44100)>100000);
    turn(EN_PRESET,1);assert(lf_page==LF_SOUND && lf_pick==1 && lf[0]==0);
    tap(B_HOME);assert(lf[0]==0);turn(EN_PRESET,2);tap(B_EDIT);
    assert(lf_page==LF_HOME && lf_pending[0]==2 && lf[0]==0);
    render(LF_BAR_SAMPLES);assert(lf[0]==2 && lf_pending[0]==255 && lf_layers[0].id==2);
    for(unsigned k=0;k<4;k++){unsigned v=lf[4+k];turn(EN_K1+k,-4);assert(lf[4+k]==v-4);}
    turn(EN_SELECT,3);assert(lf_selected==3);tap(B_EDIT);assert(lf[8]&8);render(44100);
    turn(EN_PRESET,1);assert(lf_pick==9);turn(EN_PRESET,100);assert(lf_pick==9);tap(B_EDIT);render(LF_BAR_SAMPLES);assert(lf[3]==9);
    for(unsigned k=0;k<4;k++)assert(lf_layers[k].active);
    for(unsigned k=0;k<4;k++)lf[4+k]=0;render(FS);assert(render(4096)<16384);
    fm1_in.notes=(1u<<27)-1;render(4410);for(unsigned t=0;t<3;t++)for(unsigned v=0;v<NVOICE;v++)assert(!trk[t].v[v].active);
    for(unsigned v=0;v<NDRUM;v++)assert(!drums.v[v].active);
    tap(B_REC);tap(B_SEQ);tap(B_LFO);tap(B_FX);assert(lf_page==LF_HOME && !song.rec);
    tap(B_PLAY);assert(!lf_running);render(FS);assert(render(4096)<16384);
    for(unsigned k=0;k<4;k++)assert(!lf_layers[k].active);
    /* A last-moment replacement waits a full bar for its fade, and STOP keeps the choice. */
    lf_defaults();lf_scene_play();lf_position=LF_BAR_SAMPLES-10;lf_scene_choose(0,2);render(64);assert(lf[0]==0 && lf_pending[0]==2);lf_scene_stop();assert(lf[0]==2 && lf_pending[0]==255);render(4096);tap(B_SAVE);assert(saves);
    lf_defaults();project_t p;proj_capture(&p);assert(proj_ok(&p));lf[0]=2;lf[8]=15;proj_apply(&p,1);assert(lf[0]==0 && lf[8]==1);
    pt_set(&p.t[3],LF_STORE[0],0x4C31);lf[0]=99;lf_project_get(&p);assert(lf[0]==0 && lf[1]==3);
    puts("PASS: four resident loop lanes; browse/cancel/commit; bar-boundary replacement; fixed knob levels; mute; stop; no synth/drum notes; project roundtrip and legacy migration");
    if(argc>2){outdir=argv[2];ui.msg_t=0;lf_selected=0;lf_hot=255;lf_defaults();lf_open(LF_HOME);lf_ui_draw();ppm("main");turn(EN_PRESET,1);lf_ui_draw();ppm("chooser");}
    for(unsigned k=0;k<4;k++){lf_selected=k;lf_open(LF_HOME);lf_ui_draw();for(unsigned id=lf_first[k];id<lf_first[k+1];id++){lf_pick=id;lf_open(LF_SOUND);lf_ui_draw();}}
    puts("PASS: both screens, every role and sound stay within 240x240");return 0;
}
