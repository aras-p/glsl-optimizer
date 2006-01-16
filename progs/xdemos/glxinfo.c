/*
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */


/*
 * This program is a work-alike of the IRIX glxinfo program.
 * Command line options:
 *  -t                     print wide table
 *  -v                     print verbose information
 *  -display DisplayName   specify the X display to interogate
 *  -b                     only print ID of "best" visual on screen 0
 *  -i                     use indirect rendering connection only
 *  -l                     print interesting OpenGL limits (added 5 Sep 2002)
 *
 * Brian Paul  26 January 2000
 */

#define DO_GLU  /* may want to remove this for easier XFree86 building? */

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#ifdef DO_GLU
#include <GL/glu.h>
#endif
#include <GL/glx.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#ifndef GLX_NONE_EXT
#define GLX_NONE_EXT  0x8000
#endif

#ifndef GLX_TRANSPARENT_RGB
#define GLX_TRANSPARENT_RGB 0x8008
#endif

typedef enum
{
   Normal,
   Wide,
   Verbose
} InfoMode;


struct visual_attribs
{
   /* X visual attribs */
   int id;
   int klass;
   int depth;
   int redMask, greenMask, blueMask;
   int colormapSize;
   int bitsPerRGB;

   /* GL visual attribs */
   int supportsGL;
   int transparentType;
   int transparentRedValue;
   int transparentGreenValue;
   int transparentBlueValue;
   int transparentAlphaValue;
   int transparentIndexValue;
   int bufferSize;
   int level;
   int rgba;
   int doubleBuffer;
   int stereo;
   int auxBuffers;
   int redSize, greenSize, blueSize, alphaSize;
   int depthSize;
   int stencilSize;
   int accumRedSize, accumGreenSize, accumBlueSize, accumAlphaSize;
   int numSamples, numMultisample;
   int visualCaveat;
};

   
/*
 * Print a list of extensions, with word-wrapping.
 */
static void
print_extension_list(const char *ext)
{
   const char *indentString = "    ";
   const int indent = 4;
   const int max = 79;
   int width, i, j;

   if (!ext || !ext[0])
      return;

   width = indent;
   printf(indentString);
   i = j = 0;
   while (1) {
      if (ext[j] == ' ' || ext[j] == 0) {
         /* found end of an extension name */
         const int len = j - i;
         if (width + len > max) {
            /* start a new line */
            printf("\n");
            width = indent;
            printf(indentString);
         }
         /* print the extension name between ext[i] and ext[j] */
         while (i < j) {
            printf("%c", ext[i]);
            i++;
         }
         /* either we're all done, or we'll continue with next extension */
         width += len + 1;
         if (ext[j] == 0) {
            break;
         }
         else {
            i++;
            j++;
            if (ext[j] == 0)
               break;
            printf(", ");
            width += 2;
         }
      }
      j++;
   }
   printf("\n");
}


static void
print_display_info(Display *dpy)
{
   printf("name of display: %s\n", DisplayString(dpy));
}


