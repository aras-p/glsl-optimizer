#include "eglcommon.h"

#include <VG/openvg.h>

#include <stdio.h>
#include <string.h>

static const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};

static VGPath path;
static VGPaint fill;

VGColorRampSpreadMode spread = VG_COLOR_RAMP_SPREAD_PAD;

static void
init(void)
{
   VGubyte commands[5] =  {VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS, VG_CLOSE_PATH};
   VGfloat coords[8] =    {0.0f,0.0f, 32.0f,0.0f, 32.0f,32.0f, 0.0f,32.0f };

   VGfloat rampStop[20] = {-0.5f,  1.0f, 1.0f, 1.0f, 1.0f,
                           0.25f, 1.0f, 0.0f, 0.0f, 1.0f,
                           0.75f, 0.0f, 0.0f, 1.0f, 1.0f,
                           1.5f,  0.0f, 0.0f, 0.0f, 0.0f};

   VGfloat defaultColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
   VGfloat linearGradient[4] = {0.0f, 0.0f, 0.0f, 32.0f};

   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                       1.0f, 0.0f, 0, 0, VG_PATH_CAPABILITY_ALL);
   if (path == VG_INVALID_HANDLE)
      return;

   fill = vgCreatePaint();
   if (fill == VG_INVALID_HANDLE) {
      vgDestroyPath(path);
      return;
   }

   vgSetfv(VG_CLEAR_COLOR, 4, defaultColor);
   vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);

   vgAppendPathData(path, 5, commands, coords);

   vgSetPaint(fill, VG_FILL_PATH);
   vgSetParameteri(fill, VG_PAINT_TYPE, VG_PAINT_TYPE_LINEAR_GRADIENT);
   vgSetParameteri(fill, VG_PAINT_COLOR_RAMP_SPREAD_MODE,
                   VG_COLOR_RAMP_SPREAD_REPEAT);
   vgSetParameterfv(fill, VG_PAINT_LINEAR_GRADIENT, 4, linearGradient);
   vgSetParameterfv(fill, VG_PAINT_COLOR_RAMP_STOPS, 20, rampStop);
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

   vgDrawPath(path, VG_FILL_PATH);

   vgFlush();
}


int main(int argc, char **argv)
{
   if (argc > 1) {
      const char *arg = argv[1];
      if (!strcmp("-pad", arg))
         spread = VG_COLOR_RAMP_SPREAD_PAD;
      else if (!strcmp("-repeat", arg))
         spread = VG_COLOR_RAMP_SPREAD_REPEAT;
      else if (!strcmp("-reflect", arg))
         spread = VG_COLOR_RAMP_SPREAD_REFLECT;
   }

   switch(spread) {
   case VG_COLOR_RAMP_SPREAD_PAD:
      printf("Using spread mode: pad\n");
      break;
   case VG_COLOR_RAMP_SPREAD_REPEAT:
      printf("Using spread mode: repeat\n");
      break;
   case VG_COLOR_RAMP_SPREAD_REFLECT:
      printf("Using spread mode: reflect\n");
   }

   set_window_size(200, 200);

   return run(argc, argv, init, reshape,
              draw, 0);
}
