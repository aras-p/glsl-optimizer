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
      
#include "brw_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_state.h"
#include "brw_gs.h"



static enum pipe_error compile_gs_prog( struct brw_context *brw,
                                        struct brw_gs_prog_key *key,
                                        struct brw_winsys_buffer **bo_out )
{
   struct brw_gs_compile c;
   enum pipe_error ret;
   const GLuint *program;
   GLuint program_size;

   memset(&c, 0, sizeof(c));
   
   c.key = *key;
   c.need_ff_sync = BRW_IS_IGDNG(brw);
   /* Need to locate the two positions present in vertex + header.
    * These are currently hardcoded:
    */
   c.nr_attrs = c.key.nr_attrs;

   if (BRW_IS_IGDNG(brw))
      c.nr_regs = (c.nr_attrs + 1) / 2 + 3;  /* are vertices packed, or reg-aligned? */
   else
      c.nr_regs = (c.nr_attrs + 1) / 2 + 1;  /* are vertices packed, or reg-aligned? */

   c.nr_bytes = c.nr_regs * REG_SIZE;

   
   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func);

   c.func.single_program_flow = 1;

   /* For some reason the thread is spawned with only 4 channels
    * unmasked.  
    */
   brw_set_mask_control(&c.func, BRW_MASK_DISABLE);


   /* Note that primitives which don't require a GS program have
    * already been weeded out by this stage:
    */
   switch (key->primitive) {
   case PIPE_PRIM_QUADS:
      brw_gs_quads( &c ); 
      break;
   case PIPE_PRIM_QUAD_STRIP:
      brw_gs_quad_strip( &c );
      break;
   case PIPE_PRIM_LINE_LOOP:
      brw_gs_lines( &c );
      break;
   case PIPE_PRIM_LINES:
      if (key->hint_gs_always)
	 brw_gs_lines( &c );
      else {
	 return PIPE_OK;
      }
      break;
   case PIPE_PRIM_TRIANGLES:
      if (key->hint_gs_always)
	 brw_gs_tris( &c );
      else {
	 return PIPE_OK;
      }
      break;
   case PIPE_PRIM_POINTS:
      if (key->hint_gs_always)
	 brw_gs_points( &c );
      else {
	 return PIPE_OK;
      }
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
   ret = brw_upload_cache( &brw->cache, BRW_GS_PROG,
                           &c.key, sizeof(c.key),
                           NULL, 0,
                           program, program_size,
                           &c.prog_data,
                           &brw->gs.prog_data,
                           bo_out );
   if (ret)
      return ret;

   return PIPE_OK;
}

static const unsigned gs_prim[PIPE_PRIM_MAX] = {  
   PIPE_PRIM_POINTS,
   PIPE_PRIM_LINES,
   PIPE_PRIM_LINE_LOOP,
   PIPE_PRIM_LINES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_TRIANGLES,
   PIPE_PRIM_QUADS,
   PIPE_PRIM_QUAD_STRIP,
   PIPE_PRIM_TRIANGLES
};

static void populate_key( struct brw_context *brw,
			  struct brw_gs_prog_key *key )
{
   const struct brw_fs_signature *sig = &brw->curr.fragment_shader->signature;

   memset(key, 0, sizeof(*key));

   /* PIPE_NEW_FRAGMENT_SIGNATURE */
   key->nr_attrs = sig->nr_inputs + 1;

   /* BRW_NEW_PRIMITIVE */
   key->primitive = gs_prim[brw->primitive];

   key->hint_gs_always = 0;	/* debug code? */

   key->need_gs_prog = (key->hint_gs_always ||
			brw->primitive == PIPE_PRIM_QUADS ||
			brw->primitive == PIPE_PRIM_QUAD_STRIP ||
			brw->primitive == PIPE_PRIM_LINE_LOOP);
}

/* Calculate interpolants for triangle and line rasterization.
 */
static int prepare_gs_prog(struct brw_context *brw)
{
   struct brw_gs_prog_key key;
   enum pipe_error ret;

   /* Populate the key:
    */
   populate_key(brw, &key);

   if (brw->gs.prog_active != key.need_gs_prog) {
      brw->state.dirty.cache |= CACHE_NEW_GS_PROG;
      brw->gs.prog_active = key.need_gs_prog;
   }

   if (!brw->gs.prog_active)
      return PIPE_OK;

   if (brw_search_cache(&brw->cache, BRW_GS_PROG,
                        &key, sizeof(key),
                        NULL, 0,
                        &brw->gs.prog_data,
                        &brw->gs.prog_bo))
      return PIPE_OK;

   ret = compile_gs_prog( brw, &key, &brw->gs.prog_bo );
   if (ret)
      return ret;

   return PIPE_OK;
}


const struct brw_tracked_state brw_gs_prog = {
   .dirty = {
      .mesa  = PIPE_NEW_FRAGMENT_SIGNATURE,
      .brw   = BRW_NEW_PRIMITIVE,
      .cache = 0,
   },
   .prepare = prepare_gs_prog
};
