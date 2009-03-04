/**************************************************************************
 *
 * Copyright 2009, VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/
/*
 * Author: Keith Whitwell <keithw@vmware.com>
 * Author: Jakob Bornecrantz <wallbraker@gmail.com>
 */

#include "dri_screen.h"

#include "dri_drawable.h"


#include "state_tracker/drm_api.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "pipe/p_context.h"

#include "dri_context.h"

#include "util/u_memory.h"


GLboolean
dri_create_context(const __GLcontextModes *visual,
                   __DRIcontextPrivate *cPriv,
                   void *sharedContextPrivate)
{
   __DRIscreenPrivate *sPriv = cPriv->driScreenPriv;
   struct dri_screen *screen = dri_screen(sPriv);
   struct dri_context *ctx = NULL;
   struct st_context *st_share = NULL;

   if (sharedContextPrivate) {
      st_share = ((struct dri_context *) sharedContextPrivate)->st;
   }

   ctx = CALLOC_STRUCT(dri_context);
   if (ctx == NULL)
      goto fail;

   cPriv->driverPrivate = ctx;
   ctx->cPriv = cPriv;
   ctx->sPriv = sPriv;

   driParseConfigFiles(&ctx->optionCache,
                       &screen->optionCache,
                       sPriv->myNum,
                       "dri");

   ctx->pipe = drm_api_hooks.create_context(screen->pipe_screen);

   if (ctx->pipe == NULL)
      goto fail;

   /* used in dri_flush_frontbuffer */
   ctx->pipe->priv = ctx;

   ctx->st = st_create_context(ctx->pipe, visual, st_share);
   if (ctx->st == NULL)
      goto fail;

   dri_init_extensions(ctx);

   return GL_TRUE;

fail:
   if (ctx && ctx->st)
      st_destroy_context(ctx->st);

   if (ctx && ctx->pipe)
      ctx->pipe->destroy(ctx->pipe);

   FREE(ctx);
   return FALSE;
}


void
dri_destroy_context(__DRIcontextPrivate *cPriv)
{
   struct dri_context *ctx = dri_context(cPriv);
   struct dri_screen *screen = dri_screen(cPriv->driScreenPriv);

   /* No particular reason to wait for command completion before
    * destroying a context, but it is probably worthwhile flushing it
    * to avoid having to add code elsewhere to cope with flushing a
    * partially destroyed context.
    */
   st_flush(ctx->st, 0, NULL);

   if (screen->dummyContext == ctx)
      screen->dummyContext = NULL;

   /* Also frees ctx->pipe?
    */
   st_destroy_context(ctx->st);

   FREE(ctx);
}


GLboolean
dri_unbind_context(__DRIcontextPrivate *cPriv)
{
   struct dri_context *ctx = dri_context(cPriv);
   st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
   /* XXX make_current(NULL)? */
   return GL_TRUE;
}


GLboolean
dri_make_current(__DRIcontextPrivate *cPriv,
                 __DRIdrawablePrivate *driDrawPriv,
                 __DRIdrawablePrivate *driReadPriv)
{
   if (cPriv) {
      struct dri_context *ctx = dri_context(cPriv);
      struct dri_screen *screen = dri_screen(cPriv->driScreenPriv);
      struct dri_drawable *draw = dri_drawable(driDrawPriv);
      struct dri_drawable *read = dri_drawable(driReadPriv);

      /* This is for situations in which we need a rendering context but
       * there may not be any currently bound.
       */
      screen->dummyContext = ctx;

      st_make_current(ctx->st,
                      draw->stfb,
                      read->stfb);

      /* used in dri_flush_frontbuffer */
      ctx->dPriv = driDrawPriv;

      if (driDrawPriv)
         dri_get_buffers(driDrawPriv);
      if (driDrawPriv != driReadPriv && driReadPriv)
         dri_get_buffers(driReadPriv);
   } else {
      st_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
