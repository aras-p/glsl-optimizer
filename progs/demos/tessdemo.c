
/*
 * A demo of the GLU polygon tesselation functions written by Bogdan Sikorski.
 * Updated for GLU 1.3 tessellation by Gareth Hughes <gareth@valinux.com>
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

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

#ifdef GLU_VERSION_1_2

typedef enum{ QUIT, TESSELATE, CLEAR } menu_entries;
typedef enum{ DEFINE, TESSELATED } mode_type;

static GLsizei		width, height;
static GLuint		contour_cnt;
static GLuint		triangle_cnt;

static mode_type 	mode;
static int		menu;

static GLuint		list_start;

static GLfloat		edge_color[3];

static struct {
   GLfloat	p[MAX_POINTS][2];
   GLuint	point_cnt;
} contours[MAX_CONTOURS];

static struct {
   GLsizei	no;
   GLfloat	p[3][2];
   GLclampf	color[3][3];
} triangles[MAX_TRIANGLES];



static void GLCALLBACK error_callback( GLenum err )
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

static void GLCALLBACK begin_callback( GLenum mode )
{
   /* Allow multiple triangles to be output inside the begin/end pair. */
   triangle_cnt = 0;
   triangles[triangle_cnt].no = 0;
}

static void GLCALLBACK edge_callback( GLenum flag )
{
   /* Persist the edge flag across triangles. */
   if ( flag == GL_TRUE ) {
      edge_color[0] = 1.0;
      edge_color[1] = 1.0;
      edge_color[2] = 0.5;
   } else {
      edge_color[0] = 1.0;
      edge_color[1] = 0.0;
      edge_color[2] = 0.0;
   }
}

