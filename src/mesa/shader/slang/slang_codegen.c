/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
 * Copyright (C) 2008 VMware, Inc.  All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file slang_codegen.c
 * Generate IR tree from AST.
 * \author Brian Paul
 */


/***
 *** NOTES:
 *** The new_() functions return a new instance of a simple IR node.
 *** The gen_() functions generate larger IR trees from the simple nodes.
 ***/



#include "main/imports.h"
#include "main/macros.h"
#include "main/mtypes.h"
#include "shader/program.h"
#include "shader/prog_instruction.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_statevars.h"
#include "slang_typeinfo.h"
#include "slang_builtin.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_label.h"
#include "slang_mem.h"
#include "slang_simplify.h"
#include "slang_emit.h"
#include "slang_vartable.h"
#include "slang_ir.h"
#include "slang_print.h"


/** Max iterations to unroll */
const GLuint MAX_FOR_LOOP_UNROLL_ITERATIONS = 32;

/** Max for-loop body size (in slang operations) to unroll */
const GLuint MAX_FOR_LOOP_UNROLL_BODY_SIZE = 50;

/** Max for-loop body complexity to unroll.
 * We'll compute complexity as the product of the number of iterations
 * and the size of the body.  So long-ish loops with very simple bodies
 * can be unrolled, as well as short loops with larger bodies.
 */
const GLuint MAX_FOR_LOOP_UNROLL_COMPLEXITY = 256;



static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper);

static void
slang_substitute(slang_assemble_ctx *A, slang_operation *oper,
                 GLuint substCount, slang_variable **substOld,
		 slang_operation **substNew, GLboolean isLHS);


/**
 * Retrieves type information about an operation.
 * Returns GL_TRUE on success.
 * Returns GL_FALSE otherwise.
 */
static GLboolean
typeof_operation(const struct slang_assemble_ctx_ *A,
                 slang_operation *op,
                 slang_typeinfo *ti)
{
   return _slang_typeof_operation(op, &A->space, ti, A->atoms, A->log);
}


static GLboolean
is_sampler_type(const slang_fully_specified_type *t)
{
   switch (t->specifier.type) {
   case SLANG_SPEC_SAMPLER_1D:
   case SLANG_SPEC_SAMPLER_2D:
   case SLANG_SPEC_SAMPLER_3D:
   case SLANG_SPEC_SAMPLER_CUBE:
   case SLANG_SPEC_SAMPLER_1D_SHADOW:
   case SLANG_SPEC_SAMPLER_2D_SHADOW:
   case SLANG_SPEC_SAMPLER_RECT:
   case SLANG_SPEC_SAMPLER_RECT_SHADOW:
   case SLANG_SPEC_SAMPLER_1D_ARRAY:
   case SLANG_SPEC_SAMPLER_2D_ARRAY:
   case SLANG_SPEC_SAMPLER_1D_ARRAY_SHADOW:
   case SLANG_SPEC_SAMPLER_2D_ARRAY_SHADOW:
      return GL_TRUE;
   default:
      return GL_FALSE;
   }
}


/**
 * Return the offset (in floats or ints) of the named field within
 * the given struct.  Return -1 if field not found.
 * If field is NULL, return the size of the struct instead.
 */
static GLint
_slang_field_offset(const slang_type_specifier *spec, slang_atom field)
{
   GLint offset = 0;
   GLuint i;
   for (i = 0; i < spec->_struct->fields->num_variables; i++) {
      const slang_variable *v = spec->_struct->fields->variables[i];
      const GLuint sz = _slang_sizeof_type_specifier(&v->type.specifier);
      if (sz > 1) {
         /* types larger than 1 float are register (4-float) aligned */
         offset = (offset + 3) & ~3;
      }
      if (field && v->a_name == field) {
         return offset;
      }
      offset += sz;
   }
   if (field)
      return -1; /* field not found */
   else
      return offset;  /* struct size */
}


/**
 * Return the size (in floats) of the given type specifier.
 * If the size is greater than 4, the size should be a multiple of 4
 * so that the correct number of 4-float registers are allocated.
 * For example, a mat3x2 is size 12 because we want to store the
 * 3 columns in 3 float[4] registers.
 */
GLuint
_slang_sizeof_type_specifier(const slang_type_specifier *spec)
{
   GLuint sz;
   switch (spec->type) {
   case SLANG_SPEC_VOID:
      sz = 0;
      break;
   case SLANG_SPEC_BOOL:
      sz = 1;
      break;
   case SLANG_SPEC_BVEC2:
      sz = 2;
      break;
   case SLANG_SPEC_BVEC3:
      sz = 3;
      break;
   case SLANG_SPEC_BVEC4:
      sz = 4;
      break;
   case SLANG_SPEC_INT:
      sz = 1;
      break;
   case SLANG_SPEC_IVEC2:
      sz = 2;
      break;
   case SLANG_SPEC_IVEC3:
      sz = 3;
      break;
   case SLANG_SPEC_IVEC4:
      sz = 4;
      break;
   case SLANG_SPEC_FLOAT:
      sz = 1;
      break;
   case SLANG_SPEC_VEC2:
      sz = 2;
      break;
   case SLANG_SPEC_VEC3:
      sz = 3;
      break;
   case SLANG_SPEC_VEC4:
      sz = 4;
      break;
   case SLANG_SPEC_MAT2:
      sz = 2 * 4; /* 2 columns (regs) */
      break;
   case SLANG_SPEC_MAT3:
      sz = 3 * 4;
      break;
   case SLANG_SPEC_MAT4:
      sz = 4 * 4;
      break;
   case SLANG_SPEC_MAT23:
      sz = 2 * 4; /* 2 columns (regs) */
      break;
   case SLANG_SPEC_MAT32:
      sz = 3 * 4; /* 3 columns (regs) */
      break;
   case SLANG_SPEC_MAT24:
      sz = 2 * 4;
      break;
   case SLANG_SPEC_MAT42:
      sz = 4 * 4; /* 4 columns (regs) */
      break;
   case SLANG_SPEC_MAT34:
      sz = 3 * 4;
      break;
   case SLANG_SPEC_MAT43:
      sz = 4 * 4; /* 4 columns (regs) */
      break;
   case SLANG_SPEC_SAMPLER_1D:
   case SLANG_SPEC_SAMPLER_2D:
   case SLANG_SPEC_SAMPLER_3D:
   case SLANG_SPEC_SAMPLER_CUBE:
   case SLANG_SPEC_SAMPLER_1D_SHADOW:
   case SLANG_SPEC_SAMPLER_2D_SHADOW:
   case SLANG_SPEC_SAMPLER_RECT:
   case SLANG_SPEC_SAMPLER_RECT_SHADOW:
   case SLANG_SPEC_SAMPLER_1D_ARRAY:
   case SLANG_SPEC_SAMPLER_2D_ARRAY:
   case SLANG_SPEC_SAMPLER_1D_ARRAY_SHADOW:
   case SLANG_SPEC_SAMPLER_2D_ARRAY_SHADOW:
      sz = 1; /* a sampler is basically just an integer index */
      break;
   case SLANG_SPEC_STRUCT:
      sz = _slang_field_offset(spec, 0); /* special use */
      if (sz == 1) {
         /* 1-float structs are actually troublesome to deal with since they
          * might get placed at R.x, R.y, R.z or R.z.  Return size=2 to
          * ensure the object is placed at R.x
          */
         sz = 2;
      }
      else if (sz > 4) {
         sz = (sz + 3) & ~0x3; /* round up to multiple of four */
      }
      break;
   case SLANG_SPEC_ARRAY:
      sz = _slang_sizeof_type_specifier(spec->_array);
      break;
   default:
      _mesa_problem(NULL, "Unexpected type in _slang_sizeof_type_specifier()");
      sz = 0;
   }

   if (sz > 4) {
      /* if size is > 4, it should be a multiple of four */
      assert((sz & 0x3) == 0);
   }
   return sz;
}


/**
 * Query variable/array length (number of elements).
 * This is slightly non-trivial because there are two ways to express
 * arrays: "float x[3]" vs. "float[3] x".
 * \return the length of the array for the given variable, or 0 if not an array
 */
static GLint
_slang_array_length(const slang_variable *var)
{
   if (var->type.array_len > 0) {
      /* Ex: float[4] x; */
      return var->type.array_len;
   }
   if (var->array_len > 0) {
      /* Ex: float x[4]; */
      return var->array_len;
   }
   return 0;
}


/**
 * Compute total size of array give size of element, number of elements.
 * \return size in floats
 */
static GLint
_slang_array_size(GLint elemSize, GLint arrayLen)
{
   GLint total;
   assert(elemSize > 0);
   if (arrayLen > 1) {
      /* round up base type to multiple of 4 */
      total = ((elemSize + 3) & ~0x3) * MAX2(arrayLen, 1);
   }
   else {
      total = elemSize;
   }
   return total;
}


/**
 * Return the TEXTURE_*_INDEX value that corresponds to a sampler type,
 * or -1 if the type is not a sampler.
 */
static GLint
sampler_to_texture_index(const slang_type_specifier_type type)
{
   switch (type) {
   case SLANG_SPEC_SAMPLER_1D:
      return TEXTURE_1D_INDEX;
   case SLANG_SPEC_SAMPLER_2D:
      return TEXTURE_2D_INDEX;
   case SLANG_SPEC_SAMPLER_3D:
      return TEXTURE_3D_INDEX;
   case SLANG_SPEC_SAMPLER_CUBE:
      return TEXTURE_CUBE_INDEX;
   case SLANG_SPEC_SAMPLER_1D_SHADOW:
      return TEXTURE_1D_INDEX; /* XXX fix */
   case SLANG_SPEC_SAMPLER_2D_SHADOW:
      return TEXTURE_2D_INDEX; /* XXX fix */
   case SLANG_SPEC_SAMPLER_RECT:
      return TEXTURE_RECT_INDEX;
   case SLANG_SPEC_SAMPLER_RECT_SHADOW:
      return TEXTURE_RECT_INDEX; /* XXX fix */
   case SLANG_SPEC_SAMPLER_1D_ARRAY:
      return TEXTURE_1D_ARRAY_INDEX;
   case SLANG_SPEC_SAMPLER_2D_ARRAY:
      return TEXTURE_2D_ARRAY_INDEX;
   case SLANG_SPEC_SAMPLER_1D_ARRAY_SHADOW:
      return TEXTURE_1D_ARRAY_INDEX;
   case SLANG_SPEC_SAMPLER_2D_ARRAY_SHADOW:
      return TEXTURE_2D_ARRAY_INDEX;
   default:
      return -1;
   }
}


/** helper to build a SLANG_OPER_IDENTIFIER node */
static void
slang_operation_identifier(slang_operation *oper,
                           slang_assemble_ctx *A,
                           const char *name)
{
   oper->type = SLANG_OPER_IDENTIFIER;
   oper->a_id = slang_atom_pool_atom(A->atoms, name);
}


/**
 * Called when we begin code/IR generation for a new while/do/for loop.
 */
static void
push_loop(slang_assemble_ctx *A, slang_operation *loopOper, slang_ir_node *loopIR)
{
   A->LoopOperStack[A->LoopDepth] = loopOper;
   A->LoopIRStack[A->LoopDepth] = loopIR;
   A->LoopDepth++;
}


/**
 * Called when we end code/IR generation for a new while/do/for loop.
 */
static void
pop_loop(slang_assemble_ctx *A)
{
   assert(A->LoopDepth > 0);
   A->LoopDepth--;
}


/**
 * Return pointer to slang_operation for the loop we're currently inside,
 * or NULL if not in a loop.
 */
static const slang_operation *
current_loop_oper(const slang_assemble_ctx *A)
{
   if (A->LoopDepth > 0)
      return A->LoopOperStack[A->LoopDepth - 1];
   else
      return NULL;
}


/**
 * Return pointer to slang_ir_node for the loop we're currently inside,
 * or NULL if not in a loop.
 */
static slang_ir_node *
current_loop_ir(const slang_assemble_ctx *A)
{
   if (A->LoopDepth > 0)
      return A->LoopIRStack[A->LoopDepth - 1];
   else
      return NULL;
}


/**********************************************************************/


/**
 * Map "_asm foo" to IR_FOO, etc.
 */
typedef struct
{
   const char *Name;
   slang_ir_opcode Opcode;
   GLuint HaveRetValue, NumParams;
} slang_asm_info;


static slang_asm_info AsmInfo[] = {
   /* vec4 binary op */
   { "vec4_add", IR_ADD, 1, 2 },
   { "vec4_subtract", IR_SUB, 1, 2 },
   { "vec4_multiply", IR_MUL, 1, 2 },
   { "vec4_dot", IR_DOT4, 1, 2 },
   { "vec3_dot", IR_DOT3, 1, 2 },
   { "vec2_dot", IR_DOT2, 1, 2 },
   { "vec3_nrm", IR_NRM3, 1, 1 },
   { "vec4_nrm", IR_NRM4, 1, 1 },
   { "vec3_cross", IR_CROSS, 1, 2 },
   { "vec4_lrp", IR_LRP, 1, 3 },
   { "vec4_min", IR_MIN, 1, 2 },
   { "vec4_max", IR_MAX, 1, 2 },
   { "vec4_cmp", IR_CMP, 1, 3 },
   { "vec4_clamp", IR_CLAMP, 1, 3 },
   { "vec4_seq", IR_SEQUAL, 1, 2 },
   { "vec4_sne", IR_SNEQUAL, 1, 2 },
   { "vec4_sge", IR_SGE, 1, 2 },
   { "vec4_sgt", IR_SGT, 1, 2 },
   { "vec4_sle", IR_SLE, 1, 2 },
   { "vec4_slt", IR_SLT, 1, 2 },
   /* vec4 unary */
   { "vec4_move", IR_MOVE, 1, 1 },
   { "vec4_floor", IR_FLOOR, 1, 1 },
   { "vec4_frac", IR_FRAC, 1, 1 },
   { "vec4_abs", IR_ABS, 1, 1 },
   { "vec4_negate", IR_NEG, 1, 1 },
   { "vec4_ddx", IR_DDX, 1, 1 },
   { "vec4_ddy", IR_DDY, 1, 1 },
   /* float binary op */
   { "float_power", IR_POW, 1, 2 },
   /* texture / sampler */
   { "vec4_tex_1d", IR_TEX, 1, 2 },
   { "vec4_tex_1d_bias", IR_TEXB, 1, 2 },  /* 1d w/ bias */
   { "vec4_tex_1d_proj", IR_TEXP, 1, 2 },  /* 1d w/ projection */
   { "vec4_tex_2d", IR_TEX, 1, 2 },
   { "vec4_tex_2d_bias", IR_TEXB, 1, 2 },  /* 2d w/ bias */
   { "vec4_tex_2d_proj", IR_TEXP, 1, 2 },  /* 2d w/ projection */
   { "vec4_tex_3d", IR_TEX, 1, 2 },
   { "vec4_tex_3d_bias", IR_TEXB, 1, 2 },  /* 3d w/ bias */
   { "vec4_tex_3d_proj", IR_TEXP, 1, 2 },  /* 3d w/ projection */
   { "vec4_tex_cube", IR_TEX, 1, 2 },      /* cubemap */
   { "vec4_tex_rect", IR_TEX, 1, 2 },      /* rectangle */
   { "vec4_tex_rect_bias", IR_TEX, 1, 2 }, /* rectangle w/ projection */
   { "vec4_tex_1d_array", IR_TEX, 1, 2 },
   { "vec4_tex_1d_array_bias", IR_TEXB, 1, 2 },
   { "vec4_tex_1d_array_shadow", IR_TEX, 1, 2 },
   { "vec4_tex_1d_array_bias_shadow", IR_TEXB, 1, 2 },
   { "vec4_tex_2d_array", IR_TEX, 1, 2 },
   { "vec4_tex_2d_array_bias", IR_TEXB, 1, 2 },
   { "vec4_tex_2d_array_shadow", IR_TEX, 1, 2 },
   { "vec4_tex_2d_array_bias_shadow", IR_TEXB, 1, 2 },

   /* texture / sampler but with shadow comparison */
   { "vec4_tex_1d_shadow", IR_TEX_SH, 1, 2 },
   { "vec4_tex_1d_bias_shadow", IR_TEXB_SH, 1, 2 },
   { "vec4_tex_1d_proj_shadow", IR_TEXP_SH, 1, 2 },
   { "vec4_tex_2d_shadow", IR_TEX_SH, 1, 2 },
   { "vec4_tex_2d_bias_shadow", IR_TEXB_SH, 1, 2 },
   { "vec4_tex_2d_proj_shadow", IR_TEXP_SH, 1, 2 },
   { "vec4_tex_rect_shadow", IR_TEX_SH, 1, 2 },
   { "vec4_tex_rect_proj_shadow", IR_TEXP_SH, 1, 2 },

   /* unary op */
   { "ivec4_to_vec4", IR_I_TO_F, 1, 1 }, /* int[4] to float[4] */
   { "vec4_to_ivec4", IR_F_TO_I, 1, 1 },  /* float[4] to int[4] */
   { "float_exp", IR_EXP, 1, 1 },
   { "float_exp2", IR_EXP2, 1, 1 },
   { "float_log2", IR_LOG2, 1, 1 },
   { "float_rsq", IR_RSQ, 1, 1 },
   { "float_rcp", IR_RCP, 1, 1 },
   { "float_sine", IR_SIN, 1, 1 },
   { "float_cosine", IR_COS, 1, 1 },
   { "float_noise1", IR_NOISE1, 1, 1},
   { "float_noise2", IR_NOISE2, 1, 1},
   { "float_noise3", IR_NOISE3, 1, 1},
   { "float_noise4", IR_NOISE4, 1, 1},

   { NULL, IR_NOP, 0, 0 }
};


static slang_ir_node *
new_node3(slang_ir_opcode op,
          slang_ir_node *c0, slang_ir_node *c1, slang_ir_node *c2)
{
   slang_ir_node *n = (slang_ir_node *) _slang_alloc(sizeof(slang_ir_node));
   if (n) {
      n->Opcode = op;
      n->Children[0] = c0;
      n->Children[1] = c1;
      n->Children[2] = c2;
      n->InstLocation = -1;
   }
   return n;
}

static slang_ir_node *
new_node2(slang_ir_opcode op, slang_ir_node *c0, slang_ir_node *c1)
{
   return new_node3(op, c0, c1, NULL);
}

static slang_ir_node *
new_node1(slang_ir_opcode op, slang_ir_node *c0)
{
   return new_node3(op, c0, NULL, NULL);
}

static slang_ir_node *
new_node0(slang_ir_opcode op)
{
   return new_node3(op, NULL, NULL, NULL);
}


/**
 * Create sequence of two nodes.
 */
static slang_ir_node *
new_seq(slang_ir_node *left, slang_ir_node *right)
{
   if (!left)
      return right;
   if (!right)
      return left;
   return new_node2(IR_SEQ, left, right);
}

static slang_ir_node *
new_label(slang_label *label)
{
   slang_ir_node *n = new_node0(IR_LABEL);
   assert(label);
   if (n)
      n->Label = label;
   return n;
}

static slang_ir_node *
new_float_literal(const float v[4], GLuint size)
{
   slang_ir_node *n = new_node0(IR_FLOAT);
   assert(size <= 4);
   COPY_4V(n->Value, v);
   /* allocate a storage object, but compute actual location (Index) later */
   n->Store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, size);
   return n;
}


static slang_ir_node *
new_not(slang_ir_node *n)
{
   return new_node1(IR_NOT, n);
}


/**
 * Non-inlined function call.
 */
static slang_ir_node *
new_function_call(slang_ir_node *code, slang_label *name)
{
   slang_ir_node *n = new_node1(IR_CALL, code);
   assert(name);
   if (n)
      n->Label = name;
   return n;
}


/**
 * Unconditional jump.
 */
static slang_ir_node *
new_return(slang_label *dest)
{
   slang_ir_node *n = new_node0(IR_RETURN);
   assert(dest);
   if (n)
      n->Label = dest;
   return n;
}


static slang_ir_node *
new_loop(slang_ir_node *body)
{
   return new_node1(IR_LOOP, body);
}


