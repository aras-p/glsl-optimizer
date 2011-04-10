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

#include "nvc0_pc.h"
#include "nvc0_program.h"

#define NVC0_FIXUP_CODE_RELOC 0
#define NVC0_FIXUP_DATA_RELOC 1

struct nvc0_fixup {
   uint8_t type;
   int8_t shift;
   uint32_t mask;
   uint32_t data;
   uint32_t ofst;
};

void
nvc0_relocate_program(struct nvc0_program *prog,
                      uint32_t code_base,
                      uint32_t data_base)
{
   struct nvc0_fixup *f = (struct nvc0_fixup *)prog->relocs;
   unsigned i;

   for (i = 0; i < prog->num_relocs; ++i) {
      uint32_t data;

      switch (f[i].type) {
      case NVC0_FIXUP_CODE_RELOC: data = code_base + f[i].data; break;
      case NVC0_FIXUP_DATA_RELOC: data = data_base + f[i].data; break;
      default:
         data = f[i].data;
         break;
      }
      data = (f[i].shift < 0) ? (data >> -f[i].shift) : (data << f[i].shift);

      prog->code[f[i].ofst / 4] &= ~f[i].mask;
      prog->code[f[i].ofst / 4] |= data & f[i].mask;
   }
}

static void
create_fixup(struct nv_pc *pc, uint8_t ty,
             int w, uint32_t data, uint32_t m, int s)
{
   struct nvc0_fixup *f;

   const unsigned size = sizeof(struct nvc0_fixup);
   const unsigned n = pc->num_relocs;

   if (!(n % 8))
      pc->reloc_entries = REALLOC(pc->reloc_entries, n * size, (n + 8) * size);

   f = (struct nvc0_fixup *)pc->reloc_entries;

   f[n].ofst = pc->emit_pos + w * 4;
   f[n].type = ty;
   f[n].data = data;
   f[n].mask = m;
   f[n].shift = s;

   ++pc->num_relocs;
}

static INLINE ubyte
SSIZE(struct nv_instruction *nvi, int s)
{
   return nvi->src[s]->value->reg.size;
}

static INLINE ubyte
DSIZE(struct nv_instruction *nvi, int d)
{
   return nvi->def[d]->reg.size;
}

static INLINE struct nv_reg *
SREG(struct nv_ref *ref)
{
   if (!ref)
      return NULL;
   return &ref->value->join->reg;
}

static INLINE struct nv_reg *
DREG(struct nv_value *val)
{
   if (!val)
      return NULL;
   return &val->join->reg;
}

static INLINE ubyte
SFILE(struct nv_instruction *nvi, int s)
{
   return nvi->src[s]->value->reg.file;
}

static INLINE ubyte
DFILE(struct nv_instruction *nvi, int d)
{
   return nvi->def[0]->reg.file;
}

static INLINE void
SID(struct nv_pc *pc, struct nv_ref *ref, int pos)
{
   pc->emit[pos / 32] |= (SREG(ref) ? SREG(ref)->id : 63) << (pos % 32);
}

static INLINE void
DID(struct nv_pc *pc, struct nv_value *val, int pos)
{
   pc->emit[pos / 32] |= (DREG(val) ? DREG(val)->id : 63) << (pos % 32);
}

static INLINE uint32_t
get_immd_u32(struct nv_ref *ref) /* XXX: dependent on [0]:2 */
{
   assert(ref->value->reg.file == NV_FILE_IMM);
   return ref->value->reg.imm.u32;
}

static INLINE void
set_immd_u32_l(struct nv_pc *pc, uint32_t u32)
{
   pc->emit[0] |= (u32 & 0x3f) << 26;
   pc->emit[1] |= u32 >> 6;
}

static INLINE void
set_immd_u32(struct nv_pc *pc, uint32_t u32)
{
   if ((pc->emit[0] & 0xf) == 0x2) {
      set_immd_u32_l(pc, u32);
   } else
   if ((pc->emit[0] & 0xf) == 0x3) {
      assert(!(pc->emit[1] & 0xc000));
      pc->emit[1] |= 0xc000;
      assert(!(u32 & 0xfff00000));
      set_immd_u32_l(pc, u32);
   } else {
      assert(!(pc->emit[1] & 0xc000));
      pc->emit[1] |= 0xc000;
      assert(!(u32 & 0xfff));
      set_immd_u32_l(pc, u32 >> 12);
   }
}

