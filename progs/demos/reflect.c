/* $Id: reflect.c,v 1.1 1999/08/19 00:55:40 jtg Exp $ */

/*
 * Demo of a reflective, texture-mapped surface with OpenGL.
 * Brian Paul   August 14, 1995   This file is in the public domain.
 *
 * Hardware texture mapping is highly recommended!
 *
 * The basic steps are:
 *    1. Render the reflective object (a polygon) from the normal viewpoint,
 *       setting the stencil planes = 1.
 *    2. Render the scene from a special viewpoint:  the viewpoint which
 *       is on the opposite side of the reflective plane.  Only draw where
 *       stencil = 1.  This draws the objects in the reflective surface.
 *    3. Render the scene from the original viewpoint.  This draws the
 *       objects in the normal fashion.  Use blending when drawing
 *       the reflective, textured surface.
 *
 * This is a very crude demo.  It could be much better.
 */
 
/*
 * Dirk Reiners (reiners@igd.fhg.de) made some modifications to this code.
 *
 * August 1996 - A few optimizations by Brian
 */

/*
 * April, 1997 - Added Mark Kilgard's changes.
 */

/*
 * $Log: reflect.c,v $
 * Revision 1.1  1999/08/19 00:55:40  jtg
 * Initial revision
 *
 * Revision 3.4  1999/03/28 18:22:05  brianp
 * minor clean-up
 *
 * Revision 3.3  1998/11/22 02:54:29  brianp
 * only draw one stack for gluCylinders
 *
 * Revision 3.2  1998/11/19 02:53:48  brianp
 * changed texture image and background color
 *
 * Revision 3.1  1998/11/05 04:34:04  brianp
 * moved image files to ../images/ directory
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */


#define USE_ZBUFFER


/* OK, without hardware support this is overkill. */
#define USE_TEXTURE

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "GL/glut.h"

#include "../util/readtex.c"  /* a hack, I know */


#define DEG2RAD (3.14159/180.0)


#define TABLE_TEXTURE "../images/tile.rgb"

static int ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

#define MAX_OBJECTS 2

static GLint table_list;
static GLint objects_list[MAX_OBJECTS];


static GLfloat xrot, yrot;
static GLfloat spin;



static void make_table( void )
{
   static GLfloat table_mat[] = { 1.0, 1.0, 1.0, 0.6 };
   static GLfloat gray[] = { 0.4, 0.4, 0.4, 1.0 };

   table_list = glGenLists(1);
   glNewList( table_list, GL_COMPILE );

   /* load table's texture */
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, table_mat );
/*   glMaterialfv( GL_FRONT, GL_EMISSION, gray );*/
   glMaterialfv( GL_FRONT, GL_DIFFUSE, table_mat );
   glMaterialfv( GL_FRONT, GL_AMBIENT, gray );
   
   /* draw textured square for the table */
   glPushMatrix();
   glScalef( 4.0, 4.0, 4.0 );
   glBegin( GL_POLYGON );
   glNormal3f( 0.0, 1.0, 0.0 );
   glTexCoord2f( 0.0, 0.0 );   glVertex3f( -1.0, 0.0,  1.0 );
   glTexCoord2f( 1.0, 0.0 );   glVertex3f(  1.0, 0.0,  1.0 );
   glTexCoord2f( 1.0, 1.0 );   glVertex3f(  1.0, 0.0, -1.0 );
   glTexCoord2f( 0.0, 1.0 );   glVertex3f( -1.0, 0.0, -1.0 );
   glEnd();
   glPopMatrix();

   glDisable( GL_TEXTURE_2D );

   glEndList();
}


static void make_objects( void )
{
   GLUquadricObj *q;

   static GLfloat cyan[] = { 0.0, 1.0, 1.0, 1.0 };
   static GLfloat green[] = { 0.2, 1.0, 0.2, 1.0 };
   static GLfloat black[] = { 0.0, 0.0, 0.0, 0.0 };

   q = gluNewQuadric();
   gluQuadricDrawStyle( q, GLU_FILL );
   gluQuadricNormals( q, GLU_SMOOTH );

   objects_list[0] = glGenLists(1);
   glNewList( objects_list[0], GL_COMPILE );
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, cyan );
   glMaterialfv( GL_FRONT, GL_EMISSION, black );
   gluCylinder( q, 0.5, 0.5,  1.0, 15, 1 );
   glEndList();

   objects_list[1] = glGenLists(1);
   glNewList( objects_list[1], GL_COMPILE );
   glMaterialfv( GL_FRONT, GL_AMBIENT_AND_DIFFUSE, green );
   glMaterialfv( GL_FRONT, GL_EMISSION, black );
   gluCylinder( q, 1.5, 0.0,  2.5, 15, 1 );
   glEndList();
}


static GLfloat light_pos[] = { 0.0, 20.0, 0.0, 1.0 };

static void init( void )
{
   make_table();
   make_objects();

   /* Setup texture */
#ifdef USE_TEXTURE

   Image = LoadRGBImage( TABLE_TEXTURE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", TABLE_TEXTURE);
      exit(0);
   }

   gluBuild2DMipmaps(GL_TEXTURE_2D, 3, ImgWidth, ImgHeight,
                     ImgFormat, GL_UNSIGNED_BYTE, Image);

   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
#endif


   xrot = 30.0;
   yrot = 50.0;
   spin = 0.0;

#ifndef USE_ZBUFFER
   glEnable( GL_CULL_FACE );
#endif

   glShadeModel( GL_FLAT );
   
   glEnable( GL_LIGHT0 );
   glEnable( GL_LIGHTING );

   glClearColor( 0.5, 0.5, 0.9, 1.0 );

   glEnable( GL_NORMALIZE );
}



