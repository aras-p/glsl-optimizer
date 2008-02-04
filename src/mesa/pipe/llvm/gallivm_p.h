#ifndef GALLIVM_P_H
#define GALLIVM_P_H

#ifdef MESA_LLVM

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

   //FIXME: this might not be enough for some shaders
   struct gallivm_interpolate interpolators[32*4];
   int   num_interp;
};

struct gallivm_prog {
   llvm::Module *module;
   void *function;

   int   id;
   enum gallivm_shader_type type;

   int   num_consts;

   //FIXME: this might not be enough for some shaders
   struct gallivm_interpolate interpolators[32*4];
   int   num_interp;
};

#endif /* MESA_LLVM */

#if defined __cplusplus
} // extern "C"
#endif

#endif
