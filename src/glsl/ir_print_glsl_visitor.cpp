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

#include "ir_print_glsl_visitor.h"
#include "ir_visitor.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "ir_unused_structs.h"
#include "program/hash_table.h"
#include <math.h>


class string_buffer
{
public:
	string_buffer(void* mem_ctx)
	{
		m_Capacity = 512;
		m_Ptr = (char*)ralloc_size(mem_ctx, m_Capacity);
		m_Size = 0;
		m_Ptr[0] = 0;
	}

	~string_buffer()
	{
		ralloc_free(m_Ptr);
	}

	const char* c_str() const { return m_Ptr; }

	void asprintf_append(const char *fmt, ...) PRINTFLIKE(2, 3)
	{
		va_list args;
		va_start(args, fmt);
		vasprintf_append(fmt, args);
		va_end(args);
	}

	void vasprintf_append(const char *fmt, va_list args)
	{
		assert (m_Ptr != NULL);
		vasprintf_rewrite_tail (&m_Size, fmt, args);
	}

	void vasprintf_rewrite_tail (size_t *start, const char *fmt, va_list args)
	{
		assert (m_Ptr != NULL);

		size_t new_length = printf_length(fmt, args);
		size_t needed_length = m_Size + new_length + 1;

		if (m_Capacity < needed_length)
		{
			m_Capacity = MAX2 (m_Capacity + m_Capacity/2, needed_length);
			m_Ptr = (char*)reralloc_size(ralloc_parent(m_Ptr), m_Ptr, m_Capacity);
		}

		vsnprintf(m_Ptr + m_Size, new_length+1, fmt, args);
		m_Size += new_length;
		assert (m_Capacity >= m_Size);
	}

private:
	char* m_Ptr;
	size_t m_Size;
	size_t m_Capacity;
};


static void print_type(string_buffer& buffer, const glsl_type *t, bool arraySize);
static void print_type_post(string_buffer& buffer, const glsl_type *t, bool arraySize);

static inline const char* get_precision_string (glsl_precision p)
{
	switch (p) {
	case glsl_precision_high:		return "highp ";
	case glsl_precision_medium:		return "mediump ";
	case glsl_precision_low:		return "lowp ";
	case glsl_precision_undefined:	return "";
	}
	assert(!"Should not get here.");
	return "";
}

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
		var_hash = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
		main_function_done = false;
	}
	
	~global_print_tracker() {
		hash_table_dtor (var_hash);
		ralloc_free(mem_ctx);
	}
	
	unsigned	var_counter;
	hash_table*	var_hash;
	exec_list	global_assignements;
	void* mem_ctx;
	bool	main_function_done;
};

class ir_print_glsl_visitor : public ir_visitor {
public:
	ir_print_glsl_visitor(string_buffer& buf, global_print_tracker* globals_, PrintGlslMode mode_, bool use_precision_, const _mesa_glsl_parse_state* state_)
		: buffer(buf)
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
	virtual void visit(ir_precision_statement *);
	virtual void visit(ir_typedecl_statement *);
	virtual void visit(ir_emit_vertex *);
	virtual void visit(ir_end_primitive *);
	
	void emit_assignment_part (ir_dereference* lhs, ir_rvalue* rhs, unsigned write_mask, ir_rvalue* dstIndex);
	
	int indentation;
	int expression_depth;
	string_buffer& buffer;
	global_print_tracker* globals;
	const _mesa_glsl_parse_state* state;
	PrintGlslMode mode;
	bool	use_precision;
};


