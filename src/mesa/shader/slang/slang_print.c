
/**
 * Dump/print a slang_operation tree
 */


#include "imports.h"
#include "slang_compile.h"
#include "slang_print.h"


static void
spaces(int n)
{
   while (n--)
      printf(" ");
}


static void
print_type(const slang_fully_specified_type *t)
{
   switch (t->qualifier) {
   case slang_qual_none:
      /*printf("");*/
      break;
   case slang_qual_const:
      printf("const ");
      break;
   case slang_qual_attribute:
      printf("attrib ");
      break;
   case slang_qual_varying:
      printf("varying ");
      break;
   case slang_qual_uniform:
      printf("uniform ");
      break;
   case slang_qual_out:
      printf("output ");
      break;
   case slang_qual_inout:
      printf("inout ");
      break;
   case slang_qual_fixedoutput:
      printf("fixedoutput");
      break;
   case slang_qual_fixedinput:
      printf("fixedinput");
      break;
   default:
      printf("unknown qualifer!");
   }

   switch (t->specifier.type) {
   case slang_spec_void:
      printf("void");
      break;
   case slang_spec_bool:
      printf("bool");
      break;
   case slang_spec_bvec2:
      printf("bvec2");
      break;
   case slang_spec_bvec3:
      printf("bvec3");
      break;
   case slang_spec_bvec4:
      printf("bvec4");
      break;
   case slang_spec_int:
      printf("int");
      break;
   case slang_spec_ivec2:
      printf("ivec2");
      break;
   case slang_spec_ivec3:
      printf("ivec3");
      break;
   case slang_spec_ivec4:
      printf("ivec4");
      break;
   case slang_spec_float:
      printf("float");
      break;
   case slang_spec_vec2:
      printf("vec2");
      break;
   case slang_spec_vec3:
      printf("vec3");
      break;
   case slang_spec_vec4:
      printf("vec4");
      break;
   case slang_spec_mat2:
      printf("mat2");
      break;
   case slang_spec_mat3:
      printf("mat3");
      break;
   case slang_spec_mat4:
      printf("mat4");
      break;
   case slang_spec_sampler1D:
      printf("sampler1D");
      break;
   case slang_spec_sampler2D:
      printf("sampler2D");
      break;
   case slang_spec_sampler3D:
      printf("sampler3D");
      break;
   case slang_spec_samplerCube:
      printf("samplerCube");
      break;
   case slang_spec_sampler1DShadow:
      printf("sampler1DShadow");
      break;
   case slang_spec_sampler2DShadow:
      printf("sampler2DShadow");
      break;
   case slang_spec_struct:
      printf("struct");
      break;
   case slang_spec_array:
      printf("array");
      break;
   default:
      printf("unknown type");
   }
   /*printf("\n");*/
}


static void
print_variable(const slang_variable *v, int indent)
{
   spaces(indent);
   printf("VAR ");
   print_type(&v->type);
   printf(" %s", (char *) v->a_name);
   if (v->initializer) {
      printf(" :=\n");
      slang_print_tree(v->initializer, indent + 3);
   }
   else {
      printf(";\n");
   }
}


static void
print_binary(const slang_operation *op, const char *oper, int indent)
{
   assert(op->num_children == 2);
   slang_print_tree(&op->children[0], indent + 3);
   spaces(indent);
   printf("%s\n", oper);
   slang_print_tree(&op->children[1], indent + 3);
}


static void
print_generic2(const slang_operation *op, const char *oper,
               const char *s, int indent)
{
   int i;
   if (oper) {
      spaces(indent);
      printf("[%p locals %p] %s %s\n", (void*) op, (void*) op->locals, oper, s);
   }
   for (i = 0; i < op->num_children; i++) {
      spaces(indent);
      printf("//child %d:\n", i);
      slang_print_tree(&op->children[i], indent);
   }
}

static void
print_generic(const slang_operation *op, const char *oper, int indent)
{
   print_generic2(op, oper, "", indent);
}


static const slang_variable_scope *
find_scope(const slang_variable_scope *s, slang_atom name)
{
   GLuint i;
   for (i = 0; i < s->num_variables; i++) {
      if (s->variables[i]->a_name == name)
         return s;
   }
   if (s->outer_scope)
      return find_scope(s->outer_scope, name);
   else
      return NULL;
}

static const slang_variable *
find_var(const slang_variable_scope *s, slang_atom name)
{
   GLuint i;
   for (i = 0; i < s->num_variables; i++) {
      if (s->variables[i]->a_name == name)
         return s->variables[i];
   }
   if (s->outer_scope)
      return find_var(s->outer_scope, name);
   else
      return NULL;
}


