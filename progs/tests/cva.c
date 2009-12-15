
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
#include <stddef.h>	/* for ptrdiff_t, referenced by GL.h when GL_GLEXT_LEGACY defined */
#ifdef _WIN32
#include <windows.h>
#endif
#define GL_GLEXT_LEGACY
#include <GL/glew.h>
#include <GL/glut.h>
#include <GL/glext.h>

GLfloat verts[][4] = {
   { -0.5, -0.5, -2.0, 0.0 },
   {  0.5, -0.5, -2.0, 0.0 },
   { -0.5,  0.5, -2.0, 0.0 },
   {  0.5,  0.5, -2.0, 0.0 },
};

GLubyte color[][4] = {
   { 0xff, 0x00, 0x00, 0x00 },
   { 0x00, 0xff, 0x00, 0x00 },
   { 0x00, 0x00, 0xff, 0x00 },
   { 0xff, 0xff, 0xff, 0x00 },
};

GLuint indices[] = { 0, 1, 2, 3 };

GLboolean compiled = GL_TRUE;
GLboolean doubleBuffer = GL_TRUE;


static void init( void )
{
   glClearColor( 0.0, 0.0, 0.0, 0.0 );
   glShadeModel( GL_SMOOTH );

   glFrontFace( GL_CCW );
   glCullFace( GL_BACK );
   glEnable( GL_CULL_FACE );

   glEnable( GL_DEPTH_TEST );

   glEnableClientState( GL_VERTEX_ARRAY );
   glEnableClientState( GL_COLOR_ARRAY );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 2.0, 10.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();

   glVertexPointer( 3, GL_FLOAT, sizeof(verts[0]), verts );
   glColorPointer( 4, GL_UNSIGNED_BYTE, 0, color );

#ifdef GL_EXT_compiled_vertex_array
   if ( compiled ) {
      glLockArraysEXT( 0, 4 );
   }
#endif
}

static void display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glDrawElements( GL_TRIANGLES, 3, GL_UNSIGNED_INT, indices );

   glFlush();
   if ( doubleBuffer ) {
      glutSwapBuffers();
   }
}

static void keyboard( unsigned char key, int x, int y )
{
   switch ( key ) {
      case 27:
         exit( 0 );
         break;
   }

   glutPostRedisplay();
}

static GLboolean args( int argc, char **argv )
{
    GLint i;

    doubleBuffer = GL_TRUE;

    for ( i = 1 ; i < argc ; i++ ) {
	if ( strcmp( argv[i], "-sb" ) == 0 ) {
	    doubleBuffer = GL_FALSE;
	} else if ( strcmp( argv[i], "-db" ) == 0 ) {
	    doubleBuffer = GL_TRUE;
	} else {
	    fprintf( stderr, "%s (Bad option).\n", argv[i] );
	    return GL_FALSE;
	}
    }
    return GL_TRUE;
}

int main( int argc, char **argv )
{
   GLenum type;
   char *string;
   double version;

   glutInit( &argc, argv );

   if ( args( argc, argv ) == GL_FALSE ) {
      exit( 1 );
   }

   type = GLUT_RGB | GLUT_DEPTH;
   type |= ( doubleBuffer ) ? GLUT_DOUBLE : GLUT_SINGLE;

   glutInitDisplayMode( type );
   glutInitWindowSize( 250, 250 );
   glutInitWindowPosition( 100, 100 );
   glutCreateWindow( "CVA Test" );
   glewInit();

   /* Make sure the server supports GL 1.2 vertex arrays.
    */
   string = (char *) glGetString( GL_VERSION );

   version = atof(string);
   if ( version < 1.2 ) {
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
