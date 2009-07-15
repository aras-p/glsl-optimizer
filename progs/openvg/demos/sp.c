#include "eglcommon.h"

#include <VG/openvg.h>
#include <VG/vgu.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

#include <X11/keysym.h>

#define ELEMENTS(x) (sizeof(x)/sizeof((x)[0]))

struct object {
   VGPath path;
   VGPaint fill;
   VGPaint stroke;
   VGint draw_mode;
   VGfloat matrix[9];
   VGfloat stroke_width;
};

struct character {
   struct object objects[32];
   VGint num_objects;
};
VGfloat identity_matrix[] = {1, 0, 0, 0, 1, 0, 0, 0, 1};

struct character cartman;

static void add_object_fill(const VGubyte *segments, VGint num_segments,
                            const VGfloat *coords,
                            VGuint color)
{
   struct object object;

   object.path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                              1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
   vgAppendPathData(object.path, num_segments, segments, coords);

   object.fill = vgCreatePaint();
   vgSetColor(object.fill, color);
   memcpy(object.matrix, identity_matrix, 9 * sizeof(VGfloat));
   object.draw_mode = VG_FILL_PATH;

   cartman.objects[cartman.num_objects] = object;
   ++cartman.num_objects;
}


static void add_object_stroke(const VGubyte *segments, VGint num_segments,
                              const VGfloat *coords,
                              VGuint color, VGfloat width)
{
   struct object object;

   object.path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                              1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
   vgAppendPathData(object.path, num_segments, segments, coords);

   object.stroke = vgCreatePaint();
   vgSetColor(object.stroke, color);
   memcpy(object.matrix, identity_matrix, 9 * sizeof(VGfloat));
   object.draw_mode = VG_STROKE_PATH;
   object.stroke_width = width;

   cartman.objects[cartman.num_objects] = object;
   ++cartman.num_objects;
}


static void add_object_fillm(const VGubyte *segments, VGint num_segments,
                             const VGfloat *coords,
                             VGuint color,
                             VGfloat *matrix)
{
   struct object object;

   object.path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                              1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
   vgAppendPathData(object.path, num_segments, segments, coords);

   object.fill = vgCreatePaint();
   vgSetColor(object.fill, color);
   memcpy(object.matrix, matrix, 9 * sizeof(VGfloat));
   object.draw_mode = VG_FILL_PATH;

   cartman.objects[cartman.num_objects] = object;
   ++cartman.num_objects;
}


static void add_object_m(const VGubyte *segments, VGint num_segments,
                         const VGfloat *coords,
                         VGuint fill_color,
                         VGuint stroke_color, VGfloat stroke_width,
                         VGfloat *matrix)
{
   struct object object;

   object.path = vgCreatePath(VG_PATH_FORMAT_STANDARD, VG_PATH_DATATYPE_F,
                              1, 0, 0, 0, VG_PATH_CAPABILITY_ALL);
   vgAppendPathData(object.path, num_segments, segments, coords);
   memcpy(object.matrix, matrix, 9 * sizeof(VGfloat));

   object.fill = vgCreatePaint();
   vgSetColor(object.fill, fill_color);
   object.draw_mode = VG_FILL_PATH | VG_STROKE_PATH;

   object.stroke = vgCreatePaint();
   vgSetColor(object.stroke, stroke_color);
   object.stroke_width = stroke_width;

   cartman.objects[cartman.num_objects] = object;
   ++cartman.num_objects;
}

