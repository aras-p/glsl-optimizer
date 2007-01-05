/*
 * Mesa 3-D graphics library
 * Version:  6.5.3
 *
 * Copyright (C) 2005-2007  Brian Paul   All Rights Reserved.
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
 * Mesa GLSL code generator.  Convert AST to IR tree.
 * \author Brian Paul
 */

#include "imports.h"
#include "macros.h"
#include "slang_assemble.h"
#include "slang_codegen.h"
#include "slang_compile.h"
#include "slang_storage.h"
#include "slang_error.h"
#include "slang_simplify.h"
#include "slang_emit.h"
#include "slang_ir.h"
#include "mtypes.h"
#include "program.h"
#include "prog_instruction.h"
#include "prog_parameter.h"
#include "slang_print.h"


/**
 * XXX move these into the slang_assemble_ctx struct
 */
static slang_function *CurFunction = NULL;
static slang_atom CurLoopBreak = 0;
static slang_atom CurLoopCont = 0;


static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper);


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
   { "vec4_multiply", IR_MUL, 1, 2 },
   { "vec4_dot", IR_DOT4, 1, 2 },
   { "vec3_dot", IR_DOT3, 1, 2 },
   { "vec3_cross", IR_CROSS, 1, 2 },
   { "vec4_min", IR_MIN, 1, 2 },
   { "vec4_max", IR_MAX, 1, 2 },
   { "vec4_seq", IR_SEQ, 1, 2 },
   { "vec4_sge", IR_SGE, 1, 2 },
   { "vec4_sgt", IR_SGT, 1, 2 },
   /* vec4 unary */
   { "vec4_floor", IR_FLOOR, 1, 1 },
   { "vec4_frac", IR_FRAC, 1, 1 },
   { "vec4_abs", IR_ABS, 1, 1 },
   { "vec4_negate", IR_NEG, 1, 1 },
   /* float binary op */
   { "float_add", IR_ADD, 1, 2 },
   { "float_subtract", IR_SUB, 1, 2 },
   { "float_multiply", IR_MUL, 1, 2 },
   { "float_divide", IR_DIV, 1, 2 },
   { "float_power", IR_POW, 1, 2 },
   /* texture / sampler */
   { "vec4_tex1d", IR_TEX, 1, 1 },
   { "vec4_texb1d", IR_TEXB, 1, 3 },
   { "vec4_tex2d", IR_TEX, 1, 2 },
   { "vec4_texb2d", IR_TEXB, 1, 3 },
   /* unary op */
   { "int_to_float", IR_I_TO_F, 1, 1 },
   { "float_exp", IR_EXP, 1, 1 },
   { "float_exp2", IR_EXP2, 1, 1 },
   { "float_log2", IR_LOG2, 1, 1 },
   { "float_rsq", IR_RSQ, 1, 1 },
   { "float_rcp", IR_RCP, 1, 1 },
   { "float_sine", IR_SIN, 1, 1 },
   { "float_cosine", IR_COS, 1, 1 },
   { NULL, IR_NOP, 0, 0 }
};



static slang_ir_node *
new_node(slang_ir_opcode op, slang_ir_node *left, slang_ir_node *right)
{
   slang_ir_node *n = (slang_ir_node *) calloc(1, sizeof(slang_ir_node));
   if (n) {
      n->Opcode = op;
      n->Children[0] = left;
      n->Children[1] = right;
      n->Swizzle = SWIZZLE_NOOP;
      n->Writemask = WRITEMASK_XYZW;
   }
   return n;
}

static slang_ir_node *
new_seq(slang_ir_node *left, slang_ir_node *right)
{
   /* XXX if either left or right is null, just return pointer to other?? */
   assert(left);
   assert(right);
   return new_node(IR_SEQ, left, right);
}

static slang_ir_node *
new_label(slang_atom labName)
{
   slang_ir_node *n = new_node(IR_LABEL, NULL, NULL);
   n->Target = (char *) labName; /*_mesa_strdup(name);*/
   return n;
}

static slang_ir_node *
new_float_literal(float x, float y, float z, float w)
{
   slang_ir_node *n = new_node(IR_FLOAT, NULL, NULL);
   n->Value[0] = x;
   n->Value[1] = y;
   n->Value[2] = z;
   n->Value[3] = w;
   return n;
}

/**
 * XXX maybe pass an IR node as second param to indicate the jump target???
 */
static slang_ir_node *
new_cjump(slang_atom target)
{
   slang_ir_node *n = new_node(IR_CJUMP, NULL, NULL);
   if (n)
      n->Target = (char *) target;
   return n;
}

/**
 * XXX maybe pass an IR node as second param to indicate the jump target???
 */
static slang_ir_node *
new_jump(slang_atom target)
{
   slang_ir_node *n = new_node(IR_JUMP, NULL, NULL);
   if (n)
      n->Target = (char *) target;
   return n;
}


/**
 * New IR_VAR node - a reference to a previously declared variable.
 */
static slang_ir_node *
new_var(slang_assemble_ctx *A, slang_operation *oper,
        slang_atom name, GLuint swizzle)
{
   slang_variable *v = _slang_locate_variable(oper->locals, name, GL_TRUE);
   slang_ir_node *n = new_node(IR_VAR, NULL, NULL);
   if (!v) {
      printf("VAR NOT FOUND %s\n", (char *) name);
      assert(v);
   }
   /** 
   assert(v->declared);
   **/
   assert(!oper->var || oper->var == v);
   v->used = GL_TRUE;
   oper->var = v;
   n->Swizzle = swizzle;
   n->Var = v;
   slang_resolve_storage(A->codegen, n, A->program);
   return n;
}


