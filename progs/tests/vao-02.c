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
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/**
 * \file vao-02.c
 *
 * Simple test of APPLE_vertex_array_object functionality.  This test creates
 * a VAO, pushed it (via \c glPushClientAttrib), deletes the VAO, then pops
 * it (via \c glPopClientAttrib).  After popping, the state of the VAO is
 * examined.
 * 
 * According the the APPLE_vertex_array_object spec, the contents of the VAO
 * should be restored to the values that they had when pushed.
 * 
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __darwin__
#include <GLUT/glut.h>

typedef void (* PFNGLBINDVERTEXARRAYAPPLEPROC) (GLuint array);
typedef void (* PFNGLDELETEVERTEXARRAYSAPPLEPROC) (GLsizei n, const GLuint *arrays);
typedef void (* PFNGLGENVERTEXARRAYSAPPLEPROC) (GLsizei n, GLuint *arrays);
typedef GLboolean (* PFNGLISVERTEXARRAYAPPLEPROC) (GLuint array);

#else
#include <GL/glut.h>
#endif

static PFNGLBINDVERTEXARRAYAPPLEPROC bind_vertex_array = NULL;
static PFNGLGENVERTEXARRAYSAPPLEPROC gen_vertex_arrays = NULL;
static PFNGLDELETEVERTEXARRAYSAPPLEPROC delete_vertex_arrays = NULL;
static PFNGLISVERTEXARRAYAPPLEPROC is_vertex_array = NULL;

static int Width = 400;
static int Height = 200;
static const GLfloat Near = 5.0, Far = 25.0;


static void Display( void )
{
}


static void Idle( void )
{
}


static void Visible( int vis )
{
   if ( vis == GLUT_VISIBLE ) {
      glutIdleFunc( Idle );
   }
   else {
      glutIdleFunc( NULL );
   }
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
   GLuint obj;
   int pass = 1;
   void * ptr;
   GLenum err;


   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n\n", ver_string);

   if ( !glutExtensionSupported("GL_APPLE_vertex_array_object") ) {
      printf("Sorry, this program requires GL_APPLE_vertex_array_object\n");
      exit(2);
   }

   bind_vertex_array = glutGetProcAddress( "glBindVertexArrayAPPLE" );
   gen_vertex_arrays = glutGetProcAddress( "glGenVertexArraysAPPLE" );
   delete_vertex_arrays = glutGetProcAddress( "glDeleteVertexArraysAPPLE" );
   is_vertex_array = glutGetProcAddress( "glIsVertexArrayAPPLE" );


   (*gen_vertex_arrays)( 1, & obj );
   (*bind_vertex_array)( obj );
   glVertexPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, (void *) 0xDEADBEEF);
   glEnableClientState( GL_VERTEX_ARRAY );

   glPushClientAttrib( GL_CLIENT_VERTEX_ARRAY_BIT );

   (*delete_vertex_arrays)( 1, & obj );
   
   err = glGetError();
   if (err) {
      printf( "glGetError incorrectly returned 0x%04x.\n", err );
      pass = 0;
   }

   if ( (*is_vertex_array)( obj ) ) {
      printf( "Array object is incorrectly still valid.\n" );
      pass = 0;
   }

   err = glGetError();
   if (err) {
      printf( "glGetError incorrectly returned 0x%04x.\n", err );
      pass = 0;
   }

   glPopClientAttrib();

   err = glGetError();
   if (err) {
      printf( "glGetError incorrectly returned 0x%04x.\n", err );
      pass = 0;
   }

   if ( ! (*is_vertex_array)( obj ) ) {
      printf( "Array object is incorrectly invalid.\n" );
      pass = 0;
   }

   if ( ! glIsEnabled( GL_VERTEX_ARRAY ) ) {
      printf( "Array state is incorrectly disabled.\n" );
      pass = 0;
   }

   glGetPointerv( GL_VERTEX_ARRAY_POINTER, & ptr );
   if ( ptr != (void *) 0xDEADBEEF ) {
      printf( "Array pointer is incorrectly set to 0x%p.\n", ptr );
      pass = 0;
   }

   if ( ! pass ) {
      printf( "FAIL!\n" );
      exit(1);
   }
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB );
   glutCreateWindow( "GL_APPLE_vertex_array_object demo" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutVisibilityFunc( Visible );

   Init();

   return 0;
}
