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

#include "nv50_context.h"
#include "nv50_pc.h"

#define FLAGS_CC_SHIFT    7
#define FLAGS_ID_SHIFT    12
#define FLAGS_WR_ID_SHIFT 4
#define FLAGS_CC_MASK     (0x1f << FLAGS_CC_SHIFT)
#define FLAGS_ID_MASK     (0x03 << FLAGS_ID_SHIFT)
#define FLAGS_WR_EN       (1 << 6)
#define FLAGS_WR_ID_MASK  (0x3 << FLAGS_WR_ID_SHIFT)

#define NV50_FIXUP_CODE_RELOC 0
#define NV50_FIXUP_DATA_RELOC 1

struct nv50_fixup {
   uint8_t type;
   int8_t shift;
   uint32_t mask;
   uint32_t data;
   uint32_t offset;
};

void
nv50_relocate_program(struct nv50_program *p,
                      uint32_t code_base,
                      uint32_t data_base)
{
   struct nv50_fixup *f = (struct nv50_fixup *)p->fixups;
   unsigned i;

   for (i = 0; i < p->num_fixups; ++i) {
      uint32_t data;

      switch (f[i].type) {
      case NV50_FIXUP_CODE_RELOC: data = code_base + f[i].data; break;
      case NV50_FIXUP_DATA_RELOC: data = data_base + f[i].data; break;
      default:
         data = f[i].data;
         break;
      }
      data = (f[i].shift < 0) ? (data >> -f[i].shift) : (data << f[i].shift);

      p->code[f[i].offset / 4] &= ~f[i].mask;
      p->code[f[i].offset / 4] |= data & f[i].mask;
   }
}

static void
new_fixup(struct nv_pc *pc, uint8_t ty, int w, uint32_t data, uint32_t m, int s)
{
   struct nv50_fixup *f;

   const unsigned size = sizeof(struct nv50_fixup);
   const unsigned n = pc->num_fixups;

   if (!(n % 8))
      pc->fixups = REALLOC(pc->fixups, n * size, (n + 8) * size);

   f = (struct nv50_fixup *)pc->fixups;

   f[n].offset = (pc->bin_pos + w) * 4;
   f[n].type = ty;
   f[n].data = data;
   f[n].mask = m;
   f[n].shift = s;

   ++pc->num_fixups;
}

const ubyte nv50_inst_min_size_tab[NV_OP_COUNT] =
{
   0, 0, 0, 8, 8, 4, 4, 4, 8, 4, 4, 8, 8, 8, 8, 8, /* 15 */
   8, 8, 8, 4, 0, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, /* 31 */
   8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 4, /* 47 */
   4, 8, 8, 8, 8, 8, 0, 0, 8
};

unsigned
nv50_inst_min_size(struct nv_instruction *i)
{
   int n;

   if (nv50_inst_min_size_tab[i->opcode] > 4)
      return 8;

   if (i->def[0] && i->def[0]->reg.file != NV_FILE_GPR)
      return 8;
   if (i->def[0]->join->reg.id > 63)
      return 8;

   for (n = 0; n < 3; ++n) {
      if (!i->src[n])
         break;
      if (i->src[n]->value->reg.file != NV_FILE_GPR &&
          i->src[n]->value->reg.file != NV_FILE_MEM_V)
         return 8;
      if (i->src[n]->value->reg.id > 63)
         return 8;
   }

   if (i->flags_def || i->flags_src || i->src[4])
      return 8;

   if (i->is_join)
      return 8;

   if (i->src[2]) {
      if (i->saturate || i->src[2]->mod)
         return 8;
      if (i->src[0]->mod ^ i->src[1]->mod)
         return 8;
      if ((i->src[0]->mod | i->src[1]->mod) & NV_MOD_ABS)
         return 8;
      if (i->def[0]->join->reg.id < 0 ||
          i->def[0]->join->reg.id != i->src[2]->value->join->reg.id)
         return 8;
   }

   return nv50_inst_min_size_tab[i->opcode];
}

static INLINE ubyte
STYPE(struct nv_instruction *nvi, int s)
{
   return nvi->src[s]->typecast;
}

static INLINE ubyte
DTYPE(struct nv_instruction *nvi, int d)
{
   return nvi->def[d]->reg.type;
}

static INLINE struct nv_reg *
SREG(struct nv_ref *ref)
{
   return &ref->value->join->reg;
}

static INLINE struct nv_reg *
DREG(struct nv_value *val)
{
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
   pc->emit[pos / 32] |= SREG(ref)->id << (pos % 32);
}

static INLINE void
DID(struct nv_pc *pc, struct nv_value *val, int pos)
{
   pc->emit[pos / 32] |= DREG(val)->id << (pos % 32);
}

