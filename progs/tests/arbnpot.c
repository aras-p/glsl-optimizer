/*
 * Test NPOT textures with the GL_ARB_texture_non_power_of_two extension.
 * Brian Paul
 * 2 July 2003
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include "../util/readtex.c"

#define IMAGE_FILE "../images/girl.rgb"

static GLfloat Zrot = 0;

static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   glPushMatrix();
   glRotatef(Zrot, 0, 0, 1);
   glBegin(GL_POLYGON);
   glTexCoord2f(0, 0);
   glVertex2f(-1, -1);
   glTexCoord2f(1, 0);
   glVertex2f(1, -1);
   glTexCoord2f(1, 1);
   glVertex2f(1, 1);
   glTexCoord2f(0, 1);
   glVertex2f(-1, 1);
   glEnd();
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
   glTranslatef( 0.0, 0.0, -7.0 );
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'z':
         Zrot -= 1.0;
         break;
      case 'Z':
         Zrot += 1.0;
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void Init( void )
{
   GLubyte *image;
   int imgWidth, imgHeight, minDim, w;
   GLenum imgFormat;

   if (!glutExtensionSupported("GL_ARB_texture_non_power_of_two")) {
      printf("Sorry, this program requires GL_ARB_texture_non_power_of_two\n");
      exit(1);
   }

#if 1
   image = LoadRGBImage( IMAGE_FILE, &imgWidth, &imgHeight, &imgFormat );
   if (!image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }
#else
   int i, j;
   imgFormat = GL_RGB;
   imgWidth = 3;
   imgHeight = 3;
   image = malloc(imgWidth * imgHeight * 3);
   for (i = 0; i < imgHeight; i++) {
      for (j = 0; j < imgWidth; j++) {
         int k = (i * imgWidth + j) * 3;
         if ((i + j) & 1) {
            image[k+0] = 255;
            image[k+1] = 0;
            image[k+2] = 0;
         }
         else {
            image[k+0] = 0;
            image[k+1] = 255;
            image[k+2] = 0;
         }
      }
   }
#endif

   printf("Read %d x %d\n", imgWidth, imgHeight);

   minDim = imgWidth < imgHeight ? imgWidth : imgHeight;

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   /*
    * 1D Texture.  Test proxy first, if that works, test non-proxy target.
    */
   glTexImage1D(GL_PROXY_TEXTURE_1D, 0, GL_RGB, imgWidth, 0,
                imgFormat, GL_UNSIGNED_BYTE, image);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_1D, 0, GL_TEXTURE_WIDTH, &w);
   assert(w == imgWidth || w == 0);

   if (w) {
      glTexImage1D(GL_TEXTURE_1D, 0, GL_RGB, imgWidth, 0,
                   imgFormat, GL_UNSIGNED_BYTE, image);
      assert(glGetError() == GL_NO_ERROR);
   }


   /*
    * 2D Texture
    */
   glTexImage2D(GL_PROXY_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0,
                imgFormat, GL_UNSIGNED_BYTE, image);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_2D, 0, GL_TEXTURE_WIDTH, &w);
   assert(w == imgWidth || w == 0);

   if (w) {
      glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, imgWidth, imgHeight, 0,
                   imgFormat, GL_UNSIGNED_BYTE, image);
      assert(glGetError() == GL_NO_ERROR);
   }


   /*
    * 3D Texture
    */
   glTexImage3D(GL_PROXY_TEXTURE_3D, 0, GL_RGB, imgWidth, imgHeight, 1, 0,
                imgFormat, GL_UNSIGNED_BYTE, image);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_3D, 0, GL_TEXTURE_WIDTH, &w);
   assert(w == imgWidth || w == 0);

   if (w) {
      glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB, imgWidth, imgHeight, 1, 0,
                   imgFormat, GL_UNSIGNED_BYTE, image);
      assert(glGetError() == GL_NO_ERROR);
   }


   /*
    * Cube Texture
    */
   glTexImage2D(GL_PROXY_TEXTURE_CUBE_MAP, 0, GL_RGB,
                minDim, minDim, 0,
                imgFormat, GL_UNSIGNED_BYTE, image);
   glGetTexLevelParameteriv(GL_PROXY_TEXTURE_CUBE_MAP, 0, GL_TEXTURE_WIDTH, &w);
   assert(w == minDim || w == 0);

   if (w) {
      glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, GL_RGB,
                   minDim, minDim, 0,
                   imgFormat, GL_UNSIGNED_BYTE, image);
      assert(glGetError() == GL_NO_ERROR);
   }

   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
   glEnable(GL_TEXTURE_2D);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 400, 400 );
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
