/* $Id: glutfx.c,v 1.2 2000/06/27 17:04:43 brianp Exp $ */

/*
 * Example of how one might use GLUT with the 3Dfx driver in full-screen mode.
 * Note: this only works with X since we're using Mesa's GLX "hack" for
 * using Glide.
 *
 * Goals:
 *   easy setup and input event handling with GLUT
 *   use 3Dfx hardware
 *   automatically set MESA environment variables
 *   don't lose mouse input focus
 *
 * Brian Paul   This file is in the public domain.
 */

/*
 * $Log: glutfx.c,v $
 * Revision 1.2  2000/06/27 17:04:43  brianp
 * fixed compiler warnings
 *
 * Revision 1.1.1.1  1999/08/19 00:55:40  jtg
 * Imported sources
 *
 * Revision 3.2  1999/03/28 18:18:33  brianp
 * minor clean-up
 *
 * Revision 3.1  1998/06/29 02:37:30  brianp
 * minor changes for Windows (Ted Jump)
 *
 * Revision 3.0  1998/02/14 18:42:29  brianp
 * initial rev
 *
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


#define WIDTH 640
#define HEIGHT 480


static int Window = 0;
static int ScreenWidth, ScreenHeight;
static GLuint Torus = 0;
static GLfloat Xrot = 0.0, Yrot = 0.0;



static void Display( void )
{
   static GLfloat blue[4] = {0.2, 0.2, 1.0, 1.0};
   static GLfloat red[4] = {1.0, 0.2, 0.2, 1.0};
   static GLfloat green[4] = {0.2, 1.0, 0.2, 1.0};

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Xrot, 1, 0, 0);
   glRotatef(Yrot, 0, 1, 0);

   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, blue);
   glCallList(Torus);

   glRotatef(90.0, 1, 0, 0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, red);
   glCallList(Torus);

   glRotatef(90.0, 0, 1, 0);
   glMaterialfv(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE, green);
   glCallList(Torus);

   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   float ratio = (float) width / (float) height;

   ScreenWidth = width;
   ScreenHeight = height;

   /*
    * The 3Dfx driver is limited to 640 x 480 but the X window may be larger.
    * Enforce that here.
    */
   if (width > WIDTH)
      width = WIDTH;
   if (height > HEIGHT)
      height = HEIGHT;

   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -ratio, ratio, -1.0, 1.0, 5.0, 30.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -20.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         glutDestroyWindow(Window);
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         break;
      case GLUT_KEY_DOWN:
         break;
      case GLUT_KEY_LEFT:
         break;
      case GLUT_KEY_RIGHT:
         break;
   }
   glutPostRedisplay();
}


static void MouseMove( int x, int y )
{
   Xrot = y - ScreenWidth / 2;
   Yrot = x - ScreenHeight / 2;
   glutPostRedisplay();
}


static void Init( void )
{
   Torus = glGenLists(1);
   glNewList(Torus, GL_COMPILE);
   glutSolidTorus(0.5, 2.0, 10, 20);
   glEndList();

   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);

   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);
}


int main( int argc, char *argv[] )
{
#ifndef _WIN32
   printf("NOTE: if you've got 3Dfx VooDoo hardware you must run this");
   printf(" program as root.\n\n");
   printf("Move the mouse.  Press ESC to exit.\n\n");
#endif

   /* Tell Mesa GLX to use 3Dfx driver in fullscreen mode. */
   putenv("MESA_GLX_FX=fullscreen");

   /* Disable 3Dfx Glide splash screen */
   putenv("FX_GLIDE_NO_SPLASH=");

   /* Give an initial size and position so user doesn't have to place window */
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(WIDTH, HEIGHT);
   glutInit( &argc, argv );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

   Window = glutCreateWindow(argv[0]);
   if (!Window) {
      printf("Error, couldn't open window\n");
      exit(1);
   }

   /*
    * Want the X window to fill the screen so that we don't have to
    * worry about losing the mouse input focus.
    * Note that we won't actually see the X window since we never draw
    * to it, hence, the original X screen's contents aren't disturbed.
    */
   glutFullScreen();

   Init();

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutPassiveMotionFunc( MouseMove );

   glutMainLoop();
   return 0;
}
