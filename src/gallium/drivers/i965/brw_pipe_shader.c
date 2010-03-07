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

#include "util/u_inlines.h"
#include "util/u_memory.h"
  
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_scan.h"

#include "brw_context.h"
#include "brw_wm.h"


/**
 * Determine if the given shader uses complex features such as flow
 * conditionals, loops, subroutines.
 */
static GLboolean has_flow_control(const struct tgsi_shader_info *info)
{
    return (info->opcode_count[TGSI_OPCODE_ARL] > 0 ||
	    info->opcode_count[TGSI_OPCODE_IF] > 0 ||
	    info->opcode_count[TGSI_OPCODE_ENDIF] > 0 || /* redundant - IF */
	    info->opcode_count[TGSI_OPCODE_CAL] > 0 ||
	    info->opcode_count[TGSI_OPCODE_BRK] > 0 ||   /* redundant - BGNLOOP */
	    info->opcode_count[TGSI_OPCODE_RET] > 0 ||   /* redundant - CAL */
	    info->opcode_count[TGSI_OPCODE_BGNLOOP] > 0);
}


static void scan_immediates(const struct tgsi_token *tokens,
                            const struct tgsi_shader_info *info,
                            struct brw_immediate_data *imm)
{
   struct tgsi_parse_context parse;
   boolean done = FALSE;

   imm->nr = 0;
   imm->data = MALLOC(info->immediate_count * 4 * sizeof(float));

   tgsi_parse_init( &parse, tokens );
   while (!tgsi_parse_end_of_tokens( &parse ) && !done) {
      tgsi_parse_token( &parse );

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_DECLARATION:
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE: {
	 static const float id[4] = {0,0,0,1};
	 const float *value = &parse.FullToken.FullImmediate.u[0].Float;
	 unsigned size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;
         unsigned i;

	 for (i = 0; i < size; i++)
	    imm->data[imm->nr][i] = value[i];

	 for (; i < 4; i++)
	    imm->data[imm->nr][i] = id[i];
         
         imm->nr++;
	 break;
      }

      case TGSI_TOKEN_TYPE_INSTRUCTION:
	 done = 1;
	 break;
      }
   }
}


static void brw_bind_fs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_fragment_shader *fs = (struct brw_fragment_shader *)prog;
   struct brw_context *brw = brw_context(pipe);
   
   if (brw->curr.fragment_shader == fs)
      return;

   if (brw->curr.fragment_shader == NULL ||
       fs == NULL ||
       memcmp(&brw->curr.fragment_shader->signature, &fs->signature,
              brw_fs_signature_size(&fs->signature)) != 0) {
      brw->state.dirty.mesa |= PIPE_NEW_FRAGMENT_SIGNATURE;
   }

   brw->curr.fragment_shader = fs;
   brw->state.dirty.mesa |= PIPE_NEW_FRAGMENT_SHADER;
}

static void brw_bind_vs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_context *brw = brw_context(pipe);

   brw->curr.vertex_shader = (struct brw_vertex_shader *)prog;
   brw->state.dirty.mesa |= PIPE_NEW_VERTEX_SHADER;
}



static void *brw_create_fs_state( struct pipe_context *pipe,
				  const struct pipe_shader_state *shader )
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_fragment_shader *fs;
   int i;

   fs = CALLOC_STRUCT(brw_fragment_shader);
   if (fs == NULL)
      return NULL;

   /* Duplicate tokens, scan shader
    */
   fs->id = brw->program_id++;
   fs->has_flow_control = has_flow_control(&fs->info);

   fs->tokens = tgsi_dup_tokens(shader->tokens);
   if (fs->tokens == NULL)
      goto fail;

   tgsi_scan_shader(fs->tokens, &fs->info);
   scan_immediates(fs->tokens, &fs->info, &fs->immediates);

   fs->signature.nr_inputs = fs->info.num_inputs;
   for (i = 0; i < fs->info.num_inputs; i++) {
      fs->signature.input[i].interp = fs->info.input_interpolate[i];
      fs->signature.input[i].semantic = fs->info.input_semantic_name[i];
      fs->signature.input[i].semantic_index = fs->info.input_semantic_index[i];
   }

   for (i = 0; i < fs->info.num_inputs; i++)
      if (fs->info.input_semantic_name[i] == TGSI_SEMANTIC_POSITION)
	 fs->uses_depth = 1;

   if (fs->info.uses_kill)
      fs->iz_lookup |= IZ_PS_KILL_ALPHATEST_BIT;

   if (fs->info.writes_z)
      fs->iz_lookup |= IZ_PS_COMPUTES_DEPTH_BIT;

   return (void *)fs;

