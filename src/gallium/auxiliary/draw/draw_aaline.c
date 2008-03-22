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
 * AA line stage:  AA lines are converted to texture mapped triangles.
 *
 * Authors:  Brian Paul
 */


#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_shader_tokens.h"

#include "tgsi/util/tgsi_transform.h"
#include "tgsi/util/tgsi_dump.h"

#include "draw_context.h"
#include "draw_private.h"


/**
 * Max texture level for the alpha texture used for antialiasing
 */
#define MAX_TEXTURE_LEVEL  5   /* 32 x 32 */


/**
 * Subclass of pipe_shader_state to carry extra fragment shader info.
 */
struct aaline_fragment_shader
{
   struct pipe_shader_state state;
   void *driver_fs;
   void *aaline_fs;
   void *aapoint_fs; /* not yet */
   void *sprite_fs; /* not yet */
};


/**
 * Subclass of draw_stage
 */
struct aaline_stage
{
   struct draw_stage stage;

   float half_line_width;

   /** For AA lines, this is the vertex attrib slot for the new texcoords */
   uint tex_slot;

   void *sampler_cso;
   struct pipe_texture *texture;
   uint num_samplers;
   uint num_textures;


   /*
    * Currently bound state
    */
   struct aaline_fragment_shader *fs;
   struct {
      void *sampler[PIPE_MAX_SAMPLERS];
      struct pipe_texture *texture[PIPE_MAX_SAMPLERS];
   } state;

   /*
    * Driver interface/override functions
    */
   void * (*driver_create_fs_state)(struct pipe_context *,
                                    const struct pipe_shader_state *);
   void (*driver_bind_fs_state)(struct pipe_context *, void *);
   void (*driver_delete_fs_state)(struct pipe_context *, void *);

   void (*driver_bind_sampler_states)(struct pipe_context *, unsigned,
                                      void **);
   void (*driver_set_sampler_textures)(struct pipe_context *, unsigned,
                                       struct pipe_texture **);

   struct pipe_context *pipe;
};



/**
 * Subclass of tgsi_transform_context, used for transforming the
 * user's fragment shader to add the special AA instructions.
 */
struct aa_transform_context {
   struct tgsi_transform_context base;
   uint tempsUsed;  /**< bitmask */
   int colorOutput; /**< which output is the primary color */
   int maxSampler;  /**< max sampler index found */
   int maxInput, maxGeneric;  /**< max input index found */
   int colorTemp, texTemp;  /**< temp registers */
   boolean firstInstruction;
};


/**
 * TGSI declaration transform callback.
 * Look for a free sampler, a free input attrib, and two free temp regs.
 */
static void
aa_transform_decl(struct tgsi_transform_context *ctx,
                  struct tgsi_full_declaration *decl)
{
   struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;

   if (decl->Declaration.File == TGSI_FILE_OUTPUT &&
       decl->Semantic.SemanticName == TGSI_SEMANTIC_COLOR &&
       decl->Semantic.SemanticIndex == 0) {
      aactx->colorOutput = decl->u.DeclarationRange.First;
   }
   else if (decl->Declaration.File == TGSI_FILE_SAMPLER) {
      if ((int) decl->u.DeclarationRange.Last > aactx->maxSampler)
         aactx->maxSampler = decl->u.DeclarationRange.Last + 1;
   }
   else if (decl->Declaration.File == TGSI_FILE_INPUT) {
      if ((int) decl->u.DeclarationRange.Last > aactx->maxInput)
         aactx->maxInput = decl->u.DeclarationRange.Last;
      if (decl->Semantic.SemanticName == TGSI_SEMANTIC_GENERIC &&
           (int) decl->Semantic.SemanticIndex > aactx->maxGeneric) {
         aactx->maxGeneric = decl->Semantic.SemanticIndex;
      }
   }
   else if (decl->Declaration.File == TGSI_FILE_TEMPORARY) {
      uint i;
      for (i = decl->u.DeclarationRange.First;
           i <= decl->u.DeclarationRange.Last; i++) {
         aactx->tempsUsed |= (1 << i);
      }
   }

