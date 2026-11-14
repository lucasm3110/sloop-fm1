/* SPDX-License-Identifier: GPL-3.0-only */
#define LUCAS_FM 1
#define UI_TEST_MAIN upstream_ui_test_main
#include "ui_pages_test.c"
static uint32_t lf_dirty(void) {return 1;}
int main(void) {
    host_tracks_init();panel_init();settings_init();layers_init();
    for(unsigned p=0;p<=LF_SAVE;p++) {lf_open(p);lf_ui_draw();}
    return 0;
}
