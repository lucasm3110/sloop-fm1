/* SPDX-License-Identifier: GPL-3.0-only */
/* The printed controls are the contract: SELECT focus, PRESETS sounds, ALGORITHM synthesis. */
enum { LF_HOME, LF_PLAY, LF_SOUND, LF_SEQUENCE, LF_LOOPS, LF_MIX, LF_EFFECTS, LF_SAVE, LF_ENV, LF_LFO, LF_ARP, LF_TONE, LF_ADV=255 };
static uint8_t lf_page,lf_effect=1,lf_fxpage,lf_pick,lf_save_action,lf_confirm,lf_home_loops,lf_tonepage;
static uint8_t lf_fx_saved[7],lf_fx_patch;
static uint32_t lf_confirm_ms;
static const char *const LF_PAGE[]={"HOME","SCALE / PLAY","SOUND","SEQUENCE","LOOPS","MIX","EFFECTS","SAVE","ENVELOPE","LFO","ARPEGGIATOR","SOUND EDIT"};
static const char *const LF_ROLE[]={"PAD","BASS","MELODY","DRUMS"};
static const char *const LF_EFFECT[]={"SHAPE","EQ","DRIVE","MODULATION","GLITCH / GRAIN","DELAY","REVERB"};
static const uint16_t LF_COLOR[]={RGB(172,151,255),RGB(66,218,198),RGB(255,199,104),RGB(255,112,143)};
static uint32_t lf_cache[6];
static const uint16_t LF_BG=RGB(8,9,13),LF_DIM=RGB(145,147,169),LF_TEXT=RGB(238,239,247),LF_SURFACE=RGB(22,24,34);
static uint32_t lf_voice_page(void) {return lf_page==LF_ENV || lf_page==LF_LFO || lf_page==LF_ARP || lf_page==LF_TONE;}
static uint32_t lf_dirty(void); /* project.c: compares with the last persisted working song */
UI_SMALL static void lf_open(uint32_t p)
{
    lf_advanced=(uint8_t)(p==LF_ADV);
    lf_page=(uint8_t)p; lf_confirm=0; lf_home_loops=0; ui.force=1; ui.menu=0; vis_on=0;
    ui.layer=LY_PLAY; ly_lock=LY_PLAY; song.seq_mode=0; kb_grid=0;
    lf_loop_page=(uint8_t)(p==LF_LOOPS);
}
UI_SMALL static void lf_release(uint32_t tracks) { panic_req|=(uint8_t)tracks; }
UI_SMALL static void lf_key_change(uint32_t id,int32_t delta)
{
    uint32_t i;
    int32_t v=clamp(trk[0].p[id]+delta,id==P_SCALE?1:0,id==P_SCALE?NSCALES-1:11);
    lf_release(7);
    for(i=0;i<NPART;i++) {trk[i].p[id]=(int16_t)v;lf_note[i]=(uint8_t)(60+trk[i].p[P_ROOT]);}
}
static uint32_t lf_mode(void) { return TSEL->p[P_CHORD]?2u:TSEL->p[P_QUANT]?1u:0u; }
UI_SMALL static void lf_mode_set(int32_t v)
{
    if(is_drum(TSEL)) return;
    lf_release(1u<<song.sel);
    TSEL->p[P_QUANT]=(int16_t)(v?2:0); TSEL->p[P_CHORD]=(int16_t)(v==2);
}
UI_SMALL static void lf_pedit(uint32_t id,int32_t s)
{
    const param_desc_t *d=track_desc(TSEL,id);
    TSEL->p[id]=(int16_t)clamp(TSEL->p[id]+s,d->min,d->max);
}
UI_SMALL static void lf_gedit(uint32_t id,int32_t s) { song.g[id]=(int16_t)clamp(song.g[id]+s,GP[id].min,GP[id].max); }
UI_SMALL static void lf_edit(uint32_t id,int32_t s) { lf[id]=(uint8_t)clamp(lf[id]+s,0,127); }
UI_SMALL static uint32_t lf_param(uint32_t k)
{
    static const uint8_t lfo[]={P_LRATE,P_LD_PIT,P_LD_FLT,P_LD_AMP},arp[]={P_ARATE,P_AOCT,P_AGATE,P_AHOLD};
    return lf_page==LF_ENV?P_ATK+k:lf_page==LF_LFO?lfo[k]:lf_page==LF_ARP?arp[k]:P_E0+4*lf_tonepage+k;
}
UI_SMALL static void lf_fx_knob(uint32_t k,int32_t s)
{
    static const uint8_t env[]={P_ATK,P_DEC,P_SUS,P_REL};
    static const uint8_t eq[]={LF_LOW,LF_BASS,LF_MID,LF_TREBLE};
    static const uint8_t grain[]={LF_GMIX,LF_GSIZE,LF_GDELAY,LF_SCATTER,LF_DENSITY,LF_RANDOM,LF_FEEDBACK,LF_GWIDTH};
    lf_cpu_guard=0;
    switch(lf_effect) {
    case 0: lf_pedit(env[k],s); break;
    case 1: if(!lf_fxpage) {lf_edit(eq[k],s);lf[LF_EQ]=1;} else if(k==0) {lf_edit(LF_HIGH,s);lf[LF_EQ]=1;} else if(k==3) lf[LF_EQ]=(uint8_t)(s>0); break;
    case 2: if(k<3) lf_edit(LF_DRIVE+k,s); break;
    case 3: lf_edit(LF_AMOUNT+k,s); break;
    case 4: lf_edit(grain[k+4u*(lf_fxpage!=0)],s); break;
    case 5: if(!k) { if(is_drum(TSEL)) lf_gedit(G_DRDLY,s); else lf_pedit(P_DLY,s); }
            else lf_gedit(k==1?G_DTIME:k==2?G_DFDBK:G_DCOLOR,s); break;
    case 6: if(!k) { if(is_drum(TSEL)) lf_gedit(G_DRREV,s); else lf_pedit(P_REV,s); }
            else if(k==1) lf_gedit(G_RSIZE,s); else if(k==2) lf_gedit(G_RDAMP,-s); break;
    }
}
UI_SMALL static uint32_t lf_fx_amount(void)
{
    switch(lf_effect) {
    case 1:return lf[LF_EQ];case 2:return lf[LF_DRIVE];case 3:return lf[LF_AMOUNT];case 4:return lf[LF_GMIX];
    case 5:return is_drum(TSEL)?song.g[G_DRDLY]:TSEL->p[P_DLY];
    case 6:return is_drum(TSEL)?song.g[G_DRREV]:TSEL->p[P_REV];default:return 1;
    }
}
UI_SMALL static void lf_fx_toggle(void)
{
    uint32_t old=lf_fx_amount(),v=old?0:lf_fx_saved[lf_effect]?lf_fx_saved[lf_effect]:80;
    if(old)lf_fx_saved[lf_effect]=(uint8_t)old;
    lf_cpu_guard=0;
    if(lf_effect==1)lf[LF_EQ]=(uint8_t)!old;
    else if(lf_effect<5 && lf_effect)lf[lf_effect==2?LF_DRIVE:lf_effect==3?LF_AMOUNT:LF_GMIX]=(uint8_t)v;
    else if(lf_effect>=5) {if(is_drum(TSEL))song.g[lf_effect==5?G_DRDLY:G_DRREV]=(int16_t)v;
        else TSEL->p[lf_effect==5?P_DLY:P_REV]=(int16_t)v;}
}
UI_SMALL static void lf_fx_preset(int32_t s)
{
    static const uint8_t patch[4][3][4]={
        {{0,64,64,64},{25,78,60,52},{20,58,66,84}},
        {{30,90,100,0},{80,70,82,0},{120,48,68,0}},
        {{50,2,32,90},{70,6,65,80},{90,0,90,50}},
        {{45,35,90,0},{70,85,72,0},{95,120,45,0}}};
    uint32_t i=lf_effect<3?lf_effect-1:lf_effect-3,k;
    lf_fx_patch=(uint8_t)clamp(lf_fx_patch+s,0,2);lf_fxpage=0;
    for(k=0;k<4;k++) {lf_fx_knob(k,-255);lf_fx_knob(k,patch[i][lf_fx_patch][k]);}
    if(lf_effect==1)lf[LF_HIGH]=127;
}
UI_SMALL static int lf_ui_input(uint32_t pressed)
{
    uint32_t b,k; int32_t s;
    if(pressed&(1u<<panel.btn[B_HOME])) { lf_open(LF_HOME); ui.home_t0=0; return 1; }
    if(lf_page==LF_ADV) return 0;
    enc_hold=0; ui.layer=LY_PLAY; kb_grid=0;
    lf_library_scan();
    if(pressed) ui_input_ms=fm1_ms;
    pc_poll(); fm6_poll(); ml_poll(); midi_chan_fix(); remote_poll();
    for(b=0;b<NB;b++) if(pressed&(1u<<panel.btn[b])) switch(b) {
    case B_PLAY:
        if(!ft_owns_press()) {
            transport_req=song.playing?2:1;
            if(song.playing) { lf_loops_stop(); lf_release(15); }
        } break;
    case B_REC: rec_tempo=1;rec_toggle(); break;
    case B_SCL: lf_open(LF_PLAY); break;
    case B_EDIT:
        if(lf_page==LF_HOME && lf_home_loops) lf_open(LF_LOOPS);
        else if(lf_page==LF_PLAY && !is_drum(TSEL)) {lf_release(1u<<song.sel);TSEL->p[P_AHOLD]^=1;}
        else if(lf_page==LF_SOUND) {if(is_drum(TSEL)) {lf_page=LF_ADV;lf_advanced=1;studio_open(SC_DRUM);} else {lf_tonepage=0;lf_open(LF_TONE);}}
        else if(lf_page==LF_TONE) {if(!lf_tonepage)lf_tonepage=1;else lf_open(LF_SOUND);}
        else if(lf_page==LF_EFFECTS && lf_effect) lf_fx_toggle();
        else if(lf_page==LF_LOOPS) { int rc; fm1_irq_off(); rc=lf_loop_toggle(lf_pick); fm1_irq_on();
            if(rc) ui_message(rc==1?"IMPORT A LOOP PACK":rc==2?"PREPARE KEY / BPM":"4 LAYERS BUSY"); }
        else if(lf_page==LF_SEQUENCE) { undo_swap(undo.undone?1:0); }
        else if(lf_page==LF_SAVE) {
            if(lf_save_action==2) { lf_page=LF_ADV;lf_advanced=1; ui.menu=1; ui.menu_sel=0; ui.force=1; break; }
            if(!lf_confirm || fm1_ms-lf_confirm_ms>3000u) { lf_confirm=1;lf_confirm_ms=fm1_ms;ui_message("EDIT AGAIN TO CONFIRM"); }
            else { lf_confirm=0; if(lf_save_action==0) project_save((uint32_t)song.g[G_SLOT]-1u);
                else project_load((uint32_t)song.g[G_SLOT]-1u); }
        } else lf_open(LF_SOUND); break;
    case B_ENV: lf_open(LF_ENV);break;
    case B_FX: lf_cpu_guard=0;if(lf_page==LF_EFFECTS) { lf_fxpage^=1; ui.force=1; } else lf_open(LF_EFFECTS);break;
    case B_LFO: lf_open(LF_LFO); break;
    case B_GLO: lf_open(LF_MIX); break;
    case B_ARP: lf_open(LF_ARP);break;
    case B_SEQ: if(lf_page==LF_SEQUENCE) { lf_page=LF_ADV;lf_advanced=1; open_family(FAM_SEQ); if(is_drum(TSEL)) studio_open(SC_DRUM); }
                else lf_open(LF_SEQUENCE);break;
    case B_SAVE: lf_open(LF_SAVE);break;
    case B_OCTDN: case B_OCTUP:
        if(lf_page==LF_LOOPS) lf_pick=(uint8_t)clamp((int32_t)lf_pick+(b==B_OCTDN?-4:4),0,9);
        else { lf_release(7);song.octave=(int8_t)clamp(song.octave+(b==B_OCTDN?-1:1),-3,3); }
        break;
    default:break;
    }
    if((s=panel_enc(EN_SELECT))) {
        if(lf_page==LF_LOOPS) lf_pick=(uint8_t)clamp(lf_pick+s,0,9);
        else if(lf_page==LF_SAVE) {lf_save_action=(uint8_t)clamp(lf_save_action+s,0,2);lf_confirm=0;}
        else {
            uint32_t focus=(uint32_t)clamp((lf_page==LF_HOME && lf_home_loops?4:(int32_t)song.sel)+s,0,lf_page==LF_HOME?4:3);
            lf_home_loops=(uint8_t)(focus==4);
            if(focus<4 && focus!=song.sel) {lf_release(1u<<song.sel);track_select(focus);}
        }
    }
    if((s=panel_enc(EN_ALGO))) {
        if(is_drum(TSEL) && lf_voice_page()) {}
        else if(lf_page==LF_EFFECTS) {lf_effect=(uint8_t)clamp(lf_effect+s,1,6);lf_fxpage=0;}
        else if(lf_page==LF_LFO) lf_pedit(P_LWAVE,s);
        else if(lf_page==LF_ARP) {lf_release(1u<<song.sel);lf_pedit(P_AMODE,s);}
        else if(lf_page==LF_LOOPS) {
            const lf_meta_t *m=&lf_library[lf_pick].meta;
            uint32_t flags=(uint32_t)clamp(lf[LF_FLAGS+lf_pick]+s,0,3);
            if(((flags&1) && m->root>=12) || ((flags&2) && !m->bpm)) ui_message("PACK NEEDS METADATA");
            else lf[LF_FLAGS+lf_pick]=(uint8_t)flags;
        } else if(lf_page!=LF_SAVE && !lf_home_loops) {
            lf_release(1u<<song.sel);
            if(is_drum(TSEL))lf_pedit(P_E0,s);
            else select_engine((uint32_t)clamp((int32_t)TSEL->eng_req+s,0,NENGINES-1));
        }
    }
    if((s=panel_enc(EN_PRESET))) {
        if(lf_page==LF_LOOPS) lf_pick=(uint8_t)clamp((int32_t)(lf_pick/4)+s,0,2)*4;
        else if(lf_page==LF_SAVE) {lf_gedit(G_SLOT,s);lf_confirm=0;}
        else if(lf_page==LF_SEQUENCE) { uint32_t section=(uint32_t)clamp(song.g[G_SLOT]-1+s,0,3);song.g[G_SLOT]=(int16_t)(section+1);
            if(project_used(section)) section_load(section); else ui_message("EMPTY SECTION"); }
        else if(lf_page==LF_EFFECTS && lf_effect==3) lf[LF_MOD]=(uint8_t)clamp(lf[LF_MOD]+s,0,3);
        else if(lf_page==LF_EFFECTS && lf_effect==4) lf[LF_PITCH]=(uint8_t)clamp(lf[LF_PITCH]+s,0,2);
        else if(lf_page==LF_EFFECTS)lf_fx_preset(s);
        else if(lf_page==LF_HOME && !lf_home_loops && !is_drum(TSEL)) {
            uint32_t total,cur=preset_pos(&total);lf_release(1u<<song.sel);
            if(total)preset_go((uint32_t)(((int32_t)cur+s%(int32_t)total+(int32_t)total)%(int32_t)total));
        } else if(!lf_home_loops) {
            uint32_t n=ENGINES[TSEL->eng_req%NENGINES]->npresets;
            lf_release(1u<<song.sel);
            if(is_drum(TSEL))lf_pedit(P_E0,s);
            else if(n)apply_preset((uint32_t)(((int32_t)TSEL->preset+s%(int32_t)n+(int32_t)n)%(int32_t)n));
        }
    }
    for(k=0;k<4;k++) if((s=panel_enc(EN_K1+k))) {
        if(is_drum(TSEL) && lf_voice_page()) continue;
        if(lf_page==LF_HOME) {
            if(k==3)lf_gedit(G_DRLVL,s);else trk[k].p[P_LEVEL]=(int16_t)clamp(trk[k].p[P_LEVEL]+s,0,127);
        } else if(lf_page==LF_PLAY) {
            if(k==0) lf_key_change(P_ROOT,s); else if(k==1) lf_key_change(P_SCALE,s);
            else if(k==2) lf_gedit(G_BPM,s); else lf_mode_set(clamp((int32_t)lf_mode()+s,0,2));
        } else if(lf_page==LF_SOUND) {
            if(k==0) { if(is_drum(TSEL)) lf_gedit(G_DRLVL,s); else lf_pedit(P_LEVEL,s); }
            else if(k==1) lf_pedit(P_TFLT,s);
            else if(k==2) { if(is_drum(TSEL)) lf_gedit(G_DRREV,s); else lf_pedit(P_REV,s); }
            else lf_mode_set(clamp((int32_t)lf_mode()+s,0,2));
        } else if(lf_voice_page()) {if(lf_page==LF_ARP && k==3)lf_release(1u<<song.sel);lf_pedit(lf_param(k),s);}
        else if(lf_page==LF_LOOPS) { uint32_t id=(lf_pick/4)*4+k; if(id<10) lf_edit(LF_LEVEL+id,s); }
        else if(lf_page==LF_MIX) {
            if(k==0) { if(is_drum(TSEL)) lf_gedit(G_DRLVL,s);else lf_pedit(P_LEVEL,s); }
            if(k==1) lf_pedit(P_PAN,s);
            if(k==2) TSEL->p[P_MUTE]=(int16_t)(s>0);
            if(k==3) song.solo=(uint8_t)(s>0?1u<<song.sel:0);
        } else if(lf_page==LF_SEQUENCE) {
            if(k==0) { int32_t bars=clamp(TSEL->p[P_SLEN]/16+s,1,4);TSEL->p[P_SLEN]=(int16_t)(bars*16); }
            if(k==1) rec_count=(uint8_t)(s>0);
            if(k==2) rec_quant_off=(uint8_t)(s<0);
            if(k==3) lf_gedit(G_CLOCK,s);
            rec_tempo=1;
        } else if(lf_page==LF_EFFECTS) lf_fx_knob(k,s);
        else if(lf_page==LF_SAVE && !k) {lf_gedit(G_SLOT,s);lf_confirm=0;}
    }
    return 1;
}
UI_SMALL static void lf_text(int x,int y,const char *s,uint16_t c) { cv_text(x,y,&FONT_S,s,c); }
UI_SMALL static void lf_num(int x,int y,int32_t v,uint16_t c) { char b[16];fmt_int(b,v);lf_text(x,y,b,c); }
UI_SMALL static void lf_sound_name(track_t *t,char *b)
{
    uint32_t u=user_of(t);const engine_t *e=ENGINES[t->eng_req%NENGINES];
    if(is_drum(t))str_cpy(b,DRUM_KIT_NAMES[clamp(t->p[P_E0],0,DRUM_KITS-1)],20);
    else if(u<UP_SLOTS)up_name(u,b);
    else str_cpy(b,e->presets[t->preset%e->npresets].name,20);
}
UI_SMALL static void lf_footer(const char **labels,char val[4][16])
{
    uint32_t k;
    for(k=0;k<4;k++) {
        uint16_t c=LF_COLOR[lf_page==LF_HOME?k:song.sel];int x=(int)k*60;
        cv_rect(x+1,0,58,40,LF_SURFACE);cv_rect(x+1,0,58,2,c);
        lf_text(x+4,3,labels[k],LF_DIM);
        lf_text(x+4,22,val[k],c);
    }
}
UI_SMALL static void lf_values(const char **lab,char val[4][16])
{
    static const char *const modes[]={"PIANO","NOTES","CHORD"};
    static const char *const defaults[]={"LEVEL","TONE","REVERB","KEYS"};
    uint32_t k;
    for(k=0;k<4;k++) {lab[k]=defaults[k];fmt_int(val[k],0);}
    if(lf_page==LF_HOME) {
        for(k=0;k<4;k++) {lab[k]=LF_ROLE[k];fmt_int(val[k],(k==3?song.g[G_DRLVL]:trk[k].p[P_LEVEL])*100/127);}
    } else if(lf_voice_page()) {
        for(k=0;k<4;k++) {
            uint32_t id=lf_param(k);
            const param_desc_t *d=track_desc(TSEL,id);const char *unit;
            lab[k]=d->label;param_format(d,TSEL->p[id],val[k],&unit);
            if(str_len(val[k])+str_len(unit)<=7)str_cpy(val[k]+str_len(val[k]),unit,16-str_len(val[k]));
        }
        if(lf_page==LF_ENV) {lab[0]="ATTACK";lab[1]="DECAY";lab[2]="SUSTAIN";lab[3]="RELEASE";}
        if(lf_page==LF_LFO) {lab[0]="SPEED";lab[1]="PITCH";lab[2]="FILTER";lab[3]="LEVEL";}
        if(lf_page==LF_ARP) {lab[0]="RATE";lab[1]="OCTAVE";lab[2]="GATE";lab[3]="HOLD";}
    } else if(lf_page==LF_PLAY) {
        lab[0]="KEY";lab[1]="SCALE";lab[2]="BPM";lab[3]="KEYS";
        str_cpy(val[0],N_NOTE[trk[0].p[P_ROOT]%12],16);str_cpy(val[1],N_SCALE[trk[0].p[P_SCALE]%NSCALES],16);
        fmt_int(val[2],song.g[G_BPM]);str_cpy(val[3],modes[lf_mode()],16);
    } else if(lf_page==LF_LOOPS) {
        static const char *const nums[]={"1","2","3","4","5","6","7","8","9","10"};
        for(k=0;k<4;k++) {uint32_t id=lf_pick/4*4+k;lab[k]=id<10?nums[id]:"";if(id<10) fmt_int(val[k],lf[LF_LEVEL+id]*100/127);else val[k][0]=0;}
    } else if(lf_page==LF_SEQUENCE) {
        lab[0]="BARS";lab[1]="COUNT";lab[2]="QUANT";lab[3]="CLICK";
        fmt_int(val[0],TSEL->p[P_SLEN]/16);str_cpy(val[1],rec_count?"1 BAR":"NOTE",16);
        str_cpy(val[2],rec_quant_off?"FREE":"1/16",16);str_cpy(val[3],N_CLICK[song.g[G_CLOCK]],16);
    } else if(lf_page==LF_SAVE) {
        lab[0]="SLOT";lab[1]=lab[2]=lab[3]="";fmt_int(val[0],song.g[G_SLOT]);val[1][0]=val[2][0]=val[3][0]=0;
    } else {
        fmt_int(val[0],is_drum(TSEL)?song.g[G_DRLVL]:TSEL->p[P_LEVEL]);fmt_int(val[1],TSEL->p[P_TFLT]);
        fmt_int(val[2],is_drum(TSEL)?song.g[G_DRREV]:TSEL->p[P_REV]);str_cpy(val[3],modes[lf_mode()],16);
        if(lf_page==LF_MIX) {lab[1]="PAN";lab[2]="MUTE";lab[3]="SOLO";fmt_int(val[1],TSEL->p[P_PAN]);str_cpy(val[2],TSEL->p[P_MUTE]?"ON":"OFF",16);str_cpy(val[3],song.solo&(1u<<song.sel)?"ON":"OFF",16);}
    }
    if(is_drum(TSEL) && lf_voice_page())for(k=0;k<4;k++){lab[k]="";val[k][0]=0;}
    if(lf_page==LF_EFFECTS) {
        static const char *const names[9][4]={{"ATTACK","DECAY","SUSTAIN","RELEASE"},{"LOWCUT","BASS","MID","TREBLE"},{"AMOUNT","TONE","OUTPUT",""},
            {"AMOUNT","SPEED","DEPTH","WIDTH"},{"MIX","SIZE","DELAY","SCATTER"},{"SEND","TIME","FDBACK","TONE"},{"SEND","DECAY","TONE",""},
            {"HIGHCUT","","","ACTIVE"},{"DENSITY","TIMING","FDBACK","WIDTH"}};
        uint32_t row=lf_effect;
        if(lf_fxpage && lf_effect==1) row=7;
        if(lf_fxpage && lf_effect==4) row=8;
        for(k=0;k<4;k++) {
            int32_t v=0; lab[k]=names[row][k];
            if(row==0) v=TSEL->p[P_ATK+k];
            if(row==1) v=k?((int32_t)lf[LF_LOW+k]-64):lf[LF_LOW];
            if(row==2 && k<3) v=lf[LF_DRIVE+k];
            if(row==3) v=lf[LF_AMOUNT+k];
            if(row==4) v=lf[LF_GMIX+k];
            if(row==5) v=k==0?(is_drum(TSEL)?song.g[G_DRDLY]:TSEL->p[P_DLY]):song.g[k==1?G_DTIME:k==2?G_DFDBK:G_DCOLOR];
            if(row==6) v=k==0?(is_drum(TSEL)?song.g[G_DRREV]:TSEL->p[P_REV]):k==1?song.g[G_RSIZE]:127-song.g[G_RDAMP];
            if(row==7) v=k==0?lf[LF_HIGH]:lf[LF_EQ];
            if(row==8) v=lf[LF_DENSITY+k];
            fmt_int(val[k],v); if(!lab[k][0]) val[k][0]=0;
            if(row==7 && k==3)str_cpy(val[k],v?"ON":"OFF",16);
            if(row==5 && k==1) str_cpy(val[k],N_DLY[song.g[G_DTIME]],16);
        }
    }
}
UI_SMALL static void lf_chord_name(char *b,uint8_t *notes,uint32_t n)
{
    str_cpy(b,N_NOTE[notes[0]%12],16);
    if(n>=3) {uint32_t third=(notes[1]-notes[0])%12,fifth=(notes[2]-notes[0])%12;
        str_cpy(b+str_len(b),fifth==6?"dim":third==3?"m":"",16-str_len(b));}
}
UI_SMALL static int lf_ui_draw(void)
{
    uint32_t row,k; const char *lab[4]; char val[4][16];
    if(lf_page==LF_ADV) return 0;
    lf_values(lab,val);pads_tick();
    /* Six 40-pixel strips, hashed before SPI: unchanged content is never resent. */
    for(row=0;row<6;row++) {
        uint32_t sig=2166136261u;
        cv_oy=0;cv_begin(240,40,LF_BG);
        if(!row) {
            uint16_t c=LF_COLOR[song.sel];
            lf_text(6,1,lf_page==LF_HOME?"LUCAS FM":LF_PAGE[lf_page],c);
            lf_text(185,1,ci_on?"COUNT":song.rec?"REC":rec_wait?"ARM":song.playing?"PLAY":"STOP",song.rec?LF_COLOR[3]:LF_DIM);
            cv_rect(6,36,228,1,LF_SURFACE);
            if(lf_page==LF_HOME) {
                lf_text(6,19,N_NOTE[trk[0].p[P_ROOT]%12],LF_TEXT);lf_text(30,19,N_SCALE[trk[0].p[P_SCALE]%NSCALES],LF_DIM);
                lf_num(85,19,song.g[G_BPM],LF_TEXT);lf_text(113,19,"BPM",LF_DIM);
                lf_text(144,19,lf_home_loops?"EDIT: LOOPS":"EDIT: SOUND",LF_DIM);
            } else {
                lf_num(6,19,song.sel+1,c);lf_text(20,19,LF_ROLE[song.sel],c);
                lf_text(88,19,lf_page==LF_EFFECTS?"ALGO: EFFECT":lf_page==LF_LFO?"ALGO: WAVE":lf_page==LF_ARP?"ALGO: PATTERN":lf_page==LF_LOOPS?"ALGO: MATCH":lf_page==LF_SAVE?"SELECT: ACTION":"SELECT: PART",LF_DIM);
            }
            if(lf_dirty())cv_rect(233,3,3,3,LF_COLOR[2]);
        } else if(row==5) lf_footer(lab,val);
        else if(lf_page==LF_HOME) {
            uint32_t t,active=0;cv_oy=-(int32_t)(row-1)*40;
            for(t=0;t<4;t++) {
                int y=(int)t*32;char name[20];uint16_t c=trk_silent(&trk[t])?LF_DIM:LF_COLOR[t];
                if(song.sel==t && !lf_home_loops)cv_rect(4,y,232,30,LF_SURFACE);
                cv_rect(4,y+2,3,26,c);lf_num(12,y+5,t+1,c);lf_sound_name(&trk[t],name);
                lf_text(32,y+5,name,song.sel==t && !lf_home_loops?LF_TEXT:LF_DIM);
                if(trk_silent(&trk[t]))lf_text(202,y+5,"M",LF_DIM);
                cv_rect(32,y+25,186,2,LF_SURFACE);
                cv_rect(32,y+25,(t==3?song.g[G_DRLVL]:trk[t].p[P_LEVEL])*186/127,2,c);
            }
            for(t=0;t<4;t++)active+=lf_layers[t].active!=0;
            cv_rect(4,133,232,24,lf_home_loops?RGB(43,36,62):LF_SURFACE);
            lf_text(10,137,"LOOPS",LF_COLOR[0]);lf_num(65,137,active,LF_COLOR[1]);lf_text(77,137,"/4",LF_DIM);
            lf_text(105,137,lf_home_loops?"EDIT: OPEN":"SELECT +",LF_DIM);cv_oy=0;
        } else if(lf_page==LF_SEQUENCE) {
            uint32_t t=row-1,x;track_t *tr=&trk[t];
            if(song.sel==t)cv_rect(4,0,232,38,LF_SURFACE);
            cv_rect(4,2,3,34,LF_COLOR[t]);lf_text(12,0,LF_ROLE[t],trk_silent(tr)?LF_DIM:LF_COLOR[t]);
            if(t==song.sel)lf_text(135,0,"EDIT: UNDO",LF_DIM);
            for(x=0;x<16;x++) {uint32_t st=(tr->seq_idx/16u)*16u+x,on=t==3?dstep_mask(&tr->dstep[st]):step_on(&tr->step[st]);
                cv_rect(12+x*14,23,9,on?11:3,on?LF_COLOR[t]:LF_SURFACE);
                if(song.playing && st==tr->seq_idx)cv_rect(12+x*14,18,2,20,LF_TEXT);}
        } else if(lf_page==LF_MIX) {
            uint32_t t;cv_oy=-(int32_t)(row-1)*40;
            for(t=0;t<4;t++) {int x=8+(int)t*58,v=(t==3?song.g[G_DRLVL]:trk[t].p[P_LEVEL])*100/127;
                uint16_t c=trk_silent(&trk[t])?LF_DIM:LF_COLOR[t];
                lf_num(x+14,0,t+1,c);cv_rect(x+7,25,25,100,LF_SURFACE);cv_rect(x+7,125-v,25,v,c);
                lf_num(x+7,130,v,LF_TEXT);if(t==song.sel)cv_rect(x+4,20,31,2,LF_TEXT);
            }cv_oy=0;
        } else if(lf_page==LF_PLAY) {
            uint8_t root=lf_note[song.sel],ns[4]={root,0,0,0};char name[16];uint32_t nn=1;
            if(TSEL->p[P_CHORD]) nn=chord_notes(TSEL,root,ns);
            lf_chord_name(name,ns,nn);
            if(lf_mode() && !is_drum(TSEL)) {
                static const int16_t xy[7][2]={{120,15},{176,43},{190,96},{151,140},{89,140},{50,96},{64,43}};
                uint32_t deg=0,mask=scale_mask(TSEL),d=(root+120-TSEL->p[P_ROOT])%12;
                for(k=0;k<d;k++) if(mask&(1u<<k))deg++;
                cv_oy=-(int32_t)(row-1)*40;
                for(k=0;k<7;k++) {
                    uint32_t next=(k+1)%7;int x=xy[k][0],y=xy[k][1],dx,dy;
                    cv_line(x,y,xy[next][0],xy[next][1],RGB(48,43,66));
                    for(dy=-11;dy<=11;dy++)for(dx=-11;dx<=11;dx++)if(dx*dx+dy*dy<=121)cv_pset(x+dx,y+dy,k==deg?LF_COLOR[0]:RGB(45,40,62));
                    lf_num(x-4,y-8,k+1,k==deg?LF_BG:LF_TEXT);
                }
                {const char *quality=nn<3?"NOTE":(ns[2]-ns[0])%12==6?"DIM":(ns[1]-ns[0])%12==3?"MIN":"MAJ";
                    const char *rootname=N_NOTE[root%12];cv_text(120-text_w(&FONT_L,rootname)/2,54,&FONT_L,rootname,LF_COLOR[song.sel]);
                    lf_text(120-text_w(&FONT_S,quality)/2,87,quality,LF_DIM);}
                {int width=0,x;for(k=0;k<nn;k++)width+=text_w(&FONT_S,N_NOTE[ns[k]%12])+8;x=120-width/2;
                    for(k=0;k<nn;k++){lf_text(x,107,N_NOTE[ns[k]%12],LF_TEXT);x+=text_w(&FONT_S,N_NOTE[ns[k]%12])+8;}}
                lf_text(166,0,TSEL->p[P_AHOLD]?"HELD":"FREE",LF_DIM);lf_text(4,145,"EDIT: HOLD",LF_DIM);cv_oy=0;
            } else {
                if(row==1)lf_text(8,4,is_drum(TSEL)?"DRUM KEYS":"GUIDED PIANO",LF_DIM);
                if(row==2)cv_text(82,0,&FONT_L,name,LF_COLOR[song.sel]);
                if(row==3)lf_text(8,5,"ALL NOTES PLAYABLE",LF_DIM);
                if(row==4)for(k=0;k<27;k++) {uint32_t note=53+k;uint16_t c=(scale_mask(TSEL)>>((note-TSEL->p[P_ROOT]+120)%12))&1u?LF_COLOR[0]:LF_DIM;
                    cv_rect(4+k*8,4,6,30,c);}
            }
        } else if(lf_page==LF_SOUND) {
            const engine_t *engine=ENGINES[TSEL->eng_req%NENGINES];
            int32_t pos=(int32_t)TSEL->preset+(int32_t)row-2;char name[20];
            if(row==4) {lf_text(6,0,"ALGO",LF_DIM);lf_text(43,0,is_drum(TSEL)?"DRUM KIT":ENGINES[TSEL->eng_req%NENGINES]->name,LF_COLOR[song.sel]);lf_text(6,19,"PRE: SOUND / EDIT: SHAPE",LF_DIM);}
            else {
            if(is_drum(TSEL)) {pos=TDRUM->p[P_E0]+(int)row-2;if(pos<0 || pos>=DRUM_KITS) continue;str_cpy(name,DRUM_KIT_NAMES[pos],sizeof name);}
            else {if(pos<0 || pos>=engine->npresets) continue;
                if(row==2)lf_sound_name(TSEL,name);else str_cpy(name,engine->presets[pos].name,sizeof name);}
            if(row==2) {cv_rect(4,0,232,38,LF_SURFACE);cv_rect(4,0,3,38,LF_COLOR[song.sel]);
                cv_text(14,4,text_w(&FONT_L,name)<=216?&FONT_L:&FONT_S,name,LF_COLOR[song.sel]);}
            else {cv_line(8,row==1?32:0,18,row==1?18:22,LF_DIM);lf_text(30,10,name,LF_DIM);}
            }
        } else if(lf_page==LF_LOOPS) {
            uint32_t id=lf_pick/4*4+row-1; const char *state="STOP";
            if(id>=10) continue;
            for(k=0;k<4;k++) if(lf_layers[k].active && lf_layers[k].id==id) state=lf_layers[k].queued?"WAIT":lf_layers[k].want?"PLAY":"FADE";
            if(id==lf_pick) cv_rect(0,0,3,40,LF_COLOR[0]);
            lf_num(6,0,id+1,LF_DIM);lf_text(30,0,lf_library[id].valid?lf_library[id].meta.name:"EMPTY",LF_TEXT);
            lf_text(30,20,!lf_library[id].valid?"IMPORT":!lf_matches(id)?"MISMATCH":state,LF_COLOR[1]);
            {static const char *const match[]={"FREE","KEY","BPM","K+B"};lf_text(100,20,match[lf[LF_FLAGS+id]&3],LF_DIM);}
            if(id==lf_pick)lf_text(141,20,"EDIT",LF_COLOR[0]);lf_num(195,20,lf[LF_LEVEL+id]*100/127,LF_DIM);
        } else if(lf_page==LF_EFFECTS) {
            if(row==1) cv_text(8,7,text_w(&FONT_L,LF_EFFECT[lf_effect])<224?&FONT_L:&FONT_S,LF_EFFECT[lf_effect],LF_COLOR[song.sel]);
            if(row==2) {lf_text(8,0,lf_effect==0?"SELECTED TRACK":lf_effect>=5?"SEND / SHARED BUS":"SHARED MASTER",LF_DIM);
                lf_text(8,20,lf_fx_amount()?"ACTIVE":"BYPASSED",lf_fx_amount()?LF_COLOR[1]:LF_DIM);}
            if(row==3 && lf_effect==1) {
                int x[5]={8,58,116,174,230},y[5]={10+lf[LF_LOW]/5,24-((int)lf[LF_BASS]-64)/4,24-((int)lf[LF_MID]-64)/4,24-((int)lf[LF_TREBLE]-64)/4,10+(127-lf[LF_HIGH])/5};
                cv_line(8,24,230,24,LF_SURFACE);for(k=0;k<4;k++)cv_line(x[k],y[k],x[k+1],y[k+1],LF_COLOR[song.sel]);
            }
            if(row==3 && lf_effect!=1 && lf_effect!=3 && lf_effect!=4) {cv_rect(8,14,224,6,LF_SURFACE);cv_rect(8,14,lf_fx_amount()*224/127,6,LF_COLOR[song.sel]);}
            if(row==3 && lf_effect==3) {lf_text(8,0,"PRESETS",LF_DIM);static const char *const mod[]={"CHORUS","VIBRATO","TREMOLO","PHASER"};lf_text(8,19,mod[lf[LF_MOD]%4],LF_TEXT);}
            if(row==3 && lf_effect==4) {lf_text(8,0,"PRESETS: PITCH",LF_DIM);static const char *const pitch[]={"OCTAVES","IN KEY: TONAL LOOP","FREE"};lf_text(8,19,pitch[lf[LF_PITCH]%3],LF_TEXT);}
            if(row==4) {lf_text(8,0,lf_effect?"EDIT: ON / OFF":"ALGO: EFFECT",LF_DIM);
                lf_text(8,19,(lf_effect==1 || lf_effect==4)?(lf_fxpage?"FX: PAGE 1":"FX: PAGE 2"):"ALGO: EFFECT",LF_DIM);}
        } else if(is_drum(TSEL) && lf_voice_page()) {
            if(row==2)lf_text(10,10,"SELECT A SYNTH PART",LF_TEXT);
            if(row==3)lf_text(10,10,"SEQ: EDIT DRUMS",LF_DIM);
        } else if(lf_voice_page()) {
            uint16_t c=LF_COLOR[song.sel];
            if(row==1) {
                const char *name=lf_page==LF_ENV?"AMPLITUDE":lf_page==LF_LFO?N_LWAVE[TSEL->p[P_LWAVE]]:lf_page==LF_ARP?N_AMODE[TSEL->p[P_AMODE]]:ENGINES[TSEL->eng_req%NENGINES]->page_title[lf_tonepage];
                cv_text(8,7,text_w(&FONT_L,name)<220?&FONT_L:&FONT_S,name,c);
            }
            if(lf_page==LF_ENV && (row==2 || row==3)) {
                int attack=15+TSEL->p[P_ATK]*50/127,decay=attack+20+TSEL->p[P_DEC]*35/127,sus=60-TSEL->p[P_SUS]*50/127,end=210-TSEL->p[P_REL]*35/127;
                cv_oy=-(int32_t)(row-2)*40;
                cv_line(8,67,attack,8,c);cv_line(attack,8,decay,sus,c);cv_line(decay,sus,end,sus,c);cv_line(end,sus,230,67,c);
                cv_line(8,69,230,69,LF_SURFACE);cv_oy=0;
            } else if(row==2 || row==3) {
                uint32_t start=(row-2)*2;
                for(k=start;k<start+2;k++) {int x=8+(int)(k-start)*118;
                    lf_text(x,0,lab[k],LF_DIM);lf_text(x,20,val[k],c);}
            }
            if(row==4)lf_text(8,12,lf_page==LF_TONE?(lf_tonepage?"EDIT: SOUND BROWSER":"EDIT: MORE CONTROLS"):lf_page==LF_ARP?"ALGO: OFF / PATTERN":lf_page==LF_LFO?"DEPTH 0 = NO MOVEMENT":"SELECT: PART",LF_DIM);
        } else if(lf_page==LF_SAVE) {
            static const char *const action[]={"SAVE SECTION","LOAD SECTION","SYSTEM / UPDATE"};
            if(row<4) lf_text(12,12,action[row-1],lf_save_action==row-1?LF_COLOR[0]:LF_DIM);
            if(row==4) lf_text(8,8,"EDIT: CONFIRM / OPEN",LF_DIM);
        }
        if(lf_cpu_guard && row==4) {cv_rect(0,0,240,40,LF_BG);lf_text(4,4,lf_cpu_guard==1?"NEW FX PAUSED: CPU":"VOICE LIMIT: CPU",LF_COLOR[3]);lf_text(4,23,"FX: RETRY / LESS LOAD",LF_DIM);}
        if(ui.msg_t && row==4) {cv_rect(0,0,240,40,LF_BG);lf_text(4,12,ui.msg,LF_TEXT);}
        for(k=0;k<240u*40u;k++) sig=(sig^cv_px[k])*16777619u;
        if(ui.force || sig!=lf_cache[row]) {lf_cache[row]=sig;cv_blit(0,row*40u);}
    }
    ui.force=0;ui_timers();return 1;
}