static GLboolean
slang_is_writemask(const char *field, GLuint *mask)
{
   const GLuint n = 4;
   GLuint i, bit, c = 0;

   for (i = 0; i < n && field[i]; i++) {
      switch (field[i]) {
      case 'x':
      case 'r':
         bit = WRITEMASK_X;
         break;
      case 'y':
      case 'g':
         bit = WRITEMASK_Y;
         break;
      case 'z':
      case 'b':
         bit = WRITEMASK_Z;
         break;
      case 'w':
      case 'a':
         bit = WRITEMASK_W;
         break;
      default:
         return GL_FALSE;
      }
      if (c & bit)
         return GL_FALSE;
      c |= bit;
   }
   *mask = c;
   return GL_TRUE;
}


/**
 * Check if the given function is really just a wrapper for an
 * basic assembly instruction.
 */
static GLboolean
slang_is_asm_function(const slang_function *fun)
{
   if (fun->body->type == slang_oper_block_no_new_scope &&
       fun->body->num_children == 1 &&
       fun->body->children[0].type == slang_oper_asm) {
      return GL_TRUE;
   }
   return GL_FALSE;
}


/**
 * Produce inline code for a call to an assembly instruction.
 */
static slang_operation *
slang_inline_asm_function(slang_assemble_ctx *A,
                          slang_function *fun, slang_operation *oper)
{
   const int numArgs = oper->num_children;
   const slang_operation *args = oper->children;
   GLuint i;
   slang_operation *inlined = slang_operation_new(1);

   /*assert(oper->type == slang_oper_call);  or vec4_add, etc */

   inlined->type = fun->body->children[0].type;
   inlined->a_id = fun->body->children[0].a_id;
   inlined->num_children = numArgs;
   inlined->children = slang_operation_new(numArgs);
#if 0
   inlined->locals = slang_variable_scope_copy(oper->locals);
#else
   assert(inlined->locals);
   inlined->locals->outer_scope = oper->locals->outer_scope;
#endif

   for (i = 0; i < numArgs; i++) {
      slang_operation_copy(inlined->children + i, args + i);
   }

   return inlined;
}


static void
slang_resolve_variable(slang_operation *oper)
{
   if (oper->type != slang_oper_identifier)
      return;
   if (!oper->var) {
      oper->var = _slang_locate_variable(oper->locals,
                                         (const slang_atom) oper->a_id,
                                         GL_TRUE);
      if (oper->var)
         oper->var->used = GL_TRUE;
   }
}


/**
 * Replace particular variables (slang_oper_identifier) with new expressions.
 */
