/* $Id: anisotropic.c,v 1.1 2001/03/22 15:24:15 gareth Exp $ */

/*
 * GL_ARB_texture_filter_anisotropic demo
 *
 * Gareth Hughes
 * March 2001
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

/* This is a fairly early version.  All it does is draw a couple of
 * textured quads with different forms of texture filtering.  Eventually,
 * you'll be able to adjust the maximum anisotropy and so on.
 */

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

#define TEXTURE_SIZE 256
static GLubyte image[TEXTURE_SIZE][TEXTURE_SIZE][3];

static GLfloat repeat = 1.0;

static GLfloat texcoords[] = {
   0.0, 0.0,
   0.0, 1.0,
   1.0, 0.0,
   1.0, 1.0
};

static GLfloat vertices[] = {
   -400.0, -400.0,     0.0,
   -400.0,  400.0, -7000.0,
    400.0, -400.0,     0.0,
    400.0,  400.0, -7000.0
};

static GLfloat maxAnisotropy;


static void init( void )
{
   int i, j;

   if ( !glutExtensionSupported( "GL_EXT_texture_filter_anisotropic" ) ) {
      fprintf( stderr, "Sorry, this demo requires GL_EXT_texture_filter_anisotropic.\n" );
      exit( 0 );
   }

   glClearColor( 0.0, 0.0, 0.0, 0.0 );
   glShadeModel( GL_SMOOTH );

   /* Init the vertex arrays.
    */
   glEnableClientState( GL_VERTEX_ARRAY );
   glEnableClientState( GL_TEXTURE_COORD_ARRAY );

   glVertexPointer( 3, GL_FLOAT, 0, vertices );
   glTexCoordPointer( 2, GL_FLOAT, 0, texcoords );

   /* Init the texture environment.
    */
   glEnable( GL_TEXTURE_2D );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

   glMatrixMode( GL_TEXTURE );
   glScalef( repeat, repeat, 0 );

   glMatrixMode( GL_MODELVIEW );

   glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, &maxAnisotropy );
   printf( "Maximum supported anisotropy: %.2f\n", maxAnisotropy );

   /* Make the texture image.
    */
   glPixelStorei( GL_UNPACK_ALIGNMENT, 1 );

   for ( i = 0 ; i < TEXTURE_SIZE ; i++ ) {
      for ( j = 0 ; j < TEXTURE_SIZE ; j++ ) {
	 if ( (i/4 + j/4) & 1 ) {
	    image[i][j][0] = 0;
	    image[i][j][1] = 0;
	    image[i][j][2] = 0;
	 } else {
	    image[i][j][0] = 255;
	    image[i][j][1] = 255;
	    image[i][j][2] = 255;
	 }
      }
   }

   gluBuild2DMipmaps( GL_TEXTURE_2D, GL_RGB, TEXTURE_SIZE, TEXTURE_SIZE,
		      GL_RGB, GL_UNSIGNED_BYTE, image );
}

static void display( void )
{
   GLint vp[4], w, h;

   glClear( GL_COLOR_BUFFER_BIT );

   glGetIntegerv( GL_VIEWPORT, vp );
   w = vp[2] / 2;
   h = vp[3] / 2;

   /* Upper left corner:
    */
   glViewport( 1, h + 1, w - 2, h - 2 );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );

   glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );


   /* Upper right corner:
    */
   glViewport( w + 1, h + 1, w - 2, h - 2 );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );

   glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );


   /* Lower left corner:
    */
   glViewport( 1, 1, w - 2, h - 2 );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST_MIPMAP_LINEAR );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );

   glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );


   /* Lower right corner:
    */
   glViewport( w + 1, 1, w - 2, h - 2 );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR );
   glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT, maxAnisotropy );

   glDrawArrays( GL_TRIANGLE_STRIP, 0, 4 );


   glViewport( vp[0], vp[1], vp[2], vp[3] );

   glutSwapBuffers();
}

static void reshape( int w, int h )
{
   glViewport( 0, 0, (GLsizei) w, (GLsizei) h );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 10.0, 10000.0 );

   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -10.0 );
}

static void key( unsigned char key, int x, int y )
{
   (void) x; (void) y;

   switch ( key ) {
   case 27:
      exit( 0 );
   }

   glutPostRedisplay();
}

static void usage( void )
{
   /* Nothing yet... */
}

int main( int argc, char **argv )
{
   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_DOUBLE | GLUT_RGB );
   glutInitWindowSize( 600, 300 );
   glutInitWindowPosition( 0, 0 );
   glutCreateWindow( "Anisotropic Texture Filter Demo" );

   init();

   glutDisplayFunc( display );
   glutReshapeFunc( reshape );
   glutKeyboardFunc( key );

   usage();

   glutMainLoop();

   return 0;
}
