/* SPDX-License-Identifier: GPL-3.0-only */
#define main hostsim_main
#include "hostsim.c"
#undef main
int main(void) {
    const int presets[3][2]={{0,13},{1,11},{0,21}};
    const char *names[3]={"warm-pad","glass-pad","sine-pad"};
    unsigned i,f;
    for(i=0;i<3;i++) {
        char path[256];snprintf(path,sizeof path,"assets/loops-cc0/prepared/%s.wav",names[i]);
        FILE *out=fopen(path,"wb");if(!out)return 1;
        host_tracks_init();host_preset_req(&trk[0],presets[i][0],presets[i][1]);
        trk[0].p[P_AMODE]=0;trk[0].p[P_TRANS]=0;
        wav_hdr(out,FS*6);fm1_in.notes=1u<<9; /* D4, declared root */
        for(f=0;f<FS*6;f+=CTL) {int32_t b[2*CTL];mix_block(b,CTL);for(unsigned j=0;j<CTL && f+j<FS*6;j++)wav_put(out,b[2*j],b[2*j+1]);}
        fm1_in.notes=0;for(f=0;f<FS*3;f+=CTL){int32_t b[2*CTL];mix_block(b,CTL);}fclose(out);
    }
    return 0;
}