static INLINE void
set_immd(struct nv_pc *pc, struct nv_instruction *i, int s)
{
   set_immd_u32(pc, get_immd_u32(i->src[s]));
}

static INLINE void
DVS(struct nv_pc *pc, struct nv_instruction *i)
{
   uint s = i->def[0]->reg.size;
   int n;
   for (n = 1; n < 4 && i->def[n]; ++n)
      s += i->def[n]->reg.size;
   pc->emit[0] |= ((s / 4) - 1) << 5;
}

static INLINE void
SVS(struct nv_pc *pc, struct nv_ref *src)
{
   pc->emit[0] |= (SREG(src)->size / 4 - 1) << 5;
}

static void
set_pred(struct nv_pc *pc, struct nv_instruction *i)
{
   if (i->predicate >= 0) {
      SID(pc, i->src[i->predicate], 6);
      if (i->cc)
         pc->emit[0] |= 0x2000; /* negate */
   } else {
      pc->emit[0] |= 0x1c00;
   }	   
}

static INLINE void
set_address_16(struct nv_pc *pc, struct nv_ref *src)
{
   pc->emit[0] |= (src->value->reg.address & 0x003f) << 26;
   pc->emit[1] |= (src->value->reg.address & 0xffc0) >> 6;
}

static INLINE unsigned
const_space_index(struct nv_instruction *i, int s)
{
   return SFILE(i, s) - NV_FILE_MEM_C(0);
}

static void
emit_flow(struct nv_pc *pc, struct nv_instruction *i, uint8_t op)
{
   pc->emit[0] = 0x00000007;
   pc->emit[1] = op << 24;

   if (op == 0x40 || (op >= 0x80 && op <= 0x98)) {
      /* bra, exit, ret or kil */
      pc->emit[0] |= 0x1e0;
      set_pred(pc, i);
   }

   if (i->target) {
      int32_t pcrel = i->target->emit_pos - (pc->emit_pos + 8);

      /* we will need relocations only for global functions */
      /*
      create_fixup(pc, NVC0_FIXUP_CODE_RELOC, 0, pos, 26, 0xfc000000);
      create_fixup(pc, NVC0_FIXUP_CODE_RELOC, 1, pos, -6, 0x0001ffff);
      */

      pc->emit[0] |= (pcrel & 0x3f) << 26;
      pc->emit[1] |= (pcrel >> 6) & 0x3ffff;
   }
}

/* doesn't work for vfetch, export, ld, st, mov ... */
static void
emit_form_0(struct nv_pc *pc, struct nv_instruction *i)
{
   int s;

   set_pred(pc, i);

   DID(pc, i->def[0], 14);

   for (s = 0; s < 3 && i->src[s]; ++s) {
      if (SFILE(i, s) >= NV_FILE_MEM_C(0) &&
          SFILE(i, s) <= NV_FILE_MEM_C(15)) {
         assert(!(pc->emit[1] & 0xc000));
         assert(s <= 1);
         pc->emit[1] |= 0x4000 | (const_space_index(i, s) << 10);
         set_address_16(pc, i->src[s]);
      } else
      if (SFILE(i, s) == NV_FILE_GPR) {
         SID(pc, i->src[s], s ? ((s == 2) ? 49 : 26) : 20);
      } else
      if (SFILE(i, s) == NV_FILE_IMM) {
         assert(!(pc->emit[1] & 0xc000));
         assert(s == 1 || i->opcode == NV_OP_MOV);
         set_immd(pc, i, s);
      }
   }
}

static void
emit_form_1(struct nv_pc *pc, struct nv_instruction *i)
{
   int s;

   set_pred(pc, i);

   DID(pc, i->def[0], 14);

   for (s = 0; s < 1 && i->src[s]; ++s) {
      if (SFILE(i, s) >= NV_FILE_MEM_C(0) &&
          SFILE(i, s) <= NV_FILE_MEM_C(15)) {
         assert(!(pc->emit[1] & 0xc000));
         assert(s <= 1);
         pc->emit[1] |= 0x4000 | (const_space_index(i, s) << 10);
         set_address_16(pc, i->src[s]);
      } else
      if (SFILE(i, s) == NV_FILE_GPR) {
         SID(pc, i->src[s], 26);
      } else
      if (SFILE(i, s) == NV_FILE_IMM) {
         assert(!(pc->emit[1] & 0xc000));
         assert(s == 1 || i->opcode == NV_OP_MOV);
         set_immd(pc, i, s);
      }
   }
}

