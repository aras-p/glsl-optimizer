/* $Id: glxapi.c,v 1.8 1999/11/28 20:07:33 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.3
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
 * This is the GLX API dispatcher.  Calls to the glX* functions are
 * either routed to real (SGI / Utah) GLX encoders or to Mesa's
 * pseudo-GLX module.
 */


#include <assert.h>
#include <stdlib.h>
#include "glxapi.h"


/*
 * XXX - this really shouldn't be here.
 * Instead, add -DUSE_MESA_GLX to the compiler flags when needed.
 */
#define USE_MESA_GLX 1


/* Rather than include possibly non-existant headers... */
#ifdef USE_SGI_GLX
extern struct _glxapi_table *_sgi_GetGLXDispatchtable(void);
#endif
#ifdef USE_UTAH_GLX
extern struct _glxapi_table *_utah_GetGLXDispatchTable(void);
#endif
#ifdef USE_MESA_GLX
extern struct _glxapi_table *_mesa_GetGLXDispatchTable(void);
#endif



struct display_dispatch {
   Display *Dpy;
   struct _glxapi_table *Table;
   struct display_dispatch *Next;
};

static struct display_dispatch *DispatchList = NULL;


static struct _glxapi_table *
get_dispatch(Display *dpy)
{
   static Display *prevDisplay = NULL;
   static struct _glxapi_table *prevTable = NULL;

   if (!dpy)
      return NULL;

   /* try cached display */
   if (dpy == prevDisplay) {
      return prevTable;
   }

   /* search list of display/dispatch pairs for this display */
   {
      const struct display_dispatch *d = DispatchList;
      while (d) {
         if (d->Dpy == dpy) {
            prevDisplay = dpy;
            prevTable = d->Table;
            return d->Table;  /* done! */
         }
         d = d->Next;
      }
   }

   /* A new display, determine if we should use real GLX (SGI / Utah)
    * or Mesa's pseudo-GLX.
    */
   {
      struct _glxapi_table *t = NULL;

#if defined(USE_SGI_GLX) || defined(USE_UTAH_GLX)
      if (!getenv("MESA_FORCE_SOFTX")) {
         int ignore;
         if (XQueryExtension( dpy, "GLX", &ignore, &ignore, &ignore )) {
            /* the X server has the GLX extension */
#if defined(USE_SGI_GLX)
            t = _sgi_GetGLXDispatchtable();
#elif defined(USE_UTAH_GLX)
            t = _utah_GetGLXDispatchTable();
#endif
         }
      }
#endif

#if defined(USE_MESA_GLX)
      if (!t) {
         t = _mesa_GetGLXDispatchTable();
         assert(t);  /* this has to work */
      }
#endif

      if (t) {
         struct display_dispatch *d;
         d = (struct display_dispatch *) malloc(sizeof(struct display_dispatch));
         if (d) {
            d->Dpy = dpy;
            d->Table = t;
            /* insert at head of list */
            d->Next = DispatchList;
            DispatchList = d;
            /* update cache */
            prevDisplay = dpy;
            prevTable = t;
            return t;
         }
      }
   }

   /* If we get here that means we can't use real GLX on this display
    * and the Mesa pseudo-GLX software renderer wasn't compiled in.
    * Or, we ran out of memory!
    */
   return NULL;
}



/* Set by glXMakeCurrent() and glXMakeContextCurrent() only */
static Display *CurrentDisplay = NULL;
static GLXContext CurrentContext = 0;
static GLXDrawable CurrentDrawable = 0;
static GLXDrawable CurrentReadDrawable = 0;



/*
 * GLX API entrypoints
 */


XVisualInfo *glXChooseVisual(Display *dpy, int screen, int *list)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return NULL;
   return (t->ChooseVisual)(dpy, screen, list);
}


void glXCopyContext(Display *dpy, GLXContext src, GLXContext dst, GLuint mask)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->CopyContext)(dpy, src, dst, mask);
}


GLXContext glXCreateContext(Display *dpy, XVisualInfo *visinfo, GLXContext shareList, Bool direct)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreateContext)(dpy, visinfo, shareList, direct);
}


GLXPixmap glXCreateGLXPixmap(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreateGLXPixmap)(dpy, visinfo, pixmap);
}


void glXDestroyContext(Display *dpy, GLXContext ctx)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->DestroyContext)(dpy, ctx);
}


void glXDestroyGLXPixmap(Display *dpy, GLXPixmap pixmap)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->DestroyGLXPixmap)(dpy, pixmap);
}


int glXGetConfig(Display *dpy, XVisualInfo *visinfo, int attrib, int *value)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return GLX_NO_EXTENSION;
   return (t->GetConfig)(dpy, visinfo, attrib, value);
}