   ctx->emit_declaration(ctx, decl);
}


/**
 * TGSI instruction transform callback.
 * Replace writes to result.color w/ a temp reg.
 * Upon END instruction, insert texture sampling code for antialiasing.
 */
static void
aa_transform_inst(struct tgsi_transform_context *ctx,
                  struct tgsi_full_instruction *inst)
{
   struct aa_transform_context *aactx = (struct aa_transform_context *) ctx;

   if (aactx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      uint i;

      /* find two free temp regs */
      for (i = 0; i < 32; i++) {
         if ((aactx->tempsUsed & (1 << i)) == 0) {
            /* found a free temp */
            if (aactx->colorTemp < 0)
               aactx->colorTemp  = i;
            else if (aactx->texTemp < 0)
               aactx->texTemp  = i;
            else
               break;
         }
      }
      assert(aactx->colorTemp >= 0);
      assert(aactx->texTemp >= 0);

      /* declare new generic input/texcoord */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_INPUT;
      decl.Declaration.Semantic = 1;
      decl.Semantic.SemanticName = TGSI_SEMANTIC_GENERIC;
      decl.Semantic.SemanticIndex = aactx->maxGeneric + 1;
      decl.Declaration.Interpolate = 1;
      /* XXX this could be linear... */
      decl.Interpolation.Interpolate = TGSI_INTERPOLATE_PERSPECTIVE;
      decl.u.DeclarationRange.First = 
      decl.u.DeclarationRange.Last = aactx->maxInput + 1;
      ctx->emit_declaration(ctx, &decl);

      /* declare new sampler */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_SAMPLER;
      decl.u.DeclarationRange.First = 
      decl.u.DeclarationRange.Last = aactx->maxSampler + 1;
      ctx->emit_declaration(ctx, &decl);

      /* declare new temp regs */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.u.DeclarationRange.First = 
      decl.u.DeclarationRange.Last = aactx->texTemp;
      ctx->emit_declaration(ctx, &decl);

      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.u.DeclarationRange.First = 
      decl.u.DeclarationRange.Last = aactx->colorTemp;
      ctx->emit_declaration(ctx, &decl);

      aactx->firstInstruction = FALSE;
   }

   if (inst->Instruction.Opcode == TGSI_OPCODE_END &&
       aactx->colorOutput != -1) {
      struct tgsi_full_instruction newInst;

      /* TEX */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_TEX;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = aactx->texTemp;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.InstructionExtTexture.Texture = TGSI_TEXTURE_2D;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[0].SrcRegister.Index = aactx->maxInput + 1;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_SAMPLER;
      newInst.FullSrcRegisters[1].SrcRegister.Index = aactx->maxSampler + 1;

      ctx->emit_instruction(ctx, &newInst);

      /* MOV rgb */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MOV;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
      newInst.FullDstRegisters[0].DstRegister.Index = aactx->colorOutput;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_XYZ;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = aactx->colorTemp;
      ctx->emit_instruction(ctx, &newInst);

      /* MUL alpha */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_OUTPUT;
      newInst.FullDstRegisters[0].DstRegister.Index = aactx->colorOutput;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_W;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = aactx->colorTemp;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[1].SrcRegister.Index = aactx->texTemp;
      ctx->emit_instruction(ctx, &newInst);

      /* END */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_END;
      newInst.Instruction.NumDstRegs = 0;
      newInst.Instruction.NumSrcRegs = 0;
      ctx->emit_instruction(ctx, &newInst);
   }
   else {
      /* Not an END instruction.
       * Look for writes to result.color and replace with colorTemp reg.
       */
      uint i;

      for (i = 0; i < inst->Instruction.NumDstRegs; i++) {
         struct tgsi_full_dst_register *dst = &inst->FullDstRegisters[i];
         if (dst->DstRegister.File == TGSI_FILE_OUTPUT &&
             dst->DstRegister.Index == aactx->colorOutput) {
            dst->DstRegister.File = TGSI_FILE_TEMPORARY;
            dst->DstRegister.Index = aactx->colorTemp;
         }
      }

      ctx->emit_instruction(ctx, inst);
   }
}


