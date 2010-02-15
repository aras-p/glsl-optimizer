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

/*  cubemap.c
 *
 *  This program demonstrates cube map textures.
 *  Six different colored checker board textures are
 *  created and applied to a lit sphere.
 *
 *  Pressing the 'f' and 'b' keys translate the viewer
 *  forward and backward.
 */

#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#define	imageSize 4
static GLubyte image1[imageSize][imageSize][4];
static GLubyte image2[imageSize][imageSize][4];
static GLubyte image3[imageSize][imageSize][4];
static GLubyte image4[imageSize][imageSize][4];
static GLubyte image5[imageSize][imageSize][4];
static GLubyte image6[imageSize][imageSize][4];

static GLdouble ztrans = 0.0;

static void makeImages(void)
{
   int i, j, c;
    
   for (i = 0; i < imageSize; i++) {
      for (j = 0; j < imageSize; j++) {
         c = ( ((i & 0x1) == 0) ^ ((j & 0x1) == 0) ) * 255;
         image1[i][j][0] = (GLubyte) c;
         image1[i][j][1] = (GLubyte) c;
         image1[i][j][2] = (GLubyte) c;
         image1[i][j][3] = (GLubyte) 255;

         image2[i][j][0] = (GLubyte) c;
         image2[i][j][1] = (GLubyte) c;
         image2[i][j][2] = (GLubyte) 0;
         image2[i][j][3] = (GLubyte) 255;

         image3[i][j][0] = (GLubyte) c;
         image3[i][j][1] = (GLubyte) 0;
         image3[i][j][2] = (GLubyte) c;
         image3[i][j][3] = (GLubyte) 255;

         image4[i][j][0] = (GLubyte) 0;
         image4[i][j][1] = (GLubyte) c;
         image4[i][j][2] = (GLubyte) c;
         image4[i][j][3] = (GLubyte) 255;

         image5[i][j][0] = (GLubyte) 255;
         image5[i][j][1] = (GLubyte) c;
         image5[i][j][2] = (GLubyte) c;
         image5[i][j][3] = (GLubyte) 255;

         image6[i][j][0] = (GLubyte) c;
         image6[i][j][1] = (GLubyte) c;
         image6[i][j][2] = (GLubyte) 255;
         image6[i][j][3] = (GLubyte) 255;
      }
   }
}

static void init(void)
{
   GLfloat diffuse[4] = {1.0, 1.0, 1.0, 1.0};

   glClearColor (0.0, 0.0, 0.0, 0.0);
   glEnable(GL_DEPTH_TEST);
   glShadeModel(GL_SMOOTH);

   makeImages();
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_WRAP_R, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_CUBE_MAP_EXT, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X_EXT, 0, GL_RGBA, imageSize, 
                imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image1);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X_EXT, 0, GL_RGBA, imageSize, 
                imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image4);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y_EXT, 0, GL_RGBA, imageSize,
                imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image2);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_EXT, 0, GL_RGBA, imageSize,
                imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image5);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z_EXT, 0, GL_RGBA, imageSize,
                imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image3);
   glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_EXT, 0, GL_RGBA, imageSize,
                imageSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, image6);
   glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
   glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT);
   glTexGeni(GL_R, GL_TEXTURE_GEN_MODE, GL_NORMAL_MAP_EXT); 
   glEnable(GL_TEXTURE_GEN_S);
   glEnable(GL_TEXTURE_GEN_T);
   glEnable(GL_TEXTURE_GEN_R);

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glEnable(GL_TEXTURE_CUBE_MAP_EXT);   
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glEnable(GL_AUTO_NORMAL);
   glEnable(GL_NORMALIZE);
   glMaterialfv (GL_FRONT, GL_DIFFUSE, diffuse);
}

static void display(void)
{
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   glPushMatrix ();
   glTranslatef (0.0, 0.0, ztrans);
   glutSolidSphere (5.0, 20, 10);
   glPopMatrix ();
   glutSwapBuffers();
}

static void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(40.0, (GLfloat) w/(GLfloat) h, 1.0, 300.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef(0.0, 0.0, -20.0);
}

static void keyboard (unsigned char key, int x, int y)
{
   switch (key) {
      case 'f':
         ztrans = ztrans - 0.2;
         glutPostRedisplay();
         break;
      case 'b':
         ztrans = ztrans + 0.2;
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
   glutInitDisplayMode (GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH);
   glutInitWindowSize(400, 400);
   glutInitWindowPosition(100, 100);
   glutCreateWindow (argv[0]);
   init ();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMainLoop();
   return 0;
}
