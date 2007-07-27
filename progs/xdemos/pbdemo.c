
/*
 * This program demonstrates how to do "off-screen" rendering using
 * the GLX pixel buffer extension.
 *
 * Written by Brian Paul for the "OpenGL and Window System Integration"
 * course presented at SIGGRAPH '97.  Updated on 5 October 2002.
 *
 * Usage:
 *   pbuffers width height imgfile
 * Where:
 *   width is the width, in pixels, of the image to generate.
 *   height is the height, in pixels, of the image to generate.
 *   imgfile is the name of the PPM image file to write.
 *
 *
 * This demo draws 3-D boxes with random orientation.  A pbuffer with
 * a depth (Z) buffer is prefered but if such a pbuffer can't be created
 * we use a non-depth-buffered config.
 *
 * On machines such as the SGI Indigo you may have to reconfigure your
 * display/X server to enable pbuffers.  Look in the /usr/gfx/ucode/MGRAS/vof/
 * directory for display configurationswith the _pbuf suffix.  Use
 * setmon -x <vof> to configure your X server and display for pbuffers.
 *
 * O2 systems seem to support pbuffers well.
 *
 * IR systems (at least 1RM systems) don't have single-buffered, RGBA,
 * Z-buffered pbuffer configs.  BUT, they DO have DOUBLE-buffered, RGBA,
 * Z-buffered pbuffers.  Note how we try four different fbconfig attribute
 * lists below!
 */


#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include "pbutil.h"


/* Some ugly global vars */
static Display *gDpy = NULL;
static int gScreen = 0;
static FBCONFIG gFBconfig = 0;
static PBUFFER gPBuffer = 0;
static int gWidth, gHeight;
static GLXContext glCtx;



/*
 * Create the pbuffer and return a GLXPbuffer handle.
 *
 * We loop over a list of fbconfigs trying to create
 * a pixel buffer.  We return the first pixel buffer which we successfully
 * create.
 */
static PBUFFER
MakePbuffer( Display *dpy, int screen, int width, int height )
{
#define NUM_FB_CONFIGS 4
   const char fbString[NUM_FB_CONFIGS][100] = {
      "Single Buffered, depth buffer",
      "Double Buffered, depth buffer",
      "Single Buffered, no depth buffer",
      "Double Buffered, no depth buffer"
   };
   int fbAttribs[NUM_FB_CONFIGS][100] = {
      {
         /* Single buffered, with depth buffer */
         GLX_RENDER_TYPE, GLX_RGBA_BIT,
         GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
         GLX_RED_SIZE, 1,
         GLX_GREEN_SIZE, 1,
         GLX_BLUE_SIZE, 1,
         GLX_DEPTH_SIZE, 1,
         GLX_DOUBLEBUFFER, 0,
         GLX_STENCIL_SIZE, 0,
         None
      },
      {
         /* Double buffered, with depth buffer */
         GLX_RENDER_TYPE, GLX_RGBA_BIT,
         GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
         GLX_RED_SIZE, 1,
         GLX_GREEN_SIZE, 1,
         GLX_BLUE_SIZE, 1,
         GLX_DEPTH_SIZE, 1,
         GLX_DOUBLEBUFFER, 1,
         GLX_STENCIL_SIZE, 0,
         None
      },
      {
         /* Single buffered, without depth buffer */
         GLX_RENDER_TYPE, GLX_RGBA_BIT,
         GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
         GLX_RED_SIZE, 1,
         GLX_GREEN_SIZE, 1,
         GLX_BLUE_SIZE, 1,
         GLX_DEPTH_SIZE, 0,
         GLX_DOUBLEBUFFER, 0,
         GLX_STENCIL_SIZE, 0,
         None
      },
      {
         /* Double buffered, without depth buffer */
         GLX_RENDER_TYPE, GLX_RGBA_BIT,
         GLX_DRAWABLE_TYPE, GLX_PBUFFER_BIT,
         GLX_RED_SIZE, 1,
         GLX_GREEN_SIZE, 1,
         GLX_BLUE_SIZE, 1,
         GLX_DEPTH_SIZE, 0,
         GLX_DOUBLEBUFFER, 1,
         GLX_STENCIL_SIZE, 0,
         None
      }
   };
   Bool largest = True;
   Bool preserve = False;
   FBCONFIG *fbConfigs;
   PBUFFER pBuffer = None;
   int nConfigs;
   int i;
   int attempt;

   for (attempt=0; attempt<NUM_FB_CONFIGS; attempt++) {

      /* Get list of possible frame buffer configurations */
      fbConfigs = ChooseFBConfig(dpy, screen, fbAttribs[attempt], &nConfigs);
      if (nConfigs==0 || !fbConfigs) {
         printf("Note: glXChooseFBConfig(%s) failed\n", fbString[attempt]);
         continue;
      }

#if 0 /*DEBUG*/
      for (i=0;i<nConfigs;i++) {
         printf("Config %d\n", i);
         PrintFBConfigInfo(dpy, screen, fbConfigs[i], 0);
      }
#endif

      /* Create the pbuffer using first fbConfig in the list that works. */
      for (i=0;i<nConfigs;i++) {
         pBuffer = CreatePbuffer(dpy, screen, fbConfigs[i], width, height, preserve, largest);
         if (pBuffer) {
            gFBconfig = fbConfigs[i];
            gWidth = width;
            gHeight = height;
            break;
         }
      }

      if (pBuffer!=None) {
         break;
      }
   }

   if (pBuffer) {
      printf("Using: %s\n", fbString[attempt]);
   }

   XFree(fbConfigs);

   return pBuffer;
#undef NUM_FB_CONFIGS
}



