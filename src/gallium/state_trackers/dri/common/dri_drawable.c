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
#include "dri_context.h"
#include "dri_drawable.h"
#include "dri_st_api.h"
#include "dri1_helper.h"

#include "pipe/p_screen.h"
#include "util/u_format.h"
#include "util/u_memory.h"
 
/**
 * This is called when we need to set up GL rendering to a new X window.
 */
boolean
dri_create_buffer(__DRIscreen * sPriv,
		  __DRIdrawable * dPriv,
		  const __GLcontextModes * visual, boolean isPixmap)
{
   struct dri_screen *screen = sPriv->private;
   struct dri_drawable *drawable = NULL;

   if (isPixmap)
      goto fail;		       /* not implemented */

   drawable = CALLOC_STRUCT(dri_drawable);
   if (drawable == NULL)
      goto fail;

   dri_fill_st_visual(&drawable->stvis, screen, visual);
   drawable->stfb = dri_create_st_framebuffer(drawable);
   if (drawable->stfb == NULL)
      goto fail;

   drawable->sPriv = sPriv;
   drawable->dPriv = dPriv;
   dPriv->driverPrivate = (void *)drawable;

   drawable->desired_fences = 2;

   return GL_TRUE;
fail:
   FREE(drawable);
   return GL_FALSE;
}

void
dri_destroy_buffer(__DRIdrawable * dPriv)
{
   struct dri_drawable *drawable = dri_drawable(dPriv);

   dri1_swap_fences_clear(drawable);

   dri1_destroy_pipe_surface(drawable);

   dri_destroy_st_framebuffer(drawable->stfb);

   drawable->desired_fences = 0;

   FREE(drawable);
}

/* vim: set sw=3 ts=8 sts=3 expandtab: */