char*
_mesa_print_ir_glsl(exec_list *instructions,
	    struct _mesa_glsl_parse_state *state,
		char* buffer, PrintGlslMode mode)
{
	string_buffer str(buffer);

	// print version & extensions
	if (state) {
		if (state->had_version_string)
		{
			str.asprintf_append ("#version %i", state->language_version);
			if (state->es_shader && state->language_version >= 300)
				str.asprintf_append (" es");
			str.asprintf_append ("\n");
		}
		if (state->ARB_shader_texture_lod_enable)
			str.asprintf_append ("#extension GL_ARB_shader_texture_lod : enable\n");
		if (state->EXT_shader_texture_lod_enable)
			str.asprintf_append ("#extension GL_EXT_shader_texture_lod : enable\n");
		if (state->OES_standard_derivatives_enable)
			str.asprintf_append ("#extension GL_OES_standard_derivatives : enable\n");
		if (state->EXT_shadow_samplers_enable)
			str.asprintf_append ("#extension GL_EXT_shadow_samplers : enable\n");
		if (state->EXT_frag_depth_enable)
			str.asprintf_append ("#extension GL_EXT_frag_depth : enable\n");
	}
	
	// remove unused struct declarations
	do_remove_unused_typedecls(instructions);
	
	global_print_tracker gtracker;

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
	  if (ir->ir_type == ir_type_variable) {
		ir_variable *var = static_cast<ir_variable*>(ir);
		if ((strstr(var->name, "gl_") == var->name)
			  && !var->invariant)
			continue;
	  }

	  ir_print_glsl_visitor v (str, &gtracker, mode, state->es_shader, state);
	  ir->accept(&v);
      if (ir->ir_type != ir_type_function)
		str.asprintf_append (";\n");
   }

   return ralloc_strdup(buffer, str.c_str());
}


