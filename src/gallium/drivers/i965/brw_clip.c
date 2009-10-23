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

#include "pipe/p_state.h"

#include "util/u_math.h"

#include "intel_batchbuffer.h"

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
   const GLuint *program;
   GLuint program_size;
   GLuint delta;
   GLuint i;

   memset(&c, 0, sizeof(c));
   
   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func);

   c.func.single_program_flow = 1;

   c.key = *key;
   c.need_ff_sync = BRW_IS_IGDNG(brw);

   /* Need to locate the two positions present in vertex + header.
    * These are currently hardcoded:
    */
   c.header_position_offset = ATTR_SIZE;

   if (BRW_IS_IGDNG(brw))
       delta = 3 * REG_SIZE;
   else
       delta = REG_SIZE;

   for (i = 0; i < VERT_RESULT_MAX; i++)
      if (c.key.attrs & (1<<i)) {
	 c.offset[i] = delta;
	 delta += ATTR_SIZE;
      }

   c.nr_attrs = util_count_bits(c.key.attrs);
   
   if (BRW_IS_IGDNG(brw))
       c.nr_regs = (c.nr_attrs + 1) / 2 + 3;  /* are vertices packed, or reg-aligned? */
   else
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
      if (key->do_unfilled)
	 brw_emit_unfilled_clip( &c );
      else
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
   dri_bo_unreference(brw->clip.prog_bo);
   brw->clip.prog_bo = brw_upload_cache( &brw->cache,
					 BRW_CLIP_PROG,
					 &c.key, sizeof(c.key),
					 NULL, 0,
					 program, program_size,
					 &c.prog_data,
					 &brw->clip.prog_data );
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
   /* PIPE_NEW_RAST */
   key.do_flat_shading = brw->rast.base.flatshade;
   /* PIPE_NEW_UCP */
   key.nr_userclip = brw->nr_ucp;

   if (BRW_IS_IGDNG(brw))
       key.clip_mode = BRW_CLIPMODE_KERNEL_CLIP;
   else
       key.clip_mode = BRW_CLIPMODE_NORMAL;

   /* PIPE_NEW_RAST */
   if (key.primitive == PIPE_PRIM_TRIANGLES) {
      if (brw->rast->cull_mode = PIPE_WINDING_BOTH)
	 key.clip_mode = BRW_CLIPMODE_REJECT_ALL;
      else {
	 key.fill_ccw = CLIP_CULL;
	 key.fill_cw = CLIP_CULL;

	 if (!(brw->rast->cull_mode & PIPE_WINDING_CCW)) {
	    key.fill_ccw = translate_fill(brw->rast.fill_ccw);
	 }

	 if (!(brw->rast->cull_mode & PIPE_WINDING_CW)) {
	    key.fill_cw = translate_fill(brw->rast.fill_cw);
	 }

	 if (key.fill_cw != CLIP_FILL ||
	     key.fill_ccw != CLIP_FILL) {
	    key.do_unfilled = 1;
	    key.clip_mode = BRW_CLIPMODE_CLIP_NON_REJECTED;
	 }

	 key.offset_ccw = brw->rast.offset_ccw;
	 key.offset_cw = brw->rast.offset_cw;

	 if (brw->rast.light_twoside &&
	     key.fill_cw != CLIP_CULL) 
	    key.copy_bfc_cw = 1;

	 if (brw->rast.light_twoside &&
	     key.fill_ccw != CLIP_CULL) 
	    key.copy_bfc_ccw = 1;
	 }
      }
   }

   dri_bo_unreference(brw->clip.prog_bo);
   brw->clip.prog_bo = brw_search_cache(&brw->cache, BRW_CLIP_PROG,
					&key, sizeof(key),
					NULL, 0,
					&brw->clip.prog_data);
   if (brw->clip.prog_bo == NULL)
      compile_clip_prog( brw, &key );
}


const struct brw_tracked_state brw_clip_prog = {
   .dirty = {
      .mesa  = (PIPE_NEW_RAST | 
		PIPE_NEW_UCP),
      .brw   = (BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = upload_clip_prog
};
