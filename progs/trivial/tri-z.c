/*
 * Copyright (c) 1993-1997, Silicon Graphics, Inc.
 * ALL RIGHTS RESERVED 
 * Permission to use, copy, modify, and distribute this software for 
 * any purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright notice
 * and this permission notice appear in supporting documentation, and that 
 * the name of Silicon Graphics, Inc. not be used in advertising
 * or publicity pertaining to distribution of the software without specific,
 * written prior permission. 
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS"
 * AND WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY
 * KIND, OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION,
 * LOSS OF PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF
 * THIRD PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN
 * ADVISED OF THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON
 * ANY THEORY OF LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE
 * POSSESSION, USE OR PERFORMANCE OF THIS SOFTWARE.
 * 
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor
 * clauses in the FAR or the DOD or NASA FAR Supplement.
 * Unpublished-- rights reserved under the copyright laws of the
 * United States.  Contractor/manufacturer is Silicon Graphics,
 * Inc., 2011 N.  Shoreline Blvd., Mountain View, CA 94039-7311.
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

static int leftFirst = GL_TRUE;

static struct { GLenum func; const char *str; } funcs[] = 
   {
      { GL_LESS, "GL_LESS" },
      { GL_LEQUAL, "GL_LEQUAL" },
      { GL_GREATER, "GL_GREATER" },
      { GL_GEQUAL, "GL_GEQUAL" },
      { GL_EQUAL, "GL_EQUAL" },
      { GL_NOTEQUAL, "GL_NOTEQUAL" },
      { GL_ALWAYS, "GL_ALWAYS" },
      { GL_NEVER, "GL_NEVER" },
   };

#define NUM_FUNCS (sizeof(funcs) / sizeof(funcs[0]))

static int curFunc = 0;
static double clearVal = 1.0;
static float minZ = 0.0;
static float maxZ = 1.0;

static void usage(void)
{
   printf("t   - toggle rendering order of triangles\n");
   printf("c   - toggle Z clear value between 0, 1\n");
   printf("f   - cycle through depth test functions\n");
   printf("n/N - decrease/increase depthrange minZ\n");
   printf("x/X - decrease/increase depthrange maxZ\n");
   printf("spc - reset\n");
   printf("z   - set to reverse-direction (ztrick) mode\n");
   fflush(stdout);
}


static void init(void)
{
   glEnable(GL_DEPTH_TEST);
   glClearColor (1.0, 0.0, 0.0, 0.0);
}

static void drawLeftTriangle(void)
{
   /* draw yellow triangle on LHS of screen */
   glBegin (GL_TRIANGLES);
      glColor4f(1.0, 1.0, 0.0, 0.75);
      glVertex3f(0.1, 0.9, -1.0); 
      glVertex3f(0.1, 0.1, -1.0); 
      glVertex3f(0.8, 0.5,  1.0); 
   glEnd();
}

static void drawRightTriangle(void)
{
   /* draw cyan triangle on RHS of screen */
   glBegin (GL_TRIANGLES);
      glColor4f(0.0, 1.0, 1.0, 0.75);
      glVertex3f(0.9, 0.9, 0.0); 
      glVertex3f(0.2, 0.5, 0.0); 
      glVertex3f(0.9, 0.1, 0.0); 
   glEnd();
}

void display(void)
{
   printf("GL_CLEAR_DEPTH = %.2f,  GL_DEPTH_FUNC = %s,  DepthRange(%.1f, %.1f)\n",
          clearVal, funcs[curFunc].str, minZ, maxZ);
   fflush(stdout);
   glClearDepth(clearVal);
   glDepthRange(minZ, maxZ);
   glDepthFunc(funcs[curFunc].func);

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   if (leftFirst) {
      drawLeftTriangle();
      drawRightTriangle();
   }
   else {
      drawRightTriangle();
      drawLeftTriangle();
   }

   glFlush();
}

void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   if (w <= h) 
      gluOrtho2D (0.0, 1.0, 0.0, 1.0*(GLfloat)h/(GLfloat)w);
   else 
      gluOrtho2D (0.0, 1.0*(GLfloat)w/(GLfloat)h, 0.0, 1.0);
}

/* ARGSUSED1 */
void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 'n':
         minZ -= .1;
         break;
      case 'N':
         minZ += .1;
         break;
      case 'x':
         maxZ -= .1;
         break;
      case 'X':
         maxZ += .1;
         break;
      case 'c':
      case 'C':
         clearVal = 1.0 - clearVal;
         break;
      case 'f':
      case 'F':
         curFunc = (curFunc + 1) % NUM_FUNCS;
         break;
      case 't':
      case 'T':
         leftFirst = !leftFirst;
         break;
      case ' ':
         curFunc = 0;
         clearVal = 1.0;
         minZ = 0.0;
         maxZ = 1.0;
         break;
      case 'z':
         curFunc = 2;
         clearVal = 0.0;
         minZ = 1.0;
         maxZ = 0.0;
         break;
      case 27:  /*  Escape key  */
         exit(0);
         break;
      default:
         return;
   }
   glutPostRedisplay();	
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize (200, 200);
   glutCreateWindow (argv[0]);
   glutReshapeFunc (reshape);
   glutKeyboardFunc (keyboard);
   glutDisplayFunc (display);
   init();
   usage();
   glutMainLoop();
   return 0;
}
