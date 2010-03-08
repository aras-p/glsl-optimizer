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
#include "dri_context.h"
#include "dri_st_api.h"
#include "dri1.h"

#include "pipe/p_context.h"
#include "util/u_memory.h"

GLboolean
dri_create_context(const __GLcontextModes * visual,
		   __DRIcontext * cPriv, void *sharedContextPrivate)
{
   struct st_api *stapi = dri_get_st_api();
   __DRIscreen *sPriv = cPriv->driScreenPriv;
   struct dri_screen *screen = dri_screen(sPriv);
   struct dri_context *ctx = NULL;
   struct st_context_iface *st_share = NULL;
   struct st_visual stvis;

   if (sharedContextPrivate) {
      st_share = ((struct dri_context *)sharedContextPrivate)->st;
   }

   ctx = CALLOC_STRUCT(dri_context);
   if (ctx == NULL)
      goto fail;

   cPriv->driverPrivate = ctx;
   ctx->cPriv = cPriv;
   ctx->sPriv = sPriv;
   ctx->lock = screen->drmLock;

   driParseConfigFiles(&ctx->optionCache,
		       &screen->optionCache, sPriv->myNum, "dri");

   dri_fill_st_visual(&stvis, screen, visual);
   ctx->st = stapi->create_context(stapi, screen->smapi, &stvis, st_share);
   if (ctx->st == NULL)
      goto fail;
   ctx->st->st_manager_private = (void *) ctx;

   dri_init_extensions(ctx);

   return GL_TRUE;

 fail:
   if (ctx && ctx->st)
      ctx->st->destroy(ctx->st);

   FREE(ctx);
   return FALSE;
}

void
dri_destroy_context(__DRIcontext * cPriv)
{
   struct dri_context *ctx = dri_context(cPriv);

   /* note: we are freeing values and nothing more because
    * driParseConfigFiles allocated values only - the rest
    * is owned by screen optionCache.
    */
   FREE(ctx->optionCache.values);

   /* No particular reason to wait for command completion before
    * destroying a context, but it is probably worthwhile flushing it
    * to avoid having to add code elsewhere to cope with flushing a
    * partially destroyed context.
    */
   ctx->st->flush(ctx->st, 0, NULL);
   ctx->st->destroy(ctx->st);

   FREE(ctx);
}

GLboolean
dri_unbind_context(__DRIcontext * cPriv)
{
   struct st_api *stapi = dri_get_st_api();

   if (cPriv) {
      struct dri_context *ctx = dri_context(cPriv);

      if (--ctx->bind_count == 0) {
         if (ctx->st == stapi->get_current(stapi)) {
            ctx->st->flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
            stapi->make_current(stapi, NULL, NULL, NULL);
         }
      }
   }

   return GL_TRUE;
}

GLboolean
dri_make_current(__DRIcontext * cPriv,
		 __DRIdrawable * driDrawPriv,
		 __DRIdrawable * driReadPriv)
{
   struct st_api *stapi = dri_get_st_api();

   if (cPriv) {
      struct dri_context *ctx = dri_context(cPriv);
      struct dri_drawable *draw = dri_drawable(driDrawPriv);
      struct dri_drawable *read = dri_drawable(driReadPriv);
      struct st_context_iface *old_st;

      old_st = stapi->get_current(stapi);
      if (old_st && old_st != ctx->st)
	 ctx->st->flush(old_st, PIPE_FLUSH_RENDER_CACHE, NULL);

      ++ctx->bind_count;

      if (ctx->dPriv != driDrawPriv) {
	 ctx->dPriv = driDrawPriv;
         draw->texture_stamp = driDrawPriv->lastStamp - 1;
      }
      if (ctx->rPriv != driReadPriv) {
	 ctx->rPriv = driReadPriv;
         draw->texture_stamp = driReadPriv->lastStamp - 1;
      }

      stapi->make_current(stapi, ctx->st, draw->stfb, read->stfb);
   }
   else {
      stapi->make_current(stapi, NULL, NULL, NULL);
   }

   return GL_TRUE;
}

struct dri_context *
dri_get_current(void)
{
   struct st_api *stapi = dri_get_st_api();
   struct st_context_iface *st;

   st = stapi->get_current(stapi);

   return (struct dri_context *) (st) ? st->st_manager_private : NULL;
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
