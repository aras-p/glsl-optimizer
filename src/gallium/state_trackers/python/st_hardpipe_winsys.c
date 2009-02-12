/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/**
 * @file
 * Stub for hardware pipe driver support.
 */


#include "pipe/p_compiler.h"

#include "st_winsys.h"


/* XXX: Force init_gallium symbol to be linked */
extern void init_gallium(void);
void (*force_init_gallium_linkage)(void) = &init_gallium;


static struct pipe_screen *
st_hardpipe_screen_create(void)
{
   return st_softpipe_winsys.screen_create();
}


static struct pipe_context *
st_hardpipe_context_create(struct pipe_screen *screen)
{
   return st_softpipe_winsys.context_create(screen);
}


const struct st_winsys st_hardpipe_winsys = {
   &st_hardpipe_screen_create,
   &st_hardpipe_context_create
};