static slang_ir_node *
new_break(slang_ir_node *loopNode)
{
   slang_ir_node *n = new_node0(IR_BREAK);
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   if (n) {
      /* insert this node at head of linked list of cont/break instructions */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


/**
 * Make new IR_BREAK_IF_TRUE.
 */
static slang_ir_node *
new_break_if_true(slang_assemble_ctx *A, slang_ir_node *cond)
{
   slang_ir_node *loopNode = current_loop_ir(A);
   slang_ir_node *n;
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   n = new_node1(IR_BREAK_IF_TRUE, cond);
   if (n) {
      /* insert this node at head of linked list of cont/break instructions */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


/**
 * Make new IR_CONT_IF_TRUE node.
 */
static slang_ir_node *
new_cont_if_true(slang_assemble_ctx *A, slang_ir_node *cond)
{
   slang_ir_node *loopNode = current_loop_ir(A);
   slang_ir_node *n;
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);
   n = new_node1(IR_CONT_IF_TRUE, cond);
   if (n) {
      n->Parent = loopNode; /* pointer to containing loop */
      /* insert this node at head of linked list of cont/break instructions */
      n->List = loopNode->List;
      loopNode->List = n;
   }
   return n;
}


static slang_ir_node *
new_cond(slang_ir_node *n)
{
   slang_ir_node *c = new_node1(IR_COND, n);
   return c;
}


static slang_ir_node *
new_if(slang_ir_node *cond, slang_ir_node *ifPart, slang_ir_node *elsePart)
{
   return new_node3(IR_IF, cond, ifPart, elsePart);
}


/**
 * New IR_VAR node - a reference to a previously declared variable.
 */
static slang_ir_node *
new_var(slang_assemble_ctx *A, slang_variable *var)
{
   slang_ir_node *n = new_node0(IR_VAR);
   if (n) {
      ASSERT(var);
      ASSERT(var->store);
      ASSERT(!n->Store);
      ASSERT(!n->Var);

      /* Set IR node's Var and Store pointers */
      n->Var = var;
      n->Store = var->store;
   }
   return n;
}


/**
 * Check if the given function is really just a wrapper for a
 * basic assembly instruction.
 */
static GLboolean
slang_is_asm_function(const slang_function *fun)
{
   if (fun->body->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE &&
       fun->body->num_children == 1 &&
       fun->body->children[0].type == SLANG_OPER_ASM) {
      return GL_TRUE;
   }
   return GL_FALSE;
}


static GLboolean
_slang_is_noop(const slang_operation *oper)
{
   if (!oper ||
       oper->type == SLANG_OPER_VOID ||
       (oper->num_children == 1 && oper->children[0].type == SLANG_OPER_VOID))
      return GL_TRUE;
   else
      return GL_FALSE;
}


/**
 * Recursively search tree for a node of the given type.
 */
#if 0
static slang_operation *
_slang_find_node_type(slang_operation *oper, slang_operation_type type)
{
   GLuint i;
   if (oper->type == type)
      return oper;
   for (i = 0; i < oper->num_children; i++) {
      slang_operation *p = _slang_find_node_type(&oper->children[i], type);
      if (p)
         return p;
   }
   return NULL;
}
#endif


/**
 * Count the number of operations of the given time rooted at 'oper'.
 */
static GLuint
_slang_count_node_type(const slang_operation *oper, slang_operation_type type)
{
   GLuint i, count = 0;
   if (oper->type == type) {
      return 1;
   }
   for (i = 0; i < oper->num_children; i++) {
      count += _slang_count_node_type(&oper->children[i], type);
   }
   return count;
}


/**
 * Check if the 'return' statement found under 'oper' is a "tail return"
 * that can be no-op'd.  For example:
 *
 * void func(void)
 * {
 *    .. do something ..
 *    return;   // this is a no-op
 * }
 *
 * This is used when determining if a function can be inlined.  If the
 * 'return' is not the last statement, we can't inline the function since
 * we still need the semantic behaviour of the 'return' but we don't want
 * to accidentally return from the _calling_ function.  We'd need to use an
 * unconditional branch, but we don't have such a GPU instruction (not
 * always, at least).
 */
static GLboolean
_slang_is_tail_return(const slang_operation *oper)
{
   GLuint k = oper->num_children;

   while (k > 0) {
      const slang_operation *last = &oper->children[k - 1];
      if (last->type == SLANG_OPER_RETURN)
         return GL_TRUE;
      else if (last->type == SLANG_OPER_IDENTIFIER ||
               last->type == SLANG_OPER_LABEL)
         k--; /* try prev child */
      else if (last->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE ||
               last->type == SLANG_OPER_BLOCK_NEW_SCOPE)
         /* try sub-children */
         return _slang_is_tail_return(last);
      else
         break;
   }

   return GL_FALSE;
}


/**
 * Generate a variable declaration opeartion.
 * I.e.: generate AST code for "bool flag = false;"
 */
static void
slang_generate_declaration(slang_assemble_ctx *A,
                           slang_variable_scope *scope,
                           slang_operation *decl,
                           slang_type_specifier_type type,
                           const char *name,
                           GLint initValue)
{
   slang_variable *var;

   assert(type == SLANG_SPEC_BOOL ||
          type == SLANG_SPEC_INT);

   decl->type = SLANG_OPER_VARIABLE_DECL;

   var = slang_variable_scope_grow(scope);

   slang_fully_specified_type_construct(&var->type);

   var->type.specifier.type = type;
   var->a_name = slang_atom_pool_atom(A->atoms, name);
   decl->a_id = var->a_name;
   var->initializer = slang_operation_new(1);
   slang_operation_literal_bool(var->initializer, initValue);
}


static void
slang_resolve_variable(slang_operation *oper)
{
   if (oper->type == SLANG_OPER_IDENTIFIER && !oper->var) {
      oper->var = _slang_variable_locate(oper->locals, oper->a_id, GL_TRUE);
   }
}


/**
 * Rewrite AST code for "return expression;".
 *
 * We return values from functions by assinging the returned value to
 * the hidden __retVal variable which is an extra 'out' parameter we add
 * to the function signature.
 * This code basically converts "return expr;" into "__retVal = expr; return;"
 *
 * \return the new AST code.
 */
static slang_operation *
gen_return_with_expression(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_operation *blockOper, *assignOper;

   assert(oper->type == SLANG_OPER_RETURN);

   if (A->CurFunction->header.type.specifier.type == SLANG_SPEC_VOID) {
      slang_info_log_error(A->log, "illegal return expression");
      return NULL;
   }

   blockOper = slang_operation_new(1);
   blockOper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;
   blockOper->locals->outer_scope = oper->locals->outer_scope;
   slang_operation_add_children(blockOper, 2);

   if (A->UseReturnFlag) {
      /* Emit:
       *    {
       *       if (__notRetFlag)
       *          __retVal = expr;
       *       __notRetFlag = 0;
       *    }
       */
      {
         slang_operation *ifOper = slang_oper_child(blockOper, 0);
         ifOper->type = SLANG_OPER_IF;
         slang_operation_add_children(ifOper, 3);
         {
            slang_operation *cond = slang_oper_child(ifOper, 0);
            cond->type = SLANG_OPER_IDENTIFIER;
            cond->a_id = slang_atom_pool_atom(A->atoms, "__notRetFlag");
         }
         {
            slang_operation *elseOper = slang_oper_child(ifOper, 2);
            elseOper->type = SLANG_OPER_VOID;
         }
         assignOper = slang_oper_child(ifOper, 1);
      }
      {
         slang_operation *setOper = slang_oper_child(blockOper, 1);
         setOper->type = SLANG_OPER_ASSIGN;
         slang_operation_add_children(setOper, 2);
         {
            slang_operation *lhs = slang_oper_child(setOper, 0);
            lhs->type = SLANG_OPER_IDENTIFIER;
            lhs->a_id = slang_atom_pool_atom(A->atoms, "__notRetFlag");
         }
         {
            slang_operation *rhs = slang_oper_child(setOper, 1);
            slang_operation_literal_bool(rhs, GL_FALSE);
         }
      }
   }
   else {
      /* Emit:
       *    {
       *       __retVal = expr;
       *       return_inlined;
       *    }
       */
      assignOper = slang_oper_child(blockOper, 0);
      {
         slang_operation *returnOper = slang_oper_child(blockOper, 1);
         returnOper->type = SLANG_OPER_RETURN_INLINED;
         assert(returnOper->num_children == 0);
      }
   }

   /* __retVal = expression; */
   assignOper->type = SLANG_OPER_ASSIGN;
   slang_operation_add_children(assignOper, 2);
   {
      slang_operation *lhs = slang_oper_child(assignOper, 0);
      lhs->type = SLANG_OPER_IDENTIFIER;
      lhs->a_id = slang_atom_pool_atom(A->atoms, "__retVal");
   }
   {
      slang_operation *rhs = slang_oper_child(assignOper, 1);
      slang_operation_copy(rhs, &oper->children[0]);
   }

   /*blockOper->locals->outer_scope = oper->locals->outer_scope;*/

   /*slang_print_tree(blockOper, 0);*/

   return blockOper;
}


/**
 * Rewrite AST code for "return;" (no expression).
 */
static slang_operation *
gen_return_without_expression(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_operation *newRet;

   assert(oper->type == SLANG_OPER_RETURN);

   if (A->CurFunction->header.type.specifier.type != SLANG_SPEC_VOID) {
      slang_info_log_error(A->log, "return statement requires an expression");
      return NULL;
   }

   if (A->UseReturnFlag) {
      /* Emit:
       *    __notRetFlag = 0;
       */
      {
         newRet = slang_operation_new(1);
         newRet->locals->outer_scope = oper->locals->outer_scope;
         newRet->type = SLANG_OPER_ASSIGN;
         slang_operation_add_children(newRet, 2);
         {
            slang_operation *lhs = slang_oper_child(newRet, 0);
            lhs->type = SLANG_OPER_IDENTIFIER;
            lhs->a_id = slang_atom_pool_atom(A->atoms, "__notRetFlag");
         }
         {
            slang_operation *rhs = slang_oper_child(newRet, 1);
            slang_operation_literal_bool(rhs, GL_FALSE);
         }
      }
   }
   else {
      /* Emit:
       *    return_inlined;
       */
      newRet = slang_operation_new(1);
      newRet->locals->outer_scope = oper->locals->outer_scope;
      newRet->type = SLANG_OPER_RETURN_INLINED;
   }

   /*slang_print_tree(newRet, 0);*/

   return newRet;
}




/**
 * Replace particular variables (SLANG_OPER_IDENTIFIER) with new expressions.
 */
static void
slang_substitute(slang_assemble_ctx *A, slang_operation *oper,
                 GLuint substCount, slang_variable **substOld,
		 slang_operation **substNew, GLboolean isLHS)
{
   switch (oper->type) {
   case SLANG_OPER_VARIABLE_DECL:
      {
         slang_variable *v = _slang_variable_locate(oper->locals,
                                                    oper->a_id, GL_TRUE);
         assert(v);
         if (v->initializer && oper->num_children == 0) {
            /* set child of oper to copy of initializer */
            oper->num_children = 1;
            oper->children = slang_operation_new(1);
            slang_operation_copy(&oper->children[0], v->initializer);
         }
         if (oper->num_children == 1) {
            /* the initializer */
            slang_substitute(A, &oper->children[0], substCount,
                             substOld, substNew, GL_FALSE);
         }
      }
      break;
   case SLANG_OPER_IDENTIFIER:
      assert(oper->num_children == 0);
      if (1/**!isLHS XXX FIX */) {
         slang_atom id = oper->a_id;
         slang_variable *v;
	 GLuint i;
         v = _slang_variable_locate(oper->locals, id, GL_TRUE);
	 if (!v) {
            if (strcmp((char *) oper->a_id, "__notRetFlag"))
               _mesa_problem(NULL, "var %s not found!\n", (char *) oper->a_id);
            return;
	 }

	 /* look for a substitution */
	 for (i = 0; i < substCount; i++) {
	    if (v == substOld[i]) {
               /* OK, replace this SLANG_OPER_IDENTIFIER with a new expr */
#if 0 /* DEBUG only */
	       if (substNew[i]->type == SLANG_OPER_IDENTIFIER) {
                  assert(substNew[i]->var);
                  assert(substNew[i]->var->a_name);
		  printf("Substitute %s with %s in id node %p\n",
			 (char*)v->a_name, (char*) substNew[i]->var->a_name,
			 (void*) oper);
               }
	       else {
		  printf("Substitute %s with %f in id node %p\n",
			 (char*)v->a_name, substNew[i]->literal[0],
			 (void*) oper);
               }
#endif
	       slang_operation_copy(oper, substNew[i]);
	       break;
	    }
	 }
      }
      break;

   case SLANG_OPER_RETURN:
      {
         slang_operation *newReturn;
         /* generate new 'return' code' */
         if (slang_oper_child(oper, 0)->type == SLANG_OPER_VOID)
            newReturn = gen_return_without_expression(A, oper);
         else
            newReturn = gen_return_with_expression(A, oper);

         if (!newReturn)
            return;

         /* do substitutions on the new 'return' code */
         slang_substitute(A, newReturn,
                          substCount, substOld, substNew, GL_FALSE);

         /* install new 'return' code */
         slang_operation_copy(oper, newReturn);
         slang_operation_destruct(newReturn);
      }
      break;

   case SLANG_OPER_ASSIGN:
   case SLANG_OPER_SUBSCRIPT:
      /* special case:
       * child[0] can't have substitutions but child[1] can.
       */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      slang_substitute(A, &oper->children[1],
                       substCount, substOld, substNew, GL_FALSE);
      break;
   case SLANG_OPER_FIELD:
      /* XXX NEW - test */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      break;
   default:
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) 
            slang_substitute(A, &oper->children[i],
                             substCount, substOld, substNew, GL_FALSE);
      }
   }
}


/**
 * Produce inline code for a call to an assembly instruction.
 * This is typically used to compile a call to a built-in function like this:
 *
 * vec4 mix(const vec4 x, const vec4 y, const vec4 a)
 * {
 *    __asm vec4_lrp __retVal, a, y, x;
 * }
 *
 *
 * A call to
 *     r = mix(p1, p2, p3);
 *
 * Becomes:
 *
 *              mov
 *             /   \
 *            r   vec4_lrp
 *                 /  |  \
 *                p3  p2  p1
 *
 * We basically translate a SLANG_OPER_CALL into a SLANG_OPER_ASM.
 */
static slang_operation *
slang_inline_asm_function(slang_assemble_ctx *A,
                          slang_function *fun, slang_operation *oper)
{
   const GLuint numArgs = oper->num_children;
   GLuint i;
   slang_operation *inlined;
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   slang_variable **substOld;
   slang_operation **substNew;

   ASSERT(slang_is_asm_function(fun));
   ASSERT(fun->param_count == numArgs + haveRetValue);

   /*
   printf("Inline %s as %s\n",
          (char*) fun->header.a_name,
          (char*) fun->body->children[0].a_id);
   */

   /*
    * We'll substitute formal params with actual args in the asm call.
    */
   substOld = (slang_variable **)
      _slang_alloc(numArgs * sizeof(slang_variable *));
   substNew = (slang_operation **)
      _slang_alloc(numArgs * sizeof(slang_operation *));
   for (i = 0; i < numArgs; i++) {
      substOld[i] = fun->parameters->variables[i];
      substNew[i] = oper->children + i;
   }

   /* make a copy of the code to inline */
   inlined = slang_operation_new(1);
   slang_operation_copy(inlined, &fun->body->children[0]);
   if (haveRetValue) {
      /* get rid of the __retVal child */
      inlined->num_children--;
      for (i = 0; i < inlined->num_children; i++) {
         inlined->children[i] = inlined->children[i + 1];
      }
   }

   /* now do formal->actual substitutions */
   slang_substitute(A, inlined, numArgs, substOld, substNew, GL_FALSE);

   _slang_free(substOld);
   _slang_free(substNew);

#if 0
   printf("+++++++++++++ inlined asm function %s +++++++++++++\n",
          (char *) fun->header.a_name);
   slang_print_tree(inlined, 3);
   printf("+++++++++++++++++++++++++++++++++++++++++++++++++++\n");
#endif

   return inlined;
}


/**
 * Inline the given function call operation.
 * Return a new slang_operation that corresponds to the inlined code.
 */
static slang_operation *
slang_inline_function_call(slang_assemble_ctx * A, slang_function *fun,
			   slang_operation *oper, slang_operation *returnOper)
{
   typedef enum {
      SUBST = 1,
      COPY_IN,
      COPY_OUT
   } ParamMode;
   ParamMode *paramMode;
   const GLboolean haveRetValue = _slang_function_has_return_value(fun);
   const GLuint numArgs = oper->num_children;
   const GLuint totalArgs = numArgs + haveRetValue;
   slang_operation *args = oper->children;
   slang_operation *inlined, *top;
   slang_variable **substOld;
   slang_operation **substNew;
   GLuint substCount, numCopyIn, i;
   slang_function *prevFunction;
   slang_variable_scope *newScope = NULL;

   /* save / push */
   prevFunction = A->CurFunction;
   A->CurFunction = fun;

   /*assert(oper->type == SLANG_OPER_CALL); (or (matrix) multiply, etc) */
   assert(fun->param_count == totalArgs);

   /* allocate temporary arrays */
   paramMode = (ParamMode *)
      _slang_alloc(totalArgs * sizeof(ParamMode));
   substOld = (slang_variable **)
      _slang_alloc(totalArgs * sizeof(slang_variable *));
   substNew = (slang_operation **)
      _slang_alloc(totalArgs * sizeof(slang_operation *));

#if 0
   printf("\nInline call to %s  (total vars=%d  nparams=%d)\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);
#endif

   if (haveRetValue && !returnOper) {
      /* Create 3-child comma sequence for inlined code:
       * child[0]:  declare __resultTmp
       * child[1]:  inlined function body
       * child[2]:  __resultTmp
       */
      slang_operation *commaSeq;
      slang_operation *declOper = NULL;
      slang_variable *resultVar;

      commaSeq = slang_operation_new(1);
      commaSeq->type = SLANG_OPER_SEQUENCE;
      assert(commaSeq->locals);
      commaSeq->locals->outer_scope = oper->locals->outer_scope;
      commaSeq->num_children = 3;
      commaSeq->children = slang_operation_new(3);
      /* allocate the return var */
      resultVar = slang_variable_scope_grow(commaSeq->locals);
      /*
      printf("Alloc __resultTmp in scope %p for retval of calling %s\n",
             (void*)commaSeq->locals, (char *) fun->header.a_name);
      */

      resultVar->a_name = slang_atom_pool_atom(A->atoms, "__resultTmp");
      resultVar->type = fun->header.type; /* XXX copy? */
      resultVar->isTemp = GL_TRUE;

      /* child[0] = __resultTmp declaration */
      declOper = &commaSeq->children[0];
      declOper->type = SLANG_OPER_VARIABLE_DECL;
      declOper->a_id = resultVar->a_name;
      declOper->locals->outer_scope = commaSeq->locals;

      /* child[1] = function body */
      inlined = &commaSeq->children[1];
      inlined->locals->outer_scope = commaSeq->locals;

      /* child[2] = __resultTmp reference */
      returnOper = &commaSeq->children[2];
      returnOper->type = SLANG_OPER_IDENTIFIER;
      returnOper->a_id = resultVar->a_name;
      returnOper->locals->outer_scope = commaSeq->locals;

      top = commaSeq;
   }
   else {
      top = inlined = slang_operation_new(1);
      /* XXXX this may be inappropriate!!!! */
      inlined->locals->outer_scope = oper->locals->outer_scope;
   }


   assert(inlined->locals);

   /* Examine the parameters, look for inout/out params, look for possible
    * substitutions, etc:
    *    param type      behaviour
    *     in             copy actual to local
    *     const in       substitute param with actual
    *     out            copy out
    */
   substCount = 0;
   for (i = 0; i < totalArgs; i++) {
      slang_variable *p = fun->parameters->variables[i];
      /*
      printf("Param %d: %s %s \n", i,
             slang_type_qual_string(p->type.qualifier),
	     (char *) p->a_name);
      */
      if (p->type.qualifier == SLANG_QUAL_INOUT ||
	  p->type.qualifier == SLANG_QUAL_OUT) {
	 /* an output param */
         slang_operation *arg;
         if (i < numArgs)
            arg = &args[i];
         else
            arg = returnOper;
	 paramMode[i] = SUBST;

	 if (arg->type == SLANG_OPER_IDENTIFIER)
            slang_resolve_variable(arg);

         /* replace parameter 'p' with argument 'arg' */
	 substOld[substCount] = p;
	 substNew[substCount] = arg; /* will get copied */
	 substCount++;
      }
      else if (p->type.qualifier == SLANG_QUAL_CONST) {
	 /* a constant input param */
         if (args[i].type == SLANG_OPER_IDENTIFIER ||
             args[i].type == SLANG_OPER_LITERAL_FLOAT ||
             args[i].type == SLANG_OPER_SUBSCRIPT) {
	    /* replace all occurances of this parameter variable with the
	     * actual argument variable or a literal.
	     */
	    paramMode[i] = SUBST;
            slang_resolve_variable(&args[i]);
	    substOld[substCount] = p;
	    substNew[substCount] = &args[i]; /* will get copied */
	    substCount++;
	 }
	 else {
	    paramMode[i] = COPY_IN;
	 }
      }
      else {
	 paramMode[i] = COPY_IN;
      }
      assert(paramMode[i]);
   }

   /* actual code inlining: */
   slang_operation_copy(inlined, fun->body);

   /*** XXX review this */
   assert(inlined->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE ||
          inlined->type == SLANG_OPER_BLOCK_NEW_SCOPE);
   inlined->type = SLANG_OPER_BLOCK_NEW_SCOPE;

#if 0
   printf("======================= orig body code ======================\n");
   printf("=== params scope = %p\n", (void*) fun->parameters);
   slang_print_tree(fun->body, 8);
   printf("======================= copied code =========================\n");
   slang_print_tree(inlined, 8);
#endif

   /* do parameter substitution in inlined code: */
   slang_substitute(A, inlined, substCount, substOld, substNew, GL_FALSE);

#if 0
   printf("======================= subst code ==========================\n");
   slang_print_tree(inlined, 8);
   printf("=============================================================\n");
#endif

   /* New prolog statements: (inserted before the inlined code)
    * Copy the 'in' arguments.
    */
   numCopyIn = 0;
   for (i = 0; i < numArgs; i++) {
      if (paramMode[i] == COPY_IN) {
	 slang_variable *p = fun->parameters->variables[i];
	 /* declare parameter 'p' */
	 slang_operation *decl = slang_operation_insert(&inlined->num_children,
							&inlined->children,
							numCopyIn);

	 decl->type = SLANG_OPER_VARIABLE_DECL;
         assert(decl->locals);
         decl->locals->outer_scope = inlined->locals;
	 decl->a_id = p->a_name;
	 decl->num_children = 1;
	 decl->children = slang_operation_new(1);

         /* child[0] is the var's initializer */
         slang_operation_copy(&decl->children[0], args + i);

         /* add parameter 'p' to the local variable scope here */
         {
            slang_variable *pCopy = slang_variable_scope_grow(inlined->locals);
            pCopy->type = p->type;
            pCopy->a_name = p->a_name;
            pCopy->array_len = p->array_len;
         }

         newScope = inlined->locals;
	 numCopyIn++;
      }
   }

   /* Now add copies of the function's local vars to the new variable scope */
   for (i = totalArgs; i < fun->parameters->num_variables; i++) {
      slang_variable *p = fun->parameters->variables[i];
      slang_variable *pCopy = slang_variable_scope_grow(inlined->locals);
      pCopy->type = p->type;
      pCopy->a_name = p->a_name;
      pCopy->array_len = p->array_len;
   }


   /* New epilog statements:
    * 1. Create end of function label to jump to from return statements.
    * 2. Copy the 'out' parameter vars
    */
   {
      slang_operation *lab = slang_operation_insert(&inlined->num_children,
                                                    &inlined->children,
                                                    inlined->num_children);
      lab->type = SLANG_OPER_LABEL;
      lab->label = A->curFuncEndLabel;
   }

   for (i = 0; i < totalArgs; i++) {
      if (paramMode[i] == COPY_OUT) {
	 const slang_variable *p = fun->parameters->variables[i];
	 /* actualCallVar = outParam */
	 /*if (i > 0 || !haveRetValue)*/
	 slang_operation *ass = slang_operation_insert(&inlined->num_children,
						       &inlined->children,
						       inlined->num_children);
	 ass->type = SLANG_OPER_ASSIGN;
	 ass->num_children = 2;
         ass->locals->outer_scope = inlined->locals;
	 ass->children = slang_operation_new(2);
	 ass->children[0] = args[i]; /*XXX copy */
	 ass->children[1].type = SLANG_OPER_IDENTIFIER;
	 ass->children[1].a_id = p->a_name;
         ass->children[1].locals->outer_scope = ass->locals;
      }
   }

   _slang_free(paramMode);
   _slang_free(substOld);
   _slang_free(substNew);

   /* Update scoping to use the new local vars instead of the
    * original function's vars.  This is especially important
    * for nested inlining.
    */
   if (newScope)
      slang_replace_scope(inlined, fun->parameters, newScope);

#if 0
   printf("Done Inline call to %s  (total vars=%d  nparams=%d)\n\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);
   slang_print_tree(top, 0);
#endif

   /* pop */
   A->CurFunction = prevFunction;

   return top;
}


/**
 * Insert declaration for "bool __notRetFlag" in given block operation.
 * This is used when we can't emit "early" return statements in subroutines.
 */
static void
declare_return_flag(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_operation *decl;

   assert(oper->type == SLANG_OPER_BLOCK_NEW_SCOPE ||
          oper->type == SLANG_OPER_SEQUENCE);

   decl = slang_operation_insert_child(oper, 1);

   slang_generate_declaration(A, oper->locals, decl,
                              SLANG_SPEC_BOOL, "__notRetFlag", GL_TRUE);

   /*slang_print_tree(oper, 0);*/
}


/**
 * Recursively replace instances of the old node type with the new type.
 */
static void
replace_node_type(slang_operation *oper, slang_operation_type oldType,
                  slang_operation_type newType)
{
   GLuint i;

   if (oper->type == oldType)
      oper->type = newType;

   for (i = 0; i < slang_oper_num_children(oper); i++) {
      replace_node_type(slang_oper_child(oper, i), oldType, newType);
   }
}



/**
 * Test if the given function body has an "early return".  That is, there's
 * a 'return' statement that's not the very last instruction in the body.
 */
static GLboolean
has_early_return(const slang_operation *funcBody)
{
   GLuint retCount = _slang_count_node_type(funcBody, SLANG_OPER_RETURN);
   if (retCount == 0)
      return GL_FALSE;
   else if (retCount == 1 && _slang_is_tail_return(funcBody))
      return GL_FALSE;
   else
      return GL_TRUE;
}


/**
 * Emit IR code for a function call.  This does one of two things:
 * 1. Inline the function's code
 * 2. Create an IR for the function's body and create a real call to it.
 */
static slang_ir_node *
_slang_gen_function_call(slang_assemble_ctx *A, slang_function *fun,
                         slang_operation *oper, slang_operation *dest)
{
   slang_ir_node *n;
   slang_operation *instance;
   slang_label *prevFuncEndLabel;
   char name[200];

   prevFuncEndLabel = A->curFuncEndLabel;
   sprintf(name, "__endOfFunc_%s_", (char *) fun->header.a_name);
   A->curFuncEndLabel = _slang_label_new(name);
   assert(A->curFuncEndLabel);

   /*
    * 'instance' is basically a copy of the function's body with various
    * transformations.
    */

   if (slang_is_asm_function(fun) && !dest) {
      /* assemble assembly function - tree style */
      instance = slang_inline_asm_function(A, fun, oper);
   }
   else {
      /* non-assembly function */
      /* We always generate an "inline-able" block of code here.
       * We may either:
       *  1. insert the inline code
       *  2. Generate a call to the "inline" code as a subroutine
       */
      const GLboolean earlyReturn = has_early_return(fun->body);

      if (earlyReturn && !A->EmitContReturn) {
         A->UseReturnFlag = GL_TRUE;
      }

      instance = slang_inline_function_call(A, fun, oper, dest);
      if (!instance)
         return NULL;

      if (earlyReturn) {
         /* The function we're calling has one or more 'return' statements
          * that prevent us from inlining the function's code.
          *
          * In this case, change the function's body type from
          * SLANG_OPER_BLOCK_NEW_SCOPE to SLANG_OPER_NON_INLINED_CALL.
          * During code emit this will result in a true subroutine call.
          *
          * Also, convert SLANG_OPER_RETURN_INLINED nodes to SLANG_OPER_RETURN.
          */
         slang_operation *callOper;

         assert(instance->type == SLANG_OPER_BLOCK_NEW_SCOPE ||
                instance->type == SLANG_OPER_SEQUENCE);

         if (_slang_function_has_return_value(fun) && !dest) {
            assert(instance->children[0].type == SLANG_OPER_VARIABLE_DECL);
            assert(instance->children[2].type == SLANG_OPER_IDENTIFIER);
            callOper = &instance->children[1];
         }
         else {
            callOper = instance;
         }

         if (A->UseReturnFlag) {
            /* Early returns not supported.  Create a _returnFlag variable
             * that's set upon 'return' and tested elsewhere to no-op any
             * remaining instructions in the subroutine.
             */
            assert(callOper->type == SLANG_OPER_BLOCK_NEW_SCOPE ||
                   callOper->type == SLANG_OPER_SEQUENCE);
            declare_return_flag(A, callOper);
         }
         else {
            /* We can emit real 'return' statements.  If we generated any
             * 'inline return' statements during function instantiation,
             * change them back to regular 'return' statements.
             */
            replace_node_type(instance, SLANG_OPER_RETURN_INLINED,
                              SLANG_OPER_RETURN);
         }

         callOper->type = SLANG_OPER_NON_INLINED_CALL;
         callOper->fun = fun;
         callOper->label = _slang_label_new_unique((char*) fun->header.a_name);
      }
      else {
         /* If there are any 'return' statements remaining, they're at the
          * very end of the function and can effectively become no-ops.
          */
         replace_node_type(instance, SLANG_OPER_RETURN_INLINED,
                           SLANG_OPER_VOID);
      }
   }

   if (!instance)
      return NULL;

   /* Replace the function call with the instance block (or new CALL stmt) */
   slang_operation_destruct(oper);
   *oper = *instance;
   _slang_free(instance);

#if 0
   assert(instance->locals);
   printf("*** Inlined code for call to %s:\n", (char*) fun->header.a_name);
   slang_print_tree(oper, 10);
   printf("\n");
#endif

   n = _slang_gen_operation(A, oper);

   /*_slang_label_delete(A->curFuncEndLabel);*/
   A->curFuncEndLabel = prevFuncEndLabel;

   if (A->pragmas->Debug) {
      char s[1000];
      _mesa_snprintf(s, sizeof(s), "Call/inline %s()", (char *) fun->header.a_name);
      n->Comment = _slang_strdup(s);
   }

   A->UseReturnFlag = GL_FALSE;

   return n;
}


static slang_asm_info *
slang_find_asm_info(const char *name)
{
   GLuint i;
   for (i = 0; AsmInfo[i].Name; i++) {
      if (strcmp(AsmInfo[i].Name, name) == 0) {
         return AsmInfo + i;
      }
   }
   return NULL;
}


/**
 * Some write-masked assignments are simple, but others are hard.
 * Simple example:
 *    vec3 v;
 *    v.xy = vec2(a, b);
 * Hard example:
 *    vec3 v;
 *    v.zy = vec2(a, b);
 * this gets transformed/swizzled into:
 *    v.zy = vec2(a, b).*yx*         (* = don't care)
 * This function helps to determine simple vs. non-simple.
 */
static GLboolean
_slang_simple_writemask(GLuint writemask, GLuint swizzle)
{
   switch (writemask) {
   case WRITEMASK_X:
      return GET_SWZ(swizzle, 0) == SWIZZLE_X;
   case WRITEMASK_Y:
      return GET_SWZ(swizzle, 1) == SWIZZLE_Y;
   case WRITEMASK_Z:
      return GET_SWZ(swizzle, 2) == SWIZZLE_Z;
   case WRITEMASK_W:
      return GET_SWZ(swizzle, 3) == SWIZZLE_W;
   case WRITEMASK_XY:
      return (GET_SWZ(swizzle, 0) == SWIZZLE_X)
         && (GET_SWZ(swizzle, 1) == SWIZZLE_Y);
   case WRITEMASK_XYZ:
      return (GET_SWZ(swizzle, 0) == SWIZZLE_X)
         && (GET_SWZ(swizzle, 1) == SWIZZLE_Y)
         && (GET_SWZ(swizzle, 2) == SWIZZLE_Z);
   case WRITEMASK_XYZW:
      return swizzle == SWIZZLE_NOOP;
   default:
      return GL_FALSE;
   }
}


/**
 * Convert the given swizzle into a writemask.  In some cases this
 * is trivial, in other cases, we'll need to also swizzle the right
 * hand side to put components in the right places.
 * See comment above for more info.
 * XXX this function could be simplified and should probably be renamed.
 * \param swizzle  the incoming swizzle
 * \param writemaskOut  returns the writemask
 * \param swizzleOut  swizzle to apply to the right-hand-side
 * \return GL_FALSE for simple writemasks, GL_TRUE for non-simple
 */
static GLboolean
swizzle_to_writemask(slang_assemble_ctx *A, GLuint swizzle,
                     GLuint *writemaskOut, GLuint *swizzleOut)
{
   GLuint mask = 0x0, newSwizzle[4];
   GLint i, size;

   /* make new dst writemask, compute size */
   for (i = 0; i < 4; i++) {
      const GLuint swz = GET_SWZ(swizzle, i);
      if (swz == SWIZZLE_NIL) {
         /* end */
         break;
      }
      assert(swz <= 3);

      if (swizzle != SWIZZLE_XXXX &&
          swizzle != SWIZZLE_YYYY &&
          swizzle != SWIZZLE_ZZZZ &&
          swizzle != SWIZZLE_WWWW &&
          (mask & (1 << swz))) {
         /* a channel can't be specified twice (ex: ".xyyz") */
         slang_info_log_error(A->log, "Invalid writemask '%s'",
                              _mesa_swizzle_string(swizzle, 0, 0));
         return GL_FALSE;
      }

      mask |= (1 << swz);
   }
   assert(mask <= 0xf);
   size = i;  /* number of components in mask/swizzle */

   *writemaskOut = mask;

   /* make new src swizzle, by inversion */
   for (i = 0; i < 4; i++) {
      newSwizzle[i] = i; /*identity*/
   }
   for (i = 0; i < size; i++) {
      const GLuint swz = GET_SWZ(swizzle, i);
      newSwizzle[swz] = i;
   }
   *swizzleOut = MAKE_SWIZZLE4(newSwizzle[0],
                               newSwizzle[1],
                               newSwizzle[2],
                               newSwizzle[3]);

   if (_slang_simple_writemask(mask, *swizzleOut)) {
      if (size >= 1)
         assert(GET_SWZ(*swizzleOut, 0) == SWIZZLE_X);
      if (size >= 2)
         assert(GET_SWZ(*swizzleOut, 1) == SWIZZLE_Y);
      if (size >= 3)
         assert(GET_SWZ(*swizzleOut, 2) == SWIZZLE_Z);
      if (size >= 4)
         assert(GET_SWZ(*swizzleOut, 3) == SWIZZLE_W);
      return GL_TRUE;
   }
   else
      return GL_FALSE;
}


#if 0 /* not used, but don't remove just yet */
/**
 * Recursively traverse 'oper' to produce a swizzle mask in the event
 * of any vector subscripts and swizzle suffixes.
 * Ex:  for "vec4 v",  "v[2].x" resolves to v.z
 */
static GLuint
resolve_swizzle(const slang_operation *oper)
{
   if (oper->type == SLANG_OPER_FIELD) {
      /* writemask from .xyzw suffix */
      slang_swizzle swz;
      if (_slang_is_swizzle((char*) oper->a_id, 4, &swz)) {
         GLuint swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                                        swz.swizzle[1],
                                        swz.swizzle[2],
                                        swz.swizzle[3]);
         GLuint child_swizzle = resolve_swizzle(&oper->children[0]);
         GLuint s = _slang_swizzle_swizzle(child_swizzle, swizzle);
         return s;
      }
      else
         return SWIZZLE_XYZW;
   }
   else if (oper->type == SLANG_OPER_SUBSCRIPT &&
            oper->children[1].type == SLANG_OPER_LITERAL_INT) {
      /* writemask from [index] */
      GLuint child_swizzle = resolve_swizzle(&oper->children[0]);
      GLuint i = (GLuint) oper->children[1].literal[0];
      GLuint swizzle;
      GLuint s;
      switch (i) {
      case 0:
         swizzle = SWIZZLE_XXXX;
         break;
      case 1:
         swizzle = SWIZZLE_YYYY;
         break;
      case 2:
         swizzle = SWIZZLE_ZZZZ;
         break;
      case 3:
         swizzle = SWIZZLE_WWWW;
         break;
      default:
         swizzle = SWIZZLE_XYZW;
      }
      s = _slang_swizzle_swizzle(child_swizzle, swizzle);
      return s;
   }
   else {
      return SWIZZLE_XYZW;
   }
}
#endif


#if 0
/**
 * Recursively descend through swizzle nodes to find the node's storage info.
 */
static slang_ir_storage *
get_store(const slang_ir_node *n)
{
   if (n->Opcode == IR_SWIZZLE) {
      return get_store(n->Children[0]);
   }
   return n->Store;
}
#endif


/**
 * Generate IR tree for an asm instruction/operation such as:
 *    __asm vec4_dot __retVal.x, v1, v2;
 */
static slang_ir_node *
_slang_gen_asm(slang_assemble_ctx *A, slang_operation *oper,
               slang_operation *dest)
{
   const slang_asm_info *info;
   slang_ir_node *kids[3], *n;
   GLuint j, firstOperand;

   assert(oper->type == SLANG_OPER_ASM);

   info = slang_find_asm_info((char *) oper->a_id);
   if (!info) {
      _mesa_problem(NULL, "undefined __asm function %s\n",
                    (char *) oper->a_id);
      assert(info);
      return NULL;
   }
   assert(info->NumParams <= 3);

   if (info->NumParams == oper->num_children) {
      /* Storage for result is not specified.
       * Children[0], [1], [2] are the operands.
       */
      firstOperand = 0;
   }
   else {
      /* Storage for result (child[0]) is specified.
       * Children[1], [2], [3] are the operands.
       */
      firstOperand = 1;
   }

   /* assemble child(ren) */
   kids[0] = kids[1] = kids[2] = NULL;
   for (j = 0; j < info->NumParams; j++) {
      kids[j] = _slang_gen_operation(A, &oper->children[firstOperand + j]);
      if (!kids[j])
         return NULL;
   }

   n = new_node3(info->Opcode, kids[0], kids[1], kids[2]);

   if (firstOperand) {
      /* Setup n->Store to be a particular location.  Otherwise, storage
       * for the result (a temporary) will be allocated later.
       */
      slang_operation *dest_oper;
      slang_ir_node *n0;

      dest_oper = &oper->children[0];

      n0 = _slang_gen_operation(A, dest_oper);
      if (!n0)
         return NULL;

      assert(!n->Store);
      n->Store = n0->Store;

      assert(n->Store->File != PROGRAM_UNDEFINED || n->Store->Parent);

      _slang_free(n0);
   }

   return n;
}


#if 0
static void
print_funcs(struct slang_function_scope_ *scope, const char *name)
{
   GLuint i;
   for (i = 0; i < scope->num_functions; i++) {
      slang_function *f = &scope->functions[i];
      if (!name || strcmp(name, (char*) f->header.a_name) == 0)
          printf("  %s (%d args)\n", name, f->param_count);

   }
   if (scope->outer_scope)
      print_funcs(scope->outer_scope, name);
}
#endif


/**
 * Find a function of the given name, taking 'numArgs' arguments.
 * This is the function we'll try to call when there is no exact match
 * between function parameters and call arguments.
 *
 * XXX we should really create a list of candidate functions and try
 * all of them...
 */
static slang_function *
_slang_find_function_by_argc(slang_function_scope *scope,
                             const char *name, int numArgs)
{
   while (scope) {
      GLuint i;
      for (i = 0; i < scope->num_functions; i++) {
         slang_function *f = &scope->functions[i];
         if (strcmp(name, (char*) f->header.a_name) == 0) {
            int haveRetValue = _slang_function_has_return_value(f);
            if (numArgs == f->param_count - haveRetValue)
               return f;
         }
      }
      scope = scope->outer_scope;
   }

   return NULL;
}


static slang_function *
_slang_find_function_by_max_argc(slang_function_scope *scope,
                                 const char *name)
{
   slang_function *maxFunc = NULL;
   GLuint maxArgs = 0;

   while (scope) {
      GLuint i;
      for (i = 0; i < scope->num_functions; i++) {
         slang_function *f = &scope->functions[i];
         if (strcmp(name, (char*) f->header.a_name) == 0) {
            if (f->param_count > maxArgs) {
               maxArgs = f->param_count;
               maxFunc = f;
            }
         }
      }
      scope = scope->outer_scope;
   }

   return maxFunc;
}


/**
 * Generate a new slang_function which is a constructor for a user-defined
 * struct type.
 */
static slang_function *
_slang_make_struct_constructor(slang_assemble_ctx *A, slang_struct *str)
{
   const GLint numFields = str->fields->num_variables;
   slang_function *fun = slang_function_new(SLANG_FUNC_CONSTRUCTOR);

   /* function header (name, return type) */
   fun->header.a_name = str->a_name;
   fun->header.type.qualifier = SLANG_QUAL_NONE;
   fun->header.type.specifier.type = SLANG_SPEC_STRUCT;
   fun->header.type.specifier._struct = str;

   /* function parameters (= struct's fields) */
   {
      GLint i;
      for (i = 0; i < numFields; i++) {
         /*
         printf("Field %d: %s\n", i, (char*) str->fields->variables[i]->a_name);
         */
         slang_variable *p = slang_variable_scope_grow(fun->parameters);
         *p = *str->fields->variables[i]; /* copy the variable and type */
         p->type.qualifier = SLANG_QUAL_CONST;
      }
      fun->param_count = fun->parameters->num_variables;
   }

   /* Add __retVal to params */
   {
      slang_variable *p = slang_variable_scope_grow(fun->parameters);
      slang_atom a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);
      p->a_name = a_retVal;
      p->type = fun->header.type;
      p->type.qualifier = SLANG_QUAL_OUT;
      fun->param_count++;
   }

   /* function body is:
    *    block:
    *       declare T;
    *       T.f1 = p1;
    *       T.f2 = p2;
    *       ...
    *       T.fn = pn;
    *       return T;
    */
   {
      slang_variable_scope *scope;
      slang_variable *var;
      GLint i;

      fun->body = slang_operation_new(1);
      fun->body->type = SLANG_OPER_BLOCK_NEW_SCOPE;
      fun->body->num_children = numFields + 2;
      fun->body->children = slang_operation_new(numFields + 2);

      scope = fun->body->locals;
      scope->outer_scope = fun->parameters;

      /* create local var 't' */
      var = slang_variable_scope_grow(scope);
      var->a_name = slang_atom_pool_atom(A->atoms, "t");
      var->type = fun->header.type;

      /* declare t */
      {
         slang_operation *decl;

         decl = &fun->body->children[0];
         decl->type = SLANG_OPER_VARIABLE_DECL;
         decl->locals = _slang_variable_scope_new(scope);
         decl->a_id = var->a_name;
      }

      /* assign params to fields of t */
      for (i = 0; i < numFields; i++) {
         slang_operation *assign = &fun->body->children[1 + i];

         assign->type = SLANG_OPER_ASSIGN;
         assign->locals = _slang_variable_scope_new(scope);
         assign->num_children = 2;
         assign->children = slang_operation_new(2);
         
         {
            slang_operation *lhs = &assign->children[0];

            lhs->type = SLANG_OPER_FIELD;
            lhs->locals = _slang_variable_scope_new(scope);
            lhs->num_children = 1;
            lhs->children = slang_operation_new(1);
            lhs->a_id = str->fields->variables[i]->a_name;

            lhs->children[0].type = SLANG_OPER_IDENTIFIER;
            lhs->children[0].a_id = var->a_name;
            lhs->children[0].locals = _slang_variable_scope_new(scope);

#if 0
            lhs->children[1].num_children = 1;
            lhs->children[1].children = slang_operation_new(1);
            lhs->children[1].children[0].type = SLANG_OPER_IDENTIFIER;
            lhs->children[1].children[0].a_id = str->fields->variables[i]->a_name;
            lhs->children[1].children->locals = _slang_variable_scope_new(scope);
#endif
         }

         {
            slang_operation *rhs = &assign->children[1];

            rhs->type = SLANG_OPER_IDENTIFIER;
            rhs->locals = _slang_variable_scope_new(scope);
            rhs->a_id = str->fields->variables[i]->a_name;
         }         
      }

      /* return t; */
      {
         slang_operation *ret = &fun->body->children[numFields + 1];

         ret->type = SLANG_OPER_RETURN;
         ret->locals = _slang_variable_scope_new(scope);
         ret->num_children = 1;
         ret->children = slang_operation_new(1);
         ret->children[0].type = SLANG_OPER_IDENTIFIER;
         ret->children[0].a_id = var->a_name;
         ret->children[0].locals = _slang_variable_scope_new(scope);
      }
   }
   /*
   slang_print_function(fun, 1);
   */
   return fun;
}


/**
 * Find/create a function (constructor) for the given structure name.
 */
static slang_function *
_slang_locate_struct_constructor(slang_assemble_ctx *A, const char *name)
{
   unsigned int i;
   for (i = 0; i < A->space.structs->num_structs; i++) {
      slang_struct *str = &A->space.structs->structs[i];
      if (strcmp(name, (const char *) str->a_name) == 0) {
         /* found a structure type that matches the function name */
         if (!str->constructor) {
            /* create the constructor function now */
            str->constructor = _slang_make_struct_constructor(A, str);
         }
         return str->constructor;
      }
   }
   return NULL;
}


/**
 * Generate a new slang_function to satisfy a call to an array constructor.
 * Ex:  float[3](1., 2., 3.)
 */
static slang_function *
_slang_make_array_constructor(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_type_specifier_type baseType;
   slang_function *fun;
   int num_elements;

   fun = slang_function_new(SLANG_FUNC_CONSTRUCTOR);
   if (!fun)
      return NULL;

   baseType = slang_type_specifier_type_from_string((char *) oper->a_id);

   num_elements = oper->num_children;

   /* function header, return type */
   {
      fun->header.a_name = oper->a_id;
      fun->header.type.qualifier = SLANG_QUAL_NONE;
      fun->header.type.specifier.type = SLANG_SPEC_ARRAY;
      fun->header.type.specifier._array =
         slang_type_specifier_new(baseType, NULL, NULL);
      fun->header.type.array_len = num_elements;
   }

   /* function parameters (= number of elements) */
   {
      GLint i;
      for (i = 0; i < num_elements; i++) {
         /*
         printf("Field %d: %s\n", i, (char*) str->fields->variables[i]->a_name);
         */
         slang_variable *p = slang_variable_scope_grow(fun->parameters);
         char name[10];
         _mesa_snprintf(name, sizeof(name), "p%d", i);
         p->a_name = slang_atom_pool_atom(A->atoms, name);
         p->type.qualifier = SLANG_QUAL_CONST;
         p->type.specifier.type = baseType;
      }
      fun->param_count = fun->parameters->num_variables;
   }

   /* Add __retVal to params */
   {
      slang_variable *p = slang_variable_scope_grow(fun->parameters);
      slang_atom a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);
      p->a_name = a_retVal;
      p->type = fun->header.type;
      p->type.qualifier = SLANG_QUAL_OUT;
      p->type.specifier.type = baseType;
      fun->param_count++;
   }

   /* function body is:
    *    block:
    *       declare T;
    *       T[0] = p0;
    *       T[1] = p1;
    *       ...
    *       T[n] = pn;
    *       return T;
    */
   {
      slang_variable_scope *scope;
      slang_variable *var;
      GLint i;

      fun->body = slang_operation_new(1);
      fun->body->type = SLANG_OPER_BLOCK_NEW_SCOPE;
      fun->body->num_children = num_elements + 2;
      fun->body->children = slang_operation_new(num_elements + 2);

      scope = fun->body->locals;
      scope->outer_scope = fun->parameters;

      /* create local var 't' */
      var = slang_variable_scope_grow(scope);
      var->a_name = slang_atom_pool_atom(A->atoms, "ttt");
      var->type = fun->header.type;/*XXX copy*/

      /* declare t */
      {
         slang_operation *decl;

         decl = &fun->body->children[0];
         decl->type = SLANG_OPER_VARIABLE_DECL;
         decl->locals = _slang_variable_scope_new(scope);
         decl->a_id = var->a_name;
      }

      /* assign params to elements of t */
      for (i = 0; i < num_elements; i++) {
         slang_operation *assign = &fun->body->children[1 + i];

         assign->type = SLANG_OPER_ASSIGN;
         assign->locals = _slang_variable_scope_new(scope);
         assign->num_children = 2;
         assign->children = slang_operation_new(2);
         
         {
            slang_operation *lhs = &assign->children[0];

            lhs->type = SLANG_OPER_SUBSCRIPT;
            lhs->locals = _slang_variable_scope_new(scope);
            lhs->num_children = 2;
            lhs->children = slang_operation_new(2);
 
            lhs->children[0].type = SLANG_OPER_IDENTIFIER;
            lhs->children[0].a_id = var->a_name;
            lhs->children[0].locals = _slang_variable_scope_new(scope);

            lhs->children[1].type = SLANG_OPER_LITERAL_INT;
            lhs->children[1].literal[0] = (GLfloat) i;
         }

         {
            slang_operation *rhs = &assign->children[1];

            rhs->type = SLANG_OPER_IDENTIFIER;
            rhs->locals = _slang_variable_scope_new(scope);
            rhs->a_id = fun->parameters->variables[i]->a_name;
         }         
      }

      /* return t; */
      {
         slang_operation *ret = &fun->body->children[num_elements + 1];

         ret->type = SLANG_OPER_RETURN;
         ret->locals = _slang_variable_scope_new(scope);
         ret->num_children = 1;
         ret->children = slang_operation_new(1);
         ret->children[0].type = SLANG_OPER_IDENTIFIER;
         ret->children[0].a_id = var->a_name;
         ret->children[0].locals = _slang_variable_scope_new(scope);
      }
   }

   /*
   slang_print_function(fun, 1);
   */

   return fun;
}


