#include "eglcommon.h"

#include <VG/openvg.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {1.0, 1.0, 1.0, 0.5};
VGfloat clearColor[] = {1.0f, 0.0f, 0.0f, 1.0f};/* red color */
VGImage parent;

VGPaint fill;

static void
init(void)
{
    VGImage child1, child2;
    VGubyte *data;
    VGuint  LUT[256];
    VGint i;

    data = (VGubyte *)malloc(sizeof(VGubyte)*window_width()*window_height());

    for (i=0;i<window_width()*window_height();i++) {
        data[i] = 0x00;
    }

    for (i=0; i<256; i++) {
        if ( i == 0 )
            LUT[0] = 0xFFFFFFFF;
        else
            LUT[i] = 0xFF00FFFF;
    }

    parent = vgCreateImage( VG_A_8, 64, 64, VG_IMAGE_QUALITY_NONANTIALIASED );

    vgImageSubData(parent, data, window_width(), VG_A_8, 0, 0,
                   window_width(), window_height());
    child1 = vgChildImage(parent, 0, 0, 32, 64);
    child2 = vgChildImage(parent, 32, 0, 32, 64);

    vgLookupSingle(child2, child1, LUT, VG_GREEN, VG_FALSE, VG_TRUE);
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

static void
draw(void)
{
   vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
   vgClear(0, 0, window_width(), window_height());
   //vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
   //vgLoadIdentity();
   //vgTranslate(10, 10);
   vgDrawImage(parent);
   vgFlush();
}


int main(int argc, char **argv)
{
   set_window_size(64, 64);
   return run(argc, argv, init, reshape,
              draw, 0);
}
