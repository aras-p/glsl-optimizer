/*
 * Test Z values of glBitmap.
 * Brian Paul
 * 19 Feb 2010
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>


static GLint Win = 0;


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
Display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-3.0, 3.0, -3.0, 3.0, -2.0, 2.0);

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glColor3f(1, 1, 1);
   glRasterPos2f(-2.0, 2.6);
   PrintString("Z = -1.0");
   glRasterPos2f(-0.5, 2.6);
   PrintString("Z = 0.0");
   glRasterPos2f(1.0, 2.6);
   PrintString("Z = 1.0");

   glColor3f(0, 0.4, 0.6);
   glBegin(GL_QUADS);
   glVertex3f(-2.0, -2.5, -1);
   glVertex3f(-1.0, -2.5, -1);
   glVertex3f(-1.0, 2.5, -1);
   glVertex3f(-2.0, 2.5, -1);

   glVertex3f(-0.5, -2.5, 0);
   glVertex3f(0.5, -2.5, 0);
   glVertex3f(0.5, 2.5, 0);
   glVertex3f(-0.5, 2.5, 0);

   glVertex3f(1.0, -2.5, 1);
   glVertex3f(2.0, -2.5, 1);
   glVertex3f(2.0, 2.5, 1);
   glVertex3f(1.0, 2.5, 1);
   glEnd();

   glColor3f(1, 1, 1);

   glRasterPos3f(-2.0, -1, -1.0);
   PrintString("This is a bitmap string drawn at z = -1.0");

   glRasterPos3f(-2.0, 0, 0.0);
   PrintString("This is a bitmap string drawn at z = 0.0");

   glRasterPos3f(-2.0, 1, 1.0);
   PrintString("This is a bitmap string drawn at z = 1.0");

   glRasterPos3f(-1.5, -2.8, 0.0);
   PrintString("GL_DEPTH_FUNC = GL_LEQUAL");

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
}


static void
Key(unsigned char key, int x, int y)
{
   if (key == 27) {
      glutDestroyWindow(Win);
      exit(0);
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   glClearColor(0.25, 0.25, 0.25, 0.0);
   glDepthFunc(GL_LEQUAL);
   glEnable(GL_DEPTH_TEST);
}


int
main(int argc, char *argv[])
{
   glutInitWindowSize(400, 400);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Display);
   Init();
   glutMainLoop();
   return 0;
}
