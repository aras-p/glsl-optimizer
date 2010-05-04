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
 * \file interleave.c
 * 
 * Simple test of glInterleavedArrays functionality.  For each mode, two
 * meshes are drawn.  One is drawn using interleaved arrays and the othe is
 * drawn using immediate mode.  Both should look identical.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Width = 400;
static int Height = 300;
static const GLfloat Near = 5.0, Far = 25.0;

static const GLfloat t[][4] = {
   { 0.5,  0.0, 0.0, 1.0 },

   { 0.25, 0.5, 0.0, 1.0 },
   { 0.75, 0.5, 0.0, 1.0 },

   { 0.0,  1.0, 0.0, 1.0 },
   { 0.5,  1.0, 0.0, 1.0 },
   { 1.0,  1.0, 0.0, 1.0 },
};

static const GLfloat c_f[][4] = {
   { 1.0, 0.0, 0.0, 1.0 },

   { 0.0, 1.0, 0.0, 1.0 },
   { 0.0, 1.0, 0.0, 1.0 },

   { 0.0, 0.0, 1.0, 1.0 },
   { 1.0, 0.0, 1.0, 1.0 },
   { 0.0, 0.0, 1.0, 1.0 },
};

static const GLubyte c_ub[][4] = {
   { 0xff, 0x00, 0x00, 0xff },

   { 0x00, 0xff, 0x00, 0xff },
   { 0x00, 0xff, 0x00, 0xff },

   { 0x00, 0x00, 0xff, 0xff },
   { 0xff, 0x00, 0xff, 0xff },
   { 0x00, 0x00, 0xff, 0xff },
};

static const GLfloat n[][3] = {
   { 0.0, 0.0, -1.0 },

   { 0.0, 0.0, -1.0 },
   { 0.0, 0.0, -1.0 },

   { 0.0, 0.0, -1.0 },
   { 0.0, 0.0, -1.0 },
   { 0.0, 0.0, -1.0 },
};

static const GLfloat v[][4] = {
   {  0.0,  1.0, 0.0, 1.0, },

   { -0.5,  0.0, 0.0, 1.0, },
   {  0.5,  0.0, 0.0, 1.0, },

   { -1.0, -1.0, 0.0, 1.0, },
   {  0.0, -1.0, 0.0, 1.0, },
   {  1.0, -1.0, 0.0, 1.0, },
};
   
static const unsigned indicies[12] = {
   0, 1, 2,
   1, 3, 4,
   2, 4, 5,
   1, 4, 2
};

#define NONE  { NULL, 0, 0, 0, sizeof( NULL ) }
#define V2F   { v,    2, 2 * sizeof( GLfloat ), GL_FLOAT, sizeof( v[0] ) }
#define V3F   { v,    3, 3 * sizeof( GLfloat ), GL_FLOAT, sizeof( v[0] ) }
#define V4F   { v,    4, 4 * sizeof( GLfloat ), GL_FLOAT, sizeof( v[0] ) }

#define C4UB  { c_ub, 4, 4 * sizeof( GLubyte ), GL_UNSIGNED_BYTE, sizeof( c_ub[0] ) }
#define C3F   { c_f,  3, 3 * sizeof( GLfloat ), GL_FLOAT,         sizeof( c_f[0] ) }
#define C4F   { c_f,  4, 4 * sizeof( GLfloat ), GL_FLOAT,         sizeof( c_f[0] ) }

#define T2F   { t,    2, 2 * sizeof( GLfloat ), GL_FLOAT, sizeof( t[0] ) }
#define T4F   { t,    4, 4 * sizeof( GLfloat ), GL_FLOAT, sizeof( t[0] ) }

#define N3F   { n,    3, 3 * sizeof( GLfloat ), GL_FLOAT, sizeof( n[0] ) }

struct interleave_info {
   const void * data;
   unsigned count;
   unsigned size;
   GLenum type;
   unsigned stride;
};

#define NUM_MODES      14
#define INVALID_MODE   14
#define INVALID_STRIDE 15

struct interleave_info info[ NUM_MODES ][4] = {
   { NONE, NONE, NONE, V2F },
   { NONE, NONE, NONE, V3F },
   { NONE, C4UB, NONE, V2F },
   { NONE, C4UB, NONE, V3F },
   { NONE, C3F,  NONE, V3F },

   { NONE, NONE, N3F,  V3F },
   { NONE, C4F,  N3F,  V3F },

   { T2F,  NONE, NONE, V3F },
   { T4F,  NONE, NONE, V4F },

   { T2F,  C4UB, NONE, V3F },
   { T2F,  C3F,  NONE, V3F },
   { T2F,  NONE, N3F,  V3F },
   { T2F,  C4F,  N3F,  V3F },
   { T4F,  C4F,  N3F,  V4F },
};

const char * const mode_names[ NUM_MODES ] = {
   "GL_V2F",
   "GL_V3F",
   "GL_C4UB_V2F",
   "GL_C4UB_V3F",
   "GL_C3F_V3F",
   "GL_N3F_V3F",
   "GL_C4F_N3F_V3F",
   "GL_T2F_V3F",
   "GL_T4F_V4F",
   "GL_T2F_C4UB_V3F",
   "GL_T2F_C3F_V3F",
   "GL_T2F_N3F_V3F",
   "GL_T2F_C4F_N3F_V3F",
   "GL_T4F_C4F_N3F_V4F",
};

