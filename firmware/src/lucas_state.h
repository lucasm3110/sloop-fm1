/* SPDX-License-Identifier: GPL-3.0-only */
/* Lucas FM extension v1. Stored in unused synth-only fields of the drum track;
 * FUN6, sample slot addresses, the loader and the update format stay unchanged. */
#ifndef LUCAS_FM
#define LUCAS_FM 0
#endif
#if LUCAS_FM
enum { LF_LOW, LF_BASS, LF_MID, LF_TREBLE, LF_HIGH, LF_EQ,
       LF_DRIVE, LF_DTONE, LF_OUTPUT, LF_MOD, LF_AMOUNT, LF_SPEED, LF_DEPTH, LF_WIDTH,
       LF_GMIX, LF_GSIZE, LF_GDELAY, LF_SCATTER, LF_DENSITY, LF_RANDOM, LF_FEEDBACK, LF_GWIDTH, LF_PITCH,
       LF_LEVEL, LF_FLAGS = LF_LEVEL + 10, LF_ID = LF_FLAGS + 10, LF_COUNT = LF_ID + 5 };
static uint8_t lf_advanced;
static uint8_t lf_note[4]={62,62,62,36};
static uint8_t lf[LF_COUNT] = {
    [LF_BASS]=64,[LF_MID]=64,[LF_TREBLE]=64,[LF_HIGH]=127, [LF_DTONE]=80, [LF_OUTPUT]=100,
    [LF_SPEED]=40, [LF_DEPTH]=40, [LF_WIDTH]=100,
    [LF_GSIZE]=50, [LF_GDELAY]=70, [LF_DENSITY]=40, [LF_GWIDTH]=100,
    [LF_LEVEL]=64,[LF_LEVEL+1]=64,[LF_LEVEL+2]=64,[LF_LEVEL+3]=64,[LF_LEVEL+4]=64,
    [LF_LEVEL+5]=64,[LF_LEVEL+6]=64,[LF_LEVEL+7]=64,[LF_LEVEL+8]=64,[LF_LEVEL+9]=64
};
static void lf_defaults(void)
{
    uint32_t i;
    memset(lf, 0, sizeof lf);
    lf[LF_BASS]=lf[LF_MID]=lf[LF_TREBLE]=64;
    lf[LF_HIGH]=127; lf[LF_DTONE]=80; lf[LF_OUTPUT]=100;
    lf[LF_SPEED]=lf[LF_DEPTH]=lf[LF_DENSITY]=40;
    lf[LF_WIDTH]=lf[LF_GWIDTH]=100; lf[LF_GSIZE]=50; lf[LF_GDELAY]=70;
    for(i=0;i<10;i++) lf[LF_LEVEL+i]=64;
}
#endif
