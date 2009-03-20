/*
 * Copyright (c) 2009 VMware, Inc.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * VA LINUX SYSTEM, IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * Simple test for testing ATI_envmap_bumpmap support.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glut.h>

#include "readtex.h"

static PFNGLGETTEXBUMPPARAMETERFVATIPROC glGetTexBumpParameterfvATI_func = NULL;
static PFNGLGETTEXBUMPPARAMETERIVATIPROC glGetTexBumpParameterivATI_func = NULL;
static PFNGLTEXBUMPPARAMETERFVATIPROC glTexBumpParameterfvATI_func = NULL;

static const char *TexFile = "../images/arch.rgb";

static const GLfloat Near = 5.0, Far = 25.0;

static void Display( void )
{
   /* together with the construction of dudv map, do fixed translation
      in y direction (up), some cosine deformation in x and more
      deformation in y dir */
   GLfloat bumpMatrix[4] = {0.1, 0.0, 0.2, 0.1};


   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();

   /* this is the base map */
   glActiveTexture( GL_TEXTURE0 );
   glEnable( GL_TEXTURE_2D );
   glBindTexture( GL_TEXTURE_2D, 1 );
   glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
   glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_REPLACE );
   glTexEnvf( GL_TEXTURE_ENV, GL_SOURCE0_RGB, GL_TEXTURE );

   /* bump map */
   glActiveTexture( GL_TEXTURE1 );
   glEnable( GL_TEXTURE_2D );
   glBindTexture( GL_TEXTURE_2D, 2 );
   glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
   glTexEnvf( GL_TEXTURE_ENV, GL_COMBINE_RGB, GL_BUMP_ENVMAP_ATI );
   glTexEnvf( GL_TEXTURE_ENV, GL_BUMP_TARGET_ATI, GL_TEXTURE0);

   glTexBumpParameterfvATI_func(GL_BUMP_ROT_MATRIX_ATI, bumpMatrix);

   glCallList(1);

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   GLfloat ar = (float) width / (float) height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, Near, Far );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -6.0 );
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


static void Init( void )
{
   const char * const ver_string = (const char * const)
       glGetString( GL_VERSION );
   GLfloat temp[16][16][2];
   GLubyte *image = NULL;
   GLint imgWidth, imgHeight;
   GLenum imgFormat;
   GLint i,j;
   GLint param, paramArray[16];
   GLfloat paramMat[4];

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   if ( !glutExtensionSupported("GL_ATI_envmap_bumpmap")) {
      printf("\nSorry, this program requires GL_ATI_envmap_bumpmap\n");
      exit(1);
   }

   glGetTexBumpParameterfvATI_func = glutGetProcAddress("glGetTexBumpParameterfvATI");
   glGetTexBumpParameterivATI_func = glutGetProcAddress("glGetTexBumpParameterivATI");
   glTexBumpParameterfvATI_func = glutGetProcAddress("glTexBumpParameterfvATI");

   glGetTexBumpParameterivATI_func(GL_BUMP_ROT_MATRIX_SIZE_ATI, &param);
   printf("BUMP_ROT_MATRIX_SIZE_ATI = %d\n", param);
   glGetTexBumpParameterivATI_func(GL_BUMP_NUM_TEX_UNITS_ATI, &param);
   printf("BUMP_NUM_TEX_UNITS_ATI = %d\n", param);
   glGetTexBumpParameterfvATI_func(GL_BUMP_ROT_MATRIX_ATI, paramMat);
   printf("initial rot matrix %f %f %f %f\n", paramMat[0], paramMat[1], paramMat[2], paramMat[3]);
   glGetTexBumpParameterivATI_func(GL_BUMP_TEX_UNITS_ATI, paramArray);
   printf("units supporting bump mapping: ");
   for (i = 0; i < param; i++)
      printf("%d ", paramArray[i] - GL_TEXTURE0);
   printf("\n");

   image = LoadRGBImage(TexFile, &imgWidth, &imgHeight, &imgFormat);
   if (!image) {
      printf("Couldn't read %s\n", TexFile);
      exit(0);
   }

   glBindTexture( GL_TEXTURE_2D, 1 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glTexImage2D( GL_TEXTURE_2D, 0, imgFormat, imgWidth, imgHeight, 0,
		 imgFormat, GL_UNSIGNED_BYTE, image );

   for (j = 0; j < 16; j++) {
      for (i = 0; i < 16; i++) {
         temp[j][i][0] = cos((float)(i) * 3.1415 / 16.0);
         temp[j][i][1] = -0.5;
      }
   }
   glBindTexture( GL_TEXTURE_2D, 2 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_DU8DV8_ATI, 16, 16, 0,
		 GL_DUDV_ATI, GL_FLOAT, temp );


   glNewList( 1, GL_COMPILE );
   glBegin(GL_QUADS);
   glColor3f( 0.9, 0.0, 0.0 );
   glMultiTexCoord2f( GL_TEXTURE0, 0.0, 0.0 );
   glMultiTexCoord2f( GL_TEXTURE1, 0.0, 0.0 );
   glVertex2f(-1, -1);
   glMultiTexCoord2f( GL_TEXTURE0, 1.0, 0.0 );
   glMultiTexCoord2f( GL_TEXTURE1, 1.0, 0.0 );
   glVertex2f( 1, -1);
   glMultiTexCoord2f( GL_TEXTURE0, 1.0, 1.0 );
   glMultiTexCoord2f( GL_TEXTURE1, 1.0, 1.0 );
   glVertex2f( 1,  1);
   glMultiTexCoord2f( GL_TEXTURE0, 0.0, 1.0 );
   glMultiTexCoord2f( GL_TEXTURE1, 0.0, 1.0 );
   glVertex2f(-1,  1);
   glEnd();
   glEndList();
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 400 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "GL_ATI_envmap_bumpmap test" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