GLXContext glXGetCurrentContext(void)
{
   return CurrentContext;
}


GLXDrawable glXGetCurrentDrawable(void)
{
   return CurrentDrawable;
}


Bool glXIsDirect(Display *dpy, GLXContext ctx)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return False;
   return (t->IsDirect)(dpy, ctx);
}


Bool glXMakeCurrent(Display *dpy, GLXDrawable drawable, GLXContext ctx)
{
   Bool b;
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return False;
   b = (*t->MakeCurrent)(dpy, drawable, ctx);
   if (b) {
      CurrentDisplay = dpy;
      CurrentContext = ctx;
      CurrentDrawable = drawable;
      CurrentReadDrawable = drawable;
   }
   return b;
}


Bool glXQueryExtension(Display *dpy, int *errorb, int *event)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return False;
   return (t->QueryExtension)(dpy, errorb, event);
}


Bool glXQueryVersion(Display *dpy, int *maj, int *min)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return False;
   return (t->QueryVersion)(dpy, maj, min);
}


void glXSwapBuffers(Display *dpy, GLXDrawable drawable)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->SwapBuffers)(dpy, drawable);
}


void glXUseXFont(Font font, int first, int count, int listBase)
{
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return;
   (t->UseXFont)(font, first, count, listBase);
}


void glXWaitGL(void)
{
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return;
   (t->WaitGL)();
}


void glXWaitX(void)
{
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return;
   (t->WaitX)();
}



#ifdef _GLXAPI_VERSION_1_1

const char *glXGetClientString(Display *dpy, int name)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return NULL;
   return (t->GetClientString)(dpy, name);
}


const char *glXQueryExtensionsString(Display *dpy, int screen)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return NULL;
   return (t->QueryExtensionsString)(dpy, screen);
}


const char *glXQueryServerString(Display *dpy, int screen, int name)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return NULL;
   return (t->QueryServerString)(dpy, screen, name);
}

#endif



#ifdef _GLXAPI_VERSION_1_2
Display *glXGetCurrentDisplay(void)
{
   return CurrentDisplay;
}
#endif



#ifdef _GLXAPI_VERSION_1_3

GLXFBConfig glXChooseFBConfig(Display *dpy, int screen, const int *attribList, int *nitems)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->ChooseFBConfig)(dpy, screen, attribList, nitems);
}


GLXContext glXCreateNewContext(Display *dpy, GLXFBConfig config, int renderType, GLXContext shareList, Bool direct)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreateNewContext)(dpy, config, renderType, shareList, direct);
}


GLXPbuffer glXCreatePbuffer(Display *dpy, GLXFBConfig config, const int *attribList)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreatePbuffer)(dpy, config, attribList);
}


GLXPixmap glXCreatePixmap(Display *dpy, GLXFBConfig config, Pixmap pixmap, const int *attribList)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreatePixmap)(dpy, config, pixmap, attribList);
}


GLXWindow glXCreateWindow(Display *dpy, GLXFBConfig config, Window win, const int *attribList)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreateWindow)(dpy, config, win, attribList);
}


void glXDestroyPbuffer(Display *dpy, GLXPbuffer pbuf)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->DestroyPbuffer)(dpy, pbuf);
}


void glXDestroyPixmap(Display *dpy, GLXPixmap pixmap)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->DestroyPixmap)(dpy, pixmap);
}


void glXDestroyWindow(Display *dpy, GLXWindow window)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->DestroyWindow)(dpy, window);
}


GLXDrawable glXGetCurrentReadDrawable(void)
{
   return CurrentReadDrawable;
}


int glXGetFBConfigAttrib(Display *dpy, GLXFBConfig config, int attribute, int *value)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return GLX_NO_EXTENSION;
   return (t->GetFBConfigAttrib)(dpy, config, attribute, value);
}


void glXGetSelectedEvent(Display *dpy, GLXDrawable drawable, unsigned long *mask)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->GetSelectedEvent)(dpy, drawable, mask);
}


XVisualInfo *glXGetVisualFromFBConfig(Display *dpy, GLXFBConfig config)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return NULL;
   return (t->GetVisualFromFBConfig)(dpy, config);
}


Bool glXMakeContextCurrent(Display *dpy, GLXDrawable draw, GLXDrawable read, GLXContext ctx)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   Bool b;
   if (!t)
      return False;
   b = (t->MakeContextCurrent)(dpy, draw, read, ctx);
   if (b) {
      CurrentDisplay = dpy;
      CurrentContext = ctx;
      CurrentDrawable = draw;
      CurrentReadDrawable = read;
   }
   return b;
}


