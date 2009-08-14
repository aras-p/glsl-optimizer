/*
 * Exercise EGL API functions
 */

#define EGL_EGLEXT_PROTOTYPES

#include <EGL/egl.h>
#include <EGL/eglext.h>
#include <GL/gl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>



#define PIXEL_CENTER(x) ((long)(x) + 0.5)

#define GAP 10
#define ROWS 3
#define COLS 4

#define OPENGL_WIDTH 48
#define OPENGL_HEIGHT 13


GLenum rgb, doubleBuffer, windType;
GLint windW, windH;

GLenum mode1, mode2;
GLint boxW, boxH;
GLubyte OpenGL_bits[] = {
   0x00, 0x03, 0x00, 0x00, 0x00, 0x00, 
   0x7f, 0xfb, 0xff, 0xff, 0xff, 0x01,
   0x7f, 0xfb, 0xff, 0xff, 0xff, 0x01, 
   0x00, 0x03, 0x00, 0x00, 0x00, 0x00,
   0x3e, 0x8f, 0xb7, 0xf9, 0xfc, 0x01, 
   0x63, 0xdb, 0xb0, 0x8d, 0x0d, 0x00,
   0x63, 0xdb, 0xb7, 0x8d, 0x0d, 0x00, 
   0x63, 0xdb, 0xb6, 0x8d, 0x0d, 0x00,
   0x63, 0x8f, 0xf3, 0xcc, 0x0d, 0x00, 
   0x63, 0x00, 0x00, 0x0c, 0x4c, 0x0a,
   0x63, 0x00, 0x00, 0x0c, 0x4c, 0x0e, 
   0x63, 0x00, 0x00, 0x8c, 0xed, 0x0e,
   0x3e, 0x00, 0x00, 0xf8, 0x0c, 0x00, 
};


static void Init(void)
{

    mode1 = GL_TRUE;
    mode2 = GL_TRUE;
}

static void Reshape(int width, int height)
{

    windW = (GLint)width;
    windH = (GLint)height;
}

#if 0
static void RotateColorMask(void)
{
    static GLint rotation = 0;
    
    rotation = (rotation + 1) & 0x3;
    switch (rotation) {
      case 0:
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glIndexMask( 0xff );
	break;
      case 1:
	glColorMask(GL_FALSE, GL_TRUE, GL_TRUE, GL_TRUE);
	glIndexMask(0xFE);
	break;
      case 2:
	glColorMask(GL_TRUE, GL_FALSE, GL_TRUE, GL_TRUE);
	glIndexMask(0xFD);
	break;
      case 3:
	glColorMask(GL_TRUE, GL_TRUE, GL_FALSE, GL_TRUE);
	glIndexMask(0xFB);
	break;
    }
}
#endif

static void Viewport(GLint row, GLint column)
{
    GLint x, y;

    boxW = (windW - (COLS + 1) * GAP) / COLS;
    boxH = (windH - (ROWS + 1) * GAP) / ROWS;

    x = GAP + column * (boxW + GAP);
    y = GAP + row * (boxH + GAP);

    glViewport(x, y, boxW, boxH);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(-boxW/2, boxW/2, -boxH/2, boxH/2, 0.0, 1.0);
    glMatrixMode(GL_MODELVIEW);

    glEnable(GL_SCISSOR_TEST);
    glScissor(x, y, boxW, boxH);
}

enum {
    COLOR_BLACK = 0,
    COLOR_RED,
    COLOR_GREEN,
    COLOR_YELLOW,
    COLOR_BLUE,
    COLOR_MAGENTA,
    COLOR_CYAN,
    COLOR_WHITE
};

static float RGBMap[9][3] = {
    {0, 0, 0},
    {1, 0, 0},
    {0, 1, 0},
    {1, 1, 0},
    {0, 0, 1},
    {1, 0, 1},
    {0, 1, 1},
    {1, 1, 1},
    {0.5, 0.5, 0.5}
};

static void SetColor(int c)
{
     glColor3fv(RGBMap[c]);
}