static GLboolean
_slang_is_vec_mat_type(const char *name)
{
   static const char *vecmat_types[] = {
      "float", "int", "bool",
      "vec2", "vec3", "vec4",
      "ivec2", "ivec3", "ivec4",
      "bvec2", "bvec3", "bvec4",
      "mat2", "mat3", "mat4",
      "mat2x3", "mat2x4", "mat3x2", "mat3x4", "mat4x2", "mat4x3",
      NULL
   };
   int i;
   for (i = 0; vecmat_types[i]; i++)
      if (strcmp(name, vecmat_types[i]) == 0)
         return GL_TRUE;
   return GL_FALSE;
}


/**
 * Assemble a function call, given a particular function name.
 * \param name  the function's name (operators like '*' are possible).
 */
static slang_ir_node *
_slang_gen_function_call_name(slang_assemble_ctx *A, const char *name,
                              slang_operation *oper, slang_operation *dest)
{
   slang_operation *params = oper->children;
   const GLuint param_count = oper->num_children;
   slang_atom atom;
   slang_function *fun;
   slang_ir_node *n;

   atom = slang_atom_pool_atom(A->atoms, name);
   if (atom == SLANG_ATOM_NULL)
      return NULL;

   if (oper->array_constructor) {
      /* this needs special handling */
      fun = _slang_make_array_constructor(A, oper);
   }
   else {
      /* Try to find function by name and exact argument type matching */
      GLboolean error = GL_FALSE;
      fun = _slang_function_locate(A->space.funcs, atom, params, param_count,
                                   &A->space, A->atoms, A->log, &error);
      if (error) {
         slang_info_log_error(A->log,
                              "Function '%s' not found (check argument types)",
                              name);
         return NULL;
      }
   }

   if (!fun) {
      /* Next, try locating a constructor function for a user-defined type */
      fun = _slang_locate_struct_constructor(A, name);
   }

   /*
    * At this point, some heuristics are used to try to find a function
    * that matches the calling signature by means of casting or "unrolling"
    * of constructors.
    */

   if (!fun && _slang_is_vec_mat_type(name)) {
      /* Next, if this call looks like a vec() or mat() constructor call,
       * try "unwinding" the args to satisfy a constructor.
       */
      fun = _slang_find_function_by_max_argc(A->space.funcs, name);
      if (fun) {
         if (!_slang_adapt_call(oper, fun, &A->space, A->atoms, A->log)) {
            slang_info_log_error(A->log,
                                 "Function '%s' not found (check argument types)",
                                 name);
            return NULL;
         }
      }
   }

   if (!fun && _slang_is_vec_mat_type(name)) {
      /* Next, try casting args to the types of the formal parameters */
      int numArgs = oper->num_children;
      fun = _slang_find_function_by_argc(A->space.funcs, name, numArgs);
      if (!fun || !_slang_cast_func_params(oper, fun, &A->space, A->atoms, A->log)) {
         slang_info_log_error(A->log,
                              "Function '%s' not found (check argument types)",
                              name);
         return NULL;
      }
      assert(fun);
   }

   if (!fun) {
      slang_info_log_error(A->log,
                           "Function '%s' not found (check argument types)",
                           name);
      return NULL;
   }

   if (!fun->body) {
      /* The function body may be in another compilation unit.
       * We'll try concatenating the shaders and recompile at link time.
       */
      A->UnresolvedRefs = GL_TRUE;
      return new_node1(IR_NOP, NULL);
   }

   /* type checking to be sure function's return type matches 'dest' type */
   if (dest) {
      slang_typeinfo t0;

      slang_typeinfo_construct(&t0);
      typeof_operation(A, dest, &t0);

      if (!slang_type_specifier_equal(&t0.spec, &fun->header.type.specifier)) {
         slang_info_log_error(A->log,
                              "Incompatible type returned by call to '%s'",
                              name);
         return NULL;
      }
   }

   n = _slang_gen_function_call(A, fun, oper, dest);

   if (n && !n->Store && !dest
       && fun->header.type.specifier.type != SLANG_SPEC_VOID) {
      /* setup n->Store for the result of the function call */
      GLint size = _slang_sizeof_type_specifier(&fun->header.type.specifier);
      n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, size);
      /*printf("Alloc storage for function result, size %d \n", size);*/
   }

   if (oper->array_constructor) {
      /* free the temporary array constructor function now */
      slang_function_destruct(fun);
   }

   return n;
}