static void
print_limits(void)
{
   struct token_name {
      GLuint count;
      GLenum token;
      const char *name;
   };
   static const struct token_name limits[] = {
      { 1, GL_MAX_ATTRIB_STACK_DEPTH, "GL_MAX_ATTRIB_STACK_DEPTH" },
      { 1, GL_MAX_CLIENT_ATTRIB_STACK_DEPTH, "GL_MAX_CLIENT_ATTRIB_STACK_DEPTH" },
      { 1, GL_MAX_CLIP_PLANES, "GL_MAX_CLIP_PLANES" },
      { 1, GL_MAX_COLOR_MATRIX_STACK_DEPTH, "GL_MAX_COLOR_MATRIX_STACK_DEPTH" },
      { 1, GL_MAX_ELEMENTS_VERTICES, "GL_MAX_ELEMENTS_VERTICES" },
      { 1, GL_MAX_ELEMENTS_INDICES, "GL_MAX_ELEMENTS_INDICES" },
      { 1, GL_MAX_EVAL_ORDER, "GL_MAX_EVAL_ORDER" },
      { 1, GL_MAX_LIGHTS, "GL_MAX_LIGHTS" },
      { 1, GL_MAX_LIST_NESTING, "GL_MAX_LIST_NESTING" },
      { 1, GL_MAX_MODELVIEW_STACK_DEPTH, "GL_MAX_MODELVIEW_STACK_DEPTH" },
      { 1, GL_MAX_NAME_STACK_DEPTH, "GL_MAX_NAME_STACK_DEPTH" },
      { 1, GL_MAX_PIXEL_MAP_TABLE, "GL_MAX_PIXEL_MAP_TABLE" },
      { 1, GL_MAX_PROJECTION_STACK_DEPTH, "GL_MAX_PROJECTION_STACK_DEPTH" },
      { 1, GL_MAX_TEXTURE_STACK_DEPTH, "GL_MAX_TEXTURE_STACK_DEPTH" },
      { 1, GL_MAX_TEXTURE_SIZE, "GL_MAX_TEXTURE_SIZE" },
      { 1, GL_MAX_3D_TEXTURE_SIZE, "GL_MAX_3D_TEXTURE_SIZE" },
      { 1, GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB, "GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB" },
      { 1, GL_MAX_RECTANGLE_TEXTURE_SIZE_NV, "GL_MAX_RECTANGLE_TEXTURE_SIZE_NV" },
      { 1, GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB, "GL_NUM_COMPRESSED_TEXTURE_FORMATS_ARB" },
      { 1, GL_MAX_TEXTURE_UNITS_ARB, "GL_MAX_TEXTURE_UNITS_ARB" },
      { 1, GL_MAX_TEXTURE_LOD_BIAS_EXT, "GL_MAX_TEXTURE_LOD_BIAS_EXT" },
      { 1, GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT, "GL_MAX_TEXTURE_MAX_ANISOTROPY_EXT" },
      { 2, GL_MAX_VIEWPORT_DIMS, "GL_MAX_VIEWPORT_DIMS" },
      { 2, GL_ALIASED_LINE_WIDTH_RANGE, "GL_ALIASED_LINE_WIDTH_RANGE" },
      { 2, GL_SMOOTH_LINE_WIDTH_RANGE, "GL_SMOOTH_LINE_WIDTH_RANGE" },
      { 2, GL_ALIASED_POINT_SIZE_RANGE, "GL_ALIASED_POINT_SIZE_RANGE" },
      { 2, GL_SMOOTH_POINT_SIZE_RANGE, "GL_SMOOTH_POINT_SIZE_RANGE" },
      { 0, (GLenum) 0, NULL }
   };
   GLint i, max[2];
   printf("OpenGL limits:\n");
   for (i = 0; limits[i].count; i++) {
      glGetIntegerv(limits[i].token, max);
      if (glGetError() == GL_NONE) {
         if (limits[i].count == 1)
            printf("    %s = %d\n", limits[i].name, max[0]);
         else /* XXX fix if we ever query something with more than 2 values */
            printf("    %s = %d, %d\n", limits[i].name, max[0], max[1]);
      }
   }
   /* these don't fit into the above mechanism, unfortunately */
   glGetConvolutionParameteriv(GL_CONVOLUTION_2D, GL_MAX_CONVOLUTION_WIDTH, max);
   glGetConvolutionParameteriv(GL_CONVOLUTION_2D, GL_MAX_CONVOLUTION_HEIGHT, max+1);
   if (glGetError() == GL_NONE) {
      printf("    GL_MAX_CONVOLUTION_WIDTH/HEIGHT = %d, %d\n", max[0], max[1]);
   }

}