static void init_character()
{
   {
      const VGubyte segments[] = {VG_MOVE_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CUBIC_TO_ABS,
                                  VG_CLOSE_PATH};
      const VGfloat coords[] = {181.83267, 102.60408,
                                181.83267,102.60408, 185.53793,114.5749, 186.5355,115.00243,
                                187.53306,115.42996, 286.0073,115.00243, 286.0073,115.00243,
                                286.0073,115.00243, 292.70526,103.45914, 290.85263,101.03648,
                                289.00001,98.61381, 181.54765,102.31906, 181.83267,102.60408
      };
      VGuint color = 0x7c4e32ff;
      add_object_fill(segments, ELEMENTS(segments),
                      coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_LINE_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CLOSE_PATH
      };
      const VGfloat coords[] = {188.62208,50.604156,
                                188.62208,50.604156, 176.73127,60.479579, 170.68509,69.548844,
                                164.63892,78.618109, 175.11895,79.827344, 175.11895,79.827344,
                                176.52973,98.368952,
                                176.52973,98.368952, 189.83131,110.05823, 208.97754,110.25976,
                                228.12377,110.46131, 244.24691,111.67054, 247.06846,110.25976,
                                249.89,108.849, 258.95927,106.8336, 260.16851,105.01975,
                                261.37774,103.2059, 296.84865,106.43053, 297.05019,91.919698,
                                297.25172,77.408874, 306.11945,64.308824, 282.13628,51.611853,
                                258.15311,38.914882, 189.2267,49.999539, 188.62208,50.604156
      };

      VGuint color = 0xe30000ff;
      add_object_fill(segments, ELEMENTS(segments),
                      coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,
         VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         68.25, 78.875,
         68.25,93.296, 54.642,105, 37.875,105,
         21.108,105, 7.5,93.296, 7.5,78.875,
         7.5,64.454, 21.108,52.75, 37.875,52.75,
         54.642,52.75, 68.25,64.454, 68.25,78.875
      };

      VGuint color = 0xffe1c4ff;
      VGfloat matrix[] = {
         1.6529, 0, 0,
         0, 1.582037, 0,
         172.9649,-90.0116, 1
      };
      add_object_fillm(segments, ELEMENTS(segments),
                       coords, color, matrix);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         170.14687,71.536958,
         173.53626,68.814326, 176.70232,68.971782, 180.55009,71.679467,
         184.39785,74.387153, 199.19294,80.036105, 191.52334,86.500482,
         189.02942,88.6025, 183.97032,85.787933, 180.26507,86.928011,
         178.8737,87.356121, 174.71827,89.783259, 171.8028,87.494856,
         166.95426,83.689139, 163.51779,76.861986, 170.14687,71.536958
      };

      VGuint color = 0xfff200ff;
      add_object_fill(segments, ELEMENTS(segments),
                      coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,  VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         299.83075,66.834136,
         299.83075,66.834136, 287.85993,64.69649, 284.15467,72.962055,
         280.44942,81.227621, 280.1644,78.234916, 280.1644,79.374994,
         280.1644,80.515072, 278.16927,84.077816, 284.86722,83.792796,
         291.56518,83.507777, 291.99271,86.785501, 294.84291,86.642991,
         297.6931,86.500482, 303.536,85.645423, 303.67851,80.657582,
         303.82102,75.66974, 302.68094,65.551548, 299.83075,66.834136
      };

      VGuint color = 0xfff200ff;
      add_object_fill(segments, ELEMENTS(segments),
                      coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS
      };
      const VGfloat coords[] = {
         240.83171,75.81225,
         240.83171,75.81225, 241.54426,88.495618, 242.25681,91.488323,
         242.96936,94.481028, 240.6892,108.01945, 240.83171,110.01459,
         240.97422,112.00973, 240.97422,111.01216, 240.97422,111.01216
      };
      VGuint color = 0x000000ff;
      VGfloat swidth = 1.14007807;
      add_object_stroke(segments, ELEMENTS(segments), coords, color, swidth);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,  VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_LINE_TO_ABS,  VG_LINE_TO_ABS,  VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         83.375, 95.5,
         83.375,96.121, 83.067,96.625, 82.6875,96.625,
         82.308,96.625, 82,96.121, 82,95.5,
         82,94.879, 82.308,94.375, 82.6875,94.375,
         83.066677,94.375, 83.374492,94.878024, 83.374999,95.498494,
         82.6875,95.5,
         83.375,95.5
      };
      VGuint fill_color = 0x000000ff;
      VGuint stroke_color = 0x000000ff;
      VGfloat swidth = 0.60000002;
      VGfloat matrix1[] = {
         1.140078, 0, 0,
         0, 1.140078, 0,
         145.4927, -15.10897, 1
      };
      VGfloat matrix2[] = {
         1.140078,0, 0,
         0,1.140078, 0,
         144.2814,-27.93485, 1
      };
      VGfloat matrix3[] = {
         1.140078,0, 0,
         0,1.140078, 0,
         144.1388,-3.70819, 1
      };
      add_object_m(segments, ELEMENTS(segments), coords,
                   fill_color, stroke_color, swidth, matrix1);
      add_object_m(segments, ELEMENTS(segments), coords,
                   fill_color, stroke_color, swidth, matrix2);
      add_object_m(segments, ELEMENTS(segments), coords,
                   fill_color, stroke_color, swidth, matrix3);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_LINE_TO_ABS, VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         179.41001,115.28745,
         179.41001,115.28745, 207.48443,109.30204, 236.84144,115.14494,
         236.84144,115.14494, 274.74903,109.87208, 291.8502,115.42996,
         179.41001,115.28745
      };

      VGuint color = 0x000000ff;
      add_object_fill(segments, ELEMENTS(segments),
                      coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,  VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_LINE_TO_ABS, VG_LINE_TO_ABS,  VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         83.792156,68.157364,
         83.792156,69.669865, 82.72301,70.897403, 81.40567,70.897403,
         80.08833,70.897403, 79.019185,69.669865, 79.019185,68.157364,
         79.019185,66.644862, 80.08833,65.417325, 81.40567,65.417325,
         82.721887,65.417325, 83.790391,66.642485, 83.792153,68.153696,
         81.40567,68.157364,
         83.792156,68.157364
      };
      VGuint fill_color = 0x000000ff;
      VGuint stroke_color = 0x000000ff;
      VGfloat swidth = 0.52891117;
      VGfloat matrix1[] = {
         1.140078,0, 0,
         0,1.140078, 0,
         145.2489,-15.58714, 1
      };
      add_object_m(segments, ELEMENTS(segments), coords,
                   fill_color, stroke_color, swidth, matrix1);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS
      };
      const VGfloat coords[] = {
         232.28113,66.976646,
         232.28113,66.976646, 237.98152,70.539389, 245.39202,66.549116
      };
      VGuint color = 0x000000ff;
      VGfloat swidth = 0.60299999;
      add_object_stroke(segments, ELEMENTS(segments), coords, color, swidth);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         185.96908,30.061986,
         185.96908,30.061986, 187.76995,14.508377, 203.23909,3.7427917,
         209.95028,-0.92779696, 219.37764,-4.9841866, 232.1078,-6.00046,
         246.13578,-7.1203411, 256.92106,-2.8560739, 264.81774,1.9451947,
         280.60485,11.543934, 284.31582,25.937274, 284.08015,26.526452,
         283.7266,27.410336, 240.83461,1.9346323, 185.96908,30.061986
      };
      VGuint color = 0x8ed8f8ff;
      add_object_fill(segments, ELEMENTS(segments), coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_LINE_TO_ABS, VG_CUBIC_TO_ABS,
         VG_LINE_TO_ABS, VG_CUBIC_TO_ABS, VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         185.39542,32.061757,
         185.82295,29.211562,
         185.82295,29.211562, 234.70379,2.277219, 284.01217,25.078779,
         284.86722,27.643954,
         284.86722,27.643954, 236.69893,4.5573746, 185.39542,32.061757
      };
      VGuint color = 0xfff200ff;
      add_object_fill(segments, ELEMENTS(segments), coords, color);
   }

   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS,  VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         219.74027,-5.917093,
         220.49206,-8.44929, 225.15564,-10.904934, 230.21473,-11.189954,
         235.27383,-11.474973, 243.27521,-13.287236, 249.21385,-5.724198,
         249.89961,-4.850868, 249.28247,-4.332166, 248.62298,-3.971398,
         247.79117,-3.516361, 247.13703,-3.392737, 246.16222,-3.408047,
         243.63973,-3.447664, 242.54183,-3.850701, 242.54183,-3.850701,
         242.54183,-3.850701, 238.78367,-1.737343, 236.20014,-3.565682,
         233.88436,-5.204544, 234.27626,-4.56325, 234.27626,-4.56325,
         234.27626,-4.56325, 232.33303,-2.975658, 230.85603,-2.995643,
         228.59433,-3.025282, 227.73672,-4.501857, 227.21966,-4.93027,
         226.76318,-4.932008, 226.50948,-4.491995, 226.50948,-4.491995,
         226.50948,-4.491995, 224.53199,-2.085883, 222.51431,-2.467064,
         221.48814,-2.66093, 218.91968,-3.15318, 219.74027,-5.917093
      };
      VGuint color = 0xfff200ff;
      add_object_fill(segments, ELEMENTS(segments), coords, color);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS,  VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,  VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         178.97347,166.06432,
         178.97347,181.2154, 168.0245,193.51193, 154.53381,193.51193,
         141.04312,193.51193, 130.09416,181.2154, 130.09416,166.06432,
         130.09416,150.91323, 141.04312,138.6167, 154.53381,138.6167,
         168.0245,138.6167, 178.97347,150.91323, 178.97347,166.06432
      };
      VGuint color = 0xffffffff;
      VGfloat matrix1[] = {
         0.466614,-0.23492,  0,
         0.108683,0.436638,  0,
         134.5504,-0.901632, 1
      };
      VGfloat matrix2[] = {
         -0.466614,-0.23492, 0,
         -0.108683,0.436638, 0,
         338.4496,-0.512182, 1
      };
      add_object_fillm(segments, ELEMENTS(segments), coords, color, matrix1);
      add_object_fillm(segments, ELEMENTS(segments), coords, color, matrix2);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,
         VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS,  VG_CLOSE_PATH
      };
      const VGfloat coords[] = {
         123.82758,165.06168,
         123.82758,166.79125, 122.59232,168.19497, 121.07029,168.19497,
         119.54826,168.19497, 118.313,166.79125, 118.313,165.06168,
         118.313,163.3321, 119.54826,161.92839, 121.07029,161.92839,
         122.59232,161.92839, 123.82758,163.3321, 123.82758,165.06168
      };
      VGuint color = 0x000000ff;
      VGfloat matrix1[] = {
         0.525719,0, 0,
         0,0.479931, 0,
         178.9702,-43.3532, 1
      };
      VGfloat matrix2[] = {
         0.525719,0, 0,
         0,0.479931, 0,
         165.258,-43.46162, 1
      };
      add_object_fillm(segments, ELEMENTS(segments), coords, color, matrix1);
      add_object_fillm(segments, ELEMENTS(segments), coords, color, matrix2);
   }
   {
      const VGubyte segments[] = {
         VG_MOVE_TO_ABS, VG_CUBIC_TO_ABS, VG_CUBIC_TO_ABS
      };
      const VGfloat coords[] = {
         197.25,54.5,
         197.25,54.5, 211.75,71.5, 229.25,71.5,
         246.75,71.5, 261.74147,71.132714, 277.75,50.75
      };
      VGuint color = 0x000000ff;
      VGfloat swidth = 0.60299999;
      add_object_stroke(segments, ELEMENTS(segments), coords, color, swidth);
   }
}


