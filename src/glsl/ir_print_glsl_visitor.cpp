/*
 * Copyright © 2010 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#include <cmath>
#include <limits>

#include "ir_print_glsl_visitor.h"
#include "ir_visitor.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "ir_hierarchical_visitor.h"
#include <unordered_map>
#include <unordered_set>
#include "loop_analysis.h"
#include "ir_print_loop.h"
#include "ir_buffer_blocks.h"
#include "ir_image.h"



static void print_type(string_buffer& buffer, const glsl_type *t, bool arraySize);
static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize);

static inline const char* get_precision_string (unsigned p)
{
	switch (p) {
	case GLSL_PRECISION_HIGH:		return "highp ";
	case GLSL_PRECISION_MEDIUM:		return "mediump ";
	case GLSL_PRECISION_NONE:	return "";
	}
	assert(!"Should not get here.");
	return "";
}

static const int tex_sampler_type_count = 7;
// [glsl_sampler_dim]
static const char* tex_sampler_dim_name[tex_sampler_type_count] = {
	"1D", "2D", "3D", "Cube", "Rect", "Buf", "2D", // external samplers use texture2D/texture2DProj
};


struct ga_entry : public exec_node
{
	ga_entry(ir_instruction* ir)
	{
		assert(ir);
		this->ir = ir;
	}	
	ir_instruction* ir;
};


struct global_print_tracker {
	global_print_tracker () {
		mem_ctx = ralloc_context(0);
		var_counter = 0;

		main_function_done = false;
	}
	
	~global_print_tracker() {

		ralloc_free(mem_ctx);
	}
	
	std::unordered_set<const glsl_type*> uniform_blocks;
	std::unordered_set<const glsl_type*> stage_blocks[2];
	unsigned	var_counter;
	std::unordered_map<const ir_variable*, long> var_hash;
	exec_list	global_assignements;
	void* mem_ctx;
	bool	main_function_done;
};

class ir_print_glsl_visitor : public ir_visitor {
public:
	ir_print_glsl_visitor(string_buffer& buf, global_print_tracker* globals_, PrintGlslMode mode_, bool use_precision_, const _mesa_glsl_parse_state* state_)
		: buffer(buf)
		, loopstate(NULL)
		, skipped_this_ir(false)
		, previous_skipped(false)
	{
		indentation = 0;
		expression_depth = 0;
		globals = globals_;
		mode = mode_;
		use_precision = use_precision_;
		state = state_;
	}

	virtual ~ir_print_glsl_visitor()
	{
	}


	void indent(void);
	void newline_indent();
	void end_statement_line();
	void newline_deindent();
	void print_var_name (ir_variable* v);
	void print_precision (ir_instruction* ir, const glsl_type* type);

	virtual void visit(ir_variable *);
	virtual void visit(ir_function_signature *);
	virtual void visit(ir_function *);
	virtual void visit(ir_expression *);
	virtual void visit(ir_texture *);
	virtual void visit(ir_swizzle *);
	virtual void visit(ir_dereference_variable *);
	virtual void visit(ir_dereference_array *);
	virtual void visit(ir_dereference_record *);
	virtual void visit(ir_assignment *);
	virtual void visit(ir_constant *);
	virtual void visit(ir_call *);
	virtual void visit(ir_return *);
	virtual void visit(ir_discard *);
	virtual void visit(ir_if *);
	virtual void visit(ir_loop *);
	virtual void visit(ir_loop_jump *);
	virtual void visit(ir_demote *) { buffer.asprintf_append("demote"); }
	virtual void visit(ir_barrier *) { buffer.asprintf_append("barrier()"); }
	virtual void visit(ir_emit_vertex *);
	virtual void visit(ir_end_primitive *);
	
	void emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex);

	bool emit_canonical_for (ir_loop* ir);
	bool try_print_array_assignment (ir_dereference* lhs, ir_rvalue* rhs);
	
	int indentation;
	int expression_depth;
	string_buffer& buffer;
	global_print_tracker* globals;
	const _mesa_glsl_parse_state* state;
	PrintGlslMode mode;
	loop_state* loopstate;
	print_loop_initializers* loop_initializers = nullptr;
	bool	use_precision;
	bool prototypes = false;
	bool	skipped_this_ir;
	bool	previous_skipped;
};

class struct_declarations : public ir_hierarchical_visitor {
public:
	string_buffer& out;
	bool es;
	std::unordered_set<const glsl_type*> emitted;
	struct_declarations(string_buffer& out, bool es) : out(out), es(es) {}
	void emit(const glsl_type* type) {
		type = type->without_array();
		if ((!type->is_struct() && !type->is_interface()) || !strncmp(type->name, "gl_", 3) || !emitted.insert(type).second) return;
		if (type->is_interface()) {
			for (unsigned i = 0; i < type->length; ++i) emit(type->fields.structure[i].type);
			return;
		}
		for (unsigned i = 0; i < type->length; ++i) emit(type->fields.structure[i].type);
		out.asprintf_append("struct %s {\n", type->name);
		for (unsigned i = 0; i < type->length; ++i) {
			const glsl_struct_field& field = type->fields.structure[i];
			out.asprintf_append("  ");
			const glsl_type* base = field.type->without_array();
			if (es && (base->is_float() || base->is_integer_32() || base->is_sampler()))
				out.asprintf_append("%s", get_precision_string(field.precision ? field.precision : GLSL_PRECISION_HIGH));
			print_type(out, field.type, false);
			out.asprintf_append(" %s", field.name);
			print_type_post(out, field.type, false);
			out.asprintf_append(";\n");
		}
		out.asprintf_append("};\n");
	}
	ir_visitor_status visit(ir_variable* v) override { emit(v->type); if (v->get_interface_type()) emit(v->get_interface_type()); return visit_continue; }
	ir_visitor_status visit(ir_constant* c) override { emit(c->type); return visit_continue; }
	ir_visitor_status visit_enter(ir_function_signature* f) override { emit(f->return_type); return visit_continue; }
};
static void print_struct_declarations(string_buffer& out, exec_list* ir, bool es)
{
	struct_declarations visitor(out, es);
	visit_list_elements(&visitor, ir);
}

char*
_mesa_print_ir_glsl(exec_list *instructions,
	    struct _mesa_glsl_parse_state *state,
		char* buffer, PrintGlslMode mode)
{
	string_buffer str(buffer);
	string_buffer body(buffer);

	// print version & extensions
	if (state) {
		if (state->had_version_string)
		{
			str.asprintf_append ("#version %i", state->language_version);
			if (state->es_shader)
				str.asprintf_append (" es");
			str.asprintf_append ("\n");
		}
		if (state->ARB_separate_shader_objects_enable)
			str.asprintf_append("#extension GL_ARB_separate_shader_objects : enable\n");
		if (state->EXT_shader_io_blocks_enable)
			str.asprintf_append("#extension GL_EXT_shader_io_blocks : enable\n");
		if (state->OES_shader_io_blocks_enable)
			str.asprintf_append("#extension GL_OES_shader_io_blocks : enable\n");
		if (state->ARB_shader_atomic_counters_enable)
			str.asprintf_append("#extension GL_ARB_shader_atomic_counters : enable\n");
		if (state->ARB_shader_image_load_store_enable)
			str.asprintf_append("#extension GL_ARB_shader_image_load_store : enable\n");
		if (state->ARB_shader_image_size_enable)
			str.asprintf_append("#extension GL_ARB_shader_image_size : enable\n");
		if (state->ARB_shader_storage_buffer_object_enable)
			str.asprintf_append("#extension GL_ARB_shader_storage_buffer_object : enable\n");
		if (state->ARB_uniform_buffer_object_enable)
			str.asprintf_append("#extension GL_ARB_uniform_buffer_object : enable\n");
		if (state->ARB_shading_language_420pack_enable)
			str.asprintf_append("#extension GL_ARB_shading_language_420pack : enable\n");
		if (state->ARB_enhanced_layouts_enable)
			str.asprintf_append("#extension GL_ARB_enhanced_layouts : enable\n");
		if (state->ARB_compute_shader_enable)
			str.asprintf_append("#extension GL_ARB_compute_shader : enable\n");
		if (state->ARB_shader_texture_lod_enable)
			str.asprintf_append ("#extension GL_ARB_shader_texture_lod : enable\n");
		if (state->ARB_draw_instanced_enable)
			str.asprintf_append ("#extension GL_ARB_draw_instanced : enable\n");
		if (state->EXT_gpu_shader4_enable)
			str.asprintf_append ("#extension GL_EXT_gpu_shader4 : enable\n");
		if (state->OES_EGL_image_external_essl3_enable)
			str.asprintf_append ("#extension GL_OES_EGL_image_external_essl3 : enable\n");
		if (state->EXT_shader_framebuffer_fetch_enable)
			str.asprintf_append ("#extension GL_EXT_shader_framebuffer_fetch : enable\n");
		if (state->ARB_shader_bit_encoding_enable)
			str.asprintf_append("#extension GL_ARB_shader_bit_encoding : enable\n");
		if (state->EXT_texture_array_enable)
			str.asprintf_append ("#extension GL_EXT_texture_array : enable\n");
	}
	
	if (state->stage == MESA_SHADER_FRAGMENT && state->fs_early_fragment_tests)
		str.asprintf_append("layout(early_fragment_tests) in;\n");

	if (state->stage == MESA_SHADER_COMPUTE && state->cs_input_local_size_specified) {
		str.asprintf_append("layout(local_size_x=%u", state->cs_input_local_size[0]);
		if (state->cs_input_local_size[1] != 1)
			str.asprintf_append(", local_size_y=%u", state->cs_input_local_size[1]);
		if (state->cs_input_local_size[2] != 1)
			str.asprintf_append(", local_size_z=%u", state->cs_input_local_size[2]);
		str.asprintf_append(") in;\n");
	}

	// Struct declarations are reconstructed from the types that remain in IR.
	print_struct_declarations(str, instructions, state->es_shader);
	
	global_print_tracker gtracker;
	// Declarations make raw IR printable even after function reordering.
	foreach_in_list(ir_instruction, instruction, instructions) {
		ir_function* fn = instruction->as_function();
		if (!fn || !strcmp(fn->name, "main")) continue;
		foreach_in_list(ir_function_signature, sig, &fn->signatures) {
			if (sig->is_builtin()) continue;
			ir_print_glsl_visitor declaration(str, &gtracker, kPrintGlslNone, state->es_shader, state);
			declaration.prototypes = true;
			sig->accept(&declaration);
		}
	}
	
	loop_state* ls = analyze_loop_variables(instructions);
	print_loop_initializers initializers(instructions, ls);


	foreach_in_list(ir_instruction, ir, instructions)
	{
		if (ir->ir_type == ir_type_variable) {
			ir_variable *var = static_cast<ir_variable*>(ir);
			if ((strstr(var->name, "gl_") == var->name)
			  && !var->data.invariant)
				continue;
		}

		ir_print_glsl_visitor v (body, &gtracker, mode, state->es_shader, state);
		v.loopstate = ls;
		v.loop_initializers = &initializers;

		ir->accept(&v);
		if (ir->ir_type != ir_type_function && !v.skipped_this_ir)
			body.asprintf_append (";\n");

	}
	
	delete ls;
	
	
	// Add the optimized glsl code
	str.asprintf_append("%s", body.c_str());

	return ralloc_strdup(buffer, str.c_str());
}


void ir_print_glsl_visitor::indent(void)
{
	if (previous_skipped)
		return;
	previous_skipped = false;
	for (int i = 0; i < indentation; i++)
		buffer.asprintf_append ("  ");
}

void ir_print_glsl_visitor::end_statement_line()
{
	if (!skipped_this_ir)
		buffer.asprintf_append(";\n");
	previous_skipped = skipped_this_ir;
	skipped_this_ir = false;
}

void ir_print_glsl_visitor::newline_indent()
{
	if (expression_depth % 4 == 0)
	{
		++indentation;
		buffer.asprintf_append ("\n");
		indent();
	}
}
void ir_print_glsl_visitor::newline_deindent()
{
	if (expression_depth % 4 == 0)
	{
		--indentation;
		buffer.asprintf_append ("\n");
		indent();
	}
}


void ir_print_glsl_visitor::print_var_name (ir_variable* v)
{
    long id = globals->var_hash[v];
	if (!id && v->data.mode == ir_var_temporary)
	{
        id = ++globals->var_counter;
        globals->var_hash[v] = id;
	}
    if (id)
    {
        if (v->data.mode == ir_var_temporary)
            buffer.asprintf_append ("tmpvar_%d", (int)id);
        else
        {
            // reuse trailing underscore (avoid reserved double underscores)
            const size_t length = strlen(v->name);
            const char* separator = (length > 0 && v->name[length - 1] == '_') ? "" : "_";
            buffer.asprintf_append ("%s%s%d", v->name, separator, (int)id);
        }
    }
	else
	{
		buffer.asprintf_append ("%s", v->name);
	}
}

void ir_print_glsl_visitor::print_precision (ir_instruction* ir, const glsl_type* type)
{
	if (!this->use_precision)
		return;
	if (!type) return;
	const glsl_type* base = type->without_array();
	if (!base->is_float() && !base->is_integer_32() && !base->is_sampler() && !base->is_image() && !base->is_atomic_uint()) return;
	unsigned prec = GLSL_PRECISION_NONE;
	if (ir_variable* var = ir->as_variable()) prec = var->data.precision;
	else if (ir->ir_type == ir_type_function_signature)
		prec = ((ir_function_signature*)ir)->return_precision;
	if (prec == GLSL_PRECISION_NONE) prec = GLSL_PRECISION_HIGH;
	buffer.asprintf_append("%s", get_precision_string(prec));
}


static void print_type(string_buffer& buffer, const glsl_type *t, bool arraySize)
{
	if (t->base_type == GLSL_TYPE_ARRAY) {
		print_type(buffer, t->without_array(), false);
		if (arraySize)
			for (auto a = t; a->is_array(); a = a->fields.array) buffer.asprintf_append("[%u]", a->length);
	} else if ((t->base_type == GLSL_TYPE_STRUCT)
			   && (strncmp("gl_", t->name, 3) != 0)) {
		buffer.asprintf_append ("%s", t->name);
	} else {
		buffer.asprintf_append ("%s", t->name);
	}
}

static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize)
{
	if (t->base_type == GLSL_TYPE_ARRAY) {
		if (!arraySize)
			for (auto a = t; a->is_array(); a = a->fields.array) buffer.asprintf_append("[%u]", a->length);
	}
}


void ir_print_glsl_visitor::visit(ir_variable *ir)
{
	const bool input = ir->data.mode == ir_var_shader_in;
	const glsl_type* interface = ir->get_interface_type();
	if (interface && (input || ir->data.mode == ir_var_shader_out) &&
	    strncmp(interface->name, "gl_", 3) != 0) {
		if (!globals->stage_blocks[input].insert(interface).second) { skipped_this_ir = true; return; }
		const bool instance = ir->is_interface_instance();
		const int base = ir->data.patch ? VARYING_SLOT_PATCH0 : VARYING_SLOT_VAR0;
		int nextLocation = -1;
		if (instance && ir->data.explicit_location)
			nextLocation = ir->data.location;
		else if (!instance && interface->fields.structure[0].location >= 0) {
			// Anonymous blocks retain member locations, but have no instance carrying the base location.
			bool allLocated = true;
			for (unsigned i = 0; i < interface->length; ++i)
				allLocated &= interface->fields.structure[i].location >= 0;
			if (allLocated) nextLocation = interface->fields.structure[0].location;
		}
		const bool blockLocation = nextLocation >= 0;
		if (blockLocation) buffer.asprintf_append("layout(location=%d) ", nextLocation - base);
		if (!input) {
			for (unsigned i = 0; i < interface->length; ++i) {
				const auto& field = interface->fields.structure[i];
				if (field.explicit_xfb_buffer || field.offset >= 0 || field.xfb_stride >= 0) {
					buffer.asprintf_append("layout(xfb_buffer=%d) ", field.xfb_buffer);
					break;
				}
			}
		}
		buffer.asprintf_append("%s%s %s {\n", ir->data.patch ? "patch " : "", input ? "in" : "out", interface->name);
		const char* interpolation[] = { "", "smooth ", "flat ", "noperspective " };
		for (unsigned i = 0; i < interface->length; ++i) {
			const auto& field = interface->fields.structure[i];
			buffer.asprintf_append("  ");
			if (field.location >= 0 && (!blockLocation || field.location != nextLocation))
				buffer.asprintf_append("layout(location=%d) ", field.location - base);
			if (field.location >= 0)
				nextLocation = field.location + field.type->count_attribute_slots(false);
			if (!input && field.offset >= 0) buffer.asprintf_append("layout(xfb_offset=%d) ", field.offset);
			if (!input && field.xfb_stride >= 0) buffer.asprintf_append("layout(xfb_stride=%d) ", field.xfb_stride);
			if (field.centroid) buffer.asprintf_append("centroid ");
			if (field.sample) buffer.asprintf_append("sample ");
			buffer.asprintf_append("%s", interpolation[field.interpolation]);
			const glsl_type* member = field.type->without_array();
			if (use_precision && (member->is_float() || member->is_integer_32()))
				buffer.asprintf_append("%s", get_precision_string(field.precision ? field.precision : GLSL_PRECISION_HIGH));
			print_type(buffer, field.type, false);
			buffer.asprintf_append(" %s", field.name);
			print_type_post(buffer, field.type, false);
			buffer.asprintf_append(";\n");
		}
		buffer.asprintf_append("}");
		if (instance) {
			buffer.asprintf_append(" %s", ir->name);
			print_type_post(buffer, ir->type, false);
		}
		return;
	}
	if ((ir->data.mode == ir_var_uniform || ir->data.mode == ir_var_shader_storage) && ir->is_in_buffer_block()) {
		const glsl_type* type = ir->get_interface_type();
		if (!globals->uniform_blocks.insert(type).second) { skipped_this_ir = true; return; }
		// Use the enum explicitly; its order is not part of the printer contract.
		const char* layout = "shared";
		switch (type->get_interface_packing()) {
		case GLSL_INTERFACE_PACKING_STD140: layout = "std140"; break;
		case GLSL_INTERFACE_PACKING_STD430: layout = "std430"; break;
		case GLSL_INTERFACE_PACKING_PACKED: layout = "packed"; break;
		default: break;
		}
		buffer.asprintf_append("layout(%s, %s", layout, type->get_interface_row_major() ? "row_major" : "column_major");
		if (ir->data.explicit_binding) buffer.asprintf_append(", binding=%d", ir->data.binding);
		buffer.asprintf_append(") %s %s {\n", ir->data.mode == ir_var_shader_storage ? "buffer" : "uniform", type->name);
		for (unsigned i = 0; i < type->length; ++i) {
			const auto& f = type->fields.structure[i];
			buffer.asprintf_append("  ");
			if (f.matrix_layout != GLSL_MATRIX_LAYOUT_INHERITED || f.offset >= 0) {
				buffer.asprintf_append("layout(");
				if (f.matrix_layout != GLSL_MATRIX_LAYOUT_INHERITED)
					buffer.asprintf_append("%s", f.matrix_layout == GLSL_MATRIX_LAYOUT_ROW_MAJOR ? "row_major" : "column_major");
				if (f.offset >= 0) buffer.asprintf_append("%soffset=%d", f.matrix_layout != GLSL_MATRIX_LAYOUT_INHERITED ? ", " : "", f.offset);
				buffer.asprintf_append(") ");
			}
			if (f.memory_read_only) buffer.asprintf_append("readonly ");
			if (f.memory_write_only) buffer.asprintf_append("writeonly ");
			if (f.memory_coherent) buffer.asprintf_append("coherent ");
			if (f.memory_volatile) buffer.asprintf_append("volatile ");
			if (f.memory_restrict) buffer.asprintf_append("restrict ");
			const glsl_type* base = f.type->without_array();
			if (use_precision && (base->is_float() || base->is_integer_32()))
				buffer.asprintf_append("%s", get_precision_string(f.precision ? f.precision : GLSL_PRECISION_HIGH));
			print_type(buffer, base, false);
			buffer.asprintf_append(" %s", f.name);
			for (auto t = f.type; t->is_array(); t = t->fields.array) {
				if (t->length) buffer.asprintf_append("[%u]", t->length);
				else buffer.asprintf_append("[]");
			}
			buffer.asprintf_append(";\n");
		}
		buffer.asprintf_append("}");
		if (ir->is_interface_instance()) {
			buffer.asprintf_append(" %s", ir->name);
			for (auto t = ir->type; t->is_array(); t = t->fields.array) buffer.asprintf_append("[%u]", t->length);
		}
		return;
	}

	if (ir->data.mode == ir_var_uniform && ir->type->contains_atomic())
		buffer.asprintf_append("layout(binding=%u, offset=%u) ", ir->data.binding, ir->data.offset);
	if (ir->type->contains_image()) {
		const char* format = image_format_name(ir->data.image_format);
		if (format || ir->data.explicit_binding) {
			buffer.asprintf_append("layout(");
			if (format) buffer.asprintf_append("%s", format);
			if (ir->data.explicit_binding) buffer.asprintf_append("%sbinding=%d", format ? ", " : "", ir->data.binding);
			buffer.asprintf_append(") ");
		}
		if (ir->data.memory_read_only) buffer.asprintf_append("readonly ");
		if (ir->data.memory_write_only) buffer.asprintf_append("writeonly ");
		if (ir->data.memory_coherent) buffer.asprintf_append("coherent ");
		if (ir->data.memory_volatile) buffer.asprintf_append("volatile ");
		if (ir->data.memory_restrict) buffer.asprintf_append("restrict ");
	}
	else if (ir->data.mode == ir_var_uniform && ir->type->without_array()->is_sampler() && ir->data.explicit_binding) {
		buffer.asprintf_append("layout(binding=%u) ", ir->data.binding);
	}
	const char *const cent = (ir->data.centroid) ? "centroid " : "";
	const char *const inv = (ir->data.explicit_invariant) ? "invariant " : "";
	const char* storage = "";
	switch (ir->data.mode) {
	case ir_var_uniform: storage = "uniform "; break;
	case ir_var_shader_in: storage = state->language_version >= 130 ? "in " : (state->stage == MESA_SHADER_VERTEX ? "attribute " : "varying "); break;
	case ir_var_shader_out: storage = ir->data.fb_fetch_output ? "inout " : (state->language_version >= 130 ? "out " : "varying "); break;
	case ir_var_function_in: case ir_var_const_in: storage = ir->type->contains_image() ? "" : "in "; break;
	case ir_var_function_out: storage = "out "; break;
	case ir_var_function_inout: storage = "inout "; break;
	case ir_var_shader_shared: storage = "shared "; break;
	default: break;
	}

	const char *const interp[] = { "", "smooth ", "flat ", "noperspective " };
	
	if (this->state->language_version >= 300 && ir->data.explicit_location && strncmp(ir->name, "gl_", 3) != 0)
	{
		const int binding_base = (this->state->stage == MESA_SHADER_VERTEX ? (int)VERT_ATTRIB_GENERIC0 : (int)FRAG_RESULT_DATA0);
		const int location = ir->data.location - (ir->data.mode == ir_var_uniform ? 0 : binding_base);
		buffer.asprintf_append ("layout(location=%d) ", location);
	}
	
	// give an id to any variable defined in a function that is not an uniform
	if ((this->mode == kPrintGlslNone && ir->data.mode != ir_var_uniform))
	{
		long id = globals->var_hash[ir];
		if (id == 0)
		{
			id = ++globals->var_counter;
			globals->var_hash[ir] = id;
		}
	}
	
	if (loop_initializers && loop_initializers->omitted.count(ir)) {
		skipped_this_ir = true;
		return;
	}

	// keep invariant declaration for builtin variables
	if (strstr(ir->name, "gl_") == ir->name) {
		buffer.asprintf_append ("%s", inv);
		print_var_name (ir);
		return;
	}
	
	buffer.asprintf_append ("%s%s%s%s",
							cent, inv, interp[ir->data.interpolation], storage);
	print_precision (ir, ir->type);
	print_type(buffer, ir->type, false);
	buffer.asprintf_append (" ");
	print_var_name (ir);
	print_type_post(buffer, ir->type, false);
	
	ir_constant* initializer = ir->data.mode == ir_var_uniform
		? ir->constant_initializer : ir->constant_value;
	if (initializer &&
		ir->data.mode != ir_var_shader_in &&
		ir->data.mode != ir_var_shader_out &&
		ir->data.mode != ir_var_function_in &&
		ir->data.mode != ir_var_function_out &&
		ir->data.mode != ir_var_function_inout)
	{
		buffer.asprintf_append (" = ");
		visit (initializer);
	}
}


void ir_print_glsl_visitor::visit(ir_function_signature *ir)
{
   print_precision (ir, ir->return_type);
   print_type(buffer, ir->return_type, true);
   buffer.asprintf_append (" %s (", ir->function_name());

   if (!ir->parameters.is_empty())
   {
	   buffer.asprintf_append ("\n");

	   indentation++; previous_skipped = false;
	   bool first = true;
	   foreach_in_list(ir_variable, inst, &ir->parameters) {
		  if (!first)
			  buffer.asprintf_append (",\n");
		  indent();
		  inst->accept(this);
		  first = false;
	   }
	   indentation--;

	   buffer.asprintf_append ("\n");
	   indent();
   }

   if (prototypes || !ir->is_defined)
   {
	   buffer.asprintf_append (");\n");
	   return;
   }

   buffer.asprintf_append (")\n");

   indent();
   buffer.asprintf_append ("{\n");
   indentation++; previous_skipped = false;
	
	// insert postponed global assigments
	if (strcmp(ir->function()->name, "main") == 0)
	{
		assert (!globals->main_function_done);
		globals->main_function_done = true;
		foreach_in_list(ga_entry, node, &globals->global_assignements)
		{
			ir_instruction* as = node->ir;
			as->accept(this);
			buffer.asprintf_append(";\n");
		}
	}

   foreach_in_list(ir_instruction, inst, &ir->body) {
      indent();
      inst->accept(this);
	   end_statement_line();
   }
   indentation--;
   indent();
   buffer.asprintf_append ("}\n");
}

void ir_print_glsl_visitor::visit(ir_function *ir)
{
   bool found_non_builtin_proto = false;

   foreach_in_list(ir_function_signature, sig, &ir->signatures) {
      if (!sig->is_builtin())
	 found_non_builtin_proto = true;
   }
   if (!found_non_builtin_proto)
      return;

   PrintGlslMode oldMode = this->mode;
   this->mode = kPrintGlslNone;

   foreach_in_list(ir_function_signature, sig, &ir->signatures) {
      indent();
      sig->accept(this);
      buffer.asprintf_append ("\n");
   }

   this->mode = oldMode;

   indent();
}


static const char* glsl_operator(ir_expression_operation op)
{
	switch (op) {
	case ir_unop_round_even: return "roundEven";
	case ir_unop_bitfield_reverse: return "bitfieldReverse";
	case ir_unop_bit_count: return "bitCount";
	case ir_unop_find_msb: return "findMSB";
	case ir_unop_find_lsb: return "findLSB";
	case ir_unop_interpolate_at_centroid: return "interpolateAtCentroid";
	case ir_binop_interpolate_at_offset: return "interpolateAtOffset";
	case ir_binop_interpolate_at_sample: return "interpolateAtSample";
	case ir_binop_atan2: return "atan";
	case ir_triop_bitfield_extract: return "bitfieldExtract";
	case ir_quadop_bitfield_insert: return "bitfieldInsert";
	case ir_unop_neg: return "-";
	case ir_unop_rcp: return "1.0/";
	case ir_unop_rsq: return "inversesqrt";
	case ir_binop_equal: return "==";
	case ir_binop_nequal: return "!=";
	case ir_binop_all_equal: return "==";
	case ir_binop_any_nequal: return "!=";
	case ir_triop_lrp: return "mix";
	case ir_unop_bitcast_i2f: return "intBitsToFloat";
	case ir_unop_bitcast_u2f: return "uintBitsToFloat";
	case ir_unop_bitcast_f2i: return "floatBitsToInt";
	case ir_unop_bitcast_f2u: return "floatBitsToUint";
	default: return ir_expression_operation_strings[op];
	}
}

static const char *const operator_vec_glsl_strs[] = {
	"lessThan",
	"greaterThanEqual",
	"equal",
	"notEqual",
};


static bool is_binop_func_like(ir_expression_operation op, const glsl_type* type)
{
	if ((op == ir_binop_mod && type->is_float()) ||
		(op >= ir_binop_dot && op <= ir_binop_pow) ||
		op == ir_binop_atan2 || op == ir_binop_ldexp ||
		op == ir_binop_interpolate_at_offset || op == ir_binop_interpolate_at_sample)
		return true;
	if (type->is_vector() && (op >= ir_binop_less && op <= ir_binop_nequal))
	{
		return true;
	}
	return false;
}

void ir_print_glsl_visitor::visit(ir_expression *ir)
{
	++this->expression_depth;
	newline_indent();
	
	ir_expression* lower = ir->operation == ir_binop_min ? ir->operands[0]->as_expression() : nullptr;
	if (ir->operation == ir_unop_ssbo_unsized_array_length) {
		ir->operands[0]->accept(this);
		buffer.asprintf_append(".length()");
	}
	else if (lower && lower->operation == ir_binop_max) {
		buffer.asprintf_append("clamp ("); lower->operands[0]->accept(this);
		buffer.asprintf_append(", "); lower->operands[1]->accept(this);
		buffer.asprintf_append(", "); ir->operands[1]->accept(this);
		buffer.asprintf_append(")");
	}
	else if (ir->operation == ir_unop_saturate) {
		buffer.asprintf_append("clamp ("); ir->operands[0]->accept(this);
		buffer.asprintf_append(", 0.0, 1.0)");
	}
	else if (ir->operation == ir_unop_b2f && ir->type->is_vector() &&
		ir->operands[0]->as_expression() &&
		ir->operands[0]->as_expression()->operation == ir_binop_logic_and) {
		// Recover the numeric mask product without expanding vector comparisons.
		ir_expression* mask = ir->operands[0]->as_expression();
		buffer.asprintf_append("(");
		print_type(buffer, ir->type, true); buffer.asprintf_append("(");
		mask->operands[0]->accept(this); buffer.asprintf_append(") * ");
		print_type(buffer, ir->type, true); buffer.asprintf_append("(");
		mask->operands[1]->accept(this); buffer.asprintf_append("))");
	}
	else if (ir->type->is_vector() && (ir->operation == ir_binop_logic_and || ir->operation == ir_binop_logic_or || ir->operation == ir_binop_logic_xor)) {
		// GLSL has no vector &&/||/^^ operators. Convert boolean lanes to 0/1
		// for AND/OR, or compare them for XOR, evaluating each operand once.
		if (ir->operation == ir_binop_logic_xor) {
			buffer.asprintf_append("notEqual (");
			ir->operands[0]->accept(this); buffer.asprintf_append(", ");
			ir->operands[1]->accept(this); buffer.asprintf_append(")");
		} else {
			print_type(buffer, ir->type, true);
			buffer.asprintf_append("(ivec%u(", ir->type->vector_elements);
			ir->operands[0]->accept(this);
			buffer.asprintf_append(") %s ivec%u(",
				ir->operation == ir_binop_logic_and ? "*" : "+", ir->type->vector_elements);
			ir->operands[1]->accept(this); buffer.asprintf_append("))");
		}
	}
	else if (ir->num_operands == 1) {
		if (ir->operation >= ir_unop_f2i && ir->operation <= ir_unop_u2i) {
			print_type(buffer, ir->type, true);
			buffer.asprintf_append ("(");
		} else if (ir->operation == ir_unop_rcp) {
			buffer.asprintf_append ("(1.0/(");
		} else {
			buffer.asprintf_append ("%s(", ir->operation == ir_unop_logic_not && ir->type->is_vector() ? "not" : glsl_operator(ir->operation));
		}
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append (")");
		if (ir->operation == ir_unop_rcp) {
			buffer.asprintf_append (")");
		}
	}
	else if (ir->operation == ir_triop_csel) {
		unsigned n = ir->operands[0]->type->vector_elements;
		if (n > 1) { print_type(buffer, ir->type, true); buffer.asprintf_append("("); }
		for (unsigned c = 0; c < n; ++c) {
			if (c) buffer.asprintf_append(", ");
			buffer.asprintf_append("(");
			for (unsigned i = 0; i < 3; ++i) {
				if (i) buffer.asprintf_append(i == 1 ? " ? " : " : ");
				buffer.asprintf_append("("); ir->operands[i]->accept(this); buffer.asprintf_append(")");
				if (n > 1) buffer.asprintf_append(".%c", "xyzw"[c]);
			}
			buffer.asprintf_append(")");
		}
		if (n > 1) buffer.asprintf_append(")");
	}
	else if (ir->operation == ir_quadop_vector) {
		print_type(buffer, ir->type, true); buffer.asprintf_append("(");
		for (unsigned i = 0; i < ir->type->vector_elements; ++i) {
			if (i) buffer.asprintf_append(", ");
			ir->operands[i]->accept(this);
		}
		buffer.asprintf_append(")");
	}
	else if (ir->operation == ir_binop_vector_extract)
	{
		// a[b]
		
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append ("[");
		if (ir->operands[1])
			ir->operands[1]->accept(this);
		buffer.asprintf_append ("]");
	}
	else if (is_binop_func_like(ir->operation, ir->type))
	{
		if (ir->operation == ir_binop_mod)
		{
			buffer.asprintf_append ("(");
			print_type(buffer, ir->type, true);
			buffer.asprintf_append ("(");
		}
		if (ir->type->is_vector() && (ir->operation >= ir_binop_less && ir->operation <= ir_binop_nequal))
			buffer.asprintf_append ("%s (", operator_vec_glsl_strs[ir->operation-ir_binop_less]);
		else
			buffer.asprintf_append ("%s (", (ir->operation == ir_binop_mod ? "mod" : glsl_operator(ir->operation)));
		
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append (", ");
		if (ir->operands[1])
			ir->operands[1]->accept(this);
		buffer.asprintf_append (")");
		if (ir->operation == ir_binop_mod)
            buffer.asprintf_append ("))");
	}
	else if (ir->num_operands == 2)
	{
		buffer.asprintf_append ("(");
		if (ir->operands[0])
			ir->operands[0]->accept(this);

		buffer.asprintf_append (" %s ", glsl_operator(ir->operation));

		if (ir->operands[1])
			ir->operands[1]->accept(this);
		buffer.asprintf_append (")");
	}
	else
	{
		buffer.asprintf_append("%s (", glsl_operator(ir->operation));
		for (unsigned i = 0; i < ir->num_operands; ++i) {
			if (i) buffer.asprintf_append(", ");
			ir->operands[i]->accept(this);
		}
		buffer.asprintf_append(")");
	}
	
	newline_deindent();
	--this->expression_depth;
}

void ir_print_glsl_visitor::visit(ir_texture *ir)
{
	glsl_sampler_dim sampler_dim = (glsl_sampler_dim)ir->sampler->type->sampler_dimensionality;
	const bool is_shadow = ir->sampler->type->sampler_shadow;
	const bool is_array = ir->sampler->type->sampler_array;

	if (ir->op == ir_query_levels || ir->op == ir_texture_samples || ir->op == ir_lod) {
		buffer.asprintf_append("%s (", ir->op == ir_query_levels ? "textureQueryLevels" : ir->op == ir_texture_samples ? "textureSamples" : "textureQueryLod");
		ir->sampler->accept(this);
		if (ir->coordinate) { buffer.asprintf_append(", "); ir->coordinate->accept(this); }
		buffer.asprintf_append(")");
		return;
	}
	if (ir->op == ir_txs)
	{
		buffer.asprintf_append("textureSize (");
		ir->sampler->accept(this);
		if ((sampler_dim != GLSL_SAMPLER_DIM_RECT && sampler_dim != GLSL_SAMPLER_DIM_BUF && sampler_dim != GLSL_SAMPLER_DIM_MS))
		{
			buffer.asprintf_append(", ");
			ir->lod_info.lod->accept(this);
		}
		buffer.asprintf_append(")");
		return;
	}

	const bool is_proj = ir->projector != nullptr;
    // texture function name
    //ACS: shadow lookups and lookups with dimensionality included in the name were deprecated in 130
    if(state->language_version<130) 
    {
        buffer.asprintf_append ("%s", is_shadow ? "shadow" : "texture");
        buffer.asprintf_append ("%s", tex_sampler_dim_name[sampler_dim]);
    }
    else 
    {
        if (ir->op == ir_txf || ir->op == ir_txf_ms)
            buffer.asprintf_append ("texelFetch");
        else
            buffer.asprintf_append (ir->op == ir_tg4 ? "textureGather" : "texture");
    }

	if (is_array && state->EXT_texture_array_enable)
		buffer.asprintf_append ("Array");
	
	if (is_proj)
		buffer.asprintf_append ("Proj");
	if (ir->op == ir_txl)
		buffer.asprintf_append ("Lod");
	if (ir->op == ir_txd)
		buffer.asprintf_append ("Grad");
    if (ir->offset != NULL)
        buffer.asprintf_append (ir->offset->type->is_array() ? "Offsets" : "Offset");
	
	if (ir->op == ir_txd && !state->es_shader && state->ARB_shader_texture_lod_enable)
		buffer.asprintf_append ("ARB");

	buffer.asprintf_append (" (");
	
	// sampler
	ir->sampler->accept(this);
	buffer.asprintf_append (", ");
	
	// Mesa stores projection and shadow comparison separately. Reassemble
	// the source coordinate tuple (cube-array shadows take comparison separately).
	const bool separate_shadow = is_shadow && (ir->op == ir_tg4 || (is_array && sampler_dim == GLSL_SAMPLER_DIM_CUBE));
	const bool pack = is_proj || (ir->shadow_comparator && !separate_shadow);
	const bool pad_shadow_1d = is_shadow && !is_array && sampler_dim == GLSL_SAMPLER_DIM_1D;
	if (pack) {
		unsigned n = ir->coordinate->type->vector_elements + (is_proj ? 1 : 0) +
			(ir->shadow_comparator && !separate_shadow ? 1 : 0) + (pad_shadow_1d ? 1 : 0);
		buffer.asprintf_append("vec%u(", n);
	}
	ir->coordinate->accept(this);
	if (pad_shadow_1d) buffer.asprintf_append(", 0.0");
	if (ir->shadow_comparator && !separate_shadow) {
		buffer.asprintf_append(", "); ir->shadow_comparator->accept(this);
	}
	if (is_proj) { buffer.asprintf_append(", "); ir->projector->accept(this); }
	if (pack) buffer.asprintf_append(")");
	if (separate_shadow) { buffer.asprintf_append(", "); ir->shadow_comparator->accept(this); }

	// lod
	if (ir->op == ir_txl || ir->op == ir_txf)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.lod->accept(this);
	}
	
	// sample index
	if (ir->op == ir_txf_ms)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.sample_index->accept(this);
	}

	// grad
	if (ir->op == ir_txd)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.grad.dPdx->accept(this);
		buffer.asprintf_append (", ");
		ir->lod_info.grad.dPdy->accept(this);
	}

	// texel offset
	if (ir->offset != NULL)
	{
		buffer.asprintf_append (", ");
		ir->offset->accept(this);
	}
	
	if (ir->op == ir_tg4 && !is_shadow && ir->lod_info.component) {
		buffer.asprintf_append(", "); ir->lod_info.component->accept(this);
	}
	// lod bias
	if (ir->op == ir_txb)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.bias->accept(this);
	}
	
   buffer.asprintf_append(")");
   if (is_shadow && state->language_version < 130 && ir->type->is_scalar())
      buffer.asprintf_append(".x");
}


void ir_print_glsl_visitor::visit(ir_swizzle *ir)
{
   const unsigned swiz[4] = {
      ir->mask.x,
      ir->mask.y,
      ir->mask.z,
      ir->mask.w,
   };

   if (ir->val->type == glsl_type::float_type || ir->val->type == glsl_type::int_type || ir->val->type == glsl_type::uint_type)
	{
		if (ir->mask.num_components != 1)
		{
			print_type(buffer, ir->type, true);
			buffer.asprintf_append ("(");
		}
	}

	ir->val->accept(this);
	
	if (ir->val->type == glsl_type::float_type || ir->val->type == glsl_type::int_type || ir->val->type == glsl_type::uint_type)
	{
		if (ir->mask.num_components != 1)
		{
			buffer.asprintf_append (")");
		}
		return;
	}
	
	// Swizzling scalar types is not allowed so just return now.
	if (ir->val->type->vector_elements == 1)
		return;

   buffer.asprintf_append (".");
   for (unsigned i = 0; i < ir->mask.num_components; i++) {
		buffer.asprintf_append ("%c", "xyzw"[swiz[i]]);
   }
}


void ir_print_glsl_visitor::visit(ir_dereference_variable *ir)
{
   ir_variable *var = ir->variable_referenced();
   print_var_name (var);
}


void ir_print_glsl_visitor::visit(ir_dereference_array *ir)
{
   ir->array->accept(this);
   buffer.asprintf_append ("[");
   ir->array_index->accept(this);
   buffer.asprintf_append ("]");
}


void ir_print_glsl_visitor::visit(ir_dereference_record *ir)
{
   ir->record->accept(this);
   buffer.asprintf_append (".%s", ir->record->type->fields.structure[ir->field_idx].name);
}


bool ir_print_glsl_visitor::try_print_array_assignment (ir_dereference* lhs, ir_rvalue* rhs)
{
	if (this->state->language_version >= 120)
		return false;
	ir_dereference_variable* rhsarr = rhs->as_dereference_variable();
	if (rhsarr == NULL)
		return false;
	const glsl_type* lhstype = lhs->type;
	const glsl_type* rhstype = rhsarr->type;
	if (!lhstype->is_array() || !rhstype->is_array())
		return false;
	if (lhstype->array_size() != rhstype->array_size())
		return false;
	if (lhstype->base_type != rhstype->base_type)
		return false;
	
	const unsigned size = rhstype->array_size();
	for (unsigned i = 0; i < size; i++)
	{
		lhs->accept(this);
		buffer.asprintf_append ("[%d]=", i);
		rhs->accept(this);
		buffer.asprintf_append ("[%d]", i);
		if (i != size-1)
			buffer.asprintf_append (";");
	}
	return true;
}

void ir_print_glsl_visitor::emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex)
{
	lhs->accept(this);
	
	if (dstIndex)
	{
		// if dst index is a constant, then emit a swizzle
		ir_constant* dstConst = dstIndex->as_constant();
		if (dstConst)
		{
			const char* comps = "xyzw";
			char comp = comps[dstConst->get_int_component(0)];
			buffer.asprintf_append (".%c", comp);
		}
		else
		{
			buffer.asprintf_append ("[");
			dstIndex->accept(this);
			buffer.asprintf_append ("]");
		}
	}
	
	char mask[5];
	unsigned j = 0;
	const glsl_type* lhsType = lhs->type;
	const glsl_type* rhsType = rhs->type;
	if (!dstIndex && lhsType->matrix_columns <= 1 && lhsType->vector_elements > 1 && write_mask != (1<<lhsType->vector_elements)-1)
	{
		for (unsigned i = 0; i < 4; i++) {
			if ((write_mask & (1 << i)) != 0) {
				mask[j] = "xyzw"[i];
				j++;
			}
		}
		lhsType = glsl_type::get_instance(lhsType->base_type, j, 1);
	}
	mask[j] = '\0';
	bool hasWriteMask = false;
	if (mask[0])
	{
		buffer.asprintf_append (".%s", mask);
		hasWriteMask = true;
	}
	
	buffer.asprintf_append (" = ");
	
	bool typeMismatch = !dstIndex && (lhsType != rhsType);
	const bool addSwizzle = hasWriteMask && typeMismatch;
	if (typeMismatch)
	{
		if (!addSwizzle)
			print_type(buffer, lhsType, true);
		buffer.asprintf_append ("(");
	}
	
	rhs->accept(this);
	
	if (typeMismatch)
	{
		buffer.asprintf_append (")");
		if (addSwizzle)
			buffer.asprintf_append (".%s", mask);
	}
}


// Try to print (X = X + const) as (X += const).
static bool try_print_increment (ir_print_glsl_visitor* vis, ir_assignment* ir)
{
	if (ir->condition)
		return false;
	
	// Needs to be + on rhs
	ir_expression* rhsOp = ir->rhs->as_expression();
	if (!rhsOp || rhsOp->operation != ir_binop_add)
		return false;
	
	// Needs to write to whole variable
	ir_variable* lhsVar = ir->whole_variable_written();
	if (lhsVar == NULL)
		return false;
	
	// Types must match
	if (ir->lhs->type != ir->rhs->type)
		return false;
	
	// Type must be scalar
	if (!ir->lhs->type->is_scalar())
		return false;
	
	// rhs0 must be variable deref, same one as lhs
	ir_dereference_variable* rhsDeref = rhsOp->operands[0]->as_dereference_variable();
	if (rhsDeref == NULL)
		return false;
	if (lhsVar != rhsDeref->var)
		return false;
	
	// rhs1 must be a constant
	ir_constant* rhsConst = rhsOp->operands[1]->as_constant();
	if (!rhsConst)
		return false;
	
	// print variable name
	ir->lhs->accept (vis);
	
	// print ++ or +=const
	if (ir->lhs->type->base_type <= GLSL_TYPE_INT && rhsConst->is_one())
	{
		vis->buffer.asprintf_append ("++");
	}
	else
	{
		vis->buffer.asprintf_append(" += ");
		rhsConst->accept (vis);
	}
	
	return true;
}


void ir_print_glsl_visitor::visit(ir_assignment *ir)
{
	if (loop_initializers && loop_initializers->omitted.count(ir)) {
		skipped_this_ir = true;
		return;
	}
	// assignments in global scope are postponed to main function
	if (this->mode != kPrintGlslNone)
	{
		assert (!this->globals->main_function_done);
		this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry(ir));
		buffer.asprintf_append ("//"); // for the ; that will follow (ugly, I know)
		return;
	}
	
	// if RHS is ir_triop_vector_insert, then we have to do some special dance. If source expression is:
	//   dst = vector_insert (a, b, idx)
	// then emit it like:
	//   dst = a;
	//   dst.idx = b;
	ir_expression* rhsOp = ir->rhs->as_expression();
	if (rhsOp && rhsOp->operation == ir_triop_vector_insert)
	{
		// skip assignment if lhs and rhs would be the same
		bool skip_assign = false;
		ir_dereference_variable* lhsDeref = ir->lhs->as_dereference_variable();
		ir_dereference_variable* rhsDeref = rhsOp->operands[0]->as_dereference_variable();
		if (lhsDeref && rhsDeref)
		{
			if (lhsDeref->var == rhsDeref->var)
				skip_assign = true;
		}
		
		if (!skip_assign)
		{
			emit_assignment_part(ir->lhs, rhsOp->operands[0], ir->write_mask, NULL);
			buffer.asprintf_append ("; ");
		}
		emit_assignment_part(ir->lhs, rhsOp->operands[1], ir->write_mask, rhsOp->operands[2]);
		return;
	}
	
	if (try_print_increment (this, ir))
		return;
		
	if (try_print_array_assignment (ir->lhs, ir->rhs))
		return;
		
   if (ir->condition)
   {
      ir->condition->accept(this);
	  buffer.asprintf_append (" ");
   }
	
	emit_assignment_part (ir->lhs, ir->rhs, ir->write_mask, NULL);
}


void print_float (string_buffer& buffer, float f)
{
	// Kind of roundabout way, but this is to satisfy two things:
	// * MSVC and gcc-based compilers differ a bit in how they treat float
	//   widht/precision specifiers. Want to match for tests.
	// * GLSL (early version at least) require floats to have ".0" or
	//   exponential notation.
	char tmp[64];
	snprintf(tmp, 64, "%.7g", f);

	char* posE = NULL;
	posE = strchr(tmp, 'e');
	if (!posE)
		posE = strchr(tmp, 'E');

	// snprintf formats infinity as inf.0 or -inf.0, which isn't useful here.
	// GLSL has no infinity constant so print an equivalent expression instead.
	if (f == std::numeric_limits<float>::infinity())
		strcpy(tmp, "(1.0/0.0)");

	if (f == -std::numeric_limits<float>::infinity())
		strcpy(tmp, "(-1.0/0.0)");
	
	// Do similar thing for NaN
	if (f != f)
		strcpy(tmp, "(0.0/0.0)");

	buffer.asprintf_append ("%s", tmp);

	// need to append ".0"?
	if (!strchr(tmp,'.') && (posE == NULL))
		buffer.asprintf_append(".0");
}

void ir_print_glsl_visitor::visit(ir_constant *ir)
{
	const glsl_type* type = ir->type;

	if (type == glsl_type::float_type)
	{
		if (!std::isfinite(ir->value.f[0]))
		{
			// Non-printable float. If we have bit conversions, we're fine. otherwise do hand-wavey things in print_float().
			if (state->es_shader
				|| (state->language_version >= 330)
				|| (state->ARB_shader_bit_encoding_enable))
			{
				buffer.asprintf_append("uintBitsToFloat(%uu)", ir->value.u[0]);
				return;
			}
		}
		
		print_float (buffer, ir->value.f[0]);
		return;
	}
	else if (type == glsl_type::int_type)
	{
		// Need special handling for INT_MIN
		if (ir->value.u[0] == 0x80000000)
			buffer.asprintf_append("int(0x%X)", ir->value.i[0]);
		else
			buffer.asprintf_append ("%d", ir->value.i[0]);
		return;
	}
	else if (type == glsl_type::uint_type)
	{
		// GLSL < 130 does not support uints
		if (state->language_version < 130)
			buffer.asprintf_append("%u", ir->value.u[0]);
		else
		{
			// Old Adreno drivers try to be smart with '0u' and treat that as 'const int'. Sigh.
			if (ir->value.u[0] == 0)
				buffer.asprintf_append("uint(0)");
			else
				buffer.asprintf_append("%uu", ir->value.u[0]);
		}
		return;
	}

   const glsl_type *const base_type = ir->type->get_base_type();

   print_type(buffer, type, true);
   buffer.asprintf_append ("(");

   if (ir->type->is_array()) {
      for (unsigned i = 0; i < ir->type->length; i++)
      {
	 if (i != 0)
	    buffer.asprintf_append (", ");
	 ir->get_array_element(i)->accept(this);
      }
   } else if (ir->type->is_struct()) {
      bool first = true;
      for (unsigned j = 0; j < ir->type->length; ++j) {
      ir_constant* inst = ir->const_elements[j];
	 if (!first)
	    buffer.asprintf_append (", ");
	 first = false;
	 inst->accept(this);
     } 
   }else {
      bool first = true;
      for (unsigned i = 0; i < ir->type->components(); i++) {
	 if (!first)
	    buffer.asprintf_append (", ");
	 first = false;
	 switch (base_type->base_type) {
	 case GLSL_TYPE_UINT:
	 {
		 // GLSL < 130 does not support uints
		 if (state->language_version < 130)
			 buffer.asprintf_append("%u", ir->value.u[i]);
		 else
			 buffer.asprintf_append("%uu", ir->value.u[i]);
		 break;
	 }
	 case GLSL_TYPE_INT:
	 {
		 // Need special handling for INT_MIN
		 if (ir->value.u[i] == 0x80000000)
			 buffer.asprintf_append("int(0x%X)", ir->value.i[i]);
		 else
			 buffer.asprintf_append("%d", ir->value.i[i]);
		 break;
	 }
	 case GLSL_TYPE_FLOAT: print_float(buffer, ir->value.f[i]); break;
	 case GLSL_TYPE_BOOL:  buffer.asprintf_append ("%d", ir->value.b[i]); break;
	 default: assert(0);
	 }
      }
   }
   buffer.asprintf_append (")");
}


void
ir_print_glsl_visitor::visit(ir_call *ir)
{
	// calls in global scope are postponed to main function
	if (this->mode != kPrintGlslNone)
	{
		assert (!this->globals->main_function_done);
		this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry(ir));
		buffer.asprintf_append ("//"); // for the ; that will follow (ugly, I know)
		return;
	}
	
	if (ir->return_deref)
	{
		visit(ir->return_deref);
		buffer.asprintf_append (" = ");		
	}
	
   const auto* counter = find_atomic_counter(ir->callee);
   const auto* atomic = find_buffer_atomic(ir->callee);
   const auto* barrier = find_memory_barrier(ir->callee);
   const char* image = image_function_name(ir->callee);
   buffer.asprintf_append ("%s (", counter ? counter->glsl : atomic ? atomic->glsl : barrier ? barrier->glsl : image ? image : ir->callee_name());
   bool first = true;
   foreach_in_list(ir_instruction, inst, &ir->actual_parameters) {
	  if (!first)
		  buffer.asprintf_append (", ");
      inst->accept(this);
	  first = false;
   }
   buffer.asprintf_append (")");
}


void
ir_print_glsl_visitor::visit(ir_return *ir)
{
   buffer.asprintf_append ("return");

   ir_rvalue *const value = ir->get_value();
   if (value) {
      buffer.asprintf_append (" ");
      value->accept(this);
   }
}


void
ir_print_glsl_visitor::visit(ir_discard *ir)
{
   if (ir->condition) {
      buffer.asprintf_append("if (");
      ir->condition->accept(this);
      buffer.asprintf_append(") ");
   }
   buffer.asprintf_append("discard");
}


void
ir_print_glsl_visitor::visit(ir_if *ir)
{
   buffer.asprintf_append ("if (");
   ir->condition->accept(this);

   buffer.asprintf_append (") {\n");
	indentation++; previous_skipped = false;


   foreach_in_list(ir_instruction, inst, &ir->then_instructions) {
      indent();
      inst->accept(this);
	   end_statement_line();
   }

   indentation--;
   indent();
   buffer.asprintf_append ("}");

   if (!ir->else_instructions.is_empty())
   {
	   buffer.asprintf_append (" else {\n");
	   indentation++; previous_skipped = false;

	   foreach_in_list(ir_instruction, inst, &ir->else_instructions) {
		  indent();
		  inst->accept(this);
		   end_statement_line();
	   }
	   indentation--;
	   indent();
	   buffer.asprintf_append ("}");
   }
}

bool ir_print_glsl_visitor::emit_canonical_for (ir_loop* ir)
{
	loop_variable_state* const ls = this->loopstate->get(ir);

	if (!printable_for_loop(ir, ls)) return false;

	std::unordered_set<ir_instruction*> terminator_hash;
	std::unordered_set<ir_instruction*> induction_hash;
	
	buffer.asprintf_append("for (");
	
	const auto init = loop_initializers->initializers.find(ir);
	if (init != loop_initializers->initializers.end()) {
		ir_variable* var = init->second->whole_variable_written();
		print_precision(var, var->type);
		print_type(buffer, var->type, false);
		buffer.asprintf_append(" ");
		emit_assignment_part(init->second->lhs, init->second->rhs, init->second->write_mask, nullptr);
	}
	buffer.asprintf_append("; ");
	
	// emit loop terminating conditions
	foreach_in_list(loop_terminator, term, &ls->terminators)
	{
		terminator_hash.insert(term->ir);
		
		// IR has conditions in the form of "if (x) break",
		// whereas for loop needs them negated, in the form
		// if "while (x) continue the loop".
		// See if we can print them using syntax that reads nice.
		bool handled = false;
		ir_expression* term_expr = term->ir->condition->as_expression();
		if (term_expr)
		{
			// Binary comparison conditions
			const char* termOp = NULL;
			switch (term_expr->operation)
			{
				case ir_binop_less: termOp = ">="; break;
				case ir_binop_gequal: termOp = "<"; break;
				case ir_binop_equal: termOp = "!="; break;
				case ir_binop_nequal: termOp = "=="; break;
				default: break;
			}
			if (termOp != NULL)
			{
				term_expr->operands[0]->accept(this);
				buffer.asprintf_append(" %s ", termOp);
				term_expr->operands[1]->accept(this);
				handled = true;
			}
			
			// Unary logic not
			if (!handled && term_expr->operation == ir_unop_logic_not)
			{
				term_expr->operands[0]->accept(this);
				handled = true;
			}
		}
		
		// More complex condition, print as "!(x)"
		if (!handled)
		{
			buffer.asprintf_append("!(");
			term->ir->condition->accept(this);
			buffer.asprintf_append(")");
		}
	}
	buffer.asprintf_append("; ");
	
	// emit loop induction variable updates
	bool first = true;
	foreach_in_list(loop_variable, indvar, &ls->induction_variables)
	{
		if (indvar != printed_for_induction(ir, ls)) continue;
		induction_hash.insert(indvar->first_assignment);
		if (!first)
			buffer.asprintf_append(", ");
		visit(indvar->first_assignment);
		first = false;
	}
	buffer.asprintf_append(") {\n");
	
	
	// emit loop body
	indentation++; previous_skipped = false;
	foreach_in_list(ir_instruction, inst, &ir->body_instructions) {

		// skip termination & induction statements,
		// they are part of "for" clause
		if (terminator_hash.count(inst))
			continue;
		if (induction_hash.count(inst))
			continue;
		
		indent();
		inst->accept(this);
		end_statement_line();
	}
	indentation--;
	
	indent();
	buffer.asprintf_append("}");
	


	
	return true;
}


void
ir_print_glsl_visitor::visit(ir_loop *ir)
{
	if (emit_canonical_for(ir))
		return;
	
	buffer.asprintf_append ("while (true) {\n");
	indentation++; previous_skipped = false;
	foreach_in_list(ir_instruction, inst, &ir->body_instructions) {
		indent();
		inst->accept(this);
		end_statement_line();
	}
	indentation--;
	indent();
	buffer.asprintf_append ("}");
}


void
ir_print_glsl_visitor::visit(ir_loop_jump *ir)
{
   buffer.asprintf_append ("%s", ir->is_break() ? "break" : "continue");
}

void
ir_print_glsl_visitor::visit(ir_emit_vertex *ir)
{
	buffer.asprintf_append ("emit-vertex-TODO");
}

void
ir_print_glsl_visitor::visit(ir_end_primitive *ir)
{
	buffer.asprintf_append ("end-primitive-TODO");
}