static void
print_screen_info(Display *dpy, int scrnum, Bool allowDirect, GLboolean limits)
{
   Window win;
   int attribSingle[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      None };
   int attribDouble[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None };

   XSetWindowAttributes attr;
   unsigned long mask;
   Window root;
   GLXContext ctx;
   XVisualInfo *visinfo;
   int width = 100, height = 100;

   root = RootWindow(dpy, scrnum);

   visinfo = glXChooseVisual(dpy, scrnum, attribSingle);
   if (!visinfo) {
      visinfo = glXChooseVisual(dpy, scrnum, attribDouble);
      if (!visinfo) {
         fprintf(stderr, "Error: couldn't find RGB GLX visual\n");
         return;
      }
   }

   attr.background_pixel = 0;
   attr.border_pixel = 0;
   attr.colormap = XCreateColormap(dpy, root, visinfo->visual, AllocNone);
   attr.event_mask = StructureNotifyMask | ExposureMask;
   mask = CWBackPixel | CWBorderPixel | CWColormap | CWEventMask;
   win = XCreateWindow(dpy, root, 0, 0, width, height,
		       0, visinfo->depth, InputOutput,
		       visinfo->visual, mask, &attr);

   ctx = glXCreateContext( dpy, visinfo, NULL, allowDirect );
   if (!ctx) {
      fprintf(stderr, "Error: glXCreateContext failed\n");
      XFree(visinfo);
      XDestroyWindow(dpy, win);
      return;
   }

   if (glXMakeCurrent(dpy, win, ctx)) {
      const char *serverVendor = glXQueryServerString(dpy, scrnum, GLX_VENDOR);
      const char *serverVersion = glXQueryServerString(dpy, scrnum, GLX_VERSION);
      const char *serverExtensions = glXQueryServerString(dpy, scrnum, GLX_EXTENSIONS);
      const char *clientVendor = glXGetClientString(dpy, GLX_VENDOR);
      const char *clientVersion = glXGetClientString(dpy, GLX_VERSION);
      const char *clientExtensions = glXGetClientString(dpy, GLX_EXTENSIONS);
      const char *glxExtensions = glXQueryExtensionsString(dpy, scrnum);
      const char *glVendor = (const char *) glGetString(GL_VENDOR);
      const char *glRenderer = (const char *) glGetString(GL_RENDERER);
      const char *glVersion = (const char *) glGetString(GL_VERSION);
      const char *glExtensions = (const char *) glGetString(GL_EXTENSIONS);
      int glxVersionMajor;
      int glxVersionMinor;
      char *displayName = NULL;
      char *colon = NULL, *period = NULL;
#ifdef DO_GLU
      const char *gluVersion = (const char *) gluGetString(GLU_VERSION);
      const char *gluExtensions = (const char *) gluGetString(GLU_EXTENSIONS);
#endif
      
      if (! glXQueryVersion( dpy, & glxVersionMajor, & glxVersionMinor )) {
         fprintf(stderr, "Error: glXQueryVersion failed\n");
         exit(1);
      }

      /* Strip the screen number from the display name, if present. */
      if (!(displayName = (char *) malloc(strlen(DisplayString(dpy)) + 1))) {
         fprintf(stderr, "Error: malloc() failed\n");
         exit(1);
      }
      strcpy(displayName, DisplayString(dpy));
      colon = strrchr(displayName, ':');
      if (colon) {
         period = strchr(colon, '.');
         if (period)
            *period = '\0';
      }
      printf("display: %s  screen: %d\n", displayName, scrnum);
      free(displayName);
      printf("direct rendering: %s\n", glXIsDirect(dpy, ctx) ? "Yes" : "No");
      printf("server glx vendor string: %s\n", serverVendor);
      printf("server glx version string: %s\n", serverVersion);
      printf("server glx extensions:\n");
      print_extension_list(serverExtensions);
      printf("client glx vendor string: %s\n", clientVendor);
      printf("client glx version string: %s\n", clientVersion);
      printf("client glx extensions:\n");
      print_extension_list(clientExtensions);
      printf("GLX version: %u.%u\n", glxVersionMajor, glxVersionMinor);
      printf("GLX extensions:\n");
      print_extension_list(glxExtensions);
      printf("OpenGL vendor string: %s\n", glVendor);
      printf("OpenGL renderer string: %s\n", glRenderer);
      printf("OpenGL version string: %s\n", glVersion);
      printf("OpenGL extensions:\n");
      print_extension_list(glExtensions);
      if (limits)
         print_limits();
#ifdef DO_GLU
      printf("glu version: %s\n", gluVersion);
      printf("glu extensions:\n");
      print_extension_list(gluExtensions);
#endif
   }
   else {
      fprintf(stderr, "Error: glXMakeCurrent failed\n");
   }

   glXDestroyContext(dpy, ctx);
   XFree(visinfo);
   XDestroyWindow(dpy, win);
}