void
slang_print_tree(const slang_operation *op, int indent)
{
   int i;

   switch (op->type) {

   case slang_oper_none:
      spaces(indent);
      printf("slang_oper_none\n");
      break;

   case slang_oper_block_no_new_scope:
      spaces(indent);
      printf("{ locals %p  outer %p\n", (void*)op->locals, (void*)op->locals->outer_scope);
      print_generic(op, NULL, indent+3);
      spaces(indent);
      printf("}\n");
      break;

   case slang_oper_block_new_scope:
      spaces(indent);
      printf("{{ // new scope  locals %p\n", (void*)op->locals);
      print_generic(op, NULL, indent+3);
      spaces(indent);
      printf("}}\n");
      break;

   case slang_oper_variable_decl:
      assert(op->num_children == 0 || op->num_children == 1);
      {
         slang_variable *v;
         v = _slang_locate_variable(op->locals, op->a_id, GL_TRUE);
         if (v) {
            spaces(indent);
            printf("DECL (locals=%p outer=%p) ", (void*)op->locals, (void*) op->locals->outer_scope);
            print_type(&v->type);
            printf(" %s (%p)", (char *) op->a_id,
                   (void *) find_var(op->locals, op->a_id));

            printf(" (in scope %p) ",
                   (void *) find_scope(op->locals, op->a_id));
            if (op->num_children == 1) {
               printf(" :=\n");
               slang_print_tree(&op->children[0], indent + 3);
            }
            else if (v->initializer) {
               printf(" := INITIALIZER\n");
               slang_print_tree(v->initializer, indent + 3);
            }
            else {
               printf(";\n");
            }
            /*
            spaces(indent);
            printf("TYPE: ");
            print_type(&v->type);
            spaces(indent);
            printf("ADDR: %d  size: %d\n", v->address, v->size);
            */
         }
         else {
            abort();
            spaces(indent);
            printf("DECL %s (anonymous variable!!!!)\n", (char *) op->a_id);
            /*abort();*/
         }
      }
      break;

   case slang_oper_asm:
      spaces(indent);
      printf("ASM: %s\n", (char*) op->a_id);
      print_generic(op, NULL, indent+3);
      break;

   case slang_oper_break:
      spaces(indent);
      printf("BREAK\n");
      break;

   case slang_oper_continue:
      spaces(indent);
      printf("CONTINUE\n");
      break;

   case slang_oper_discard:
      spaces(indent);
      printf("DISCARD\n");
      break;

   case slang_oper_return:
      spaces(indent);
      printf("RETURN\n");
      if (op->num_children > 0)
         slang_print_tree(&op->children[0], indent + 3);
      break;

   case slang_oper_goto:
      spaces(indent);
      printf("GOTO %s\n", (char *) op->a_id);
      break;

   case slang_oper_label:
      spaces(indent);
      printf("LABEL %s\n", (char *) op->a_id);
      break;

   case slang_oper_expression:
      spaces(indent);
      printf("EXPR:  locals %p\n", (void*) op->locals);
      /*print_generic(op, "slang_oper_expression", indent);*/
      slang_print_tree(&op->children[0], indent + 3);
      break;

   case slang_oper_if:
      spaces(indent);
      printf("IF\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("THEN\n");
      slang_print_tree(&op->children[1], indent + 3);
      spaces(indent);
      printf("ELSE\n");
      slang_print_tree(&op->children[2], indent + 3);
      spaces(indent);
      printf("ENDIF\n");
      break;

   case slang_oper_while:
      assert(op->num_children == 2);
      spaces(indent);
      printf("WHILE cond:\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("WHILE body:\n");
      slang_print_tree(&op->children[1], indent + 3);
      break;

   case slang_oper_do:
      spaces(indent);
      printf("DO body:\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("DO cond:\n");
      slang_print_tree(&op->children[1], indent + 3);
      break;

   case slang_oper_for:
      spaces(indent);
      printf("FOR init:\n");
      slang_print_tree(&op->children[0], indent + 3);
      spaces(indent);
      printf("FOR while:\n");
      slang_print_tree(&op->children[1], indent + 3);
      spaces(indent);
      printf("FOR step:\n");
      slang_print_tree(&op->children[2], indent + 3);
      spaces(indent);
      printf("FOR body:\n");
      slang_print_tree(&op->children[3], indent + 3);
      spaces(indent);
      printf("ENDFOR\n");
      /*
      print_generic(op, "FOR", indent + 3);
      */
      break;

   case slang_oper_void:
      spaces(indent);
      printf("(oper-void)\n");
      break;

   case slang_oper_literal_bool:
      spaces(indent);
      /*printf("slang_oper_literal_bool\n");*/
      printf("%s\n", op->literal[0] ? "TRUE" : "FALSE");
      break;

   case slang_oper_literal_int:
      spaces(indent);
      /*printf("slang_oper_literal_int\n");*/
      printf("(%d %d %d %d)\n", (int) op->literal[0], (int) op->literal[1],
             (int) op->literal[2], (int) op->literal[3]);
      break;

   case slang_oper_literal_float:
      spaces(indent);
      /*printf("slang_oper_literal_float\n");*/
      printf("(%f %f %f %f)\n", op->literal[0], op->literal[1], op->literal[2],
             op->literal[3]);
      break;

   case slang_oper_identifier:
      spaces(indent);
      if (op->var && op->var->a_name)
         printf("VAR %s  (in scope %p)\n", (char *) op->var->a_name,
                (void *) find_scope(op->locals, op->a_id));
      else
         printf("VAR' %s  (in scope %p)\n", (char *) op->a_id,
                (void *) find_scope(op->locals, op->a_id));
      break;

   case slang_oper_sequence:
      print_generic(op, "COMMA-SEQ", indent+3);
      break;

   case slang_oper_assign:
      spaces(indent);
      printf("ASSIGNMENT  locals %p\n", (void*)op->locals);
      print_binary(op, ":=", indent);
      break;

   case slang_oper_addassign:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "+=", indent);
      break;

   case slang_oper_subassign:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "-=", indent);
      break;

   case slang_oper_mulassign:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "*=", indent);
      break;

   case slang_oper_divassign:
      spaces(indent);
      printf("ASSIGN\n");
      print_binary(op, "/=", indent);
      break;

	/*slang_oper_modassign,*/
	/*slang_oper_lshassign,*/
	/*slang_oper_rshassign,*/
	/*slang_oper_orassign,*/
	/*slang_oper_xorassign,*/
	/*slang_oper_andassign,*/
   case slang_oper_select:
      spaces(indent);
      printf("slang_oper_select n=%d\n", op->num_children);
      assert(op->num_children == 3);
      slang_print_tree(&op->children[0], indent+3);
      spaces(indent);
      printf("?\n");
      slang_print_tree(&op->children[1], indent+3);
      spaces(indent);
      printf(":\n");
      slang_print_tree(&op->children[2], indent+3);
      break;

   case slang_oper_logicalor:
      print_binary(op, "||", indent);
      break;

   case slang_oper_logicalxor:
      print_binary(op, "^^", indent);
      break;

   case slang_oper_logicaland:
      print_binary(op, "&&", indent);
      break;

   /*slang_oper_bitor*/
   /*slang_oper_bitxor*/
   /*slang_oper_bitand*/
   case slang_oper_equal:
      print_binary(op, "==", indent);
      break;

   case slang_oper_notequal:
      print_binary(op, "!=", indent);
      break;

   case slang_oper_less:
      print_binary(op, "<", indent);
      break;

   case slang_oper_greater:
      print_binary(op, ">", indent);
      break;

   case slang_oper_lessequal:
      print_binary(op, "<=", indent);
      break;

   case slang_oper_greaterequal:
      print_binary(op, ">=", indent);
      break;

   /*slang_oper_lshift*/
   /*slang_oper_rshift*/
   case slang_oper_add:
      print_binary(op, "+", indent);
      break;

   case slang_oper_subtract:
      print_binary(op, "-", indent);
      break;

   case slang_oper_multiply:
      print_binary(op, "*", indent);
      break;

   case slang_oper_divide:
      print_binary(op, "/", indent);
      break;

   /*slang_oper_modulus*/
   case slang_oper_preincrement:
      spaces(indent);
      printf("PRE++\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case slang_oper_predecrement:
      spaces(indent);
      printf("PRE--\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case slang_oper_plus:
      spaces(indent);
      printf("slang_oper_plus\n");
      break;

   case slang_oper_minus:
      spaces(indent);
      printf("slang_oper_minus\n");
      break;

   /*slang_oper_complement*/
   case slang_oper_not:
      spaces(indent);
      printf("NOT\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case slang_oper_subscript:
      spaces(indent);
      printf("slang_oper_subscript\n");
      print_generic(op, NULL, indent+3);
      break;

   case slang_oper_call:
#if 0
         slang_function *fun
            = _slang_locate_function(A->space.funcs, oper->a_id,
                                     oper->children,
                                     oper->num_children, &A->space, A->atoms);
#endif
      spaces(indent);
      printf("CALL %s(\n", (char *) op->a_id);
      for (i = 0; i < op->num_children; i++) {
         slang_print_tree(&op->children[i], indent+3);
         if (i + 1 < op->num_children) {
            spaces(indent + 3);
            printf(",\n");
         }
      }
      spaces(indent);
      printf(")\n");
      break;

   case slang_oper_field:
      spaces(indent);
      printf("FIELD %s of\n", (char*) op->a_id);
      slang_print_tree(&op->children[0], indent+3);
      break;

   case slang_oper_postincrement:
      spaces(indent);
      printf("POST++\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   case slang_oper_postdecrement:
      spaces(indent);
      printf("POST--\n");
      slang_print_tree(&op->children[0], indent+3);
      break;

   default:
      printf("unknown op->type %d\n", (int) op->type);
   }

}



void
slang_print_function(const slang_function *f, GLboolean body)
{
   int i;

#if 0
   if (_mesa_strcmp((char *) f->header.a_name, "main") != 0)
     return;
#endif

   printf("FUNCTION %s (\n",
          (char *) f->header.a_name);

   for (i = 0; i < f->param_count; i++) {
      print_variable(f->parameters->variables[i], 3);
   }

   printf(")\n");
   if (body && f->body)
      slang_print_tree(f->body, 0);
}





const char *
slang_type_qual_string(slang_type_qualifier q)
{
   switch (q) {
   case slang_qual_none:
      return "none";
   case slang_qual_const:
      return "const";
   case slang_qual_attribute:
      return "attribute";
   case slang_qual_varying:
      return "varying";
   case slang_qual_uniform:
      return "uniform";
   case slang_qual_out:
      return "out";
   case slang_qual_inout:
      return "inout";
   case slang_qual_fixedoutput:
      return "fixedoutput";
   case slang_qual_fixedinput:
      return "fixedinputk";
   default:
      return "qual?";
   }
}


static const char *
slang_type_string(slang_type_specifier_type t)
{
   switch (t) {
   case slang_spec_void:
      return "void";
   case slang_spec_bool:
      return "bool";
   case slang_spec_bvec2:
      return "bvec2";
   case slang_spec_bvec3:
      return "bvec3";
   case slang_spec_bvec4:
      return "bvec4";
   case slang_spec_int:
      return "int";
   case slang_spec_ivec2:
      return "ivec2";
   case slang_spec_ivec3:
      return "ivec3";
   case slang_spec_ivec4:
      return "ivec4";
   case slang_spec_float:
      return "float";
   case slang_spec_vec2:
      return "vec2";
   case slang_spec_vec3:
      return "vec3";
   case slang_spec_vec4:
      return "vec4";
   case slang_spec_mat2:
      return "mat2";
   case slang_spec_mat3:
      return "mat3";
   case slang_spec_mat4:
      return "mat4";
   case slang_spec_sampler1D:
      return "sampler1D";
   case slang_spec_sampler2D:
      return "sampler2D";
   case slang_spec_sampler3D:
      return "sampler3D";
   case slang_spec_samplerCube:
      return "samplerCube";
   case slang_spec_sampler1DShadow:
      return "sampler1DShadow";
   case slang_spec_sampler2DShadow:
      return "sampler2DShadow";
   case slang_spec_struct:
      return "struct";
   case slang_spec_array:
      return "array";
   default:
      return "type?";
   }
}


static const char *
slang_fq_type_string(const slang_fully_specified_type *t)
{
   static char str[1000];
   sprintf(str, "%s %s", slang_type_qual_string(t->qualifier),
      slang_type_string(t->specifier.type));
   return str;
}


void
slang_print_type(const slang_fully_specified_type *t)
{
   printf("%s %s", slang_type_qual_string(t->qualifier),
      slang_type_string(t->specifier.type));
}


#if 0
static char *
slang_var_string(const slang_variable *v)
{
   static char str[1000];
   sprintf(str, "%s : %s",
           (char *) v->a_name,
           slang_fq_type_string(&v->type));
   return str;
}
#endif


void
slang_print_variable(const slang_variable *v)
{
   printf("Name: %s\n", (char *) v->a_name);
   printf("Type: %s\n", slang_fq_type_string(&v->type));
}


void
_slang_print_var_scope(const slang_variable_scope *vars, int indent)
{
   GLuint i;

   spaces(indent);
   printf("Var scope %p  %d vars:\n", (void *) vars, vars->num_variables);
   for (i = 0; i < vars->num_variables; i++) {
      spaces(indent + 3);
      printf("%s (at %p)\n", (char *) vars->variables[i]->a_name, (void*) (vars->variables + i));
   }
   spaces(indent + 3);
   printf("outer_scope = %p\n", (void*) vars->outer_scope);

   if (vars->outer_scope) {
      /*spaces(indent + 3);*/
      _slang_print_var_scope(vars->outer_scope, indent + 3);
   }
}



int
slang_checksum_tree(const slang_operation *op)
{
   int s = op->num_children;
   int i;

   for (i = 0; i < op->num_children; i++) {
      s += slang_checksum_tree(&op->children[i]);
   }
   return s;
}
