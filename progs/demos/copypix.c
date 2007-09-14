/**
 * glCopyPixels test
 * 
 * Brian Paul
 * 14 Sep 2007
 */


#define GL_GLEXT_PROTOTYPES

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#include "readtex.h"

#define IMAGE_FILE "../images/arch.rgb"

static int ImgWidth, ImgHeight;
static GLenum ImgFormat;
static GLubyte *Image = NULL;

static int WinWidth = 800, WinHeight = 800;
static int Xpos, Ypos;
static int Scissor = 0;
static float Xzoom, Yzoom;
static GLboolean DrawFront = GL_FALSE;
static GLboolean Dither = GL_TRUE;


static void Reset( void )
{
   Xpos = Ypos = 20;
   Scissor = 0;
   Xzoom = Yzoom = 1.0;
}


static void Display( void )
{
   const int dx = (WinWidth - ImgWidth) / 2;
   const int dy = (WinHeight - ImgHeight) / 2;

   if (DrawFront) {
      glDrawBuffer(GL_FRONT);
      glReadBuffer(GL_FRONT);
   }
   else {
      glDrawBuffer(GL_BACK);
      glReadBuffer(GL_BACK);
   }

   glClear( GL_COLOR_BUFFER_BIT );

   /* draw original image */
   glWindowPos2iARB(dx, dy);                    
   glDrawPixels(ImgWidth, ImgHeight, ImgFormat, GL_UNSIGNED_BYTE, Image);

   if (Scissor)
      glEnable(GL_SCISSOR_TEST);

   /* draw copy */
   glPixelZoom(Xzoom, Yzoom);
   glWindowPos2iARB(Xpos, Ypos);
   glCopyPixels(dx, dy, ImgWidth, ImgHeight, GL_COLOR);
   glPixelZoom(1, 1);

   glDisable(GL_SCISSOR_TEST);

   if (DrawFront)
      glFinish();
   else
      glutSwapBuffers();
}


static void Reshape( int width, int height )
{
   WinWidth = width;
   WinHeight = height;

   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho( 0.0, width, 0.0, height, 0.0, 2.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();

   glScissor(width/4, height/4, width/2, height/2);
}


static void Key( unsigned char key, int x, int y )
{
   (void) x;
   (void) y;
   switch (key) {
      case ' ':
         Reset();
         break;
      case 'd':
         Dither = !Dither;
         if (Dither)
            glEnable(GL_DITHER);
         else
            glDisable(GL_DITHER);
         break;
      case 's':
         Scissor = !Scissor;
         break;
      case 'x':
         Xzoom -= 0.1;
         break;
      case 'X':
         Xzoom += 0.1;
         break;
      case 'y':
         Yzoom -= 0.1;
         break;
      case 'Y':
         Yzoom += 0.1;
         break;
      case 'f':
         DrawFront = !DrawFront;
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
   const int step = (glutGetModifiers() & GLUT_ACTIVE_SHIFT) ? 10 : 1;
   (void) x;
   (void) y;
   switch (key) {
      case GLUT_KEY_UP:
         Ypos += step;
         break;
      case GLUT_KEY_DOWN:
         Ypos -= step;
         break;
      case GLUT_KEY_LEFT:
         Xpos -= step;
         break;
      case GLUT_KEY_RIGHT:
         Xpos += step;
         break;
   }
   glutPostRedisplay();
}


static void Init( GLboolean ciMode, const char *filename )
{
   printf("GL_VERSION = %s\n", (char *) glGetString(GL_VERSION));
   printf("GL_RENDERER = %s\n", (char *) glGetString(GL_RENDERER));

   Image = LoadRGBImage( filename, &ImgWidth, &ImgHeight, &ImgFormat );
   if (!Image) {
      printf("Couldn't read %s\n", filename);
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

   printf("Loaded %d by %d image\n", ImgWidth, ImgHeight );

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
   glPixelStorei(GL_UNPACK_ROW_LENGTH, ImgWidth);

   Reset();
}


static void Usage(void)
{
   printf("Keys:\n");
   printf("       SPACE  Reset Parameters\n");
   printf("     Up/Down  Move image up/down (SHIFT for large step)\n");
   printf("  Left/Right  Move image left/right (SHIFT for large step)\n");
   printf("           x  Decrease X-axis PixelZoom\n");
   printf("           X  Increase X-axis PixelZoom\n");
   printf("           y  Decrease Y-axis PixelZoom\n");
   printf("           Y  Increase Y-axis PixelZoom\n");
   printf("           s  Toggle GL_SCISSOR_TEST\n");
   printf("           f  Toggle front/back buffer drawing\n");
   printf("         ESC  Exit\n");
}


int main( int argc, char *argv[] )
{
   GLboolean ciMode = GL_FALSE;
   const char *filename = IMAGE_FILE;
   int i = 1;

   if (argc > i && strcmp(argv[i], "-ci")==0) {
      ciMode = GL_TRUE;
      i++;
   }
   if (argc > i) {
      filename = argv[i];
   }

   glutInit( &argc, argv );
   glutInitWindowPosition( 0, 0 );
   glutInitWindowSize( WinWidth, WinHeight );

   if (ciMode)
      glutInitDisplayMode( GLUT_INDEX | GLUT_DOUBLE );
   else
      glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE);

   glutCreateWindow(argv[0]);

   Init(ciMode, filename);
   Usage();

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );

   glutMainLoop();
   return 0;
}