int glXQueryContext(Display *dpy, GLXContext ctx, int attribute, int *value)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   assert(t);
   if (!t)
      return 0; /* XXX correct? */
   return (t->QueryContext)(dpy, ctx, attribute, value);
}


void glXQueryDrawable(Display *dpy, GLXDrawable draw, int attribute, unsigned int *value)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->QueryDrawable)(dpy, draw, attribute, value);
}


void glXSelectEvent(Display *dpy, GLXDrawable drawable, unsigned long mask)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->SelectEvent)(dpy, drawable, mask);
}

#endif /* _GLXAPI_VERSION_1_3 */


#ifdef _GLXAPI_EXT_import_context

void glXFreeContextEXT(Display *dpy, GLXContext context)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->FreeContextEXT)(dpy, context);
}


GLXContextID glXGetContextIDEXT(const GLXContext context)
{
   /* XXX is this function right? */
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return 0;
   return (t->GetContextIDEXT)(context);
}


Display *glXGetCurrentDisplayEXT(void)
{
   return CurrentDisplay;
}


GLXContext glXImportContextEXT(Display *dpy, GLXContextID contextID)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->ImportContextEXT)(dpy, contextID);
}

int glXQueryContextInfoEXT(Display *dpy, GLXContext context, int attribute,int *value)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;  /* XXX ok? */
   return (t->QueryContextInfoEXT)(dpy, context, attribute, value);
}

#endif


#ifdef _GLXAPI_SGI_video_sync

int glXGetVideoSyncSGI(unsigned int *count)
{
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return 0;
   return (t->GetVideoSyncSGI)(count);
}


int glXWaitVideoSyncSGI(int divisor, int remainder, unsigned int *count)
{
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return 0;
   return (t->WaitVideoSyncSGI)(divisor, remainder, count);
}

#endif


#ifdef _GLXAPI_MESA_copy_sub_buffer

void glXCopySubBufferMESA(Display *dpy, GLXDrawable drawable, int x, int y, int width, int height)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return;
   (t->CopySubBufferMESA)(dpy, drawable, x, y, width, height);
}

#endif


#ifdef _GLXAPI_MESA_release_buffers

Bool glXReleaseBuffersMESA(Display *dpy, Window w)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return False;
   return (t->ReleaseBuffersMESA)(dpy, w);
}

#endif


#ifdef _GLXAPI_MESA_pixmap_colormap

GLXPixmap glXCreateGLXPixmapMESA(Display *dpy, XVisualInfo *visinfo, Pixmap pixmap, Colormap cmap)
{
   struct _glxapi_table *t = get_dispatch(dpy);
   if (!t)
      return 0;
   return (t->CreateGLXPixmapMESA)(dpy, visinfo, pixmap, cmap);
}

#endif


#ifdef _GLXAPI_MESA_set_3dfx_mode

GLboolean glXSet3DfxModeMESA(GLint mode)
{
   struct _glxapi_table *t = get_dispatch(CurrentDisplay);
   if (!t)
      return False;
   return (t->Set3DfxModeMESA)(mode);
}

#endif



/**********************************************************************/
/* GLX API management functions                                       */
/**********************************************************************/


const char *
_glxapi_get_version(void)
{
   return "1.3";
}


/*
 * Return array of extension strings.
 */
const char **
_glxapi_get_extensions(void)
{
   static const char *extensions[] = {
#ifdef _GLXAPI_EXT_import_context
      "GLX_EXT_import_context",
#endif
#ifdef _GLXAPI_SGI_video_sync
      "GLX_SGI_video_sync",
#endif
#ifdef _GLXAPI_MESA_copy_sub_buffer
      "GLX_MESA_copy_sub_buffer",
#endif
#ifdef _GLXAPI_MESA_release_buffers
      "GLX_MESA_release_buffers",
#endif
#ifdef _GLXAPI_MESA_pixmap_colormap
      "GLX_MESA_pixmap_colormap",
#endif
#ifdef _GLXAPI_MESA_set_3dfx_mode
      "GLX_MESA_set_3dfx_mode",
#endif
      NULL
   };
   return extensions;
}


/*
 * Return size of the GLX dispatch table, in entries, not bytes.
 */
GLuint
_glxapi_get_dispatch_table_size(void)
{
   return sizeof(struct _glxapi_table) / sizeof(void *);
}


static int
generic_no_op_func(void)
{
   return 0;
}


/*
 * Initialize all functions in given dispatch table to be no-ops
 */
void
_glxapi_set_no_op_table(struct _glxapi_table *t)
{
   GLuint n = _glxapi_get_dispatch_table_size();
   GLuint i;
   void **dispatch = (void **) t;
   for (i = 0; i < n; i++) {
      dispatch[i] = (void *) generic_no_op_func;
   }
}



