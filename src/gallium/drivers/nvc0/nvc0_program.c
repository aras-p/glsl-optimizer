/*
 * Copyright 2010 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pipe/p_shader_tokens.h"
#include "pipe/p_defines.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_dump.h"

#include "nvc0_context.h"
#include "nvc0_pc.h"

static unsigned
nvc0_tgsi_src_mask(const struct tgsi_full_instruction *inst, int c)
{
   unsigned mask = inst->Dst[0].Register.WriteMask;

   switch (inst->Instruction.Opcode) {
   case TGSI_OPCODE_COS:
   case TGSI_OPCODE_SIN:
      return (mask & 0x8) | ((mask & 0x7) ? 0x1 : 0x0);
   case TGSI_OPCODE_DP3:
      return 0x7;
   case TGSI_OPCODE_DP4:
   case TGSI_OPCODE_DPH:
   case TGSI_OPCODE_KIL: /* WriteMask ignored */
      return 0xf;
   case TGSI_OPCODE_DST:
      return mask & (c ? 0xa : 0x6);
   case TGSI_OPCODE_EX2:
   case TGSI_OPCODE_EXP:
   case TGSI_OPCODE_LG2:
   case TGSI_OPCODE_LOG:
   case TGSI_OPCODE_POW:
   case TGSI_OPCODE_RCP:
   case TGSI_OPCODE_RSQ:
   case TGSI_OPCODE_SCS:
      return 0x1;
   case TGSI_OPCODE_IF:
      return 0x1;
   case TGSI_OPCODE_LIT:
      return 0xb;
   case TGSI_OPCODE_TEX:
   case TGSI_OPCODE_TXB:
   case TGSI_OPCODE_TXL:
   case TGSI_OPCODE_TXP:
   {
      const struct tgsi_instruction_texture *tex;

      assert(inst->Instruction.Texture);
      tex = &inst->Texture;

      mask = 0x7;
      if (inst->Instruction.Opcode != TGSI_OPCODE_TEX &&
          inst->Instruction.Opcode != TGSI_OPCODE_TXD)
         mask |= 0x8; /* bias, lod or proj */

      switch (tex->Texture) {
      case TGSI_TEXTURE_1D:
         mask &= 0x9;
         break;
      case TGSI_TEXTURE_SHADOW1D:
         mask &= 0x5;
         break;
      case TGSI_TEXTURE_2D:
         mask &= 0xb;
         break;
      default:
         break;
      }
   }
  	   return mask;
   case TGSI_OPCODE_XPD:
   {
      unsigned x = 0;
      if (mask & 1) x |= 0x6;
      if (mask & 2) x |= 0x5;
      if (mask & 4) x |= 0x3;
      return x;
   }
   default:
      break;
   }

   return mask;
}

static void
nvc0_indirect_inputs(struct nvc0_translation_info *ti, int id)
{
   int i, c;

   for (i = 0; i < PIPE_MAX_SHADER_INPUTS; ++i)
      for (c = 0; c < 4; ++c)
         ti->input_access[i][c] = id;

   ti->indirect_inputs = TRUE;
}

static void
nvc0_indirect_outputs(struct nvc0_translation_info *ti, int id)
{
   int i, c;

   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i)
      for (c = 0; c < 4; ++c)
         ti->output_access[i][c] = id;

   ti->indirect_outputs = TRUE;
}

static INLINE unsigned
nvc0_system_value_location(unsigned sn, unsigned si, boolean *is_input)
{
   /* NOTE: locations 0xfxx indicate special regs */
   switch (sn) {
      /*
   case TGSI_SEMANTIC_VERTEXID:
      *is_input = TRUE;
      return 0x2fc;
      */
   case TGSI_SEMANTIC_PRIMID:
      *is_input = TRUE;
      return 0x60;
      /*
   case TGSI_SEMANTIC_LAYER_INDEX:
      return 0x64;
   case TGSI_SEMANTIC_VIEWPORT_INDEX:
      return 0x68;
      */
   case TGSI_SEMANTIC_INSTANCEID:
      *is_input = TRUE;
      return 0x2f8;
   case TGSI_SEMANTIC_FACE:
      *is_input = TRUE;
      return 0x3fc;
      /*
   case TGSI_SEMANTIC_INVOCATIONID:
      return 0xf11;
      */
   default:
      assert(0);
      return 0x000;
   }
}

