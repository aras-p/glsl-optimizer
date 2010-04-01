#include "eglcommon.h"

#include <VG/openvg.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {1.0, 1.0, 1.0, 0.5};

VGImage srcImg;
VGImage dstImg;

VGPaint fill;

VGfloat bgCol[4] = {0.906f, 0.914f, 0.761f, 1.0f};

static void
init(void)
{
    VGfloat red[4];
    VGfloat grey[4];
    VGfloat orange[4];
    VGfloat blue[4];
    VGfloat black[4];
    VGfloat white[4];
    VGshort transKernel[49] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0};

    red[0] = 0.6710f;
    red[1] = 0.1060f;
    red[2] = 0.1330f;
    red[3] = 1.0f;

    grey[0] = 0.6347f;
    grey[1] = 0.6561f;
    grey[2] = 0.6057f;
    grey[3] = 1.0f;

    orange[0] = 1.0000f;
    orange[1] = 0.8227f;
    orange[2] = 0.5057f;
    orange[3] = 1.0f;

    blue[0] = 0.0000f;
    blue[1] = 0.6908f;
    blue[2] = 0.8595f;
    blue[3] = 1.0f;

    black[0] = 0;
    black[1] = 0;
    black[2] = 0;
    black[3] = 1.0f;

    white[0] = 1;
    white[1] = 1;
    white[2] = 1;
    white[3] = 1.0f;

    vgSetfv(VG_TILE_FILL_COLOR, 4, blue);

    vgSeti(VG_FILTER_CHANNEL_MASK, 14);

    /* Setup images */
    srcImg = vgCreateImage(VG_sRGBA_8888, 32, 32,
                           VG_IMAGE_QUALITY_NONANTIALIASED);
    dstImg = vgCreateImage(VG_sRGBA_8888, 32, 32,
                           VG_IMAGE_QUALITY_NONANTIALIASED);

    vgSetfv(VG_CLEAR_COLOR, 4, black);
    vgClearImage(srcImg, 0, 0, 32, 32);
    vgSetfv(VG_CLEAR_COLOR, 4, red);
    vgClearImage(srcImg, 3, 3, 27, 27);

    vgSetfv(VG_CLEAR_COLOR, 4, orange);
    vgClearImage(dstImg, 0, 0, 32, 32);

    transKernel[8] = 1;
    vgConvolve(dstImg, srcImg, 3, 3, 3, 0, transKernel,
               1, 0, VG_TILE_FILL);
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

static void
draw(void)
{
   vgSetfv(VG_CLEAR_COLOR, 4, bgCol);
   vgClear(0, 0, window_width(), window_height());
   vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
   vgLoadIdentity();
   vgTranslate(10, 10);
   vgDrawImage(dstImg);
   vgFlush();
}


int main(int argc, char **argv)
{
   set_window_size(64, 64);
   return run(argc, argv, init, reshape,
              draw, 0);
}
