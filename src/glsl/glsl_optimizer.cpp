#include "glsl_optimizer.h"
#include "main/mtypes.h"
#include "ir.h"
#include "ir_optimization.h"
#include "builtin_functions.h"
#include "string_to_uint_map.h"
#include "ir_stats.h"
#include "ir_print_glsl_visitor.h"
#include "ir_print_metal_visitor.h"
#include "opt_add_neg_to_sub.h"
#include "glsl_parser_extras.h"
#include "loop_analysis.h"
#include "program.h"
#include "linker.h"
#include "standalone_scaffolding.h"


static gl_program* new_program(gl_context*, gl_shader_stage stage, GLuint id, bool is_arb_asm)
{
	gl_program* program = rzalloc(NULL, gl_program);
	program->RefCount = 1;
	program->Id = id;
	program->Format = GL_PROGRAM_FORMAT_ASCII_ARB;
	program->info.stage = stage;
	program->is_arb_asm = is_arb_asm;
	return program;
}

static void
initialize_mesa_context(struct gl_context *ctx, glslopt_target api)
{
	gl_api mesaAPI;
	switch(api)
	{
		default:
		case kGlslTargetOpenGL:
			mesaAPI = API_OPENGL_COMPAT;
			break;
		case kGlslTargetOpenGLES30:
			mesaAPI = API_OPENGL_CORE;
			break;
		case kGlslTargetMetal:
			mesaAPI = API_OPENGL_CORE;
			break;
	}
	initialize_context_to_defaults (ctx, mesaAPI);

	switch(api)
	{
	default:
	case kGlslTargetOpenGL:
		ctx->Const.GLSLVersion = 150;
		break;
	case kGlslTargetOpenGLES30:
		ctx->Extensions.ARB_ES3_compatibility = true;
		ctx->Extensions.EXT_shader_framebuffer_fetch = true;
		break;
	case kGlslTargetMetal:
		ctx->Extensions.ARB_ES3_compatibility = true;
		ctx->Extensions.EXT_shader_framebuffer_fetch = true;
		break;
	}


   // allow high amount of texcoords
   ctx->Const.MaxTextureCoordUnits = 16;

   ctx->Const.Program[MESA_SHADER_VERTEX].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxTextureImageUnits = 16;
   ctx->Const.Program[MESA_SHADER_GEOMETRY].MaxTextureImageUnits = 16;

   // Note: use lower unroll iterations than Mesa default
   for (auto& options : ctx->Const.ShaderCompilerOptions)
      options.MaxUnrollIterations = 8;

   ctx->Const.MaxDrawBuffers = 4;
   ctx->Const.MaxVertexStreams = 1;
   ctx->Const.MaxTransformFeedbackBuffers = 4;
   ctx->Const.MaxTransformFeedbackSeparateComponents = 64;
   ctx->Const.MaxTransformFeedbackInterleavedComponents = 64;
   ctx->Const.Program[MESA_SHADER_VERTEX].MaxUniformBlocks = 12;
   ctx->Const.Program[MESA_SHADER_FRAGMENT].MaxUniformBlocks = 12;

   ctx->Driver.NewProgram = new_program;
}


struct glslopt_ctx {
	glslopt_ctx (glslopt_target target) {
		this->target = target;
		initialize_mesa_context (&mesa_ctx, target);
		_mesa_glsl_builtin_functions_init_or_ref();
	}
	~glslopt_ctx() {
		_mesa_glsl_builtin_functions_decref();
	}
	struct gl_context mesa_ctx;
	glslopt_target target;
};

glslopt_ctx* glslopt_initialize (glslopt_target target)
{
	if (target != kGlslTargetOpenGL && target != kGlslTargetOpenGLES30 && target != kGlslTargetMetal)
		return NULL;
	return new glslopt_ctx(target);
}

void glslopt_cleanup (glslopt_ctx* ctx)
{
	delete ctx;
}

void glslopt_set_max_unroll_iterations (glslopt_ctx* ctx, unsigned iterations)
{
	for (int i = 0; i < MESA_SHADER_STAGES; ++i)
		ctx->mesa_ctx.Const.ShaderCompilerOptions[i].MaxUnrollIterations = iterations;
}

struct glslopt_shader_var
{
	const char* name;
	glslopt_basic_type type;
	glslopt_precision prec;
	int vectorSize;
	int matrixSize;
	int arraySize;
	int location;
};

