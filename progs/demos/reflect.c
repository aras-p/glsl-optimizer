
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
 * Authors:
 *   Brian Paul
 *   Dirk Reiners (reiners@igd.fhg.de) made some modifications to this code.
 *   Mark Kilgard (April 1997)
 *   Brian Paul (April 2000 - added keyboard d/s options)
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include "GL/glut.h"
#include "showbuffer.h"
#include "readtex.h"


#define DEG2RAD (3.14159/180.0)

#define TABLE_TEXTURE "../images/tile.rgb"

static GLint ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

#define MAX_OBJECTS 2
static GLint table_list;
static GLint objects_list[MAX_OBJECTS];

static GLfloat xrot, yrot;
static GLfloat spin;

static GLint Width = 400, Height = 300;
static GLenum ShowBuffer = GL_NONE;
static GLboolean Anim = GL_TRUE;

/* performance info */
static GLint T0 = 0;
static GLint Frames = 0;


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


static void init( void )
{
   make_table();
   make_objects();

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

   xrot = 30.0;
   yrot = 50.0;
   spin = 0.0;

   glShadeModel( GL_FLAT );
   
   glEnable( GL_LIGHT0 );
   glEnable( GL_LIGHTING );

   glClearColor( 0.5, 0.5, 0.9, 0.0 );

   glEnable( GL_NORMALIZE );
}



static void reshape(int w, int h)
{
   GLfloat yAspect = 2.5;
   GLfloat xAspect = yAspect * (float) w / (float) h;
   Width = w;
   Height = h;
   glViewport(0, 0, w, h);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   glFrustum( -xAspect, xAspect, -yAspect, yAspect, 10.0, 30.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
}



static void draw_objects( GLfloat eyex, GLfloat eyey, GLfloat eyez )
{
   (void) eyex;
   (void) eyey;
   (void) eyez;
#ifndef USE_ZBUFFER
   if (eyex<0.5) {
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
   else {	
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
   static GLfloat light_pos[] = { 0.0, 20.0, 0.0, 1.0 };
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
   glDisable( GL_DEPTH_TEST );
   glEnable( GL_STENCIL_TEST );
   glStencilFunc( GL_ALWAYS, 1, 0xffffffff );
   glStencilOp( GL_REPLACE, GL_REPLACE, GL_REPLACE );
   glColorMask( GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE );
   draw_table();
   glColorMask( GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE );

   glEnable( GL_DEPTH_TEST );

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

   glEnable( GL_TEXTURE_2D );
   draw_table();
   glDisable( GL_TEXTURE_2D );
   glDisable( GL_BLEND );

   /* view from top */
   glPushMatrix();

   draw_objects(eyex, eyey, eyez);

   glPopMatrix();

   glPopMatrix();

   if (ShowBuffer == GL_DEPTH) {
      ShowDepthBuffer(Width, Height, 1.0, 0.0);
   }
   else if (ShowBuffer == GL_STENCIL) {
      ShowStencilBuffer(Width, Height, 255.0, 0.0);
   }
   else if (ShowBuffer == GL_ALPHA) {
      ShowAlphaBuffer(Width, Height);
   }

   glutSwapBuffers();

   {
      GLint t = glutGet(GLUT_ELAPSED_TIME);
      Frames++;
      if (t - T0 >= 5000) {
         GLfloat seconds = (t - T0) / 1000.0;
         GLfloat fps = Frames / seconds;
         printf("%d frames in %g seconds = %g FPS\n", Frames, seconds, fps);
         T0 = t;
         Frames = 0;
      }
   }
}


static void idle( void )
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;
   spin += 60.0 * dt;
   yrot += 90.0 * dt;
   glutPostRedisplay();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   if (key == 'd') {
      ShowBuffer = GL_DEPTH;
   }
   else if (key == 's') {
      ShowBuffer = GL_STENCIL;
   }
   else if (key == 'a') {
      ShowBuffer = GL_ALPHA;
   }
   else if (key == ' ') {
      Anim = !Anim;
      if (Anim)
         glutIdleFunc(idle);
      else
         glutIdleFunc(NULL);
   }
   else if (key==27) {
      exit(0);
   }
   else {
      ShowBuffer = GL_NONE;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         xrot += 3.0;
         if ( xrot > 85 )
            xrot = 85;
         break;
      case GLUT_KEY_DOWN:
         xrot -= 3.0;
         if ( xrot < 5 )
            xrot = 5;
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


int main( int argc, char *argv[] )
{
   glutInit(&argc, argv);
   glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGB | GLUT_DEPTH | GLUT_STENCIL | GLUT_ALPHA);
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
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
