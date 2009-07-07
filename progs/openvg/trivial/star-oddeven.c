#include "eglcommon.h"

#include <VG/openvg.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat green_color[4] = {0.0, 1.0, 0.0, 0.8};
const VGfloat black_color[4] = {0.0, 0.0, 0.0, 1.0};

VGPath path;
VGPaint fill;


static void draw_point(VGfloat x, VGfloat y)
{

   static const VGubyte cmds[] = {VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS,
                                  VG_LINE_TO_ABS, VG_CLOSE_PATH};
   const VGfloat coords[]   = {  x - 2,  y - 2,
                                 x + 2,  y - 2,
                                 x + 2,  y + 2,
                                 x - 2,  y + 2};
   VGPath path;
   VGPaint fill;

   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1, 0, 0, 0,
                       VG_PATH_CAPABILITY_ALL);
   vgAppendPathData(path, 5, cmds, coords);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, black_color);
   vgSetPaint(fill, VG_FILL_PATH);

   vgDrawPath(path, VG_FILL_PATH);

   vgDestroyPath(path);
   vgDestroyPaint(fill);
}

static void draw_marks(VGPath path)
{
    VGfloat point[2], tangent[2];
    int i = 0;

    for (i = 0; i < 1300; i += 50) {
        vgPointAlongPath(path, 0, 6, i,
                         point + 0, point + 1,
                         tangent + 0, tangent + 1);
        draw_point(point[0], point[1]);
    }
}

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
                       VG_PATH_CAPABILITY_ALL);
   vgAppendPathData(path, 6, cmds, coords);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, green_color);
   vgSetPaint(fill, VG_FILL_PATH);

   vgSetfv(VG_CLEAR_COLOR, 4, white_color);
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
    VGfloat point[2], tangent[2];
    int i = 0;

    vgClear(0, 0, window_width(), window_height());

    vgSetPaint(fill, VG_FILL_PATH);
    vgDrawPath(path, VG_FILL_PATH);

    draw_marks(path);


    vgFlush();
}


int main(int argc, char **argv)
{
   return run(argc, argv, init, reshape,
              draw, 0);
}