static void
emit_neg_abs_1_2(struct nv_pc *pc, struct nv_instruction *i)
{
   if (i->src[0]->mod & NV_MOD_ABS)
      pc->emit[0] |= 1 << 7;
   if (i->src[0]->mod & NV_MOD_NEG)
      pc->emit[0] |= 1 << 9;
   if (i->src[1]->mod & NV_MOD_ABS)
      pc->emit[0] |= 1 << 6;
   if (i->src[1]->mod & NV_MOD_NEG)
      pc->emit[0] |= 1 << 8;
}

static void
emit_add_f32(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0x50000000;

   emit_form_0(pc, i);

   emit_neg_abs_1_2(pc, i);

   if (i->saturate)
      pc->emit[1] |= 1 << 17;
}

static void
emit_mul_f32(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0x58000000;

   emit_form_0(pc, i);

   if ((i->src[0]->mod ^ i->src[1]->mod) & NV_MOD_NEG)
      pc->emit[1] |= 1 << 25;

   if (i->saturate)
      pc->emit[0] |= 1 << 5;
}

static void
emit_mad_f32(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0x30000000;

   emit_form_0(pc, i);

   if ((i->src[0]->mod ^ i->src[1]->mod) & NV_MOD_NEG)
      pc->emit[0] |= 1 << 9;

   if (i->src[2]->mod & NV_MOD_NEG)
      pc->emit[0] |= 1 << 8;

   if (i->saturate)
      pc->emit[0] |= 1 << 5;
}

static void
emit_minmax(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0x08000000;

   if (NV_BASEOP(i->opcode) == NV_OP_MAX)
      pc->emit[1] |= 0x001e0000;
   else
      pc->emit[1] |= 0x000e0000; /* predicate ? */

   emit_form_0(pc, i);

   emit_neg_abs_1_2(pc, i);

   switch (i->opcode) {
   case NV_OP_MIN_U32:
   case NV_OP_MAX_U32:
      pc->emit[0] |= 3;
      break;
   case NV_OP_MIN_S32:
   case NV_OP_MAX_S32:
      pc->emit[0] |= 3 | (1 << 5);
      break;
   case NV_OP_MIN_F32:
   case NV_OP_MAX_F32:
   default:
      break;
   }
}

static void
emit_tex(struct nv_pc *pc, struct nv_instruction *i)
{
   int src1 = i->tex_array + i->tex_dim + i->tex_cube;

   assert(src1 < 6);

   pc->emit[0] = 0x00000086;
   pc->emit[1] = 0x80000000;

   switch (i->opcode) {
   case NV_OP_TEX: pc->emit[1] = 0x80000000; break;
   case NV_OP_TXB: pc->emit[1] = 0x84000000; break;
   case NV_OP_TXL: pc->emit[1] = 0x86000000; break;
   case NV_OP_TXF: pc->emit[1] = 0x90000000; break;
   case NV_OP_TXG: pc->emit[1] = 0xe0000000; break;
   default:
      assert(0);
      break;
   }

   if (i->tex_array)
      pc->emit[1] |= 0x00080000; /* layer index is u16, first value of SRC0 */
   if (i->tex_shadow)
      pc->emit[1] |= 0x01000000; /* shadow is part of SRC1, after bias/lod */

   set_pred(pc, i);

   DID(pc, i->def[0], 14);
   SID(pc, i->src[0], 20);
   SID(pc, i->src[src1], 26); /* may be NULL -> $r63 */

   pc->emit[1] |= i->tex_mask << 14;
   pc->emit[1] |= (i->tex_dim - 1) << 20;
   if (i->tex_cube)
      pc->emit[1] |= 3 << 20;

   assert(i->ext.tex.s < 16);

   pc->emit[1] |= i->ext.tex.t;
   pc->emit[1] |= i->ext.tex.s << 8;

   if (i->tex_live)
      pc->emit[0] |= 1 << 9;
}

