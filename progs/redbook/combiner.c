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

/*  combiner.c
 *  This program renders a variety of quads showing different
 *  effects of texture combiner functions.
 *
 *  The first row renders an untextured polygon (so you can 
 *  compare the fragment colors) and then the 2 textures.
 *  The second row shows several different combiner functions
 *  on a single texture:  replace, modulate, add, add-signed,
 *  and subtract.  
 *  The third row shows the interpolate combiner function
 *  on a single texture with a constant color/alpha value,
 *  varying the amount of interpolation.  
 *  The fourth row uses multitexturing with two textures
 *  and different combiner functions.
 *  The fifth row are some combiner experiments:  using the
 *  scaling factor and reversing the order of subtraction
 *  for a combination function.
 */
#include <GL/glew.h>
#include <GL/glut.h>
#include <stdlib.h>
#include <stdio.h>

#define	imageWidth 8
#define	imageHeight 8
/*     arrays for two textures	    */
static GLubyte image0[imageHeight][imageWidth][4];
static GLubyte image1[imageHeight][imageWidth][4];

static GLuint texName[4];

static void makeImages(void)
{
   int i, j, c;
   for (i = 0; i < imageHeight; i++) {
      for (j = 0; j < imageWidth; j++) {
         c = ((i&2)==0)*255;   /*  horiz b & w stripes  */
         image0[i][j][0] = (GLubyte) c;
         image0[i][j][1] = (GLubyte) c;
         image0[i][j][2] = (GLubyte) c;
         image0[i][j][3] = (GLubyte) 255;
         c = ((j&4)!=0)*128;   /*  wider vertical 50% cyan and black stripes  */
         image1[i][j][0] = (GLubyte) 0;
         image1[i][j][1] = (GLubyte) c;
         image1[i][j][2] = (GLubyte) c;
         image1[i][j][3] = (GLubyte) 255;
      }
   }
}

static void init(void)
{    
   glClearColor (0.0, 0.0, 0.0, 0.0);
   glShadeModel(GL_SMOOTH);

   makeImages();
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   glGenTextures(4, texName);

   glBindTexture(GL_TEXTURE_2D, texName[0]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 
                0, GL_RGBA, GL_UNSIGNED_BYTE, image0);

   glBindTexture(GL_TEXTURE_2D, texName[1]);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, imageWidth, imageHeight, 
                0, GL_RGBA, GL_UNSIGNED_BYTE, image1);

/*  smooth-shaded polygon with multiple texture coordinates  */
   glNewList (1, GL_COMPILE);
   glBegin(GL_QUADS);
   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 0.0); 
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 0.0); 
   glColor3f (0.5, 1.0, 0.25);
   glVertex3f(0.0, 0.0, 0.0);
   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 0.0, 2.0); 
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 2.0); 
   glColor3f (1.0, 1.0, 1.0);
   glVertex3f(0.0, 1.0, 0.0);
   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 2.0, 2.0); 
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 2.0, 2.0); 
   glColor3f (1.0, 1.0, 1.0);
   glVertex3f(1.0, 1.0, 0.0);
   glMultiTexCoord2fARB(GL_TEXTURE0_ARB, 2.0, 0.0); 
   glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 2.0, 0.0); 
   glColor3f (1.0, 0.5, 0.25);
   glVertex3f(1.0, 0.0, 0.0);
   glEnd();
   glEndList ();
}

