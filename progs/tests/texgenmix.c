
/*
 * Demonstrates mixed texgen/non-texgen texture coordinates.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

#undef max
#undef min
#define max( a, b )	((a) >= (b) ? (a) : (b))
#define min( a, b )	((a) <= (b) ? (a) : (b))

GLfloat labelColor0[4] = { 1.0, 1.0, 1.0, 1.0 };
GLfloat labelColor1[4] = { 1.0, 1.0, 0.4, 1.0 };
GLfloat *labelInfoColor = labelColor0;

GLboolean doubleBuffered = GL_TRUE;
GLboolean drawTextured = GL_TRUE;

int textureWidth = 64;
int textureHeight = 64;

int winWidth = 580, winHeight = 720;

const GLfloat texmat_swap_rq[16] = { 1.0, 0.0, 0.0, 0.0,
                                     0.0, 1.0, 0.0, 0.0,
                                     0.0, 0.0, 0.0, 1.0,
                                     0.0, 0.0, 1.0, 0.0};

const GLfloat nullPlane[4] = { 0.0, 0.0, 0.0, 0.0 };
const GLfloat ObjPlaneS1[4] = { 1.0, 0.0, 1.0, 0.0 };
const GLfloat ObjPlaneS2[4] = { 0.5, 0.0, 0.0, 0.0 };
const GLfloat ObjPlaneS3[4] = { 1.0, 0.0, 0.0, 0.0 };
const GLfloat ObjPlaneT[4] = { 0.0, 1.0, 0.0, 0.0 };
const GLfloat ObjPlaneT2[4] = { 0.0, 0.5, 0.0, 0.0 };
const GLfloat ObjPlaneT3[4] = { 0.0, 1.0, 0.0, 0.0 };
const GLfloat ObjPlaneR[4] = { 0.0, 0.0, 1.0, 0.0 };
const GLfloat ObjPlaneQ[4] = { 0.0, 0.0, 0.0, 0.5 };


static void checkErrors( void )
{
   GLenum error;

   while ( (error = glGetError()) != GL_NO_ERROR ) {
      fprintf( stderr, "Error: %s\n", (char *) gluErrorString( error ) );
   }
}

static void drawString( const char *string, GLfloat x, GLfloat y,
                        const GLfloat color[4] )
{
   glColor4fv( color );
   glRasterPos2f( x, y );

   while ( *string ) {
      glutBitmapCharacter( GLUT_BITMAP_TIMES_ROMAN_10, *string );
      string++;
   }
}

static void begin2D( int width, int height )
{
   glMatrixMode( GL_PROJECTION );

   glPushMatrix();
   glLoadIdentity();

   glOrtho( 0, width, 0, height, -1, 1 );
   glMatrixMode( GL_MODELVIEW );

   glPushMatrix();
   glLoadIdentity();
}

static void end2D( void )
{
   glMatrixMode( GL_PROJECTION );
   glPopMatrix();
   glMatrixMode( GL_MODELVIEW );
   glPopMatrix();
}

static void initialize( void )
{
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();

   glOrtho( -1.5, 1.5, -1.5, 1.5, -1.5, 1.5 );

   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();

   glShadeModel( GL_FLAT );
}

/* ARGSUSED1 */
static void keyboard( unsigned char c, int x, int y )
{
   switch ( c ) {
   case 't':
      drawTextured = !drawTextured;
      break;
   case 27:             /* Escape key should force exit. */
      exit(0);
      break;
   default:
      break;
   }
   glutPostRedisplay();
}

/* ARGSUSED1 */
static void special( int key, int x, int y )
{
   switch ( key ) {
   case GLUT_KEY_DOWN:
      break;
   case GLUT_KEY_UP:
      break;
   case GLUT_KEY_LEFT:
      break;
   case GLUT_KEY_RIGHT:
      break;
   default:
      break;
   }
   glutPostRedisplay();
}

static void
reshape( int w, int h )
{
   winWidth = w;
   winHeight = h;
   /* No need to call glViewPort here since "draw" calls it! */
}

