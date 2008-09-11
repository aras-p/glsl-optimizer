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

/* Authors:
 *   Zack Rusin
 */

#include "sp_context.h"
#include "sp_state.h"
#include "sp_fs.h"


#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "pipe/p_inlines.h"
#include "tgsi/tgsi_sse2.h"

#if 0

struct sp_llvm_fragment_shader {
   struct sp_fragment_shader base;
   struct gallivm_prog *llvm_prog;
};

static void
shade_quad_llvm(struct quad_stage *qs,
                struct quad_header *quad)
{
   struct quad_shade_stage *qss = quad_shade_stage(qs);
   struct softpipe_context *softpipe = qs->softpipe;
   float dests[4][16][4] ALIGN16_ATTRIB;
   float inputs[4][16][4] ALIGN16_ATTRIB;
   const float fx = (float) quad->x0;
   const float fy = (float) quad->y0;
   struct gallivm_prog *llvm = qss->llvm_prog;

   inputs[0][0][0] = fx;
   inputs[1][0][0] = fx + 1.0f;
   inputs[2][0][0] = fx;
   inputs[3][0][0] = fx + 1.0f;

   inputs[0][0][1] = fy;
   inputs[1][0][1] = fy;
   inputs[2][0][1] = fy + 1.0f;
   inputs[3][0][1] = fy + 1.0f;


   gallivm_prog_inputs_interpolate(llvm, inputs, quad->coef);

#if DLLVM
   debug_printf("MASK = %d\n", quad->mask);
   for (int i = 0; i < 4; ++i) {
      for (int j = 0; j < 2; ++j) {
         debug_printf("IN(%d,%d) [%f %f %f %f]\n", i, j, 
                inputs[i][j][0], inputs[i][j][1], inputs[i][j][2], inputs[i][j][3]);
      }
   }
#endif

   quad->mask &=
      gallivm_fragment_shader_exec(llvm, fx, fy, dests, inputs,
                                   softpipe->mapped_constants[PIPE_SHADER_FRAGMENT],
                                   qss->samplers);
#if DLLVM
   debug_printf("OUT LLVM = 1[%f %f %f %f], 2[%f %f %f %f]\n",
          dests[0][0][0], dests[0][0][1], dests[0][0][2], dests[0][0][3], 
          dests[0][1][0], dests[0][1][1], dests[0][1][2], dests[0][1][3]);
#endif

   /* store result color */
   if (qss->colorOutSlot >= 0) {
      unsigned i;
      /* XXX need to handle multiple color outputs someday */
      allvmrt(qss->stage.softpipe->fs->info.output_semantic_name[qss->colorOutSlot]
             == TGSI_SEMANTIC_COLOR);
      for (i = 0; i < QUAD_SIZE; ++i) {
         quad->outputs.color[0][0][i] = dests[i][qss->colorOutSlot][0];
         quad->outputs.color[0][1][i] = dests[i][qss->colorOutSlot][1];
         quad->outputs.color[0][2][i] = dests[i][qss->colorOutSlot][2];
         quad->outputs.color[0][3][i] = dests[i][qss->colorOutSlot][3];
      }
   }
#if DLLVM
   for (int i = 0; i < QUAD_SIZE; ++i) {
      debug_printf("QLLVM%d(%d) [%f, %f, %f, %f]\n", i, qss->colorOutSlot,
             quad->outputs.color[0][0][i],
             quad->outputs.color[0][1][i],
             quad->outputs.color[0][2][i],
             quad->outputs.color[0][3][i]);
   }
#endif

   /* store result Z */
   if (qss->depthOutSlot >= 0) {
      /* output[slot] is new Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = dests[i][0][2];
      }
   }
   else {
      /* copy input Z (which was interpolated by the executor) to output Z */
      uint i;
      for (i = 0; i < 4; i++) {
         quad->outputs.depth[i] = inputs[i][0][2];
      }
   }
#if DLLVM
   debug_printf("D [%f, %f, %f, %f] mask = %d\n",
             quad->outputs.depth[0],
             quad->outputs.depth[1],
             quad->outputs.depth[2],
             quad->outputs.depth[3], quad->mask);
#endif

   /* shader may cull fragments */
   if( quad->mask ) {
      qs->next->run( qs->next, quad );
   }
}


unsigned 
run_llvm_fs( const struct sp_fragment_shader *base,
	     struct foo *machine )
{
}


void 
delete_llvm_fs( struct sp_fragment_shader *base )
{
   FREE(base);
}


struct sp_fragment_shader *
softpipe_create_fs_llvm(struct softpipe_context *softpipe,
		       const struct pipe_shader_state *templ)
{
   struct sp_llvm_fragment_shader *shader = NULL;

   /* LLVM fragment shaders currently disabled:
    */
   state = CALLOC_STRUCT(sp_llvm_shader_state);
   if (!state)
      return NULL;

   state->llvm_prog = 0;

   if (!gallivm_global_cpu_engine()) {
      gallivm_cpu_engine_create(state->llvm_prog);
   }
   else
      gallivm_cpu_jit_compile(gallivm_global_cpu_engine(), state->llvm_prog);
   
   if (shader) {
      shader->base.run = run_llvm_fs;
      shader->base.delete = delete_llvm_fs;
   }

   return shader;
}


#else

struct sp_fragment_shader *
softpipe_create_fs_llvm(struct softpipe_context *softpipe,
		       const struct pipe_shader_state *templ)
{
   return NULL;
}

#endif
