/*
 * Copyright (C) 2006 Ben Skeggs.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */

/*
 * Authors:
 *   Ben Skeggs <darktama@iinet.net.au>
 */

#include "glheader.h"
#include "macros.h"
#include "enums.h"

#include "program.h"
#include "programopt.h"
#include "program_instruction.h"

#include "nouveau_context.h"
#include "nouveau_shader.h"

static nvsFixedReg _tx_mesa_vp_dst_reg[VERT_RESULT_MAX] = {
   NVS_FR_POSITION, NVS_FR_COL0, NVS_FR_COL1, NVS_FR_FOGCOORD,
   NVS_FR_TEXCOORD0, NVS_FR_TEXCOORD1, NVS_FR_TEXCOORD2, NVS_FR_TEXCOORD3,
   NVS_FR_TEXCOORD4, NVS_FR_TEXCOORD5, NVS_FR_TEXCOORD6, NVS_FR_TEXCOORD7,
   NVS_FR_POINTSZ, NVS_FR_BFC0, NVS_FR_BFC1, NVS_FR_UNKNOWN /* EDGE */
};

static nvsFixedReg _tx_mesa_fp_dst_reg[FRAG_RESULT_MAX] = {
   NVS_FR_FRAGDATA0 /* COLR */, NVS_FR_FRAGDATA0 /* COLH */,
   NVS_FR_UNKNOWN /* DEPR */
};

static nvsFixedReg _tx_mesa_vp_src_reg[VERT_ATTRIB_MAX] = {
   NVS_FR_POSITION, NVS_FR_WEIGHT, NVS_FR_NORMAL, NVS_FR_COL0, NVS_FR_COL1,
   NVS_FR_FOGCOORD, NVS_FR_UNKNOWN /* COLOR_INDEX */, NVS_FR_UNKNOWN,
   NVS_FR_TEXCOORD0, NVS_FR_TEXCOORD1, NVS_FR_TEXCOORD2, NVS_FR_TEXCOORD3,
   NVS_FR_TEXCOORD4, NVS_FR_TEXCOORD5, NVS_FR_TEXCOORD6, NVS_FR_TEXCOORD7,
/* Generic attribs 0-15, aliased to the above */
   NVS_FR_POSITION, NVS_FR_WEIGHT, NVS_FR_NORMAL, NVS_FR_COL0, NVS_FR_COL1,
   NVS_FR_FOGCOORD, NVS_FR_UNKNOWN /* COLOR_INDEX */, NVS_FR_UNKNOWN,
   NVS_FR_TEXCOORD0, NVS_FR_TEXCOORD1, NVS_FR_TEXCOORD2, NVS_FR_TEXCOORD3,
   NVS_FR_TEXCOORD4, NVS_FR_TEXCOORD5, NVS_FR_TEXCOORD6, NVS_FR_TEXCOORD7
};

static nvsFixedReg _tx_mesa_fp_src_reg[FRAG_ATTRIB_MAX] = {
   NVS_FR_POSITION, NVS_FR_COL0, NVS_FR_COL1, NVS_FR_FOGCOORD,
   NVS_FR_TEXCOORD0, NVS_FR_TEXCOORD1, NVS_FR_TEXCOORD2, NVS_FR_TEXCOORD3,
   NVS_FR_TEXCOORD4, NVS_FR_TEXCOORD5, NVS_FR_TEXCOORD6, NVS_FR_TEXCOORD7
};

static nvsSwzComp _tx_mesa_swizzle[4] = {
   NVS_SWZ_X, NVS_SWZ_Y, NVS_SWZ_Z, NVS_SWZ_W
};

