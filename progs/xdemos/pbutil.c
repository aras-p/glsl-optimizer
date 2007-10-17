
/*
 * OpenGL pbuffers utility functions.
 *
 * Brian Paul
 * Original code: April 1997
 * Updated on 5 October 2002
 * Updated again on 3 January 2005 to use GLX 1.3 functions in preference
 * to the GLX_SGIX_fbconfig/pbuffer extensions.
 */

#include <stdio.h>
#include <string.h>
#include "pbutil.h"


/**
 * Test if we pixel buffers are available for a particular X screen.
 * Input:  dpy - the X display
 *         screen - screen number
 * Return:  0 = fbconfigs not available.
 *          1 = fbconfigs are available via GLX 1.3.
 *          2 = fbconfigs and pbuffers are available via GLX_SGIX_fbconfig
 */
int
QueryFBConfig(Display *dpy, int screen)
{
#if defined(GLX_VERSION_1_3)
   {
      /* GLX 1.3 supports pbuffers */
      int glxVersionMajor, glxVersionMinor;
      if (!glXQueryVersion(dpy, &glxVersionMajor, &glxVersionMinor)) {
         /* GLX not available! */
         return 0;
      }
      if (glxVersionMajor * 100 + glxVersionMinor >= 103) {
         return 1;
      }
      /* fall-through */
   }
#endif

   /* Try the SGIX extensions */
   {
      char *extensions;
      extensions = (char *) glXQueryServerString(dpy, screen, GLX_EXTENSIONS);
      if (extensions && strstr(extensions,"GLX_SGIX_fbconfig")) {
	 return 2;
      }
   }

   return 0;
}

/**
 * Test if we pixel buffers are available for a particular X screen.
 * Input:  dpy - the X display
 *         screen - screen number
 * Return:  0 = pixel buffers not available.
 *          1 = pixel buffers are available via GLX 1.3.
 *          2 = pixel buffers are available via GLX_SGIX_fbconfig/pbuffer.
 */
int
QueryPbuffers(Display *dpy, int screen)
{
   int ret;

   ret = QueryFBConfig(dpy, screen);
   if (ret == 2) {
      char *extensions;
      extensions = (char *) glXQueryServerString(dpy, screen, GLX_EXTENSIONS);
      if (extensions && strstr(extensions, "GLX_SGIX_pbuffer"))
	 return 2;
      else
	 return 0;
   }
   else
      return ret;
}

FBCONFIG *
ChooseFBConfig(Display *dpy, int screen, const int attribs[], int *nConfigs)
{
   int fbcSupport = QueryPbuffers(dpy, screen);
#if defined(GLX_VERSION_1_3)
   if (fbcSupport == 1) {
      return glXChooseFBConfig(dpy, screen, attribs, nConfigs);
   }
#endif
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (fbcSupport == 2) {
      return glXChooseFBConfigSGIX(dpy, screen, (int *) attribs, nConfigs);
   }
#endif
   return NULL;
}


FBCONFIG *
GetAllFBConfigs(Display *dpy, int screen, int *nConfigs)
{
   int fbcSupport = QueryFBConfig(dpy, screen);
#if defined(GLX_VERSION_1_3)
   if (fbcSupport == 1) {
      return glXGetFBConfigs(dpy, screen, nConfigs);
   }
#endif
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (fbcSupport == 2) {
      /* The GLX_SGIX_fbconfig extensions says to pass NULL to get list
       * of all available configurations.
       */
      return glXChooseFBConfigSGIX(dpy, screen, NULL, nConfigs);
   }
#endif
   return NULL;
}


XVisualInfo *
GetVisualFromFBConfig(Display *dpy, int screen, FBCONFIG config)
{
   int fbcSupport = QueryFBConfig(dpy, screen);
#if defined(GLX_VERSION_1_3)
   if (fbcSupport == 1) {
      return glXGetVisualFromFBConfig(dpy, config);
   }
#endif
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (fbcSupport == 2) {
      return glXGetVisualFromFBConfigSGIX(dpy, config);
   }
#endif
   return NULL;
}


/**
 * Either use glXGetFBConfigAttrib() or glXGetFBConfigAttribSGIX()
 * to query an fbconfig attribute.
 */