struct glslopt_shader
{
	glslopt_shader ()
		: rawOutput(0)
		, optimizedOutput(0)
		, status(false)
		, uniformCount(0)
		, uniformsSize(0)
		, inputCount(0)
		, textureCount(0)
		, statsMath(0)
		, statsTex(0)
		, statsFlow(0)
	{
		infoLog = "Shader not compiled yet";
		
		// Results own their arena and keep shared types/builtins alive independently
		// of the context, including when another context is destroyed.
		_mesa_glsl_builtin_functions_init_or_ref();
		mem_ctx = ralloc_context(NULL);
		whole_program = rzalloc(mem_ctx, gl_shader_program);
		whole_program->data = rzalloc(whole_program, gl_shader_program_data);
		whole_program->data->InfoLog = ralloc_strdup(whole_program->data, "");
		whole_program->data->LinkStatus = LINKING_SUCCESS;
		whole_program->AttributeBindings = new string_to_uint_map;
		whole_program->FragDataBindings = new string_to_uint_map;
		whole_program->FragDataIndexBindings = new string_to_uint_map;
		whole_program->Shaders = ralloc_array(whole_program, gl_shader*, 1);
		shader = rzalloc(whole_program, gl_shader);
		shader->RefCount = 1;
		whole_program->Shaders[0] = shader;
		whole_program->NumShaders = 1;
	}

	~glslopt_shader()
	{
		delete whole_program->AttributeBindings;
		delete whole_program->FragDataBindings;
		delete whole_program->FragDataIndexBindings;
		ralloc_free(mem_ctx);
		_mesa_glsl_builtin_functions_decref();
	}

	void* mem_ctx;
	struct gl_shader_program* whole_program;
	struct gl_shader* shader;

	static const int kMaxShaderUniforms = 1024;
	static const int kMaxShaderInputs = 128;
	static const int kMaxShaderTextures = 128;
	glslopt_shader_var uniforms[kMaxShaderUniforms];
	glslopt_shader_var inputs[kMaxShaderInputs];
	glslopt_shader_var textures[kMaxShaderInputs];
	int uniformCount, uniformsSize;
	int inputCount;
	int textureCount;
	int statsMath, statsTex, statsFlow;

	char*	rawOutput;
	char*	optimizedOutput;
	const char*	infoLog;
	bool	status;
};

static void do_optimization_passes(exec_list* ir, bool linked, glslopt_ctx* ctx, gl_shader_stage stage)
{
	bool progress;
	// FIXME: Shouldn't need to bound the number of passes
	int passes = 0,
		kMaximumPasses = 1000;
	do {
		progress = propagate_precision(ir);
		++passes;
		if (linked) {
			progress |= do_function_inlining(ir);
			progress |= do_dead_functions(ir);
			progress |= do_structure_splitting(ir);
		}
		progress |= propagate_precision(ir);
		progress |= do_if_simplification(ir);
		progress |= opt_flatten_nested_if_blocks(ir);
		progress |= do_copy_propagation_elements(ir);

		if (linked)
		{
			progress |= do_vectorize(ir);
		}
		if (linked) {
			progress |= do_dead_code(ir,false);
		} else {
			progress |= do_dead_code_unlinked(ir);
		}
		progress |= do_dead_code_local(ir);
		progress |= do_tree_grafting(ir);
		progress |= do_constant_propagation(ir);
		if (linked) {
			progress |= do_constant_variable(ir);
		} else {
			progress |= do_constant_variable_unlinked(ir);
		}
		progress |= do_constant_folding(ir);
		progress |= do_minmax_prune(ir);
		progress |= do_cse(ir);
		progress |= do_rebalance_tree(ir);
		progress |= do_algebraic(ir, ctx->mesa_ctx.Const.NativeIntegers, &ctx->mesa_ctx.Const.ShaderCompilerOptions[stage]);
		progress |= do_lower_jumps(ir);
		progress |= do_vec_index_to_swizzle(ir);
		progress |= lower_vector_insert(ir, false);
		progress |= optimize_swizzles(ir);
		progress |= optimize_split_arrays(ir, linked, ctx->target == kGlslTargetMetal && stage == MESA_SHADER_FRAGMENT);
		progress |= optimize_redundant_jumps(ir);

		// do loop stuff only when linked; otherwise causes duplicate loop induction variable
		// problems (ast-in.txt test)
		if (linked)
		{
			loop_state *ls = analyze_loop_variables(ir);
			if (ls->loop_found) {
				progress |= unroll_loops(ir, ls, &ctx->mesa_ctx.Const.ShaderCompilerOptions[stage]);
			}
			delete ls;
		}
	} while (progress && passes < kMaximumPasses);

	if (ctx->target != kGlslTargetMetal)
	{
		// GLSL/ES does not have saturate, so lower it
		lower_instructions(ir, SAT_TO_CLAMP);
	}
}

