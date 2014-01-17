/**************************************************************************
 * 
 * Copyright 2007 VMware, Inc.
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

#ifndef LP_TEXTURE_H
#define LP_TEXTURE_H


#include "pipe/p_state.h"
#include "util/u_debug.h"
#include "lp_limits.h"


enum lp_texture_usage
{
   LP_TEX_USAGE_READ = 100,
   LP_TEX_USAGE_READ_WRITE,
   LP_TEX_USAGE_WRITE_ALL
};


struct pipe_context;
struct pipe_screen;
struct llvmpipe_context;

struct sw_displaytarget;


/** A 1D/2D/3D image, one mipmap level */
struct llvmpipe_texture_image
{
   void *data;
};


/**
 * llvmpipe subclass of pipe_resource.  A texture, drawing surface,
 * vertex buffer, const buffer, etc.
 * Textures are stored differently than other types of objects such as
 * vertex buffers and const buffers.
 * The latter are simple malloc'd blocks of memory.
 */
struct llvmpipe_resource
{
   struct pipe_resource base;

   /** Row stride in bytes */
   unsigned row_stride[LP_MAX_TEXTURE_LEVELS];
   /** Image stride (for cube maps, array or 3D textures) in bytes */
   unsigned img_stride[LP_MAX_TEXTURE_LEVELS];
   /** Number of 3D slices or cube faces per level */
   unsigned num_slices_faces[LP_MAX_TEXTURE_LEVELS];
   /** Offset to start of mipmap level, in bytes */
   unsigned linear_mip_offsets[LP_MAX_TEXTURE_LEVELS];

   /**
    * Display target, for textures with the PIPE_BIND_DISPLAY_TARGET
    * usage.
    */
   struct sw_displaytarget *dt;

   /**
    * Malloc'ed data for regular textures, or a mapping to dt above.
    */
   struct llvmpipe_texture_image linear_img;

   /**
    * Data for non-texture resources.
    */
   void *data;

   boolean userBuffer;  /** Is this a user-space buffer? */
   unsigned timestamp;

   unsigned id;  /**< temporary, for debugging */

#ifdef DEBUG
   /** for linked list */
   struct llvmpipe_resource *prev, *next;
#endif
};


struct llvmpipe_transfer
{
   struct pipe_transfer base;

   unsigned long offset;
};


/** cast wrappers */
static INLINE struct llvmpipe_resource *
llvmpipe_resource(struct pipe_resource *pt)
{
   return (struct llvmpipe_resource *) pt;
}


static INLINE const struct llvmpipe_resource *
llvmpipe_resource_const(const struct pipe_resource *pt)
{
   return (const struct llvmpipe_resource *) pt;
}


static INLINE struct llvmpipe_transfer *
llvmpipe_transfer(struct pipe_transfer *pt)
{
   return (struct llvmpipe_transfer *) pt;
}


void llvmpipe_init_screen_resource_funcs(struct pipe_screen *screen);
void llvmpipe_init_context_resource_funcs(struct pipe_context *pipe);


static INLINE boolean
llvmpipe_resource_is_texture(const struct pipe_resource *resource)
{
   switch (resource->target) {
   case PIPE_BUFFER:
      return FALSE;
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_3D:
   case PIPE_TEXTURE_CUBE:
      return TRUE;
   default:
      assert(0);
      return FALSE;
   }
}


static INLINE boolean
llvmpipe_resource_is_1d(const struct pipe_resource *resource)
{
   switch (resource->target) {
   case PIPE_BUFFER:
   case PIPE_TEXTURE_1D:
   case PIPE_TEXTURE_1D_ARRAY:
      return TRUE;
   case PIPE_TEXTURE_2D:
   case PIPE_TEXTURE_2D_ARRAY:
   case PIPE_TEXTURE_RECT:
   case PIPE_TEXTURE_3D:
   case PIPE_TEXTURE_CUBE:
      return FALSE;
   default:
      assert(0);
      return FALSE;
   }
}


static INLINE unsigned
llvmpipe_layer_stride(struct pipe_resource *resource,
                      unsigned level)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   assert(level < LP_MAX_TEXTURE_2D_LEVELS);
   return lpr->img_stride[level];
}


static INLINE unsigned
llvmpipe_resource_stride(struct pipe_resource *resource,
                         unsigned level)
{
   struct llvmpipe_resource *lpr = llvmpipe_resource(resource);
   assert(level < LP_MAX_TEXTURE_2D_LEVELS);
   return lpr->row_stride[level];
}


void *
llvmpipe_resource_map(struct pipe_resource *resource,
                      unsigned level,
                      unsigned layer,
                      enum lp_texture_usage tex_usage);

void
llvmpipe_resource_unmap(struct pipe_resource *resource,
                        unsigned level,
                        unsigned layer);


void *
llvmpipe_resource_data(struct pipe_resource *resource);


unsigned
llvmpipe_resource_size(const struct pipe_resource *resource);


ubyte *
llvmpipe_get_texture_image_address(struct llvmpipe_resource *lpr,
                                   unsigned face_slice, unsigned level);

void *
llvmpipe_get_texture_image(struct llvmpipe_resource *resource,
                           unsigned face_slice, unsigned level,
                           enum lp_texture_usage usage);

void *
llvmpipe_get_texture_image_all(struct llvmpipe_resource *lpr,
                               unsigned level,
                               enum lp_texture_usage usage);

ubyte *
llvmpipe_get_texture_tile_linear(struct llvmpipe_resource *lpr,
                                 unsigned face_slice, unsigned level,
                                 enum lp_texture_usage usage,
                                 unsigned x, unsigned y);


extern void
llvmpipe_print_resources(void);


#define LP_UNREFERENCED         0
#define LP_REFERENCED_FOR_READ  (1 << 0)
#define LP_REFERENCED_FOR_WRITE (1 << 1)

unsigned int
llvmpipe_is_resource_referenced( struct pipe_context *pipe,
                                 struct pipe_resource *presource,
                                 unsigned level);

unsigned
llvmpipe_get_format_alignment(enum pipe_format format);

#endif /* LP_TEXTURE_H */
