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
 * \file crossbar.c
 * 
 * Simple test of GL_ARB_texture_env_crossbar functionality.  Several squares
 * are drawn with different texture combine modes, but all should be rendered
 * with the same final color.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

static const GLint tests[][8] = {
   { 1, GL_REPLACE,  GL_PRIMARY_COLOR, GL_PRIMARY_COLOR,
     2, GL_REPLACE,  GL_TEXTURE,       GL_PRIMARY_COLOR },
   { 3, GL_REPLACE,  GL_PRIMARY_COLOR, GL_PRIMARY_COLOR,
     2, GL_SUBTRACT, GL_TEXTURE0,      GL_TEXTURE1 },
   { 2, GL_REPLACE,  GL_PRIMARY_COLOR, GL_PRIMARY_COLOR,
     2, GL_REPLACE,  GL_TEXTURE0,      GL_TEXTURE0 },
   { 2, GL_REPLACE,  GL_PRIMARY_COLOR, GL_PRIMARY_COLOR,
     1, GL_SUBTRACT, GL_TEXTURE0,      GL_TEXTURE1 },
   { 3, GL_ADD,      GL_TEXTURE1,      GL_TEXTURE1,
     2, GL_MODULATE, GL_TEXTURE1,      GL_PREVIOUS },
   { 3, GL_ADD,      GL_TEXTURE1,      GL_TEXTURE1,
     4, GL_MODULATE, GL_TEXTURE0,      GL_PREVIOUS },
};

#define NUM_TESTS (sizeof(tests) / sizeof(tests[0]))

static int Width = 100 * (NUM_TESTS + 1);
static int Height = 200;
static const GLfloat Near = 5.0, Far = 25.0;


static void Display( void )
{
   unsigned i;


   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();

   /* This is the "reference" square.
    */

   glActiveTexture( GL_TEXTURE0 );
   glDisable( GL_TEXTURE_2D );
   glActiveTexture( GL_TEXTURE1 );
   glDisable( GL_TEXTURE_2D );

   glTranslatef(-(NUM_TESTS * 1.5), 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   for ( i = 0 ; i < NUM_TESTS ; i++ ) {
      glActiveTexture( GL_TEXTURE0 );
      glEnable( GL_TEXTURE_2D );
      glBindTexture( GL_TEXTURE_2D, tests[i][0] );
      glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
      glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, tests[i][1] );
      glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, tests[i][2] );
      glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, tests[i][3] );

      glActiveTexture( GL_TEXTURE1 );
      glEnable( GL_TEXTURE_2D );
      glBindTexture( GL_TEXTURE_2D, tests[i][4] );
      glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_COMBINE );
      glTexEnvi( GL_TEXTURE_ENV, GL_COMBINE_RGB, tests[i][5] );
      glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE0_RGB, tests[i][6] );
      glTexEnvi( GL_TEXTURE_ENV, GL_SOURCE1_RGB, tests[i][7] );

      glCallList(1);
   }

   glPopMatrix();

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
   float ver = strtof( ver_string, NULL );
   GLint tex_units;
   GLint temp[ 256 ];


   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   if ( (!glutExtensionSupported("GL_ARB_multitexture") 
	 && (ver < 1.3))
	|| (!glutExtensionSupported("GL_ARB_texture_env_combine") 
	    && !glutExtensionSupported("GL_EXT_texture_env_combine")
	    && (ver < 1.3))
	|| (!glutExtensionSupported("GL_ARB_texture_env_crossbar")
	    && !glutExtensionSupported("GL_NV_texture_env_combine4")
	    && (ver < 1.4)) ) {
      printf("\nSorry, this program requires GL_ARB_multitexture and either\n"
	     "GL_ARB_texture_env_combine or GL_EXT_texture_env_combine (or OpenGL 1.3).\n"
	     "Either GL_ARB_texture_env_crossbar or GL_NV_texture_env_combine4 (or\n"
	     "OpenGL 1.4) are also required.\n");
      exit(1);
   }

   glGetIntegerv( GL_MAX_TEXTURE_UNITS, & tex_units );
   if ( tex_units < 2 ) {
      printf("\nSorry, this program requires at least 2 texture units.\n");
      exit(1);
   }

   printf("\nAll %u squares should be the same color.\n", NUM_TESTS + 1);
   
   (void) memset( temp, 0x00, sizeof( temp ) );
   glBindTexture( GL_TEXTURE_2D, 1 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, temp );

   (void) memset( temp, 0x7f, sizeof( temp ) );
   glBindTexture( GL_TEXTURE_2D, 2 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, temp );

   (void) memset( temp, 0xff, sizeof( temp ) );
   glBindTexture( GL_TEXTURE_2D, 3 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, temp );

   (void) memset( temp, 0x3f, sizeof( temp ) );
   glBindTexture( GL_TEXTURE_2D, 4 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 8, 8, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, temp );


   glNewList( 1, GL_COMPILE );
   glTranslatef(3.0, 0, 0);
   glBegin(GL_QUADS);
   glColor3f( 0.9, 0.0, 0.0 );
   glMultiTexCoord2f( GL_TEXTURE0, 0.5, 0.5 );
   glMultiTexCoord2f( GL_TEXTURE1, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();
   glEndList();
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "GL_ARB_texture_env_crossbar test" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
