/* SPDX-License-Identifier: GPL-3.0-only */
/* Four independent ADPCM streams, ten imported library entries. No synth voice
 * stealing, no resampling to claim tempo/key matching: mismatched packs wait. */
#define LF_PACK_MAGIC 0x314C464Cu
#define LF_LIBRARY 10u
#define LF_LAYERS 4u
typedef struct { char name[12]; uint8_t kind, root, flags, id; uint16_t bpm, bars; uint32_t reserved; } lf_meta_t;
_Static_assert(sizeof(lf_meta_t)==24u,"loop pack v1 metadata");
typedef struct { smp_zone_t zone; lf_meta_t meta; uint8_t slot, valid; } lf_entry_t;
static lf_entry_t lf_library[LF_LIBRARY];
typedef struct { voice_t dec; uint32_t frac; int32_t prev, next, gain; uint8_t id, active, want, queued; } lf_layer_t;
static lf_layer_t lf_layers[LF_LAYERS];
static uint32_t lf_scan_gen = 0xFFFFFFFFu, lf_last_bar = 0xFFFFFFFFu;
static uint8_t lf_loop_page;
static uint32_t lf_bar(void); /* seq.c owns the transport clock */
static int lf_matches(uint32_t id)
{
    const lf_meta_t *m=&lf_library[id].meta;
    uint32_t f=lf[LF_FLAGS+id];
    return (!(f&1u) || (m->root<12u && m->root==(uint32_t)trk[0].p[P_ROOT])) &&
           (!(f&2u) || (m->bpm && m->bpm==(uint32_t)song.g[G_BPM]));
}
static void lf_loops_stop(void)
{
    uint32_t i;
    for(i=0;i<LF_LAYERS;i++) lf_layers[i].want=lf_layers[i].queued=0;
}
static void lf_library_scan(void)
{
    uint32_t s,j,fingerprint=0,stored=0;
    for(j=0;j<5;j++) stored|=(uint32_t)lf[LF_ID+j]<<(j*7);
    if(lf_scan_gen==smp_user_gen) return;
    fm1_irq_off();
    lf_scan_gen=smp_user_gen;
    memset(lf_layers,0,sizeof lf_layers);
    memset(lf_library,0,sizeof lf_library);
    for(s=0;s<SMP_USER_SLOTS;s++) {
        const smp_user_hdr_t *h=(const smp_user_hdr_t*)smp_user_xip(s);
        if(!usr_nz[s] || h->rsv2[0]!=LF_PACK_MAGIC || h->data_len<128u || h->nz>4u) continue;
        fingerprint^=h->crc+(s+1u)*0x9E3779B9u;
        for(j=0;j<h->nz;j++) {
            const lf_meta_t *m=(const lf_meta_t*)(smp_user_xip(s)+512u+j*24u);
            uint32_t id=m->id,k;
            if(id>=LF_LIBRARY || m->kind>2u || m->flags>3u || (m->root>11u && m->root!=255u) ||
               m->bpm>300u || m->bars>16u || h->zone[j].off<128u || !h->zone[j].looped ||
               h->zone[j].rate>65536u || !smp_zone_ok(&h->zone[j],h->data_len)) continue;
            lf_library[id].zone=h->zone[j]; lf_library[id].meta=*m;
            for(k=0;k<11u;k++) if((uint8_t)lf_library[id].meta.name[k]<32u || (uint8_t)lf_library[id].meta.name[k]>126u) lf_library[id].meta.name[k]=0;
            lf_library[id].meta.name[11]=0;
            lf_library[id].slot=(uint8_t)s; lf_library[id].valid=1;
        }
    }
    if(fingerprint!=stored) {
        for(j=0;j<10;j++)lf[LF_FLAGS+j]=lf_library[j].valid?lf_library[j].meta.flags:0;
        for(j=0;j<5;j++)lf[LF_ID+j]=(uint8_t)((fingerprint>>(j*7))&127u);
    }
    fm1_irq_on();
}
static int32_t lf_sample_next(const lf_entry_t *e,voice_t *v)
{
    const uint8_t *data=smp_user_xip(e->slot)+512u;
    return sample_decode(&e->zone,v,1,data[e->zone.off+(v->ph[0]>>1)]);
}
/* 0 success, 1 unavailable, 2 pack key/tempo mismatch, 3 four layers busy. */
static int lf_loop_toggle(uint32_t id)
{
    uint32_t i;
    if(id>=LF_LIBRARY || !lf_library[id].valid) return 1;
    for(i=0;i<LF_LAYERS;i++) if(lf_layers[i].active && lf_layers[i].id==id) {
        lf_layer_t *v=&lf_layers[i];
        if(v->queued) { v->queued=0; v->want=!v->want; }
        else if(lf_library[id].meta.kind==2u && song.playing) { v->queued=1; v->want=!v->want; }
        else v->want=!v->want;
        return 0;
    }
    if(!lf_matches(id)) return 2;
    for(i=0;i<LF_LAYERS;i++) if(!lf_layers[i].active) {
        lf_layer_t *v=&lf_layers[i];
        memset(v,0,sizeof *v); v->id=(uint8_t)id; v->active=v->want=1;
        v->queued=(uint8_t)(lf_library[id].meta.kind==2u && song.playing);
        v->dec.gate=1;
        v->next=lf_sample_next(&lf_library[id],&v->dec);
        return 0;
    }
    return 3;
}
static void lf_loops_mix(int32_t *l,int32_t *r,uint32_t n)
{
    uint32_t i,j,bar=lf_bar(),edge=bar!=lf_last_bar;
    lf_last_bar=bar;
    memset(lf_tonal,0,sizeof lf_tonal);lf_grain_ref=255;
    {uint32_t count=0;for(j=0;j<LF_LAYERS;j++) if(lf_layers[j].active && lf_layers[j].want) {
        const lf_meta_t *m=&lf_library[lf_layers[j].id].meta;
        if(m->kind==1 && m->root<12) {lf_grain_ref=m->root;count++;}
    }if(count!=1)lf_grain_ref=255;}
    for(j=0;j<LF_LAYERS;j++) {
        lf_layer_t *v=&lf_layers[j];
        const lf_entry_t *e;
        int32_t target;
        if(!v->active) continue;
        e=&lf_library[v->id];
        if(!e->valid || !usr_nz[e->slot]) { memset(v,0,sizeof *v); continue; }
        if(!lf_matches(v->id)) v->want=v->queued=0;
        if(v->queued && (edge || !song.playing)) v->queued=0;
        target=(v->queued ? !v->want : v->want) ? (int32_t)lf[LF_LEVEL+v->id]*64 : 0;
        for(i=0;i<n;i++) {
            int32_t x;
            v->gain += clamp(target-v->gain,-8,8);
            if(v->queued && v->want && !v->gain) continue;
            x=v->prev+(((v->next-v->prev)*(int32_t)(v->frac>>1))>>15);
            v->frac+=e->zone.rate;
            if(v->frac>=65536u) { v->frac-=65536u; v->prev=v->next; v->next=lf_sample_next(e,&v->dec); }
            x=(x*v->gain)>>15;
            l[i]+=x; r[i]+=x;
            if(e->meta.kind==1 && e->meta.root==lf_grain_ref) lf_tonal[i]+=x;
        }
        if(!v->want && !v->queued && !v->gain) v->active=0;
    }
}
