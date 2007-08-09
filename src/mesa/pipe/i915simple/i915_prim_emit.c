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


#include "imports.h"
#include "macros.h"

#include "pipe/draw/draw_private.h"

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

static INLINE unsigned pack_ub4( unsigned char b0,
				 unsigned char b1,
				 unsigned char b2,
				 unsigned char b3 )
{
   return ((((unsigned int)b0) << 0) |
	   (((unsigned int)b1) << 8) |
	   (((unsigned int)b2) << 16) |
	   (((unsigned int)b3) << 24));
}

static INLINE unsigned fui( float f )
{
   union {
      float f;
      unsigned ui;
   } fi;

   fi.f = f;
   return fi.ui;
}

static INLINE unsigned char float_to_ubyte( float f )
{
   unsigned char ub;
   UNCLAMPED_FLOAT_TO_UBYTE(ub, f);
   return ub;
}


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
		
		


static INLINE void 
emit_prim( struct draw_stage *stage, 
	   struct prim_header *prim,
	   unsigned hwprim,
	   unsigned nr )
{
   struct i915_context *i915 = setup_stage(stage)->i915;
   struct i915_winsys *winsys = i915->winsys;
   unsigned vertex_size = 4 * sizeof(int);
   unsigned *ptr;
   unsigned i;

   if (i915->dirty)
      i915_update_derived( i915 );

   if (i915->hardware_dirty)
      i915_emit_hardware_state( i915 );

   ptr = winsys->batch_start( winsys, nr * vertex_size, 0 );
   if (ptr == 0) {
      winsys->batch_flush( winsys );
      ptr = winsys->batch_start( winsys, nr * vertex_size, 0 );
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
      emit_hw_vertex(i915, prim->v[i]);
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
