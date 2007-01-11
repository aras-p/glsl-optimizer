
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
      if (s->variables[i].a_name == name)
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
      if (s->variables[i].a_name == name)
         return &s->variables[i];
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
      print_variable(&f->parameters->variables[i], 3);
   }

   printf(")\n");
   if (body && f->body)
      slang_print_tree(f->body, 0);
}




/* operation */
#define OP_END 0
#define OP_BLOCK_BEGIN_NO_NEW_SCOPE 1
#define OP_BLOCK_BEGIN_NEW_SCOPE 2
#define OP_DECLARE 3
#define OP_ASM 4
#define OP_BREAK 5
#define OP_CONTINUE 6
#define OP_DISCARD 7
#define OP_RETURN 8
#define OP_EXPRESSION 9
#define OP_IF 10
#define OP_WHILE 11
#define OP_DO 12
#define OP_FOR 13
#define OP_PUSH_VOID 14
#define OP_PUSH_BOOL 15
#define OP_PUSH_INT 16
#define OP_PUSH_FLOAT 17
#define OP_PUSH_IDENTIFIER 18
#define OP_SEQUENCE 19
#define OP_ASSIGN 20
#define OP_ADDASSIGN 21
#define OP_SUBASSIGN 22
#define OP_MULASSIGN 23
#define OP_DIVASSIGN 24
/*#define OP_MODASSIGN 25*/
/*#define OP_LSHASSIGN 26*/
/*#define OP_RSHASSIGN 27*/
/*#define OP_ORASSIGN 28*/
/*#define OP_XORASSIGN 29*/
/*#define OP_ANDASSIGN 30*/
#define OP_SELECT 31
#define OP_LOGICALOR 32
#define OP_LOGICALXOR 33
#define OP_LOGICALAND 34
/*#define OP_BITOR 35*/
/*#define OP_BITXOR 36*/
/*#define OP_BITAND 37*/
#define OP_EQUAL 38
#define OP_NOTEQUAL 39
#define OP_LESS 40
#define OP_GREATER 41
#define OP_LESSEQUAL 42
#define OP_GREATEREQUAL 43
/*#define OP_LSHIFT 44*/
/*#define OP_RSHIFT 45*/
#define OP_ADD 46
#define OP_SUBTRACT 47
#define OP_MULTIPLY 48
#define OP_DIVIDE 49
/*#define OP_MODULUS 50*/
#define OP_PREINCREMENT 51
#define OP_PREDECREMENT 52
#define OP_PLUS 53
#define OP_MINUS 54
/*#define OP_COMPLEMENT 55*/
#define OP_NOT 56
#define OP_SUBSCRIPT 57
#define OP_CALL 58
#define OP_FIELD 59
#define OP_POSTINCREMENT 60
#define OP_POSTDECREMENT 61


