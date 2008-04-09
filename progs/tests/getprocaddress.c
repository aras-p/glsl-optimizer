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
#include <math.h>


typedef void (*generic_func)();

#define EQUAL(X, Y)  (fabs((X) - (Y)) < 0.001)

/**
 * The following functions are used to check that the named OpenGL function
 * actually does what it's supposed to do.
 * The naming of these functions is signficant.  The getprocaddress.py script
 * scans this file and extracts these function names.
 */


static GLboolean
test_ActiveTextureARB(generic_func func)
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
test_SecondaryColor3fEXT(generic_func func)
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
test_ActiveStencilFaceEXT(generic_func func)
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


static GLboolean
test_VertexAttrib1fvARB(generic_func func)
{
   PFNGLVERTEXATTRIB1FVARBPROC vertexAttrib1fvARB = (PFNGLVERTEXATTRIB1FVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLfloat v[1] = {25.0};
   const GLfloat def[1] = {0};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib1fvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (res[0] == 25.0 && res[1] == 0.0 && res[2] == 0.0 && res[3] == 1.0);
   (*vertexAttrib1fvARB)(6, def);
   return pass;
}

static GLboolean
test_VertexAttrib4NubvARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NUBVARBPROC vertexAttrib4NubvARB = (PFNGLVERTEXATTRIB4NUBVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLubyte v[4] = {255, 0, 255, 0};
   const GLubyte def[4] = {0, 0, 0, 255};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NubvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (res[0] == 1.0 && res[1] == 0.0 && res[2] == 1.0 && res[3] == 0.0);
   (*vertexAttrib4NubvARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4NuivARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NUIVARBPROC vertexAttrib4NuivARB = (PFNGLVERTEXATTRIB4NUIVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLuint v[4] = {0xffffffff, 0, 0xffffffff, 0};
   const GLuint def[4] = {0, 0, 0, 0xffffffff};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NuivARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 1.0) && EQUAL(res[1], 0.0) && EQUAL(res[2], 1.0) && EQUAL(res[3], 0.0));
   (*vertexAttrib4NuivARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4ivARB(generic_func func)
{
   PFNGLVERTEXATTRIB4IVARBPROC vertexAttrib4ivARB = (PFNGLVERTEXATTRIB4IVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLint v[4] = {1, 2, -3, 4};
   const GLint def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4ivARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 1.0) && EQUAL(res[1], 2.0) && EQUAL(res[2], -3.0) && EQUAL(res[3], 4.0));
   (*vertexAttrib4ivARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4NsvARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NSVARBPROC vertexAttrib4NsvARB = (PFNGLVERTEXATTRIB4NSVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLshort v[4] = {0, 32767, 32767, 0};
   const GLshort def[4] = {0, 0, 0, 32767};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NsvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 0.0) && EQUAL(res[1], 1.0) && EQUAL(res[2], 1.0) && EQUAL(res[3], 0.0));
   (*vertexAttrib4NsvARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4NusvARB(generic_func func)
{
   PFNGLVERTEXATTRIB4NUSVARBPROC vertexAttrib4NusvARB = (PFNGLVERTEXATTRIB4NUSVARBPROC) func;
   PFNGLGETVERTEXATTRIBFVARBPROC getVertexAttribfvARB = (PFNGLGETVERTEXATTRIBFVARBPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvARB");

   const GLushort v[4] = {0xffff, 0, 0xffff, 0};
   const GLushort def[4] = {0, 0, 0, 0xffff};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4NusvARB)(6, v);
   (*getVertexAttribfvARB)(6, GL_CURRENT_VERTEX_ATTRIB_ARB, res);
   pass = (EQUAL(res[0], 1.0) && EQUAL(res[1], 0.0) && EQUAL(res[2], 1.0) && EQUAL(res[3], 0.0));
   (*vertexAttrib4NusvARB)(6, def);
   return pass;
}


static GLboolean
test_VertexAttrib4ubNV(generic_func func)
{
   PFNGLVERTEXATTRIB4UBNVPROC vertexAttrib4ubNV = (PFNGLVERTEXATTRIB4UBNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLubyte v[4] = {255, 0, 255, 0};
   const GLubyte def[4] = {0, 0, 0, 255};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4ubNV)(6, v[0], v[1], v[2], v[3]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   pass = (res[0] == 1.0 && res[1] == 0.0 && res[2] == 1.0 && res[3] == 0.0);
   (*vertexAttrib4ubNV)(6, def[0], def[1], def[2], def[3]);
   return pass;
}


static GLboolean
test_VertexAttrib2sNV(generic_func func)
{
   PFNGLVERTEXATTRIB2SNVPROC vertexAttrib2sNV = (PFNGLVERTEXATTRIB2SNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLshort v[2] = {2, -4,};
   const GLshort def[2] = {0, 0};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib2sNV)(6, v[0], v[1]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   pass = (EQUAL(res[0], 2) && EQUAL(res[1], -4) && EQUAL(res[2], 0) && res[3] == 1.0);
   (*vertexAttrib2sNV)(6, def[0], def[1]);
   return pass;
}


static GLboolean
test_VertexAttrib3fNV(generic_func func)
{
   PFNGLVERTEXATTRIB3FNVPROC vertexAttrib3fNV = (PFNGLVERTEXATTRIB3FNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLfloat v[3] = {0.2, 0.4, 0.8};
   const GLfloat def[3] = {0, 0, 0};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib3fNV)(6, v[0], v[1], v[2]);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   pass = (EQUAL(res[0], 0.2) && EQUAL(res[1], 0.4) && EQUAL(res[2], 0.8) && res[3] == 1.0);
   (*vertexAttrib3fNV)(6, def[0], def[1], def[2]);
   return pass;
}


static GLboolean
test_VertexAttrib4dvNV(generic_func func)
{
   PFNGLVERTEXATTRIB4DVNVPROC vertexAttrib4dvNV = (PFNGLVERTEXATTRIB4DVNVPROC) func;
   PFNGLGETVERTEXATTRIBFVNVPROC getVertexAttribfvNV = (PFNGLGETVERTEXATTRIBFVNVPROC) glXGetProcAddressARB((const GLubyte *) "glGetVertexAttribfvNV");

   const GLdouble v[4] = {0.2, 0.4, 0.8, 1.2};
   const GLdouble def[4] = {0, 0, 0, 1};
   GLfloat res[4];
   GLboolean pass;
   (*vertexAttrib4dvNV)(6, v);
   (*getVertexAttribfvNV)(6, GL_CURRENT_ATTRIB_NV, res);
   pass = (EQUAL(res[0], 0.2) && EQUAL(res[1], 0.4) && EQUAL(res[2], 0.8) && EQUAL(res[3], 1.2));
   (*vertexAttrib4dvNV)(6, def);
   return pass;
}


static GLboolean
test_StencilFuncSeparateATI(generic_func func)
{
#ifdef GL_ATI_separate_stencil
   PFNGLSTENCILFUNCSEPARATEATIPROC stencilFuncSeparateATI = (PFNGLSTENCILFUNCSEPARATEATIPROC) func;
   GLint frontFunc, backFunc;
   GLint frontRef, backRef;
   GLint frontMask, backMask;
   (*stencilFuncSeparateATI)(GL_LESS, GL_GREATER, 2, 0xa);
   glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);
   glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);
   glGetIntegerv(GL_STENCIL_REF, &frontRef);
   glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
   glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontMask);
   glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backMask);
   if (frontFunc != GL_LESS ||
       backFunc != GL_GREATER ||
       frontRef != 2 ||
       backRef != 2 ||
       frontMask != 0xa ||
       backMask != 0xa)
      return GL_FALSE;
#endif
   return GL_TRUE;
}

static GLboolean
test_StencilFuncSeparate(generic_func func)
{
#ifdef GL_VERSION_2_0
   PFNGLSTENCILFUNCSEPARATEPROC stencilFuncSeparate = (PFNGLSTENCILFUNCSEPARATEPROC) func;
   GLint frontFunc, backFunc;
   GLint frontRef, backRef;
   GLint frontMask, backMask;
   (*stencilFuncSeparate)(GL_BACK, GL_GREATER, 2, 0xa);
   glGetIntegerv(GL_STENCIL_FUNC, &frontFunc);
   glGetIntegerv(GL_STENCIL_BACK_FUNC, &backFunc);
   glGetIntegerv(GL_STENCIL_REF, &frontRef);
   glGetIntegerv(GL_STENCIL_BACK_REF, &backRef);
   glGetIntegerv(GL_STENCIL_VALUE_MASK, &frontMask);
   glGetIntegerv(GL_STENCIL_BACK_VALUE_MASK, &backMask);
   if (frontFunc != GL_ALWAYS ||
       backFunc != GL_GREATER ||
       frontRef != 0 ||
       backRef != 2 ||
       frontMask == 0xa || /* might be 0xff or ~0 */
       backMask != 0xa)
      return GL_FALSE;
#endif
   return GL_TRUE;
}

static GLboolean
test_StencilOpSeparate(generic_func func)
{
#ifdef GL_VERSION_2_0
   PFNGLSTENCILOPSEPARATEPROC stencilOpSeparate = (PFNGLSTENCILOPSEPARATEPROC) func;
   GLint frontFail, backFail;
   GLint frontZFail, backZFail;
   GLint frontZPass, backZPass;
   (*stencilOpSeparate)(GL_BACK, GL_INCR, GL_DECR, GL_INVERT);
   glGetIntegerv(GL_STENCIL_FAIL, &frontFail);
   glGetIntegerv(GL_STENCIL_BACK_FAIL, &backFail);
   glGetIntegerv(GL_STENCIL_PASS_DEPTH_FAIL, &frontZFail);
   glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_FAIL, &backZFail);
   glGetIntegerv(GL_STENCIL_PASS_DEPTH_PASS, &frontZPass);
   glGetIntegerv(GL_STENCIL_BACK_PASS_DEPTH_PASS, &backZPass);
   if (frontFail != GL_KEEP ||
       backFail != GL_INCR ||
       frontZFail != GL_KEEP ||
       backZFail != GL_DECR ||
       frontZPass != GL_KEEP ||
       backZPass != GL_INVERT)
      return GL_FALSE;
#endif
   return GL_TRUE;
}

static GLboolean
test_StencilMaskSeparate(generic_func func)
{
#ifdef GL_VERSION_2_0
   PFNGLSTENCILMASKSEPARATEPROC stencilMaskSeparate = (PFNGLSTENCILMASKSEPARATEPROC) func;
   GLint frontMask, backMask;
   (*stencilMaskSeparate)(GL_BACK, 0x1b);
   glGetIntegerv(GL_STENCIL_WRITEMASK, &frontMask);
   glGetIntegerv(GL_STENCIL_BACK_WRITEMASK, &backMask);
   if (frontMask == 0x1b ||
       backMask != 0x1b)
      return GL_FALSE;
#endif
   return GL_TRUE;
}


/*
 * The following file is auto-generated with Python.
 */
#include "getproclist.h"



static int
extension_supported(const char *haystack, const char *needle)
{
   const char *p = strstr(haystack, needle);
   if (p) {
      /* found string, make sure next char is space or zero */
      const int len = strlen(needle);
      if (p[len] == ' ' || p[len] == 0)
         return 1;
      else
         return 0;
   }
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
         const char *version = (const char *) glGetString(GL_VERSION);
         if (entry->name[1] == '1') {
            /* check GL version 1.x */
            if (version[0] == '1' &&
                version[1] == '.' &&
                version[2] >= entry->name[3])
               doTests = 1;
            else
               doTests = 0;
         }
         else if (entry->name[1] == '2') {
            if (version[0] == '2' &&
                version[1] == '.' &&
                version[2] >= entry->name[3])
               doTests = 1;
            else
               doTests = 0;
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
         generic_func funcPtr = (generic_func) glXGetProcAddressARB((const GLubyte *) entry->name);
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
      GLX_STENCIL_SIZE, 1,
      None };
   int attribDouble[] = {
      GLX_RGBA,
      GLX_RED_SIZE, 1,
      GLX_GREEN_SIZE, 1,
      GLX_BLUE_SIZE, 1,
      GLX_STENCIL_SIZE, 1,
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