static INLINE uint32_t
get_immd_u32(struct nv_ref *ref)
{
   assert(ref->value->reg.file == NV_FILE_IMM);
   return ref->value->reg.imm.u32;
}

static INLINE void
set_immd_u32(struct nv_pc *pc, uint32_t u32)
{
   pc->emit[1] |= 3;
   pc->emit[0] |= (u32 & 0x3f) << 16;
   pc->emit[1] |= (u32 >> 6) << 2;
}

static INLINE void
set_immd(struct nv_pc *pc, struct nv_ref *ref)
{
   assert(ref->value->reg.file == NV_FILE_IMM);
   set_immd_u32(pc, get_immd_u32(ref));
}

/* Allocate data in immediate buffer, if we want to load the immediate
 * for a constant buffer instead of inlining it into the code.
 */
static void
nv_pc_alloc_immd(struct nv_pc *pc, struct nv_ref *ref)
{
   uint32_t i, val = get_immd_u32(ref);

   for (i = 0; i < pc->immd_count; ++i)
      if (pc->immd_buf[i] == val)
         break;

   if (i == pc->immd_count) {
      if (!(pc->immd_count % 8))
         pc->immd_buf = REALLOC(pc->immd_buf,
				pc->immd_count * 4, (pc->immd_count + 8) * 4);
      pc->immd_buf[pc->immd_count++] = val;
   }

   SREG(ref)->id = i;
}

static INLINE void
set_pred(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(!(pc->emit[1] & 0x00003f80));

   pc->emit[1] |= i->cc << 7;
   if (i->flags_src)
      pc->emit[1] |= SREG(i->flags_src)->id << 12;
}

static INLINE void
set_pred_wr(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(!(pc->emit[1] & 0x00000070));

   if (i->flags_def)
      pc->emit[1] |= (DREG(i->flags_def)->id << 4) | 0x40;
}

static INLINE void
set_a16_bits(struct nv_pc *pc, uint id)
{
   ++id; /* $a0 is always 0 */
   pc->emit[0] |= (id & 3) << 26;
   pc->emit[1] |= id & 4;
}

static INLINE void
set_addr(struct nv_pc *pc, struct nv_instruction *i)
{
   if (i->src[4])
      set_a16_bits(pc, SREG(i->src[4])->id);
}

static void
set_dst(struct nv_pc *pc, struct nv_value *value)
{
   struct nv_reg *reg = &value->join->reg;

   if (reg->id < 0) {
      pc->emit[0] |= (127 << 2) | 1; /* set 'long'-bit to catch bugs */
      pc->emit[1] |= 0x8;
      return;
   }

   if (reg->file == NV_FILE_OUT)
      pc->emit[1] |= 0x8;
   else
   if (reg->file == NV_FILE_ADDR)
      assert(0);

   pc->emit[0] |= reg->id << 2;
}

static void
set_src_0(struct nv_pc *pc, struct nv_ref *ref)
{
   struct nv_reg *reg = SREG(ref);

   if (reg->file == NV_FILE_MEM_S)
      pc->emit[1] |= 0x00200000;
   else
   if (reg->file == NV_FILE_MEM_P)
      pc->emit[0] |= 0x01800000;
   else
   if (reg->file != NV_FILE_GPR)
      NOUVEAU_ERR("invalid src0 register file: %d\n", reg->file);

   assert(reg->id < 128);
   pc->emit[0] |= reg->id << 9;
}

static void
set_src_1(struct nv_pc *pc, struct nv_ref *ref)
{
   struct nv_reg *reg = SREG(ref);

   if (reg->file >= NV_FILE_MEM_C(0) &&
       reg->file <= NV_FILE_MEM_C(15)) {
      assert(!(pc->emit[1] & 0x01800000));

      pc->emit[0] |= 0x00800000;
      pc->emit[1] |= (reg->file - NV_FILE_MEM_C(0)) << 22;
   } else
   if (reg->file != NV_FILE_GPR)
      NOUVEAU_ERR("invalid src1 register file: %d\n", reg->file);

   assert(reg->id < 128);
   pc->emit[0] |= reg->id << 16;
}

static void
set_src_2(struct nv_pc *pc, struct nv_ref *ref)
{
   struct nv_reg *reg = SREG(ref);

   if (reg->file >= NV_FILE_MEM_C(0) &&
       reg->file <= NV_FILE_MEM_C(15)) {
      assert(!(pc->emit[1] & 0x01800000));

      pc->emit[0] |= 0x01000000;
      pc->emit[1] |= (reg->file - NV_FILE_MEM_C(0)) << 22;
   } else
   if (reg->file != NV_FILE_GPR)
      NOUVEAU_ERR("invalid src2 register file: %d\n", reg->file);

   assert(reg->id < 128);
   pc->emit[1] |= reg->id << 14;
}

/* the default form:
 * - long instruction
 * - 1 to 3 sources in slots 0, 1, 2
 * - address & flags
 */