static slang_ir_node *
_slang_gen_method_call(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_atom *a_length = slang_atom_pool_atom(A->atoms, "length");
   slang_ir_node *n;
   slang_variable *var;

   /* NOTE: In GLSL 1.20, there's only one kind of method
    * call: array.length().  Anything else is an error.
    */
   if (oper->a_id != a_length) {
      slang_info_log_error(A->log,
                           "Undefined method call '%s'", (char *) oper->a_id);
      return NULL;
   }

   /* length() takes no arguments */
   if (oper->num_children > 0) {
      slang_info_log_error(A->log, "Invalid arguments to length() method");
      return NULL;
   }

   /* lookup the object/variable */
   var = _slang_variable_locate(oper->locals, oper->a_obj, GL_TRUE);
   if (!var || var->type.specifier.type != SLANG_SPEC_ARRAY) {
      slang_info_log_error(A->log,
                           "Undefined object '%s'", (char *) oper->a_obj);
      return NULL;
   }

   /* Create a float/literal IR node encoding the array length */
   n = new_node0(IR_FLOAT);
   if (n) {
      n->Value[0] = (float) _slang_array_length(var);
      n->Store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, 1);
   }
   return n;
}


static GLboolean
_slang_is_constant_cond(const slang_operation *oper, GLboolean *value)
{
   if (oper->type == SLANG_OPER_LITERAL_FLOAT ||
       oper->type == SLANG_OPER_LITERAL_INT ||
       oper->type == SLANG_OPER_LITERAL_BOOL) {
      if (oper->literal[0])
         *value = GL_TRUE;
      else
         *value = GL_FALSE;
      return GL_TRUE;
   }
   else if (oper->type == SLANG_OPER_EXPRESSION &&
            oper->num_children == 1) {
      return _slang_is_constant_cond(&oper->children[0], value);
   }
   return GL_FALSE;
}


/**
 * Test if an operation is a scalar or boolean.
 */
static GLboolean
_slang_is_scalar_or_boolean(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_typeinfo type;
   GLint size;

   slang_typeinfo_construct(&type);
   typeof_operation(A, oper, &type);
   size = _slang_sizeof_type_specifier(&type.spec);
   slang_typeinfo_destruct(&type);
   return size == 1;
}


/**
 * Test if an operation is boolean.
 */
static GLboolean
_slang_is_boolean(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_typeinfo type;
   GLboolean isBool;

   slang_typeinfo_construct(&type);
   typeof_operation(A, oper, &type);
   isBool = (type.spec.type == SLANG_SPEC_BOOL);
   slang_typeinfo_destruct(&type);
   return isBool;
}


/**
 * Check if a loop contains a 'continue' statement.
 * Stop looking if we find a nested loop.
 */
static GLboolean
_slang_loop_contains_continue(const slang_operation *oper)
{
   switch (oper->type) {
   case SLANG_OPER_CONTINUE:
      return GL_TRUE;
   case SLANG_OPER_FOR:
   case SLANG_OPER_DO:
   case SLANG_OPER_WHILE:
      /* stop upon finding a nested loop */
      return GL_FALSE;
   default:
       /* recurse */
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            const slang_operation *child = slang_oper_child_const(oper, i);
            if (_slang_loop_contains_continue(child))
               return GL_TRUE;
         }
      }
      return GL_FALSE;
   }
}


/**
 * Check if a loop contains a 'continue' or 'break' statement.
 * Stop looking if we find a nested loop.
 */
static GLboolean
_slang_loop_contains_continue_or_break(const slang_operation *oper)
{
   switch (oper->type) {
   case SLANG_OPER_CONTINUE:
   case SLANG_OPER_BREAK:
      return GL_TRUE;
   case SLANG_OPER_FOR:
   case SLANG_OPER_DO:
   case SLANG_OPER_WHILE:
      /* stop upon finding a nested loop */
      return GL_FALSE;
   default:
       /* recurse */
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            const slang_operation *child = slang_oper_child_const(oper, i);
            if (_slang_loop_contains_continue_or_break(child))
               return GL_TRUE;
         }
      }
      return GL_FALSE;
   }
}


/**
 * Replace 'break' and 'continue' statements inside a do and while loops.
 * This is a recursive helper function used by
 * _slang_gen_do/while_without_continue().
 */
static void
replace_break_and_cont(slang_assemble_ctx *A, slang_operation *oper)
{
   switch (oper->type) {
   case SLANG_OPER_BREAK:
      /* replace 'break' with "_notBreakFlag = false; break" */
      {
         slang_operation *block = oper;
         block->type = SLANG_OPER_BLOCK_NEW_SCOPE;
         slang_operation_add_children(block, 2);
         {
            slang_operation *assign = slang_oper_child(block, 0);
            assign->type = SLANG_OPER_ASSIGN;
            slang_operation_add_children(assign, 2);
            {
               slang_operation *lhs = slang_oper_child(assign, 0);
               slang_operation_identifier(lhs, A, "_notBreakFlag");
            }
            {
               slang_operation *rhs = slang_oper_child(assign, 1);
               slang_operation_literal_bool(rhs, GL_FALSE);
            }
         }
         {
            slang_operation *brk = slang_oper_child(block, 1);
            brk->type = SLANG_OPER_BREAK;
            assert(!brk->children);
         }
      }
      break;
   case SLANG_OPER_CONTINUE:
      /* convert continue into a break */
      oper->type = SLANG_OPER_BREAK;
      break;
   case SLANG_OPER_FOR:
   case SLANG_OPER_DO:
   case SLANG_OPER_WHILE:
      /* stop upon finding a nested loop */
      break;
   default:
      /* recurse */
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            replace_break_and_cont(A, slang_oper_child(oper, i));
         }
      }
   }
}


