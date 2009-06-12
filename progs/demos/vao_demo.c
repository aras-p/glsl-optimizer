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

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef __darwin__
#include <GLUT/glut.h>

typedef void (* PFNGLBINDVERTEXARRAYAPPLEPROC) (GLuint array);
typedef void (* PFNGLDELETEVERTEXARRAYSAPPLEPROC) (GLsizei n, const GLuint *arrays);
typedef void (* PFNGLGENVERTEXARRAYSAPPLEPROC) (GLsizei n, const GLuint *arrays);
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
static int Win = 0;
static const GLfloat Near = 5.0, Far = 25.0;
static GLfloat angle = 0.0;

static GLuint cube_array_obj = 0;
static GLuint oct_array_obj = 0;

static const GLfloat cube_vert[] = {
    -0.5, -0.5, -0.5,  1.0,
     0.5, -0.5, -0.5,  1.0,
     0.5,  0.5, -0.5,  1.0,
    -0.5,  0.5, -0.5,  1.0,

    -0.5, -0.5,  0.5,  1.0,
     0.5, -0.5,  0.5,  1.0,
     0.5,  0.5,  0.5,  1.0,
    -0.5,  0.5,  0.5,  1.0,

    -0.5,  0.5, -0.5,  1.0,
     0.5,  0.5, -0.5,  1.0,
     0.5,  0.5,  0.5,  1.0,
    -0.5,  0.5,  0.5,  1.0,

    -0.5, -0.5, -0.5,  1.0,
     0.5, -0.5, -0.5,  1.0,
     0.5, -0.5,  0.5,  1.0,
    -0.5, -0.5,  0.5,  1.0,

     0.5, -0.5, -0.5,  1.0,
     0.5, -0.5,  0.5,  1.0,
     0.5,  0.5,  0.5,  1.0,
     0.5,  0.5, -0.5,  1.0,

    -0.5, -0.5, -0.5,  1.0,
    -0.5, -0.5,  0.5,  1.0,
    -0.5,  0.5,  0.5,  1.0,
    -0.5,  0.5, -0.5,  1.0,

};

static const GLfloat cube_color[] = {
     1.0, 0.0, 0.0, 1.0,
     1.0, 0.0, 0.0, 1.0,
     1.0, 0.0, 0.0, 1.0,
     1.0, 0.0, 0.0, 1.0,

     0.0, 1.0, 0.0, 1.0,
     0.0, 1.0, 0.0, 1.0,
     0.0, 1.0, 0.0, 1.0,
     0.0, 1.0, 0.0, 1.0,

     0.0, 0.0, 1.0, 1.0,
     0.0, 0.0, 1.0, 1.0,
     0.0, 0.0, 1.0, 1.0,
     0.0, 0.0, 1.0, 1.0,

     1.0, 0.0, 1.0, 1.0,
     1.0, 0.0, 1.0, 1.0,
     1.0, 0.0, 1.0, 1.0,
     1.0, 0.0, 1.0, 1.0,

     1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0,
     1.0, 1.0, 1.0, 1.0,

     0.5, 0.5, 0.5, 1.0,
     0.5, 0.5, 0.5, 1.0,
     0.5, 0.5, 0.5, 1.0,
     0.5, 0.5, 0.5, 1.0,
};

static const GLfloat oct_vert[] = {
    0.0,  0.0,  0.7071, 1.0,
    0.5,  0.5,  0.0,  1.0,
   -0.5,  0.5,  0.0,  1.0,

    0.0,  0.0,  0.7071, 1.0,
    0.5, -0.5,  0.0,  1.0,
   -0.5, -0.5,  0.0,  1.0,

    0.0,  0.0,  0.7071, 1.0,
   -0.5, -0.5,  0.0,  1.0,
   -0.5,  0.5,  0.0,  1.0,

    0.0,  0.0,  0.7071, 1.0,
    0.5,  0.5,  0.0,  1.0,
    0.5, -0.5,  0.0,  1.0,


    0.0,  0.0, -0.7071, 1.0,
    0.5,  0.5,  0.0,  1.0,
   -0.5,  0.5,  0.0,  1.0,

    0.0,  0.0, -0.7071, 1.0,
    0.5, -0.5,  0.0,  1.0,
   -0.5, -0.5,  0.0,  1.0,

    0.0,  0.0, -0.7071, 1.0,
   -0.5, -0.5,  0.0,  1.0,
   -0.5,  0.5,  0.0,  1.0,

    0.0,  0.0, -0.7071, 1.0,
    0.5,  0.5,  0.0,  1.0,
    0.5, -0.5,  0.0,  1.0,
};

