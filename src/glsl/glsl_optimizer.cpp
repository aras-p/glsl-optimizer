#include "glsl_optimizer.h"
#include "main/mtypes.h"
#include "ir.h"
#include "ir_buffer_blocks.h"
#include "ir_image.h"
#include "ir_hierarchical_visitor.h"
#include <unordered_map>
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
	ctx->Extensions.ARB_shader_storage_buffer_object = true;
	ctx->Extensions.ARB_shader_image_load_store = true;
	ctx->Extensions.ARB_shader_image_size = true;

	switch(api)
	{
	default:
	case kGlslTargetOpenGL:
		ctx->Const.GLSLVersion = 450;
		break;
	case kGlslTargetOpenGLES30:
	case kGlslTargetMetal:
		if (api == kGlslTargetMetal) ctx->Const.GLSLVersion = 450;
		ctx->Extensions.ARB_ES3_compatibility = true;
		ctx->Extensions.ARB_ES3_1_compatibility = true;
		ctx->Extensions.ARB_ES3_2_compatibility = true;
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
   // Compiler resource limits, independent of the eventual target device.
   ctx->Const.MaxCombinedTextureImageUnits = MAX_COMBINED_TEXTURE_IMAGE_UNITS;
   ctx->Const.MaxUniformBufferBindings = 72;
   ctx->Const.MaxUniformBlockSize = 16384;
   ctx->Const.UniformBufferOffsetAlignment = 16;
   ctx->Const.MaxCombinedUniformBlocks = 72;
   ctx->Const.MaxShaderStorageBufferBindings = 16;
   ctx->Const.MaxShaderStorageBlockSize = 1 << 27;
   ctx->Const.ShaderStorageBufferOffsetAlignment = 16;
   ctx->Const.MaxCombinedShaderStorageBlocks = 48;
   ctx->Const.MaxUserAssignableUniformLocations = 1024;
   ctx->Const.MaxAtomicBufferBindings = 8;
   ctx->Const.MaxAtomicBufferSize = 16384;
   ctx->Const.MaxCombinedAtomicBuffers = 48;
   ctx->Const.MaxCombinedAtomicCounters = 24576;
   ctx->Const.MaxImageUnits = 8;
   ctx->Const.MaxImageSamples = 1;
   ctx->Const.MaxCombinedImageUniforms = 48;
   ctx->Const.MaxCombinedShaderOutputResources = 16;
   ctx->Const.MaxComputeSharedMemorySize = 32768;
   for (auto& program : ctx->Const.Program) {
      program.MaxUniformBlocks = 12;
      program.MaxCombinedUniformComponents = program.MaxUniformComponents +
         program.MaxUniformBlocks * (ctx->Const.MaxUniformBlockSize / 4);
      program.MaxShaderStorageBlocks = 8;
      program.MaxAtomicBuffers = 8;
      program.MaxAtomicCounters = 4096;
      program.MaxImageUniforms = 8;
   }

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

struct glslopt_buffer {
	glslopt_buffer_desc desc;
	std::vector<glslopt_buffer_member_desc> members;
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
	std::vector<glslopt_buffer> buffers;
	int uniformCount, uniformsSize;
	int inputCount;
	int textureCount;
	int statsMath, statsTex, statsFlow;
	int computeLocalSize[3] = {};

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
	else if (type->is_image())
		out->type = type->sampler_dimensionality == GLSL_SAMPLER_DIM_2D ?
			(type->sampler_array ? kGlslTypeImage2DArray : kGlslTypeImage2D) :
			type->sampler_dimensionality == GLSL_SAMPLER_DIM_3D ? kGlslTypeImage3D : kGlslTypeOther;
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
	out->vectorSize = (type->is_sampler() || type->is_image()) ? 0 : type->vector_elements;
	out->matrixSize = (type->is_sampler() || type->is_image()) ? 0 : type->matrix_columns;
	
	// precision
	switch (prec)
	{
		case GLSL_PRECISION_HIGH: out->prec = kGlslPrecHigh; break;
		case GLSL_PRECISION_MEDIUM: out->prec = kGlslPrecMedium; break;
		default: out->prec = kGlslPrecHigh; break;
	}
}

// Each reflected field has one usage node; arrays combine all element accesses.
// Aggregate accesses mark descendants, while field accesses mark ancestors only.
class buffer_usage_visitor : public ir_hierarchical_visitor {
public:
	struct node {
		unsigned first_child = 0;
		unsigned child_count = 0;
		bool referenced = false;
		bool used = false;
	};
	std::vector<node> nodes;
	std::vector<unsigned> roots;
	std::unordered_map<const ir_variable*, unsigned> variables;

	void build(unsigned index, const glsl_type* type) {
		unsigned count = type->is_array() ? 1 :
			(type->is_struct() || type->is_interface()) ? type->length : 0;
		unsigned first = (unsigned)nodes.size();
		nodes[index].first_child = first;
		nodes[index].child_count = count;
		nodes.resize(first + count);
		for (unsigned i = 0; i < count; ++i)
			build(first + i, type->is_array() ? type->fields.array : type->fields.structure[i].type);
	}

	buffer_usage_visitor(const std::vector<buffer_block>& blocks, exec_list* ir) {
		for (const auto& block : blocks) {
			unsigned root = (unsigned)nodes.size();
			roots.push_back(root);
			nodes.emplace_back();
			build(root, block.type);
			string_to_uint_map fields;
			if (!block.variable->is_interface_instance())
				for (unsigned i = 0; i < block.type->length; ++i)
					fields.put(nodes[root].first_child + i, block.type->fields.structure[i].name);
			foreach_in_list(ir_instruction, instruction, ir) {
				auto* v = instruction->as_variable();
				if (!v || (v->data.mode != ir_var_uniform && v->data.mode != ir_var_shader_storage) || v->get_interface_type() != block.type) continue;
				unsigned member = root;
				if (!v->is_interface_instance()) {
					bool found = fields.get(member, v->name);
					assert(found);
					if (!found) continue;
				}
				variables[v] = member;
			}
		}
	}

	unsigned resolve(ir_rvalue* value) {
		if (auto s = value->as_swizzle()) return resolve(s->val);
		if (auto v = value->as_dereference_variable()) return variables.at(v->var);
		if (auto r = value->as_dereference_record())
			return nodes[resolve(r->record)].first_child + r->field_idx;
		auto a = value->as_dereference_array();
		a->array_index->accept(this);
		unsigned parent = resolve(a->array);
		return a->array->type->is_array() && !a->array->type->without_array()->is_interface() ? nodes[parent].first_child : parent;
	}
	ir_visitor_status record(ir_rvalue* value) {
		auto v = value->variable_referenced();
		if (!v || !variables.count(v)) return visit_continue;
		nodes[resolve(value)].referenced = true;
		return visit_continue_with_parent;
	}
	ir_visitor_status visit(ir_dereference_variable* value) override { record(value); return visit_continue; }
	ir_visitor_status visit_enter(ir_dereference_record* value) override { return record(value); }
	ir_visitor_status visit_enter(ir_dereference_array* value) override { return record(value); }

	bool propagate(unsigned index, bool whole = false) {
		auto& n = nodes[index];
		whole |= n.referenced;
		n.used = whole;
		for (unsigned i = 0; i < n.child_count; ++i)
			n.used |= propagate(n.first_child + i, whole);
		return n.used;
	}
};

static void reflect_buffer_member(glslopt_shader* shader, glslopt_buffer& buffer,
	const glsl_type* type, const char* name, int parent, unsigned offset, bool row, bool known, bool std430,
	const buffer_usage_visitor& usage, unsigned usageIndex, bool readOnly, bool writeOnly)
{
	int index = (int)buffer.members.size();
	unsigned stride = type->is_array() ? buffer_array_stride(type->fields.array, row, std430) : 0;
	unsigned matrixStride = type->is_matrix() ? buffer_array_stride(row ? type->row_type() : type->column_type(), false, std430) : 0;
	buffer.members.push_back({ralloc_strdup(shader->mem_ctx, name), ralloc_strdup(shader->mem_ctx, type->name),
		parent, known ? (int)offset : -1, type->is_array() ? (type->length ? (int)type->length : -1) : 0,
		type->is_array() ? (known ? (int)stride : -1) : 0,
		type->is_matrix() ? (known ? (int)matrixStride : -1) : 0, type->is_matrix() && row, usage.nodes[usageIndex].used, readOnly, writeOnly});
	if (type->is_array()) {
		reflect_buffer_member(shader, buffer, type->fields.array, "[]", index, offset, row, known, std430,
			usage, usage.nodes[usageIndex].first_child, readOnly, writeOnly);
	} else if (type->is_struct()) {
		unsigned cursor = 0;
		for (unsigned i = 0; i < type->length; ++i) {
			const auto& f = type->fields.structure[i];
			cursor = buffer_field_offset(type, i, cursor, row, std430);
			bool fieldRow = buffer_field_row_major(f, row);
			reflect_buffer_member(shader, buffer, f.type, f.name, index, offset + cursor, fieldRow, known, std430,
				usage, usage.nodes[usageIndex].first_child + i, readOnly, writeOnly);
			cursor += buffer_size(f.type, fieldRow, std430);
		}
	}
}

class atomic_counter_usage : public ir_hierarchical_visitor {
public:
   std::unordered_set<const ir_variable*> used;
   ir_visitor_status visit(ir_dereference_variable* value) override {
      if (value->var->type->contains_atomic()) used.insert(value->var);
      return visit_continue;
   }
   ir_visitor_status visit_enter(ir_function_signature* signature) override {
      return signature->is_builtin() ? visit_continue_with_parent : visit_continue;
   }
};

static void reflect_atomic_counter(glslopt_shader* shader, glslopt_buffer& buffer,
   const glsl_type* type, const char* name, int parent, unsigned offset, bool used)
{
   int index = (int)buffer.members.size();
   buffer.members.push_back({ralloc_strdup(shader->mem_ctx, name), ralloc_strdup(shader->mem_ctx, type->name),
      parent, (int)offset, type->is_array() ? (int)type->length : 0,
      type->is_array() ? (int)type->fields.array->atomic_size() : 0, 0, false, used, false, false});
   if (type->is_array()) reflect_atomic_counter(shader, buffer, type->fields.array, "[]", index, offset, used);
}

static void find_shader_atomic_counters(glslopt_shader* shader, exec_list* ir, bool metal)
{
   auto counters = collect_atomic_counter_buffers(ir);
   if (counters.empty()) return;
   atomic_counter_usage usage;
   visit_list_elements(&usage, ir);
   for (const auto& counter : counters) {
      glslopt_buffer buffer;
      buffer.desc = {ralloc_asprintf(shader->mem_ctx, "atomic_counters_%u", counter.binding), nullptr,
         kGlslBufferAtomicCounter, kGlslLayoutAtomicCounter, (int)counter.binding, 1, (int)counter.size,
         metal ? (int)counter.metal_index : -1, -1};
      for (auto* v : counter.variables)
         reflect_atomic_counter(shader, buffer, v->type, v->name, -1, v->data.offset, usage.used.count(v) != 0);
      shader->buffers.push_back(std::move(buffer));
   }
}

static void find_shader_buffers(glslopt_shader* shader, exec_list* ir, bool metal)
{
	auto blocks = collect_buffer_blocks(ir, metal);
	if (blocks.empty()) return;
	unsigned sizeTableIndex = metal ? buffer_size_table_index(blocks) : 0;
	buffer_usage_visitor usage(blocks, ir);
	visit_list_elements(&usage, ir);
	for (unsigned root : usage.roots) usage.propagate(root);
	for (unsigned blockIndex = 0; blockIndex < blocks.size(); ++blockIndex) {
		const auto& b = blocks[blockIndex];
		glslopt_buffer buffer;
		auto layout = b.type->get_interface_packing();
		bool std430 = buffer_uses_std430(b, metal);
		bool known = metal || std430 || layout == GLSL_INTERFACE_PACKING_STD140;
		const glsl_type* last = b.type->fields.structure[b.type->length - 1].type;
		bool runtime = last->is_array() && !last->length;
		buffer.desc = {ralloc_strdup(shader->mem_ctx, b.type->name),
			b.variable->is_interface_instance() ? ralloc_strdup(shader->mem_ctx, b.variable->name) : nullptr,
			b.variable->data.mode == ir_var_shader_storage ? kGlslBufferStorage : kGlslBufferUniform, layout == GLSL_INTERFACE_PACKING_STD430 ? kGlslLayoutStd430 : layout == GLSL_INTERFACE_PACKING_STD140 ? kGlslLayoutStd140 :
			layout == GLSL_INTERFACE_PACKING_PACKED ? kGlslLayoutPacked : kGlslLayoutShared,
			b.variable->data.explicit_binding ? b.variable->data.binding : -1,
			(int)b.array_size, known && !runtime ? (int)buffer_block_size(b.type, std430) : -1, metal ? (int)b.metal_index : -1,
			metal && b.needs_size ? (int)sizeTableIndex : -1};
		unsigned cursor = 0;
		bool row = b.type->get_interface_row_major();
		for (unsigned i = 0; i < b.type->length; ++i) {
			const auto& f = b.type->fields.structure[i];
			cursor = buffer_field_offset(b.type, i, cursor, row, std430);
			bool fieldRow = buffer_field_row_major(f, row);
			reflect_buffer_member(shader, buffer, f.type, f.name, -1, cursor, fieldRow, known, std430,
				usage, usage.nodes[usage.roots[blockIndex]].first_child + i, f.memory_read_only, f.memory_write_only);
			cursor += buffer_size(f.type, fieldRow, std430);
		}
		shader->buffers.push_back(std::move(buffer));
	}
}

static void find_shader_variables(glslopt_shader* sh, exec_list* ir, bool metal)
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
			if (var->data.explicit_location && var->is_interface_instance())
				v.location -= var->data.patch ? VARYING_SLOT_PATCH0 : VARYING_SLOT_VAR0;
			++sh->inputCount;
		}
		if (var->data.mode == ir_var_uniform && !var->is_in_buffer_block() && !var->type->without_array()->is_sampler() && !var->type->contains_image() && !var->type->contains_atomic())
		{
			if (sh->uniformCount >= glslopt_shader::kMaxShaderUniforms)
				continue;
			
			glslopt_shader_var& v = sh->uniforms[sh->uniformCount];
			v.name = ralloc_strdup(sh->mem_ctx, var->name);
			glsl_type_to_optimizer_desc(var->type, var->data.precision, &v);
			v.location = var->data.explicit_location ? var->data.location : -1;
			++sh->uniformCount;
		}
		if (var->data.mode == ir_var_uniform && (var->type->without_array()->is_sampler() || var->type->contains_image()))
		{
			if (sh->textureCount >= glslopt_shader::kMaxShaderTextures)
				continue;
			
			glslopt_shader_var& v = sh->textures[sh->textureCount];
			v.name = ralloc_strdup(sh->mem_ctx, var->name);
			glsl_type_to_optimizer_desc(var->type, var->data.precision, &v);
			v.location = !metal && var->data.explicit_binding ? var->data.binding :
				var->data.explicit_location ? var->data.location : -1;
			++sh->textureCount;
		}
	}
}


