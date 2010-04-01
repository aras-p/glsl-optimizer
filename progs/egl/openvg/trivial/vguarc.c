#include "eglcommon.h"

#include <VG/openvg.h>
#include <VG/vgu.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {0.4, 0.1, 1.0, 1.0};

VGPath path;
VGPaint paint;


static void
init(void)
{
    VGfloat clearColor[] = {0.0f, 0.0f, 0.0f, 1.0f};/* black color */
    VGfloat greenColor[] = {0.0f, 1.0f, 0.0f, 1.0f};/* green color */
    VGint arcType = VGU_ARC_OPEN;
    VGfloat x, y, w, h, startAngle, angleExtent;

    x = 150;
    y = 150;
    w = 150;
    h = 150;
#if 0
    startAngle  = -540.0f;
    angleExtent = 270.0f;
#else
    startAngle  = 270.0f;
    angleExtent = 90.0f;
#endif

    paint = vgCreatePaint();

    vgSetPaint(paint, VG_STROKE_PATH);
    vgSetParameterfv(paint, VG_PAINT_COLOR, 4, greenColor);
    vgSetParameteri( paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
    vgSetf(VG_STROKE_LINE_WIDTH, 6.0f);
    vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);
    vgSetfv(VG_CLEAR_COLOR, 4, clearColor);

    path  = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                         1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);

    vguArc(path, x, y, w, h, startAngle, angleExtent, arcType);

    vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
    vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_BEVEL);
    vgSetf(VG_STROKE_MITER_LIMIT, 4.0f);
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
   vgClear(0, 0, window_width(), window_height());
   vgDrawPath(path, VG_STROKE_PATH);

   vgFlush();
}


int main(int argc, char **argv)
{
    // set_window_size(64, 63);
   return run(argc, argv, init, reshape,
              draw, 0);
}
