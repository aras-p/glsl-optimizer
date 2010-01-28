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
 * \file no_s3tc.c
 * Test program to verify the behavior of an OpenGL implementation when
 * an application calls \c glCompressedTexImage2D with an unsupported (but
 * valid) compression format.  The most common example is calling it with
 * \c GL_COMPRESSED_RGBA_S3TC_DXT1_EXT when GL_EXT_texture_compression_s3tc
 * is not supported.
 * 
 * This tests Mesa bug #1028405.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

static unsigned data[16];

int
main( int argc, char ** argv )
{
   float gl_version;
   GLenum format;
   GLuint size;
   GLuint width;
   GLenum err;


   glutInit( & argc, argv );
   glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 300, 300 );
   glutCreateWindow( "No S3TC Test" );
   glewInit();

   gl_version = strtod( (const char *) glGetString( GL_VERSION ), NULL );
   if ( ! glutExtensionSupported( "GL_ARB_texture_compression" )
	&& (gl_version < 1.3) ) {
      fprintf( stderr, "Either OpenGL 1.3 or GL_ARB_texture_compression "
	       "must be supported.\n" );
      return( EXIT_SUCCESS );
   }

   
   if ( ! glutExtensionSupported( "GL_EXT_texture_compression_s3tc" ) ) {
      format = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
      width = 4;
      size = 8;
   }
   else if ( ! glutExtensionSupported( "GL_3DFX_texture_compression_FXT1" ) ) {
      format = GL_COMPRESSED_RGBA_FXT1_3DFX;
      width = 8;
      size = 16;
   }
   else {
      fprintf( stderr, "Either GL_EXT_texture_compression_s3tc or "
	       "GL_3DFX_texture_compression_FXT1 must NOT be supported.\n" );
      return( EXIT_SUCCESS );
   }
	
   glCompressedTexImage2D( GL_TEXTURE_2D, 0, format, width, 4, 0,
			   size, data );
   err = glGetError();
   if ( err != GL_INVALID_ENUM ) {
      fprintf( stderr, "GL error 0x%04x should have been generated, but "
	       "0x%04x was generated instead.\n", GL_INVALID_ENUM, err );
   }
	
   return (err == GL_INVALID_ENUM) ? EXIT_SUCCESS : EXIT_FAILURE;
}
