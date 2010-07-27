#include "draw_private.h"
#include "draw_context.h"

#include "draw_llvm.h"

#include "gallivm/lp_bld_arit.h"
#include "gallivm/lp_bld_struct.h"
#include "gallivm/lp_bld_format.h"
#include "gallivm/lp_bld_debug.h"
#include "gallivm/lp_bld_type.h"

#include "util/u_memory.h"
#include "util/u_format.h"
#include "pipe/p_state.h"


#define DRAW_DBG 0

static  LLVMValueRef
from_64_float(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMDoubleType(), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildFPTrunc(builder, l, LLVMFloatType(), "");
}

static LLVMValueRef
from_32_float(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMFloatType(), 0) , "");
   return LLVMBuildLoad(builder, bc, "");
}

static INLINE LLVMValueRef
from_8_uscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   return LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_16_uscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(16), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_32_uscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(32), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_8_sscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   return LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_16_sscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(16), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_32_sscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(32), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
}


static INLINE LLVMValueRef
from_8_unorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   LLVMValueRef uscaled = LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 255.), "");
}

static INLINE LLVMValueRef
from_16_unorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(16), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 65535.), "");
}

static INLINE LLVMValueRef
from_32_unorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(32), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");

   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 4294967295.), "");
}

static INLINE LLVMValueRef
from_8_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 127.0), "");
}

static INLINE LLVMValueRef
from_16_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(16), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 32767.0f), "");
}

static INLINE LLVMValueRef
from_32_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(32), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");

   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 2147483647.0), "");
}

static INLINE LLVMValueRef
from_32_fixed(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMPointerType(LLVMIntType(32), 0) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");

   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 65536.0), "");
}

static LLVMValueRef
to_64_float(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPExt(builder, l, LLVMDoubleType(), "");
}

static LLVMValueRef
to_32_float(LLVMBuilderRef builder, LLVMValueRef fp)
{
   return LLVMBuildLoad(builder, fp, "");
}

static INLINE LLVMValueRef
to_8_uscaled(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPToUI(builder, l, LLVMIntType(8), "");
}

static INLINE LLVMValueRef
to_16_uscaled(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPToUI(builder, l, LLVMIntType(16), "");
}

static INLINE LLVMValueRef
to_32_uscaled(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPToUI(builder, l, LLVMIntType(32), "");
}

static INLINE LLVMValueRef
to_8_sscaled(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPToSI(builder, l, LLVMIntType(8), "");
}

static INLINE LLVMValueRef
to_16_sscaled(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPToSI(builder, l, LLVMIntType(16), "");
}

static INLINE LLVMValueRef
to_32_sscaled(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPToSI(builder, l, LLVMIntType(32), "");
}

static INLINE LLVMValueRef
to_8_unorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToUI(builder, l, LLVMIntType(8), "");
   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 255.), "");
}

static INLINE LLVMValueRef
to_16_unorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToUI(builder, l, LLVMIntType(32), "");
   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 65535.), "");
}

static INLINE LLVMValueRef
to_32_unorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToUI(builder, l, LLVMIntType(32), "");

   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 4294967295.), "");
}

static INLINE LLVMValueRef
to_8_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(8), "");
   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 127.0), "");
}

static INLINE LLVMValueRef
to_16_snorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(16), "");
   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 32767.0f), "");
}

static INLINE LLVMValueRef
to_32_snorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(32), "");

   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 2147483647.0), "");
}

static INLINE LLVMValueRef
to_32_fixed(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(32), "");

   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(LLVMFloatType(), 65536.0), "");
}

typedef LLVMValueRef (*from_func)(LLVMBuilderRef, LLVMValueRef);
typedef  LLVMValueRef (*to_func)(LLVMBuilderRef, LLVMValueRef);

/* so that underneath can avoid function calls which are prohibited
 * for static initialization we need this conversion */
enum ll_type {
   LL_Double,
   LL_Float,
   LL_Int32,
   LL_Int16,
   LL_Int8
};

static INLINE LLVMTypeRef
ll_type_to_llvm(enum ll_type type)
{
   switch (type) {
   case LL_Double:
      return LLVMDoubleType();
   case LL_Float:
      return LLVMFloatType();
   case LL_Int32:
      return LLVMInt32Type();
   case LL_Int16:
      return LLVMIntType(16);
   case LL_Int8:
      return LLVMIntType(8);
   }
   return LLVMIntType(8);
}

static INLINE int
ll_type_size(enum ll_type type)
{
   switch (type) {
   case LL_Double:
      return 8;
   case LL_Float:
      return 4;
   case LL_Int32:
      return 4;
   case LL_Int16:
      return 2;
   case LL_Int8:
      return 1;
   }
   return 1;
}

