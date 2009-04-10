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

#include <math.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <GL/glut.h>


#define CI_OFFSET_1 16
#define CI_OFFSET_2 32


GLenum doubleBuffer;
GLboolean smooth = GL_TRUE;
GLfloat width = 1.0;


static void Init(void)
{
   float range[2], aarange[2];
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);
   glGetFloatv(GL_LINE_WIDTH_RANGE, aarange);
   glGetFloatv(GL_ALIASED_LINE_WIDTH_RANGE, range);
   printf("Non-AA line width range: %f .. %f\n", range[0], range[1]);
   printf("AA line width range: %f .. %f\n", aarange[0], aarange[1]);
   fflush(stdout);
}


static void Reshape(int width, int height)
{
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0, 0, -30);
}

static void Key(unsigned char key, int x, int y)
{
   switch (key) {
   case 'w':
      width -= 0.5;
      if (width < 0.5)
         width = 0.5;
      break;
   case 'W':
      width += 0.5;
      break;
   case 's':
      smooth = !smooth;
      break;
   case 27:
      exit(1);
   default:
      return;
   }
   printf("LineWidth: %g\n", width);
   glutPostRedisplay();
}


static void Draw(void)
{
   float a;

   glClear(GL_COLOR_BUFFER_BIT); 

   glLineWidth(width);

   if (smooth) {
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
      glEnable(GL_BLEND);
      glEnable(GL_LINE_SMOOTH);
   }

   glColor3f(1, 1, 1);

   glBegin(GL_LINES);
   for (a = 0; a < 3.14159; a += 0.2) {
      float x = .9 * cos(a);
      float y = .9 * sin(a);

      glVertex2f(-x, -y);
      glVertex2f( x,  y);
   }
   glEnd();

   glDisable(GL_LINE_SMOOTH);
   glDisable(GL_BLEND);

   glFlush();

   if (doubleBuffer) {
      glutSwapBuffers();
   }
}

static GLenum Args(int argc, char **argv)
{
   GLint i;

   doubleBuffer = GL_FALSE;

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


int main(int argc, char **argv)
{
   GLenum type;

   glutInit(&argc, argv);

   if (Args(argc, argv) == GL_FALSE) {
      exit(1);
   }

   glutInitWindowPosition(0, 0); glutInitWindowSize( 250, 250);

   type = GLUT_RGB;
   type |= (doubleBuffer) ? GLUT_DOUBLE : GLUT_SINGLE;
   glutInitDisplayMode(type);

   if (glutCreateWindow(argv[0]) == GL_FALSE) {
      exit(1);
   }

   Init();

   glutReshapeFunc(Reshape);
   glutKeyboardFunc(Key);
   glutDisplayFunc(Draw);
   glutMainLoop();
   return 0;
}
