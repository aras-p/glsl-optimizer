#include "eglcommon.h"

#include <VG/openvg.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {0.4, 0.1, 1.0, 1.0};

VGPath path;
VGPaint fill;


static void
init(void)
{
   static const VGubyte sqrCmds[5] = {VG_MOVE_TO_ABS, VG_HLINE_TO_ABS, VG_VLINE_TO_ABS, VG_HLINE_TO_ABS, VG_CLOSE_PATH};
   static const VGfloat sqrCoords[5]   = {50.0f, 50.0f, 250.0f, 250.0f, 50.0f};
   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1, 0, 0, 0,
                       VG_PATH_CAPABILITY_APPEND_TO);
   vgAppendPathData(path, 5, sqrCmds, sqrCoords);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, color);
   vgSetPaint(fill, VG_FILL_PATH);

   vgSetfv(VG_CLEAR_COLOR, 4, white_color);
   vgSetf(VG_STROKE_LINE_WIDTH, 10);
   vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_BUTT);
   vgSeti(VG_STROKE_JOIN_STYLE, VG_JOIN_ROUND);
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
   vgSeti(VG_MATRIX_MODE, VG_MATRIX_STROKE_PAINT_TO_USER);
   vgLoadIdentity();
   vgScale(2.25, 2.25);
   vgDrawPath(path, VG_STROKE_PATH);

   vgFlush();
}


int main(int argc, char **argv)
{
   return run(argc, argv, init, reshape,
              draw, 0);
}
