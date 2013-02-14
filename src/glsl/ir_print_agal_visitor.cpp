/*
Copyright (c) 2012 Adobe Systems Incorporated

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "ir_print_agal_visitor.h"
#include "ir_visitor.h"
#include "glsl_types.h"
#include "glsl_parser_extras.h"
#include "ir_unused_structs.h"
#include "program/hash_table.h"
#include <math.h>

static char* print_type(char* buffer, const glsl_type *t, bool arraySize);
static char* print_type_post(char* buffer, const glsl_type *t, bool arraySize);

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
	ga_entry(ir_assignment *ass)
	{
		assert(ass);
		this->ass = ass;
	}	
	ir_assignment* ass;
};


struct global_agal_tracker {
	global_agal_tracker () {
		mem_ctx = ralloc_context(0);
		main_function_done = false;
		fullywritten = hash_table_ctor(0, hash_table_pointer_hash, hash_table_pointer_compare);
	}
	
	~global_agal_tracker() {
		ralloc_free(mem_ctx);
	}
	exec_list	global_assignements;
	void* mem_ctx;
	bool	main_function_done;
	hash_table *fullywritten;
};

class ir_print_agal_visitor : public ir_visitor {
public:
	ir_print_agal_visitor(char* buf, global_agal_tracker* globals_, PrintGlslMode mode_, _mesa_glsl_parse_state *_state)
	{
		indentation = 0;
		buffer = buf;
		globals = globals_;
		mode = mode_;
		state = _state;
		use_precision = false;		
	}

	virtual ~ir_print_agal_visitor()
	{
	}

	void indent(void);
	void print_var_name (ir_variable* v);
	void print_precision (ir_instruction* ir);

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

	int indentation;
	char* buffer;
	global_agal_tracker* globals;
	PrintGlslMode mode;
	bool	use_precision;
	_mesa_glsl_parse_state *state;
	
};

static int writeMask = 0, readComponents= 0, writeComponents = 0;


char*
_mesa_print_ir_agal(exec_list *instructions,
	    struct _mesa_glsl_parse_state *state,
		char* buffer, PrintGlslMode mode)
{
	if (state) {
		if (state->ARB_shader_texture_lod_enable)
			ralloc_strcat (&buffer, "#extension GL_ARB_shader_texture_lod : enable\n");
		if (state->EXT_shader_texture_lod_enable)
			ralloc_strcat (&buffer, "#extension GL_EXT_shader_texture_lod : enable\n");
		if (state->OES_standard_derivatives_enable)
			ralloc_strcat (&buffer, "#extension GL_OES_standard_derivatives : enable\n");
	}
   if (state) {
	   ir_struct_usage_visitor v;
	   v.run (instructions);

      for (unsigned i = 0; i < state->num_user_structures; i++) {
	 const glsl_type *const s = state->user_structures[i];

	 if (!v.has_struct_entry(s))
		 continue;

	 ralloc_asprintf_append (&buffer, "struct %s {\n",
		s->name);

	 for (unsigned j = 0; j < s->length; j++) {
	    ralloc_asprintf_append (&buffer, "  ");
		if (state->es_shader)
			ralloc_asprintf_append (&buffer, "%s", get_precision_string(s->fields.structure[j].precision));
	    buffer = print_type(buffer, s->fields.structure[j].type, false);
	    ralloc_asprintf_append (&buffer, " %s", s->fields.structure[j].name);
        buffer = print_type_post(buffer, s->fields.structure[j].type, false);
        ralloc_asprintf_append (&buffer, "\n");
	 }

	 ralloc_asprintf_append (&buffer, "};\n");
      }
   }
	
	global_agal_tracker gtracker;

	int len=0;

   foreach_iter(exec_list_iterator, iter, *instructions) {
      ir_instruction *ir = (ir_instruction *)iter.get();
	  if (ir->ir_type == ir_type_variable) {
		ir_variable *var = static_cast<ir_variable*>(ir);
		if (strstr(var->name, "gl_") == var->name)
			continue;
	  }

	  ir_print_agal_visitor v (buffer, &gtracker, mode, state);
	  len = strlen(buffer);
	  ir->accept(&v);
		if(state->error)
	  	return NULL;

	  buffer = v.buffer;
      if (ir->ir_type != ir_type_function && len != strlen(buffer))
		ralloc_asprintf_append (&buffer, "\n");
   }

   return buffer;
}


void ir_print_agal_visitor::indent(void)
{
   for (int i = 0; i < indentation; i++)
      ralloc_asprintf_append (&buffer, "  ");
}

void ir_print_agal_visitor::print_var_name (ir_variable* v)
{
	ralloc_asprintf_append (&buffer, "%s", v->name);
}

void ir_print_agal_visitor::print_precision (ir_instruction* ir)
{
	if (!this->use_precision)
		return;
	if (ir->type && !ir->type->is_float() && (!ir->type->is_array() || !ir->type->element_type()->is_float()))
		return;
	glsl_precision prec = precision_from_ir(ir);
	if (prec == glsl_precision_high || prec == glsl_precision_undefined)
	{
		if (ir->ir_type == ir_type_function_signature)
			return;
	}
	ralloc_asprintf_append (&buffer, "%s", get_precision_string(prec));
}


static char*
print_type(char* buffer, const glsl_type *t, bool arraySize)
{
   if (t->base_type == GLSL_TYPE_ARRAY) {
      buffer = print_type(buffer, t->fields.array, true);
      if (arraySize)
         ralloc_asprintf_append (&buffer, "[%u]", t->length);
   } else if ((t->base_type == GLSL_TYPE_STRUCT)
	      && (strncmp("gl_", t->name, 3) != 0)) {
      ralloc_asprintf_append (&buffer, "%s", t->name);
   } else {
      ralloc_asprintf_append (&buffer, "%s", t->name);
   }
   return buffer;
}

static char*
print_type_post(char* buffer, const glsl_type *t, bool arraySize)
{
	if (t->base_type == GLSL_TYPE_ARRAY) {
		if (!arraySize)
			ralloc_asprintf_append (&buffer, "[%u]", t->length);
	}
	return buffer;
}

static int failcount = 0;

void ir_print_agal_visitor::visit(ir_variable *ir)
{

}


void ir_print_agal_visitor::visit(ir_function_signature *ir)
{
	if(strcmp(ir->function_name(), "main") != 0)
		return;

   foreach_iter(exec_list_iterator, iter, ir->body) {
      ir_instruction *const inst = (ir_instruction *) iter.get();

      indent();
      inst->accept(this);
	  ralloc_asprintf_append (&buffer, "\n");
   }
}


void ir_print_agal_visitor::visit(ir_function *ir)
{
	if(strcmp(ir->name, "main") != 0)
		return;

   bool found_non_builtin_proto = false;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();
      if (sig->is_defined || !sig->is_builtin)
	 found_non_builtin_proto = true;
   }
   if (!found_non_builtin_proto)
      return;

   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_function_signature *const sig = (ir_function_signature *) iter.get();

      indent();
      sig->accept(this);
      ralloc_asprintf_append (&buffer, "\n");
   }

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
	"exp",
	"log",
	"exp2",
	"log2",
	"int",
	"float",
	"bool",
	"float",
	"bool",
	"int",
	"float",
	"int",
	"int",
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
	"noise",
	"+",
	"-",
	"*",
	"/",
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
	"vectorTODO",
};

void ir_print_agal_visitor::visit(ir_expression *ir)
{
	if (ir->get_num_operands() == 1) {
		if (ir->operation >= ir_unop_f2i && ir->operation <= ir_unop_u2f) {
			buffer = print_type(buffer, ir->type, true);
			ralloc_asprintf_append(&buffer, "(");
		} else if (ir->operation == ir_unop_rcp) {
			ralloc_asprintf_append (&buffer, "(1.0/(");
		} else {
			ralloc_asprintf_append (&buffer, "%s(", operator_glsl_strs[ir->operation]);
		}
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		ralloc_asprintf_append (&buffer, ")");
		if (ir->operation == ir_unop_rcp) {
			ralloc_asprintf_append (&buffer, ")");
		}
	}
	else if (ir->operation == ir_binop_equal ||
			 ir->operation == ir_binop_nequal ||
			 ir->operation == ir_binop_mod ||
			 ir->operation == ir_binop_dot)
	{
		if (ir->operation == ir_binop_mod)
		{
			ralloc_asprintf_append (&buffer, "(");
			buffer = print_type(buffer, ir->type, true);
			ralloc_asprintf_append (&buffer, "(");
		}
		ralloc_asprintf_append (&buffer, "%s (", operator_glsl_strs[ir->operation]);
		if (ir->operands[0])
			ir->operands[0]->accept(this);
		ralloc_asprintf_append (&buffer, ", ");
		if (ir->operands[1])
			ir->operands[1]->accept(this);
		ralloc_asprintf_append (&buffer, ")");
		if (ir->operation == ir_binop_mod)
            ralloc_asprintf_append (&buffer, "))");
	}
	else {
		ralloc_asprintf_append (&buffer, "(");
		if (ir->operands[0])
			ir->operands[0]->accept(this);

		ralloc_asprintf_append (&buffer, " %s ", operator_glsl_strs[ir->operation]);

		if (ir->operands[1])
			ir->operands[1]->accept(this);
		ralloc_asprintf_append (&buffer, ")");
	}

}


void ir_print_agal_visitor::visit(ir_texture *ir)
{
   ralloc_asprintf_append (&buffer, "(%s ", ir->opcode_string());

   ir->sampler->accept(this);
   ralloc_asprintf_append (&buffer, " ");

   ir->coordinate->accept(this);
	
   if (ir->offset != NULL) {
      ir->offset->accept(this);
   }
	
   if (ir->op != ir_txf) {
      if (ir->projector)
	 ir->projector->accept(this);
      else
	 ralloc_asprintf_append (&buffer, "1");

      if (ir->shadow_comparitor) {
	 ralloc_asprintf_append (&buffer, " ");
	 ir->shadow_comparitor->accept(this);
      } else {
	 ralloc_asprintf_append (&buffer, " ()");
      }
   }

   ralloc_asprintf_append (&buffer, " ");
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
      ralloc_asprintf_append (&buffer, "(");
      ir->lod_info.grad.dPdx->accept(this);
      ralloc_asprintf_append (&buffer, " ");
      ir->lod_info.grad.dPdy->accept(this);
      ralloc_asprintf_append (&buffer, ")");
      break;
   };
   ralloc_asprintf_append (&buffer, ")");
}

void ir_print_agal_visitor::visit(ir_swizzle *ir)
{
   const unsigned swiz[4] = {
      ir->mask.x,
      ir->mask.y,
      ir->mask.z,
      ir->mask.w,
   };
	ir->val->accept(this);

   ralloc_asprintf_append (&buffer, ".");

   int p=0;
   for (unsigned i = 0; i < 4; i++) {
		ralloc_asprintf_append (&buffer, "%c", "xyzw"[swiz[p]]);
		if((writeMask & (1 << i) && p+1 < ir->mask.num_components) || p+1 < readComponents)
			p++;
   }
}

void ir_print_agal_visitor::visit(ir_dereference_variable *ir)
{
   ir_variable *var = ir->variable_referenced();
   print_var_name (var);
}


void ir_print_agal_visitor::visit(ir_dereference_array *ir)
{
   ir->array->accept(this);
   ralloc_asprintf_append (&buffer, "[");
   ir->array_index->accept(this);
   ralloc_asprintf_append (&buffer, "]");
}

int countElements(ir_instruction *ir)
{
	if(ir->as_swizzle()) {
		return ir->as_swizzle()->mask.num_components;
	} else if(ir->as_variable()) {
		return ir->as_variable()->component_slots();
	} else if (ir->as_dereference_variable()) {
		return countElements(ir->as_dereference_variable()->var);
	} else if (ir->as_dereference_array()) {
		return countElements(ir->as_dereference_array()->array);
	} else {
		abort();
	}
}

int min(int x, int y) { return x < y ? x : y; }

static void computeSwizzle(char *swizbuf, ir_assignment *ir)
{
	writeMask = ir->write_mask;
	writeComponents = 0;
   char mask[5] = {0,0,0,0,0};
   for (unsigned i = 0; i < 4; i++) {
	   if ((ir->write_mask & (1 << i)) != 0) {
		   mask[writeComponents] = "xyzw"[i];
		   writeComponents++;
	   }
   }
   mask[writeComponents] = '\0';
   if (!mask[0])
   {
	   strcpy(&mask[0], "xyzw");
   }

   strcpy(swizbuf, &mask[0]);
}

void ir_print_agal_visitor::visit(ir_assignment *ir)
{
   /*if (ir->condition)
   {
   	ralloc_asprintf_append (&buffer, "// condition ");
      ir->condition->accept(this);
	  ralloc_asprintf_append (&buffer, "\n");
   }*/

   ir_instruction *rhs = ir->rhs;
   ir_instruction *op1 = NULL, *op2 = NULL;

   if(ir->rhs->as_swizzle()) {
		rhs = ir->rhs->as_swizzle()->val;

		if(rhs->as_dereference_variable()) {
			rhs = ir->rhs;
		} else {
   			//fprintf(stderr, "DROPPING SWIZZLE\n");
   		}
    }

    char mask[5] = {0,0,0,0,0};
	computeSwizzle(&mask[0], ir);
	readComponents = 0;
	if(strlen(mask) < 4 && ir->lhs->as_dereference_variable()) {
		ir_dereference_variable *dv = ir->lhs->as_dereference_variable();
		ir_variable *var =  dv->variable_referenced();
		if(var->mode == ir_var_out && !hash_table_find(globals->fullywritten, var)) {
			// this out param hasn't been written too yet
			// as a terrible hack we fully write to all components using
			// whatever is in the first constant var
			hash_table_insert(globals->fullywritten, (void*)0x1, var);
			ralloc_asprintf_append (&buffer, "mov %s, %cc0\n", var->name, mode == kPrintGlslVertex ? 'v' : 'f');
		}
	}

   ir_expression *expr = rhs->as_expression();
   ir_texture *tex = rhs->as_texture();
   bool isBinOp = false;

    if(expr) {
    	op1 = expr->operands[0];
		op2 = expr->operands[1];
		int ec1,ec2;
		ir_instruction *tmp=NULL;

		//ralloc_asprintf_append (&buffer, "// expr: %s\n", expr->operator_string());
	   switch(expr->operation)
	   {
			case ir_binop_add: ralloc_asprintf_append (&buffer, "add "); isBinOp=true; break;
			case ir_binop_sub: ralloc_asprintf_append (&buffer, "sub "); isBinOp=true; break;
			case ir_binop_mul: {
				ir_variable *mat=NULL, *vec=NULL;
				int matsz = 0;
				ir_variable *var1=NULL,*var2=NULL;

				if(op1->as_dereference_variable()) {
					var1 = op1->as_dereference_variable()->variable_referenced();
				} else if(op1->as_swizzle()) {
					if(op1->as_swizzle()->val->as_dereference())
						var1 = op1->as_swizzle()->val->as_dereference()->variable_referenced();
				}

				if(op2->as_dereference_variable()) {
					var2 = op2->as_dereference_variable()->variable_referenced();
				} else if(op2->as_swizzle()) {
					if(op2->as_swizzle()->val->as_dereference())
						var2 = op2->as_swizzle()->val->as_dereference()->variable_referenced();
				}

				if(var1) {
					int sz = var1->component_slots();
					switch(sz) {
						case 9:
						case 12:
						case 16:
							matsz = sz;
							mat = var1;
							tmp = op1; // swap the order so second operaand is the matrix
							op1 = op2;
							op2 = tmp;
							break;
						default:
							vec = var1;
							break;
					}
				}
				if(var2) {
					int sz = var2->component_slots();
					switch(sz) {
						case 9:
						case 12:
						case 16:
							matsz = sz;
							mat = var2;
							break;
						default:
							vec = var2;
							break;
					}
				}

				if(mat && vec) {
					switch(matsz) {
						case 9:  ralloc_asprintf_append (&buffer, "m33 "); readComponents = 3; break;
						case 12: ralloc_asprintf_append (&buffer, "m34 "); readComponents = 4; break;
						case 16: ralloc_asprintf_append (&buffer, "m44 "); readComponents = 4; break;
						default: abort();
					}
				} else {
					ralloc_asprintf_append (&buffer, "mul ");
				}
				isBinOp=true;
				break;
			}
			case ir_binop_div: ralloc_asprintf_append (&buffer, "div "); isBinOp=true; break;
			case ir_binop_max: ralloc_asprintf_append (&buffer, "max "); isBinOp=true; break;
			case ir_binop_min: ralloc_asprintf_append (&buffer, "min "); isBinOp=true; break;
			case ir_binop_pow: ralloc_asprintf_append (&buffer, "pow "); isBinOp=true; break;
			case ir_binop_dot:
      			ec1 = countElements(op1);
      			ec2 = countElements(op2);
      			readComponents = expr->operation == ir_binop_dot ? min(ec1,ec2) : 0;
      			ralloc_asprintf_append (&buffer, (readComponents == 3) ? "dp3 " : "dp4 ");
      			isBinOp=true;
				break;
			
			case ir_binop_logic_and: ralloc_asprintf_append (&buffer, "add "); isBinOp=true; break;

			case ir_binop_less: ralloc_asprintf_append (&buffer, "slt "); isBinOp=true; break;
			case ir_binop_gequal: ralloc_asprintf_append (&buffer, "sge "); isBinOp=true; break;
			case ir_binop_equal: ralloc_asprintf_append (&buffer, "seq "); isBinOp=true; break;
			case ir_binop_nequal: ralloc_asprintf_append (&buffer, "sne "); isBinOp=true; break;
			case ir_binop_greater: ralloc_asprintf_append (&buffer, "slt "); isBinOp=true; tmp = op2; op2 = op1; op1 = tmp; break;
			case ir_binop_lequal: ralloc_asprintf_append (&buffer, "sge "); isBinOp=true; tmp = op2; op2 = op1; op1 = tmp; break;
			case ir_unop_fract:ralloc_asprintf_append (&buffer, "frc "); isBinOp=false; break;
			case ir_unop_b2f:  ralloc_asprintf_append (&buffer, "mov "); isBinOp=false; break;
			case ir_unop_rcp:  ralloc_asprintf_append (&buffer, "rcp "); isBinOp=false; break;
			case ir_unop_rsq:  ralloc_asprintf_append (&buffer, "rsq "); isBinOp=false; break;
			case ir_unop_neg:  ralloc_asprintf_append (&buffer, "neg "); isBinOp=false; break;
			case ir_unop_abs:  ralloc_asprintf_append (&buffer, "abs "); isBinOp=false; break;
			case ir_unop_sin:  ralloc_asprintf_append (&buffer, "sin "); isBinOp=false; break;
			case ir_unop_cos:  ralloc_asprintf_append (&buffer, "cos "); isBinOp=false; break;
			case ir_unop_exp2:  ralloc_asprintf_append (&buffer, "exp "); isBinOp=false; break;
			case ir_unop_log2: ralloc_asprintf_append (&buffer, "log "); isBinOp=false; break;
			case ir_unop_sqrt: ralloc_asprintf_append (&buffer, "sqt "); isBinOp=false; break;
			//case ir_unop_logic_not: ralloc_asprintf_append (&buffer, ""); isBinOp=false; break;
			default:           fprintf (stderr, "// default op %s\n", expr->operator_string()); ralloc_asprintf_append (&buffer, "mov "); break;
	   }
    } else if(tex) {
    	ralloc_asprintf_append (&buffer, "tex ");
    	isBinOp=true;
    	op1 = tex->coordinate;
    	op2 = tex->sampler;
    } else if(rhs->as_swizzle()) {
	  ralloc_asprintf_append (&buffer, "mov ");
      op1 = rhs;
    } else if(rhs->as_dereference()) {
	  ralloc_asprintf_append (&buffer, "mov ");
      op1 = rhs;
    } else if(rhs->as_call()) {
    	ir_call *call = rhs->as_call();
    	exec_list_iterator args = call->iterator();
    	const char *name = call->get_callee()->function_name();
	  	if     (strcmp(name, "normalize") == 0) { ralloc_asprintf_append (&buffer, "nrm "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "fract") == 0)     { ralloc_asprintf_append (&buffer, "frc "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "sin") == 0)       { ralloc_asprintf_append (&buffer, "sin "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "cos") == 0)       { ralloc_asprintf_append (&buffer, "cos "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "abs") == 0)       { ralloc_asprintf_append (&buffer, "abs "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "saturate") == 0)  { ralloc_asprintf_append (&buffer, "sat "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "abs") == 0)       { ralloc_asprintf_append (&buffer, "abs "); op1 = (ir_instruction*)args.get(); }
      	else if(strcmp(name, "dot") == 0)       { 
      		op1 = (ir_instruction*)args.get();
      		args.next(); 
      		op2 = (ir_instruction*)args.get();
      		isBinOp = true;
      		int ec1 = countElements(op1);
      		int ec2 = countElements(op2);
      		readComponents = min(ec1,ec2); 
      		ralloc_asprintf_append (&buffer, (readComponents == 3) ? "dp3 " : "dp4 ");
      	}
      	else if(strcmp(name, "pow") == 0)       { ralloc_asprintf_append (&buffer, "pow "); op1 = (ir_instruction*)args.get(); args.next(); op2 = (ir_instruction*)args.get(); isBinOp = true; }
      	else if(strcmp(name, "cross") == 0)     { ralloc_asprintf_append (&buffer, "crs "); op1 = (ir_instruction*)args.get(); args.next(); op2 = (ir_instruction*)args.get(); isBinOp = true; readComponents = 3; }
      	else if(strcmp(name, "clamp") == 0)     { ralloc_asprintf_append (&buffer, "sat "); op1 = (ir_instruction*)args.get(); args.next(); op2 = (ir_instruction*)args.get(); isBinOp = true; }
      	else if(strcmp(name, "max") == 0)       { ralloc_asprintf_append (&buffer, "max "); op1 = (ir_instruction*)args.get(); args.next(); op2 = (ir_instruction*)args.get(); isBinOp = true; }
      	else if(strcmp(name, "min") == 0)       { ralloc_asprintf_append (&buffer, "min "); op1 = (ir_instruction*)args.get(); args.next(); op2 = (ir_instruction*)args.get(); isBinOp = true; }
      	else {
      		ralloc_asprintf_append (&buffer, "UNHANDLED FUNCTION: %s", name);
			return;
      	}
    } else {
		ralloc_asprintf_append (&buffer, "UNHANDLED: %d : ", rhs->ir_type);
		ir->rhs->accept(this);
		return;
	}

   ir->lhs->accept(this);
   ralloc_asprintf_append (&buffer, ".%s, ", mask);


   if(isBinOp) {
   		op1->accept(this);
   		ralloc_asprintf_append (&buffer, ", ");
   		op2->accept(this);
   } else {
		op1->accept(this);
   }

   if(tex) {
   		ir_variable *sampler = tex->sampler->variable_referenced();
   		if (!sampler->type->is_sampler())
   			fprintf(stderr, "Incorrect sampler: %d\n", tex->op);
   		if(tex->op != ir_tex )
   			fprintf(stderr, "Unhandled text op: %d\n", tex->op);
	  ralloc_asprintf_append (&buffer, " <");

	  switch(sampler->type->sampler_dimensionality) {
	  	case GLSL_SAMPLER_DIM_2D: ralloc_asprintf_append (&buffer, "2d,wrap"); break;
	  	case GLSL_SAMPLER_DIM_3D: ralloc_asprintf_append (&buffer, "3d,wrap"); break;
	  	case GLSL_SAMPLER_DIM_CUBE: ralloc_asprintf_append (&buffer, "cube,clamp"); break;
	  	default: fprintf(stderr, "Unhandled texture sampler dimensionality: %d\n", sampler->type->sampler_dimensionality); break;
	  }
	  ralloc_asprintf_append (&buffer, ",linear>");
	}
}