static void glsl_type_to_optimizer_desc(const glsl_type* type, unsigned prec, glslopt_shader_var* out)
{
	out->arraySize = type->array_size();

	// type; use element type when in array
	if (type->is_array())
		type = type->without_array();

	if (type->is_float())
		out->type = kGlslTypeFloat;
	else if (type->is_integer_32())
		out->type = kGlslTypeInt;
	else if (type->is_boolean())
		out->type = kGlslTypeBool;
	else if (type->is_sampler())
	{
		if (type->sampler_dimensionality == GLSL_SAMPLER_DIM_2D)
		{
			if (type->sampler_shadow)
				out->type = kGlslTypeTex2DShadow;
			else if (type->sampler_array)
				out->type = kGlslTypeTex2DArray;
			else
				out->type = kGlslTypeTex2D;
		}
		else if (type->sampler_dimensionality == GLSL_SAMPLER_DIM_3D)
			out->type = kGlslTypeTex3D;
		else if (type->sampler_dimensionality == GLSL_SAMPLER_DIM_CUBE)
			out->type = kGlslTypeTexCube;
		else
			out->type = kGlslTypeOther;
	}
	else
		out->type = kGlslTypeOther;
	
	// sizes
	out->vectorSize = type->is_sampler() ? 0 : type->vector_elements;
	out->matrixSize = type->is_sampler() ? 0 : type->matrix_columns;
	
	// precision
	switch (prec)
	{
		case GLSL_PRECISION_HIGH: out->prec = kGlslPrecHigh; break;
		case GLSL_PRECISION_MEDIUM: out->prec = kGlslPrecMedium; break;
		default: out->prec = kGlslPrecHigh; break;
	}
}

static void find_shader_variables(glslopt_shader* sh, exec_list* ir)
{
	foreach_in_list(ir_instruction, node, ir)
	{
		ir_variable* const var = node->as_variable();
		if (var == NULL)
			continue;
		if (var->data.mode == ir_var_shader_in)
		{
			if (sh->inputCount >= glslopt_shader::kMaxShaderInputs)
				continue;

			glslopt_shader_var& v = sh->inputs[sh->inputCount];
			v.name = ralloc_strdup(sh->mem_ctx, var->name);
			glsl_type_to_optimizer_desc(var->type, var->data.precision, &v);
			v.location = var->data.explicit_location ? var->data.location : -1;
			++sh->inputCount;
		}
		if (var->data.mode == ir_var_uniform && !var->type->without_array()->is_sampler())
		{
			if (sh->uniformCount >= glslopt_shader::kMaxShaderUniforms)
				continue;
			
			glslopt_shader_var& v = sh->uniforms[sh->uniformCount];
			v.name = ralloc_strdup(sh->mem_ctx, var->name);
			glsl_type_to_optimizer_desc(var->type, var->data.precision, &v);
			v.location = var->data.explicit_location ? var->data.location : -1;
			++sh->uniformCount;
		}
		if (var->data.mode == ir_var_uniform && var->type->without_array()->is_sampler())
		{
			if (sh->textureCount >= glslopt_shader::kMaxShaderTextures)
				continue;
			
			glslopt_shader_var& v = sh->textures[sh->textureCount];
			v.name = ralloc_strdup(sh->mem_ctx, var->name);
			glsl_type_to_optimizer_desc(var->type, var->data.precision, &v);
			v.location = var->data.explicit_location ? var->data.location : -1;
			++sh->textureCount;
		}
	}
}