static INLINE unsigned
nvc0_varying_location(unsigned sn, unsigned si)
{
   switch (sn) {
   case TGSI_SEMANTIC_POSITION:
      return 0x70;
   case TGSI_SEMANTIC_COLOR:
      return 0x280 + (si * 16); /* are these hard-wired ? */
   case TGSI_SEMANTIC_BCOLOR:
      return 0x2a0 + (si * 16);
   case TGSI_SEMANTIC_FOG:
      return 0x270;
   case TGSI_SEMANTIC_PSIZE:
      return 0x6c;
      /*
   case TGSI_SEMANTIC_PNTC:
      return 0x2e0;
      */
   case TGSI_SEMANTIC_GENERIC:
      /* We'd really like to distinguish between TEXCOORD and GENERIC here,
       * since only 0x300 to 0x37c can be replaced by sprite coordinates.
       * Also, gl_PointCoord should be a system value and must be assigned to
       * address 0x2e0. For now, let's cheat:
       */
      assert(si < 31);
      if (si <= 7)
         return 0x300 + si * 16;
      if (si == 9)
         return 0x2e0;
      return 0x80 + ((si - 8) * 16);
   case TGSI_SEMANTIC_NORMAL:
      return 0x360;
   case TGSI_SEMANTIC_PRIMID:
      return 0x40;
   case TGSI_SEMANTIC_FACE:
      return 0x3fc;
   case TGSI_SEMANTIC_EDGEFLAG: /* doesn't exist, set value like for an sreg */
      return 0xf00;
      /*
   case TGSI_SEMANTIC_CLIP_DISTANCE:
      return 0x2c0 + (si * 4);
      */
   default:
      assert(0);
      return 0x000;
   }
}

