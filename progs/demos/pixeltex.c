/*
 * GL_SGIS_pixel_texture demo
 *
 * Brian Paul
 * 6 Apr 2000
 *
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


/*
 * How this works:
 * 1. We load the image into a 2D texture.
 * 2. We generate a sequence of RGB images in which the R component
 *    is really the S texture coordinate and the G component is really
 *    the T texture coordinate.
 *    By warping the mapping from R to S and G to T we can get non-linear
 *    distortions.
 * 3. Draw the warped image (a 2-D warping function) with pixel texgen
 *    enabled.
 * 4. Loop over the warped images to animate.
 *
 * The pixel texgen extension can also be used to do color-space
 * conversions.  For example, we could convert YCR to RGB with a
 * 3D texture map which takes YCR as the S,T,R texture coordinate and
 * returns RGB texel values.
 *
 * You can use this extension in (at least) two ways:
 * 1. glDrawPixels w/ color space conversion/warping
 * 2. glDrawPixels to spatially warp another image in texture memory
 *
 * We're basically using glDrawPixels to draw a texture coordinate image.
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>
#include <GL/glext.h>
#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/girl.rgb"

static int ImgWidth = 300, ImgHeight = 300;
#define FRAMES 20
static GLubyte *ImgData[FRAMES];
static GLint Frame = 0;

static GLboolean TextureFlag = GL_TRUE;


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   if (TextureFlag) {
      glEnable(GL_PIXEL_TEXTURE_SGIS);
      glEnable(GL_TEXTURE_2D);
   }
   else {
      glDisable(GL_PIXEL_TEXTURE_SGIS);
      glDisable(GL_TEXTURE_2D);
   }

   glColor3f(1, 1, 1);
   glRasterPos2f(10, 10);
   glDrawPixels(ImgWidth, ImgHeight, GL_RGB, GL_UNSIGNED_BYTE, ImgData[Frame]);

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0, width, 0, height, -1, 1);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case ' ':
         TextureFlag = !TextureFlag;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Idle(void)
{
   Frame++;
   if (Frame >= FRAMES)
      Frame = 0;
   glutPostRedisplay();
}


static GLubyte warp(GLfloat s, int frame)
{
   static const GLfloat PI = 3.14159265;
   static int halfFrame = FRAMES / 2;
   GLfloat y, weight, v;
   if (frame >= halfFrame)
      frame = halfFrame - (frame - halfFrame);
   y = sin(s * PI);
   weight = (float) frame / (FRAMES-1);
   v = y * (0.8 * weight + 0.2);
   return (GLint) (v * 255.0F);
}


static void InitImage(void)
{
   int i, j, frame;
   for (frame = 0; frame < FRAMES; frame++) {
      ImgData[frame] = (GLubyte *) malloc(ImgWidth * ImgHeight * 3);
      for (i = 0; i < ImgHeight; i++) {
         for (j = 0; j < ImgWidth; j++) {
            GLubyte *pixel = ImgData[frame] + (i * ImgWidth + j) * 3;
            pixel[0] = warp((float) j / (ImgWidth - 0), frame);
            pixel[1] = warp((float) i / (ImgHeight - 0), frame);
            pixel[2] = 0.0;
         }
      }
   }
}


static void Init( int argc, char *argv[] )
{
   const char *exten = (const char *) glGetString(GL_EXTENSIONS);
   if (!strstr(exten, "GL_SGIS_pixel_texture")) {
      printf("Sorry, GL_SGIS_pixel_texture not supported by this renderer.\n");
      exit(1);
   }

   /* linear filtering looks nicer, but it's slower, since it's in software */
#if 1
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
#else
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
#endif

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   if (!LoadRGBMipmaps(TEXTURE_FILE, GL_RGB)) {
      printf("Error: couldn't load texture image\n");
      exit(1);
   }

   glClearColor(0.3, 0.3, 0.4, 1.0);

   InitImage();

   printf("Hit SPACE to toggle pixel texgen\n");
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 330, 330 );
   glutInitWindowPosition( 0, 0 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0] );

   Init( argc, argv );

   glutKeyboardFunc( Key );
   glutReshapeFunc( Reshape );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
