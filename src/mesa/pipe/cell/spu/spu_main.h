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

#ifndef SPU_MAIN_H
#define SPU_MAIN_H


#include <spu_mfcio.h>

#include "pipe/cell/common.h"
#include "pipe/draw/draw_vertex.h"
#include "pipe/p_state.h"


typedef union
{
   vector float v;
   float f[4];
} float4;


struct spu_framebuffer {
   void *color_start;              /**< addr of color surface in main memory */
   void *depth_start;              /**< addr of depth surface in main memory */
   enum pipe_format color_format;
   enum pipe_format depth_format;
   uint width, height;             /**< size in pixels */
   uint width_tiles, height_tiles; /**< width and height in tiles */

   uint color_clear_value;
   uint depth_clear_value;

   uint zsize;                     /**< 0, 2 or 4 bytes per Z */
} ALIGN16_ATTRIB;


/**
 * All SPU global/context state will be in singleton object of this type:
 */
struct spu_global
{
   struct cell_init_info init;

   struct spu_framebuffer fb;
   struct pipe_depth_stencil_alpha_state depth_stencil;
   struct pipe_blend_state blend;
   struct pipe_sampler_state sampler[PIPE_MAX_SAMPLERS];
   struct cell_command_texture texture;

   struct vertex_info vertex_info;

   /* XXX more state to come */

} ALIGN16_ATTRIB;


extern struct spu_global spu;
extern boolean Debug;




/* DMA TAGS */

#define TAG_SURFACE_CLEAR     10
#define TAG_VERTEX_BUFFER     11
#define TAG_READ_TILE_COLOR   12
#define TAG_READ_TILE_Z       13
#define TAG_WRITE_TILE_COLOR  14
#define TAG_WRITE_TILE_Z      15
#define TAG_INDEX_BUFFER      16
#define TAG_BATCH_BUFFER      17
#define TAG_MISC              18
#define TAG_TEXTURE_TILE      19
#define TAG_INSTRUCTION_FETCH 20



static INLINE void
wait_on_mask(unsigned tagMask)
{
   mfc_write_tag_mask( tagMask );
   /* wait for completion of _any_ DMAs specified by tagMask */
   mfc_read_tag_status_any();
}


static INLINE void
wait_on_mask_all(unsigned tagMask)
{
   mfc_write_tag_mask( tagMask );
   /* wait for completion of _any_ DMAs specified by tagMask */
   mfc_read_tag_status_all();
}





static INLINE void
memset16(ushort *d, ushort value, uint count)
{
   uint i;
   for (i = 0; i < count; i++)
      d[i] = value;
}


static INLINE void
memset32(uint *d, uint value, uint count)
{
   uint i;
   for (i = 0; i < count; i++)
      d[i] = value;
}


#endif /* SPU_MAIN_H */
