/*
 * Copyright (c) 1993-2003, Silicon Graphics, Inc.
 * All Rights Reserved
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose and without fee is hereby granted, provided that the above
 * copyright notice appear in all copies and that both the copyright
 * notice and this permission notice appear in supporting documentation,
 * and that the name of Silicon Graphics, Inc. not be used in
 * advertising or publicity pertaining to distribution of the software
 * without specific, written prior permission.
 *
 * THE MATERIAL EMBODIED ON THIS SOFTWARE IS PROVIDED TO YOU "AS-IS" AND
 * WITHOUT WARRANTY OF ANY KIND, EXPRESS, IMPLIED OR OTHERWISE,
 * INCLUDING WITHOUT LIMITATION, ANY WARRANTY OF MERCHANTABILITY OR
 * FITNESS FOR A PARTICULAR PURPOSE.  IN NO EVENT SHALL SILICON
 * GRAPHICS, INC.  BE LIABLE TO YOU OR ANYONE ELSE FOR ANY DIRECT,
 * SPECIAL, INCIDENTAL, INDIRECT OR CONSEQUENTIAL DAMAGES OF ANY KIND,
 * OR ANY DAMAGES WHATSOEVER, INCLUDING WITHOUT LIMITATION, LOSS OF
 * PROFIT, LOSS OF USE, SAVINGS OR REVENUE, OR THE CLAIMS OF THIRD
 * PARTIES, WHETHER OR NOT SILICON GRAPHICS, INC.  HAS BEEN ADVISED OF
 * THE POSSIBILITY OF SUCH LOSS, HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, ARISING OUT OF OR IN CONNECTION WITH THE POSSESSION, USE
 * OR PERFORMANCE OF THIS SOFTWARE.
 *
 * US Government Users Restricted Rights 
 * Use, duplication, or disclosure by the Government is subject to
 * restrictions set forth in FAR 52.227.19(c)(2) or subparagraph
 * (c)(1)(ii) of the Rights in Technical Data and Computer Software
 * clause at DFARS 252.227-7013 and/or in similar or successor clauses
 * in the FAR or the DOD or NASA FAR Supplement.  Unpublished - rights
 * reserved under the copyright laws of the United States.
 *
 * Contractor/manufacturer is:
 *	Silicon Graphics, Inc.
 *	1500 Crittenden Lane
 *	Mountain View, CA  94043
 *	United State of America
 *
 * OpenGL(R) is a registered trademark of Silicon Graphics, Inc.
 */

/*
 *  pointp.c
 *  This program demonstrates point parameters and their effect
 *  on point primitives.
 *  250 points are randomly generated within a 10 by 10 by 40
 *  region, centered at the origin.  In some modes (including the
 *  default), points that are closer to the viewer will appear larger.
 *
 *  Pressing the 'l', 'q', and 'c' keys switch the point
 *  parameters attenuation mode to linear, quadratic, or constant,
 *  respectively.  
 *  Pressing the 'f' and 'b' keys move the viewer forward 
 *  and backwards.  In either linear or quadratic attenuation
 *  mode, the distance from the viewer to the point will change
 *  the size of the point primitive.
 *  Pressing the '+' and '-' keys will change the current point
 *  size.  In this program, the point size is bounded, so it
 *  will not get less than 2.0, nor greater than GL_POINT_SIZE_MAX.
 */

#include <GL/glew.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

static GLfloat psize = 7.0;
static GLfloat pmax[1];
static GLfloat constant[3] = {1.0, 0.0, 0.0};
static GLfloat linear[3] = {0.0, 0.12, 0.0};
static GLfloat quadratic[3] = {0.0, 0.0, 0.01};

static void init(void) 
{
   int i;

   srand (12345);
   
   glNewList(1, GL_COMPILE);
   glBegin (GL_POINTS);
      for (i = 0; i < 250; i++) {
          glColor3f (1.0, ((rand()/(float) RAND_MAX) * 0.5) + 0.5, 
                          rand()/(float) RAND_MAX);
/*  randomly generated vertices:
    -5 < x < 5;  -5 < y < 5;  -5 < z < -45  */
          glVertex3f ( ((rand()/(float)RAND_MAX) * 10.0) - 5.0, 
                       ((rand()/(float)RAND_MAX) * 10.0) - 5.0, 
                       ((rand()/(float)RAND_MAX) * 40.0) - 45.0); 
      }
   glEnd();
   glEndList();

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_POINT_SMOOTH);
   glEnable(GL_BLEND);
   glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); 
   glPointSize(psize);
   glGetFloatv(GL_POINT_SIZE_MAX_EXT, pmax);

   glPointParameterfvEXT (GL_DISTANCE_ATTENUATION_EXT, linear);
   glPointParameterfEXT (GL_POINT_FADE_THRESHOLD_SIZE_EXT, 2.0);
}

static void display(void)
{
   glClear (GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glCallList (1);
   glutSwapBuffers ();
}

static void reshape (int w, int h)
{
   glViewport (0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode (GL_PROJECTION);
   glLoadIdentity ();
   gluPerspective (35.0, 1.0, 0.25, 200.0);
   glMatrixMode (GL_MODELVIEW);
   glTranslatef (0.0, 0.0, -10.0);
}

static void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 'b':
            glMatrixMode (GL_MODELVIEW);
            glTranslatef (0.0, 0.0, -0.5);
            glutPostRedisplay();
         break;
      case 'c':
            glPointParameterfvEXT (GL_DISTANCE_ATTENUATION_EXT, constant);
            glutPostRedisplay();
         break;
      case 'f':
            glMatrixMode (GL_MODELVIEW);
            glTranslatef (0.0, 0.0, 0.5);
            glutPostRedisplay();
         break;
      case 'l':
            glPointParameterfvEXT (GL_DISTANCE_ATTENUATION_EXT, linear);
            glutPostRedisplay();
         break;
      case 'q':
            glPointParameterfvEXT (GL_DISTANCE_ATTENUATION_EXT, quadratic);
            glutPostRedisplay();
         break;
      case '+':
            if (psize < (pmax[0] + 1.0))
               psize = psize + 1.0;
            glPointSize (psize);
            glutPostRedisplay();
         break;
      case '-':
            if (psize >= 2.0)
               psize = psize - 1.0;
            glPointSize (psize);
            glutPostRedisplay();
         break;
      case 27:
         exit(0);
         break;
   }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH | GLUT_MULTISAMPLE);
   glutInitWindowSize (500, 500); 
   glutInitWindowPosition (100, 100);
   glutCreateWindow (argv[0]);
   glewInit();
   init ();
   glutDisplayFunc (display); 
   glutReshapeFunc (reshape);
   glutKeyboardFunc (keyboard);
   glutMainLoop();
   return 0;
}
