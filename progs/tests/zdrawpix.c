/**
 * Test glDrawPixels(GL_DEPTH_COMPONENT)
 *
 * We load a window-sized buffer of Z values so that Z=1 at the top and
 * Z=0 at the bottom (and interpolate between).
 * We draw that image into the Z buffer, then draw an ordinary cube.
 * The bottom part of the cube should be "clipped" where the cube fails
 * the Z test.
 *
 * Press 'd' to view the Z buffer as a grayscale image.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "../util/showbuffer.c"


static int Win;
static GLfloat Xrot = 50, Yrot = 40, Zpos = 6;
static GLboolean Anim = GL_FALSE;

static int Width = 200, Height = 200;
static GLfloat *z;
static GLboolean showZ = 0;


static void
Idle(void)
{
   Xrot += 3.0;
   Yrot += 4.0;
   glutPostRedisplay();
}


static void
Draw(void)
{
   glClearColor(0, 0, 0.5, 0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

#if 1
   glColor3f(1, 0, 0);
   glWindowPos2i(0,0);
   glColorMask(0,0,0,0);
   glDrawPixels(Width, Height, GL_DEPTH_COMPONENT, GL_FLOAT, z);
#elif 0
   glPushMatrix();
   glTranslatef(-0.75, 0, Zpos);
   glutSolidSphere(1.0, 20, 10);
   glPopMatrix();
#endif
   glColorMask(1,1,1,1);

   /* draw cube */
   glPushMatrix();
   glTranslatef(0, 0, Zpos);
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glutSolidCube(2.0);
   glPopMatrix();

#if 0
   /* drawpixels after cube */
   glColor3f(1, 0, 0);
   glWindowPos2i(0,0);
   //glColorMask(0,0,0,0);
   glDrawPixels(Width, Height, GL_DEPTH_COMPONENT, GL_FLOAT, z);
#endif

   if (showZ) {
      ShowDepthBuffer(Width, Height, 0.0, 1.0);
   }

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum(-1.0, 1.0, -1.0, 1.0, 5.0, 30.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -15.0);

   Width = width;
   Height = height;

   z = (float *) malloc(width * height * 4);
   {
      int i, j, k = 0;
      for (i = 0; i < height; i++) {
         float zval = (float) i / (height - 1);
         for (j = 0; j < width; j++) {
            z[k++] = zval;
         }
      }
   }
}


static void
Key(unsigned char key, int x, int y)
{
   const GLfloat step = 1.0;
   (void) x;
   (void) y;
   switch (key) {
      case 'a':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(Idle);
         else
            glutIdleFunc(NULL);
         break;
      case 'd':
         showZ = !showZ;
         break;
      case 'z':
         Zpos -= step;
         break;
      case 'Z':
         Zpos += step;
         break;
      case 27:
         glutDestroyWindow(Win);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
SpecialKey(int key, int x, int y)
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Xrot -= step;
         break;
      case GLUT_KEY_DOWN:
         Xrot += step;
         break;
      case GLUT_KEY_LEFT:
         Yrot -= step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot += step;
         break;
   }
   glutPostRedisplay();
}


static void
Init(void)
{
   /* setup lighting, etc */
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
}


int
main(int argc, char *argv[])
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(400, 400);
   glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
   Win = glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   if (Anim)
      glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