static const char *
visual_class_name(int cls)
{
   switch (cls) {
      case StaticColor:
         return "StaticColor";
      case PseudoColor:
         return "PseudoColor";
      case StaticGray:
         return "StaticGray";
      case GrayScale:
         return "GrayScale";
      case TrueColor:
         return "TrueColor";
      case DirectColor:
         return "DirectColor";
      default:
         return "";
   }
}


static const char *
visual_class_abbrev(int cls)
{
   switch (cls) {
      case StaticColor:
         return "sc";
      case PseudoColor:
         return "pc";
      case StaticGray:
         return "sg";
      case GrayScale:
         return "gs";
      case TrueColor:
         return "tc";
      case DirectColor:
         return "dc";
      default:
         return "";
   }
}


static void
get_visual_attribs(Display *dpy, XVisualInfo *vInfo,
                   struct visual_attribs *attribs)
{
   const char *ext = glXQueryExtensionsString(dpy, vInfo->screen);

   memset(attribs, 0, sizeof(struct visual_attribs));

   attribs->id = vInfo->visualid;
#if defined(__cplusplus) || defined(c_plusplus)
   attribs->klass = vInfo->c_class;
#else
   attribs->klass = vInfo->class;
#endif
   attribs->depth = vInfo->depth;
   attribs->redMask = vInfo->red_mask;
   attribs->greenMask = vInfo->green_mask;
   attribs->blueMask = vInfo->blue_mask;
   attribs->colormapSize = vInfo->colormap_size;
   attribs->bitsPerRGB = vInfo->bits_per_rgb;

   if (glXGetConfig(dpy, vInfo, GLX_USE_GL, &attribs->supportsGL) != 0)
      return;
   glXGetConfig(dpy, vInfo, GLX_BUFFER_SIZE, &attribs->bufferSize);
   glXGetConfig(dpy, vInfo, GLX_LEVEL, &attribs->level);
   glXGetConfig(dpy, vInfo, GLX_RGBA, &attribs->rgba);
   glXGetConfig(dpy, vInfo, GLX_DOUBLEBUFFER, &attribs->doubleBuffer);
   glXGetConfig(dpy, vInfo, GLX_STEREO, &attribs->stereo);
   glXGetConfig(dpy, vInfo, GLX_AUX_BUFFERS, &attribs->auxBuffers);
   glXGetConfig(dpy, vInfo, GLX_RED_SIZE, &attribs->redSize);
   glXGetConfig(dpy, vInfo, GLX_GREEN_SIZE, &attribs->greenSize);
   glXGetConfig(dpy, vInfo, GLX_BLUE_SIZE, &attribs->blueSize);
   glXGetConfig(dpy, vInfo, GLX_ALPHA_SIZE, &attribs->alphaSize);
   glXGetConfig(dpy, vInfo, GLX_DEPTH_SIZE, &attribs->depthSize);
   glXGetConfig(dpy, vInfo, GLX_STENCIL_SIZE, &attribs->stencilSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_RED_SIZE, &attribs->accumRedSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_GREEN_SIZE, &attribs->accumGreenSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_BLUE_SIZE, &attribs->accumBlueSize);
   glXGetConfig(dpy, vInfo, GLX_ACCUM_ALPHA_SIZE, &attribs->accumAlphaSize);

   /* get transparent pixel stuff */
   glXGetConfig(dpy, vInfo,GLX_TRANSPARENT_TYPE, &attribs->transparentType);
   if (attribs->transparentType == GLX_TRANSPARENT_RGB) {
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_RED_VALUE, &attribs->transparentRedValue);
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_GREEN_VALUE, &attribs->transparentGreenValue);
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_BLUE_VALUE, &attribs->transparentBlueValue);
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_ALPHA_VALUE, &attribs->transparentAlphaValue);
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_INDEX) {
     glXGetConfig(dpy, vInfo, GLX_TRANSPARENT_INDEX_VALUE, &attribs->transparentIndexValue);
   }

   /* multisample attribs */
#ifdef GLX_ARB_multisample
   if (strstr("GLX_ARB_multisample", ext) == 0) {
      glXGetConfig(dpy, vInfo, GLX_SAMPLE_BUFFERS_ARB, &attribs->numMultisample);
      glXGetConfig(dpy, vInfo, GLX_SAMPLES_ARB, &attribs->numSamples);
   }
