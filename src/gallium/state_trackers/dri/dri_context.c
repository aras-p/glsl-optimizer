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
#include "state_tracker/dri1_api.h"
#include "state_tracker/st_public.h"
#include "state_tracker/st_context.h"
#include "pipe/p_context.h"

#include "dri_context.h"

#include "util/u_memory.h"

GLboolean
dri_create_context(const __GLcontextModes * visual,
		   __DRIcontext * cPriv, void *sharedContextPrivate)
{
   __DRIscreen *sPriv = cPriv->driScreenPriv;
   struct dri_screen *screen = dri_screen(sPriv);
   struct dri_context *ctx = NULL;
   struct st_context *st_share = NULL;

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
   ctx->d_stamp = -1;
   ctx->r_stamp = -1;

   driParseConfigFiles(&ctx->optionCache,
		       &screen->optionCache, sPriv->myNum, "dri");

   ctx->pipe = screen->pipe_screen->context_create( screen->pipe_screen,
						    ctx );

   if (ctx->pipe == NULL)
      goto fail;

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
   st_flush(ctx->st, 0, NULL);

   /* Also frees ctx->pipe?
    */
   st_destroy_context(ctx->st);

   FREE(ctx);
}

GLboolean
dri_unbind_context(__DRIcontext * cPriv)
{
   if (cPriv) {
      struct dri_context *ctx = dri_context(cPriv);

      if (--ctx->bind_count == 0) {
	 if (ctx->st && ctx->st == st_get_current()) {
	    st_flush(ctx->st, PIPE_FLUSH_RENDER_CACHE, NULL);
	    st_make_current(NULL, NULL, NULL);
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
   if (cPriv) {
      struct dri_context *ctx = dri_context(cPriv);
      struct dri_drawable *draw = dri_drawable(driDrawPriv);
      struct dri_drawable *read = dri_drawable(driReadPriv);
      struct st_context *old_st = st_get_current();

      if (old_st && old_st != ctx->st)
	 st_flush(old_st, PIPE_FLUSH_RENDER_CACHE, NULL);

      ++ctx->bind_count;

      if (ctx->dPriv != driDrawPriv) {
	 ctx->dPriv = driDrawPriv;
	 ctx->d_stamp = driDrawPriv->lastStamp - 1;
      }
      if (ctx->rPriv != driReadPriv) {
	 ctx->rPriv = driReadPriv;
	 ctx->r_stamp = driReadPriv->lastStamp - 1;
      }

      st_make_current(ctx->st, draw->stfb, read->stfb);

      if (__dri1_api_hooks) {
	 dri1_update_drawables(ctx, draw, read);
      } else {
	 dri_update_buffer(ctx->pipe->screen,
			   ctx->pipe->priv);
      }
   } else {
      st_make_current(NULL, NULL, NULL);
   }

   return GL_TRUE;
}

static void
st_dri_lock(struct pipe_context *pipe)
{
   dri_lock((struct dri_context *)pipe->priv);
}

static void
st_dri_unlock(struct pipe_context *pipe)
{
   dri_unlock((struct dri_context *)pipe->priv);
}

static boolean
st_dri_is_locked(struct pipe_context *pipe)
{
   return ((struct dri_context *)pipe->priv)->isLocked;
}

static boolean
st_dri_lost_lock(struct pipe_context *pipe)
{
   return ((struct dri_context *)pipe->priv)->wsLostLock;
}

static void
st_dri_clear_lost_lock(struct pipe_context *pipe)
{
   ((struct dri_context *)pipe->priv)->wsLostLock = FALSE;
}

struct dri1_api_lock_funcs dri1_lf = {
   .lock = st_dri_lock,
   .unlock = st_dri_unlock,
   .is_locked = st_dri_is_locked,
   .is_lock_lost = st_dri_lost_lock,
   .clear_lost_lock = st_dri_clear_lost_lock
};

/* vim: set sw=3 ts=8 sts=3 expandtab: */
