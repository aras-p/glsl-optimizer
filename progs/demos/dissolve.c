/**
 * Dissolve between two images using randomized/patterned stencil buffer
 * and varying stencil ref.
 *
 * Brian Paul
 * 29 Jan 2010
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>
#include "readtex.h"

#define FILE1 "../images/bw.rgb"
#define FILE2 "../images/arch.rgb"


static int Win;
static int WinWidth = 400, WinHeight = 400;
static GLboolean Anim = GL_TRUE;

static int ImgWidth[2], ImgHeight[2];
static GLenum ImgFormat[2];
static GLubyte *Image[2];
static GLfloat ScaleX[2], ScaleY[2];

static GLubyte StencilRef = 0;

static int Mode = 0;


static void
Idle(void)
{
   StencilRef = (GLint) (glutGet(GLUT_ELAPSED_TIME) / 10);
   glutPostRedisplay();
}


static void
FillRandomPixels(GLubyte *b)
{
   int i;
   for (i = 0; i < WinWidth * WinHeight; i++) {
      b[i] = rand() & 0xff;
   }
}


static void
FillRandomRects(GLubyte *b)
{
   int i;

   memset(b, 0, WinWidth * WinHeight);

   for (i = 0; i < 256; i++) {
      int x = rand() % WinWidth;
      int y = rand() % WinHeight;
      int w = rand() % 60;
      int h = rand() % 60;
      int ix, iy;

      if (x + w > WinWidth)
         w = WinWidth - x;
      if (y + h > WinHeight)
         h = WinHeight - y;

      for (iy = 0; iy < h; iy++) {
         for (ix = 0; ix < w; ix++) {
            int p = (y + iy) * WinWidth + x + ix;
            b[p] = i;
         }
      }
   }
}


static void
FillWipe(GLubyte *b)
{
   int iy, ix;

   memset(b, 0, WinWidth * WinHeight);

   for (iy = 0; iy < WinHeight; iy++) {
      for (ix = 0; ix < WinWidth; ix++) {
         int p = iy * WinWidth + ix;
         b[p] = 2 * ix + iy / 2;
      }
   }
}


static void
FillMoire(GLubyte *b)
{
   int iy, ix;

   memset(b, 0, WinWidth * WinHeight);

   for (iy = 0; iy < WinHeight; iy++) {
      for (ix = 0; ix < WinWidth; ix++) {
         int p = iy * WinWidth + ix;
         b[p] = (ix / 2) * (ix / 2) - (iy / 2) * (iy / 2);
      }
   }
}


static void
FillWaves(GLubyte *b)
{
   int iy, ix;

   memset(b, 0, WinWidth * WinHeight);

   for (iy = 0; iy < WinHeight; iy++) {
      for (ix = 0; ix < WinWidth; ix++) {
         int p = iy * WinWidth + ix;
         float x = 8.0 * 3.1415 * ix / (float) WinWidth;
         b[p] = (int) (25.0 * sin(x) ) - iy*2;
      }
   }
}


typedef void (*FillFunc)(GLubyte *b);


static FillFunc Funcs[] = {
   FillRandomPixels,
   FillRandomRects,
   FillWipe,
   FillMoire,
   FillWaves
};

#define NUM_MODES (sizeof(Funcs) / sizeof(Funcs[0]))



static void
InitStencilBuffer(void)
{
   GLubyte *b = malloc(WinWidth * WinHeight);

   Funcs[Mode](b);

   glStencilFunc(GL_ALWAYS, 0, ~0);
   glPixelZoom(1.0, 1.0);
   glDrawPixels(WinWidth, WinHeight, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, b);

   free(b);
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   glPixelZoom(ScaleX[0], ScaleY[0]);
   glStencilFunc(GL_LESS, StencilRef, ~0);
   glDrawPixels(ImgWidth[0], ImgHeight[0], ImgFormat[0], GL_UNSIGNED_BYTE, Image[0]);

   glPixelZoom(ScaleX[1], ScaleY[1]);
   glStencilFunc(GL_GEQUAL, StencilRef, ~0);
   glDrawPixels(ImgWidth[1], ImgHeight[1], ImgFormat[1], GL_UNSIGNED_BYTE, Image[1]);

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   InitStencilBuffer();

   ScaleX[0] = (float) width / ImgWidth[0];
   ScaleY[0] = (float) height / ImgHeight[0];

   ScaleX[1] = (float) width / ImgWidth[1];
   ScaleY[1] = (float) height / ImgHeight[1];
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case 'a':
   case ' ':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(Idle);
      else
         glutIdleFunc(NULL);
      break;
   case 'i':
      InitStencilBuffer();
      break;
   case '-':
      StencilRef--;
      break;
   case '+':
      StencilRef++;
      break;
   case 'm':
      Mode = (Mode + 1) % NUM_MODES;
      InitStencilBuffer();
      break;
   case 27:
      glutDestroyWindow(Win);
      exit(0);
      break;
   }
   glutPostRedisplay();
}



static void
Init(void)
{
   Image[0] = LoadRGBImage(FILE1, &ImgWidth[0], &ImgHeight[0], &ImgFormat[0]);
   if (!Image[0]) {
      printf("Couldn't read %s\n", FILE1);
      exit(0);
   }

   Image[1] = LoadRGBImage(FILE2, &ImgWidth[1], &ImgHeight[1], &ImgFormat[1]);
   if (!Image[1]) {
      printf("Couldn't read %s\n", FILE2);
      exit(0);
   }

   glEnable(GL_STENCIL_TEST);
   glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}


int
main(int argc, char *argv[])
{
   glutInitWindowSize(WinWidth, WinHeight);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_STENCIL);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init();

   printf("Keys:\n");
   printf("  a/SPACE  toggle animation\n");
   printf("  +/-      single step\n");
   printf("  i        re-init pattern\n");
   printf("  m        change pattern/dissolve mode\n");
   printf("  ESC      exit\n");

   glutMainLoop();
   return 0;
}
