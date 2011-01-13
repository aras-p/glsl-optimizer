#include "glsl_optimizer.h"
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "ir_print_glsl_visitor.h"
#include "ir_print_visitor.h"
#include "loop_analysis.h"
#include "program.h"

extern "C" struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type);

struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = talloc_zero(NULL, struct gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      shader->RefCount = 1;
   }
   return shader;
}


static void
initialize_mesa_context(struct gl_context *ctx, gl_api api)
{
   memset(ctx, 0, sizeof(*ctx));

   ctx->API = api;

   ctx->Extensions.ARB_draw_buffers = GL_TRUE;
   ctx->Extensions.ARB_fragment_coord_conventions = GL_TRUE;
   ctx->Extensions.EXT_texture_array = GL_TRUE;
   ctx->Extensions.NV_texture_rectangle = GL_TRUE;

   /* 1.10 minimums. */
   ctx->Const.MaxLights = 8;
   ctx->Const.MaxClipPlanes = 8;
   ctx->Const.MaxTextureUnits = 2;

   /* More than the 1.10 minimum to appease parser tests taken from
    * apps that (hopefully) already checked the number of coords.
    */
   ctx->Const.MaxTextureCoordUnits = 4;

   ctx->Const.VertexProgram.MaxAttribs = 16;
   ctx->Const.VertexProgram.MaxUniformComponents = 512;
   ctx->Const.MaxVarying = 8;
   ctx->Const.MaxVertexTextureImageUnits = 0;
   ctx->Const.MaxCombinedTextureImageUnits = 2;
   ctx->Const.MaxTextureImageUnits = 2;
   ctx->Const.FragmentProgram.MaxUniformComponents = 64;

   ctx->Const.MaxDrawBuffers = 2;

   ctx->Driver.NewShader = _mesa_new_shader;
}


struct glslopt_ctx {
	glslopt_ctx (bool openglES) {
		mem_ctx = talloc_new (NULL);
		initialize_mesa_context (&mesa_ctx, openglES ? API_OPENGLES2 : API_OPENGL);
	}
	~glslopt_ctx() {
		talloc_free (mem_ctx);
	}
	struct gl_context mesa_ctx;
	void* mem_ctx;
};

glslopt_ctx* glslopt_initialize (bool openglES)
{
	return new glslopt_ctx(openglES);
}

void glslopt_cleanup (glslopt_ctx* ctx)
{
	delete ctx;
	_mesa_glsl_release_types();
	_mesa_glsl_release_functions();
}



struct glslopt_shader {
	static void* operator new(size_t size, void *ctx)
	{
		void *node;
		node = talloc_size(ctx, size);
		assert(node != NULL);
		return node;
	}
	static void operator delete(void *node)
	{
		talloc_free(node);
	}

	glslopt_shader ()
		: rawOutput(0)
		, optimizedOutput(0)
		, status(false)
	{
		infoLog = "Shader not compiled yet";
	}

	char*	rawOutput;
	char*	optimizedOutput;
	char*	infoLog;
	bool	status;
};

static inline void debug_print_ir (const char* name, exec_list* ir, _mesa_glsl_parse_state* state, void* memctx)
{
	#if 0
	printf("**** %s:\n", name);
	_mesa_print_ir (ir, state);
	//char* foobar = _mesa_print_ir_glsl(ir, state, talloc_strdup(memctx, ""), kPrintGlslFragment);
	//printf(foobar);
	validate_ir_tree(ir);
	#endif
}

