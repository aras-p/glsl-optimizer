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
 * IBM AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

const GLenum filter_modes[] = {
     GL_NEAREST,
     GL_LINEAR,
     GL_NEAREST_MIPMAP_NEAREST,
     GL_NEAREST_MIPMAP_LINEAR,
     GL_LINEAR_MIPMAP_NEAREST,
     GL_LINEAR_MIPMAP_LINEAR,
};

static GLenum min_filter = GL_LINEAR_MIPMAP_LINEAR;
static GLenum mag_filter = GL_LINEAR;

static unsigned segments = 64;
static GLfloat * position_data = NULL;
static GLfloat * texcoord_data = NULL;
static GLfloat max_anisotropy = 0.0;
static GLfloat anisotropy = 1.0;

static void generate_tunnel( unsigned num_segs, GLfloat ** pos_data,
    GLfloat ** tex_data );
static void generate_textures( unsigned mode );

#define min(a,b)  ( (a) < (b) ) ? (a) : (b)
#define max(a,b)  ( (a) > (b) ) ? (a) : (b)


static void Display( void )
{
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, min_filter );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, mag_filter );
   
   if ( max_anisotropy > 0.0 ) {
      glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY_EXT,
		       anisotropy );
   }

   glClear( GL_COLOR_BUFFER_BIT );
   glLoadIdentity();
   glTranslatef( 0.0f, 0.0f, -19.0f );

   glVertexPointer( 4, GL_FLOAT, 0, position_data );
   glTexCoordPointer( 2, GL_FLOAT, 0, texcoord_data );
   glEnableClientState( GL_VERTEX_ARRAY );
   glEnableClientState( GL_TEXTURE_COORD_ARRAY );
   glDrawArrays( GL_QUADS, 0, 4 * segments );
   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport(0, 0, width, height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   gluPerspective(45.0f, (GLfloat)(width)/(GLfloat)(height), 0.1f, 100.0f);
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   GLfloat new_anisotropy = anisotropy;

   (void) x;
   (void) y;


   switch( key ) {
   case 'a': {
      new_anisotropy = anisotropy - 1.0;
      break;
   }

   case 'A': {
      new_anisotropy = anisotropy + 1.0;
      break;
   }

   case 's': {
      segments--;
      if ( segments < 3 ) {
	 segments = 3;
      }
      generate_tunnel( segments, & position_data, & texcoord_data );
      break;
   }

   case 'S': {
      segments++;
      if ( segments > 128 ) {
	 segments = 128;
      }
      generate_tunnel( segments, & position_data, & texcoord_data );
      break;
   }
   case 'q':
   case 'Q':
   case 27:
      exit(0);
      break;
   }

   new_anisotropy = max( new_anisotropy, 1.0 );
   new_anisotropy = min( new_anisotropy, max_anisotropy );
   if ( new_anisotropy != anisotropy ) {
      anisotropy = new_anisotropy;
      printf( "Texture anisotropy: %f%s\n", anisotropy,
	      (anisotropy == 1.0) ? " (disabled)" : "" );
   }

   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   (void) key;
   glutPostRedisplay();
}


static void menu_handler( int selection )
{
   switch( selection >> 3 ) {
   case 0:
      glBindTexture( GL_TEXTURE_2D, selection );
      break;
      
   case 1:
      min_filter = filter_modes[ selection & 7 ];
      break;

   case 2:
      mag_filter = filter_modes[ selection & 7 ];
      break;
   }

   glutPostRedisplay();
}