/**
 * Generate the frag shader we'll use for drawing AA lines.
 * This will be the user's shader plus some texture/modulate instructions.
 */
static void
generate_aaline_fs(struct aaline_stage *aaline)
{
   const struct pipe_shader_state *orig_fs = &aaline->fs->state;
   struct draw_context *draw = aaline->stage.draw;
   struct pipe_shader_state aaline_fs;
   struct aa_transform_context transform;

#define MAX 1000

   aaline_fs = *orig_fs; /* copy to init */
   aaline_fs.tokens = MALLOC(sizeof(struct tgsi_token) * MAX);

   memset(&transform, 0, sizeof(transform));
   transform.colorOutput = -1;
   transform.maxSampler = -1;
   transform.maxInput = -1;
   transform.maxGeneric = -1;
   transform.colorTemp = -1;
   transform.texTemp = -1;
   transform.firstInstruction = TRUE;
   transform.base.transform_instruction = aa_transform_inst;
   transform.base.transform_declaration = aa_transform_decl;

   tgsi_transform_shader(orig_fs->tokens,
                         (struct tgsi_token *) aaline_fs.tokens,
                         MAX, &transform.base);

#if 0 /* DEBUG */
   tgsi_dump(orig_fs->tokens, 0);
   tgsi_dump(aaline_fs.tokens, 0);
#endif

   aaline->fs->aaline_fs
      = aaline->driver_create_fs_state(aaline->pipe, &aaline_fs);

   /* advertise the extra post-transform vertex attributes which will have
    * the texcoords.
    */
   draw->extra_vp_outputs.semantic_name = TGSI_SEMANTIC_GENERIC;
   draw->extra_vp_outputs.semantic_index = transform.maxGeneric + 1;
}


/**
 * Create the texture map we'll use for antialiasing the lines.
 */
static void
aaline_create_texture(struct aaline_stage *aaline)
{
   struct pipe_context *pipe = aaline->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture texTemp;
   uint level;

   memset(&texTemp, 0, sizeof(texTemp));
   texTemp.target = PIPE_TEXTURE_2D;
   texTemp.format = PIPE_FORMAT_U_A8; /* XXX verify supported by driver! */
   texTemp.last_level = MAX_TEXTURE_LEVEL;
   texTemp.width[0] = 1 << MAX_TEXTURE_LEVEL;
   texTemp.height[0] = 1 << MAX_TEXTURE_LEVEL;
   texTemp.depth[0] = 1;
   texTemp.cpp = 1;

   aaline->texture = screen->texture_create(screen, &texTemp);

   /* Fill in mipmap images.
    * Basically each level is solid opaque, except for the outermost
    * texels which are zero.  Special case the 1x1 and 2x2 levels.
    */
   for (level = 0; level <= MAX_TEXTURE_LEVEL; level++) {
      struct pipe_surface *surface;
      const uint size = aaline->texture->width[level];
      ubyte *data;
      uint i, j;

      assert(aaline->texture->width[level] == aaline->texture->height[level]);

      surface = screen->get_tex_surface(screen, aaline->texture, 0, level, 0);
      data = pipe_surface_map(surface);

      for (i = 0; i < size; i++) {
         for (j = 0; j < size; j++) {
            ubyte d;
            if (size == 1) {
               d = 255;
            }
            else if (size == 2) {
               d = 200; /* tuneable */
            }
            else if (i == 0 || j == 0 || i == size - 1 || j == size - 1) {
               d = 0;
            }
            else {
               d = 255;
            }
            data[i * surface->pitch + j] = d;
         }
      }

      /* unmap */
      pipe_surface_unmap(surface);
      pipe_surface_reference(&surface, NULL);
      pipe->texture_update(pipe, aaline->texture, 0, (1 << level));
   }
}


/**
 * Create the sampler CSO that'll be used for antialiasing.
 * By using a mipmapped texture, we don't have to generate a different
 * texture image for each line size.
 */