struct draw_llvm_translate {
   int format;
   from_func from;
   to_func to;
   enum ll_type type;
   int num_components;
} translates[] =
{
   {PIPE_FORMAT_R64_FLOAT,          from_64_float, to_64_float, LL_Double, 1},
   {PIPE_FORMAT_R64G64_FLOAT,       from_64_float, to_64_float, LL_Double, 2},
   {PIPE_FORMAT_R64G64B64_FLOAT,    from_64_float, to_64_float, LL_Double, 3},
   {PIPE_FORMAT_R64G64B64A64_FLOAT, from_64_float, to_64_float, LL_Double, 4},
   {PIPE_FORMAT_R32_FLOAT,          from_32_float, to_32_float, LL_Float, 1},
   {PIPE_FORMAT_R32G32_FLOAT,       from_32_float, to_32_float, LL_Float, 2},
   {PIPE_FORMAT_R32G32B32_FLOAT,    from_32_float, to_32_float, LL_Float, 3},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, from_32_float, to_32_float, LL_Float, 4},

   {PIPE_FORMAT_R32_UNORM,          from_32_unorm, to_32_unorm, LL_Int32, 1},
   {PIPE_FORMAT_R32G32_UNORM,       from_32_unorm, to_32_unorm, LL_Int32, 2},
   {PIPE_FORMAT_R32G32B32_UNORM,    from_32_unorm, to_32_unorm, LL_Int32, 3},
   {PIPE_FORMAT_R32G32B32A32_UNORM, from_32_unorm, to_32_unorm, LL_Int32, 4},

   {PIPE_FORMAT_R32_USCALED,          from_32_uscaled, to_32_uscaled, LL_Int32, 1},
   {PIPE_FORMAT_R32G32_USCALED,       from_32_uscaled, to_32_uscaled, LL_Int32, 2},
   {PIPE_FORMAT_R32G32B32_USCALED,    from_32_uscaled, to_32_uscaled, LL_Int32, 3},
   {PIPE_FORMAT_R32G32B32A32_USCALED, from_32_uscaled, to_32_uscaled, LL_Int32, 4},

   {PIPE_FORMAT_R32_SNORM,          from_32_snorm, to_32_snorm, LL_Int32, 1},
   {PIPE_FORMAT_R32G32_SNORM,       from_32_snorm, to_32_snorm, LL_Int32, 2},
   {PIPE_FORMAT_R32G32B32_SNORM,    from_32_snorm, to_32_snorm, LL_Int32, 3},
   {PIPE_FORMAT_R32G32B32A32_SNORM, from_32_snorm, to_32_snorm, LL_Int32, 4},

   {PIPE_FORMAT_R32_SSCALED,          from_32_sscaled, to_32_sscaled, LL_Int32, 1},
   {PIPE_FORMAT_R32G32_SSCALED,       from_32_sscaled, to_32_sscaled, LL_Int32, 2},
   {PIPE_FORMAT_R32G32B32_SSCALED,    from_32_sscaled, to_32_sscaled, LL_Int32, 3},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, from_32_sscaled, to_32_sscaled, LL_Int32, 4},

   {PIPE_FORMAT_R16_UNORM,          from_16_unorm, to_16_unorm, LL_Int16, 1},
   {PIPE_FORMAT_R16G16_UNORM,       from_16_unorm, to_16_unorm, LL_Int16, 2},
   {PIPE_FORMAT_R16G16B16_UNORM,    from_16_unorm, to_16_unorm, LL_Int16, 3},
   {PIPE_FORMAT_R16G16B16A16_UNORM, from_16_unorm, to_16_unorm, LL_Int16, 4},

   {PIPE_FORMAT_R16_USCALED,          from_16_uscaled, to_16_uscaled, LL_Int16, 1},
   {PIPE_FORMAT_R16G16_USCALED,       from_16_uscaled, to_16_uscaled, LL_Int16, 2},
   {PIPE_FORMAT_R16G16B16_USCALED,    from_16_uscaled, to_16_uscaled, LL_Int16, 3},
   {PIPE_FORMAT_R16G16B16A16_USCALED, from_16_uscaled, to_16_uscaled, LL_Int16, 4},

   {PIPE_FORMAT_R16_SNORM,          from_16_snorm, to_16_snorm, LL_Int16, 1},
   {PIPE_FORMAT_R16G16_SNORM,       from_16_snorm, to_16_snorm, LL_Int16, 2},
   {PIPE_FORMAT_R16G16B16_SNORM,    from_16_snorm, to_16_snorm, LL_Int16, 3},
   {PIPE_FORMAT_R16G16B16A16_SNORM, from_16_snorm, to_16_snorm, LL_Int16, 4},

   {PIPE_FORMAT_R16_SSCALED,          from_16_sscaled, to_16_sscaled, LL_Int16, 1},
   {PIPE_FORMAT_R16G16_SSCALED,       from_16_sscaled, to_16_sscaled, LL_Int16, 2},
   {PIPE_FORMAT_R16G16B16_SSCALED,    from_16_sscaled, to_16_sscaled, LL_Int16, 3},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, from_16_sscaled, to_16_sscaled, LL_Int16, 4},

   {PIPE_FORMAT_R8_UNORM,       from_8_unorm, to_8_unorm, LL_Int8, 1},
   {PIPE_FORMAT_R8G8_UNORM,     from_8_unorm, to_8_unorm, LL_Int8, 2},
   {PIPE_FORMAT_R8G8B8_UNORM,   from_8_unorm, to_8_unorm, LL_Int8, 3},
   {PIPE_FORMAT_R8G8B8A8_UNORM, from_8_unorm, to_8_unorm, LL_Int8, 4},

   {PIPE_FORMAT_R8_USCALED,       from_8_uscaled, to_8_uscaled, LL_Int8, 1},
   {PIPE_FORMAT_R8G8_USCALED,     from_8_uscaled, to_8_uscaled, LL_Int8, 2},
   {PIPE_FORMAT_R8G8B8_USCALED,   from_8_uscaled, to_8_uscaled, LL_Int8, 3},
   {PIPE_FORMAT_R8G8B8A8_USCALED, from_8_uscaled, to_8_uscaled, LL_Int8, 4},

   {PIPE_FORMAT_R8_SNORM,       from_8_snorm, to_8_snorm, LL_Int8, 1},
   {PIPE_FORMAT_R8G8_SNORM,     from_8_snorm, to_8_snorm, LL_Int8, 2},
   {PIPE_FORMAT_R8G8B8_SNORM,   from_8_snorm, to_8_snorm, LL_Int8, 3},
   {PIPE_FORMAT_R8G8B8A8_SNORM, from_8_snorm, to_8_snorm, LL_Int8, 4},

   {PIPE_FORMAT_R8_SSCALED,       from_8_sscaled, to_8_sscaled, LL_Int8, 1},
   {PIPE_FORMAT_R8G8_SSCALED,     from_8_sscaled, to_8_sscaled, LL_Int8, 2},
   {PIPE_FORMAT_R8G8B8_SSCALED,   from_8_sscaled, to_8_sscaled, LL_Int8, 3},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, from_8_sscaled, to_8_sscaled, LL_Int8, 4},

   {PIPE_FORMAT_R32_FIXED,          from_32_fixed, to_32_fixed, LL_Int32, 1},
   {PIPE_FORMAT_R32G32_FIXED,       from_32_fixed, to_32_fixed, LL_Int32, 2},
   {PIPE_FORMAT_R32G32B32_FIXED,    from_32_fixed, to_32_fixed, LL_Int32, 3},
   {PIPE_FORMAT_R32G32B32A32_FIXED, from_32_fixed, to_32_fixed, LL_Int32, 4},
};


