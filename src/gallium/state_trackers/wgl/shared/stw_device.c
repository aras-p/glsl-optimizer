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

#include <windows.h>

#include "pipe/p_debug.h"
#include "pipe/p_screen.h"

#include "shared/stw_device.h"
#include "shared/stw_winsys.h"
#include "shared/stw_pixelformat.h"
#include "shared/stw_public.h"
#include "stw.h"


struct stw_device *stw_dev = NULL;


/**
 * XXX: Dispatch pipe_screen::flush_front_buffer to our 
 * stw_winsys::flush_front_buffer.
 */
static void 
st_flush_frontbuffer(struct pipe_screen *screen,
                     struct pipe_surface *surf,
                     void *context_private )
{
   const struct stw_winsys *stw_winsys = stw_dev->stw_winsys;
   HDC hdc = (HDC)context_private;
   
   stw_winsys->flush_frontbuffer(screen, surf, hdc);
}


boolean
stw_shared_init(const struct stw_winsys *stw_winsys)
{
   static struct stw_device stw_dev_storage;

   assert(!stw_dev);

   stw_dev = &stw_dev_storage;
   memset(stw_dev, 0, sizeof(*stw_dev));

   stw_dev->stw_winsys = stw_winsys;

   stw_dev->screen = stw_winsys->create_screen();
   if(!stw_dev->screen)
      goto error1;

   stw_dev->screen->flush_frontbuffer = st_flush_frontbuffer;
   
   pixelformat_init();

   return TRUE;

error1:
   stw_dev = NULL;
   return FALSE;
}


void
stw_shared_cleanup(void)
{
   stw_dev = NULL;
}