static void
emit_form_MAD(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] |= 1;

   set_pred(pc, i);
   set_pred_wr(pc, i);

   if (i->def[0])
      set_dst(pc, i->def[0]);
   else {
      pc->emit[0] |= 0x01fc;
      pc->emit[1] |= 0x0008;
   }

   if (i->src[0])
      set_src_0(pc, i->src[0]);

   if (i->src[1])
      set_src_1(pc, i->src[1]);

   if (i->src[2])
      set_src_2(pc, i->src[2]);

   set_addr(pc, i);
}

/* like default form, but 2nd source in slot 2, no 3rd source */
static void
emit_form_ADD(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] |= 1;

   if (i->def[0])
      set_dst(pc, i->def[0]);
   else {
      pc->emit[0] |= 0x01fc;
      pc->emit[1] |= 0x0008;
   }

   set_pred(pc, i);
   set_pred_wr(pc, i);

   if (i->src[0])
      set_src_0(pc, i->src[0]);

   if (i->src[1])
      set_src_2(pc, i->src[1]);

   set_addr(pc, i);
}

/* short mul */
static void
emit_form_MUL(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(!i->is_long && !(pc->emit[0] & 1));

   assert(i->def[0]);
   set_dst(pc, i->def[0]);

   if (i->src[0])
      set_src_0(pc, i->src[0]);

   if (i->src[1])
      set_src_1(pc, i->src[1]);
}

/* default immediate form
 * - 1 to 3 sources where last is immediate
 * - no address or predicate possible
 */
static void
emit_form_IMM(struct nv_pc *pc, struct nv_instruction *i, ubyte mod_mask)
{
   pc->emit[0] |= 1;

   assert(i->def[0]);
   assert(i->src[0]);
   set_dst(pc, i->def[0]);

   assert(!i->src[4] && !i->flags_src && !i->flags_def);

   if (i->src[2]) {
      set_immd(pc, i->src[2]);
      set_src_0(pc, i->src[1]);
      set_src_1(pc, i->src[0]);
   } else
   if (i->src[1]) {
      set_immd(pc, i->src[1]);
      set_src_0(pc, i->src[0]);
   } else
      set_immd(pc, i->src[0]);

   assert(!mod_mask);
}

static void
set_ld_st_size(struct nv_pc *pc, int s, ubyte type)
{
   switch (type) {
   case NV_TYPE_F64:
      pc->emit[1] |= 0x8000 << s;
      break;
   case NV_TYPE_F32:
   case NV_TYPE_S32:
   case NV_TYPE_U32:
      pc->emit[1] |= 0xc000 << s;
      break;
   case NV_TYPE_S16:
      pc->emit[1] |= 0x6000 << s;
      break;
   case NV_TYPE_U16:
      pc->emit[1] |= 0x4000 << s;
      break;
   case NV_TYPE_S8:
      pc->emit[1] |= 0x2000 << s;
      break;
   default:
      break;
   }
}

static void
emit_ld(struct nv_pc *pc, struct nv_instruction *i)
{
   ubyte sf = SFILE(i, 0);

   if (sf == NV_FILE_IMM) {
      sf = NV_FILE_MEM_C(0);
      nv_pc_alloc_immd(pc, i->src[0]);

      new_fixup(pc, NV50_FIXUP_DATA_RELOC, 0, SREG(i->src[0])->id, 0xffff, 9);
   }

   if (sf == NV_FILE_MEM_S ||
       sf == NV_FILE_MEM_P) {
      pc->emit[0] = 0x10000001;
      pc->emit[1] = 0x04200000 | (0x3c << 12);
      if (sf == NV_FILE_MEM_P)
         pc->emit[0] |= 0x01800000;
   } else
   if (sf >= NV_FILE_MEM_C(0) &&
       sf <= NV_FILE_MEM_C(15)) {
      pc->emit[0] = 0x10000001;
      pc->emit[1] = 0x24000000;
      pc->emit[1] |= (sf - NV_FILE_MEM_C(0)) << 22;
   } else
   if (sf >= NV_FILE_MEM_G(0) &&
       sf <= NV_FILE_MEM_G(15)) {
      pc->emit[0] = 0xd0000001 | ((sf - NV_FILE_MEM_G(0)) << 16);
      pc->emit[1] = 0xa0000000;

      assert(i->src[4] && SREG(i->src[4])->file == NV_FILE_GPR);
      SID(pc, i->src[4], 9);
   } else
   if (sf == NV_FILE_MEM_L) {
      pc->emit[0] = 0xd0000001;
      pc->emit[1] = 0x40000000;

      set_addr(pc, i);
   } else {
      NOUVEAU_ERR("invalid ld source file\n");
      abort();
   }

   set_ld_st_size(pc, (sf == NV_FILE_MEM_L) ? 8 : 0, STYPE(i, 0));

   set_dst(pc, i->def[0]);
   set_pred_wr(pc, i);

   set_pred(pc, i);

   if (sf < NV_FILE_MEM_G(0) ||
       sf > NV_FILE_MEM_G(15)) {
      SID(pc, i->src[0], 9);
      set_addr(pc, i);
   }
}

