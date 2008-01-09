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


#define CELL_CMD_EXIT          1
#define CELL_CMD_FRAMEBUFFER   2
#define CELL_CMD_CLEAR_SURFACE 3
#define CELL_CMD_FINISH        4
#define CELL_CMD_RENDER        5


/**
 * Tell SPUs about the framebuffer size, location
 */
struct cell_command_framebuffer
{
   int width, height;
   void *color_start, *depth_start;
   enum pipe_format color_format, depth_format;
} ALIGN16_ATTRIB;


/**
 * Clear framebuffer to the given value/color.
 */
struct cell_command_clear_surface
{
   uint surface; /**< Temporary: 0=color, 1=Z */
   uint value;
} ALIGN16_ATTRIB;


struct cell_command_render
{
   uint prim_type;
   uint num_verts, num_attribs;
   float xmin, ymin, xmax, ymax;
   void *vertex_data;
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
} ALIGN16_ATTRIB;


/** Temporary */
#define CELL_MAX_VERTS 48
#define CELL_MAX_ATTRIBS 2
struct cell_prim_buffer
{
   float vertex[CELL_MAX_VERTS][CELL_MAX_ATTRIBS][4] ALIGN16_ATTRIB;
   float xmin, ymin, xmax, ymax;
   uint num_verts;
} ALIGN16_ATTRIB;



#endif /* CELL_COMMON_H */
