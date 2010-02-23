


#include "util/u_memory.h"
#include "pipe/p_state.h"
#include "translate.h"


#define DRAW_DBG 0

typedef void (*fetch_func)(const void *ptr, float *attrib);
typedef void (*emit_func)(const float *attrib, void *ptr);



struct translate_generic {
   struct translate translate;

   struct {
      enum translate_element_type type;

      fetch_func fetch;
      unsigned buffer;
      unsigned input_offset;
      unsigned instance_divisor;

      emit_func emit;
      unsigned output_offset;

      char *input_ptr;
      unsigned input_stride;

   } attrib[PIPE_MAX_ATTRIBS];

   unsigned nr_attrib;
};


static struct translate_generic *translate_generic( struct translate *translate )
{
   return (struct translate_generic *)translate;
}

/**
 * Fetch a float[4] vertex attribute from memory, doing format/type
 * conversion as needed.
 *
 * This is probably needed/dupliocated elsewhere, eg format
 * conversion, texture sampling etc.
 */
#define ATTRIB( NAME, SZ, TYPE, FROM, TO )		\
static void						\
fetch_##NAME(const void *ptr, float *attrib)		\
{							\
   const float defaults[4] = { 0.0f,0.0f,0.0f,1.0f };	\
   unsigned i;						\
							\
   for (i = 0; i < SZ; i++) {				\
      attrib[i] = FROM(i);				\
   }							\
							\
   for (; i < 4; i++) {					\
      attrib[i] = defaults[i];				\
   }							\
}							\
							\
static void						\
emit_##NAME(const float *attrib, void *ptr)		\
{  \
   unsigned i;						\
   TYPE *out = (TYPE *)ptr;				\
							\
   for (i = 0; i < SZ; i++) {				\
      out[i] = TO(attrib[i]);				\
   }							\
}

{

   return conv = instr(builder, bc, "");
}

static INLINE LLVMValueRef
from_64_float(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMDoubleType() , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildFPTrunc(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_32_float(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMFloatType() , "");
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
                                      LLVMIntType(16) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_32_uscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(32) , "");
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
                                      LLVMIntType(16) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
}

static INLINE LLVMValueRef
from_32_sscaled(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(32) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   return LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
}


static INLINE LLVMValueRef
from_8_unorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   LLVMValueRef uscaled = LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 255.));
}

static INLINE LLVMValueRef
from_16_unorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(16) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 65535.));
}

static INLINE LLVMValueRef
from_32_unorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(32) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildUIToFP(builder, l, LLVMFloatType(), "");

   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 4294967295.));
}

static INLINE LLVMValueRef
from_8_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 127.0));
}

static INLINE LLVMValueRef
from_16_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(16) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");
   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 32767.0f));
}

static INLINE LLVMValueRef
from_32_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(32) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");

   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 2147483647.0));
}

static INLINE LLVMValueRef
from_32_fixed(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef bc = LLVMBuildBitCast(builder, val,
                                      LLVMIntType(32) , "");
   LLVMValueRef l = LLVMBuildLoad(builder, bc, "");
   LLVMValueRef uscaled = LLVMBuildSIToFP(builder, l, LLVMFloatType(), "");

   return LLVMBuildFDiv(builder, uscaled,
                        LLVMConstReal(builder, 65536.0));
}

static INLINE LLVMValueRef
to_64_float(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   return LLVMBuildFPExt(builder, l, LLVMDoubleType(), "");
}

static INLINE LLVMValueRef
to_32_float(LLVMBuilderRef builder, LLVMValueRef fp)
{
   return LLVMBuildLoad(builder, fp, "");
}

atic INLINE LLVMValueRef
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
                        LLVMConstReal(builder, 255.));
}

static INLINE LLVMValueRef
to_16_unorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToUI(builder, l, LLVMIntType(32), "");
   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(builder, 65535.));
}

static INLINE LLVMValueRef
to_32_unorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToUI(builder, l, LLVMIntType(32), "");

   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(builder, 4294967295.));
}

