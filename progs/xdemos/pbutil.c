
/*
 * OpenGL pbuffers utility functions.
 *
 * Brian Paul
 * April 1997
 * Updated on 5 October 2002
 */


#include <stdio.h>
#include <string.h>
#include "pbutil.h"



/*
 * Test if we pixel buffers are available for a particular X screen.
 * Input:  dpy - the X display
 *         screen - screen number
 * Return:  0 = pixel buffers not available.
 *          1 = pixel buffers are available.
 */
int
QueryPbuffers(Display *dpy, int screen)
{
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   char *extensions;

   extensions = (char *) glXQueryServerString(dpy, screen, GLX_EXTENSIONS);
   if (!strstr(extensions,"GLX_SGIX_fbconfig")) {
      return 0;
   }
   if (!strstr(extensions,"GLX_SGIX_pbuffer")) {
      return 0;
   }
   return 1;
#else
   return 0;
#endif
}



#ifdef GLX_SGIX_fbconfig


/*
 * Print parameters for a GLXFBConfig to stdout.
 * Input:  dpy - the X display
 *         fbConfig - the fbconfig handle
 *         horizFormat - if true, print in horizontal format
 */
void
PrintFBConfigInfo(Display *dpy, GLXFBConfigSGIX fbConfig, Bool horizFormat)
{
   int pbAttribs[] = {GLX_LARGEST_PBUFFER_SGIX, True,
		      GLX_PRESERVED_CONTENTS_SGIX, False,
		      None};
   GLXPbufferSGIX pBuffer;
   int width=2, height=2;
   int bufferSize, level, doubleBuffer, stereo, auxBuffers;
   int redSize, greenSize, blueSize, alphaSize;
   int depthSize, stencilSize;
   int accumRedSize, accumBlueSize, accumGreenSize, accumAlphaSize;
   int sampleBuffers, samples;
   int drawableType, renderType, xRenderable, xVisual, id;
   int maxWidth, maxHeight, maxPixels;
   int optWidth, optHeight;

   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_BUFFER_SIZE, &bufferSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_LEVEL, &level);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_DOUBLEBUFFER, &doubleBuffer);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_STEREO, &stereo);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_AUX_BUFFERS, &auxBuffers);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_RED_SIZE, &redSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_GREEN_SIZE, &greenSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_BLUE_SIZE, &blueSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_ALPHA_SIZE, &alphaSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_DEPTH_SIZE, &depthSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_STENCIL_SIZE, &stencilSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_ACCUM_RED_SIZE, &accumRedSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_ACCUM_GREEN_SIZE, &accumGreenSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_ACCUM_BLUE_SIZE, &accumBlueSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_ACCUM_ALPHA_SIZE, &accumAlphaSize);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_SAMPLE_BUFFERS_SGIS, &sampleBuffers);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_SAMPLES_SGIS, &samples);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_DRAWABLE_TYPE_SGIX, &drawableType);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_RENDER_TYPE_SGIX, &renderType);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_X_RENDERABLE_SGIX, &xRenderable);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_X_VISUAL_TYPE_EXT, &xVisual);
   if (!xRenderable || !(drawableType & GLX_WINDOW_BIT_SGIX))
      xVisual = -1;
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_FBCONFIG_ID_SGIX, &id);

   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_MAX_PBUFFER_WIDTH_SGIX,
			    &maxWidth);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_MAX_PBUFFER_HEIGHT_SGIX,
			    &maxHeight);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_MAX_PBUFFER_PIXELS_SGIX,
			    &maxPixels);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_OPTIMAL_PBUFFER_WIDTH_SGIX,
			    &optWidth);
   glXGetFBConfigAttribSGIX(dpy, fbConfig, GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX,
			    &optHeight);

   pBuffer = CreatePbuffer(dpy, fbConfig, width, height, pbAttribs);

   if (horizFormat) {
      printf("0x%03x ", id);
      if (xVisual==GLX_STATIC_GRAY)        printf("StaticGray  ");
      else if (xVisual==GLX_GRAY_SCALE)    printf("GrayScale   ");
      else if (xVisual==GLX_STATIC_COLOR)  printf("StaticColor ");
      else if (xVisual==GLX_PSEUDO_COLOR)  printf("PseudoColor ");
      else if (xVisual==GLX_TRUE_COLOR)    printf("TrueColor   ");
      else if (xVisual==GLX_DIRECT_COLOR)  printf("DirectColor ");
      else                            printf("  -none-    ");
      printf(" %3d %3d   %s   %s  %s   %2s   ", bufferSize, level,
	     (renderType & GLX_RGBA_BIT_SGIX) ? "y" : "n",
	     (renderType & GLX_COLOR_INDEX_BIT_SGIX) ? "y" : "n",
	     doubleBuffer ? "y" : "n",
	     stereo ? "y" : "n");
      printf("%2d %2d %2d %2d  ", redSize, greenSize, blueSize, alphaSize);
      printf("%2d %2d  ", depthSize, stencilSize);
      printf("%2d %2d %2d %2d", accumRedSize, accumGreenSize, accumBlueSize,
	     accumAlphaSize);
      printf("    %2d    %2d", sampleBuffers, samples);
      printf("       %s", pBuffer ? "y" : "n");
      printf("\n");
   }
   else {
      printf("Id 0x%x\n", id);
      printf("  Buffer Size: %d\n", bufferSize);
      printf("  Level: %d\n", level);
      printf("  Double Buffer: %s\n", doubleBuffer ? "yes" : "no");
      printf("  Stereo: %s\n", stereo ? "yes" : "no");
      printf("  Aux Buffers: %d\n", auxBuffers);
      printf("  Red Size: %d\n", redSize);
      printf("  Green Size: %d\n", greenSize);
      printf("  Blue Size: %d\n", blueSize);
      printf("  Alpha Size: %d\n", alphaSize);
      printf("  Depth Size: %d\n", depthSize);
      printf("  Stencil Size: %d\n", stencilSize);
      printf("  Accum Red Size: %d\n", accumRedSize);
      printf("  Accum Green Size: %d\n", accumGreenSize);
      printf("  Accum Blue Size: %d\n", accumBlueSize);
      printf("  Accum Alpha Size: %d\n", accumAlphaSize);
      printf("  Sample Buffers: %d\n", sampleBuffers);
      printf("  Samples/Pixel: %d\n", samples);
      printf("  Drawable Types: ");
      if (drawableType & GLX_WINDOW_BIT_SGIX)  printf("Window ");
      if (drawableType & GLX_PIXMAP_BIT_SGIX)  printf("Pixmap ");
      if (drawableType & GLX_PBUFFER_BIT_SGIX)  printf("PBuffer");
      printf("\n");
      printf("  Render Types: ");
      if (renderType & GLX_RGBA_BIT_SGIX)  printf("RGBA ");
      if (renderType & GLX_COLOR_INDEX_BIT_SGIX)  printf("CI ");
      printf("\n");
      printf("  X Renderable: %s\n", xRenderable ? "yes" : "no");
      /*
      printf("  Max width: %d\n", maxWidth);
      printf("  Max height: %d\n", maxHeight);
      printf("  Max pixels: %d\n", maxPixels);
      printf("  Optimum width: %d\n", optWidth);
      printf("  Optimum height: %d\n", optHeight);
      */
      printf("  Pbuffer: %s\n", pBuffer ? "yes" : "no");
   }

   if (pBuffer) {
      glXDestroyGLXPbufferSGIX(dpy, pBuffer);
   }
}



