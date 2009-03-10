/*
 * Exercise GL_EXT_fog_coord
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

static int Width = 600;
static int Height = 200;
static GLfloat Near = 5.0, Far = 25.0;


static void Display( void )
{
   GLfloat t;

   glClearColor(0.2, 0.2, 0.8, 0);
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   for (t = 0.0; t <= 1.0; t += 0.25) {
      GLfloat f = Near + t * (Far - Near);
      printf("glFogCoord(%4.1f)\n", f);
      glFogCoordfEXT(f);

      glPushMatrix();
         glTranslatef(t * 10.0 - 5.0, 0, 0);
         glBegin(GL_POLYGON);
         glVertex2f(-1, -1);
         glVertex2f( 1, -1);
         glVertex2f( 1,  1);
         glVertex2f(-1,  1);
         glEnd();
      glPopMatrix();
   }
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


static void Init( void )
{
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   /* setup lighting, etc */
   if (!glutExtensionSupported("GL_EXT_fog_coord")) {
      printf("Sorry, this program requires GL_EXT_fog_coord\n");
      exit(1);
   }
   glFogi(GL_FOG_COORDINATE_SOURCE_EXT, GL_FOG_COORDINATE_EXT);
   glFogi(GL_FOG_MODE, GL_LINEAR);
   glFogf(GL_FOG_START, Near);
   glFogf(GL_FOG_END, Far);
   glEnable(GL_FOG);
   printf("Squares should be colored from white -> gray -> black.\n");
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
