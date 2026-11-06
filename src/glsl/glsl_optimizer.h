#pragma once
#ifndef GLSL_OPTIMIZER_H
#define GLSL_OPTIMIZER_H

/*
 Main GLSL optimizer interface.
 See ../../README.md for more instructions.

 General usage:

 ctx = glslopt_initialize();
 for (lots of shaders) {
   shader = glslopt_optimize (ctx, shaderType, shaderSource, options);
   if (glslopt_get_status (shader)) {
     newSource = glslopt_get_output (shader);
   } else {
     errorLog = glslopt_get_log (shader);
   }
   glslopt_shader_delete (shader);
 }
 glslopt_cleanup (ctx);
*/

struct glslopt_shader;
struct glslopt_ctx;

enum glslopt_shader_type {
	kGlslOptShaderVertex = 0,
	kGlslOptShaderFragment,
	kGlslOptShaderCompute,
};

// Options flags for glsl_optimize
enum glslopt_options {
	kGlslOptionSkipPreprocessor = (1<<0), // Skip preprocessing shader source. Saves some time if you know you don't need it.
	kGlslOptionNotFullShader = (1<<1), // Passed shader is not the full shader source. This makes some optimizations weaker.
};

// Optimizer target language
enum glslopt_target {
	kGlslTargetOpenGL = 0,
	kGlslTargetOpenGLES30 = 2,
	kGlslTargetMetal = 3,
};

// Type info
enum glslopt_basic_type {
	kGlslTypeFloat = 0,
	kGlslTypeInt,
	kGlslTypeBool,
	kGlslTypeTex2D,
	kGlslTypeTex3D,
	kGlslTypeTexCube,
	kGlslTypeTex2DShadow,
	kGlslTypeTex2DArray,
	kGlslTypeOther,
	kGlslTypeImage2D,
	kGlslTypeImage2DArray,
	kGlslTypeImage3D,
	kGlslTypeImage1D,
	kGlslTypeImage1DArray,
	kGlslTypeImageCube,
	kGlslTypeImageCubeArray,
	kGlslTypeImageBuffer,
	kGlslTypeImage2DMS,
	kGlslTypeImage2DMSArray,
	kGlslTypeCount
};
enum glslopt_precision {
	kGlslPrecHigh = 0,
	kGlslPrecMedium,
	kGlslPrecLow, // never happens anymore, retained only for API compatibility
	kGlslPrecCount
};

glslopt_ctx* glslopt_initialize (glslopt_target target);
void glslopt_cleanup (glslopt_ctx* ctx);

void glslopt_set_max_unroll_iterations (glslopt_ctx* ctx, unsigned iterations);

glslopt_shader* glslopt_optimize (glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource, unsigned options);
bool glslopt_get_status (glslopt_shader* shader);
const char* glslopt_get_output (glslopt_shader* shader);
const char* glslopt_get_raw_output (glslopt_shader* shader);
const char* glslopt_get_log (glslopt_shader* shader);
void glslopt_shader_delete (glslopt_shader* shader);

// Fixed compute workgroup dimensions, or zero for non-compute shaders.
void glslopt_shader_get_compute_local_size(glslopt_shader* shader, int* x, int* y, int* z);

int glslopt_shader_get_input_count (glslopt_shader* shader);
void glslopt_shader_get_input_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation);
int glslopt_shader_get_uniform_count (glslopt_shader* shader);
int glslopt_shader_get_uniform_total_size (glslopt_shader* shader);
void glslopt_shader_get_uniform_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation);
// Includes sampled textures and storage images. See the README for location semantics.
int glslopt_shader_get_texture_count (glslopt_shader* shader);
void glslopt_shader_get_texture_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation);

enum glslopt_image_component_type {
	kGlslImageComponentUnknown = 0,
	kGlslImageComponentFloat,
	kGlslImageComponentInt,
	kGlslImageComponentUInt,
};
struct glslopt_image_desc {
	const char* format; // GLSL layout name (e.g. "rgba8" or "r32ui"), NULL if unspecified.
	glslopt_image_component_type componentType; // Image operation type. Normalized formats use float.
	bool readOnly;
	bool writeOnly;
};
// Index uses the texture list above. Returns NULL for samplers and invalid indices.
// Access flags reflect the qualifiers. Neither flag means read/write, both means query only.
// The descriptor remains valid until glslopt_shader_delete.
const glslopt_image_desc* glslopt_shader_get_image_desc(glslopt_shader* shader, int textureIndex);

// Buffer names and descriptors remain valid until glslopt_shader_delete.
enum glslopt_buffer_kind { kGlslBufferUniform, kGlslBufferStorage, kGlslBufferAtomicCounter };
enum glslopt_buffer_layout { kGlslLayoutStd140, kGlslLayoutShared, kGlslLayoutPacked, kGlslLayoutStd430, kGlslLayoutAtomicCounter };
struct glslopt_buffer_desc {
	const char* name;
	const char* instanceName; // NULL for anonymous blocks
	glslopt_buffer_kind kind;
	glslopt_buffer_layout layout;
	int binding;
	int arraySize; // 1 for non-array blocks; size is per array element
	int size; // allocation size rounded to block alignment; -1 for unknown/runtime-sized storage
	int metalBufferIndex; // -1 for GLSL targets
	int metalSizeBufferIndex; // Byte-size table slot, or -1. See the README for the table layout.
};
// Buffer members form a tree in declaration order; parentIndex=-1 is for block fields.
// Array nodes have one child describing element zero, with name "[]".
// Offsets are relative to the block; add index * arrayStride for each array ancestor.
// Struct nodes have field children.
// Scalars/vectors/matrices are leaves.
struct glslopt_buffer_member_desc {
	const char* name;
	const char* typeName; // GLSL type name, preserving uint vs int
	int parentIndex;
	int offset;
	int arraySize; // 0 for non-array nodes; -1 for runtime-sized arrays
	int arrayStride; // 0 when inapplicable
	int matrixStride; // 0 when inapplicable
	bool rowMajor; // true only for row-major matrix leaves
	bool used; // referenced by optimized IR (reads or writes); arrays combine usage across elements
	bool readOnly; // SSBO member access qualifiers, inherited by descendants
	bool writeOnly;
};
int glslopt_shader_get_buffer_count(glslopt_shader* shader);
const glslopt_buffer_desc* glslopt_shader_get_buffer_desc(glslopt_shader* shader, int index);
int glslopt_shader_get_buffer_member_count(glslopt_shader* shader, int index);
const glslopt_buffer_member_desc* glslopt_shader_get_buffer_member_desc(glslopt_shader* shader, int blockIndex, int memberIndex);

// Get *very* approximate shader stats:
// Number of math, texture and flow control instructions.
void glslopt_shader_get_stats (glslopt_shader* shader, int* approxMath, int* approxTex, int* approxFlow);


#endif /* GLSL_OPTIMIZER_H */
