/* $Id: tessdemo.c,v 1.5 2000/01/24 22:54:05 gareth Exp $ */

/*
 * A demo of the GLU polygon tesselation functions written by Bogdan Sikorski.
 * This demo isn't built by the Makefile because it needs GLUT.  After you've
 * installed GLUT you can try this demo.
 * Here's the command for IRIX, for example:
   cc -g -ansi -prototypes -fullwarn -float -I../include -DSHM tess_demo.c -L../lib -lglut -lMesaGLU -lMesaGL -lm -lX11 -lXext -lXmu -lfpe -lXext -o tess_demo
 */

/*
 * Updated for GLU 1.3 tessellation by Gareth Hughes <garethh@bell-labs.com>
 */

/*
 * $Log: tessdemo.c,v $
 * Revision 1.5  2000/01/24 22:54:05  gareth
 * Removed '#if 0' from second pass.
 *
 * Revision 1.4  2000/01/23 21:25:39  gareth
 * Merged 3.2 updates, namely combine callback for intersecting
 * contours.
 *
 * Revision 1.3.2.1  1999/11/16 11:09:09  gareth
 * Added combine callback.  Converted vertices from ints to floats.
 *
 * Revision 1.3  1999/11/04 04:00:42  gareth
 * Updated demo for new GLU 1.3 tessellation.  Added optimized rendering
 * by saving the output of the tessellation into display lists.
 *
 * Revision 1.2  1999/09/19 20:09:00  tanner
 *
 * lots of autoconf updates
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 3.5  1999/03/28 18:24:37  brianp
 * minor clean-up
 *
 * Revision 3.4  1999/02/14 03:37:07  brianp
 * fixed callback problem
 *
 * Revision 3.3  1998/07/26 01:25:26  brianp
 * removed include of gl.h and glu.h
 *
 * Revision 3.2  1998/06/29 02:37:30  brianp
 * minor changes for Windows (Ted Jump)
 *
 * Revision 3.1  1998/06/09 01:53:49  brianp
 * main() should return an int
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */


#include <GL/glut.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_POINTS	256
#define MAX_CONTOURS	32
#define MAX_TRIANGLES	256

#ifndef GLCALLBACK
#ifdef CALLBACK
#define GLCALLBACK CALLBACK
#else
#define GLCALLBACK
#endif
#endif

typedef enum{ QUIT, TESSELATE, CLEAR } menu_entries;
typedef enum{ DEFINE, TESSELATED } mode_type;

static GLsizei		width, height;
static GLuint		contour_cnt;
static GLuint		triangle_cnt;

static mode_type 	mode;
static int		menu;

static GLuint		list_start;

static GLfloat		edge_color[3];

static struct
{
   GLfloat	p[MAX_POINTS][2];
   GLuint	point_cnt;
} contours[MAX_CONTOURS];

static struct
{
   GLsizei	no;
   GLfloat	p[3][2];
   GLclampf	color[3][3];
} triangles[MAX_TRIANGLES];



void GLCALLBACK error_callback( GLenum err )
{
   int		len, i;
   char const	*str;

   glColor3f( 0.9, 0.9, 0.9 );
   glRasterPos2i( 5, 5 );

   str = (const char *) gluErrorString( err );
   len = strlen( str );

   for ( i = 0 ; i < len ; i++ ) {
      glutBitmapCharacter( GLUT_BITMAP_9_BY_15, str[i] );
   }
}

void GLCALLBACK begin_callback( GLenum mode )
{
   /* Allow multiple triangles to be output inside the begin/end pair. */
   triangle_cnt = 0;
   triangles[triangle_cnt].no = 0;
}

void GLCALLBACK edge_callback( GLenum flag )
{
   /* Persist the edge flag across triangles. */
   if ( flag == GL_TRUE )
   {
      edge_color[0] = 1.0;
      edge_color[1] = 1.0;
      edge_color[2] = 0.5;
   }
   else
   {
      edge_color[0] = 1.0;
      edge_color[1] = 0.0;
      edge_color[2] = 0.0;
   }
}

void GLCALLBACK end_callback()
{
   GLint	i;

   glBegin( GL_LINES );

   /* Output the three edges of each triangle as lines colored
      according to their edge flag. */
   for ( i = 0 ; i < triangle_cnt ; i++ )
   {
      glColor3f( triangles[i].color[0][0],
		 triangles[i].color[0][1],
		 triangles[i].color[0][2] );

      glVertex2f( triangles[i].p[0][0], triangles[i].p[0][1] );
      glVertex2f( triangles[i].p[1][0], triangles[i].p[1][1] );

      glColor3f( triangles[i].color[1][0],
		 triangles[i].color[1][1],
		 triangles[i].color[1][2] );

      glVertex2f( triangles[i].p[1][0], triangles[i].p[1][1] );
      glVertex2f( triangles[i].p[2][0], triangles[i].p[2][1] );

      glColor3f( triangles[i].color[2][0],
		 triangles[i].color[2][1],
		 triangles[i].color[2][2] );

      glVertex2f( triangles[i].p[2][0], triangles[i].p[2][1] );
      glVertex2f( triangles[i].p[0][0], triangles[i].p[0][1] );
   }

   glEnd();
}

