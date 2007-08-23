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

/* Author:
 *    Brian Paul
 */


#ifndef DRAW_VERTEX_H
#define DRAW_VERTEX_H


/***
 *** XXX There's a lot of legacy tokens here that'll eventually go away.
 *** (at least we don't include vf/vf.h anymore)
 ***/


enum {
   VF_ATTRIB_POS = 0,
   VF_ATTRIB_WEIGHT = 1,
   VF_ATTRIB_NORMAL = 2,
   VF_ATTRIB_COLOR0 = 3,
   VF_ATTRIB_COLOR1 = 4,
   VF_ATTRIB_FOG = 5,
   VF_ATTRIB_COLOR_INDEX = 6,
   VF_ATTRIB_EDGEFLAG = 7,
   VF_ATTRIB_TEX0 = 8,
   VF_ATTRIB_TEX1 = 9,
   VF_ATTRIB_TEX2 = 10,
   VF_ATTRIB_TEX3 = 11,
   VF_ATTRIB_TEX4 = 12,
   VF_ATTRIB_TEX5 = 13,
   VF_ATTRIB_TEX6 = 14,
   VF_ATTRIB_TEX7 = 15,
   VF_ATTRIB_VAR0 = 16,
   VF_ATTRIB_VAR1 = 17,
   VF_ATTRIB_VAR2 = 18,
   VF_ATTRIB_VAR3 = 19,
   VF_ATTRIB_VAR4 = 20,
   VF_ATTRIB_VAR5 = 21,
   VF_ATTRIB_VAR6 = 22,
   VF_ATTRIB_VAR7 = 23,
   VF_ATTRIB_POINTSIZE = 24,
   VF_ATTRIB_BFC0 = 25,
   VF_ATTRIB_BFC1 = 26,
   VF_ATTRIB_CLIP_POS = 27,
   VF_ATTRIB_VERTEX_HEADER = 28,
   VF_ATTRIB_MAX = 29
};

#define MAX_VARYING 8
enum
{
   FRAG_ATTRIB_WPOS = 0,
   FRAG_ATTRIB_COL0 = 1,
   FRAG_ATTRIB_COL1 = 2,
   FRAG_ATTRIB_FOGC = 3,
   FRAG_ATTRIB_TEX0 = 4,
   FRAG_ATTRIB_TEX1 = 5,
   FRAG_ATTRIB_TEX2 = 6,
   FRAG_ATTRIB_TEX3 = 7,
   FRAG_ATTRIB_TEX4 = 8,
   FRAG_ATTRIB_TEX5 = 9,
   FRAG_ATTRIB_TEX6 = 10,
   FRAG_ATTRIB_TEX7 = 11,
   FRAG_ATTRIB_VAR0 = 12,  /**< shader varying */
   FRAG_ATTRIB_MAX = (FRAG_ATTRIB_VAR0 + MAX_VARYING)
};

#define FRAG_BIT_WPOS  (1 << FRAG_ATTRIB_WPOS)
#define FRAG_BIT_COL0  (1 << FRAG_ATTRIB_COL0)
#define FRAG_BIT_COL1  (1 << FRAG_ATTRIB_COL1)
#define FRAG_BIT_FOGC  (1 << FRAG_ATTRIB_FOGC)
#define FRAG_BIT_TEX0  (1 << FRAG_ATTRIB_TEX0)



#define MAX_DRAW_BUFFERS 4

enum
{
   FRAG_RESULT_COLR = 0,
   FRAG_RESULT_COLH = 1,
   FRAG_RESULT_DEPR = 2,
   FRAG_RESULT_DATA0 = 3,
   FRAG_RESULT_MAX = (FRAG_RESULT_DATA0 + MAX_DRAW_BUFFERS)
};



#define MAX_VERT_ATTRIBS 12  /* OK? */

#define FORMAT_OMIT 0
#define FORMAT_1F   1
#define FORMAT_2F   2
#define FORMAT_3F   3
#define FORMAT_4F   4
#define FORMAT_4F_VIEWPORT   4
#define FORMAT_4UB  5


struct vertex_info
{
   uint num_attribs;
   uint hwfmt[2];      /**< hardware format info for this format */
   uint attr_mask;     /**< mask of VF_ATTR_ bits */
   uint slot_to_attrib[MAX_VERT_ATTRIBS];
   uint attrib_to_slot[VF_ATTRIB_MAX];
   uint interp_mode[MAX_VERT_ATTRIBS];
   uint format[MAX_VERT_ATTRIBS];   /**< FORMAT_x */
   uint size;          /**< total vertex size in dwords */
};



#endif /* DRAW_VERTEX_H */