/* 0: cos, 1: sin, 2: ex2, 3: lg2, 4: rcp, 5: rsqrt */
static void
emit_flop(struct nv_pc *pc, struct nv_instruction *i, ubyte op)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0xc8000000;

   set_pred(pc, i);

   DID(pc, i->def[0], 14);
   SID(pc, i->src[0], 20);

   pc->emit[0] |= op << 26;

   if (op >= 3) {
      if (i->src[0]->mod & NV_MOD_NEG) pc->emit[0] |= 1 << 9;
      if (i->src[0]->mod & NV_MOD_ABS) pc->emit[0] |= 1 << 7;
   } else {
      assert(!i->src[0]->mod);
   }
}

static void
emit_quadop(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0x48000000;

   set_pred(pc, i);

   assert(SFILE(i, 0) == NV_FILE_GPR && SFILE(i, 1) == NV_FILE_GPR);

   DID(pc, i->def[0], 14);
   SID(pc, i->src[0], 20);
   SID(pc, i->src[0], 26);

   pc->emit[0] |= i->lanes << 6; /* l0, l1, l2, l3, dx, dy */
   pc->emit[1] |= i->quadop;
}

static void
emit_ddx(struct nv_pc *pc, struct nv_instruction *i)
{
   i->quadop = 0x99;
   i->lanes = 4;
   i->src[1] = i->src[0];
   emit_quadop(pc, i);
}

static void
emit_ddy(struct nv_pc *pc, struct nv_instruction *i)
{
   i->quadop = 0xa5;
   i->lanes = 5;
   i->src[1] = i->src[0];
   emit_quadop(pc, i);
}

/* preparation op (preex2, presin / convert to fixed point) */
static void
emit_preop(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0x60000000;

   if (i->opcode == NV_OP_PREEX2)
      pc->emit[0] |= 0x20;

   emit_form_1(pc, i);

   if (i->src[0]->mod & NV_MOD_NEG) pc->emit[0] |= 1 << 8;
   if (i->src[0]->mod & NV_MOD_ABS) pc->emit[0] |= 1 << 6;
}

static void
emit_shift(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000003;

   switch (i->opcode) {
   case NV_OP_SAR:
      pc->emit[0] |= 0x20; /* fall through */
   case NV_OP_SHR:
      pc->emit[1] = 0x58000000;
      break;
   case NV_OP_SHL:
   default:
      pc->emit[1] = 0x60000000;
      break;
   }

   emit_form_0(pc, i);
}

static void
emit_bitop(struct nv_pc *pc, struct nv_instruction *i)
{
   if (SFILE(i, 1) == NV_FILE_IMM) {
      pc->emit[0] = 0x00000002;
      pc->emit[1] = 0x38000000;
   } else {
      pc->emit[0] = 0x00000003;
      pc->emit[1] = 0x68000000;
   }
   
   switch (i->opcode) {
   case NV_OP_OR:
      pc->emit[0] |= 0x40;
      break;
   case NV_OP_XOR:
      pc->emit[0] |= 0x80;
      break;
   case NV_OP_AND:
   default:
      break;
   }

   emit_form_0(pc, i);
}

static void
emit_set(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;

   switch (i->opcode) {
   case NV_OP_SET_S32:
      pc->emit[0] |= 0x20; /* fall through */
   case NV_OP_SET_U32:
      pc->emit[0] |= 0x3;
      pc->emit[1] = 0x100e0000;
      break;
   case NV_OP_SET_F32_AND:
      pc->emit[1] = 0x18000000;
      break;
   case NV_OP_SET_F32_OR:
      pc->emit[1] = 0x18200000;
      break;
   case NV_OP_SET_F32_XOR:
      pc->emit[1] = 0x18400000;
      break;
   case NV_OP_FSET_F32:
      pc->emit[0] |= 0x20; /* fall through */
   case NV_OP_SET_F32:
   default:
      pc->emit[1] = 0x180e0000;
      break;
   }

   if (DFILE(i, 0) == NV_FILE_PRED) {
      pc->emit[0] |= 0x1c000;
      pc->emit[1] += 0x08000000;
   }

   pc->emit[1] |= i->set_cond << 23;

   emit_form_0(pc, i);

   emit_neg_abs_1_2(pc, i); /* maybe assert that U/S32 don't use mods */
}

static void
emit_selp(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000004;
   pc->emit[1] = 0x20000000;

   emit_form_0(pc, i);

   if (i->cc || (i->src[2]->mod & NV_MOD_NOT))
      pc->emit[1] |= 1 << 20;
}