static void loadTexture( int width, int height )
{
   int		alphaSize = 1;
   int		rgbSize = 3;
   GLubyte	*texImage, *p;
   int		elementsPerGroup, elementSize, groupSize, rowSize;
   int		i, j;


   elementsPerGroup = alphaSize + rgbSize;
   elementSize = sizeof(GLubyte);
   groupSize = elementsPerGroup * elementSize;
   rowSize = width * groupSize;

   if ( (texImage = (GLubyte *) malloc( height * rowSize ) ) == NULL ) {
      fprintf( stderr, "texture malloc failed\n" );
      return;
   }

   for ( i = 0 ; i < height ; i++ )
   {
      p = texImage + i * rowSize;

      for ( j = 0 ; j < width ; j++ )
      {
	 if ( rgbSize > 0 )
	 {
	    /**
	     ** +-----+-----+
	     ** |     |     |
	     ** |  R  |  G  |
	     ** |     |     |
	     ** +-----+-----+
	     ** |     |     |
	     ** |  Y  |  B  |
	     ** |     |     |
	     ** +-----+-----+
	     **/
	    if ( i > height / 2 ) {
	       if ( j < width / 2 ) {
		  p[0] = 0xff;
		  p[1] = 0x00;
		  p[2] = 0x00;
	       } else {
		  p[0] = 0x00;
		  p[1] = 0xff;
		  p[2] = 0x00;
	       }
	    } else {
	       if ( j < width / 2 ) {
		  p[0] = 0xff;
		  p[1] = 0xff;
		  p[2] = 0x00;
	       } else {
		  p[0] = 0x00;
		  p[1] = 0x00;
		  p[2] = 0xff;
	       }
	    }
	    p += 3 * elementSize;
	 }

	 if ( alphaSize > 0 )
	 {
	    /**
	     ** +-----------+
	     ** |     W     |
	     ** |  +-----+  |
	     ** |  |     |  |
	     ** |  |  B  |  |
	     ** |  |     |  |
	     ** |  +-----+  |
	     ** |           |
	     ** +-----------+
	     **/
	    int i2 = i - height / 2;
	    int j2 = j - width / 2;
	    int h8 = height / 8;
	    int w8 = width / 8;
	    if ( -h8 <= i2 && i2 <= h8 && -w8 <= j2 && j2 <= w8 ) {
	       p[0] = 0x00;
	    } else if ( -2 * h8 <= i2 && i2 <= 2 * h8 && -2 * w8 <= j2 && j2 <= 2 * w8 ) {
	       p[0] = 0x55;
	    } else if ( -3 * h8 <= i2 && i2 <= 3 * h8 && -3 * w8 <= j2 && j2 <= 3 * w8 ) {
	       p[0] = 0xaa;
	    } else {
	       p[0] = 0xff;
	    }
	    p += elementSize;
	 }
      }
   }

   glTexImage2D( GL_TEXTURE_2D, 0,
		 GL_RGBA, width, height, 0,
		 GL_RGBA, GL_UNSIGNED_BYTE, texImage );

   free( texImage );
}