static nvsOpcode _tx_mesa_opcode[] = {
   [OPCODE_ABS] = NVS_OP_ABS, [OPCODE_ADD] = NVS_OP_ADD,
   [OPCODE_ARA] = NVS_OP_ARA, [OPCODE_ARL] = NVS_OP_ARL,
   [OPCODE_ARL_NV] = NVS_OP_ARL, [OPCODE_ARR] = NVS_OP_ARR,
   [OPCODE_CMP] = NVS_OP_CMP, [OPCODE_COS] = NVS_OP_COS,
   [OPCODE_DDX] = NVS_OP_DDX, [OPCODE_DDY] = NVS_OP_DDY,
   [OPCODE_DP3] = NVS_OP_DP3, [OPCODE_DP4] = NVS_OP_DP4,
   [OPCODE_DPH] = NVS_OP_DPH, [OPCODE_DST] = NVS_OP_DST,
   [OPCODE_EX2] = NVS_OP_EX2, [OPCODE_EXP] = NVS_OP_EXP,
   [OPCODE_FLR] = NVS_OP_FLR, [OPCODE_FRC] = NVS_OP_FRC,
   [OPCODE_KIL] = NVS_OP_EMUL, [OPCODE_KIL_NV] = NVS_OP_KIL,
   [OPCODE_LG2] = NVS_OP_LG2, [OPCODE_LIT] = NVS_OP_LIT,
   [OPCODE_LOG] = NVS_OP_LOG,
   [OPCODE_LRP] = NVS_OP_LRP,
   [OPCODE_MAD] = NVS_OP_MAD, [OPCODE_MAX] = NVS_OP_MAX,
   [OPCODE_MIN] = NVS_OP_MIN, [OPCODE_MOV] = NVS_OP_MOV,
   [OPCODE_MUL] = NVS_OP_MUL,
   [OPCODE_PK2H] = NVS_OP_PK2H, [OPCODE_PK2US] = NVS_OP_PK2US,
   [OPCODE_PK4B] = NVS_OP_PK4B, [OPCODE_PK4UB] = NVS_OP_PK4UB,
   [OPCODE_POW] = NVS_OP_POW, [OPCODE_POPA] = NVS_OP_POPA,
   [OPCODE_PUSHA] = NVS_OP_PUSHA,
   [OPCODE_RCC] = NVS_OP_RCC, [OPCODE_RCP] = NVS_OP_RCP,
   [OPCODE_RFL] = NVS_OP_RFL, [OPCODE_RSQ] = NVS_OP_RSQ,
   [OPCODE_SCS] = NVS_OP_SCS, [OPCODE_SEQ] = NVS_OP_SEQ,
   [OPCODE_SFL] = NVS_OP_SFL, [OPCODE_SGE] = NVS_OP_SGE,
   [OPCODE_SGT] = NVS_OP_SGT, [OPCODE_SIN] = NVS_OP_SIN,
   [OPCODE_SLE] = NVS_OP_SLE, [OPCODE_SLT] = NVS_OP_SLT,
   [OPCODE_SNE] = NVS_OP_SNE, [OPCODE_SSG] = NVS_OP_SSG,
   [OPCODE_STR] = NVS_OP_STR, [OPCODE_SUB] = NVS_OP_SUB,
   [OPCODE_SWZ] = NVS_OP_MOV,
   [OPCODE_TEX] = NVS_OP_TEX, [OPCODE_TXB] = NVS_OP_TXB,
   [OPCODE_TXD] = NVS_OP_TXD,
   [OPCODE_TXL] = NVS_OP_TXL, [OPCODE_TXP] = NVS_OP_TXP,
   [OPCODE_TXP_NV] = NVS_OP_TXP,
   [OPCODE_UP2H] = NVS_OP_UP2H, [OPCODE_UP2US] = NVS_OP_UP2US,
   [OPCODE_UP4B] = NVS_OP_UP4B, [OPCODE_UP4UB] = NVS_OP_UP4UB,
   [OPCODE_X2D] = NVS_OP_X2D,
   [OPCODE_XPD] = NVS_OP_XPD
};

static nvsCond _tx_mesa_condmask[] = {
   NVS_COND_UNKNOWN, NVS_COND_GT, NVS_COND_LT, NVS_COND_UN, NVS_COND_GE,
   NVS_COND_LE, NVS_COND_NE, NVS_COND_NE, NVS_COND_TR, NVS_COND_FL
};

struct pass0_rec {
   int nvs_ipos;
   int next_temp;
   int swzconst_done;
   int swzconst_id;
   nvsRegister const_half;
};

#define X NVS_SWZ_X
#define Y NVS_SWZ_Y
#define Z NVS_SWZ_Z
#define W NVS_SWZ_W