static void
slang_substitute(slang_assemble_ctx *A, slang_operation *oper,
                 GLuint substCount, slang_variable **substOld,
		 slang_operation **substNew, GLboolean isLHS)
{
   switch (oper->type) {
   case slang_oper_variable_decl:
      {
         slang_variable *v = _slang_locate_variable(oper->locals,
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
            slang_substitute(A, &oper->children[0], substCount, substOld, substNew, GL_FALSE);
         }
      }
      break;
   case slang_oper_identifier:
      assert(oper->num_children == 0);
      if (1/**!isLHS XXX FIX */) {
         slang_atom id = oper->a_id;
         slang_variable *v;
	 GLuint i;
         v = _slang_locate_variable(oper->locals, id, GL_TRUE);
	 if (!v) {
	    printf("var %s not found!\n", (char *) oper->a_id);
	    break;
	 }

	 /* look for a substitution */
	 for (i = 0; i < substCount; i++) {
	    if (v == substOld[i]) {
               /* OK, replace this slang_oper_identifier with a new expr */
	       assert(substNew[i]->type == slang_oper_identifier ||
                      substNew[i]->type == slang_oper_literal_float);
#if 0 /* DEBUG only */
	       if (substNew[i]->type == slang_oper_identifier) {
                  assert(substNew[i]->var);
                  assert(substNew[i]->var->a_name);
		  printf("Substitute %s with %s in id node %p\n",
			 (char*)v->a_name, (char*) substNew[i]->var->a_name,
			 (void*) oper);
               }
	       else
		  printf("Substitute %s with %f in id node %p\n",
			 (char*)v->a_name, substNew[i]->literal[0],
			 (void*) oper);
#endif
	       slang_operation_copy(oper, substNew[i]);
	       break;
	    }
	 }
      }
      break;
#if 0 /* XXX rely on default case below */
   case slang_oper_return:
      /* do return replacement here too */
      assert(oper->num_children == 0 || oper->num_children == 1);
      if (oper->num_children == 1) {
         slang_substitute(A, &oper->children[0],
                          substCount, substOld, substNew, GL_FALSE);
      }
      break;
#endif
   case slang_oper_assign:
   case slang_oper_subscript:
      /* special case:
       * child[0] can't have substitutions but child[1] can.
       */
      slang_substitute(A, &oper->children[0],
                       substCount, substOld, substNew, GL_TRUE);
      slang_substitute(A, &oper->children[1],
                       substCount, substOld, substNew, GL_FALSE);
      break;
   case slang_oper_field:
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

   /*assert(oper->type == slang_oper_call); (or (matrix) multiply, etc) */
   assert(fun->param_count == totalArgs);

   /* allocate temporary arrays */
   paramMode = (ParamMode *)
      _mesa_calloc(totalArgs * sizeof(ParamMode));
   substOld = (slang_variable **)
      _mesa_calloc(totalArgs * sizeof(slang_variable *));
   substNew = (slang_operation **)
      _mesa_calloc(totalArgs * sizeof(slang_operation *));

   printf("\nInline call to %s  (total vars=%d  nparams=%d)\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);


   if (haveRetValue && !returnOper) {
      /* Create comma sequence for inlined code, the left child will be the
       * function body and the right child will be a variable (__retVal)
       * that will get the return value.
       */
      slang_operation *commaSeq;
      slang_operation *declOper = NULL;
      slang_variable *resultVar;

      commaSeq = slang_operation_new(1);
      commaSeq->type = slang_oper_sequence;
      assert(commaSeq->locals);
      commaSeq->locals->outer_scope = oper->locals->outer_scope;
      commaSeq->num_children = 3;
      commaSeq->children = slang_operation_new(3);
      /* allocate the return var */
      resultVar = slang_variable_scope_grow(commaSeq->locals);
      /*
      printf("ALLOC __retVal from scope %p\n", (void*) commaSeq->locals);
      */
      printf("Alloc __resultTemp in scope %p for retval of calling %s\n",
             (void*)commaSeq->locals, (char *) fun->header.a_name);

      resultVar->a_name = slang_atom_pool_atom(A->atoms, "__resultTmp");
      resultVar->type = fun->header.type; /* XXX copy? */
      /*resultVar->type.qualifier = slang_qual_out;*/

      /* child[0] = __resultTmp declaration */
      declOper = &commaSeq->children[0];
      declOper->type = slang_oper_variable_decl;
      declOper->a_id = resultVar->a_name;
      declOper->locals->outer_scope = commaSeq->locals; /*** ??? **/

      /* child[1] = function body */
      inlined = &commaSeq->children[1];
      /* XXXX this may be inappropriate!!!!: */
      inlined->locals->outer_scope = commaSeq->locals;

      /* child[2] = __resultTmp reference */
      returnOper = &commaSeq->children[2];
      returnOper->type = slang_oper_identifier;
      returnOper->a_id = resultVar->a_name;
      returnOper->locals->outer_scope = commaSeq->locals;
      declOper->locals->outer_scope = commaSeq->locals;

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
      slang_variable *p = &fun->parameters->variables[i];
      printf("Param %d: %s %s \n", i,
             slang_type_qual_string(p->type.qualifier),
	     (char *) p->a_name);
      if (p->type.qualifier == slang_qual_inout ||
	  p->type.qualifier == slang_qual_out) {
	 /* an output param */
         slang_operation *arg;
         if (i < numArgs)
            arg = &args[i];
         else
            arg = returnOper;
	 paramMode[i] = SUBST;
	 assert(arg->type == slang_oper_identifier
		/*||arg->type == slang_oper_variable_decl*/);
         slang_resolve_variable(arg);
         /* replace parameter 'p' with argument 'arg' */
	 substOld[substCount] = p;
	 substNew[substCount] = arg; /* will get copied */
	 substCount++;
      }
      else if (p->type.qualifier == slang_qual_const) {
	 /* a constant input param */
	 if (args[i].type == slang_oper_identifier ||
	     args[i].type == slang_oper_literal_float) {
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

#if 00
   printf("ABOUT to inline body %p with checksum %d\n",
          (char *) fun->body, slang_checksum_tree(fun->body));
#endif

   /* actual code inlining: */
   slang_operation_copy(inlined, fun->body);

#if 000
   printf("======================= orig body code ======================\n");
   printf("=== params scope = %p\n", (void*) fun->parameters);
   slang_print_tree(fun->body, 8);
   printf("======================= copied code =========================\n");
   slang_print_tree(inlined, 8);
#endif

   /* do parameter substitution in inlined code: */
   slang_substitute(A, inlined, substCount, substOld, substNew, GL_FALSE);

#if 000
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
	 slang_variable *p = &fun->parameters->variables[i];
	 /* declare parameter 'p' */
	 slang_operation *decl = slang_operation_insert(&inlined->num_children,
							&inlined->children,
							numCopyIn);
         printf("COPY_IN %s from expr\n", (char*)p->a_name);
	 decl->type = slang_oper_variable_decl;
         assert(decl->locals);
	 decl->locals = fun->parameters;
	 decl->a_id = p->a_name;
	 decl->num_children = 1;
	 decl->children = slang_operation_new(1);

         /* child[0] is the var's initializer */
         slang_operation_copy(&decl->children[0], args + i);

	 numCopyIn++;
      }
   }

   /* New epilog statements:
    * 1. Create end of function label to jump to from return statements.
    * 2. Copy the 'out' parameter vars
    */
   {
      slang_operation *lab = slang_operation_insert(&inlined->num_children,
                                                    &inlined->children,
                                                    inlined->num_children);
      lab->type = slang_oper_label;
      lab->a_id = slang_atom_pool_atom(A->atoms, CurFunction->end_label);
   }

   for (i = 0; i < totalArgs; i++) {
      if (paramMode[i] == COPY_OUT) {
	 const slang_variable *p = &fun->parameters->variables[i];
	 /* actualCallVar = outParam */
	 /*if (i > 0 || !haveRetValue)*/
	 slang_operation *ass = slang_operation_insert(&inlined->num_children,
						       &inlined->children,
						       inlined->num_children);
	 ass->type = slang_oper_assign;
	 ass->num_children = 2;
	 ass->locals = _slang_variable_scope_new(inlined->locals);
	 assert(ass->locals);
	 ass->children = slang_operation_new(2);
	 ass->children[0] = args[i]; /*XXX copy */
	 ass->children[1].type = slang_oper_identifier;
	 ass->children[1].a_id = p->a_name;
	 ass->children[1].locals = _slang_variable_scope_new(ass->locals);
      }
   }

   _mesa_free(paramMode);
   _mesa_free(substOld);
   _mesa_free(substNew);

   printf("Done Inline call to %s  (total vars=%d  nparams=%d)\n",
	  (char *) fun->header.a_name,
	  fun->parameters->num_variables, numArgs);

   return top;
}


static slang_ir_node *
_slang_gen_function_call(slang_assemble_ctx *A, slang_function *fun,
			     slang_operation *oper, slang_operation *dest)
{
   slang_ir_node *n;
   slang_operation *inlined;
   slang_function *prevFunc;

   prevFunc = CurFunction;
   CurFunction = fun;

   if (!CurFunction->end_label) {
      char name[200];
      sprintf(name, "__endOfFunc_%s_", (char *) CurFunction->header.a_name);
      CurFunction->end_label = slang_atom_pool_gen(A->atoms, name);
   }

   if (slang_is_asm_function(fun) && !dest) {
      /* assemble assembly function - tree style */
      inlined = slang_inline_asm_function(A, fun, oper);
   }
   else {
      /* non-assembly function */
      inlined = slang_inline_function_call(A, fun, oper, dest);
   }

   /* Replace the function call with the inlined block */
#if 0
   slang_operation_construct(oper);
   slang_operation_copy(oper, inlined);
#else
   *oper = *inlined;
#endif


#if 0
   assert(inlined->locals);
   printf("*** Inlined code for call to %s:\n",
          (char*) fun->header.a_name);

   slang_print_tree(oper, 10);
   printf("\n");
#endif

   /* assemble what we just made XXX here??? */
   n = _slang_gen_operation(A, oper);

   CurFunction = prevFunc;

   return n;
}


static slang_asm_info *
slang_find_asm_info(const char *name)
{
   GLuint i;
   for (i = 0; AsmInfo[i].Name; i++) {
      if (_mesa_strcmp(AsmInfo[i].Name, name) == 0) {
         return AsmInfo + i;
      }
   }
   return NULL;
}


static GLuint
make_writemask(char *field)
{
   GLuint mask = 0x0;
   while (*field) {
      switch (*field) {
      case 'x':
         mask |= WRITEMASK_X;
         break;
      case 'y':
         mask |= WRITEMASK_Y;
         break;
      case 'z':
         mask |= WRITEMASK_Z;
         break;
      case 'w':
         mask |= WRITEMASK_W;
         break;
      default:
         abort();
      }
      field++;
   }
   if (mask == 0x0)
      return WRITEMASK_XYZW;
   else
      return mask;
}


/**
 * Generate IR tree for an asm instruction/operation such as:
 *    __asm vec4_dot __retVal.x, v1, v2;
 */
static slang_ir_node *
_slang_gen_asm(slang_assemble_ctx *A, slang_operation *oper,
                   slang_operation *dest)
{
   const slang_asm_info *info;
   slang_ir_node *kids[2], *n;
   GLuint j, firstOperand;

   assert(oper->type == slang_oper_asm);

   info = slang_find_asm_info((char *) oper->a_id);
   assert(info);
   assert(info->NumParams <= 2);

   if (info->NumParams == oper->num_children) {
      /* storage for result not specified */
      firstOperand = 0;
   }
   else {
      /* storage for result (child[0]) is specified */
      firstOperand = 1;
   }

   /* assemble child(ren) */
   kids[0] = kids[1] = NULL;
   for (j = 0; j < info->NumParams; j++) {
      kids[j] = _slang_gen_operation(A, &oper->children[firstOperand + j]);
   }

   n = new_node(info->Opcode, kids[0], kids[1]);

   if (firstOperand) {
      /* Setup n->Store to be a particular location.  Otherwise, storage
       * for the result (a temporary) will be allocated later.
       */
      GLuint writemask = WRITEMASK_XYZW;
      slang_operation *dest_oper;
      slang_ir_node *n0;

      dest_oper = &oper->children[0];
      if (dest_oper->type == slang_oper_field) {
         /* writemask */
         writemask = make_writemask((char*) dest_oper->a_id);
         dest_oper = &dest_oper->children[0];
      }

      assert(dest_oper->type == slang_oper_identifier);
      n0 = _slang_gen_operation(A, dest_oper);
      assert(n0->Var);
      assert(n0->Store);

      n->Store = n0->Store;
      n->Writemask = writemask;

      free(n0);
   }

   return n;
}



static GLboolean
_slang_is_noop(const slang_operation *oper)
{
   if (!oper ||
       oper->type == slang_oper_void ||
       (oper->num_children == 1 && oper->children[0].type == slang_oper_void))
      return GL_TRUE;
   else
      return GL_FALSE;
}


static slang_ir_node *
_slang_gen_cond(slang_ir_node *n)
{
   slang_ir_node *c = new_node(IR_COND, n, NULL);
   return c;
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

   atom = slang_atom_pool_atom(A->atoms, name);
   if (atom == SLANG_ATOM_NULL)
      return NULL;

   fun = _slang_locate_function(A->space.funcs, atom, params, param_count,
				&A->space, A->atoms);
   if (!fun) {
      RETURN_ERROR2("Undefined function", name, 0);
   }

   return _slang_gen_function_call(A, fun, oper, dest);
}


/**
 * Generate IR tree for a while-loop.
 */
static slang_ir_node *
_slang_gen_while(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * label "__startWhile"
    * eval expr (child[0]), updating condcodes
    * branch if false to "__endWhile"
    * code body
    * jump "__startWhile"
    * label "__endWhile"
    */
   slang_atom startAtom = slang_atom_pool_gen(A->atoms, "__startWhile");
   slang_atom endAtom = slang_atom_pool_gen(A->atoms, "__endWhile");
   slang_ir_node *startLab, *cond, *bra, *body, *jump, *endLab, *tree;
   slang_atom prevLoopBreak = CurLoopBreak;
   slang_atom prevLoopCont = CurLoopCont;

   /* Push this loop */
   CurLoopBreak = endAtom;
   CurLoopCont = startAtom;

   startLab = new_label(startAtom);
   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = _slang_gen_cond(cond);
   tree = new_seq(startLab, cond);

   bra = new_cjump(endAtom);
   tree = new_seq(tree, bra);

   body = _slang_gen_operation(A, &oper->children[1]);
   tree = new_seq(tree, body);

   jump = new_jump(startAtom);
   tree = new_seq(tree, jump);

   endLab = new_label(endAtom);
   tree = new_seq(tree, endLab);

   /* Pop this loop */
   CurLoopBreak = prevLoopBreak;
   CurLoopCont = prevLoopCont;

   return tree;
}


/**
 * Generate IR tree for a for-loop.
 */
static slang_ir_node *
_slang_gen_for(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * init code (child[0])
    * label "__startFor"
    * eval expr (child[1]), updating condcodes
    * branch if false to "__endFor"
    * code body (child[3])
    * label "__continueFor"
    * incr code (child[2])
    * jump "__startFor"
    * label "__endFor"
    */
   slang_atom startAtom = slang_atom_pool_gen(A->atoms, "__startFor");
   slang_atom contAtom = slang_atom_pool_gen(A->atoms, "__continueFor");
   slang_atom endAtom = slang_atom_pool_gen(A->atoms, "__endFor");
   slang_ir_node *init, *startLab, *cond, *bra, *body, *contLab;
   slang_ir_node *incr, *jump, *endLab, *tree;
   slang_atom prevLoopBreak = CurLoopBreak;
   slang_atom prevLoopCont = CurLoopCont;

   /* Push this loop */
   CurLoopBreak = endAtom;
   CurLoopCont = contAtom;

   init = _slang_gen_operation(A, &oper->children[0]);
   startLab = new_label(startAtom);
   tree = new_seq(init, startLab);

   cond = _slang_gen_operation(A, &oper->children[1]);
   cond = _slang_gen_cond(cond);
   tree = new_seq(tree, cond);

   bra = new_cjump(endAtom);
   tree = new_seq(tree, bra);

   body = _slang_gen_operation(A, &oper->children[3]);
   tree = new_seq(tree, body);

   contLab = new_label(contAtom);
   tree = new_seq(tree, contLab);

   incr = _slang_gen_operation(A, &oper->children[2]);
   tree = new_seq(tree, incr);

   jump = new_jump(startAtom);
   tree = new_seq(tree, jump);

   endLab = new_label(endAtom);
   tree = new_seq(tree, endLab);

   /* Pop this loop */
   CurLoopBreak = prevLoopBreak;
   CurLoopCont = prevLoopCont;

   return tree;
}


/**
 * Generate IR tree for an if/then/else conditional.
 */
static slang_ir_node *
_slang_gen_if(slang_assemble_ctx * A, const slang_operation *oper)
{
   /*
    * eval expr (child[0]), updating condcodes
    * branch if false to _else or _endif
    * "true" code block
    * if haveElseClause clause:
    *    jump "__endif"
    *    label "__else"
    *    "false" code block
    * label "__endif"
    */
   const GLboolean haveElseClause = !_slang_is_noop(&oper->children[2]);
   slang_ir_node *cond, *bra, *trueBody, *endifLab, *tree;
   slang_atom elseAtom = slang_atom_pool_gen(A->atoms, "__else");
   slang_atom endifAtom = slang_atom_pool_gen(A->atoms, "__endif");

   cond = _slang_gen_operation(A, &oper->children[0]);
   cond = _slang_gen_cond(cond);
   /*assert(cond->Store);*/
   bra = new_cjump(haveElseClause ? elseAtom : endifAtom);
   tree = new_seq(cond, bra);

   trueBody = _slang_gen_operation(A, &oper->children[1]);
   tree = new_seq(tree, trueBody);

   if (haveElseClause) {
      /* else clause */
      slang_ir_node *jump, *elseLab, *falseBody;
      jump = new_jump(endifAtom);
      tree = new_seq(tree, jump);

      elseLab = new_label(elseAtom);
      tree = new_seq(tree, elseLab);

      falseBody = _slang_gen_operation(A, &oper->children[2]);
      tree = new_seq(tree, falseBody);
   }

   endifLab = new_label(endifAtom);
   tree = new_seq(tree, endifLab);

   return tree;
}


/**
 * Generate IR tree for a return statement.
 */
static slang_ir_node *
_slang_gen_return(slang_assemble_ctx * A, slang_operation *oper)
{
   if (oper->num_children == 0 ||
       (oper->num_children == 1 &&
        oper->children[0].type == slang_oper_void)) {
      /* Convert from:
       *   return;
       * To:
       *   goto __endOfFunction;
       */
      slang_ir_node *n;
      slang_operation gotoOp;
      slang_operation_construct(&gotoOp);
      gotoOp.type = slang_oper_goto;
      gotoOp.a_id = slang_atom_pool_atom(A->atoms, CurFunction->end_label);
      /* assemble the new code */
      n = _slang_gen_operation(A, &gotoOp);
      /* destroy temp code */
      slang_operation_destruct(&gotoOp);
      return n;
   }
   else {
      /*
       * Convert from:
       *   return expr;
       * To:
       *   __retVal = expr;
       *   goto __endOfFunction;
       */
      slang_operation *block, *assign, *jump;
      slang_atom a_retVal;
      slang_ir_node *n;

      a_retVal = slang_atom_pool_atom(A->atoms, "__retVal");
      assert(a_retVal);

#if 1 /* DEBUG */
      {
         slang_variable *v
            = _slang_locate_variable(oper->locals, a_retVal, GL_TRUE);
         assert(v);
      }
#endif

      block = slang_operation_new(1);
      block->type = slang_oper_block_no_new_scope;
      block->num_children = 2;
      block->children = slang_operation_new(2);
      assert(block->locals);
      block->locals->outer_scope = oper->locals->outer_scope;

      /* child[0]: __retVal = expr; */
      assign = &block->children[0];
      assign->type = slang_oper_assign;
      assign->locals->outer_scope = block->locals;
      assign->num_children = 2;
      assign->children = slang_operation_new(2);
      /* lhs (__retVal) */
      assign->children[0].type = slang_oper_identifier;
      assign->children[0].a_id = a_retVal;
      assign->children[0].locals->outer_scope = assign->locals;
      /* rhs (expr) */
      /* XXX we might be able to avoid this copy someday */
      slang_operation_copy(&assign->children[1], &oper->children[0]);

      /* child[1]: goto __endOfFunction */
      jump = &block->children[1];
      jump->type = slang_oper_goto;
      assert(CurFunction->end_label);
      jump->a_id = slang_atom_pool_atom(A->atoms, CurFunction->end_label);

#if 0 /* debug */
      printf("NEW RETURN:\n");
      slang_print_tree(block, 0);
#endif

      /* assemble the new code */
      n = _slang_gen_operation(A, block);
      slang_operation_delete(block);
      return n;
   }
}


/**
 * Generate IR tree for a variable declaration.
 */
static slang_ir_node *
_slang_gen_declaration(slang_assemble_ctx *A, slang_operation *oper)
{
   slang_ir_node *n;
   slang_ir_node *varDecl;
   slang_variable *v;

   assert(oper->num_children == 0 || oper->num_children == 1);

   v = _slang_locate_variable(oper->locals, oper->a_id, GL_TRUE);
   assert(v);

   varDecl = new_node(IR_VAR_DECL, NULL, NULL);
   if (!varDecl)
      return NULL;

   varDecl->Var = v;
   v->declared = GL_TRUE;

   slang_resolve_storage(A->codegen, varDecl, A->program);

   if (oper->num_children > 0) {
      /* child is initializer */
      slang_ir_node *var, *init, *rhs;
      assert(oper->num_children == 1);
      var = new_var(A, oper, oper->a_id, SWIZZLE_NOOP);
      /* XXX make copy of this initializer? */
      /*
        printf("\n*** ASSEMBLE INITIALIZER %p\n", (void*) v->initializer);
      */
      rhs = _slang_gen_operation(A, &oper->children[0]);
      init = new_node(IR_MOVE, var, rhs);
      /*assert(rhs->Opcode != IR_SEQ);*/
      n = new_seq(varDecl, init);
   }
   else if (v->initializer) {
      slang_ir_node *var, *init, *rhs;
      var = new_var(A, oper, oper->a_id, SWIZZLE_NOOP);
      /* XXX make copy of this initializer? */
      /*
        printf("\n*** ASSEMBLE INITIALIZER %p\n", (void*) v->initializer);
      */
      rhs = _slang_gen_operation(A, v->initializer);
      init = new_node(IR_MOVE, var, rhs);
      /*
        assert(rhs->Opcode != IR_SEQ);
      */
      n = new_seq(varDecl, init);
   }
   else {
      n = varDecl;
   }
   return n;
}


/**
 * Generate IR tree for a variable (such as in an expression).
 */
static slang_ir_node *
_slang_gen_variable(slang_assemble_ctx * A, slang_operation *oper)
{
   /* If there's a variable associated with this oper (from inlining)
    * use it.  Otherwise, use the oper's var id.
    */
   slang_atom aVar = oper->var ? oper->var->a_name : oper->a_id;
   slang_ir_node *n = new_var(A, oper, aVar, SWIZZLE_NOOP);
   assert(oper->var);
   return n;
}


/**
 * Generate IR tree for an assignment (=).
 */
static slang_ir_node *
_slang_gen_assignment(slang_assemble_ctx * A, slang_operation *oper)
{
   if (oper->children[0].type == slang_oper_identifier &&
       oper->children[1].type == slang_oper_call) {
      /* Sspecial case of:  x = f(a, b)
       * Replace with f(a, b, x)  (where x == hidden __retVal out param)
       */
      slang_ir_node *n;
      n = _slang_gen_function_call_name(A,
                                      (const char *) oper->children[1].a_id,
                                      &oper->children[1], &oper->children[0]);
      return n;
   }
   else {
      slang_operation *lhs = &oper->children[0];
      slang_ir_node *n, *c0, *c1;
      GLuint mask = WRITEMASK_XYZW;
      if (lhs->type == slang_oper_field) {
         /* XXXX this is a hack! */
         /* writemask */
         if (!slang_is_writemask((char *) lhs->a_id, &mask))
            mask = WRITEMASK_XYZW;
         lhs = &lhs->children[0];
      }
      c0 = _slang_gen_operation(A, lhs);
      c1 = _slang_gen_operation(A, &oper->children[1]);

      n = new_node(IR_MOVE, c0, c1);
      /*
        assert(c1->Opcode != IR_SEQ);
      */
      if (c0->Writemask != WRITEMASK_XYZW)
         /* XXX this is a hack! */
         n->Writemask = c0->Writemask;
      else
         n->Writemask = mask;
      return n;
   }
}


/**
 * Generate IR tree for referencing a field in a struct (or basic vector type)
 */
static slang_ir_node *
_slang_gen_field(slang_assemble_ctx * A, slang_operation *oper)
{
   slang_assembly_typeinfo ti;

   slang_assembly_typeinfo_construct(&ti);
   _slang_typeof_operation(A, &oper->children[0], &ti);

   if (_slang_type_is_vector(ti.spec.type)) {
      /* the field should be a swizzle */
      const GLuint rows = _slang_type_dim(ti.spec.type);
      slang_swizzle swz;
      slang_ir_node *n;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         RETURN_ERROR("Bad swizzle", 0);
      }
      n = _slang_gen_operation(A, &oper->children[0]);
      n->Swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                                 swz.swizzle[1],
                                 swz.swizzle[2],
                                 swz.swizzle[3]);
      return n;
   }
   else if (ti.spec.type == slang_spec_float) {
      const GLuint rows = 1;
      slang_swizzle swz;
      slang_ir_node *n;
      if (!_slang_is_swizzle((char *) oper->a_id, rows, &swz)) {
         RETURN_ERROR("Bad swizzle", 0);
      }
      n = _slang_gen_operation(A, &oper->children[0]);
      n->Swizzle = MAKE_SWIZZLE4(swz.swizzle[0],
                                 swz.swizzle[1],
                                 swz.swizzle[2],
                                 swz.swizzle[3]);
      return n;
   }
   else {
      /* the field is a structure member */
      abort();
   }
}


/**
 * Generate IR tree for an array element reference.
 */
static slang_ir_node *
_slang_gen_subscript(slang_assemble_ctx * A, slang_operation *oper)
{
   if (oper->children[1].type == slang_oper_literal_int) {
      /* compile-time constant index - OK */
      slang_assembly_typeinfo array_ti, elem_ti;
      slang_ir_node *base;
      GLint index;

      /* get type of array element */
      slang_assembly_typeinfo_construct(&elem_ti);
      _slang_typeof_operation(A, oper, &elem_ti);

      /* get type of array */
      slang_assembly_typeinfo_construct(&array_ti);
      _slang_typeof_operation(A, &oper->children[0], &array_ti);


      base = _slang_gen_operation(A, &oper->children[0]);
      assert(base->Opcode == IR_VAR);
      assert(base->Store);

      index = (GLint) oper->children[1].literal[0];
      /*printf("element[%d]\n", index);*/
      /* new storage info since we don't want to change the original */
      base->Store = _slang_clone_ir_storage(base->Store);
      if (_slang_type_is_vector(array_ti.spec.type)) {
         /* scalar element (float) of a basic vector (vec3) */
         const GLuint max = _slang_type_dim(array_ti.spec.type);
         if (index >= max) {
            RETURN_ERROR("array index out of bounds", 0);
         }
         assert(index < 4);
         /* use swizzle to access the element */
         base->Swizzle = SWIZZLE_X + index;
         base->Writemask = WRITEMASK_X << index;
      }
      else {
         /* bias Index by array subscript, update storage size */
         base->Store->Index += index;
         base->Store->Size = _slang_sizeof_type_specifier(&elem_ti.spec);
      }
      return base;
   }
   else {
      /* run-time index - not supported yet - TBD */
      abort();
      return NULL;
   }
}


/**
 * Generate IR tree for a slang_operation (AST node)
 */
static slang_ir_node *
_slang_gen_operation(slang_assemble_ctx * A, slang_operation *oper)
{
   switch (oper->type) {
   case slang_oper_block_no_new_scope:
   case slang_oper_block_new_scope:
      /* list of operations */
      assert(oper->num_children > 0);
      {
         slang_ir_node *n, *tree = NULL;
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            n = _slang_gen_operation(A, &oper->children[i]);
            if (!n)
               return NULL; /* error must have occured */
            tree = tree ? new_seq(tree, n) : n;
         }
         return tree;
      }
      break;
   case slang_oper_expression:
      return _slang_gen_operation(A, &oper->children[0]);
      break;
   case slang_oper_while:
      return _slang_gen_while(A, oper);
   case slang_oper_for:
      return _slang_gen_for(A, oper);
   case slang_oper_break:
      if (!CurLoopBreak) {
         RETURN_ERROR("'break' not in loop", 0);
      }
      return new_jump(CurLoopBreak);
   case slang_oper_continue:
      if (!CurLoopCont) {
         RETURN_ERROR("'continue' not in loop", 0);
      }
      return new_jump(CurLoopCont);
   case slang_oper_equal:
      return new_node(IR_SEQUAL,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_notequal:
      return new_node(IR_SNEQUAL,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_greater:
      return new_node(IR_SGT,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_less:
      /* child[0] < child[1]  ---->   child[1] > child[0] */
#if 0
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
         /* XXX tranpose children */
	 n = _slang_gen_function_call_name(A, "<", oper, NULL);
	 return n;
      }
#else
      /** the operands must be ints or floats, not vectors */
      return new_node(IR_SGT,
                      _slang_gen_operation(A, &oper->children[1]),
                      _slang_gen_operation(A, &oper->children[0]));
#endif
   case slang_oper_greaterequal:
      return new_node(IR_SGE,
                      _slang_gen_operation(A, &oper->children[0]),
                      _slang_gen_operation(A, &oper->children[1]));
   case slang_oper_lessequal:
      /* child[0] <= child[1]  ---->   child[1] >= child[0] */
      return new_node(IR_SGE,
                      _slang_gen_operation(A, &oper->children[1]),
                      _slang_gen_operation(A, &oper->children[0]));
   case slang_oper_add:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+", oper, NULL);
	 return n;
      }
   case slang_oper_subtract:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case slang_oper_multiply:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
         n = _slang_gen_function_call_name(A, "*", oper, NULL);
	 return n;
      }
   case slang_oper_divide:
      {
         slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/", oper, NULL);
	 return n;
      }
   case slang_oper_minus:
      {
         slang_ir_node *n;
         assert(oper->num_children == 1);
	 n = _slang_gen_function_call_name(A, "-", oper, NULL);
	 return n;
      }
   case slang_oper_plus:
      /* +expr   --> do nothing */
      return _slang_gen_operation(A, &oper->children[0]);
   case slang_oper_variable_decl:
      return _slang_gen_declaration(A, oper);
   case slang_oper_assign:
      return _slang_gen_assignment(A, oper);
   case slang_oper_addassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "+=", oper, NULL);
         /* The result of this operation should be stored back into child[0] */
         assert(n->Children[0]->Store);
         n->Store = n->Children[0]->Store;
	 return n;
      }
   case slang_oper_subassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "-=", oper, NULL);
         /* The result of this operation should be stored back into child[0] */
         assert(n->Children[0]->Store);
         n->Store = n->Children[0]->Store;
	 return n;
      }
      break;
   case slang_oper_mulassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "*=", oper, NULL);
         /* The result of this operation should be stored back into child[0] */
         assert(n->Children[0]->Store);
         n->Store = n->Children[0]->Store;
	 return n;
      }
   case slang_oper_divassign:
      {
	 slang_ir_node *n;
         assert(oper->num_children == 2);
	 n = _slang_gen_function_call_name(A, "/=", oper, NULL);
         /* The result of this operation should be stored back into child[0] */
         assert(n->Children[0]->Store);
         n->Store = n->Children[0]->Store;
	 return n;
      }
   case slang_oper_asm:
      return _slang_gen_asm(A, oper, NULL);
   case slang_oper_call:
      return _slang_gen_function_call_name(A, (const char *) oper->a_id,
                                           oper, NULL);
   case slang_oper_return:
      return _slang_gen_return(A, oper);
   case slang_oper_goto:
      return new_jump((char*) oper->a_id);
   case slang_oper_label:
      return new_label((char*) oper->a_id);
   case slang_oper_identifier:
      return _slang_gen_variable(A, oper);
   case slang_oper_if:
      return _slang_gen_if(A, oper);
   case slang_oper_field:
      return _slang_gen_field(A, oper);
   case slang_oper_subscript:
      return _slang_gen_subscript(A, oper);
   case slang_oper_literal_float:
      return new_float_literal(oper->literal[0], oper->literal[1],
                               oper->literal[2], oper->literal[3]);
   case slang_oper_literal_int:
      return new_float_literal(oper->literal[0], 0, 0, 0);
   case slang_oper_literal_bool:
      return new_float_literal(oper->literal[0], 0, 0, 0);
   case slang_oper_postincrement:
      /* XXX not 100% about this */
      {
	 slang_ir_node *var = _slang_gen_operation(A, &oper->children[0]);
	 slang_ir_node *one = new_float_literal(1.0, 1.0, 1.0, 1.0);
	 slang_ir_node *sum = new_node(IR_ADD, var, one);
	 slang_ir_node *assign = new_node(IR_MOVE, var, sum);
         assert(sum->Opcode != IR_SEQ);
	 return assign;
      }
      break;
   case slang_oper_sequence:
      {
         slang_ir_node *tree = NULL;
         GLuint i;
         for (i = 0; i < oper->num_children; i++) {
            slang_ir_node *n = _slang_gen_operation(A, &oper->children[i]);
            tree = tree ? new_seq(tree, n) : n;
         }
         return tree;
      }
      break;
   case slang_oper_none:
      return NULL;
   default:
      printf("Unhandled node type %d\n", oper->type);
      abort();
      return new_node(IR_NOP, NULL, NULL);
   }
      abort();
   return NULL;
}


