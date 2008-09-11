/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef AUB_WINSYS_H
#define AUB_WINSYS_H

struct pipe_context;
struct pipe_winsys;
struct pipe_buffer;
struct pipe_surface;

struct pipe_winsys *
xmesa_create_pipe_winsys_aub( void );

void
xmesa_destroy_pipe_winsys_aub( struct pipe_winsys *winsys );



struct pipe_context *
xmesa_create_i965simple( struct pipe_winsys *winsys );



void xmesa_buffer_subdata_aub(struct pipe_winsys *winsys, 
			      struct pipe_buffer *buf,
			      unsigned long offset, 
			      unsigned long size, 
			      const void *data,
			      unsigned aub_type,
			      unsigned aub_sub_type);

void xmesa_commands_aub(struct pipe_winsys *winsys,
			unsigned *cmds,
			unsigned nr_dwords);


void xmesa_display_aub( /* struct pipe_winsys *winsys, */
   struct pipe_surface *surface );

extern struct pipe_winsys *
xmesa_get_pipe_winsys_aub(struct xmesa_visual *xm_vis);

#endif