#endif
   else {
      attribs->numSamples = 0;
      attribs->numMultisample = 0;
   }

#if defined(GLX_EXT_visual_rating)
   if (ext && strstr(ext, "GLX_EXT_visual_rating")) {
      glXGetConfig(dpy, vInfo, GLX_VISUAL_CAVEAT_EXT, &attribs->visualCaveat);
   }
   else {
      attribs->visualCaveat = GLX_NONE_EXT;
   }
#else
   attribs->visualCaveat = 0;
#endif
}


static void
print_visual_attribs_verbose(const struct visual_attribs *attribs)
{
   printf("Visual ID: %x  depth=%d  class=%s\n",
          attribs->id, attribs->depth, visual_class_name(attribs->klass));
   printf("    bufferSize=%d level=%d renderType=%s doubleBuffer=%d stereo=%d\n",
          attribs->bufferSize, attribs->level, attribs->rgba ? "rgba" : "ci",
          attribs->doubleBuffer, attribs->stereo);
   printf("    rgba: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n",
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize);
   printf("    auxBuffers=%d depthSize=%d stencilSize=%d\n",
          attribs->auxBuffers, attribs->depthSize, attribs->stencilSize);
   printf("    accum: redSize=%d greenSize=%d blueSize=%d alphaSize=%d\n",
          attribs->accumRedSize, attribs->accumGreenSize,
          attribs->accumBlueSize, attribs->accumAlphaSize);
   printf("    multiSample=%d  multiSampleBuffers=%d\n",
          attribs->numSamples, attribs->numMultisample);
#ifdef GLX_EXT_visual_rating
   if (attribs->visualCaveat == GLX_NONE_EXT || attribs->visualCaveat == 0)
      printf("    visualCaveat=None\n");
   else if (attribs->visualCaveat == GLX_SLOW_VISUAL_EXT)
      printf("    visualCaveat=Slow\n");
   else if (attribs->visualCaveat == GLX_NON_CONFORMANT_VISUAL_EXT)
      printf("    visualCaveat=Nonconformant\n");
#endif
   if (attribs->transparentType == GLX_NONE) {
     printf("    Opaque.\n");
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_RGB) {
     printf("    Transparent RGB: Red=%d Green=%d Blue=%d Alpha=%d\n",attribs->transparentRedValue,attribs->transparentGreenValue,attribs->transparentBlueValue,attribs->transparentAlphaValue);
   }
   else if (attribs->transparentType == GLX_TRANSPARENT_INDEX) {
     printf("    Transparent index=%d\n",attribs->transparentIndexValue);
   }
}


static void
print_visual_attribs_short_header(void)
{
 printf("   visual  x  bf lv rg d st colorbuffer ax dp st accumbuffer  ms  cav\n");
 printf(" id dep cl sp sz l  ci b ro  r  g  b  a bf th cl  r  g  b  a ns b eat\n");
 printf("----------------------------------------------------------------------\n");
}


static void
print_visual_attribs_short(const struct visual_attribs *attribs)
{
   char *caveat = NULL;
#ifdef GLX_EXT_visual_rating
   if (attribs->visualCaveat == GLX_NONE_EXT || attribs->visualCaveat == 0)
      caveat = "None";
   else if (attribs->visualCaveat == GLX_SLOW_VISUAL_EXT)
      caveat = "Slow";
   else if (attribs->visualCaveat == GLX_NON_CONFORMANT_VISUAL_EXT)
      caveat = "Ncon";
   else
      caveat = "None";
#else
   caveat = "None";
#endif 

   printf("0x%2x %2d %2s %2d %2d %2d %1s %2s %2s %2d %2d %2d %2d %2d %2d %2d",
          attribs->id,
          attribs->depth,
          visual_class_abbrev(attribs->klass),
          attribs->transparentType != GLX_NONE,
          attribs->bufferSize,
          attribs->level,
          attribs->rgba ? "r" : "c",
          attribs->doubleBuffer ? "y" : ".",
          attribs->stereo ? "y" : ".",
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize,
          attribs->auxBuffers,
          attribs->depthSize,
          attribs->stencilSize
          );

   printf(" %2d %2d %2d %2d %2d %1d %s\n",
          attribs->accumRedSize, attribs->accumGreenSize,
          attribs->accumBlueSize, attribs->accumAlphaSize,
          attribs->numSamples, attribs->numMultisample,
          caveat
          );
}


