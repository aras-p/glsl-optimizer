/*
 * Test glRead/DrawPixels for GL_STENCIL_INDEX, with pixelzoom.
 * 
 * Brian Paul
 * 8 April 2010
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static GLint WinWidth = 500, WinHeight = 500;
static GLboolean TestPacking = GL_FALSE;
static GLboolean TestList = GL_FALSE;


static GLuint StencilValue = 192;

static void Display(void)
{
   GLubyte stencil[100 * 100 * 2];
   GLubyte stencil2[400 * 400]; /* *2 to test pixelstore stuff */
   GLuint list;

   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);

   glEnable(GL_DEPTH_TEST);

   glStencilOp(GL_INVERT, GL_KEEP, GL_REPLACE);
   glStencilFunc(GL_ALWAYS, StencilValue, ~0);
   glEnable(GL_STENCIL_TEST);

   /* draw a sphere */
   glViewport(0, 0, 100, 100);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 0);  /* clip away back half of sphere */
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glutSolidSphere(1.0, 20, 10);

   glDisable(GL_STENCIL_TEST);

   if (TestPacking) {
      glPixelStorei(GL_PACK_ROW_LENGTH, 120);
      glPixelStorei(GL_PACK_SKIP_PIXELS, 5);
   }

   /* read the stencil image */
   glReadPixels(0, 0, 100, 100, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil);
   {
      GLubyte min, max;
      int i;
      min = max = stencil[0];
      for (i = 1; i < 100 * 100; i++) {
         if (stencil[i] < min)
            min = stencil[i];
         if (stencil[i] > max)
            max = stencil[i];
      }
      printf("Stencil value range: [%u, %u]\n", min, max);
      if (max != StencilValue) {
         printf("Error: expected max stencil val of %u, found %u\n",
                StencilValue, max);
      }
   }

   /* Draw the Z image as luminance above original rendering */
   glWindowPos2i(0, 100);
   glDrawPixels(100, 100, GL_LUMINANCE, GL_UNSIGNED_BYTE, stencil);

   if (1) {
      if (TestPacking) {
         glPixelStorei(GL_PACK_ROW_LENGTH, 0);
         glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 120);
         glPixelStorei(GL_UNPACK_SKIP_PIXELS, 5);
      }

      /* draw stencil image with scaling (into stencil buffer) */
      glPixelZoom(4.0, 4.0);
      glColor4f(1, 0, 0, 0);
      glWindowPos2i(100, 0);

      if (TestList) {
         list = glGenLists(1);
         glNewList(list, GL_COMPILE);
         glDrawPixels(100, 100, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil);
         glEndList();
         glCallList(list);
         glDeleteLists(list, 1);
      }
      else {
         glDrawPixels(100, 100, GL_STENCIL_INDEX, GL_UNSIGNED_BYTE, stencil);
      }

      if (TestPacking) {
         glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
         glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
      }

      /* read back scaled stencil image */
      glReadPixels(100, 0, 400, 400, GL_STENCIL_INDEX,
                   GL_UNSIGNED_BYTE, stencil2);
      /* draw as luminance */
      glPixelZoom(1.0, 1.0);
      glWindowPos2i(100, 0);
      glDrawPixels(400, 400, GL_LUMINANCE, GL_UNSIGNED_BYTE, stencil2);
   }

   glutSwapBuffers();
}


static void Reshape(int width, int height)
{
   WinWidth = width;
   WinHeight = height;
   glViewport(0, 0, width, height);
}


static void Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'p':
         TestPacking = !TestPacking;
         printf("Test pixel pack/unpack: %d\n", TestPacking);
         break;
      case 'l':
         TestList = !TestList;
         printf("Test dlist: %d\n", TestList);
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init(void)
{
   const GLfloat blue[4] = {.1, .1, 1.0, 1.0};
   const GLfloat gray[4] = {0.2, 0.2, 0.2, 1.0};
   const GLfloat white[4] = {1.0, 1.0, 1.0, 1.0};
   const GLfloat pos[4] = {0, 0, 10, 0};
   GLint bits;

   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   glGetIntegerv(GL_STENCIL_BITS, &bits);
   printf("Stencil bits: %d\n", bits);

   assert(bits >= 8);

   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blue);
   glLightfv(GL_LIGHT0, GL_AMBIENT, gray);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
}


int main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WinWidth, WinHeight);
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL | GLUT_DOUBLE);
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   Init();
   glutMainLoop();
   return 0;
}
