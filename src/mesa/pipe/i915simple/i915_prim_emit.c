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


#include "pipe/draw/draw_private.h"
#include "pipe/p_util.h"

#include "i915_context.h"
#include "i915_winsys.h"
#include "i915_reg.h"
#include "i915_state.h"
#include "i915_batch.h"



/**
 * Primitive emit to hardware.  No support for vertex buffers or any
 * nice fast paths.
 */
struct setup_stage {
   struct draw_stage stage; /**< This must be first (base class) */

   struct i915_context *i915;   
};



/**
 * Basically a cast wrapper.
 */
static INLINE struct setup_stage *setup_stage( struct draw_stage *stage )
{
   return (struct setup_stage *)stage;
}


#if 0
/* Hardcoded vertex format: xyz/rgba
 */
static INLINE void
emit_hw_vertex( struct i915_context *i915,
		struct vertex_header *vertex )
{
   OUT_BATCH( fui(vertex->data[0][0]) );
   OUT_BATCH( fui(vertex->data[0][1]) );
   OUT_BATCH( fui(vertex->data[0][2]) );

   /* colors are ARGB (MSB to LSB) */
   OUT_BATCH( pack_ub4(float_to_ubyte( vertex->data[1][2] ),
                       float_to_ubyte( vertex->data[1][1] ),
                       float_to_ubyte( vertex->data[1][0] ),
                       float_to_ubyte( vertex->data[1][3] )) );
}
#endif


/**
 * Extract the needed fields from vertex_header and emit i915 dwords.
 * Recall that the vertices are constructed by the 'draw' module and
 * have a couple of slots at the beginning (1-dword header, 4-dword
 * clip pos) that we ignore here.
 */
static INLINE void
emit_hw_vertex2( struct i915_context *i915,
                 const struct vertex_header *vertex)
{
   const struct vertex_info *vinfo = &i915->current.vertex_info;
   uint i;
   uint count = 0;  /* for debug/sanity */

   for (i = 0; i < vinfo->num_attribs; i++) {
      switch (vinfo->format[i]) {
      case FORMAT_OMIT:
         /* no-op */
         break;
      case FORMAT_1F:
         OUT_BATCH( fui(vertex->data[i][0]) );
         count++;
         break;
      case FORMAT_2F:
         OUT_BATCH( fui(vertex->data[i][0]) );
         OUT_BATCH( fui(vertex->data[i][1]) );
         count += 2;
         break;
      case FORMAT_3F:
         OUT_BATCH( fui(vertex->data[i][0]) );
         OUT_BATCH( fui(vertex->data[i][1]) );
         OUT_BATCH( fui(vertex->data[i][2]) );
         count += 3;
         break;
      case FORMAT_4F:
         OUT_BATCH( fui(vertex->data[i][0]) );
         OUT_BATCH( fui(vertex->data[i][1]) );
         OUT_BATCH( fui(vertex->data[i][2]) );
         OUT_BATCH( fui(vertex->data[i][3]) );
         count += 4;
         break;
      case FORMAT_4UB:
         OUT_BATCH( pack_ub4(float_to_ubyte( vertex->data[i][2] ),
                             float_to_ubyte( vertex->data[i][1] ),
                             float_to_ubyte( vertex->data[i][0] ),
                             float_to_ubyte( vertex->data[i][3] )) );
         count += 1;
         break;
      default:
         assert(0);
      }
   }
   assert(count == vinfo->size);
}



static INLINE void 
emit_prim( struct draw_stage *stage, 
	   struct prim_header *prim,
	   unsigned hwprim,
	   unsigned nr )
{
   struct i915_context *i915 = setup_stage(stage)->i915;
#if 0
   unsigned vertex_size = 4 * sizeof(int);
#else
   unsigned vertex_size = i915->current.vertex_info.size * 4; /* in bytes */
#endif
   unsigned *ptr;
   unsigned i;

   assert(vertex_size >= 12); /* never smaller than 12 bytes */

   if (i915->dirty)
      i915_update_derived( i915 );

   if (i915->hardware_dirty)
      i915_emit_hardware_state( i915 );

   ptr = BEGIN_BATCH( nr * vertex_size, 0 );
   if (ptr == 0) {
      FLUSH_BATCH();

      /* Make sure state is re-emitted after a flush: 
       */
      i915_update_derived( i915 );
      i915_emit_hardware_state( i915 );

      ptr = BEGIN_BATCH( nr * vertex_size, 0 );
      if (ptr == 0) {
	 assert(0);
	 return;
      }
   }

   /* Emit each triangle as a single primitive.  I told you this was
    * simple.
    */
   OUT_BATCH(_3DPRIMITIVE | 
	     hwprim |
	     ((4 + vertex_size * nr)/4 - 2));

   for (i = 0; i < nr; i++) {
      emit_hw_vertex2(i915, prim->v[i]);
      ptr += vertex_size / sizeof(int);
   }
}


static void 
setup_tri( struct draw_stage *stage, struct prim_header *prim )
{
   emit_prim( stage, prim, PRIM3D_TRILIST, 3 );
}


static void
setup_line(struct draw_stage *stage, struct prim_header *prim)
{
   emit_prim( stage, prim, PRIM3D_LINELIST, 2 );
}


static void
setup_point(struct draw_stage *stage, struct prim_header *prim)
{
   emit_prim( stage, prim, PRIM3D_POINTLIST, 1 );
}



static void setup_begin( struct draw_stage *stage )
{
}


static void setup_end( struct draw_stage *stage )
{
}

static void reset_stipple_counter( struct draw_stage *stage )
{
}


/**
 * Create a new primitive setup/render stage.
 */
struct draw_stage *i915_draw_render_stage( struct i915_context *i915 )
{
   struct setup_stage *setup = CALLOC_STRUCT(setup_stage);

   setup->i915 = i915;
   setup->stage.draw = i915->draw;
   setup->stage.begin = setup_begin;
   setup->stage.point = setup_point;
   setup->stage.line = setup_line;
   setup->stage.tri = setup_tri;
   setup->stage.end = setup_end;
   setup->stage.reset_stipple_counter = reset_stipple_counter;

   return &setup->stage;
}