static void
emit_st(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(SFILE(i, 1) == NV_FILE_GPR);
   assert(SFILE(i, 0) == NV_FILE_MEM_L);

   pc->emit[0] = 0xd0000001;
   pc->emit[1] = 0x60000000;

   SID(pc, i->src[1], 2);
   SID(pc, i->src[0], 9);

   set_ld_st_size(pc, 8, STYPE(i, 1));

   set_addr(pc, i);
   set_pred(pc, i);
}

static int
verify_mov(struct nv_instruction *i)
{
   ubyte sf = SFILE(i, 0);
   ubyte df = DFILE(i, 0);

   if (df == NV_FILE_GPR)
      return 0;

   if (df != NV_FILE_OUT &&
       df != NV_FILE_FLAGS &&
       df != NV_FILE_ADDR)
      return 1;

   if (sf == NV_FILE_FLAGS)
      return 2;
   if (sf == NV_FILE_ADDR)
      return 3;
   if (sf == NV_FILE_IMM && df != NV_FILE_OUT)
      return 4;

   return 0;
}

static void
emit_mov(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(!verify_mov(i));

   if (SFILE(i, 0) >= NV_FILE_MEM_S)
      emit_ld(pc, i);
   else
   if (SFILE(i, 0) == NV_FILE_FLAGS) {
      pc->emit[0] = 0x00000001 | (DREG(i->def[0])->id << 2);
      pc->emit[1] = 0x20000780 | (SREG(i->src[0])->id << 12);
   } else
   if (SFILE(i, 0) == NV_FILE_ADDR) {
      pc->emit[0] = 0x00000001 | (DREG(i->def[0])->id << 2);
      pc->emit[1] = 0x40000780;
      set_a16_bits(pc, SREG(i->src[0])->id);
   } else
   if (DFILE(i, 0) == NV_FILE_FLAGS) {
      pc->emit[0] = 0x00000001;
      pc->emit[1] = 0xa0000000 | (1 << 6);
      set_pred(pc, i);
      pc->emit[0] |= SREG(i->src[0])->id << 9;
      pc->emit[1] |= DREG(i->def[0])->id << 4;
   } else
   if (SFILE(i, 0) == NV_FILE_IMM) {
      if (i->opcode == NV_OP_LDA) {
         emit_ld(pc, i);
      } else {
         pc->emit[0] = 0x10008001;
         pc->emit[1] = 0x00000003;

         emit_form_IMM(pc, i, 0);
      }
   } else {
      pc->emit[0] = 0x10000000;
      pc->emit[0] |= DREG(i->def[0])->id << 2;
      pc->emit[0] |= SREG(i->src[0])->id << 9;

      if (!i->is_long) {
         pc->emit[0] |= 0x8000;
      } else {
         pc->emit[0] |= 0x00000001;
         pc->emit[1] = 0x0403c000;

         set_pred(pc, i);
      }
   }

   if (DFILE(i, 0) == NV_FILE_OUT)
      pc->emit[1] |= 0x8;
}

static void
emit_interp(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x80000000;

   assert(DFILE(i, 0) == NV_FILE_GPR);
   assert(SFILE(i, 0) == NV_FILE_MEM_V);

   DID(pc, i->def[0], 2);
   SID(pc, i->src[0], 16);

   if (i->flat)
      pc->emit[0] |= 1 << 8;
   else
   if (i->opcode == NV_OP_PINTERP) {
      pc->emit[0] |= 1 << 25;
      pc->emit[0] |= SREG(i->src[1])->id << 9;
   }

   if (i->centroid)
      pc->emit[0] |= 1 << 24;

   assert(i->is_long || !i->flags_src);

   if (i->is_long) {
      set_pred(pc, i);

      pc->emit[1] |=
	      (pc->emit[0] & (3 << 24)) >> (24 - 16) |
	      (pc->emit[0] & (1 <<  8)) >> (18 -  8);

      pc->emit[0] |= 1;
      pc->emit[0] &= ~0x03000100;
   }
}

static void
emit_minmax(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x30000000;
   pc->emit[1] = (i->opcode == NV_OP_MIN) ? (2 << 28) : 0;

   switch (DTYPE(i, 0)) {
   case NV_TYPE_F32:
      pc->emit[0] |= 0x80000000;
      pc->emit[1] |= 0x80000000;
      break;
   case NV_TYPE_S32:
      pc->emit[1] |= 0x8c000000;
      break;
   case NV_TYPE_U32:
      pc->emit[1] |= 0x84000000;
      break;
   }
	
   emit_form_MAD(pc, i);

   if (i->src[0]->mod & NV_MOD_ABS) pc->emit[1] |= 0x00100000;
   if (i->src[1]->mod & NV_MOD_ABS) pc->emit[1] |= 0x00080000;
}

