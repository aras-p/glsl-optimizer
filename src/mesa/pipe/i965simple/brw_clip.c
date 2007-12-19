/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_clip.h"

#define FRONT_UNFILLED_BIT  0x1
#define BACK_UNFILLED_BIT   0x2


static void compile_clip_prog( struct brw_context *brw,
			     struct brw_clip_prog_key *key )
{
   struct brw_clip_compile c;
   const unsigned *program;
   unsigned program_size;
   unsigned delta;
   unsigned i;

   memset(&c, 0, sizeof(c));

   /* Begin the compilation:
    */
   brw_init_compile(&c.func);

   c.func.single_program_flow = 1;

   c.key = *key;


   /* Need to locate the two positions present in vertex + header.
    * These are currently hardcoded:
    */
   c.header_position_offset = ATTR_SIZE;

   for (i = 0, delta = REG_SIZE; i < PIPE_MAX_SHADER_OUTPUTS; i++)
      if (c.key.attrs & (1<<i)) {
	 c.offset[i] = delta;
	 delta += ATTR_SIZE;
      }

   c.nr_attrs = brw_count_bits(c.key.attrs);
   c.nr_regs = (c.nr_attrs + 1) / 2 + 1;  /* are vertices packed, or reg-aligned? */
   c.nr_bytes = c.nr_regs * REG_SIZE;

   c.prog_data.clip_mode = c.key.clip_mode; /* XXX */

   /* For some reason the thread is spawned with only 4 channels
    * unmasked.
    */
   brw_set_mask_control(&c.func, BRW_MASK_DISABLE);


   /* Would ideally have the option of producing a program which could
    * do all three:
    */
   switch (key->primitive) {
   case PIPE_PRIM_TRIANGLES:
#if 0
      if (key->do_unfilled)
	 brw_emit_unfilled_clip( &c );
      else
#endif
	 brw_emit_tri_clip( &c );
      break;
   case PIPE_PRIM_LINES:
      brw_emit_line_clip( &c );
      break;
   case PIPE_PRIM_POINTS:
      brw_emit_point_clip( &c );
      break;
   default:
      assert(0);
      return;
   }



   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   /* Upload
    */
   brw->clip.prog_gs_offset = brw_upload_cache( &brw->cache[BRW_CLIP_PROG],
						&c.key,
						sizeof(c.key),
						program,
						program_size,
						&c.prog_data,
						&brw->clip.prog_data );
}


static boolean search_cache( struct brw_context *brw,
			       struct brw_clip_prog_key *key )
{
   return brw_search_cache(&brw->cache[BRW_CLIP_PROG],
			   key, sizeof(*key),
			   &brw->clip.prog_data,
			   &brw->clip.prog_gs_offset);
}




/* Calculate interpolants for triangle and line rasterization.
 */
static void upload_clip_prog(struct brw_context *brw)
{
   struct brw_clip_prog_key key;

   memset(&key, 0, sizeof(key));

   /* Populate the key:
    */
   /* BRW_NEW_REDUCED_PRIMITIVE */
   key.primitive = brw->reduced_primitive;
   /* CACHE_NEW_VS_PROG */
   key.attrs = brw->vs.prog_data->outputs_written;
   /* BRW_NEW_RASTER */
   key.do_flat_shading = (brw->attribs.Raster->flatshade);
   /* BRW_NEW_CLIP */
   key.nr_userclip = brw->attribs.Clip.nr; /* XXX */

#if 0
   key.clip_mode = BRW_CLIPMODE_NORMAL;

   if (key.primitive == PIPE_PRIM_TRIANGLES) {
      if (brw->attribs.Raster->cull_mode == PIPE_WINDING_BOTH)
	 key.clip_mode = BRW_CLIPMODE_REJECT_ALL;
      else {
         if (brw->attribs.Raster->fill_cw != PIPE_POLYGON_MODE_FILL ||
             brw->attribs.Raster->fill_ccw != PIPE_POLYGON_MODE_FILL)
            key.do_unfilled = 1;

	 /* Most cases the fixed function units will handle.  Cases where
	  * one or more polygon faces are unfilled will require help:
	  */
	 if (key.do_unfilled) {
	    key.clip_mode = BRW_CLIPMODE_CLIP_NON_REJECTED;

	    if (brw->attribs.Raster->offset_cw ||
                brw->attribs.Raster->offset_ccw) {
	       key.offset_units = brw->attribs.Raster->offset_units;
	       key.offset_factor = brw->attribs.Raster->offset_scale;
	    }
            key.fill_ccw = brw->attribs.Raster->fill_ccw;
            key.fill_cw = brw->attribs.Raster->fill_cw;
            key.offset_ccw = brw->attribs.Raster->offset_ccw;
            key.offset_cw = brw->attribs.Raster->offset_cw;
            if (brw->attribs.Raster->light_twoside &&
                key.fill_cw != CLIP_CULL)
               key.copy_bfc_cw = 1;
	 }
      }
   }
#else
   key.clip_mode = BRW_CLIPMODE_ACCEPT_ALL;
#endif

   if (!search_cache(brw, &key))
      compile_clip_prog( brw, &key );
}

const struct brw_tracked_state brw_clip_prog = {
   .dirty = {
      .brw   = (BRW_NEW_RASTERIZER |
		BRW_NEW_CLIP |
		BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG
   },
   .update = upload_clip_prog
};
