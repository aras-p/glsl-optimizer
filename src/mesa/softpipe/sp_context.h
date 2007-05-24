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

#ifndef SP_CONTEXT_H
#define SP_CONTEXT_H

#include "mtypes.h"

struct softpipe_context *generic_create( void );
				      
/* Drawing currently kludged up via the existing tnl/ module.  
 */
struct vertex_buffer;

struct softpipe_context {

   void (*destroy)( struct softpipe_context * );

   void (*set_clip_state)( struct softpipe_context *,
			   const struct softpipe_clip_state * );

   void (*set_viewport)( struct softpipe_context *,
			 const struct softpipe_viewport * );

   void (*set_setup_state)( struct softpipe_context *,
			    const struct softpipe_setup_state * );

   void (*set_scissor_rect)( struct softpipe_context *,
			     const struct softpipe_scissor_rect * );

   void (*set_blend_state)( struct softpipe_context *,
                            const struct softpipe_blend_state * );

   void (*set_fs_state)( struct softpipe_context *,
			 const struct softpipe_fs_state * );

   void (*set_polygon_stipple)( struct softpipe_context *,
				const struct softpipe_poly_stipple * );

   void (*set_cbuf_state)( struct softpipe_context *,
			   const struct softpipe_surface * );


   void (*draw_vb)( struct softpipe_context *softpipe,
		    struct vertex_buffer *VB );
};




#endif
