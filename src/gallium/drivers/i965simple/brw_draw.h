 /**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef BRW_DRAW_H
#define BRW_DRAW_H

#include "pipe/p_context.h"

struct brw_context;



void brw_init_draw_functions( struct brw_context *brw );


boolean brw_upload_vertices( struct brw_context *brw,
			       unsigned min_index,
			       unsigned max_index );

boolean brw_upload_indices(struct brw_context *brw,
                           const struct pipe_buffer *index_buffer,
                           int ib_size, int start, int count);

boolean brw_upload_vertex_buffers( struct brw_context *brw );
boolean brw_upload_vertex_elements( struct brw_context *brw );

unsigned brw_translate_surface_format( unsigned id );



#endif