/**
 * Transform a while-loop so that continue statements are converted to breaks.
 * Then do normal IR code generation.
 *
 * Before:
 * 
 * while (LOOPCOND) {
 *    A;
 *    if (IFCOND)
 *       continue;
 *    B;
 *    break;
 *    C;
 * }
 * 
 * After:
 * 
 * {
 *    bool _notBreakFlag = 1;
 *    while (_notBreakFlag && LOOPCOND) {
 *       do {
 *          A;
 *          if (IFCOND) {
 *             break;  // was continue
 *          }
 *          B;
 *          _notBreakFlag = 0; // was
 *          break;             // break
 *          C;
 *       } while (0)
 *    }
 * }
 */
static slang_ir_node *
_slang_gen_while_without_continue(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_operation *top;
   slang_operation *innerBody;

   assert(oper->type == SLANG_OPER_WHILE);

   top = slang_operation_new(1);
   top->type = SLANG_OPER_BLOCK_NEW_SCOPE;
   top->locals->outer_scope = oper->locals->outer_scope;
   slang_operation_add_children(top, 2);

   /* declare: bool _notBreakFlag = true */
   {
      slang_operation *condDecl = slang_oper_child(top, 0);
      slang_generate_declaration(A, top->locals, condDecl,
                                 SLANG_SPEC_BOOL, "_notBreakFlag", GL_TRUE);
   }

   /* build outer while-loop:  while (_notBreakFlag && LOOPCOND) { ... } */
   {
      slang_operation *outerWhile = slang_oper_child(top, 1);
      outerWhile->type = SLANG_OPER_WHILE;
      slang_operation_add_children(outerWhile, 2);

      /* _notBreakFlag && LOOPCOND */
      {
         slang_operation *cond = slang_oper_child(outerWhile, 0);
         cond->type = SLANG_OPER_LOGICALAND;
         slang_operation_add_children(cond, 2);
         {
            slang_operation *notBreak = slang_oper_child(cond, 0);
            slang_operation_identifier(notBreak, A, "_notBreakFlag");
         }
         {
            slang_operation *origCond = slang_oper_child(cond, 1);
            slang_operation_copy(origCond, slang_oper_child(oper, 0));
         }
      }

      /* inner loop */
      {
         slang_operation *innerDo = slang_oper_child(outerWhile, 1);
         innerDo->type = SLANG_OPER_DO;
         slang_operation_add_children(innerDo, 2);

         /* copy original do-loop body into inner do-loop's body */
         innerBody = slang_oper_child(innerDo, 0);
         slang_operation_copy(innerBody, slang_oper_child(oper, 1));
         innerBody->locals->outer_scope = innerDo->locals;

         /* inner do-loop's condition is constant/false */
         {
            slang_operation *constFalse = slang_oper_child(innerDo, 1);
            slang_operation_literal_bool(constFalse, GL_FALSE);
         }
      }
   }

   /* Finally, in innerBody,
    *   replace "break" with "_notBreakFlag = 0; break"
    *   replace "continue" with "break"
    */
   replace_break_and_cont(A, innerBody);

   /*slang_print_tree(top, 0);*/

   return _slang_gen_operation(A, top);

   return NULL;
}


/**
 * Generate loop code using high-level IR_LOOP instruction
 */
static slang_ir_node *
_slang_gen_while(slang_assemble_ctx * A, slang_operation *oper)
{
   /*
    * LOOP:
    *    BREAK if !expr (child[0])
    *    body code (child[1])
    */
   slang_ir_node *loop, *breakIf, *body;
   GLboolean isConst, constTrue = GL_FALSE;

   if (!A->EmitContReturn) {
      /* We don't want to emit CONT instructions.  If this while-loop has
       * a continue, translate it away.
       */
      if (_slang_loop_contains_continue(slang_oper_child(oper, 1))) {
         return _slang_gen_while_without_continue(A, oper);
      }
   }

   /* type-check expression */
   if (!_slang_is_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log, "scalar/boolean expression expected for 'while'");
      return NULL;
   }

   /* Check if loop condition is a constant */
   isConst = _slang_is_constant_cond(&oper->children[0], &constTrue);

   if (isConst && !constTrue) {
      /* loop is never executed! */
      return new_node0(IR_NOP);
   }

   /* Begin new loop */
   loop = new_loop(NULL);

   /* save loop state */
   push_loop(A, oper, loop);

   if (isConst && constTrue) {
      /* while(nonzero constant), no conditional break */
      breakIf = NULL;
   }
   else {
      slang_ir_node *cond
         = new_cond(new_not(_slang_gen_operation(A, &oper->children[0])));
      breakIf = new_break_if_true(A, cond);
   }
   body = _slang_gen_operation(A, &oper->children[1]);
   loop->Children[0] = new_seq(breakIf, body);

   /* Do infinite loop detection */
   /* loop->List is head of linked list of break/continue nodes */
   if (!loop->List && isConst && constTrue) {
      /* infinite loop detected */
      pop_loop(A);
      slang_info_log_error(A->log, "Infinite loop detected!");
      return NULL;
   }

   /* restore loop state */
   pop_loop(A);

   return loop;
}


/**
 * Transform a do-while-loop so that continue statements are converted to breaks.
 * Then do normal IR code generation.
 *
 * Before:
 * 
 * do {
 *    A;
 *    if (IFCOND)
 *       continue;
 *    B;
 *    break;
 *    C;
 * } while (LOOPCOND);
 * 
 * After:
 * 
 * {
 *    bool _notBreakFlag = 1;
 *    do {
 *       do {
 *          A;
 *          if (IFCOND) {
 *             break;  // was continue
 *          }
 *          B;
 *          _notBreakFlag = 0; // was
 *          break;             // break
 *          C;
 *       } while (0)
 *    } while (_notBreakFlag && LOOPCOND);
 * }
 */
static slang_ir_node *
_slang_gen_do_without_continue(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_operation *top;
   slang_operation *innerBody;

   assert(oper->type == SLANG_OPER_DO);

   top = slang_operation_new(1);
   top->type = SLANG_OPER_BLOCK_NEW_SCOPE;
   top->locals->outer_scope = oper->locals->outer_scope;
   slang_operation_add_children(top, 2);

   /* declare: bool _notBreakFlag = true */
   {
      slang_operation *condDecl = slang_oper_child(top, 0);
      slang_generate_declaration(A, top->locals, condDecl,
                                 SLANG_SPEC_BOOL, "_notBreakFlag", GL_TRUE);
   }

   /* build outer do-loop:  do { ... } while (_notBreakFlag && LOOPCOND) */
   {
      slang_operation *outerDo = slang_oper_child(top, 1);
      outerDo->type = SLANG_OPER_DO;
      slang_operation_add_children(outerDo, 2);

      /* inner do-loop */
      {
         slang_operation *innerDo = slang_oper_child(outerDo, 0);
         innerDo->type = SLANG_OPER_DO;
         slang_operation_add_children(innerDo, 2);

         /* copy original do-loop body into inner do-loop's body */
         innerBody = slang_oper_child(innerDo, 0);
         slang_operation_copy(innerBody, slang_oper_child(oper, 0));
         innerBody->locals->outer_scope = innerDo->locals;

         /* inner do-loop's condition is constant/false */
         {
            slang_operation *constFalse = slang_oper_child(innerDo, 1);
            slang_operation_literal_bool(constFalse, GL_FALSE);
         }
      }

      /* _notBreakFlag && LOOPCOND */
      {
         slang_operation *cond = slang_oper_child(outerDo, 1);
         cond->type = SLANG_OPER_LOGICALAND;
         slang_operation_add_children(cond, 2);
         {
            slang_operation *notBreak = slang_oper_child(cond, 0);
            slang_operation_identifier(notBreak, A, "_notBreakFlag");
         }
         {
            slang_operation *origCond = slang_oper_child(cond, 1);
            slang_operation_copy(origCond, slang_oper_child(oper, 1));
         }
      }
   }

   /* Finally, in innerBody,
    *   replace "break" with "_notBreakFlag = 0; break"
    *   replace "continue" with "break"
    */
   replace_break_and_cont(A, innerBody);

   /*slang_print_tree(top, 0);*/

   return _slang_gen_operation(A, top);
}


/**
 * Generate IR tree for a do-while loop using high-level LOOP, IF instructions.
 */
static slang_ir_node *
_slang_gen_do(slang_assemble_ctx * A, slang_operation *oper)
{
   /*
    * LOOP:
    *    body code (child[0])
    *    tail code:
    *       BREAK if !expr (child[1])
    */
   slang_ir_node *loop;
   GLboolean isConst, constTrue;

   if (!A->EmitContReturn) {
      /* We don't want to emit CONT instructions.  If this do-loop has
       * a continue, translate it away.
       */
      if (_slang_loop_contains_continue(slang_oper_child(oper, 0))) {
         return _slang_gen_do_without_continue(A, oper);
      }
   }

   /* type-check expression */
   if (!_slang_is_boolean(A, &oper->children[1])) {
      slang_info_log_error(A->log, "scalar/boolean expression expected for 'do/while'");
      return NULL;
   }

   loop = new_loop(NULL);

   /* save loop state */
   push_loop(A, oper, loop);

   /* loop body: */
   loop->Children[0] = _slang_gen_operation(A, &oper->children[0]);

   /* Check if loop condition is a constant */
   isConst = _slang_is_constant_cond(&oper->children[1], &constTrue);
   if (isConst && constTrue) {
      /* do { } while(1)   ==> no conditional break */
      loop->Children[1] = NULL; /* no tail code */
   }
   else {
      slang_ir_node *cond
         = new_cond(new_not(_slang_gen_operation(A, &oper->children[1])));
      loop->Children[1] = new_break_if_true(A, cond);
   }

   /* XXX we should do infinite loop detection, as above */

   /* restore loop state */
   pop_loop(A);

   return loop;
}


/**
 * Recursively count the number of operations rooted at 'oper'.
 * This gives some kind of indication of the size/complexity of an operation.
 */
static GLuint
sizeof_operation(const slang_operation *oper)
{
   if (oper) {
      GLuint count = 1; /* me */
      GLuint i;
      for (i = 0; i < oper->num_children; i++) {
         count += sizeof_operation(&oper->children[i]);
      }
      return count;
   }
   else {
      return 0;
   }
}


/**
 * Determine if a for-loop can be unrolled.
 * At this time, only a rather narrow class of for loops can be unrolled.
 * See code for details.
 * When a loop can't be unrolled because it's too large we'll emit a
 * message to the log.
 */
static GLboolean
_slang_can_unroll_for_loop(slang_assemble_ctx * A, const slang_operation *oper)
{
   GLuint bodySize;
   GLint start, end;
   const char *varName;
   slang_atom varId;

   if (oper->type != SLANG_OPER_FOR)
      return GL_FALSE;

   assert(oper->num_children == 4);

   if (_slang_loop_contains_continue_or_break(slang_oper_child_const(oper, 3)))
      return GL_FALSE;

   /* children[0] must be either "int i=constant" or "i=constant" */
   if (oper->children[0].type == SLANG_OPER_BLOCK_NO_NEW_SCOPE) {
      slang_variable *var;

      if (oper->children[0].children[0].type != SLANG_OPER_VARIABLE_DECL)
         return GL_FALSE;

      varId = oper->children[0].children[0].a_id;

      var = _slang_variable_locate(oper->children[0].children[0].locals,
                                   varId, GL_TRUE);
      if (!var)
         return GL_FALSE;
      if (!var->initializer)
         return GL_FALSE;
      if (var->initializer->type != SLANG_OPER_LITERAL_INT)
         return GL_FALSE;
      start = (GLint) var->initializer->literal[0];
   }
   else if (oper->children[0].type == SLANG_OPER_EXPRESSION) {
      if (oper->children[0].children[0].type != SLANG_OPER_ASSIGN)
         return GL_FALSE;
      if (oper->children[0].children[0].children[0].type != SLANG_OPER_IDENTIFIER)
         return GL_FALSE;
      if (oper->children[0].children[0].children[1].type != SLANG_OPER_LITERAL_INT)
         return GL_FALSE;

      varId = oper->children[0].children[0].children[0].a_id;

      start = (GLint) oper->children[0].children[0].children[1].literal[0];
   }
   else {
      return GL_FALSE;
   }

   /* children[1] must be "i<constant" */
   if (oper->children[1].type != SLANG_OPER_EXPRESSION)
      return GL_FALSE;
   if (oper->children[1].children[0].type != SLANG_OPER_LESS)
      return GL_FALSE;
   if (oper->children[1].children[0].children[0].type != SLANG_OPER_IDENTIFIER)
      return GL_FALSE;
   if (oper->children[1].children[0].children[1].type != SLANG_OPER_LITERAL_INT)
      return GL_FALSE;

   end = (GLint) oper->children[1].children[0].children[1].literal[0];

   /* children[2] must be "i++" or "++i" */
   if (oper->children[2].type != SLANG_OPER_POSTINCREMENT &&
       oper->children[2].type != SLANG_OPER_PREINCREMENT)
      return GL_FALSE;
   if (oper->children[2].children[0].type != SLANG_OPER_IDENTIFIER)
      return GL_FALSE;

   /* make sure the same variable name is used in all places */
   if ((oper->children[1].children[0].children[0].a_id != varId) ||
       (oper->children[2].children[0].a_id != varId))
      return GL_FALSE;

   varName = (const char *) varId;

   /* children[3], the loop body, can't be too large */
   bodySize = sizeof_operation(&oper->children[3]);
   if (bodySize > MAX_FOR_LOOP_UNROLL_BODY_SIZE) {
      slang_info_log_print(A->log,
                           "Note: 'for (%s ... )' body is too large/complex"
                           " to unroll",
                           varName);
      return GL_FALSE;
   }

   if (start >= end)
      return GL_FALSE; /* degenerate case */

   if ((GLuint)(end - start) > MAX_FOR_LOOP_UNROLL_ITERATIONS) {
      slang_info_log_print(A->log,
                           "Note: 'for (%s=%d; %s<%d; ++%s)' is too"
                           " many iterations to unroll",
                           varName, start, varName, end, varName);
      return GL_FALSE;
   }

   if ((end - start) * bodySize > MAX_FOR_LOOP_UNROLL_COMPLEXITY) {
      slang_info_log_print(A->log,
                           "Note: 'for (%s=%d; %s<%d; ++%s)' will generate"
                           " too much code to unroll",
                           varName, start, varName, end, varName);
      return GL_FALSE;
   }

   return GL_TRUE; /* we can unroll the loop */
}


/**
 * Unroll a for-loop.
 * First we determine the number of iterations to unroll.
 * Then for each iteration:
 *   make a copy of the loop body
 *   replace instances of the loop variable with the current iteration value
 *   generate IR code for the body
 * \return pointer to generated IR code or NULL if error, out of memory, etc.
 */
static slang_ir_node *
_slang_unroll_for_loop(slang_assemble_ctx * A, const slang_operation *oper)
{
   GLint start, end, iter;
   slang_ir_node *n, *root = NULL;
   slang_atom varId;

   if (oper->children[0].type == SLANG_OPER_BLOCK_NO_NEW_SCOPE) {
      /* for (int i=0; ... */
      slang_variable *var;

      varId = oper->children[0].children[0].a_id;
      var = _slang_variable_locate(oper->children[0].children[0].locals,
                                   varId, GL_TRUE);
      assert(var);
      start = (GLint) var->initializer->literal[0];
   }
   else {
      /* for (i=0; ... */
      varId = oper->children[0].children[0].children[0].a_id;
      start = (GLint) oper->children[0].children[0].children[1].literal[0];
   }

   end = (GLint) oper->children[1].children[0].children[1].literal[0];

   for (iter = start; iter < end; iter++) {
      slang_operation *body;

      /* make a copy of the loop body */
      body = slang_operation_new(1);
      if (!body)
         return NULL;

      if (!slang_operation_copy(body, &oper->children[3]))
         return NULL;

      /* in body, replace instances of 'varId' with literal 'iter' */
      {
         slang_variable *oldVar;
         slang_operation *newOper;

         oldVar = _slang_variable_locate(oper->locals, varId, GL_TRUE);
         if (!oldVar) {
            /* undeclared loop variable */
            slang_operation_delete(body);
            return NULL;
         }

         newOper = slang_operation_new(1);
         newOper->type = SLANG_OPER_LITERAL_INT;
         newOper->literal_size = 1;
         newOper->literal[0] = (GLfloat) iter;

         /* replace instances of the loop variable with newOper */
         slang_substitute(A, body, 1, &oldVar, &newOper, GL_FALSE);
      }

      /* do IR codegen for body */
      n = _slang_gen_operation(A, body);
      if (!n)
         return NULL;

      root = new_seq(root, n);

      slang_operation_delete(body);
   }

   return root;
}


/**
 * Replace 'continue' statement with 'break' inside a for-loop.
 * This is a recursive helper function used by _slang_gen_for_without_continue().
 */
static void
replace_continue_with_break(slang_assemble_ctx *A, slang_operation *oper)
{
   switch (oper->type) {
   case SLANG_OPER_CONTINUE:
      oper->type = SLANG_OPER_BREAK;
      break;
   case SLANG_OPER_FOR:
   case SLANG_OPER_DO:
   case SLANG_OPER_WHILE:
      /* stop upon finding a nested loop */
      break;
   default:
      /* recurse */
      {
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            replace_continue_with_break(A, slang_oper_child(oper, i));
         }
      }
   }
}


/**
 * Transform a for-loop so that continue statements are converted to breaks.
 * Then do normal IR code generation.
 *
 * Before:
 * 
 * for (INIT; LOOPCOND; INCR) {
 *    A;
 *    if (IFCOND) {
 *       continue;
 *    }
 *    B;
 * }
 * 
 * After:
 * 
 * {
 *    bool _condFlag = 1;
 *    for (INIT; _condFlag; ) {
 *       for ( ; _condFlag = LOOPCOND; INCR) {
 *          A;
 *          if (IFCOND) {
 *             break;
 *          }
 *          B;
 *       }
 *       if (_condFlag)
 *          INCR;
 *    }
 * }
 */
static slang_ir_node *
_slang_gen_for_without_continue(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_operation *top;
   slang_operation *outerFor, *innerFor, *init, *cond, *incr;
   slang_operation *lhs, *rhs;

   assert(oper->type == SLANG_OPER_FOR);

   top = slang_operation_new(1);
   top->type = SLANG_OPER_BLOCK_NEW_SCOPE;
   top->locals->outer_scope = oper->locals->outer_scope;
   slang_operation_add_children(top, 2);

   /* declare: bool _condFlag = true */
   {
      slang_operation *condDecl = slang_oper_child(top, 0);
      slang_generate_declaration(A, top->locals, condDecl,
                                 SLANG_SPEC_BOOL, "_condFlag", GL_TRUE);
   }

   /* build outer loop:  for (INIT; _condFlag; ) { */
   outerFor = slang_oper_child(top, 1);
   outerFor->type = SLANG_OPER_FOR;
   slang_operation_add_children(outerFor, 4);

   init = slang_oper_child(outerFor, 0);
   slang_operation_copy(init, slang_oper_child(oper, 0));

   cond = slang_oper_child(outerFor, 1);
   cond->type = SLANG_OPER_IDENTIFIER;
   cond->a_id = slang_atom_pool_atom(A->atoms, "_condFlag");

   incr = slang_oper_child(outerFor, 2);
   incr->type = SLANG_OPER_VOID;

   /* body of the outer loop */
   {
      slang_operation *block = slang_oper_child(outerFor, 3);

      slang_operation_add_children(block, 2);
      block->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE;

      /* build inner loop:  for ( ; _condFlag = LOOPCOND; INCR) { */
      {
         innerFor = slang_oper_child(block, 0);

         /* make copy of orig loop */
         slang_operation_copy(innerFor, oper);
         assert(innerFor->type == SLANG_OPER_FOR);
         innerFor->locals->outer_scope = block->locals;

         init = slang_oper_child(innerFor, 0);
         init->type = SLANG_OPER_VOID; /* leak? */

         cond = slang_oper_child(innerFor, 1);
         slang_operation_destruct(cond);
         cond->type = SLANG_OPER_ASSIGN;
         cond->locals = _slang_variable_scope_new(innerFor->locals);
         slang_operation_add_children(cond, 2);

         lhs = slang_oper_child(cond, 0);
         lhs->type = SLANG_OPER_IDENTIFIER;
         lhs->a_id = slang_atom_pool_atom(A->atoms, "_condFlag");

         rhs = slang_oper_child(cond, 1);
         slang_operation_copy(rhs, slang_oper_child(oper, 1));
      }

      /* if (_condFlag) INCR; */
      {
         slang_operation *ifop = slang_oper_child(block, 1);
         ifop->type = SLANG_OPER_IF;
         slang_operation_add_children(ifop, 2);

         /* re-use cond node build above */
         slang_operation_copy(slang_oper_child(ifop, 0), cond);

         /* incr node from original for-loop operation */
         slang_operation_copy(slang_oper_child(ifop, 1),
                              slang_oper_child(oper, 2));
      }

      /* finally, replace "continue" with "break" in the inner for-loop */
      replace_continue_with_break(A, slang_oper_child(innerFor, 3));
   }

   return _slang_gen_operation(A, top);
}



