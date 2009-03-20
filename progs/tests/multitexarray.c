/*
 * Test vertex arrays and multitexture.
 * Press 'a' to toggle vertex arrays on/off.
 * When you run this program you should see a square with four colors:
 *
 *      +------+------+
 *      |yellow| pink |
 *      +------+------+
 *      |green | blue |
 *      +------+------+
 */


#include <assert.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include "GL/glew.h"
#include "GL/glut.h"

static GLuint Window = 0;

static GLuint TexObj[2];
static GLfloat Angle = 0.0f;
static GLboolean UseArrays = 1, Anim = 0;

static GLfloat VertArray[4][2] = {
   {-1.2, -1.2}, {1.2, -1.2}, {1.2, 1.2}, {-1.2, 1.2}
};

static GLfloat Tex0Array[4][2] = {
   {0, 0}, {1, 0}, {1, 1}, {0, 1}
};

static GLfloat Tex1Array[4][2] = {
   {0, 0}, {1, 0}, {1, 1}, {0, 1}
};


static void init_arrays(void)
{
   glVertexPointer(2, GL_FLOAT, 0, VertArray);
   glEnableClientState(GL_VERTEX_ARRAY);

   glClientActiveTextureARB(GL_TEXTURE0_ARB);
   glTexCoordPointer(2, GL_FLOAT, 0, Tex0Array);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);

   glClientActiveTextureARB(GL_TEXTURE1_ARB);
   glTexCoordPointer(2, GL_FLOAT, 0, Tex1Array);
   glEnableClientState(GL_TEXTURE_COORD_ARRAY);
}


static void draw( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glColor3f( 0.0, 0.0, 0.0 );

   /* draw first polygon */
   glPushMatrix();
      glRotatef( Angle, 0.0, 0.0, 1.0 );

      if (UseArrays) {
         glDrawArrays(GL_POLYGON, 0, 4);
      }
      else {
         glBegin( GL_POLYGON );
            glTexCoord2f( 0.0, 0.0 );
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 0.0);
            glVertex2f( -1.0, -1.0 );

            glTexCoord2f( 1.0, 0.0 );
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, 0.0);
            glVertex2f(  1.0, -1.0 );

            glTexCoord2f( 1.0, 1.0 );
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 1.0, 1.0);
            glVertex2f(  1.0,  1.0 );

            glTexCoord2f( 0.0, 1.0 );
            glMultiTexCoord2fARB(GL_TEXTURE1_ARB, 0.0, 1.0);
            glVertex2f( -1.0,  1.0 );
         glEnd();
      }

   glPopMatrix();

   glutSwapBuffers();
}



static void idle( void )
{
   Angle += 2.0;
   glutPostRedisplay();
}



/* change view Angle, exit upon ESC */
static void key(unsigned char k, int x, int y)
{
   (void) x;
   (void) y;
   switch (k) {
      case 'a':
         UseArrays = !UseArrays;
         printf("UseArrays: %d\n", UseArrays);
         break;
      case ' ':
         Anim = !Anim;
         if (Anim)
            glutIdleFunc(idle);
         else
            glutIdleFunc(NULL);
         break;
      case 27:
         glDeleteTextures( 2, TexObj );
         glutDestroyWindow(Window);
         exit(0);
   }
   glutPostRedisplay();
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
   static int width=8, height=8;
   GLubyte tex[64][3];
   GLint i, j;

   /* generate texture object IDs */
   glGenTextures( 2, TexObj );

   /*
    * setup first texture object
    */
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glEnable( GL_TEXTURE_2D );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );

   glBindTexture( GL_TEXTURE_2D, TexObj[0] );
   assert(glIsTexture(TexObj[0]));

   /* red over black */
   for (i=0;i<height;i++) {
      for (j=0;j<width;j++) {
         int p = i*width+j;
         if (i < height / 2) {
            tex[p][0] = 0;   tex[p][1] = 0;   tex[p][2] = 0;
         }
         else {
            tex[p][0] = 255;   tex[p][1] = 0;     tex[p][2] = 0;
         }
      }
   }

   glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
   glTexParameteri( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );


   /*
    * setup second texture object
    */
   glActiveTextureARB(GL_TEXTURE1_ARB);
   glEnable( GL_TEXTURE_2D );
   glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_ADD );

   glBindTexture( GL_TEXTURE_2D, TexObj[1] );
   assert(glIsTexture(TexObj[1]));

   /* left=green, right = blue */
   for (i=0;i<height;i++) {
      for (j=0;j<width;j++) {
         int p = i*width+j;
         if (j < width / 2) {
            tex[p][0] = 0;     tex[p][1] = 255;   tex[p][2] = 0;
         }
         else {
            tex[p][0] = 0;     tex[p][1] = 0;     tex[p][2] = 255;
         }
      }
   }
   glTexImage2D( GL_TEXTURE_2D, 0, 3, width, height, 0,
                 GL_RGB, GL_UNSIGNED_BYTE, tex );
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
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );

   Window = glutCreateWindow("Texture Objects");
   glewInit();
   if (!Window) {
      exit(1);
   }

   init();
   init_arrays();

   glutReshapeFunc( reshape );
   glutKeyboardFunc( key );
   if (Anim)
      glutIdleFunc( idle );
   glutDisplayFunc( draw );
   glutMainLoop();
   return 0;
}