static void
emit_slct(struct nv_pc *pc, struct nv_instruction *i)
{
   uint8_t cc = i->set_cond;

   pc->emit[0] = 0x00000000;

   switch (i->opcode) {
   case NV_OP_SLCT_S32:
      pc->emit[0] |= 0x20; /* fall through */
   case NV_OP_SLCT_U32:
      pc->emit[0] |= 0x3;
      pc->emit[1] = 0x30000000;
      break;
   case NV_OP_SLCT_F32:
   default:
      pc->emit[1] = 0x38000000;
      break;
   }

   emit_form_0(pc, i);

   if (i->src[2]->mod & NV_MOD_NEG)
      cc = nvc0_ir_reverse_cc(cc);

   pc->emit[1] |= cc << 23;
}

static void
emit_cvt(struct nv_pc *pc, struct nv_instruction *i)
{
   uint32_t rint;

   pc->emit[0] = 0x00000004;
   pc->emit[1] = 0x10000000;

   /* if no type conversion specified, get type from opcode */
   if (i->opcode != NV_OP_CVT && i->ext.cvt.d == i->ext.cvt.s)
      i->ext.cvt.d = i->ext.cvt.s = NV_OPTYPE(i->opcode);

   switch (i->ext.cvt.d) {
   case NV_TYPE_F32:
      switch (i->ext.cvt.s) {
      case NV_TYPE_F32: pc->emit[1] = 0x10000000; break;
      case NV_TYPE_S32: pc->emit[0] |= 0x200; /* fall through */
      case NV_TYPE_U32: pc->emit[1] = 0x18000000; break;
      }
      break;
   case NV_TYPE_S32: pc->emit[0] |= 0x80; /* fall through */
   case NV_TYPE_U32:
      switch (i->ext.cvt.s) {
      case NV_TYPE_F32: pc->emit[1] = 0x14000000; break;
      case NV_TYPE_S32: pc->emit[0] |= 0x200; /* fall through */
      case NV_TYPE_U32: pc->emit[1] = 0x1c000000; break;
      }
      break;
   default:
      assert(!"cvt: unknown type");
      break;
   }

   rint = (i->ext.cvt.d == NV_TYPE_F32) ? 1 << 7 : 0;

   if (i->opcode == NV_OP_FLOOR) {
      pc->emit[0] |= rint;
      pc->emit[1] |= 2 << 16;
   } else
   if (i->opcode == NV_OP_CEIL) {
      pc->emit[0] |= rint;
      pc->emit[1] |= 4 << 16;
   } else
   if (i->opcode == NV_OP_TRUNC) {
      pc->emit[0] |= rint;
      pc->emit[1] |= 6 << 16;
   }

   if (i->saturate || i->opcode == NV_OP_SAT)
      pc->emit[0] |= 0x20;

   if (NV_BASEOP(i->opcode) == NV_OP_ABS || i->src[0]->mod & NV_MOD_ABS)
      pc->emit[0] |= 1 << 6;
   if (NV_BASEOP(i->opcode) == NV_OP_NEG || i->src[0]->mod & NV_MOD_NEG)
      pc->emit[0] |= 1 << 8;

   pc->emit[0] |= util_logbase2(DREG(i->def[0])->size) << 20;
   pc->emit[0] |= util_logbase2(SREG(i->src[0])->size) << 23;

   emit_form_1(pc, i);
}

static void
emit_interp(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000000;
   pc->emit[1] = 0xc07e0000;

   DID(pc, i->def[0], 14);

   set_pred(pc, i);

   if (i->indirect)
      SID(pc, i->src[i->indirect], 20);
   else
      SID(pc, NULL, 20);

   if (i->opcode == NV_OP_PINTERP) {
      pc->emit[0] |= 0x040;
      SID(pc, i->src[1], 26);

      if (i->src[0]->value->reg.address >= 0x280 &&
          i->src[0]->value->reg.address <= 0x29c)
         pc->emit[0] |= 0x080; /* XXX: ? */
   } else {
      SID(pc, NULL, 26);
   }

   pc->emit[1] |= i->src[0]->value->reg.address & 0xffff;

   if (i->centroid)
      pc->emit[0] |= 0x100;
   else
   if (i->flat)
      pc->emit[0] |= 0x080;
}

