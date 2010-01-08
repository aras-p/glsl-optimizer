
/*
 * Example of how to use the GL_MESA_window_pos extension.
 * Brian Paul   This file is in the public domain.
 */

#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "GL/glew.h"
#include "GL/glut.h"

#include "readtex.h"

#define IMAGE_FILE "../images/girl.rgb"


#ifndef M_PI
#  define M_PI 3.14159265
#endif



static GLubyte *Image;
static int ImgWidth, ImgHeight;
static GLenum ImgFormat;

static PFNGLWINDOWPOS2FPROC WindowPosFunc;

static void draw( void )
{
   GLfloat angle;

   glClear( GL_COLOR_BUFFER_BIT );

   for (angle = -45.0; angle <= 135.0; angle += 10.0) {
      GLfloat x = 50.0 + 200.0 * cos( angle * M_PI / 180.0 );
      GLfloat y = 50.0 + 200.0 * sin( angle * M_PI / 180.0 );

      /* Don't need to worry about the modelview or projection matrices!!! */
      (*WindowPosFunc)( x, y );

      glDrawPixels( ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, Image );
   }
   glFinish();
}


static void key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 27:
         exit(0);
   }
}


/* new window size or exposure */
static void reshape( int width, int height )
{
   glViewport(0, 0, (GLint)width, (GLint)height);
}


static void init( void )
{
   if (GLEW_ARB_window_pos) {
      printf("Using GL_ARB_window_pos\n");
      WindowPosFunc = glWindowPos2fARB;
   }
   else
   if (GLEW_MESA_window_pos) {
      printf("Using GL_MESA_window_pos\n");
      WindowPosFunc = glWindowPos2fMESA;
   }
   else
   {
      printf("Sorry, GL_ARB/MESA_window_pos extension not available.\n");
      exit(1);
   }

   Image = LoadRGBImage( IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
}


int main( int argc, char *argv[] )
{
   glutInitWindowSize(500, 500);
   glutInit(&argc, argv);
   glutInitDisplayMode( GLUT_RGB );

   if (glutCreateWindow("winpos") <= 0) {
      exit(0);
   }

   glewInit();

   init();

   glutReshapeFunc( reshape );
   glutKeyboardFunc( key );
   glutDisplayFunc( draw );
   glutMainLoop();
   return 0;
}
