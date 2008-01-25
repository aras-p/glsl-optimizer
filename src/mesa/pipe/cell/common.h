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


/** for sanity checking */
#define ASSERT_ALIGN16(ptr) \
   assert((((unsigned long) (ptr)) & 0xf) == 0);



#define TILE_SIZE 32


/**
 * The low byte of a mailbox word contains the command opcode.
 * Remaining higher bytes are command specific.
 */
#define CELL_CMD_OPCODE_MASK 0xf

#define CELL_CMD_EXIT          1
#define CELL_CMD_FRAMEBUFFER   2
#define CELL_CMD_CLEAR_SURFACE 3
#define CELL_CMD_FINISH        4
#define CELL_CMD_RENDER        5
#define CELL_CMD_BATCH         6
#define CELL_CMD_STATE_DEPTH_STENCIL 7
#define CELL_CMD_STATE_SAMPLER       8


#define CELL_NUM_BATCH_BUFFERS 3
#define CELL_BATCH_BUFFER_SIZE 1024  /**< 16KB would be the max */

#define CELL_BUFFER_STATUS_FREE 10
#define CELL_BUFFER_STATUS_USED 20



/**
 * Tell SPUs about the framebuffer size, location
 */
struct cell_command_framebuffer
{
   uint opcode;
   int width, height;
   void *color_start, *depth_start;
   enum pipe_format color_format, depth_format;
} ALIGN16_ATTRIB;


/**
 * Clear framebuffer to the given value/color.
 */
struct cell_command_clear_surface
{
   uint opcode;
   uint surface; /**< Temporary: 0=color, 1=Z */
   uint value;
} ALIGN16_ATTRIB;


#define CELL_MAX_VBUF_SIZE    (16 * 1024)
#define CELL_MAX_VBUF_INDEXES 1024


struct cell_command_render
{
   uint opcode;
   uint prim_type;
   uint num_verts;
   uint vertex_size;  /**< bytes per vertex */
   uint dummy;       /* XXX this dummy field works around a compiler bug */
   uint num_indexes;
   const void *vertex_data;
   const ushort *index_data;
   float xmin, ymin, xmax, ymax;
} ALIGN16_ATTRIB;


/** XXX unions don't seem to work */
struct cell_command
{
   struct cell_command_framebuffer fb;
   struct cell_command_clear_surface clear;
   struct cell_command_render render;
} ALIGN16_ATTRIB;


/** This is the object passed to spe_create_thread() */
struct cell_init_info
{
   unsigned id;
   unsigned num_spus;
   struct cell_command *cmd;
   ubyte *batch_buffers[CELL_NUM_BATCH_BUFFERS];
   uint *buffer_status;  /**< points at cell_context->buffer_status */
} ALIGN16_ATTRIB;


#endif /* CELL_COMMON_H */
