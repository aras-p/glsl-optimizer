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
 * AA point stage:  AA points are converted to quads and rendered with a
 * special fragment shader.  Another approach would be to use a texture
 * map image of a point, but experiments indicate the quality isn't nearly
 * as good as this approach.
 *
 * Note: this looks a lot like draw_aaline.c but there's actually little
 * if any code that can be shared.
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
#include "draw_vs.h"


/*
 * Enabling NORMALIZE might give _slightly_ better results.
 * Basically, it controls whether we compute distance as d=sqrt(x*x+y*y) or
 * d=x*x+y*y.  Since we're working with a unit circle, the later seems
 * close enough and saves some costly instructions.
 */
#define NORMALIZE 0


/**
 * Subclass of pipe_shader_state to carry extra fragment shader info.
 */
struct aapoint_fragment_shader
{
   struct pipe_shader_state state;
   void *driver_fs;   /**< the regular shader */
   void *aapoint_fs;  /**< the aa point-augmented shader */
   int generic_attrib; /**< The generic input attrib/texcoord we'll use */
};


/**
 * Subclass of draw_stage
 */
struct aapoint_stage
{
   struct draw_stage stage;

   int psize_slot;
   float radius;

   /** this is the vertex attrib slot for the new texcoords */
   uint tex_slot;

   /*
    * Currently bound state
    */
   struct aapoint_fragment_shader *fs;

   /*
    * Driver interface/override functions
    */
   void * (*driver_create_fs_state)(struct pipe_context *,
                                    const struct pipe_shader_state *);
   void (*driver_bind_fs_state)(struct pipe_context *, void *);
   void (*driver_delete_fs_state)(struct pipe_context *, void *);

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
   int maxInput, maxGeneric;  /**< max input index found */
   int tmp0, colorTemp;  /**< temp registers */
   boolean firstInstruction;
};


/**
 * TGSI declaration transform callback.
 * Look for two free temp regs and available input reg for new texcoords.
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
   struct tgsi_full_instruction newInst;

   if (aactx->firstInstruction) {
      /* emit our new declarations before the first instruction */

      struct tgsi_full_declaration decl;
      const int texInput = aactx->maxInput + 1;
      int tmp0;
      uint i;

      /* find two free temp regs */
      for (i = 0; i < 32; i++) {
         if ((aactx->tempsUsed & (1 << i)) == 0) {
            /* found a free temp */
            if (aactx->tmp0 < 0)
               aactx->tmp0 = i;
            else if (aactx->colorTemp < 0)
               aactx->colorTemp = i;
            else
               break;
         }
      }

      assert(aactx->colorTemp != aactx->tmp0);

      tmp0 = aactx->tmp0;

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
      decl.u.DeclarationRange.Last = texInput;
      ctx->emit_declaration(ctx, &decl);

      /* declare new temp regs */
      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.u.DeclarationRange.First = 
      decl.u.DeclarationRange.Last = tmp0;
      ctx->emit_declaration(ctx, &decl);

      decl = tgsi_default_full_declaration();
      decl.Declaration.File = TGSI_FILE_TEMPORARY;
      decl.u.DeclarationRange.First = 
      decl.u.DeclarationRange.Last = aactx->colorTemp;
      ctx->emit_declaration(ctx, &decl);

      aactx->firstInstruction = FALSE;


      /*
       * Emit code to compute fragment coverage, kill if outside point radius
       *
       * Temp reg0 usage:
       *  t0.x = distance of fragment from center point
       *  t0.y = boolean, is t0.x > 1.0, also misc temp usage
       *  t0.z = temporary for computing 1/(1-k) value
       *  t0.w = final coverage value
       */

      /* MUL t0.xy, tex, tex;  # compute x^2, y^2 */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_XY;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[0].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[1].SrcRegister.Index = texInput;
      ctx->emit_instruction(ctx, &newInst);

      /* ADD t0.x, t0.x, t0.y;  # x^2 + y^2 */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_ADD;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_X;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[1].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
      ctx->emit_instruction(ctx, &newInst);

#if NORMALIZE  /* OPTIONAL normalization of length */
      /* RSQ t0.x, t0.x; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_RSQ;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      ctx->emit_instruction(ctx, &newInst);

      /* RCP t0.x, t0.x; */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_RCP;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_X;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      ctx->emit_instruction(ctx, &newInst);
