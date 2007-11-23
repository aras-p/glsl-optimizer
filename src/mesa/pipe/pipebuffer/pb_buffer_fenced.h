/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * \file
 * Buffer fencing.
 * 
 * "Fenced buffers" is actually a misnomer. They should be referred as 
 * "fenceable buffers", i.e, buffers that can be fenced, but I couldn't find
 * the word "fenceable" in the dictionary.
 * 
 * A "fenced buffer" is a decorator around a normal buffer, which adds two 
 * special properties:
 * - the ability for the destruction to be delayed by a fence;
 * - reference counting.
 * 
 * Usually DMA buffers have a life-time that will extend the life-time of its 
 * handle. The end-of-life is dictated by the fence signalling. 
 * 
 * Between the handle's destruction, and the fence signalling, the buffer is 
 * stored in a fenced buffer list.
 * 
 * \author José Fonseca <jrfonseca@tungstengraphics.com>
 */

#ifndef PB_BUFFER_FENCED_H_
#define PB_BUFFER_FENCED_H_


#include <assert.h>


struct pipe_winsys;
struct pipe_buffer;
struct pipe_fence_handle;


/**
 * List of buffers which are awaiting fence signalling.
 */
struct fenced_buffer_list;


/**
 * The fenced buffer's virtual function table.
 * 
 * NOTE: Made public for debugging purposes.
 */
extern const struct pipe_buffer_vtbl fenced_buffer_vtbl;


/**
 * Create a fenced buffer list.
 * 
 * See also fenced_bufmgr_create for a more convenient way to use this.
 */
struct fenced_buffer_list *
fenced_buffer_list_create(struct pipe_winsys *winsys);


/**
 * Walk the fenced buffer list to check and free signalled buffers.
 */ 
void
fenced_buffer_list_check_free(struct fenced_buffer_list *fenced_list, 
                              int wait);

void
fenced_buffer_list_destroy(struct fenced_buffer_list *fenced_list);


/**
 * Wrap a buffer in a fenced buffer.
 * 
 * NOTE: this will not increase the buffer reference count.
 */
struct pipe_buffer *
fenced_buffer_create(struct fenced_buffer_list *fenced, 
                     struct pipe_buffer *buffer);


/**
 * Set a buffer's fence.
 * 
 * NOTE: Although it takes a generic pipe buffer argument, it will fail
 * on everything but buffers returned by fenced_buffer_create.
 */
void
buffer_fence(struct pipe_buffer *buf,
             struct pipe_fence_handle *fence);


/**
 * Remove the buffer's fence.
 * 
 * NOTE: Although it takes a generic pipe buffer argument, it will fail
 * on everything but buffers returned by fenced_buffer_create.
 */
void
buffer_unfence(struct pipe_buffer *buf);


/**
 * Wait for the buffer fence to signal.
 *
 * See also pipe_winsys::fence_finish().
 */ 
int
buffer_finish(struct pipe_buffer *buf, 
              unsigned flag);



#endif /*PB_BUFFER_FENCED_H_*/