static int
GetFBConfigAttrib(Display *dpy, int screen,
#if defined(GLX_VERSION_1_3)
                  const GLXFBConfig config,
#elif defined(GLX_SGIX_fbconfig)
                  const GLXFBConfigSGIX config,
#endif
                  int attrib
                  )
{
   int fbcSupport = QueryFBConfig(dpy, screen);
   int value = 0;

#if defined(GLX_VERSION_1_3)
   if (fbcSupport == 1) {
      /* ok */
      if (glXGetFBConfigAttrib(dpy, config, attrib, &value) != 0) {
         value = 0;
      }
      return value;
   }
   /* fall-through */
#endif

#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (fbcSupport == 2) {
      if (glXGetFBConfigAttribSGIX(dpy, config, attrib, &value) != 0) {
         value = 0;
      }
      return value;
   }
#endif
   
   return value;
}



/**
 * Print parameters for a GLXFBConfig to stdout.
 * Input:  dpy - the X display
 *         screen - the X screen number
 *         fbConfig - the fbconfig handle
 *         horizFormat - if true, print in horizontal format
 */
void
PrintFBConfigInfo(Display *dpy, int screen, FBCONFIG config, Bool horizFormat)
{
   PBUFFER pBuffer;
   int width=2, height=2;
   int bufferSize, level, doubleBuffer, stereo, auxBuffers;
   int redSize, greenSize, blueSize, alphaSize;
   int depthSize, stencilSize;
   int accumRedSize, accumBlueSize, accumGreenSize, accumAlphaSize;
   int sampleBuffers, samples;
   int drawableType, renderType, xRenderable, xVisual, id;
   int maxWidth, maxHeight, maxPixels;
   int optWidth, optHeight;
   int floatComponents = 0;

   /* do queries using the GLX 1.3 tokens (same as the SGIX tokens) */
   bufferSize     = GetFBConfigAttrib(dpy, screen, config, GLX_BUFFER_SIZE);
   level          = GetFBConfigAttrib(dpy, screen, config, GLX_LEVEL);
   doubleBuffer   = GetFBConfigAttrib(dpy, screen, config, GLX_DOUBLEBUFFER);
   stereo         = GetFBConfigAttrib(dpy, screen, config, GLX_STEREO);
   auxBuffers     = GetFBConfigAttrib(dpy, screen, config, GLX_AUX_BUFFERS);
   redSize        = GetFBConfigAttrib(dpy, screen, config, GLX_RED_SIZE);
   greenSize      = GetFBConfigAttrib(dpy, screen, config, GLX_GREEN_SIZE);
   blueSize       = GetFBConfigAttrib(dpy, screen, config, GLX_BLUE_SIZE);
   alphaSize      = GetFBConfigAttrib(dpy, screen, config, GLX_ALPHA_SIZE);
   depthSize      = GetFBConfigAttrib(dpy, screen, config, GLX_DEPTH_SIZE);
   stencilSize    = GetFBConfigAttrib(dpy, screen, config, GLX_STENCIL_SIZE);
   accumRedSize   = GetFBConfigAttrib(dpy, screen, config, GLX_ACCUM_RED_SIZE);
   accumGreenSize = GetFBConfigAttrib(dpy, screen, config, GLX_ACCUM_GREEN_SIZE);
   accumBlueSize  = GetFBConfigAttrib(dpy, screen, config, GLX_ACCUM_BLUE_SIZE);
   accumAlphaSize = GetFBConfigAttrib(dpy, screen, config, GLX_ACCUM_ALPHA_SIZE);
   sampleBuffers  = GetFBConfigAttrib(dpy, screen, config, GLX_SAMPLE_BUFFERS);
   samples        = GetFBConfigAttrib(dpy, screen, config, GLX_SAMPLES);
   drawableType   = GetFBConfigAttrib(dpy, screen, config, GLX_DRAWABLE_TYPE);
   renderType     = GetFBConfigAttrib(dpy, screen, config, GLX_RENDER_TYPE);
   xRenderable    = GetFBConfigAttrib(dpy, screen, config, GLX_X_RENDERABLE);
   xVisual        = GetFBConfigAttrib(dpy, screen, config, GLX_X_VISUAL_TYPE);
   if (!xRenderable || !(drawableType & GLX_WINDOW_BIT_SGIX))
      xVisual = -1;

   id        = GetFBConfigAttrib(dpy, screen, config, GLX_FBCONFIG_ID);
   maxWidth  = GetFBConfigAttrib(dpy, screen, config, GLX_MAX_PBUFFER_WIDTH);
   maxHeight = GetFBConfigAttrib(dpy, screen, config, GLX_MAX_PBUFFER_HEIGHT);
   maxPixels = GetFBConfigAttrib(dpy, screen, config, GLX_MAX_PBUFFER_PIXELS);
#if defined(GLX_SGIX_pbuffer)
   optWidth  = GetFBConfigAttrib(dpy, screen, config, GLX_OPTIMAL_PBUFFER_WIDTH_SGIX);
   optHeight = GetFBConfigAttrib(dpy, screen, config, GLX_OPTIMAL_PBUFFER_HEIGHT_SGIX);
#else
   optWidth = optHeight = 0;
#endif
#if defined(GLX_NV_float_buffer)
   floatComponents = GetFBConfigAttrib(dpy, screen, config, GLX_FLOAT_COMPONENTS_NV);
#endif

   /* See if we can create a pbuffer with this config */
   pBuffer = CreatePbuffer(dpy, screen, config, width, height, False, False);

   if (horizFormat) {
      printf("0x%-9x ", id);
      if (xVisual==GLX_STATIC_GRAY)        printf("StaticGray  ");
      else if (xVisual==GLX_GRAY_SCALE)    printf("GrayScale   ");
      else if (xVisual==GLX_STATIC_COLOR)  printf("StaticColor ");
      else if (xVisual==GLX_PSEUDO_COLOR)  printf("PseudoColor ");
      else if (xVisual==GLX_TRUE_COLOR)    printf("TrueColor   ");
      else if (xVisual==GLX_DIRECT_COLOR)  printf("DirectColor ");
      else                            printf("  -none-    ");
      printf(" %3d %3d   %s   %s  %s   %2s   ", bufferSize, level,
	     (renderType & GLX_RGBA_BIT_SGIX) ? "y" : ".",
	     (renderType & GLX_COLOR_INDEX_BIT_SGIX) ? "y" : ".",
	     doubleBuffer ? "y" : ".",
	     stereo ? "y" : ".");
      printf("%2d %2d %2d %2d  ", redSize, greenSize, blueSize, alphaSize);
      printf("%2d %2d  ", depthSize, stencilSize);
      printf("%2d %2d %2d %2d", accumRedSize, accumGreenSize, accumBlueSize,
	     accumAlphaSize);
      printf("    %2d    %2d", sampleBuffers, samples);
      printf("       %s       %c", pBuffer ? "y" : ".",
             ".y"[floatComponents]);
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
      if (drawableType & GLX_WINDOW_BIT)  printf("Window ");
      if (drawableType & GLX_PIXMAP_BIT)  printf("Pixmap ");
      if (drawableType & GLX_PBUFFER_BIT)  printf("PBuffer");
      printf("\n");
      printf("  Render Types: ");
      if (renderType & GLX_RGBA_BIT_SGIX)  printf("RGBA ");
      if (renderType & GLX_COLOR_INDEX_BIT_SGIX)  printf("CI ");
      printf("\n");
      printf("  X Renderable: %s\n", xRenderable ? "yes" : "no");

      printf("  Pbuffer: %s\n", pBuffer ? "yes" : "no");
      printf("  Max Pbuffer width: %d\n", maxWidth);
      printf("  Max Pbuffer height: %d\n", maxHeight);
      printf("  Max Pbuffer pixels: %d\n", maxPixels);
      printf("  Optimum Pbuffer width: %d\n", optWidth);
      printf("  Optimum Pbuffer height: %d\n", optHeight);

      printf("  Float Components: %s\n", floatComponents ? "yes" : "no");
   }

   if (pBuffer) {
      DestroyPbuffer(dpy, screen, pBuffer);
   }
}



