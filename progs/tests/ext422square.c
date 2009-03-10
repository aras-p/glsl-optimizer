/*
 * Exercise the EXT_422_pixels extension, a less convenient
 * alternative to MESA_ycbcr_texture.  Requires ARB_fragment_program
 * to perform the final YUV->RGB conversion.
 *
 * Brian Paul   13 September 2002
 * Keith Whitwell 30 November 2004
 */


#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glew.h>
#include <GL/glut.h>
#include <assert.h>

#include "../util/readtex.c"   /* I know, this is a hack. */

#define TEXTURE_FILE "../images/tile.rgb"

static GLfloat Xrot = 0, Yrot = 0, Zrot = 0;
static GLint ImgWidth, ImgHeight;
static GLushort *ImageYUV = NULL;
static const GLuint yuvObj = 100;
static const GLuint rgbObj = 101;

static void Init( int argc, char *argv[] );

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
   static int firsttime = 1;

   if (firsttime) {
      firsttime = 0;
      Init( 0, 0 );		/* don't ask */
   }

   glClear( GL_COLOR_BUFFER_BIT );
   glBindTexture(GL_TEXTURE_2D, yuvObj);

   glPushMatrix();
      glEnable(GL_FRAGMENT_PROGRAM_ARB); 
      glTranslatef( -1.1, 0.0, -15.0 );
      glRotatef(Xrot, 1.0, 0.0, 0.0);
      glRotatef(Yrot, 0.0, 1.0, 0.0);
      glRotatef(Zrot, 0.0, 0.0, 1.0);
      glBindTexture(GL_TEXTURE_2D, yuvObj);
      DrawObject();
   glPopMatrix();

   glPushMatrix();
      glDisable(GL_FRAGMENT_PROGRAM_ARB);
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




/* #define LINEAR_FILTER */

static void Init( int argc, char *argv[] )
{
   const char *file;
   const GLfloat yuvtorgb[16] = {
      1.164,   1.164,    1.164,      0,
      0,       -.391,    2.018,      0,
      1.596,   -.813,    0.0,        0,
      (-.0625*1.164 + -.5*1.596),     (-.0625*1.164 + -.5*-.813 + -.5*-.391),     (-.0625*1.164 + -.5*2.018),      1  
   };

   if (!glutExtensionSupported("GL_ARB_fragment_program")) {
      printf("Error: GL_ARB_fragment_program not supported!\n");
      exit(1);
   }

   if (!glutExtensionSupported("GL_EXT_422_pixels")) {
      printf("Error: GL_EXT_422_pixels not supported!\n");
      exit(1);
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   file = TEXTURE_FILE;

   /* Load the texture as YCbCr.
    */
   glBindTexture(GL_TEXTURE_2D, yuvObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   ImageYUV = LoadYUVImage(file, &ImgWidth, &ImgHeight );
   if (!ImageYUV) {
      printf("Couldn't read %s\n", TEXTURE_FILE);
      exit(0);
   }

   glTexImage2D(GL_TEXTURE_2D, 0,
		GL_RGB,
		ImgWidth, ImgHeight, 0,
		GL_422_EXT,
		GL_UNSIGNED_BYTE, ImageYUV); 

   glEnable(GL_TEXTURE_2D);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);

   {
      static const char *modulateYUV =
	 "!!ARBfp1.0\n"
	 "TEMP R0;\n"
	 "TEX R0, fragment.texcoord[0], texture[0], 2D; \n"

  	 "ADD R0, R0, {-0.0625, -0.5, -0.5, 0.0}; \n"
   	 "DP3 result.color.x, R0, {1.164,  1.596,  0.0}; \n"   
  	 "DP3 result.color.y, R0, {1.164, -0.813, -0.391}; \n" 
  	 "DP3 result.color.z, R0, {1.164,  0.0,    2.018}; \n" 
  	 "MOV result.color.w, R0.w; \n"  

	 "END"
	 ;

      GLuint modulateProg;


      /* Setup the fragment program */
      glGenProgramsARB(1, &modulateProg);
      glBindProgramARB(GL_FRAGMENT_PROGRAM_ARB, modulateProg);
      glProgramStringARB(GL_FRAGMENT_PROGRAM_ARB, GL_PROGRAM_FORMAT_ASCII_ARB,
			 strlen(modulateYUV), (const GLubyte *)modulateYUV);

      printf("glGetError = 0x%x\n", (int) glGetError());
      printf("glError(GL_PROGRAM_ERROR_STRING_ARB) = %s\n",
	     (char *) glGetString(GL_PROGRAM_ERROR_STRING_ARB));
      assert(glIsProgramARB(modulateProg));

   }

   /* Now the same, but use a color matrix to do the conversion at
    * upload time:
    */
   glBindTexture(GL_TEXTURE_2D, rgbObj);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

   glMatrixMode( GL_COLOR_MATRIX );
   glLoadMatrixf( yuvtorgb );
   
   glTexImage2D(GL_TEXTURE_2D, 0,
                GL_RGB,
		ImgWidth, ImgHeight, 0,
                GL_422_EXT,
		GL_UNSIGNED_BYTE, ImageYUV);

   glLoadIdentity();
   glMatrixMode( GL_MODELVIEW );

   glEnable(GL_TEXTURE_2D);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);


   glShadeModel(GL_FLAT);
   glClearColor(0.3, 0.3, 0.4, 1.0);
}


int main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowSize( 300, 300 );
   glutInitWindowPosition( 0, 0 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0] );
   glewInit();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutMainLoop();
   return 0;
}