/**
 * Generate IR for a for-loop.  Unrolling will be done when possible.
 */
static slang_ir_node *
_slang_gen_for(slang_assemble_ctx * A, slang_operation *oper)
{
   GLboolean unroll;

   if (!A->EmitContReturn) {
      /* We don't want to emit CONT instructions.  If this for-loop has
       * a continue, translate it away.
       */
      if (_slang_loop_contains_continue(slang_oper_child(oper, 3))) {
         return _slang_gen_for_without_continue(A, oper);
      }
   }

   unroll = _slang_can_unroll_for_loop(A, oper);
   if (unroll) {
      slang_ir_node *code = _slang_unroll_for_loop(A, oper);
      if (code)
         return code;
   }

   assert(oper->type == SLANG_OPER_FOR);

   /* conventional for-loop code generation */
   {
      /*
       * init code (child[0])
       * LOOP:
       *    BREAK if !expr (child[1])
       *    body code (child[3])
       *    tail code:
       *       incr code (child[2])   // XXX continue here
       */
      slang_ir_node *loop, *cond, *breakIf, *body, *init, *incr;
      init = _slang_gen_operation(A, &oper->children[0]);
      loop = new_loop(NULL);

      /* save loop state */
      push_loop(A, oper, loop);

      cond = new_cond(new_not(_slang_gen_operation(A, &oper->children[1])));
      breakIf = new_break_if_true(A, cond);
      body = _slang_gen_operation(A, &oper->children[3]);
      incr = _slang_gen_operation(A, &oper->children[2]);

      loop->Children[0] = new_seq(breakIf, body);
      loop->Children[1] = incr;  /* tail code */

      /* restore loop state */
      pop_loop(A);

      return new_seq(init, loop);
   }
}


static slang_ir_node *
_slang_gen_continue(slang_assemble_ctx * A, const slang_operation *oper)
{
   slang_ir_node *n, *cont, *incr = NULL, *loopNode;

   assert(oper->type == SLANG_OPER_CONTINUE);
   loopNode = current_loop_ir(A);
   assert(loopNode);
   assert(loopNode->Opcode == IR_LOOP);

   cont = new_node0(IR_CONT);
   if (cont) {
      cont->Parent = loopNode;
      /* insert this node at head of linked list of cont/break instructions */
      cont->List = loopNode->List;
      loopNode->List = cont;
   }

   n = new_seq(incr, cont);
   return n;
}


/**
 * Determine if the given operation is of a specific type.
 */
static GLboolean
is_operation_type(const slang_operation *oper, slang_operation_type type)
{
   if (oper->type == type)
      return GL_TRUE;
   else if ((oper->type == SLANG_OPER_BLOCK_NEW_SCOPE ||
             oper->type == SLANG_OPER_BLOCK_NO_NEW_SCOPE) &&
            oper->num_children == 1)
      return is_operation_type(&oper->children[0], type);
   else
      return GL_FALSE;
}


/**
 * Generate IR tree for an if/then/else conditional using high-level
 * IR_IF instruction.
 */
static slang_ir_node *
_slang_gen_if(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * eval expr (child[0])
    * IF expr THEN
    *    if-body code
    * ELSE
    *    else-body code
    * ENDIF
    */
   const GLboolean haveElseClause = !_slang_is_noop(&oper->children[2]);
   slang_ir_node *ifNode, *cond, *ifBody, *elseBody;
   GLboolean isConst, constTrue;

   /* type-check expression */
   if (!_slang_is_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log, "boolean expression expected for 'if'");
      return NULL;
   }

   if (!_slang_is_scalar_or_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log, "scalar/boolean expression expected for 'if'");
      return NULL;
   }

   isConst = _slang_is_constant_cond(&oper->children[0], &constTrue);
   if (isConst) {
      if (constTrue) {
         /* if (true) ... */
         return _slang_gen_operation(A, &oper->children[1]);
      }
      else {
         /* if (false) ... */
         return _slang_gen_operation(A, &oper->children[2]);
      }
   }

   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = new_cond(cond);

   if (is_operation_type(&oper->children[1], SLANG_OPER_BREAK)
       && !haveElseClause) {
      /* Special case: generate a conditional break */
      ifBody = new_break_if_true(A, cond);
      return ifBody;
   }
   else if (is_operation_type(&oper->children[1], SLANG_OPER_CONTINUE)
            && !haveElseClause
            && current_loop_oper(A)
            && current_loop_oper(A)->type != SLANG_OPER_FOR) {
      /* Special case: generate a conditional continue */
      ifBody = new_cont_if_true(A, cond);
      return ifBody;
   }
   else {
      /* general case */
      ifBody = _slang_gen_operation(A, &oper->children[1]);
      if (haveElseClause)
         elseBody = _slang_gen_operation(A, &oper->children[2]);
      else
         elseBody = NULL;
      ifNode = new_if(cond, ifBody, elseBody);
      return ifNode;
   }
}



static slang_ir_node *
_slang_gen_not(slang_assemble_ctx * A, const slang_operation *oper)
{
   slang_ir_node *n;

   assert(oper->type == SLANG_OPER_NOT);

   /* type-check expression */
   if (!_slang_is_scalar_or_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log,
                           "scalar/boolean expression expected for '!'");
      return NULL;
   }

   n = _slang_gen_operation(A, &oper->children[0]);
   if (n)
      return new_not(n);
   else
      return NULL;
}


static slang_ir_node *
_slang_gen_xor(slang_assemble_ctx * A, const slang_operation *oper)
{
   slang_ir_node *n1, *n2;

   assert(oper->type == SLANG_OPER_LOGICALXOR);

   if (!_slang_is_scalar_or_boolean(A, &oper->children[0]) ||
       !_slang_is_scalar_or_boolean(A, &oper->children[0])) {
      slang_info_log_error(A->log,
                           "scalar/boolean expressions expected for '^^'");
      return NULL;
   }

   n1 = _slang_gen_operation(A, &oper->children[0]);
   if (!n1)
      return NULL;
   n2 = _slang_gen_operation(A, &oper->children[1]);
   if (!n2)
      return NULL;
   return new_node2(IR_NOTEQUAL, n1, n2);
}


/**
 * Generate IR node for storage of a temporary of given size.
 */
static slang_ir_node *
_slang_gen_temporary(GLint size)
{
   slang_ir_storage *store;
   slang_ir_node *n = NULL;

   store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -2, size);
   if (store) {
      n = new_node0(IR_VAR_DECL);
      if (n) {
         n->Store = store;
      }
      else {
         _slang_free(store);
      }
   }
   return n;
}


/**
 * Generate program constants for an array.
 * Ex: const vec2[3] v = vec2[3](vec2(1,1), vec2(2,2), vec2(3,3));
 * This will allocate and initialize three vector constants, storing
 * the array in constant memory, not temporaries like a non-const array.
 * This can also be used for uniform array initializers.
 * \return GL_TRUE for success, GL_FALSE if failure (semantic error, etc).
 */
static GLboolean
make_constant_array(slang_assemble_ctx *A,
                    slang_variable *var,
                    slang_operation *initializer)
{
   struct gl_program *prog = A->program;
   const GLenum datatype = _slang_gltype_from_specifier(&var->type.specifier);
   const char *varName = (char *) var->a_name;
   const GLuint numElements = initializer->num_children;
   GLint size;
   GLuint i, j;
   GLfloat *values;

   if (!var->store) {
      var->store = _slang_new_ir_storage(PROGRAM_UNDEFINED, -6, -6);
   }
   size = var->store->Size;

   assert(var->type.qualifier == SLANG_QUAL_CONST ||
          var->type.qualifier == SLANG_QUAL_UNIFORM);
   assert(initializer->type == SLANG_OPER_CALL);
   assert(initializer->array_constructor);

   values = (GLfloat *) malloc(numElements * 4 * sizeof(GLfloat));

   /* convert constructor params into ordinary floats */
   for (i = 0; i < numElements; i++) {
      const slang_operation *op = &initializer->children[i];
      if (op->type != SLANG_OPER_LITERAL_FLOAT) {
         /* unsupported type for this optimization */
         free(values);
         return GL_FALSE;
      }
      for (j = 0; j < op->literal_size; j++) {
         values[i * 4 + j] = op->literal[j];
      }
      for ( ; j < 4; j++) {
         values[i * 4 + j] = 0.0f;
      }
   }

   /* slightly different paths for constants vs. uniforms */
   if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      var->store->File = PROGRAM_UNIFORM;
      var->store->Index = _mesa_add_uniform(prog->Parameters, varName,
                                            size, datatype, values);
   }
   else {
      var->store->File = PROGRAM_CONSTANT;
      var->store->Index = _mesa_add_named_constant(prog->Parameters, varName,
                                                   values, size);
   }
   assert(var->store->Size == size);

   free(values);

   return GL_TRUE;
}



/**
 * Generate IR node for allocating/declaring a variable (either a local or
 * a global).
 * Generally, this involves allocating an slang_ir_storage instance for the
 * variable, choosing a register file (temporary, constant, etc).
 * For ordinary variables we do not yet allocate storage though.  We do that
 * when we find the first actual use of the variable to avoid allocating temp
 * regs that will never get used.
 * At this time, uniforms are always allocated space in this function.
 *
 * \param initializer  Optional initializer expression for the variable.
 */
static slang_ir_node *
_slang_gen_var_decl(slang_assemble_ctx *A, slang_variable *var,
                    slang_operation *initializer)
{
   const char *varName = (const char *) var->a_name;
   const GLenum datatype = _slang_gltype_from_specifier(&var->type.specifier);
   slang_ir_node *varDecl, *n;
   slang_ir_storage *store;
   GLint arrayLen, size, totalSize;  /* if array then totalSize > size */
   gl_register_file file;

   /*assert(!var->declared);*/
   var->declared = GL_TRUE;

   /* determine GPU register file for simple cases */
   if (is_sampler_type(&var->type)) {
      file = PROGRAM_SAMPLER;
   }
   else if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      file = PROGRAM_UNIFORM;
   }
   else {
      file = PROGRAM_TEMPORARY;
   }

   size = _slang_sizeof_type_specifier(&var->type.specifier);
   if (size <= 0) {
      slang_info_log_error(A->log, "invalid declaration for '%s'", varName);
      return NULL;
   }

   arrayLen = _slang_array_length(var);
   totalSize = _slang_array_size(size, arrayLen);

   /* Allocate IR node for the declaration */
   varDecl = new_node0(IR_VAR_DECL);
   if (!varDecl)
      return NULL;

   /* Allocate slang_ir_storage for this variable if needed.
    * Note that we may not actually allocate a constant or temporary register
    * until later.
    */
   if (!var->store) {
      GLint index = -7;  /* TBD / unknown */
      var->store = _slang_new_ir_storage(file, index, totalSize);
      if (!var->store)
         return NULL; /* out of memory */
   }

   /* set the IR node's Var and Store pointers */
   varDecl->Var = var;
   varDecl->Store = var->store;


   store = var->store;

   /* if there's an initializer, generate IR for the expression */
   if (initializer) {
      slang_ir_node *varRef, *init;

      if (var->type.qualifier == SLANG_QUAL_CONST) {
         /* if the variable is const, the initializer must be a const
          * expression as well.
          */
#if 0
         if (!_slang_is_constant_expr(initializer)) {
            slang_info_log_error(A->log,
                                 "initializer for %s not constant", varName);
            return NULL;
         }
#endif
      }

      if (var->type.qualifier == SLANG_QUAL_UNIFORM &&
          !A->allow_uniform_initializers) {
         slang_info_log_error(A->log,
                              "initializer for uniform %s not allowed",
                              varName);
         return NULL;
      }

      /* IR for the variable we're initializing */
      varRef = new_var(A, var);
      if (!varRef) {
         slang_info_log_error(A->log, "out of memory");
         return NULL;
      }

      /* constant-folding, etc here */
      _slang_simplify(initializer, &A->space, A->atoms); 

      /* look for simple constant-valued variables and uniforms */
      if (var->type.qualifier == SLANG_QUAL_CONST ||
          var->type.qualifier == SLANG_QUAL_UNIFORM) {

         if (initializer->type == SLANG_OPER_CALL &&
             initializer->array_constructor) {
            /* array initializer */
            if (make_constant_array(A, var, initializer))
               return varRef;
         }
         else if (initializer->type == SLANG_OPER_LITERAL_FLOAT ||
                  initializer->type == SLANG_OPER_LITERAL_INT) {
            /* simple float/vector initializer */
            if (store->File == PROGRAM_UNIFORM) {
               store->Index = _mesa_add_uniform(A->program->Parameters,
                                                varName,
                                                totalSize, datatype,
                                                initializer->literal);
               store->Swizzle = _slang_var_swizzle(size, 0);
               return varRef;
            }
#if 0
            else {
               store->File = PROGRAM_CONSTANT;
               store->Index = _mesa_add_named_constant(A->program->Parameters,
                                                       varName,
                                                       initializer->literal,
                                                       totalSize);
               store->Swizzle = _slang_var_swizzle(size, 0);
               return varRef;
            }
#endif
         }
      }

      /* IR for initializer */
      init = _slang_gen_operation(A, initializer);
      if (!init)
         return NULL;

      /* XXX remove this when type checking is added above */
      if (init->Store && init->Store->Size != totalSize) {
         slang_info_log_error(A->log, "invalid assignment (wrong types)");
         return NULL;
      }

      /* assign RHS to LHS */
      n = new_node2(IR_COPY, varRef, init);
      n = new_seq(varDecl, n);
   }
   else {
      /* no initializer */
      n = varDecl;
   }

   if (store->File == PROGRAM_UNIFORM && store->Index < 0) {
      /* always need to allocate storage for uniforms at this point */
      store->Index = _mesa_add_uniform(A->program->Parameters, varName,
                                       totalSize, datatype, NULL);
      store->Swizzle = _slang_var_swizzle(size, 0);
   }

#if 0
   printf("%s var %p %s  store=%p index=%d size=%d\n",
          __FUNCTION__, (void *) var, (char *) varName,
          (void *) store, store->Index, store->Size);
#endif

   return n;
}


/**
 * Generate code for a selection expression:   b ? x : y
 * XXX In some cases we could implement a selection expression
 * with an LRP instruction (use the boolean as the interpolant).
 * Otherwise, we use an IF/ELSE/ENDIF construct.
 */
static slang_ir_node *
_slang_gen_select(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_ir_node *cond, *ifNode, *trueExpr, *falseExpr, *trueNode, *falseNode;
   slang_ir_node *tmpDecl, *tmpVar, *tree;
   slang_typeinfo type0, type1, type2;
   int size, isBool, isEqual;

   assert(oper->type == SLANG_OPER_SELECT);
   assert(oper->num_children == 3);

   /* type of children[0] must be boolean */
   slang_typeinfo_construct(&type0);
   typeof_operation(A, &oper->children[0], &type0);
   isBool = (type0.spec.type == SLANG_SPEC_BOOL);
   slang_typeinfo_destruct(&type0);
   if (!isBool) {
      slang_info_log_error(A->log, "selector type is not boolean");
      return NULL;
   }

   slang_typeinfo_construct(&type1);
   slang_typeinfo_construct(&type2);
   typeof_operation(A, &oper->children[1], &type1);
   typeof_operation(A, &oper->children[2], &type2);
   isEqual = slang_type_specifier_equal(&type1.spec, &type2.spec);
   slang_typeinfo_destruct(&type1);
   slang_typeinfo_destruct(&type2);
   if (!isEqual) {
      slang_info_log_error(A->log, "incompatible types for ?: operator");
      return NULL;
   }

   /* size of x or y's type */
   size = _slang_sizeof_type_specifier(&type1.spec);
   assert(size > 0);

   /* temporary var */
   tmpDecl = _slang_gen_temporary(size);

   /* the condition (child 0) */
   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = new_cond(cond);

   /* if-true body (child 1) */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;
   trueExpr = _slang_gen_operation(A, &oper->children[1]);
   trueNode = new_node2(IR_COPY, tmpVar, trueExpr);

   /* if-false body (child 2) */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;
   falseExpr = _slang_gen_operation(A, &oper->children[2]);
   falseNode = new_node2(IR_COPY, tmpVar, falseExpr);

   ifNode = new_if(cond, trueNode, falseNode);

   /* tmp var value */
   tmpVar = new_node0(IR_VAR);
   tmpVar->Store = tmpDecl->Store;

   tree = new_seq(ifNode, tmpVar);
   tree = new_seq(tmpDecl, tree);

   /*_slang_print_ir_tree(tree, 10);*/
   return tree;
}


/**
 * Generate code for &&.
 */
static slang_ir_node *
_slang_gen_logical_and(slang_assemble_ctx *A, slang_operation *oper)
{
   /* rewrite "a && b" as  "a ? b : false" */
   slang_operation *select;
   slang_ir_node *n;

   select = slang_operation_new(1);
   select->type = SLANG_OPER_SELECT;
   slang_operation_add_children(select, 3);

   slang_operation_copy(slang_oper_child(select, 0), &oper->children[0]);
   slang_operation_copy(slang_oper_child(select, 1), &oper->children[1]);
   slang_operation_literal_bool(slang_oper_child(select, 2), GL_FALSE);

   n = _slang_gen_select(A, select);
   return n;
}


/**
 * Generate code for ||.
 */
static slang_ir_node *
_slang_gen_logical_or(slang_assemble_ctx *A, slang_operation *oper)
{
   /* rewrite "a || b" as  "a ? true : b" */
   slang_operation *select;
   slang_ir_node *n;

   select = slang_operation_new(1);
   select->type = SLANG_OPER_SELECT;
   slang_operation_add_children(select, 3);

   slang_operation_copy(slang_oper_child(select, 0), &oper->children[0]);
   slang_operation_literal_bool(slang_oper_child(select, 1), GL_TRUE);
   slang_operation_copy(slang_oper_child(select, 2), &oper->children[1]);

   n = _slang_gen_select(A, select);
   return n;
}


/**
 * Generate IR tree for a return statement.
 */
static slang_ir_node *
_slang_gen_return(slang_assemble_ctx * A, slang_operation *oper)
{
   assert(oper->type == SLANG_OPER_RETURN);
   return new_return(A->curFuncEndLabel);
}


#if 0
/**
 * Determine if the given operation/expression is const-valued.
 */
static GLboolean
_slang_is_constant_expr(const slang_operation *oper)
{
   slang_variable *var;
   GLuint i;

   switch (oper->type) {
   case SLANG_OPER_IDENTIFIER:
      var = _slang_variable_locate(oper->locals, oper->a_id, GL_TRUE);
      if (var && var->type.qualifier == SLANG_QUAL_CONST)
         return GL_TRUE;
      return GL_FALSE;
   default:
      for (i = 0; i < oper->num_children; i++) {
         if (!_slang_is_constant_expr(&oper->children[i]))
            return GL_FALSE;
      }
      return GL_TRUE;
   }
}
#endif