static void
pass0_append_fragment(nouveauShader *nvs, nvsFragmentHeader *fragment)
{
   nvsFragmentList *list = calloc(1, sizeof(nvsFragmentList));
   if (!list)
      return;

   list->fragment = fragment;
   list->prev     = nvs->list_tail;
   if ( nvs->list_tail)
      nvs->list_tail->next = list;
   if (!nvs->list_head)
      nvs->list_head = list;
   nvs->list_tail = list;

   nvs->inst_count++;
}

static void
pass0_make_reg(nouveauShader *nvs, nvsRegister *reg,
      	       nvsRegFile file, unsigned int index)
{
   struct pass0_rec *rec = nvs->pass_rec;

   /* defaults */
   *reg = nvr_unused;
   /* -1 == quick-and-dirty temp alloc */
   if (file == NVS_FILE_TEMP && index == -1) {
      index = rec->next_temp++;
      assert(index < NVS_MAX_TEMPS);
   }
   reg->file   = file;
   reg->index  = index;
}

static void
pass0_make_swizzle(nvsSwzComp *swz, unsigned int mesa)
{
   int i;

   for (i=0;i<4;i++)
      swz[i] = _tx_mesa_swizzle[GET_SWZ(mesa, i)];
}

static nvsOpcode
pass0_make_opcode(enum prog_opcode op)
{
   if (op > MAX_OPCODE)
      return NVS_OP_UNKNOWN;
   return _tx_mesa_opcode[op];
}

static nvsCond
pass0_make_condmask(GLuint mesa)
{
   if (mesa > COND_FL)
      return NVS_COND_UNKNOWN;
   return _tx_mesa_condmask[mesa];
}

static unsigned int
pass0_make_mask(GLuint mesa_mask)
{
   unsigned int mask = 0;

   if (mesa_mask & WRITEMASK_X) mask |= SMASK_X;
   if (mesa_mask & WRITEMASK_Y) mask |= SMASK_Y;
   if (mesa_mask & WRITEMASK_Z) mask |= SMASK_Z;
   if (mesa_mask & WRITEMASK_W) mask |= SMASK_W;

   return mask;
}

static nvsTexTarget
pass0_make_tex_target(GLuint mesa)
{
   switch (mesa) {
   case TEXTURE_1D_INDEX:   return NVS_TEX_TARGET_1D;
   case TEXTURE_2D_INDEX:   return NVS_TEX_TARGET_2D;
   case TEXTURE_3D_INDEX:   return NVS_TEX_TARGET_3D;
   case TEXTURE_CUBE_INDEX: return NVS_TEX_TARGET_CUBE;
   case TEXTURE_RECT_INDEX: return NVS_TEX_TARGET_RECT;
   default:
      return NVS_TEX_TARGET_UNKNOWN;
   }
}

static void
pass0_make_dst_reg(nvsPtr nvs, nvsRegister *reg,
      		   struct prog_dst_register *dst)
{
   struct gl_program *mesa = (struct gl_program*)&nvs->mesa.vp;
   nvsFixedReg sfr;

   switch (dst->File) {
   case PROGRAM_OUTPUT:
      if (mesa->Target == GL_VERTEX_PROGRAM_ARB) {
	 sfr = (dst->Index < VERT_RESULT_MAX) ?
	    _tx_mesa_vp_dst_reg[dst->Index] : NVS_FR_UNKNOWN;
      } else {
	 sfr = (dst->Index < FRAG_RESULT_MAX) ?
	    _tx_mesa_fp_dst_reg[dst->Index] : NVS_FR_UNKNOWN;
      }
      pass0_make_reg(nvs, reg, NVS_FILE_RESULT, sfr);
      break;
   case PROGRAM_TEMPORARY:
      pass0_make_reg(nvs, reg, NVS_FILE_TEMP, dst->Index);
      break;
   case PROGRAM_ADDRESS:
      pass0_make_reg(nvs, reg, NVS_FILE_ADDRESS, dst->Index);
      break;
   default:
      fprintf(stderr, "Unknown dest file %d\n", dst->File);
      assert(0);
   }
}

