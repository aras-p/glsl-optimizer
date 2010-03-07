/**************************************************************************
 *
 * Copyright (C) 2009 VMware, Inc.
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

#ifndef DRI_CONTEXT_H
#define DRI_CONTEXT_H

#include "pipe/p_compiler.h"
#include "drm.h"
#include "dri_util.h"

struct pipe_context;
struct pipe_fence;
struct st_context;
struct dri_drawable;

struct dri_context
{
   /* dri */
   __DRIscreen *sPriv;
   __DRIcontext *cPriv;
   __DRIdrawable *dPriv;
   __DRIdrawable *rPriv;

   driOptionCache optionCache;

   unsigned int d_stamp;
   unsigned int r_stamp;

   drmLock *lock;
   boolean isLocked;
   boolean stLostLock;
   boolean wsLostLock;

   unsigned int bind_count;

   /* gallium */
   struct st_context *st;
   struct pipe_context *pipe;
};

static INLINE struct dri_context *
dri_context(__DRIcontext * driContextPriv)
{
   return (struct dri_context *)driContextPriv->driverPrivate;
}

static INLINE void
dri_lock(struct dri_context *ctx)
{
   drm_context_t hw_context = ctx->cPriv->hHWContext;
   char ret = 0;

   DRM_CAS(ctx->lock, hw_context, DRM_LOCK_HELD | hw_context, ret);
   if (ret) {
      drmGetLock(ctx->sPriv->fd, hw_context, 0);
      ctx->stLostLock = TRUE;
      ctx->wsLostLock = TRUE;
   }
   ctx->isLocked = TRUE;
}

static INLINE void
dri_unlock(struct dri_context *ctx)
{
   ctx->isLocked = FALSE;
   DRM_UNLOCK(ctx->sPriv->fd, ctx->lock, ctx->cPriv->hHWContext);
}

/***********************************************************************
 * dri_context.c
 */
extern struct dri1_api_lock_funcs dri1_lf;

void dri_destroy_context(__DRIcontext * driContextPriv);

boolean dri_unbind_context(__DRIcontext * driContextPriv);

boolean
dri_make_current(__DRIcontext * driContextPriv,
		 __DRIdrawable * driDrawPriv,
		 __DRIdrawable * driReadPriv);

boolean
dri_create_context(const __GLcontextModes * visual,
		   __DRIcontext * driContextPriv,
		   void *sharedContextPrivate);

/***********************************************************************
 * dri_extensions.c
 */
void dri_init_extensions(struct dri_context *ctx);

#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
