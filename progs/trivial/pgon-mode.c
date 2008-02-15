/**
 * Test glPolygonMode.
 * A tri-strip w/ two tris is drawn so that the first tri is front-facing
 * but the second tri is back-facing.
 * Set glPolygonMode differently for the front/back faces
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>

static int Win;
static GLfloat Zrot = 0;
static GLboolean FrontFillBackUnfilled = GL_TRUE;
static GLboolean Lines = GL_TRUE;


static void
Draw(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (FrontFillBackUnfilled) {
      if (Lines) {
         printf("FrontMode = FILL, BackMode = LINE\n");
         glPolygonMode(GL_BACK, GL_LINE);
      }
      else {
         printf("FrontMode = FILL, BackMode = POINT\n");
         glPolygonMode(GL_BACK, GL_POINT);
      }
      glPolygonMode(GL_FRONT, GL_FILL);
   }
   else {
      if (Lines) {
         printf("FrontMode = LINE, BackMode = FILL\n");
         glPolygonMode(GL_FRONT, GL_LINE);
      }
      else {
         printf("FrontMode = POINT, BackMode = FILL\n");
         glPolygonMode(GL_FRONT, GL_POINT);
      }
      glPolygonMode(GL_BACK, GL_FILL);
   }

   glPushMatrix();
   glRotatef(Zrot, 0, 0, 1);

   glBegin(GL_TRIANGLE_STRIP);
   glVertex2f(-1, 0);
   glVertex2f( 1, 0);
   glVertex2f(0,  1);
   glVertex2f(0, -1);
   glEnd();

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
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
   case 'p':
      FrontFillBackUnfilled = !FrontFillBackUnfilled;
      break;
   case 'l':
      Lines = !Lines;
      break;
   case 'z':
      Zrot -= step;
      break;
   case 'Z':
      Zrot += step;
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
   printf("GL_RENDERER = %s\n", (char*) glGetString(GL_RENDERER));

   glLineWidth(3.0);
   glPointSize(3.0);

   glColor4f(1, 1, 1, 0.8);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

   printf("Press 'p' to toggle polygon mode\n");
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
