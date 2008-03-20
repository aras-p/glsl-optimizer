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

#include "cell/common.h"
#include "draw/draw_vertex.h"
#include "pipe/p_state.h"



#define MAX_WIDTH 1024
#define MAX_HEIGHT 1024


typedef union {
   ushort us[TILE_SIZE][TILE_SIZE];
   uint   ui[TILE_SIZE][TILE_SIZE];
   vector unsigned short us8[TILE_SIZE/2][TILE_SIZE/4];
   vector unsigned int ui4[TILE_SIZE/2][TILE_SIZE/2];
} tile_t;


#define TILE_STATUS_CLEAR   1
#define TILE_STATUS_DEFINED 2  /**< defined in FB, but not in local store */
#define TILE_STATUS_CLEAN   3  /**< in local store, but not changed */
#define TILE_STATUS_DIRTY   4  /**< modified locally, but not put back yet */
#define TILE_STATUS_GETTING 5  /**< mfc_get() called but not yet arrived */


struct spu_frag_test_results {
   qword mask;
   qword depth;
   qword stencil;
};

typedef struct spu_frag_test_results (*frag_test_func)(qword frag_mask,
    qword pixel_depth, qword pixel_stencil, qword frag_depth,
    qword frag_alpha, qword facing);


struct spu_blend_results {
   qword r;
   qword g;
   qword b;
   qword a;
};

typedef struct spu_blend_results (*blend_func)(
    qword frag_r, qword frag_g, qword frag_b, qword frag_a,
    qword pixel_r, qword pixel_g, qword pixel_b, qword pixel_a,
    qword frag_mask);

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
   boolean read_depth;
   boolean read_stencil;
   frag_test_func frag_test;
   
   boolean read_fb;
   blend_func blend;

   struct pipe_sampler_state sampler[PIPE_MAX_SAMPLERS];
   struct cell_command_texture texture;

   struct vertex_info vertex_info;

   /* XXX more state to come */


   /** current color and Z tiles */
   tile_t ctile ALIGN16_ATTRIB;
   tile_t ztile ALIGN16_ATTRIB;

   /** Current tiles' status */
   ubyte cur_ctile_status, cur_ztile_status;

   /** Status of all tiles in framebuffer */
   ubyte ctile_status[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;
   ubyte ztile_status[MAX_HEIGHT/TILE_SIZE][MAX_WIDTH/TILE_SIZE] ALIGN16_ATTRIB;


   /** for converting RGBA to PIPE_FORMAT_x colors */
   vector unsigned char color_shuffle;

   vector float tex_size;
   vector unsigned int tex_size_mask; /**< == int(size - 1) */
   vector unsigned int tex_size_x_mask; /**< == int(size - 1) */
   vector unsigned int tex_size_y_mask; /**< == int(size - 1) */

   vector float (*sample_texture)(vector float texcoord);

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
#define TAG_DCACHE0           20
#define TAG_DCACHE1           21
#define TAG_DCACHE2           22
#define TAG_DCACHE3           23



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
