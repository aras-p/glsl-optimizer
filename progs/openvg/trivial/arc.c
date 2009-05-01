#include "eglcommon.h"

#include <VG/openvg.h>
#include <math.h>

const VGfloat clear_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {1.0, 1.0, 1.0, 0.5};

VGPath vgPath;

static void ellipse(VGPath vgPath, VGfloat rx, VGfloat ry, VGfloat angle)
{
    static const VGubyte cmd[] =
    { VG_MOVE_TO_ABS, VG_SCCWARC_TO_REL, VG_SCCWARC_TO_REL, VG_CLOSE_PATH };

    VGfloat val[12];
    VGfloat c = cos(angle) * rx;
    VGfloat s = sin(angle) * rx;

    val[0] = c;
    val[1] = s;
    val[2] = rx;
    val[3] = ry;
    val[4] = angle;
    val[5] = -2.0f * c;
    val[6] = -2.0f * s;
    val[7] = rx;
    val[8] = ry;
    val[9] = angle;
    val[10] = 2.0f * c;
    val[11] = 2.0f * s;

    vgClearPath(vgPath, VG_PATH_CAPABILITY_ALL);
    vgAppendPathData(vgPath, sizeof(cmd), cmd, val);
    vgDrawPath(vgPath, VG_FILL_PATH | VG_STROKE_PATH);
}

static void
init(void)
{
    VGPaint vgPaint;

    vgSetfv(VG_CLEAR_COLOR, 4, clear_color);
    vgPath = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                          VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0,
                          VG_PATH_CAPABILITY_ALL);

    vgPaint = vgCreatePaint();
    vgSetParameteri(vgPaint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetColor(vgPaint, 0x00ff00ff);
    vgSetPaint(vgPaint, VG_FILL_PATH);

    vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);
    vgSeti(VG_BLEND_MODE, VG_BLEND_SRC_OVER);
    vgSetf(VG_STROKE_LINE_WIDTH, 2.0f);
    vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_SQUARE);
    vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_MITER);
    vgSetf(VG_STROKE_MITER_LIMIT, 4.0f);
    vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

static void
draw(void)
{
    vgClear(0, 0, window_width(), window_height());

#if 0
    vgLoadIdentity();
    vgTranslate(40.0f, 24.0f);
    vgScale(0.61804f, 0.61804f);
    vgShear(-1.0f, 0.0f);
    vgDrawPath(vgPath, VG_FILL_PATH | VG_STROKE_PATH);
#else

    /* row 1, col 1: Identity transform. */

    vgLoadIdentity();
    vgTranslate(8.0f, 8.0f);
    ellipse(vgPath, 4.0f, 4.0f, 0.0f);

    /* row 1, col 2: 10^3 horizontal squeeze. */

    vgLoadIdentity();
    vgTranslate(24.0f, 8.0f);
    vgScale(1.0e-3f, 1.0f);
    ellipse(vgPath, 4.0e3f, 4.0f, 0.0f);

    /* row 1, col 3: 10^6 horizontal squeeze. */

    vgLoadIdentity();
    vgTranslate(40.0f, 8.0f);
    vgScale(1.0e-6f, 1.0f);
    ellipse(vgPath, 4.0e6f, 4.0f, 0.0f);

    /* row 1, col 4: 10^9 horizontal squeeze. */

    vgLoadIdentity();
    vgTranslate(56.0f, 8.0f);
    vgScale(1.0e-9f, 1.0f);
    ellipse(vgPath, 4.0e9f, 4.0f, 0.0f);

    /* row 2, col 1: 10^3 vertical squeeze. */

    vgLoadIdentity();
    vgTranslate(8.0f, 24.0f);
    vgScale(1.0f, 1.0e-3f);
    ellipse(vgPath, 4.0f, 4.0e3f, 0.0f);

    /* row 2, col 2: Shear 0. */

    vgLoadIdentity();
    vgTranslate(24.0f, 24.0f);
    vgShear(0.0f, 0.0f);
    ellipse(vgPath, 4.0f, 4.0f, 0.0f);

    /* row 2, col 3: Horizontal shear -1. */

    vgLoadIdentity();
    vgTranslate(40.0f, 24.0f);
    vgScale(0.61804f, 0.61804f);
    vgShear(-1.0f, 0.0f);
    ellipse(vgPath, 10.47213f, 4.0f, 31.717f);
#endif
    vgFlush();
}


int main(int argc, char **argv)
{
    set_window_size(64, 64);
   return run(argc, argv, init, reshape,
              draw, 0);
}