struct name_address_pair {
   const char *Name;
   GLvoid *Address;
};

static struct name_address_pair GLX_functions[] = {
   { "glXChooseVisual", (GLvoid *) glXChooseVisual },
   { "glXCopyContext", (GLvoid *) glXCopyContext },
   { "glXCreateContext", (GLvoid *) glXCreateContext },
   { "glXCreateGLXPixmap", (GLvoid *) glXCreateGLXPixmap },
   { "glXDestroyContext", (GLvoid *) glXDestroyContext },
   { "glXDestroyGLXPixmap", (GLvoid *) glXDestroyGLXPixmap },
   { "glXGetConfig", (GLvoid *) glXGetConfig },
   { "glXIsDirect", (GLvoid *) glXIsDirect },
   { "glXMakeCurrent", (GLvoid *) glXMakeCurrent },
   { "glXQueryExtension", (GLvoid *) glXQueryExtension },
   { "glXQueryVersion", (GLvoid *) glXQueryVersion },
   { "glXSwapBuffers", (GLvoid *) glXSwapBuffers },
   { "glXUseXFont", (GLvoid *) glXUseXFont },
   { "glXWaitGL", (GLvoid *) glXWaitGL },
   { "glXWaitX", (GLvoid *) glXWaitX },

#ifdef _GLXAPI_VERSION_1_1
   { "glXGetClientString", (GLvoid *) glXGetClientString },
   { "glXQueryExtensionsString", (GLvoid *) glXQueryExtensionsString },
   { "glXQueryServerString", (GLvoid *) glXQueryServerString },
#endif

#ifdef _GLXAPI_VERSION_1_2
   /*{ "glXGetCurrentDisplay", (GLvoid *) glXGetCurrentDisplay },*/
#endif

#ifdef _GLXAPI_VERSION_1_3
   { "glXChooseFBConfig", (GLvoid *) glXChooseFBConfig },
   { "glXCreateNewContext", (GLvoid *) glXCreateNewContext },
   { "glXCreatePbuffer", (GLvoid *) glXCreatePbuffer },
   { "glXCreatePixmap", (GLvoid *) glXCreatePixmap },
   { "glXCreateWindow", (GLvoid *) glXCreateWindow },
   { "glXDestroyPbuffer", (GLvoid *) glXDestroyPbuffer },
   { "glXDestroyPixmap", (GLvoid *) glXDestroyPixmap },
   { "glXDestroyWindow", (GLvoid *) glXDestroyWindow },
   { "glXGetFBConfigAttrib", (GLvoid *) glXGetFBConfigAttrib },
   { "glXGetSelectedEvent", (GLvoid *) glXGetSelectedEvent },
   { "glXGetVisualFromFBConfig", (GLvoid *) glXGetVisualFromFBConfig },
   { "glXMakeContextCurrent", (GLvoid *) glXMakeContextCurrent },
   { "glXQueryContext", (GLvoid *) glXQueryContext },
   { "glXQueryDrawable", (GLvoid *) glXQueryDrawable },
   { "glXSelectEvent", (GLvoid *) glXSelectEvent },
#endif

#ifdef _GLXAPI_SGI_video_sync
   { "glXGetVideoSyncSGI", (GLvoid *) glXGetVideoSyncSGI },
   { "glXWaitVideoSyncSGI", (GLvoid *) glXWaitVideoSyncSGI },
#endif

#ifdef _GLXAPI_MESA_copy_sub_buffer
   { "glXCopySubBufferMESA", (GLvoid *) glXCopySubBufferMESA },
#endif

#ifdef _GLXAPI_MESA_release_buffers
   { "glXReleaseBuffersMESA", (GLvoid *) glXReleaseBuffersMESA },
#endif

#ifdef _GLXAPI_MESA_pixmap_colormap
   { "glXCreateGLXPixmapMESA", (GLvoid *) glXCreateGLXPixmapMESA },
#endif

#ifdef _GLXAPI_MESA_set_3dfx_mode
   { "glXSet3DfxModeMESA", (GLvoid *) glXSet3DfxModeMESA },
#endif

   { NULL, NULL }   /* end of list */
};



/*
 * Return address of named glX function, or NULL if not found.
 */
const GLvoid *
_glxapi_get_proc_address(const char *funcName)
{
   GLuint i;
   for (i = 0; GLX_functions[i].Name; i++) {
      if (strcmp(GLX_functions[i].Name, funcName) == 0)
         return GLX_functions[i].Address;
   }
   return NULL;
}