static void GLCALLBACK end_callback()
{
   GLuint	i;

   glBegin( GL_LINES );

   /* Output the three edges of each triangle as lines colored
      according to their edge flag. */
   for ( i = 0 ; i < triangle_cnt ; i++ ) {
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

static void GLCALLBACK vertex_callback( void *data )
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
   if ( ++(triangles[triangle_cnt].no) == 3 ) {
      triangle_cnt++;
      triangles[triangle_cnt].no = 0;
   }
}

static void GLCALLBACK combine_callback( GLdouble coords[3],
					 GLdouble *vertex_data[4],
					 GLfloat weight[4], void **data )
{
   GLfloat	*vertex;

   vertex = (GLfloat *) malloc( 2 * sizeof(GLfloat) );

   vertex[0] = (GLfloat) coords[0];
   vertex[1] = (GLfloat) coords[1];

   *data = vertex;
}


static void set_screen_wh( GLsizei w, GLsizei h )
{
   width = w;
   height = h;
}

typedef void (GLAPIENTRY *callback_t)();

static void tesse( void )
{
   GLUtesselator	*tobj;
   GLdouble		data[3];
   GLuint		i, j, point_cnt;

   list_start = glGenLists( 2 );

   tobj = gluNewTess();

   if ( tobj != NULL ) {
      gluTessNormal( tobj, 0.0, 0.0, 1.0 );
      gluTessCallback( tobj, GLU_TESS_BEGIN, (callback_t) glBegin );
      gluTessCallback( tobj, GLU_TESS_VERTEX, (callback_t) glVertex2fv );
      gluTessCallback( tobj, GLU_TESS_END, (callback_t) glEnd );
      gluTessCallback( tobj, GLU_TESS_ERROR, (callback_t) error_callback );
      gluTessCallback( tobj, GLU_TESS_COMBINE, (callback_t) combine_callback );

      glNewList( list_start, GL_COMPILE );
      gluBeginPolygon( tobj );

      for ( j = 0 ; j <= contour_cnt ; j++ ) {
	 point_cnt = contours[j].point_cnt;
	 gluNextContour( tobj, GLU_UNKNOWN );

	 for ( i = 0 ; i < point_cnt ; i++ ) {
	    data[0] = (GLdouble)( contours[j].p[i][0] );
	    data[1] = (GLdouble)( contours[j].p[i][1] );
	    data[2] = 0.0;
	    gluTessVertex( tobj, data, contours[j].p[i] );
	 }
      }

      gluEndPolygon( tobj );
      glEndList();

      gluTessCallback( tobj, GLU_TESS_BEGIN, (callback_t) begin_callback );
      gluTessCallback( tobj, GLU_TESS_VERTEX, (callback_t) vertex_callback );
      gluTessCallback( tobj, GLU_TESS_END, (callback_t) end_callback );
      gluTessCallback( tobj, GLU_TESS_EDGE_FLAG, (callback_t) edge_callback );

      glNewList( list_start + 1, GL_COMPILE );
      gluBeginPolygon( tobj );

      for ( j = 0 ; j <= contour_cnt ; j++ ) {
	 point_cnt = contours[j].point_cnt;
	 gluNextContour( tobj, GLU_UNKNOWN );

	 for ( i = 0 ; i < point_cnt ; i++ ) {
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

static void left_down( int x1, int y1 )
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

   if ( point_cnt ) {
      glVertex2fv( contours[contour_cnt].p[point_cnt-1] );
      glVertex2fv( P );
   } else {
      glVertex2fv( P );
      glVertex2fv( P );
   }

   glEnd();
   glFinish();

   contours[contour_cnt].point_cnt++;
}

static void middle_down( int x1, int y1 )
{
   GLuint	point_cnt;
   (void) x1;
   (void) y1;

   point_cnt = contours[contour_cnt].point_cnt;

   if ( point_cnt > 2 ) {
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

static void mouse_clicked( int button, int state, int x, int y )
{
   x -= x%10;
   y -= y%10;

   switch ( button ) {
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

static void display( void )
{
   GLuint i,j;
   GLsizei ii, jj;
   GLuint point_cnt;

   glClear( GL_COLOR_BUFFER_BIT );

   switch ( mode ) {
   case DEFINE:
      /* draw grid */
      glColor3f( 0.6, 0.5, 0.5 );

      glBegin( GL_LINES );

      for ( ii = 0 ; ii < width ; ii += 10 ) {
	 for ( jj = 0 ; jj < height ; jj += 10 ) {
	    glVertex2i( 0, jj );
	    glVertex2i( width, jj );
	    glVertex2i( ii, height );
	    glVertex2i( ii, 0 );
	 }
      }

      glEnd();

      glColor3f( 1.0, 1.0, 0.0 );

      for ( i = 0 ; i <= contour_cnt ; i++ ) {
	 point_cnt = contours[i].point_cnt;

	 glBegin( GL_LINES );

	 switch ( point_cnt ) {
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
	    for ( j = 0 ; j < point_cnt ; j++ ) {
	       glVertex2fv( contours[i].p[j] );
	       glVertex2fv( contours[i].p[j+1] );
	    }
	    if ( contours[i].p[j+1][0] == -1 ) {
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

static void clear( void )
{
   contour_cnt = 0;
   contours[0].point_cnt = 0;
   triangle_cnt = 0;

   glutMouseFunc( mouse_clicked );

   mode = DEFINE;

   glDeleteLists( list_start, 2 );
   list_start = 0;
}

static void quit( void )
{
   exit( 0 );
}

static void menu_selected( int entry )
{
   switch ( entry ) {
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

static void key_pressed( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;

   switch ( key ) {
   case 'c':
   case 'C':
      clear();
      break;
   case 't':
   case 'T':
      tesse();
      break;
   case 27:
   case 'q':
   case 'Q':
      quit();
      break;
   }

   glutPostRedisplay();
}

static void myinit( void )
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

#endif


static void usage( void )
{
   printf( "Use left mouse button to place vertices.\n" );
   printf( "Press middle mouse button when done.\n" );
   printf( "Select tesselate from the pop-up menu.\n" );
}


int main( int argc, char **argv )
{
   const char *version = (const char *) gluGetString( GLU_VERSION );
   printf( "GLU version string: %s\n", version );
   if ( strstr( version, "1.0" ) || strstr( version, "1.1" ) ) {
      fprintf( stderr, "Sorry, this demo reqiures GLU 1.2 or later.\n" );
      exit( 1 );
   }

   usage();

   glutInit( &argc, argv );
   glutInitDisplayMode( GLUT_SINGLE | GLUT_RGB );
   glutInitWindowPosition(0, 0);
   glutInitWindowSize( 400, 400 );
   glutCreateWindow( argv[0] );

   /* GH: Bit of a hack...
    */
#ifdef GLU_VERSION_1_2
   myinit();

   glutDisplayFunc( display );
   glutReshapeFunc( reshape );

   glutMainLoop();
#endif

   return 0;
}
