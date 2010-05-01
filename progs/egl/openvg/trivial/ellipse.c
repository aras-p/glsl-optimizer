#include "eglcommon.h"

#include <VG/openvg.h>

#include <math.h>
#include <stdlib.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {0.0, 0.0, 0.0, 1.0};

VGPath path;
VGPaint paint;

static void
init(void)
{
   VGfloat clearColor[] = {1.0f, 1.0f, 1.0f, 1.0f};/* white color */
   VGfloat fillColor[] = {1.0f, 0.0f, 0.0f, 1.0f};/* red color */
   static const VGubyte segments[4] = {VG_MOVE_TO_ABS,
                                       VG_SCCWARC_TO_ABS,
                                       VG_SCCWARC_TO_ABS,
                                       VG_CLOSE_PATH};
   VGfloat data[12];
   const VGfloat cx = 0, cy=29, width=80, height=40;
   const VGfloat hw = width * 0.5f;
   const VGfloat hh = height * 0.5f;

   data[0] = cx + hw;
   data[1] = cy;
   data[2] = hw;
   data[3] = hh;
   data[4] = 0;
   data[5] = cx - hw;
   data[6] = cy;
   data[7] = hw;
   data[8] = hh;
   data[9] = 0;
   data[10] = data[0];
   data[11] = cy;

   vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
   vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);


   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
   if (path == VG_INVALID_HANDLE) {
      return;
   }
   paint = vgCreatePaint();
   if (paint == VG_INVALID_HANDLE) {
      vgDestroyPath(path);
      return;
   }

   vgAppendPathData(path, 4, segments, data);
   vgSetParameterfv(paint, VG_PAINT_COLOR, 4, fillColor);
   vgSetParameteri( paint, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
   vgSetPaint(paint, VG_FILL_PATH);
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
   vgLoadIdentity();
   vgTranslate(50, 21);
   vgDrawPath(path, VG_FILL_PATH);
   vgFlush();
}


int main(int argc, char **argv)
{
   set_window_size(100, 100);
   return run(argc, argv, init, reshape,
              draw, 0);
}