static bool metal_uniform_type_supported(const glsl_type* type, bool allowMatrices = true)
{
	if (type->is_array()) return metal_uniform_type_supported(type->fields.array, allowMatrices);
	if (type->is_struct() || type->is_interface()) {
		for (unsigned i = 0; i < type->length; ++i)
			if (!metal_uniform_type_supported(type->fields.structure[i].type, allowMatrices)) return false;
		return true;
	}
	if (type->is_matrix() && !allowMatrices) return false;
	return type->is_float() || type->is_integer_32() || type->is_boolean();
}

// Reject buffer operations that cannot be represented by the selected printer.
class storage_validation : public ir_hierarchical_visitor {
public:
	bool metal;
	bool vertex;
	const char* error = nullptr;
	storage_validation(bool metal, bool vertex) : metal(metal), vertex(vertex) {}
	ir_visitor_status visit(ir_variable* var) override {
		if (metal && var->type->contains_image() && var->data.memory_volatile)
			error = "Volatile image output is not supported yet (Metal backend).";
		return visit_continue;
	}
	void check_type(const glsl_type* type) {
		if (type->is_array()) {
			check_type(type->fields.array);
		} else if (type->is_struct() || type->is_interface()) {
			for (unsigned i = 0; i < type->length; ++i) {
				const auto& f = type->fields.structure[i];
				if (f.memory_coherent || f.memory_volatile)
					error = "Metal coherent/volatile storage buffers are not supported.";
				check_type(f.type);
			}
		}
	}
	ir_visitor_status visit_enter(ir_function_signature* signature) override {
		return signature->is_builtin() ? visit_continue_with_parent : visit_continue;
	}
	ir_visitor_status visit_enter(ir_call* call) override {
		if (!call->callee->is_builtin()) return visit_continue;
		if (find_image_atomic(call->callee) && metal) {
			auto* image = ((ir_instruction*)call->actual_parameters.get_head())->as_rvalue();
			if (image->type->sampled_type != GLSL_TYPE_INT && image->type->sampled_type != GLSL_TYPE_UINT)
				error = "Metal image atomics require 32-bit integer images (r32i or r32ui).";
			return visit_continue;
		}
		if (image_function_name(call->callee)) return visit_continue;
		if (!strncmp(call->callee_name(), "__intrinsic_image_", 18) || !strncmp(call->callee_name(), "image", 5)) {
			error = "Unsupported image operation.";
			return visit_continue;
		}
		if (const auto* counter = find_atomic_counter(call->callee)) {
			if (metal && vertex && counter->intrinsic != ir_intrinsic_atomic_counter_read)
				error = "Metal vertex shaders can only read atomic counters.";
			return visit_continue;
		}
		if (find_memory_barrier(call->callee)) return visit_continue;
		if (find_buffer_atomic(call->callee)) {
			auto* target = ((ir_instruction*)call->actual_parameters.get_head())->as_rvalue();
			auto* var = target->variable_referenced();
			if (metal && (!var || (var->data.mode != ir_var_shader_storage && var->data.mode != ir_var_shader_shared) ||
				!target->type->is_scalar() || !target->type->is_integer_32()))
				error = "Metal atomics require a 32-bit int or uint in an SSBO or shared variable.";
			if (metal && var && var->data.mode == ir_var_shader_shared) {
				// Array indices and swizzles retain the underlying storage width.
				// Stop at record fields so their declared precision is preserved.
				ir_rvalue* storage = target;
				while (true) {
					if (auto* array = storage->as_dereference_array()) storage = array->array;
					else if (auto* swizzle = storage->as_swizzle()) storage = swizzle->val;
					else break;
				}
				if (storage->get_precision() == GLSL_PRECISION_MEDIUM)
					error = "Metal shared atomics require highp int or uint storage.";
			}
			return visit_continue;
		}
		if (strstr(call->callee_name(), "atomic") || strstr(call->callee_name(), "barrier") ||
			strstr(call->callee_name(), "Barrier"))
			error = "Buffer atomics and memory barriers are not supported.";
		return visit_continue;
	}
};