static INLINE LLVMValueRef
to_8_snorm(LLVMBuilderRef builder, LLVMValueRef val)
{
   LLVMValueRef l = LLVMBuildLoad(builder, val, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(8), "");
   return LLVMBuildFMUL(builder, uscaled,
                        LLVMConstReal(builder, 127.0));
}

static INLINE LLVMValueRef
to_16_snorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(16), "");
   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(builder, 32767.0f));
}

static INLINE LLVMValueRef
to_32_snorm(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(32), "");

   return LLVMBuildFMUL(builder, uscaled,
                        LLVMConstReal(builder, 2147483647.0));
}

static INLINE LLVMValueRef
to_32_fixed(LLVMBuilderRef builder, LLVMValueRef fp)
{
   LLVMValueRef l = LLVMBuildLoad(builder, fp, "");
   LLVMValueRef uscaled = LLVMBuildFPToSI(builder, l, LLVMIntType(32), "");

   return LLVMBuildFMul(builder, uscaled,
                        LLVMConstReal(builder, 65536.0));
}

static LLVMValueRef
fetch(LLVMValueRef ptr, int val_size, int nr_components,
     LLVMValueRef res)
{
   int i;
   int offset = 0;

   for (i = 0; i < nr_components; ++i) {
      LLVMValueRef src_index = LLVMConstInt(LLVMInt32Type(), offset, 0);
      LLVMValueRef dst_index = LLVMConstInt(LLVMInt32Type(), i, 0);
      //getelementptr i8* ptr, i64 offset
      LLVMValueRef src_tmp = LLVMBuildGEP(builder, ptr, &src_index, 1, "");
      //getelementptr float* res, i64 i
      LLVMValueRef res_tmp = LLVMBuildGEP(builder, res, &dst_index, 1, "");
      //bitcast i8* src, to res_type*
      //load res_type src
      //convert res_type src to float
      //store float src, float *dst src
      offset += val_size;
   }
}


static void
fetch_B8G8R8A8_UNORM(const void *ptr, float *attrib)
{
   attrib[2] = FROM_8_UNORM(0);
   attrib[1] = FROM_8_UNORM(1);
   attrib[0] = FROM_8_UNORM(2);
   attrib[3] = FROM_8_UNORM(3);
}

static void
emit_B8G8R8A8_UNORM( const float *attrib, void *ptr)
{
   ubyte *out = (ubyte *)ptr;
   out[2] = TO_8_UNORM(attrib[0]);
   out[1] = TO_8_UNORM(attrib[1]);
   out[0] = TO_8_UNORM(attrib[2]);
   out[3] = TO_8_UNORM(attrib[3]);
}

static void
fetch_NULL( const void *ptr, float *attrib )
{
   attrib[0] = 0;
   attrib[1] = 0;
   attrib[2] = 0;
   attrib[3] = 1;
}

static void
emit_NULL( const float *attrib, void *ptr )
{
   /* do nothing is the only sensible option */
}

typedef LLVMValueRef (*from_func)(LLVMBuilderRef, LLVMValueRef);
typedef  LLVMValueRef (*to_func)(LLVMBuilderRef, LLVMValueRef);

