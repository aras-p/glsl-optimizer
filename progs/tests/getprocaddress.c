/* $Id: getprocaddress.c,v 1.6 2002/11/08 15:35:46 brianp Exp $ */

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
 * Test that glXGetProcAddress works.
 */

#define GLX_GLXEXT_PROTOTYPES

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <GL/gl.h>
#include <GL/glx.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


static GLboolean
test_ActiveTextureARB(void *func)
{
   PFNGLACTIVETEXTUREARBPROC activeTexture = (PFNGLACTIVETEXTUREARBPROC) func;
   GLint t;
   GLboolean pass;
   (*activeTexture)(GL_TEXTURE1_ARB);
   glGetIntegerv(GL_ACTIVE_TEXTURE_ARB, &t);
   pass = (t == GL_TEXTURE1_ARB);
   (*activeTexture)(GL_TEXTURE0_ARB);  /* restore default */
   return pass;
}


static GLboolean
test_SecondaryColor3fEXT(void *func)
{
   PFNGLSECONDARYCOLOR3FEXTPROC secColor3f = (PFNGLSECONDARYCOLOR3FEXTPROC) func;
   GLfloat color[4];
   GLboolean pass;
   (*secColor3f)(1.0, 1.0, 0.0);
   glGetFloatv(GL_CURRENT_SECONDARY_COLOR_EXT, color);
   pass = (color[0] == 1.0 && color[1] == 1.0 && color[2] == 0.0);
   (*secColor3f)(0.0, 0.0, 0.0);  /* restore default */
   return pass;
}


static GLboolean
test_ActiveStencilFaceEXT(void *func)
{
   PFNGLACTIVESTENCILFACEEXTPROC activeFace = (PFNGLACTIVESTENCILFACEEXTPROC) func;
   GLint face;
   GLboolean pass;
   (*activeFace)(GL_BACK);
   glGetIntegerv(GL_ACTIVE_STENCIL_FACE_EXT, &face);
   pass = (face == GL_BACK);
   (*activeFace)(GL_FRONT);  /* restore default */
   return pass;
}




/*
 * The following header file is auto-generated with Python.  The Python
 * script looks in this file for functions named "test_*" as seen above.
 */
#include "getproclist.h"



static int
extension_supported(const char *haystack, const char *needle)
{
   if (strstr(haystack, needle))
      return 1;
   else
      return 0;
}


static void
check_functions( const char *extensions )
{
   struct name_test_pair *entry;
   int failures = 0, passes = 0;
   int totalFail = 0, totalPass = 0;
   int doTests;

   for (entry = functions; entry->name; entry++) {
      if (entry->name[0] == '-') {
         if (entry->name[1] == '1') {
            doTests = 1;
         }
         else {
            /* check if the named extension is available */
            doTests = extension_supported(extensions, entry->name+1);
         }
         if (doTests)
            printf("Testing %s functions\n", entry->name + 1);
         totalFail += failures;
         totalPass += passes;
         failures = 0;
         passes = 0;
      }
      else if (doTests) {
         void *funcPtr = (void *) glXGetProcAddressARB((const GLubyte *) entry->name);
         if (funcPtr) {
            if (entry->test) {
               GLboolean b;
               printf("   Validating %s:", entry->name);
               b = (*entry->test)(funcPtr);
               if (b) {
                  printf(" Pass\n");
                  passes++;
               }
               else {
                  printf(" FAIL!!!\n");
                  failures++;
               }
            }
            else {
               passes++;
            }
         }
         else {
            printf("   glXGetProcAddress(%s) failed!\n", entry->name);
            failures++;
         }
      }

      if (doTests && (!(entry+1)->name || (entry+1)->name[0] == '-')) {
         if (failures > 0) {
            printf("   %d failed.\n", failures);
         }
         if (passes > 0) {
            printf("   %d passed.\n", passes);
         }
      }
   }
   totalFail += failures;
   totalPass += passes;

   printf("-----------------------------\n");
   printf("Total: %d pass  %d fail\n", totalPass, totalFail);
}



static void
print_screen_info(Display *dpy, int scrnum, Bool allowDirect)
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
      XDestroyWindow(dpy, win);
      return;
   }

   if (glXMakeCurrent(dpy, win, ctx)) {
      check_functions( (const char *) glGetString(GL_EXTENSIONS) );
   }
   else {
      fprintf(stderr, "Error: glXMakeCurrent failed\n");
   }

   glXDestroyContext(dpy, ctx);
   XDestroyWindow(dpy, win);
}


int
main(int argc, char *argv[])
{
   char *displayName = NULL;
   Display *dpy;

   dpy = XOpenDisplay(displayName);
   if (!dpy) {
      fprintf(stderr, "Error: unable to open display %s\n", displayName);
      return -1;
   }

   print_screen_info(dpy, 0, GL_TRUE);

   XCloseDisplay(dpy);

   return 0;
}
