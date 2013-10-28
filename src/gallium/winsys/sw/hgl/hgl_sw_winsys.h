/**************************************************************************
 *
 * Copyright 2009 Artur Wyszynski <harakash@gmail.com>
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
#ifndef _HGL_SOFTWAREWINSYS_H
#define _HGL_SOFTWAREWINSYS_H


#include "pipe/p_defines.h"
#include "state_tracker/st_api.h"
#include "state_tracker/sw_winsys.h"

#include "bitmap_wrapper.h"


struct haiku_displaytarget
{
	enum pipe_format format;
	color_space colorSpace;

	unsigned width;
	unsigned height;
	unsigned stride;

	unsigned size;

	void* data;
};


struct sw_winsys* hgl_create_sw_winsys();


#endif
