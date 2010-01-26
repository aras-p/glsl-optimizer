/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/


#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "util/u_memory.h"

#include "svga_tgsi_emit.h"

static boolean translate_vs_ps_semantic( struct tgsi_declaration_semantic semantic,
                                         unsigned *usage,
                                         unsigned *idx )
{
   switch (semantic.Name) {
   case TGSI_SEMANTIC_POSITION:  
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_POSITION;
      break;
   case TGSI_SEMANTIC_COLOR:     

      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_COLOR;
      break;
   case TGSI_SEMANTIC_BCOLOR:
      *idx = semantic.Index + 2; /* sharing with COLOR */
      *usage = SVGA3D_DECLUSAGE_COLOR;
      break;
   case TGSI_SEMANTIC_FOG:       
      *idx = 0;
      assert(semantic.Index == 0);
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      break;
   case TGSI_SEMANTIC_PSIZE:     
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_PSIZE;
      break;
   case TGSI_SEMANTIC_GENERIC:   
      *idx = semantic.Index + 1; /* texcoord[0] is reserved for fog */
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      break;
   case TGSI_SEMANTIC_NORMAL:    
      *idx = semantic.Index;
      *usage = SVGA3D_DECLUSAGE_NORMAL;
      break;
   default:
      assert(0);
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      *idx = 0;
      return FALSE;
   }

   return TRUE;
}


static boolean emit_decl( struct svga_shader_emitter *emit,
                          SVGA3dShaderDestToken reg,
                          unsigned usage, 
                          unsigned index )
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   dcl.dst = reg;
   dcl.usage = usage;
   dcl.index = index;
   dcl.values[0] |= 1<<31;

   return  (emit_instruction(emit, opcode) &&
            svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}

static boolean emit_vface_decl( struct svga_shader_emitter *emit )
{
   if (!emit->emitted_vface) {
      SVGA3dShaderDestToken reg =
         dst_register( SVGA3DREG_MISCTYPE,
                       SVGA3DMISCREG_FACE );

      if (!emit_decl( emit, reg, 0, 0 ))
         return FALSE;

      emit->emitted_vface = TRUE;
   }
   return TRUE;
}

static boolean ps30_input( struct svga_shader_emitter *emit,
                           struct tgsi_declaration_semantic semantic,
                           unsigned idx )
{
   unsigned usage, index;
   SVGA3dShaderDestToken reg;

   if (semantic.Name == TGSI_SEMANTIC_POSITION) {
      emit->input_map[idx] = src_register( SVGA3DREG_MISCTYPE,
                                           SVGA3DMISCREG_POSITION );

      emit->input_map[idx].base.swizzle = TRANSLATE_SWIZZLE( TGSI_SWIZZLE_X,
                                                             TGSI_SWIZZLE_Y,
                                                             TGSI_SWIZZLE_Y,
                                                             TGSI_SWIZZLE_Y );

      reg = writemask( dst(emit->input_map[idx]),
                       TGSI_WRITEMASK_XY );

      return emit_decl( emit, reg, 0, 0 );
   }
   else if (emit->key.fkey.light_twoside &&
            (semantic.Name == TGSI_SEMANTIC_COLOR)) {

      if (!translate_vs_ps_semantic( semantic, &usage, &index ))
         return FALSE;

      emit->internal_color_idx[emit->internal_color_count] = idx;
      emit->input_map[idx] = src_register( SVGA3DREG_INPUT, emit->ps30_input_count );
      emit->ps30_input_count++;
      emit->internal_color_count++;

      reg = dst( emit->input_map[idx] );

      if (!emit_decl( emit, reg, usage, index ))
         return FALSE;

      semantic.Name = TGSI_SEMANTIC_BCOLOR;
      if (!translate_vs_ps_semantic( semantic, &usage, &index ))
         return FALSE;

      reg = dst_register( SVGA3DREG_INPUT, emit->ps30_input_count++ );

      if (!emit_decl( emit, reg, usage, index ))
         return FALSE;

      if (!emit_vface_decl( emit ))
         return FALSE;

      return TRUE;
   }
   else if (semantic.Name == TGSI_SEMANTIC_FACE) {
      if (!emit_vface_decl( emit ))
         return FALSE;
      emit->emit_frontface = TRUE;
      emit->internal_frontface_idx = idx;
      return TRUE;
   }
   else {

      if (!translate_vs_ps_semantic( semantic, &usage, &index ))
         return FALSE;

      emit->input_map[idx] = src_register( SVGA3DREG_INPUT, emit->ps30_input_count++ );
      reg = dst( emit->input_map[idx] );

      return emit_decl( emit, reg, usage, index );
   }

}


/* PS output registers are the same as 2.0
 */