/*
 * Do all the X / GLX setup stuff.
 */
static int
Setup(int width, int height)
{
   int pbSupport;
   XVisualInfo *visInfo;

   /* Open the X display */
   gDpy = XOpenDisplay(NULL);
   if (!gDpy) {
      printf("Error: couldn't open default X display.\n");
      return 0;
   }

   /* Get default screen */
   gScreen = DefaultScreen(gDpy);

   /* Test that pbuffers are available */
   pbSupport = QueryPbuffers(gDpy, gScreen);
   if (pbSupport == 1) {
      printf("Using GLX 1.3 Pbuffers\n");
   }
   else if (pbSupport == 2) {
      printf("Using SGIX Pbuffers\n");
   }
   else {
      printf("Error: pbuffers not available on this screen\n");
      XCloseDisplay(gDpy);
      return 0;
   }

   /* Create Pbuffer */
   gPBuffer = MakePbuffer( gDpy, gScreen, width, height );
   if (gPBuffer==None) {
      printf("Error: couldn't create pbuffer\n");
      XCloseDisplay(gDpy);
      return 0;
   }

   /* Get corresponding XVisualInfo */
   visInfo = GetVisualFromFBConfig(gDpy, gScreen, gFBconfig);
   if (!visInfo) {
      printf("Error: can't get XVisualInfo from FBconfig\n");
      XCloseDisplay(gDpy);
      return 0;
   }

   /* Create GLX context */
   glCtx = glXCreateContext(gDpy, visInfo, NULL, True);
   if (!glCtx) {
      /* try indirect */
      glCtx = glXCreateContext(gDpy, visInfo, NULL, False);
      if (!glCtx) {
         printf("Error: Couldn't create GLXContext\n");
         XFree(visInfo);
         XCloseDisplay(gDpy);
         return 0;
      }
      else {
         printf("Warning: using indirect GLXContext\n");
      }
   }

   /* Bind context to pbuffer */
   if (!glXMakeCurrent(gDpy, gPBuffer, glCtx)) {
      printf("Error: glXMakeCurrent failed\n");
      XFree(visInfo);
      XCloseDisplay(gDpy);
      return 0;
   }

   return 1;  /* Success!! */
}



/* One-time GL setup */
static void
InitGL(void)
{
   static GLfloat pos[4] = {0.0, 0.0, 10.0, 0.0};
   glEnable(GL_LIGHTING);
   glEnable(GL_LIGHT0);
   glLightfv(GL_LIGHT0, GL_POSITION, pos);
   glEnable(GL_NORMALIZE);
   glEnable(GL_DEPTH_TEST);
   glEnable(GL_CULL_FACE);

   glViewport(0, 0, gWidth, gHeight);
   glMatrixMode( GL_PROJECTION );
   glLoadIdentity();
   glFrustum( -1.0, 1.0, -1.0, 1.0, 5.0, 25.0 );
   glMatrixMode( GL_MODELVIEW );
   glLoadIdentity();
   glTranslatef( 0.0, 0.0, -15.0 );
}


/* Return random float in [0,1] */
static float
Random(void)
{
   int i = rand();
   return (float) (i % 1000) / 1000.0;
}


static void
RandomColor(void)
{
   GLfloat c[4];
   c[0] = Random();
   c[1] = Random();
   c[2] = Random();
   c[3] = 1.0;
   glMaterialfv(GL_FRONT, GL_AMBIENT_AND_DIFFUSE, c);
}