static void drawSample( int x, int y, int w, int h,
                        int texgenenabled, int coordnr )
{
   char buf[255];

   glViewport( x, y, w, h );
   glScissor( x, y, w, h );

   glClearColor( 0.1, 0.1, 0.1, 1.0 );
   glClear( GL_COLOR_BUFFER_BIT );

   begin2D( w, h );
   if (texgenenabled == 2) {
      sprintf( buf, "TexCoord%df", coordnr);
      drawString( buf, 10, h - 15, labelInfoColor );
      sprintf( buf, "texgen enabled for %s coordinate(s)", coordnr == 2 ? "S" : "S/T");
      drawString( buf, 10, 5, labelInfoColor );
   }
   else if (texgenenabled == 0) {
      sprintf( buf, "TexCoord%df", coordnr);
      drawString( buf, 10, h - 15, labelInfoColor );
      drawString( "no texgen", 10, 5, labelInfoColor );
   }
   else if (texgenenabled == 1) {
      drawString( "no TexCoord", 10, h - 15, labelInfoColor );
      sprintf( buf, "texgen enabled for %s coordinate(s)",
         coordnr == 2 ? "S/T" : (coordnr == 3 ? "S/T/R" : "S/T/R/Q"));
      drawString( buf, 10, 5, labelInfoColor );
   }

   end2D();

   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP );

   loadTexture( textureWidth, textureHeight );

   if ( drawTextured ) {
      glEnable( GL_TEXTURE_2D );
   }

   glDisable( GL_TEXTURE_GEN_S );
   glDisable( GL_TEXTURE_GEN_T );
   glDisable( GL_TEXTURE_GEN_R );
   glDisable( GL_TEXTURE_GEN_Q );

   glMatrixMode( GL_TEXTURE );
   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );
   glPushMatrix();

   switch (coordnr) {
   case 2:
      switch (texgenenabled) {
      case 0:
         glBegin( GL_QUADS );
         glTexCoord2f( 0.0, 0.0 );
         glVertex2f( -0.8, -0.8 );

         glTexCoord2f( 1.0, 0.0 );
         glVertex2f( 0.8, -0.8 );

         glTexCoord2f( 1.0, 1.0 );
         glVertex2f( 0.8, 0.8 );

         glTexCoord2f( 0.0, 1.0 );
         glVertex2f( -0.8, 0.8 );
         glEnd();
         break;
      case 1:
         glTranslatef( -0.8, -0.8, 0.0 );
         glScalef( 1.6, 1.6, 1.0 );
         glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGenfv(GL_S, GL_OBJECT_PLANE, ObjPlaneS3);
         glTexGenfv(GL_T, GL_OBJECT_PLANE, ObjPlaneT3);
         glTexGenfv(GL_R, GL_OBJECT_PLANE, nullPlane);
         glTexGenfv(GL_Q, GL_OBJECT_PLANE, nullPlane);

         glEnable( GL_TEXTURE_GEN_S );
         glEnable( GL_TEXTURE_GEN_T );

         /* Issue a texcoord here to be sure Q isn't left over from a
          * previous sample.
          */
         glTexCoord1f( 0.0 );
         glBegin( GL_QUADS );
         glVertex2f( 0.0, 0.0 );
         glVertex2f( 1.0, 0.0 );
         glVertex2f( 1.0, 1.0 );
         glVertex2f( 0.0, 1.0 );
         glEnd();
         break;
      case 2:
         /* make sure that texgen T and non-texgen S coordinate are wrong */
         glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGenfv(GL_S, GL_OBJECT_PLANE, ObjPlaneS1);
         glTexGenfv(GL_T, GL_OBJECT_PLANE, nullPlane);
         glTexGenfv(GL_R, GL_OBJECT_PLANE, nullPlane);
         glTexGenfv(GL_Q, GL_OBJECT_PLANE, nullPlane);

         glEnable( GL_TEXTURE_GEN_S );

         glBegin( GL_QUADS );
         /* use z coordinate to get correct texgen values... */
         glTexCoord2f( 0.0, 0.0 );
         glVertex3f( -0.8, -0.8, 0.8 );

         glTexCoord2f( 0.0, 0.0 );
         glVertex3f( 0.8, -0.8, 0.2 );

         glTexCoord2f( 0.0, 1.0 );
         glVertex3f( 0.8, 0.8, 0.2 );

         glTexCoord2f( 0.0, 1.0 );
         glVertex3f( -0.8, 0.8, 0.8 );
         glEnd();
         break;
      }
      break;
   case 3:
      glMatrixMode( GL_TEXTURE );
      glLoadMatrixf( texmat_swap_rq );
      glMatrixMode( GL_MODELVIEW );
      glTranslatef( -0.8, -0.8, 0.0 );
      glScalef( 1.6, 1.6, 1.0 );
      switch (texgenenabled) {
      case 0:
         glBegin( GL_QUADS );
         glTexCoord3f( 0.0, 0.0, 0.5 );
         glVertex2f( 0.0, 0.0 );

         glTexCoord3f( 0.5, 0.0, 0.5 );
         glVertex2f( 1.0, 0.0 );

         glTexCoord3f( 0.5, 0.5, 0.5 );
         glVertex2f( 1.0, 1.0 );

         glTexCoord3f( 0.0, 0.5, 0.5 );
         glVertex2f( 0.0, 1.0 );
         glEnd();
         break;
      case 1:
         glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGenfv(GL_S, GL_OBJECT_PLANE, ObjPlaneS2);
         glTexGenfv(GL_T, GL_OBJECT_PLANE, ObjPlaneT2);
         glTexGenfv(GL_R, GL_OBJECT_PLANE, ObjPlaneR);
         glTexGenfv(GL_Q, GL_OBJECT_PLANE, nullPlane);

         glEnable( GL_TEXTURE_GEN_S );
         glEnable( GL_TEXTURE_GEN_T );
         glEnable( GL_TEXTURE_GEN_R );

         glTexCoord1f( 0.0 ); /* to make sure Q is 1.0 */
         glBegin( GL_QUADS );
         glVertex3f( 0.0, 0.0, 0.5 );
         glVertex3f( 1.0, 0.0, 0.5 );
         glVertex3f( 1.0, 1.0, 0.5 );
         glVertex3f( 0.0, 1.0, 0.5 );
         glEnd();
         break;
      case 2:
         /* make sure that texgen R/Q and non-texgen S/T coordinates are wrong */
         glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGenfv(GL_S, GL_OBJECT_PLANE, ObjPlaneS2);
         glTexGenfv(GL_T, GL_OBJECT_PLANE, ObjPlaneT2);
         glTexGenfv(GL_R, GL_OBJECT_PLANE, nullPlane);
         glTexGenfv(GL_Q, GL_OBJECT_PLANE, nullPlane);

         glEnable( GL_TEXTURE_GEN_S );
         glEnable( GL_TEXTURE_GEN_T );

         glBegin( GL_QUADS );
         glTexCoord3f( 0.0, 0.0, 0.5 );
         glVertex2f( 0.0, 0.0);

         glTexCoord3f( 0.0, 0.0, 0.5 );
         glVertex2f( 1.0, 0.0);

         glTexCoord3f( 0.0, 0.0, 0.5 );
         glVertex2f( 1.0, 1.0);

         glTexCoord3f( 0.0, 0.0, 0.5 );
         glVertex2f( 0.0, 1.0);
         glEnd();
         break;
      }
      break;
   case 4:
      switch (texgenenabled) {
      case 0:
         glBegin( GL_QUADS );
         /* don't need r coordinate but still setting it I'm mean */
         glTexCoord4f( 0.0, 0.0, 0.0, 0.5 );
         glVertex2f( -0.8, -0.8 );

         glTexCoord4f( 0.5, 0.0, 0.2, 0.5 );
         glVertex2f( 0.8, -0.8 );

         glTexCoord4f( 0.5, 0.5, 0.5, 0.5 );
         glVertex2f( 0.8, 0.8 );

         glTexCoord4f( 0.0, 0.5, 0.5, 0.5 );
         glVertex2f( -0.8, 0.8 );
         glEnd();
         break;
      case 1:
         glTranslatef( -0.8, -0.8, 0.0 );
         glScalef( 1.6, 1.6, 1.0 );
         /* make sure that texgen R/Q and non-texgen S/T coordinates are wrong */
         glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGenfv(GL_S, GL_OBJECT_PLANE, ObjPlaneS2);
         glTexGenfv(GL_T, GL_OBJECT_PLANE, ObjPlaneT2);
         glTexGenfv(GL_R, GL_OBJECT_PLANE, ObjPlaneR);
         glTexGenfv(GL_Q, GL_OBJECT_PLANE, ObjPlaneQ);

         glEnable( GL_TEXTURE_GEN_S );
         glEnable( GL_TEXTURE_GEN_T );
         glEnable( GL_TEXTURE_GEN_R );
         glEnable( GL_TEXTURE_GEN_Q );

         glBegin( GL_QUADS );
         glVertex2f( 0.0, 0.0 );
         glVertex2f( 1.0, 0.0 );
         glVertex2f( 1.0, 1.0 );
         glVertex2f( 0.0, 1.0 );
         glEnd();
         break;
      case 2:
         glTranslatef( -0.8, -0.8, 0.0 );
         glScalef( 1.6, 1.6, 1.0 );
         /* make sure that texgen R/Q and non-texgen S/T coordinates are wrong */
         glTexGeni( GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_R, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGeni( GL_Q, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR );
         glTexGenfv(GL_S, GL_OBJECT_PLANE, ObjPlaneS2);
         glTexGenfv(GL_T, GL_OBJECT_PLANE, ObjPlaneT2);
         glTexGenfv(GL_R, GL_OBJECT_PLANE, nullPlane);
         glTexGenfv(GL_Q, GL_OBJECT_PLANE, nullPlane);

         glEnable( GL_TEXTURE_GEN_S );
         glEnable( GL_TEXTURE_GEN_T );

         glBegin( GL_QUADS );
         glTexCoord4f( 0.0, 0.0, 0.0, 0.5 );
         glVertex2f( 0.0, 0.0 );

         glTexCoord4f( 0.0, 0.0, 0.2, 0.5 );
         glVertex2f( 1.0, 0.0 );

         glTexCoord4f( 0.0, 0.0, 0.5, 0.5 );
         glVertex2f( 1.0, 1.0 );

         glTexCoord4f( 0.0, 0.0, 0.75, 0.5 );
         glVertex2f( 0.0, 1.0 );
         glEnd();
         break;
      }
      break;
   }

   glPopMatrix();
   glDisable( GL_TEXTURE_2D );

}

