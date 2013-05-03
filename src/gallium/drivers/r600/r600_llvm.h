
#ifndef R600_LLVM_H
#define R600_LLVM_H

#if defined R600_USE_LLVM || defined HAVE_OPENCL

#include "radeon_llvm.h"
#include <llvm-c/Core.h>

struct r600_bytecode;
struct r600_shader_ctx;
struct radeon_llvm_context;
enum radeon_family;

LLVMModuleRef r600_tgsi_llvm(
	struct radeon_llvm_context * ctx,
	const struct tgsi_token * tokens);

unsigned r600_llvm_compile(
	LLVMModuleRef mod,
	enum radeon_family family,
	struct r600_bytecode *bc,
	boolean *use_kill,
	unsigned dump);

#endif /* defined R600_USE_LLVM || defined HAVE_OPENCL */

#endif /* R600_LLVM_H */