void GLCALLBACK vertex_callback( void *data )
{
   GLsizei	no;
   GLfloat	*p;

   p = (GLfloat *) data;
   no = triangles[triangle_cnt].no;

   triangles[triangle_cnt].p[no][0] = p[0];
   triangles[triangle_cnt].p[no][1] = p[1];

   triangles[triangle_cnt].color[no][0] = edge_color[0];
   triangles[triangle_cnt].color[no][1] = edge_color[1];
   triangles[triangle_cnt].color[no][2] = edge_color[2];

   /* After every three vertices, initialize the next triangle. */
   if ( ++(triangles[triangle_cnt].no) == 3 )
   {
      triangle_cnt++;
      triangles[triangle_cnt].no = 0;
   }
}

void GLCALLBACK combine_callback( GLdouble coords[3],
				  GLdouble *vertex_data[4],
				  GLfloat weight[4], void **data )
{
   GLfloat	*vertex;
   int		i;

   vertex = (GLfloat *) malloc( 2 * sizeof(GLfloat) );

   vertex[0] = (GLfloat) coords[0];
   vertex[1] = (GLfloat) coords[1];

   *data = vertex;
}


void set_screen_wh( GLsizei w, GLsizei h )
{
   width = w;
   height = h;
}

void tesse( void )
{
   GLUtesselator	*tobj;
   GLdouble		data[3];
   GLuint		i, j, point_cnt;

   list_start = glGenLists( 2 );

   tobj = gluNewTess();

   if ( tobj != NULL )
   {
      gluTessNormal( tobj, 0.0, 0.0, 1.0 );
      gluTessCallback( tobj, GLU_TESS_BEGIN, glBegin );
      gluTessCallback( tobj, GLU_TESS_VERTEX, glVertex2fv );
      gluTessCallback( tobj, GLU_TESS_END, glEnd );
      gluTessCallback( tobj, GLU_TESS_ERROR, error_callback );
      gluTessCallback( tobj, GLU_TESS_COMBINE, combine_callback );

      glNewList( list_start, GL_COMPILE );
      gluBeginPolygon( tobj );

      for ( j = 0 ; j <= contour_cnt ; j++ )
      {
	 point_cnt = contours[j].point_cnt;
	 gluNextContour( tobj, GLU_UNKNOWN );

	 for ( i = 0 ; i < point_cnt ; i++ )
	 {
	    data[0] = (GLdouble)( contours[j].p[i][0] );
	    data[1] = (GLdouble)( contours[j].p[i][1] );
	    data[2] = 0.0;
	    gluTessVertex( tobj, data, contours[j].p[i] );
	 }
      }

      gluEndPolygon( tobj );
      glEndList();

      gluTessCallback( tobj, GLU_TESS_BEGIN, begin_callback );
      gluTessCallback( tobj, GLU_TESS_VERTEX, vertex_callback );
      gluTessCallback( tobj, GLU_TESS_END, end_callback );
      gluTessCallback( tobj, GLU_TESS_EDGE_FLAG, edge_callback );

      glNewList( list_start + 1, GL_COMPILE );
      gluBeginPolygon( tobj );

      for ( j = 0 ; j <= contour_cnt ; j++ )
      {
	 point_cnt = contours[j].point_cnt;
	 gluNextContour( tobj, GLU_UNKNOWN );

	 for ( i = 0 ; i < point_cnt ; i++ )
	 {
	    data[0] = (GLdouble)( contours[j].p[i][0] );
	    data[1] = (GLdouble)( contours[j].p[i][1] );
	    data[2] = 0.0;
	    gluTessVertex( tobj, data, contours[j].p[i] );
	 }
      }

      gluEndPolygon( tobj );
      glEndList();

      gluDeleteTess( tobj );

      glutMouseFunc( NULL );
      mode = TESSELATED;
   }
}

void left_down( int x1, int y1 )
{
   GLfloat	P[2];
   GLuint	point_cnt;

   /* translate GLUT into GL coordinates */

   P[0] = x1;
   P[1] = height - y1;

   point_cnt = contours[contour_cnt].point_cnt;

   contours[contour_cnt].p[point_cnt][0] = P[0];
   contours[contour_cnt].p[point_cnt][1] = P[1];

   glBegin( GL_LINES );

   if ( point_cnt )
   {
      glVertex2fv( contours[contour_cnt].p[point_cnt-1] );
      glVertex2fv( P );
   }
   else
   {
      glVertex2fv( P );
      glVertex2fv( P );
   }

   glEnd();
   glFinish();

   contours[contour_cnt].point_cnt++;
}

void middle_down( int x1, int y1 )
{
   GLuint	point_cnt;
   (void) x1;
   (void) y1;

   point_cnt = contours[contour_cnt].point_cnt;

   if ( point_cnt > 2 )
   {
      glBegin( GL_LINES );

      glVertex2fv( contours[contour_cnt].p[0] );
      glVertex2fv( contours[contour_cnt].p[point_cnt-1] );

      contours[contour_cnt].p[point_cnt][0] = -1;

      glEnd();
      glFinish();

      contour_cnt++;
      contours[contour_cnt].point_cnt = 0;
   }
}

