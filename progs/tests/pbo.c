/*
 * GL_EXT_pixel_buffer_object test
 * 
 * Brian Paul
 * 11 March 2004
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <GL/glew.h>
#include <GL/glut.h>

#include "../util/readtex.c"  /* a hack, I know */

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

static GLuint DrawPBO, TempPBO;


static GLenum ReadFormat = GL_BGRA;
static GLenum ReadType = GL_UNSIGNED_INT_8_8_8_8_REV;



static void
CheckError(int line)
{
   GLenum err = glGetError();
   if (err) {
      printf("GL Error 0x%x at line %d\n", (int) err, line);
   }
}


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


static void
Display( void )
{
   glClearColor(.3, .3, .3, 1);
   glClear( GL_COLOR_BUFFER_BIT );

   CheckError(__LINE__);

   /** Unbind UNPACK pixel buffer before calling glBitmap */
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

   glRasterPos2i(5, ImgHeight+25);
   PrintString("f = toggle front/back  s = toggle scale/bias  b = benchmark");

   glRasterPos2i(5, ImgHeight+40);
   PrintString("GL_EXT_pixel_buffer_object test");

   /* draw original image */
   glRasterPos2i(APosX, 5);
   PrintString("Original");
   glRasterPos2i(APosX, APosY);
   glEnable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   /*** Draw from the DrawPBO */
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, DrawPBO);
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, 0);
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

   CheckError(__LINE__);

   /* do readpixels, drawpixels */
   glRasterPos2i(BPosX, 5);
   PrintString("Read/DrawPixels");
   SetupPixelTransfer(ScaleAndBias);
   /*** read into the Temp PBO */
   glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, TempPBO);
   CheckError(__LINE__);
   if (Benchmark) {
      GLint reads = 0;
      GLint endTime;
      GLint startTime = glutGet(GLUT_ELAPSED_TIME);
      GLdouble seconds, pixelsPerSecond;
      printf("Benchmarking...\n");
      do {
         glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                      ReadFormat, ReadType, 0);
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
      glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                   ReadFormat, ReadType, 0);
   }
   CheckError(__LINE__);
   glRasterPos2i(BPosX, BPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);

   CheckError(__LINE__);

   /*** draw from the Temp PBO */
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, TempPBO);
   glDrawPixels(ImgWidth, ImgHeight, ReadFormat, ReadType, 0);
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

   CheckError(__LINE__);

   /* do copypixels */
   glRasterPos2i(CPosX, 5);
   PrintString("CopyPixels");
   glRasterPos2i(CPosX, CPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(ScaleAndBias);
   glCopyPixels(APosX, APosY, ImgWidth, ImgHeight, GL_COLOR);

   CheckError(__LINE__);

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
Init(void)
{
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   if (!glutExtensionSupported("GL_EXT_pixel_buffer_object")) {
      printf("Sorry, this demo requires GL_EXT_pixel_buffer_object\n");
      exit(0);
   }

   Image = LoadRGBImage( IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }

   printf("Loaded %d by %d image\n", ImgWidth, ImgHeight );

   if (ImgFormat == GL_RGB) {
      /* convert to RGBA */
      int i;
      GLubyte *image2 = (GLubyte *) malloc(ImgWidth * ImgHeight * 4);
      printf("Converting RGB image to RGBA\n");
      for (i = 0; i < ImgWidth * ImgHeight; i++) {
         image2[i*4+0] = Image[i*3+0];
         image2[i*4+1] = Image[i*3+1];
         image2[i*4+2] = Image[i*3+2];
         image2[i*4+3] = 255;
      }
      free(Image);
      Image = image2;
      ImgFormat = GL_RGBA;
   }

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, ImgWidth);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ROW_LENGTH, ImgWidth);

   Reset();

   /* put image into DrawPBO */
   glGenBuffersARB(1, &DrawPBO);
   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, DrawPBO);
   glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_EXT,
                   ImgWidth * ImgHeight * 4, Image, GL_STATIC_DRAW);

   /* Setup TempPBO - used for glReadPixels & glDrawPixels */
   glGenBuffersARB(1, &TempPBO);
   glBindBufferARB(GL_PIXEL_PACK_BUFFER_EXT, TempPBO);
   glBufferDataARB(GL_PIXEL_PACK_BUFFER_EXT,
                   ImgWidth * ImgHeight * 4, NULL, GL_DYNAMIC_COPY);

}


int
main( int argc, char *argv[] )
{
   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( 750, 250 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0]);
   glewInit();
   Init();
   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutMainLoop();
   return 0;
}
