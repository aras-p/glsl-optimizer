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

#define NVXX_DEBUG 0

#define PRINT(args...) debug_printf(args)

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(a) (sizeof(a) / sizeof(a[0]))
#endif

static const char *norm = "\x1b[00m";
static const char *gree = "\x1b[32m";
static const char *blue = "\x1b[34m";
static const char *cyan = "\x1b[36m";
static const char *orng = "\x1b[33m";
static const char *mgta = "\x1b[35m";

static const char *nv_opcode_names[NV_OP_COUNT + 1] = {
   "phi",
   "extract",
   "combine",
   "lda",
   "sta",
   "mov",
   "add",
   "sub",
   "neg",
   "mul",
   "mad",
   "cvt",
   "sat",
   "not",
   "and",
   "or",
   "xor",
   "shl",
   "shr",
   "rcp",
   "undef",
   "rsqrt",
   "lg2",
   "sin",
   "cos",
   "ex2",
   "presin",
   "preex2",
   "min",
   "max",
   "set",
   "sad",
   "kil",
   "bra",
   "call",
   "ret",
   "break",
   "breakaddr",
   "joinat",
   "tex",
   "texbias",
   "texlod",
   "texfetch",
   "texsize",
   "dfdx",
   "dfdy",
   "quadop",
   "linterp",
   "pinterp",
   "abs",
   "ceil",
   "floor",
   "trunc",
   "nop",
   "select",
   "export",
   "join",
   "BAD_OP"
};

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
nv_opcode_name(uint opcode)
{
   return nv_opcode_names[MIN2(opcode, ARRAY_SIZE(nv_opcode_names) - 1)];
}

static INLINE const char *
nv_type_name(ubyte type)
{
   switch (type) {
   case NV_TYPE_U16: return "u16";
   case NV_TYPE_S16: return "s16";
   case NV_TYPE_F32: return "f32";
   case NV_TYPE_U32: return "u32";
   case NV_TYPE_S32: return "s32";
   case NV_TYPE_P32: return "p32";
   case NV_TYPE_F64: return "f64";
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
   const char ac =  (a && nv_value_allocated(a)) ? '$' : '%';

   if (buf >= 0)
      PRINT(" %s%c%i[", cyan, c, buf);
   else
      PRINT(" %s%c[", cyan, c);
   if (a)
      PRINT("%s%ca%i%s+", mgta, ac, nv_value_id(a), cyan);
   PRINT("%s0x%x%s]", orng, offset, cyan);
}

static INLINE void
nv_print_cond(struct nv_instruction *nvi)
{
   char pfx = nv_value_allocated(nvi->flags_src->value->join) ? '$' : '%';

   PRINT("%s%s %s%cc%i ",
         gree, nv_cond_name(nvi->cc),
         mgta, pfx, nv_value_id(nvi->flags_src->value));
}

static INLINE void
nv_print_value(struct nv_value *value, struct nv_value *ind, ubyte type)
{
   char reg_pfx = '$';

   if (type == NV_TYPE_ANY)
      type = value->reg.type;

   if (value->reg.file != NV_FILE_FLAGS)
      PRINT(" %s%s", gree, nv_type_name(type));

   if (!nv_value_allocated(value->join))
      reg_pfx = '%';

   switch (value->reg.file) {
   case NV_FILE_GPR:
      PRINT(" %s%cr%i", blue, reg_pfx, nv_value_id(value));
      break;
   case NV_FILE_OUT:
      PRINT(" %s%co%i", mgta, reg_pfx, nv_value_id(value));
      break;
   case NV_FILE_ADDR:
      PRINT(" %s%ca%i", mgta, reg_pfx, nv_value_id(value));
      break;
   case NV_FILE_FLAGS:
      PRINT(" %s%cc%i", mgta, reg_pfx, nv_value_id(value));
      break;
   case NV_FILE_MEM_L:
      nv_print_address('l', -1, ind, nv_value_id(value));
      break;
   case NV_FILE_MEM_S:
      nv_print_address('s', -1, ind, 4 * nv_value_id(value));
      break;
   case NV_FILE_MEM_P:
      nv_print_address('p', -1, ind, 4 * nv_value_id(value));
      break;
   case NV_FILE_MEM_V:
      nv_print_address('v', -1, ind, 4 * nv_value_id(value));
      break;
   case NV_FILE_IMM:
      switch (type) {
      case NV_TYPE_U16:
      case NV_TYPE_S16:
         PRINT(" %s0x%04x", orng, value->reg.imm.u32);
         break;
      case NV_TYPE_F32:
         PRINT(" %s%f", orng, value->reg.imm.f32);
         break;
      case NV_TYPE_F64:
         PRINT(" %s%f", orng, value->reg.imm.f64);
         break;
      case NV_TYPE_U32:
      case NV_TYPE_S32:
      case NV_TYPE_P32:
         PRINT(" %s0x%08x", orng, value->reg.imm.u32);
         break;
      }
      break;
   default:
      if (value->reg.file >= NV_FILE_MEM_G(0) &&
          value->reg.file <= NV_FILE_MEM_G(15))
         nv_print_address('g', value->reg.file - NV_FILE_MEM_G(0), ind,
                          nv_value_id(value) * 4);
      else
      if (value->reg.file >= NV_FILE_MEM_C(0) &&
          value->reg.file <= NV_FILE_MEM_C(15))
         nv_print_address('c', value->reg.file - NV_FILE_MEM_C(0), ind,
                          nv_value_id(value) * 4);
      else
         NOUVEAU_ERR(" BAD_FILE[%i]", nv_value_id(value));
      break;
   }
}

static INLINE void
nv_print_ref(struct nv_ref *ref, struct nv_value *ind)
{
   nv_print_value(ref->value, ind, ref->typecast);
}

void
nv_print_instruction(struct nv_instruction *i)
{
   int j;

   PRINT("%i: ", i->serial);

   if (i->flags_src)
      nv_print_cond(i);

   PRINT("%s", gree);
   if (i->opcode == NV_OP_SET)
      PRINT("set %s", nv_cond_name(i->set_cond));
   else
   if (i->saturate)
      PRINT("sat %s", nv_opcode_name(i->opcode));
   else
      PRINT("%s", nv_opcode_name(i->opcode));

   if (i->flags_def)
      nv_print_value(i->flags_def, NULL, NV_TYPE_ANY);

   /* Only STORE & STA can write to MEM, and they do not def
    * anything, so the address is thus part of the source.
    */
   if (i->def[0])
      nv_print_value(i->def[0], NULL, NV_TYPE_ANY);
   else
   if (i->target)
      PRINT(" %s(BB:%i)", orng, i->target->id);
   else
      PRINT(" #");

   for (j = 0; j < 4; ++j) {
      if (!i->src[j])
         continue;

      if (i->src[j]->mod)
         PRINT(" %s%s", gree, nv_modifier_string(i->src[j]->mod));

      nv_print_ref(i->src[j],
                   (j == nv50_indirect_opnd(i)) ?
                   i->src[4]->value : NULL);
   }
   PRINT(" %s%c\n", norm, i->is_long ? 'l' : 's');
}