void ir_print_glsl_visitor::indent(void)
{
   for (int i = 0; i < indentation; i++)
      buffer.asprintf_append ("  ");
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
    long id = (long)hash_table_find (globals->var_hash, v);
	if (!id && v->mode == ir_var_temporary)
	{
        id = ++globals->var_counter;
        hash_table_insert (globals->var_hash, (void*)id, v);
	}
    if (id)
    {
        if (v->mode == ir_var_temporary)
            buffer.asprintf_append ("tmpvar_%d", (int)id);
        else
            buffer.asprintf_append ("%s_%d", v->name, (int)id);
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
	if (type &&
		!type->is_float() &&
		!type->is_sampler() &&
		(!type->is_array() || !type->element_type()->is_float())
	)
	{
		return;
	}
	glsl_precision prec = precision_from_ir(ir);
	
	// skip precision for samplers that end up being lowp (default anyway) or undefined;
	// except always emit it for shadowmap samplers (some drivers don't implement
	// default EXT_shadow_samplers precision)
	if (type && type->is_sampler() && !type->sampler_shadow)
	{
		if (prec == glsl_precision_low || prec == glsl_precision_undefined)
			return;
	}
	
	if (prec == glsl_precision_high || prec == glsl_precision_undefined)
	{
		if (ir->ir_type == ir_type_function_signature)
			return;
	}
	buffer.asprintf_append ("%s", get_precision_string(prec));
}


static void print_type(string_buffer& buffer, const glsl_type *t, bool arraySize)
{
	if (t->base_type == GLSL_TYPE_ARRAY) {
		print_type(buffer, t->fields.array, true);
		if (arraySize)
			buffer.asprintf_append ("[%u]", t->length);
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
			buffer.asprintf_append ("[%u]", t->length);
	}
}


void ir_print_glsl_visitor::visit(ir_variable *ir)
{
	const char *const cent = (ir->centroid) ? "centroid " : "";
	const char *const inv = (ir->invariant) ? "invariant " : "";
	const char *const mode[3][ir_var_mode_count] =
	{
		{ "", "uniform ", "in ",        "out ",     "in ", "out ", "inout ", "", "", "" },
		{ "", "uniform ", "attribute ", "varying ", "in ", "out ", "inout ", "", "", "" },
		{ "", "uniform ", "varying ",   "out ",     "in ", "out ", "inout ", "", "", "" },
	};
	
	const char *const interp[] = { "", "smooth ", "flat ", "noperspective " };
	
	if (this->state->language_version >= 300 && ir->explicit_location)
	{
		const int binding_base = (this->state->target == vertex_shader ? (int)VERT_ATTRIB_GENERIC0 : (int)FRAG_RESULT_DATA0);
		const int location = ir->location - binding_base;
		buffer.asprintf_append ("layout(location=%d) ", location);
	}
	
	int decormode = this->mode;
	// GLSL 1.30 and up use "in" and "out" for everything
	if (this->state->language_version >= 130)
	{
		decormode = 0;
	}
	
	// give an id to any variable defined in a function that is not an uniform
	if ((this->mode == kPrintGlslNone && ir->mode != ir_var_uniform))
	{
		long id = (long)hash_table_find (globals->var_hash, ir);
		if (id == 0)
		{
			id = ++globals->var_counter;
			hash_table_insert (globals->var_hash, (void*)id, ir);
		}
	}
	
	// keep invariant declaration for builtin variables
	if (strstr(ir->name, "gl_") == ir->name) {
		buffer.asprintf_append ("%s", inv);
		print_var_name (ir);
		return;
	}
	
	buffer.asprintf_append ("%s%s%s%s",
							cent, inv, interp[ir->interpolation], mode[decormode][ir->mode]);
	print_precision (ir, ir->type);
	print_type(buffer, ir->type, false);
	buffer.asprintf_append (" ");
	print_var_name (ir);
	print_type_post(buffer, ir->type, false);
	
	if (ir->constant_value &&
		ir->mode != ir_var_shader_in &&
		ir->mode != ir_var_shader_out &&
		ir->mode != ir_var_function_in &&
		ir->mode != ir_var_function_out &&
		ir->mode != ir_var_function_inout)
	{
		buffer.asprintf_append (" = ");
		visit (ir->constant_value);
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

	   indentation++;
	   bool first = true;
	   foreach_iter(exec_list_iterator, iter, ir->parameters) {
		  ir_variable *const inst = (ir_variable *) iter.get();

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

   if (ir->body.is_empty())
   {
	   buffer.asprintf_append (");\n");
	   return;
   }

   buffer.asprintf_append (")\n");

   indent();
   buffer.asprintf_append ("{\n");
   indentation++;
	
	// insert postponed global assigments
	if (strcmp(ir->function()->name, "main") == 0)
	{
		assert (!globals->main_function_done);
		globals->main_function_done = true;
		foreach_iter(exec_list_iterator, it, globals->global_assignements)
		{
			ir_instruction* as = ((ga_entry *)it.get())->ir;
			as->accept(this);
			buffer.asprintf_append(";\n");
		}
	}

   foreach_iter(exec_list_iterator, iter, ir->body) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
	  buffer.asprintf_append (";\n");
   }
   indentation--;
   indent();
   buffer.asprintf_append ("}\n");
}


void ir_print_glsl_visitor::visit(ir_function *ir)
{
   bool found_non_builtin_proto = false;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();
      if (!sig->is_builtin())
	 found_non_builtin_proto = true;
   }
   if (!found_non_builtin_proto)
      return;

   PrintGlslMode oldMode = this->mode;
   this->mode = kPrintGlslNone;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();

      indent();
      sig->accept(this);
      buffer.asprintf_append ("\n");
   }

   this->mode = oldMode;

   indent();
}