struct draw_llvm_translate {
   int format;
   from_func from;
   to_func to;
   LLVMTypeRef type;
   int num_components;
} translates[] =
{
   {PIPE_FORMAT_R64_FLOAT,          from_64_float, to_64_float, LLVMDoubleType(), 1},
   {PIPE_FORMAT_R64G64_FLOAT,       from_64_float, to_64_float, LLVMDoubleType(), 2},
   {PIPE_FORMAT_R64G64B64_FLOAT,    from_64_float, to_64_float, LLVMDoubleType(), 3},
   {PIPE_FORMAT_R64G64B64A64_FLOAT, from_64_float, to_64_float, LLVMDoubleType(), 4},

   {PIPE_FORMAT_R32_FLOAT,          from_32_float, to_32_float, LLVMFloatType(), 1},
   {PIPE_FORMAT_R32G32_FLOAT,       from_32_float, to_32_float, LLVMFloatType(), 2},
   {PIPE_FORMAT_R32G32B32_FLOAT,    from_32_float, to_32_float, LLVMFloatType(), 3},
   {PIPE_FORMAT_R32G32B32A32_FLOAT, from_32_float, to_32_float, LLVMFloatType(), 4},

   {PIPE_FORMAT_R32_UNORM,          from_32_unorm, to_32_unorm, LLVMIntType(32), 1},
   {PIPE_FORMAT_R32G32_UNORM,       from_32_unorm, to_32_unorm, LLVMIntType(32), 2},
   {PIPE_FORMAT_R32G32B32_UNORM,    from_32_unorm, to_32_unorm, LLVMIntType(32), 3},
   {PIPE_FORMAT_R32G32B32A32_UNORM, from_32_unorm, to_32_unorm, LLVMIntType(32), 4},

   {PIPE_FORMAT_R32_USCALED,          from_32_uscaled, to_32_uscaled, LLVMIntType(32), 1},
   {PIPE_FORMAT_R32G32_USCALED,       from_32_uscaled, to_32_uscaled, LLVMIntType(32), 2},
   {PIPE_FORMAT_R32G32B32_USCALED,    from_32_uscaled, to_32_uscaled, LLVMIntType(32), 3},
   {PIPE_FORMAT_R32G32B32A32_USCALED, from_32_uscaled, to_32_uscaled, LLVMIntType(32), 4},

   {PIPE_FORMAT_R32_SNORM,          from_32_snorm, to_32_snorm, LLVMIntType(32), 1},
   {PIPE_FORMAT_R32G32_SNORM,       from_32_snorm, to_32_snorm, LLVMIntType(32), 2},
   {PIPE_FORMAT_R32G32B32_SNORM,    from_32_snorm, to_32_snorm, LLVMIntType(32), 3},
   {PIPE_FORMAT_R32G32B32A32_SNORM, from_32_snorm, to_32_snorm, LLVMIntType(32), 4},

   {PIPE_FORMAT_R32_SSCALED,          from_32_sscaled, to_32_sscaled, LLVMIntType(32), 1},
   {PIPE_FORMAT_R32G32_SSCALED,       from_32_sscaled, to_32_sscaled, LLVMIntType(32), 2},
   {PIPE_FORMAT_R32G32B32_SSCALED,    from_32_sscaled, to_32_sscaled, LLVMIntType(32), 3},
   {PIPE_FORMAT_R32G32B32A32_SSCALED, from_32_sscaled, to_32_sscaled, LLVMIntType(32), 4},

   {PIPE_FORMAT_R16_UNORM,          from_16_unorm, to_16_unorm, LLVMIntType(16), 1},
   {PIPE_FORMAT_R16G16_UNORM,       from_16_unorm, to_16_unorm, LLVMIntType(16), 2},
   {PIPE_FORMAT_R16G16B16_UNORM,    from_16_unorm, to_16_unorm, LLVMIntType(16), 3},
   {PIPE_FORMAT_R16G16B16A16_UNORM, from_16_unorm, to_16_unorm, LLVMIntType(16), 4},

   {PIPE_FORMAT_R16_USCALED,          from_16_uscaled, to_16_uscaled, LLVMIntType(16), 1},
   {PIPE_FORMAT_R16G16_USCALED,       from_16_uscaled, to_16_uscaled, LLVMIntType(16), 2},
   {PIPE_FORMAT_R16G16B16_USCALED,    from_16_uscaled, to_16_uscaled, LLVMIntType(16), 3},
   {PIPE_FORMAT_R16G16B16A16_USCALED, from_16_uscaled, to_16_uscaled, LLVMIntType(16), 4},

   {PIPE_FORMAT_R16_SNORM,          from_16_snorm, to_16_snorm, LLVMIntType(16), 1},
   {PIPE_FORMAT_R16G16_SNORM,       from_16_snorm, to_16_snorm, LLVMIntType(16), 2},
   {PIPE_FORMAT_R16G16B16_SNORM,    from_16_snorm, to_16_snorm, LLVMIntType(16), 3},
   {PIPE_FORMAT_R16G16B16A16_SNORM, from_16_snorm, to_16_snorm, LLVMIntType(16), 4},

   {PIPE_FORMAT_R16_SSCALED,          from_16_sscaled, to_16_sscaled, LLVMIntType(16), 1},
   {PIPE_FORMAT_R16G16_SSCALED,       from_16_sscaled, to_16_sscaled, LLVMIntType(16), 2},
   {PIPE_FORMAT_R16G16B16_SSCALED,    from_16_sscaled, to_16_sscaled, LLVMIntType(16), 3},
   {PIPE_FORMAT_R16G16B16A16_SSCALED, from_16_sscaled, to_16_sscaled, LLVMIntType(16), 4},

   {PIPE_FORMAT_R8_UNORM,       from_8_unorm, to_8_unorm, LLVMIntType(8), 1},
   {PIPE_FORMAT_R8G8_UNORM,     from_8_unorm, to_8_unorm, LLVMIntType(8), 2},
   {PIPE_FORMAT_R8G8B8_UNORM,   from_8_unorm, to_8_unorm, LLVMIntType(8), 3},
   {PIPE_FORMAT_R8G8B8A8_UNORM, from_8_unorm, to_8_unorm, LLVMIntType(8), 4},

   {PIPE_FORMAT_R8_USCALED,       from_8_uscaled, to_8_uscaled, LLVMIntType(8), 1},
   {PIPE_FORMAT_R8G8_USCALED,     from_8_uscaled, to_8_uscaled, LLVMIntType(8), 2},
   {PIPE_FORMAT_R8G8B8_USCALED,   from_8_uscaled, to_8_uscaled, LLVMIntType(8), 3},
   {PIPE_FORMAT_R8G8B8A8_USCALED, from_8_uscaled, to_8_uscaled, LLVMIntType(8), 4},

   {PIPE_FORMAT_R8_SNORM,       from_8_snorm, to_8_snorm, LLVMIntType(8), 1},
   {PIPE_FORMAT_R8G8_SNORM,     from_8_snorm, to_8_snorm, LLVMIntType(8), 2},
   {PIPE_FORMAT_R8G8B8_SNORM,   from_8_snorm, to_8_snorm, LLVMIntType(8), 3},
   {PIPE_FORMAT_R8G8B8A8_SNORM, from_8_snorm, to_8_snorm, LLVMIntType(8), 4},

   {PIPE_FORMAT_R8_SSCALED,       from_8_sscaled, to_8_sscaled, LLVMIntType(8), 1},
   {PIPE_FORMAT_R8G8_SSCALED,     from_8_sscaled, to_8_sscaled, LLVMIntType(8), 2},
   {PIPE_FORMAT_R8G8B8_SSCALED,   from_8_sscaled, to_8_sscaled, LLVMIntType(8), 3},
   {PIPE_FORMAT_R8G8B8A8_SSCALED, from_8_sscaled, to_8_sscaled, LLVMIntType(8), 4},

   {PIPE_FORMAT_R32_FIXED,          from_32_fixed, to_32_fixed, LLVMIntType(32), 1},
   {PIPE_FORMAT_R32G32_FIXED,       from_32_fixed, to_32_fixed, LLVMIntType(32), 2},
   {PIPE_FORMAT_R32G32B32_FIXED,    from_32_fixed, to_32_fixed, LLVMIntType(32), 3},
   {PIPE_FORMAT_R32G32B32A32_FIXED, from_32_fixed, to_32_fixed, LLVMIntType(32), 4},

   {PIPE_FORMAT_A8R8G8B8_UNORM, from_8_unorm, to_8_unorm, LLVMIntType(8), 4},
   {PIPE_FORMAT_B8G8R8A8_UNORM, from_8_unorm, to_8_unorm, LLVMIntType(), 4},
};

