/*
 Copyright (c) 2009 Apple Inc.
 
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
#include "apple_xgl_api_stereo.h"
#include "apple_xgl_api.h"
#include "apple_glx_context.h"

extern struct apple_xgl_api __gl_api;
/* 
 * These are special functions for stereoscopic support 
 * differences in MacOS X.
 */
void
glDrawBuffer(GLenum mode)
{
   GLXContext gc = glXGetCurrentContext();

   if (gc && apple_glx_context_uses_stereo(gc->apple)) {
      GLenum buf[2];
      GLsizei n = 0;

      switch (mode) {
      case GL_BACK:
         buf[0] = GL_BACK_LEFT;
         buf[1] = GL_BACK_RIGHT;
         n = 2;
         break;
      case GL_FRONT:
         buf[0] = GL_FRONT_LEFT;
         buf[1] = GL_FRONT_RIGHT;
         n = 2;
         break;

      default:
         buf[0] = mode;
         n = 1;
         break;
      }

      __gl_api.DrawBuffers(n, buf);
   }
   else {
      __gl_api.DrawBuffer(mode);
   }
}


void
glDrawBuffers(GLsizei n, const GLenum * bufs)
{
   GLXContext gc = glXGetCurrentContext();

   if (gc && apple_glx_context_uses_stereo(gc->apple)) {
      GLenum newbuf[n + 2];
      GLsizei i, outi = 0;
      bool have_back = false;
      bool have_front = false;

      for (i = 0; i < n; ++i) {
         if (GL_BACK == bufs[i]) {
            have_back = true;
         }
         else if (GL_FRONT == bufs[i]) {
            have_back = true;
         }
         else {
            newbuf[outi++] = bufs[i];
         }
      }

      if (have_back) {
         newbuf[outi++] = GL_BACK_LEFT;
         newbuf[outi++] = GL_BACK_RIGHT;
      }

      if (have_front) {
         newbuf[outi++] = GL_FRONT_LEFT;
         newbuf[outi++] = GL_FRONT_RIGHT;
      }

      __gl_api.DrawBuffers(outi, newbuf);
   }
   else {
      __gl_api.DrawBuffers(n, bufs);
   }
}

void
glDrawBuffersARB(GLsizei n, const GLenum * bufs)
{
   glDrawBuffers(n, bufs);
}
