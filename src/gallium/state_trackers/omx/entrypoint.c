/**************************************************************************
 *
 * Copyright 2013 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/*
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 *
 */

#include <assert.h>
#include <string.h>

#include <X11/Xlib.h>

#include "os/os_thread.h"
#include "util/u_memory.h"

#include "entrypoint.h"
#include "vid_dec.h"
#include "vid_enc.h"

pipe_static_mutex(omx_lock);
static Display *omx_display = NULL;
static struct vl_screen *omx_screen = NULL;
static unsigned omx_usecount = 0;

int omx_component_library_Setup(stLoaderComponentType **stComponents)
{
   OMX_ERRORTYPE r;
   unsigned i = 0;

   if (stComponents == NULL)
      return 2;

   /* component 0 - video decoder */
   r = vid_dec_LoaderComponent(stComponents[i]);
   if (r == OMX_ErrorNone)
      ++i;

   /* component 1 - video encoder */
   r = vid_enc_LoaderComponent(stComponents[i]);
   if (r == OMX_ErrorNone)
      ++i;

   return i;
}

struct vl_screen *omx_get_screen(void)
{
   pipe_mutex_lock(omx_lock);

   if (!omx_display) {
      omx_display = XOpenDisplay(NULL);
      if (!omx_display) {
         pipe_mutex_unlock(omx_lock);
         return NULL;
      }
   }

   if (!omx_screen) {
      omx_screen = vl_screen_create(omx_display, 0);
      if (!omx_screen) {
         pipe_mutex_unlock(omx_lock);
         return NULL;
      }
   }

   ++omx_usecount;

   pipe_mutex_unlock(omx_lock);
   return omx_screen;
}

void omx_put_screen(void)
{
   pipe_mutex_lock(omx_lock);
   if ((--omx_usecount) == 0) {
      vl_screen_destroy(omx_screen);
      XCloseDisplay(omx_display);
      omx_screen = NULL;
      omx_display = NULL;
   }
   pipe_mutex_unlock(omx_lock);
}

OMX_ERRORTYPE omx_workaround_Destructor(OMX_COMPONENTTYPE *comp)
{
   omx_base_component_PrivateType* priv = (omx_base_component_PrivateType*)comp->pComponentPrivate;

   priv->state = OMX_StateInvalid;
   tsem_up(priv->messageSem);

   /* wait for thread to exit */;
   pthread_join(priv->messageHandlerThread, NULL);

   return omx_base_component_Destructor(comp);
}
