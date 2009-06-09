/*
 * Copyright (c) 1991, 1992, 1993 Silicon Graphics, Inc.
 * Copyright (c) 2009 VMware, Inc.
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

struct program
{
   unsigned width;
   unsigned height;
   int i;
};

struct program prog;

static void init(void)
{
   fprintf(stderr, "GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
   fprintf(stderr, "GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
   fprintf(stderr, "GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
   fflush(stderr);

   prog.i = 0;
}

static void reshape(int width, int height)
{
   glViewport(0, 0, 100, 100);

   prog.width = width;
   prog.height = height;

   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glOrtho(-1.0, 1.0, -1.0, 1.0, -0.5, 1000.0);
   glMatrixMode(GL_MODELVIEW);
}

static void key(unsigned char key, int x, int y)
{

   switch (key) {
   case 27:
      exit(1);
   default:
      glutPostRedisplay();
      return;
   }
}

static void drawQuad(void)
{
   glBegin(GL_QUADS);
   glVertex2d(-1.0, -1.0);
   glVertex2d( 1.0, -1.0);
   glVertex2d( 1.0,  1.0);
   glVertex2d(-1.0,  1.0);
   glEnd();
}

static void draw(void)
{
   int i;

   glClearColor(0.0, 0.0, 1.0, 0.0);
   glClear(GL_COLOR_BUFFER_BIT);

   i = prog.i++;
   if (prog.i >= 3)
      prog.i = 0;

   glEnable(GL_SCISSOR_TEST);

   {
      glColor4d(1.0, 0.0, 0.0, 1.0);

      glScissor(i, i, 10 - 2*i, 10 - 2*i);
      drawQuad();
   }

   glDisable(GL_SCISSOR_TEST);

   //glutSwapBuffers();
   glFlush();
}

int main(int argc, char **argv)
{
   GLenum type;

   glutInit(&argc, argv);

   prog.width = 200;
   prog.height = 200;

   glutInitWindowPosition(100, 0);
   glutInitWindowSize(prog.width, prog.height);

   //type = GLUT_RGB | GLUT_DOUBLE;
   type = GLUT_RGB | GLUT_SINGLE;
   glutInitDisplayMode(type);

   if (glutCreateWindow(*argv) == GL_FALSE) {
      exit(1);
   }

   init();

   glutReshapeFunc(reshape);
   glutKeyboardFunc(key);
   glutDisplayFunc(draw);
   glutMainLoop();
   return 0;
}
