#ifndef GALLIVM_P_H
#define GALLIVM_P_H

#ifdef MESA_LLVM

#include "gallivm.h"
#include "pipe/p_shader_tokens.h"
#include "pipe/p_compiler.h"

namespace llvm {
   class Module;
}

#if defined __cplusplus
extern "C" {
#endif

enum gallivm_shader_type;
enum gallivm_vector_layout;

struct gallivm_interpolate {
   int attrib;
   int chan;
   int type;
};

struct gallivm_ir {
   llvm::Module *module;
   int id;
   enum gallivm_shader_type type;
   enum gallivm_vector_layout layout;
   int num_components;
   int   num_consts;

   /* FIXME: this might not be enough for some shaders */
   struct gallivm_interpolate interpolators[32*4];
   int   num_interp;
};

struct gallivm_prog {
   llvm::Module *module;
   void *function;

   int   id;
   enum gallivm_shader_type type;

   int   num_consts;

   /* FIXME: this might not be enough for some shaders */
   struct gallivm_interpolate interpolators[32*4];
   int   num_interp;
};

static INLINE void gallivm_swizzle_components(int swizzle,
                                              int *xc, int *yc,
                                              int *zc, int *wc)
{
   int x = swizzle / 1000; swizzle -= x * 1000;
   int y = swizzle / 100;  swizzle -= y * 100;
   int z = swizzle / 10;   swizzle -= z * 10;
   int w = swizzle;

   if (xc) *xc = x;
   if (yc) *yc = y;
   if (zc) *zc = z;
   if (wc) *wc = w;
}

static INLINE boolean gallivm_is_swizzle(int swizzle)
{
   const int NO_SWIZZLE = TGSI_SWIZZLE_X * 1000 + TGSI_SWIZZLE_Y * 100 +
                          TGSI_SWIZZLE_Z * 10 + TGSI_SWIZZLE_W;
   return swizzle != NO_SWIZZLE;
}

static INLINE int gallivm_x_swizzle(int swizzle)
{
   int x;
   gallivm_swizzle_components(swizzle, &x, 0, 0, 0);
   return x;
}

static INLINE int gallivm_y_swizzle(int swizzle)
{
   int y;
   gallivm_swizzle_components(swizzle, 0, &y, 0, 0);
   return y;
}

static INLINE int gallivm_z_swizzle(int swizzle)
{
   int z;
   gallivm_swizzle_components(swizzle, 0, 0, &z, 0);
   return z;
}

static INLINE int gallivm_w_swizzle(int swizzle)
{
   int w;
   gallivm_swizzle_components(swizzle, 0, 0, 0, &w);
   return w;
}

#endif /* MESA_LLVM */

#if defined __cplusplus
}
#endif

#endif