static const char *const operator_glsl_strs[] = {
	"~",
	"!",
	"-",
	"abs",
	"sign",
	"1.0/",
	"inversesqrt",
	"sqrt",
	"normalize",
	"exp",
	"log",
	"exp2",
	"log2",
	"int",		// f2i
	"int",		// f2u
	"float",	// i2f
	"bool",		// f2b
	"float",	// b2f
	"bool",		// i2b
	"int",		// b2i
	"float",	// u2f
	"int",		// i2u
	"int",		// u2i
	"float",	// bit i2f
	"int",		// bit f2i
	"float",	// bit u2f
	"int",		// bit f2u
	"any",
	"trunc",
	"ceil",
	"floor",
	"fract",
	"roundEven",
	"sin",
	"cos",
	"sin", // reduced
	"cos", // reduced
	"dFdx",
	"dFdy",
	"packSnorm2x16",
	"packSnorm4x8",
	"packUnorm2x16",
	"packUnorm4x8",
	"packHalf2x16",
	"unpackSnorm2x16",
	"unpackSnorm4x8",
	"unpackUnorm2x16",
	"unpackUnorm4x8",
	"unpackHalf2x16",
	"unpackHalf2x16_splitX_TODO",
	"unpackHalf2x16_splitY_TODO",
	"bitfieldReverse",
	"bitCount",
	"findMSB",
	"findLSB",
	"noise",
	"+",
	"-",
	"*",
	"*_imul_high_TODO",
	"/",
	"carry_TODO",
	"borrow_TODO",
	"mod",
	"<",
	">",
	"<=",
	">=",
	"equal",
	"notEqual",
	"==",
	"!=",
	"<<",
	">>",
	"&",
	"^",
	"|",
	"&&",
	"^^",
	"||",
	"dot",
	"min",
	"max",
	"pow",
	"packHalf2x16_split_TODO",
	"bfm_TODO",
	"uboloadTODO",
	"ldexp_TODO",
	"vectorExtract_TODO",
	"fma",
	"clamp",
	"mix",
	"csel_TODO",
	"bfi_TODO",
	"bitfield_extract_TODO",
	"vector_insert_TODO",
	"bitfield_insert_TODO",
	"vectorTODO",
};

static const char *const operator_vec_glsl_strs[] = {
	"lessThan",
	"greaterThan",
	"lessThanEqual",
	"greaterThanEqual",
	"equal",
	"notEqual",
};


static bool is_binop_func_like(ir_expression_operation op, const glsl_type* type)
{
	if (op == ir_binop_equal || 
		op == ir_binop_nequal ||
		op == ir_binop_mod ||
		(op >= ir_binop_dot && op <= ir_binop_pow))
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
	
	if (ir->get_num_operands() == 1) {
		if (ir->operation >= ir_unop_f2i && ir->operation < ir_unop_any) {
			print_type(buffer, ir->type, true);
			buffer.asprintf_append ("(");
		} else if (ir->operation == ir_unop_rcp) {
			buffer.asprintf_append ("(1.0/(");
		} else {
			buffer.asprintf_append ("%s(", operator_glsl_strs[ir->operation]);
		}
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append (")");
		if (ir->operation == ir_unop_rcp) {
			buffer.asprintf_append (")");
		}
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
			buffer.asprintf_append ("%s (", operator_glsl_strs[ir->operation]);
		
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append (", ");
		if (ir->operands[1])
			ir->operands[1]->accept(this);
		buffer.asprintf_append (")");
		if (ir->operation == ir_binop_mod)
            buffer.asprintf_append ("))");
	}
	else if (ir->get_num_operands() == 2)
	{
		buffer.asprintf_append ("(");
		if (ir->operands[0])
			ir->operands[0]->accept(this);

		buffer.asprintf_append (" %s ", operator_glsl_strs[ir->operation]);

		if (ir->operands[1])
			ir->operands[1]->accept(this);
		buffer.asprintf_append (")");
	}
	else
	{
		// ternary op
		buffer.asprintf_append ("%s (", operator_glsl_strs[ir->operation]);
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		buffer.asprintf_append (", ");
		if (ir->operands[1])
			ir->operands[1]->accept(this);
		buffer.asprintf_append (", ");
		if (ir->operands[2])
			ir->operands[2]->accept(this);
		buffer.asprintf_append (")");
	}
	
	newline_deindent();
	--this->expression_depth;
}

