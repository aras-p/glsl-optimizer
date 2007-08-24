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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef SP_HEADERS_H
#define SP_HEADERS_H

#include "../tgsi/exec/tgsi_core.h"

#define PRIM_POINT 1
#define PRIM_LINE  2
#define PRIM_TRI   3


/* The rasterizer generates 2x2 quads of fragment and feeds them to
 * the current fp_machine (see below).  
 */
#define QUAD_BOTTOM_LEFT  0
#define QUAD_BOTTOM_RIGHT 1
#define QUAD_TOP_LEFT     2
#define QUAD_TOP_RIGHT    3

#define MASK_BOTTOM_LEFT  0x1
#define MASK_BOTTOM_RIGHT 0x2
#define MASK_TOP_LEFT     0x4
#define MASK_TOP_RIGHT    0x8
#define MASK_ALL          0xf


/**
 * Encodes everything we need to know about a 2x2 pixel block.  Uses
 * "Channel-Serial" or "SoA" layout.  
 */
struct quad_header {
   int x0;
   int y0;
   unsigned mask:4;
   unsigned facing:1;   /**< Front (0) or back (1) facing? */
   unsigned prim:2;     /**< PRIM_POINT, LINE, TRI */

   struct {
      float color[NUM_CHANNELS][QUAD_SIZE];	/* rrrr, gggg, bbbb, aaaa */
      float depth[QUAD_SIZE];
   } outputs;

   float coverage[QUAD_SIZE];    /** fragment coverage for antialiasing */

   const struct tgsi_interp_coef *coef;

   unsigned nr_attrs;
};


#endif /* SP_HEADERS_H */
