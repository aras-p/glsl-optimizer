/*
 * Test glRead/DrawPixels for GL_DEPTH_COMPONENT, with pixelzoom.
 * 
 * Brian Paul
 * 23 August 2003
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


#define ZWIDTH 100
#define ZHEIGHT 100

#define ZOOM 4

#define ZWIDTH2 (ZOOM * ZWIDTH)
#define ZHEIGHT2 (ZOOM * ZHEIGHT)

static GLint WinWidth = ZWIDTH + ZWIDTH2, WinHeight = ZHEIGHT + ZHEIGHT2;
static GLboolean Invert = GL_FALSE;
static GLboolean TestPacking = GL_FALSE;
static GLboolean TestList = GL_FALSE;


static void Display(void)
{
   GLfloat depth[ZWIDTH * ZHEIGHT * 2];
   GLfloat depth2[ZWIDTH2 * ZHEIGHT2]; /* *2 to test pixelstore stuff */
   GLuint list;
   GLenum depthType = GL_FLOAT;

   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glEnable(GL_DEPTH_TEST);

   /* draw a sphere */
   glViewport(0, 0, ZWIDTH, ZHEIGHT);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 0);  /* clip away back half of sphere */
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glutSolidSphere(1.0, 20, 10);

   if (TestPacking) {
      glPixelStorei(GL_PACK_ROW_LENGTH, 120);
      glPixelStorei(GL_PACK_SKIP_PIXELS, 5);
   }

   /* read the depth image */
   glReadPixels(0, 0, ZWIDTH, ZHEIGHT, GL_DEPTH_COMPONENT, depthType, depth);
   if (depthType == GL_FLOAT) {
      GLfloat min, max;
      int i;
      min = max = depth[0];
      for (i = 1; i < ZWIDTH * ZHEIGHT; i++) {
         if (depth[i] < min)
            min = depth[i];
         if (depth[i] > max)
            max = depth[i];
      }
      printf("Depth value range: [%f, %f]\n", min, max);
   }

   /* debug */
   if (0) {
      int i;
      float *z = depth + ZWIDTH * 50;
      printf("z at y=50:\n");
      for (i = 0; i < ZWIDTH; i++) {
         printf("%5.3f ", z[i]);
         if ((i + 1) % 12 == 0)
            printf("\n");
      }
      printf("\n");
   }

   /* Draw the Z image as luminance above original rendering */
   glWindowPos2i(0, ZHEIGHT);
   glDrawPixels(ZWIDTH, ZHEIGHT, GL_LUMINANCE, depthType, depth);

   if (TestPacking) {
      glPixelStorei(GL_PACK_ROW_LENGTH, 0);
      glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 120);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, 5);
   }

   /* draw depth image with scaling (into z buffer) */
   glPixelZoom(ZOOM, ZOOM);
   glColor4f(1, 0, 0, 0);
   glWindowPos2i(ZWIDTH, 0);
   if (Invert) {
      glPixelTransferf(GL_DEPTH_SCALE, -1.0);
      glPixelTransferf(GL_DEPTH_BIAS, 1.0);
   }
   if (TestList) {
      list = glGenLists(1);
      glNewList(list, GL_COMPILE);
      glDrawPixels(ZWIDTH, ZHEIGHT, GL_DEPTH_COMPONENT, depthType, depth);
      glEndList();
      glCallList(list);
      glDeleteLists(list, 1);
   }
   else {
      glDrawPixels(ZWIDTH, ZHEIGHT, GL_DEPTH_COMPONENT, depthType, depth);
   }
   if (Invert) {
      glPixelTransferf(GL_DEPTH_SCALE, 1.0);
      glPixelTransferf(GL_DEPTH_BIAS, 0.0);
   }

   if (TestPacking) {
      glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
      glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
   }

   glDisable(GL_DEPTH_TEST);

   /* read back scaled depth image */
   glReadPixels(ZWIDTH, 0, ZWIDTH2, ZHEIGHT2, GL_DEPTH_COMPONENT, GL_FLOAT, depth2);

   /* debug */
   if (0) {
      int i;
      float *z = depth2 + ZWIDTH2 * 200;
      printf("z at y=200:\n");
      for (i = 0; i < ZWIDTH2; i++) {
         printf("%5.3f ", z[i]);
         if ((i + 1) % 12 == 0)
            printf("\n");
      }
      printf("\n");
   }

   /* draw as luminance */
   glPixelZoom(1.0, 1.0);
   glWindowPos2i(ZWIDTH, 0);
   glDrawPixels(ZWIDTH2, ZHEIGHT2, GL_LUMINANCE, GL_FLOAT, depth2);

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
      case 'i':
         Invert = !Invert;
         break;
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
   GLint z;

   glGetIntegerv(GL_DEPTH_BITS, &z);

   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_DEPTH_BITS = %d\n", z);

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
   glutInitDisplayMode(GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE);
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   Init();
   glutMainLoop();
   return 0;
}
