/*
 * GL_ARB_pixel_buffer_object test
 *
 * Command line options:
 *    -w WIDTH -h HEIGHT   sets window size
 *
 */

#define GL_GLEXT_PROTOTYPES

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <GL/glut.h>

#include "readtex.h"


#define ANIMATE 10
#define PBO 11
#define QUIT 100

static GLuint DrawPBO;

static GLboolean Animate = GL_TRUE;
static GLboolean use_pbo = 1;
static GLboolean whole_rect = 1;

static GLfloat Drift = 0.0;
static GLfloat drift_increment = 1/255.0;
static GLfloat Xrot = 20.0, Yrot = 30.0;

static GLuint Width = 1024;
static GLuint Height = 512;


static void Idle( void )
{
   if (Animate) {

      Drift += drift_increment;
      if (Drift >= 1.0)
         Drift = 0.0;

      glutPostRedisplay();
   }
}

/*static int max( int a, int b ) { return a > b ? a : b; }*/
static int min( int a, int b ) { return a < b ? a : b; }

static void DrawObject()
{
   GLint size = Width * Height * 4;
   
   if (use_pbo) {
      /* XXX: This is extremely important - semantically makes the buffer
       * contents undefined, but in practice means that the driver can
       * release the old copy of the texture and allocate a new one
       * without waiting for outstanding rendering to complete.
       */
      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, DrawPBO);
      glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_EXT, size, NULL, GL_STREAM_DRAW_ARB);

      {
	 char *image = glMapBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, GL_WRITE_ONLY_ARB);
      
	 printf("char %d\n", (unsigned char)(Drift * 255));

	 memset(image, (unsigned char)(Drift * 255), size);
      
	 glUnmapBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT);
      }
   

      /* BGRA is required for most hardware paths:
       */
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, Width, Height, 0,
		   GL_BGRA, GL_UNSIGNED_BYTE, NULL);
   }
   else {
      static char *image = NULL;

      if (image == NULL) 
	 image = malloc(size);

      glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, 0);

      memset(image, (unsigned char)(Drift * 255), size);

      /* BGRA should be the fast path for regular uploads as well.
       */
      glTexImage2D(GL_TEXTURE_RECTANGLE_ARB, 0, GL_RGBA, Width, Height, 0,
		   GL_BGRA, GL_UNSIGNED_BYTE, image);
   }

   {
      int x,y,w,h;

      if (whole_rect) {
	 x = y = 0;
	 w = Width;
	 h = Height;
      }
      else {
	 x = y = 0;
	 w = min(10, Width);
	 h = min(10, Height);
      }

      glBegin(GL_QUADS);

      glTexCoord2f( x, y);
      glVertex2f( x, y );

      glTexCoord2f( x, y + h);
      glVertex2f( x, y + h);

      glTexCoord2f( x + w + .5, y + h);
      glVertex2f( x + w, y + h );

      glTexCoord2f( x + w, y + .5);
      glVertex2f( x + w, y );

      glEnd();
   }
}



static void Display( void )
{
   static GLint T0 = 0;
   static GLint Frames = 0;
   GLint t;

   glClear( GL_COLOR_BUFFER_BIT );

   glPushMatrix();
      DrawObject();
   glPopMatrix();

   glutSwapBuffers();

   Frames++;

   t = glutGet(GLUT_ELAPSED_TIME);
   if (t - T0 >= 1000) {
      GLfloat seconds = (t - T0) / 1000.0;

      GLfloat fps = Frames / seconds;
      printf("%d frames in %6.3f seconds = %6.3f FPS\n", Frames, seconds, fps);

      drift_increment = 2.2 * seconds / Frames;
      T0 = t;
      Frames = 0;
   }
}


static void Reshape( int width, int height )
{
   glViewport( 0, 0, width, height );
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
/*    glFrustum( -1.0, 1.0, -1.0, 1.0, 10.0, 100.0 ); */
   gluOrtho2D( 0, width, height, 0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef(0.375, 0.375, 0);
}


static void ModeMenu(int entry)
{
   if (entry==ANIMATE) {
      Animate = !Animate;
   }
   else if (entry==PBO) {
      use_pbo = !use_pbo;
   }
   else if (entry==QUIT) {
      exit(0);
   }

   glutPostRedisplay();
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


static void Init( int argc, char *argv[] )
{
   const char *exten = (const char *) glGetString(GL_EXTENSIONS);
   GLuint texObj;
   GLint size;


   if (!strstr(exten, "GL_ARB_pixel_buffer_object")) {
      printf("Sorry, GL_ARB_pixel_buffer_object not supported by this renderer.\n");
      exit(1);
   }

   glGetIntegerv(GL_MAX_TEXTURE_SIZE, &size);
   printf("%d x %d max texture size\n", size, size);

   glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

   /* allocate two texture objects */
   glGenTextures(1, &texObj);

   /* setup the texture objects */
   glActiveTextureARB(GL_TEXTURE0_ARB);
   glBindTexture(GL_TEXTURE_RECTANGLE_ARB, texObj);

   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
   glTexParameteri(GL_TEXTURE_RECTANGLE_ARB, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

   glGenBuffersARB(1, &DrawPBO);

   glBindBufferARB(GL_PIXEL_UNPACK_BUFFER_EXT, DrawPBO);
   glBufferDataARB(GL_PIXEL_UNPACK_BUFFER_EXT,
		   Width * Height * 4, NULL, GL_STREAM_DRAW);

   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

   glEnable(GL_TEXTURE_RECTANGLE_ARB);

   glShadeModel(GL_SMOOTH);
   glClearColor(0.3, 0.3, 0.4, 1.0);

   if (argc > 1 && strcmp(argv[1], "-info")==0) {
      printf("GL_RENDERER   = %s\n", (char *) glGetString(GL_RENDERER));
      printf("GL_VERSION    = %s\n", (char *) glGetString(GL_VERSION));
      printf("GL_VENDOR     = %s\n", (char *) glGetString(GL_VENDOR));
      printf("GL_EXTENSIONS = %s\n", (char *) glGetString(GL_EXTENSIONS));
   }
}


int main( int argc, char *argv[] )
{
   GLint i;

   glutInit( &argc, argv );

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-w") == 0) {
         Width = atoi(argv[i+1]);
         if (Width <= 0) {
            printf("Error, bad width\n");
            exit(1);
         }
         i++;
      }
      else if (strcmp(argv[i], "-h") == 0) {
         Height = atoi(argv[i+1]);
         if (Height <= 0) {
            printf("Error, bad height\n");
            exit(1);
         }
         i++;
      }
   }

   glutInitWindowSize( Width, Height );
   glutInitWindowPosition( 0, 0 );
   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE );
   glutCreateWindow(argv[0] );

   Init( argc, argv );

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutSpecialFunc( SpecialKey );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutCreateMenu(ModeMenu);
   glutAddMenuEntry("Toggle Animation", ANIMATE);
   glutAddMenuEntry("Toggle PBO", PBO);
   glutAddMenuEntry("Quit", QUIT);
   glutAttachMenu(GLUT_RIGHT_BUTTON);

   glutMainLoop();
   return 0;
}
