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

#ifndef BRW_RESOURCE_H
#define BRW_RESOURCE_H

struct brw_screen;

#include "util/u_transfer.h"
#include "util/u_debug.h"

#include "brw_screen.h"		/* for brw_surface */

struct brw_context;
struct brw_screen;


struct brw_buffer {
   struct u_resource b;

   /* One of either bo or user_buffer will be non-null, depending on
    * whether this is a hardware or user buffer.
    */
   struct brw_winsys_buffer *bo;
   void *user_buffer;

   /* Mapped pointer??
    */
   void *ptr;
};

#define BRW_MAX_TEXTURE_2D_LEVELS 11  /* max 1024x1024 */
#define BRW_MAX_TEXTURE_3D_LEVELS  8  /* max 128x128x128 */



struct brw_texture {
   struct u_resource b;
   struct brw_winsys_buffer *bo;
   struct brw_surface_state ss;

   unsigned *image_offset[BRW_MAX_TEXTURE_2D_LEVELS];
   unsigned nr_images[BRW_MAX_TEXTURE_2D_LEVELS];
   unsigned level_offset[BRW_MAX_TEXTURE_2D_LEVELS];

   boolean compressed;
   unsigned brw_target;
   unsigned pitch;
   unsigned tiling;
   unsigned cpp;
   unsigned total_height;

   struct brw_surface views[2];
};


void brw_init_screen_resource_functions(struct brw_screen *is);
void brw_init_resource_functions(struct brw_context *brw );

extern struct u_resource_vtbl brw_buffer_vtbl;
extern struct u_resource_vtbl brw_texture_vtbl;

static INLINE struct brw_texture *brw_texture( struct pipe_resource *resource )
{
   struct brw_texture *tex = (struct brw_texture *)resource;
   assert(tex->b.vtbl == &brw_texture_vtbl);
   return tex;
}

static INLINE struct brw_buffer *brw_buffer( struct pipe_resource *resource )
{
   struct brw_buffer *tex = (struct brw_buffer *)resource;
   assert(tex->b.vtbl == &brw_buffer_vtbl);
   return tex;
}

struct pipe_resource *
brw_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template);

struct pipe_resource *
brw_texture_from_handle(struct pipe_screen * screen,
			const struct pipe_resource *template,
			struct winsys_handle *whandle);


struct pipe_resource *
brw_user_buffer_create(struct pipe_screen *screen,
                        void *ptr,
                        unsigned bytes,
			unsigned usage);

struct pipe_resource *
brw_buffer_create(struct pipe_screen *screen,
		   const struct pipe_resource *template);


/*
boolean
brw_is_format_supported( struct pipe_screen *screen,
			 enum pipe_format format,
			 enum pipe_texture_target target,
			 unsigned sample_count,
			 unsigned tex_usage,
			 unsigned geom_flags );
*/

/* Pipe buffer helpers
 */
static INLINE boolean
brw_buffer_is_user_buffer( const struct pipe_resource *buf )
{
   return ((const struct brw_buffer *)buf)->user_buffer != NULL;
}


/***********************************************************************
 * Internal functions 
 */
GLboolean brw_texture_layout(struct brw_screen *brw_screen,
			     struct brw_texture *tex );

void brw_update_texture( struct brw_screen *brw_screen,
			 struct brw_texture *tex );



#endif /* BRW_RESOURCE_H */