static void
emit_add_f32(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0xb0000000;

   assert(!((i->src[0]->mod | i->src[1]->mod) & NV_MOD_ABS));

   if (SFILE(i, 1) == NV_FILE_IMM) {
      emit_form_IMM(pc, i, 0);

      if (i->src[0]->mod & NV_MOD_NEG) pc->emit[0] |= 0x8000;
      if (i->src[1]->mod & NV_MOD_NEG) pc->emit[0] |= 1 << 22;
   } else
   if (i->is_long) {
      emit_form_ADD(pc, i);

      if (i->src[0]->mod & NV_MOD_NEG) pc->emit[1] |= 1 << 26;
      if (i->src[1]->mod & NV_MOD_NEG) pc->emit[1] |= 1 << 27;

      if (i->saturate)
         pc->emit[1] |= 0x20000000;
   } else {
      emit_form_MUL(pc, i);

      if (i->src[0]->mod & NV_MOD_NEG) pc->emit[0] |= 0x8000;
      if (i->src[1]->mod & NV_MOD_NEG) pc->emit[0] |= 1 << 22;
   }
}

static void
emit_add_b32(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0x20008000;

   if (SFILE(i, 1) == NV_FILE_IMM) {
      emit_form_IMM(pc, i, 0);
   } else
   if (i->is_long) {
      pc->emit[0] = 0x20000000;
      pc->emit[1] = 0x04000000;
      emit_form_ADD(pc, i);
   } else {
      emit_form_MUL(pc, i);
   }

   if (i->src[0]->mod & NV_MOD_NEG) pc->emit[0] |= 1 << 28;
   if (i->src[1]->mod & NV_MOD_NEG) pc->emit[0] |= 1 << 22;
}

static void
emit_add_a16(struct nv_pc *pc, struct nv_instruction *i)
{
   int s = (i->opcode == NV_OP_MOV) ? 0 : 1;

   pc->emit[0] = 0xd0000001 | ((uint16_t)get_immd_u32(i->src[s]) << 9);
   pc->emit[1] = 0x20000000;

   pc->emit[0] |= (DREG(i->def[0])->id + 1) << 2;

   set_pred(pc, i);

   if (s && i->src[0])
      set_a16_bits(pc, SREG(i->src[0])->id);
}

static void
emit_flow(struct nv_pc *pc, struct nv_instruction *i, ubyte flow_op)
{
   pc->emit[0] = 0x00000003 | (flow_op << 28);
   pc->emit[1] = 0x00000000;

   set_pred(pc, i);

   if (i->target && (i->opcode != NV_OP_BREAK)) {
      uint32_t pos = i->target->bin_pos;

      new_fixup(pc, NV50_FIXUP_CODE_RELOC, 0, pos, 0xffff << 11, 9);
      new_fixup(pc, NV50_FIXUP_CODE_RELOC, 1, pos, 0x3f << 14, -4);

      pc->emit[0] |= ((pos >>  2) & 0xffff) << 11;
      pc->emit[1] |= ((pos >> 18) & 0x003f) << 14;
   }
}

static INLINE void
emit_add(struct nv_pc *pc, struct nv_instruction *i)
{
   if (DFILE(i, 0) == NV_FILE_ADDR)
      emit_add_a16(pc, i);
   else {
      switch (DTYPE(i, 0)) {
      case NV_TYPE_F32:
         emit_add_f32(pc, i);
         break;
      case NV_TYPE_U32:
      case NV_TYPE_S32:
         emit_add_b32(pc, i);
         break;
      }
   }
}

static void
emit_bitop2(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0xd0000000;

   if (SFILE(i, 1) == NV_FILE_IMM) {
      emit_form_IMM(pc, i, 0);

      if (i->opcode == NV_OP_OR)
         pc->emit[0] |= 0x0100;
      else
      if (i->opcode == NV_OP_XOR)
         pc->emit[0] |= 0x8000;
   } else {
      emit_form_MAD(pc, i);

      pc->emit[1] |= 0x04000000;

      if (i->opcode == NV_OP_OR)
         pc->emit[1] |= 0x4000;
      else
      if (i->opcode == NV_OP_XOR)
         pc->emit[1] |= 0x8000;
   }
}

static void
emit_arl(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(SFILE(i, 0) == NV_FILE_GPR);
   assert(SFILE(i, 1) == NV_FILE_IMM);

   assert(!i->flags_def);

   pc->emit[0] = 0x00000001;
   pc->emit[1] = 0xc0000000;

   pc->emit[0] |= (i->def[0]->reg.id + 1) << 2;
   set_pred(pc, i);
   set_src_0(pc, i->src[0]);
   pc->emit[0] |= (get_immd_u32(i->src[1]) & 0x3f) << 16;
}

