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

#include "main/mtypes.h"

/**
 * Windows framebuffer, derived from gl_framebuffer.
 */
struct stw_framebuffer
{
   struct st_framebuffer *stfb;
   HDC hDC;
   HWND hWnd;
   struct stw_framebuffer *next;
};

struct stw_framebuffer *
stw_framebuffer_create(
   HDC hdc,
   GLvisual *visual,
   GLuint width,
   GLuint height );

void
stw_framebuffer_destroy(
   struct stw_framebuffer *fb );

void
stw_framebuffer_resize(
   struct stw_framebuffer *fb,
   GLuint width,
   GLuint height );

struct stw_framebuffer *
stw_framebuffer_from_hdc(
   HDC hdc );

boolean
stw_framebuffer_init_thread(void);

void
stw_framebuffer_cleanup_thread(void);

#endif /* STW_FRAMEBUFFER_H */
