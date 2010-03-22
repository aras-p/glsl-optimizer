/*
 * Copyright © 2010 Pauli Nieminen
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */


/*
 * Test case for huge cva arrays. Mesa code has to split this to multiple VBOs
 * which had memory access error.
 * This test case doesn't render incorrectly but valgrind showed the memory
 * access error.
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stddef.h>	/* for ptrdiff_t, referenced by GL.h when GL_GLEXT_LEGACY defined */
#ifdef _WIN32
#include <windows.h>
#endif
#define GL_GLEXT_LEGACY
#include <GL/glut.h>

GLfloat *verts;

GLubyte *color;

GLuint *indices;

#define rows 1000 /* Create 1000x1000 vertex grid */
#define row_width 5000.0
#define grid_depth -50.0
GLuint nr_verts_in_row = rows; 
GLuint nr_indices_in_strip = rows * 2;

GLboolean double_buffer;
GLboolean compiled = GL_TRUE;

static void generate_verts( void )
{
	unsigned x, y;
	GLfloat step = row_width /(GLfloat)(nr_verts_in_row - 1); 
	verts = malloc(sizeof(verts[0]) * 4 * nr_verts_in_row * nr_verts_in_row);

	for (y = 0; y < nr_verts_in_row; ++y) {
		for (x = 0; x < nr_verts_in_row; ++x) {
			unsigned idx = 4*(x + y * nr_verts_in_row);
			verts[idx + 0] = step * x - row_width/2.0;
			verts[idx + 1] = step * y - row_width/2.0;
			/* deep enough not to be vissible */
			verts[idx + 2] = grid_depth;
			verts[idx + 3] = 0.0;
		}
	}
	glVertexPointer( 3, GL_FLOAT, sizeof(verts[0])*4, verts );
}

static void generate_colors( void )
{
	unsigned x, y;
	GLfloat step = 255.0/(GLfloat)(nr_verts_in_row - 1);
	color = malloc(sizeof(color[0]) * 4 * nr_verts_in_row *	nr_verts_in_row);

	for (y = 0; y < nr_verts_in_row; ++y) {
		for (x = 0; x < nr_verts_in_row; ++x) {
			unsigned idx = 4*(x + y * nr_verts_in_row);
			color[idx + 0] = (GLubyte)(step * x);
			color[idx + 1] = 0x00;
			color[idx + 2] = (GLubyte)(step * y);
			color[idx + 3] = 0x00;
		}
	}
	glColorPointer( 4, GL_UNSIGNED_BYTE, 0, color );
}

static void generate_indices( void )
{
	unsigned strip, i;

	/* indice size * number of strips * number of indices in strip */
	indices = malloc(sizeof(indices[0]) * (nr_verts_in_row - 1) *
			(nr_indices_in_strip));

	for (strip = 0; strip < nr_verts_in_row - 1; strip += 2) {
		for (i = 0; i < nr_indices_in_strip; i+=2) {
			unsigned idx = i + strip * nr_indices_in_strip;
			unsigned idx2 = (nr_indices_in_strip - i - 2) + (strip +
					1) * (nr_indices_in_strip);
			indices[idx + 1] = i/2 + strip*nr_verts_in_row;
			indices[idx] = i/2 + (strip + 1)*nr_verts_in_row;
			if (strip + 1 < nr_verts_in_row - 1) {
				indices[idx2] = i/2 + (strip + 1)*nr_verts_in_row;
				indices[idx2 + 1] = i/2 + (strip + 2)*nr_verts_in_row;
			}
		}
	}
}

static void init( void )
{


	generate_verts();
	generate_colors();
	generate_indices();

	glClearColor( 0.0, 0.0, 0.0, 0.0 );
	glShadeModel( GL_SMOOTH );

	glEnableClientState( GL_VERTEX_ARRAY );
	glEnableClientState( GL_COLOR_ARRAY );

	glMatrixMode( GL_PROJECTION );
	glLoadIdentity();
	glFrustum( -100.0, 100.0, -100.0, 100.0, 1.0, 100.0 );
	glMatrixMode( GL_MODELVIEW );
	glLoadIdentity();

#ifdef GL_EXT_compiled_vertex_array
	if ( compiled ) {
		glLockArraysEXT( 0, rows * rows );
	}
#endif
}

static void display( void )
{
	glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

	glDrawElements( GL_TRIANGLE_STRIP, nr_indices_in_strip * (nr_verts_in_row - 1) , GL_UNSIGNED_INT, indices );

	if ( double_buffer )
		glutSwapBuffers();
	else
		glFlush();
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

    double_buffer = GL_TRUE;

    for ( i = 1 ; i < argc ; i++ ) {
	if ( strcmp( argv[i], "-sb" ) == 0 ) {
	    double_buffer = GL_FALSE;
	} else if ( strcmp( argv[i], "-db" ) == 0 ) {
	    double_buffer = GL_TRUE;
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
   type |= ( double_buffer ) ? GLUT_DOUBLE : GLUT_SINGLE;

   glutInitDisplayMode( type );
   glutInitWindowSize( 250, 250 );
   glutInitWindowPosition( 100, 100 );
   glutCreateWindow( "CVA Test" );

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