static boolean ps30_output( struct svga_shader_emitter *emit,
                            struct tgsi_declaration_semantic semantic,
                            unsigned idx )
{
   SVGA3dShaderDestToken reg;

   switch (semantic.Name) {
   case TGSI_SEMANTIC_COLOR:
      if (emit->unit == PIPE_SHADER_FRAGMENT &&
          emit->key.fkey.white_fragments) {

         emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                               emit->nr_hw_temp++ );
         emit->temp_col[idx] = emit->output_map[idx];
         emit->true_col[idx] = dst_register( SVGA3DREG_COLOROUT, 
                                              semantic.Index );
      }
      else {
         emit->output_map[idx] = dst_register( SVGA3DREG_COLOROUT, 
                                               semantic.Index );
      }
      break;
   case TGSI_SEMANTIC_POSITION:
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_pos = emit->output_map[idx];
      emit->true_pos = dst_register( SVGA3DREG_DEPTHOUT, 
                                     semantic.Index );
      break;
   default:
      assert(0);
      reg = dst_register( SVGA3DREG_COLOROUT, 0 );
      break;
   }

   return TRUE;
}


/* We still make up the input semantics the same as in 2.0
 */
static boolean vs30_input( struct svga_shader_emitter *emit,
                           struct tgsi_declaration_semantic semantic,
                           unsigned idx )
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;
   unsigned usage, index;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   if (emit->key.vkey.zero_stride_vertex_elements & (1 << idx)) {
      unsigned i;
      unsigned offset = 0;
      unsigned start_idx = emit->info.file_max[TGSI_FILE_CONSTANT] + 1;
      /* adjust for prescale constants */
      start_idx += emit->key.vkey.need_prescale ? 2 : 0;
      /* compute the offset from the start of zero stride constants */
      for (i = 0; i < PIPE_MAX_ATTRIBS && i < idx; ++i) {
         if (emit->key.vkey.zero_stride_vertex_elements & (1<<i))
            ++offset;
      }
      emit->input_map[idx] = src_register( SVGA3DREG_CONST,
                                           start_idx + offset );
   } else {
      emit->input_map[idx] = src_register( SVGA3DREG_INPUT, idx );
      dcl.dst = dst_register( SVGA3DREG_INPUT, idx );

      assert(dcl.dst.reserved0);

      svga_generate_vdecl_semantics( idx, &usage, &index );

      dcl.usage = usage;
      dcl.index = index;
      dcl.values[0] |= 1<<31;

      return  (emit_instruction(emit, opcode) &&
               svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
   }
   return TRUE;
}

/* VS3.0 outputs have proper declarations and semantic info for
 * matching against PS inputs.
 */
static boolean vs30_output( struct svga_shader_emitter *emit,
                         struct tgsi_declaration_semantic semantic,
                         unsigned idx )
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;
   unsigned usage, index;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   if (!translate_vs_ps_semantic( semantic, &usage, &index ))
      return FALSE;

   dcl.dst = dst_register( SVGA3DREG_OUTPUT, idx );
   dcl.usage = usage;
   dcl.index = index;
   dcl.values[0] |= 1<<31;

   if (semantic.Name == TGSI_SEMANTIC_POSITION) {
      assert(idx == 0);
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_pos = emit->output_map[idx];
      emit->true_pos = dcl.dst;
   }
   else if (semantic.Name == TGSI_SEMANTIC_PSIZE) {
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_psiz = emit->output_map[idx];

      /* This has the effect of not declaring psiz (below) and not 
       * emitting the final MOV to true_psiz in the postamble.
       */
      if (!emit->key.vkey.allow_psiz)
         return TRUE;

      emit->true_psiz = dcl.dst;
   }
   else {
      emit->output_map[idx] = dcl.dst;
   }


   return  (emit_instruction(emit, opcode) &&
            svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}

static boolean ps30_sampler( struct svga_shader_emitter *emit,
                          struct tgsi_declaration_semantic semantic,
                          unsigned idx )
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   dcl.dst = dst_register( SVGA3DREG_SAMPLER, idx );
   dcl.type = svga_tgsi_sampler_type( emit, idx );
   dcl.values[0] |= 1<<31;

   return  (emit_instruction(emit, opcode) &&
            svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


boolean svga_translate_decl_sm30( struct svga_shader_emitter *emit,
                             const struct tgsi_full_declaration *decl )
{
   unsigned first = decl->Range.First;
   unsigned last = decl->Range.Last;
   unsigned semantic = 0;
   unsigned semantic_idx = 0;
   unsigned idx;

   if (decl->Declaration.Semantic) {
      semantic = decl->Semantic.Name;
      semantic_idx = decl->Semantic.Index;
   }

   for( idx = first; idx <= last; idx++ ) {
      boolean ok;

      switch (decl->Declaration.File) {
      case TGSI_FILE_SAMPLER:
         assert (emit->unit == PIPE_SHADER_FRAGMENT);
         ok = ps30_sampler( emit, decl->Semantic, idx );
         break;

      case TGSI_FILE_INPUT:
         if (emit->unit == PIPE_SHADER_VERTEX)
            ok = vs30_input( emit, decl->Semantic, idx );
         else
            ok = ps30_input( emit, decl->Semantic, idx );
         break;

      case TGSI_FILE_OUTPUT:
         if (emit->unit == PIPE_SHADER_VERTEX)
            ok = vs30_output( emit, decl->Semantic, idx );
         else
            ok = ps30_output( emit, decl->Semantic, idx );
         break;

      default:
         /* don't need to declare other vars */
         ok = TRUE;
      }

      if (!ok)
         return FALSE;
   }

   return TRUE;
}



