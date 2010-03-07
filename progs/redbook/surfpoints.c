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
 *  surfpoints.c
 *  This program is a modification of the earlier surface.c
 *  program.  The vertex data are not directly rendered,
 *  but are instead passed to the callback function.  
 *  The values of the tessellated vertices are printed 
 *  out there.
 *
 *  This program draws a NURBS surface in the shape of a 
 *  symmetrical hill.  The 'c' keyboard key allows you to 
 *  toggle the visibility of the control points themselves.  
 *  Note that some of the control points are hidden by the  
 *  surface itself.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef GLU_VERSION_1_3

#ifndef CALLBACK
#define CALLBACK
#endif

GLfloat ctlpoints[4][4][3];
int showPoints = 0;

GLUnurbsObj *theNurb;

/*
 *  Initializes the control points of the surface to a small hill.
 *  The control points range from -3 to +3 in x, y, and z
 */
static void init_surface(void)
{
   int u, v;
   for (u = 0; u < 4; u++) {
      for (v = 0; v < 4; v++) {
         ctlpoints[u][v][0] = 2.0*((GLfloat)u - 1.5);
         ctlpoints[u][v][1] = 2.0*((GLfloat)v - 1.5);

         if ( (u == 1 || u == 2) && (v == 1 || v == 2))
            ctlpoints[u][v][2] = 3.0;
         else
            ctlpoints[u][v][2] = -3.0;
      }
   }				
}				

static void CALLBACK nurbsError(GLenum errorCode)
{
   const GLubyte *estring;

   estring = gluErrorString(errorCode);
   fprintf (stderr, "Nurbs Error: %s\n", estring);
   exit (0);
}

static void CALLBACK beginCallback(GLenum whichType)
{
   glBegin (whichType);  /*  resubmit rendering directive  */
   printf ("glBegin(");
   switch (whichType) {  /*  print diagnostic message  */
      case GL_LINES:
	 printf ("GL_LINES)\n");
	 break;
      case GL_LINE_LOOP:
	 printf ("GL_LINE_LOOP)\n");
	 break;
      case GL_LINE_STRIP:
	 printf ("GL_LINE_STRIP)\n");
	 break;
      case GL_TRIANGLES:
	 printf ("GL_TRIANGLES)\n");
	 break;
      case GL_TRIANGLE_STRIP:
	 printf ("GL_TRIANGLE_STRIP)\n");
	 break;
      case GL_TRIANGLE_FAN:
	 printf ("GL_TRIANGLE_FAN)\n");
	 break;
      case GL_QUADS:
	 printf ("GL_QUADS)\n");
	 break;
      case GL_QUAD_STRIP:
	 printf ("GL_QUAD_STRIP)\n");
	 break;
      case GL_POLYGON:
	 printf ("GL_POLYGON)\n");
	 break;
      default:
	 break;
   }
}

static void CALLBACK endCallback()
{
   glEnd();  /*  resubmit rendering directive  */
   printf ("glEnd()\n");
}

static void CALLBACK vertexCallback(GLfloat *vertex)
{
   glVertex3fv(vertex);  /*  resubmit rendering directive  */
   printf ("glVertex3f (%5.3f, %5.3f, %5.3f)\n", 
	   vertex[0], vertex[1], vertex[2]);
}

static void CALLBACK normalCallback(GLfloat *normal)
{
   glNormal3fv(normal);  /*  resubmit rendering directive  */
   printf ("glNormal3f (%5.3f, %5.3f, %5.3f)\n", 
           normal[0], normal[1], normal[2]);
}
			
/*  Initialize material property and depth buffer.
 */
static void init(void)
{
   GLfloat mat_diffuse[] = { 0.7, 0.7, 0.7, 1.0 };
   GLfloat mat_specular[] = { 1.0, 1.0, 1.0, 1.0 };
   GLfloat mat_shininess[] = { 100.0 };

   glClearColor (0.0, 0.0, 0.0, 0.0);
   glMaterialfv(GL_FRONT, GL_DIFFUSE, mat_diffuse);
   glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
   glMaterialfv(GL_FRONT, GL_SHININESS, mat_shininess);

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_AUTO_NORMAL);
   glEnable(GL_NORMALIZE);

   init_surface();

   theNurb = gluNewNurbsRenderer();
   gluNurbsProperty(theNurb, GLU_NURBS_MODE, 
		    GLU_NURBS_TESSELLATOR);
   gluNurbsProperty(theNurb, GLU_SAMPLING_TOLERANCE, 25.0);
   gluNurbsProperty(theNurb, GLU_DISPLAY_MODE, GLU_FILL);
   gluNurbsCallback(theNurb, GLU_ERROR, nurbsError);
   gluNurbsCallback(theNurb, GLU_NURBS_BEGIN, beginCallback);
   gluNurbsCallback(theNurb, GLU_NURBS_VERTEX, vertexCallback);
   gluNurbsCallback(theNurb, GLU_NURBS_NORMAL, normalCallback);
   gluNurbsCallback(theNurb, GLU_NURBS_END, endCallback);

}

static void display(void)
{
   GLfloat knots[8] = {0.0, 0.0, 0.0, 0.0, 1.0, 1.0, 1.0, 1.0};
   int i, j;

   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix();
   glRotatef(330.0, 1.,0.,0.);
   glScalef (0.5, 0.5, 0.5);

   gluBeginSurface(theNurb);
   gluNurbsSurface(theNurb, 
                   8, knots, 8, knots,
                   4 * 3, 3, &ctlpoints[0][0][0], 
                   4, 4, GL_MAP2_VERTEX_3);
   gluEndSurface(theNurb);

   if (showPoints) {
      glPointSize(5.0);
      glDisable(GL_LIGHTING);
      glColor3f(1.0, 1.0, 0.0);
      glBegin(GL_POINTS);
      for (i = 0; i < 4; i++) {
         for (j = 0; j < 4; j++) {
	    glVertex3f(ctlpoints[i][j][0], 
               ctlpoints[i][j][1], ctlpoints[i][j][2]);
         }
      }
      glEnd();
      glEnable(GL_LIGHTING);
   }
   glPopMatrix();
   glFlush();
}

static void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective (45.0, (GLdouble)w/(GLdouble)h, 3.0, 8.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef (0.0, 0.0, -5.0);
}

static void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 'c':
      case 'C':
         showPoints = !showPoints;
         glutPostRedisplay();
         break;
      case 27:
         exit(0);
         break;
      default:
         break;
   }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize (500, 500);
   glutInitWindowPosition (100, 100);
   glutCreateWindow(argv[0]);
   init();
   glutReshapeFunc(reshape);
   glutDisplayFunc(display);
   glutKeyboardFunc (keyboard);
   glutMainLoop();
   return 0; 
}

#else
int main(int argc, char** argv)
{
    fprintf (stderr, "This program demonstrates a feature which is introduced in the\n");
    fprintf (stderr, "OpenGL Utility Library (GLU) Version 1.3.\n");
    fprintf (stderr, "If your implementation of GLU has the right extensions,\n");
    fprintf (stderr, "you may be able to modify this program to make it run.\n");
    return 0;
}
#endif

