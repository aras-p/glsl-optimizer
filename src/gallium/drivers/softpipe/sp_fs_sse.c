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

/**
 * Execute fragment shader using runtime SSE code generation.
 */

#include "sp_context.h"
#include "sp_state.h"
#include "sp_fs.h"
#include "sp_quad.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_sse2.h"


#if defined(PIPE_ARCH_X86)

#include "rtasm/rtasm_x86sse.h"



/**
 * Subclass of sp_fragment_shader
 */
struct sp_sse_fragment_shader
{
   struct sp_fragment_shader base;
   struct x86_function sse2_program;
   tgsi_sse2_fs_function func;
   float immediates[TGSI_EXEC_NUM_IMMEDIATES][4];
};


/** cast wrapper */
static INLINE struct sp_sse_fragment_shader *
sp_sse_fragment_shader(const struct sp_fragment_shader *base)
{
   return (struct sp_sse_fragment_shader *) base;
}


static void
fs_sse_prepare( const struct sp_fragment_shader *base,
		struct tgsi_exec_machine *machine,
		struct tgsi_sampler **samplers )
{
   machine->Samplers = samplers;
}



/**
 * Compute quad X,Y,Z,W for the four fragments in a quad.
 *
 * This should really be part of the compiled shader.
 */
static void
setup_pos_vector(const struct tgsi_interp_coef *coef,
		    float x, float y,
		    struct tgsi_exec_vector *quadpos)
{
   uint chan;
   /* do X */
   quadpos->xyzw[0].f[0] = x;
   quadpos->xyzw[0].f[1] = x + 1;
   quadpos->xyzw[0].f[2] = x;
   quadpos->xyzw[0].f[3] = x + 1;

   /* do Y */
   quadpos->xyzw[1].f[0] = y;
   quadpos->xyzw[1].f[1] = y;
   quadpos->xyzw[1].f[2] = y + 1;
   quadpos->xyzw[1].f[3] = y + 1;

   /* do Z and W for all fragments in the quad */
   for (chan = 2; chan < 4; chan++) {
      const float dadx = coef->dadx[chan];
      const float dady = coef->dady[chan];
      const float a0 = coef->a0[chan] + dadx * x + dady * y;
      quadpos->xyzw[chan].f[0] = a0;
      quadpos->xyzw[chan].f[1] = a0 + dadx;
      quadpos->xyzw[chan].f[2] = a0 + dady;
      quadpos->xyzw[chan].f[3] = a0 + dadx + dady;
   }
}


/* TODO: codegenerate the whole run function, skip this wrapper.
 * TODO: break dependency on tgsi_exec_machine struct
 * TODO: push Position calculation into the generated shader
 * TODO: process >1 quad at a time
 */
static unsigned 
fs_sse_run( const struct sp_fragment_shader *base,
	    struct tgsi_exec_machine *machine,
	    struct quad_header *quad )
{
   struct sp_sse_fragment_shader *shader = sp_sse_fragment_shader(base);

   /* Compute X, Y, Z, W vals for this quad -- place in temp[0] for now */
   setup_pos_vector(quad->posCoef, 
                    (float)quad->input.x0, (float)quad->input.y0, 
                    machine->Temps);

   /* init kill mask */
   tgsi_set_kill_mask(machine, 0x0);
   tgsi_set_exec_mask(machine, 1, 1, 1, 1);

   shader->func( machine,
                 (const float (*)[4])machine->Consts[0],
                 (const float (*)[4])shader->immediates,
		 machine->InterpCoefs
		 /*, &machine->QuadPos*/
      );

   quad->inout.mask &= ~(machine->Temps[TGSI_EXEC_TEMP_KILMASK_I].xyzw[TGSI_EXEC_TEMP_KILMASK_C].u[0]);
   if (quad->inout.mask == 0)
      return FALSE;

   /* store outputs */
   {
      const ubyte *sem_name = base->info.output_semantic_name;
      const ubyte *sem_index = base->info.output_semantic_index;
      const uint n = base->info.num_outputs;
      uint i;
      for (i = 0; i < n; i++) {
         switch (sem_name[i]) {
         case TGSI_SEMANTIC_COLOR:
            {
               uint cbuf = sem_index[i];
               memcpy(quad->output.color[cbuf],
                      &machine->Outputs[i].xyzw[0].f[0],
                      sizeof(quad->output.color[0]) );
            }
            break;
         case TGSI_SEMANTIC_POSITION:
            {
               uint j;
               for (j = 0; j < 4; j++) {
                  quad->output.depth[j] = machine->Outputs[0].xyzw[2].f[j];
               }
            }
            break;
         }
      }
   }

   return TRUE;
}


static void 
fs_sse_delete( struct sp_fragment_shader *base )
{
   struct sp_sse_fragment_shader *shader = sp_sse_fragment_shader(base);

   x86_release_func( &shader->sse2_program );
   FREE(shader);
}


struct sp_fragment_shader *
softpipe_create_fs_sse(struct softpipe_context *softpipe,
		       const struct pipe_shader_state *templ)
{
   struct sp_sse_fragment_shader *shader;

   if (!softpipe->use_sse)
      return NULL;

   shader = CALLOC_STRUCT(sp_sse_fragment_shader);
   if (!shader)
      return NULL;

   x86_init_func( &shader->sse2_program );
   
   if (!tgsi_emit_sse2( templ->tokens, &shader->sse2_program,
                        shader->immediates, FALSE )) {
      FREE(shader);
      return NULL;
   }

   shader->func = (tgsi_sse2_fs_function) x86_get_func( &shader->sse2_program );
   if (!shader->func) {
      x86_release_func( &shader->sse2_program );
      FREE(shader);
      return NULL;
   }

   shader->base.shader.tokens = NULL; /* don't hold reference to templ->tokens */
   shader->base.prepare = fs_sse_prepare;
   shader->base.run = fs_sse_run;
   shader->base.delete = fs_sse_delete;

   return &shader->base;
}


#else

/* Maybe put this variant in the header file.
 */
struct sp_fragment_shader *
softpipe_create_fs_sse(struct softpipe_context *softpipe,
		       const struct pipe_shader_state *templ)
{
   return NULL;
}

#endif
