/* $Id: renormal.c,v 1.3 1999/09/17 12:27:01 brianp Exp $ */

/*
 * Test GL_EXT_rescale_normal extension
 * Brian Paul  January 1998   This program is in the public domain.
 */

/*
 * $Id: renormal.c,v 1.3 1999/09/17 12:27:01 brianp Exp $
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glut.h>


static GLfloat Phi = 0.0;


static void Idle(void)
{
   Phi += 0.1;
   glutPostRedisplay();
}


static void Display( void )
{
   GLfloat scale = 0.6 + 0.5 * sin(Phi);
   glClear( GL_COLOR_BUFFER_BIT );
   glPushMatrix();
   glScalef(scale, scale, scale);
   glutSolidSphere(2.0, 20, 20);
   glPopMatrix();
   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
}



static void Init( void )
{
   static GLfloat mat[4] = { 0.8, 0.8, 0.0, 1.0 };
   static GLfloat pos[4] = { -1.0, 1.0, 1.0, 0.0 };

   /* setup lighting, etc */
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, mat);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);

   glEnable(GL_CULL_FACE);

   glDisable(GL_RESCALE_NORMAL_EXT);
   glDisable(GL_NORMALIZE);
}


#define UNSCALED  1
#define NORMALIZE 2
#define RESCALE   3
#define QUIT      4


static void ModeMenu(int entry)
{
   if (entry==UNSCALED) {
      glDisable(GL_RESCALE_NORMAL_EXT);
      glDisable(GL_NORMALIZE);
   }
   else if (entry==NORMALIZE) {
      glEnable(GL_NORMALIZE);
      glDisable(GL_RESCALE_NORMAL_EXT);
   }
   else if (entry==RESCALE) {
      glDisable(GL_NORMALIZE);
      glEnable(GL_RESCALE_NORMAL_EXT);
   }
   else if (entry==QUIT) {
      exit(0);
   }
   glutPostRedisplay();
}

static void
key(unsigned char k, int x, int y)
{
  (void) x;
  (void) y;
  switch (k) {
  case 27:  /* Escape */
    exit(0);
    break;
  default:
    return;
  }
  glutPostRedisplay();
}

int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 400, 400 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   glutCreateWindow(argv[0]);

   Init();

   glutIdleFunc( Idle );
   glutReshapeFunc( Reshape );
   glutDisplayFunc( Display );
   glutKeyboardFunc(key);

   glutCreateMenu(ModeMenu);
   glutAddMenuEntry("Unscaled", UNSCALED);
   glutAddMenuEntry("Normalize", NORMALIZE);
   glutAddMenuEntry("Rescale EXT", RESCALE);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
