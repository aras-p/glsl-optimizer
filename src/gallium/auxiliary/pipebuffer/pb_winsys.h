/**************************************************************************
 *
 * Copyright 2009 VMWare, Inc.
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

/**
 * @file
 * Drop-in replacements for winsys buffer callbacks.
 *
 * The requirement to use this functions is that all pipe_buffers must be in
 * fact pb_buffers.
 * 
 * @author Jose Fonseca <jfonseca@vmware.com>
 */

#ifndef PB_WINSYS_H_
#define PB_WINSYS_H_


#include "pipe/p_compiler.h"
#include "pipe/p_debug.h"
#include "pipe/p_state.h"
#include "pipe/p_inlines.h"


#ifdef __cplusplus
extern "C" {
#endif


struct pipe_buffer *
pb_winsys_user_buffer_create(struct pipe_winsys *winsys,
			     void *data, 
			     unsigned bytes);
                                
void *
pb_winsys_buffer_map(struct pipe_winsys *winsys,
		     struct pipe_buffer *buf,
		     unsigned flags);

void
pb_winsys_buffer_unmap(struct pipe_winsys *winsys,
		       struct pipe_buffer *buf);

void
pb_winsys_buffer_destroy(struct pipe_winsys *winsys,
			 struct pipe_buffer *buf);

void 
pb_init_winsys(struct pipe_winsys *winsys);


#ifdef __cplusplus
}
#endif

#endif /*PB_WINSYS_H_*/