static void Point(void)
{
    GLint i;

    glBegin(GL_POINTS);
	SetColor(COLOR_WHITE);
	glVertex2i(0, 0);
	for (i = 1; i < 8; i++) {
	    GLint j = i * 2;
	    SetColor(COLOR_BLACK+i);
	    glVertex2i(-j, -j);
	    glVertex2i(-j, 0);
	    glVertex2i(-j, j);
	    glVertex2i(0, j);
	    glVertex2i(j, j);
	    glVertex2i(j, 0);
	    glVertex2i(j, -j);
	    glVertex2i(0, -j);
	}
    glEnd();
}

static void Lines(void)
{
    GLint i;

    glPushMatrix();

    glTranslatef(-12, 0, 0);
    for (i = 1; i < 8; i++) {
	SetColor(COLOR_BLACK+i);
	glBegin(GL_LINES);
	    glVertex2i(-boxW/4, -boxH/4);
	    glVertex2i(boxW/4, boxH/4);
	glEnd();
	glTranslatef(4, 0, 0);
    }

    glPopMatrix();

    glBegin(GL_LINES);
	glVertex2i(0, 0);
    glEnd();
}

static void LineStrip(void)
{

    glBegin(GL_LINE_STRIP);
	SetColor(COLOR_RED);
	glVertex2f(PIXEL_CENTER(-boxW/4), PIXEL_CENTER(-boxH/4));
	SetColor(COLOR_GREEN);
	glVertex2f(PIXEL_CENTER(-boxW/4), PIXEL_CENTER(boxH/4));
	SetColor(COLOR_BLUE);
	glVertex2f(PIXEL_CENTER(boxW/4), PIXEL_CENTER(boxH/4));
	SetColor(COLOR_WHITE);
	glVertex2f(PIXEL_CENTER(boxW/4), PIXEL_CENTER(-boxH/4));
    glEnd();

    glBegin(GL_LINE_STRIP);
	glVertex2i(0, 0);
    glEnd();
}

static void LineLoop(void)
{

    glBegin(GL_LINE_LOOP);
	SetColor(COLOR_RED);
	glVertex2f(PIXEL_CENTER(-boxW/4), PIXEL_CENTER(-boxH/4));
	SetColor(COLOR_GREEN);
	glVertex2f(PIXEL_CENTER(-boxW/4), PIXEL_CENTER(boxH/4));
	SetColor(COLOR_BLUE);
	glVertex2f(PIXEL_CENTER(boxW/4), PIXEL_CENTER(boxH/4));
	SetColor(COLOR_WHITE);
	glVertex2f(PIXEL_CENTER(boxW/4), PIXEL_CENTER(-boxH/4));
    glEnd();

    glEnable(GL_LOGIC_OP);
    glLogicOp(GL_XOR);

    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE);

    SetColor(COLOR_MAGENTA);
    glBegin(GL_LINE_LOOP);
	glVertex2f(PIXEL_CENTER(-boxW/8), PIXEL_CENTER(-boxH/8));
	glVertex2f(PIXEL_CENTER(-boxW/8), PIXEL_CENTER(boxH/8));
    glEnd();
    glBegin(GL_LINE_LOOP);
	glVertex2f(PIXEL_CENTER(-boxW/8), PIXEL_CENTER(boxH/8+5));
	glVertex2f(PIXEL_CENTER(boxW/8), PIXEL_CENTER(boxH/8+5));
    glEnd();
    glDisable(GL_LOGIC_OP);
    glDisable(GL_BLEND);

    SetColor(COLOR_GREEN);
    glBegin(GL_POINTS);
	glVertex2i(0, 0);
    glEnd();

    glBegin(GL_LINE_LOOP);
	glVertex2i(0, 0);
    glEnd();
}

