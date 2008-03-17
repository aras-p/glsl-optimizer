/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * @file
 * Mipmap generation utility
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_shader_tokens.h"

#include "util/u_draw_quad.h"
#include "util/u_gen_mipmap.h"

#include "tgsi/util/tgsi_build.h"
#include "tgsi/util/tgsi_dump.h"
#include "tgsi/util/tgsi_parse.h"


/**
 * Make simple fragment shader:
 *  TEX OUT[0], IN[0], SAMP[0], 2D;
 *  END;
 */
static void
make_fragment_shader(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   uint maxTokens = 100;
   struct tgsi_token *tokens;
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_FRAGMENT;
   uint ti = 0;
   struct pipe_shader_state shader;

   tokens = (struct tgsi_token *) malloc(maxTokens * sizeof(tokens[0]));

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare TEX[0] input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   /* XXX this could be linear... */
   decl.Declaration.Interpolate = 1;
   decl.Interpolation.Interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare color[0] output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_COLOR;
   decl.Semantic.SemanticIndex = 0;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare sampler */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_SAMPLER;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* TEX instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_TEX;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 0;
   inst.Instruction.NumSrcRegs = 2;
   inst.InstructionExtTexture.Texture = TGSI_TEXTURE_2D;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 0;
   inst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
   inst.FullSrcRegisters[1].SrcRegister.Index = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;
   ctx->fs = pipe->create_fs_state(pipe, &shader);
}


/**
 * Make simple fragment shader:
 *  MOV OUT[0], IN[0];
 *  MOV OUT[1], IN[1];
 *  END;
 *
 * XXX eliminate this when vertex passthrough-mode is more solid.
 */
static void
make_vertex_shader(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;
   uint maxTokens = 100;
   struct tgsi_token *tokens;
   struct tgsi_header *header;
   struct tgsi_processor *processor;
   struct tgsi_full_declaration decl;
   struct tgsi_full_instruction inst;
   const uint procType = TGSI_PROCESSOR_VERTEX;
   uint ti = 0;
   struct pipe_shader_state shader;

   tokens = (struct tgsi_token *) malloc(maxTokens * sizeof(tokens[0]));

   /* shader header
    */
   *(struct tgsi_version *) &tokens[0] = tgsi_build_version();

   header = (struct tgsi_header *) &tokens[1];
   *header = tgsi_build_header();

   processor = (struct tgsi_processor *) &tokens[2];
   *processor = tgsi_build_processor( procType, header );

   ti = 3;

   /* declare POS input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   /*
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_POSITION;
   decl.Semantic.SemanticIndex = 0;
   */
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);
   /* declare TEX[0] input */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_INPUT;
   /*
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   */
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 1;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare POS output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_POSITION;
   decl.Semantic.SemanticIndex = 0;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 0;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* declare TEX[0] output */
   decl = tgsi_default_full_declaration();
   decl.Declaration.File = TGSI_FILE_OUTPUT;
   decl.Declaration.Semantic = 1;
   decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
   decl.Semantic.SemanticIndex = 0;
   decl.u.DeclarationRange.First = 
   decl.u.DeclarationRange.Last = 1;
   ti += tgsi_build_full_declaration(&decl,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti);

   /* MOVE out[0], in[0];  # POS */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_MOV;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 0;
   inst.Instruction.NumSrcRegs = 1;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

   /* MOVE out[1], in[1];  # TEX */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_MOV;
   inst.Instruction.NumDstRegs = 1;
   inst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
   inst.FullDstRegisters[0].DstRegister.Index = 1;
   inst.Instruction.NumSrcRegs = 1;
   inst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
   inst.FullSrcRegisters[0].SrcRegister.Index = 1;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

   /* END instruction */
   inst = tgsi_default_full_instruction();
   inst.Instruction.Opcode = TGSI_OPCODE_END;
   inst.Instruction.NumDstRegs = 0;
   inst.Instruction.NumSrcRegs = 0;
   ti += tgsi_build_full_instruction(&inst,
                                     &tokens[ti],
                                     header,
                                     maxTokens - ti );

#if 0 /*debug*/
   tgsi_dump(tokens, 0);
#endif

   shader.tokens = tokens;
   ctx->vs = pipe->create_vs_state(pipe, &shader);
}


/**
 * Create a mipmap generation context.
 * The idea is to create one of these and re-use it each time we need to
 * generate a mipmap.
 */
struct gen_mipmap_state *
util_create_gen_mipmap(struct pipe_context *pipe)
{
   struct pipe_blend_state blend;
   struct pipe_depth_stencil_alpha_state depthstencil;
   struct pipe_rasterizer_state rasterizer;
   struct gen_mipmap_state *ctx;

   ctx = CALLOC_STRUCT(gen_mipmap_state);
   if (!ctx)
      return NULL;

   ctx->pipe = pipe;

   /* we don't use blending, but need to set valid values */
   memset(&blend, 0, sizeof(blend));
   blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
   blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
   blend.colormask = PIPE_MASK_RGBA;
   ctx->blend = pipe->create_blend_state(pipe, &blend);

   /* depth/stencil/alpha */
   memset(&depthstencil, 0, sizeof(depthstencil));
   ctx->depthstencil = pipe->create_depth_stencil_alpha_state(pipe, &depthstencil);

   /* rasterizer */
   memset(&rasterizer, 0, sizeof(rasterizer));
   rasterizer.front_winding = PIPE_WINDING_CW;
   rasterizer.cull_mode = PIPE_WINDING_NONE;
   rasterizer.bypass_clipping = 1;  /* bypasses viewport too */
   //rasterizer.bypass_vs = 1;
   ctx->rasterizer = pipe->create_rasterizer_state(pipe, &rasterizer);

#if 0
   /* viewport */
   ctx->viewport.scale[0] = 1.0;
   ctx->viewport.scale[1] = 1.0;
   ctx->viewport.scale[2] = 1.0;
   ctx->viewport.scale[3] = 1.0;
   ctx->viewport.translate[0] = 0.0;
   ctx->viewport.translate[1] = 0.0;
   ctx->viewport.translate[2] = 0.0;
   ctx->viewport.translate[3] = 0.0;
#endif

   make_vertex_shader(ctx);
   make_fragment_shader(ctx);

   return ctx;
}


