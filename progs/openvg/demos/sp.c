#include "eglcommon.h"

#include <VG/openvg.h>
#include <VG/vgu.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#include <X11/keysym.h>

struct object {
   VGPath path;
   VGPaint fill;
   VGPaint stroke;
   VGint draw_mode;
};

struct character {
   struct object objects[32];
   VGint num_objects;
};

struct character cartman;

static void init_character()
{
   struct object object;
   VGint num_objects = 0;

   {
      const VGint num_segments = 6;
      const VGubyte segments[] = {VG_MOVE_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CLOSE_PATH};
      const VGfloat coords[] = {181.83267, 102.60408,
                                181.83267,102.60408 185.53793,114.5749 186.5355,115.00243,
                                187.53306,115.42996 286.0073,115.00243 286.0073,115.00243,
                                286.0073,115.00243 292.70526,103.45914 290.85263,101.03648,
                                289.00001,98.61381 181.54765,102.31906 181.83267,102.60408
      };
      VGuint color = 0x7c4e32ff;
      object.path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                          1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
      vgAppendPathData(object.path, num_segments, segments, coords);

      object.fill = vgCreatePaint();
      vgSetColor(object.fill, color);
      character.objects[objects.num_objects] = object;
      ++objects.num_objects;
   }
   {
      
   }
}


static void
init(void)
{
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

int  key_press(unsigned key)
{
    switch(key) {
    case XK_Right:

        break;
    case XK_Left:
        break;
    case XK_Up:
        break;
    case XK_Down:
        break;
    case 'a':
        break;
    case 's':
        break;
    default:
        break;
    }
    return VG_FALSE;
}

static void
draw(void)
{
}


int main(int argc, char **argv)
{
    set_window_size(400, 400);
    return run(argc, argv, init, reshape, draw, key_press);
}