static void
emit_shift(struct nv_pc *pc, struct nv_instruction *i)
{
   if (DFILE(i, 0) == NV_FILE_ADDR) {
      emit_arl(pc, i);
      return;
   }

   pc->emit[0] = 0x30000001;
   pc->emit[1] = 0xc4000000;

   if (i->opcode == NV_OP_SHR)
      pc->emit[1] |= 1 << 29;

   if (SFILE(i, 1) == NV_FILE_IMM) {
      pc->emit[1] |= 1 << 20;
      pc->emit[0] |= (get_immd_u32(i->src[1]) & 0x7f) << 16;

      set_pred(pc, i);
   } else
      emit_form_MAD(pc, i);

   if (STYPE(i, 0) == NV_TYPE_S32)
      pc->emit[1] |= 1 << 27;
}

static void
emit_flop(struct nv_pc *pc, struct nv_instruction *i)
{
   struct nv_ref *src0 = i->src[0];

   pc->emit[0] = 0x90000000;

   assert(STYPE(i, 0) == NV_TYPE_F32);
   assert(SFILE(i, 0) == NV_FILE_GPR);

   if (!i->is_long) {
      emit_form_MUL(pc, i);
      assert(i->opcode == NV_OP_RCP && !src0->mod);
      return;
   }

   pc->emit[1] = (i->opcode - NV_OP_RCP) << 29;

   emit_form_MAD(pc, i);

   if (src0->mod & NV_MOD_NEG) pc->emit[1] |= 0x04000000;
   if (src0->mod & NV_MOD_ABS) pc->emit[1] |= 0x00100000;
}

static void
emit_mad_f32(struct nv_pc *pc, struct nv_instruction *i)
{
   const boolean neg_mul = (i->src[0]->mod ^ i->src[1]->mod) & NV_MOD_NEG;
   const boolean neg_add = (i->src[2]->mod & NV_MOD_NEG);

   pc->emit[0] = 0xe0000000;

   if (!i->is_long) {
      emit_form_MUL(pc, i);
      assert(!neg_mul && !neg_add);
      return;
   }

   emit_form_MAD(pc, i);

   if (neg_mul) pc->emit[1] |= 0x04000000;
   if (neg_add) pc->emit[1] |= 0x08000000;

   if (i->saturate)
      pc->emit[1] |= 0x20000000;
}

static INLINE void
emit_mad(struct nv_pc *pc, struct nv_instruction *i)
{
   emit_mad_f32(pc, i);
}

static void
emit_mul_f32(struct nv_pc *pc, struct nv_instruction *i)
{
   boolean neg = (i->src[0]->mod ^ i->src[1]->mod) & NV_MOD_NEG;

   pc->emit[0] = 0xc0000000;

   if (SFILE(i, 1) == NV_FILE_IMM) {
      emit_form_IMM(pc, i, 0);

      if (neg)
         pc->emit[0] |= 0x8000;
   } else
   if (i->is_long) {
      emit_form_MAD(pc, i);

      if (neg)
         pc->emit[1] |= 0x08 << 24;
   } else {
      emit_form_MUL(pc, i);

      if (neg)
         pc->emit[0] |= 0x8000;
   }
}

static void
emit_set(struct nv_pc *pc, struct nv_instruction *nvi)
{
   assert(nvi->is_long);

   pc->emit[0] = 0x30000000;
   pc->emit[1] = 0x60000000;

   pc->emit[1] |= nvi->set_cond << 14;

   switch (STYPE(nvi, 0)) {
   case NV_TYPE_U32: pc->emit[1] |= 0x04000000; break;
   case NV_TYPE_S32: pc->emit[1] |= 0x0c000000; break;
   case NV_TYPE_F32: pc->emit[0] |= 0x80000000; break;
   default:
      assert(0);
      break;
   }

   emit_form_MAD(pc, nvi);
}

#define CVT_RN    (0x00 << 16)
#define CVT_FLOOR (0x02 << 16)
#define CVT_CEIL  (0x04 << 16)
#define CVT_TRUNC (0x06 << 16)
#define CVT_SAT   (0x08 << 16)
#define CVT_ABS   (0x10 << 16)

