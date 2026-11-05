#pragma once

#include "ir.h"
#include <cstring>

struct image_atomic_function {
   ir_intrinsic_id intrinsic;
   const char* glsl;
   const char* metal;
};

inline const image_atomic_function* find_image_atomic(const ir_function_signature* signature)
{
   if (!signature->is_builtin()) return nullptr;
   bool intrinsic = signature->is_intrinsic();
   if (intrinsic) {
      if (signature->intrinsic_id < ir_intrinsic_image_atomic_add ||
          signature->intrinsic_id > ir_intrinsic_image_atomic_comp_swap) return nullptr;
   } else if (strncmp(signature->function_name(), "imageAtomic", 11) != 0) {
      return nullptr;
   }
   static const image_atomic_function functions[] = {
      {ir_intrinsic_image_atomic_add, "imageAtomicAdd", "atomic_fetch_add"},
      {ir_intrinsic_image_atomic_min, "imageAtomicMin", "atomic_fetch_min"},
      {ir_intrinsic_image_atomic_max, "imageAtomicMax", "atomic_fetch_max"},
      {ir_intrinsic_image_atomic_and, "imageAtomicAnd", "atomic_fetch_and"},
      {ir_intrinsic_image_atomic_or, "imageAtomicOr", "atomic_fetch_or"},
      {ir_intrinsic_image_atomic_xor, "imageAtomicXor", "atomic_fetch_xor"},
      {ir_intrinsic_image_atomic_exchange, "imageAtomicExchange", "atomic_exchange"},
      {ir_intrinsic_image_atomic_comp_swap, "imageAtomicCompSwap", nullptr},
   };
   for (const auto& f : functions)
      if (intrinsic ? signature->intrinsic_id == f.intrinsic : strcmp(signature->function_name(), f.glsl) == 0) return &f;
   return nullptr;
}

inline const char* image_function_name(const ir_function_signature* signature)
{
   if (!signature->is_builtin()) return nullptr;
   if (const auto* atomic = find_image_atomic(signature)) return atomic->glsl;
   if (signature->is_intrinsic()) {
      switch (signature->intrinsic_id) {
      case ir_intrinsic_image_load: return "imageLoad";
      case ir_intrinsic_image_store: return "imageStore";
      case ir_intrinsic_image_size: return "imageSize";
      default: return nullptr;
      }
   }
   const char* name = signature->function_name();
   return !strcmp(name, "imageLoad") || !strcmp(name, "imageStore") || !strcmp(name, "imageSize") ? name : nullptr;
}
