#include "eglcommon.h"

#include <VG/openvg.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {0.9, 0.1, 0.1, 0.8};

VGPath path;
VGPaint fill;


static void
init(void)
{
   static const VGubyte sqrCmds[10] = {VG_MOVE_TO_ABS,
                                       VG_LINE_TO_ABS,
                                       VG_CUBIC_TO_ABS,
                                       VG_LINE_TO_ABS,
                                       VG_CUBIC_TO_ABS,
                                       VG_LINE_TO_ABS,
                                       VG_CUBIC_TO_ABS,
                                       VG_LINE_TO_ABS,
                                       VG_CUBIC_TO_ABS,
                                       VG_CLOSE_PATH};
   static const VGfloat sqrCoords[]   = {
      45.885571, 62.857143,
      154.11442, 62.857143,
      162.1236, 62.857143, 168.57142, 70.260744, 168.57142, 79.457144,
      168.57142, 123.4,
      168.57142, 132.5964, 162.1236,  140, 154.11442, 140,
      45.885571, 140,
      37.876394, 140, 31.428572, 132.5964, 31.428572, 123.4,
      31.428572, 79.457144,
      31.428572, 70.260744, 37.876394,62.857143, 45.885571,62.857143
   };
   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1, 0, 0, 0,
                       VG_PATH_CAPABILITY_APPEND_TO);
   vgAppendPathData(path, 10, sqrCmds, sqrCoords);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, color);
   vgSetPaint(fill, VG_FILL_PATH);

   vgSetfv(VG_CLEAR_COLOR, 4, white_color);
   vgSetf(VG_STROKE_LINE_WIDTH, 6);
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
}


int main(int argc, char **argv)
{
   return run(argc, argv, init, reshape,
              draw, 0);
}
