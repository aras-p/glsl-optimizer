
/*
 * glReadPixels and glCopyPixels test
 * 
 * Brian Paul   March 1, 2000  This file is in the public domain.
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glut.h>

#include "readtex.h"

#define IMAGE_FILE "../images/girl.rgb"

static int ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

static int APosX, APosY;  /* simple drawpixels */
static int BPosX, BPosY;  /* read/draw pixels */
static int CPosX, CPosY;  /* copypixels */

static GLboolean DrawFront = GL_FALSE;
static GLboolean ScaleAndBias = GL_FALSE;
static GLboolean Benchmark = GL_FALSE;
static GLubyte *TempImage = NULL;

#define COMBO 1
#if COMBO == 0
#define ReadFormat ImgFormat
#define ReadType GL_UNSIGNED_BYTE
#elif COMBO == 1
static GLenum ReadFormat = GL_RGBA;
static GLenum ReadType = GL_UNSIGNED_BYTE;
#elif COMBO == 2
static GLenum ReadFormat = GL_RGB;
static GLenum ReadType = GL_UNSIGNED_BYTE;
#elif COMBO == 3
static GLenum ReadFormat = GL_RGB;
static GLenum ReadType = GL_UNSIGNED_SHORT_5_6_5;
#elif COMBO == 4
static GLenum ReadFormat = GL_RGBA;
static GLenum ReadType = GL_UNSIGNED_SHORT_1_5_5_5_REV;
#elif COMBO == 5
static GLenum ReadFormat = GL_BGRA;
static GLenum ReadType = GL_UNSIGNED_SHORT_5_5_5_1;
#elif COMBO == 6
static GLenum ReadFormat = GL_BGRA;
static GLenum ReadType = GL_UNSIGNED_SHORT_4_4_4_4_REV;
#elif COMBO == 7
static GLenum ReadFormat = GL_RGBA;
static GLenum ReadType = GL_HALF_FLOAT_ARB;
#undef GL_OES_read_format
#endif


static void
Reset( void )
{
   APosX = 5;     APosY = 20;
   BPosX = APosX + ImgWidth + 5;   BPosY = 20;
   CPosX = BPosX + ImgWidth + 5;   CPosY = 20;
}


static void
PrintString(const char *s)
{
   while (*s) {
      glutBitmapCharacter(GLUT_BITMAP_8_BY_13, (int) *s);
      s++;
   }
}


static void
SetupPixelTransfer(GLboolean invert)
{
   if (invert) {
      glPixelTransferf(GL_RED_SCALE, -1.0);
      glPixelTransferf(GL_RED_BIAS, 1.0);
      glPixelTransferf(GL_GREEN_SCALE, -1.0);
      glPixelTransferf(GL_GREEN_BIAS, 1.0);
      glPixelTransferf(GL_BLUE_SCALE, -1.0);
      glPixelTransferf(GL_BLUE_BIAS, 1.0);
   }
   else {
      glPixelTransferf(GL_RED_SCALE, 1.0);
      glPixelTransferf(GL_RED_BIAS, 0.0);
      glPixelTransferf(GL_GREEN_SCALE, 1.0);
      glPixelTransferf(GL_GREEN_BIAS, 0.0);
      glPixelTransferf(GL_BLUE_SCALE, 1.0);
      glPixelTransferf(GL_BLUE_BIAS, 0.0);
   }
}


/**
 * Exercise Pixel Pack parameters by reading the image in four pieces.
 */
static void
ComplexReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
                  GLenum format, GLenum type, GLvoid *pixels)
{
   const GLsizei width0 = width / 2;
   const GLsizei width1 = width - width0;
   const GLsizei height0 = height / 2;
   const GLsizei height1 = height - height0;

   glPixelStorei(GL_PACK_ROW_LENGTH, width);

   /* lower-left quadrant */
   glReadPixels(x, y, width0, height0, format, type, pixels);

   /* lower-right quadrant */
   glPixelStorei(GL_PACK_SKIP_PIXELS, width0);
   glReadPixels(x + width0, y, width1, height0, format, type, pixels);

   /* upper-left quadrant */
   glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_PACK_SKIP_ROWS, height0);
   glReadPixels(x, y + height0, width0, height1, format, type, pixels);

   /* upper-right quadrant */
   glPixelStorei(GL_PACK_SKIP_PIXELS, width0);
   glPixelStorei(GL_PACK_SKIP_ROWS, height0);
   glReadPixels(x + width0, y + height0, width1, height1, format, type, pixels);

   /* restore defaults */
   glPixelStorei(GL_PACK_SKIP_PIXELS, 0);
   glPixelStorei(GL_PACK_SKIP_ROWS, 0);
   glPixelStorei(GL_PACK_ROW_LENGTH, ImgWidth);
}



