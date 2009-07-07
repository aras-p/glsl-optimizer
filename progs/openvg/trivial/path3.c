#include "eglcommon.h"

#include <VG/openvg.h>
#include <VG/vgu.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

static void
init(void)
{
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}


static void
draw(void)
{
    VGint WINDSIZEX = window_width();
    VGint WINDSIZEY = window_height();
    VGPath path;
    VGPaint paint;

    VGfloat clearColor[] = {1.0f, 1.0f, 1.0f, 0.0f};/* white color */
    VGfloat fillColor[] = {1.0f, 0.0f, 0.0f, 1.0f};/* red color */

#if 1
    VGubyte commands[4] = {VG_MOVE_TO_ABS, VG_LCWARC_TO_ABS, VG_SCWARC_TO_ABS, VG_CLOSE_PATH};
#else
    VGubyte commands[4] = {VG_MOVE_TO_ABS, VG_SCCWARC_TO_ABS, VG_LCCWARC_TO_ABS,VG_CLOSE_PATH};
#endif
    VGfloat coords[] = {32.0f,   0.0f,
                        -32.0f, -32.0f, 0.0f, 64.0f, 32.0f,
                        -32.0f, -32.0f, 0.0f, 32.0f, 0.0f};


    vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
    vgClear(0, 0, WINDSIZEX, WINDSIZEY);
    vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);

    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity();
    //vgTranslate(32.0f, 32.0f);

    path = vgCreatePath( VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                         1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL );
    if ( path == VG_INVALID_HANDLE ) {
        return;
    }
    paint = vgCreatePaint();
    if ( paint == VG_INVALID_HANDLE ) {
        vgDestroyPath(path);
        return;
    }

    vgAppendPathData(path, 4, commands, coords);
    vgSetParameterfv(paint, VG_PAINT_COLOR, 4, fillColor);
    vgSetParameteri( paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetPaint(paint, VG_FILL_PATH);
    vgDrawPath(path, VG_FILL_PATH);

    vgDestroyPath(path);
    vgDestroyPaint(paint);
}


int main(int argc, char **argv)
{
    set_window_size(64, 64);
    return run(argc, argv, init, reshape,
               draw, 0);
}