#define CVT_X32_X32 0x04004000
#define CVT_X32_S32 0x04014000
#define CVT_F32_F32 ((0xc0 << 24) | CVT_X32_X32)
#define CVT_S32_F32 ((0x88 << 24) | CVT_X32_X32)
#define CVT_U32_F32 ((0x80 << 24) | CVT_X32_X32)
#define CVT_F32_S32 ((0x40 << 24) | CVT_X32_S32)
#define CVT_F32_U32 ((0x40 << 24) | CVT_X32_X32)
#define CVT_S32_S32 ((0x08 << 24) | CVT_X32_S32)
#define CVT_S32_U32 ((0x08 << 24) | CVT_X32_X32)
#define CVT_U32_S32 ((0x00 << 24) | CVT_X32_S32)
#define CVT_U32_U32 ((0x00 << 24) | CVT_X32_X32)

#define CVT_NEG 0x20000000
#define CVT_RI  0x08000000

static void
emit_cvt(struct nv_pc *pc, struct nv_instruction *nvi)
{
   ubyte dst_type = nvi->def[0] ? DTYPE(nvi, 0) : STYPE(nvi, 0);

   pc->emit[0] = 0xa0000000;

   switch (dst_type) {
   case NV_TYPE_F32:
      switch (STYPE(nvi, 0)) {
      case NV_TYPE_F32: pc->emit[1] = CVT_F32_F32; break;
      case NV_TYPE_S32: pc->emit[1] = CVT_F32_S32; break;
      case NV_TYPE_U32: pc->emit[1] = CVT_F32_U32; break;
      }
      break;
   case NV_TYPE_S32:
      switch (STYPE(nvi, 0)) {
      case NV_TYPE_F32: pc->emit[1] = CVT_S32_F32; break;
      case NV_TYPE_S32: pc->emit[1] = CVT_S32_S32; break;
      case NV_TYPE_U32: pc->emit[1] = CVT_S32_U32; break;
      }
      break;
   case NV_TYPE_U32:
      switch (STYPE(nvi, 0)) {
      case NV_TYPE_F32: pc->emit[1] = CVT_U32_F32; break;
      case NV_TYPE_S32: pc->emit[1] = CVT_U32_S32; break;
      case NV_TYPE_U32: pc->emit[1] = CVT_U32_U32; break;
      }
      break;
   }
   if (pc->emit[1] == CVT_F32_F32 &&
       (nvi->opcode == NV_OP_CEIL || nvi->opcode == NV_OP_FLOOR ||
	nvi->opcode == NV_OP_TRUNC))
       pc->emit[1] |= CVT_RI;

   switch (nvi->opcode) {
   case NV_OP_CEIL:  pc->emit[1] |= CVT_CEIL; break;
   case NV_OP_FLOOR: pc->emit[1] |= CVT_FLOOR; break;
   case NV_OP_TRUNC: pc->emit[1] |= CVT_TRUNC; break;

   case NV_OP_ABS: pc->emit[1] |= CVT_ABS; break;
   case NV_OP_SAT: pc->emit[1] |= CVT_SAT; break;
   case NV_OP_NEG: pc->emit[1] |= CVT_NEG; break;
   default:
      assert(nvi->opcode == NV_OP_CVT);
      break;
   }
   assert(nvi->opcode != NV_OP_ABS || !(nvi->src[0]->mod & NV_MOD_NEG));

   if (nvi->src[0]->mod & NV_MOD_NEG) pc->emit[1] ^= CVT_NEG;
   if (nvi->src[0]->mod & NV_MOD_ABS) pc->emit[1] |= CVT_ABS;

   emit_form_MAD(pc, nvi);
}

static void
emit_tex(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0xf0000001;
   pc->emit[1] = 0x00000000;

   DID(pc, i->def[0], 2);

   set_pred(pc, i);

   pc->emit[0] |= i->tex_t << 9;
   pc->emit[0] |= i->tex_s << 17;

   pc->emit[0] |= (i->tex_argc - 1) << 22;

   pc->emit[0] |= (i->tex_mask & 0x3) << 25;
   pc->emit[1] |= (i->tex_mask & 0xc) << 12;

   if (i->tex_live)
      pc->emit[1] |= 4;

   if (i->tex_cube)
      pc->emit[0] |= 0x08000000;

   if (i->opcode == NV_OP_TXB)
      pc->emit[1] |= 0x20000000;
   else
   if (i->opcode == NV_OP_TXL)
      pc->emit[1] |= 0x40000000;
}

static void
emit_cvt2fixed(struct nv_pc *pc, struct nv_instruction *i)
{
   ubyte mod = i->src[0]->mod;

   pc->emit[0] = 0xb0000000;
   pc->emit[1] = 0xc0000000;

   if (i->opcode == NV_OP_PREEX2)
      pc->emit[1] |= 0x4000;

   emit_form_MAD(pc, i);

   if (mod & NV_MOD_NEG) pc->emit[1] |= 0x04000000;
   if (mod & NV_MOD_ABS) pc->emit[1] |= 0x00100000;
}