static const GLfloat oct_color[] = {
     1.0, 0.64, 0.0, 1.0,
     1.0, 0.64, 0.0, 1.0,
     1.0, 0.64, 0.0, 1.0,

     0.8, 0.51, 0.0, 1.0,
     0.8, 0.51, 0.0, 1.0,
     0.8, 0.51, 0.0, 1.0,

     0.5, 0.32, 0.0, 1.0,
     0.5, 0.32, 0.0, 1.0,
     0.5, 0.32, 0.0, 1.0,

     0.2, 0.13, 0.0, 1.0,
     0.2, 0.13, 0.0, 1.0,
     0.2, 0.13, 0.0, 1.0,

     0.2, 0.13, 0.0, 1.0,
     0.2, 0.13, 0.0, 1.0,
     0.2, 0.13, 0.0, 1.0,

     0.5, 0.32, 0.0, 1.0,
     0.5, 0.32, 0.0, 1.0,
     0.5, 0.32, 0.0, 1.0,

     0.8, 0.51, 0.0, 1.0,
     0.8, 0.51, 0.0, 1.0,
     0.8, 0.51, 0.0, 1.0,

     1.0, 0.64, 0.0, 1.0,
     1.0, 0.64, 0.0, 1.0,
     1.0, 0.64, 0.0, 1.0,
};

static void Display( void )
{
   glClearColor(0.1, 0.1, 0.4, 0);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
   glRotatef( angle, 0.0 * angle , 0.0 * angle, 1.0 );


   (*bind_vertex_array)( cube_array_obj );
   glPushMatrix();
   glTranslatef(-1.5, 0, 0);
   glRotatef( angle, 0.3 * angle , 0.8 * angle, 1.0 );
   glDrawArrays( GL_QUADS, 0, 4 * 6 );
   glPopMatrix();


   (*bind_vertex_array)( oct_array_obj );
   glPushMatrix();
   glTranslatef(1.5, 0, 0);
   glRotatef( angle, 0.3 * angle , 0.8 * angle, 1.0 );
   glDrawArrays( GL_TRIANGLES, 0, 3 * 8 );
   glPopMatrix();

   glutSwapBuffers();
}


static void Idle( void )
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;

   angle += 70.0 * dt;  /* 70 degrees per second */
   angle = fmod(angle, 360.0); /* prevents eventual overflow */

   glutPostRedisplay();
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
         (*delete_vertex_arrays)( 1, & cube_array_obj );
         (*delete_vertex_arrays)( 1, & oct_array_obj );
         glutDestroyWindow(Win);
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

   if ( !glutExtensionSupported("GL_APPLE_vertex_array_object") ) {
      printf("Sorry, this program requires GL_APPLE_vertex_array_object\n");
      exit(1);
   }

   bind_vertex_array = (PFNGLBINDVERTEXARRAYAPPLEPROC) glutGetProcAddress( "glBindVertexArrayAPPLE" );
   gen_vertex_arrays = (PFNGLGENVERTEXARRAYSAPPLEPROC) glutGetProcAddress( "glGenVertexArraysAPPLE" );
   delete_vertex_arrays = (PFNGLDELETEVERTEXARRAYSAPPLEPROC) glutGetProcAddress( "glDeleteVertexArraysAPPLE" );
   is_vertex_array = (PFNGLISVERTEXARRAYAPPLEPROC) glutGetProcAddress( "glIsVertexArrayAPPLE" );

   assert(bind_vertex_array);
   assert(gen_vertex_arrays);
   assert(delete_vertex_arrays);
   assert(is_vertex_array);

   glEnable( GL_DEPTH_TEST );
   
   (*gen_vertex_arrays)( 1, & cube_array_obj );
   (*bind_vertex_array)( cube_array_obj );
   glVertexPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, cube_vert);
   glColorPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, cube_color);
   glEnableClientState( GL_VERTEX_ARRAY );
   glEnableClientState( GL_COLOR_ARRAY );

   (*gen_vertex_arrays)( 1, & oct_array_obj );
   (*bind_vertex_array)( oct_array_obj );
   glVertexPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, oct_vert);
   glColorPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, oct_color);
   glEnableClientState( GL_VERTEX_ARRAY );
   glEnableClientState( GL_COLOR_ARRAY );

   (*bind_vertex_array)( 0 );
   glVertexPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, (void *) 0xDEADBEEF );
   glColorPointer( 4, GL_FLOAT, sizeof(GLfloat) * 4, (void *) 0xBADDC0DE );
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   Win = glutCreateWindow( "GL_APPLE_vertex_array_object demo" );
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutVisibilityFunc( Visible );
   Init();
   glutMainLoop();
   return 0;
}
