#include "eglcommon.h"

#include <VG/openvg.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {0.4, 0.1, 1.0, 1.0};

VGPath path;
VGPaint fill;


static void
init(void)
{
   /* Absent VG_CLOSE_PATH */
   VGubyte commands[] = {VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS,
                         VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS};
   VGfloat clearColor[] = {1.0f, 1.0f, 1.0f, 1.0f};/* white color */
   VGfloat fillColor[] = {1.0f, 0.0f, 0.0f, 1.0f};/* red color */
   VGfloat coords[] = {-16.0f, -16.0f, 0.0f, -16.0f, 0.0f, 0.0f, -16.0f, 0.0f,
                       0.0f, 0.0f, 16.0f, 0.0f, 16.0f, 16.0f, 0.0f, 16.0f};

   vgSetfv(VG_CLEAR_COLOR, 4, clearColor);
   vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgTranslate(32.0f, 32.0f);

   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1.0f, 0.0f, 0, 0,
                       VG_PATH_CAPABILITY_ALL);
   if (path == VG_INVALID_HANDLE)
      return;
   fill = vgCreatePaint();
   if (fill == VG_INVALID_HANDLE) {
      vgDestroyPath(path);
      return;
   }
   vgAppendPathData(path, 8, commands, coords);
   vgSetPaint(fill, VG_FILL_PATH);
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, fillColor);
   vgSetParameteri(fill, VG_PAINT_TYPE, VG_PAINT_TYPE_COLOR);
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
   vgDrawPath(path, VG_FILL_PATH);

   vgFlush();
}


int main(int argc, char **argv)
{
   set_window_size(64, 64);
   return run(argc, argv, init, reshape,
              draw, 0);
}
