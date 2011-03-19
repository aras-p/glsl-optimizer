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

#define PRINT(args...) debug_printf(args)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

static const char *norm = "\x1b[00m";
static const char *gree = "\x1b[32m";
static const char *blue = "\x1b[34m";
static const char *cyan = "\x1b[36m";
static const char *yllw = "\x1b[33m";
static const char *mgta = "\x1b[35m";

static const char *nv_cond_names[] =
{
   "never", "lt" , "eq" , "le" , "gt" , "ne" , "ge" , "",
   "never", "ltu", "equ", "leu", "gtu", "neu", "geu", "",
   "o", "c", "a", "s"
};

static const char *nv_modifier_strings[] =
{
   "",
   "neg",
   "abs",
   "neg abs",
   "not",
   "not neg"
   "not abs",
   "not neg abs",
   "sat",
   "BAD_MOD"
};

const char *
nvc0_opcode_name(uint opcode)
{
   return nvc0_op_info_table[MIN2(opcode, NV_OP_COUNT)].name;
}

static INLINE const char *
nv_type_name(ubyte type, ubyte size)
{
   switch (type) {
   case NV_TYPE_U16: return "u16";
   case NV_TYPE_S16: return "s16";
   case NV_TYPE_F32: return "f32";
   case NV_TYPE_U32: return "u32";
   case NV_TYPE_S32: return "s32";
   case NV_TYPE_P32: return "p32";
   case NV_TYPE_F64: return "f64";
   case NV_TYPE_ANY:
   {
      switch (size) {
      case 1: return "b8";
      case 2: return "b16";
      case 4: return "b32";
      case 8: return "b64";
      case 12: return "b96";
      case 16: return "b128";
      default:
         return "BAD_SIZE";
      }
   }
   default:
      return "BAD_TYPE";
   }
}

static INLINE const char *
nv_cond_name(ubyte cc)
{
   return nv_cond_names[MIN2(cc, 19)];
}

static INLINE const char *
nv_modifier_string(ubyte mod)
{
   return nv_modifier_strings[MIN2(mod, 9)];
}

static INLINE int
nv_value_id(struct nv_value *value)
{
   if (value->join->reg.id >= 0)
      return value->join->reg.id;
   return value->n;
}

static INLINE boolean
nv_value_allocated(struct nv_value *value)
{
   return (value->reg.id >= 0) ? TRUE : FALSE;
}

static INLINE void
nv_print_address(const char c, int buf, struct nv_value *a, int offset)
{
   const char ac = (a && nv_value_allocated(a)) ? '$' : '%';
   char sg;

   if (offset < 0) {
      sg = '-';
      offset = -offset;
   } else {
      sg = '+';
   }

   if (buf >= 0)
      PRINT(" %s%c%i[", cyan, c, buf);
   else
      PRINT(" %s%c[", cyan, c);
   if (a)
      PRINT("%s%ca%i%s%c", mgta, ac, nv_value_id(a), cyan, sg);
   PRINT("%s0x%x%s]", yllw, offset, cyan);
}

