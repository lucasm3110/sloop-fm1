/* SPDX-License-Identifier: GPL-3.0-only */
/* Four fixed loop lanes. Decode muted lanes too, so bringing one in stays in phase.
 * Replacements wait for the bar, fade the old lane out, then start from its loop head. */
static const uint8_t lf_first[5]={0,3,5,8,10};
static uint8_t lf_running,lf_pending[4]={255,255,255,255};
static uint32_t lf_position;
static uint8_t lf_wait[4];
#define LF_BAR_SAMPLES 147000u /* one bar at 72 BPM, 44.1 kHz */
static void lf_scene_start(uint32_t lane,uint32_t id)
{
    lf_layer_t *v=&lf_layers[lane];
    memset(v,0,sizeof *v);v->id=(uint8_t)id;
    if(id>=10 || !lf_library[id].valid)return;
    v->active=1;v->dec.gate=1;v->next=lf_sample_next(&lf_library[id],&v->dec);
}
static void lf_scene_play(void)
{
    uint32_t j;
    lf_position=0;lf_running=1;song.playing=1;
    for(j=0;j<4;j++) {lf_pending[j]=255;lf_wait[j]=0;lf_scene_start(j,lf[j]);}
}
static void lf_scene_stop(void) {
    uint32_t j;lf_running=0;song.playing=0;
    for(j=0;j<4;j++)if(lf_pending[j]<10){lf[j]=lf_pending[j];lf_pending[j]=255;}
}
static void lf_scene_choose(uint32_t lane,uint32_t id)
{
    if(id<lf_first[lane] || id>=lf_first[lane+1] || !lf_library[id].valid)return;
    if(lf_running){lf_pending[lane]=(uint8_t)id;lf_wait[lane]=(uint8_t)(lf_position>LF_BAR_SAMPLES-1600u);}
    else {lf[lane]=(uint8_t)id;lf_pending[lane]=255;}
}
static void lf_scene_mix(int32_t *l,int32_t *r,uint32_t n)
{
    uint32_t i,j;
    /* This instrument has no keyboard synth, MIDI note engine or drum sequencer. */

    for(i=0;i<n;i++) {
        for(j=0;j<4;j++) {
            lf_layer_t *v=&lf_layers[j];const lf_entry_t *e;int32_t target,x;
            if(lf_running && !lf_position && lf_pending[j]!=255) {
                if(lf_wait[j])lf_wait[j]=0;
                else {lf[j]=lf_pending[j];lf_pending[j]=255;lf_scene_start(j,lf[j]);}
            }
            if(!v->active)continue;
            e=&lf_library[v->id];
            if(!e->valid || !usr_nz[e->slot]) {v->active=0;continue;}
            target=lf_running && (lf[8]&(1u<<j))?(int32_t)lf[4+j]*80:0;
            if(lf_pending[j]!=255 && !lf_wait[j] && lf_position>LF_BAR_SAMPLES-1600u)target=0;
            v->want=(uint8_t)(target!=0);v->gain+=clamp(target-v->gain,-16,16);
            x=v->prev+(((v->next-v->prev)*(int32_t)(v->frac>>1))>>15);
            v->frac+=e->zone.rate;
            if(v->frac>=65536u){v->frac-=65536u;v->prev=v->next;v->next=lf_sample_next(e,&v->dec);}
            x=x*v->gain>>15;l[i]+=x;r[i]+=x;
            if(!lf_running && !v->gain)v->active=0;
        }
        if(lf_running && ++lf_position>=LF_BAR_SAMPLES) {
            lf_position=0;
            /* The prepared library has exactly one bar; reset fractional resampling drift. */
            for(j=0;j<4;j++)if(lf_layers[j].active && lf_pending[j]==255) {
                int32_t gain=lf_layers[j].gain;lf_scene_start(j,lf[j]);lf_layers[j].gain=gain;
            }
        }
    }
}
