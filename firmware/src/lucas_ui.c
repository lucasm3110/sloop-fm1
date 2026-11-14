/* SPDX-License-Identifier: GPL-3.0-only */
/* Two views, four volume knobs. Sound browsing is a draft until EDIT commits it. */
enum {LF_HOME,LF_SOUND};
static uint8_t lf_page,lf_selected,lf_pick,lf_hot=255;
static uint32_t lf_hot_until,lf_cache[6];
static const char *const LF_ROLE[]={"pad","bass","melody","rhythm"};
static const uint16_t LF_COLOR[]={RGB(39,88,223),RGB(123,156,100),RGB(206,158,28),RGB(208,99,40)};
static const uint16_t LF_BG=RGB(248,244,232),LF_INK=RGB(37,38,36),LF_DIM=RGB(126,126,118),LF_TRACK=RGB(206,204,193);
static void lf_save_scene(void);
UI_SMALL static void lf_open(uint32_t page){lf_page=(uint8_t)page;ui.force=1;ui.menu=0;vis_on=0;lf_advanced=0;ui.layer=LY_PLAY;ly_lock=LY_PLAY;kb_grid=0;}
UI_SMALL static void lf_name(uint32_t id,char *b)
{
    uint32_t i;str_cpy(b,id<10 && lf_library[id].valid?lf_library[id].meta.name:"no loop",20);
    for(i=0;b[i];i++)if(b[i]>='A' && b[i]<='Z')b[i]+=32;
}
UI_SMALL static int lf_ui_input(uint32_t pressed)
{
    int32_t s;uint32_t k;
    enc_hold=0;mi_r=mi_w;lf_library_scan();
    if(pressed)ui_input_ms=fm1_ms;
    if(pressed&(1u<<panel.btn[B_HOME]))lf_open(LF_HOME);
    if(pressed&(1u<<panel.btn[B_PLAY])) {
        fm1_irq_off();if(lf_running)lf_scene_stop();else lf_scene_play();fm1_irq_on();
    }
    if(pressed&(1u<<panel.btn[B_EDIT])) {
        fm1_irq_off();
        if(lf_page==LF_SOUND) {lf_scene_choose(lf_selected,lf_pick);lf[8]|=1u<<lf_selected;lf_open(LF_HOME);}
        else lf[8]^=1u<<lf_selected;
        fm1_irq_on();
    }
    if(pressed&(1u<<panel.btn[B_SAVE])) {
        if(lf_running)ui_message("stop to save");else lf_save_scene();
    }
    if((s=panel_enc(EN_SELECT))) {
        lf_selected=(uint8_t)clamp((int32_t)lf_selected+s,0,3);lf_open(LF_HOME);
    }
    if((s=panel_enc(EN_PRESET))) {
        if(lf_page!=LF_SOUND)lf_pick=lf_pending[lf_selected]<10?lf_pending[lf_selected]:lf[lf_selected];
        lf_pick=(uint8_t)clamp((int32_t)lf_pick+s,lf_first[lf_selected],lf_first[lf_selected+1]-1);
        lf_open(LF_SOUND);
    }
    /* No hidden engine, drum, sequencing, or effect pages. */
    panel_enc(EN_ALGO);
    for(k=0;k<4;k++)if((s=panel_enc(EN_K1+k))) {
        lf[4+k]=(uint8_t)clamp(lf[4+k]+s,0,127);lf_hot=(uint8_t)k;lf_hot_until=fm1_ms+1200;
    }
    return 1;
}
UI_SMALL static void lf_text(int x,int y,const char *s,const felucca_font_t *f,uint16_t c){cv_text(x,y,f,s,c);}
UI_SMALL static void lf_disc(int x,int y,int r,uint16_t c)
{
    int dy,dx;
    for(dy=-r;dy<=r;dy++)if(y+dy+cv_oy>=0 && y+dy+cv_oy<(int)cv_h)
        for(dx=-r;dx<=r;dx++)if(dx*dx+dy*dy<=r*r)cv_pset(x+dx,y+dy,c);
}
UI_SMALL static void lf_capsule(int cx,int top,int bottom,int r,uint16_t c)
{
    lf_disc(cx,top,r,c);lf_disc(cx,bottom,r,c);cv_rect(cx-r,top,2*r+1,bottom-top,c);
}
UI_SMALL static void lf_point(uint32_t p,int cx,int top,int bottom,int r,int *x,int *y)
{
    uint32_t a=(p&63u)<<25;int sn=osc_sine(a),cs=osc_sine(a+0x40000000u);
    if(p<64){*x=cx+cs*r/32768;*y=top-sn*r/32768;}
    else if(p<128){*x=cx-r;*y=top+(bottom-top)*(int)(p-64)/64;}
    else if(p<192){*x=cx-cs*r/32768;*y=bottom+sn*r/32768;}
    else {*x=cx+r;*y=bottom-(bottom-top)*(int)(p-192)/64;}
}
UI_SMALL static void lf_ring(int cx,int top,int bottom,int r,uint32_t lane,int selected)
{
    uint32_t pos=lf_running?lf_position*256u/LF_BAR_SAMPLES:42u,k;
    uint16_t color=(lf[8]&(1u<<lane))?LF_COLOR[lane]:LF_TRACK;
    int x,y;
    if(selected){lf_capsule(cx,top,bottom,r+4,LF_INK);lf_capsule(cx,top,bottom,r+2,LF_BG);}
    lf_capsule(cx,top,bottom,r,LF_TRACK);lf_capsule(cx,top,bottom,r-3,LF_BG);
    for(k=0;k<=pos;k+=3){lf_point(k,cx,top,bottom,r-1,&x,&y);lf_disc(x,y,2,color);}
    lf_point(pos,cx,top,bottom,r-1,&x,&y);lf_disc(x,y,5,LF_BG);lf_disc(x,y,4,color);
}
UI_SMALL static int lf_ui_draw(void)
{
    uint32_t row,k;char b[24],name[20];
    if(lf_hot!=255 && (int32_t)(fm1_ms-lf_hot_until)>=0)lf_hot=255;
    for(row=0;row<6;row++) {
        uint32_t sig=2166136261u;cv_oy=0;cv_begin(240,40,LF_BG);cv_oy=-(int32_t)row*40;
        if(lf_page==LF_HOME) {
            lf_text(12,9,"lucas",&FONT_LL,LF_INK);
            if(lf_running){for(k=0;k<10;k++)cv_line(182,13+k,192-k,18,LF_INK);}
            else {cv_rect(182,13,3,12,LF_DIM);cv_rect(188,13,3,12,LF_DIM);}
            lf_text(201,9,"72",&FONT_LL,LF_INK);
            for(k=0;k<4;k++) {
                int cx=33+(int)k*58;lf_ring(cx,68,132,20,k,k==lf_selected);
                fmt_int(b,k+1);lf_text(cx-text_w(&FONT_LL,b)/2,87,b,&FONT_LL,LF_INK);
                lf_text(cx-text_w(&FONT_LS,LF_ROLE[k])/2,162,LF_ROLE[k],&FONT_LS,LF_INK);
                if(!(lf[8]&(1u<<k)))cv_line(cx-5,179,cx+5,179,LF_DIM);
            }
            if(lf_hot<4) {
                fmt_int(b,lf[4+lf_hot]*100/127);lf_text(12,193,LF_ROLE[lf_hot],&FONT_LL,LF_COLOR[lf_hot]);
                lf_text(190,193,b,&FONT_LL,LF_COLOR[lf_hot]);
            } else {lf_name(lf[lf_selected],name);lf_text(12,193,name,text_w(&FONT_LL,name)<=218?&FONT_LL:&FONT_LS,LF_INK);}
            lf_text(12,221,lf_pending[lf_selected]<10?"next bar...":"D major",&FONT_LS,LF_DIM);
        } else {
            fmt_int(b,lf_selected+1);str_cpy(b+1," / ",23);str_cpy(b+4,LF_ROLE[lf_selected],20);
            lf_text(12,10,b,&FONT_LL,LF_INK);
            lf_ring(120,57,83,13,lf_selected,0);
            for(k=0;k<3;k++) {
                int id=(int)lf_pick+(int)k-1,y=112+(int)k*30;
                if(id<lf_first[lf_selected] || id>=lf_first[lf_selected+1])continue;
                lf_name((uint32_t)id,name);
                lf_text(32,y,name,k==1?&FONT_LL:&FONT_LS,k==1?LF_INK:LF_DIM);
                if(k==1){cv_rect(13,y+3,4,20,LF_COLOR[lf_selected]);lf_disc(15,y+3,2,LF_COLOR[lf_selected]);}
            }
            lf_text(14,218,"D major / 1 bar",&FONT_LS,LF_DIM);
        }
        if(ui.msg_t){cv_rect(0,215,240,25,LF_BG);lf_name(99,name);str_cpy(b,ui.msg,sizeof b);
            for(k=0;b[k];k++)if(b[k]>='A' && b[k]<='Z')b[k]+=32;
            lf_text(12,219,b,&FONT_LS,LF_INK);}
        cv_oy=0;
        for(k=0;k<240u*40u;k++)sig=(sig^cv_px[k])*16777619u;
        if(ui.force || sig!=lf_cache[row]){lf_cache[row]=sig;cv_blit(0,row*40u);}
    }
    ui.force=0;ui_timers();return 1;
}
