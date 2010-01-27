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


#if DEBUG
/* These debug macros use the unusual construction ", ##__VA_ARGS__"
 * which expands to the expected comma + args if variadic arguments
 * are supplied, but swallows the comma if there are no variadic
 * arguments (which avoids syntax errors that would otherwise occur).
 */
#define D_PRINTF(flag, format,...) \
   if (spu.init.debug_flags & (flag)) \
      printf("SPU %u: " format, spu.init.id, ##__VA_ARGS__)
#else
#define D_PRINTF(...)
#endif


/**
 * A tile is basically a TILE_SIZE x TILE_SIZE block of 4-byte pixels.
 * The data may be addressed through several different types.
 */
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


/** Function for sampling textures */
typedef void (*spu_sample_texture_2d_func)(vector float s,
                                           vector float t,
                                           uint unit, uint level, uint face,
                                           vector float colors[4]);


/** Function for performing per-fragment ops */
typedef void (*spu_fragment_ops_func)(uint x, uint y,
                                      tile_t *colorTile,
                                      tile_t *depthStencilTile,
                                      vector float fragZ,
                                      vector float fragRed,
                                      vector float fragGreen,
                                      vector float fragBlue,
                                      vector float fragAlpha,
                                      vector unsigned int mask);

/** Function for running fragment program */
typedef vector unsigned int (*spu_fragment_program_func)(vector float *inputs,
                                                         vector float *outputs,
                                                         vector float *constants);


PIPE_ALIGN_TYPE(16,
struct spu_framebuffer
{
   void *color_start;              /**< addr of color surface in main memory */
   void *depth_start;              /**< addr of depth surface in main memory */
   enum pipe_format color_format;
   enum pipe_format depth_format;
   uint width;                     /**< width in pixels */
   uint height;                    /**< height in pixels */
   uint width_tiles;               /**< width in tiles */
   uint height_tiles;              /**< width in tiles */

   uint color_clear_value;
   uint depth_clear_value;

   uint zsize;                     /**< 0, 2 or 4 bytes per Z */
   float zscale;                   /**< 65535.0, 2^24-1 or 2^32-1 */
});


/** per-texture level info */
PIPE_ALIGN_TYPE(16,
struct spu_texture_level
{
   void *start;
   ushort width;
   ushort height;
   ushort depth;
   ushort tiles_per_row;
   uint bytes_per_image;
   /** texcoord scale factors */
   vector float scale_s;
   vector float scale_t;
   vector float scale_r;
   /** texcoord masks (if REPEAT then size-1, else ~0) */
   vector signed int mask_s;
   vector signed int mask_t;
   vector signed int mask_r;
   /** texcoord clamp limits */
   vector signed int max_s;
   vector signed int max_t;
   vector signed int max_r;
});


PIPE_ALIGN_TYPE(16,
struct spu_texture
{
   struct spu_texture_level level[CELL_MAX_TEXTURE_LEVELS];
   uint max_level;
   uint target;  /**< PIPE_TEXTURE_x */
});


/**
 * All SPU global/context state will be in a singleton object of this type:
 */
PIPE_ALIGN_TYPE(16,
struct spu_global
{
   /** One-time init/constant info */
   struct cell_init_info init;

   /*
    * Current state
    */
   struct spu_framebuffer fb;
   struct pipe_depth_stencil_alpha_state depth_stencil_alpha;
   struct pipe_blend_state blend;
   struct pipe_blend_color blend_color;
   struct pipe_sampler_state sampler[PIPE_MAX_SAMPLERS];
   struct pipe_rasterizer_state rasterizer;
   struct spu_texture texture[PIPE_MAX_SAMPLERS];
   struct vertex_info vertex_info;

   /** Current color and Z tiles */
   PIPE_ALIGN_VAR(16) tile_t ctile;
   PIPE_ALIGN_VAR(16) tile_t ztile;

   /** Read depth/stencil tiles? */
   boolean read_depth_stencil;

   /** Current tiles' status */
   ubyte cur_ctile_status;
   ubyte cur_ztile_status;

   /** Status of all tiles in framebuffer */
   PIPE_ALIGN_VAR(16) ubyte ctile_status[CELL_MAX_HEIGHT/TILE_SIZE][CELL_MAX_WIDTH/TILE_SIZE];
   PIPE_ALIGN_VAR(16) ubyte ztile_status[CELL_MAX_HEIGHT/TILE_SIZE][CELL_MAX_WIDTH/TILE_SIZE];

   /** Current fragment ops machine code, at 8-byte boundary */
   uint *fragment_ops_code;
   uint fragment_ops_code_size;
   /** Current fragment ops functions, 0 = frontfacing, 1 = backfacing */
   spu_fragment_ops_func fragment_ops[2];

   /** Current fragment program machine code, at 8-byte boundary */
   PIPE_ALIGN_VAR(8) uint fragment_program_code[SPU_MAX_FRAGMENT_PROGRAM_INSTS];
   /** Current fragment ops function */
   spu_fragment_program_func fragment_program;

   /** Current texture sampler function */
   spu_sample_texture_2d_func sample_texture_2d[CELL_MAX_SAMPLERS];
   spu_sample_texture_2d_func min_sample_texture_2d[CELL_MAX_SAMPLERS];
   spu_sample_texture_2d_func mag_sample_texture_2d[CELL_MAX_SAMPLERS];

   /** Fragment program constants */
   vector float constants[4 * CELL_MAX_CONSTANTS];

});


extern struct spu_global spu;



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
#define TAG_FENCE             24


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