/**
 * Destroy a mipmap generation context
 */
void
util_destroy_gen_mipmap(struct gen_mipmap_state *ctx)
{
   struct pipe_context *pipe = ctx->pipe;

   pipe->delete_blend_state(pipe, ctx->blend);
   pipe->delete_depth_stencil_alpha_state(pipe, ctx->depthstencil);
   pipe->delete_rasterizer_state(pipe, ctx->rasterizer);
   pipe->delete_vs_state(pipe, ctx->vs);
   pipe->delete_fs_state(pipe, ctx->fs);

   FREE(ctx);
}


#if 0
static void
simple_viewport(struct pipe_context *pipe, uint width, uint height)
{
   struct pipe_viewport_state vp;

   vp.scale[0] =  0.5 * width;
   vp.scale[1] = -0.5 * height;
   vp.scale[2] = 1.0;
   vp.scale[3] = 1.0;
   vp.translate[0] = 0.5 * width;
   vp.translate[1] = 0.5 * height;
   vp.translate[2] = 0.0;
   vp.translate[3] = 0.0;

   pipe->set_viewport_state(pipe, &vp);
}
#endif


/**
 * Generate mipmap images.  It's assumed all needed texture memory is
 * already allocated.
 *
 * \param pt  the texture to generate mipmap levels for
 * \param face  which cube face to generate mipmaps for (0 for non-cube maps)
 * \param baseLevel  the first mipmap level to use as a src
 * \param lastLevel  the last mipmap level to generate
 */
void
util_gen_mipmap(struct gen_mipmap_state *ctx,
                struct pipe_texture *pt,
                uint face, uint baseLevel, uint lastLevel)
{
   struct pipe_context *pipe = ctx->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_framebuffer_state fb;
   struct pipe_sampler_state sampler;
   void *sampler_cso;
   uint dstLevel;
   uint zslice = 0;

   /* check if we can render in the texture's format */
   if (!screen->is_format_supported(screen, pt->format, PIPE_SURFACE)) {
#if 0
      fallback_gen_mipmap(ctx, pt, face, baseLevel, lastLevel);
#endif
      return;
   }

   /* init framebuffer state */
   memset(&fb, 0, sizeof(fb));
   fb.num_cbufs = 1;

   /* sampler state */
   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NEAREST;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.normalized_coords = 1;

   /* bind our state */
   pipe->bind_blend_state(pipe, ctx->blend);
   pipe->bind_depth_stencil_alpha_state(pipe, ctx->depthstencil);
   pipe->bind_rasterizer_state(pipe, ctx->rasterizer);
   pipe->bind_vs_state(pipe, ctx->vs);
   pipe->bind_fs_state(pipe, ctx->fs);
#if 0
   pipe->set_viewport_state(pipe, &ctx->viewport);
#endif

   /*
    * XXX for small mipmap levels, it may be faster to use the software
    * fallback path...
    */
   for (dstLevel = baseLevel + 1; dstLevel <= lastLevel; dstLevel++) {
      const uint srcLevel = dstLevel - 1;

      /*
       * Setup framebuffer / dest surface
       */
      fb.cbufs[0] = screen->get_tex_surface(screen, pt, face, dstLevel, zslice);
      pipe->set_framebuffer_state(pipe, &fb);

      /*
       * Setup sampler state
       * Note: we should only have to set the min/max LOD clamps to ensure
       * we grab texels from the right mipmap level.  But some hardware
       * has trouble with min clamping so we also set the lod_bias to
       * try to work around that.
       */
      sampler.min_lod = sampler.max_lod = srcLevel;
      sampler.lod_bias = srcLevel;
      sampler_cso = pipe->create_sampler_state(pipe, &sampler);
      pipe->bind_sampler_states(pipe, 1, &sampler_cso);

#if 0
      simple_viewport(pipe, pt->width[dstLevel], pt->height[dstLevel]);
#endif

      pipe->set_sampler_textures(pipe, 1, &pt);

      /* quad coords in window coords (bypassing clipping, viewport mapping) */
      util_draw_texquad(pipe,
                        0.0F, 0.0F, /* x0, y0 */
                        (float) pt->width[dstLevel], /* x1 */
                        (float) pt->height[dstLevel], /* y1 */
                        0.0F);  /* z */


      pipe->flush(pipe, PIPE_FLUSH_WAIT);

      /*pipe->texture_update(pipe, pt);  not really needed */

      pipe->delete_sampler_state(pipe, sampler_cso);
   }

   /* Note: caller must restore pipe/gallium state at this time */
}