static void
emit_vfetch(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x03f00006;
   pc->emit[1] = 0x06000000 | i->src[0]->value->reg.address;
   if (i->patch)
      pc->emit[0] |= 0x100;

   set_pred(pc, i);

   DVS(pc, i);
   DID(pc, i->def[0], 14);

   SID(pc, (i->indirect >= 0) ? i->src[i->indirect] : NULL, 26);
}

static void
emit_export(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000006;
   pc->emit[1] = 0x0a000000;
   if (i->patch)
      pc->emit[0] |= 0x100;

   set_pred(pc, i);

   assert(SFILE(i, 0) == NV_FILE_MEM_V);
   assert(SFILE(i, 1) == NV_FILE_GPR);

   SID(pc, i->src[1], 26); /* register source */
   SVS(pc, i->src[0]);

   pc->emit[1] |= i->src[0]->value->reg.address & 0xfff;

   SID(pc, (i->indirect >= 0) ? i->src[i->indirect] : NULL, 20);
}

static void
emit_mov(struct nv_pc *pc, struct nv_instruction *i)
{
   if (i->opcode == NV_OP_MOV)
      i->lanes = 0xf;

   if (SFILE(i, 0) == NV_FILE_IMM) {
      pc->emit[0] = 0x000001e2;
      pc->emit[1] = 0x18000000;
   } else
   if (SFILE(i, 0) == NV_FILE_PRED) {
      pc->emit[0] = 0x1c000004;
      pc->emit[1] = 0x080e0000;
   } else {
      pc->emit[0] = 0x00000004 | (i->lanes << 5);
      pc->emit[1] = 0x28000000;
   }

   emit_form_1(pc, i);
}

static void
emit_ldst_size(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(NV_IS_MEMORY_FILE(SFILE(i, 0)));

   switch (SSIZE(i, 0)) {
   case 1:
      if (NV_TYPE_ISSGD(i->ext.cvt.s))
         pc->emit[0] |= 0x20;
      break;
   case 2:
      pc->emit[0] |= 0x40;
      if (NV_TYPE_ISSGD(i->ext.cvt.s))
         pc->emit[0] |= 0x20;
      break;
   case 4: pc->emit[0] |= 0x80; break;
   case 8: pc->emit[0] |= 0xa0; break;
   case 16: pc->emit[0] |= 0xc0; break;
   default:
      NOUVEAU_ERR("invalid load/store size %u\n", SSIZE(i, 0));
      break;
   }
}

static void
emit_ld_common(struct nv_pc *pc, struct nv_instruction *i)
{
   emit_ldst_size(pc, i);

   set_pred(pc, i);
   set_address_16(pc, i->src[0]);

   SID(pc, (i->indirect >= 0) ? i->src[i->indirect] : NULL, 20);
   DID(pc, i->def[0], 14);
}

static void
emit_ld_const(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x00000006;
   pc->emit[1] = 0x14000000 | (const_space_index(i, 0) << 10);

   emit_ld_common(pc, i);
}

static void
emit_ld(struct nv_pc *pc, struct nv_instruction *i)
{
   if (SFILE(i, 0) >= NV_FILE_MEM_C(0) &&
       SFILE(i, 0) <= NV_FILE_MEM_C(15)) {
      if (SSIZE(i, 0) == 4 && i->indirect < 0) {
         i->lanes = 0xf;
         emit_mov(pc, i);
      } else {
         emit_ld_const(pc, i);
      }
   } else
   if (SFILE(i, 0) == NV_FILE_MEM_L) {
      pc->emit[0] = 0x00000005;
      pc->emit[1] = 0xc0000000;

      emit_ld_common(pc, i);
   } else {
      NOUVEAU_ERR("emit_ld(%u): not handled yet\n", SFILE(i, 0));
      abort();
   }
}

static void
emit_st(struct nv_pc *pc, struct nv_instruction *i)
{
   if (SFILE(i, 0) != NV_FILE_MEM_L)
      NOUVEAU_ERR("emit_st(%u): file not handled yet\n", SFILE(i, 0));

   pc->emit[0] = 0x00000005 | (0 << 8); /* write-back caching */
   pc->emit[1] = 0xc8000000;

   emit_ldst_size(pc, i);

   set_pred(pc, i);
   set_address_16(pc, i->src[0]);

   SID(pc, (i->indirect >= 0) ? i->src[i->indirect] : NULL, 20);
   DID(pc, i->src[1]->value, 14);
}

