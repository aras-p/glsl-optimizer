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

#include "nv50_program.h"
#include "nv50_pc.h"
#include "nv50_context.h"

#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_util.h"
#include "tgsi/tgsi_dump.h"

#include "codegen/nv50_ir_driver.h"

static INLINE unsigned
bitcount4(const uint32_t val)
{
   static const unsigned cnt[16]
   = { 0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4 };
   return cnt[val & 0xf];
}

static unsigned
nv50_tgsi_src_mask(const struct tgsi_full_instruction *inst, int c)
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
nv50_indirect_inputs(struct nv50_translation_info *ti, int id)
{
   int i, c;

   for (i = 0; i < PIPE_MAX_SHADER_INPUTS; ++i)
      for (c = 0; c < 4; ++c)
         ti->input_access[i][c] = id;

   ti->indirect_inputs = TRUE;
}

static void
nv50_indirect_outputs(struct nv50_translation_info *ti, int id)
{
   int i, c;

   for (i = 0; i < PIPE_MAX_SHADER_OUTPUTS; ++i)
      for (c = 0; c < 4; ++c)
         ti->output_access[i][c] = id;

   ti->indirect_outputs = TRUE;
}

static void
prog_inst(struct nv50_translation_info *ti,
          const struct tgsi_full_instruction *inst, int id)
{
   const struct tgsi_dst_register *dst;
   const struct tgsi_src_register *src;
   int s, c, k;
   unsigned mask;

   if (inst->Instruction.Opcode == TGSI_OPCODE_BGNSUB) {
      ti->subr[ti->subr_nr].pos = id - 1;
      ti->subr[ti->subr_nr].id = ti->subr_nr + 1; /* id 0 is main program */
      ++ti->subr_nr;
   }

   if (inst->Dst[0].Register.File == TGSI_FILE_OUTPUT) {
      dst = &inst->Dst[0].Register;

      for (c = 0; c < 4; ++c) {
         if (dst->Indirect)
            nv50_indirect_outputs(ti, id);
         if (!(dst->WriteMask & (1 << c)))
            continue;
         ti->output_access[dst->Index][c] = id;
      }

      if (inst->Instruction.Opcode == TGSI_OPCODE_MOV &&
          inst->Src[0].Register.File == TGSI_FILE_INPUT &&
          dst->Index == ti->edgeflag_out)
         ti->p->vp.edgeflag = inst->Src[0].Register.Index;
   } else
   if (inst->Dst[0].Register.File == TGSI_FILE_TEMPORARY) {
      if (inst->Dst[0].Register.Indirect)
         ti->store_to_memory = TRUE;
   }

   for (s = 0; s < inst->Instruction.NumSrcRegs; ++s) {
      src = &inst->Src[s].Register;
      if (src->File == TGSI_FILE_TEMPORARY)
         if (inst->Src[s].Register.Indirect)
            ti->store_to_memory = TRUE;
      if (src->File != TGSI_FILE_INPUT)
         continue;
      mask = nv50_tgsi_src_mask(inst, s);

      if (inst->Src[s].Register.Indirect)
         nv50_indirect_inputs(ti, id);

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
prog_subroutine_inst(struct nv50_subroutine *subr,
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
      mask = nv50_tgsi_src_mask(inst, s);

      assert(!inst->Src[s].Register.Indirect);

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

static void
prog_immediate(struct nv50_translation_info *ti,
               const struct tgsi_full_immediate *imm)
{
   int c;
   unsigned n = ti->immd32_nr++;

   assert(ti->immd32_nr <= ti->scan.immediate_count);

   for (c = 0; c < 4; ++c)
      ti->immd32[n * 4 + c] = imm->u[c].Uint;

   ti->immd32_ty[n] = imm->Immediate.DataType;
}

static INLINE unsigned
translate_interpolate(const struct tgsi_full_declaration *decl)
{
   unsigned mode;

   if (decl->Declaration.Interpolate == TGSI_INTERPOLATE_CONSTANT)
      mode = NV50_INTERP_FLAT;
   else
   if (decl->Declaration.Interpolate == TGSI_INTERPOLATE_PERSPECTIVE)
      mode = 0;
   else
      mode = NV50_INTERP_LINEAR;

   if (decl->Declaration.Centroid)
      mode |= NV50_INTERP_CENTROID;

   return mode;
}

static void
prog_decl(struct nv50_translation_info *ti,
          const struct tgsi_full_declaration *decl)
{
   unsigned i, first, last, sn = 0, si = 0;

   first = decl->Range.First;
   last = decl->Range.Last;

   if (decl->Declaration.Semantic) {
      sn = decl->Semantic.Name;
      si = decl->Semantic.Index;
   }

   switch (decl->Declaration.File) {
   case TGSI_FILE_INPUT:
      for (i = first; i <= last; ++i)
         ti->interp_mode[i] = translate_interpolate(decl);

      if (!decl->Declaration.Semantic)
         break;

      for (i = first; i <= last; ++i) {
         ti->p->in[i].sn = sn;
         ti->p->in[i].si = si;
      }

      switch (sn) {
      case TGSI_SEMANTIC_FACE:
         break;
      case TGSI_SEMANTIC_COLOR:
         if (ti->p->type == PIPE_SHADER_FRAGMENT)
            ti->p->vp.bfc[si] = first;
         break;
      }
      break;
   case TGSI_FILE_OUTPUT:
      if (!decl->Declaration.Semantic)
         break;

      for (i = first; i <= last; ++i) {
         ti->p->out[i].sn = sn;
         ti->p->out[i].si = si;
      }

      switch (sn) {
      case TGSI_SEMANTIC_BCOLOR:
         ti->p->vp.bfc[si] = first;
         break;
      case TGSI_SEMANTIC_PSIZE:
         ti->p->vp.psiz = first;
         break;
      case TGSI_SEMANTIC_EDGEFLAG:
         ti->edgeflag_out = first;
         break;
      default:
         break;
      }
      break;
   case TGSI_FILE_SYSTEM_VALUE:
      /* For VP/GP inputs, they are put in s[] after the last normal input.
       * Let sysval_map reflect the order of the sysvals in s[] and fixup later.
       */
      switch (decl->Semantic.Name) {
      case TGSI_SEMANTIC_FACE:
         break;
      case TGSI_SEMANTIC_INSTANCEID:
         ti->p->vp.attrs[2] |= NV50_3D_VP_GP_BUILTIN_ATTR_EN_INSTANCE_ID;
         ti->sysval_map[first] = 2;
         break;
      case TGSI_SEMANTIC_PRIMID:
         break;
         /*
      case TGSI_SEMANTIC_PRIMIDIN:
         break;
      case TGSI_SEMANTIC_VERTEXID:
         break;
         */
      default:
         break;
      }
      break;
   case TGSI_FILE_CONSTANT:
      ti->p->parm_size = MAX2(ti->p->parm_size, (last + 1) * 16);
      break;
   case TGSI_FILE_ADDRESS:
   case TGSI_FILE_SAMPLER:
   case TGSI_FILE_TEMPORARY:
      break;
   default:
      assert(0);
      break;
   }
}

static int
nv50_vertprog_prepare(struct nv50_translation_info *ti)
{
   struct nv50_program *p = ti->p;
   int i, c;
   unsigned num_inputs = 0;

   ti->input_file = NV_FILE_MEM_S;
   ti->output_file = NV_FILE_OUT;

   for (i = 0; i <= ti->scan.file_max[TGSI_FILE_INPUT]; ++i) {
      p->in[i].id = i;
      p->in[i].hw = num_inputs;

      for (c = 0; c < 4; ++c) {
         if (!ti->input_access[i][c])
            continue;
         ti->input_map[i][c] = num_inputs++;
         p->vp.attrs[(4 * i + c) / 32] |= 1 << ((i * 4 + c) % 32);
      }
   }

   for (i = 0; i <= ti->scan.file_max[TGSI_FILE_OUTPUT]; ++i) {
      p->out[i].id = i;
      p->out[i].hw = p->max_out;

      for (c = 0; c < 4; ++c) {
         if (!ti->output_access[i][c])
            continue;
         ti->output_map[i][c] = p->max_out++;
         p->out[i].mask |= 1 << c;
      }
   }

   p->vp.clpd = p->max_out;
   p->max_out += p->vp.clpd_nr;

   for (i = 0; i < TGSI_SEMANTIC_COUNT; ++i) {
      switch (ti->sysval_map[i]) {
      case 2:
         if (!(ti->p->vp.attrs[2] & NV50_3D_VP_GP_BUILTIN_ATTR_EN_VERTEX_ID))
            ti->sysval_map[i] = 1;
         ti->sysval_map[i] = (ti->sysval_map[i] - 1) + num_inputs;
         break;
      default:
         break;
      }
   }

   if (p->vp.psiz < 0x40)
      p->vp.psiz = p->out[p->vp.psiz].hw;

   return 0;
}

static int
nv50_fragprog_prepare(struct nv50_translation_info *ti)
{
   struct nv50_program *p = ti->p;
   int i, j, c;
   unsigned nvary, nintp, depr;
   unsigned n = 0, m = 0, skip = 0;
   ubyte sn[16], si[16];

   /* FP flags */

   if (ti->scan.writes_z) {
      p->fp.flags[1] = 0x11;
      p->fp.flags[0] |= NV50_3D_FP_CONTROL_EXPORTS_Z;
   }

   if (ti->scan.uses_kill)
      p->fp.flags[0] |= NV50_3D_FP_CONTROL_USES_KIL;

   /* FP inputs */

   ti->input_file = NV_FILE_MEM_V;
   ti->output_file = NV_FILE_GPR;

   /* count non-flat inputs, save semantic info */
   for (i = 0; i < p->in_nr; ++i) {
      m += (ti->interp_mode[i] & NV50_INTERP_FLAT) ? 0 : 1;
      sn[i] = p->in[i].sn;
      si[i] = p->in[i].si;
   }

   /* reorder p->in[] so that non-flat inputs are first and
    * kick out special inputs that don't use VP/GP_RESULT_MAP
    */
   nintp = 0;
   for (i = 0; i < p->in_nr; ++i) {
      if (sn[i] == TGSI_SEMANTIC_POSITION) {
         for (c = 0; c < 4; ++c) {
            ti->input_map[i][c] = nintp;
            if (ti->input_access[i][c]) {
               p->fp.interp |= 1 << (24 + c);
               ++nintp;
            }
         }
         skip++;
         continue;
      } else
      if (sn[i] == TGSI_SEMANTIC_FACE) {
         ti->input_map[i][0] = 255;
         skip++;
         continue;
      }

      j = (ti->interp_mode[i] & NV50_INTERP_FLAT) ? m++ : n++;

      if (sn[i] == TGSI_SEMANTIC_COLOR)
         p->vp.bfc[si[i]] = j;
	   
      p->in[j].linear = (ti->interp_mode[i] & NV50_INTERP_LINEAR) ? 1 : 0;
      p->in[j].id = i;
      p->in[j].sn = sn[i];
      p->in[j].si = si[i];
   }
   assert(n <= m);
   p->in_nr -= skip;

   if (!(p->fp.interp & (8 << 24))) {
      p->fp.interp |= (8 << 24);
      ++nintp;
   }

   /* after HPOS */
   p->fp.colors = 4 << NV50_3D_SEMANTIC_COLOR_FFC0_ID__SHIFT;

   for (i = 0; i < p->in_nr; ++i) {
      int j = p->in[i].id;
      p->in[i].hw = nintp;

      for (c = 0; c < 4; ++c) {
         if (!ti->input_access[j][c])
            continue;
         p->in[i].mask |= 1 << c;
         ti->input_map[j][c] = nintp++;
      }
      /* count color inputs */
      if (i == p->vp.bfc[0] || i == p->vp.bfc[1])
         p->fp.colors += bitcount4(p->in[i].mask) << 16;
   }
   nintp -= bitcount4(p->fp.interp >> 24); /* subtract position inputs */
   nvary = nintp;
   if (n < m)
      nvary -= p->in[n].hw;

   p->fp.interp |= nvary << NV50_3D_FP_INTERPOLANT_CTRL_COUNT_NONFLAT__SHIFT;
   p->fp.interp |= nintp << NV50_3D_FP_INTERPOLANT_CTRL_COUNT__SHIFT;

   /* FP outputs */

   if (p->out_nr > (1 + (ti->scan.writes_z ? 1 : 0)))
      p->fp.flags[0] |= NV50_3D_FP_CONTROL_MULTIPLE_RESULTS;

   depr = p->out_nr;
   for (i = 0; i < p->out_nr; ++i) {
      p->out[i].id = i;
      if (p->out[i].sn == TGSI_SEMANTIC_POSITION) {
         depr = i;
         continue;
      }
      p->out[i].hw = p->max_out;
      p->out[i].mask = 0xf;

      for (c = 0; c < 4; ++c)
         ti->output_map[i][c] = p->max_out++;
   }
   if (depr < p->out_nr) {
      p->out[depr].mask = 0x4;
      p->out[depr].hw = ti->output_map[depr][2] = p->max_out++;
   } else {
      /* allowed values are 1, 4, 5, 8, 9, ... */
      p->max_out = MAX2(4, p->max_out);
   }

   return 0;
}

static int
nv50_geomprog_prepare(struct nv50_translation_info *ti)
{
   ti->input_file = NV_FILE_MEM_S;
   ti->output_file = NV_FILE_OUT;

   assert(0);
   return 1;
}

static int
nv50_prog_scan(struct nv50_translation_info *ti)
{
   struct nv50_program *p = ti->p;
   struct tgsi_parse_context parse;
   int ret, i;

   p->vp.edgeflag = 0x40;
   p->vp.psiz = 0x40;
   p->vp.bfc[0] = 0x40;
   p->vp.bfc[1] = 0x40;
   p->gp.primid = 0x80;

   tgsi_scan_shader(p->pipe.tokens, &ti->scan);

#if NV50_DEBUG & NV50_DEBUG_SHADER
   tgsi_dump(p->pipe.tokens, 0);
#endif

   ti->subr =
      CALLOC(ti->scan.opcode_count[TGSI_OPCODE_BGNSUB], sizeof(ti->subr[0]));

   ti->immd32 = (uint32_t *)MALLOC(ti->scan.immediate_count * 16);
   ti->immd32_ty = (ubyte *)MALLOC(ti->scan.immediate_count * sizeof(ubyte));

   ti->insns = MALLOC(ti->scan.num_instructions * sizeof(ti->insns[0]));

   tgsi_parse_init(&parse, p->pipe.tokens);
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
         ti->insns[ti->inst_nr] = parse.FullToken.FullInstruction;
         prog_inst(ti, &parse.FullToken.FullInstruction, ++ti->inst_nr);
         break;
      }
   }

   /* Scan to determine which registers are inputs/outputs of a subroutine. */
   for (i = 0; i < ti->subr_nr; ++i) {
      int pc = ti->subr[i].id;
      while (ti->insns[pc].Instruction.Opcode != TGSI_OPCODE_ENDSUB)
         prog_subroutine_inst(&ti->subr[i], &ti->insns[pc++]);
   }

   p->in_nr = ti->scan.file_max[TGSI_FILE_INPUT] + 1;
   p->out_nr = ti->scan.file_max[TGSI_FILE_OUTPUT] + 1;

   switch (p->type) {
   case PIPE_SHADER_VERTEX:
      ret = nv50_vertprog_prepare(ti);
      break;
   case PIPE_SHADER_FRAGMENT:
      ret = nv50_fragprog_prepare(ti);
      break;
   case PIPE_SHADER_GEOMETRY:
      ret = nv50_geomprog_prepare(ti);
      break;
   default:
      assert(!"unsupported program type");
      ret = -1;
      break;
   }

   assert(!ret);
   return ret;
}

/* Temporary, need a reference to nv50_ir_generate_code in libnv50 or
 * it "gets disappeared" and cannot be used in libnvc0 ...
 */
boolean
nv50_program_translate_new(struct nv50_program *p)
{
   struct nv50_ir_prog_info info;

   return nv50_ir_generate_code(&info);
}

boolean
nv50_program_translate(struct nv50_program *p)
{
   struct nv50_translation_info *ti;
   int ret;

   ti = CALLOC_STRUCT(nv50_translation_info);
   ti->p = p;

   ti->edgeflag_out = PIPE_MAX_SHADER_OUTPUTS;

   ret = nv50_prog_scan(ti);
   if (ret) {
      NOUVEAU_ERR("unsupported shader program\n");
      goto out;
   }

   ret = nv50_generate_code(ti);
   if (ret) {
      NOUVEAU_ERR("error during shader translation\n");
      goto out;
   }

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
nv50_program_destroy(struct nv50_context *nv50, struct nv50_program *p)
{
   const struct pipe_shader_state pipe = p->pipe;
   const ubyte type = p->type;

   if (p->mem)
      nouveau_heap_free(&p->mem);

   if (p->code)
      FREE(p->code);

   if (p->fixups)
      FREE(p->fixups);

   memset(p, 0, sizeof(*p));

   p->pipe = pipe;
   p->type = type;
}