static void display(void)
{
   static GLfloat constColor[4] = {0.0, 0.0, 0.0, 0.0};  /*  for use as constant texture color  */

   glClear(GL_COLOR_BUFFER_BIT);

   glDisable(GL_TEXTURE_2D);  /*  untextured polygon--see the "fragment" colors  */
   glPushMatrix();
   glTranslatef(0.0, 5.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glEnable(GL_TEXTURE_2D);
/*  draw ordinary textured polys; 1 texture unit; combine mode disabled  */
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   glPushMatrix();
   glBindTexture(GL_TEXTURE_2D, texName[0]);
   glTranslatef(1.0, 5.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glPushMatrix();
   glBindTexture(GL_TEXTURE_2D, texName[1]);
   glTranslatef(2.0, 5.0, 0.0);
   glCallList(1);
   glPopMatrix();

/*  different combine modes enabled; 1 texture unit
 *  defaults are:
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
 */
   glBindTexture(GL_TEXTURE_2D, texName[0]);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
   glPushMatrix();
   glTranslatef(1.0, 4.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
   glPushMatrix();
   glTranslatef(2.0, 4.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
   glPushMatrix();
   glTranslatef(3.0, 4.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
   glPushMatrix();
   glTranslatef(4.0, 4.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_SUBTRACT_ARB);
   glPushMatrix();
   glTranslatef(5.0, 4.0, 0.0);
   glCallList(1);
   glPopMatrix();

/*  interpolate combine with constant color; 1 texture unit 
 *  use different alpha values for constant color
 *  defaults are:
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
 */
   constColor[3] = 0.2;
   glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constColor);
   glBindTexture(GL_TEXTURE_2D, texName[0]);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_INTERPOLATE_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE2_RGB_ARB, GL_CONSTANT_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND2_RGB_ARB, GL_SRC_ALPHA);
   glPushMatrix();
   glTranslatef(1.0, 3.0, 0.0);
   glCallList(1);
   glPopMatrix();

   constColor[3] = 0.4;
   glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constColor);
   glPushMatrix();
   glTranslatef(2.0, 3.0, 0.0);
   glCallList(1);
   glPopMatrix();

   constColor[3] = 0.6;
   glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constColor);
   glPushMatrix();
   glTranslatef(3.0, 3.0, 0.0);
   glCallList(1);
   glPopMatrix();

   constColor[4] = 0.8;
   glTexEnvfv(GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, constColor);
   glPushMatrix();
   glTranslatef(4.0, 3.0, 0.0);
   glCallList(1);
   glPopMatrix();

/*  combine textures 0 & 1
 *  defaults are:
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_TEXTURE);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_PREVIOUS_ARB);
 *  glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
 */

   glActiveTextureARB (GL_TEXTURE0_ARB);
   glEnable (GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texName[0]);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glActiveTextureARB (GL_TEXTURE1_ARB);
   glEnable (GL_TEXTURE_2D);
   glBindTexture(GL_TEXTURE_2D, texName[1]);
   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
   glPushMatrix();
   glTranslatef(1.0, 2.0, 0.0);
   glCallList(1);
   glPopMatrix();

   /*  try different combiner modes of texture unit 1  */
   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
   glPushMatrix();
   glTranslatef(2.0, 2.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD);
   glPushMatrix();
   glTranslatef(3.0, 2.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_ADD_SIGNED_ARB);
   glPushMatrix();
   glTranslatef(4.0, 2.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_SUBTRACT_ARB);
   glPushMatrix();
   glTranslatef(5.0, 2.0, 0.0);
   glCallList(1);
   glPopMatrix();

/*  some experiments */

/*  see the effect of RGB_SCALE   */
   glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 2.0);
   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_REPLACE);
   glPushMatrix();
   glTranslatef(1.0, 1.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_MODULATE);
   glPushMatrix();
   glTranslatef(2.0, 1.0, 0.0);
   glCallList(1);
   glPopMatrix();
   glTexEnvf(GL_TEXTURE_ENV, GL_RGB_SCALE_ARB, 1.0);

/*  using SOURCE0 and SOURCE1, reverse the order of subtraction Arg1-Arg0  */

   glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_COMBINE_RGB_ARB, GL_SUBTRACT_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE0_RGB_ARB, GL_PREVIOUS_ARB);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND0_RGB_ARB, GL_SRC_COLOR);
   glTexEnvf(GL_TEXTURE_ENV, GL_SOURCE1_RGB_ARB, GL_TEXTURE);
   glTexEnvf(GL_TEXTURE_ENV, GL_OPERAND1_RGB_ARB, GL_SRC_COLOR);
   glPushMatrix();
   glTranslatef(5.0, 1.0, 0.0);
   glCallList(1);
   glPopMatrix();

   glActiveTextureARB (GL_TEXTURE1_ARB);  /*  deactivate multitexturing  */
   glDisable (GL_TEXTURE_2D);
   glActiveTextureARB (GL_TEXTURE0_ARB);  /*  activate single texture unit  */

   glFlush();
}

static void reshape(int w, int h)
{
   glViewport(0, 0, (GLsizei) w, (GLsizei) h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluOrtho2D(0.0, 7.0, 0.0, 7.0);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}

static void keyboard (unsigned char key, int x, int y)
{
   switch (key) {
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
   glutInitDisplayMode(GLUT_SINGLE | GLUT_RGB);
   glutInitWindowSize(400, 400);
   glutInitWindowPosition(100, 100);
   glutCreateWindow(argv[0]);
   glewInit();
   init();
   glutDisplayFunc(display);
   glutReshapeFunc(reshape);
   glutKeyboardFunc(keyboard);
   glutMainLoop();
   return 0; 
}
