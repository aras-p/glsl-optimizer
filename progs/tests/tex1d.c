
/* Exercise 1D textures
 */

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "GL/glew.h"
#include "GL/glut.h"

static GLuint Window = 0;
static GLuint TexObj[2];
static GLfloat Angle = 0.0f;


static void draw( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glColor3f( 1.0, 1.0, 1.0 );

   /* draw first polygon */
   glPushMatrix();
   glTranslatef( -1.0, 0.0, 0.0 );
   glRotatef( Angle, 0.0, 0.0, 1.0 );
   glBindTexture( GL_TEXTURE_1D, TexObj[0] );
   glBegin( GL_POLYGON );
   glTexCoord1f( 0.0 );   glVertex2f( -1.0, -1.0 );
   glTexCoord1f( 1.0 );   glVertex2f(  1.0, -1.0 );
   glTexCoord1f( 1.0 );   glVertex2f(  1.0,  1.0 );
   glTexCoord1f( 0.0 );   glVertex2f( -1.0,  1.0 );
   glEnd();
   glPopMatrix();

   glutSwapBuffers();
}



/*
static void idle( void )
{
   Angle += 2.0;
   glutPostRedisplay();
}
*/



/* change view Angle, exit upon ESC */
static void key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
      case 27:
         exit(0);
   }
}



/* new window size or exposure */
static void reshape( int width, int height )
{
   glViewport(0, 0, (GLint)width, (GLint)height);
   glMatrixMode(GL_PROJECTION);
   glLoadIdentity();
   /*   glOrtho( -3.0, 3.0, -3.0, 3.0, -10.0, 10.0 );*/
   glFrustum( -2.0, 2.0, -2.0, 2.0, 6.0, 20.0 );
   glMatrixMode(GL_MODELVIEW);
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -8.0 );
}


static void init( void )
{
   GLubyte tex[256][3];
   GLint i;


   glDisable( GL_DITHER );

   /* Setup texturing */
   glEnable( GL_TEXTURE_1D );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );


   /* generate texture object IDs */
   glGenTextures( 2, TexObj );

   /* setup first texture object */
   glBindTexture( GL_TEXTURE_1D, TexObj[0] );


   for (i = 0; i < 256; i++) {
      GLfloat f;

      /* map 0..255 to -PI .. PI */
      f = ((i / 255.0) - .5) * (3.141592 * 2);

      f = sin(f);

      /* map -1..1 to 0..255 */
      tex[i][0] = (f+1.0)/2.0 * 255.0;
      tex[i][1] = 0;
      tex[i][2] = 0;
   }

   glTexImage1D( GL_TEXTURE_1D, 0, 3, 256, 0, GL_RGB, GL_UNSIGNED_BYTE, tex );
   glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_1D, GL_TEXTURE_WRAP_T, GL_REPEAT );
}



int main( int argc, char *argv[] )
{
   glutInit(&argc, argv);
   glutInitWindowPosition(0, 0);
   glutInitWindowSize(300, 300);
   glutInitDisplayMode( GLUT_RGB | GLUT_DEPTH | GLUT_DOUBLE );

   Window = glutCreateWindow("Texture Objects");
   glewInit();
   if (!Window) {
      exit(1);
   }

   init();

   glutReshapeFunc( reshape );
   glutKeyboardFunc( key );
/*    glutIdleFunc( idle ); */
   glutDisplayFunc( draw );
   glutMainLoop();
   return 0;
}