static INLINE unsigned
nvc0_interp_mode(const struct tgsi_full_declaration *decl)
{
   unsigned mode;

   if (decl->Declaration.Interpolate == TGSI_INTERPOLATE_CONSTANT)
      mode = NVC0_INTERP_FLAT;
   else
   if (decl->Declaration.Interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
      mode = NVC0_INTERP_PERSPECTIVE;
   else
   if (decl->Declaration.Semantic && decl->Semantic.Name == TGSI_SEMANTIC_COLOR)
      mode = NVC0_INTERP_PERSPECTIVE;
   else
      mode = NVC0_INTERP_LINEAR;

   if (decl->Declaration.Centroid)
      mode |= NVC0_INTERP_CENTROID;

   return mode;
}

static void
prog_immediate(struct nvc0_translation_info *ti,
               const struct tgsi_full_immediate *imm)
{
   int c;
   unsigned n = ti->immd32_nr++;

   assert(ti->immd32_nr <= ti->scan.immediate_count);

   for (c = 0; c < 4; ++c)
      ti->immd32[n * 4 + c] = imm->u[c].Uint;

   ti->immd32_ty[n] = imm->Immediate.DataType;
}

static boolean
prog_decl(struct nvc0_translation_info *ti,
          const struct tgsi_full_declaration *decl)
{
   unsigned i, c;
   unsigned sn = TGSI_SEMANTIC_GENERIC;
   unsigned si = 0;
   const unsigned first = decl->Range.First;
   const unsigned last = decl->Range.Last;

   if (decl->Declaration.Semantic) {
      sn = decl->Semantic.Name;
      si = decl->Semantic.Index;
   }
   
   switch (decl->Declaration.File) {
   case TGSI_FILE_INPUT:
      for (i = first; i <= last; ++i) {
         if (ti->prog->type == PIPE_SHADER_VERTEX) {
            for (c = 0; c < 4; ++c)
               ti->input_loc[i][c] = 0x80 + i * 16 + c * 4;
         } else {
            for (c = 0; c < 4; ++c)
               ti->input_loc[i][c] = nvc0_varying_location(sn, si) + c * 4;
            /* for sprite coordinates: */
            ti->prog->fp.in_pos[i] = ti->input_loc[i][0] / 4;
         }
         if (ti->prog->type == PIPE_SHADER_FRAGMENT)
            ti->interp_mode[i] = nvc0_interp_mode(decl);
      }
      break;
   case TGSI_FILE_OUTPUT:
      for (i = first; i <= last; ++i, ++si) {
         if (ti->prog->type == PIPE_SHADER_FRAGMENT) {
            si = i;
            if (i == ti->fp_depth_output) {
               ti->output_loc[i][2] = (ti->scan.num_outputs - 1) * 4;
            } else {
               if (i > ti->fp_depth_output)
                  si -= 1;
               for (c = 0; c < 4; ++c)
                  ti->output_loc[i][c] = si * 4 + c;
            }
         } else {
            if (sn == TGSI_SEMANTIC_EDGEFLAG)
               ti->edgeflag_out = i;
            for (c = 0; c < 4; ++c)
               ti->output_loc[i][c] = nvc0_varying_location(sn, si) + c * 4;
            /* for TFB_VARYING_LOCS: */
            ti->prog->vp.out_pos[i] = ti->output_loc[i][0] / 4;
         }
      }
      break;
   case TGSI_FILE_SYSTEM_VALUE:
      i = first;
      ti->sysval_loc[i] = nvc0_system_value_location(sn, si, &ti->sysval_in[i]);
      assert(first == last);
      break;
   case TGSI_FILE_TEMPORARY:
      ti->temp128_nr = MAX2(ti->temp128_nr, last + 1);
      break;
   case TGSI_FILE_NULL:
   case TGSI_FILE_CONSTANT:
   case TGSI_FILE_SAMPLER:
   case TGSI_FILE_ADDRESS:
   case TGSI_FILE_IMMEDIATE:
   case TGSI_FILE_PREDICATE:
      break;
   default:
      NOUVEAU_ERR("unhandled TGSI_FILE %d\n", decl->Declaration.File);
      return FALSE;
   }
   return TRUE;
}

static void
prog_inst(struct nvc0_translation_info *ti,
          const struct tgsi_full_instruction *inst, int id)
{
   const struct tgsi_dst_register *dst;
   const struct tgsi_src_register *src;
   int s, c, k;
   unsigned mask;

   if (inst->Instruction.Opcode == TGSI_OPCODE_BGNSUB) {
      ti->subr[ti->num_subrs].first_insn = id - 1;
      ti->subr[ti->num_subrs].id = ti->num_subrs + 1; /* id 0 is main program */
      ++ti->num_subrs;
   }

   if (inst->Dst[0].Register.File == TGSI_FILE_OUTPUT) {
      dst = &inst->Dst[0].Register;

      for (c = 0; c < 4; ++c) {
         if (dst->Indirect)
            nvc0_indirect_outputs(ti, id);
         if (!(dst->WriteMask & (1 << c)))
            continue;
         ti->output_access[dst->Index][c] = id;
      }

      if (inst->Instruction.Opcode == TGSI_OPCODE_MOV &&
          inst->Src[0].Register.File == TGSI_FILE_INPUT &&
          dst->Index == ti->edgeflag_out)
         ti->prog->vp.edgeflag = inst->Src[0].Register.Index;
   } else
   if (inst->Dst[0].Register.File == TGSI_FILE_TEMPORARY) {
      if (inst->Dst[0].Register.Indirect)
         ti->require_stores = TRUE;
   }

   for (s = 0; s < inst->Instruction.NumSrcRegs; ++s) {
      src = &inst->Src[s].Register;
      if (src->File == TGSI_FILE_TEMPORARY)
         if (inst->Src[s].Register.Indirect)
            ti->require_stores = TRUE;
      if (src->File != TGSI_FILE_INPUT)
         continue;
      mask = nvc0_tgsi_src_mask(inst, s);

      if (inst->Src[s].Register.Indirect)
         nvc0_indirect_inputs(ti, id);

      for (c = 0; c < 4; ++c) {
         if (!(mask & (1 << c)))
            continue;
         k = tgsi_util_get_full_src_register_swizzle(&inst->Src[s], c);
         if (k <= TGSI_SWIZZLE_W)
            ti->input_access[src->Index][k] = id;
      }
   }
}

/* Probably should introduce something like struct tgsi_function_declaration
 * instead of trying to guess inputs/outputs.
 */
static void
prog_subroutine_inst(struct nvc0_subroutine *subr,
                     const struct tgsi_full_instruction *inst)
{
   const struct tgsi_dst_register *dst;
   const struct tgsi_src_register *src;
   int s, c, k;
   unsigned mask;

   for (s = 0; s < inst->Instruction.NumSrcRegs; ++s) {
      src = &inst->Src[s].Register;
      if (src->File != TGSI_FILE_TEMPORARY)
         continue;
      mask = nvc0_tgsi_src_mask(inst, s);

      for (c = 0; c < 4; ++c) {
         k = tgsi_util_get_full_src_register_swizzle(&inst->Src[s], c);

         if ((mask & (1 << c)) && k < TGSI_SWIZZLE_W)
            if (!(subr->retv[src->Index / 32][k] & (1 << (src->Index % 32))))
               subr->argv[src->Index / 32][k] |= 1 << (src->Index % 32);
      }
   }

   if (inst->Dst[0].Register.File == TGSI_FILE_TEMPORARY) {
      dst = &inst->Dst[0].Register;

      for (c = 0; c < 4; ++c)
         if (dst->WriteMask & (1 << c))
            subr->retv[dst->Index / 32][c] |= 1 << (dst->Index % 32);
   }
}

static int
nvc0_vp_gp_gen_header(struct nvc0_program *vp, struct nvc0_translation_info *ti)
{
   int i, c;
   unsigned a;

   for (a = 0x80/4, i = 0; i <= ti->scan.file_max[TGSI_FILE_INPUT]; ++i) {
      for (c = 0; c < 4; ++c, ++a)
         if (ti->input_access[i][c])
            vp->hdr[5 + a / 32] |= 1 << (a % 32); /* VP_ATTR_EN */
   }

   for (i = 0; i <= ti->scan.file_max[TGSI_FILE_OUTPUT]; ++i) {
      a = (ti->output_loc[i][0] - 0x40) / 4;
      if (ti->output_loc[i][0] >= 0xf00)
         continue;
      for (c = 0; c < 4; ++c, ++a) {
         if (!ti->output_access[i][c])
            continue;
         vp->hdr[13 + a / 32] |= 1 << (a % 32); /* VP_EXPORT_EN */
      }
   }

   for (i = 0; i < TGSI_SEMANTIC_COUNT; ++i) {
      a = ti->sysval_loc[i] / 4;
      if (a > 0 && a < (0xf00 / 4))
         vp->hdr[(ti->sysval_in[i] ? 5 : 13) + a / 32] |= 1 << (a % 32);
   }

   return 0;
}

static int
nvc0_vp_gen_header(struct nvc0_program *vp, struct nvc0_translation_info *ti)
{
   vp->hdr[0] = 0x20461;
   vp->hdr[4] = 0xff000;

   vp->hdr[18] = (1 << vp->vp.num_ucps) - 1;

   return nvc0_vp_gp_gen_header(vp, ti);
}

static int
nvc0_gp_gen_header(struct nvc0_program *gp, struct nvc0_translation_info *ti)
{
   unsigned invocations = 1;
   unsigned max_output_verts, output_prim;
   unsigned i;

   gp->hdr[0] = 0x21061;

   for (i = 0; i < ti->scan.num_properties; ++i) {
      switch (ti->scan.properties[i].name) {
      case TGSI_PROPERTY_GS_OUTPUT_PRIM:
         output_prim = ti->scan.properties[i].data[0];
         break;
      case TGSI_PROPERTY_GS_MAX_OUTPUT_VERTICES:
         max_output_verts = ti->scan.properties[i].data[0];
         assert(max_output_verts < 512);
         break;
         /*
      case TGSI_PROPERTY_GS_INVOCATIONS:
         invocations = ti->scan.properties[i].data[0];
         assert(invocations <= 32);
         break;
         */
      default:
         break;
      }
   }

   gp->hdr[2] = MIN2(invocations, 32) << 24;

   switch (output_prim) {
   case PIPE_PRIM_POINTS:
      gp->hdr[3] = 0x01000000;
      gp->hdr[0] |= 0xf0000000;
      break;
   case PIPE_PRIM_LINE_STRIP:
      gp->hdr[3] = 0x06000000;
      gp->hdr[0] |= 0x10000000;
      break;
   case PIPE_PRIM_TRIANGLE_STRIP:
      gp->hdr[3] = 0x07000000;
      gp->hdr[0] |= 0x10000000;
      break;
   default:
      assert(0);
      break;
   }

   gp->hdr[4] = max_output_verts & 0x1ff;

   return nvc0_vp_gp_gen_header(gp, ti);
}

static int
nvc0_fp_gen_header(struct nvc0_program *fp, struct nvc0_translation_info *ti)
{
   int i, c;
   unsigned a, m;
   
   fp->hdr[0] = 0x21462;
   fp->hdr[5] = 0x80000000; /* getting a trap if FRAG_COORD_UMASK.w = 0 */

   if (ti->scan.uses_kill)
      fp->hdr[0] |= 0x8000;
   if (ti->scan.writes_z) {
      fp->hdr[19] |= 0x2;
      if (ti->scan.num_outputs > 2)
         fp->hdr[0] |= 0x4000; /* FP_MULTIPLE_COLOR_OUTPUTS */
   } else {
   if (ti->scan.num_outputs > 1)
      fp->hdr[0] |= 0x4000; /* FP_MULTIPLE_COLOR_OUTPUTS */
   }

   for (i = 0; i <= ti->scan.file_max[TGSI_FILE_INPUT]; ++i) {
      m = ti->interp_mode[i] & 3;
      for (c = 0; c < 4; ++c) {
         if (!ti->input_access[i][c])
            continue;
         a = ti->input_loc[i][c] / 2;
         if (ti->input_loc[i][c] >= 0x2c0)
            a -= 32;
         if (ti->input_loc[i][0] == 0x70)
            fp->hdr[5] |= 1 << (28 + c); /* FRAG_COORD_UMASK */
         else
         if (ti->input_loc[i][0] == 0x2e0)
            fp->hdr[14] |= 1 << (24 + c); /* POINT_COORD */
         else
            fp->hdr[4 + a / 32] |= m << (a % 32);
      }
   }

   for (i = 0; i <= ti->scan.file_max[TGSI_FILE_OUTPUT]; ++i) {
      if (i != ti->fp_depth_output)
         fp->hdr[18] |= 0xf << ti->output_loc[i][0];
   }

   for (i = 0; i < TGSI_SEMANTIC_COUNT; ++i) {
      a = ti->sysval_loc[i] / 2;
      if ((a > 0) && (a < 0xf00 / 2))
         fp->hdr[4 + a / 32] |= NVC0_INTERP_FLAT << (a % 32);
   }

   return 0;
}

static boolean
nvc0_prog_scan(struct nvc0_translation_info *ti)
{
   struct nvc0_program *prog = ti->prog;
   struct tgsi_parse_context parse;
   int ret;
   unsigned i;

#if NV50_DEBUG & NV50_DEBUG_SHADER
   tgsi_dump(prog->pipe.tokens, 0);
#endif

   tgsi_scan_shader(prog->pipe.tokens, &ti->scan);

   if (ti->prog->type == PIPE_SHADER_FRAGMENT) {
      ti->fp_depth_output = 255;
      for (i = 0; i < ti->scan.num_outputs; ++i)
         if (ti->scan.output_semantic_name[i] == TGSI_SEMANTIC_POSITION)
            ti->fp_depth_output = i;
   }

   ti->subr =
      CALLOC(ti->scan.opcode_count[TGSI_OPCODE_BGNSUB], sizeof(ti->subr[0]));

   ti->immd32 = (uint32_t *)MALLOC(ti->scan.immediate_count * 16);
   ti->immd32_ty = (ubyte *)MALLOC(ti->scan.immediate_count * sizeof(ubyte));

   ti->insns = MALLOC(ti->scan.num_instructions * sizeof(ti->insns[0]));

   tgsi_parse_init(&parse, prog->pipe.tokens);
   while (!tgsi_parse_end_of_tokens(&parse)) {
      tgsi_parse_token(&parse);

      switch (parse.FullToken.Token.Type) {
      case TGSI_TOKEN_TYPE_IMMEDIATE:
         prog_immediate(ti, &parse.FullToken.FullImmediate);
         break;
      case TGSI_TOKEN_TYPE_DECLARATION:
         prog_decl(ti, &parse.FullToken.FullDeclaration);
         break;
      case TGSI_TOKEN_TYPE_INSTRUCTION:
         ti->insns[ti->num_insns] = parse.FullToken.FullInstruction;
         prog_inst(ti, &parse.FullToken.FullInstruction, ++ti->num_insns);
         break;
      default:
         break;
      }
   }

   for (i = 0; i < ti->num_subrs; ++i) {
      unsigned pc = ti->subr[i].id;
      while (ti->insns[pc].Instruction.Opcode != TGSI_OPCODE_ENDSUB)
         prog_subroutine_inst(&ti->subr[i], &ti->insns[pc++]);
   }

   switch (prog->type) {
   case PIPE_SHADER_VERTEX:
      ti->input_file = NV_FILE_MEM_A;
      ti->output_file = NV_FILE_MEM_V;
      ret = nvc0_vp_gen_header(prog, ti);
      break;
      /*
   case PIPE_SHADER_TESSELLATION_CONTROL:
      ret = nvc0_tcp_gen_header(ti);
      break;
   case PIPE_SHADER_TESSELLATION_EVALUATION:
      ret = nvc0_tep_gen_header(ti);
      break;
   case PIPE_SHADER_GEOMETRY:
      ret = nvc0_gp_gen_header(ti);
      break;
      */
   case PIPE_SHADER_FRAGMENT:
      ti->input_file = NV_FILE_MEM_V;
      ti->output_file = NV_FILE_GPR;

      if (ti->scan.writes_z)
         prog->flags[0] = 0x11; /* ? */
      else
      if (!ti->scan.uses_kill && !ti->global_stores)
         prog->fp.early_z = 1;

      ret = nvc0_fp_gen_header(prog, ti);
      break;
   default:
      assert(!"unsupported program type");
      ret = -1;
      break;
   }

   if (ti->require_stores) {
      prog->hdr[0] |= 1 << 26;
      prog->hdr[1] |= ti->temp128_nr * 16; /* l[] size */
   }

   assert(!ret);
   return ret;
}

boolean
nvc0_program_translate(struct nvc0_program *prog)
{
   struct nvc0_translation_info *ti;
   int ret;

   ti = CALLOC_STRUCT(nvc0_translation_info);
   ti->prog = prog;

   ti->edgeflag_out = PIPE_MAX_SHADER_OUTPUTS;

   prog->vp.edgeflag = PIPE_MAX_ATTRIBS;

   if (prog->type == PIPE_SHADER_VERTEX && prog->vp.num_ucps)
      ti->append_ucp = TRUE;

   ret = nvc0_prog_scan(ti);
   if (ret) {
      NOUVEAU_ERR("unsupported shader program\n");
      goto out;
   }

   ret = nvc0_generate_code(ti);
   if (ret)
      NOUVEAU_ERR("shader translation failed\n");

#if NV50_DEBUG & NV50_DEBUG_SHADER
   unsigned i;
   for (i = 0; i < sizeof(prog->hdr) / sizeof(prog->hdr[0]); ++i)
      debug_printf("HDR[%02lx] = 0x%08x\n",
                   i * sizeof(prog->hdr[0]), prog->hdr[i]);
#endif

out:
   if (ti->immd32)
      FREE(ti->immd32);
   if (ti->immd32_ty)
      FREE(ti->immd32_ty);
   if (ti->insns)
      FREE(ti->insns);
   if (ti->subr)
      FREE(ti->subr);
   FREE(ti);
   return ret ? FALSE : TRUE;
}

void
nvc0_program_destroy(struct nvc0_context *nvc0, struct nvc0_program *prog)
{
   if (prog->res)
      nouveau_resource_free(&prog->res);

   if (prog->code)
      FREE(prog->code);
   if (prog->relocs)
      FREE(prog->relocs);

   memset(prog->hdr, 0, sizeof(prog->hdr));

   prog->translated = FALSE;
}