static void
pass0_make_src_reg(nvsPtr nvs, nvsRegister *reg, struct prog_src_register *src)
{
   struct gl_program *mesa = (struct gl_program *)&nvs->mesa.vp.Base;
   struct gl_program_parameter_list *p = mesa->Parameters;

   *reg = nvr_unused;

   switch (src->File) {
   case PROGRAM_INPUT:
      reg->file = NVS_FILE_ATTRIB;
      if (mesa->Target == GL_VERTEX_PROGRAM_ARB) {
	 reg->index = (src->Index < VERT_ATTRIB_MAX) ?
	    _tx_mesa_vp_src_reg[src->Index] : NVS_FR_UNKNOWN;
      } else {
	 reg->index = (src->Index < FRAG_ATTRIB_MAX) ?
	    _tx_mesa_fp_src_reg[src->Index] : NVS_FR_UNKNOWN;
      }
      break;
   /* All const types seem to get shoved into here, not really sure why */
   case PROGRAM_STATE_VAR:
      switch (p->Parameters[src->Index].Type) {
      case PROGRAM_NAMED_PARAM:
      case PROGRAM_CONSTANT:
	 nvs->params[src->Index].source_val = NULL;
	 COPY_4V(nvs->params[src->Index].val, p->ParameterValues[src->Index]); 
	 break;
      case PROGRAM_STATE_VAR:
	 nvs->params[src->Index].source_val = p->ParameterValues[src->Index];
	 break;
      default:
	 fprintf(stderr, "Unknown parameter type %d\n",
	       p->Parameters[src->Index].Type);
	 assert(0);
	 break;
      }

      if (src->RelAddr) {
	 reg->indexed	= 1;
	 reg->addr_reg	= 0;
	 reg->addr_comp	= NVS_SWZ_X;
      } else
	 reg->indexed = 0;
      reg->file  = NVS_FILE_CONST;
      reg->index = src->Index;
      break;
   case PROGRAM_TEMPORARY:
      reg->file  = NVS_FILE_TEMP;
      reg->index = src->Index;
      break;
   default:
      fprintf(stderr, "Unknown source type %d\n", src->File);
      assert(0);
   }

   /* per-component negate handled elsewhere */
   reg->negate = src->NegateBase != 0;
   reg->abs    = src->Abs;
   pass0_make_swizzle(reg->swizzle, src->Swizzle);
}

static nvsInstruction *
pass0_emit(nouveauShader *nvs, nvsOpcode op, nvsRegister dst,
      	   unsigned int mask, int saturate,
	   nvsRegister src0, nvsRegister src1, nvsRegister src2)
{
   struct pass0_rec *rec = nvs->pass_rec;
   nvsInstruction *sif = NULL;

   /* Seems mesa doesn't explicitly 0 this.. */
   if (nvs->mesa.vp.Base.Target == GL_VERTEX_PROGRAM_ARB)
      saturate = 0;

   sif = calloc(1, sizeof(nvsInstruction));
   if (sif) {
      sif->header.type     = NVS_INSTRUCTION;
      sif->header.position = rec->nvs_ipos++;
      sif->op		= op;
      sif->saturate	= saturate;
      sif->dest		= dst;
      sif->mask		= mask;
      sif->src[0]	= src0;
      sif->src[1]	= src1;
      sif->src[2]	= src2;
      sif->cond		= COND_TR;
      sif->cond_reg	= 0;
      sif->cond_test	= 0;
      sif->cond_update	= 0;
      pass0_make_swizzle(sif->cond_swizzle, SWIZZLE_NOOP);
      pass0_append_fragment(nvs, (nvsFragmentHeader *)sif);
   }

   return sif;
}

