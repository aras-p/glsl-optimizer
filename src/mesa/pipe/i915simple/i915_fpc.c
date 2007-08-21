/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#if 0
#include <strings.h>

#include "glheader.h"
#include "macros.h"
#include "enums.h"
#endif

#include "i915_fpc.h"



void
i915_program_error(struct i915_fp_compile *p, const char *msg)
{
   fprintf(stderr, "i915_program_error: %s", msg);
   p->fp->error = 1;
}


static struct i915_fp_compile *
i915_init_compile(struct i915_context *i915, struct i915_fragment_program *fp)
{
   struct i915_fp_compile *p = CALLOC_STRUCT(i915_fp_compile);

   p->fp = fp;
#if 0
   p->env_param = NULL; /*i915->intel.ctx.FragmentProgram.Parameters;*/
#endif
   p->constants = i915->fs.constants;
   p->nr_tex_indirect = 1;      /* correct? */
   p->nr_tex_insn = 0;
   p->nr_alu_insn = 0;
   p->nr_decl_insn = 0;

   memset(p->constant_flags, 0, sizeof(p->constant_flags));

   p->csr = p->program;
   p->decl = p->declarations;
   p->decl_s = 0;
   p->decl_t = 0;
   p->temp_flag = 0xffff000;
   p->utemp_flag = ~0x7;

#if 0
   p->fp->translated = 0;
   p->fp->error = 0;
   p->fp->nr_constants = 0;
#endif
   p->fp->wpos_tex = -1;
   p->fp->nr_params = 0;

   *(p->decl++) = _3DSTATE_PIXEL_SHADER_PROGRAM;

   return p;
}

/* Copy compile results to the fragment program struct and destroy the
 * compilation context.
 */
static void
i915_fini_compile(struct i915_fp_compile *p)
{
   uint program_size = p->csr - p->program;
   uint decl_size = p->decl - p->declarations;

   if (p->nr_tex_indirect > I915_MAX_TEX_INDIRECT)
      i915_program_error(p, "Exceeded max nr indirect texture lookups");

   if (p->nr_tex_insn > I915_MAX_TEX_INSN)
      i915_program_error(p, "Exceeded max TEX instructions");

   if (p->nr_alu_insn > I915_MAX_ALU_INSN)
      i915_program_error(p, "Exceeded max ALU instructions");

   if (p->nr_decl_insn > I915_MAX_DECL_INSN)
      i915_program_error(p, "Exceeded max DECL instructions");

   if (p->fp->error) {
      p->fp->NumNativeInstructions = 0;
      p->fp->NumNativeAluInstructions = 0;
      p->fp->NumNativeTexInstructions = 0;
      p->fp->NumNativeTexIndirections = 0;
      return;
   }
   else {
      p->fp->NumNativeInstructions = (p->nr_alu_insn +
                                      p->nr_tex_insn +
                                      p->nr_decl_insn);
      p->fp->NumNativeAluInstructions = p->nr_alu_insn;
      p->fp->NumNativeTexInstructions = p->nr_tex_insn;
      p->fp->NumNativeTexIndirections = p->nr_tex_indirect;
   }

   p->declarations[0] |= program_size + decl_size - 2;

   /* Copy compilation results to fragment program struct: 
    */
   memcpy(p->fp->program, 
	  p->declarations, 
	  decl_size * sizeof(uint));

   memcpy(p->fp->program + decl_size, 
	  p->program, 
	  program_size * sizeof(uint));
      
   p->fp->program_size = program_size + decl_size;

   /* Release the compilation struct: 
    */
   free(p);
}


/**
 * Find an unused texture coordinate slot to use for fragment WPOS.
 * Update p->fp->wpos_tex with the result (-1 if no used texcoord slot is found).
 */
static void
find_wpos_space(struct i915_fp_compile *p)
{
   const uint inputs = p->shader->inputs_read;
   uint i;

   p->fp->wpos_tex = -1;

   if (inputs & FRAG_BIT_WPOS) {
      for (i = 0; i < I915_TEX_UNITS; i++) {
	 if ((inputs & (FRAG_BIT_TEX0 << i)) == 0) {
	    p->fp->wpos_tex = i;
	    return;
	 }
      }

      i915_program_error(p, "No free texcoord for wpos value");
   }
}



void i915_compile_fragment_program( struct i915_context *i915,
				    struct i915_fragment_program *fp )
{
   struct i915_fp_compile *p = i915_init_compile(i915, fp);
   struct tgsi_token *tokens = i915->fs.tokens;

   find_wpos_space(p);

   i915_translate_program(p, tokens);
   i915_fixup_depth_write(p);

   i915_fini_compile(p);
#if 0
   fp->translated = 1;
#endif
}