static LLVMValueRef
fetch(LLVMBuilderRef builder,
      LLVMValueRef ptr, int val_size, int nr_components,
      from_func func)
{
   int i;
   int offset = 0;
   LLVMValueRef res = LLVMConstNull(
      LLVMVectorType(LLVMFloatType(), 4));
   LLVMValueRef defaults[4];

   defaults[0] = LLVMConstReal(LLVMFloatType(), 0);
   defaults[1] = LLVMConstReal(LLVMFloatType(), 0);
   defaults[2] = LLVMConstReal(LLVMFloatType(), 0);
   defaults[3] = LLVMConstReal(LLVMFloatType(), 1);

   for (i = 0; i < nr_components; ++i) {
      LLVMValueRef src_index = LLVMConstInt(LLVMInt32Type(), offset, 0);
      LLVMValueRef dst_index = LLVMConstInt(LLVMInt32Type(), i, 0);
      LLVMValueRef src_tmp;
      LLVMValueRef component;

      src_tmp = LLVMBuildGEP(builder, ptr, &src_index, 1, "src_tmp");

      /* convert src_tmp to float */
      component = func(builder, src_tmp);

      /* vec.comp = component */
      res = LLVMBuildInsertElement(builder,
                                   res,
                                   component,
                                   dst_index, "");
      offset += val_size;
   }
   for (; i < 4; ++i) {
      LLVMValueRef dst_index = LLVMConstInt(LLVMInt32Type(), i, 0);
      res = LLVMBuildInsertElement(builder,
                                   res,
                                   defaults[i],
                                   dst_index, "");
   }
   return res;
}


LLVMValueRef
draw_llvm_translate_from(LLVMBuilderRef builder,
                         LLVMValueRef vbuffer,
                         enum pipe_format from_format)
{
   const struct util_format_description *format_desc;
   LLVMValueRef zero;
   int i;
   struct lp_type type = lp_float32_vec4_type();

   /*
    * The above can only cope with straight arrays: no bitfields,
    * swizzles, or half floats.
    */

   for (i = 0; i < Elements(translates); ++i) {
      if (translates[i].format == from_format) {
         /*LLVMTypeRef type = ll_type_to_llvm(translates[i].type);*/
         return fetch(builder,
                      vbuffer,
                      ll_type_size(translates[i].type),
                      translates[i].num_components,
                      translates[i].from);
      }
   }


   /*
    * This doesn't handle anything bigger than 32bits, or half floats
    * yet.
    *
    * TODO: unify all this code into lp_build_fetch_rgba_aos().
    */

   format_desc = util_format_description(from_format);
   zero = LLVMConstNull(LLVMInt32Type());
   return lp_build_fetch_rgba_aos(builder, format_desc, type, vbuffer, zero, zero, zero);
}