static INLINE void
nv_print_value(struct nv_value *value, struct nv_value *indir, ubyte type)
{
   char reg_pfx = nv_value_allocated(value->join) ? '$' : '%';

   if (value->reg.file != NV_FILE_PRED)
      PRINT(" %s%s", gree, nv_type_name(type, value->reg.size));

   switch (value->reg.file) {
   case NV_FILE_GPR:
      PRINT(" %s%cr%i", blue, reg_pfx, nv_value_id(value));
      if (value->reg.size == 8)
         PRINT("d");
      if (value->reg.size == 16)
         PRINT("q");
      break;
   case NV_FILE_PRED:
      PRINT(" %s%cp%i", mgta, reg_pfx, nv_value_id(value));
      break;
   case NV_FILE_COND:
      PRINT(" %s%cc%i", mgta, reg_pfx, nv_value_id(value));
      break;
   case NV_FILE_MEM_L:
      nv_print_address('l', -1, indir, value->reg.address);
      break;
   case NV_FILE_MEM_G:
      nv_print_address('g', -1, indir, value->reg.address);
      break;
   case NV_FILE_MEM_A:
      nv_print_address('a', -1, indir, value->reg.address);
      break;
   case NV_FILE_MEM_V:
      nv_print_address('v', -1, indir, value->reg.address);
      break;
   case NV_FILE_IMM:
      switch (type) {
      case NV_TYPE_U16:
      case NV_TYPE_S16:
         PRINT(" %s0x%04x", yllw, value->reg.imm.u32);
         break;
      case NV_TYPE_F32:
         PRINT(" %s%f", yllw, value->reg.imm.f32);
         break;
      case NV_TYPE_F64:
         PRINT(" %s%f", yllw, value->reg.imm.f64);
         break;
      case NV_TYPE_U32:
      case NV_TYPE_S32:
      case NV_TYPE_P32:
      case NV_TYPE_ANY:
         PRINT(" %s0x%08x", yllw, value->reg.imm.u32);
         break;
      }
      break;
   default:
      if (value->reg.file >= NV_FILE_MEM_C(0) &&
          value->reg.file <= NV_FILE_MEM_C(15))
         nv_print_address('c', value->reg.file - NV_FILE_MEM_C(0), indir,
                          value->reg.address);
      else
         NOUVEAU_ERR(" BAD_FILE[%i]", nv_value_id(value));
      break;
   }
}

static INLINE void
nv_print_ref(struct nv_ref *ref, struct nv_value *indir, ubyte type)
{
   nv_print_value(ref->value, indir, type);
}

void
nvc0_print_instruction(struct nv_instruction *i)
{
   int s;

   PRINT("%i: ", i->serial);

   if (i->predicate >= 0) {
      PRINT("%s%s", gree, i->cc ? "fl" : "tr");
      nv_print_ref(i->src[i->predicate], NULL, NV_TYPE_U8);
      PRINT(" ");
   }

   PRINT("%s", gree);
   if (NV_BASEOP(i->opcode) == NV_OP_SET)
      PRINT("%s %s", nvc0_opcode_name(i->opcode), nv_cond_name(i->set_cond));
   else
   if (i->saturate)
      PRINT("sat %s", nvc0_opcode_name(i->opcode));
   else
      PRINT("%s", nvc0_opcode_name(i->opcode));

   if (i->opcode == NV_OP_CVT)
      nv_print_value(i->def[0], NULL, i->ext.cvt.d);
   else
   if (i->def[0])
      nv_print_value(i->def[0], NULL, NV_OPTYPE(i->opcode));
   else
   if (i->target)
      PRINT(" %s(BB:%i)", yllw, i->target->id);
   else
      PRINT(" #");

   for (s = 1; s < 4 && i->def[s]; ++s)
      nv_print_value(i->def[s], NULL, NV_OPTYPE(i->opcode));
   if (s > 1)
      PRINT("%s ,", norm);

   for (s = 0; s < 6 && i->src[s]; ++s) {
      ubyte type;
      if (s == i->indirect || s == i->predicate)
         continue;
      if (i->opcode == NV_OP_CVT)
         type = i->ext.cvt.s;
      else
         type = NV_OPTYPE(i->opcode);

      if (i->src[s]->mod)
         PRINT(" %s%s", gree, nv_modifier_string(i->src[s]->mod));

      if (i->indirect >= 0 &&
          NV_IS_MEMORY_FILE(i->src[s]->value->reg.file))
         nv_print_ref(i->src[s], i->src[i->indirect]->value, type);
      else
         nv_print_ref(i->src[s], NULL, type);
   }
   PRINT(" %s\n", norm);
}

#define NV_MOD_SGN_12  ((NV_MOD_ABS | NV_MOD_NEG) | ((NV_MOD_ABS | NV_MOD_NEG) << 4))
#define NV_MOD_NEG_123 (NV_MOD_NEG | (NV_MOD_NEG << 4) | (NV_MOD_NEG << 8))
#define NV_MOD_NEG_3   (NV_MOD_NEG << 8)

#define NV_MOD_SGN NV_MOD_SGN_12