// [glsl_sampler_dim]
static const char* tex_sampler_dim_name[] = {
	"1D", "2D", "3D", "Cube", "Rect", "Buf", "External",
};
static int tex_sampler_dim_size[] = {
	1, 2, 3, 3, 2, 2, 2,
};

void ir_print_glsl_visitor::visit(ir_texture *ir)
{
	glsl_sampler_dim sampler_dim = (glsl_sampler_dim)ir->sampler->type->sampler_dimensionality;
	const bool is_shadow = ir->sampler->type->sampler_shadow;
	const glsl_type* uv_type = ir->coordinate->type;
	const int uv_dim = uv_type->vector_elements;
	int sampler_uv_dim = tex_sampler_dim_size[sampler_dim];
	if (is_shadow)
		sampler_uv_dim += 1;
	const bool is_proj = (uv_dim > sampler_uv_dim);
	
    // texture function name
    //ACS: shadow lookups and lookups with dimensionality included in the name were deprecated in 130
    if(state->language_version<130) 
    {
        buffer.asprintf_append ("%s", is_shadow ? "shadow" : "texture");
        buffer.asprintf_append ("%s", tex_sampler_dim_name[sampler_dim]);
    }
    else 
    {
        buffer.asprintf_append ("texture");
    }
	
	if (is_proj)
		buffer.asprintf_append ("Proj");
	if (ir->op == ir_txl)
		buffer.asprintf_append ("Lod");
	if (ir->op == ir_txd)
		buffer.asprintf_append ("Grad");
	
	if (state->es_shader)
	{
		if ( (is_shadow && state->EXT_shadow_samplers_enable) ||
			(ir->op == ir_txl && state->EXT_shader_texture_lod_enable) )
		{
			buffer.asprintf_append ("EXT");
		}
	}
	
	if(ir->op == ir_txd)
	{
		if(state->es_shader && state->EXT_shader_texture_lod_enable)
			buffer.asprintf_append ("EXT");
		else if(!state->es_shader && state->ARB_shader_texture_lod_enable)
			buffer.asprintf_append ("ARB");
	}
	
	buffer.asprintf_append (" (");
	
	// sampler
	ir->sampler->accept(this);
	buffer.asprintf_append (", ");
	
	// texture coordinate
	ir->coordinate->accept(this);
	
	// lod bias
	if (ir->op == ir_txb)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.bias->accept(this);
	}
	
	// lod
	if (ir->op == ir_txl)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.lod->accept(this);
	}
	
	// grad
	if (ir->op == ir_txd)
	{
		buffer.asprintf_append (", ");
		ir->lod_info.grad.dPdx->accept(this);
		buffer.asprintf_append (", ");
		ir->lod_info.grad.dPdy->accept(this);
	}
	
	/*
	
   if (ir->offset != NULL) {
      ir->offset->accept(this);
   }
	
   if (ir->op != ir_txf) {
      if (ir->projector)
	 ir->projector->accept(this);
      else
	 buffer.asprintf_append ("1");

      if (ir->shadow_comparitor) {
	 buffer.asprintf_append (" ");
	 ir->shadow_comparitor->accept(this);
      } else {
	 buffer.asprintf_append (" ()");
      }
   }

   buffer.asprintf_append (" ");
   switch (ir->op)
   {
   case ir_tex:
      break;
   case ir_txb:
      ir->lod_info.bias->accept(this);
      break;
   case ir_txl:
   case ir_txf:
      ir->lod_info.lod->accept(this);
      break;
   case ir_txd:
      buffer.asprintf_append ("(");
      ir->lod_info.grad.dPdx->accept(this);
      buffer.asprintf_append (" ");
      ir->lod_info.grad.dPdy->accept(this);
      buffer.asprintf_append (")");
      break;
   };
	 */
   buffer.asprintf_append (")");
}