GLXContext
CreateContext(Display *dpy, int screen, FBCONFIG config)
{
   int fbcSupport = QueryFBConfig(dpy, screen);
#if defined(GLX_VERSION_1_3)
   if (fbcSupport == 1) {
      /* GLX 1.3 */
      GLXContext c;
      c = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, True);
      if (!c) {
         /* try indirect */
         c = glXCreateNewContext(dpy, config, GLX_RGBA_TYPE, NULL, False);
      }
      return c;
   }
#endif
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (fbcSupport == 2) {
      GLXContext c;
      c = glXCreateContextWithConfigSGIX(dpy, config, GLX_RGBA_TYPE_SGIX, NULL, True);
      if (!c) {
         c = glXCreateContextWithConfigSGIX(dpy, config, GLX_RGBA_TYPE_SGIX, NULL, False);
      }
      return c;
   }
#endif
   return 0;
}


void
DestroyContext(Display *dpy, GLXContext ctx)
{
   glXDestroyContext(dpy, ctx);
}


/* This is only used by CreatePbuffer() */
static int XErrorFlag = 0;
static int HandleXError(Display *dpy, XErrorEvent *event)
{
    XErrorFlag = 1;
    return 0;
}


/**
 * Create a Pbuffer.  Use an X error handler to deal with potential
 * BadAlloc errors.
 *
 * Input:  dpy - the X display
 *         fbConfig - an FBConfig as returned by glXChooseFBConfigSGIX().
 *         width, height - size of pixel buffer to request, in pixels.
 *         pbAttribs - list of optional pixel buffer attributes
 * Return:  a Pbuffer or None.
 */
