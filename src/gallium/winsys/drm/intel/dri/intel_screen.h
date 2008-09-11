/**************************************************************************
 *
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef _INTEL_SCREEN_H_
#define _INTEL_SCREEN_H_

#include "dri_util.h"
#include "i830_common.h"
#include "xmlconfig.h"
#include "ws_dri_bufpool.h"

#include "pipe/p_compiler.h"

#include "intel_be_device.h"

struct intel_screen
{
   struct intel_be_device base;

   struct {
      drm_handle_t handle;

      /* We create a static dri buffer for the frontbuffer.
       */
      struct _DriBufferObject *buffer;
      struct pipe_surface *surface;
      struct pipe_texture *texture;

      char *map;                   /* memory map */
      int offset;                  /* from start of video mem, in bytes */
      int pitch;                   /* row stride, in bytes */
      int width;
      int height;
      int size;
      int cpp;                     /* for front and back buffers */
   } front;

   int deviceID;
   int drmMinor;

   drmI830Sarea *sarea;

   /**
   * Configuration cache with default values for all contexts
   */
   driOptionCache optionCache;

   boolean havePools;

   /**
    * Temporary(?) context to use for SwapBuffers or other situations in
    * which we need a rendering context, but none is currently bound.
    */
   struct intel_context *dummyContext;

   /*
    * New stuff form the i915tex integration
    */
   unsigned batch_id;


   struct pipe_winsys *winsys;
};



/** cast wrapper */
static INLINE struct intel_screen *
intel_screen(__DRIscreenPrivate *sPriv)
{
   return (struct intel_screen *) sPriv->private;
}


extern void
intelUpdateScreenRotation(__DRIscreenPrivate * sPriv, drmI830Sarea * sarea);


extern void intelDestroyContext(__DRIcontextPrivate * driContextPriv);

extern boolean intelUnbindContext(__DRIcontextPrivate * driContextPriv);

extern boolean
intelMakeCurrent(__DRIcontextPrivate * driContextPriv,
                 __DRIdrawablePrivate * driDrawPriv,
                 __DRIdrawablePrivate * driReadPriv);


extern boolean
intelCreatePools(__DRIscreenPrivate *sPriv);

extern boolean
intelCreateContext(const __GLcontextModes * visual,
                   __DRIcontextPrivate * driContextPriv,
                   void *sharedContextPrivate);


#endif
