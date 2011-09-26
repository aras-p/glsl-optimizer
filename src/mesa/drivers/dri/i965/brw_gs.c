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
      
#include "main/glheader.h"
#include "main/macros.h"
#include "main/enums.h"

#include "intel_batchbuffer.h"

#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_state.h"
#include "brw_gs.h"

#include "glsl/ralloc.h"

static void compile_gs_prog( struct brw_context *brw,
			     struct brw_gs_prog_key *key )
{
   struct intel_context *intel = &brw->intel;
   struct brw_gs_compile c;
   const GLuint *program;
   void *mem_ctx;
   GLuint program_size;

   /* Gen6: VF has already converted into polygon, and LINELOOP is
    * converted to LINESTRIP at the beginning of the 3D pipeline.
    */
   if (intel->gen >= 6)
      return;

   memset(&c, 0, sizeof(c));
   
   c.key = *key;
   /* The geometry shader needs to access the entire VUE. */
   struct brw_vue_map vue_map;
   brw_compute_vue_map(&vue_map, intel, c.key.userclip_active, c.key.attrs);
   c.nr_regs = (vue_map.num_slots + 1)/2;

   mem_ctx = NULL;
   
   /* Begin the compilation:
    */
   brw_init_compile(brw, &c.func, mem_ctx);

   c.func.single_program_flow = 1;

   /* For some reason the thread is spawned with only 4 channels
    * unmasked.  
    */
   brw_set_mask_control(&c.func, BRW_MASK_DISABLE);


   /* Note that primitives which don't require a GS program have
    * already been weeded out by this stage:
    */

   switch (key->primitive) {
   case _3DPRIM_QUADLIST:
      brw_gs_quads( &c, key );
      break;
   case _3DPRIM_QUADSTRIP:
      brw_gs_quad_strip( &c, key );
      break;
   case _3DPRIM_LINELOOP:
      brw_gs_lines( &c );
      break;
   default:
      ralloc_free(mem_ctx);
      return;
   }

   /* get the program
    */
   program = brw_get_program(&c.func, &program_size);

   if (unlikely(INTEL_DEBUG & DEBUG_GS)) {
      int i;

      printf("gs:\n");
      for (i = 0; i < program_size / sizeof(struct brw_instruction); i++)
	 brw_disasm(stdout, &((struct brw_instruction *)program)[i],
		    intel->gen);
      printf("\n");
    }

   brw_upload_cache(&brw->cache, BRW_GS_PROG,
		    &c.key, sizeof(c.key),
		    program, program_size,
		    &c.prog_data, sizeof(c.prog_data),
		    &brw->gs.prog_offset, &brw->gs.prog_data);
   ralloc_free(mem_ctx);
}

static const GLenum gs_prim[] = {
   [_3DPRIM_POINTLIST]  = _3DPRIM_POINTLIST,
   [_3DPRIM_LINELIST]   = _3DPRIM_LINELIST,
   [_3DPRIM_LINELOOP]   = _3DPRIM_LINELOOP,
   [_3DPRIM_LINESTRIP]  = _3DPRIM_LINELIST,
   [_3DPRIM_TRILIST]    = _3DPRIM_TRILIST,
   [_3DPRIM_TRISTRIP]   = _3DPRIM_TRILIST,
   [_3DPRIM_TRIFAN]     = _3DPRIM_TRILIST,
   [_3DPRIM_QUADLIST]   = _3DPRIM_QUADLIST,
   [_3DPRIM_QUADSTRIP]  = _3DPRIM_QUADSTRIP,
   [_3DPRIM_POLYGON]    = _3DPRIM_TRILIST,
   [_3DPRIM_RECTLIST]   = _3DPRIM_RECTLIST,
};

static void populate_key( struct brw_context *brw,
			  struct brw_gs_prog_key *key )
{
   struct gl_context *ctx = &brw->intel.ctx;
   struct intel_context *intel = &brw->intel;

   memset(key, 0, sizeof(*key));

   /* CACHE_NEW_VS_PROG */
   key->attrs = brw->vs.prog_data->outputs_written;

   /* BRW_NEW_PRIMITIVE */
   key->primitive = gs_prim[brw->primitive];

   /* _NEW_LIGHT */
   key->pv_first = (ctx->Light.ProvokingVertex == GL_FIRST_VERTEX_CONVENTION);
   if (key->primitive == _3DPRIM_QUADLIST && ctx->Light.ShadeModel != GL_FLAT) {
      /* Provide consistent primitive order with brw_set_prim's
       * optimization of single quads to trifans.
       */
      key->pv_first = GL_TRUE;
   }

   /* _NEW_TRANSFORM */
   key->userclip_active = (ctx->Transform.ClipPlanesEnabled != 0);

   key->need_gs_prog = (intel->gen >= 6)
      ? 0
      : (brw->primitive == _3DPRIM_QUADLIST ||
	 brw->primitive == _3DPRIM_QUADSTRIP ||
	 brw->primitive == _3DPRIM_LINELOOP);
}

/* Calculate interpolants for triangle and line rasterization.
 */
static void prepare_gs_prog(struct brw_context *brw)
{
   struct brw_gs_prog_key key;
   /* Populate the key:
    */
   populate_key(brw, &key);

   if (brw->gs.prog_active != key.need_gs_prog) {
      brw->state.dirty.cache |= CACHE_NEW_GS_PROG;
      brw->gs.prog_active = key.need_gs_prog;
   }

   if (brw->gs.prog_active) {
      if (!brw_search_cache(&brw->cache, BRW_GS_PROG,
			    &key, sizeof(key),
			    &brw->gs.prog_offset, &brw->gs.prog_data)) {
	 compile_gs_prog( brw, &key );
      }
   }
}


const struct brw_tracked_state brw_gs_prog = {
   .dirty = {
      .mesa  = (_NEW_LIGHT |
                _NEW_TRANSFORM),
      .brw   = BRW_NEW_PRIMITIVE,
      .cache = CACHE_NEW_VS_PROG
   },
   .prepare = prepare_gs_prog
};