void
slang_print_opcode(unsigned int opcode)
{
   switch (opcode) {
   case OP_PUSH_VOID:
      printf("OP_PUSH_VOID\n");
      break;
   case OP_PUSH_BOOL:
      printf("OP_PUSH_BOOL\n");
      break;
   case OP_PUSH_INT:
      printf("OP_PUSH_INT\n");
      break;
   case OP_PUSH_FLOAT:
      printf("OP_PUSH_FLOAT\n");
      break;
   case OP_PUSH_IDENTIFIER:
      printf("OP_PUSH_IDENTIFIER\n");
      break;
   case OP_SEQUENCE:
      printf("OP_SEQUENCE\n");
      break;
   case OP_ASSIGN:
      printf("OP_ASSIGN\n");
      break;
   case OP_ADDASSIGN:
      printf("OP_ADDASSIGN\n");
      break;
   case OP_SUBASSIGN:
      printf("OP_SUBASSIGN\n");
      break;
   case OP_MULASSIGN:
      printf("OP_MULASSIGN\n");
      break;
   case OP_DIVASSIGN:
      printf("OP_DIVASSIGN\n");
      break;
   /*case OP_MODASSIGN:*/
   /*case OP_LSHASSIGN:*/
   /*case OP_RSHASSIGN:*/
   /*case OP_ORASSIGN:*/
   /*case OP_XORASSIGN:*/
   /*case OP_ANDASSIGN:*/
   case OP_SELECT:
      printf("OP_SELECT\n");
      break;
   case OP_LOGICALOR:
      printf("OP_LOGICALOR\n");
      break;
   case OP_LOGICALXOR:
      printf("OP_LOGICALXOR\n");
      break;
   case OP_LOGICALAND:
      printf("OP_LOGICALAND\n");
      break;
   /*case OP_BITOR:*/
   /*case OP_BITXOR:*/
   /*case OP_BITAND:*/
   case OP_EQUAL:
      printf("OP_EQUAL\n");
      break;
   case OP_NOTEQUAL:
      printf("OP_NOTEQUAL\n");
      break;
   case OP_LESS:
      printf("OP_LESS\n");
      break;
   case OP_GREATER:
      printf("OP_GREATER\n");
      break;
   case OP_LESSEQUAL:
      printf("OP_LESSEQUAL\n");
      break;
   case OP_GREATEREQUAL:
      printf("OP_GREATEREQUAL\n");
      break;
   /*case OP_LSHIFT:*/
   /*case OP_RSHIFT:*/
   case OP_ADD:
      printf("OP_ADD\n");
      break;
   case OP_SUBTRACT:
      printf("OP_SUBTRACT\n");
      break;
   case OP_MULTIPLY:
      printf("OP_MULTIPLY\n");
      break;
   case OP_DIVIDE:
      printf("OP_DIVIDE\n");
      break;
   /*case OP_MODULUS:*/
   case OP_PREINCREMENT:
      printf("OP_PREINCREMENT\n");
      break;
   case OP_PREDECREMENT:
      printf("OP_PREDECREMENT\n");
      break;
   case OP_PLUS:
      printf("OP_PLUS\n");
      break;
   case OP_MINUS:
      printf("OP_MINUS\n");
      break;
   case OP_NOT:
      printf("OP_NOT\n");
      break;
   /*case OP_COMPLEMENT:*/
   case OP_SUBSCRIPT:
      printf("OP_SUBSCRIPT\n");
      break;
   case OP_CALL:
      printf("OP_CALL\n");
      break;
   case OP_FIELD:
      printf("OP_FIELD\n");
      break;
   case OP_POSTINCREMENT:
      printf("OP_POSTINCREMENT\n");
      break;
   case OP_POSTDECREMENT:
      printf("OP_POSTDECREMENT\n");
      break;
   default:
      printf("UNKNOWN OP %d\n", opcode);
   }
}



