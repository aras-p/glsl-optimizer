/**************************************************************************
 * 
 * Copyright 2009 Younes Manton.
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

#include <assert.h>
#include <X11/Xlibint.h>
#include <X11/extensions/XvMClib.h>
#include <pipe/p_screen.h>
#include <pipe/p_video_context.h>
#include <pipe/p_video_state.h>
#include <pipe/p_state.h>
#include <vl_winsys.h>
#include <util/u_memory.h>
#include <util/u_debug.h>
#include <vl/vl_csc.h>
#include "xvmc_private.h"

static Status Validate(Display *dpy, XvPortID port, int surface_type_id,
                       unsigned int width, unsigned int height, int flags,
                       bool *found_port, int *screen, int *chroma_format,
                       int *mc_type, int *surface_flags)
{
   bool found_surface = false;
   XvAdaptorInfo *adaptor_info;
   unsigned int num_adaptors;
   int num_types;
   unsigned int max_width, max_height;
   Status ret;

   assert(dpy);
   assert(found_port);
   assert(screen);
   assert(chroma_format);
   assert(mc_type);
   assert(surface_flags);

   *found_port = false;

   for (unsigned int i = 0; i < XScreenCount(dpy); ++i) {
      ret = XvQueryAdaptors(dpy, XRootWindow(dpy, i), &num_adaptors, &adaptor_info);
      if (ret != Success)
         return ret;

      for (unsigned int j = 0; j < num_adaptors && !*found_port; ++j) {
         for (unsigned int k = 0; k < adaptor_info[j].num_ports && !*found_port; ++k) {
            XvMCSurfaceInfo *surface_info;

            if (adaptor_info[j].base_id + k != port)
               continue;

            *found_port = true;

            surface_info = XvMCListSurfaceTypes(dpy, adaptor_info[j].base_id, &num_types);
            if (!surface_info) {
               XvFreeAdaptorInfo(adaptor_info);
               return BadAlloc;
            }

            for (unsigned int l = 0; l < num_types && !found_surface; ++l) {
               if (surface_info[l].surface_type_id != surface_type_id)
                  continue;

               found_surface = true;
               max_width = surface_info[l].max_width;
               max_height = surface_info[l].max_height;
               *chroma_format = surface_info[l].chroma_format;
               *mc_type = surface_info[l].mc_type;
               *surface_flags = surface_info[l].flags;
               *screen = i;
            }

            XFree(surface_info);
         }
      }

      XvFreeAdaptorInfo(adaptor_info);
   }

   if (!*found_port)
      return XvBadPort;
   if (!found_surface)
      return BadMatch;
   if (width > max_width || height > max_height)
      return BadValue;
   if (flags != XVMC_DIRECT && flags != 0)
      return BadValue;

   return Success;
}

static enum pipe_video_profile ProfileToPipe(int xvmc_profile)
{
   if (xvmc_profile & XVMC_MPEG_1)
      assert(0);
   if (xvmc_profile & XVMC_MPEG_2)
      return PIPE_VIDEO_PROFILE_MPEG2_MAIN;
   if (xvmc_profile & XVMC_H263)
      assert(0);
   if (xvmc_profile & XVMC_MPEG_4)
      assert(0);
	
   assert(0);

   return -1;
}

static enum pipe_video_chroma_format FormatToPipe(int xvmc_format)
{
   switch (xvmc_format) {
      case XVMC_CHROMA_FORMAT_420:
         return PIPE_VIDEO_CHROMA_FORMAT_420;
      case XVMC_CHROMA_FORMAT_422:
         return PIPE_VIDEO_CHROMA_FORMAT_422;
      case XVMC_CHROMA_FORMAT_444:
         return PIPE_VIDEO_CHROMA_FORMAT_444;
      default:
         assert(0);
   }

   return -1;
}

Status XvMCCreateContext(Display *dpy, XvPortID port, int surface_type_id,
                         int width, int height, int flags, XvMCContext *context)
{
   bool found_port;
   int scrn;
   int chroma_format;
   int mc_type;
   int surface_flags;
   Status ret;
   struct pipe_screen *screen;
   struct pipe_video_context *vpipe;
   XvMCContextPrivate *context_priv;
   float csc[16];

   assert(dpy);

   if (!context)
      return XvMCBadContext;

   ret = Validate(dpy, port, surface_type_id, width, height, flags,
                  &found_port, &scrn, &chroma_format, &mc_type, &surface_flags);

   /* Success and XvBadPort have the same value */
   if (ret != Success || !found_port)
      return ret;

   /* XXX: Current limits */
   if (chroma_format != XVMC_CHROMA_FORMAT_420) {
      debug_printf("[XvMCg3dvl] Cannot decode requested surface type. Unsupported chroma format.\n");
      return BadImplementation;
   }
   if (mc_type != (XVMC_MOCOMP | XVMC_MPEG_2)) {
      debug_printf("[XvMCg3dvl] Cannot decode requested surface type. Non-MPEG2/Mocomp acceleration unsupported.\n");
      return BadImplementation;
   }
   if (!(surface_flags & XVMC_INTRA_UNSIGNED)) {
      debug_printf("[XvMCg3dvl] Cannot decode requested surface type. Signed intra unsupported.\n");
      return BadImplementation;
   }

   context_priv = CALLOC(1, sizeof(XvMCContextPrivate));
   if (!context_priv)
      return BadAlloc;

   /* TODO: Reuse screen if process creates another context */
   screen = vl_screen_create(dpy, scrn);

   if (!screen) {
      FREE(context_priv);
      return BadAlloc;
   }

   vpipe = vl_video_create(dpy, scrn, screen, ProfileToPipe(mc_type),
                           FormatToPipe(chroma_format), width, height);

   if (!vpipe) {
      screen->destroy(screen);
      FREE(context_priv);
      return BadAlloc;
   }

   /* TODO: Define some Xv attribs to allow users to specify color standard, procamp */
   vl_csc_get_matrix
   (
      debug_get_bool_option("G3DVL_NO_CSC", FALSE) ?
      VL_CSC_COLOR_STANDARD_IDENTITY : VL_CSC_COLOR_STANDARD_BT_601,
      NULL, true, csc
   );
   vpipe->set_csc_matrix(vpipe, csc);

   context_priv->vpipe = vpipe;

   context->context_id = XAllocID(dpy);
   context->surface_type_id = surface_type_id;
   context->width = width;
   context->height = height;
   context->flags = flags;
   context->port = port;
   context->privData = context_priv;
	
   SyncHandle();

   return Success;
}

Status XvMCDestroyContext(Display *dpy, XvMCContext *context)
{
   struct pipe_screen *screen;
   struct pipe_video_context *vpipe;
   XvMCContextPrivate *context_priv;

   assert(dpy);

   if (!context || !context->privData)
      return XvMCBadContext;

   context_priv = context->privData;
   vpipe = context_priv->vpipe;
   pipe_surface_reference(&context_priv->backbuffer, NULL);
   screen = vpipe->screen;
   vpipe->destroy(vpipe);
   screen->destroy(screen);
   FREE(context_priv);
   context->privData = NULL;

   return Success;
}
