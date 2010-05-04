/*
 * Copyright (C) 2000  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file bug_3195.c
 *
 * Simple regression test for bug #3195.  A bug in the i180 driver caused
 * a segfault (inside the driver) when the LOD bias is adjusted and no texture
 * is enabled.  This test, which is based on progs/demos/lodbias.c, sets up
 * all the texturing, disables all textures, adjusts the LOD bias, then
 * re-enables \c GL_TEXTURE_2D.
 *
 * \author Brian Paul
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "readtex.h"

#define TEXTURE_FILE "../images/girl.rgb"

static GLfloat Xrot = 0, Yrot = -30, Zrot = 0;
static GLint Bias = 0, BiasStepSign = +1; /* ints avoid fp precision problem */
static GLint BiasMin = -400, BiasMax = 400;



static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}

static void Idle( void )
{
   static int lastTime = 0;
   int time = glutGet(GLUT_ELAPSED_TIME);
   int step;

   if (lastTime == 0)
      lastTime = time;
   else if (time - lastTime < 10)
      return;

   step = (time - lastTime) / 10 * BiasStepSign;
   lastTime = time;

   Bias += step;
   if (Bias < BiasMin) {
      exit(0);
   }
   else if (Bias > BiasMax) {
      Bias = BiasMax;
      BiasStepSign = -1;
   }

   glutPostRedisplay();
}


static void Display( void )
{
   char str[100];

   glClear( GL_COLOR_BUFFER_BIT );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(-1, 1, -1, 1, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   
   glDisable(GL_TEXTURE_2D);
   glColor3f(1,1,1);
   glRasterPos3f(-0.9, -0.9, 0.0);
   sprintf(str, "Texture LOD Bias = %4.1f", Bias * 0.01);
   PrintString(str);

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -8.0 );

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);
   glRotatef(Zrot, 0, 0, 1);

   glTexEnvf(GL_TEXTURE_FILTER_CONTROL_EXT, GL_TEXTURE_LOD_BIAS_EXT, 0.01 * Bias);
   glEnable(GL_TEXTURE_2D);

   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);  glVertex2f(-1, -1);
   glTexCoord2f(2, 0);  glVertex2f( 1, -1);
   glTexCoord2f(2, 2);  glVertex2f( 1,  1);
   glTexCoord2f(0, 2);  glVertex2f(-1,  1);
   glEnd();

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   const GLfloat step = 3.0;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Xrot -= step;
         break;
      case GLUT_KEY_DOWN:
         Xrot += step;
         break;
      case GLUT_KEY_LEFT:
         Yrot -= step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot += step;
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   GLfloat maxBias;
   const char * const ver_string = (const char *)
       glGetString( GL_VERSION );

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   printf("\nThis program should function nearly identically to Mesa's lodbias demo.\n"
	  "It should cycle through the complet LOD bias range once and exit.  If bug\n"
	  "#3195 still exists, the demo should crash almost immediatly.\n");
   printf("This is a regression test for bug #3195.\n");
   printf("https://bugs.freedesktop.org/show_bug.cgi?id=3195\n");

   if (!glutExtensionSupported("GL_EXT_texture_lod_bias")) {
      printf("Sorry, GL_EXT_texture_lod_bias not supported by this renderer.\n");
      exit(1);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   if (glutExtensionSupported("GL_SGIS_generate_mipmap")) {
      /* test auto mipmap generation */
      GLint width, height, i;
      GLenum format;
      GLubyte *image = LoadRGBImage(TEXTURE_FILE, &width, &height, &format);
      if (!image) {
         printf("Error: could not load texture image %s\n", TEXTURE_FILE);
         exit(1);
      }
      /* resize to 256 x 256 */
      if (width != 256 || height != 256) {
         GLubyte *newImage = malloc(256 * 256 * 4);
         gluScaleImage(format, width, height, GL_UNSIGNED_BYTE, image,
                       256, 256, GL_UNSIGNED_BYTE, newImage);
         free(image);
         image = newImage;
      }
      printf("Using GL_SGIS_generate_mipmap\n");
      glTexParameteri(GL_TEXTURE_2D, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
      glTexImage2D(GL_TEXTURE_2D, 0, format, 256, 256, 0,
                   format, GL_UNSIGNED_BYTE, image);
      free(image);

      /* make sure mipmap was really generated correctly */
      width = height = 256;
      for (i = 0; i < 9; i++) {
         GLint w, h;
         glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_WIDTH, &w);
         glGetTexLevelParameteriv(GL_TEXTURE_2D, i, GL_TEXTURE_HEIGHT, &h);
         printf("Level %d size: %d x %d\n", i, w, h);
         assert(w == width);
         assert(h == height);
         width /= 2;
         height /= 2;
      }

   }
   else if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
      printf("Error: could not load texture image %s\n", TEXTURE_FILE);
      exit(1);
   }

   /* mipmapping required for this extension */
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   glGetFloatv(GL_MAX_TEXTURE_LOD_BIAS_EXT, &maxBias);
   printf("LOD bias range: [%g, %g]\n", -maxBias, maxBias);
   BiasMin = -100 * maxBias;
   BiasMax =  100 * maxBias;

   /* Since we have (about) 8 mipmap levels, no need to bias beyond
    * the range [-1, +8].
    */
   if (BiasMin < -100)
      BiasMin = -100;
   if (BiasMax > 800)
      BiasMax = 800;
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 350, 350 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "Bug #3195 Test" );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutIdleFunc(Idle);
   Init();
   glutMainLoop();
   return 0;
}