/* This function borrowed from Mark Kilgard's GLUT */
static void
drawBox(GLfloat x0, GLfloat x1, GLfloat y0, GLfloat y1,
  GLfloat z0, GLfloat z1, GLenum type)
{
  static GLfloat n[6][3] =
  {
    {-1.0, 0.0, 0.0},
    {0.0, 1.0, 0.0},
    {1.0, 0.0, 0.0},
    {0.0, -1.0, 0.0},
    {0.0, 0.0, 1.0},
    {0.0, 0.0, -1.0}
  };
  static GLint faces[6][4] =
  {
    {0, 1, 2, 3},
    {3, 2, 6, 7},
    {7, 6, 5, 4},
    {4, 5, 1, 0},
    {5, 6, 2, 1},
    {7, 4, 0, 3}
  };
  GLfloat v[8][3], tmp;
  GLint i;

  if (x0 > x1) {
    tmp = x0;
    x0 = x1;
    x1 = tmp;
  }
  if (y0 > y1) {
    tmp = y0;
    y0 = y1;
    y1 = tmp;
  }
  if (z0 > z1) {
    tmp = z0;
    z0 = z1;
    z1 = tmp;
  }
  v[0][0] = v[1][0] = v[2][0] = v[3][0] = x0;
  v[4][0] = v[5][0] = v[6][0] = v[7][0] = x1;
  v[0][1] = v[1][1] = v[4][1] = v[5][1] = y0;
  v[2][1] = v[3][1] = v[6][1] = v[7][1] = y1;
  v[0][2] = v[3][2] = v[4][2] = v[7][2] = z0;
  v[1][2] = v[2][2] = v[5][2] = v[6][2] = z1;

  for (i = 0; i < 6; i++) {
    glBegin(type);
    glNormal3fv(&n[i][0]);
    glVertex3fv(&v[faces[i][0]][0]);
    glVertex3fv(&v[faces[i][1]][0]);
    glVertex3fv(&v[faces[i][2]][0]);
    glVertex3fv(&v[faces[i][3]][0]);
    glEnd();
  }
}



/* Render a scene */
static void
Render(void)
{
   int NumBoxes = 100;
   int i;

   glClearColor(0.2, 0.2, 0.9, 0.0);
   glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

   for (i=0;i<NumBoxes;i++) {
      float tx = -2.0 + 4.0 * Random();
      float ty = -2.0 + 4.0 * Random();
      float tz =  4.0 - 16.0 * Random();
      float sx = 0.1 + Random() * 0.4;
      float sy = 0.1 + Random() * 0.4;
      float sz = 0.1 + Random() * 0.4;
      float rx = Random();
      float ry = Random();
      float rz = Random();
      float ra = Random() * 360.0;
      glPushMatrix();
      glTranslatef(tx, ty, tz);
      glRotatef(ra, rx, ry, rz);
      glScalef(sx, sy, sz);
      RandomColor();
      drawBox(-1.0, 1.0, -1.0, 1.0, -1.0, 1.0, GL_POLYGON);
      glPopMatrix();
   }

   glFinish();
}



static void
WriteFile(const char *filename)
{
   FILE *f;
   GLubyte *image;
   int i;

   image = malloc(gWidth * gHeight * 3 * sizeof(GLubyte));
   if (!image) {
      printf("Error: couldn't allocate image buffer\n");
      return;
   }

   glPixelStorei(GL_PACK_ALIGNMENT, 1);
   glReadPixels(0, 0, gWidth, gHeight, GL_RGB, GL_UNSIGNED_BYTE, image);

   f = fopen(filename, "w");
   if (!f) {
      printf("Couldn't open image file: %s\n", filename);
      return;
   }
   fprintf(f,"P6\n");
   fprintf(f,"# ppm-file created by %s\n", "trdemo2");
   fprintf(f,"%i %i\n", gWidth, gHeight);
   fprintf(f,"255\n");
   fclose(f);
   f = fopen(filename, "ab");  /* now append binary data */
   if (!f) {
      printf("Couldn't append to image file: %s\n", filename);
      return;
   }

   for (i=0;i<gHeight;i++) {
      GLubyte *rowPtr;
      /* Remember, OpenGL images are bottom to top.  Have to reverse. */
      rowPtr = image + (gHeight-1-i) * gWidth*3;
      fwrite(rowPtr, 1, gWidth*3, f);
   }

   fclose(f);
   free(image);

   printf("Wrote %d by %d image file: %s\n", gWidth, gHeight, filename);
}



/*
 * Print message describing command line parameters.
 */
static void
Usage(const char *appName)
{
   printf("Usage:\n");
   printf("  %s width height imgfile\n", appName);
   printf("Where imgfile is a ppm file\n");
}



int
main(int argc, char *argv[])
{
   if (argc!=4) {
      Usage(argv[0]);
   }
   else {
      int width = atoi(argv[1]);
      int height = atoi(argv[2]);
      char *fileName = argv[3];
      if (width<=0) {
         printf("Error: width parameter must be at least 1.\n");
         return 1;
      }
      if (height<=0) {
         printf("Error: height parameter must be at least 1.\n");
         return 1;
      }
      if (!Setup(width, height)) {
         return 1;
      }
      InitGL();
      Render();
      WriteFile(fileName);
      DestroyPbuffer(gDpy, gScreen, gPBuffer);
   }
   return 0;
}