#endif

      /* SGT t0.y, t0.xxxx, t0.wwww;  # bool b = d > 1 (NOTE t0.w == 1) */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SGT;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_Y;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[1].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
      ctx->emit_instruction(ctx, &newInst);

      /* KILP -t0.yyyy;   # if b, KILL */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_KILP;
      newInst.Instruction.NumDstRegs = 0;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.Negate = 1;
      ctx->emit_instruction(ctx, &newInst);


      /* compute coverage factor = (1-d)/(1-k) */

      /* SUB t0.z, tex.w, tex.z;  # m = 1 - k */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SUB;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_Z;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[0].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[1].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* RCP t0.z, t0.z;  # t0.z = 1 / m */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_RCP;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_Z;
      newInst.Instruction.NumSrcRegs = 1;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* SUB t0.y, 1, t0.x;  # d = 1 - d */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SUB;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_Y;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[0].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[1].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
      ctx->emit_instruction(ctx, &newInst);

      /* MUL t0.w, t0.y, t0.z;   # coverage = d * m */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_MUL;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_W;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[1].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* SLE t0.y, t0.x, tex.z;  # bool b = distance <= k */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_SLE;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_Y;
      newInst.Instruction.NumSrcRegs = 2;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_X;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[1].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_Z;
      ctx->emit_instruction(ctx, &newInst);

      /* CMP t0.w, -t0.y, tex.w, t0.w;
       *  # if -t0.y < 0 then
       *       t0.w = 1
       *    else
       *       t0.w = t0.w
       */
      newInst = tgsi_default_full_instruction();
      newInst.Instruction.Opcode = TGSI_OPCODE_CMP;
      newInst.Instruction.NumDstRegs = 1;
      newInst.FullDstRegisters[0].DstRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullDstRegisters[0].DstRegister.Index = tmp0;
      newInst.FullDstRegisters[0].DstRegister.WriteMask = TGSI_WRITEMASK_W;
      newInst.Instruction.NumSrcRegs = 3;
      newInst.FullSrcRegisters[0].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[0].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleX = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleY = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleZ = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.SwizzleW = TGSI_SWIZZLE_Y;
      newInst.FullSrcRegisters[0].SrcRegister.Negate = 1;
      newInst.FullSrcRegisters[1].SrcRegister.File = TGSI_FILE_INPUT;
      newInst.FullSrcRegisters[1].SrcRegister.Index = texInput;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleX = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[1].SrcRegister.SwizzleW = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[2].SrcRegister.File = TGSI_FILE_TEMPORARY;
      newInst.FullSrcRegisters[2].SrcRegister.Index = tmp0;
      newInst.FullSrcRegisters[2].SrcRegister.SwizzleX = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[2].SrcRegister.SwizzleY = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[2].SrcRegister.SwizzleZ = TGSI_SWIZZLE_W;
      newInst.FullSrcRegisters[2].SrcRegister.SwizzleW = TGSI_SWIZZLE_W;
      ctx->emit_instruction(ctx, &newInst);

   }

   if (inst->Instruction.Opcode == TGSI_OPCODE_END) {
      /* add alpha modulation code at tail of program */

      /* MOV result.color.xyz, colorTemp; */
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

      /* MUL result.color.w, colorTemp, tmp0.w; */
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
      newInst.FullSrcRegisters[1].SrcRegister.Index = aactx->tmp0;
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
   }

   ctx->emit_instruction(ctx, inst);
}


/**
 * Generate the frag shader we'll use for drawing AA lines.
 * This will be the user's shader plus some texture/modulate instructions.
 */
static void
generate_aapoint_fs(struct aapoint_stage *aapoint)
{
   const struct pipe_shader_state *orig_fs = &aapoint->fs->state;
   struct pipe_shader_state aapoint_fs;
   struct aa_transform_context transform;

#define MAX 1000

   aapoint_fs = *orig_fs; /* copy to init */
   aapoint_fs.tokens = MALLOC(sizeof(struct tgsi_token) * MAX);

   memset(&transform, 0, sizeof(transform));
   transform.colorOutput = -1;
   transform.maxInput = -1;
   transform.maxGeneric = -1;
   transform.colorTemp = -1;
   transform.tmp0 = -1;
   transform.firstInstruction = TRUE;
   transform.base.transform_instruction = aa_transform_inst;
   transform.base.transform_declaration = aa_transform_decl;

   tgsi_transform_shader(orig_fs->tokens,
                         (struct tgsi_token *) aapoint_fs.tokens,
                         MAX, &transform.base);

#if 0 /* DEBUG */
   printf("draw_aapoint, orig shader:\n");
   tgsi_dump(orig_fs->tokens, 0);
   printf("draw_aapoint, new shader:\n");
   tgsi_dump(aapoint_fs.tokens, 0);
#endif

   aapoint->fs->aapoint_fs
      = aapoint->driver_create_fs_state(aapoint->pipe, &aapoint_fs);

   aapoint->fs->generic_attrib = transform.maxGeneric + 1;
}


/**
 * When we're about to draw our first AA line in a batch, this function is
 * called to tell the driver to bind our modified fragment shader.
 */
