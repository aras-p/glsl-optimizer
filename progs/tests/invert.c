/*
 * (C) Copyright IBM Corporation 2005
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
 * \file invert.c
 * 
 * Simple test of GL_MESA_pack_invert functionality.  Three squares are
 * drawn.  The first two should look the same, and the third one should
 * look inverted.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#include "readtex.h"

#define IMAGE_FILE "../images/tree3.rgb"

static int Width = 420;
static int Height = 150;
static const GLfloat Near = 5.0, Far = 25.0;

static GLubyte * image = NULL;
static GLubyte * temp_image = NULL;
static GLuint img_width = 0;
static GLuint img_height = 0;
static GLuint img_format = 0;

PFNGLWINDOWPOS2IPROC win_pos_2i = NULL;


static void Display( void )
{
   GLint err;


   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT );


   /* This is the "reference" square.
    */

   (*win_pos_2i)( 5, 5 );
   glDrawPixels( img_width, img_height, img_format, GL_UNSIGNED_BYTE, image );

   glPixelStorei( GL_PACK_INVERT_MESA, GL_FALSE );
   err = glGetError();
   if ( err != GL_NO_ERROR ) {
      printf( "Setting PACK_INVERT_MESA to false generated an error (0x%04x).\n",
	      err );
   }

   glReadPixels( 5, 5, img_width, img_height, img_format, GL_UNSIGNED_BYTE, temp_image );
   (*win_pos_2i)( 5 + 1 * (10 + img_width), 5 );
   glDrawPixels( img_width, img_height, img_format, GL_UNSIGNED_BYTE, temp_image );

   glPixelStorei( GL_PACK_INVERT_MESA, GL_TRUE );
   err = glGetError();
   if ( err != GL_NO_ERROR ) {
      printf( "Setting PACK_INVERT_MESA to true generated an error (0x%04x).\n",
	      err );
   }

   glReadPixels( 5, 5, img_width, img_height, img_format, GL_UNSIGNED_BYTE, temp_image );
   (*win_pos_2i)( 5 + 2 * (10 + img_width), 5 );
   glDrawPixels( img_width, img_height, img_format, GL_UNSIGNED_BYTE, temp_image );

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   GLfloat ar = (float) width / (float) height;
   Width = width;
   Height = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ar, ar, -1.0, 1.0, Near, Far );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
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
   const float ver = strtof( ver_string, NULL );


   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   if ( !glutExtensionSupported("GL_MESA_pack_invert") ) {
      printf("\nSorry, this program requires GL_MESA_pack_invert.\n");
      exit(1);
   }

   if ( ver >= 1.4 ) {
      win_pos_2i = (PFNGLWINDOWPOS2IPROC) glutGetProcAddress( "glWindowPos2i" );
   }
   else if ( glutExtensionSupported("GL_ARB_window_pos") ) {
      win_pos_2i = (PFNGLWINDOWPOS2IPROC) glutGetProcAddress( "glWindowPos2iARB" );
   }
   else if ( glutExtensionSupported("GL_MESA_window_pos") ) {
      win_pos_2i = (PFNGLWINDOWPOS2IPROC) glutGetProcAddress( "glWindowPos2iMESA" );
   }
   

   /* Do this check as a separate if-statement instead of as an else in case
    * one of the required extensions is supported but glutGetProcAddress
    * returns NULL.
    */

   if ( win_pos_2i == NULL ) {
      printf("\nSorry, this program requires either GL 1.4 (or higher),\n"
	     "GL_ARB_window_pos, or GL_MESA_window_pos.\n");
      exit(1);
   }

   printf("\nThe left 2 squares should be the same color, and the right\n"
	  "square should look upside-down.\n");
   

   image = LoadRGBImage( IMAGE_FILE, & img_width, & img_height,
			 & img_format );
   if ( image == NULL ) {
      printf( "Could not open image file \"%s\".\n", IMAGE_FILE );
      exit(1);
   }

   temp_image = malloc( 3 * img_height * img_width );
   if ( temp_image == NULL ) {
      printf( "Could not allocate memory for temporary image.\n" );
      exit(1);
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "GL_MESA_pack_invert test" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
