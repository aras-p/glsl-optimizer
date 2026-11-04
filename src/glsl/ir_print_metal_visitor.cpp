/*
 * Copyright © 2014 Unity Technologies
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

#include "ir_print_metal_visitor.h"
#include "ir_visitor.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "ir_hierarchical_visitor.h"
#include "ir_optimization.h"
#include <unordered_map>
#include <unordered_set>
#include "loop_analysis.h"
#include "ir_print_loop.h"

#include <math.h>


static unsigned precision_from_ir(ir_instruction* ir)
{
	if (!ir) return GLSL_PRECISION_NONE;
	if (ir_variable* v = ir->as_variable()) return v->data.precision;
	if (ir_rvalue* r = ir->as_rvalue()) return r->get_precision();
	if (ir->ir_type == ir_type_function_signature) return ((ir_function_signature*)ir)->return_precision;
	return GLSL_PRECISION_NONE;
}
static unsigned higher_precision(unsigned a, unsigned b)
{
	if (!a) return b;
	if (!b) return a;
	return a < b ? a : b;
}
static void print_type_precision(string_buffer&, const glsl_type*, unsigned, bool);
static void print_metal_structs(string_buffer&, exec_list*);

static void print_type(string_buffer& buffer, ir_instruction* ir, const glsl_type *t, bool arraySize);
static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize);


struct ga_entry_metal : public exec_node
{
	ga_entry_metal(ir_instruction* ir)
	{
		assert(ir);
		this->ir = ir;
	}
	ir_instruction* ir;
};
struct gconst_entry_metal : public exec_node
{
	gconst_entry_metal(ir_constant* ir, unsigned id)
	{
		assert(ir);
		this->ir = ir;
		this->id = id;
	}
	ir_constant* ir;
	unsigned id;
};


struct global_print_tracker_metal
{
	global_print_tracker_metal ()
	{
		mem_ctx = ralloc_context(0);
		var_counter = 0;
		const_counter = 0;
		main_function_done = false;
	}

	~global_print_tracker_metal()
	{
		ralloc_free(mem_ctx);
	}

	unsigned	var_counter;
	std::unordered_map<const ir_variable*, long> var_hash;
	exec_list	global_assignements;

	unsigned	const_counter;
	std::unordered_map<const ir_constant*, long> const_hash;
	exec_list	global_constants;

	void*		mem_ctx;
	bool		main_function_done;
};


struct metal_print_context
{
	metal_print_context(char* buffer)
	: str(buffer)
	, prefixStr(ralloc_strdup(buffer, ""))
	, inputStr(ralloc_strdup(buffer, ""))
	, outputStr(ralloc_strdup(buffer, ""))
	, inoutStr(ralloc_strdup(buffer, ""))
	, uniformStr(ralloc_strdup(buffer, ""))
	, paramsStr(ralloc_strdup(buffer, ""))
	, typedeclStr(ralloc_strdup(buffer, ""))
	, writingParams(false)
	, matrixCastsDone(false)
	, matrixConstructorsDone(false)
	, shadowSamplerDone(false)
	, textureCounter(0)
	, attributeCounter(0)
	, uniformLocationCounter(0)
	, colorCounter(0)
	{
	}

	string_buffer str;
	string_buffer prefixStr;
	string_buffer inputStr;
	string_buffer outputStr;
	string_buffer inoutStr;
	string_buffer uniformStr;
	string_buffer paramsStr;
	string_buffer typedeclStr;
	bool writingParams;
	bool matrixCastsDone;
	bool matrixConstructorsDone;
	bool shadowSamplerDone;
	int textureCounter;
	int attributeCounter;
	int uniformLocationCounter;
	int colorCounter;
};


class ir_print_metal_visitor : public ir_visitor {
public:
	ir_print_metal_visitor(metal_print_context& ctx_, string_buffer& buf, global_print_tracker_metal* globals_, PrintGlslMode mode_, const _mesa_glsl_parse_state* state_)
		: ctx(ctx_)
		, buffer(buf)
		, loopstate(NULL)
		, inside_lhs(false)
		, skipped_this_ir(false)
		, previous_skipped(false)
		, mode_whole(mode_)
	{
		indentation = 0;
		expression_depth = 0;
		globals = globals_;
		mode = mode_;
		state = state_;
	}

	virtual ~ir_print_metal_visitor()
	{
	}


	void indent(void);
	void newline_indent();
	void end_statement_line();
	void newline_deindent();
	void print_var_name (ir_variable* v);
	void print_array_initializer(ir_constant* value, unsigned precision);

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
	virtual void visit(ir_demote *) { buffer.asprintf_append("discard_fragment()"); }
	virtual void visit(ir_barrier *) { buffer.asprintf_append("threadgroup_barrier(mem_flags::mem_threadgroup)"); }
	virtual void visit(ir_emit_vertex *);
	virtual void visit(ir_end_primitive *);

	void emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex);

	bool emit_canonical_for (ir_loop* ir);

	metal_print_context& ctx;
	int indentation;
	int expression_depth;
	string_buffer& buffer;
	global_print_tracker_metal* globals;
	const _mesa_glsl_parse_state* state;
	PrintGlslMode mode;
	const PrintGlslMode mode_whole;
	loop_state* loopstate;
	print_loop_initializers* loop_initializers = nullptr;
	bool	inside_lhs;
	bool	skipped_this_ir;
	bool	previous_skipped;
};


char*
_mesa_print_ir_metal(exec_list *instructions,
	    struct _mesa_glsl_parse_state *state,
		char* buffer, PrintGlslMode mode, int* outUniformsSize)
{
	if (mode == kPrintGlslFragment) optimize_split_arrays(instructions, false, true);
	metal_print_context ctx(buffer);

	// includes, prefix etc.
	ctx.prefixStr.asprintf_append ("#include <metal_stdlib>\n");
	ctx.prefixStr.asprintf_append ("#pragma clang diagnostic ignored \"-Wparentheses-equality\"\n");
	ctx.prefixStr.asprintf_append ("using namespace metal;\n");

	ctx.inputStr.asprintf_append("struct xlatMtlShaderInput {\n");
	ctx.outputStr.asprintf_append("struct xlatMtlShaderOutput {\n");
	ctx.uniformStr.asprintf_append("struct xlatMtlShaderUniform {\n");

	print_metal_structs(ctx.typedeclStr, instructions);

	global_print_tracker_metal gtracker;

	loop_state* ls = analyze_loop_variables(instructions);
	print_loop_initializers initializers(instructions, ls);


	foreach_in_list(ir_instruction, ir, instructions)
	{
		string_buffer* strOut = &ctx.str;
		ctx.writingParams = false;
		if (ir->ir_type == ir_type_variable)
		{
			ir_variable *var = static_cast<ir_variable*>(ir);

			// skip gl_ variables if they aren't used/assigned
			if (strstr(var->name, "gl_") == var->name)
			{
				if (!var->data.used && !var->data.assigned)
					continue;
			}

			//
			if (var->data.mode == ir_var_uniform)
			{
				if (var->type->without_array()->is_sampler())
				{
					strOut = &ctx.paramsStr;
					ctx.writingParams = true;
					strOut->asprintf_append ("\n  , ");
				}
				else
					strOut = &ctx.uniformStr;
			}
			if (var->data.mode == ir_var_system_value)
			{
				strOut = &ctx.paramsStr;
				ctx.writingParams = true;
				strOut->asprintf_append ("\n  , ");
			}
			if (var->data.mode == ir_var_shader_in)
				strOut = &ctx.inputStr;
			if (var->data.mode == ir_var_shader_out)
				strOut = &ctx.outputStr;
			if (var->data.mode == ir_var_shader_out && var->data.fb_fetch_output)
				strOut = &ctx.inoutStr;
		}
		

		ir_print_metal_visitor v (ctx, *strOut, &gtracker, mode, state);
		v.loopstate = ls;
		v.loop_initializers = &initializers;

		if (strOut == &ctx.inputStr || strOut == &ctx.outputStr || strOut == &ctx.inoutStr || strOut == &ctx.uniformStr)
			strOut->asprintf_append("  ");
		ir->accept(&v);
		if (ir->ir_type != ir_type_function && !v.skipped_this_ir)
		{
			if (!ctx.writingParams)
				strOut->asprintf_append (";\n");
		}
	}

	delete ls;

	// append inout variables to both input & output structs
	if (!ctx.inoutStr.empty())
	{
		ctx.inputStr.asprintf_append("%s", ctx.inoutStr.c_str());
		ctx.outputStr.asprintf_append("%s", ctx.inoutStr.c_str());
	}
	ctx.inputStr.asprintf_append("};\n");
	ctx.outputStr.asprintf_append("};\n");
	ctx.uniformStr.asprintf_append("};\n");

	// emit global array/struct constants
	
	ctx.prefixStr.asprintf_append("%s", ctx.typedeclStr.c_str());
	foreach_in_list_safe(gconst_entry_metal, node, &gtracker.global_constants)
	{
		ir_constant* c = node->ir;

		ir_print_metal_visitor v (ctx, ctx.prefixStr, &gtracker, mode, state);

		v.buffer.asprintf_append ("constant ");
		print_type(v.buffer, c, c->type, false);
		v.buffer.asprintf_append (" _xlat_mtl_const%i", (int)((gconst_entry_metal*)node)->id);
		print_type_post(v.buffer, c->type, false);
		v.buffer.asprintf_append (" = {");

		if (c->type->is_array())
		{
			for (unsigned i = 0; i < c->type->length; i++)
			{
				if (i != 0)
					v.buffer.asprintf_append (", ");
				ir_constant* element = c->get_array_element(i);
				// Array precision applies to its elements, including propagated constants.
				if ((c->get_precision() == GLSL_PRECISION_MEDIUM) !=
					(element->get_precision() == GLSL_PRECISION_MEDIUM))
					v.print_array_initializer(element, c->get_precision());
				else
					element->accept(&v);
			}
		}
		else
		{
			assert(c->type->is_struct());
			bool first = true;
			for (unsigned component = 0; component < c->type->length; ++component)
			{
				if (!first)
					v.buffer.asprintf_append (", ");
				first = false;
				const glsl_struct_field& field = c->type->fields.structure[component];
				const bool cast = field.type->is_numeric() && field.precision == GLSL_PRECISION_MEDIUM;
				if (cast) { print_type_precision(v.buffer, field.type, field.precision, true); v.buffer.asprintf_append("("); }
				c->const_elements[component]->accept(&v);
				if (cast) v.buffer.asprintf_append(")");
			}
		}
		v.buffer.asprintf_append ("};\n");
	}


	ctx.prefixStr.asprintf_append("%s", ctx.inputStr.c_str());
	ctx.prefixStr.asprintf_append("%s", ctx.outputStr.c_str());
	ctx.prefixStr.asprintf_append("%s", ctx.uniformStr.c_str());
	ctx.prefixStr.asprintf_append("%s", ctx.str.c_str());

	*outUniformsSize = ctx.uniformLocationCounter;

	return ralloc_strdup(buffer, ctx.prefixStr.c_str());
}


void ir_print_metal_visitor::indent(void)
{
	if (previous_skipped)
		return;
	previous_skipped = false;
	for (int i = 0; i < indentation; i++)
		buffer.asprintf_append ("  ");
}

void ir_print_metal_visitor::end_statement_line()
{
	if (!skipped_this_ir)
		buffer.asprintf_append(";\n");
	previous_skipped = skipped_this_ir;
	skipped_this_ir = false;
}

void ir_print_metal_visitor::newline_indent()
{
	if (expression_depth % 4 == 0)
	{
		++indentation;
		buffer.asprintf_append ("\n");
		indent();
	}
}
void ir_print_metal_visitor::newline_deindent()
{
	if (expression_depth % 4 == 0)
	{
		--indentation;
		buffer.asprintf_append ("\n");
		indent();
	}
}


void ir_print_metal_visitor::print_var_name (ir_variable* v)
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



static void print_type_precision(string_buffer& buffer, const glsl_type *t, unsigned prec, bool arraySize)
{
	const bool halfPrec = (prec == GLSL_PRECISION_MEDIUM);

	const char* typeName = t->name;
	// scalars
	if (!strcmp(typeName, "float"))
		typeName = halfPrec ? "half" : "float";
	else if (!strcmp(typeName, "int"))
		typeName = halfPrec ? "short" : "int";
	else if (!strcmp(typeName, "uint")) typeName = halfPrec ? "ushort" : "uint";
	else if (!strcmp(typeName, "uvec2")) typeName = halfPrec ? "ushort2" : "uint2";
	else if (!strcmp(typeName, "uvec3")) typeName = halfPrec ? "ushort3" : "uint3";
	else if (!strcmp(typeName, "uvec4")) typeName = halfPrec ? "ushort4" : "uint4";
	// vectors
	else if (!strcmp(typeName, "vec2"))
		typeName = halfPrec ? "half2" : "float2";
	else if (!strcmp(typeName, "vec3"))
		typeName = halfPrec ? "half3" : "float3";
	else if (!strcmp(typeName, "vec4"))
		typeName = halfPrec ? "half4" : "float4";
	else if (!strcmp(typeName, "ivec2"))
		typeName = halfPrec ? "short2" : "int2";
	else if (!strcmp(typeName, "ivec3"))
		typeName = halfPrec ? "short3" : "int3";
	else if (!strcmp(typeName, "ivec4"))
		typeName = halfPrec ? "short4" : "int4";
	else if (!strcmp(typeName, "bvec2"))
		typeName = "bool2";
	else if (!strcmp(typeName, "bvec3"))
		typeName = "bool3";
	else if (!strcmp(typeName, "bvec4"))
		typeName = "bool4";
	// matrices
	else if (!strcmp(typeName, "mat2"))
		typeName = halfPrec ? "half2x2" : "float2x2";
	else if (!strcmp(typeName, "mat3"))
		typeName = halfPrec ? "half3x3" : "float3x3";
	else if (!strcmp(typeName, "mat4"))
		typeName = halfPrec ? "half4x4" : "float4x4";
	// non-square matrices
	else if (!strcmp(typeName, "mat2x2"))
		typeName = halfPrec ? "half2x2" : "float2x2";
	else if (!strcmp(typeName, "mat2x3"))
		typeName = halfPrec ? "half2x3" : "float2x3";
	else if (!strcmp(typeName, "mat2x4"))
		typeName = halfPrec ? "half2x4" : "float2x4";
	else if (!strcmp(typeName, "mat3x2"))
		typeName = halfPrec ? "half3x2" : "float3x2";
	else if (!strcmp(typeName, "mat3x3"))
		typeName = halfPrec ? "half3x3" : "float3x3";
	else if (!strcmp(typeName, "mat3x4"))
		typeName = halfPrec ? "half3x4" : "float3x4";
	else if (!strcmp(typeName, "mat4x2"))
		typeName = halfPrec ? "half4x2" : "float4x2";
	else if (!strcmp(typeName, "mat4x3"))
		typeName = halfPrec ? "half4x3" : "float4x3";
	else if (!strcmp(typeName, "mat4x4"))
		typeName = halfPrec ? "half4x4" : "float4x4";
	// samplers
	else if (!strcmp(typeName, "sampler2D") || !strcmp(typeName, "samplerExternalOES"))
		typeName = halfPrec ? "texture2d<half>" : "texture2d<float>";
	else if (!strcmp(typeName, "samplerCube"))
		typeName = halfPrec ? "texturecube<half>" : "texturecube<float>";
	else if (!strcmp(typeName, "sampler3D"))
		typeName = halfPrec ? "texture3d<half>" : "texture3d<float>";
	else if (!strcmp(typeName, "sampler2DShadow"))
		typeName = "depth2d<float>"; // depth type must always be float
	else if (!strcmp(typeName, "samplerCubeShadow"))
		typeName = "depthcube<float>"; // depth type must always be float
	else if (!strcmp(typeName, "sampler2DArray"))
		typeName = halfPrec ? "texture2d_array<half>" : "texture2d_array<float>";

	if (t->base_type == GLSL_TYPE_ARRAY) {
		print_type_precision(buffer, t->fields.array, prec, true);
		if (arraySize)
			buffer.asprintf_append ("[%u]", t->length);
	} else if ((t->base_type == GLSL_TYPE_STRUCT)
			   && (strncmp("gl_", typeName, 3) != 0)) {
		buffer.asprintf_append ("%s", typeName);
	} else {
		buffer.asprintf_append ("%s", typeName);
	}
}


static void print_type(string_buffer& buffer, ir_instruction* ir, const glsl_type *t, bool arraySize)
{
	unsigned prec = precision_from_ir(ir);
	print_type_precision(buffer, t, prec, arraySize);
}


static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize)
{
	if (t->base_type == GLSL_TYPE_ARRAY) {
		if (!arraySize)
			buffer.asprintf_append ("[%u]", t->length);
	}
}

class metal_struct_declarations : public ir_hierarchical_visitor {
public:
	string_buffer& out;
	std::unordered_set<const glsl_type*> emitted;
	metal_struct_declarations(string_buffer& out) : out(out) {}
	void emit(const glsl_type* type) {
		type = type->without_array();
		if (!type->is_struct() || !emitted.insert(type).second) return;
		for (unsigned i = 0; i < type->length; ++i) emit(type->fields.structure[i].type);
		out.asprintf_append("struct %s {\n", type->name);
		for (unsigned i = 0; i < type->length; ++i) {
			const glsl_struct_field& f = type->fields.structure[i];
			out.asprintf_append("  "); print_type_precision(out, f.type, f.precision, false);
			out.asprintf_append(" %s", f.name); print_type_post(out, f.type, false);
			out.asprintf_append(";\n");
		}
		out.asprintf_append("};\n");
	}
	ir_visitor_status visit(ir_variable* v) override { emit(v->type); return visit_continue; }
	ir_visitor_status visit(ir_constant* c) override { emit(c->type); return visit_continue; }
	ir_visitor_status visit_enter(ir_function_signature* f) override { emit(f->return_type); return visit_continue; }
};
static void print_metal_structs(string_buffer& out, exec_list* ir)
{
	metal_struct_declarations visitor(out); visit_list_elements(&visitor, ir);
}

static void get_metal_type_size(const glsl_type* type, unsigned prec, int& size, int& alignment)
{
	if (type->is_array()) {
		get_metal_type_size(type->fields.array, prec, size, alignment);
		size = ((size + alignment - 1) & ~(alignment - 1)) * type->length;
		return;
	}
	if (type->is_struct()) {
		size = 0; alignment = 1;
		for (unsigned i = 0; i < type->length; ++i) {
			int field_size, field_align;
			get_metal_type_size(type->fields.structure[i].type, type->fields.structure[i].precision, field_size, field_align);
			size = ((size + field_align - 1) & ~(field_align - 1)) + field_size;
			alignment = MAX2(alignment, field_align);
		}
		size = (size + alignment - 1) & ~(alignment - 1);
		return;
	}
	int scalar = type->is_boolean() ? 1 : prec == GLSL_PRECISION_MEDIUM ? 2 : 4;
	int vector = type->vector_elements == 3 ? 4 : type->vector_elements;
	alignment = scalar * vector;
	size = alignment * type->matrix_columns;
}

void ir_print_metal_visitor::print_array_initializer(ir_constant* value, unsigned precision)
{
	if (value->type->is_array()) {
		buffer.asprintf_append("{");
		for (unsigned i = 0; i < value->type->length; ++i) {
			if (i) buffer.asprintf_append(", ");
			print_array_initializer(value->get_array_element(i), precision);
		}
		buffer.asprintf_append("}");
	} else {
		const bool cast = value->type->is_numeric();
		if (cast) { print_type_precision(buffer, value->type, precision, true); buffer.asprintf_append("("); }
		value->accept(this);
		if (cast) buffer.asprintf_append(")");
	}
}

void ir_print_metal_visitor::visit(ir_variable *ir)
{
	const char* storage = (ir->data.mode == ir_var_function_out || ir->data.mode == ir_var_function_inout) ? "thread " : "";

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

	// auto/temp variables in global scope are postponed to main function
	if (this->mode != kPrintGlslNone && (ir->data.mode == ir_var_auto || ir->data.mode == ir_var_temporary))
	{
		assert (!this->globals->main_function_done);
		this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry_metal(ir));
		skipped_this_ir = true;
		return;
	}

	buffer.asprintf_append ("%s", storage);
	if (!strcmp(ir->name, "gl_VertexID") || !strcmp(ir->name, "gl_InstanceID"))
		buffer.asprintf_append("uint");
	else if (!strcmp(ir->name, "gl_PointCoord"))
		buffer.asprintf_append("float2");
	else
		print_type(buffer, ir, ir->type, false);
	buffer.asprintf_append (" ");
	if (ir->data.mode == ir_var_function_out || ir->data.mode == ir_var_function_inout) buffer.asprintf_append("&");
	print_var_name (ir);
	print_type_post(buffer, ir->type, false);

	// special built-in variables
	if (!strcmp(ir->name, "gl_FragDepth"))
		buffer.asprintf_append (" [[depth(any)]]");
	else if (!strcmp(ir->name, "gl_FragCoord"))
		buffer.asprintf_append (" [[position]]");
	else if (!strcmp(ir->name, "gl_FrontFacing"))
		buffer.asprintf_append (" [[front_facing]]");
	else if (!strcmp(ir->name, "gl_PointCoord"))
		buffer.asprintf_append (" [[point_coord]]");
	else if (!strcmp(ir->name, "gl_PointSize"))
		buffer.asprintf_append (" [[point_size]]");
	else if (!strcmp(ir->name, "gl_Position"))
		buffer.asprintf_append (" [[position]]");
	else if (!strcmp(ir->name, "gl_VertexID"))
		buffer.asprintf_append (" [[vertex_id]]");
	else if (!strcmp(ir->name, "gl_InstanceID"))
		buffer.asprintf_append (" [[instance_id]]");

	// Interpolation belongs to fragment inputs / vertex outputs, not attributes
	// or render targets. Metal combines interpolation and sampling location.
	const bool varying = (mode_whole == kPrintGlslFragment && ir->data.mode == ir_var_shader_in) ||
		(mode_whole == kPrintGlslVertex && ir->data.mode == ir_var_shader_out);
	if (varying && strncmp(ir->name, "gl_", 3) != 0) {
		if (ir->data.interpolation == INTERP_MODE_FLAT)
			buffer.asprintf_append(" [[flat]]");
		else if (ir->data.centroid || ir->data.sample || ir->data.interpolation != INTERP_MODE_NONE)
			buffer.asprintf_append(" [[%s_%s]]",
				ir->data.sample ? "sample" : ir->data.centroid ? "centroid" : "center",
				ir->data.interpolation == INTERP_MODE_NOPERSPECTIVE ? "no_perspective" : "perspective");
	}
	// Metal supports invariance on the position output only.
	if (mode_whole == kPrintGlslVertex && !strcmp(ir->name, "gl_Position") && ir->data.invariant)
		buffer.asprintf_append(" [[invariant]]");

	// vertex shader input attribute?
	if (this->mode_whole == kPrintGlslVertex && ir->data.mode == ir_var_shader_in)
	{
		buffer.asprintf_append (" [[attribute(%i)]]", ctx.attributeCounter);
		ir->data.explicit_location = 1;
		ir->data.location = ctx.attributeCounter;
		++ctx.attributeCounter;
	}

	// fragment shader output?
	if (this->mode_whole == kPrintGlslFragment && ir->data.mode == ir_var_shader_out)
	{
		if (!ir->data.explicit_location)
		{
			ir->data.explicit_location = 1;
			ir->data.location = FRAG_RESULT_DATA0 + ctx.colorCounter;
			++ctx.colorCounter;
		}

		if (ir->data.explicit_location)
		{
			const int binding_base = (int)FRAG_RESULT_DATA0;
			const int location = ir->data.location - binding_base;
			if (location >= 0 && !ir->type->is_array())
				buffer.asprintf_append (" [[color(%d)]]", location);
		}
	}

	// uniform texture?
	if (ir->data.mode == ir_var_uniform && ctx.writingParams)
	{
		buffer.asprintf_append (" [[texture(%i)]]", ctx.textureCounter);
		buffer.asprintf_append (", sampler _mtlsmp_%s [[sampler(%i)]]", ir->name, ctx.textureCounter);
		ir->data.explicit_location = 1;
		ir->data.location = ctx.textureCounter;
		++ctx.textureCounter;
	}
	// regular uniform?
	if (ir->data.mode == ir_var_uniform && !ctx.writingParams)
	{
		int size, align;
		get_metal_type_size(ir->type, (unsigned)ir->data.precision, size, align);

		int loc = ctx.uniformLocationCounter;
		loc = (loc + align-1) & ~(align-1); // align it

		ir->data.explicit_location = 1;
		ir->data.location = loc;

		loc += size;
		ctx.uniformLocationCounter = loc;
	}

	if (ir->constant_value &&
		ir->data.mode != ir_var_shader_in &&
		ir->data.mode != ir_var_shader_out &&
		ir->data.mode != ir_var_function_in &&
		ir->data.mode != ir_var_function_out &&
		ir->data.mode != ir_var_function_inout)
	{
		buffer.asprintf_append (" = ");
		if (ir->type->is_array()) print_array_initializer(ir->constant_value, ir->data.precision);
		else visit (ir->constant_value);
	}

	if (!ir->constant_value && (ir->data.mode == ir_var_auto || ir->data.mode == ir_var_temporary) && !ir->type->is_array() && (ir->type->matrix_columns == 1)) {
		switch (ir->type->base_type) {
			case GLSL_TYPE_INT:
			case GLSL_TYPE_FLOAT:
				buffer.asprintf_append (" = 0");
				break;
			case GLSL_TYPE_BOOL:
				buffer.asprintf_append (" = false");
				break;
			default:
				break;
		}
	}
}


void ir_print_metal_visitor::visit(ir_function_signature *ir)
{
	const bool isMain = (strcmp(ir->function()->name, "main") == 0);

	if (!isMain)
	{
		print_type(buffer, ir, ir->return_type, true);
		buffer.asprintf_append (" %s (", ir->function_name());

		if (!ir->parameters.is_empty())
		{
			buffer.asprintf_append ("\n");

			indentation++; previous_skipped = false;
			bool first = true;
			foreach_in_list(ir_variable, inst, &ir->parameters)
			{
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
	}
	else
	{
		if (this->mode_whole == kPrintGlslFragment)
			buffer.asprintf_append ("fragment ");
		if (this->mode_whole == kPrintGlslVertex)
			buffer.asprintf_append ("vertex ");
		buffer.asprintf_append ("xlatMtlShaderOutput xlatMtlMain (xlatMtlShaderInput _mtl_i [[stage_in]], constant xlatMtlShaderUniform& _mtl_u [[buffer(0)]]");
		if (!ctx.paramsStr.empty())
		{
			buffer.asprintf_append ("%s", ctx.paramsStr.c_str());
		}
	}

   if (ir->body.is_empty())
   {
	   buffer.asprintf_append (");\n");
	   return;
   }

   buffer.asprintf_append (")\n");

   indent();
   buffer.asprintf_append ("{\n");
   indentation++; previous_skipped = false;

	if (isMain)
	{
		// output struct
		indent(); buffer.asprintf_append ("xlatMtlShaderOutput _mtl_o;\n");

		// insert postponed global assigments and variable declarations
		assert (!globals->main_function_done);
		globals->main_function_done = true;
		foreach_in_list(ga_entry_metal, node, &globals->global_assignements)
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

	if (isMain)
	{
		// return stuff
		indent(); buffer.asprintf_append ("return _mtl_o;\n");
	}

   indentation--;
   indent();
   buffer.asprintf_append ("}\n");
}

void ir_print_metal_visitor::visit(ir_function *ir)
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


static const char* metal_operator(ir_expression_operation op)
{
	switch (op) {
	case ir_unop_bit_not: return "~";
	case ir_unop_logic_not: return "!";
	case ir_unop_neg: return "-";
	case ir_unop_abs: return "abs";
	case ir_unop_sign: return "sign";
	case ir_unop_rcp: return "1.0/";
	case ir_unop_rsq: return "rsqrt";
	case ir_unop_sqrt: return "sqrt";
	case ir_unop_normalize: return "normalize";
	case ir_unop_exp: return "exp";
	case ir_unop_log: return "log";
	case ir_unop_exp2: return "exp2";
	case ir_unop_log2: return "log2";
	case ir_unop_f2i: return "int";
	case ir_unop_f2u: return "int";
	case ir_unop_i2f: return "float";
	case ir_unop_f2b: return "bool";
	case ir_unop_b2f: return "float";
	case ir_unop_i2b: return "bool";
	case ir_unop_b2i: return "int";
	case ir_unop_u2f: return "float";
	case ir_unop_i2u: return "int";
	case ir_unop_u2i: return "int";
	case ir_unop_bitcast_i2f: return "as_type_";
	case ir_unop_bitcast_f2i: return "as_type_";
	case ir_unop_bitcast_u2f: return "as_type_";
	case ir_unop_bitcast_f2u: return "as_type_";
	case ir_unop_trunc: return "trunc";
	case ir_unop_ceil: return "ceil";
	case ir_unop_floor: return "floor";
	case ir_unop_fract: return "fract";
	case ir_unop_round_even: return "rint";
	case ir_unop_sin: return "sin";
	case ir_unop_cos: return "cos";
	case ir_unop_dFdx: return "dfdx";
	case ir_unop_dFdx_coarse: return "dfdx";
	case ir_unop_dFdx_fine: return "dfdx";
	case ir_unop_dFdy: return "dfdy";
	case ir_unop_dFdy_coarse: return "dfdy";
	case ir_unop_dFdy_fine: return "dfdy";
	case ir_unop_pack_snorm_2x16: return "packSnorm2x16";
	case ir_unop_pack_snorm_4x8: return "packSnorm4x8";
	case ir_unop_pack_unorm_2x16: return "packUnorm2x16";
	case ir_unop_pack_unorm_4x8: return "packUnorm4x8";
	case ir_unop_pack_half_2x16: return "packHalf2x16";
	case ir_unop_unpack_snorm_2x16: return "unpackSnorm2x16";
	case ir_unop_unpack_snorm_4x8: return "unpackSnorm4x8";
	case ir_unop_unpack_unorm_2x16: return "unpackUnorm2x16";
	case ir_unop_unpack_unorm_4x8: return "unpackUnorm4x8";
	case ir_unop_unpack_half_2x16: return "unpackHalf2x16";
	case ir_unop_bitfield_reverse: return "reverse_bits";
	case ir_unop_bit_count: return "popcount";
	case ir_unop_find_msb: return "findMSB";
	case ir_unop_find_lsb: return "findLSB";
	case ir_unop_saturate: return "saturate";
	case ir_binop_add: return "+";
	case ir_binop_sub: return "-";
	case ir_binop_mul: return "*";
	case ir_binop_div: return "/";
	case ir_binop_mod: return "fmod";
	case ir_binop_less: return "<";
	case ir_binop_gequal: return ">=";
	case ir_binop_equal: return "==";
	case ir_binop_nequal: return "!=";
	case ir_binop_all_equal: return "==";
	case ir_binop_any_nequal: return "!=";
	case ir_binop_lshift: return "<<";
	case ir_binop_rshift: return ">>";
	case ir_binop_bit_and: return "&";
	case ir_binop_bit_xor: return "^";
	case ir_binop_bit_or: return "|";
	case ir_binop_logic_and: return "&&";
	case ir_binop_logic_xor: return "!=";
	case ir_binop_logic_or: return "||";
	case ir_binop_dot: return "dot";
	case ir_binop_min: return "min";
	case ir_binop_max: return "max";
	case ir_binop_pow: return "pow";
	case ir_triop_fma: return "fma";
	case ir_triop_lrp: return "mix";
	default: return ir_expression_operation_strings[op];
	}
}

static bool is_binop_func_like(ir_expression_operation op, const glsl_type* type)
{
	if ((op == ir_binop_mod && type->is_float()) ||
		(op >= ir_binop_dot && op <= ir_binop_pow))
		return true;
	return false;
}


static bool is_different_precision(unsigned a, unsigned b)
{
	// Treat "undefined" as high precision
	if (a == GLSL_PRECISION_NONE)
		a = GLSL_PRECISION_HIGH;
	if (b == GLSL_PRECISION_NONE)
		b = GLSL_PRECISION_HIGH;

	return a != b;
}


static void print_cast(string_buffer& buffer, unsigned prec, ir_rvalue* ir)
{
	buffer.asprintf_append ("(");
	print_type_precision(buffer, ir->type, prec, false);
	buffer.asprintf_append (")");
}


void ir_print_metal_visitor::visit(ir_expression *ir)
{
	++this->expression_depth;
	newline_indent();

	unsigned arg_prec = GLSL_PRECISION_NONE;
	if (ir->operands[0])
		arg_prec = higher_precision(arg_prec, ir->operands[0]->get_precision());
	if (ir->operands[1])
		arg_prec = higher_precision(arg_prec, ir->operands[1]->get_precision());
	if (ir->operands[2])
		arg_prec = higher_precision(arg_prec, ir->operands[2]->get_precision());
	unsigned res_prec = ir->get_precision();

	bool op0cast = ir->operands[0] && is_different_precision(arg_prec, ir->operands[0]->get_precision());
	bool op1cast = ir->operands[1] && is_different_precision(arg_prec, ir->operands[1]->get_precision());
	bool op2cast = ir->operands[2] && is_different_precision(arg_prec, ir->operands[2]->get_precision());
	const bool op0matrix = ir->operands[0] && ir->operands[0]->type->is_matrix();
	const bool op1matrix = ir->operands[1] && ir->operands[1]->type->is_matrix();
	bool op0castTo1 = false;
	bool op1castTo0 = false;

	// Metal does not support matrix precision casts, so when any of the arguments is a matrix,
	// take precision from it. This isn't fully robust now, but oh well.
	if (op0cast && op0matrix && !op1cast)
	{
		op0cast = false;
		arg_prec = ir->operands[0]->get_precision();
		op1cast = ir->operands[1] && is_different_precision(arg_prec, ir->operands[1]->get_precision());
	}
	if (op1cast && op1matrix && !op0cast)
	{
		op1cast = false;
		arg_prec = ir->operands[1]->get_precision();
		op0cast = ir->operands[0] && is_different_precision(arg_prec, ir->operands[0]->get_precision());
	}

	// Metal does not have matrix+scalar and matrix-scalar operations; we need to create matrices
	// out of the non-matrix argument.
	if (ir->operation == ir_binop_add || ir->operation == ir_binop_sub)
	{
		if (op0matrix && !op1matrix)
		{
			op1cast = true;
			op1castTo0 = true;
		}
		if (op1matrix && !op0matrix)
		{
			op0cast = true;
			op0castTo1 = true;
		}
		if (op1castTo0 || op0castTo1)
		{
			if (!ctx.matrixConstructorsDone)
			{
				ctx.prefixStr.asprintf_append(
											  "inline float4x4 _xlinit_float4x4(float v) { return float4x4(float4(v), float4(v), float4(v), float4(v)); }\n"
											  "inline float3x3 _xlinit_float3x3(float v) { return float3x3(float3(v), float3(v), float3(v)); }\n"
											  "inline float2x2 _xlinit_float2x2(float v) { return float2x2(float2(v), float2(v)); }\n"
											  "inline half4x4 _xlinit_half4x4(half v) { return half4x4(half4(v), half4(v), half4(v), half4(v)); }\n"
											  "inline half3x3 _xlinit_half3x3(half v) { return half3x3(half3(v), half3(v), half3(v)); }\n"
											  "inline half2x2 _xlinit_half2x2(half v) { return half2x2(half2(v), half2(v)); }\n"
											  );
				ctx.matrixConstructorsDone = true;
			}
		}
	}
	
	
	const bool conversion = ir->operation >= ir_unop_f2i && ir->operation <= ir_unop_u2i;
	const bool rescast = !conversion && is_different_precision(arg_prec, res_prec) && !ir->type->is_boolean();
	if (rescast)
	{
		buffer.asprintf_append ("(");
		print_cast (buffer, res_prec, ir);
	}
	
	if (ir->num_operands == 1)
	{
		if (op0cast && !conversion)
			print_cast (buffer, arg_prec, ir->operands[0]);
		if (ir->operation >= ir_unop_f2i && ir->operation <= ir_unop_u2i) {
			print_type(buffer, ir, ir->type, true);
			buffer.asprintf_append ("(");
		} else if (ir->operation >= ir_unop_bitcast_i2f && ir->operation <= ir_unop_bitcast_f2u) {
			buffer.asprintf_append("as_type<");
			print_type(buffer, ir, ir->type, true);
			buffer.asprintf_append(">(");
		} else if (ir->operation == ir_unop_rcp) {
			const bool halfCast = (arg_prec == GLSL_PRECISION_MEDIUM);
			buffer.asprintf_append (halfCast ? "((half)1.0/(" : "(1.0/(");
		} else {
			buffer.asprintf_append ("%s(", metal_operator(ir->operation));
		}
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append (")");
		if (ir->operation == ir_unop_rcp) {
			buffer.asprintf_append (")");
		}
	}
	else if (ir->operation == ir_triop_csel) {
		buffer.asprintf_append("select(");
		if (op2cast) print_cast(buffer, arg_prec, ir->operands[2]);
		ir->operands[2]->accept(this);
		buffer.asprintf_append(", ");
		if (op1cast) print_cast(buffer, arg_prec, ir->operands[1]);
		ir->operands[1]->accept(this);
		buffer.asprintf_append(", "); ir->operands[0]->accept(this); buffer.asprintf_append(")");
	}
	else if (ir->operands[0]->type->is_vector() &&
		(ir->operation == ir_binop_all_equal || ir->operation == ir_binop_any_nequal)) {
		buffer.asprintf_append(ir->operation == ir_binop_all_equal ? "all(" : "any(");
		if (op0cast) print_cast(buffer, arg_prec, ir->operands[0]);
		ir->operands[0]->accept(this);
		buffer.asprintf_append(" %s ", metal_operator(ir->operation));
		if (op1cast) print_cast(buffer, arg_prec, ir->operands[1]);
		ir->operands[1]->accept(this); buffer.asprintf_append(")");
	}
	else if (ir->operation == ir_quadop_vector) {
		print_type(buffer, ir, ir->type, true); buffer.asprintf_append("(");
		for (unsigned i = 0; i < ir->type->vector_elements; ++i) {
			if (i) buffer.asprintf_append(", "); ir->operands[i]->accept(this);
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
		// binary operation that must be printed like a function, "foo(a,b)"
		if (ir->operation == ir_binop_mod)
		{
			buffer.asprintf_append ("(");
			print_type(buffer, ir, ir->type, true);
			buffer.asprintf_append ("(");
		}
		buffer.asprintf_append ("%s (", metal_operator(ir->operation));
		
		if (ir->operands[0])
		{
			if (op0cast)
				print_cast (buffer, arg_prec, ir->operands[0]);
			ir->operands[0]->accept(this);
		}
		buffer.asprintf_append (", ");
		if (ir->operands[1])
		{
			if (op1cast)
				print_cast (buffer, arg_prec, ir->operands[1]);
			ir->operands[1]->accept(this);
		}
		buffer.asprintf_append (")");
		if (ir->operation == ir_binop_mod)
            buffer.asprintf_append ("))");
	}
	else if (ir->num_operands == 2 && ir->operation == ir_binop_div && op0matrix && !op1matrix)
	{
		// "matrix/scalar" - Metal does not have it, so print multiply by inverse instead
		buffer.asprintf_append ("(");
		ir->operands[0]->accept(this);
		const bool halfCast = (arg_prec == GLSL_PRECISION_MEDIUM);
		buffer.asprintf_append (halfCast ? " * (1.0h/half(" : " * (1.0/(");
		ir->operands[1]->accept(this);
		buffer.asprintf_append (")))");
	}
	else if (ir->num_operands == 2)
	{
		// regular binary operator
		buffer.asprintf_append ("(");
		if (ir->operands[0])
		{
			if (op0castTo1)
			{
				buffer.asprintf_append ("_xlinit_");
				print_type_precision(buffer, ir->operands[1]->type, arg_prec, false);
				buffer.asprintf_append ("(");
			}
			else if (op0cast)
			{
				print_cast (buffer, arg_prec, ir->operands[0]);
				buffer.asprintf_append ("(");
			}
			ir->operands[0]->accept(this);
			if (op0castTo1 || op0cast)
			{
				buffer.asprintf_append (")");
			}
		}

		buffer.asprintf_append (" %s ", ir->operation == ir_binop_mod ? "%" : metal_operator(ir->operation));

		if (ir->operands[1])
		{
			if (op1castTo0)
			{
				buffer.asprintf_append ("_xlinit_");
				print_type_precision(buffer, ir->operands[0]->type, arg_prec, false);
				buffer.asprintf_append ("(");
			}
			else if (op1cast)
			{
				print_cast (buffer, arg_prec, ir->operands[1]);
				buffer.asprintf_append ("(");
			}
			ir->operands[1]->accept(this);
			if (op1castTo0 || op1cast)
			{
				buffer.asprintf_append (")");
			}
		}
		buffer.asprintf_append (")");
	}
	else
	{
		// ternary op
		buffer.asprintf_append ("%s (", metal_operator(ir->operation));
		if (ir->operands[0])
		{
			if (op0cast)
				print_cast (buffer, arg_prec, ir->operands[0]);
			ir->operands[0]->accept(this);
		}
		buffer.asprintf_append (", ");
		if (ir->operands[1])
		{
			if (op1cast)
				print_cast (buffer, arg_prec, ir->operands[1]);
			ir->operands[1]->accept(this);
		}
		buffer.asprintf_append (", ");
		if (ir->operands[2])
		{
			if (op2cast)
				print_cast (buffer, arg_prec, ir->operands[2]);
			ir->operands[2]->accept(this);
		}
		buffer.asprintf_append (")");
	}
	
	if (rescast)
	{
		buffer.asprintf_append (")");
	}
	
	
	newline_deindent();
	--this->expression_depth;
}


static void print_texture_uv(ir_print_metal_visitor* vis, ir_texture* ir, bool is_shadow, bool is_proj, bool is_array)
{
	const unsigned dim = ir->sampler->type->sampler_dimensionality;
	const unsigned n = (dim == GLSL_SAMPLER_DIM_CUBE || dim == GLSL_SAMPLER_DIM_3D) ? 3 : 2;
	vis->buffer.asprintf_append("(float%u)((", n);
	ir->coordinate->accept(vis);
	vis->buffer.asprintf_append(")");
	if (is_array) vis->buffer.asprintf_append(n == 3 ? ".xyz" : ".xy");
	vis->buffer.asprintf_append(")");
	if (is_proj) { vis->buffer.asprintf_append(" / (float)("); ir->projector->accept(vis); vis->buffer.asprintf_append(")"); }
	if (is_array) {
		vis->buffer.asprintf_append(", uint(("); ir->coordinate->accept(vis);
		vis->buffer.asprintf_append(n == 3 ? ").w)" : ").z)");
	}
	if (is_shadow) {
		vis->buffer.asprintf_append(", saturate((float)("); ir->shadow_comparator->accept(vis);
		vis->buffer.asprintf_append(")");
		if (is_proj) { vis->buffer.asprintf_append(" / (float)("); ir->projector->accept(vis); vis->buffer.asprintf_append(")"); }
		vis->buffer.asprintf_append(")");
	}
}

void ir_print_metal_visitor::visit(ir_texture *ir)
{
	if (ir->op == ir_txf)
	{
		ir->sampler->accept(this);
		buffer.asprintf_append (ir->sampler->type->sampler_dimensionality == GLSL_SAMPLER_DIM_3D ? ".read(uint3(" : ".read(uint2(");
		ir->coordinate->accept(this);
		if (ir->offset) { buffer.asprintf_append(" + "); ir->offset->accept(this); }
		buffer.asprintf_append ("), uint(");
		ir->lod_info.lod->accept(this);
		buffer.asprintf_append ("))");
		return;
	}
	if (ir->op == ir_txs)
	{
		buffer.asprintf_append (ir->type->vector_elements == 3 ? "int3(" : "int2(");
		ir->sampler->accept(this);
		buffer.asprintf_append (".get_width(");
		ir->lod_info.lod->accept(this);
		buffer.asprintf_append ("), ");
		ir->sampler->accept(this);
		buffer.asprintf_append (".get_height(");
		ir->lod_info.lod->accept(this);
		buffer.asprintf_append (")");
		if (ir->type->vector_elements == 3) {
			buffer.asprintf_append(", "); ir->sampler->accept(this);
			buffer.asprintf_append(ir->sampler->type->sampler_array ? ".get_array_size()" : ".get_depth()");
		}
		buffer.asprintf_append (")");
		return;
	}
	glsl_sampler_dim sampler_dim = (glsl_sampler_dim)ir->sampler->type->sampler_dimensionality;
	const bool is_shadow = ir->sampler->type->sampler_shadow;
	const bool is_array = ir->sampler->type->sampler_array;
	const bool is_proj = ir->projector != nullptr;
	
	const bool halfShadow = is_shadow && ir->get_precision() == GLSL_PRECISION_MEDIUM;
	if (halfShadow) buffer.asprintf_append("half(");
	// texture name & call to sample
	ir->sampler->accept(this);
	if (is_shadow)
	{
		// For shadow sampling, Metal right now needs a hardcoded sampler state :|
		if (!ctx.shadowSamplerDone)
		{
			ctx.prefixStr.asprintf_append("constexpr sampler _mtl_xl_shadow_sampler(address::clamp_to_edge, filter::linear, compare_func::less_equal);\n");
			ctx.shadowSamplerDone = true;
		}
		buffer.asprintf_append (".sample_compare(_mtl_xl_shadow_sampler");
	}
	else
	{
		buffer.asprintf_append (".sample(_mtlsmp_");
		ir->sampler->accept(this);
	}
	buffer.asprintf_append (", ");

	// texture coordinate
	print_texture_uv (this, ir, is_shadow, is_proj, is_array);

	// lod bias
	if (ir->op == ir_txb)
	{
		buffer.asprintf_append (", bias(");
		ir->lod_info.bias->accept(this);
		buffer.asprintf_append (")");
	}
	
	// lod
	if (ir->op == ir_txl)
	{
		buffer.asprintf_append (", level(");
		ir->lod_info.lod->accept(this);
		buffer.asprintf_append (")");
	}
	
	// grad
	if (ir->op == ir_txd)
	{
		if (sampler_dim == GLSL_SAMPLER_DIM_CUBE)
			buffer.asprintf_append (", gradientcube((float3)(");
		else if (sampler_dim == GLSL_SAMPLER_DIM_3D)
			buffer.asprintf_append (", gradient3d((float3)(");
		else
			buffer.asprintf_append (", gradient2d((float2)(");

		ir->lod_info.grad.dPdx->accept(this);

		if (sampler_dim == GLSL_SAMPLER_DIM_CUBE || sampler_dim == GLSL_SAMPLER_DIM_3D)
			buffer.asprintf_append ("), (float3)(");
		else
			buffer.asprintf_append ("), (float2)(");

		ir->lod_info.grad.dPdy->accept(this);
		buffer.asprintf_append ("))");
	}
	
	if (ir->offset) { buffer.asprintf_append(", "); ir->offset->accept(this); }

	buffer.asprintf_append (")");
	if (halfShadow) buffer.asprintf_append(")");
}


void ir_print_metal_visitor::visit(ir_swizzle *ir)
{
   const unsigned swiz[4] = {
      ir->mask.x,
      ir->mask.y,
      ir->mask.z,
      ir->mask.w,
   };

	if (ir->val->type == glsl_type::float_type || ir->val->type == glsl_type::int_type)
	{
		if (ir->mask.num_components != 1)
		{
			print_type(buffer, ir, ir->type, true);
			buffer.asprintf_append ("(");
		}
	}

	ir->val->accept(this);
	
	if (ir->val->type == glsl_type::float_type || ir->val->type == glsl_type::int_type)
	{
		if (ir->mask.num_components != 1)
		{
			buffer.asprintf_append (")");
		}
		return;
	}

   buffer.asprintf_append (".");
   for (unsigned i = 0; i < ir->mask.num_components; i++) {
		buffer.asprintf_append ("%c", "xyzw"[swiz[i]]);
   }
}

static void print_var_inout (string_buffer& buf, ir_variable* var, bool insideLHS)
{
	if (var->data.mode == ir_var_shader_in)
		buf.asprintf_append ("_mtl_i.");
	if (var->data.mode == ir_var_shader_out && !var->data.fb_fetch_output)
		buf.asprintf_append ("_mtl_o.");
	if (var->data.mode == ir_var_uniform && !var->type->without_array()->is_sampler())
		buf.asprintf_append ("_mtl_u.");
	if (var->data.mode == ir_var_shader_out && var->data.fb_fetch_output)
		buf.asprintf_append (insideLHS ? "_mtl_o." : "_mtl_i.");
}

void ir_print_metal_visitor::visit(ir_dereference_variable *ir)
{
	ir_variable *var = ir->variable_referenced();
	const bool signedID = !strcmp(var->name, "gl_VertexID") || !strcmp(var->name, "gl_InstanceID");
	const bool halfCoord = !strcmp(var->name, "gl_PointCoord") && var->data.precision == GLSL_PRECISION_MEDIUM;
	if (signedID) buffer.asprintf_append("int(");
	if (halfCoord) buffer.asprintf_append("half2(");
	print_var_inout(buffer, var, this->inside_lhs);
	print_var_name (var);
	if (signedID || halfCoord) buffer.asprintf_append(")");
}


void ir_print_metal_visitor::visit(ir_dereference_array *ir)
{
   ir->array->accept(this);
   buffer.asprintf_append ("[");
   ir->array_index->accept(this);
   buffer.asprintf_append ("]");
}


void ir_print_metal_visitor::visit(ir_dereference_record *ir)
{
   ir->record->accept(this);
   buffer.asprintf_append (".%s", ir->record->type->fields.structure[ir->field_idx].name);
}




void ir_print_metal_visitor::emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex)
{
	// Metal arrays are not assignable; copy elements, retaining precision casts.
	if (lhs->type->is_array()) {
		void* mem_ctx = ralloc_parent(lhs);
		for (unsigned i = 0; i < lhs->type->length; ++i) {
			if (i) { buffer.asprintf_append(";\n"); indent(); }
			auto* dst = new(mem_ctx) ir_dereference_array(lhs, new(mem_ctx) ir_constant(i));
			auto* src = new(mem_ctx) ir_dereference_array(rhs, new(mem_ctx) ir_constant(i));
			emit_assignment_part(dst, src, (1u << dst->type->vector_elements) - 1, nullptr);
		}
		return;
	}
	const bool prev_lhs_flag = this->inside_lhs;
	this->inside_lhs = true;

	lhs->accept(this);

	this->inside_lhs = prev_lhs_flag;

	const glsl_type* lhsType = lhs->type;
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
		
		if (lhsType->matrix_columns <= 1 && lhsType->vector_elements > 1)
			lhsType = glsl_type::get_instance(lhsType->base_type, 1, 1);
	}
	
	char mask[5];
	unsigned j = 0;
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
	
	const bool typeMismatch = !dstIndex && (lhsType != rhsType);
	
	const bool precMismatch = is_different_precision (lhs->get_precision(), rhs->get_precision());
	const bool addSwizzle = hasWriteMask && typeMismatch;
	if (typeMismatch || precMismatch)
	{
		if (!addSwizzle)
		{
			if (lhsType->is_matrix())
			{
				// Metal does not have matrix precision casts right now, so emit workaround
				// functions that would do that.
				if (!ctx.matrixCastsDone)
				{
					ctx.prefixStr.asprintf_append(
												  "inline float4x4 _xlcast_float4x4(half4x4 v) { return float4x4(float4(v[0]), float4(v[1]), float4(v[2]), float4(v[3])); }\n"
												  "inline float3x3 _xlcast_float3x3(half3x3 v) { return float3x3(float3(v[0]), float3(v[1]), float3(v[2])); }\n"
												  "inline float2x2 _xlcast_float2x2(half2x2 v) { return float2x2(float2(v[0]), float2(v[1])); }\n"
												  "inline half4x4 _xlcast_half4x4(float4x4 v) { return half4x4(half4(v[0]), half4(v[1]), half4(v[2]), half4(v[3])); }\n"
												  "inline half3x3 _xlcast_half3x3(float3x3 v) { return half3x3(half3(v[0]), half3(v[1]), half3(v[2])); }\n"
												  "inline half2x2 _xlcast_half2x2(float2x2 v) { return half2x2(half2(v[0]), half2(v[1])); }\n"
												  );
					ctx.matrixCastsDone = true;
				}
				buffer.asprintf_append ("_xlcast_");
			}
			print_type(buffer, lhs, lhsType, true);
		}
		buffer.asprintf_append ("(");
	}
	
	rhs->accept(this);
	
	if (typeMismatch || precMismatch)
	{
		buffer.asprintf_append (")");
		if (addSwizzle)
			buffer.asprintf_append (".%s", mask);
	}
}


// Try to print (X = X + const) as (X += const), mostly to satisfy
// OpenGL ES 2.0 loop syntax restrictions.
static bool try_print_increment (ir_print_metal_visitor* vis, ir_assignment* ir)
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
	const bool prev_lhs_flag = vis->inside_lhs;
	vis->inside_lhs = true;
	
	ir->lhs->accept (vis);
	
	vis->inside_lhs = prev_lhs_flag;
	

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


void ir_print_metal_visitor::visit(ir_assignment *ir)
{
	if (loop_initializers && loop_initializers->omitted.count(ir)) {
		skipped_this_ir = true;
		return;
	}
	// assignments in global scope are postponed to main function
	if (this->mode != kPrintGlslNone)
	{
		assert (!this->globals->main_function_done);
		this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry_metal(ir));
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

	if (ir->condition)
	{
	  ir->condition->accept(this);
	  buffer.asprintf_append (" ");
	}

	emit_assignment_part (ir->lhs, ir->rhs, ir->write_mask, NULL);
}


void ir_print_metal_visitor::visit(ir_constant *ir)
{
	const glsl_type* type = ir->type;

	// hoist array & struct constants into global scope
	if (type->is_array() || type->is_struct())
	{
		size_t id = globals->const_hash[ir];
		if (id == 0)
		{
			id = ++globals->const_counter;
			globals->const_hash[ir] = id;
			globals->global_constants.push_tail(new(globals->mem_ctx) gconst_entry_metal(ir,id));
		}
		buffer.asprintf_append("_xlat_mtl_const%i", (int)id);
		return;
	}

	if (type == glsl_type::float_type)
	{
		print_float (buffer, ir->value.f[0]);
		return;
	}
	else if (type == glsl_type::int_type)
	{
		buffer.asprintf_append ("%d", ir->value.i[0]);
		return;
	}
	else if (type == glsl_type::uint_type)
	{
		buffer.asprintf_append ("%u", ir->value.u[0]);
		return;
	}

   const glsl_type *const base_type = ir->type->get_base_type();

   print_type(buffer, ir, type, true);
   buffer.asprintf_append ("(");

	// should be dealt with above
	assert(!ir->type->is_array());
	assert(!ir->type->is_struct());
	bool first = true;

	// Metal needs matrices to be constructed from vectors, not from a bunch of scalars.
	// So instead of printing mat2(1,2,3,4) like in glsl, we have to print float2x2(float2(1,2), float2(3,4))
	// here.
	const bool mtx = ir->type->is_matrix();
	const glsl_type* vec_type = NULL; // matrix column type
	if (mtx)
		vec_type = glsl_type::get_instance (ir->type->base_type, ir->type->vector_elements, 1);

	for (unsigned i = 0; i < ir->type->components(); i++)
	{
		if (!first)
		{
			if (mtx && (i % ir->type->vector_elements == 0))
				buffer.asprintf_append (")");
			buffer.asprintf_append (", ");
		}
		first = false;

		if (mtx && (i % ir->type->vector_elements == 0))
		{
			print_type(buffer, ir, vec_type, true);
			buffer.asprintf_append ("(");
		}

		switch (base_type->base_type) {
		case GLSL_TYPE_UINT:  buffer.asprintf_append ("%u", ir->value.u[i]); break;
		case GLSL_TYPE_INT:   buffer.asprintf_append ("%d", ir->value.i[i]); break;
		case GLSL_TYPE_FLOAT: print_float(buffer, ir->value.f[i]); break;
		case GLSL_TYPE_BOOL:  buffer.asprintf_append ("%d", ir->value.b[i]); break;
		default: assert(0);
		}
	}
	if (mtx)
		buffer.asprintf_append (")");
	buffer.asprintf_append (")");
}


void
ir_print_metal_visitor::visit(ir_call *ir)
{
	// calls in global scope are postponed to main function
	if (this->mode != kPrintGlslNone)
	{
		assert (!this->globals->main_function_done);
		this->globals->global_assignements.push_tail (new(this->globals->mem_ctx) ga_entry_metal(ir));
		buffer.asprintf_append ("//"); // for the ; that will follow (ugly, I know)
		return;
	}

	if (ir->return_deref)
	{
		visit(ir->return_deref);
		buffer.asprintf_append (" = ");
	}

   buffer.asprintf_append ("%s (", ir->callee_name());
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
ir_print_metal_visitor::visit(ir_return *ir)
{
   buffer.asprintf_append ("return");

   ir_rvalue *const value = ir->get_value();
   if (value) {
      buffer.asprintf_append (" ");
      value->accept(this);
   }
}


void
ir_print_metal_visitor::visit(ir_discard *ir)
{
   if (ir->condition) {
      buffer.asprintf_append("if (");
      ir->condition->accept(this);
      buffer.asprintf_append(") ");
   }
   buffer.asprintf_append("discard_fragment()");
}


void
ir_print_metal_visitor::visit(ir_if *ir)
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


bool ir_print_metal_visitor::emit_canonical_for (ir_loop* ir)
{
	loop_variable_state* const ls = this->loopstate->get(ir);

	if (!printable_for_loop(ir, ls)) return false;

	std::unordered_set<ir_instruction*> terminator_hash;
	std::unordered_set<ir_instruction*> induction_hash;
	
	buffer.asprintf_append("for (");
	
	const auto init = loop_initializers->initializers.find(ir);
	if (init != loop_initializers->initializers.end()) {
		ir_variable* var = init->second->whole_variable_written();
		print_type(buffer, var, var->type, false);
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
ir_print_metal_visitor::visit(ir_loop *ir)
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
ir_print_metal_visitor::visit(ir_loop_jump *ir)
{
   buffer.asprintf_append ("%s", ir->is_break() ? "break" : "continue");
}

void
ir_print_metal_visitor::visit(ir_emit_vertex *ir)
{
	buffer.asprintf_append ("emit-vertex-TODO");
}

void
ir_print_metal_visitor::visit(ir_end_primitive *ir)
{
	buffer.asprintf_append ("end-primitive-TODO");
}
