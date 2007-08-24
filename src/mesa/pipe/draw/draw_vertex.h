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

#include "pipe/tgsi/exec/tgsi_attribs.h"


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
   uint hwfmt[4];      /**< hardware format info for this format */
   uint attr_mask;     /**< mask of VF_ATTR_ bits */
   uint slot_to_attrib[MAX_VERT_ATTRIBS];
   uint attrib_to_slot[TGSI_ATTRIB_MAX];
   uint interp_mode[MAX_VERT_ATTRIBS];
   uint format[MAX_VERT_ATTRIBS];   /**< FORMAT_x */
   uint size;          /**< total vertex size in dwords */
};



#endif /* DRAW_VERTEX_H */