/**
 * Check if an assignment of type t1 to t0 is legal.
 * XXX more cases needed.
 */
static GLboolean
_slang_assignment_compatible(slang_assemble_ctx *A,
                             slang_operation *op0,
                             slang_operation *op1)
{
   slang_typeinfo t0, t1;
   GLuint sz0, sz1;

   if (op0->type == SLANG_OPER_POSTINCREMENT ||
       op0->type == SLANG_OPER_POSTDECREMENT) {
      return GL_FALSE;
   }

   slang_typeinfo_construct(&t0);
   typeof_operation(A, op0, &t0);

   slang_typeinfo_construct(&t1);
   typeof_operation(A, op1, &t1);

   sz0 = _slang_sizeof_type_specifier(&t0.spec);
   sz1 = _slang_sizeof_type_specifier(&t1.spec);

#if 1
   if (sz0 != sz1) {
      /*printf("assignment size mismatch %u vs %u\n", sz0, sz1);*/
      return GL_FALSE;
   }
#endif

   if (t0.spec.type == SLANG_SPEC_STRUCT &&
       t1.spec.type == SLANG_SPEC_STRUCT &&
       t0.spec._struct->a_name != t1.spec._struct->a_name)
      return GL_FALSE;

   if (t0.spec.type == SLANG_SPEC_FLOAT &&
       t1.spec.type == SLANG_SPEC_BOOL)
      return GL_FALSE;

#if 0 /* not used just yet - causes problems elsewhere */
   if (t0.spec.type == SLANG_SPEC_INT &&
       t1.spec.type == SLANG_SPEC_FLOAT)
      return GL_FALSE;
#endif

   if (t0.spec.type == SLANG_SPEC_BOOL &&
       t1.spec.type == SLANG_SPEC_FLOAT)
      return GL_FALSE;

   if (t0.spec.type == SLANG_SPEC_BOOL &&
       t1.spec.type == SLANG_SPEC_INT)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Generate IR tree for a local variable declaration.
 * Basically do some error checking and call _slang_gen_var_decl().
 */
static slang_ir_node *
_slang_gen_declaration(slang_assemble_ctx *A, slang_operation *oper)
{
   const char *varName = (char *) oper->a_id;
   slang_variable *var;
   slang_ir_node *varDecl;
   slang_operation *initializer;

   assert(oper->type == SLANG_OPER_VARIABLE_DECL);
   assert(oper->num_children <= 1);


   /* lookup the variable by name */
   var = _slang_variable_locate(oper->locals, oper->a_id, GL_TRUE);
   if (!var)
      return NULL;  /* "shouldn't happen" */

   if (var->type.qualifier == SLANG_QUAL_ATTRIBUTE ||
       var->type.qualifier == SLANG_QUAL_VARYING ||
       var->type.qualifier == SLANG_QUAL_UNIFORM) {
      /* can't declare attribute/uniform vars inside functions */
      slang_info_log_error(A->log,
                "local variable '%s' cannot be an attribute/uniform/varying",
                varName);
      return NULL;
   }

#if 0
   if (v->declared) {
      slang_info_log_error(A->log, "variable '%s' redeclared", varName);
      return NULL;
   }
#endif

   /* check if the var has an initializer */
   if (oper->num_children > 0) {
      assert(oper->num_children == 1);
      initializer = &oper->children[0];
   }
   else if (var->initializer) {
      initializer = var->initializer;
   }
   else {
      initializer = NULL;
   }

   if (initializer) {
      /* check/compare var type and initializer type */
      if (!_slang_assignment_compatible(A, oper, initializer)) {
         slang_info_log_error(A->log, "incompatible types in assignment");
         return NULL;
      }         
   }
   else {
      if (var->type.qualifier == SLANG_QUAL_CONST) {
         slang_info_log_error(A->log,
                       "const-qualified variable '%s' requires initializer",
                       varName);
         return NULL;
      }
   }

   /* Generate IR node */
   varDecl = _slang_gen_var_decl(A, var, initializer);
   if (!varDecl)
      return NULL;

   return varDecl;
}


/**
 * Generate IR tree for a reference to a variable (such as in an expression).
 * This is different from a variable declaration.
 */
static slang_ir_node *
_slang_gen_variable(slang_assemble_ctx * A, slang_operation *oper)
{
   /* If there's a variable associated with this oper (from inlining)
    * use it.  Otherwise, use the oper's var id.
    */
   slang_atom name = oper->var ? oper->var->a_name : oper->a_id;
   slang_variable *var = _slang_variable_locate(oper->locals, name, GL_TRUE);
   slang_ir_node *n;
   if (!var) {
      slang_info_log_error(A->log, "undefined variable '%s'", (char *) name);
      return NULL;
   }
   assert(var->declared);
   n = new_var(A, var);
   return n;
}



/**
 * Return the number of components actually named by the swizzle.
 * Recall that swizzles may have undefined/don't-care values.
 */
static GLuint
swizzle_size(GLuint swizzle)
{
   GLuint size = 0, i;
   for (i = 0; i < 4; i++) {
      GLuint swz = GET_SWZ(swizzle, i);
      size += (swz >= 0 && swz <= 3);
   }
   return size;
}


static slang_ir_node *
_slang_gen_swizzle(slang_ir_node *child, GLuint swizzle)
{
   slang_ir_node *n = new_node1(IR_SWIZZLE, child);
   assert(child);
   if (n) {
      assert(!n->Store);
      n->Store = _slang_new_ir_storage_relative(0,
                                                swizzle_size(swizzle),
                                                child->Store);
      assert(n->Store);
      n->Store->Swizzle = swizzle;
   }
   return n;
}


static GLboolean
is_store_writable(const slang_assemble_ctx *A, const slang_ir_storage *store)
{
   while (store->Parent)
      store = store->Parent;

   if (!(store->File == PROGRAM_OUTPUT ||
         store->File == PROGRAM_TEMPORARY ||
         (store->File == PROGRAM_VARYING &&
          A->program->Target == GL_VERTEX_PROGRAM_ARB))) {
      return GL_FALSE;
   }
   else {
      return GL_TRUE;
   }
}


/**
 * Walk up an IR storage path to compute the final swizzle.
 * This is used when we find an expression such as "foo.xz.yx".
 */
static GLuint
root_swizzle(const slang_ir_storage *st)
{
   GLuint swizzle = st->Swizzle;
   while (st->Parent) {
      st = st->Parent;
      swizzle = _slang_swizzle_swizzle(st->Swizzle, swizzle);
   }
   return swizzle;
}


/**
 * Generate IR tree for an assignment (=).
 */
static slang_ir_node *
_slang_gen_assignment(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_operation *pred = NULL;
   slang_ir_node *n = NULL;

   if (oper->children[0].type == SLANG_OPER_IDENTIFIER) {
      /* Check that var is writeable */
      const char *varName = (char *) oper->children[0].a_id;
      slang_variable *var
         = _slang_variable_locate(oper->children[0].locals,
                                  oper->children[0].a_id, GL_TRUE);
      if (!var) {
         slang_info_log_error(A->log, "undefined variable '%s'", varName);
         return NULL;
      }

      if (var->type.qualifier == SLANG_QUAL_CONST ||
          var->type.qualifier == SLANG_QUAL_ATTRIBUTE ||
          var->type.qualifier == SLANG_QUAL_UNIFORM ||
          (var->type.qualifier == SLANG_QUAL_VARYING &&
           A->program->Target == GL_FRAGMENT_PROGRAM_ARB)) {
         slang_info_log_error(A->log,
                              "illegal assignment to read-only variable '%s'",
                              varName);
         return NULL;
      }

      /* check if we need to predicate this assignment based on __notRetFlag */
      if ((var->is_global ||
           var->type.qualifier == SLANG_QUAL_OUT ||
           var->type.qualifier == SLANG_QUAL_INOUT) && A->UseReturnFlag) {
         /* create predicate, used below */
         pred = slang_operation_new(1);
         pred->type = SLANG_OPER_IDENTIFIER;
         pred->a_id = slang_atom_pool_atom(A->atoms, "__notRetFlag");
         pred->locals->outer_scope = oper->locals->outer_scope;
      }
   }

   if (oper->children[0].type == SLANG_OPER_IDENTIFIER &&
       oper->children[1].type == SLANG_OPER_CALL) {
      /* Special case of:  x = f(a, b)
       * Replace with f(a, b, x)  (where x == hidden __retVal out param)
       *
       * XXX this could be even more effective if we could accomodate
       * cases such as "v.x = f();"  - would help with typical vertex
       * transformation.
       */
      n = _slang_gen_function_call_name(A,
                                      (const char *) oper->children[1].a_id,
                                      &oper->children[1], &oper->children[0]);
   }
   else {
      slang_ir_node *lhs, *rhs;

      /* lhs and rhs type checking */
      if (!_slang_assignment_compatible(A,
                                        &oper->children[0],
                                        &oper->children[1])) {
         slang_info_log_error(A->log, "incompatible types in assignment");
         return NULL;
      }

      lhs = _slang_gen_operation(A, &oper->children[0]);
      if (!lhs) {
         return NULL;
      }

      if (!lhs->Store) {
         slang_info_log_error(A->log,
                              "invalid left hand side for assignment");
         return NULL;
      }

      /* check that lhs is writable */
      if (!is_store_writable(A, lhs->Store)) {
         slang_info_log_error(A->log,
                              "illegal assignment to read-only l-value");
         return NULL;
      }

      rhs = _slang_gen_operation(A, &oper->children[1]);
      if (lhs && rhs) {
         /* convert lhs swizzle into writemask */
         const GLuint swizzle = root_swizzle(lhs->Store);
         GLuint writemask, newSwizzle = 0x0;
         if (!swizzle_to_writemask(A, swizzle, &writemask, &newSwizzle)) {
            /* Non-simple writemask, need to swizzle right hand side in
             * order to put components into the right place.
             */
            rhs = _slang_gen_swizzle(rhs, newSwizzle);
         }
         n = new_node2(IR_COPY, lhs, rhs);
      }
      else {
         return NULL;
      }
   }

   if (n && pred) {
      /* predicate the assignment code on __notRetFlag */
      slang_ir_node *top, *cond;

      cond = _slang_gen_operation(A, pred);
      top = new_if(cond, n, NULL);
      return top;
   }
   return n;
}


/**
 * Generate IR tree for referencing a field in a struct (or basic vector type)
 */
static slang_ir_node *
_slang_gen_struct_field(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_typeinfo ti;

   /* type of struct */
   slang_typeinfo_construct(&ti);
   typeof_operation(A, &oper->children[0], &ti);

   if (_slang_type_is_vector(ti.spec.type)) {
      /* the field should be a swizzle */
      const GLuint rows = _slang_type_dim(ti.spec.type);
      slang_swizzle swz;
      slang_ir_node *n;
      GLuint swizzle;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         slang_info_log_error(A->log, "Bad swizzle");
         return NULL;
      }
      swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                              swz.swizzle[1],
                              swz.swizzle[2],
                              swz.swizzle[3]);

      n = _slang_gen_operation(A, &oper->children[0]);
      /* create new parent node with swizzle */
      if (n)
         n = _slang_gen_swizzle(n, swizzle);
      return n;
   }
   else if (   ti.spec.type == SLANG_SPEC_FLOAT
            || ti.spec.type == SLANG_SPEC_INT
            || ti.spec.type == SLANG_SPEC_BOOL) {
      const GLuint rows = 1;
      slang_swizzle swz;
      slang_ir_node *n;
      GLuint swizzle;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         slang_info_log_error(A->log, "Bad swizzle");
      }
      swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                              swz.swizzle[1],
                              swz.swizzle[2],
                              swz.swizzle[3]);
      n = _slang_gen_operation(A, &oper->children[0]);
      /* create new parent node with swizzle */
      n = _slang_gen_swizzle(n, swizzle);
      return n;
   }
   else {
      /* the field is a structure member (base.field) */
      /* oper->children[0] is the base */
      /* oper->a_id is the field name */
      slang_ir_node *base, *n;
      slang_typeinfo field_ti;
      GLint fieldSize, fieldOffset = -1;

      /* type of field */
      slang_typeinfo_construct(&field_ti);
      typeof_operation(A, oper, &field_ti);

      fieldSize = _slang_sizeof_type_specifier(&field_ti.spec);
      if (fieldSize > 0)
         fieldOffset = _slang_field_offset(&ti.spec, oper->a_id);

      if (fieldSize == 0 || fieldOffset < 0) {
         const char *structName;
         if (ti.spec._struct)
            structName = (char *) ti.spec._struct->a_name;
         else
            structName = "unknown";
         slang_info_log_error(A->log,
                              "\"%s\" is not a member of struct \"%s\"",
                              (char *) oper->a_id, structName);
         return NULL;
      }
      assert(fieldSize >= 0);

      base = _slang_gen_operation(A, &oper->children[0]);
      if (!base) {
         /* error msg should have already been logged */
         return NULL;
      }

      n = new_node1(IR_FIELD, base);
      if (!n)
         return NULL;

      n->Field = (char *) oper->a_id;

      /* Store the field's offset in storage->Index */
      n->Store = _slang_new_ir_storage(base->Store->File,
                                       fieldOffset,
                                       fieldSize);

      return n;
   }
}


/**
 * Gen code for array indexing.
 */
static slang_ir_node *
_slang_gen_array_element(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_typeinfo array_ti;

   /* get array's type info */
   slang_typeinfo_construct(&array_ti);
   typeof_operation(A, &oper->children[0], &array_ti);

   if (_slang_type_is_vector(array_ti.spec.type)) {
      /* indexing a simple vector type: "vec4 v; v[0]=p;" */
      /* translate the index into a swizzle/writemask: "v.x=p" */
      const GLuint max = _slang_type_dim(array_ti.spec.type);
      GLint index;
      slang_ir_node *n;

      index = (GLint) oper->children[1].literal[0];
      if (oper->children[1].type != SLANG_OPER_LITERAL_INT ||
          index >= (GLint) max) {
#if 0
         slang_info_log_error(A->log, "Invalid array index for vector type");
         printf("type = %d\n", oper->children[1].type);
         printf("index = %d, max = %d\n", index, max);
         printf("array = %s\n", (char*)oper->children[0].a_id);
         printf("index = %s\n", (char*)oper->children[1].a_id);
         return NULL;
#else
         index = 0;
#endif
      }

      n = _slang_gen_operation(A, &oper->children[0]);
      if (n) {
         /* use swizzle to access the element */
         GLuint swizzle = MAKE_SWIZZLE4(SWIZZLE_X + index,
                                        SWIZZLE_NIL,
                                        SWIZZLE_NIL,
                                        SWIZZLE_NIL);
         n = _slang_gen_swizzle(n, swizzle);
      }
      return n;
   }
   else {
      /* conventional array */
      slang_typeinfo elem_ti;
      slang_ir_node *elem, *array, *index;
      GLint elemSize, arrayLen;

      /* size of array element */
      slang_typeinfo_construct(&elem_ti);
      typeof_operation(A, oper, &elem_ti);
      elemSize = _slang_sizeof_type_specifier(&elem_ti.spec);

      if (_slang_type_is_matrix(array_ti.spec.type))
         arrayLen = _slang_type_dim(array_ti.spec.type);
      else
         arrayLen = array_ti.array_len;

      slang_typeinfo_destruct(&array_ti);
      slang_typeinfo_destruct(&elem_ti);

      if (elemSize <= 0) {
         /* unknown var or type */
         slang_info_log_error(A->log, "Undefined variable or type");
         return NULL;
      }

      array = _slang_gen_operation(A, &oper->children[0]);
      index = _slang_gen_operation(A, &oper->children[1]);
      if (array && index) {
         /* bounds check */
         GLint constIndex = -1;
         if (index->Opcode == IR_FLOAT) {
            constIndex = (int) index->Value[0];
            if (constIndex < 0 || constIndex >= arrayLen) {
               slang_info_log_error(A->log,
                                "Array index out of bounds (index=%d size=%d)",
                                 constIndex, arrayLen);
               _slang_free_ir_tree(array);
               _slang_free_ir_tree(index);
               return NULL;
            }
         }

         if (!array->Store) {
            slang_info_log_error(A->log, "Invalid array");
            return NULL;
         }

         elem = new_node2(IR_ELEMENT, array, index);

         /* The storage info here will be updated during code emit */
         elem->Store = _slang_new_ir_storage(array->Store->File,
                                             array->Store->Index,
                                             elemSize);
         elem->Store->Swizzle = _slang_var_swizzle(elemSize, 0);
         return elem;
      }
      else {
         _slang_free_ir_tree(array);
         _slang_free_ir_tree(index);
         return NULL;
      }
   }
}


static slang_ir_node *
_slang_gen_compare(slang_assemble_ctx *A, slang_operation *oper,
                   slang_ir_opcode opcode)
{
   slang_typeinfo t0, t1;
   slang_ir_node *n;
   
   slang_typeinfo_construct(&t0);
   typeof_operation(A, &oper->children[0], &t0);

   slang_typeinfo_construct(&t1);
   typeof_operation(A, &oper->children[0], &t1);

   if (t0.spec.type == SLANG_SPEC_ARRAY ||
       t1.spec.type == SLANG_SPEC_ARRAY) {
      slang_info_log_error(A->log, "Illegal array comparison");
      return NULL;
   }

   if (oper->type != SLANG_OPER_EQUAL &&
       oper->type != SLANG_OPER_NOTEQUAL) {
      /* <, <=, >, >= can only be used with scalars */
      if ((t0.spec.type != SLANG_SPEC_INT &&
           t0.spec.type != SLANG_SPEC_FLOAT) ||
          (t1.spec.type != SLANG_SPEC_INT &&
           t1.spec.type != SLANG_SPEC_FLOAT)) {
         slang_info_log_error(A->log, "Incompatible type(s) for inequality operator");
         return NULL;
      }
   }

   n =  new_node2(opcode,
                  _slang_gen_operation(A, &oper->children[0]),
                  _slang_gen_operation(A, &oper->children[1]));

   /* result is a bool (size 1) */
   n->Store = _slang_new_ir_storage(PROGRAM_TEMPORARY, -1, 1);

   return n;
}


#if 0
static void
print_vars(slang_variable_scope *s)
{
   int i;
   printf("vars: ");
   for (i = 0; i < s->num_variables; i++) {
      printf("%s %d, \n",
             (char*) s->variables[i]->a_name,
             s->variables[i]->declared);
   }

   printf("\n");
}
#endif


#if 0
static void
_slang_undeclare_vars(slang_variable_scope *locals)
{
   if (locals->num_variables > 0) {
      int i;
      for (i = 0; i < locals->num_variables; i++) {
         slang_variable *v = locals->variables[i];
         printf("undeclare %s at %p\n", (char*) v->a_name, v);
         v->declared = GL_FALSE;
      }
   }
}
#endif


/**
 * Generate IR tree for a slang_operation (AST node)
 */