glslopt_shader* glslopt_optimize (glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource, unsigned options)
{
	glslopt_shader* shader = new (ctx->mem_ctx) glslopt_shader ();

	GLenum glType = 0;
	PrintGlslMode printMode;
	switch (type) {
	case kGlslOptShaderVertex: glType = GL_VERTEX_SHADER; printMode = kPrintGlslVertex; break;
	case kGlslOptShaderFragment: glType = GL_FRAGMENT_SHADER; printMode = kPrintGlslFragment; break;
	}
	if (!glType)
	{
		shader->infoLog = talloc_asprintf (ctx->mem_ctx, "Unknown shader type %d", (int)type);
		shader->status = false;
		return shader;
	}

	_mesa_glsl_parse_state* state = new (ctx->mem_ctx) _mesa_glsl_parse_state (&ctx->mesa_ctx, glType, ctx->mem_ctx);
	state->error = 0;

	if (!(options & kGlslOptionSkipPreprocessor))
	{
		state->error = preprocess (state, &shaderSource, &state->info_log, state->extensions, ctx->mesa_ctx.API);
		if (state->error)
		{
			shader->status = !state->error;
			shader->infoLog = state->info_log;
			return shader;
		}
	}

	_mesa_glsl_lexer_ctor (state, shaderSource);
	_mesa_glsl_parse (state);
	_mesa_glsl_lexer_dtor (state);

	exec_list* ir = new (ctx->mem_ctx) exec_list();

	if (!state->error && !state->translation_unit.is_empty())
		_mesa_ast_to_hir (ir, state);

	// Un-optimized output
	if (!state->error) {
		validate_ir_tree(ir);
		shader->rawOutput = _mesa_print_ir_glsl(ir, state, talloc_strdup(ctx->mem_ctx, ""), printMode);
	}

	// Optimization passes
	const bool linked = !(options & kGlslOptionNotFullShader);
	if (!state->error && !ir->is_empty())
	{
		bool progress;
		do {
			progress = false;
			bool progress2;
			debug_print_ir ("Initial", ir, state, ctx->mem_ctx);
			progress2 = do_function_inlining(ir); progress |= progress2; if (progress2) debug_print_ir ("After inlining", ir, state, ctx->mem_ctx);
			progress2 = do_dead_functions(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead functions", ir, state, ctx->mem_ctx);
			progress2 = do_structure_splitting(ir); progress |= progress2; if (progress2) debug_print_ir ("After struct splitting", ir, state, ctx->mem_ctx);
			progress2 = do_if_simplification(ir); progress |= progress2; if (progress2) debug_print_ir ("After if simpl", ir, state, ctx->mem_ctx);
			progress2 = do_copy_propagation(ir); progress |= progress2; if (progress2) debug_print_ir ("After copy propagation", ir, state, ctx->mem_ctx);
			if (!linked) {
				progress2 = do_dead_code_unlinked(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead code unlinked", ir, state, ctx->mem_ctx);
			} else {
				progress2 = do_dead_code(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead code", ir, state, ctx->mem_ctx);
			}
			progress2 = do_dead_code_local(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead code local", ir, state, ctx->mem_ctx);
			progress2 = do_tree_grafting(ir); progress |= progress2; if (progress2) debug_print_ir ("After tree grafting", ir, state, ctx->mem_ctx);
			progress2 = do_constant_propagation(ir); progress |= progress2; if (progress2) debug_print_ir ("After const propagation", ir, state, ctx->mem_ctx);
			if (!linked) {
				progress2 = do_constant_variable_unlinked(ir); progress |= progress2; if (progress2) debug_print_ir ("After const variable unlinked", ir, state, ctx->mem_ctx);
			} else {
				progress2 = do_constant_variable(ir); progress |= progress2; if (progress2) debug_print_ir ("After const variable", ir, state, ctx->mem_ctx);
			}
			progress2 = do_constant_folding(ir); progress |= progress2; if (progress2) debug_print_ir ("After const folding", ir, state, ctx->mem_ctx);
			progress2 = do_algebraic(ir); progress |= progress2; if (progress2) debug_print_ir ("After algebraic", ir, state, ctx->mem_ctx);
			progress2 = do_lower_jumps(ir); progress |= progress2; if (progress2) debug_print_ir ("After lower jumps", ir, state, ctx->mem_ctx);
			progress2 = do_vec_index_to_swizzle(ir); progress |= progress2; if (progress2) debug_print_ir ("After vec index to swizzle", ir, state, ctx->mem_ctx);
			//progress2 = do_vec_index_to_cond_assign(ir); progress |= progress2; if (progress2) debug_print_ir ("After vec index to cond assign", ir, state, ctx->mem_ctx);
			progress2 = do_swizzle_swizzle(ir); progress |= progress2; if (progress2) debug_print_ir ("After swizzle swizzle", ir, state, ctx->mem_ctx);
			progress2 = do_noop_swizzle(ir); progress |= progress2; if (progress2) debug_print_ir ("After noop swizzle", ir, state, ctx->mem_ctx);
			progress2 = optimize_redundant_jumps(ir); progress |= progress2; if (progress2) debug_print_ir ("After redundant jumps", ir, state, ctx->mem_ctx);

			loop_state *ls = analyze_loop_variables(ir);
			progress2 = set_loop_controls(ir, ls); progress |= progress2;
			progress2 = unroll_loops(ir, ls, 8); progress |= progress2;
			delete ls;
		} while (progress);

		validate_ir_tree(ir);
	}

	// Final optimized output
	if (!state->error)
	{
		shader->optimizedOutput = _mesa_print_ir_glsl(ir, state, talloc_strdup(ctx->mem_ctx, ""), printMode);
	}

	shader->status = !state->error;
	shader->infoLog = state->info_log;

	talloc_free (ir);
	talloc_free (state);

	return shader;
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