void mouse_clicked( int button, int state, int x, int y )
{
   x -= x%10;
   y -= y%10;

   switch ( button )
   {
   case GLUT_LEFT_BUTTON:
      if ( state == GLUT_DOWN ) {
	 left_down( x, y );
      }
      break;
   case GLUT_MIDDLE_BUTTON:
      if ( state == GLUT_DOWN ) {
	 middle_down( x, y );
      }
      break;
   }
}

void display( void )
{
   GLuint i,j;
   GLuint point_cnt;

   glClear( GL_COLOR_BUFFER_BIT );

   switch ( mode )
   {
   case DEFINE:
      /* draw grid */
      glColor3f( 0.6, 0.5, 0.5 );

      glBegin( GL_LINES );

      for ( i = 0 ; i < width ; i += 10 )
      {
	 for ( j = 0 ; j < height ; j += 10 )
	 {
	    glVertex2i( 0, j );
	    glVertex2i( width, j );
	    glVertex2i( i, height );
	    glVertex2i( i, 0 );
	 }
      }

      glColor3f( 1.0, 1.0, 0.0 );

      for ( i = 0 ; i <= contour_cnt ; i++ )
      {
	 point_cnt = contours[i].point_cnt;

	 glBegin( GL_LINES );

	 switch ( point_cnt )
	 {
	 case 0:
	    break;
	 case 1:
	    glVertex2fv( contours[i].p[0] );
	    glVertex2fv( contours[i].p[0] );
	    break;
	 case 2:
	    glVertex2fv( contours[i].p[0] );
	    glVertex2fv( contours[i].p[1] );
	    break;
	 default:
	    --point_cnt;
	    for ( j = 0 ; j < point_cnt ; j++ )
	    {
	       glVertex2fv( contours[i].p[j] );
	       glVertex2fv( contours[i].p[j+1] );
	    }
	    if ( contours[i].p[j+1][0] == -1 )
	    {
	       glVertex2fv( contours[i].p[0] );
	       glVertex2fv( contours[i].p[j] );
	    }
	    break;
	 }

	 glEnd();
      }

      glFinish();
      break;

   case TESSELATED:
      /* draw triangles */
      glColor3f( 0.7, 0.7, 0.0 );
      glCallList( list_start );

      glLineWidth( 2.0 );
      glCallList( list_start + 1 );
      glLineWidth( 1.0 );

      glFlush();
      break;
   }

   glColor3f( 1.0, 1.0, 0.0 );
}

void clear( void )
{
   contour_cnt = 0;
   contours[0].point_cnt = 0;
   triangle_cnt = 0;

   glutMouseFunc( mouse_clicked );

   mode = DEFINE;

   glDeleteLists( list_start, 2 );
   list_start = 0;
}

void quit( void )
{
   exit( 0 );
}

void menu_selected( int entry )
{
   switch ( entry )
   {
   case CLEAR:
      clear();
      break;
   case TESSELATE:
      tesse();
      break;
   case QUIT:
      quit();
      break;
   }

   glutPostRedisplay();
}

void key_pressed( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;

   switch ( key )
   {
   case 'c':
   case 'C':
      clear();
      break;
   case 't':
   case 'T':
      tesse();
      break;
   case 'q':
   case 'Q':
      quit();
      break;
   }

   glutPostRedisplay();
}

void myinit( void )
{
   /* clear background to gray */
   glClearColor( 0.4, 0.4, 0.4, 0.0 );
   glShadeModel( GL_FLAT );
   glPolygonMode( GL_FRONT, GL_FILL );

   menu = glutCreateMenu( menu_selected );

   glutAddMenuEntry( "clear", CLEAR );
   glutAddMenuEntry( "tesselate", TESSELATE );
   glutAddMenuEntry( "quit", QUIT );

   glutAttachMenu( GLUT_RIGHT_BUTTON );

   glutMouseFunc( mouse_clicked );
   glutKeyboardFunc( key_pressed );

   contour_cnt = 0;
   mode = DEFINE;
}

static void reshape( GLsizei w, GLsizei h )
{
   glViewport( 0, 0, w, h );

   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, (GLdouble)w, 0.0, (GLdouble)h, -1.0, 1.0 );

   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();

   set_screen_wh( w, h );
}


static void usage( void )
{
   printf( "Use left mouse button to place vertices.\n" );
   printf( "Press middle mouse button when done.\n" );
   printf( "Select tesselate from the pop-up menu.\n" );
}


/*
 * Main Loop
 * Open window with initial window size, title bar,
 * RGBA display mode, and handle input events.
 */
int main( int argc, char **argv )
{
    usage();

    glutInit( &argc, argv );
    glutInitDisplayMode( GLUT_SINGLE | GLUT_RGB );
    glutInitWindowSize( 400, 400 );
    glutCreateWindow( argv[0] );

    myinit();

    glutDisplayFunc( display );
    glutReshapeFunc( reshape );

    glutMainLoop();

    return 0;
}