static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper)
{
   switch (oper->type) {
   case SLANG_OPER_BLOCK_NEW_SCOPE:
      {
         slang_ir_node *n;

         _slang_push_var_table(A->vartable);

         oper->type = SLANG_OPER_BLOCK_NO_NEW_SCOPE; /* temp change */
         n = _slang_gen_operation(A, oper);
         oper->type = SLANG_OPER_BLOCK_NEW_SCOPE; /* restore */

         _slang_pop_var_table(A->vartable);

         /*_slang_undeclare_vars(oper->locals);*/
         /*print_vars(oper->locals);*/

         if (n)
            n = new_node1(IR_SCOPE, n);
         return n;
      }
      break;

   case SLANG_OPER_BLOCK_NO_NEW_SCOPE:
      /* list of operations */
      if (oper->num_children > 0)
      {
         slang_ir_node *n, *tree = NULL;
         GLuint i;

         for (i = 0; i < oper->num_children; i++) {
            n = _slang_gen_operation(A, &oper->children[i]);
            if (!n) {
               _slang_free_ir_tree(tree);
               return NULL; /* error must have occured */
            }
            tree = new_seq(tree, n);
         }

         return tree;
      }
      else {
         return new_node0(IR_NOP);
      }

   case SLANG_OPER_EXPRESSION:
      return _slang_gen_operation(A, &oper->children[0]);

   case SLANG_OPER_FOR:
      return _slang_gen_for(A, oper);
   case SLANG_OPER_DO:
      return _slang_gen_do(A, oper);
   case SLANG_OPER_WHILE:
      return _slang_gen_while(A, oper);
   case SLANG_OPER_BREAK:
      if (!current_loop_oper(A)) {
         slang_info_log_error(A->log, "'break' not in loop");
         return NULL;
      }
      return new_break(current_loop_ir(A));
   case SLANG_OPER_CONTINUE:
      if (!current_loop_oper(A)) {
         slang_info_log_error(A->log, "'continue' not in loop");
         return NULL;
      }
      return _slang_gen_continue(A, oper);
   case SLANG_OPER_DISCARD:
      return new_node0(IR_KILL);

   case SLANG_OPER_EQUAL:
      return _slang_gen_compare(A, oper, IR_EQUAL);
   case SLANG_OPER_NOTEQUAL:
      return _slang_gen_compare(A, oper, IR_NOTEQUAL);
   case SLANG_OPER_GREATER:
      return _slang_gen_compare(A, oper, IR_SGT);
   case SLANG_OPER_LESS:
      return _slang_gen_compare(A, oper, IR_SLT);
   case SLANG_OPER_GREATEREQUAL:
      return _slang_gen_compare(A, oper, IR_SGE);
   case SLANG_OPER_LESSEQUAL:
      return _slang_gen_compare(A, oper, IR_SLE);
   case SLANG_OPER_ADD:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+", oper, NULL);
	 return n;
      }
   case SLANG_OPER_SUBTRACT:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case SLANG_OPER_MULTIPLY:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
         n = _slang_gen_function_call_name(A, "*", oper, NULL);
	 return n;
      }
   case SLANG_OPER_DIVIDE:
      {
         slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/", oper, NULL);
	 return n;
      }
   case SLANG_OPER_MINUS:
      {
         slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case SLANG_OPER_PLUS:
      /* +expr   --> do nothing */
      return _slang_gen_operation(A, &oper->children[0]);
   case SLANG_OPER_VARIABLE_DECL:
      return _slang_gen_declaration(A, oper);
   case SLANG_OPER_ASSIGN:
      return _slang_gen_assignment(A, oper);
   case SLANG_OPER_ADDASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+=", oper, NULL);
	 return n;
      }
   case SLANG_OPER_SUBASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-=", oper, NULL);
	 return n;
      }
      break;
   case SLANG_OPER_MULASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "*=", oper, NULL);
	 return n;
      }
   case SLANG_OPER_DIVASSIGN:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/=", oper, NULL);
	 return n;
      }
   case SLANG_OPER_LOGICALAND:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_logical_and(A, oper);
	 return n;
      }
   case SLANG_OPER_LOGICALOR:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_logical_or(A, oper);
	 return n;
      }
   case SLANG_OPER_LOGICALXOR:
      return _slang_gen_xor(A, oper);
   case SLANG_OPER_NOT:
      return _slang_gen_not(A, oper);
   case SLANG_OPER_SELECT:  /* b ? x : y */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 3);
	 n = _slang_gen_select(A, oper);
	 return n;
      }

   case SLANG_OPER_ASM:
      return _slang_gen_asm(A, oper, NULL);
   case SLANG_OPER_CALL:
      return _slang_gen_function_call_name(A, (const char *) oper->a_id,
                                           oper, NULL);
   case SLANG_OPER_METHOD:
      return _slang_gen_method_call(A, oper);
   case SLANG_OPER_RETURN:
      return _slang_gen_return(A, oper);
   case SLANG_OPER_RETURN_INLINED:
      return _slang_gen_return(A, oper);
   case SLANG_OPER_LABEL:
      return new_label(oper->label);
   case SLANG_OPER_IDENTIFIER:
      return _slang_gen_variable(A, oper);
   case SLANG_OPER_IF:
      return _slang_gen_if(A, oper);
   case SLANG_OPER_FIELD:
      return _slang_gen_struct_field(A, oper);
   case SLANG_OPER_SUBSCRIPT:
      return _slang_gen_array_element(A, oper);
   case SLANG_OPER_LITERAL_FLOAT:
      /* fall-through */
   case SLANG_OPER_LITERAL_INT:
      /* fall-through */
   case SLANG_OPER_LITERAL_BOOL:
      return new_float_literal(oper->literal, oper->literal_size);

   case SLANG_OPER_POSTINCREMENT:   /* var++ */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__postIncr", oper, NULL);
	 return n;
      }
   case SLANG_OPER_POSTDECREMENT:   /* var-- */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "__postDecr", oper, NULL);
	 return n;
      }
   case SLANG_OPER_PREINCREMENT:   /* ++var */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "++", oper, NULL);
	 return n;
      }
   case SLANG_OPER_PREDECREMENT:   /* --var */
      {
	 slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "--", oper, NULL);
	 return n;
      }

   case SLANG_OPER_NON_INLINED_CALL:
   case SLANG_OPER_SEQUENCE:
      {
         slang_ir_node *tree = NULL;
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            slang_ir_node *n = _slang_gen_operation(A, &oper->children[i]);
            tree = new_seq(tree, n);
            if (n)
               tree->Store = n->Store;
         }
         if (oper->type == SLANG_OPER_NON_INLINED_CALL) {
            tree = new_function_call(tree, oper->label);
         }
         return tree;
      }

   case SLANG_OPER_NONE:
   case SLANG_OPER_VOID:
      /* returning NULL here would generate an error */
      return new_node0(IR_NOP);

   default:
      _mesa_problem(NULL, "bad node type %d in _slang_gen_operation",
                    oper->type);
      return new_node0(IR_NOP);
   }

   return NULL;
}


/**
 * Check if the given type specifier is a rectangular texture sampler.
 */
static GLboolean
is_rect_sampler_spec(const slang_type_specifier *spec)
{
   while (spec->_array) {
      spec = spec->_array;
   }
   return spec->type == SLANG_SPEC_SAMPLER_RECT ||
          spec->type == SLANG_SPEC_SAMPLER_RECT_SHADOW;
}



/**
 * Called by compiler when a global variable has been parsed/compiled.
 * Here we examine the variable's type to determine what kind of register
 * storage will be used.
 *
 * A uniform such as "gl_Position" will become the register specification
 * (PROGRAM_OUTPUT, VERT_RESULT_HPOS).  Or, uniform "gl_FogFragCoord"
 * will be (PROGRAM_INPUT, FRAG_ATTRIB_FOGC).
 *
 * Samplers are interesting.  For "uniform sampler2D tex;" we'll specify
 * (PROGRAM_SAMPLER, index) where index is resolved at link-time to an
 * actual texture unit (as specified by the user calling glUniform1i()).
 */
GLboolean
_slang_codegen_global_variable(slang_assemble_ctx *A, slang_variable *var,
                               slang_unit_type type)
{
   struct gl_program *prog = A->program;
   const char *varName = (char *) var->a_name;
   GLboolean success = GL_TRUE;
   slang_ir_storage *store = NULL;
   int dbg = 0;
   const GLenum datatype = _slang_gltype_from_specifier(&var->type.specifier);
   const GLint size = _slang_sizeof_type_specifier(&var->type.specifier);
   const GLint arrayLen = _slang_array_length(var);
   const GLint totalSize = _slang_array_size(size, arrayLen);
   GLint texIndex = sampler_to_texture_index(var->type.specifier.type);

   var->is_global = GL_TRUE;

   /* check for sampler2D arrays */
   if (texIndex == -1 && var->type.specifier._array)
      texIndex = sampler_to_texture_index(var->type.specifier._array->type);

   if (texIndex != -1) {
      /* This is a texture sampler variable...
       * store->File = PROGRAM_SAMPLER
       * store->Index = sampler number (0..7, typically)
       * store->Size = texture type index (1D, 2D, 3D, cube, etc)
       */
      if (var->initializer) {
         slang_info_log_error(A->log, "illegal assignment to '%s'", varName);
         return GL_FALSE;
      }
#if FEATURE_es2_glsl /* XXX should use FEATURE_texture_rect */
      /* disallow rect samplers */
      if (is_rect_sampler_spec(&var->type.specifier)) {
         slang_info_log_error(A->log, "invalid sampler type for '%s'", varName);
         return GL_FALSE;
      }
#else
      (void) is_rect_sampler_spec; /* silence warning */
#endif
      {
         GLint sampNum = _mesa_add_sampler(prog->Parameters, varName, datatype);
         store = _slang_new_ir_storage_sampler(sampNum, texIndex, totalSize);

         /* If we have a sampler array, then we need to allocate the 
	  * additional samplers to ensure we don't allocate them elsewhere.
	  * We can't directly use _mesa_add_sampler() as that checks the
	  * varName and gets a match, so we call _mesa_add_parameter()
	  * directly and use the last sampler number from the call above.
	  */
	 if (arrayLen > 0) {
	    GLint a = arrayLen - 1;
	    GLint i;
	    for (i = 0; i < a; i++) {
               GLfloat value = (GLfloat)(i + sampNum + 1);
               (void) _mesa_add_parameter(prog->Parameters, PROGRAM_SAMPLER,
                                 varName, 1, datatype, &value, NULL, 0x0);
	    }
	 }
      }
      if (dbg) printf("SAMPLER ");
   }
   else if (var->type.qualifier == SLANG_QUAL_UNIFORM) {
      /* Uniform variable */
      const GLuint swizzle = _slang_var_swizzle(totalSize, 0);

      if (prog) {
         /* user-defined uniform */
         if (datatype == GL_NONE) {
	    if ((var->type.specifier.type == SLANG_SPEC_ARRAY &&
	         var->type.specifier._array->type == SLANG_SPEC_STRUCT) ||
                (var->type.specifier.type == SLANG_SPEC_STRUCT)) {
               /* temporary work-around */
               GLenum datatype = GL_FLOAT;
               GLint uniformLoc = _mesa_add_uniform(prog->Parameters, varName,
                                                    totalSize, datatype, NULL);
               store = _slang_new_ir_storage_swz(PROGRAM_UNIFORM, uniformLoc,
                                                 totalSize, swizzle);
	 
	       if (arrayLen > 0) {
	          GLint a = arrayLen - 1;
	          GLint i;
	          for (i = 0; i < a; i++) {
                     GLfloat value = (GLfloat)(i + uniformLoc + 1);
                     (void) _mesa_add_parameter(prog->Parameters, PROGRAM_UNIFORM,
                                 varName, 1, datatype, &value, NULL, 0x0);
                  }
	       }

               /* XXX what we need to do is unroll the struct into its
                * basic types, creating a uniform variable for each.
                * For example:
                * struct foo {
                *   vec3 a;
                *   vec4 b;
                * };
                * uniform foo f;
                *
                * Should produce uniforms:
                * "f.a"  (GL_FLOAT_VEC3)
                * "f.b"  (GL_FLOAT_VEC4)
                */

               if (var->initializer) {
                  slang_info_log_error(A->log,
                     "unsupported initializer for uniform '%s'", varName);
                  return GL_FALSE;
               }
            }
            else {
               slang_info_log_error(A->log,
                                    "invalid datatype for uniform variable %s",
                                    varName);
               return GL_FALSE;
            }
         }
         else {
            /* non-struct uniform */
            if (!_slang_gen_var_decl(A, var, var->initializer))
               return GL_FALSE;
            store = var->store;
         }
      }
      else {
         /* pre-defined uniform, like gl_ModelviewMatrix */
         /* We know it's a uniform, but don't allocate storage unless
          * it's really used.
          */
         store = _slang_new_ir_storage_swz(PROGRAM_STATE_VAR, -1,
                                           totalSize, swizzle);
      }
      if (dbg) printf("UNIFORM (sz %d) ", totalSize);
   }
   else if (var->type.qualifier == SLANG_QUAL_VARYING) {
      /* varyings must be float, vec or mat */
      if (!_slang_type_is_float_vec_mat(var->type.specifier.type) &&
          var->type.specifier.type != SLANG_SPEC_ARRAY) {
         slang_info_log_error(A->log,
                              "varying '%s' must be float/vector/matrix",
                              varName);
         return GL_FALSE;
      }

      if (var->initializer) {
         slang_info_log_error(A->log, "illegal initializer for varying '%s'",
                              varName);
         return GL_FALSE;
      }

      if (prog) {
         /* user-defined varying */
         GLbitfield flags;
         GLint varyingLoc;
         GLuint swizzle;

         flags = 0x0;
         if (var->type.centroid == SLANG_CENTROID)
            flags |= PROG_PARAM_BIT_CENTROID;
         if (var->type.variant == SLANG_INVARIANT)
            flags |= PROG_PARAM_BIT_INVARIANT;

         varyingLoc = _mesa_add_varying(prog->Varying, varName,
                                        totalSize, flags);
         swizzle = _slang_var_swizzle(size, 0);
         store = _slang_new_ir_storage_swz(PROGRAM_VARYING, varyingLoc,
                                           totalSize, swizzle);
      }
      else {
         /* pre-defined varying, like gl_Color or gl_TexCoord */
         if (type == SLANG_UNIT_FRAGMENT_BUILTIN) {
            /* fragment program input */
            GLuint swizzle;
            GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB,
                                             &swizzle);
            assert(index >= 0);
            assert(index < FRAG_ATTRIB_MAX);
            store = _slang_new_ir_storage_swz(PROGRAM_INPUT, index,
                                              size, swizzle);
         }
         else {
            /* vertex program output */
            GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
            GLuint swizzle = _slang_var_swizzle(size, 0);
            assert(index >= 0);
            assert(index < VERT_RESULT_MAX);
            assert(type == SLANG_UNIT_VERTEX_BUILTIN);
            store = _slang_new_ir_storage_swz(PROGRAM_OUTPUT, index,
                                              size, swizzle);
         }
         if (dbg) printf("V/F ");
      }
      if (dbg) printf("VARYING ");
   }
   else if (var->type.qualifier == SLANG_QUAL_ATTRIBUTE) {
      GLuint swizzle;
      GLint index;
      /* attributes must be float, vec or mat */
      if (!_slang_type_is_float_vec_mat(var->type.specifier.type)) {
         slang_info_log_error(A->log,
                              "attribute '%s' must be float/vector/matrix",
                              varName);
         return GL_FALSE;
      }

      if (prog) {
         /* user-defined vertex attribute */
         const GLint attr = -1; /* unknown */
         swizzle = _slang_var_swizzle(size, 0);
         index = _mesa_add_attribute(prog->Attributes, varName,
                                     size, datatype, attr);
         assert(index >= 0);
         index = VERT_ATTRIB_GENERIC0 + index;
      }
      else {
         /* pre-defined vertex attrib */
         index = _slang_input_index(varName, GL_VERTEX_PROGRAM_ARB, &swizzle);
         assert(index >= 0);
      }
      store = _slang_new_ir_storage_swz(PROGRAM_INPUT, index, size, swizzle);
      if (dbg) printf("ATTRIB ");
   }
   else if (var->type.qualifier == SLANG_QUAL_FIXEDINPUT) {
      GLuint swizzle = SWIZZLE_XYZW; /* silence compiler warning */
      GLint index = _slang_input_index(varName, GL_FRAGMENT_PROGRAM_ARB,
                                       &swizzle);
      store = _slang_new_ir_storage_swz(PROGRAM_INPUT, index, size, swizzle);
      if (dbg) printf("INPUT ");
   }
   else if (var->type.qualifier == SLANG_QUAL_FIXEDOUTPUT) {
      if (type == SLANG_UNIT_VERTEX_BUILTIN) {
         GLint index = _slang_output_index(varName, GL_VERTEX_PROGRAM_ARB);
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, size);
      }
      else {
         GLint index = _slang_output_index(varName, GL_FRAGMENT_PROGRAM_ARB);
         GLint specialSize = 4; /* treat all fragment outputs as float[4] */
         assert(type == SLANG_UNIT_FRAGMENT_BUILTIN);
         store = _slang_new_ir_storage(PROGRAM_OUTPUT, index, specialSize);
      }
      if (dbg) printf("OUTPUT ");
   }
   else if (var->type.qualifier == SLANG_QUAL_CONST && !prog) {
      /* pre-defined global constant, like gl_MaxLights */
      store = _slang_new_ir_storage(PROGRAM_CONSTANT, -1, size);
      if (dbg) printf("CONST ");
   }
   else {
      /* ordinary variable (may be const) */
      slang_ir_node *n;

      /* IR node to declare the variable */
      n = _slang_gen_var_decl(A, var, var->initializer);

      /* emit GPU instructions */
      success = _slang_emit_code(n, A->vartable, A->program, A->pragmas, GL_FALSE, A->log);

      _slang_free_ir_tree(n);
   }

   if (dbg) printf("GLOBAL VAR %s  idx %d\n", (char*) var->a_name,
                   store ? store->Index : -2);

   if (store)
      var->store = store;  /* save var's storage info */

   var->declared = GL_TRUE;

   return success;
}


/**
 * Produce an IR tree from a function AST (fun->body).
 * Then call the code emitter to convert the IR tree into gl_program
 * instructions.
 */
GLboolean
_slang_codegen_function(slang_assemble_ctx * A, slang_function * fun)
{
   slang_ir_node *n;
   GLboolean success = GL_TRUE;

   if (strcmp((char *) fun->header.a_name, "main") != 0) {
      /* we only really generate code for main, all other functions get
       * inlined or codegen'd upon an actual call.
       */
#if 0
      /* do some basic error checking though */
      if (fun->header.type.specifier.type != SLANG_SPEC_VOID) {
         /* check that non-void functions actually return something */
         slang_operation *op
            = _slang_find_node_type(fun->body, SLANG_OPER_RETURN);
         if (!op) {
            slang_info_log_error(A->log,
                                 "function \"%s\" has no return statement",
                                 (char *) fun->header.a_name);
            printf(
                   "function \"%s\" has no return statement\n",
                   (char *) fun->header.a_name);
            return GL_FALSE;
         }
      }
#endif
      return GL_TRUE;  /* not an error */
   }

#if 0
   printf("\n*********** codegen_function %s\n", (char *) fun->header.a_name);
   slang_print_function(fun, 1);
#endif

   /* should have been allocated earlier: */
   assert(A->program->Parameters );
   assert(A->program->Varying);
   assert(A->vartable);

   A->LoopDepth = 0;
   A->UseReturnFlag = GL_FALSE;
   A->CurFunction = fun;

   /* fold constant expressions, etc. */
   _slang_simplify(fun->body, &A->space, A->atoms);

#if 0
   printf("\n*********** simplified %s\n", (char *) fun->header.a_name);
   slang_print_function(fun, 1);
#endif

   /* Create an end-of-function label */
   A->curFuncEndLabel = _slang_label_new("__endOfFunc__main");

   /* push new vartable scope */
   _slang_push_var_table(A->vartable);

   /* Generate IR tree for the function body code */
   n = _slang_gen_operation(A, fun->body);
   if (n)
      n = new_node1(IR_SCOPE, n);

   /* pop vartable, restore previous */
   _slang_pop_var_table(A->vartable);

   if (!n) {
      /* XXX record error */
      return GL_FALSE;
   }

   /* append an end-of-function-label to IR tree */
   n = new_seq(n, new_label(A->curFuncEndLabel));

   /*_slang_label_delete(A->curFuncEndLabel);*/
   A->curFuncEndLabel = NULL;

#if 0
   printf("************* New AST for %s *****\n", (char*)fun->header.a_name);
   slang_print_function(fun, 1);
#endif
#if 0
   printf("************* IR for %s *******\n", (char*)fun->header.a_name);
   _slang_print_ir_tree(n, 0);
#endif
#if 0
   printf("************* End codegen function ************\n\n");
#endif

   if (A->UnresolvedRefs) {
      /* Can't codegen at this time.
       * At link time we'll concatenate all the vertex shaders and/or all
       * the fragment shaders and try recompiling.
       */
      return GL_TRUE;
   }

   /* Emit program instructions */
   success = _slang_emit_code(n, A->vartable, A->program, A->pragmas, GL_TRUE, A->log);
   _slang_free_ir_tree(n);

   /* free codegen context */
   /*
   free(A->codegen);
   */

   return success;
}

