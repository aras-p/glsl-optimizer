/*
 * Mesa 3-D graphics library
 * Version:  7.10
 *
 * Copyright (C) 2011 LunarG Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */
#include "util/u_debug.h"
#include "state_tracker/st_api.h"
#include "egl_st.h"

/* for st/mesa */
#include "state_tracker/st_gl_api.h"
/* for st/vega */
#include "vg_api.h"

static struct st_api *
st_GL_create_api(void)
{
#if FEATURE_GL || FEATURE_ES1 || FEATURE_ES2
   return st_gl_api_create();
#else
   return NULL;
#endif
}

static struct st_api *
st_OpenVG_create_api(void)
{
#if FEATURE_VG
   return (struct st_api *) vg_api_get();
#else
   return NULL;
#endif
}

struct st_api *
egl_st_create_api(enum st_api_type api)
{
   struct st_api *stapi;

   switch (api) {
   case ST_API_OPENGL:
      stapi = st_GL_create_api();
      break;
   case ST_API_OPENVG:
      stapi = st_OpenVG_create_api();
      break;
   default:
      assert(!"Unknown API Type\n");
      stapi = NULL;
      break;
   }

   return stapi;
}

uint
egl_st_get_profile_mask(enum st_api_type api)
{
   uint mask = 0x0;

   switch (api) {
   case ST_API_OPENGL:
#if FEATURE_GL
      mask |= ST_PROFILE_DEFAULT_MASK;
#endif
#if FEATURE_ES1
      mask |= ST_PROFILE_OPENGL_ES1_MASK;
#endif
#if FEATURE_ES2
      mask |= ST_PROFILE_OPENGL_ES2_MASK;
#endif
      break;
   case ST_API_OPENVG:
#if FEATURE_VG
      mask |= ST_PROFILE_DEFAULT_MASK;
#endif
      break;
   default:
      break;
   }

   return mask;
}
