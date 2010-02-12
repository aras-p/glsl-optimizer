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
 * Types and tokens which are common to the SPU and PPU code.
 */


#ifndef CELL_COMMON_H
#define CELL_COMMON_H

#include "pipe/p_compiler.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include <stdio.h>

/** The standard assert macro doesn't seem to work reliably */
#define ASSERT(x) \
   if (!(x)) { \
      ubyte *p = NULL; \
      fprintf(stderr, "%s:%d: %s(): assertion %s failed.\n", \
              __FILE__, __LINE__, __FUNCTION__, #x);             \
      *p = 0; \
      exit(1); \
   }


#define JOIN(x, y) JOIN_AGAIN(x, y)
#define JOIN_AGAIN(x, y) x ## y

#define STATIC_ASSERT(e) \
{typedef char JOIN(assertion_failed_at_line_, __LINE__) [(e) ? 1 : -1];}



/** for sanity checking */
#define ASSERT_ALIGN16(ptr) \
  ASSERT((((unsigned long) (ptr)) & 0xf) == 0);


/** round up value to next multiple of 4 */
#define ROUNDUP4(k)  (((k) + 0x3) & ~0x3)

/** round up value to next multiple of 8 */
#define ROUNDUP8(k)  (((k) + 0x7) & ~0x7)

/** round up value to next multiple of 16 */
#define ROUNDUP16(k)  (((k) + 0xf) & ~0xf)


#define CELL_MAX_SPUS 8

#define CELL_MAX_SAMPLERS 4
#define CELL_MAX_TEXTURE_LEVELS 12  /* 2k x 2k */
#define CELL_MAX_CONSTANTS 32  /**< number of float[4] constants */
#define CELL_MAX_WIDTH 1024    /**< max framebuffer width */
#define CELL_MAX_HEIGHT 1024   /**< max framebuffer width */

#define TILE_SIZE 32


/**
 * The low byte of a mailbox word contains the command opcode.
 * Remaining higher bytes are command specific.
 */
#define CELL_CMD_OPCODE_MASK 0xff

#define CELL_CMD_EXIT                 1
#define CELL_CMD_CLEAR_SURFACE        2
#define CELL_CMD_FINISH               3
#define CELL_CMD_RENDER               4
#define CELL_CMD_BATCH                5
#define CELL_CMD_RELEASE_VERTS        6
#define CELL_CMD_STATE_FRAMEBUFFER   10
#define CELL_CMD_STATE_FRAGMENT_OPS  11
#define CELL_CMD_STATE_SAMPLER       12
#define CELL_CMD_STATE_TEXTURE       13
#define CELL_CMD_STATE_VERTEX_INFO   14
#define CELL_CMD_STATE_VIEWPORT      15
#define CELL_CMD_STATE_UNIFORMS      16
#define CELL_CMD_STATE_VS_ARRAY_INFO 17
#define CELL_CMD_STATE_BIND_VS       18
#define CELL_CMD_STATE_FRAGMENT_PROGRAM 19
#define CELL_CMD_STATE_ATTRIB_FETCH  20
#define CELL_CMD_STATE_FS_CONSTANTS  21
#define CELL_CMD_STATE_RASTERIZER    22
#define CELL_CMD_VS_EXECUTE          23
#define CELL_CMD_FLUSH_BUFFER_RANGE  24
#define CELL_CMD_FENCE               25


/** Command/batch buffers */
#define CELL_NUM_BUFFERS 4
#define CELL_BUFFER_SIZE (4*1024)  /**< 16KB would be the max */

#define CELL_BUFFER_STATUS_FREE 10
#define CELL_BUFFER_STATUS_USED 20

/** Debug flags */
#define CELL_DEBUG_CHECKER              (1 << 0)
#define CELL_DEBUG_ASM                  (1 << 1)
#define CELL_DEBUG_SYNC                 (1 << 2)
#define CELL_DEBUG_FRAGMENT_OPS         (1 << 3)
#define CELL_DEBUG_FRAGMENT_OP_FALLBACK (1 << 4)
#define CELL_DEBUG_CMD                  (1 << 5)
#define CELL_DEBUG_CACHE                (1 << 6)

#define CELL_FENCE_IDLE      0
#define CELL_FENCE_EMITTED   1
#define CELL_FENCE_SIGNALLED 2

#define CELL_FACING_FRONT    0
#define CELL_FACING_BACK     1

struct cell_fence
{
   /** There's a 16-byte status qword per SPU */
   volatile uint status[CELL_MAX_SPUS][4];
};

#ifdef __SPU__
typedef vector unsigned int opcode_t;
#else
typedef unsigned int opcode_t[4];
#endif

/**
 * Fence command sent to SPUs.  In response, the SPUs will write
 * CELL_FENCE_STATUS_SIGNALLED back to the fence status word in main memory.
 */
struct cell_command_fence
{
   opcode_t opcode;      /**< CELL_CMD_FENCE */
   struct cell_fence *fence;
   uint32_t pad_[3];
};


/**
 * Command to specify per-fragment operations state and generated code.
 * Note that this is a variant-length structure, allocated with as 
 * much memory as needed to hold the generated code; the "code"
 * field *must* be the last field in the structure.  Also, the entire
 * length of the structure (including the variant code field) must be
 * a multiple of 8 bytes; we require that this structure itself be
 * a multiple of 8 bytes, and that the generated code also be a multiple
 * of 8 bytes.
 *
 * Also note that the dsa, blend, blend_color fields are really only needed
 * for the fallback/C per-pixel code.  They're not used when we generate
 * dynamic SPU fragment code (which is the normal case), and will eventually
 * be removed from this structure.
 */
struct cell_command_fragment_ops
{
   opcode_t opcode;      /**< CELL_CMD_STATE_FRAGMENT_OPS */

   /* Fields for the fallback case */
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_blend_state blend;
   struct pipe_blend_color blend_color;

   /* Fields for the generated SPU code */
   unsigned total_code_size;
   unsigned front_code_index;
   unsigned back_code_index;
   /* this field has variant length, and must be the last field in 
    * the structure
    */
   unsigned code[0];
};


/** Max instructions for fragment programs */
#define SPU_MAX_FRAGMENT_PROGRAM_INSTS 512

/**
 * Command to send a fragment program to SPUs.
 */
struct cell_command_fragment_program
{
   opcode_t opcode;      /**< CELL_CMD_STATE_FRAGMENT_PROGRAM */
   uint num_inst;        /**< Number of instructions */
   uint32_t pad[3];
   unsigned code[SPU_MAX_FRAGMENT_PROGRAM_INSTS];
};


/**
 * Tell SPUs about the framebuffer size, location
 */
struct cell_command_framebuffer
{
   opcode_t opcode;     /**< CELL_CMD_STATE_FRAMEBUFFER */
   int width, height;
   void *color_start, *depth_start;
   enum pipe_format color_format, depth_format;
   uint32_t pad_[2];
};


/**
 * Tell SPUs about rasterizer state.
 */
struct cell_command_rasterizer
{
   opcode_t opcode;    /**< CELL_CMD_STATE_RASTERIZER */
   struct pipe_rasterizer_state rasterizer;
   uint32_t pad[1];
};


/**
 * Clear framebuffer to the given value/color.
 */
struct cell_command_clear_surface
{
   opcode_t opcode;     /**< CELL_CMD_CLEAR_SURFACE */
   uint surface; /**< Temporary: 0=color, 1=Z */
   uint value;
   uint32_t pad[2];
};


/**
 * Array info used by the vertex shader's vertex puller.
 */
struct cell_array_info
{
   uint64_t base;      /**< Base address of the 0th element. */
   uint attr;          /**< Attribute that this state is for. */
   uint pitch;         /**< Byte pitch from one entry to the next. */
   uint size;
   uint function_offset;
};


struct cell_attribute_fetch_code
{
   uint64_t base;
   uint size;
};


struct cell_buffer_range
{
   uint64_t base;
   unsigned size;
};


struct cell_shader_info
{
   uint64_t declarations;
   uint64_t instructions;
   uint64_t  immediates;

   unsigned num_outputs;
   unsigned num_declarations;
   unsigned num_instructions;
   unsigned num_immediates;
};


#define SPU_VERTS_PER_BATCH 64
struct cell_command_vs
{
   opcode_t opcode;       /**< CELL_CMD_VS_EXECUTE */
   uint64_t vOut[SPU_VERTS_PER_BATCH];
   unsigned num_elts;
   unsigned elts[SPU_VERTS_PER_BATCH];
   float plane[12][4];
   unsigned nr_planes;
   unsigned nr_attrs;
};


struct cell_command_render
{
   opcode_t opcode;   /**< CELL_CMD_RENDER */
   uint prim_type;    /**< PIPE_PRIM_x */
   uint num_verts;
   uint vertex_size;  /**< bytes per vertex */
   uint num_indexes;
   uint vertex_buf;  /**< which cell->buffer[] contains the vertex data */
   float xmin, ymin, xmax, ymax;  /* XXX another dummy field */
   uint min_index;
   boolean inline_verts;
   uint32_t pad_[1];
};


struct cell_command_release_verts
{
   opcode_t opcode;         /**< CELL_CMD_RELEASE_VERTS */
   uint vertex_buf;    /**< in [0, CELL_NUM_BUFFERS-1] */
   uint32_t pad_[3];
};


struct cell_command_sampler
{
   opcode_t opcode;         /**< CELL_CMD_STATE_SAMPLER */
   uint unit;
   struct pipe_sampler_state state;
   uint32_t pad_[3];
};


struct cell_command_texture
{
   opcode_t opcode;     /**< CELL_CMD_STATE_TEXTURE */
   uint target;         /**< PIPE_TEXTURE_x */
   uint unit;
   void *start[CELL_MAX_TEXTURE_LEVELS];   /**< Address in main memory */
   ushort width[CELL_MAX_TEXTURE_LEVELS];
   ushort height[CELL_MAX_TEXTURE_LEVELS];
   ushort depth[CELL_MAX_TEXTURE_LEVELS];
};


#define MAX_SPU_FUNCTIONS 12
/**
 * Used to tell the PPU about the address of particular functions in the
 * SPU's address space.
 */
struct cell_spu_function_info
{
   uint num;
   char names[MAX_SPU_FUNCTIONS][16];
   uint addrs[MAX_SPU_FUNCTIONS];
   char pad[12];   /**< Pad struct to multiple of 16 bytes (256 currently) */
};


/** This is the object passed to spe_create_thread() */
PIPE_ALIGN_TYPE(16,
struct cell_init_info
{
   unsigned id;
   unsigned num_spus;
   unsigned debug_flags;  /**< mask of CELL_DEBUG_x flags */
   float inv_timebase;    /**< 1.0/timebase, for perf measurement */

   /** Buffers for command batches, vertex/index data */
   ubyte *buffers[CELL_NUM_BUFFERS];
   uint *buffer_status;  /**< points at cell_context->buffer_status */

   struct cell_spu_function_info *spu_functions;
});


#endif /* CELL_COMMON_H */