static void display( void )
{
   int		numX = 3, numY = 3;
   float	xBase = (float) winWidth * 0.01;
   float	xOffset = (winWidth - xBase) / numX;
   float	xSize = max( xOffset - xBase, 1 );
   float	yBase = (float) winHeight * 0.01;
   float	yOffset = (winHeight - yBase) / numY;
   float	ySize = max( yOffset - yBase, 1 );
   float	x, y;
   int		i, j;

   glViewport( 0, 0, winWidth, winHeight );
   glDisable( GL_SCISSOR_TEST );
   glClearColor( 0.0, 0.0, 0.0, 0.0 );
   glClear( GL_COLOR_BUFFER_BIT );
   glEnable( GL_SCISSOR_TEST );

   x = xBase;
   y = (winHeight - 1) - yOffset;

   for ( i = 0 ; i < numY ; i++ )
   {

      labelInfoColor = labelColor1;


      for ( j = 0 ; j < numX ; j++ ) {
	 drawSample( x, y, xSize, ySize, i, j+2 );
	 x += xOffset;
      }

      x = xBase;
      y -= yOffset;
   }

   if ( doubleBuffered ) {
      glutSwapBuffers();
   } else {
      glFlush();
   }

   checkErrors();
}

static void usage( char *name )
{
   fprintf( stderr, "usage: %s [ options ]\n", name );
   fprintf( stderr, "\n" );
   fprintf( stderr, "options:\n" );
   fprintf( stderr, "    -sb    single buffered\n" );
   fprintf( stderr, "    -db    double buffered\n" );
   fprintf( stderr, "    -info  print OpenGL driver info\n" );
}

