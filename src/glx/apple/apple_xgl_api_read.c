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

/*
 * This file works with the glXMakeContextCurrent readable drawable. 
 *
 * The way it works is by swapping the currentDrawable for the currentReadable
 * drawable if they are different.
 */
#include <stdbool.h>
#include "apple_xgl_api_read.h"
#include "apple_xgl_api.h"
#include "apple_cgl.h"
#include "apple_glx_context.h"

extern struct apple_xgl_api __gl_api;

struct apple_xgl_saved_state
{
   bool swapped;
};

static void
SetRead(struct apple_xgl_saved_state *saved)
{
   GLXContext gc = __glXGetCurrentContext();

   /*
    * By default indicate that the state was not swapped, so that UnsetRead
    * functions correctly.
    */
   saved->swapped = false;

   /* 
    * If the readable drawable isn't the same as the drawable then 
    * the user has requested a readable drawable with glXMakeContextCurrent().
    * We emulate this behavior by switching the current drawable.
    */
   if (None != gc->currentReadable
       && gc->currentReadable != gc->currentDrawable) {
      Display *dpy = glXGetCurrentDisplay();

      saved->swapped = true;

      if (apple_glx_make_current_context(dpy, gc->apple, gc->apple,
                                         gc->currentReadable)) {
         /* An error occurred, so try to restore the old context state. */
         (void) apple_glx_make_current_context(dpy, gc->apple, gc->apple,
                                               gc->currentDrawable);
         saved->swapped = false;
      }
   }
}

static void
UnsetRead(struct apple_xgl_saved_state *saved)
{
   if (saved->swapped) {
      GLXContext gc = __glXGetCurrentContext();
      Display *dpy = glXGetCurrentDisplay();

      if (apple_glx_make_current_context(dpy, gc->apple, gc->apple,
                                         gc->currentDrawable)) {
         /*
          * An error occurred restoring the drawable.
          * It's unclear what to do about that. 
          */
      }
   }
}

void
glReadPixels(GLint x, GLint y, GLsizei width, GLsizei height,
             GLenum format, GLenum type, void *pixels)
{
   struct apple_xgl_saved_state saved;

   SetRead(&saved);

   __gl_api.ReadPixels(x, y, width, height, format, type, pixels);

   UnsetRead(&saved);
}

void
glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
   struct apple_xgl_saved_state saved;

   SetRead(&saved);

   __gl_api.CopyPixels(x, y, width, height, type);

   UnsetRead(&saved);
}

void
glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y,
                 GLsizei width)
{
   struct apple_xgl_saved_state saved;

   SetRead(&saved);

   __gl_api.CopyColorTable(target, internalformat, x, y, width);

   UnsetRead(&saved);
}
