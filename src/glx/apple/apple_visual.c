/*
 Copyright (c) 2008, 2009 Apple Inc.
 
 Permission is hereby granted, free of charge, to any person
 obtaining a copy of this software and associated documentation files
 (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge,
 publish, distribute, sublicense, and/or sell copies of the Software,
 and to permit persons to whom the Software is furnished to do so,
 subject to the following conditions:
 
 The above copyright notice and this permission notice shall be
 included in all copies or substantial portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT.  IN NO EVENT SHALL THE ABOVE LISTED COPYRIGHT
 HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 DEALINGS IN THE SOFTWARE.
 
 Except as contained in this notice, the name(s) of the above
 copyright holders shall not be used in advertising or otherwise to
 promote the sale, use or other dealings in this Software without
 prior written authorization.
*/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <GL/gl.h>
#include <OpenGL/OpenGL.h>
#include <OpenGL/CGLContext.h>
#include <OpenGL/CGLRenderers.h>
#include "apple_cgl.h"
#include "apple_visual.h"
#include "apple_glx.h"
#include "glcontextmodes.h"

enum
{
   MAX_ATTR = 60
};

/*mode is a __GlcontextModes*/
void
apple_visual_create_pfobj(CGLPixelFormatObj * pfobj, const void *mode,
                          bool * double_buffered, bool * uses_stereo,
                          bool offscreen)
{
   CGLPixelFormatAttribute attr[MAX_ATTR];
   const __GLcontextModes *c = mode;
   int numattr = 0;
   GLint vsref = 0;
   CGLError error = 0;

   if (offscreen) {
      apple_glx_diagnostic
         ("offscreen rendering enabled.  Using kCGLPFAOffScreen\n");

      attr[numattr++] = kCGLPFAOffScreen;
      attr[numattr++] = kCGLPFAColorSize;
      attr[numattr++] = 32;
   }
   else if (getenv("LIBGL_ALWAYS_SOFTWARE") != NULL) {
      apple_glx_diagnostic
         ("Software rendering requested.  Using kCGLRendererGenericFloatID.\n");
      attr[numattr++] = kCGLPFARendererID;
      attr[numattr++] = kCGLRendererGenericFloatID;
   }
   else if (getenv("LIBGL_ALLOW_SOFTWARE") != NULL) {
      apple_glx_diagnostic
         ("Software rendering is not being excluded.  Not using kCGLPFAAccelerated.\n");
   }
   else {
      attr[numattr++] = kCGLPFAAccelerated;
   }

   /* 
    * The program chose a config based on the fbconfigs or visuals.
    * Those are based on the attributes from CGL, so we probably
    * do want the closest match for the color, depth, and accum.
    */
   attr[numattr++] = kCGLPFAClosestPolicy;

   if (c->stereoMode) {
      attr[numattr++] = kCGLPFAStereo;
      *uses_stereo = true;
   }
   else {
      *uses_stereo = false;
   }

   if (c->doubleBufferMode) {
      attr[numattr++] = kCGLPFADoubleBuffer;
      *double_buffered = true;
   }
   else {
      *double_buffered = false;
   }

   attr[numattr++] = kCGLPFAColorSize;
   attr[numattr++] = c->redBits + c->greenBits + c->blueBits;
   attr[numattr++] = kCGLPFAAlphaSize;
   attr[numattr++] = c->alphaBits;

   if ((c->accumRedBits + c->accumGreenBits + c->accumBlueBits) > 0) {
      attr[numattr++] = kCGLPFAAccumSize;
      attr[numattr++] = c->accumRedBits + c->accumGreenBits +
         c->accumBlueBits + c->accumAlphaBits;
   }

   if (c->depthBits > 0) {
      attr[numattr++] = kCGLPFADepthSize;
      attr[numattr++] = c->depthBits;
   }

   if (c->stencilBits > 0) {
      attr[numattr++] = kCGLPFAStencilSize;
      attr[numattr++] = c->stencilBits;
   }

   if (c->sampleBuffers > 0) {
      attr[numattr++] = kCGLPFAMultisample;
      attr[numattr++] = kCGLPFASampleBuffers;
      attr[numattr++] = c->sampleBuffers;
      attr[numattr++] = kCGLPFASamples;
      attr[numattr++] = c->samples;
   }

   attr[numattr++] = 0;

   assert(numattr < MAX_ATTR);

   error = apple_cgl.choose_pixel_format(attr, pfobj, &vsref);

   if (error) {
      fprintf(stderr, "error: %s\n", apple_cgl.error_string(error));
      abort();
   }
}
