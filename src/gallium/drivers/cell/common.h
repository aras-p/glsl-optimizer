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
#include "pipe/p_util.h"
#include "pipe/p_format.h"


/** The standard assert macro doesn't seem to work reliably */
#define ASSERT(x) \
   if (!(x)) { \
      ubyte *p = NULL; \
      fprintf(stderr, "%s:%d: %s(): assertion %s failed.\n", \
              __FILE__, __LINE__, __FUNCTION__, #x);             \
      *p = 0; \
      exit(1); \
   }


/** for sanity checking */
#define ASSERT_ALIGN16(ptr) \
  ASSERT((((unsigned long) (ptr)) & 0xf) == 0);


/** round up value to next multiple of 4 */
#define ROUNDUP4(k)  (((k) + 0x3) & ~0x3)

/** round up value to next multiple of 8 */
#define ROUNDUP8(k)  (((k) + 0x7) & ~0x7)

/** round up value to next multiple of 16 */
#define ROUNDUP16(k)  (((k) + 0xf) & ~0xf)


#define CELL_MAX_SPUS 6

#define CELL_MAX_SAMPLERS 4

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
#define CELL_CMD_STATE_DEPTH_STENCIL 11
#define CELL_CMD_STATE_SAMPLER       12
#define CELL_CMD_STATE_TEXTURE       13
#define CELL_CMD_STATE_VERTEX_INFO   14
#define CELL_CMD_STATE_VIEWPORT      15
#define CELL_CMD_STATE_UNIFORMS      16
#define CELL_CMD_STATE_VS_ARRAY_INFO 17
#define CELL_CMD_STATE_BIND_VS       18
#define CELL_CMD_STATE_BLEND         19
#define CELL_CMD_STATE_ATTRIB_FETCH  20
#define CELL_CMD_STATE_LOGICOP       21
#define CELL_CMD_VS_EXECUTE          22
#define CELL_CMD_FLUSH_BUFFER_RANGE  23


#define CELL_NUM_BUFFERS 4
#define CELL_BUFFER_SIZE (4*1024)  /**< 16KB would be the max */

#define CELL_BUFFER_STATUS_FREE 10
#define CELL_BUFFER_STATUS_USED 20



/**
 */
struct cell_command_depth_stencil_alpha_test {
   uint64_t base;               /**< Effective address of code start. */
   unsigned size;               /**< Size in bytes of SPE code. */
   unsigned read_depth;         /**< Flag: should depth be read? */
   unsigned read_stencil;       /**< Flag: should stencil be read? */
};


/**
 * Upload code to perform framebuffer blend operation
 */
struct cell_command_blend {
   uint64_t base;               /**< Effective address of code start. */
   unsigned size;               /**< Size in bytes of SPE code. */
   unsigned read_fb;            /**< Flag: should framebuffer be read? */
};


struct cell_command_logicop {
   uint64_t base;               /**< Effective address of code start. */
   unsigned size;               /**< Size in bytes of SPE code. */
};


/**
 * Tell SPUs about the framebuffer size, location
 */
struct cell_command_framebuffer
{
   uint64_t opcode;     /**< CELL_CMD_FRAMEBUFFER */
   int width, height;
   void *color_start, *depth_start;
   enum pipe_format color_format, depth_format;
};


/**
 * Clear framebuffer to the given value/color.
 */
struct cell_command_clear_surface
{
   uint64_t opcode;     /**< CELL_CMD_CLEAR_SURFACE */
   uint surface; /**< Temporary: 0=color, 1=Z */
   uint value;
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


struct cell_attribute_fetch_code {
   uint64_t base;
   uint size;
};


struct cell_buffer_range {
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
   uint64_t opcode;       /**< CELL_CMD_VS_EXECUTE */
   uint64_t vOut[SPU_VERTS_PER_BATCH];
   unsigned num_elts;
   unsigned elts[SPU_VERTS_PER_BATCH];
   float plane[12][4];
   unsigned nr_planes;
   unsigned nr_attrs;
};


struct cell_command_render
{
   uint64_t opcode;   /**< CELL_CMD_RENDER */
   uint prim_type;    /**< PIPE_PRIM_x */
   uint num_verts;
   uint vertex_size;  /**< bytes per vertex */
   uint num_indexes;
   uint vertex_buf;  /**< which cell->buffer[] contains the vertex data */
   float xmin, ymin, xmax, ymax;  /* XXX another dummy field */
   uint min_index;
   boolean inline_verts;
};


struct cell_command_release_verts
{
   uint64_t opcode;         /**< CELL_CMD_RELEASE_VERTS */
   uint vertex_buf;    /**< in [0, CELL_NUM_BUFFERS-1] */
};


struct cell_command_texture
{
   struct {
      void *start;         /**< Address in main memory */
      ushort width, height;
   } texture[CELL_MAX_SAMPLERS];
};


/** XXX unions don't seem to work */
/* XXX this should go away; all commands should be placed in batch buffers */
struct cell_command
{
#if 0
   struct cell_command_framebuffer fb;
   struct cell_command_clear_surface clear;
   struct cell_command_render render;
#endif
   struct cell_command_vs vs;
} ALIGN16_ATTRIB;


/** This is the object passed to spe_create_thread() */
struct cell_init_info
{
   unsigned id;
   unsigned num_spus;
   struct cell_command *cmd;

   /** Buffers for command batches, vertex/index data */
   ubyte *buffers[CELL_NUM_BUFFERS];
   uint *buffer_status;  /**< points at cell_context->buffer_status */
} ALIGN16_ATTRIB;


#endif /* CELL_COMMON_H */
