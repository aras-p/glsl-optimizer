/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef STW_FRAMEBUFFER_H
#define STW_FRAMEBUFFER_H

#include <windows.h>

#include "main/mtypes.h"

#include "pipe/p_thread.h"

struct stw_pixelformat_info;

/**
 * Windows framebuffer, derived from gl_framebuffer.
 */
struct stw_framebuffer
{
   HDC hDC;
   HWND hWnd;

   int iPixelFormat;
   const struct stw_pixelformat_info *pfi;
   GLvisual visual;

   pipe_mutex mutex;
   struct st_framebuffer *stfb;
   
   /* FIXME: Make this work for multiple contexts bound to the same framebuffer */
   boolean must_resize;
   unsigned width;
   unsigned height;
   
   /** This is protected by stw_device::mutex, not the mutex above */
   struct stw_framebuffer *next;
};

struct stw_framebuffer *
stw_framebuffer_create_locked(
   HDC hdc,
   int iPixelFormat );

BOOL
stw_framebuffer_allocate(
   struct stw_framebuffer *fb );

void
stw_framebuffer_update(
   struct stw_framebuffer *fb);

void
stw_framebuffer_cleanup(void);

struct stw_framebuffer *
stw_framebuffer_from_hdc_locked(
   HDC hdc );

struct stw_framebuffer *
stw_framebuffer_from_hdc(
   HDC hdc );

#endif /* STW_FRAMEBUFFER_H */
