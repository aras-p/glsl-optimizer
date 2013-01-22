#include "glsl_optimizer.h"
#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "ir_print_glsl_visitor.h"
#include "ir_print_agal_visitor.h"
#include "ir_expression_flattening.h"
#include "ir_print_visitor.h"
#include "loop_analysis.h"
#include "program.h"
#include "../../agalassembler/agal.h"
#include <string>

extern "C" struct gl_shader *
_mesa_new_shader(struct gl_context *ctx, GLuint name, GLenum type);


PrintGlslMode printMode;

static void
initialize_mesa_context(struct gl_context *ctx, gl_api api)
{
   memset(ctx, 0, sizeof(*ctx));

   ctx->API = api;

   ctx->Extensions.ARB_draw_buffers = GL_TRUE;
   ctx->Extensions.ARB_fragment_coord_conventions = GL_TRUE;
   ctx->Extensions.EXT_texture_array = GL_TRUE;
   ctx->Extensions.NV_texture_rectangle = GL_TRUE;
   ctx->Extensions.ARB_shader_texture_lod = GL_TRUE;

   // Enable opengl es extensions we care about here
   if (api == API_OPENGLES2)
   {
      ctx->Extensions.OES_standard_derivatives = GL_TRUE;
   }


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
		mem_ctx = ralloc_context (NULL);
		initialize_mesa_context (&mesa_ctx, openglES ? API_OPENGLES2 : API_OPENGL);
	}
	~glslopt_ctx() {
		ralloc_free (mem_ctx);
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
		node = ralloc_size(ctx, size);
		assert(node != NULL);
		return node;
	}
	static void operator delete(void *node)
	{
		ralloc_free(node);
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
	const char*	infoLog;
	bool	status;
};

static inline void debug_print_ir (const char* name, exec_list* ir, _mesa_glsl_parse_state* state, void* memctx)
{
	#if 0
	fprintf(stderr, "**** %s:\n", name);
	//_mesa_print_ir (ir, state);
	char* foobar = _mesa_print_ir_glsl(ir, state, ralloc_strdup(memctx, ""), kPrintGlslFragment);
	printf(foobar);
	validate_ir_tree(ir);
	#endif
}

static void propagate_precision_deref(ir_instruction *ir, void *data)
{
	ir_dereference_variable* der = ir->as_dereference_variable();
	if (der && der->get_precision() == glsl_precision_undefined && der->var->precision != glsl_precision_undefined)
	{
		der->set_precision ((glsl_precision)der->var->precision);
		*(bool*)data = true;
	}
	ir_swizzle* swz = ir->as_swizzle();
	if (swz && swz->get_precision() == glsl_precision_undefined && swz->val->get_precision() != glsl_precision_undefined)
	{
		swz->set_precision (swz->val->get_precision());
		*(bool*)data = true;
	}
	
}

static void propagate_precision_assign(ir_instruction *ir, void *data)
{
	ir_assignment* ass = ir->as_assignment();
	if (ass && ass->lhs && ass->rhs)
	{
		glsl_precision lp = ass->lhs->get_precision();
		glsl_precision rp = ass->rhs->get_precision();
		if (rp == glsl_precision_undefined)
			return;
		ir_variable* lhs_var = ass->lhs->variable_referenced();
		if (lp == glsl_precision_undefined)
		{		
			if (lhs_var)
				lhs_var->precision = rp;
			ass->lhs->set_precision (rp);
			*(bool*)data = true;
		}
	}
}

static void propagate_precision_call(ir_instruction *ir, void *data)
{
	ir_call* call = ir->as_call();
	if (!call)
		return;
	if (call->get_precision() == glsl_precision_undefined /*&& call->get_callee()->precision == glsl_precision_undefined*/)
	{
		glsl_precision prec_params_max = glsl_precision_undefined;
		exec_list_iterator iter_sig  = call->get_callee()->parameters.iterator();
		foreach_iter(exec_list_iterator, iter_param, call->actual_parameters)
		{
			ir_variable* sig_param = (ir_variable*)iter_sig.get();
			ir_rvalue* param = (ir_rvalue*)iter_param.get();
			
			glsl_precision p = (glsl_precision)sig_param->precision;
			if (p == glsl_precision_undefined)
				p = param->get_precision();
			
			prec_params_max = higher_precision (prec_params_max, p);
			
			iter_sig.next();
		}
		if (call->get_precision() != prec_params_max)
		{
			call->set_precision (prec_params_max);
			*(bool*)data = true;
		}
	}
}