PBUFFER
CreatePbuffer(Display *dpy, int screen, FBCONFIG config,
              int width, int height, Bool largest, Bool preserve)
{
   int (*oldHandler)(Display *, XErrorEvent *);
   PBUFFER pBuffer = None;
   int pbSupport = QueryPbuffers(dpy, screen);

   /* Catch X protocol errors with our own error handler */
   oldHandler = XSetErrorHandler(HandleXError);
   XErrorFlag = 0;

#if defined(GLX_VERSION_1_3)
   if (pbSupport == 1) {
      /* GLX 1.3 */
      int attribs[100], i = 0;
      attribs[i++] = GLX_PBUFFER_WIDTH;
      attribs[i++] = width;
      attribs[i++] = GLX_PBUFFER_HEIGHT;
      attribs[i++] = height;
      attribs[i++] = GLX_PRESERVED_CONTENTS;
      attribs[i++] = preserve;
      attribs[i++] = GLX_LARGEST_PBUFFER;
      attribs[i++] = largest;
      attribs[i++] = 0;
      pBuffer = glXCreatePbuffer(dpy, config, attribs);
   }
   else
#endif
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (pbSupport == 2) {
      int attribs[100], i = 0;
      attribs[i++] = GLX_PRESERVED_CONTENTS;
      attribs[i++] = preserve;
      attribs[i++] = GLX_LARGEST_PBUFFER;
      attribs[i++] = largest;
      attribs[i++] = 0;
      pBuffer = glXCreateGLXPbufferSGIX(dpy, config, width, height, attribs);
   }
   else
#endif
   {
      pBuffer = None;
   }

   XSync(dpy, False);
   /* Restore original X error handler */
   (void) XSetErrorHandler(oldHandler);

   /* Return pbuffer (may be None) */
   if (!XErrorFlag && pBuffer != None) {
      /*printf("config %d worked!\n", i);*/
      return pBuffer;
   }
   else {
      return None;
   }
}


void
DestroyPbuffer(Display *dpy, int screen, PBUFFER pbuffer)
{
   int pbSupport = QueryPbuffers(dpy, screen);
#if defined(GLX_VERSION_1_3)
   if (pbSupport == 1) {
      glXDestroyPbuffer(dpy, pbuffer);
      return;
   }
#endif
#if defined(GLX_SGIX_fbconfig) && defined(GLX_SGIX_pbuffer)
   if (pbSupport == 2) {
      glXDestroyGLXPbufferSGIX(dpy, pbuffer);
      return;
   }
#endif
}