void
nvc0_emit_instruction(struct nv_pc *pc, struct nv_instruction *i)
{
#if NV50_DEBUG & NV50_DEBUG_SHADER
   debug_printf("EMIT: "); nvc0_print_instruction(i);
#endif

   switch (i->opcode) {
   case NV_OP_VFETCH:
      emit_vfetch(pc, i);
      break;
   case NV_OP_EXPORT:
      if (!pc->is_fragprog)
         emit_export(pc, i);
      break;
   case NV_OP_MOV:
      emit_mov(pc, i);
      break;
   case NV_OP_LD:
      emit_ld(pc, i);
      break;
   case NV_OP_ST:
      emit_st(pc, i);
      break;
   case NV_OP_LINTERP:
   case NV_OP_PINTERP:
      emit_interp(pc, i);
      break;
   case NV_OP_ADD_F32:
      emit_add_f32(pc, i);
      break;
   case NV_OP_AND:
   case NV_OP_OR:
   case NV_OP_XOR:
      emit_bitop(pc, i);
      break;
   case NV_OP_CVT:
   case NV_OP_ABS_F32:
   case NV_OP_ABS_S32:
   case NV_OP_NEG_F32:
   case NV_OP_NEG_S32:
   case NV_OP_SAT:
   case NV_OP_CEIL:
   case NV_OP_FLOOR:
   case NV_OP_TRUNC:
      emit_cvt(pc, i);
      break;
   case NV_OP_DFDX:
      emit_ddx(pc, i);
      break;
   case NV_OP_DFDY:
      emit_ddy(pc, i);
      break;
   case NV_OP_COS:
      emit_flop(pc, i, 0);
      break;
   case NV_OP_SIN:
      emit_flop(pc, i, 1);
      break;
   case NV_OP_EX2:
      emit_flop(pc, i, 2);
      break;
   case NV_OP_LG2:
      emit_flop(pc, i, 3);
      break;
   case NV_OP_RCP:
      emit_flop(pc, i, 4);
      break;
   case NV_OP_RSQ:
      emit_flop(pc, i, 5);
      break;
   case NV_OP_PRESIN:
   case NV_OP_PREEX2:
      emit_preop(pc, i);
      break;
   case NV_OP_MAD_F32:
      emit_mad_f32(pc, i);
      break;
   case NV_OP_MAX_F32:
   case NV_OP_MAX_S32:
   case NV_OP_MAX_U32:
   case NV_OP_MIN_F32:
   case NV_OP_MIN_S32:
   case NV_OP_MIN_U32:
      emit_minmax(pc, i);
      break;
   case NV_OP_MUL_F32:
      emit_mul_f32(pc, i);
      break;
   case NV_OP_SET_F32:
   case NV_OP_SET_F32_AND:
   case NV_OP_SET_F32_OR:
   case NV_OP_SET_F32_XOR:
   case NV_OP_SET_S32:
   case NV_OP_SET_U32:
   case NV_OP_FSET_F32:
      emit_set(pc, i);
      break;
   case NV_OP_SHL:
   case NV_OP_SHR:
   case NV_OP_SAR:
      emit_shift(pc, i);
      break;
   case NV_OP_TEX:
   case NV_OP_TXB:
   case NV_OP_TXL:
      emit_tex(pc, i);
      break;
   case NV_OP_BRA:
      emit_flow(pc, i, 0x40);
      break;
   case NV_OP_CALL:
      emit_flow(pc, i, 0x50);
      break;
   case NV_OP_JOINAT:
      emit_flow(pc, i, 0x60);
      break;
   case NV_OP_EXIT:
      emit_flow(pc, i, 0x80);
      break;
   case NV_OP_RET:
      emit_flow(pc, i, 0x90);
      break;
   case NV_OP_KIL:
      emit_flow(pc, i, 0x98);
      break;
   case NV_OP_JOIN:
   case NV_OP_NOP:
      pc->emit[0] = 0x00003de4;
      pc->emit[1] = 0x40000000;
      break;
   case NV_OP_SELP:
      emit_selp(pc, i);
      break;
   case NV_OP_SLCT_F32:
   case NV_OP_SLCT_S32:
   case NV_OP_SLCT_U32:
      emit_slct(pc, i);
      break;
   default:
      NOUVEAU_ERR("unhandled NV_OP: %d\n", i->opcode);
      abort();
      break;
   }

   if (i->join)
      pc->emit[0] |= 0x10;
}
