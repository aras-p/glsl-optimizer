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
 * \file blendminmax.c
 * 
 * Simple test of GL_EXT_blend_minmax functionality.  Four squares are drawn
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

   /* This is the "reference" square.
    */

   glTranslatef(-4.5, 0, 0);
   glBlendEquation( GL_FUNC_ADD );
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   /* GL_MIN and GL_MAX are supposed to ignore the blend function setting.
    * To test that, we set the blend function to GL_ZERO for both color and
    * alpha each time GL_MIN or GL_MAX is used.
    *
    * Apple ships an extension called GL_ATI_blend_weighted_minmax (supported
    * on Mac OS X 10.2 and later).  I believe the difference with that
    * extension is that it uses the blend function.  However, I have no idea
    * what the enums are for it.  The extension is listed at Apple's developer
    * site, but there is no documentation.
    * 
    * http://developer.apple.com/opengl/extensions.html
    */

   glTranslatef(3.0, 0, 0);
   glBlendEquation( GL_FUNC_ADD );
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   glBlendEquation( GL_MAX );
   glBlendFunc( GL_ZERO, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.2, 0.2, 0.2 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   glTranslatef(3.0, 0, 0);
   glBlendEquation( GL_FUNC_ADD );
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   glBlendEquation( GL_MIN );
   glBlendFunc( GL_ZERO, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.8, 0.8, 0.8 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();


   glTranslatef(3.0, 0, 0);
   glBlendEquation( GL_FUNC_ADD );
   glBlendFunc( GL_ONE, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.8, 0.8, 0.8 );
   glVertex2f(-1, -1);
   glVertex2f( 1, -1);
   glVertex2f( 1,  1);
   glVertex2f(-1,  1);
   glEnd();

   glBlendEquation( GL_MIN );
   glBlendFunc( GL_ZERO, GL_ZERO );
   glBegin(GL_QUADS);
   glColor3f( 0.5, 0.5, 0.5 );
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

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   if ( !glutExtensionSupported("GL_ARB_imaging") && !glutExtensionSupported("GL_EXT_blend_minmax")) {
      printf("Sorry, this program requires either GL_ARB_imaging or GL_EXT_blend_minmax.\n");
      exit(1);
   }

   printf("\nAll 4 squares should be the same color.\n");
   glEnable( GL_BLEND );
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "GL_EXT_blend_minmax test" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
