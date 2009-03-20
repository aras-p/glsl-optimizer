/*
 * Test the GL_NV_texture_rectangle and GL_MESA_ycrcb_texture extensions.
 *
 * Brian Paul   13 September 2002
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/tile.rgb"

static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLint ImgWidth, ImgHeight;
static GLushort *ImageYUV = NULL;
static GLubyte *ImageRGB = NULL;
static const GLuint yuvObj = 100;
static const GLuint rgbObj = 101;


static void DrawObject(void)
{
   glBegin(GL_QUADS);

   glTexCoord2f(0, 0);
   glVertex2f(-1.0, -1.0);

   glTexCoord2f(1, 0);
   glVertex2f(1.0, -1.0);

   glTexCoord2f(1, 1);
   glVertex2f(1.0, 1.0);

   glTexCoord2f(0, 1);
   glVertex2f(-1.0, 1.0);

   glEnd();
}


static void Display( void )
{
   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      glTranslatef( -1.1, 0.0, -15.0 );
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, yuvObj);
      DrawObject();
   glPopMatrix();

   glPushMatrix();
      glTranslatef(  1.1, 0.0, -15.0 );
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, rgbObj);
      DrawObject();
   glPopMatrix();

   glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.1, 1.1, -1.1, 1.1, 10.0, 100.0 );
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


static void SpecialKey( int key, int x, int y )
{
   float step = 3.0;
   (void) x;
   (void) y;

   switch (key) {
      case GLUT_KEY_UP:
         Xrot += step;
         break;
      case GLUT_KEY_DOWN:
         Xrot -= step;
         break;
      case GLUT_KEY_LEFT:
         Yrot += step;
         break;
      case GLUT_KEY_RIGHT:
         Yrot -= step;
         break;
   }
   glutPostRedisplay();
}


#define CLAMP( X, MIN, MAX )  ( (X)<(MIN) ? (MIN) : ((X)>(MAX) ? (MAX) : (X)) )



/* #define LINEAR_FILTER */

static void Init( int argc, char *argv[] )
{
   const char *file;
   GLenum  format;

   if (!glutExtensionSupported("GL_MESA_ycbcr_texture")) {
      printf("Sorry, GL_MESA_ycbcr_texture is required\n");
      exit(0);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   if (argc > 1)
      file = argv[1];
   else
      file = TEXTURE_FILE;

   /* First load the texture as YCbCr.
    */

   glBindTexture(GL_TEXTURE_2D, yuvObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   ImageYUV = LoadYUVImage(file, &ImgWidth, &ImgHeight );
   if (!ImageYUV) {
      printf("Couldn't read %s\n", TEXTURE_FILE);
      exit(0);
   }

   printf("Image: %dx%d\n", ImgWidth, ImgHeight);


   glTexImage2D(GL_TEXTURE_2D, 0,
                GL_YCBCR_MESA, 
		ImgWidth, ImgHeight, 0,
                GL_YCBCR_MESA, 
		GL_UNSIGNED_SHORT_8_8_MESA, ImageYUV);

   glEnable(GL_TEXTURE_2D);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   

   /* Now load the texture as RGB.
    */

   glBindTexture(GL_TEXTURE_2D, rgbObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   ImageRGB = LoadRGBImage(file, &ImgWidth, &ImgHeight, &format );
   if (!ImageRGB) {
      printf("Couldn't read %s\n", TEXTURE_FILE);
      exit(0);
   }

   printf("Image: %dx%d\n", ImgWidth, ImgHeight);


   glTexImage2D(GL_TEXTURE_2D, 0,
                format,
		ImgWidth, ImgHeight, 0,
                format,
		GL_UNSIGNED_BYTE, ImageRGB);

   glEnable(GL_TEXTURE_2D);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);



   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
   
   printf( "Both images should appear the same.\n" );
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 300, 300 );
   glutInitWindowPosition( 0, 0 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0] );
   glewInit();

   Init( argc, argv );

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );

   glutMainLoop();
   return 0;
}
