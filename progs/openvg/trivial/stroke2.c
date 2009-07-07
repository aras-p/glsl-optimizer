#include "eglcommon.h"

#include <VG/openvg.h>
#include <X11/keysym.h>
#include <stdio.h>

VGPaint stroke;
VGPath target;
VGPath lines;

VGfloat xform[9];
VGfloat color1[4];
VGfloat color2[4];
VGfloat bgCol[4];

static void
init(void)
{
    VGubyte lineCmds[6];
    VGfloat lineCoords[8];
    VGfloat arcCoords[5];
    VGubyte sccCmd[1];
    VGubyte scCmd[1];
    VGubyte lccCmd[1];
    VGubyte lcCmd[1];
    VGubyte moveCmd[1];
    VGfloat moveCoords[2];
    VGint i;

    bgCol[0] = 1.0f;
    bgCol[1] = 1.0f;
    bgCol[2] = 1.0f;
    bgCol[3] = 1.0f;

    vgSetfv(VG_CLEAR_COLOR, 4, bgCol);
    vgSeti(VG_RENDERING_QUALITY, VG_RENDERING_QUALITY_NONANTIALIASED);

    stroke = vgCreatePaint();
    /* Red */
    color1[0] = 1.0f;
    color1[1] = 0.0f;
    color1[2] = 0.0f;
    color1[3] = 1.0f;

    /* Orange */
    color2[0] = 1.0000f;
    color2[1] = 1.0f;
    color2[2] = 0.0f;
    color2[3] = 1.0f;
    vgSetPaint(stroke, VG_STROKE_PATH);

    vgSeti(VG_STROKE_CAP_STYLE, VG_CAP_SQUARE);

    {
        VGfloat temp[9] = {0, 0, 0, 0, 0, 0, 0, 0, 0};
        for (i = 0; i < 9; i++)
        {
            xform[i] = temp[i];
        }
    }
    vgGetMatrix(xform);

    target = vgCreatePath(VG_PATH_FORMAT_STANDARD,
                          VG_PATH_DATATYPE_F, 1, 0, 0, 0, VG_PATH_CAPABILITY_TRANSFORM_TO);

#if 0
    /* Line path */
    {
        VGubyte temp[6] = {VG_MOVE_TO_ABS, VG_VLINE_TO_REL,
                           VG_MOVE_TO_ABS, VG_VLINE_TO_REL,
                           VG_HLINE_TO_REL, VG_VLINE_TO_REL};
        for (i = 0; i < 6; i++)
        {
            lineCmds[i] = temp[i];
        }
    }
    {
        VGfloat temp[8] = {0.5f, 0.8f, -0.6f, 0.28f, 0.6f, -0.4f, 0.44f, 0.4f};
        for (i = 0; i < 8; i++)
        {
            lineCoords[i] = temp[i] * window_width();
        }
    }
#else
    {
        VGfloat temp[5] = {0.35f, 0.15f, 29, 0.3f, 0.4f};
        for (i = 0; i < 5; i++)
        {
            arcCoords[i] = temp[i] * window_width();
        }
        arcCoords[2] = 29;
    }

    {
        VGubyte temp[1] = {VG_SCCWARC_TO_ABS};
        for (i = 0; i < 1; i++)
        {
            sccCmd[i] = temp[i];
        }
    }
    {
        VGubyte temp[1] = {VG_SCWARC_TO_ABS};
        for (i = 0; i < 1; i++)
        {
            scCmd[i] = temp[i];
        }
    }
    {
        VGubyte temp[1] = {VG_LCCWARC_TO_ABS};
        for (i = 0; i < 1; i++)
        {
            lccCmd[i] = temp[i];
        }
    }
    {
        VGubyte temp[1] = {VG_LCWARC_TO_ABS};
        for (i = 0; i < 1; i++)
        {
            lcCmd[i] = temp[i];
        }
    }

    {
        VGubyte temp[1] = {VG_MOVE_TO_ABS};
        for (i = 0; i < 1; i++)
        {
            moveCmd[i] = temp[i];
        }
    }
    {
        VGfloat temp[2] = {0.7f, 0.6f};
        for (i = 0; i < 2; i++)
        {
            moveCoords[i] = temp[i] * window_width();
        }
    }
#endif

    lines = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F, 1,
                         0, 0, 0,
                         VG_PATH_CAPABILITY_APPEND_TO|
                         VG_PATH_CAPABILITY_TRANSFORM_FROM);
#if 0
    vgAppendPathData(lines, 6, lineCmds, lineCoords);
#else
    vgAppendPathData(lines, 1, moveCmd, moveCoords);
    vgAppendPathData(lines, 1, sccCmd, arcCoords);
    vgAppendPathData(lines, 1, moveCmd, moveCoords);
    vgAppendPathData(lines, 1, scCmd, arcCoords);
    vgAppendPathData(lines, 1, moveCmd, moveCoords);
    vgAppendPathData(lines, 1, lccCmd, arcCoords);
    vgAppendPathData(lines, 1, moveCmd, moveCoords);
    vgAppendPathData(lines, 1, lcCmd, arcCoords);
#endif

    vgLoadIdentity();
    vgTranslate(0.25f * window_width(), 0.25f * window_height());
    vgRotate(30);
    vgTranslate(-0.25f * window_width(), -0.25f * window_height());
    vgTransformPath(target, lines);}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

static void
draw(void)
{
    vgClear(0, 0, window_width(), window_height());
    vgLoadMatrix(xform);
    vgLoadIdentity();
    vgTranslate(0.25f * window_width(), 0.25f * window_height());
    vgRotate(30);
    vgTranslate(-0.25f * window_width(), -0.25f * window_height());
    vgSetf(VG_STROKE_LINE_WIDTH, 7);
    vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, color1);
    vgDrawPath(lines, VG_STROKE_PATH);

    vgLoadMatrix(xform);
    vgSetParameterfv(stroke, VG_PAINT_COLOR, 4, color2);
    vgSetf(VG_STROKE_LINE_WIDTH, 3);
    vgDrawPath(target, VG_STROKE_PATH);
}

static int  key_press(unsigned key)
{
    switch(key) {
    case XK_c:
    case XK_C:
        break;
    case XK_j:
    case XK_J:
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
