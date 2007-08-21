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
 *    Keith Whitwell
 */


#include "pipe/p_defines.h"
#include "pipe/p_context.h"
#include "pipe/p_winsys.h"
#include "pipe/p_util.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"
#include "pipe/draw/draw_prim.h"


/**
 * Draw vertex arrays
 * This is the main entrypoint into the drawing module.
 * \param prim  one of PIPE_PRIM_x
 * \param start  index of first vertex to draw
 * \param count  number of vertices to draw
 */
void
draw_arrays(struct draw_context *draw, unsigned prim,
            unsigned start, unsigned count)
{
   /* tell drawing pipeline we're beginning drawing */
   draw->pipeline.first->begin( draw->pipeline.first );

   draw_invalidate_vcache( draw );

   draw_set_prim( draw, prim );

   /* drawing done here: */
   draw_prim(draw, start, count);

   /* draw any left-over buffered prims */
   draw_flush(draw);

   /* tell drawing pipeline we're done drawing */
   draw->pipeline.first->end( draw->pipeline.first );
}


/* XXX move this into draw_context.c? */

#define EMIT_ATTR( VF_ATTR, STYLE, SIZE )			\
do {								\
   if (draw->nr_attrs >= 2)					\
      draw->vf_attr_to_slot[VF_ATTR] = draw->nr_attrs - 2;	\
   draw->attrs[draw->nr_attrs].attrib = VF_ATTR;		\
   /*draw->attrs[draw->nr_attrs].format = STYLE*/;              \
   draw->nr_attrs++;						\
   draw->vertex_size += SIZE;					\
} while (0)


void
draw_set_vertex_attributes( struct draw_context *draw,
                            const unsigned *slot_to_vf_attr,
                            unsigned nr_attrs )
{
   unsigned i;

   memset(draw->vf_attr_to_slot, 0, sizeof(draw->vf_attr_to_slot));
   draw->nr_attrs = 0;
   draw->vertex_size = 0;

   /*
    * First three attribs are always the same: header, clip pos, winpos
    */
   EMIT_ATTR(VF_ATTRIB_VERTEX_HEADER, EMIT_1F, 1);
   EMIT_ATTR(VF_ATTRIB_CLIP_POS, EMIT_4F, 4);

   assert(slot_to_vf_attr[0] == VF_ATTRIB_POS);
   EMIT_ATTR(slot_to_vf_attr[0], EMIT_4F_VIEWPORT, 4);

   /*
    * Remaining attribs (color, texcoords, etc)
    */
   for (i = 1; i < nr_attrs; i++) 
      EMIT_ATTR(slot_to_vf_attr[i], EMIT_4F, 4);

   draw->vertex_size *= 4; /* floats to bytes */
}