static void Bitmap(void)
{

    glBegin(GL_LINES);
	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/2, 0);
	glVertex2i(boxW/2, 0);
	glVertex2i(0, -boxH/2);
	glVertex2i(0, boxH/2);
	SetColor(COLOR_RED);
	glVertex2i(0, -3);
	glVertex2i(0, -3+OPENGL_HEIGHT);
	SetColor(COLOR_BLUE);
	glVertex2i(0, -3);
	glVertex2i(OPENGL_WIDTH, -3);
    glEnd();

    SetColor(COLOR_GREEN);

    glPixelStorei(GL_UNPACK_LSB_FIRST, GL_TRUE);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    glRasterPos2i(0, 0);
    glBitmap(OPENGL_WIDTH, OPENGL_HEIGHT, 0, 3, 0.0, 0.0, OpenGL_bits);
}

static void Triangles(void)
{

    glBegin(GL_TRIANGLES);
	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/4, -boxH/4);
	SetColor(COLOR_RED);
	glVertex2i(-boxW/8, -boxH/16);
	SetColor(COLOR_BLUE);
	glVertex2i(boxW/8, -boxH/16);

	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/4, boxH/4);
	SetColor(COLOR_RED);
	glVertex2i(-boxW/8, boxH/16);
	SetColor(COLOR_BLUE);
	glVertex2i(boxW/8, boxH/16);
    glEnd();

    glBegin(GL_TRIANGLES);
	glVertex2i(0, 0);
	glVertex2i(-100, 100);
    glEnd();
}

static void TriangleStrip(void)
{

    glBegin(GL_TRIANGLE_STRIP);
	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/4, -boxH/4);
	SetColor(COLOR_RED);
	glVertex2i(-boxW/4, boxH/4);
	SetColor(COLOR_BLUE);
	glVertex2i(0, -boxH/4);
	SetColor(COLOR_WHITE);
	glVertex2i(0, boxH/4);
	SetColor(COLOR_CYAN);
	glVertex2i(boxW/4, -boxH/4);
	SetColor(COLOR_YELLOW);
	glVertex2i(boxW/4, boxH/4);
    glEnd();

    glBegin(GL_TRIANGLE_STRIP);
	glVertex2i(0, 0);
	glVertex2i(-100, 100);
    glEnd();
}

static void TriangleFan(void)
{
    GLint vx[8][2];
    GLint x0, y0, x1, y1, x2, y2, x3, y3;
    GLint i;

    y0 = -boxH/4;
    y1 = y0 + boxH/2/3;
    y2 = y1 + boxH/2/3;
    y3 = boxH/4;
    x0 = -boxW/4;
    x1 = x0 + boxW/2/3;
    x2 = x1 + boxW/2/3;
    x3 = boxW/4;

    vx[0][0] = x0; vx[0][1] = y1;
    vx[1][0] = x0; vx[1][1] = y2;
    vx[2][0] = x1; vx[2][1] = y3;
    vx[3][0] = x2; vx[3][1] = y3;
    vx[4][0] = x3; vx[4][1] = y2;
    vx[5][0] = x3; vx[5][1] = y1;
    vx[6][0] = x2; vx[6][1] = y0;
    vx[7][0] = x1; vx[7][1] = y0;

    glBegin(GL_TRIANGLE_FAN);
	SetColor(COLOR_WHITE);
	glVertex2i(0, 0);
	for (i = 0; i < 8; i++) {
	    SetColor(COLOR_WHITE-i);
	    glVertex2iv(vx[i]);
	}
    glEnd();

    glBegin(GL_TRIANGLE_FAN);
	glVertex2i(0, 0);
	glVertex2i(-100, 100);
    glEnd();
}

static void Rect(void)
{

    SetColor(COLOR_GREEN);
    glRecti(-boxW/4, -boxH/4, boxW/4, boxH/4);
}