static void
init(void)
{
   float clear_color[4] = {1.0, 1.0, 1.0, 1.0};
   vgSetfv(VG_CLEAR_COLOR, 4, clear_color);

   init_character();
}

/* new window size or exposure */
static void
reshape(int w, int h)
{
}

static int
key_press(unsigned key)
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
   VGint i;
   VGfloat save_matrix[9];

   vgClear(0, 0, window_width(), window_height());

   vgSeti(VG_MATRIX_MODE, VG_MATRIX_PATH_USER_TO_SURFACE);
   vgLoadIdentity();
   vgScale(2, 2);
   vgTranslate(160, 60);
   vgRotate(180);
   vgTranslate(-160, -100);
   vgGetMatrix(save_matrix);
   for (i = 0; i < cartman.num_objects; ++i) {
      struct object object = cartman.objects[i];
      if ((object.draw_mode & VG_STROKE_PATH)) {
         vgSetf(VG_STROKE_LINE_WIDTH, object.stroke_width);
         vgSetPaint(object.stroke, VG_STROKE_PATH);
      }
      if ((object.draw_mode & VG_FILL_PATH))
         vgSetPaint(object.fill, VG_FILL_PATH);
      vgMultMatrix(object.matrix);
      vgDrawPath(object.path, object.draw_mode);
      vgLoadMatrix(save_matrix);
   }

   vgFlush();
}


int main(int argc, char **argv)
{
    set_window_size(400, 400);
    return run(argc, argv, init, reshape, draw, key_press);
}
