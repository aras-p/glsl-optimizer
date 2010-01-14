/**
 * Test frustum/user clipping w/ glPolygonMode LINE/POINT.
 *
 * The bottom/left and bottom/right verts are outside the frustum and clipped.
 * The top vertex is clipped by a user clipping plane.
 *
 * A filled gray reference triangle is shown underneath the points/lines.
 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>


static int win;


static void
ColorTri(void)
{
   glBegin(GL_TRIANGLES);
   glColor3f(1, 0, 0);   glVertex3f(-1.5, -0.8, 0.0);
   glColor3f(0, 1, 0);   glVertex3f( 1.5, -0.8, 0.0);
   glColor3f(0, 0, 1);   glVertex3f( 0.0,  0.9, 0.0);
   glEnd();
}


static void
GrayTri(void)
{
   glColor3f(0.3, 0.3, 0.3);
   glBegin(GL_TRIANGLES);
   glVertex3f(-1.5, -0.8, 0.0);
   glVertex3f( 1.5, -0.8, 0.0);
   glVertex3f( 0.0,  0.9, 0.0);
   glEnd();
}


static void
Draw(void)
{
   static const GLdouble plane[4] = { 0, -1.0, 0, 0.5 };

   glClear(GL_COLOR_BUFFER_BIT); 

   glPointSize(13.0);
   glLineWidth(5.0);

   glClipPlane(GL_CLIP_PLANE0, plane);
   glEnable(GL_CLIP_PLANE0);

   glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
   GrayTri();

   glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
   ColorTri();

   glPolygonMode(GL_FRONT_AND_BACK, GL_POINT);
   ColorTri();

   glutSwapBuffers();
}


static void Reshape(int width, int height)
{
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0);
   glMatrixMode(GL_MODELVIEW);
}


static void
Key(unsigned char key, int x, int y)
{
   if (key == 27) {
      glutDestroyWindow(win);
      exit(0);
   }
   else {
      glutPostRedisplay();
   }
}


static void
Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);
}


int
main(int argc, char **argv)
{
   glutInitWindowSize(300, 300);
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_DEPTH);
   win = glutCreateWindow(*argv);
   if (!win) {
      return 1;
   }
   Init();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   glutMainLoop();
   return 0;
}
