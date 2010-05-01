#include "eglcommon.h"

#include <VG/openvg.h>
#include <VG/vgu.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <X11/keysym.h>

//VGint x_pos = -10, y_pos = -10;
VGint x_pos = 0, y_pos = 4;
VGint img_width = 120, img_height = 120;

static void RectToPath(VGPath path, VGfloat x, VGfloat y, VGfloat width, VGfloat height)
{
    static const VGubyte segments[5] = {VG_MOVE_TO_ABS,
                                        VG_HLINE_TO_ABS,
                                        VG_VLINE_TO_ABS,
                                        VG_HLINE_TO_ABS,
                                        VG_CLOSE_PATH};
    VGfloat data[5];

    data[0] = x;
    data[1] = y;
    data[2] = x + width;
    data[3] = y + height;
    data[4] = x;

    vgAppendPathData(path, 5, segments, data);
}

static void
init(void)
{
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

int  key_press(unsigned key)
{
    switch(key) {
    case XK_Right:
        x_pos +=1;
        break;
    case XK_Left:
        x_pos -=1;
        break;
    case XK_Up:
        y_pos +=1;
        break;
    case XK_Down:
        y_pos -=1;
        break;
    case 'a':
        img_width  -= 5;
        img_height -= 5;
        break;
    case 's':
        img_width  += 5;
        img_height += 5;
        break;
    default:
        break;
    }
    fprintf(stderr, "Posi = %dx%d\n", x_pos, y_pos);
    fprintf(stderr, "Size = %dx%d\n", img_width, img_height);
    return VG_FALSE;
}

static void
draw(void)
{
    VGint WINDSIZEX = window_width();
    VGint WINDSIZEY = window_height();

    VGPaint fill;
    VGPath box;
    VGfloat color[4]		= {1.f, 0.f, 0.f, 1.f};
    VGfloat bgCol[4]		= {0.7f, 0.7f, 0.7f, 1.0f};
    VGfloat transCol[4]         = {0.f, 0.f, 0.f, 0.f};
    VGImage image = vgCreateImage(VG_sRGBA_8888, img_width, img_height,
                                  VG_IMAGE_QUALITY_NONANTIALIASED);

    /* Background clear */
    fill = vgCreatePaint();
    vgSetParameterfv(fill, VG_PAINT_COLOR, 4, color);
    vgSetPaint(fill, VG_FILL_PATH);

    box = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
    /* Rectangle to cover completely 16x16 pixel area. */
    RectToPath(box, 0, 0, 64, 64);

    vgSetfv(VG_CLEAR_COLOR, 4, transCol);
    vgClearImage(image, 0, 0, img_width, img_height);
    vgSetfv(VG_CLEAR_COLOR, 4, color);
    vgClearImage(image, 10, 10, 12, 12);
    //vgImageSubData(image, pukki_64x64_data, pukki_64x64_stride,
    //               VG_sRGBA_8888, 0, 0, 32, 32);
    vgSeti(VG_MASKING, VG_TRUE);
    vgLoadIdentity();

    vgSetfv(VG_CLEAR_COLOR, 4, bgCol);
    vgClear(0, 0, WINDSIZEX, WINDSIZEY);


    vgMask(image, VG_FILL_MASK, 0, 0, window_width(), window_height());
    vgMask(image, VG_SET_MASK, x_pos, y_pos, 100, 100);

    vgDrawPath(box, VG_FILL_PATH);

    //vgSeti(VG_MATRIX_MODE, VG_MATRIX_IMAGE_USER_TO_SURFACE);
    //vgTranslate(-10, -10);
    //vgDrawImage(image);


    vgDestroyPaint(fill);
    vgDestroyPath(box);
}


int main(int argc, char **argv)
{
    set_window_size(64, 64);
    return run(argc, argv, init, reshape,
               draw, key_press);
}
