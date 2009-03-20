/*
 * Test glTexSubImage mid-way through a frame.
 *
 * The same texture is used for both quads but it gets redefined
 * with glTexSubImage (or glTexImage) after the first quad.
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "GL/glew.h"
#include "GL/glut.h"

static GLuint Window = 0;
static GLboolean Anim = GL_FALSE;
static GLfloat Angle = 0.0f;



static void
first_texture(void)
{
   static int width=8, height=8;
   static GLubyte tex1[] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 0, 1, 0, 0, 0,
     0, 0, 0, 1, 1, 0, 0, 0,
     0, 0, 0, 0, 1, 0, 0, 0,
     0, 0, 0, 0, 1, 0, 0, 0,
     0, 0, 0, 0, 1, 0, 0, 0,
     0, 0, 0, 1, 1, 1, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0 };

   GLubyte tex[64][3];
   GLint i, j;

   /* red on white */
   for (i=0;i<height;i++) {
      for (j=0;j<width;j++) {
         int p = i*width+j;
         if (tex1[(height-i-1)*width+j]) {
            tex[p][0] = 255;   tex[p][1] = 0;     tex[p][2] = 0;
         }
         else {
            tex[p][0] = 255;   tex[p][1] = 255;   tex[p][2] = 255;
         }
      }
   }

   glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex );
}


static void
second_texture(void)
{
   static int width=8, height=8;

   static GLubyte tex2[] = {
     0, 0, 0, 0, 0, 0, 0, 0,
     0, 0, 0, 2, 2, 0, 0, 0,
     0, 0, 2, 0, 0, 2, 0, 0,
     0, 0, 0, 0, 0, 2, 0, 0,
     0, 0, 0, 0, 2, 0, 0, 0,
     0, 0, 0, 2, 0, 0, 0, 0,
     0, 0, 2, 2, 2, 2, 0, 0,
     0, 0, 0, 0, 0, 0, 0, 0 };

   GLubyte tex[64][3];
   GLint i, j;

   /* green on blue */
   for (i=0;i<height;i++) {
      for (j=0;j<width;j++) {
         int p = i*width+j;
         if (tex2[(height-i-1)*width+j]) {
            tex[p][0] = 0;     tex[p][1] = 255;   tex[p][2] = 0;
         }
         else {
            tex[p][0] = 0;     tex[p][1] = 0;     tex[p][2] = 255;
         }
      }
   }
#if 0
   glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex );
#else
   glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, width, height,
                 GL_RGB, GL_UNSIGNED_BYTE, tex );
#endif
}



static void draw( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glColor3f( 1.0, 1.0, 1.0 );

   /* draw first polygon */
   glPushMatrix();
   glTranslatef( -1.0, 0.0, 0.0 );
   glRotatef( Angle, 0.0, 0.0, 1.0 );

   first_texture();

   glBegin( GL_POLYGON );
   glTexCoord2f( 0.0, 0.0 );   glVertex2f( -1.0, -1.0 );
   glTexCoord2f( 1.0, 0.0 );   glVertex2f(  1.0, -1.0 );
   glTexCoord2f( 1.0, 1.0 );   glVertex2f(  1.0,  1.0 );
   glTexCoord2f( 0.0, 1.0 );   glVertex2f( -1.0,  1.0 );
   glEnd();
   glPopMatrix();

   /* draw second polygon */
   glPushMatrix();
   glTranslatef( 1.0, 0.0, 0.0 );
   glRotatef( Angle-90.0, 0.0, 1.0, 0.0 );

   second_texture();

   glBegin( GL_POLYGON );
   glTexCoord2f( 0.0, 0.0 );   glVertex2f( -1.0, -1.0 );
   glTexCoord2f( 1.0, 0.0 );   glVertex2f(  1.0, -1.0 );
   glTexCoord2f( 1.0, 1.0 );   glVertex2f(  1.0,  1.0 );
   glTexCoord2f( 0.0, 1.0 );   glVertex2f( -1.0,  1.0 );
   glEnd();
   glPopMatrix();

   glutSwapBuffers();
}



static void idle( void )
{
   static double t0 = -1.;
   double dt, t = glutGet(GLUT_ELAPSED_TIME) / 1000.0;
   if (t0 < 0.0)
      t0 = t;
   dt = t - t0;
   t0 = t;
   Angle += 120.0*dt;
   glutPostRedisplay();
}



/* change view Angle, exit upon ESC */
static void key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
   case 'a':
      Anim = !Anim;
      if (Anim)
         glutIdleFunc( idle );
      else
         glutIdleFunc( NULL );
      break;
   case 27:
      glutDestroyWindow(Window);
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
   /* Setup texturing */
   glEnable( GL_TEXTURE_2D );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_DECAL );


   glBindTexture( GL_TEXTURE_2D, 0 );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );
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
   if (Anim)
      glutIdleFunc( idle );
   glutDisplayFunc( draw );
   glutMainLoop();
   return 0;
}
