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

/*  texture3d.c
 *  This program demonstrates using a three-dimensional texture.
 *  It creates a 3D texture and then renders two rectangles
 *  with different texture coordinates to obtain different
 *  "slices" of the 3D texture.
 */
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#ifdef GL_VERSION_1_2
#define	iWidth 16
#define	iHeight 16
#define iDepth 16

static GLubyte image[iDepth][iHeight][iWidth][3];
static GLuint texName;

/*  Create a 16x16x16x3 array with different color values in
 *  each array element [r, g, b].  Values range from 0 to 255.
 */

static void makeImage(void)
{
   int s, t, r;
    
   for (s = 0; s < 16; s++)
      for (t = 0; t < 16; t++)
         for (r = 0; r < 16; r++) {
            image[r][t][s][0] = (GLubyte) (s * 17);
            image[r][t][s][1] = (GLubyte) (t * 17);
            image[r][t][s][2] = (GLubyte) (r * 17);
         }
}

static void init(void)
{    
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_FLAT);
   glEnable(GL_DEPTH_TEST);

   makeImage();
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glGenTextures(1, &texName);
   glBindTexture(GL_TEXTURE_3D, texName);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, 
                   GL_NEAREST);
   glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, 
                   GL_NEAREST);
   glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, iWidth, iHeight,
                iDepth, 0, GL_RGB, GL_UNSIGNED_BYTE, image);
   glEnable(GL_TEXTURE_3D);
}

static void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
   glBegin(GL_QUADS);
   glTexCoord3f(0.0, 0.0, 0.0); glVertex3f(-2.25, -1.0, 0.0);
   glTexCoord3f(0.0, 1.0, 0.0); glVertex3f(-2.25, 1.0, 0.0);
   glTexCoord3f(1.0, 1.0, 1.0); glVertex3f(-0.25, 1.0, 0.0);
   glTexCoord3f(1.0, 0.0, 1.0); glVertex3f(-0.25, -1.0, 0.0);

   glTexCoord3f(0.0, 0.0, 1.0); glVertex3f(0.25, -1.0, 0.0);
   glTexCoord3f(0.0, 1.0, 1.0); glVertex3f(0.25, 1.0, 0.0);
   glTexCoord3f(1.0, 1.0, 0.0); glVertex3f(2.25, 1.0, 0.0);
   glTexCoord3f(1.0, 0.0, 0.0); glVertex3f(2.25, -1.0, 0.0);
   glEnd();
   glFlush();
}

static void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(60.0, (GLfloat) w/(GLfloat) h, 1.0, 30.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -4.0);
}

static void keyboard(unsigned char key, int x, int y)
{
   switch (key) {
      case 27:
         exit(0);
         break;
   }
}

int main(int argc, char** argv)
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(250, 250);
   glutInitWindowPosition(100, 100);
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
    fprintf (stderr, "This program demonstrates a feature which is not in OpenGL Version 1.0 or 1.1.\n");
    fprintf (stderr, "If your implementation of OpenGL has the right extensions,\n");
    fprintf (stderr, "you may be able to modify this program to make it run.\n");
    return 0;
}
#endif