/**
 * Fetch vertex attributes for 'count' vertices.
 */
static void PIPE_CDECL generic_run_elts( struct translate *translate,
                                         const unsigned *elts,
                                         unsigned count,
                                         unsigned instance_id,
                                         void *output_buffer )
{
   struct translate_generic *tg = translate_generic(translate);
   char *vert = output_buffer;
   unsigned nr_attrs = tg->nr_attrib;
   unsigned attr;
   unsigned i;

   /* loop over vertex attributes (vertex shader inputs)
    */
   for (i = 0; i < count; i++) {
      unsigned elt = *elts++;

      for (attr = 0; attr < nr_attrs; attr++) {
	 float data[4];
         const char *src;

	 char *dst = (vert +
		      tg->attrib[attr].output_offset);

         if (tg->attrib[attr].instance_divisor) {
            src = tg->attrib[attr].input_ptr +
                  tg->attrib[attr].input_stride *
                  (instance_id / tg->attrib[attr].instance_divisor);
         } else {
            src = tg->attrib[attr].input_ptr +
                  tg->attrib[attr].input_stride * elt;
         }

	 tg->attrib[attr].fetch( src, data );

         if (0) debug_printf("vert %d/%d attr %d: %f %f %f %f\n",
                             i, elt, attr, data[0], data[1], data[2], data[3]);

	 tg->attrib[attr].emit( data, dst );
      }

      vert += tg->translate.key.output_stride;
   }
}



