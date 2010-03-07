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


static boolean ps20_input( struct svga_shader_emitter *emit,
                           struct tgsi_declaration_semantic semantic,
                           unsigned idx )
{
   struct src_register reg;
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   switch (semantic.Name) {
   case TGSI_SEMANTIC_POSITION:
      /* Special case:
       */
      reg = src_register( SVGA3DREG_MISCTYPE, 
                          SVGA3DMISCREG_POSITION );
      break;
   case TGSI_SEMANTIC_COLOR:
      reg = src_register( SVGA3DREG_INPUT, 
                          semantic.Index );
      break;
   case TGSI_SEMANTIC_FOG:
      assert(semantic.Index == 0);
      reg = src_register( SVGA3DREG_TEXTURE, 0 );
      break;
   case TGSI_SEMANTIC_GENERIC:
      reg = src_register( SVGA3DREG_TEXTURE,
                          semantic.Index + 1 );
      break;
   default:
      assert(0);
      return TRUE;
   }

   emit->input_map[idx] = reg;

   dcl.dst = dst( reg );

   dcl.usage = 0;
   dcl.index = 0;

   dcl.values[0] |= 1<<31;

   return  (emit_instruction(emit, opcode) &&
            svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


static boolean ps20_output( struct svga_shader_emitter *emit,
                            struct tgsi_declaration_semantic semantic,
                            unsigned idx )
{
   SVGA3dShaderDestToken reg;

   switch (semantic.Name) {
   case TGSI_SEMANTIC_COLOR:
      if (semantic.Index < PIPE_MAX_COLOR_BUFS) {
         unsigned cbuf = semantic.Index;

         emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                               emit->nr_hw_temp++ );
         emit->temp_col[cbuf] = emit->output_map[idx];
         emit->true_col[cbuf] = dst_register( SVGA3DREG_COLOROUT, 
                                              semantic.Index );
      }
      else {
         assert(0);
         reg = dst_register( SVGA3DREG_COLOROUT, 0 );
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


static boolean vs20_input( struct svga_shader_emitter *emit,
                           struct tgsi_declaration_semantic semantic,
                           unsigned idx )
{
   SVGA3DOpDclArgs dcl;
   SVGA3dShaderInstToken opcode;

   opcode = inst_token( SVGA3DOP_DCL );
   dcl.values[0] = 0;
   dcl.values[1] = 0;

   emit->input_map[idx] = src_register( SVGA3DREG_INPUT, idx );
   dcl.dst = dst_register( SVGA3DREG_INPUT, idx );

   assert(dcl.dst.reserved0);

   /* Mesa doesn't provide use with VS input semantics (they're
    * actually pretty meaningless), so we just generate some plausible
    * ones here.  This has to match what we declare in the vdecl code
    * in svga_pipe_vertex.c.
    */
   if (idx == 0) {
      dcl.usage = SVGA3D_DECLUSAGE_POSITION;
      dcl.index = 0;
   }
   else {
      dcl.usage = SVGA3D_DECLUSAGE_TEXCOORD;
      dcl.index = idx - 1;
   }

   dcl.values[0] |= 1<<31;

   return  (emit_instruction(emit, opcode) &&
            svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


static boolean vs20_output( struct svga_shader_emitter *emit,
                         struct tgsi_declaration_semantic semantic,
                         unsigned idx )
{
   /* Don't emit dcl instruction for vs20 inputs
    */

   /* Just build the register map table: 
    */
   switch (semantic.Name) {
   case TGSI_SEMANTIC_POSITION:
      assert(semantic.Index == 0);
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_pos = emit->output_map[idx];
      emit->true_pos = dst_register( SVGA3DREG_RASTOUT, 
                                     SVGA3DRASTOUT_POSITION);
      break;
   case TGSI_SEMANTIC_PSIZE:
      assert(semantic.Index == 0);
      emit->output_map[idx] = dst_register( SVGA3DREG_TEMP,
                                            emit->nr_hw_temp++ );
      emit->temp_psiz = emit->output_map[idx];
      emit->true_psiz = dst_register( SVGA3DREG_RASTOUT, 
                                      SVGA3DRASTOUT_PSIZE );
      break;
   case TGSI_SEMANTIC_FOG:
      assert(semantic.Index == 0);
      emit->output_map[idx] = dst_register( SVGA3DREG_TEXCRDOUT, 0 );
      break;
   case TGSI_SEMANTIC_COLOR:
      /* oD0 */
      emit->output_map[idx] = dst_register( SVGA3DREG_ATTROUT,
                                            semantic.Index );
      break;
   case TGSI_SEMANTIC_GENERIC:
      emit->output_map[idx] = dst_register( SVGA3DREG_TEXCRDOUT,
                                            semantic.Index + 1 );
      break;
   default:
      assert(0);
      emit->output_map[idx] = dst_register(  SVGA3DREG_TEMP, 0 );
      return FALSE;
   }

   return TRUE;
}

static boolean ps20_sampler( struct svga_shader_emitter *emit,
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

   return  (emit_instruction(emit, opcode) &&
            svga_shader_emit_dwords( emit, dcl.values, Elements(dcl.values)));
}


boolean svga_translate_decl_sm20( struct svga_shader_emitter *emit,
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
         ok = ps20_sampler( emit, decl->Semantic, idx );
         break;

      case TGSI_FILE_INPUT:
         if (emit->unit == PIPE_SHADER_VERTEX)
            ok = vs20_input( emit, decl->Semantic, idx );
         else
            ok = ps20_input( emit, decl->Semantic, idx );
         break;

      case TGSI_FILE_OUTPUT:
         if (emit->unit == PIPE_SHADER_VERTEX)
            ok = vs20_output( emit, decl->Semantic, idx );
         else
            ok = ps20_output( emit, decl->Semantic, idx );
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