static void PolygonFunc(void)
{
    GLint vx[8][2];
    GLint x0, y0, x1, y1, x2, y2, x3, y3;
    GLint i;

    y0 = -boxH/4;
    y1 = y0 + boxH/2/3;
    y2 = y1 + boxH/2/3;
    y3 = boxH/4;
    x0 = -boxW/4;
    x1 = x0 + boxW/2/3;
    x2 = x1 + boxW/2/3;
    x3 = boxW/4;

    vx[0][0] = x0; vx[0][1] = y1;
    vx[1][0] = x0; vx[1][1] = y2;
    vx[2][0] = x1; vx[2][1] = y3;
    vx[3][0] = x2; vx[3][1] = y3;
    vx[4][0] = x3; vx[4][1] = y2;
    vx[5][0] = x3; vx[5][1] = y1;
    vx[6][0] = x2; vx[6][1] = y0;
    vx[7][0] = x1; vx[7][1] = y0;

    glBegin(GL_POLYGON);
	for (i = 0; i < 8; i++) {
	    SetColor(COLOR_WHITE-i);
	    glVertex2iv(vx[i]);
	}
    glEnd();

    glBegin(GL_POLYGON);
	glVertex2i(0, 0);
	glVertex2i(100, 100);
    glEnd();
}

static void Quads(void)
{

    glBegin(GL_QUADS);
	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/4, -boxH/4);
	SetColor(COLOR_RED);
	glVertex2i(-boxW/8, -boxH/16);
	SetColor(COLOR_BLUE);
	glVertex2i(boxW/8, -boxH/16);
	SetColor(COLOR_WHITE);
	glVertex2i(boxW/4, -boxH/4);

	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/4, boxH/4);
	SetColor(COLOR_RED);
	glVertex2i(-boxW/8, boxH/16);
	SetColor(COLOR_BLUE);
	glVertex2i(boxW/8, boxH/16);
	SetColor(COLOR_WHITE);
	glVertex2i(boxW/4, boxH/4);
    glEnd();

    glBegin(GL_QUADS);
	glVertex2i(0, 0);
	glVertex2i(100, 100);
	glVertex2i(-100, 100);
    glEnd();
}

static void QuadStrip(void)
{

    glBegin(GL_QUAD_STRIP);
	SetColor(COLOR_GREEN);
	glVertex2i(-boxW/4, -boxH/4);
	SetColor(COLOR_RED);
	glVertex2i(-boxW/4, boxH/4);
	SetColor(COLOR_BLUE);
	glVertex2i(0, -boxH/4);
	SetColor(COLOR_WHITE);
	glVertex2i(0, boxH/4);
	SetColor(COLOR_CYAN);
	glVertex2i(boxW/4, -boxH/4);
	SetColor(COLOR_YELLOW);
	glVertex2i(boxW/4, boxH/4);
    glEnd();

    glBegin(GL_QUAD_STRIP);
	glVertex2i(0, 0);
	glVertex2i(100, 100);
	glVertex2i(-100, 100);
    glEnd();
}

static void Draw(EGLDisplay dpy, EGLSurface surf)
{

    glViewport(0, 0, windW, windH);
    glDisable(GL_SCISSOR_TEST);

    glPushAttrib(GL_COLOR_BUFFER_BIT);

    glColorMask(1, 1, 1, 1);
    glIndexMask(~0);

    glClearColor(0.0, 0.0, 0.0, 0.0);
    glClear(GL_COLOR_BUFFER_BIT);

    glPopAttrib();

    if (mode1) {
	glShadeModel(GL_SMOOTH);
    } else {
	glShadeModel(GL_FLAT);
    }

    if (mode2) {
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    } else {
	glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    }

    Viewport(0, 0); Point();
    Viewport(0, 1); Lines();
    Viewport(0, 2); LineStrip();
    Viewport(0, 3); LineLoop();

    Viewport(1, 0); Bitmap();

    Viewport(1, 1); TriangleFan();
    Viewport(1, 2); Triangles();
    Viewport(1, 3); TriangleStrip();

    Viewport(2, 0); Rect();
    Viewport(2, 1); PolygonFunc();
    Viewport(2, 2); Quads();
    Viewport(2, 3); QuadStrip();

    glFlush();

    if (doubleBuffer) {
	eglSwapBuffers(dpy, surf);
    }
}

