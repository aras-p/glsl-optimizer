/*
 * (C) Copyright IBM Corporation 2004
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
 * \file blendsquare.c
 * 
 * Simple test of GL_NV_blend_square functionality.  Four squares are drawn
 * with different blending modes, but all should be rendered with the same
 * final color.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <GL/glut.h>

static int Width = 400;
static int Height = 200;
static const GLfloat Near = 5.0, Far = 25.0;


static void Display( void )
{
   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();

   glTranslatef(-4.5, 0, 0);
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.5 * 0.5, 0.5 * 0.5, 0.5 * 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   glTranslatef(3.0, 0, 0);
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   glBlendFunc( GL_DST_COLOR, GL_ZERO );
   glBegin(GL_QUADS);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   glTranslatef(3.0, 0, 0);
   glBlendFunc( GL_SRC_COLOR, GL_ZERO );
   glBegin(GL_QUADS);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   glTranslatef(3.0, 0, 0);
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   glBlendFunc( GL_ZERO, GL_DST_COLOR );
   glBegin(GL_QUADS);
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

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
   const double version = strtod( ver_string, NULL );

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   if ( (version < 1.4) && !glutExtensionSupported("GL_NV_blend_square")) {
      printf("Sorry, this program requires either OpenGL 1.4 or GL_NV_blend_square\n");
      exit(1);
   }

   printf("\nAll 4 squares should be the same color.  The two on the left are drawn\n"
	  "without NV_blend_square functionality, and the two on the right are drawn\n"
	  "with NV_blend_square functionality.  If the two on the left are dark, but\n"
	  "the two on the right are not, then NV_blend_square is broken.\n");
   glEnable( GL_BLEND );
   glBlendEquation( GL_FUNC_ADD );
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "GL_NV_blend_square test" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