static void
emit_ddx(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(i->is_long && SFILE(i, 0) == NV_FILE_GPR);

   pc->emit[0] = (i->src[0]->mod & NV_MOD_NEG) ? 0xc0240001 : 0xc0140001;
   pc->emit[1] = (i->src[0]->mod & NV_MOD_NEG) ? 0x86400000 : 0x89800000;

   DID(pc, i->def[0], 2);
   SID(pc, i->src[0], 9);
   SID(pc, i->src[0], 32 + 14);

   set_pred(pc, i);
   set_pred_wr(pc, i);
}

static void
emit_ddy(struct nv_pc *pc, struct nv_instruction *i)
{
   assert(i->is_long && SFILE(i, 0) == NV_FILE_GPR);

   pc->emit[0] = (i->src[0]->mod & NV_MOD_NEG) ? 0xc0250001 : 0xc0150001;
   pc->emit[1] = (i->src[0]->mod & NV_MOD_NEG) ? 0x85800000 : 0x8a400000;

   DID(pc, i->def[0], 2);
   SID(pc, i->src[0], 9);
   SID(pc, i->src[0], 32 + 14);

   set_pred(pc, i);
   set_pred_wr(pc, i);
}

static void
emit_quadop(struct nv_pc *pc, struct nv_instruction *i)
{
   pc->emit[0] = 0xc0000000;
   pc->emit[1] = 0x80000000;

   emit_form_ADD(pc, i);

   pc->emit[0] |= i->lanes << 16;

   pc->emit[0] |= (i->quadop & 0x03) << 20;
   pc->emit[1] |= (i->quadop & 0xfc) << 20;
}

void
nv50_emit_instruction(struct nv_pc *pc, struct nv_instruction *i)
{
   /* nv_print_instruction(i); */

   switch (i->opcode) {
   case NV_OP_MOV:
      if (DFILE(i, 0) == NV_FILE_ADDR)
         emit_add_a16(pc, i);
      else
         emit_mov(pc, i);
      break;
   case NV_OP_LDA:
      emit_mov(pc, i);
      break;
   case NV_OP_STA:
      emit_st(pc, i);
      break;
   case NV_OP_LINTERP:
   case NV_OP_PINTERP:
      emit_interp(pc, i);
      break;
   case NV_OP_ADD:
      emit_add(pc, i);
      break;
   case NV_OP_AND:
   case NV_OP_OR:
   case NV_OP_XOR:
      emit_bitop2(pc, i);
      break;
   case NV_OP_CVT:
   case NV_OP_ABS:
   case NV_OP_NEG:
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
   case NV_OP_RCP:
   case NV_OP_RSQ:
   case NV_OP_LG2:
   case NV_OP_SIN:
   case NV_OP_COS:
   case NV_OP_EX2:
      emit_flop(pc, i);
      break;
   case NV_OP_PRESIN:
   case NV_OP_PREEX2:
      emit_cvt2fixed(pc, i);
      break;
   case NV_OP_MAD:
      emit_mad(pc, i);
      break;
   case NV_OP_MAX:
   case NV_OP_MIN:
      emit_minmax(pc, i);
      break;
   case NV_OP_MUL:
      emit_mul_f32(pc, i);
      break;
   case NV_OP_SET:
      emit_set(pc, i);
      break;
   case NV_OP_SHL:
   case NV_OP_SHR:
      emit_shift(pc, i);
      break;
   case NV_OP_TEX:
   case NV_OP_TXB:
   case NV_OP_TXL:
      emit_tex(pc, i);
      break;
   case NV_OP_QUADOP:
      emit_quadop(pc, i);
      break;
   case NV_OP_KIL:
      emit_flow(pc, i, 0x0);
      break;
   case NV_OP_BRA:
      emit_flow(pc, i, 0x1);
      break;
   case NV_OP_CALL:
      emit_flow(pc, i, 0x2);
      break;
   case NV_OP_RET:
      emit_flow(pc, i, 0x3);
      break;
   case NV_OP_BREAKADDR:
      emit_flow(pc, i, 0x4);
      break;
   case NV_OP_BREAK:
      emit_flow(pc, i, 0x5);
      break;
   case NV_OP_JOINAT:
      emit_flow(pc, i, 0xa);
      break;
   case NV_OP_NOP:
   case NV_OP_JOIN:
      pc->emit[0] = 0xf0000001;
      pc->emit[1] = 0xe0000000;
      break;
   case NV_OP_PHI:
   case NV_OP_UNDEF:
   case NV_OP_SUB:
      NOUVEAU_ERR("operation \"%s\" should have been eliminated\n",
                  nv_opcode_name(i->opcode));
      break;
   default:
      NOUVEAU_ERR("unhandled NV_OP: %d\n", i->opcode);
      abort();
      break;
   }

   if (i->is_join) {
      assert(i->is_long && !(pc->emit[1] & 1));
      pc->emit[1] |= 2;
   }

   assert((pc->emit[0] & 1) == i->is_long);
}
