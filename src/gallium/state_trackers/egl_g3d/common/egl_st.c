/*
 * Mesa 3-D graphics library
 * Version:  7.8
 *
 * Copyright (C) 2009-2010 Chia-I Wu <olv@0xlab.org>
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

#include <dlfcn.h>
#include "pipe/p_compiler.h"
#include "util/u_memory.h"
#include "egllog.h"
#include "EGL/egl.h" /* for EGL_api_BIT */

#include "egl_st.h"

#ifndef HAVE_DLADDR
#define HAVE_DLADDR 1
#endif

#if HAVE_DLADDR

static const char *
egl_g3d_st_names[] = {
#define ST_PUBLIC(name, ...) #name,
#include "st_public_tmp.h"
   NULL
};

static boolean
egl_g3d_fill_st(struct egl_g3d_st *stapi, void *sym)
{
   st_proc *procs = (st_proc *) stapi;
   void *handle;
   Dl_info info;
   const char **name;

   if (!dladdr(sym, &info))
      return FALSE;
   handle = dlopen(info.dli_fname, RTLD_LAZY | RTLD_LOCAL | RTLD_NODELETE);
   if (!handle)
      return FALSE;

   for (name = egl_g3d_st_names; *name; name++) {
      st_proc proc = (st_proc) dlsym(handle, *name);
      if (!proc) {
         _eglLog(_EGL_WARNING, "%s is missing in %s", *name, info.dli_fname);
         memset(stapi, 0, sizeof(*stapi));
         dlclose(handle);
         return FALSE;
      }
      *procs++ = proc;
   }

   dlclose(handle);
   return TRUE;
}

#else /* HAVE_DLADDR */

static boolean
egl_g3d_fill_st(struct egl_g3d_st *stapi, void *sym)
{
#define ST_PUBLIC(name, ...) stapi->name = name;
#include "st_public_tmp.h"
   return TRUE;
}

#endif /* HAVE_DLADDR */

static boolean
egl_g3d_init_st(struct egl_g3d_st *stapi, const char *api)
{
   void *handle, *sym;
   boolean res = FALSE;

   /* already initialized */
   if (stapi->st_notify_swapbuffers != NULL)
      return TRUE;

   handle = dlopen(NULL, RTLD_LAZY | RTLD_LOCAL);
   if (!handle)
      return FALSE;

   sym = dlsym(handle, api);
   if (sym && egl_g3d_fill_st(stapi, sym))
      res = TRUE;

   dlclose(handle);
   return res;
}

static struct {
   const char *symbol;
   EGLint api_bit;
} egl_g3d_st_info[NUM_EGL_G3D_STS] = {
   { "st_api_OpenGL_ES1",  EGL_OPENGL_ES_BIT },
   { "st_api_OpenVG",      EGL_OPENVG_BIT },
   { "st_api_OpenGL_ES2",  EGL_OPENGL_ES2_BIT },
   { "st_api_OpenGL",      EGL_OPENGL_BIT },
};

const struct egl_g3d_st *
egl_g3d_get_st(enum egl_g3d_st_api api)
{
   static struct egl_g3d_st all_trackers[NUM_EGL_G3D_STS];

   if (egl_g3d_init_st(&all_trackers[api], egl_g3d_st_info[api].symbol)) {
      all_trackers[api].api_bit = egl_g3d_st_info[api].api_bit;
      return &all_trackers[api];
   }
   else {
      return NULL;
   }
}
