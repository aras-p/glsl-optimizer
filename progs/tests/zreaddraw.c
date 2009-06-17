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

static GLint WinWidth = 500, WinHeight = 500;


static void Display(void)
{
   GLfloat depth[100 * 100];
   GLfloat depth2[400 * 400];
   GLfloat min, max;
   int i;

   glClearColor(0.5, 0.5, 0.5, 1.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   /* draw a sphere */
   glViewport(0, 0, 100, 100);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 0);  /* clip away back half of sphere */
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glutSolidSphere(1.0, 20, 10);

   /* read the depth image */
   glReadPixels(0, 0, 100, 100, GL_DEPTH_COMPONENT, GL_FLOAT, depth);
   min = max = depth[0];
   for (i = 1; i < 100 * 100; i++) {
      if (depth[i] < min)
         min = depth[i];
      if (depth[i] > max)
         max = depth[i];
   }
   printf("Depth value range: [%f, %f]\n", min, max);

   /* draw depth image with scaling (into z buffer) */
   glPixelZoom(4.0, 4.0);
   glWindowPos2i(100, 0);
   glDrawPixels(100, 100, GL_DEPTH_COMPONENT, GL_FLOAT, depth);

   /* read back scaled depth image */
   glReadPixels(100, 0, 400, 400, GL_DEPTH_COMPONENT, GL_FLOAT, depth2);
   /* draw as luminance */
   glPixelZoom(1.0, 1.0);
   glDrawPixels(400, 400, GL_LUMINANCE, GL_FLOAT, depth2);

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

   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   glMaterialfv(GL_FRONT_AND_BACK, GL_DIFFUSE, blue);
   glLightfv(GL_LIGHT0, GL_AMBIENT, gray);
   glLightfv(GL_LIGHT0, GL_DIFFUSE, white);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
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
