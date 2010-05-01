#include "eglcommon.h"

#include <VG/openvg.h>
#include <stdio.h>

float red_color[4] = {1.0, 0.0, 0.0, 1.0};
float blue_color[4] = {0.0, 0.0, 1.0, 1.0};

static void
init(void)
{
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
   vgLoadIdentity();
}

static void
draw(void)
{
    VGint scissor[4] = {100, 100, 25, 25};
    vgSetfv(VG_CLEAR_COLOR, 4, red_color);
    vgClear(0, 0, window_width(), window_height());

    vgSetfv(VG_CLEAR_COLOR, 4, blue_color);
    vgClear(50, 50, 50, 50);

    //vgSetiv(VG_SCISSOR_RECTS, 4, scissor);
    //vgSeti(VG_SCISSORING, VG_TRUE);
    vgCopyPixels(100, 100, 50, 50, 50, 50);
    vgClear(150, 150, 50, 50);
}


int main(int argc, char **argv)
{
   return run(argc, argv, init, reshape,
              draw, 0);
}
