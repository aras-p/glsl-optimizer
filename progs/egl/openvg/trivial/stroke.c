#include "eglcommon.h"

#include <VG/openvg.h>
#include <X11/keysym.h>
#include <stdio.h>

const VGfloat white_color[4] = {1.0, 1.0, 1.0, 1.0};
const VGfloat color[4] = {0.4, 0.1, 1.0, 1.0};

VGPath path;
VGPaint fill;

VGint cap_style = VG_CAP_BUTT;
VGint join_style = VG_JOIN_MITER;

static void
init(void)
{
#if 0
   static const VGubyte cmds[] = {VG_MOVE_TO_ABS,
                                  VG_CUBIC_TO_ABS,
   };
   static const VGfloat coords[]   = {30, 30, 264, 0, 0, 264, 234, 234
   };
#else
   static const VGubyte cmds[] = {VG_MOVE_TO_ABS,
                                  VG_LINE_TO_ABS,
                                  VG_LINE_TO_ABS
   };
   static const VGfloat coords[]   = {30, 30, 202, 30, 150, 224
   };
#endif
   VGfloat dash_pattern[2] = { 20.f, 20.f };
   path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1, 0, 0, 0,
                       VG_PATH_CAPABILITY_APPEND_TO);
   vgAppendPathData(path, 3, cmds, coords);

   fill = vgCreatePaint();
   vgSetParameterfv(fill, VG_PAINT_COLOR, 4, color);
   vgSetPaint(fill, VG_FILL_PATH);

   vgSetfv(VG_CLEAR_COLOR, 4, white_color);
   vgSetf(VG_STROKE_LINE_WIDTH, 20);
   vgSeti(VG_STROKE_CAP_STYLE, cap_style);
   vgSeti(VG_STROKE_JOIN_STYLE, join_style);
   vgSetfv(VG_STROKE_DASH_PATTERN, 2, dash_pattern);
   vgSetf(VG_STROKE_DASH_PHASE, 0.0f);
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

static int  key_press(unsigned key)
{
    switch(key) {
    case XK_c:
    case XK_C:
        ++cap_style;
        if (cap_style > VG_CAP_SQUARE)
            cap_style = VG_CAP_BUTT;
        switch(cap_style) {
        case VG_CAP_BUTT:
            fprintf(stderr, "Cap style 'butt'\n");
            break;
        case VG_CAP_ROUND:
            fprintf(stderr, "Cap style 'round'\n");
            break;
        case VG_CAP_SQUARE:
            fprintf(stderr, "Cap style 'square'\n");
            break;
        }
        vgSeti(VG_STROKE_CAP_STYLE, cap_style);
        break;
    case XK_j:
    case XK_J:
        ++join_style;
        if (join_style > VG_JOIN_BEVEL)
            join_style = VG_JOIN_MITER;
        switch(join_style) {
        case VG_JOIN_MITER:
            fprintf(stderr, "Join style 'miter'\n");
            break;
        case VG_JOIN_ROUND:
            fprintf(stderr, "Join style 'round'\n");
            break;
        case VG_JOIN_BEVEL:
            fprintf(stderr, "Join style 'bevel'\n");
            break;
        }
        vgSeti(VG_STROKE_JOIN_STYLE, join_style);
        break;
    default:
        break;
    }

    return VG_TRUE;
}

int main(int argc, char **argv)
{
   return run(argc, argv, init, reshape,
              draw, key_press);
}