static void
aaline_create_sampler(struct aaline_stage *aaline)
{
   struct pipe_sampler_state sampler;
   struct pipe_context *pipe = aaline->pipe;

   memset(&sampler, 0, sizeof(sampler));
   sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
   sampler.min_mip_filter = PIPE_TEX_MIPFILTER_LINEAR;
   sampler.min_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.mag_img_filter = PIPE_TEX_FILTER_LINEAR;
   sampler.normalized_coords = 1;
   sampler.min_lod = 0.0f;
   sampler.max_lod = MAX_TEXTURE_LEVEL;

   aaline->sampler_cso = pipe->create_sampler_state(pipe, &sampler);
}


/**
 * When we're about to draw our first AA line in a batch, this function is
 * called to tell the driver to bind our modified fragment shader.
 */
static void
bind_aaline_fragment_shader(struct aaline_stage *aaline)
{
   if (!aaline->fs->aaline_fs) {
      generate_aaline_fs(aaline);
   }
   aaline->driver_bind_fs_state(aaline->pipe, aaline->fs->aaline_fs);
}



static INLINE struct aaline_stage *
aaline_stage( struct draw_stage *stage )
{
   return (struct aaline_stage *) stage;
}


static void
passthrough_point(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->point(stage->next, header);
}


static void
passthrough_tri(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->tri(stage->next, header);
}


/**
 * Draw a wide line by drawing a quad, using geometry which will
 * fullfill GL's antialiased line requirements.
 */
static void
aaline_line(struct draw_stage *stage, struct prim_header *header)
{
   const struct aaline_stage *aaline = aaline_stage(stage);
   const float half_width = aaline->half_line_width;
   struct prim_header tri;
   struct vertex_header *v[8];
   uint texPos = aaline->tex_slot;
   float *pos, *tex;
   float dx = header->v[1]->data[0][0] - header->v[0]->data[0][0];
   float dy = header->v[1]->data[0][1] - header->v[0]->data[0][1];
   double a = atan2(dy, dx);
   float c_a = (float) cos(a), s_a = (float) sin(a);
   uint i;

   /* XXX the ends of lines aren't quite perfect yet, but probably passable */
   dx = 0.5F * half_width;
   dy = half_width;

   /* allocate/dup new verts */
   for (i = 0; i < 8; i++) {
      v[i] = dup_vert(stage, header->v[i/4], i);
   }

   /*
    * Quad strip for line from v0 to v1 (*=endpoints):
    *
    *  1   3                     5   7
    *  +---+---------------------+---+
    *  |                             |
    *  | *v0                     v1* |
    *  |                             |
    *  +---+---------------------+---+
    *  0   2                     4   6
    */

   /* new verts */
   pos = v[0]->data[0];
   pos[0] += (-dx * c_a -  dy * s_a);
   pos[1] += (-dx * s_a +  dy * c_a);

   pos = v[1]->data[0];
   pos[0] += (-dx * c_a - -dy * s_a);
   pos[1] += (-dx * s_a + -dy * c_a);

   pos = v[2]->data[0];
   pos[0] += ( dx * c_a -  dy * s_a);
   pos[1] += ( dx * s_a +  dy * c_a);

   pos = v[3]->data[0];
   pos[0] += ( dx * c_a - -dy * s_a);
   pos[1] += ( dx * s_a + -dy * c_a);

   pos = v[4]->data[0];
   pos[0] += (-dx * c_a -  dy * s_a);
   pos[1] += (-dx * s_a +  dy * c_a);

   pos = v[5]->data[0];
   pos[0] += (-dx * c_a - -dy * s_a);
   pos[1] += (-dx * s_a + -dy * c_a);

   pos = v[6]->data[0];
   pos[0] += ( dx * c_a -  dy * s_a);
   pos[1] += ( dx * s_a +  dy * c_a);

   pos = v[7]->data[0];
   pos[0] += ( dx * c_a - -dy * s_a);
   pos[1] += ( dx * s_a + -dy * c_a);

   /* new texcoords */
   tex = v[0]->data[texPos];
   ASSIGN_4V(tex, 0, 0, 0, 1);

   tex = v[1]->data[texPos];
   ASSIGN_4V(tex, 0, 1, 0, 1);

   tex = v[2]->data[texPos];
   ASSIGN_4V(tex, .5, 0, 0, 1);

   tex = v[3]->data[texPos];
   ASSIGN_4V(tex, .5, 1, 0, 1);

   tex = v[4]->data[texPos];
   ASSIGN_4V(tex, .5, 0, 0, 1);

   tex = v[5]->data[texPos];
   ASSIGN_4V(tex, .5, 1, 0, 1);

   tex = v[6]->data[texPos];
   ASSIGN_4V(tex, 1, 0, 0, 1);

   tex = v[7]->data[texPos];
   ASSIGN_4V(tex, 1, 1, 0, 1);

   /* emit 6 tris for the quad strip */
   tri.v[0] = v[2];  tri.v[1] = v[1];  tri.v[2] = v[0];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[3];  tri.v[1] = v[1];  tri.v[2] = v[2];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[4];  tri.v[1] = v[3];  tri.v[2] = v[2];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[5];  tri.v[1] = v[3];  tri.v[2] = v[4];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[6];  tri.v[1] = v[5];  tri.v[2] = v[4];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[7];  tri.v[1] = v[5];  tri.v[2] = v[6];
   stage->next->tri( stage->next, &tri );
}


