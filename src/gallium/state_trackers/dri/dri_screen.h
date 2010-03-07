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

#ifndef DRI_SCREEN_H
#define DRI_SCREEN_H

#include "dri_util.h"
#include "xmlconfig.h"

#include "pipe/p_compiler.h"

#include "state_tracker/dri1_api.h"

struct dri_screen
{
   /* dri */
   __DRIscreen *sPriv;

   /**
    * Configuration cache with default values for all contexts
    */
   driOptionCache optionCache;

   /* drm */
   int fd;
   drmLock *drmLock;

   /* gallium */
   struct drm_api *api;
   struct pipe_winsys *pipe_winsys;
   struct pipe_screen *pipe_screen;
   boolean d_depth_bits_last;
   boolean sd_depth_bits_last;
   boolean auto_fake_front;
};

/** cast wrapper */
static INLINE struct dri_screen *
dri_screen(__DRIscreen * sPriv)
{
   return (struct dri_screen *)sPriv->private;
}

/***********************************************************************
 * dri_screen.c
 */

extern struct dri1_api *__dri1_api_hooks;

#endif

/* vim: set sw=3 ts=8 sts=3 expandtab: */