static void PIPE_CDECL generic_run( struct translate *translate,
                                    unsigned start,
                                    unsigned count,
                                    unsigned instance_id,
                                    void *output_buffer )
{
   struct translate_generic *tg = translate_generic(translate);
   char *vert = output_buffer;
   unsigned nr_attrs = tg->nr_attrib;
   unsigned attr;
   unsigned i;

   /* loop over vertex attributes (vertex shader inputs)
    */
   for (i = 0; i < count; i++) {
      unsigned elt = start + i;

      for (attr = 0; attr < nr_attrs; attr++) {
	 float data[4];

	 char *dst = (vert +
		      tg->attrib[attr].output_offset);

         if (tg->attrib[attr].type == TRANSLATE_ELEMENT_NORMAL) {
            const char *src;

            if (tg->attrib[attr].instance_divisor) {
               src = tg->attrib[attr].input_ptr +
                     tg->attrib[attr].input_stride *
                     (instance_id / tg->attrib[attr].instance_divisor);
            } else {
               src = tg->attrib[attr].input_ptr +
                     tg->attrib[attr].input_stride * elt;
            }

            tg->attrib[attr].fetch( src, data );
         } else {
            data[0] = (float)instance_id;
         }

         if (0) debug_printf("vert %d attr %d: %f %f %f %f\n",
                             i, attr, data[0], data[1], data[2], data[3]);

	 tg->attrib[attr].emit( data, dst );
      }

      vert += tg->translate.key.output_stride;
   }
}



static void generic_set_buffer( struct translate *translate,
				unsigned buf,
				const void *ptr,
				unsigned stride )
{
   struct translate_generic *tg = translate_generic(translate);
   unsigned i;

   for (i = 0; i < tg->nr_attrib; i++) {
      if (tg->attrib[i].buffer == buf) {
	 tg->attrib[i].input_ptr = ((char *)ptr +
				    tg->attrib[i].input_offset);
	 tg->attrib[i].input_stride = stride;
      }
   }
}


static void generic_release( struct translate *translate )
{
   /* Refcount?
    */
   FREE(translate);
}

struct translate *translate_generic_create( const struct translate_key *key )
{
   struct translate_generic *tg = CALLOC_STRUCT(translate_generic);
   unsigned i;

   if (tg == NULL)
      return NULL;

   tg->translate.key = *key;
   tg->translate.release = generic_release;
   tg->translate.set_buffer = generic_set_buffer;
   tg->translate.run_elts = generic_run_elts;
   tg->translate.run = generic_run;

   for (i = 0; i < key->nr_elements; i++) {
      tg->attrib[i].type = key->element[i].type;

      tg->attrib[i].fetch = get_fetch_func(key->element[i].input_format);
      tg->attrib[i].buffer = key->element[i].input_buffer;
      tg->attrib[i].input_offset = key->element[i].input_offset;
      tg->attrib[i].instance_divisor = key->element[i].instance_divisor;

      tg->attrib[i].emit = get_emit_func(key->element[i].output_format);
      tg->attrib[i].output_offset = key->element[i].output_offset;

   }

   tg->nr_attrib = key->nr_elements;


   return &tg->translate;
}