static char* print_float (char* buffer, float f)
{
	const char* fmt = "%.6g";
	if (fabsf(fmodf(f,1.0f)) < 0.00001f)
		fmt = "%.1f";
	ralloc_asprintf_append (&buffer, fmt, f);
	return buffer;
}

void ir_print_agal_visitor::visit(ir_constant *ir)
{
	const glsl_type* type = ir->type;

	if (type == glsl_type::float_type)
	{
		ralloc_asprintf_append (&buffer, "%d", (int)ir->value.f[0]);
		return;
	}
	else if (type == glsl_type::int_type)
	{
		ralloc_asprintf_append (&buffer, "%d", ir->value.i[0]);
		return;
	}
	else if (type == glsl_type::uint_type)
	{
		ralloc_asprintf_append (&buffer, "%u", ir->value.u[0]);
		return;
	}

   const glsl_type *const base_type = ir->type->get_base_type();

   buffer = print_type(buffer, type, true);
   ralloc_asprintf_append (&buffer, "(");

   if (ir->type->is_array()) {
      for (unsigned i = 0; i < ir->type->length; i++)
	 ir->get_array_element(i)->accept(this);
   } else {
      bool first = true;
      for (unsigned i = 0; i < ir->type->components(); i++) {
	 if (!first)
	    ralloc_asprintf_append (&buffer, ", ");
	 first = false;
	 switch (base_type->base_type) {
	 case GLSL_TYPE_UINT:  ralloc_asprintf_append (&buffer, "%u", ir->value.u[i]); break;
	 case GLSL_TYPE_INT:   ralloc_asprintf_append (&buffer, "%d", ir->value.i[i]); break;
	 case GLSL_TYPE_FLOAT: buffer = print_float(buffer, ir->value.f[i]); break;
	 case GLSL_TYPE_BOOL:  ralloc_asprintf_append (&buffer, "%d", ir->value.b[i]); break;
	 default: assert(0);
	 }
      }
   }
   ralloc_asprintf_append (&buffer, ")");
}