/**
 * Produce an IR tree from a function AST.
 * Then call the code emitter to convert the IR tree into a gl_program.
 */
struct slang_ir_node_ *
_slang_codegen_function(slang_assemble_ctx * A, slang_function * fun)
{
   slang_ir_node *n, *endLabel;

   if (_mesa_strcmp((char *) fun->header.a_name, "main") != 0 &&
       _mesa_strcmp((char *) fun->header.a_name, "foo") != 0 &&
       _mesa_strcmp((char *) fun->header.a_name, "bar") != 0)
     return 0;

   printf("\n*********** Assemble function2(%s)\n", (char*)fun->header.a_name);
#if 1
   slang_print_function(fun, 1);
#endif

   /* should have been allocated earlier: */
   assert(A->program->Parameters );
   assert(A->program->Varying);

   A->codegen = _slang_new_codegen_context();

   /*printf("** Begin Simplify\n");*/
   slang_simplify(fun->body, &A->space, A->atoms);
   /*printf("** End Simplify\n");*/

   CurFunction = fun;

   if (!CurFunction->end_label)
      CurFunction->end_label = slang_atom_pool_gen(A->atoms, "__endOfFunc_main_");

   n = _slang_gen_operation(A, fun->body);

   if (n) {
      endLabel = new_label(fun->end_label);
      n = new_seq(n, endLabel);
   }

   CurFunction = NULL;


#if 1
   printf("************* New body for %s *****\n", (char*)fun->header.a_name);
   slang_print_function(fun, 1);

   printf("************* IR for %s *******\n", (char*)fun->header.a_name);
   slang_print_ir(n, 0);
   printf("************* End assemble function2 ************\n\n");
#endif

   if (_mesa_strcmp((char*) fun->header.a_name, "main") == 0) {
      _slang_emit_code(n, A->codegen, A->program);
   }

   return n;
}


