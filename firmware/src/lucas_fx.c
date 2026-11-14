/* SPDX-License-Identifier: GPL-3.0-only */
/* Bounded shared master processing. Mono-compatible modulation uses delayed
 * signal, never polarity inversion; width changes wet modulation phase only. */
#define LF_RING 2048u
#define LF_GBUF 8192u
static int16_t lf_modbuf[2][LF_RING],lf_grainbuf[LF_GBUF];
static uint32_t lf_mw,lf_gw,lf_phase,lf_grain_clock,lf_random=0x4c464d31u;
static int32_t lf_smooth[LF_COUNT],lf_eqstate[2][4],lf_drive_lp[2],lf_phaser[2][4];
static int32_t lf_tonal[CTL];
static uint8_t lf_grain_ref=255;
static int32_t lf_fx_delta[2];
static uint32_t lf_guard_fade=64;
typedef struct {uint32_t pos,step,age,len;int32_t pan;} lf_grain_t;
static lf_grain_t lf_grains[4];
static uint32_t lf_rand(void) {lf_random^=lf_random<<13;lf_random^=lf_random>>17;lf_random^=lf_random<<5;return lf_random;}
static int32_t lf_delay(const int16_t *buf,uint32_t size,uint32_t pos)
{
    uint32_t k=(pos>>16)&(size-1),f=pos&65535u;
    return buf[k]+(((int32_t)(buf[(k+1)&(size-1)]-buf[k])*(int32_t)(f>>1))>>15);
}
static int32_t lf_pitch_offset(int32_t d,uint32_t root); /* seq.c: selected scale */
static void lf_master_fx(int32_t *l,int32_t *r,uint32_t n)
{
    uint32_t i,c,j;int32_t gm,amount,dl=l[n-1],dr=r[n-1];
    /* Preserve held notes before the original overload fallback. Fade the last
     * effect contribution to dry over 64 samples without paying for more DSP. */
    if(lf_cpu_guard) {
        for(i=0;i<n && lf_guard_fade;i++,lf_guard_fade--) {
            l[i]+=lf_fx_delta[0]*(int32_t)lf_guard_fade/64;
            r[i]+=lf_fx_delta[1]*(int32_t)lf_guard_fade/64;
        }
        return;
    }
    lf_guard_fade=64;
    for(i=0;i<LF_LEVEL;i++) lf_smooth[i]+=(int32_t)(lf[i]*256-lf_smooth[i])/8;
    gm=lf_smooth[LF_GMIX]>>8;amount=lf_smooth[LF_AMOUNT]>>8;
    if(!gm) {memset(lf_grains,0,sizeof lf_grains);lf_grain_clock=0;}
    /* Bypass must also release the CPU budget: no ring-buffer work while dry. */
    if(!lf[LF_EQ] && lf_smooth[LF_DRIVE]<256 && !amount && !gm) {lf_fx_delta[0]=lf_fx_delta[1]=0;return;}
    for(i=0;i<n;i++) {
        int32_t x[2]={l[i],r[i]},wet[2]={0,0};
        for(c=0;c<2;c++) {
            if(lf[LF_EQ]) {
                int32_t *s=lf_eqstate[c],bass,mid,treble;
                s[0]+=mulq15(x[c]-s[0],30+lf[LF_LOW]*20);
                if(lf[LF_LOW]) x[c]-=s[0];
                s[1]+=mulq15(x[c]-s[1],1100);s[2]+=mulq15(x[c]-s[2],8000);
                bass=s[1];mid=s[2]-s[1];treble=x[c]-s[2];
                x[c]+=((bass*(lf_smooth[LF_BASS]/256-64)+mid*(lf_smooth[LF_MID]/256-64)+treble*(lf_smooth[LF_TREBLE]/256-64))>>6);
                s[3]+=mulq15(x[c]-s[3],600+lf[LF_HIGH]*250);
                if(lf[LF_HIGH]<127) x[c]=s[3];
            }
            if(lf_smooth[LF_DRIVE]>255) {
                int32_t a=lf_smooth[LF_DRIVE]>>8,y=softclip(clamp(x[c],-100000,100000)*(128+a*3)/128);
                lf_drive_lp[c]+=mulq15(y-lf_drive_lp[c],2000+lf[LF_DTONE]*220);
                x[c]+=(lf_drive_lp[c]-x[c])*a/127;
                x[c]=x[c]*(lf_smooth[LF_OUTPUT]>>8)/100;
            }
            if(amount) {
                lf_modbuf[c][lf_mw&(LF_RING-1)]=(int16_t)clamp(x[c],-32768,32767);
                uint32_t phase=lf_phase+(c?((uint32_t)(lf_smooth[LF_WIDTH]>>8)<<24):0);
                int32_t wave=osc_sine(phase),depth=lf_smooth[LF_DEPTH]>>8,y;
                if(lf[LF_MOD]<2) {
                    int32_t delay=(500<<8)+(wave*depth>>6);
                    y=lf_delay(lf_modbuf[c],LF_RING,((lf_mw<<16)-(uint32_t)(delay<<8)));
                    if(!lf[LF_MOD]) y=(x[c]+y)/2;
                } else if(lf[LF_MOD]==2) y=mulq15(x[c],32767-((wave+32768)*depth>>8));
                else {
                    int32_t coeff=10000+(wave*depth>>8);y=x[c];
                    for(j=0;j<4;j++) {int32_t z=lf_phaser[c][j]-mulq15(y,coeff);lf_phaser[c][j]=y+mulq15(z,coeff);y=z;}
                    y=(x[c]+y)/2;
                }
                x[c]+=(y-x[c])*amount/127;
            }
        }
        lf_mw++;lf_phase+=LFO_INC[lf[LF_SPEED]&127]>>CTL_LOG2;
        if(gm) {
            uint32_t period=200u+(127u-lf[LF_DENSITY])*80u;
            if(++lf_grain_clock>=period) {
                lf_grain_clock=lf[LF_RANDOM]?(lf_rand()%(1u+lf[LF_RANDOM]*25u)):0;
                for(j=0;j<4;j++) if(!lf_grains[j].len) {
                    lf_grain_t *g=&lf_grains[j];int32_t d=0;uint32_t size=256u+lf[LF_GSIZE]*30u;
                    if(lf[LF_SCATTER] && (lf[LF_PITCH]!=1 || lf_grain_ref<12u)) {
                        d=(int32_t)(lf_rand()%25u)-12;d=d*lf[LF_SCATTER]/127;
                        if(!lf[LF_PITCH]) d=d<-5?-12:d>5?12:0;
                        if(lf[LF_PITCH]==1) d=lf_pitch_offset(d,lf_grain_ref);
                    }
                    g->step=pow2_q16(d*16);g->len=size;g->age=0;
                    /* Whole window stays behind the writer even at +12 semitones. */
                    g->pos=(lf_gw-(size+256u+lf[LF_GDELAY]*15u))<<16;
                    g->pan=((int32_t)(lf_rand()%255u)-127)*lf[LF_GWIDTH]/127;
                    break;
                }
            }
            for(j=0;j<4;j++) {lf_grain_t *g=&lf_grains[j];if(g->len) {
                int32_t env=(int32_t)(g->age<g->len/2?g->age:g->len-g->age)*32768/(int32_t)(g->len/2);
                int32_t y=mulq15(lf_delay(lf_grainbuf,LF_GBUF,g->pos),env)/4;
                wet[0]+=y*(127-g->pan)/254;wet[1]+=y*(127+g->pan)/254;
                g->pos+=g->step;if(++g->age>=g->len)g->len=0;
            }}
        }
        if(gm) {
            int32_t feed=lf[LF_PITCH]==1?(lf_grain_ref<12?lf_tonal[i]:0):(x[0]+x[1])/2;
            lf_grainbuf[lf_gw++&(LF_GBUF-1)]=(int16_t)clamp(feed+(wet[0]+wet[1])*lf[LF_FEEDBACK]/400,-24000,24000);
        }
        /* MIX is a real dry/wet crossfade; four windowed grains need makeup gain. */
        l[i]=(x[0]*(127-gm)+wet[0]*gm*4)/127;
        r[i]=(x[1]*(127-gm)+wet[1]*gm*4)/127;
    }
    lf_fx_delta[0]=l[n-1]-dl;lf_fx_delta[1]=r[n-1]-dr;
}
