/* SPDX-License-Identifier: GPL-3.0-only */
/* Offline one-bar phrases from the retained SLOOP instruments, D major / 72. */
#define main hostsim_main
#include "hostsim.c"
#undef main
int main(int argc,char **argv) {
    const char *names[]={"warm-pad","soft-air","glass-tide","sub-bass","round-bass","soft-keys","tiny-bells","slow-pluck"};
    const char *presets[]={"WARM PAD","SINE PAD","GLASS PAD","SUB BASS","FM BASS","RHODES","MUSIC BOX","FM PLUCK"};
    const unsigned engine[]={0,0,1,0,1,1,1,1};const uint32_t bar=147000;
    for(unsigned id=0;id<8;id++) {
        char path[512];snprintf(path,sizeof path,"%s/%s.wav",argc>1?argv[1]:"build/loops04",names[id]);
        FILE *out=fopen(path,"wb");if(!out)return 1;host_tracks_init();
        unsigned p;for(p=0;p<ENGINES[engine[id]]->npresets;p++)if(!strcmp(ENGINES[engine[id]]->presets[p].name,presets[id]))break;
        if(p==ENGINES[engine[id]]->npresets)return 2;
        host_preset_req(&trk[0],engine[id],p);trk[0].p[P_AMODE]=0;trk[0].p[P_TRANS]=0;trk[0].p[P_QUANT]=0;trk[0].p[P_CHORD]=0;
        song.g[G_BPM]=72;song.octave=id==3 || id==4?-2:0;wav_hdr(out,bar);
        for(unsigned f=0;f<bar*3;f+=CTL) {
            uint32_t step=(f%bar)/(bar/4),into=f%(bar/4);const uint8_t notes[]={9,16,13,21};
            fm1_in.notes=id<3?(1u<<9|1u<<13|1u<<16):id<5?(into<bar/5?1u<<(step==3?16:9):0):(into<bar/10?1u<<notes[step%4]:0);
            int32_t b[2*CTL];mix_block(b,CTL);
            for(unsigned j=0;j<CTL && f+j<bar*3;j++)if(f+j>=bar*2)wav_put(out,b[2*j],b[2*j+1]);
        }
        fm1_in.notes=0;for(unsigned f=0;f<FS*3;f+=CTL){int32_t b[2*CTL];mix_block(b,CTL);}fclose(out);
    }
    return 0;
}