static void Init( void )
{
   glDisable(GL_CULL_FACE);
   glEnable(GL_TEXTURE_2D);
   glClearColor(0.0f, 0.0f, 0.4f, 0.0f);
   glShadeModel(GL_SMOOTH);
   glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);

   generate_tunnel( segments, & position_data, & texcoord_data );

   glBindTexture( GL_TEXTURE_2D, 1 );
   generate_textures(1);
   
   glBindTexture( GL_TEXTURE_2D, 2 );
   generate_textures(2);

   glBindTexture( GL_TEXTURE_2D, 3 );
   generate_textures(3);

   if ( glutExtensionSupported( "GL_EXT_texture_filter_anisotropic" ) ) {
      glGetFloatv( GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, & max_anisotropy );
   }
   
   printf("Maximum texture anisotropy: %f\n", max_anisotropy );
   
   /* Create the menus. */

   glutCreateMenu( menu_handler );
   glutAddMenuEntry( "Min filter: GL_NEAREST",                 8 + 0 );
   glutAddMenuEntry( "Min filter: GL_LINEAR",                  8 + 1 );
   glutAddMenuEntry( "Min filter: GL_NEAREST_MIMMAP_NEAREST",  8 + 2 );
   glutAddMenuEntry( "Min filter: GL_NEAREST_MIMMAP_LINEAR",   8 + 3 );
   glutAddMenuEntry( "Min filter: GL_LINEAR_MIMMAP_NEAREST",   8 + 4 );
   glutAddMenuEntry( "Min filter: GL_LINEAR_MIMMAP_LINEAR",    8 + 5 );
   glutAddMenuEntry( "Mag filter: GL_NEAREST",                16 + 0 );
   glutAddMenuEntry( "Mag filter: GL_LINEAR",                 16 + 1 );
   glutAddMenuEntry( "Texture: regular mipmaps",                   1 );
   glutAddMenuEntry( "Texture: blended mipmaps",                   2 );
   glutAddMenuEntry( "Texture: color mipmaps",                     3 );
   glutAttachMenu( GLUT_RIGHT_BUTTON );
}


static void generate_tunnel( unsigned num_segs, GLfloat ** pos_data,
			     GLfloat ** tex_data )
{
   const GLfloat far_distance = 20.0f;
   const GLfloat near_distance = -90.0f;
   const GLfloat far_tex = 30.0f;
   const GLfloat near_tex = 0.0f;
   const GLfloat angle_step = (2 * M_PI) / num_segs;
   const GLfloat tex_coord_step = 2.0 / num_segs;
   GLfloat angle = 0.0f;
   GLfloat tex_coord = 0.0f;
   unsigned i;
   GLfloat * position;
   GLfloat * texture;


   position = realloc( *pos_data, sizeof( GLfloat ) * num_segs * 4 * 4 );
   texture = realloc( *tex_data, sizeof( GLfloat ) * num_segs * 4 * 2 );

   *pos_data = position;
   *tex_data = texture;

   for ( i = 0 ; i < num_segs ; i++ ) {
      position[0] = 2.5 * sinf( angle );
      position[1] = 2.5 * cosf( angle );
      position[2] = (i & 1) ? far_distance : near_distance;
      position[3] = 1.0f;

      position[4] = position[0];
      position[5] = position[1];
      position[6] = (i & 1) ? near_distance : far_distance;
      position[7] = 1.0f;

      position += 8;

      texture[0] = tex_coord;
      texture[1] = (i & 1) ? far_tex : near_tex;
      texture += 2;

      texture[0] = tex_coord;
      texture[1] = (i & 1) ? near_tex : far_tex;
      texture += 2;

      angle += angle_step;
      tex_coord += tex_coord_step;

      position[0] = 2.5 * sinf( angle );
      position[1] = 2.5 * cosf( angle );
      position[2] = (i & 1) ? near_distance : far_distance;
      position[3] = 1.0f;

      position[4] = position[0];
      position[5] = position[1];
      position[6] = (i & 1) ? far_distance : near_distance;
      position[7] = 1.0f;

      position += 8;

      texture[0] = tex_coord;
      texture[1] = (i & 1) ? near_tex : far_tex;
      texture += 2;

      texture[0] = tex_coord;
      texture[1] = (i & 1) ? far_tex : near_tex;
      texture += 2;
   }
}