static void
bind_aapoint_fragment_shader(struct aapoint_stage *aapoint)
{
   if (!aapoint->fs->aapoint_fs) {
      generate_aapoint_fs(aapoint);
   }
   aapoint->driver_bind_fs_state(aapoint->pipe, aapoint->fs->aapoint_fs);
}



static INLINE struct aapoint_stage *
aapoint_stage( struct draw_stage *stage )
{
   return (struct aapoint_stage *) stage;
}


static void
passthrough_line(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->line(stage->next, header);
}


static void
passthrough_tri(struct draw_stage *stage, struct prim_header *header)
{
   stage->next->tri(stage->next, header);
}


/**
 * Draw an AA point by drawing a quad.
 */
static void
aapoint_point(struct draw_stage *stage, struct prim_header *header)
{
   const struct aapoint_stage *aapoint = aapoint_stage(stage);
   struct prim_header tri;
   struct vertex_header *v[4];
   uint texPos = aapoint->tex_slot;
   float radius, *pos, *tex;
   uint i;
   float k;

   if (aapoint->psize_slot >= 0) {
      radius = 0.5f * header->v[0]->data[aapoint->psize_slot][0];
   }
   else {
      radius = aapoint->radius;
   }

   /*
    * Note: the texcoords (generic attrib, really) we use are special:
    * The S and T components simply vary from -1 to +1.
    * The R component is k, below.
    * The Q component is 1.0 and will used as a handy constant in the
    * fragment shader.
    */

   /*
    * k is the threshold distance from the point's center at which
    * we begin alpha attenuation (the coverage value).
    * Operating within a unit circle, we'll compute the fragment's
    * distance 'd' from the center point using the texcoords.
    * IF d > 1.0 THEN
    *    KILL fragment
    * ELSE IF d > k THEN
    *    compute coverage in [0,1] proportional to d in [k, 1].
    * ELSE
    *    coverage = 1.0;  // full coverage
    * ENDIF
    *
    * Note: the ELSEIF and ELSE clauses are actually implemented with CMP to
    * avoid using IF/ELSE/ENDIF TGSI opcodes.
    */

#if !NORMALIZE
   k = 1.0f / radius;
   k = 1.0f - 2.0f * k + k * k;
#else
   k = 1.0f - 1.0f / radius;
#endif

   /* allocate/dup new verts */
   for (i = 0; i < 4; i++) {
      v[i] = dup_vert(stage, header->v[0], i);
   }

   /* new verts */
   pos = v[0]->data[0];
   pos[0] -= radius;
   pos[1] -= radius;

   pos = v[1]->data[0];
   pos[0] += radius;
   pos[1] -= radius;

   pos = v[2]->data[0];
   pos[0] += radius;
   pos[1] += radius;

   pos = v[3]->data[0];
   pos[0] -= radius;
   pos[1] += radius;

   /* new texcoords */
   tex = v[0]->data[texPos];
   ASSIGN_4V(tex, -1, -1, k, 1);

   tex = v[1]->data[texPos];
   ASSIGN_4V(tex,  1, -1, k, 1);

   tex = v[2]->data[texPos];
   ASSIGN_4V(tex,  1,  1, k, 1);

   tex = v[3]->data[texPos];
   ASSIGN_4V(tex, -1,  1, k, 1);

   /* emit 2 tris for the quad strip */
   tri.v[0] = v[0];
   tri.v[1] = v[1];
   tri.v[2] = v[2];
   stage->next->tri( stage->next, &tri );

   tri.v[0] = v[0];
   tri.v[1] = v[2];
   tri.v[2] = v[3];
   stage->next->tri( stage->next, &tri );
}


static void
aapoint_first_point(struct draw_stage *stage, struct prim_header *header)
{
   auto struct aapoint_stage *aapoint = aapoint_stage(stage);
   struct draw_context *draw = stage->draw;

   assert(draw->rasterizer->point_smooth);

   if (draw->rasterizer->point_size <= 2.0)
      aapoint->radius = 1.0;
   else
      aapoint->radius = 0.5f * draw->rasterizer->point_size;

   /*
    * Bind (generate) our fragprog.
    */
   bind_aapoint_fragment_shader(aapoint);

   /* update vertex attrib info */
   aapoint->tex_slot = draw->num_vs_outputs;
   assert(aapoint->tex_slot > 0); /* output[0] is vertex pos */

   draw->extra_vp_outputs.semantic_name = TGSI_SEMANTIC_GENERIC;
   draw->extra_vp_outputs.semantic_index = aapoint->fs->generic_attrib;
   draw->extra_vp_outputs.slot = aapoint->tex_slot;

   /* find psize slot in post-transform vertex */
   aapoint->psize_slot = -1;
   if (draw->rasterizer->point_size_per_vertex) {
      /* find PSIZ vertex output */
      const struct draw_vertex_shader *vs = draw->vertex_shader;
      uint i;
      for (i = 0; i < vs->info.num_outputs; i++) {
         if (vs->info.output_semantic_name[i] == TGSI_SEMANTIC_PSIZE) {
            aapoint->psize_slot = i;
            break;
         }
      }
   }

   /* now really draw first line */
   stage->point = aapoint_point;
   stage->point(stage, header);
}


