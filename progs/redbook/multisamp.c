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
 *  multisamp.c
 *  This program draws shows how to use multisampling to 
 *  draw anti-aliased geometric primitives.  The same
 *  display list, a pinwheel of triangles and lines of
 *  varying widths, is rendered twice.  Multisampling is 
 *  enabled when the left side is drawn.  Multisampling is
 *  disabled when the right side is drawn.
 *
 *  Pressing the 'b' key toggles drawing of the checkerboard
 *  background.  Antialiasing is sometimes easier to see
 *  when objects are rendered over a contrasting background.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

static int bgtoggle = 1;

/*  
 *  Print out state values related to multisampling.
 *  Create display list with "pinwheel" of lines and
 *  triangles. 
 */
static void init(void)
{
   static GLint buf[1], sbuf[1];
   int i, j;

   glClearColor(0.0, 0.0, 0.0, 0.0);
   glGetIntegerv (GL_SAMPLE_BUFFERS_ARB, buf);
   printf ("number of sample buffers is %d\n", buf[0]);
   glGetIntegerv (GL_SAMPLES_ARB, sbuf);
   printf ("number of samples is %d\n", sbuf[0]);

   glNewList (1, GL_COMPILE);
   for (i = 0; i < 19; i++) {
      glPushMatrix();
      glRotatef(360.0*(float)i/19.0, 0.0, 0.0, 1.0);
      glColor3f (1.0, 1.0, 1.0);
      glLineWidth((i%3)+1.0);
      glBegin (GL_LINES);
         glVertex2f (0.25, 0.05);
         glVertex2f (0.9, 0.2);
      glEnd ();
      glColor3f (0.0, 1.0, 1.0);
      glBegin (GL_TRIANGLES);
         glVertex2f (0.25, 0.0);
         glVertex2f (0.9, 0.0);
         glVertex2f (0.875, 0.10);
      glEnd ();
      glPopMatrix();
   }
   glEndList ();

   glNewList (2, GL_COMPILE);
   glColor3f (1.0, 0.5, 0.0);
   glBegin (GL_QUADS);
   for (i = 0; i < 16; i++) {
      for (j = 0; j < 16; j++) {
         if (((i + j) % 2) == 0) {
            glVertex2f (-2.0 + (i * 0.25), -2.0 + (j * 0.25));
            glVertex2f (-2.0 + (i * 0.25), -1.75 + (j * 0.25));
            glVertex2f (-1.75 + (i * 0.25), -1.75 + (j * 0.25));
            glVertex2f (-1.75 + (i * 0.25), -2.0 + (j * 0.25));
         }
      }
   }
   glEnd ();
   glEndList ();
}

/*  Draw two sets of primitives, so that you can 
 *  compare the user of multisampling against its absence.
 *
 *  This code enables antialiasing and draws one display list
 *  and disables and draws the other display list
 */
static void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT);

   if (bgtoggle) 
      glCallList (2);

   glEnable (GL_MULTISAMPLE_ARB);
   glPushMatrix();
   glTranslatef (-1.0, 0.0, 0.0);
   glCallList (1);
   glPopMatrix();

   glDisable (GL_MULTISAMPLE_ARB);
   glPushMatrix();
   glTranslatef (1.0, 0.0, 0.0);
   glCallList (1);
   glPopMatrix();
   glutSwapBuffers();
}

static void reshape(int w, int h)
{
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   if (w <= (2 * h)) 
      gluOrtho2D (-2.0, 2.0, 
         -2.0*(GLfloat)h/(GLfloat)w, 2.0*(GLfloat)h/(GLfloat)w);
   else 
      gluOrtho2D (-2.0*(GLfloat)w/(GLfloat)h, 
         2.0*(GLfloat)w/(GLfloat)h, -2.0, 2.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

static void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 'b':
      case 'B':
         bgtoggle = !bgtoggle;
         glutPostRedisplay();
         break;
      case 27:  /*  Escape Key  */
         exit(0);
      default:
         break;
    }
}

/*  Main Loop
 *  Open window with initial window size, title bar, 
 *  RGBA display mode, and handle input events.
 */
int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_MULTISAMPLE);
   glutInitWindowSize (600, 300);
   glutCreateWindow (argv[0]);
   init();
   glutReshapeFunc (reshape);
   glutKeyboardFunc (keyboard);
   glutDisplayFunc (display);
   glutMainLoop();
   return 0;
}