void
ir_print_agal_visitor::visit(ir_call *ir)
{
   ralloc_asprintf_append (&buffer, "%s (", ir->callee_name());
   bool first = true;
   foreach_iter(exec_list_iterator, iter, *ir) {
      ir_instruction *const inst = (ir_instruction *) iter.get();
	  if (!first)
		  ralloc_asprintf_append (&buffer, ", ");
      inst->accept(this);
	  first = false;
   }
   ralloc_asprintf_append (&buffer, ")");
}

void
ir_print_agal_visitor::visit(ir_discard *ir)
{
	if (ir->condition == NULL)
		abort();

	ir_rvalue *rv = ir->condition;

	ralloc_asprintf_append (&buffer, "neg ");
	ir->condition->accept(this);
	ralloc_asprintf_append (&buffer, ".x ");
	ir->condition->accept(this);
	ralloc_asprintf_append (&buffer, ".x\n");


   ralloc_asprintf_append (&buffer, "kil ");
	ir->condition->accept(this);
	ralloc_asprintf_append (&buffer, ".x\n");
}

void ir_print_agal_visitor::visit(ir_dereference_record *ir)
{
   ralloc_asprintf_append (&state->info_log, "\nstructs still exist.\n");
}

void
ir_print_agal_visitor::visit(ir_return *ir)
{
	ralloc_asprintf_append (&state->info_log, "\nreturns still exist.\n");
}

void
ir_print_agal_visitor::visit(ir_if *ir)
{
	ralloc_asprintf_append (&state->info_log, "\nunflattened branches still exist.\n");
}

void
ir_print_agal_visitor::visit(ir_loop *ir)
{
	ralloc_asprintf_append (&state->info_log, "\ndynamic looping is unsupported in agal.\n");
}

void
ir_print_agal_visitor::visit(ir_loop_jump *ir)
{
	ralloc_asprintf_append (&state->info_log, "\ndynamic looping is unsupported in agal.\n");
}