static void
pass0_fixup_swizzle(nvsPtr nvs,
      		    struct prog_src_register *src,
		    unsigned int sm1,
		    unsigned int sm2)
{
   static const float sc[4] = { 1.0, 0.0, -1.0, 0.0 };
   struct pass0_rec *rec = nvs->pass_rec;
   int fixup_1, fixup_2;
   nvsRegister sr, dr = nvr_unused;
   nvsRegister sm1const, sm2const;

   if (!rec->swzconst_done) {
      struct gl_program *prog = &nvs->mesa.vp.Base;
      rec->swzconst_id = _mesa_add_unnamed_constant(prog->Parameters, sc, 4);
      rec->swzconst_done = 1;
      COPY_4V(nvs->params[rec->swzconst_id].val, sc);
   }

   fixup_1 = (sm1 != MAKE_SWIZZLE4(0,0,0,0) && sm2 != MAKE_SWIZZLE4(2,2,2,2));
   fixup_2 = (sm2 != MAKE_SWIZZLE4(2,2,2,2));

   if (src->File != PROGRAM_TEMPORARY && src->File != PROGRAM_INPUT) {
      /* We can't use more than one const in an instruction, so move the const
       * into a temp, and swizzle from there.
       *TODO: should just emit the swizzled const, instead of swizzling it
       *      in the shader.. would need to reswizzle any state params when they
       *      change however..
       */
      pass0_make_reg(nvs, &dr, NVS_FILE_TEMP, -1);
      pass0_make_src_reg(nvs, &sr, src);
      pass0_emit(nvs, NVS_OP_MOV, dr, SMASK_ALL, 0, sr, nvr_unused, nvr_unused);
      pass0_make_reg(nvs, &sr, NVS_FILE_TEMP, dr.index);
   } else {
      if (fixup_1)
	 src->NegateBase = 0;
      pass0_make_src_reg(nvs, &sr, src);
      pass0_make_reg(nvs, &dr, NVS_FILE_TEMP, -1);
   }

   pass0_make_reg(nvs, &sm1const, NVS_FILE_CONST, rec->swzconst_id);
   pass0_make_swizzle(sm1const.swizzle, sm1);
   if (fixup_1 && fixup_2) {
      /* Any combination with SWIZZLE_ONE */
      pass0_make_reg(nvs, &sm2const, NVS_FILE_CONST, rec->swzconst_id);
      pass0_make_swizzle(sm2const.swizzle, sm2);
      pass0_emit(nvs, NVS_OP_MAD, dr, SMASK_ALL, 0, sr, sm1const, sm2const);
   } else {
      /* SWIZZLE_ZERO || arbitrary negate */
      pass0_emit(nvs, NVS_OP_MUL, dr, SMASK_ALL, 0, sr, sm1const, nvr_unused);
   }

   src->File	= PROGRAM_TEMPORARY;
   src->Index	= dr.index;
   src->Swizzle	= SWIZZLE_NOOP;
}

