
/*
 * This program demonstrates how to do "off-screen" rendering using
 * the GLX pixel buffer extension.
 *
 * Written by Brian Paul for the "OpenGL and Window System Integration"
 * course presented at SIGGRAPH '97.  Updated on 5 October 2002.
 *
 * Updated on 31 January 2004 to use native GLX by
 * Andrew P. Lentvorski, Jr. <bsder@allcaps.org>
 *
 * Usage:
 *   glxpbdemo width height imgfile
 * Where:
 *   width is the width, in pixels, of the image to generate.
 *   height is the height, in pixels, of the image to generate.
 *   imgfile is the name of the PPM image file to write.
 *
 *
 * This demo draws 3-D boxes with random orientation.
 *
 * On machines such as the SGI Indigo you may have to reconfigure your
 * display/X server to enable pbuffers.  Look in the /usr/gfx/ucode/MGRAS/vof/
 * directory for display configurations with the _pbuf suffix.  Use
 * setmon -x <vof> to configure your X server and display for pbuffers.
 *
 * O2 systems seem to support pbuffers well.
 *
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xlib.h>
#include <GL/glx.h>

/* Some ugly global vars */
static GLXFBConfig gFBconfig = 0;
static Display *gDpy = NULL;
static int gScreen = 0;
static GLXPbuffer gPBuffer = 0;
static int gWidth, gHeight;


/*
 * Test for appropriate version of GLX to run this program
 * Input:  dpy - the X display
 *         screen - screen number
 * Return:  0 = GLX not available.
 *          1 = GLX available.
 */
static int
RuntimeQueryGLXVersion(Display *dpy, int screen)
{
#if defined(GLX_VERSION_1_3) || defined(GLX_VERSION_1_4)
   char *glxversion;
 
   glxversion = (char *) glXGetClientString(dpy, GLX_VERSION);
   if (!(strstr(glxversion, "1.3") || strstr(glxversion, "1.4")))
      return 0;

   glxversion = (char *) glXQueryServerString(dpy, screen, GLX_VERSION);
   if (!(strstr(glxversion, "1.3") || strstr(glxversion, "1.4")))
      return 0;

   return 1;
#else
   return 0;
#endif
}



/*
 * Create the pbuffer and return a GLXPbuffer handle.
 */
static GLXPbuffer
MakePbuffer( Display *dpy, int screen, int width, int height )
{
   GLXFBConfig *fbConfigs;
   GLXFBConfig chosenFBConfig;
   GLXPbuffer pBuffer = None;

   int nConfigs;
   int fbconfigid;

   int fbAttribs[] = {
      GLX_RENDER_TYPE, GLX_RGBA_BIT,
      GLX_DEPTH_SIZE, 1,
      GLX_DRAWABLE_TYPE, GLX_PIXMAP_BIT | GLX_PBUFFER_BIT,
      None
   };

   int pbAttribs[] = {
      GLX_PBUFFER_WIDTH, 0,
      GLX_PBUFFER_HEIGHT, 0,
      GLX_LARGEST_PBUFFER, False,
      GLX_PRESERVED_CONTENTS, False,
      None
   };

   pbAttribs[1] = width;
   pbAttribs[3] = height;

   fbConfigs = glXChooseFBConfig(dpy, screen, fbAttribs, &nConfigs);

   if (0 == nConfigs || !fbConfigs) {
      printf("Error: glxChooseFBConfig failed\n");
      XCloseDisplay(dpy);
      return 0;
   }

   chosenFBConfig = fbConfigs[0];

   glXGetFBConfigAttrib(dpy, chosenFBConfig, GLX_FBCONFIG_ID, &fbconfigid);
   printf("Chose 0x%x as fbconfigid\n", fbconfigid);

   /* Create the pbuffer using first fbConfig in the list that works. */
   pBuffer = glXCreatePbuffer(dpy, chosenFBConfig, pbAttribs);

   if (pBuffer) {
      gFBconfig = chosenFBConfig;
      gWidth = width;
      gHeight = height;
   }

   XFree(fbConfigs);

   return pBuffer;
}



/*
 * Do all the X / GLX setup stuff.
 */
static int
Setup(int width, int height)
{
#if defined(GLX_VERSION_1_3) || defined(GLX_VERSION_1_4)
   GLXContext glCtx;

   /* Open the X display */
   gDpy = XOpenDisplay(NULL);
   if (!gDpy) {
      printf("Error: couldn't open default X display.\n");
      return 0;
   }

   /* Get default screen */
   gScreen = DefaultScreen(gDpy);

   /* Test that GLX is available */
   if (!RuntimeQueryGLXVersion(gDpy, gScreen)) {
     printf("Error: GLX 1.3 or 1.4 not available\n");
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

   /* Create GLX context */
   glCtx = glXCreateNewContext(gDpy, gFBconfig, GLX_RGBA_TYPE, NULL, True);
   if (glCtx) {
      if (!glXIsDirect(gDpy, glCtx)) {
         printf("Warning: using indirect GLXContext\n");
      }
   }
   else {
      printf("Error: Couldn't create GLXContext\n");
      XCloseDisplay(gDpy);
      return 0;
   }

   /* Bind context to pbuffer */
   if (!glXMakeCurrent(gDpy, gPBuffer, glCtx)) {
      printf("Error: glXMakeCurrent failed\n");
      XCloseDisplay(gDpy);
      return 0;
   }

   return 1;  /* Success!! */
#else
   printf("Error: GLX version 1.3 or 1.4 not available at compile time\n");
   return 0;
#endif
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

   InitGL();
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

      printf("Setup completed\n");
      Render();
      printf("Render completed.\n");
      WriteFile(fileName);
      printf("File write completed.\n");

      glXDestroyPbuffer( gDpy, gPBuffer );
   }
   return 0;
}