static void
aapoint_flush(struct draw_stage *stage, unsigned flags)
{
   struct draw_context *draw = stage->draw;
   struct aapoint_stage *aapoint = aapoint_stage(stage);
   struct pipe_context *pipe = aapoint->pipe;

   stage->point = aapoint_first_point;
   stage->next->flush( stage->next, flags );

   /* restore original frag shader */
   aapoint->driver_bind_fs_state(pipe, aapoint->fs->driver_fs);

   draw->extra_vp_outputs.slot = 0;
}


static void
aapoint_reset_stipple_counter(struct draw_stage *stage)
{
   stage->next->reset_stipple_counter( stage->next );
}


static void
aapoint_destroy(struct draw_stage *stage)
{
   draw_free_temp_verts( stage );
   FREE( stage );
}


static struct aapoint_stage *
draw_aapoint_stage(struct draw_context *draw)
{
   struct aapoint_stage *aapoint = CALLOC_STRUCT(aapoint_stage);

   draw_alloc_temp_verts( &aapoint->stage, 4 );

   aapoint->stage.draw = draw;
   aapoint->stage.next = NULL;
   aapoint->stage.point = aapoint_first_point;
   aapoint->stage.line = passthrough_line;
   aapoint->stage.tri = passthrough_tri;
   aapoint->stage.flush = aapoint_flush;
   aapoint->stage.reset_stipple_counter = aapoint_reset_stipple_counter;
   aapoint->stage.destroy = aapoint_destroy;

   return aapoint;
}


static struct aapoint_stage *
aapoint_stage_from_pipe(struct pipe_context *pipe)
{
   struct draw_context *draw = (struct draw_context *) pipe->draw;
   return aapoint_stage(draw->pipeline.aapoint);
}


/**
 * This function overrides the driver's create_fs_state() function and
 * will typically be called by the state tracker.
 */
static void *
aapoint_create_fs_state(struct pipe_context *pipe,
                       const struct pipe_shader_state *fs)
{
   struct aapoint_stage *aapoint = aapoint_stage_from_pipe(pipe);
   struct aapoint_fragment_shader *aafs = CALLOC_STRUCT(aapoint_fragment_shader);

   if (aafs) {
      aafs->state = *fs;

      /* pass-through */
      aafs->driver_fs = aapoint->driver_create_fs_state(aapoint->pipe, fs);
   }

   return aafs;
}


static void
aapoint_bind_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aapoint_stage *aapoint = aapoint_stage_from_pipe(pipe);
   struct aapoint_fragment_shader *aafs = (struct aapoint_fragment_shader *) fs;
   /* save current */
   aapoint->fs = aafs;
   /* pass-through */
   aapoint->driver_bind_fs_state(aapoint->pipe,
                                 (aafs ? aafs->driver_fs : NULL));
}


static void
aapoint_delete_fs_state(struct pipe_context *pipe, void *fs)
{
   struct aapoint_stage *aapoint = aapoint_stage_from_pipe(pipe);
   struct aapoint_fragment_shader *aafs = (struct aapoint_fragment_shader *) fs;
   /* pass-through */
   aapoint->driver_delete_fs_state(aapoint->pipe, aafs->driver_fs);
   FREE(aafs);
}


/**
 * Called by drivers that want to install this AA point prim stage
 * into the draw module's pipeline.  This will not be used if the
 * hardware has native support for AA points.
 */
void
draw_install_aapoint_stage(struct draw_context *draw,
                           struct pipe_context *pipe)
{
   struct aapoint_stage *aapoint;

   pipe->draw = (void *) draw;

   /*
    * Create / install AA point drawing / prim stage
    */
   aapoint = draw_aapoint_stage( draw );
   assert(aapoint);
   draw->pipeline.aapoint = &aapoint->stage;

   aapoint->pipe = pipe;

   /* save original driver functions */
   aapoint->driver_create_fs_state = pipe->create_fs_state;
   aapoint->driver_bind_fs_state = pipe->bind_fs_state;
   aapoint->driver_delete_fs_state = pipe->delete_fs_state;

   /* override the driver's functions */
   pipe->create_fs_state = aapoint_create_fs_state;
   pipe->bind_fs_state = aapoint_bind_fs_state;
   pipe->delete_fs_state = aapoint_delete_fs_state;
}