#define SET_SWZ(fs, cp, c) fs = (fs & ~(0x7<<(cp*3))) | (c<<(cp*3))
static void
pass0_check_sources(nvsPtr nvs, struct prog_instruction *inst)
{
   unsigned int insrc = -1, constsrc = -1;
   int i;

   for (i=0;i<_mesa_num_inst_src_regs(inst->Opcode);i++) {
      struct prog_src_register *src = &inst->SrcReg[i];
      unsigned int sm_1 = 0, sm_2 = 0;
      nvsRegister sr, dr;
      int do_mov = 0, c;

      /* Build up swizzle masks as if we were going to use
       * "MAD new, src, const1, const2" to support arbitrary negation
       * and SWIZZLE_ZERO/SWIZZLE_ONE.
       */
      for (c=0;c<4;c++) {
	 if (GET_SWZ(src->Swizzle, c) == SWIZZLE_ZERO) {
	    SET_SWZ(sm_1, c, SWIZZLE_Y); /* 0.0 */
	    SET_SWZ(sm_2, c, SWIZZLE_Y);
	    SET_SWZ(src->Swizzle, c, SWIZZLE_X);
	 } else if (GET_SWZ(src->Swizzle, c) == SWIZZLE_ONE) {
	    SET_SWZ(sm_1, c, SWIZZLE_Y);
	    if (src->NegateBase & (1<<c))
	       SET_SWZ(sm_2, c, SWIZZLE_Z); /* -1.0 */
	    else
	       SET_SWZ(sm_2, c, SWIZZLE_X); /* 1.0 */
	    SET_SWZ(src->Swizzle, c, SWIZZLE_X);
	 } else {
	    if (src->NegateBase & (1<<c))
	       SET_SWZ(sm_1, c, SWIZZLE_Z); /* -[xyzw] */
	    else
	       SET_SWZ(sm_1, c, SWIZZLE_X); /* [xyzw] */
	    SET_SWZ(sm_2, c, SWIZZLE_Y);
	 }
      }
      /* Unless we're multiplying by 1.0 or -1.0 on all components, and we're
       * adding nothing to any component we have to emulate the swizzle.
       */
      if ((sm_1 != MAKE_SWIZZLE4(0,0,0,0) && sm_1 != MAKE_SWIZZLE4(2,2,2,2)) ||
	       sm_2 != MAKE_SWIZZLE4(1,1,1,1)) {
	 pass0_fixup_swizzle(nvs, src, sm_1, sm_2);
	 /* The source is definitely in a temp now, so don't bother checking 
	  * for multiple ATTRIB/CONST regs.
	  */
	 continue;
      }

      /* HW can't use more than one ATTRIB or PARAM in a single instruction */
      switch (src->File) {
      case PROGRAM_INPUT:
	 if (insrc != -1 && insrc != src->Index) 
	    do_mov = 1;
	 else insrc = src->Index;
	 break;
      case PROGRAM_STATE_VAR:
	 if (constsrc != -1 && constsrc != src->Index)
	    do_mov = 1;
	 else constsrc = src->Index;
	 break;
      default:
	 break;
      }

      /* Emit any extra ATTRIB/CONST to a temp, and modify the Mesa instruction
       * to point at the temp.
       */
      if (do_mov) {
	 pass0_make_src_reg(nvs, &sr, src);
	 pass0_make_reg(nvs, &dr, NVS_FILE_TEMP, -1);
	 pass0_emit(nvs, NVS_OP_MOV, dr, SMASK_ALL, 0,
	       sr, nvr_unused, nvr_unused);

	 src->File	= PROGRAM_TEMPORARY;
	 src->Index	= dr.index;
	 src->Swizzle= SWIZZLE_NOOP;
      }
   }
}

