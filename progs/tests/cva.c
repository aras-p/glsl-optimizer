/* $Id: cva.c,v 1.1 2000/11/01 03:14:12 gareth Exp $ */

/*
 * Trivial CVA test, good for testing driver fastpaths (especially
 * indexed vertex buffers if they are supported).
 *
 * Gareth Hughes
 * November 2000
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#define GL_GLEXT_LEGACY
#include <GL/glut.h>


GLfloat verts[][4] = {
   { 0.25, 0.25, 0.0, 0.0 },
   { 0.75, 0.25, 0.0, 0.0 },
   { 0.25, 0.75, 0.0, 0.0 },
};

GLubyte color[][4] = {
   { 0xff, 0x00, 0x00, 0x00 },
   { 0x00, 0xff, 0x00, 0x00 },
   { 0x00, 0x00, 0xff, 0x00 },
};

GLuint indices[] = { 0, 1, 2 };

GLboolean compiled = GL_TRUE;


void init( void )
{
   glClearColor( 0.0, 0.0, 0.0, 0.0 );
   glShadeModel( GL_SMOOTH );

   glEnable( GL_DEPTH_TEST );

   glEnableClientState( GL_VERTEX_ARRAY );
   glEnableClientState( GL_COLOR_ARRAY );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, 1.0, 0.0, 1.0, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );

   glVertexPointer( 3, GL_FLOAT, sizeof(verts[0]), verts );
   glColorPointer( 4, GL_UNSIGNED_BYTE, 0, color );

#ifdef GL_EXT_compiled_vertex_array
   if ( compiled ) {
      glLockArraysEXT( 0, 3 );
   }
#endif
}

void display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glDrawElements( GL_TRIANGLES, 3, GL_UNSIGNED_INT, indices );

   glFlush();
}

void keyboard( unsigned char key, int x, int y )
{
   switch ( key ) {
      case 27:
         exit( 0 );
         break;
   }
}

int main( int argc, char **argv )
{
   char *string;

   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_SINGLE | GLUT_RGB | GLUT_DEPTH );
   glutInitWindowSize( 250, 250 );
   glutInitWindowPosition( 100, 100 );
   glutCreateWindow( "CVA Test" );

   /* Make sure the server supports GL 1.2 vertex arrays.
    */
   string = (char *) glGetString( GL_VERSION );

   if ( !strstr( string, "1.2" ) ) {
      fprintf( stderr, "This program requires OpenGL 1.2 vertex arrays.\n" );
      exit( -1 );
   }

   /* See if the server supports compiled vertex arrays.
    */
   string = (char *) glGetString( GL_EXTENSIONS );

   if ( !strstr( string, "GL_EXT_compiled_vertex_array" ) ) {
      fprintf( stderr, "Compiled vertex arrays not supported by this renderer.\n" );
      compiled = GL_FALSE;
   }

   init();

   glutDisplayFunc( display );
   glutKeyboardFunc( keyboard );
   glutMainLoop();

   return 0;
}