static void
aaline_first_line(struct draw_stage *stage, struct prim_header *header)
{
   auto struct aaline_stage *aaline = aaline_stage(stage);
   struct draw_context *draw = stage->draw;
   struct pipe_context *pipe = aaline->pipe;
   uint num = MAX2(aaline->num_textures, aaline->num_samplers);

   assert(draw->rasterizer->line_smooth);

   if (draw->rasterizer->line_width <= 3.0)
      aaline->half_line_width = 1.5f;
   else
      aaline->half_line_width = 0.5f * draw->rasterizer->line_width;

   aaline->tex_slot = draw->num_vs_outputs;
   assert(aaline->tex_slot > 0); /* output[0] is vertex pos */
   draw->extra_vp_outputs.slot = aaline->tex_slot;

   /*
    * Bind our fragprog, sampler and texture
    */
   bind_aaline_fragment_shader(aaline);

   aaline->state.sampler[num] = aaline->sampler_cso;
   aaline->state.texture[num] = aaline->texture;

   aaline->driver_bind_sampler_states(pipe, num + 1, aaline->state.sampler);
   aaline->driver_set_sampler_textures(pipe, num + 1, aaline->state.texture);

   /* now really draw first line */
   stage->line = aaline_line;
   stage->line(stage, header);
}


static void
aaline_flush(struct draw_stage *stage, unsigned flags)
{
   struct draw_context *draw = stage->draw;
   struct aaline_stage *aaline = aaline_stage(stage);
   struct pipe_context *pipe = aaline->pipe;

   stage->line = aaline_first_line;
   stage->next->flush( stage->next, flags );

   /* restore original frag shader */
   aaline->driver_bind_fs_state(pipe, aaline->fs->driver_fs);

   /* XXX restore original texture, sampler state */
   aaline->driver_bind_sampler_states(pipe, aaline->num_samplers,
                                      aaline->state.sampler);
   aaline->driver_set_sampler_textures(pipe, aaline->num_textures,
                                       aaline->state.texture);

   draw->extra_vp_outputs.slot = 0;
}


static void
aaline_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter( stage->next );
}


static void
aaline_destroy(struct draw_stage *stage)
{
   struct aaline_stage *aaline = aaline_stage(stage);

   aaline->pipe->delete_sampler_state(aaline->pipe, aaline->sampler_cso);

   pipe_texture_release(&aaline->texture);

   draw_free_temp_verts( stage );

   FREE( stage );
}


