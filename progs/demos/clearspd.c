
/*
 * Simple GLUT program to measure glClear() and glutSwapBuffers() speed.
 * Brian Paul  February 15, 1997  This file in public domain.
 */


#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glut.h>


static float MinPeriod = 2.0;   /* 2 seconds */
static int ColorMode = GLUT_RGB;
static int Width = 400.0;
static int Height = 400.0;
static int Loops = 100;
static float ClearColor = 0.0;
static GLbitfield BufferMask = GL_COLOR_BUFFER_BIT;
static GLboolean SwapFlag = GL_FALSE;



static void Idle( void )
{
   glutPostRedisplay();
}


static void Display( void )
{
   double t0, t1;
   double clearRate;
   double pixelRate;
   int i;

   glClearColor( ClearColor, ClearColor, ClearColor, 0.0 );
   ClearColor += 0.1;
   if (ClearColor>1.0)
      ClearColor = 0.0;

   if (SwapFlag) {
      t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      for (i=0;i<Loops;i++) {
         glClear( BufferMask );
         glutSwapBuffers();
      }
      glFinish();
      t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   }
   else {
      t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      for (i=0;i<Loops;i++) {
         glClear( BufferMask );
      }
      glFinish();
      t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
      glutSwapBuffers();
   }

   /* NOTE: If clearspd doesn't map it's window immediately on
    * starting, swaps will be istantaneous, so this will send Loops
    * towards infinity.  When a window is finally mapped, it may be
    * minutes before the first call to glutSwapBuffers, making it look
    * like there's a driver bug.
    */
   if (t1-t0 < MinPeriod) {
      /* Next time do more clears to get longer elapsed time */
      Loops *= 2;
      return;
   }

   clearRate = Loops / (t1-t0);
   pixelRate = clearRate * Width * Height;
   if (SwapFlag) {
      printf("Rate: %d clears+swaps in %gs = %g clears+swaps/s   %g pixels/s\n",
             Loops, t1-t0, clearRate, pixelRate );
   }
   else {
      printf("Rate: %d clears in %gs = %g clears/s   %g pixels/s\n",
             Loops, t1-t0, clearRate, pixelRate);
   }
}


static void Reshape( int width, int height )
{
   Width = width;
   Height = height;
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glOrtho(0.0, width, 0.0, height, -1.0, 1.0);
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
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


static void Init( int argc, char *argv[] )
{
   int i;
   for (i=1; i<argc; i++) {
      if (strcmp(argv[i],"+rgb")==0)
         ColorMode = GLUT_RGB;
      else if (strcmp(argv[i],"+ci")==0)
         ColorMode = GLUT_INDEX;
      else if (strcmp(argv[i],"-color")==0)
         BufferMask = 0;
      else if (strcmp(argv[i],"+depth")==0)
         BufferMask |= GL_DEPTH_BUFFER_BIT;
      else if (strcmp(argv[i],"+alpha")==0)
         ColorMode = GLUT_RGB | GLUT_ALPHA;
      else if (strcmp(argv[i],"+stencil")==0)
         BufferMask |= GL_STENCIL_BUFFER_BIT;
      else if (strcmp(argv[i],"+accum")==0)
         BufferMask |= GL_ACCUM_BUFFER_BIT;
      else if (strcmp(argv[i],"-width")==0) {
         Width = atoi(argv[i+1]);
         i++;
      }
      else if (strcmp(argv[i],"-height")==0) {
         Height = atoi(argv[i+1]);
         i++;
      }
      else if (strcmp(argv[i],"+swap")==0) {
         SwapFlag = GL_TRUE;
      }
      else if (strcmp(argv[i],"-swap")==0) {
         SwapFlag = GL_FALSE;
      }
      else
         printf("Unknown option: %s\n", argv[i]);
   }

   if (ColorMode & GLUT_ALPHA)
      printf("Mode:  RGB + Alpha\n");
   else if (ColorMode==GLUT_RGB)
      printf("Mode:  RGB\n");
   else
      printf("Mode:  Color Index\n");
   printf("SwapBuffers: %s\n", SwapFlag ? "yes" : "no" );
   printf("Size: %d x %d\n", Width, Height);
   printf("Buffers: ");
   if (BufferMask & GL_COLOR_BUFFER_BIT)  printf("color ");
   if (BufferMask & GL_DEPTH_BUFFER_BIT)  printf("depth ");
   if (BufferMask & GL_STENCIL_BUFFER_BIT)  printf("stencil ");
   if (BufferMask & GL_ACCUM_BUFFER_BIT)  printf("accum ");
   printf("\n");
}


static void Help( const char *program )
{
   printf("%s options:\n", program);
   printf("  +rgb       RGB mode\n");
   printf("  +ci        color index mode\n");
   printf("  -color     don't clear color buffer\n");
   printf("  +alpha     clear alpha buffer\n");
   printf("  +depth     clear depth buffer\n");
   printf("  +stencil   clear stencil buffer\n");
   printf("  +accum     clear accum buffer\n");
   printf("  +swap      also do SwapBuffers\n");
   printf("  -swap      don't do SwapBuffers\n");
}


int main( int argc, char *argv[] )
{
   GLint mode;

   printf("For options:  %s -help\n", argv[0]);

   Init( argc, argv );

   glutInit( &argc, argv );
   glutInitWindowSize( (int) Width, (int) Height );
   glutInitWindowPosition( 0, 0 );

   mode = ColorMode | GLUT_DOUBLE;
   if (BufferMask & GL_STENCIL_BUFFER_BIT)
      mode |= GLUT_STENCIL;
   if (BufferMask & GL_ACCUM_BUFFER_BIT)
      mode |= GLUT_ACCUM;
   if (BufferMask & GL_DEPTH_BUFFER_BIT)
      mode |= GLUT_DEPTH;
         
   glutInitDisplayMode(mode);

   glutCreateWindow( argv[0] );

   if (argc==2 && strcmp(argv[1],"-help")==0) {
      Help(argv[0]);
      return 0;
   }

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
