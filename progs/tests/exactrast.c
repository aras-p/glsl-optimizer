/**
 * Test for exact point/line/polygon rasterization, or at least rasterization
 * that fits the tolerance of the OpenGL spec.
 *
 * Brian Paul
 * 9 Nov 2007
 */

/*
 * Notes:
 * - 'm' to cycle through point, hline, vline and quad drawing
 * - Use cursor keys to translate coordinates (z to reset)
 * - Resize window to check for proper rasterization
 * - Make sure your LCD is running in its native resolution
 *   
 * If translation is (0,0):
 *  a point will be drawn where x%2==0 and y%2==0,
 *  a horizontal line will be drawn where x%2==0,
 *  a vertical line will be drawn where y%2==0,
 *  for quads, pixels will be set where (x%4)!=3 and (y%4)!=3
 *
 * XXX todo: do glReadPixels and test that the results are what's expected.
 * Upon failure, iterate over sub-pixel translations to find the ideal offset.
 */


#include <stdio.h>
#include <stdlib.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Width = 400, Height = 400;
static int Win;
static float Xtrans = 0, Ytrans = 0;
static float Step = 0.125;

enum {
   MODE_POINTS,
   MODE_HLINES,
   MODE_VLINES,
   MODE_QUADS,
   NUM_MODES
};

static int Mode = MODE_POINTS;


static void
Draw(void)
{
   /* See the OpenGL Programming Guide, Appendix H, "OpenGL Correctness Tips"
    * for information about the 0.375 translation factor.
    */
   float tx = 0.375, ty = 0.375;
   int i, j;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glTranslatef(tx + Xtrans, ty + Ytrans, 0);

   if (Mode == MODE_POINTS) {
      glBegin(GL_POINTS);
      for (j = 0; j < Height; j += 2) {
         for (i = 0; i < Width; i += 2) {
            glVertex2f(i, j);
         }
      }
      glEnd();
   }
   else if (Mode == MODE_HLINES) {
      glBegin(GL_LINES);
      for (i = 0; i < Height; i += 2) {
         glVertex2f(0,     i);
         glVertex2f(Width, i);
      }
      glEnd();
   }
   else if (Mode == MODE_VLINES) {
      glBegin(GL_LINES);
      for (i = 0; i < Width; i += 2) {
         glVertex2f(i, 0     );
         glVertex2f(i, Height);
      }
      glEnd();
   }
   else if (Mode == MODE_QUADS) {
      glBegin(GL_QUADS);
      for (j = 0; j < Height; j += 4) {
         for (i = 0; i < Width; i += 4) {
            glVertex2f(i,     j    );
            glVertex2f(i + 3, j    );
            glVertex2f(i + 3, j + 3);
            glVertex2f(i,     j + 3);
         }
      }
      glEnd();
   }

   glPopMatrix();

   glutSwapBuffers();
}


static void
Reshape(int width, int height)
{
   Width = width;
   Height = height;
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void
Key(unsigned char key, int x, int y)
{
   (void) x;
   (void) y;
   switch (key) {
   case 'm':
   case 'M':
      Mode = (Mode + 1) % NUM_MODES;
      break;
   case 'z':
   case 'Z':
      Xtrans = Ytrans = 0;
      printf("Translation: %f, %f\n", Xtrans, Ytrans);
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
   (void) x;
   (void) y;
   switch (key) {
   case GLUT_KEY_UP:
      Ytrans += Step;
      break;
   case GLUT_KEY_DOWN:
      Ytrans -= Step;
      break;
   case GLUT_KEY_LEFT:
      Xtrans -= Step;
      break;
   case GLUT_KEY_RIGHT:
      Xtrans += Step;
      break;
   }
   glutPostRedisplay();
   printf("Translation: %f, %f\n", Xtrans, Ytrans);
}


static void
Init(void)
{
}


static void
Usage(void)
{
   printf("Keys:\n");
   printf("  up/down/left/right - translate by %f\n", Step);
   printf("  z - reset translation to zero\n");
   printf("  m - change rendering mode (points, hlines, vlines, quads)\n");
   printf("  Esc - exit\n");
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
   glutSpecialFunc(SpecialKey);
   glutDisplayFunc(Draw);
   Init();
   Usage();
   glutMainLoop();
   return 0;
}
