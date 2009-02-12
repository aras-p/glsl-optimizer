/**
 * Test flat shading and clipping.
 *
 * Brian Paul
 * 30 August 2007
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

static int Win;
static GLfloat Scale = 2.0, Zrot = 50;
static GLenum Mode = GL_LINE_LOOP;
static GLboolean Smooth = 0;
static GLenum PolygonMode = GL_FILL;


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (Smooth)
      glShadeModel(GL_SMOOTH);
   else
      glShadeModel(GL_FLAT);

   glPushMatrix();
   glScalef(Scale, Scale, 1);
   glRotatef(Zrot, 0, 0, 1);

   glPolygonMode(GL_FRONT_AND_BACK, PolygonMode);

   glBegin(Mode);
   glColor3f(1, 0, 0);
   glVertex2f(-1, -1);
   glColor3f(0, 1, 0);
   glVertex2f( 2, -1);
   glColor3f(0, 0, 1);
   glVertex2f( 0, 1);
   glEnd();

   glPushMatrix();
   glScalef(0.9, 0.9, 1);
   glBegin(Mode);
   glColor3f(1, 0, 0);
   glVertex2f( 0, 1);

   glColor3f(0, 0, 1);
   glVertex2f( 2, -1);

   glColor3f(0, 1, 0);
   glVertex2f(-1, -1);

   glEnd();
   glPopMatrix();

   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 25.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
      case 'p':
         if (Mode == GL_TRIANGLES)
            Mode = GL_LINE_LOOP;
         else
            Mode = GL_TRIANGLES;
         break;
      case 'f':
         if (PolygonMode == GL_POINT)
            PolygonMode = GL_LINE;
         else if (PolygonMode == GL_LINE)
            PolygonMode = GL_FILL;
         else
            PolygonMode = GL_POINT;
         printf("PolygonMode = 0x%x\n", PolygonMode);
         break;
      case 'r':
         Zrot -= 5.0;
         break;
      case 'R':
         Zrot += 5.0;
         break;
      case 'z':
         Scale *= 1.1;
         break;
      case 'Z':
         Scale /= 1.1;
         break;
      case 's':
         Smooth = !Smooth;
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
   printf("Usage:\n");
   printf("  z/Z:  change triangle size\n");
   printf("  r/R:  rotate\n");
   printf("  p:    toggle line/fill mode\n");
   printf("  s:    toggle smooth/flat shading\n");
   printf("  f:    switch polygon fill mode\n");
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   Init();
   glutMainLoop();
   return 0;
}
