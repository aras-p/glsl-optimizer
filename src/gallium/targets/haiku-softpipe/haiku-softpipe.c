/**************************************************************************
 *
 * Copyright 2013 Alexander von Gluck IV <kallisti5@unixzen.com>
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
 **************************************************************************/


#include "haiku-softpipe.h"

#include "util/u_debug.h"
#include "sw/hgl/hgl_sw_winsys.h"

#include "softpipe/sp_texture.h"
#include "softpipe/sp_screen.h"
#include "softpipe/sp_public.h"

#ifdef HAVE_LLVMPIPE
#include "llvmpipe/lp_texture.h"
#include "llvmpipe/lp_screen.h"
#include "llvmpipe/lp_public.h"
#endif


struct pipe_screen*
hgl_sw_screen_create(void)
{
	struct sw_winsys* winsys = hgl_create_sw_winsys();
	struct pipe_screen* screen = NULL;

	if (!winsys)
		return NULL;

	#ifdef HAVE_LLVMPIPE
	screen = llvmpipe_create_screen(winsys);
	#else
	screen = softpipe_create_screen(winsys);
	#endif
	
	if (!screen) {
		winsys->destroy(winsys);
		return NULL;
	}
	
	return screen;
}
