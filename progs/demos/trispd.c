
/*
 * Simple GLUT program to measure triangle strip rendering speed.
 * Brian Paul  February 15, 1997  This file is in the public domain.
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <GL/glut.h>


static float MinPeriod = 2.0;   /* 2 seconds */
static float Width = 400.0;
static float Height = 400.0;
static int Loops = 1;
static int Size = 50;
static int Texture = 0;



static void Idle( void )
{
   glutPostRedisplay();
}


static void Display( void )
{
   float x, y;
   float xStep;
   float yStep;
   double t0, t1;
   double triRate;
   double pixelRate;
   int triCount;
   int i;
   float red[3] = { 1.0, 0.0, 0.0 };
   float blue[3] = { 0.0, 0.0, 1.0 };

   xStep = yStep = sqrt( 2.0 * Size );

   glClear( GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT );

   triCount = 0;
   t0 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   if (Texture) {
      float uStep = xStep / Width;
      float vStep = yStep / Height;
      float u, v;
      for (i=0; i<Loops; i++) {
	 for (y=1.0, v=0.0f; y<Height-yStep; y+=yStep, v+=vStep) {
	    glBegin(GL_TRIANGLE_STRIP);
	    for (x=1.0, u=0.0f; x<Width; x+=xStep, u+=uStep) {
	       glColor3fv(red);
	       glTexCoord2f(u, v);	    
	       glVertex2f(x, y);
	       glColor3fv(blue);
	       glTexCoord2f(u, v+vStep);
	       glVertex2f(x, y+yStep);
	       triCount += 2;
	    }
	    glEnd();
	    triCount -= 2;
	 }
      }
   }
   else {
      for (i=0; i<Loops; i++) {
	 for (y=1.0; y<Height-yStep; y+=yStep) {
	    glBegin(GL_TRIANGLE_STRIP);
	    for (x=1.0; x<Width; x+=xStep) {
	       glColor3fv(red);
	       glVertex2f(x, y);
	       glColor3fv(blue);
	       glVertex2f(x, y+yStep);
	       triCount += 2;
	    }
	    glEnd();
	    triCount -= 2;
	 }
      }
   }
   glFinish();
   t1 = glutGet(GLUT_ELAPSED_TIME) * 0.001;
   
   if (t1-t0 < MinPeriod) {
      /* Next time draw more triangles to get longer elapsed time */
      Loops *= 2;
      return;
   }

   triRate = triCount / (t1-t0);
   pixelRate = triRate * Size;
   printf("Rate: %d tri in %gs = %g tri/s  %d pixels/s\n",
          triCount, t1-t0, triRate, (int)pixelRate);

   glutSwapBuffers();
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


static void LoadTex(int comp, int filter)
{
   GLubyte *pixels;
   int x, y;
   pixels = (GLubyte *) malloc(4*256*256);
   for (y = 0; y < 256; ++y)
      for (x = 0; x < 256; ++x) {
	 pixels[(y*256+x)*4+0] = (int)(128.5 + 127.0 * cos(0.024544 * x));
	 pixels[(y*256+x)*4+1] = 255;
	 pixels[(y*256+x)*4+2] = (int)(128.5 + 127.0 * cos(0.024544 * y));
	 pixels[(y*256+x)*4+3] = 255;
      }
   glEnable(GL_TEXTURE_2D);
   glTexImage2D(GL_TEXTURE_2D, 0, comp, 256, 256, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, filter);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
   glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
   glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
   printf("Texture: GL_MODULATE, %d comps, %s\n", comp, filter == GL_NEAREST ? "GL_NEAREST" : "GL_LINEAR");
}


static void Init( int argc, char *argv[] )
{
   GLint shade;
   GLint rBits, gBits, bBits;
   int filter = GL_NEAREST, comp = 3;

   int i;
   for (i=1; i<argc; i++) {
      if (strcmp(argv[i],"-dither")==0)
         glDisable(GL_DITHER);
      else if (strcmp(argv[i],"+dither")==0)
         glEnable(GL_DITHER);
      else if (strcmp(argv[i],"+smooth")==0)
         glShadeModel(GL_SMOOTH);
      else if (strcmp(argv[i],"+flat")==0)
         glShadeModel(GL_FLAT);
      else if (strcmp(argv[i],"+depth")==0)
         glEnable(GL_DEPTH_TEST);
      else if (strcmp(argv[i],"-depth")==0)
         glDisable(GL_DEPTH_TEST);
      else if (strcmp(argv[i],"-size")==0) {
         Size = atoi(argv[i+1]);
         i++;
      }
      else if (strcmp(argv[i],"-texture")==0)
	 Texture = 0;
      else if (strcmp(argv[i],"+texture")==0)
	 Texture = 1;
      else if (strcmp(argv[i],"-linear")==0)
	 filter = GL_NEAREST;
      else if (strcmp(argv[i],"+linear")==0)
	 filter = GL_LINEAR;
      else if (strcmp(argv[i],"-persp")==0)
	 glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
      else if (strcmp(argv[i],"+persp")==0)
	 glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
      else if (strcmp(argv[i],"-comp")==0) {
	 comp = atoi(argv[i+1]);
	 i++;
      }
      else
         printf("Unknown option: %s\n", argv[i]);
   }

   glGetIntegerv(GL_SHADE_MODEL, &shade);

   printf("Dither: %s\n", glIsEnabled(GL_DITHER) ? "on" : "off");
   printf("ShadeModel: %s\n", (shade==GL_FLAT) ? "flat" : "smooth");
   printf("DepthTest: %s\n", glIsEnabled(GL_DEPTH_TEST) ? "on" : "off");
   printf("Size: %d pixels\n", Size);

   if (Texture)
      LoadTex(comp, filter);

   glGetIntegerv(GL_RED_BITS, &rBits);
   glGetIntegerv(GL_GREEN_BITS, &gBits);
   glGetIntegerv(GL_BLUE_BITS, &bBits);
   printf("RedBits: %d  GreenBits: %d  BlueBits: %d\n", rBits, gBits, bBits);
}


static void Help( const char *program )
{
   printf("%s options:\n", program);
   printf("  +/-dither      enable/disable dithering\n");
   printf("  +/-depth       enable/disable depth test\n");
   printf("  +flat          flat shading\n");
   printf("  +smooth        smooth shading\n");
   printf("  -size pixels   specify pixels/triangle\n");
   printf("  +/-texture     enable/disable texture\n");
   printf("  -comp n        texture format\n");
   printf("  +/-linear      bilinear texture filter\n");
   printf("  +/-persp       perspective correction hint\n");
}


int main( int argc, char *argv[] )
{
   printf("For options:  %s -help\n", argv[0]);
   glutInit( &argc, argv );
   glutInitWindowSize( (int) Width, (int) Height );
   glutInitWindowPosition( 0, 0 );

   glutInitDisplayMode( GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH );

   glutCreateWindow( argv[0] );

   if (argc==2 && strcmp(argv[1],"-help")==0) {
      Help(argv[0]);
      return 0;
   }

   Init( argc, argv );

   glutReshapeFunc( Reshape );
   glutKeyboardFunc( Key );
   glutDisplayFunc( Display );
   glutIdleFunc( Idle );

   glutMainLoop();
   return 0;
}
