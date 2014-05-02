/*
 * Copyright © 2013 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#if defined(GLX_DIRECT_RENDERING) && !defined(GLX_USE_APPLEGL)

#include "glxclient.h"
#include "glx_error.h"
#include "dri2.h"
#include "dri_interface.h"
#include "dri2_priv.h"
#if defined(HAVE_DRI3)
#include "dri3_priv.h"
#endif

static int
dri2_convert_glx_query_renderer_attribs(int attribute)
{
   switch (attribute) {
   case GLX_RENDERER_VENDOR_ID_MESA:
      return __DRI2_RENDERER_VENDOR_ID;
   case GLX_RENDERER_DEVICE_ID_MESA:
      return __DRI2_RENDERER_DEVICE_ID;
   case GLX_RENDERER_VERSION_MESA:
      return __DRI2_RENDERER_VERSION;
   case GLX_RENDERER_ACCELERATED_MESA:
      return __DRI2_RENDERER_ACCELERATED;
   case GLX_RENDERER_VIDEO_MEMORY_MESA:
      return __DRI2_RENDERER_VIDEO_MEMORY;
   case GLX_RENDERER_UNIFIED_MEMORY_ARCHITECTURE_MESA:
      return __DRI2_RENDERER_UNIFIED_MEMORY_ARCHITECTURE;
   case GLX_RENDERER_PREFERRED_PROFILE_MESA:
      return __DRI2_RENDERER_PREFERRED_PROFILE;
   case GLX_RENDERER_OPENGL_CORE_PROFILE_VERSION_MESA:
      return __DRI2_RENDERER_OPENGL_CORE_PROFILE_VERSION;
   case GLX_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION_MESA:
      return __DRI2_RENDERER_OPENGL_COMPATIBILITY_PROFILE_VERSION;
   case GLX_RENDERER_OPENGL_ES_PROFILE_VERSION_MESA:
      return __DRI2_RENDERER_OPENGL_ES_PROFILE_VERSION;
   case GLX_RENDERER_OPENGL_ES2_PROFILE_VERSION_MESA:
      return __DRI2_RENDERER_OPENGL_ES2_PROFILE_VERSION;
   default:
      return -1;
   }
}

_X_HIDDEN int
dri2_query_renderer_integer(struct glx_screen *base, int attribute,
                            unsigned int *value)
{
   struct dri2_screen *const psc = (struct dri2_screen *) base;

   /* Even though there are invalid values (and
    * dri2_convert_glx_query_renderer_attribs may return -1), the higher level
    * GLX code is required to perform the filtering.  Assume that we got a
    * good value.
    */
   const int dri_attribute = dri2_convert_glx_query_renderer_attribs(attribute);

   if (psc->rendererQuery == NULL)
      return -1;

   return psc->rendererQuery->queryInteger(psc->driScreen, dri_attribute,
                                           value);
}

_X_HIDDEN int
dri2_query_renderer_string(struct glx_screen *base, int attribute,
                           const char **value)
{
   struct dri2_screen *const psc = (struct dri2_screen *) base;

   /* Even though queryString only accepts a subset of the possible GLX
    * queries, the higher level GLX code is required to perform the filtering.
    * Assume that we got a good value.
    */
   const int dri_attribute = dri2_convert_glx_query_renderer_attribs(attribute);

   if (psc->rendererQuery == NULL)
      return -1;

   return psc->rendererQuery->queryString(psc->driScreen, dri_attribute, value);
}

#if defined(HAVE_DRI3)
_X_HIDDEN int
dri3_query_renderer_integer(struct glx_screen *base, int attribute,
                            unsigned int *value)
{
   struct dri3_screen *const psc = (struct dri3_screen *) base;

   /* Even though there are invalid values (and
    * dri2_convert_glx_query_renderer_attribs may return -1), the higher level
    * GLX code is required to perform the filtering.  Assume that we got a
    * good value.
    */
   const int dri_attribute = dri2_convert_glx_query_renderer_attribs(attribute);

   if (psc->rendererQuery == NULL)
      return -1;

   return psc->rendererQuery->queryInteger(psc->driScreen, dri_attribute,
                                           value);
}

_X_HIDDEN int
dri3_query_renderer_string(struct glx_screen *base, int attribute,
                           const char **value)
{
   struct dri3_screen *const psc = (struct dri3_screen *) base;

   /* Even though queryString only accepts a subset of the possible GLX
    * queries, the higher level GLX code is required to perform the filtering.
    * Assume that we got a good value.
    */
   const int dri_attribute = dri2_convert_glx_query_renderer_attribs(attribute);

   if (psc->rendererQuery == NULL)
      return -1;

   return psc->rendererQuery->queryString(psc->driScreen, dri_attribute, value);
}
#endif /* HAVE_DRI3 */

#endif /* GLX_DIRECT_RENDERING */
