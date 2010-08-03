#pragma once
#ifndef GLSL_OPTIMIZER_H
#define GLSL_OPTIMIZER_H

struct glslopt_shader;
struct glslopt_ctx;

enum glslopt_shader_type {
	kGlslOptShaderVertex = 0,
	kGlslOptShaderFragment,
};

glslopt_ctx* glslopt_initialize ();
void glslopt_cleanup (glslopt_ctx* ctx);

glslopt_shader* glslopt_optimize (glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource);
bool glslopt_get_status (glslopt_shader* shader);
const char* glslopt_get_output (glslopt_shader* shader);
const char* glslopt_get_raw_output (glslopt_shader* shader);
const char* glslopt_get_log (glslopt_shader* shader);
void glslopt_shader_delete (glslopt_shader* shader);


#endif /* GLSL_OPTIMIZER_H */