static void
write_ppm(const char *filename, const GLubyte *buffer, int width, int height)
{
   const int binary = 0;
   FILE *f = fopen( filename, "w" );
   if (f) {
      int i, x, y;
      const GLubyte *ptr = buffer;
      if (binary) {
         fprintf(f,"P6\n");
         fprintf(f,"# ppm-file created by osdemo.c\n");
         fprintf(f,"%i %i\n", width,height);
         fprintf(f,"255\n");
         fclose(f);
         f = fopen( filename, "ab" );  /* reopen in binary append mode */
         for (y=height-1; y>=0; y--) {
            for (x=0; x<width; x++) {
               i = (y*width + x) * 4;
               fputc(ptr[i], f);   /* write red */
               fputc(ptr[i+1], f); /* write green */
               fputc(ptr[i+2], f); /* write blue */
            }
         }
      }
      else {
         /*ASCII*/
         int counter = 0;
         fprintf(f,"P3\n");
         fprintf(f,"# ascii ppm file created by osdemo.c\n");
         fprintf(f,"%i %i\n", width, height);
         fprintf(f,"255\n");
         for (y=height-1; y>=0; y--) {
            for (x=0; x<width; x++) {
               i = (y*width + x) * 4;
               fprintf(f, " %3d %3d %3d", ptr[i], ptr[i+1], ptr[i+2]);
               counter++;
               if (counter % 5 == 0)
                  fprintf(f, "\n");
            }
         }
      }
      fclose(f);
   }
}

#include "../../src/egl/main/egldisplay.h"

typedef struct fb_display
{
   _EGLDisplay Base;  /* base class/object */
   void *pFB;
} fbDisplay;


int
main(int argc, char *argv[])
{
   int maj, min;
   EGLContext ctx;
   EGLSurface screen_surf;
   EGLConfig configs[10];
   EGLScreenMESA screen;
   EGLModeMESA mode;
   EGLint numConfigs, count;
   EGLBoolean b;
   const EGLint screenAttribs[] = {
      EGL_WIDTH, 1024,
      EGL_HEIGHT, 768,
      EGL_NONE
   };

   /*
   EGLDisplay d = eglGetDisplay(EGL_DEFAULT_DISPLAY);
   */
   EGLDisplay d = eglGetDisplay("!EGL_i915");
   assert(d);

   if (!eglInitialize(d, &maj, &min)) {
      printf("demo: eglInitialize failed\n");
      exit(1);
   }

   printf("EGL version = %d.%d\n", maj, min);
   printf("EGL_VENDOR = %s\n", eglQueryString(d, EGL_VENDOR));

   eglGetConfigs(d, configs, 10, &numConfigs);
   eglGetScreensMESA(d, &screen, 1, &count);
   eglGetModesMESA(d, screen, &mode, 1, &count);

   ctx = eglCreateContext(d, configs[0], EGL_NO_CONTEXT, NULL);
   if (ctx == EGL_NO_CONTEXT) {
      printf("failed to create context\n");
      return 0;
   }

   screen_surf = eglCreateScreenSurfaceMESA(d, configs[0], screenAttribs);
   if (screen_surf == EGL_NO_SURFACE) {
      printf("failed to create screen surface\n");
      return 0;
   }

   eglShowScreenSurfaceMESA(d, screen, screen_surf, mode);

   b = eglMakeCurrent(d, screen_surf, screen_surf, ctx);
   if (!b) {
      printf("make current failed\n");
      return 0;
   }
   glViewport(0, 0, 1024, 768);


   Init();
   Reshape(1024, 768);

   glDrawBuffer( GL_FRONT );
   glClearColor( 0, 1.0, 0, 1);

   glClear( GL_COLOR_BUFFER_BIT );

   doubleBuffer = 1;
   glDrawBuffer( GL_BACK );

   Draw(d, screen_surf);

   write_ppm("dump.ppm", ((struct fb_display *)_eglLookupDisplay(d))->pFB, 1024, 768);

   eglDestroySurface(d, screen_surf);
   eglDestroyContext(d, ctx);
   eglTerminate(d);

   return 0;
}