static GLint
sampler_to_texture_index(const slang_type_specifier_type type)
{
   switch (type) {
   case slang_spec_sampler1D:
      return TEXTURE_1D_INDEX;
   case slang_spec_sampler2D:
      return TEXTURE_2D_INDEX;
   case slang_spec_sampler3D:
      return TEXTURE_3D_INDEX;
   case slang_spec_samplerCube:
      return TEXTURE_CUBE_INDEX;
   case slang_spec_sampler1DShadow:
      return TEXTURE_1D_INDEX; /* XXX fix */
   case slang_spec_sampler2DShadow:
      return TEXTURE_2D_INDEX; /* XXX fix */
   default:
      return -1;
   }
}



static GLint
slang_alloc_sampler(struct gl_program *prog, const char *name)
{
   GLint i = _mesa_add_sampler(prog->Parameters, name);
   return i;
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
void
_slang_codegen_global_variable(slang_variable *var, struct gl_program *prog)
{
   GLint texIndex;
   slang_ir_storage *store = NULL;

   texIndex = sampler_to_texture_index(var->type.specifier.type);

   if (texIndex != -1) {
      /* Texture sampler:
       * store->File = PROGRAM_SAMPLER
       * store->Index = sampler uniform location
       * store->Size = texture type index (1D, 2D, 3D, cube, etc)
       */
      GLint samplerUniform = slang_alloc_sampler(prog, (char *) var->a_name);
      store = _slang_new_ir_storage(PROGRAM_SAMPLER, samplerUniform, texIndex);
      printf("SAMPLER ");
   }
   else if (var->type.qualifier == slang_qual_uniform) {
      printf("UNIFORM ");
   }

   printf("CODEGEN VAR %s\n", (char*) var->a_name);

   assert(!var->aux);
#if 1
   var->aux = store;
#endif
   /**
      XXX allocate variable storage (aux), at least the register file.
    */
}