struct nv_op_info nvc0_op_info_table[NV_OP_COUNT + 1] =
{
   { NV_OP_UNDEF,  "undef",  NV_TYPE_ANY, 0, /* fcvpoi */ 0, 0, 0, 0, 1, 0, 0 },
   { NV_OP_BIND,   "bind",   NV_TYPE_ANY, 0, /* fcvpoi */ 0, 0, 1, 0, 1, 0, 0 },
   { NV_OP_MERGE,  "merge",  NV_TYPE_ANY, 0, /* fcvpoi */ 0, 0, 1, 0, 1, 0, 0 },
   { NV_OP_PHI,    "phi",    NV_TYPE_ANY, 0, /* fcvpoi */ 0, 0, 0, 0, 1, 0, 0 },
   { NV_OP_SELECT, "select", NV_TYPE_ANY, 0, /* fcvpoi */ 0, 0, 0, 0, 1, 0, 0 },
   { NV_OP_NOP,    "nop",    NV_TYPE_ANY, 0, /* fcvpoi */ 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_LD,  "ld",  NV_TYPE_ANY, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_ST,  "st",  NV_TYPE_ANY, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_MOV, "mov", NV_TYPE_ANY, 0, 0, 0, 0, 1, 0, 1, 0 },
   { NV_OP_AND, "and", NV_TYPE_U32, NV_MOD_NOT, 0, 1, 0, 1, 0, 6, 0 },
   { NV_OP_OR,  "or",  NV_TYPE_U32, NV_MOD_NOT, 0, 1, 0, 1, 0, 6, 0 },
   { NV_OP_XOR, "xor", NV_TYPE_U32, NV_MOD_NOT, 0, 1, 0, 1, 0, 6, 0 },
   { NV_OP_SHL, "shl", NV_TYPE_U32, 0, 0, 0, 0, 1, 0, 1, 0 },
   { NV_OP_SHR, "shr", NV_TYPE_U32, 0, 0, 0, 0, 1, 0, 1, 0 },
   { NV_OP_NOT, "not", NV_TYPE_U32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_SET, "set", NV_TYPE_ANY, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_ADD, "add", NV_TYPE_F32, NV_MOD_SGN, 0, 1, 0, 1, 0, 2, 2 },
   { NV_OP_SUB, "sub", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 2, 2 },
   { NV_OP_MUL, "mul", NV_TYPE_F32, NV_MOD_NEG_123, 0, 1, 0, 1, 0, 2, 2 },
   { NV_OP_MAD, "mad", NV_TYPE_F32, NV_MOD_NEG_123, 0, 1, 0, 1, 0, 2, 2 },
   { NV_OP_ABS, "abs", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_NEG, "neg", NV_TYPE_F32, NV_MOD_ABS, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_MAX, "max", NV_TYPE_F32, NV_MOD_SGN, 0, 1, 0, 1, 0, 2, 2 },
   { NV_OP_MIN, "min", NV_TYPE_F32, NV_MOD_SGN, 0, 1, 0, 1, 0, 2, 2 },
   { NV_OP_CVT, "cvt", NV_TYPE_ANY, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_CEIL,  "ceil",  NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_FLOOR, "floor", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_TRUNC, "trunc", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_SAD, "sad", NV_TYPE_S32, 0, 0, 1, 0, 1, 0, 0, 0 },

   { NV_OP_VFETCH,  "vfetch",  NV_TYPE_ANY, 0, 0, 0, 1, 1, 0, 0, 0 },
   { NV_OP_PFETCH,  "pfetch",  NV_TYPE_U32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_EXPORT,  "export",  NV_TYPE_ANY, 0, 0, 0, 1, 1, 0, 0, 0 },
   { NV_OP_LINTERP, "linterp", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_PINTERP, "pinterp", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_EMIT,    "emit",    NV_TYPE_ANY, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_RESTART, "restart", NV_TYPE_ANY, 0, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_TEX, "tex",      NV_TYPE_F32, 0, 0, 0, 1, 1, 0, 0, 0 },
   { NV_OP_TXB, "texbias",  NV_TYPE_F32, 0, 0, 0, 1, 1, 0, 0, 0 },
   { NV_OP_TXL, "texlod",   NV_TYPE_F32, 0, 0, 0, 1, 1, 0, 0, 0 },
   { NV_OP_TXF, "texfetch", NV_TYPE_U32, 0, 0, 0, 1, 1, 0, 0, 0 },
   { NV_OP_TXQ, "texquery", NV_TYPE_U32, 0, 0, 0, 1, 1, 0, 0, 0 },