/* This is only used by CreatePbuffer() */
static int XErrorFlag = 0;
static int HandleXError( Display *dpy, XErrorEvent *event )
{
    XErrorFlag = 1;
    return 0;
}


/*
 * Create a Pbuffer.  Use an X error handler to deal with potential
 * BadAlloc errors.
 *
 * Input:  dpy - the X display
 *         fbConfig - an FBConfig as returned by glXChooseFBConfigSGIX().
 *         width, height - size of pixel buffer to request, in pixels.
 *         pbAttribs - list of pixel buffer attributes as used by
 *                     glXCreateGLXPbufferSGIX().
 * Return:  a pixel buffer or None.
 */
GLXPbufferSGIX
CreatePbuffer( Display *dpy, GLXFBConfigSGIX fbConfig,
	       int width, int height, int *pbAttribs )
{
   int (*oldHandler)( Display *, XErrorEvent * );
   GLXPbufferSGIX pBuffer = None;

   /* Catch X protocol errors with our own error handler */
   oldHandler = XSetErrorHandler( HandleXError );

   XErrorFlag = 0;
   pBuffer = glXCreateGLXPbufferSGIX(dpy, fbConfig, width, height, pbAttribs);

   /* Restore original X error handler */
   (void) XSetErrorHandler( oldHandler );

   /* Return pbuffer (may be None) */
   if (!XErrorFlag && pBuffer!=None) {
      /*printf("config %d worked!\n", i);*/
      return pBuffer;
   }
   else {
      return None;
   }
}



#endif  /*GLX_SGIX_fbconfig*/