static bool propagate_precision(exec_list* list)
{
	bool anyProgress = false;
	bool res;
	do {
		res = false;
		foreach_iter(exec_list_iterator, iter, *list) {
			ir_instruction* ir = (ir_instruction*)iter.get();
			visit_tree (ir, propagate_precision_deref, &res);
			visit_tree (ir, propagate_precision_assign, &res);
			visit_tree (ir, propagate_precision_call, &res);
		}
		anyProgress |= res;
	} while (res);
	return anyProgress;
}

static bool emitComma = false;

class ir_type_printing_visitor : public ir_hierarchical_visitor {
public:
	glslopt_shader *shader;
   ir_type_printing_visitor(glslopt_shader* _shader)
   {
   		shader = _shader;
   }
   virtual ir_visitor_status visit(ir_variable *);
};

ir_visitor_status
ir_type_printing_visitor::visit(ir_variable *ir)
{
   ralloc_asprintf_append (&shader->optimizedOutput, "%c\"%s\":\"%s\"\n", emitComma ? ',' : ' ', ir->name, ir->type->name);
   emitComma = true;

   return visit_continue;
}

bool
do_print_types(exec_list *instructions, glslopt_shader* shader)
{
   ir_type_printing_visitor v(shader);
   v.run(instructions);
   return false;
}

class ir_storage_printing_visitor : public ir_hierarchical_visitor {
public:
	glslopt_shader *shader;
   ir_storage_printing_visitor(glslopt_shader* _shader)
   {
   		shader = _shader;
   }
   virtual ir_visitor_status visit(ir_variable *);
};

ir_visitor_status
ir_storage_printing_visitor::visit(ir_variable *ir)
{
   ralloc_asprintf_append (&shader->optimizedOutput, "%c\"%s\":\"%s\"\n", emitComma ? ',' : ' ', ir->name, ir_variable_mode_names[ir->mode]);
   emitComma = true;

   return visit_continue;
}

bool
do_print_storage(exec_list *instructions, glslopt_shader* shader)
{
   ir_storage_printing_visitor v(shader);
   v.run(instructions);
   return false;
}

class ir_constant_printing_visitor : public ir_hierarchical_visitor {
public:
	glslopt_shader *shader;
	int numConsts;
   ir_constant_printing_visitor(glslopt_shader* _shader)
   {
   		numConsts = 0;
   		shader = _shader;
   }
   virtual ir_visitor_status visit(ir_variable *);
};

ir_visitor_status
ir_constant_printing_visitor::visit(ir_variable *ir)
{
	ir_constant *c = ir->constant_value;

   if(!c)
   	return visit_continue;

   if(ir->mode == ir_var_out)
   	return visit_continue;

   if(c->type == glsl_type::vec4_type || c->type == glsl_type::vec3_type || c->type == glsl_type::vec2_type || c->type == glsl_type::float_type) {
   		int n = c->type->vector_elements;

		ralloc_asprintf_append (&shader->optimizedOutput, "%c\"%s\": [%f, %f, %f, %f]\n", emitComma ? ',' : ' ', ir->name,
		n > 0 ? ir->constant_value->get_float_component(0) : 0.0f,
		n > 1 ? ir->constant_value->get_float_component(1) : 0.0f,
		n > 2 ? ir->constant_value->get_float_component(2) : 0.0f,
		n > 3 ? ir->constant_value->get_float_component(3) : 0.0f);
		numConsts++;
   } else {
   	ralloc_asprintf_append (&shader->optimizedOutput, "%c\"%s\": UNHANDLED_CONST_TYPE\n", emitComma ? ',' : ' ', ir->name);
   }
   emitComma = true;

   return visit_continue;
}

bool
do_print_constants(exec_list *instructions, glslopt_shader* shader)
{
   ir_constant_printing_visitor v(shader);
   v.run(instructions);
   if(v.numConsts == 0) {
   	ralloc_asprintf_append (&shader->optimizedOutput, " \"%s\": [0.0, 0.0, 0.0, 0.0]\n", printMode == kPrintGlslVertex ? "vc0" : "fc0");
   }
   return false;
}


