/* $Id: glxapi.h,v 1.6 2000/06/08 22:50:28 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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


#ifndef _glxapi_h_
#define _glxapi_h_


#define GLX_GLXEXT_LEGACY
#define GL_GLEXT_PROTOTYPES
#include "GL/glx.h"


/*
 * Almost all the GLX API functions get routed through this dispatch table.
 * The exceptions are the glXGetCurrentXXX() functions.
 *
 * This dispatch table allows multiple GLX client-side modules to coexist.
 * Specifically, a real GLX library (like SGI's or the Utah GLX) and Mesa's
 * pseudo-GLX can be present at the same time.  The former being used on
 * GLX-enabled X servers and the later on non-GLX X servers.
 */
struct _glxapi_table {
   /* GLX 1.0 functions */
   XVisualInfo *(*ChooseVisual)(Display *dpy, int screen, int *list);
   void (*CopyContext)(Display *dpy, GLXContext src, GLXContext dst, unsigned long mask);
   GLXContext (*CreateContext)(Display *dpy, XVisualInfo *visinfo, GLXContext shareList, Bool direct);
   GLXPixmap (*CreateGLXPixmap)(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap);
   void (*DestroyContext)(Display *dpy, GLXContext ctx);
   void (*DestroyGLXPixmap)(Display *dpy, GLXPixmap pixmap);
   int (*GetConfig)(Display *dpy, XVisualInfo *visinfo, int attrib, int *value);
   /*GLXContext (*GetCurrentContext)(void);*/
   /*GLXDrawable (*GetCurrentDrawable)(void);*/
   Bool (*IsDirect)(Display *dpy, GLXContext ctx);
   Bool (*MakeCurrent)(Display *dpy, GLXDrawable drawable, GLXContext ctx);
   Bool (*QueryExtension)(Display *dpy, int *errorb, int *event);
   Bool (*QueryVersion)(Display *dpy, int *maj, int *min);
   void (*SwapBuffers)(Display *dpy, GLXDrawable drawable);
   void (*UseXFont)(Font font, int first, int count, int listBase);
   void (*WaitGL)(void);
   void (*WaitX)(void);

#ifdef GLX_VERSION_1_1
   const char *(*GetClientString)(Display *dpy, int name);
   const char *(*QueryExtensionsString)(Display *dpy, int screen);
   const char *(*QueryServerString)(Display *dpy, int screen, int name);
#endif

#ifdef GLX_VERSION_1_2
   /*Display *(*GetCurrentDisplay)(void);*/
#endif

#ifdef GLX_VERSION_1_3
   GLXFBConfig *(*ChooseFBConfig)(Display *dpy, int screen, const int *attribList, int *nitems);
   GLXContext (*CreateNewContext)(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct);
   GLXPbuffer (*CreatePbuffer)(Display *dpy, GLXFBConfig config, const int *attribList);
   GLXPixmap (*CreatePixmap)(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList);
   GLXWindow (*CreateWindow)(Display *dpy, GLXFBConfig config, Window win, const int *attribList);
   void (*DestroyPbuffer)(Display *dpy, GLXPbuffer pbuf);
   void (*DestroyPixmap)(Display *dpy, GLXPixmap pixmap);
   void (*DestroyWindow)(Display *dpy, GLXWindow window);
   /*GLXDrawable (*GetCurrentReadDrawable)(void);*/
   int (*GetFBConfigAttrib)(Display *dpy, GLXFBConfig config, int attribute, int *value);
   GLXFBConfig *(*GetFBConfigs)(Display *dpy, int screen, int *nelements);
   void (*GetSelectedEvent)(Display *dpy, GLXDrawable drawable, unsigned long *mask);
   XVisualInfo *(*GetVisualFromFBConfig)(Display *dpy, GLXFBConfig config);
   Bool (*MakeContextCurrent)(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx);
   int (*QueryContext)(Display *dpy, GLXContext ctx, int attribute, int *value);
   void (*QueryDrawable)(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value);
   void (*SelectEvent)(Display *dpy, GLXDrawable drawable, unsigned long mask);
#endif

#ifdef GLX_EXT_import_context
   void (*FreeContextEXT)(Display *dpy, GLXContext context);
   GLXContextID (*GetContextIDEXT)(const GLXContext context);
   Display *(*GetCurrentDisplayEXT)(void);
   GLXContext (*ImportContextEXT)(Display *dpy, GLXContextID contextID);
   int (*QueryContextInfoEXT)(Display *dpy, GLXContext context, int attribute,int *value);
#endif

#ifdef GLX_SGI_video_sync
   int (*GetVideoSyncSGI)(unsigned int *count);
   int (*WaitVideoSyncSGI)(int divisor, int remainder, unsigned int *count);
#endif

   /*
    * XXX thesa Mesa-specific functions might not belong here
    */

#ifdef GLX_MESA_copy_sub_buffer
   void (*CopySubBufferMESA)(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height);
#endif

#ifdef GLX_MESA_release_buffers
   Bool (*ReleaseBuffersMESA)(Display *dpy, Window w);
#endif

#ifdef GLX_MESA_pixmap_colormap
   GLXPixmap (*CreateGLXPixmapMESA)(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap, Colormap cmap);
#endif

#ifdef GLX_MESA_set_3dfx_mode
   GLboolean (*Set3DfxModeMESA)(GLint mode);
#endif

};



extern const char *
_glxapi_get_version(void);


extern const char **
_glxapi_get_extensions(void);


extern GLuint
_glxapi_get_dispatch_table_size(void);


extern void
_glxapi_set_no_op_table(struct _glxapi_table *t);


extern const GLvoid *
_glxapi_get_proc_address(const char *funcName);


#endif
