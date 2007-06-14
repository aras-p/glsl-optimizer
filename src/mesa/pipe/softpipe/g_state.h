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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef PRIM_H
#define PRIM_H

#include "glheader.h"
#include "softpipe/sp_state.h"


void generic_set_clip_state( struct softpipe_context *,
			     const struct softpipe_clip_state * );

void generic_set_viewport( struct softpipe_context *,
			   const struct softpipe_viewport * );

void generic_set_setup_state( struct softpipe_context *,
			      const struct softpipe_setup_state * );

void generic_set_scissor_rect( struct softpipe_context *,
			       const struct softpipe_scissor_rect * );

void generic_set_fs_state( struct softpipe_context *,
			   const struct softpipe_fs_state * );

void generic_set_polygon_stipple( struct softpipe_context *,
				  const struct softpipe_poly_stipple * );

void generic_set_cbuf_state( struct softpipe_context *,
			     const struct softpipe_surface * );

void generic_update_derived( struct generic_context *generic );

#endif