   { NV_OP_QUADOP, "quadop", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_DFDX,   "dfdx",   NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_DFDY,   "dfdy",   NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_KIL,  "kil",  NV_TYPE_ANY, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_BRA,  "bra",  NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },
   { NV_OP_CALL, "call", NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },
   { NV_OP_RET,  "ret",  NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },
   { NV_OP_RET,  "exit", NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },
   { NV_OP_NOP,  "ud",   NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },
   { NV_OP_NOP,  "ud",   NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },

   { NV_OP_JOINAT, "joinat", NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },
   { NV_OP_JOIN,   "join",   NV_TYPE_ANY, 0, 1, 0, 0, 1, 0, 0, 0 },

   { NV_OP_ADD, "add", NV_TYPE_S32, 0, 0, 1, 0, 1, 0, 1, 0 },
   { NV_OP_MUL, "mul", NV_TYPE_S32, 0, 0, 1, 0, 1, 0, 1, 0 },
   { NV_OP_ABS, "abs", NV_TYPE_S32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_NEG, "neg", NV_TYPE_S32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_MAX, "max", NV_TYPE_S32, 0, 0, 1, 0, 1, 0, 0, 0 },
   { NV_OP_MIN, "max", NV_TYPE_U32, 0, 0, 1, 0, 1, 0, 0, 0 },
   { NV_OP_MAX, "min", NV_TYPE_S32, 0, 0, 1, 0, 1, 0, 0, 0 },
   { NV_OP_MIN, "min", NV_TYPE_U32, 0, 0, 1, 0, 1, 0, 0, 0 },
   { NV_OP_SET, "set", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 2, 2 },
   { NV_OP_SET, "set", NV_TYPE_S32, 0, 0, 0, 0, 1, 0, 2, 2 },
   { NV_OP_SET, "set", NV_TYPE_U32, 0, 0, 0, 0, 1, 0, 2, 2 },
   { NV_OP_SHR, "sar", NV_TYPE_S32, 0, 0, 0, 0, 1, 0, 1, 0 },
   { NV_OP_RCP, "rcp", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_RSQ, "rsqrt", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_LG2, "lg2", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_SIN, "sin", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_COS, "cos", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_EX2, "ex2", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_PRESIN, "presin", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 1 },
   { NV_OP_PREEX2, "preex2", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 0, 1 },
   { NV_OP_SAT, "sat", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_SET_F32_AND, "and set", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_SET_F32_OR,  "or set",  NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },
   { NV_OP_SET_F32_XOR, "xor set", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_SELP, "selp", NV_TYPE_U32, 0, 0, 0, 0, 1, 0, 0, 0 },

   { NV_OP_SLCT, "slct", NV_TYPE_F32, NV_MOD_NEG_3, 0, 0, 0, 1, 0, 2, 2 },
   { NV_OP_SLCT, "slct", NV_TYPE_S32, NV_MOD_NEG_3, 0, 0, 0, 1, 0, 2, 2 },
   { NV_OP_SLCT, "slct", NV_TYPE_U32, NV_MOD_NEG_3, 0, 0, 0, 1, 0, 2, 2 },

   { NV_OP_ADD, "sub", NV_TYPE_F32, 0, 0, 0, 0, 1, 0, 1, 0 },

   { NV_OP_SET, "fset", NV_TYPE_F32, NV_MOD_SGN, 0, 0, 0, 1, 0, 2, 2 },

   { NV_OP_TXG, "texgrad", NV_TYPE_F32, 0, 0, 0, 1, 1, 0, 0, 0 },

   { NV_OP_UNDEF, "BAD_OP", NV_TYPE_ANY, 0, 0, 0, 0, 0, 0, 0, 0 }
};