static void reshape(int w, int h)
{
   GLfloat aspect = (float) w / (float) h;

   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -aspect, aspect, -1.0, 1.0, 4.0, 300.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}



static void draw_objects( GLfloat eyex, GLfloat eyey, GLfloat eyez )
{
   (void) eyex;
   (void) eyey;
   (void) eyez;
#ifndef USE_ZBUFFER
	if (eyex<0.5)
	{
#endif
	   glPushMatrix();
	   glTranslatef( 1.0, 1.5, 0.0 );
	   glRotatef( spin, 1.0, 0.5, 0.0 );
	   glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
	   glCallList( objects_list[0] );
	   glPopMatrix();
	
	   glPushMatrix();
	   glTranslatef( -1.0, 0.85+3.0*fabs( cos(0.01*spin) ), 0.0 );
	   glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
	   glRotatef( spin, 1.0, 0.5, 0.0 );
	   glScalef( 0.5, 0.5, 0.5 );
	   glCallList( objects_list[1] );
	   glPopMatrix();
#ifndef USE_ZBUFFER
	}
	else
	{	
	   glPushMatrix();
	   glTranslatef( -1.0, 0.85+3.0*fabs( cos(0.01*spin) ), 0.0 );
	   glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
	   glRotatef( spin, 1.0, 0.5, 0.0 );
	   glScalef( 0.5, 0.5, 0.5 );
	   glCallList( objects_list[1] );
	   glPopMatrix();

	   glPushMatrix();
	   glTranslatef( 1.0, 1.5, 0.0 );
	   glRotatef( spin, 1.0, 0.5, 0.0 );
	   glRotatef( 0.5*spin, 0.0, 0.5, 1.0 );
	   glCallList( objects_list[0] );
	   glPopMatrix();
	}
#endif
}



static void draw_table( void )
{
   glCallList( table_list );
}



static void draw_scene( void )
{
   GLfloat dist = 20.0;
   GLfloat eyex, eyey, eyez;

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


   eyex = dist * cos(yrot*DEG2RAD) * cos(xrot*DEG2RAD);
   eyez = dist * sin(yrot*DEG2RAD) * cos(xrot*DEG2RAD);
   eyey = dist * sin(xrot*DEG2RAD);

   /* view from top */
   glPushMatrix();
   gluLookAt( eyex, eyey, eyez, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0 );

   glLightfv( GL_LIGHT0, GL_POSITION, light_pos );
 
   /* draw table into stencil planes */
   glEnable( GL_STENCIL_TEST );
#ifdef USE_ZBUFFER
   glDisable( GL_DEPTH_TEST );
#endif
   glStencilFunc( GL_ALWAYS, 1, 0xffffffff );
   glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
   glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
   draw_table();
   glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

#ifdef USE_ZBUFFER
   glEnable( GL_DEPTH_TEST );
#endif


   /* render view from below (reflected viewport) */
   /* only draw where stencil==1 */
   if (eyey>0.0) {
      glPushMatrix();
 
      glStencilFunc( GL_EQUAL, 1, 0xffffffff );  /* draw if ==1 */
      glStencilOp( GL_KEEP, GL_KEEP, GL_KEEP );
      glScalef( 1.0, -1.0, 1.0 );

      /* Reposition light in reflected space. */
      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);

      draw_objects(eyex, eyey, eyez);
      glPopMatrix();

      /* Restore light's original unreflected position. */
      glLightfv(GL_LIGHT0, GL_POSITION, light_pos);
   }

   glDisable( GL_STENCIL_TEST );

   glEnable( GL_BLEND );
   glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

#ifdef USE_TEXTURE
   glEnable( GL_TEXTURE_2D );
#endif
   draw_table();
   glDisable( GL_TEXTURE_2D );
   glDisable( GL_BLEND );

   /* view from top */
   glPushMatrix();

   draw_objects(eyex, eyey, eyez);

   glPopMatrix();

   glPopMatrix();

   glutSwapBuffers();
}



#if 0
void draw_scene(void)
{
   GLfloat dist = 20.0;
   GLfloat eyex, eyey, eyez;

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);


   eyex = dist * cos(yrot*DEG2RAD) * cos(xrot*DEG2RAD);
   eyez = dist * sin(yrot*DEG2RAD) * cos(xrot*DEG2RAD);
   eyey = dist * sin(xrot*DEG2RAD);

   /* view from top */
   glPushMatrix();
   gluLookAt( eyex, eyey, eyez, 0.0, 0.0, 0.0,  0.0, 1.0, 0.0 );

   draw_table();

   glPopMatrix();

   glutSwapBuffers();
}
#endif


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   if (key==27)
      exit(0);
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         xrot += 3.0;
#ifndef USE_ZBUFFER
		 if ( xrot > 180 )	xrot = 180;
#endif
         break;
      case GLUT_KEY_DOWN:
         xrot -= 3.0;
#ifndef USE_ZBUFFER
		 if ( xrot < 0 )	xrot = 0;
#endif
         break;
      case GLUT_KEY_LEFT:
         yrot += 3.0;
         break;
      case GLUT_KEY_RIGHT:
         yrot -= 3.0;
         break;
   }
   glutPostRedisplay();
}



static void idle( void )
{
   spin += 2.0;
   yrot += 3.0;
   glutPostRedisplay();
}



int main( int argc, char *argv[] )
{
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB 
#ifdef USE_ZBUFFER
		| GLUT_DEPTH 
#endif
		| GLUT_STENCIL);
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize(400, 300 );
   glutCreateWindow(argv[0]);
   glutReshapeFunc(reshape);
   glutDisplayFunc(draw_scene);
   glutKeyboardFunc(Key);
   glutSpecialFunc(SpecialKey);
   glutIdleFunc(idle);

   init();

   glutMainLoop();
   return 0;
}