glslopt_shader* glslopt_optimize (glslopt_ctx* ctx, glslopt_shader_type type, const char* shaderSource, unsigned options)
{
	if (!ctx)
		return NULL;
	glslopt_shader* result = new glslopt_shader;
	gl_shader* shader = result->shader;
	switch (type) {
	case kGlslOptShaderVertex: shader->Stage = MESA_SHADER_VERTEX; break;
	case kGlslOptShaderFragment: shader->Stage = MESA_SHADER_FRAGMENT; break;
	case kGlslOptShaderCompute: shader->Stage = MESA_SHADER_COMPUTE; break;
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
	if (type == kGlslOptShaderCompute) {
		if (!state->cs_input_local_size_specified && ctx->target == kGlslTargetMetal) {
			result->infoLog = "Metal compute output requires a fixed local group size.";
			return result;
		}
		for (unsigned i = 0; i < 3; ++i)
			result->computeLocalSize[i] = state->cs_input_local_size[i];
	}
	if (ctx->target != kGlslTargetMetal && shader->IsES && shader->Version < 300) {
		result->infoLog = "OpenGL ES 2.0 output is no longer supported; use #version 300 es or newer.";
		return result;
	}

	exec_list* ir = shader->ir;
	bool hasStorage = false;
	bool hasShared = false;
	bool hasImages = false;
	bool hasCounters = false;
	bool hasStageBlocks = false;
	storage_validation storageValidation(ctx->target == kGlslTargetMetal, type == kGlslOptShaderVertex);

	// These declarations need dedicated backend support before they can be
	// printed safely. In particular, Metal's ordinary-uniform layout is not
	// a substitute for std140/std430 buffer layouts.
	foreach_in_list(ir_instruction, instruction, ir) {
		ir_variable* var = instruction->as_variable();
		if (!var || strncmp(var->name, "gl_", 3) == 0)
			continue;
		hasStorage |= var->data.mode == ir_var_shader_storage;
		hasShared |= var->data.mode == ir_var_shader_shared;
		hasImages |= var->type->contains_image();
		hasCounters |= var->type->contains_atomic();
		const char* unsupported = nullptr;
		const auto* stageBlock = var->get_interface_type();
		if (ctx->target == kGlslTargetMetal && stageBlock &&
		    (var->data.mode == ir_var_shader_in || var->data.mode == ir_var_shader_out)) {
			hasStageBlocks = true;
			if (!metal_uniform_type_supported(stageBlock))
				unsupported = "Stage interface block member type";
		}
		else if (ctx->target == kGlslTargetMetal && var->type->contains_image()) {
			const glsl_type* image = var->type;
			if (!image->is_image() ||
				(image->sampler_dimensionality != GLSL_SAMPLER_DIM_2D && image->sampler_dimensionality != GLSL_SAMPLER_DIM_3D))
				unsupported = "Image dimension or resource-array";
			else if (var->data.memory_volatile)
				unsupported = "Volatile image";
		}
		// Metal matrix constructors do not support threadgroup storage.
		else if (ctx->target == kGlslTargetMetal && var->data.mode == ir_var_shader_shared && !metal_uniform_type_supported(var->type, false))
			unsupported = "Shared-variable type";
		if (unsupported) {
			result->infoLog = ralloc_asprintf(result->mem_ctx,
				"%s output is not supported yet (%s backend).", unsupported,
				ctx->target == kGlslTargetMetal ? "Metal" : "GLSL");
			return result;
		}
	}

	if (ctx->target == kGlslTargetMetal) {
		auto counters = collect_atomic_counter_buffers(ir);
		if (!counters.empty() && counters.back().metal_index > 30) {
			result->infoLog = "Metal atomic-counter buffers exceed the available buffer slots (1 through 30).";
			return result;
		}
		auto blocks = collect_buffer_blocks(ir, true);
		if (buffer_size_table_index(blocks) > 30) {
			result->infoLog = "Metal buffer size table exceeds the available buffer slots (1 through 30).";
			return result;
		}
		for (const auto& block : blocks) {
			if (block.variable->data.mode == ir_var_shader_storage) {
				storageValidation.check_type(block.type);
			}
			if (!metal_uniform_type_supported(block.type)) {
				result->infoLog = "Metal buffer blocks support only 32-bit float/integer and boolean storage.";
				return result;
			}
			if (block.metal_index + block.array_size > 31) {
				result->infoLog = "Metal buffer blocks exceed the available buffer slots (1 through 30).";
				return result;
			}
		}
	}
	if (hasStorage || hasImages || hasCounters || (ctx->target == kGlslTargetMetal && type == kGlslOptShaderCompute))
		visit_list_elements(&storageValidation, ir);
	if (storageValidation.error) { result->infoLog = storageValidation.error; return result; }
	if (ctx->target == kGlslTargetMetal && (hasStorage || hasShared || hasImages || hasCounters || hasStageBlocks)) {
		// Stage interfaces, buffers, threadgroup storage and images live inside the Metal entry point.
		// Inline helpers so their resource references remain in scope.
		while (do_function_inlining(ir)) {}
		do_dead_functions(ir);
		propagate_precision(ir);
	}

	// Note: restore source-level subtraction outside of regular optimization loop
    // (inside the loop, other passes would undo it).
	add_neg_to_sub_visitor source_arithmetic;
	visit_list_elements(&source_arithmetic, ir);

	const PrintGlslMode printMode = type == kGlslOptShaderVertex ? kPrintGlslVertex :
		type == kGlslOptShaderCompute ? kPrintGlslCompute : kPrintGlslFragment;
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
	find_shader_variables(result, ir, ctx->target == kGlslTargetMetal);
	find_shader_buffers(result, ir, ctx->target == kGlslTargetMetal);
	find_shader_atomic_counters(result, ir, ctx->target == kGlslTargetMetal);
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

void glslopt_shader_get_compute_local_size(glslopt_shader* shader, int* x, int* y, int* z)
{
	*x = shader->computeLocalSize[0];
	*y = shader->computeLocalSize[1];
	*z = shader->computeLocalSize[2];
}

int glslopt_shader_get_buffer_count(glslopt_shader* shader) { return (int)shader->buffers.size(); }
const glslopt_buffer_desc* glslopt_shader_get_buffer_desc(glslopt_shader* shader, int index)
{
	return index >= 0 && index < (int)shader->buffers.size() ? &shader->buffers[index].desc : nullptr;
}
int glslopt_shader_get_buffer_member_count(glslopt_shader* shader, int index)
{
	return index >= 0 && index < (int)shader->buffers.size() ? (int)shader->buffers[index].members.size() : 0;
}
const glslopt_buffer_member_desc* glslopt_shader_get_buffer_member_desc(glslopt_shader* shader, int blockIndex, int memberIndex)
{
	if (blockIndex < 0 || blockIndex >= (int)shader->buffers.size()) return nullptr;
	const auto& members = shader->buffers[blockIndex].members;
	return memberIndex >= 0 && memberIndex < (int)members.size() ? &members[memberIndex] : nullptr;
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
