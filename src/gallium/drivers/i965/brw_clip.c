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

#include "brw_screen.h"
#include "brw_batchbuffer.h"
#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_pipe_rast.h"
#include "brw_clip.h"


#define FRONT_UNFILLED_BIT  0x1
#define BACK_UNFILLED_BIT   0x2


static enum pipe_error
compile_clip_prog( struct brw_context *brw,
                   struct brw_clip_prog_key *key,
                   struct brw_winsys_buffer **bo_out )
{
   enum pipe_error ret;
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

   c.chipset = brw->chipset;
   c.key = *key;
   c.need_ff_sync = c.chipset.is_igdng;

   /* Need to locate the two positions present in vertex + header.
    * These are currently hardcoded:
    */
   c.header_position_offset = ATTR_SIZE;

   if (c.chipset.is_igdng)
       delta = 3 * REG_SIZE;
   else
       delta = REG_SIZE;

   /* XXX: c.offset is now pretty redundant:
    */
   for (i = 0; i < c.key.nr_attrs; i++) {
      c.offset[i] = delta;
      delta += ATTR_SIZE;
   }

   /* XXX: c.nr_attrs is very redundant:
    */
   c.nr_attrs = c.key.nr_attrs;
   
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
      return PIPE_ERROR_BAD_INPUT;
   }

	 

   /* get the program
    */
   ret = brw_get_program(&c.func, &program, &program_size);
   if (ret)
      return ret;

   /* Upload
    */
   ret = brw_upload_cache( &brw->cache,
                           BRW_CLIP_PROG,
                           &c.key, sizeof(c.key),
                           NULL, 0,
                           program, program_size,
                           &c.prog_data,
                           &brw->clip.prog_data,
                           bo_out );
   if (ret)
      return ret;

   return PIPE_OK;
}

/* Calculate interpolants for triangle and line rasterization.
 */
static enum pipe_error
upload_clip_prog(struct brw_context *brw)
{
   enum pipe_error ret;
   struct brw_clip_prog_key key;

   /* Populate the key, starting from the almost-complete version from
    * the rast state. 
    */

   /* PIPE_NEW_RAST */
   memcpy(&key, &brw->curr.rast->clip_key, sizeof key);

   /* BRW_NEW_REDUCED_PRIMITIVE */
   key.primitive = brw->reduced_primitive;

   /* PIPE_NEW_VS */
   key.nr_attrs = brw->curr.vertex_shader->info.file_max[TGSI_FILE_OUTPUT] + 1;

   /* PIPE_NEW_CLIP */
   key.nr_userclip = brw->curr.ucp.nr;

   /* Already cached?
    */
   if (brw_search_cache(&brw->cache, BRW_CLIP_PROG,
                        &key, sizeof(key),
                        NULL, 0,
                        &brw->clip.prog_data,
                        &brw->clip.prog_bo))
      return PIPE_OK;

   /* Compile new program:
    */
   ret = compile_clip_prog( brw, &key, &brw->clip.prog_bo );
   if (ret)
      return ret;

   return PIPE_OK;
}


const struct brw_tracked_state brw_clip_prog = {
   .dirty = {
      .mesa  = (PIPE_NEW_RAST | 
		PIPE_NEW_CLIP),
      .brw   = (BRW_NEW_REDUCED_PRIMITIVE),
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = upload_clip_prog
};
