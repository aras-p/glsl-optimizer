/**
 * Measure fill rates for basic shading/texturing modes.
 *
 * Brian Paul
 * 1 April 2008
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "readtex.h"

#define TEXTURE_1_FILE "../images/tile.rgb"
#define TEXTURE_2_FILE "../images/reflect.rgb"

static int Win;
static int Width = 1010, Height = 1010;
static GLuint Textures[2];


/**
 * Draw quad 10 pixels shorter, narrower than window size.
 */
static void
DrawQuad(void)
{
   glBegin(GL_POLYGON);

   glColor3f(1.0, 0.5, 0.5);
   glMultiTexCoord2f(GL_TEXTURE0, 0, 0);
   glMultiTexCoord2f(GL_TEXTURE1, 0, 0);
   glVertex2f(5, 5);

   glColor3f(0.5, 1.0, 0.5);
   glMultiTexCoord2f(GL_TEXTURE0, 1, 0);
   glMultiTexCoord2f(GL_TEXTURE1, 1, 0);
   glVertex2f(Width - 5, 5);

   glColor3f(0.5, 0.5, 1.0);
   glMultiTexCoord2f(GL_TEXTURE0, 1, 1);
   glMultiTexCoord2f(GL_TEXTURE1, 1, 1);
   glVertex2f(Width - 5, Height - 5);

   glColor3f(1.0, 0.5, 1.0);
   glMultiTexCoord2f(GL_TEXTURE0, 0, 1);
   glMultiTexCoord2f(GL_TEXTURE1, 0, 1);
   glVertex2f(5, Height - 5);

   glEnd();
}


/**
 * Compute rate for drawing large quad with given shading/texture state.
 */
static void
RunTest(GLenum shading, GLuint numTextures, GLenum texFilter)
{
   const GLdouble minPeriod = 2.0;
   GLdouble t0, t1;
   GLdouble pixels, rate;
   GLint i, iters;

   glActiveTexture(GL_TEXTURE0);
   if (numTextures > 0) {
      glBindTexture(GL_TEXTURE_2D, Textures[0]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFilter);
      glEnable(GL_TEXTURE_2D);
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }

   glActiveTexture(GL_TEXTURE1);
   if (numTextures > 1) {
      glBindTexture(GL_TEXTURE_2D, Textures[1]);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, texFilter);
      glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, texFilter);
      glEnable(GL_TEXTURE_2D);
   }
   else {
      glDisable(GL_TEXTURE_2D);
   }

   glShadeModel(shading);


   glFinish();

   iters = 1;
   do {
      iters *= 4;
      t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      for (i = 0; i < iters; i++) {
         DrawQuad();
      }
      glFinish();
      t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   } while (t1 - t0 < minPeriod);

   glutSwapBuffers();

   pixels = (double) iters * (Width - 10) * (Height - 10);
   rate = pixels / (t1 - t0);
   rate /= 1000000.0;  /* megapixels/second */

   printf("%s ", shading == GL_FLAT ? "GL_FLAT" : "GL_SMOOTH");
   printf("Textures=%u ", numTextures);
   printf("Filter=%s: ", texFilter == GL_LINEAR ? "GL_LINEAR" : "GL_NEAREST");
   printf("%g MPixels/sec\n", rate);
}


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   RunTest(GL_FLAT, 0, GL_NEAREST);
   RunTest(GL_SMOOTH, 0, GL_NEAREST);
   RunTest(GL_SMOOTH, 1, GL_NEAREST);
   RunTest(GL_SMOOTH, 1, GL_LINEAR);
   RunTest(GL_SMOOTH, 2, GL_NEAREST);
   RunTest(GL_SMOOTH, 2, GL_LINEAR);

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   Width = width;
   Height = height;
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
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
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   glGenTextures(2, Textures);

   glBindTexture(GL_TEXTURE_2D, Textures[0]);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   if (!LoadRGBMipmaps(TEXTURE_1_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }

   glBindTexture(GL_TEXTURE_2D, Textures[1]);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   if (!LoadRGBMipmaps(TEXTURE_2_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }

}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(Width, Height);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
