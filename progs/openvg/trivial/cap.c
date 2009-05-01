#include "eglcommon.h"

#include <VG/openvg.h>

#include <math.h>
#include <stdlib.h>
#include <stdio.h>

static void
init(void)
{

}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

const int subtest = 0;
static void
draw(void)
{
    VGPath line;
    VGPaint fillPaint;
    VGubyte lineCommands[3] = {VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS};
    VGfloat lineCoords[] =   {-2.0f,-1.0f, 0.0f,0.0f, -1.0f, -2.0f};
    VGfloat clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};/* black color */
    VGfloat fillColor[] = {1.0f, 1.0f, 1.0f, 1.0f};/* white color */
    //VGfloat testRadius = 60.0f;
    VGfloat testRadius = 10.0f;
    int WINDSIZEX = window_width();
    int WINDSIZEY = window_height();

    line = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                        1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
    fillPaint = vgCreatePaint();

    vgSetf(VG_STROKE_LINE_WIDTH, 1.0f);
    //vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_ROUND);
    vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
    vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_ROUND);
    //vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_BEVEL);

    vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_BETTER);

    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
    vgLoadIdentity();
    vgTranslate(60, 60);
    vgScale(testRadius * 2, testRadius * 2);

    vgAppendPathData(line, 3, lineCommands, lineCoords);

    vgSetfv(VG_CLEAR_COLOR, 4, clearColor);

    vgSetPaint(fillPaint, VG_STROKE_PATH);

    vgSetParameterfv(fillPaint, VG_PAINT_COLOR, 4, fillColor);
    vgSetParameteri( fillPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);

    vgClear(0, 0, WINDSIZEX, WINDSIZEY);
    vgDrawPath(line, VG_STROKE_PATH);

    vgDestroyPath(line);
    vgDestroyPaint(fillPaint);
}


int main(int argc, char **argv)
{
   set_window_size(100, 100);
   return run(argc, argv, init, reshape,
              draw, 0);
}
