/* $Id: readpix.c,v 1.1 2000/03/01 16:23:14 brianp Exp $ */

/*
 * glReadPixels and glCopyPixels test
 * 
 * Brian Paul   March 1, 2000  This file is in the public domain.
 */

/*
 * $Log: readpix.c,v $
 * Revision 1.1  2000/03/01 16:23:14  brianp
 * test glDraw/Read/CopyPixels()
 *
 */


#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
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

static GLubyte *TempImage = NULL;


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
   glClear( GL_COLOR_BUFFER_BIT );

   glRasterPos2i(5, ImgHeight+25);
   PrintString("f = toggle front/back  b = toggle scale/bias");

   /* draw original image */
   glRasterPos2i(APosX, 5);
   PrintString("Original");
   glRasterPos2i(APosX, APosY);
   glEnable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, Image);

   /* do readpixels, drawpixels */
   glRasterPos2i(BPosX, 5);
   PrintString("Read/DrawPixels");
   SetupPixelTransfer(ScaleAndBias);
   glReadPixels(APosX, APosY, ImgWidth, ImgHeight,
                ImgFormat, GL_UNSIGNED_BYTE, TempImage);
   glRasterPos2i(BPosX, BPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(GL_FALSE);
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, TempImage);

   /* do copypixels */
   glRasterPos2i(CPosX, 5);
   PrintString("CopyPixels");
   glRasterPos2i(CPosX, CPosY);
   glDisable(GL_DITHER);
   SetupPixelTransfer(ScaleAndBias);
   glCopyPixels(APosX, APosY, ImgWidth, ImgHeight, GL_COLOR);

   if (!DrawFront)
      glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, width, 0.0, height, -1.0, 1.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case 'b':
         ScaleAndBias = !ScaleAndBias;
         break;
      case 'f':
         DrawFront = !DrawFront;
         if (DrawFront)
            glDrawBuffer(GL_FRONT);
         else
            glDrawBuffer(GL_BACK);
         printf("glDrawBuffer(%s)\n", DrawFront ? "GL_FRONT" : "GL_BACK");
         break;
      case 27:
         exit(0);
         break;
   }
   glutPostRedisplay();
}


static void SpecialKey( int key, int x, int y )
{
   (void) x;
   (void) y;
#if 0
   switch (key) {
      case GLUT_KEY_UP:
         Ypos += 1;
         break;
      case GLUT_KEY_DOWN:
         Ypos -= 1;
         break;
      case GLUT_KEY_LEFT:
         Xpos -= 1;
         break;
      case GLUT_KEY_RIGHT:
         Xpos += 1;
         break;
   }
#endif
   glutPostRedisplay();
}


static void
Init( GLboolean ciMode )
{
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   Image = LoadRGBImage( IMAGE_FILE, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", IMAGE_FILE);
      exit(0);
   }

   if (ciMode) {
      /* Convert RGB image to grayscale */
      GLubyte *indexImage = malloc( ImgWidth * ImgHeight );
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

   printf("Loaded %d by %d image\n", ImgWidth, ImgHeight );

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, ImgWidth);
   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glPixelStorei(GL_PACK_ROW_LENGTH, ImgWidth);

   Reset();

   TempImage = (GLubyte *) malloc(ImgWidth * ImgHeight * 4 * sizeof(GLubyte));
   assert(TempImage);
}


int main( int argc, char *argv[] )
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
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );

   glutMainLoop();
   return 0;
}