static void
print_visual_attribs_long_header(void)
{
 printf("Vis  Vis   Visual Trans  buff lev render DB ste  r   g   b   a  aux dep ste  accum buffers  MS   MS\n");
 printf(" ID Depth   Type  parent size el   type     reo sz  sz  sz  sz  buf th  ncl  r   g   b   a  num bufs\n");
 printf("----------------------------------------------------------------------------------------------------\n");
}


static void
print_visual_attribs_long(const struct visual_attribs *attribs)
{
   printf("0x%2x %2d %-11s %2d     %2d %2d  %4s %3d %3d %3d %3d %3d %3d",
          attribs->id,
          attribs->depth,
          visual_class_name(attribs->klass),
          attribs->transparentType != GLX_NONE,
          attribs->bufferSize,
          attribs->level,
          attribs->rgba ? "rgba" : "ci  ",
          attribs->doubleBuffer,
          attribs->stereo,
          attribs->redSize, attribs->greenSize,
          attribs->blueSize, attribs->alphaSize
          );

   printf(" %3d %4d %2d %3d %3d %3d %3d  %2d  %2d\n",
          attribs->auxBuffers,
          attribs->depthSize,
          attribs->stencilSize,
          attribs->accumRedSize, attribs->accumGreenSize,
          attribs->accumBlueSize, attribs->accumAlphaSize,
          attribs->numSamples, attribs->numMultisample
          );
}


static void
print_visual_info(Display *dpy, int scrnum, InfoMode mode)
{
   XVisualInfo theTemplate;
   XVisualInfo *visuals;
   int numVisuals;
   long mask;
   int i;

   /* get list of all visuals on this screen */
   theTemplate.screen = scrnum;
   mask = VisualScreenMask;
   visuals = XGetVisualInfo(dpy, mask, &theTemplate, &numVisuals);

   if (mode == Verbose) {
      for (i = 0; i < numVisuals; i++) {
         struct visual_attribs attribs;
         get_visual_attribs(dpy, &visuals[i], &attribs);
         print_visual_attribs_verbose(&attribs);
      }
   }
   else if (mode == Normal) {
      print_visual_attribs_short_header();
      for (i = 0; i < numVisuals; i++) {
         struct visual_attribs attribs;
         get_visual_attribs(dpy, &visuals[i], &attribs);
         print_visual_attribs_short(&attribs);
      }
   }
   else if (mode == Wide) {
      print_visual_attribs_long_header();
      for (i = 0; i < numVisuals; i++) {
         struct visual_attribs attribs;
         get_visual_attribs(dpy, &visuals[i], &attribs);
         print_visual_attribs_long(&attribs);
      }
   }

   XFree(visuals);
}


/*
 * Stand-alone Mesa doesn't really implement the GLX protocol so it
 * doesn't really know the GLX attributes associated with an X visual.
 * The first time a visual is presented to Mesa's pseudo-GLX it
 * attaches ancilliary buffers to it (like depth and stencil).
 * But that usually only works if glXChooseVisual is used.
 * This function calls glXChooseVisual() to sort of "prime the pump"
 * for Mesa's GLX so that the visuals that get reported actually
 * reflect what applications will see.
 * This has no effect when using true GLX.
 */
static void
mesa_hack(Display *dpy, int scrnum)
{
   static int attribs[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_DEPTH_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
      GLX_ACCUM_RED_SIZE, 1,
      GLX_ACCUM_GREEN_SIZE, 1,
      GLX_ACCUM_BLUE_SIZE, 1,
      GLX_ACCUM_ALPHA_SIZE, 1,
      GLX_DOUBLEBUFFER,
      None
   };
   XVisualInfo *visinfo;

   visinfo = glXChooseVisual(dpy, scrnum, attribs);
   if (visinfo)
      XFree(visinfo);
}


/*
 * Examine all visuals to find the so-called best one.
 * We prefer deepest RGBA buffer with depth, stencil and accum
 * that has no caveats.
 */
