/*
 * Exercise GL_EXT_secondary_color
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
      GLfloat x =  t * 10.0 - 5.0;
      GLfloat g = t;

      /* top row: untextured */
      glColor3f(1, 0, 0);
      glPushMatrix();
         glTranslatef(x, 1.2, 0);
#if defined(GL_EXT_secondary_color)
         glSecondaryColor3fEXT(0, g, 0);
#endif
         glBegin(GL_POLYGON);
         glVertex2f(-1, -1);
         glVertex2f( 1, -1);
         glVertex2f( 1,  1);
         glVertex2f(-1,  1);
         glEnd();
      glPopMatrix();

      /* bottom row: textured */
      glColor3f(1, 1, 1);
      glEnable(GL_TEXTURE_2D);
      glPushMatrix();
         glTranslatef(x, -1.2, 0);
#if defined(GL_EXT_secondary_color)
         glSecondaryColor3fEXT(0, g, 0);
#endif
         glBegin(GL_POLYGON);
         glTexCoord2f(0, 0);  glVertex2f(-1, -1);
         glTexCoord2f(1, 0);  glVertex2f( 1, -1);
         glTexCoord2f(1, 1);  glVertex2f( 1,  1);
         glTexCoord2f(0, 1);  glVertex2f(-1,  1);
         glEnd();
      glPopMatrix();
      glDisable(GL_TEXTURE_2D);
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
   GLubyte image[4*4][3];
   GLint i;
   if (!glutExtensionSupported("GL_EXT_secondary_color")) {
      printf("Sorry, this program requires GL_EXT_secondary_color\n");
      exit(1);
   }

   /* setup red texture with one back texel */
   for (i = 0; i < 4*4; i++) {
      if (i == 0) {
         image[i][0] = 0;
         image[i][1] = 0;
         image[i][2] = 0;
      }
      else {
         image[i][0] = 255;
         image[i][1] = 0;
         image[i][2] = 0;
      }
   }
   glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 4, 4, 0,
                GL_RGB, GL_UNSIGNED_BYTE, image);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

#if defined(GL_EXT_secondary_color)
   glEnable(GL_COLOR_SUM_EXT);
#endif
   glLightModeli(GL_LIGHT_MODEL_COLOR_CONTROL, GL_SEPARATE_SPECULAR_COLOR);

   printf("Squares should be colored from red -> orange -> yellow.\n");
   printf("Top row is untextured.\n");
   printf("Bottom row is textured (red texture with one black texel).\n");
   printf("Rows should be identical, except for lower-left texel.\n");
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( Width, Height );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   Init();
   glutMainLoop();
   return 0;
}