const char *
slang_asm_string(slang_assembly_type t)
{
   switch (t) {
      /* core */
   case slang_asm_none:
      return "none";
   case slang_asm_float_copy:
      return "float_copy";
   case slang_asm_float_move:
      return "float_move";
   case slang_asm_float_push:
      return "float_push";
   case slang_asm_float_deref:
      return "float_deref";
   case slang_asm_float_add:
      return "float_add";
   case slang_asm_float_multiply:
      return "float_multiply";
   case slang_asm_float_divide:
      return "float_divide";
   case slang_asm_float_negate:
      return "float_negate";
   case slang_asm_float_less:
      return "float_less";
   case slang_asm_float_equal_exp:
      return "float_equal";
   case slang_asm_float_equal_int:
      return "float_equal";
   case slang_asm_float_to_int:
      return "float_to_int";
   case slang_asm_float_sine:
      return "float_sine";
   case slang_asm_float_arcsine:
      return "float_arcsine";
   case slang_asm_float_arctan:
      return "float_arctan";
   case slang_asm_float_power:
      return "float_power";
   case slang_asm_float_log2:
      return "float_log2";
   case slang_asm_vec4_floor:
      return "vec4_floor";
   case slang_asm_float_ceil:
      return "float_ceil";
   case slang_asm_float_noise1:
      return "float_noise1";
   case slang_asm_float_noise2:
      return "float_noise2";
   case slang_asm_float_noise3:
      return "float_noise3";
   case slang_asm_float_noise4:
      return "float_noise4";
   case slang_asm_int_copy:
      return "int_copy";
   case slang_asm_int_move:
      return "int_move";
   case slang_asm_int_push:
      return "int_push";
   case slang_asm_int_deref:
      return "int_deref";
   case slang_asm_int_to_float:
      return "int_to_float";
   case slang_asm_int_to_addr:
      return "int_to_addr";
   case slang_asm_bool_copy:
      return "bool_copy";
   case slang_asm_bool_move:
      return "bool_move";
   case slang_asm_bool_push:
      return "bool_push";
   case slang_asm_bool_deref:
      return "bool_deref";
   case slang_asm_addr_copy:
      return "addr_copy";
   case slang_asm_addr_push:
      return "addr_push";
   case slang_asm_addr_deref:
      return "addr_deref";
   case slang_asm_addr_add:
      return "addr_add";
   case slang_asm_addr_multiply:
      return "addr_multiply";
   case slang_asm_vec4_tex1d:
      return "vec4_tex1d";
   case slang_asm_vec4_tex2d:
      return "vec4_tex2d";
   case slang_asm_vec4_tex3d:
      return "vec4_tex3d";
   case slang_asm_vec4_texcube:
      return "vec4_texcube";
   case slang_asm_vec4_shad1d:
      return "vec4_shad1d";
   case slang_asm_vec4_shad2d:
      return "vec4_shad2d";
   case slang_asm_jump:
      return "jump";
   case slang_asm_jump_if_zero:
      return "jump_if_zero";
   case slang_asm_enter:
      return "enter";
   case slang_asm_leave:
      return "leave";
   case slang_asm_local_alloc:
      return "local_alloc";
   case slang_asm_local_free:
      return "local_free";
   case slang_asm_local_addr:
      return "local_addr";
   case slang_asm_global_addr:
      return "global_addr";
   case slang_asm_call:
      return "call";
   case slang_asm_return:
      return "return";
   case slang_asm_discard:
      return "discard";
   case slang_asm_exit:
      return "exit";
      /* GL_MESA_shader_debug */
   case slang_asm_float_print:
      return "float_print";
   case slang_asm_int_print:
      return "int_print";
   case slang_asm_bool_print:
      return "bool_print";
      /* vec4 */
   case slang_asm_float_to_vec4:
      return "float_to_vec4";
   case slang_asm_vec4_add:
      return "vec4_add";
   case slang_asm_vec4_subtract:
      return "vec4_subtract";
   case slang_asm_vec4_multiply:
      return "vec4_multiply";
   case slang_asm_vec4_divide:
      return "vec4_divide";
   case slang_asm_vec4_negate:
      return "vec4_negate";
   case slang_asm_vec4_dot:
      return "vec4_dot";
   case slang_asm_vec4_copy:
      return "vec4_copy";
   case slang_asm_vec4_deref:
      return "vec4_deref";
   case slang_asm_vec4_equal_int:
      return "vec4_equal";
   default:
      return "??asm??";
   }
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


static char *
slang_var_string(const slang_variable *v)
{
   static char str[1000];
   sprintf(str, "%s : %s",
           (char *) v->a_name,
           slang_fq_type_string(&v->type));
   return str;
}


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
      printf("%s\n", (char *) vars->variables[i].a_name);
   }
   spaces(indent + 3);
   printf("outer_scope = %p\n", (void*) vars->outer_scope);

   if (vars->outer_scope) {
      spaces(indent + 3);
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
