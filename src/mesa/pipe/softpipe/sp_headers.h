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
#define QUAD_SIZE        (2*2)

#define MASK_BOTTOM_LEFT  0x1
#define MASK_BOTTOM_RIGHT 0x2
#define MASK_TOP_LEFT     0x4
#define MASK_TOP_RIGHT    0x8
#define MASK_ALL          0xf


#define NUM_CHANNELS   4	/* avoid confusion between 4 pixels and 4 channels */


struct setup_coefficient {
   GLfloat a0[NUM_CHANNELS];	/* in an xyzw layout */
   GLfloat dadx[NUM_CHANNELS];
   GLfloat dady[NUM_CHANNELS];
};



/* Encodes everything we need to know about a 2x2 pixel block.  Uses
 * "Channel-Serial" or "SoA" layout.  
 *
 * Will expand to include non-attribute things like AA coverage and
 * maybe prefetched depth from the depth buffer.
 */
struct quad_header {
   GLint x0;
   GLint y0;
   GLuint mask;
   GLuint facing;   /**< Front (0) or back (1) facing? */

   struct {
      GLfloat color[4][QUAD_SIZE];	/* rrrr, gggg, bbbb, aaaa */
      GLfloat depth[QUAD_SIZE];
   } outputs;

   const struct setup_coefficient *coef;

   const enum interp_mode *interp; /* XXX: this information should be
				    * encoded in fragment program DECL
				    * statements. */

   GLuint nr_attrs;
};


#endif /* SP_HEADERS_H */