static int
find_best_visual(Display *dpy, int scrnum)
{
   XVisualInfo theTemplate;
   XVisualInfo *visuals;
   int numVisuals;
   long mask;
   int i;
   struct visual_attribs bestVis;

   /* get list of all visuals on this screen */
   theTemplate.screen = scrnum;
   mask = VisualScreenMask;
   visuals = XGetVisualInfo(dpy, mask, &theTemplate, &numVisuals);

   /* init bestVis with first visual info */
   get_visual_attribs(dpy, &visuals[0], &bestVis);

   /* try to find a "better" visual */
   for (i = 1; i < numVisuals; i++) {
      struct visual_attribs vis;

      get_visual_attribs(dpy, &visuals[i], &vis);

      /* always skip visuals with caveats */
      if (vis.visualCaveat != GLX_NONE_EXT)
         continue;

      /* see if this vis is better than bestVis */
      if ((!bestVis.supportsGL && vis.supportsGL) ||
          (bestVis.visualCaveat != GLX_NONE_EXT) ||
          (!bestVis.rgba && vis.rgba) ||
          (!bestVis.doubleBuffer && vis.doubleBuffer) ||
          (bestVis.redSize < vis.redSize) ||
          (bestVis.greenSize < vis.greenSize) ||
          (bestVis.blueSize < vis.blueSize) ||
          (bestVis.alphaSize < vis.alphaSize) ||
          (bestVis.depthSize < vis.depthSize) ||
          (bestVis.stencilSize < vis.stencilSize) ||
          (bestVis.accumRedSize < vis.accumRedSize)) {
         /* found a better visual */
         bestVis = vis;
      }
   }

   XFree(visuals);

   return bestVis.id;
}


static void
usage(void)
{
   printf("Usage: glxinfo [-v] [-t] [-h] [-i] [-b] [-display <dname>]\n");
   printf("\t-v: Print visuals info in verbose form.\n");
   printf("\t-t: Print verbose table.\n");
   printf("\t-display <dname>: Print GLX visuals on specified server.\n");
   printf("\t-h: This information.\n");
   printf("\t-i: Force an indirect rendering context.\n");
   printf("\t-b: Find the 'best' visual and print it's number.\n");
   printf("\t-l: Print interesting OpenGL limits.\n");
}


int
main(int argc, char *argv[])
{
   char *displayName = NULL;
   Display *dpy;
   int numScreens, scrnum;
   InfoMode mode = Normal;
   GLboolean findBest = GL_FALSE;
   GLboolean limits = GL_FALSE;
   Bool allowDirect = True;
   int i;

   for (i = 1; i < argc; i++) {
      if (strcmp(argv[i], "-display") == 0 && i + 1 < argc) {
         displayName = argv[i + 1];
         i++;
      }
      else if (strcmp(argv[i], "-t") == 0) {
         mode = Wide;
      }
      else if (strcmp(argv[i], "-v") == 0) {
         mode = Verbose;
      }
      else if (strcmp(argv[i], "-b") == 0) {
         findBest = GL_TRUE;
      }
      else if (strcmp(argv[i], "-i") == 0) {
         allowDirect = False;
      }
      else if (strcmp(argv[i], "-l") == 0) {
         limits = GL_TRUE;
      }
      else if (strcmp(argv[i], "-h") == 0) {
         usage();
         return 0;
      }
      else {
         printf("Unknown option `%s'\n", argv[i]);
         usage();
         return 0;
      }
   }

   dpy = XOpenDisplay(displayName);
   if (!dpy) {
      fprintf(stderr, "Error: unable to open display %s\n", displayName);
      return -1;
   }

   if (findBest) {
      int b;
      mesa_hack(dpy, 0);
      b = find_best_visual(dpy, 0);
      printf("%d\n", b);
   }
   else {
      numScreens = ScreenCount(dpy);
      print_display_info(dpy);
      for (scrnum = 0; scrnum < numScreens; scrnum++) {
         mesa_hack(dpy, scrnum);
         print_screen_info(dpy, scrnum, allowDirect, limits);
         printf("\n");
         print_visual_info(dpy, scrnum, mode);
         if (scrnum + 1 < numScreens)
            printf("\n\n");
      }
   }

   XCloseDisplay(dpy);

   return 0;
}