static unsigned interleave_mode = 0;
static GLboolean use_invalid_mode = GL_FALSE;
static GLboolean use_invalid_stride = GL_FALSE;

#define DEREF(item,idx) (void *) & ((char *)curr_info[item].data)[idx * curr_info[item].stride]

static void Display( void )
{
   const struct interleave_info * const curr_info = info[ interleave_mode ];

   /* 4 floats for 12 verticies for 4 data elements.
    */
   char data[ (sizeof( GLfloat ) * 4) * 12 * 4 ];

   unsigned i;
   unsigned offset;
   GLenum err;
   GLenum format;
   GLsizei stride;


   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();

   glTranslatef(-1.5, 0, 0);

   glColor3fv( c_f[0] );

   if ( curr_info[0].data != NULL ) {
      glEnable( GL_TEXTURE_2D );
   }
   else {
      glDisable( GL_TEXTURE_2D );
   }


   offset = 0;
   glBegin(GL_TRIANGLES);
   for ( i = 0 ; i < 12 ; i++ ) {
      const unsigned index = indicies[i];


      /* Handle the vertex texture coordinate.
       */
      if ( curr_info[0].data != NULL ) {
	 if ( curr_info[0].count == 2 ) {
	    glTexCoord2fv( DEREF(0, index) );
	 }
	 else {
	    glTexCoord4fv( DEREF(0, index) );
	 }

	 (void) memcpy( & data[ offset ], DEREF(0, index),
			curr_info[0].size );
	 offset += curr_info[0].size;
      }


      /* Handle the vertex color.
       */
      if ( curr_info[1].data != NULL ) {
	 if ( curr_info[1].type == GL_FLOAT ) {
	    if ( curr_info[1].count == 3 ) {
	       glColor3fv( DEREF(1, index) );
	    }
	    else {
	       glColor4fv( DEREF(1, index) );
	    }
	 }
	 else {
	    glColor4ubv( DEREF(1, index) );
	 }

	 (void) memcpy( & data[ offset ], DEREF(1, index), 
			curr_info[1].size );
	 offset += curr_info[1].size;
      }

      
      /* Handle the vertex normal.
       */
      if ( curr_info[2].data != NULL ) {
	 glNormal3fv( DEREF(2, index) );

	 (void) memcpy( & data[ offset ], DEREF(2, index),
			curr_info[2].size );
	 offset += curr_info[2].size;
      }


      switch( curr_info[3].count ) {
      case 2:
	 glVertex2fv( DEREF(3, index) );
	 break;
      case 3:
	 glVertex3fv( DEREF(3, index) );
	 break;
      case 4:
	 glVertex4fv( DEREF(3, index) );
	 break;
      }

      (void) memcpy( & data[ offset ], DEREF(3, index),
		     curr_info[3].size );
	 offset += curr_info[3].size;
   }
   glEnd();


   glTranslatef(3.0, 0, 0);

   /* The masking with ~0x2A00 is a bit of a hack to make sure that format
    * ends up with an invalid value no matter what rand() returns.
    */
   format = (use_invalid_mode)
     ? (rand() & ~0x2A00) : GL_V2F + interleave_mode;
   stride = (use_invalid_stride) ? -abs(rand()) : 0;

   (void) glGetError();
   glInterleavedArrays( format, stride, data );
   err = glGetError();
   if ( err ) {
      printf("glInterleavedArrays(0x%04x, %d, %p) generated the error 0x%04x\n",
	     format, stride, data, err );
   }
   else {
      glDrawArrays( GL_TRIANGLES, 0, 12 );
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


static void ModeMenu( int entry )
{
   if ( entry == INVALID_MODE ) {
      use_invalid_mode = GL_TRUE;
      use_invalid_stride = GL_FALSE;
   }
   else if ( entry == INVALID_STRIDE ) {
      use_invalid_mode = GL_FALSE;
      use_invalid_stride = GL_TRUE;
   }
   else {
      use_invalid_mode = GL_FALSE;
      use_invalid_stride = GL_FALSE;
      interleave_mode = entry;
   }
}

static void Init( void )
{
   const char * const ver_string = (const char *)
       glGetString( GL_VERSION );
   const GLubyte tex[16] = {
      0xff, 0x00, 0xff, 0x00, 
      0x00, 0xff, 0x00, 0xff, 
      0xff, 0x00, 0xff, 0x00, 
      0x00, 0xff, 0x00, 0xff, 
   };

   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", ver_string);

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );
   glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, 4, 4, 0,
		 GL_LUMINANCE, GL_UNSIGNED_BYTE, tex );

   printf("Use the context menu (right click) to select the interleaved array mode.\n");
   printf("Press ESCAPE to exit.\n\n");
   printf("NOTE: This is *NOT* a very good test of the modes that use normals.\n");
}


int main( int argc, char *argv[] )
{
   unsigned i;

   srand( time( NULL ) );

   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "glInterleavedArrays test" );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );

   glutCreateMenu( ModeMenu );
   for ( i = 0 ; i < NUM_MODES ; i++ ) {
      glutAddMenuEntry( mode_names[i], i);
   }

   glutAddMenuEntry( "Random invalid mode",   INVALID_MODE);
   glutAddMenuEntry( "Random invalid stride", INVALID_STRIDE);

   glutAttachMenu(GLUT_RIGHT_BUTTON);

   Init();
   glutMainLoop();
   return 0;
}