void ir_print_glsl_visitor::visit(ir_swizzle *ir)
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
			print_type(buffer, ir->type, true);
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
   buffer.asprintf_append (".%s", ir->field);
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

void ir_print_glsl_visitor::visit(ir_assignment *ir)
{
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
	
   if (ir->condition)
   {
      ir->condition->accept(this);
	  buffer.asprintf_append (" ");
   }
	
	emit_assignment_part (ir->lhs, ir->rhs, ir->write_mask, NULL);
}

static void print_float (string_buffer& buffer, float f)
{
	// Kind of roundabout way, but this is to satisfy two things:
	// * MSVC and gcc-based compilers differ a bit in how they treat float
	//   widht/precision specifiers. Want to match for tests.
	// * GLSL (early version at least) require floats to have ".0" or
	//   exponential notation.
	char tmp[64];
	snprintf(tmp, 64, "%.6g", f);

	char* posE = NULL;
	posE = strchr(tmp, 'e');
	if (!posE)
		posE = strchr(tmp, 'E');

	#if _MSC_VER
	// While gcc would print something like 1.0e+07, MSVC will print 1.0e+007 -
	// only for exponential notation, it seems, will add one extra useless zero. Let's try to remove
	// that so compiler output matches.
	if (posE != NULL)
	{
		if((posE[1] == '+' || posE[1] == '-') && posE[2] == '0')
		{
			char* p = posE+2;
			while (p[0])
			{
				p[0] = p[1];
				++p;
			}
		}
	}
	#endif

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

   print_type(buffer, type, true);
   buffer.asprintf_append ("(");

   if (ir->type->is_array()) {
      for (unsigned i = 0; i < ir->type->length; i++)
      {
	 if (i != 0)
	    buffer.asprintf_append (", ");
	 ir->get_array_element(i)->accept(this);
      }
   } else if (ir->type->is_record()) {
      bool first = true;
      foreach_iter(exec_list_iterator, iter, ir->components) {
	 if (!first)
	    ralloc_asprintf_append (&buffer, ", ");
	 first = false;
	 ir_constant* inst = (ir_constant*)iter.get();
	 inst->accept(this);
     } 
   }else {
      bool first = true;
      for (unsigned i = 0; i < ir->type->components(); i++) {
	 if (!first)
	    buffer.asprintf_append (", ");
	 first = false;
	 switch (base_type->base_type) {
	 case GLSL_TYPE_UINT:  buffer.asprintf_append ("%u", ir->value.u[i]); break;
	 case GLSL_TYPE_INT:   buffer.asprintf_append ("%d", ir->value.i[i]); break;
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
	
   buffer.asprintf_append ("%s (", ir->callee_name());
   bool first = true;
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_instruction *const inst = (ir_instruction *) iter.get();
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
   buffer.asprintf_append ("discard");

   if (ir->condition != NULL) {
      buffer.asprintf_append (" TODO ");
      ir->condition->accept(this);
   }
}


void
ir_print_glsl_visitor::visit(ir_if *ir)
{
   buffer.asprintf_append ("if (");
   ir->condition->accept(this);

   buffer.asprintf_append (") {\n");
   indentation++;

   foreach_iter(exec_list_iterator, iter, ir->then_instructions) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
      buffer.asprintf_append (";\n");
   }

   indentation--;
   indent();
   buffer.asprintf_append ("}");

   if (!ir->else_instructions.is_empty())
   {
	   buffer.asprintf_append (" else {\n");
	   indentation++;

	   foreach_iter(exec_list_iterator, iter, ir->else_instructions) {
		  ir_instruction *const inst = (ir_instruction *) iter.get();

		  indent();
		  inst->accept(this);
		  buffer.asprintf_append (";\n");
	   }
	   indentation--;
	   indent();
	   buffer.asprintf_append ("}");
   }
}


void
ir_print_glsl_visitor::visit(ir_loop *ir)
{
	bool noData = (ir->counter == NULL && ir->from == NULL && ir->to == NULL && ir->increment == NULL);
	if (noData) {
		buffer.asprintf_append ("while (true) {\n");
		indentation++;
		foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
			ir_instruction *const inst = (ir_instruction *) iter.get();
			indent();
			inst->accept(this);
			buffer.asprintf_append (";\n");
		}
		indentation--;
		indent();
		buffer.asprintf_append ("}");
		return;
	}

	bool canonicalFor = (ir->counter && ir->from && ir->to && ir->increment);
	if (canonicalFor)
	{
		buffer.asprintf_append ("for (");
		ir->counter->accept (this);
		buffer.asprintf_append (" = ");
		ir->from->accept (this);
		buffer.asprintf_append ("; ");
		print_var_name (ir->counter);

		// IR cmp operator is when to terminate loop; whereas GLSL for loop syntax
		// is while to continue the loop. Invert the meaning of operator when outputting.
		const char* termOp = NULL;
		switch (ir->cmp) {
		case ir_binop_less: termOp = ">="; break;
		case ir_binop_greater: termOp = "<="; break;
		case ir_binop_lequal: termOp = ">"; break;
		case ir_binop_gequal: termOp = "<"; break;
		case ir_binop_equal: termOp = "!="; break;
		case ir_binop_nequal: termOp = "=="; break;
		default: assert(false);
		}
		buffer.asprintf_append (" %s ", termOp);
		ir->to->accept (this);
		buffer.asprintf_append ("; ");
		// IR already has instructions that modify the loop counter in the body
		//print_var_name (ir->counter);
		//buffer.asprintf_append (" = ");
		//print_var_name (ir->counter);
		//buffer.asprintf_append ("+(");
		//ir->increment->accept (this);
		//buffer.asprintf_append (")");
		buffer.asprintf_append (") {\n");
		indentation++;
		foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
			ir_instruction *const inst = (ir_instruction *) iter.get();
			indent();
			inst->accept(this);
			buffer.asprintf_append (";\n");
		}
		indentation--;
		indent();
		buffer.asprintf_append ("}");
		return;
	}


   buffer.asprintf_append ("( TODO loop (");
   if (ir->counter != NULL)
      ir->counter->accept(this);
   buffer.asprintf_append (") (");
   if (ir->from != NULL)
      ir->from->accept(this);
   buffer.asprintf_append (") (");
   if (ir->to != NULL)
      ir->to->accept(this);
   buffer.asprintf_append (") (");
   if (ir->increment != NULL)
      ir->increment->accept(this);
   buffer.asprintf_append (") (\n");
   indentation++;

   foreach_iter(exec_list_iterator, iter, ir->body_instructions) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
      buffer.asprintf_append (";\n");
   }
   indentation--;
   indent();
   buffer.asprintf_append ("))\n");
}


void
ir_print_glsl_visitor::visit(ir_loop_jump *ir)
{
   buffer.asprintf_append ("%s", ir->is_break() ? "break" : "continue");
}

void
ir_print_glsl_visitor::visit(ir_precision_statement *ir)
{
	buffer.asprintf_append ("%s", ir->precision_statement);
}

void
ir_print_glsl_visitor::visit(ir_typedecl_statement *ir)
{
	const glsl_type *const s = ir->type_decl;
	buffer.asprintf_append ("struct %s {\n", s->name);

	for (unsigned j = 0; j < s->length; j++) {
		buffer.asprintf_append ("  ");
		if (state->es_shader)
			buffer.asprintf_append ("%s", get_precision_string(s->fields.structure[j].precision));
		print_type(buffer, s->fields.structure[j].type, false);
		buffer.asprintf_append (" %s", s->fields.structure[j].name);
		print_type_post(buffer, s->fields.structure[j].type, false);
		buffer.asprintf_append (";\n");
	}
	buffer.asprintf_append ("}");
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