static GLboolean
pass0_emulate_instruction(nouveauShader *nvs, struct prog_instruction *inst)
{
   nvsFunc *shader = nvs->func;
   nvsRegister src[3], dest, temp;
   nvsInstruction *nvsinst;
   struct pass0_rec *rec = nvs->pass_rec;
   unsigned int mask = pass0_make_mask(inst->DstReg.WriteMask);
   int i, sat;

   sat = (inst->SaturateMode == SATURATE_ZERO_ONE);

   /* Build all the "real" regs for the instruction */
   for (i=0; i<_mesa_num_inst_src_regs(inst->Opcode); i++)
      pass0_make_src_reg(nvs, &src[i], &inst->SrcReg[i]);
   if (inst->Opcode != OPCODE_KIL)
      pass0_make_dst_reg(nvs, &dest, &inst->DstReg);

   switch (inst->Opcode) {
   case OPCODE_ABS:
      if (shader->caps & SCAP_SRC_ABS)
	 pass0_emit(nvs, NVS_OP_MOV, dest, mask, sat,
	       	      nvsAbs(src[0]), nvr_unused, nvr_unused);
      else
	 pass0_emit(nvs, NVS_OP_MAX, dest, mask, sat,
	       	      src[0], nvsNegate(src[0]), nvr_unused);
      break;
   case OPCODE_KIL:
      /* This is only in ARB shaders, so we don't have to worry
       * about clobbering a CC reg as they aren't supported anyway.
       */
      /* MOVC0 temp, src */
      pass0_make_reg(nvs, &temp, NVS_FILE_TEMP, -1);
      nvsinst = pass0_emit(nvs, NVS_OP_MOV, temp, SMASK_ALL, 0,
	    		   src[0], nvr_unused, nvr_unused);
      nvsinst->cond_update = 1;
      nvsinst->cond_reg    = 0;
      /* KIL_NV (LT0.xyzw) temp */
      nvsinst = pass0_emit(nvs, NVS_OP_KIL, nvr_unused, 0, 0,
	    		   nvr_unused, nvr_unused, nvr_unused);
      nvsinst->cond      = COND_LT;
      nvsinst->cond_reg  = 0;
      nvsinst->cond_test = 1;
      pass0_make_swizzle(nvsinst->cond_swizzle, MAKE_SWIZZLE4(0,1,2,3));
      break;
   case OPCODE_LIT:
      break;
   case OPCODE_LRP:
      pass0_make_reg(nvs, &temp, NVS_FILE_TEMP, -1);
      pass0_emit(nvs, NVS_OP_MAD, temp, mask, 0,
	    	 nvsNegate(src[0]), src[2], src[2]);
      pass0_emit(nvs, NVS_OP_MAD, dest, mask, sat,
	    	 src[0], src[1], temp);
      break;
   case OPCODE_POW:
      if (shader->SupportsOpcode(shader, NVS_OP_LG2) &&
	    shader->SupportsOpcode(shader, NVS_OP_EX2)) {
	 pass0_make_reg(nvs, &temp, NVS_FILE_TEMP, -1);
	 /* LG2 temp.x, src0.c */
	 pass0_emit(nvs, NVS_OP_LG2, temp, SMASK_X, 0,
	       	      nvsSwizzle(src[0], X, X, X, X),
		      nvr_unused,
		      nvr_unused);
	 /* MUL temp.x, temp.x, src1.c */
	 pass0_emit(nvs, NVS_OP_MUL, temp, SMASK_X, 0,
	       	      nvsSwizzle(temp, X, X, X, X),
		      nvsSwizzle(src[1], X, X, X, X),
		      nvr_unused);
	 /* EX2 dest, temp.x */
	 pass0_emit(nvs, NVS_OP_EX2, dest, mask, sat,
	       	      nvsSwizzle(temp, X, X, X, X),
		      nvr_unused,
		      nvr_unused);
      } else {
	 /* can we use EXP/LOG instead of EX2/LG2?? */
	 fprintf(stderr, "Implement POW for NV20 vtxprog!\n");
	 return GL_FALSE;
      }
      break;
   case OPCODE_RSQ:
      if (rec->const_half.file != NVS_FILE_CONST) {
	 GLfloat const_half[4] = { 0.5, 0.0, 0.0, 0.0 };
	 pass0_make_reg(nvs, &rec->const_half, NVS_FILE_CONST,
	       _mesa_add_unnamed_constant(nvs->mesa.vp.Base.Parameters,
		  			  const_half, 4));
	 COPY_4V(nvs->params[rec->const_half.index].val, const_half);
      }
      pass0_make_reg(nvs, &temp, NVS_FILE_TEMP, -1);
      pass0_emit(nvs, NVS_OP_LG2, temp, SMASK_X, 0,
	    	 nvsAbs(nvsSwizzle(src[0], X, X, X, X)),
		 nvr_unused,
		 nvr_unused);
      pass0_emit(nvs, NVS_OP_MUL, temp, SMASK_X, 0,
	    	 nvsSwizzle(temp, X, X, X, X),
		 nvsNegate(rec->const_half),
		 nvr_unused);
      pass0_emit(nvs, NVS_OP_EX2, dest, mask, sat,
	    	 nvsSwizzle(temp, X, X, X, X),
		 nvr_unused,
		 nvr_unused);
      break;
   case OPCODE_SCS:
      if (mask & SMASK_X)
	 pass0_emit(nvs, NVS_OP_COS, dest, SMASK_X, sat,
	       	      nvsSwizzle(src[0], X, X, X, X),
		      nvr_unused,
		      nvr_unused);
      if (mask & SMASK_Y)
	 pass0_emit(nvs, NVS_OP_SIN, dest, SMASK_Y, sat,
	       	      nvsSwizzle(src[0], X, X, X, X),
		      nvr_unused,
		      nvr_unused);
      break;
   case OPCODE_SUB:
      pass0_emit(nvs, NVS_OP_ADD, dest, mask, sat,
	    	 src[0], nvsNegate(src[1]), nvr_unused);
      break;
   case OPCODE_XPD:
      pass0_make_reg(nvs, &temp, NVS_FILE_TEMP, -1);
      pass0_emit(nvs, NVS_OP_MUL, temp, SMASK_ALL, 0,
	    	   nvsSwizzle(src[0], Z, X, Y, Y),
		   nvsSwizzle(src[1], Y, Z, X, X),
		   nvr_unused);
      pass0_emit(nvs, NVS_OP_MAD, dest, (mask & ~SMASK_W), sat,
	    	   nvsSwizzle(src[0], Y, Z, X, X),
		   nvsSwizzle(src[1], Z, X, Y, Y),
		   nvsNegate(temp));
      break;
   default:
      fprintf(stderr, "hw doesn't support opcode \"%s\", and no emulation found\n",
	      _mesa_opcode_string(inst->Opcode));
      return GL_FALSE;
   }

   return GL_TRUE;
}