static struct aaline_stage *
draw_aaline_stage(struct draw_context *draw)
{
   struct aaline_stage *aaline = CALLOC_STRUCT(aaline_stage);

   draw_alloc_temp_verts( &aaline->stage, 8 );

   aaline->stage.draw = draw;
   aaline->stage.next = NULL;
   aaline->stage.point = passthrough_point;
   aaline->stage.line = aaline_first_line;
   aaline->stage.tri = passthrough_tri;
   aaline->stage.flush = aaline_flush;
   aaline->stage.reset_stipple_counter = aaline_reset_stipple_counter;
   aaline->stage.destroy = aaline_destroy;

   return aaline;
}


static struct aaline_stage *
aaline_stage_from_pipe(struct pipe_context *pipe)
{
   struct draw_context *draw = (struct draw_context *) pipe->draw;
   return aaline_stage(draw->pipeline.aaline);
}


/**
 * This function overrides the driver's create_fs_state() function and
 * will typically be called by the state tracker.
 */
static void *
aaline_create_fs_state(struct pipe_context *pipe,
                       const struct pipe_shader_state *fs)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   struct aaline_fragment_shader *aafs = CALLOC_STRUCT(aaline_fragment_shader);

   if (aafs) {
      aafs->state = *fs;

      /* pass-through */
      aafs->driver_fs = aaline->driver_create_fs_state(aaline->pipe, fs);
   }

   return aafs;
}


static void
aaline_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   struct aaline_fragment_shader *aafs = (struct aaline_fragment_shader *) fs;
   /* save current */
   aaline->fs = aafs;
   /* pass-through */
   aaline->driver_bind_fs_state(aaline->pipe,
                                (aafs ? aafs->driver_fs : NULL));
}


static void
aaline_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   struct aaline_fragment_shader *aafs = (struct aaline_fragment_shader *) fs;
   /* pass-through */
   aaline->driver_delete_fs_state(aaline->pipe, aafs->driver_fs);
   FREE(aafs);
}


static void
aaline_bind_sampler_states(struct pipe_context *pipe,
                           unsigned num, void **sampler)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   /* save current */
   memcpy(aaline->state.sampler, sampler, num * sizeof(void *));
   aaline->num_samplers = num;
   /* pass-through */
   aaline->driver_bind_sampler_states(aaline->pipe, num, sampler);
}


static void
aaline_set_sampler_textures(struct pipe_context *pipe,
                            unsigned num, struct pipe_texture **texture)
{
   struct aaline_stage *aaline = aaline_stage_from_pipe(pipe);
   /* save current */
   memcpy(aaline->state.texture, texture, num * sizeof(struct pipe_texture *));
   aaline->num_textures = num;
   /* pass-through */
   aaline->driver_set_sampler_textures(aaline->pipe, num, texture);
}


/**
 * Called by drivers that want to install this AA line prim stage
 * into the draw module's pipeline.  This will not be used if the
 * hardware has native support for AA lines.
 */
void
draw_install_aaline_stage(struct draw_context *draw, struct pipe_context *pipe)
{
   struct aaline_stage *aaline;

   pipe->draw = (void *) draw;

   /*
    * Create / install AA line drawing / prim stage
    */
   aaline = draw_aaline_stage( draw );
   assert(aaline);
   draw->pipeline.aaline = &aaline->stage;

   aaline->pipe = pipe;

   /* create special texture, sampler state */
   aaline_create_texture(aaline);
   aaline_create_sampler(aaline);

   /* save original driver functions */
   aaline->driver_create_fs_state = pipe->create_fs_state;
   aaline->driver_bind_fs_state = pipe->bind_fs_state;
   aaline->driver_delete_fs_state = pipe->delete_fs_state;

   aaline->driver_bind_sampler_states = pipe->bind_sampler_states;
   aaline->driver_set_sampler_textures = pipe->set_sampler_textures;

   /* override the driver's functions */
   pipe->create_fs_state = aaline_create_fs_state;
   pipe->bind_fs_state = aaline_bind_fs_state;
   pipe->delete_fs_state = aaline_delete_fs_state;

   pipe->bind_sampler_states = aaline_bind_sampler_states;
   pipe->set_sampler_textures = aaline_set_sampler_textures;
}
