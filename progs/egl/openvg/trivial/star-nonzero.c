#include "eglcommon.h"

#include <VG/openvg.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat green_color[4] = {0.0, 1.0, 0.0, 0.8};

VGPath path;
VGPaint fill;


static void
init(void)
{
   static const VGubyte cmds[6] = {VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS,
                                   VG_LINE_TO_ABS, VG_CLOSE_PATH};
   static const VGfloat coords[]   = {  0,  200,
                                        300,  200,
                                        50,   0,
                                        150, 300,
                                        250,   0};
   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1, 0, 0, 0,
                       VG_PATH_CAPABILITY_APPEND_TO);
   vgAppendPathData(path, 6, cmds, coords);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, green_color);
   vgSetPaint(fill, VG_FILL_PATH);

   vgSetfv(VG_CLEAR_COLOR, 4, white_color);
   vgSeti(VG_FILL_RULE, VG_NON_ZERO);
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
   vgDrawPath(path, VG_FILL_PATH | VG_STROKE_PATH);

   vgFlush();
}


int main(int argc, char **argv)
{
   return run(argc, argv, init, reshape,
              draw, 0);
}
