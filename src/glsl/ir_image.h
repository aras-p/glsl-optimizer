#pragma once

#include "ir.h"
#include <cstring>

inline const char* image_function_name(const ir_function_signature* signature)
{
   if (!signature->is_builtin()) return nullptr;
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
