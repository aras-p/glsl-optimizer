
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
   "(undefined)",
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
   "BAD_OP"
};

static const char *nv_cond_names[] =
{
   "never", "lt" , "eq" , "le" , "gt" , "ne" , "ge" , "",
   "never", "ltu", "equ", "leu", "gtu", "neu", "geu", ""
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
   return nv_cond_names[MIN2(cc, 15)];
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
   if (buf >= 0)
      PRINT(" %s%c%i[", cyan, c, buf);
   else
      PRINT(" %s%c[", cyan, c);
   if (a)
      PRINT("%s$a%i%s+", mgta, nv_value_id(a), cyan);
   PRINT("%s0x%x%s]", orng, offset, cyan);
}

static INLINE void
nv_print_cond(struct nv_instruction *nvi)
{
   PRINT("%s%s%s$c%i ",
         gree, nv_cond_name(nvi->cc),
         mgta, nv_value_id(nvi->flags_src->value));
}

static INLINE void
nv_print_value(struct nv_value *value, struct nv_value *ind, ubyte type)
{
   char reg_pfx = '$';

   if (type == NV_TYPE_ANY)
      type = value->reg.type;

   if (value->reg.file != NV_FILE_FLAGS)
      PRINT(" %s%s", gree, nv_type_name(type));

   if (!nv_value_allocated(value))
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
      PRINT(" #");

   for (j = 0; j < 4; ++j) {
      if (!i->src[j])
         continue;

      if (i->src[j]->mod)
         PRINT(" %s", nv_modifier_string(i->src[j]->mod));

      nv_print_ref(i->src[j],
                   (j == nv50_indirect_opnd(i)) ?
                   i->src[4]->value : NULL);
   }
   if (!i->is_long)
      PRINT(" %ss", norm);
   PRINT("\n");
}
