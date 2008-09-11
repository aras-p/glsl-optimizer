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


#include "sp_context.h"
#include "sp_state.h"
#include "sp_fs.h"
#include "sp_headers.h"


#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_memory.h"
#include "pipe/p_inlines.h"
#include "tgsi/tgsi_exec.h"
#include "tgsi/tgsi_sse2.h"


#ifdef PIPE_ARCH_X86

#include "rtasm/rtasm_x86sse.h"

/* Surely this should be defined somewhere in a tgsi header:
 */
typedef void (PIPE_CDECL *codegen_function)(
   const struct tgsi_exec_vector *input,
   struct tgsi_exec_vector *output,
   const float (*constant)[4],
   struct tgsi_exec_vector *temporary,
   const struct tgsi_interp_coef *coef,
   float (*immediates)[4]
   //, const struct tgsi_exec_vector *quadPos
 );


struct sp_sse_fragment_shader {
   struct sp_fragment_shader base;
   struct x86_function             sse2_program;
   codegen_function func;
   float immediates[TGSI_EXEC_NUM_IMMEDIATES][4];
};



static void
fs_sse_prepare( const struct sp_fragment_shader *base,
		struct tgsi_exec_machine *machine,
		struct tgsi_sampler *samplers )
{
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
   struct sp_sse_fragment_shader *shader = (struct sp_sse_fragment_shader *) base;

   /* Compute X, Y, Z, W vals for this quad -- place in temp[0] for now */
   sp_setup_pos_vector(quad->posCoef, 
		       (float)quad->input.x0, (float)quad->input.y0, 
		       machine->Temps);

   /* init kill mask */
   machine->Temps[TGSI_EXEC_TEMP_KILMASK_I].xyzw[TGSI_EXEC_TEMP_KILMASK_C].u[0] = 0x0;

   shader->func( machine->Inputs,
		 machine->Outputs,
		 machine->Consts,
		 machine->Temps,
		 machine->InterpCoefs,
                 shader->immediates
		 //	 , &machine->QuadPos
      );

   return ~(machine->Temps[TGSI_EXEC_TEMP_KILMASK_I].xyzw[TGSI_EXEC_TEMP_KILMASK_C].u[0]);
}


static void 
fs_sse_delete( struct sp_fragment_shader *base )
{
   struct sp_sse_fragment_shader *shader = (struct sp_sse_fragment_shader *) base;

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

   shader->func = (codegen_function) x86_get_func( &shader->sse2_program );
   if (!shader->func) {
      x86_release_func( &shader->sse2_program );
      FREE(shader);
      return NULL;
   }

   shader->base.shader = *templ;
   shader->base.prepare = fs_sse_prepare;
   shader->base.run = fs_sse_run;
   shader->base.delete = fs_sse_delete;

   return &shader->base;
}


#else

/* Maybe put this varient in the header file.
 */
struct sp_fragment_shader *
softpipe_create_fs_sse(struct softpipe_context *softpipe,
		       const struct pipe_shader_state *templ)
{
   return NULL;
}

#endif
