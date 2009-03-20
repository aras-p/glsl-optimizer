/*
 * (C) Copyright IBM Corporation 2006
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
 * \file bug_3050.c
 *
 * Simple regression test for bug #3050.  Create a texture and make a few
 * calls to \c glGetTexLevelParameteriv.  If the bug still exists, trying
 * to get \c GL_TEXTURE_WITDH will cause a protocol error.
 * 
 * This test \b only applies to indirect-rendering.  This may mean that the
 * test needs to be run with the environment variable \c LIBGL_ALWAYS_INDIRECT
 * set to a non-zero value.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Width = 400;
static int Height = 200;
static const GLfloat Near = 5.0, Far = 25.0;


static void Display( void )
{
}


static void Reshape( int width, int height )
{
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
   unsigned i;
   static const GLenum pnames[] = {
      GL_TEXTURE_RED_SIZE,
      GL_TEXTURE_GREEN_SIZE,
      GL_TEXTURE_BLUE_SIZE,
      GL_TEXTURE_ALPHA_SIZE,
      GL_TEXTURE_LUMINANCE_SIZE,
      GL_TEXTURE_INTENSITY_SIZE,
      GL_TEXTURE_BORDER,
      GL_TEXTURE_INTERNAL_FORMAT,
      GL_TEXTURE_WIDTH,
      GL_TEXTURE_HEIGHT,
      GL_TEXTURE_DEPTH,
      ~0
   };
   
       
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));

   printf("\nThis program should log some data about a texture and exit.\n");
   printf("This is a regression test for bug #3050.  If the bug still\n");
   printf("exists, a GLX protocol error will be generated.\n");
   printf("https://bugs.freedesktop.org/show_bug.cgi?id=3050\n\n");


   if ( ! glutExtensionSupported( "GL_NV_texture_rectangle" )
	&& ! glutExtensionSupported( "GL_EXT_texture_rectangle" )
	&& ! glutExtensionSupported( "GL_ARB_texture_rectangle" ) ) {
      printf( "This test requires one of GL_ARB_texture_rectangle, GL_EXT_texture_rectangle,\n"
	      "or GL_NV_texture_rectangle be supported\n." );
      exit( 1 );
   }
	

   glBindTexture( GL_TEXTURE_RECTANGLE_NV, 1 );
   glTexImage2D( GL_PROXY_TEXTURE_RECTANGLE_NV, 0, GL_RGBA, 8, 8, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, NULL );

   for ( i = 0 ; pnames[i] != ~0 ; i++ ) {
      GLint param_i;
      GLfloat param_f;
      GLenum err;

      glGetTexLevelParameteriv( GL_PROXY_TEXTURE_RECTANGLE_NV, 0, pnames[i], & param_i );
      err = glGetError();

      if ( err ) {
	 printf("glGetTexLevelParameteriv(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, 0x%04x, & param) generated a GL\n"
		"error of 0x%04x!",
		pnames[i], err );
	 exit( 1 );
      }
      else {
	 printf("glGetTexLevelParameteriv(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, 0x%04x, & param) = 0x%04x\n",
		pnames[i], param_i );
      }


      glGetTexLevelParameterfv( GL_PROXY_TEXTURE_RECTANGLE_NV, 0, pnames[i], & param_f );
      err = glGetError();

      if ( err ) {
	 printf("glGetTexLevelParameterfv(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, 0x%04x, & param) generated a GL\n"
		"error of 0x%04x!\n",
		pnames[i], err );
	 exit( 1 );
      }
      else {
	 printf("glGetTexLevelParameterfv(GL_PROXY_TEXTURE_RECTANGLE_NV, 0, 0x%04x, & param) = %.1f (0x%04x)\n",
		pnames[i], param_f, (GLint) param_f );
      }
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "Bug #3050 Test" );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   return 0;
}