static void generate_textures( unsigned mode )
{
#define LEVEL_COLORS 6
   const GLfloat colors[LEVEL_COLORS][3] = {
	{ 1.0, 0.0, 0.0 },  /* 32 x 32 */
	{ 0.0, 1.0, 0.0 },  /* 16 x 16 */
	{ 0.0, 0.0, 1.0 },  /*  8 x  8 */
	{ 1.0, 0.0, 1.0 },  /*  4 x  4 */
	{ 1.0, 1.0, 1.0 },  /*  2 x  2 */
	{ 1.0, 1.0, 0.0 }   /*  1 x  1 */
   };
   const unsigned checkers_per_level = 2;
   GLfloat * tex;
   unsigned level;
   unsigned size;
   GLint max_size;


   glGetIntegerv( GL_MAX_TEXTURE_SIZE, & max_size );
   if ( max_size > 512 ) {
      max_size = 512;
   }

   tex = malloc( sizeof( GLfloat ) * 3 * max_size * max_size );

   level = 0;
   for ( size = max_size ; size > 0 ; size >>= 1 ) {
      unsigned divisor = size / checkers_per_level;
      unsigned i;
      unsigned j;
      GLfloat checkers[2][3];


      if ((level == 0) || (mode == 1)) {
	 checkers[0][0] = 1.0;
	 checkers[0][1] = 1.0;
	 checkers[0][2] = 1.0;
	 checkers[1][0] = 0.0;
	 checkers[1][1] = 0.0;
	 checkers[1][2] = 0.0;
      }
      else if (mode == 2) {
	 checkers[0][0] = colors[level % LEVEL_COLORS][0];
	 checkers[0][1] = colors[level % LEVEL_COLORS][1];
	 checkers[0][2] = colors[level % LEVEL_COLORS][2];
	 checkers[1][0] = colors[level % LEVEL_COLORS][0] * 0.5;
	 checkers[1][1] = colors[level % LEVEL_COLORS][1] * 0.5;
	 checkers[1][2] = colors[level % LEVEL_COLORS][2] * 0.5;
      }
      else {
	 checkers[0][0] = colors[level % LEVEL_COLORS][0];
	 checkers[0][1] = colors[level % LEVEL_COLORS][1];
	 checkers[0][2] = colors[level % LEVEL_COLORS][2];
	 checkers[1][0] = colors[level % LEVEL_COLORS][0];
	 checkers[1][1] = colors[level % LEVEL_COLORS][1];
	 checkers[1][2] = colors[level % LEVEL_COLORS][2];
      }

      if ( divisor == 0 ) {
	 divisor = 1;

	 checkers[0][0] = (checkers[0][0] + checkers[1][0]) / 2;
	 checkers[0][1] = (checkers[0][0] + checkers[1][0]) / 2;
	 checkers[0][2] = (checkers[0][0] + checkers[1][0]) / 2;
	 checkers[1][0] = checkers[0][0];
	 checkers[1][1] = checkers[0][1];
	 checkers[1][2] = checkers[0][2];
      }


      for ( i = 0 ; i < size ; i++ ) {
	 for ( j = 0 ; j < size ; j++ ) {
	    const unsigned idx = ((i ^ j) / divisor) & 1;

	    tex[ ((i * size) + j) * 3 + 0] = checkers[ idx ][0];
	    tex[ ((i * size) + j) * 3 + 1] = checkers[ idx ][1];
	    tex[ ((i * size) + j) * 3 + 2] = checkers[ idx ][2];
	 }
      }
      
      glTexImage2D( GL_TEXTURE_2D, level, GL_RGB, size, size, 0,
		    GL_RGB, GL_FLOAT, tex );
      level++;
   }

   free( tex );
}


int main( int argc, char ** argv )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 800, 600 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow( "Texture Filter Test" );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );

   Init();

   printf("\nUse the right-button menu to select the texture and filter mode.\n");
   printf("Use 'A' and 'a' to increase and decrease the aniotropy.\n");
   printf("Use 'S' and 's' to increase and decrease the number of cylinder segments.\n");
   printf("Use 'q' to exit.\n\n");

   glutMainLoop();
   return 0;
}
