/*
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 *
 * Permission to use, copy, modify, distribute, and sell this software and
 * its documentation for any purpose is hereby granted without fee, provided
 * that (i) the above copyright notices and this permission notice appear in
 * all copies of the software and related documentation, and (ii) the name of
 * Silicon Graphics may not be used in any advertising or
 * publicity relating to the software without the specific, prior written
 * permission of Silicon Graphics.
 *
 * THE SOFTWARE IS PROVIDED "AS-IS" AND WITHOUT WARRANTY OF
 * ANY KIND,
 * EXPRESS, IMPLIED OR OTHERWISE, INCLUDING WITHOUT LIMITATION, ANY
 * WARRANTY OF MERCHANTABILITY OR FITNESS FOR A PARTICULAR PURPOSE.
 *
 * IN NO EVENT SHALL SILICON GRAPHICS BE LIABLE FOR
 * ANY SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS,
 * WHETHER OR NOT ADVISED OF THE POSSIBILITY OF DAMAGE, AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>

GLenum doubleBuffer = 1;
int win;
static float tx = 0;
static float ty = 0;
static float tw = 0;
static float th = 0;

static float win_width = 250;
static float win_height = 250;

static void Init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

   glClearColor(0, 0, 0, 0.0);
}

static void Reshape(int width, int height)
{
   win_width = width;
   win_height = height;
   glutPostRedisplay();
}


static void Key(unsigned char key, int x, int y)
{
   switch (key) {
   case 27:
      exit(0);
   case 'w':
      tw += 1.0;
      break;
   case 'W':
      tw -= 1.0;
      break;
   case 'h':
      th += 1.0;
      break;
   case 'H':
      th -= 1.0;
      break;
   case ' ':
      tw = th = tx = ty = 0;
      break;
   default:
      break;
   }
   glutPostRedisplay();
}


static void Draw(void)
{
   int i;
   float w = tw + win_width;
   float h = th + win_height;

   fprintf(stderr, "glViewport(%f %f %f %f)\n", tx, ty, w, h);
   fflush(stderr);

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode(GL_MODELVIEW);

   glClear(GL_COLOR_BUFFER_BIT); 


   /***********************************************************************
    * Should be clipped to be no larger than the triangles:
    */
   glViewport(tx, ty, w, h);

   glBegin(GL_POLYGON);
   glColor3f(1,1,0); 
   glVertex3f(-100, -100, -30.0);
   glVertex3f(-100, 100, -30.0);
   glVertex3f(100, 100, -30.0);
   glVertex3f(100, -100, -30.0);
   glEnd();

   glBegin(GL_POLYGON);
   glColor3f(0,1,1); 
   glVertex3f(-10, -10, -30.0);
   glVertex3f(-10, 10, -30.0);
   glVertex3f(10, 10, -30.0);
   glVertex3f(10, -10, -30.0);
   glEnd();

   glBegin(GL_POLYGON);
   glColor3f(1,0,0); 
   glVertex3f(-2, -2, -30.0);
   glVertex3f(-2, 2, -30.0);
   glVertex3f(2, 2, -30.0);
   glVertex3f(2, -2, -30.0);
   glEnd();


   glBegin(GL_POLYGON);
   glColor3f(.5,.5,1); 
   glVertex3f(-1, -1, -30.0);
   glVertex3f(-1, 1, -30.0);
   glVertex3f(1, 1, -30.0);
   glVertex3f(1, -1, -30.0);
   glEnd();

   /***********************************************************************
    */
   glViewport(0, 0, win_width, win_height);
   glBegin(GL_LINES);
   glColor3f(1,1,0); 
   glVertex3f(-1, 0, -30.0);
   glVertex3f(1, 0, -30.0);

   glVertex3f(0,  -1, -30.0);
   glVertex3f(0,  1, -30.0);
   glEnd();


   /***********************************************************************
    */
   glViewport(tx, ty, w, h);
   glBegin(GL_TRIANGLES);
   glColor3f(1,0,0); 
   glVertex3f(-1, -1, -30.0);
   glVertex3f(0, -1, -30.0);
   glVertex3f(-.5,  -.5, -30.0);

   glColor3f(1,1,1);
   glVertex3f(0, -1, -30.0);
   glVertex3f(1, -1, -30.0);
   glVertex3f(.5,  -.5, -30.0);

   glVertex3f(-.5, -.5, -30.0);
   glVertex3f(.5, -.5, -30.0);
   glVertex3f(0,  0, -30.0);


   glColor3f(0,1,0); 
   glVertex3f(1, 1, -30.0);
   glVertex3f(0, 1, -30.0);
   glVertex3f(.5,  .5, -30.0);

   glColor3f(1,1,1);
   glVertex3f(0, 1, -30.0);
   glVertex3f(-1, 1, -30.0);
   glVertex3f(-.5,  .5, -30.0);

   glVertex3f(.5, .5, -30.0);
   glVertex3f(-.5, .5, -30.0);
   glVertex3f( 0,  0, -30.0);

   glEnd();


   glViewport(0, 0, win_width, win_height);

   glBegin(GL_LINES);
   glColor3f(.5,.5,0); 
   for (i = -10; i < 10; i++) {
      float f = i / 10.0;

      if (i == 0)
         continue;

      glVertex3f(-1, f, -30.0);
      glVertex3f(1, f, -30.0);

      glVertex3f(f, -1, -30.0);
      glVertex3f(f, 1, -30.0);
   }
   glEnd();



   glFlush();

   if (doubleBuffer) {
      glutSwapBuffers();
   }
}

static GLenum Args(int argc, char **argv)
{
   GLint i;

   if (getenv("VPX"))
      tx = atof(getenv("VPX"));

   if (getenv("VPY"))
      ty = atof(getenv("VPY"));


   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-sb") == 0) {
         doubleBuffer = GL_FALSE;
      } else if (strcmp(argv[i], "-db") == 0) {
         doubleBuffer = GL_TRUE;
      } else {
         fprintf(stderr, "%s (Bad option).\n", argv[i]);
         return GL_FALSE;
      }
   }
   return GL_TRUE;
}


static void
special(int k, int x, int y)
{
   switch (k) {
   case GLUT_KEY_UP:
      ty += 1.0;
      break;
   case GLUT_KEY_DOWN:
      ty -= 1.0;
      break;
   case GLUT_KEY_LEFT:
      tx -= 1.0;
      break;
   case GLUT_KEY_RIGHT:
      tx += 1.0;
      break;
   default:
      break;
   }
   glutPostRedisplay();
}


int main(int argc, char **argv)
{
   GLenum type;

   glutInit(&argc, argv);

   if (Args(argc, argv) == GL_FALSE) {
      exit(1);
   }

   glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

   type = GLUT_RGB | GLUT_ALPHA;
   type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
   glutInitDisplayMode(type);

   win = glutCreateWindow(*argv);
   if (!win) {
      exit(1);
   }

   Init();

   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key); 
   glutSpecialFunc(special);
   glutDisplayFunc(Draw);
   glutMainLoop();
   return 0;
}