static void
print_agal_var_mapping(const void *key, void *data, void *closure) {
	char *nm = (char*)data;
	glslopt_shader* shader = (glslopt_shader*)closure;
	if(! ((nm[0] == 'v' && nm[1] == 't') || (nm[0] == 'f' && nm[1] == 't'))) {
		ralloc_asprintf_append (&shader->optimizedOutput, "%c\"%s\":\"%s\"\n", emitComma ? ',' : ' ', key, data);
		emitComma = true;
	}
}

bool glslOptimizerVerbose = false;

void dump(const char *nm, exec_list *ir, _mesa_glsl_parse_state *state, PrintGlslMode printMode, bool validate = true)
{
	if(!glslOptimizerVerbose) return;

	fprintf(stderr, "glsl %s:\n%s", nm, _mesa_print_ir_glsl(ir, state, NULL, printMode));
	if(validate) {
		fprintf(stderr, "validate: \n"); validate_ir_tree(ir);
	}
}

glslopt_shader* glslopt_optimize (glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource, unsigned options)
{
	glslopt_shader* shader = new (ctx->mem_ctx) glslopt_shader ();

	GLenum glType = 0;
	switch (type) {
	case kGlslOptShaderVertex: glType = GL_VERTEX_SHADER; printMode = kPrintGlslVertex; break;
	case kGlslOptShaderFragment: glType = GL_FRAGMENT_SHADER; printMode = kPrintGlslFragment; break;
	}
	if (!glType)
	{
		shader->infoLog = ralloc_asprintf (ctx->mem_ctx, "Unknown shader type %d", (int)type);
		shader->status = false;
		return shader;
	}

	_mesa_glsl_parse_state* state = new (ctx->mem_ctx) _mesa_glsl_parse_state (&ctx->mesa_ctx, glType, ctx->mem_ctx);
	state->error = 0;
	state->language_version = 130;

	const char* vspreamble = 
	"vec4 ftransform() { return gl_ModelViewProjectionMatrix * gl_Vertex; }\n";

	const char* fspreamble = 
	"";

	const char *modifiedSource = ralloc_asprintf (ctx->mem_ctx, "%s%s", printMode == kPrintGlslVertex ? vspreamble : fspreamble, shaderSource);

	if (!(options & kGlslOptionSkipPreprocessor))
	{
		state->error = preprocess (state, &modifiedSource, &state->info_log, state->extensions, ctx->mesa_ctx.API);
		if (state->error)
		{
			shader->status = !state->error;
			shader->infoLog = state->info_log;
			return shader;
		}
	}

	_mesa_glsl_lexer_ctor (state, modifiedSource);
	_mesa_glsl_parse (state);
	_mesa_glsl_lexer_dtor (state);

	exec_list* ir = new (ctx->mem_ctx) exec_list();

	if (!state->error && !state->translation_unit.is_empty())
		_mesa_ast_to_hir (ir, state);

	// Un-optimized output
	if (!state->error) {
		validate_ir_tree(ir);
		shader->rawOutput = _mesa_print_ir_glsl(ir, state, ralloc_strdup(ctx->mem_ctx, ""), printMode);
	}

	hash_table *oldnames = NULL;

	shader->optimizedOutput = NULL;
	ralloc_asprintf_append (&shader->optimizedOutput, "{\n");

	dump("pre-opt", ir, state, printMode);

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
			progress2 = propagate_precision (ir); progress |= progress2; if (progress2) debug_print_ir ("After prec propagation", ir, state, ctx->mem_ctx);
			progress2 = do_copy_propagation(ir); progress |= progress2; if (progress2) debug_print_ir ("After copy propagation", ir, state, ctx->mem_ctx);
			progress2 = do_copy_propagation_elements(ir); progress |= progress2; if (progress2) debug_print_ir ("After copy propagation elems", ir, state, ctx->mem_ctx);
			if (!linked) {
				progress2 = do_dead_code_unlinked(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead code unlinked", ir, state, ctx->mem_ctx);
			} else {
				progress2 = do_dead_code(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead code", ir, state, ctx->mem_ctx);
			}
			progress2 = do_dead_code_local(ir); progress |= progress2; if (progress2) debug_print_ir ("After dead code local", ir, state, ctx->mem_ctx);
			progress2 = propagate_precision (ir); progress |= progress2; if (progress2) debug_print_ir ("After prec propagation", ir, state, ctx->mem_ctx);
			//	progress2 = do_tree_grafting(ir); progress |= progress2; if (progress2) debug_print_ir ("After tree grafting", ir, state, ctx->mem_ctx);
			progress2 = do_constant_propagation(ir); progress |= progress2; if (progress2) debug_print_ir ("After const propagation", ir, state, ctx->mem_ctx);
			if (!linked) {
				progress2 = do_constant_variable_unlinked(ir); progress |= progress2; if (progress2) debug_print_ir ("After const variable unlinked", ir, state, ctx->mem_ctx);
			} else {
				progress2 = do_constant_variable(ir); progress |= progress2; if (progress2) debug_print_ir ("After const variable", ir, state, ctx->mem_ctx);
			}
			progress2 = do_constant_folding(ir); progress |= progress2; if (progress2) debug_print_ir ("After const folding", ir, state, ctx->mem_ctx);
			progress2 = do_algebraic(ir); progress |= progress2; if (progress2) debug_print_ir ("After algebraic", ir, state, ctx->mem_ctx);
			progress2 = lower_discard(ir); progress |= progress2; if (progress2) debug_print_ir ("After lower discard", ir, state, ctx->mem_ctx);
			progress2 = do_lower_jumps(ir); progress |= progress2; if (progress2) debug_print_ir ("After lower jumps", ir, state, ctx->mem_ctx);
			progress2 = lower_if_to_cond_assign(ir); progress |= progress2; if (progress2) debug_print_ir ("After lower if", ir, state, ctx->mem_ctx);
			progress2 = do_vec_index_to_swizzle(ir); progress |= progress2; if (progress2) debug_print_ir ("After vec index to swizzle", ir, state, ctx->mem_ctx);
			//progress2 = do_vec_index_to_cond_assign(ir); progress |= progress2; if (progress2) debug_print_ir ("After vec index to cond assign", ir, state, ctx->mem_ctx);
			progress2 = do_swizzle_swizzle(ir); progress |= progress2; if (progress2) debug_print_ir ("After swizzle swizzle", ir, state, ctx->mem_ctx);
			progress2 = do_noop_swizzle(ir); progress |= progress2; if (progress2) debug_print_ir ("After noop swizzle", ir, state, ctx->mem_ctx);
			progress2 = optimize_redundant_jumps(ir); progress |= progress2; if (progress2) debug_print_ir ("After redundant jumps", ir, state, ctx->mem_ctx);

			loop_state *ls = analyze_loop_variables(ir);
			progress2 = set_loop_controls(ir, ls); progress |= progress2;
			progress2 = unroll_loops(ir, ls, 8); progress |= progress2;
			progress2 = do_agal_expression_flattening(ir, false); progress |= progress2;
			do_lower_conditionl_assigns_to_agal(ir); dump("post-lowercond", ir, state, printMode);

			delete ls;
		} while (progress);

		dump("post-opt", ir, state, printMode);

		do_tree_grafting(ir); dump("after-graft", ir, state, printMode);



		do_agal_expression_flattening(ir, true); dump("after flattening", ir, state, printMode);

		do_lower_arrays(ir); dump("post-opt", ir, state, printMode);

		//do_tree_grafting(ir); dump("after-graft", ir, state, printMode);

		do_agal_expression_flattening(ir, true); dump("post-opt", ir, state, printMode);

		do_algebraic(ir); dump("post-opt", ir, state, printMode);

		do_remove_casts(ir); dump("post-opt", ir, state, printMode);

		do_hoist_constants(ir); dump("post-opt", ir, state, printMode);

		do_agal_expression_flattening(ir, true); dump("post-opt", ir, state, printMode);

		for(int i=0; i<6; i++) {
				do_shorten_liveranges(ir);  dump("after-shorten-liveranges", ir, state, printMode);
		}
		
		dump("post-opt", ir, state, printMode);

		do_swizzle_everything(ir); dump("post-swizz", ir, state, printMode);
		
		do_coalesce_floats(ir);
		
		do_swizzle_swizzle(ir);

		do_coalesce_temps(ir); dump("post-opt", ir, state, printMode);

		do_unique_variables(ir); dump("post-unique", ir, state, printMode);

		oldnames = do_remap_agalvars(ir, printMode);
		validate_ir_tree(ir);

		//schedule_instructions(ir, state, printMode);

		//const char *glslout2 = _mesa_print_ir_glsl(ir, state, ralloc_strdup(ctx->mem_ctx, ""), printMode);
		//fprintf(stderr, "glsl:\n%s", glslout2);
	}

	const char *agalasmout = strlen(state->info_log) > 0 ? "" : _mesa_print_ir_agal(ir, state, ralloc_strdup(ctx->mem_ctx, ""), printMode);
	const char *infoLog = state->info_log;
	std::string sanitisedInfoLog;
	while(infoLog && *infoLog)
	{
		switch(*infoLog) {
			case 10:
			case 13:
				sanitisedInfoLog += "\\n";
				break;
			default:
				sanitisedInfoLog += infoLog[0];
		}
		infoLog++;
	}
	ralloc_asprintf_append (&shader->optimizedOutput, "\"info\":\"%s\"", sanitisedInfoLog.c_str());

	// Final optimized output
	if (strlen(state->info_log) > 0) {
		ralloc_asprintf_append (&shader->optimizedOutput, "\n}\n");
	} else {
		ralloc_asprintf_append (&shader->optimizedOutput, ",\n");
		//shader->optimizedOutput = NULL;

		//ralloc_asprintf_append (&shader->optimizedOutput, "\"glsl-raw\":\"%s\",\n", shader->rawOutput);

		//const char *glslout = _mesa_print_ir_glsl(ir, state, ralloc_strdup(ctx->mem_ctx, ""), printMode);
		//ralloc_asprintf_append (&shader->optimizedOutput, "\"glsl-final\":\"%s\",\n", glslout);

		ralloc_asprintf_append (&shader->optimizedOutput, "\"varnames\" : \n{\n");

		emitComma = false;
		hash_table_call_foreach(oldnames, print_agal_var_mapping, shader);
		ralloc_asprintf_append (&shader->optimizedOutput, "},\n");

		emitComma = false;
		ralloc_asprintf_append (&shader->optimizedOutput, "\"consts\" : \n{\n");
		do_print_constants(ir, shader);
		ralloc_asprintf_append (&shader->optimizedOutput, "},\n");

		emitComma = false;
		ralloc_asprintf_append (&shader->optimizedOutput, "\"types\" : \n{\n");
		do_print_types(ir, shader);
		ralloc_asprintf_append (&shader->optimizedOutput, "},\n");

		emitComma = false;
		ralloc_asprintf_append (&shader->optimizedOutput, "\"storage\" : \n{\n");
		do_print_storage(ir, shader);
		ralloc_asprintf_append (&shader->optimizedOutput, "},\n");

		if(glslOptimizerVerbose)
			fprintf(stderr, "agal:\n%s", agalasmout);
		std::string sanitisedAGAL;
		const char *asmsrc = agalasmout;
		while(*asmsrc)
		{
			switch(*asmsrc) {
				case 10:
				case 13:
					sanitisedAGAL += "\\n";
					break;
				default:
					sanitisedAGAL += asmsrc[0];
			}
			asmsrc++;
		}
		ralloc_asprintf_append (&shader->optimizedOutput, "\"agalasm\":\"%s\"\n}\n", sanitisedAGAL.c_str());

		if(glslOptimizerVerbose) {
			char *agalout;
			size_t agalsz = 0;
			AGAL::Assemble(agalasmout, printMode == kPrintGlslFragment ? AGAL::shadertype_fragment : AGAL::shadertype_vertex, &agalout, &agalsz);

			//AGAL::Graph *depgraph = AGAL::CreateDependencyGraph(agalout, agalsz, 0);
			//fprintf(stderr, "digraph agaldepgraph {\n");
			//for(int i=0; i<depgraph->edges.length; i++) {
			//	fprintf(stderr, "%d -> %d\n", depgraph->edges[i].srcidx, depgraph->edges[i].dstidx);
			//}
			//fprintf(stderr, "}\n");

			FlashString disasmout;
			if(agalout)
				AGAL::Disassemble (agalout, agalsz, &disasmout);
			fprintf(stderr, "//--- Disasm ---\n");
			fprintf(stderr, "%s", disasmout.CStr());
			fprintf(stderr, "//--- Disasm ---\n");
		}
	}

	shader->status = !state->error;
	shader->infoLog = state->info_log;

	ralloc_free (ir);
	ralloc_free (state);

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