static void instructions( void )
{
   fprintf( stderr, "texgenmix - mixed texgen/non-texgen texture coordinate test\n" );
   fprintf( stderr, "all quads should look the same!\n" );
   fprintf( stderr, "\n" );
   fprintf( stderr, "  [t] - toggle texturing\n" );
}

int main( int argc, char *argv[] )
{
   GLboolean info = GL_FALSE;
   int i;

   glutInit( &argc, argv );

   for ( i = 1 ; i < argc ; i++ ) {
      if ( !strcmp( "-sb", argv[i] ) ) {
	 doubleBuffered = GL_FALSE;
      } else if ( !strcmp( "-db", argv[i] ) ) {
	 doubleBuffered = GL_TRUE;
      } else if ( !strcmp( "-info", argv[i] ) ) {
	 info = GL_TRUE;
      } else {
	 usage( argv[0] );
	 exit( 1 );
      }
   }

   if ( doubleBuffered ) {
      glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   } else {
      glutInitDisplayMode( GLUT_RGB | GLUT_SINGLE );
   }

   glutInitWindowSize( winWidth, winHeight );
   glutInitWindowPosition( 0, 0 );
   glutCreateWindow( "Mixed texgen/non-texgen texture coordinate test" );
   glewInit();

   initialize();
   instructions();

   if ( info ) {
      printf( "\n" );
      printf( "GL_RENDERER   = %s\n", (char *) glGetString( GL_RENDERER ) );
      printf( "GL_VERSION    = %s\n", (char *) glGetString( GL_VERSION ) );
      printf( "GL_VENDOR     = %s\n", (char *) glGetString( GL_VENDOR ) ) ;
      printf( "GL_EXTENSIONS = %s\n", (char *) glGetString( GL_EXTENSIONS ) );
   }

   glutDisplayFunc( display );
   glutReshapeFunc( reshape );
   glutKeyboardFunc( keyboard );
   glutSpecialFunc( special );
   glutMainLoop();

   return 0;
}