static void
Display( void )
{
   glClearColor(.3, .3, .3, 1);
   glClear( GL_COLOR_BUFFER_BIT );

   glRasterPos2i(5, ImgHeight+25);
   PrintString("f = toggle front/back  s = toggle scale/bias  b = benchmark");

   /* draw original image */
   glRasterPos2i(APosX, 5);
   PrintString("Original");
   glRasterPos2i(APosX, APosY);
   glEnable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, Image);

   /* might try alignment=4 here for testing */
   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);

   /* do readpixels, drawpixels */
   glRasterPos2i(BPosX, 5);
   PrintString("Read/DrawPixels");
   SetupPixelTransfer(ScaleAndBias);
   if (Benchmark) {
      GLint reads = 0;
      GLint endTime;
      GLint startTime = glutGet(GLUT_ELAPSED_TIME);
      GLdouble seconds, pixelsPerSecond;
      printf("Benchmarking...\n");
      do {
         glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                      ReadFormat, ReadType, TempImage);
         reads++;
         endTime = glutGet(GLUT_ELAPSED_TIME);
      } while (endTime - startTime < 4000);   /* 4 seconds */
      seconds = (double) (endTime - startTime) / 1000.0;
      pixelsPerSecond = reads * ImgWidth * ImgHeight / seconds;
      printf("Result:  %d reads in %f seconds = %f pixels/sec\n",
             reads, seconds, pixelsPerSecond);
      Benchmark = GL_FALSE;
   }
   else {
      /* clear the temporary image to white (helpful for debugging */
      memset(TempImage, 255, ImgWidth * ImgHeight * 4);
#if 1
      glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                   ReadFormat, ReadType, TempImage);
      (void) ComplexReadPixels;
#else
      /* you might use this when debugging */
      ComplexReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                        ReadFormat, ReadType, TempImage);
#endif
   }
   glRasterPos2i(BPosX, BPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   glDrawPixels(ImgWidth, ImgHeight, ReadFormat, ReadType, TempImage);

   /* do copypixels */
   glRasterPos2i(CPosX, 5);
   PrintString("CopyPixels");
   glRasterPos2i(CPosX, CPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(ScaleAndBias);
   glCopyPixels(APosX, APosY, ImgWidth, ImgHeight, GL_COLOR);

   if (!DrawFront)
      glutSwapBuffers();
   else
      glFinish();
}


static void
Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, width, 0.0, height, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void
Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         Benchmark = GL_TRUE;
         break;
      case 's':
         ScaleAndBias = !ScaleAndBias;
         break;
      case 'f':
         DrawFront = !DrawFront;
         if (DrawFront) {
            glDrawBuffer(GL_FRONT);
            glReadBuffer(GL_FRONT);
         }
         else {
            glDrawBuffer(GL_BACK);
            glReadBuffer(GL_BACK);
         }
         printf("glDrawBuffer(%s)\n", DrawFront ? "GL_FRONT" : "GL_BACK");
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void
Init( GLboolean ciMode )
{
   GLboolean have_read_format = GL_FALSE;

   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   Image = LoadRGBImage( IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }

   if (ciMode) {
      /* Convert RGB image to grayscale */
      GLubyte *indexImage = (GLubyte *) malloc( ImgWidth * ImgHeight );
      GLint i;
      for (i=0; i<ImgWidth*ImgHeight; i++) {
         int gray = Image[i*3] + Image[i*3+1] + Image[i*3+2];
         indexImage[i] = gray / 3;
      }
      free(Image);
      Image = indexImage;
      ImgFormat = GL_COLOR_INDEX;

      for (i=0;i<255;i++) {
         float g = i / 255.0;
         glutSetColor(i, g, g, g);
      }
   }

#ifdef GL_OES_read_format
   if ( glutExtensionSupported( "GL_OES_read_format" ) ) {
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_TYPE_OES,   (GLint *) &ReadType);
      glGetIntegerv(GL_IMPLEMENTATION_COLOR_READ_FORMAT_OES, (GLint *) &ReadFormat);

      have_read_format = GL_TRUE;
   }
#endif

   printf( "GL_OES_read_format %ssupported.  "
	   "Using type / format = 0x%04x / 0x%04x\n",
	   (have_read_format) ? "" : "not ",
	   ReadType, ReadFormat );

   printf("Loaded %d by %d image\n", ImgWidth, ImgHeight );

   glPixelStorei(GL_UNPACK_ROW_LENGTH, ImgWidth);
   glPixelStorei(GL_PACK_ROW_LENGTH, ImgWidth);

   Reset();

   /* allocate large TempImage to store and image data type, plus an
    * extra 1KB in case we're tinkering with pack alignment.
    */
   TempImage = (GLubyte *) malloc(ImgWidth * ImgHeight * 4 * 4
                                  + 1000);
   assert(TempImage);
}


int
main( int argc, char *argv[] )
{
   GLboolean ciMode = GL_FALSE;
   if (argc > 1 && strcmp(argv[1], "-ci")==0) {
      ciMode = GL_TRUE;
   }
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 750, 250 );
   if (ciMode)
      glutInitDisplayMode( GLUT_INDEX | GLUT_DOUBLE );
   else
      glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   Init(ciMode);
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutMainLoop();
   return 0;
}