glslopt_shader* glslopt_optimize (glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource, unsigned options)
{
	if (!ctx)
		return NULL;
	glslopt_shader* result = new glslopt_shader;
	gl_shader* shader = result->shader;
	switch (type) {
	case kGlslOptShaderVertex: shader->Stage = MESA_SHADER_VERTEX; break;
	case kGlslOptShaderFragment: shader->Stage = MESA_SHADER_FRAGMENT; break;
	default:
		result->infoLog = ralloc_asprintf(result->mem_ctx, "Unknown shader type %d", (int)type);
		return result;
	}
	if (!shaderSource) {
		result->infoLog = "Shader source is null";
		return result;
	}
	shader->Source = ralloc_strdup(shader, shaderSource);
	_mesa_glsl_parse_state* state = nullptr;
	_mesa_glsl_compile_shader(&ctx->mesa_ctx, shader, false, false,
		(options & kGlslOptionSkipPreprocessor) != 0, &state);
	result->infoLog = shader->InfoLog;
	if (shader->CompileStatus == COMPILE_FAILURE)
		return result;
	if (ctx->target != kGlslTargetMetal && shader->IsES && shader->Version < 300) {
		result->infoLog = "OpenGL ES 2.0 output is no longer supported; use #version 300 es or newer.";
		return result;
	}

	exec_list* ir = shader->ir;

	// Note: restore source-level subtraction outside of regular optimization loop
    // (inside the loop, other passes would undo it).
	add_neg_to_sub_visitor source_arithmetic;
	visit_list_elements(&source_arithmetic, ir);

	const PrintGlslMode printMode = type == kGlslOptShaderVertex ? kPrintGlslVertex : kPrintGlslFragment;
	if (ctx->target != kGlslTargetMetal)
		result->rawOutput = _mesa_print_ir_glsl(ir, state, ralloc_strdup(result->mem_ctx, ""), printMode);
	else
		result->rawOutput = _mesa_print_ir_metal(ir, state, ralloc_strdup(result->mem_ctx, ""), printMode, &result->uniformsSize);
	const bool linked = !(options & kGlslOptionNotFullShader);
	if (linked) {
		gl_linked_shader* linked_shader = link_intrastage_shaders(result->mem_ctx,
			&ctx->mesa_ctx, result->whole_program, result->whole_program->Shaders, 1, false);
		if (!linked_shader) {
			result->infoLog = result->whole_program->data->InfoLog;
			return result;
		}
		result->whole_program->_LinkedShaders[shader->Stage] = linked_shader;
		ir = linked_shader->ir;
	}
	do_optimization_passes(ir, linked, ctx, shader->Stage);
	validate_ir_tree(ir);
	visit_list_elements(&source_arithmetic, ir);
	if (ctx->target != kGlslTargetMetal)
		result->optimizedOutput = _mesa_print_ir_glsl(ir, state, ralloc_strdup(result->mem_ctx, ""), printMode);
	else
		result->optimizedOutput = _mesa_print_ir_metal(ir, state, ralloc_strdup(result->mem_ctx, ""), printMode, &result->uniformsSize);
	find_shader_variables(result, ir);
	calculate_shader_stats(ir, &result->statsMath, &result->statsTex, &result->statsFlow);
	result->status = true;
	return result;
}

void glslopt_shader_delete (glslopt_shader* shader)
{
	delete shader;
}

bool glslopt_get_status (glslopt_shader* shader)
{
	return shader->status;
}

const char* glslopt_get_output (glslopt_shader* shader)
{
	return shader->optimizedOutput;
}

const char* glslopt_get_raw_output (glslopt_shader* shader)
{
	return shader->rawOutput;
}

const char* glslopt_get_log (glslopt_shader* shader)
{
	return shader->infoLog;
}

int glslopt_shader_get_input_count (glslopt_shader* shader)
{
	return shader->inputCount;
}

int glslopt_shader_get_uniform_count (glslopt_shader* shader)
{
	return shader->uniformCount;
}

int glslopt_shader_get_uniform_total_size (glslopt_shader* shader)
{
	return shader->uniformsSize;
}

int glslopt_shader_get_texture_count (glslopt_shader* shader)
{
	return shader->textureCount;
}

void glslopt_shader_get_input_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
	const glslopt_shader_var& v = shader->inputs[index];
	*outName = v.name;
	*outType = v.type;
	*outPrec = v.prec;
	*outVecSize = v.vectorSize;
	*outMatSize = v.matrixSize;
	*outArraySize = v.arraySize;
	*outLocation = v.location;
}

void glslopt_shader_get_uniform_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
	const glslopt_shader_var& v = shader->uniforms[index];
	*outName = v.name;
	*outType = v.type;
	*outPrec = v.prec;
	*outVecSize = v.vectorSize;
	*outMatSize = v.matrixSize;
	*outArraySize = v.arraySize;
	*outLocation = v.location;
}

void glslopt_shader_get_texture_desc (glslopt_shader* shader, int index, const char** outName, glslopt_basic_type* outType, glslopt_precision* outPrec, int* outVecSize, int* outMatSize, int* outArraySize, int* outLocation)
{
	const glslopt_shader_var& v = shader->textures[index];
	*outName = v.name;
	*outType = v.type;
	*outPrec = v.prec;
	*outVecSize = v.vectorSize;
	*outMatSize = v.matrixSize;
	*outArraySize = v.arraySize;
	*outLocation = v.location;
}

void glslopt_shader_get_stats (glslopt_shader* shader, int* approxMath, int* approxTex, int* approxFlow)
{
	*approxMath = shader->statsMath;
	*approxTex = shader->statsTex;
	*approxFlow = shader->statsFlow;
}