fail:
   FREE(fs);
   return NULL;
}


static void *brw_create_vs_state( struct pipe_context *pipe,
				  const struct pipe_shader_state *shader )
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_vertex_shader *vs;
   unsigned i;

   vs = CALLOC_STRUCT(brw_vertex_shader);
   if (vs == NULL)
      return NULL;

   /* Duplicate tokens, scan shader
    */
   vs->tokens = tgsi_dup_tokens(shader->tokens);
   if (vs->tokens == NULL)
      goto fail;

   tgsi_scan_shader(vs->tokens, &vs->info);
   scan_immediates(vs->tokens, &vs->info, &vs->immediates);

   vs->id = brw->program_id++;
   vs->has_flow_control = has_flow_control(&vs->info);

   vs->output_hpos = BRW_OUTPUT_NOT_PRESENT;
   vs->output_color0 = BRW_OUTPUT_NOT_PRESENT;
   vs->output_color1 = BRW_OUTPUT_NOT_PRESENT;
   vs->output_bfc0 = BRW_OUTPUT_NOT_PRESENT;
   vs->output_bfc1 = BRW_OUTPUT_NOT_PRESENT;
   vs->output_edgeflag = BRW_OUTPUT_NOT_PRESENT;

   for (i = 0; i < vs->info.num_outputs; i++) {
      int index = vs->info.output_semantic_index[i];
      switch (vs->info.output_semantic_name[i]) {
      case TGSI_SEMANTIC_POSITION:
         vs->output_hpos = i;
         break;
      case TGSI_SEMANTIC_COLOR:
         if (index == 0)
            vs->output_color0 = i;
         else
            vs->output_color1 = i;
         break;
      case TGSI_SEMANTIC_BCOLOR:
         if (index == 0)
            vs->output_bfc0 = i;
         else
            vs->output_bfc1 = i;
         break;
      case TGSI_SEMANTIC_EDGEFLAG:
         vs->output_edgeflag = i;
         break;
      }
   }

   
   /* Done:
    */
   return (void *)vs;

fail:
   FREE(vs);
   return NULL;
}


static void brw_delete_fs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_fragment_shader *fs = (struct brw_fragment_shader *)prog;

   bo_reference(&fs->const_buffer, NULL);
   FREE( (void *)fs->tokens );
   FREE( fs );
}


static void brw_delete_vs_state( struct pipe_context *pipe, void *prog )
{
   struct brw_fragment_shader *vs = (struct brw_fragment_shader *)prog;

   /* Delete draw shader
    */
   FREE( (void *)vs->tokens );
   FREE( vs );
}


static void brw_set_constant_buffer(struct pipe_context *pipe,
                                     uint shader, uint index,
                                     struct pipe_buffer *buf)
{
   struct brw_context *brw = brw_context(pipe);

   assert(index == 0);

   if (shader == PIPE_SHADER_FRAGMENT) {
      pipe_buffer_reference( &brw->curr.fragment_constants,
                             buf );

      brw->state.dirty.mesa |= PIPE_NEW_FRAGMENT_CONSTANTS;
   }
   else {
      pipe_buffer_reference( &brw->curr.vertex_constants,
                             buf );

      brw->state.dirty.mesa |= PIPE_NEW_VERTEX_CONSTANTS;
   }
}


void brw_pipe_shader_init( struct brw_context *brw )
{
   brw->base.set_constant_buffer = brw_set_constant_buffer;

   brw->base.create_vs_state = brw_create_vs_state;
   brw->base.bind_vs_state = brw_bind_vs_state;
   brw->base.delete_vs_state = brw_delete_vs_state;

   brw->base.create_fs_state = brw_create_fs_state;
   brw->base.bind_fs_state = brw_bind_fs_state;
   brw->base.delete_fs_state = brw_delete_fs_state;
}

void brw_pipe_shader_cleanup( struct brw_context *brw )
{
   pipe_buffer_reference( &brw->curr.fragment_constants, NULL );
   pipe_buffer_reference( &brw->curr.vertex_constants, NULL );
}