static GLboolean
pass0_translate_instructions(nouveauShader *nvs)
{
   struct gl_program *prog = (struct gl_program *)&nvs->mesa.vp;
   nvsFunc *shader = nvs->func;
   int ipos;

   for (ipos=0; ipos<prog->NumInstructions; ipos++) {
      struct prog_instruction *inst = &prog->Instructions[ipos];

      if (inst->Opcode == OPCODE_END)
	 break;

      /* Deal with multiple ATTRIB/PARAM in a single instruction */
      pass0_check_sources(nvs, inst);
 
      /* Now it's safe to do the prog_instruction->nvsInstruction conversion */
      if (shader->SupportsOpcode(shader, pass0_make_opcode(inst->Opcode))) {
	 nvsInstruction *nvsinst;
	 nvsRegister src[3], dest;
	 int i;

	 for (i=0; i<_mesa_num_inst_src_regs(inst->Opcode); i++)
	    pass0_make_src_reg(nvs, &src[i], &inst->SrcReg[i]);
	 pass0_make_dst_reg(nvs, &dest, &inst->DstReg);

	 nvsinst = pass0_emit(nvs,
	       		      pass0_make_opcode(inst->Opcode),
			      dest,
			      pass0_make_mask(inst->DstReg.WriteMask),
			      (inst->SaturateMode != SATURATE_OFF),
			      src[0], src[1], src[2]);
	 nvsinst->tex_unit   = inst->TexSrcUnit;
	 nvsinst->tex_target = pass0_make_tex_target(inst->TexSrcTarget);
	 /* TODO when NV_fp/vp is implemented */
	 nvsinst->cond       = COND_TR;
      } else {
	 if (!pass0_emulate_instruction(nvs, inst))
	    return GL_FALSE;
      }
   }

   return GL_TRUE;
}

GLboolean
nouveau_shader_pass0(GLcontext *ctx, nouveauShader *nvs)
{
   nouveauContextPtr		 nmesa	= NOUVEAU_CONTEXT(ctx);
   struct gl_program		*prog	= (struct gl_program*)nvs;
   struct gl_vertex_program	*vp 	= (struct gl_vertex_program *)prog;
   struct gl_fragment_program	*fp	= (struct gl_fragment_program *)prog;
   struct pass0_rec *rec;
   int ret;

   switch (prog->Target) {
   case GL_VERTEX_PROGRAM_ARB:
      nvs->func = &nmesa->VPfunc;
      if (vp->IsPositionInvariant)
	 _mesa_insert_mvp_code(ctx, vp);
#if 0
      if (IS_FIXEDFUNCTION_PROG && CLIP_PLANES_USED)
	 pass0_insert_ff_clip_planes();
#endif
      break;
   case GL_FRAGMENT_PROGRAM_ARB:
      nvs->func = &nmesa->FPfunc;
      if (fp->FogOption != GL_NONE)
	 _mesa_append_fog_code(ctx, fp);
      break;
   default:
      fprintf(stderr, "Unknown program type %d", prog->Target);
      return GL_FALSE;
   }

   rec = calloc(1, sizeof(struct pass0_rec));
   rec->next_temp = prog->NumTemporaries;
   nvs->pass_rec = rec;

   ret = pass0_translate_instructions(nvs);
   if (!ret) {
      /* DESTROY list */
   }

   free(nvs->pass_rec);
   return ret;
}

