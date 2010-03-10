/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "../pp/sl_pp_public.h"
#include "sl_cl_parse.h"


/* revision number - increment after each change affecting emitted output */
#define REVISION                                   5

/* external declaration (or precision or invariant stmt) */
#define EXTERNAL_NULL                              0
#define EXTERNAL_FUNCTION_DEFINITION               1
#define EXTERNAL_DECLARATION                       2
#define DEFAULT_PRECISION                          3
#define INVARIANT_STMT                             4

/* precision */
#define PRECISION_DEFAULT                          0
#define PRECISION_LOW                              1
#define PRECISION_MEDIUM                           2
#define PRECISION_HIGH                             3

/* declaration */
#define DECLARATION_FUNCTION_PROTOTYPE             1
#define DECLARATION_INIT_DECLARATOR_LIST           2

/* function type */
#define FUNCTION_ORDINARY                          0
#define FUNCTION_CONSTRUCTOR                       1
#define FUNCTION_OPERATOR                          2

/* function call type */
#define FUNCTION_CALL_NONARRAY                     0
#define FUNCTION_CALL_ARRAY                        1

/* operator type */
#define OPERATOR_ADDASSIGN                         1
#define OPERATOR_SUBASSIGN                         2
#define OPERATOR_MULASSIGN                         3
#define OPERATOR_DIVASSIGN                         4
/*#define OPERATOR_MODASSIGN                         5*/
/*#define OPERATOR_LSHASSIGN                         6*/
/*#define OPERATOR_RSHASSIGN                         7*/
/*#define OPERATOR_ORASSIGN                          8*/
/*#define OPERATOR_XORASSIGN                         9*/
/*#define OPERATOR_ANDASSIGN                         10*/
#define OPERATOR_LOGICALXOR                        11
/*#define OPERATOR_BITOR                             12*/
/*#define OPERATOR_BITXOR                            13*/
/*#define OPERATOR_BITAND                            14*/
#define OPERATOR_LESS                              15
#define OPERATOR_GREATER                           16
#define OPERATOR_LESSEQUAL                         17
#define OPERATOR_GREATEREQUAL                      18
/*#define OPERATOR_LSHIFT                            19*/
/*#define OPERATOR_RSHIFT                            20*/
#define OPERATOR_MULTIPLY                          21
#define OPERATOR_DIVIDE                            22
/*#define OPERATOR_MODULUS                           23*/
#define OPERATOR_INCREMENT                         24
#define OPERATOR_DECREMENT                         25
#define OPERATOR_PLUS                              26
#define OPERATOR_MINUS                             27
/*#define OPERATOR_COMPLEMENT                        28*/
#define OPERATOR_NOT                               29

/* init declarator list */
#define DECLARATOR_NONE                            0
#define DECLARATOR_NEXT                            1

/* variable declaration */
#define VARIABLE_NONE                              0
#define VARIABLE_IDENTIFIER                        1
#define VARIABLE_INITIALIZER                       2
#define VARIABLE_ARRAY_EXPLICIT                    3
#define VARIABLE_ARRAY_UNKNOWN                     4

/* type qualifier */
#define TYPE_QUALIFIER_NONE                        0
#define TYPE_QUALIFIER_CONST                       1
#define TYPE_QUALIFIER_ATTRIBUTE                   2
#define TYPE_QUALIFIER_VARYING                     3
#define TYPE_QUALIFIER_UNIFORM                     4
#define TYPE_QUALIFIER_FIXEDOUTPUT                 5
#define TYPE_QUALIFIER_FIXEDINPUT                  6

/* invariant qualifier */
#define TYPE_VARIANT                               90
#define TYPE_INVARIANT                             91

/* centroid qualifier */
#define TYPE_CENTER                                95
#define TYPE_CENTROID                              96

/* layout qualifiers */
#define LAYOUT_QUALIFIER_NONE                      0
#define LAYOUT_QUALIFIER_UPPER_LEFT                1
#define LAYOUT_QUALIFIER_PIXEL_CENTER_INTEGER      2

/* type specifier */
#define TYPE_SPECIFIER_VOID                        0
#define TYPE_SPECIFIER_BOOL                        1
#define TYPE_SPECIFIER_BVEC2                       2
#define TYPE_SPECIFIER_BVEC3                       3
#define TYPE_SPECIFIER_BVEC4                       4
#define TYPE_SPECIFIER_INT                         5
#define TYPE_SPECIFIER_IVEC2                       6
#define TYPE_SPECIFIER_IVEC3                       7
#define TYPE_SPECIFIER_IVEC4                       8
#define TYPE_SPECIFIER_FLOAT                       9
#define TYPE_SPECIFIER_VEC2                        10
#define TYPE_SPECIFIER_VEC3                        11
#define TYPE_SPECIFIER_VEC4                        12
#define TYPE_SPECIFIER_MAT2                        13
#define TYPE_SPECIFIER_MAT3                        14
#define TYPE_SPECIFIER_MAT4                        15
#define TYPE_SPECIFIER_SAMPLER1D                   16
#define TYPE_SPECIFIER_SAMPLER2D                   17
#define TYPE_SPECIFIER_SAMPLER3D                   18
#define TYPE_SPECIFIER_SAMPLERCUBE                 19
#define TYPE_SPECIFIER_SAMPLER1DSHADOW             20
#define TYPE_SPECIFIER_SAMPLER2DSHADOW             21
#define TYPE_SPECIFIER_SAMPLER2DRECT               22
#define TYPE_SPECIFIER_SAMPLER2DRECTSHADOW         23
#define TYPE_SPECIFIER_STRUCT                      24
#define TYPE_SPECIFIER_TYPENAME                    25

/* OpenGL 2.1 */
#define TYPE_SPECIFIER_MAT23                       26
#define TYPE_SPECIFIER_MAT32                       27
#define TYPE_SPECIFIER_MAT24                       28
#define TYPE_SPECIFIER_MAT42                       29
#define TYPE_SPECIFIER_MAT34                       30
#define TYPE_SPECIFIER_MAT43                       31

/* GL_EXT_texture_array */
#define TYPE_SPECIFIER_SAMPLER_1D_ARRAY            32
#define TYPE_SPECIFIER_SAMPLER_2D_ARRAY            33
#define TYPE_SPECIFIER_SAMPLER_1D_ARRAY_SHADOW     34
#define TYPE_SPECIFIER_SAMPLER_2D_ARRAY_SHADOW     35

/* type specifier array */
#define TYPE_SPECIFIER_NONARRAY                    0
#define TYPE_SPECIFIER_ARRAY                       1

/* structure field */
#define FIELD_NONE                                 0
#define FIELD_NEXT                                 1
#define FIELD_ARRAY                                2

/* operation */
#define OP_END                                     0
#define OP_BLOCK_BEGIN_NO_NEW_SCOPE                1
#define OP_BLOCK_BEGIN_NEW_SCOPE                   2
#define OP_DECLARE                                 3
#define OP_ASM                                     4
#define OP_BREAK                                   5
#define OP_CONTINUE                                6
#define OP_DISCARD                                 7
#define OP_RETURN                                  8
#define OP_EXPRESSION                              9
#define OP_IF                                      10
#define OP_WHILE                                   11
#define OP_DO                                      12
#define OP_FOR                                     13
#define OP_PUSH_VOID                               14
#define OP_PUSH_BOOL                               15
#define OP_PUSH_INT                                16
#define OP_PUSH_FLOAT                              17
#define OP_PUSH_IDENTIFIER                         18
#define OP_SEQUENCE                                19
#define OP_ASSIGN                                  20
#define OP_ADDASSIGN                               21
#define OP_SUBASSIGN                               22
#define OP_MULASSIGN                               23
#define OP_DIVASSIGN                               24
/*#define OP_MODASSIGN                               25*/
/*#define OP_LSHASSIGN                               26*/
/*#define OP_RSHASSIGN                               27*/
/*#define OP_ORASSIGN                                28*/
/*#define OP_XORASSIGN                               29*/
/*#define OP_ANDASSIGN                               30*/
#define OP_SELECT                                  31
#define OP_LOGICALOR                               32
#define OP_LOGICALXOR                              33
#define OP_LOGICALAND                              34
/*#define OP_BITOR                                   35*/
/*#define OP_BITXOR                                  36*/
/*#define OP_BITAND                                  37*/
#define OP_EQUAL                                   38
#define OP_NOTEQUAL                                39
#define OP_LESS                                    40
#define OP_GREATER                                 41
#define OP_LESSEQUAL                               42
#define OP_GREATEREQUAL                            43
/*#define OP_LSHIFT                                  44*/
/*#define OP_RSHIFT                                  45*/
#define OP_ADD                                     46
#define OP_SUBTRACT                                47
#define OP_MULTIPLY                                48
#define OP_DIVIDE                                  49
/*#define OP_MODULUS                                 50*/
#define OP_PREINCREMENT                            51
#define OP_PREDECREMENT                            52
#define OP_PLUS                                    53
#define OP_MINUS                                   54
/*#define OP_COMPLEMENT                              55*/
#define OP_NOT                                     56
#define OP_SUBSCRIPT                               57
#define OP_CALL                                    58
#define OP_FIELD                                   59
#define OP_POSTINCREMENT                           60
#define OP_POSTDECREMENT                           61
#define OP_PRECISION                               62
#define OP_METHOD                                  63

/* parameter qualifier */
#define PARAM_QUALIFIER_IN                         0
#define PARAM_QUALIFIER_OUT                        1
#define PARAM_QUALIFIER_INOUT                      2

/* function parameter */
#define PARAMETER_NONE                             0
#define PARAMETER_NEXT                             1

/* function parameter array presence */
#define PARAMETER_ARRAY_NOT_PRESENT                0
#define PARAMETER_ARRAY_PRESENT                    1


struct parse_dict {
   int _void;
   int _float;
   int _int;
   int _bool;
   int vec2;
   int vec3;
   int vec4;
   int bvec2;
   int bvec3;
   int bvec4;
   int ivec2;
   int ivec3;
   int ivec4;
   int mat2;
   int mat3;
   int mat4;
   int mat2x3;
   int mat3x2;
   int mat2x4;
   int mat4x2;
   int mat3x4;
   int mat4x3;
   int sampler1D;
   int sampler2D;
   int sampler3D;
   int samplerCube;
   int sampler1DShadow;
   int sampler2DShadow;
   int sampler2DRect;
   int sampler2DRectShadow;
   int sampler1DArray;
   int sampler2DArray;
   int sampler1DArrayShadow;
   int sampler2DArrayShadow;

   int invariant;

   int centroid;

   int precision;
   int lowp;
   int mediump;
   int highp;

   int _const;
   int attribute;
   int varying;
   int uniform;
   int __fixed_output;
   int __fixed_input;

   int in;
   int out;
   int inout;

   int layout;
   int origin_upper_left;
   int pixel_center_integer;

   int _struct;

   int __constructor;
   int __operator;
   int ___asm;

   int _if;
   int _else;
   int _for;
   int _while;
   int _do;

   int _continue;
   int _break;
   int _return;
   int discard;

   int _false;
   int _true;

   int all;
   int _GL_ARB_fragment_coord_conventions;
};


struct parse_context {
   struct sl_pp_context *context;

   struct parse_dict dict;

   struct sl_pp_token_info *tokens;
   unsigned int tokens_read;
   unsigned int tokens_cap;

   unsigned char *out_buf;
   unsigned int out_cap;

   unsigned int shader_type;
   unsigned int parsing_builtin;

   unsigned int fragment_coord_conventions:1;

   char error[256];
   int process_error;
};


struct parse_state {
   unsigned int in;
   unsigned int out;
};


static unsigned int
_emit(struct parse_context *ctx,
      unsigned int *out,
      unsigned char b)
{
   if (*out == ctx->out_cap) {
      ctx->out_cap += 4096;
      ctx->out_buf = (unsigned char *)realloc(ctx->out_buf, ctx->out_cap * sizeof(unsigned char));
   }
   ctx->out_buf[*out] = b;
   return (*out)++;
}


static void
_update(struct parse_context *ctx,
        unsigned int out,
        unsigned char b)
{
   ctx->out_buf[out] = b;
}


static void
_error(struct parse_context *ctx,
       const char *msg)
{
   if (ctx->error[0] == '\0') {
      strncpy(ctx->error, msg, sizeof(ctx->error) - 1);
      ctx->error[sizeof(ctx->error) - 1] = '\0';
   }
}


static const struct sl_pp_token_info *
_fetch_token(struct parse_context *ctx,
             unsigned int pos)
{
   if (ctx->process_error) {
      return NULL;
   }

   while (pos >= ctx->tokens_read) {
      if (ctx->tokens_read == ctx->tokens_cap) {
         ctx->tokens_cap += 1024;
         ctx->tokens = realloc(ctx->tokens,
                               ctx->tokens_cap * sizeof(struct sl_pp_token_info));
         if (!ctx->tokens) {
            _error(ctx, "out of memory");
            ctx->process_error = 1;
            return NULL;
         }
      }
      if (sl_pp_process_get(ctx->context, &ctx->tokens[ctx->tokens_read])) {
         _error(ctx, sl_pp_context_error_message(ctx->context));
         ctx->process_error = 1;
         return NULL;
      }
      switch (ctx->tokens[ctx->tokens_read].token) {
      case SL_PP_COMMA:
      case SL_PP_SEMICOLON:
      case SL_PP_LBRACE:
      case SL_PP_RBRACE:
      case SL_PP_LPAREN:
      case SL_PP_RPAREN:
      case SL_PP_LBRACKET:
      case SL_PP_RBRACKET:
      case SL_PP_DOT:
      case SL_PP_INCREMENT:
      case SL_PP_ADDASSIGN:
      case SL_PP_PLUS:
      case SL_PP_DECREMENT:
      case SL_PP_SUBASSIGN:
      case SL_PP_MINUS:
      case SL_PP_BITNOT:
      case SL_PP_NOTEQUAL:
      case SL_PP_NOT:
      case SL_PP_MULASSIGN:
      case SL_PP_STAR:
      case SL_PP_DIVASSIGN:
      case SL_PP_SLASH:
      case SL_PP_MODASSIGN:
      case SL_PP_MODULO:
      case SL_PP_LSHIFTASSIGN:
      case SL_PP_LSHIFT:
      case SL_PP_LESSEQUAL:
      case SL_PP_LESS:
      case SL_PP_RSHIFTASSIGN:
      case SL_PP_RSHIFT:
      case SL_PP_GREATEREQUAL:
      case SL_PP_GREATER:
      case SL_PP_EQUAL:
      case SL_PP_ASSIGN:
      case SL_PP_AND:
      case SL_PP_BITANDASSIGN:
      case SL_PP_BITAND:
      case SL_PP_XOR:
      case SL_PP_BITXORASSIGN:
      case SL_PP_BITXOR:
      case SL_PP_OR:
      case SL_PP_BITORASSIGN:
      case SL_PP_BITOR:
      case SL_PP_QUESTION:
      case SL_PP_COLON:
      case SL_PP_IDENTIFIER:
      case SL_PP_UINT:
      case SL_PP_FLOAT:
      case SL_PP_EXTENSION_REQUIRE:
      case SL_PP_EXTENSION_ENABLE:
      case SL_PP_EXTENSION_WARN:
      case SL_PP_EXTENSION_DISABLE:
      case SL_PP_EOF:
         ctx->tokens_read++;
         break;
      default:
         ; /* no-op */
      }
   }
   return &ctx->tokens[pos];
}


/**
 * Try to parse/match a particular token.
 * \return 0 for success, -1 for error.
 */
static int
_parse_token(struct parse_context *ctx,
             enum sl_pp_token token,
             struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);

   if (input && input->token == token) {
      ps->in++;
      return 0;
   }
   return -1;
}


/**
 * Try to parse an identifer.
 * \return 0 for success, -1 for error
 */
static int
_parse_id(struct parse_context *ctx,
          int id,
          struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);

   if (input && input->token == SL_PP_IDENTIFIER && input->data.identifier == id) {
      ps->in++;
      return 0;
   }
   return -1;
}


static int
_parse_identifier(struct parse_context *ctx,
                  struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);

   if (input && input->token == SL_PP_IDENTIFIER) {
      const char *cstr = sl_pp_context_cstr(ctx->context, input->data.identifier);

      do {
         _emit(ctx, &ps->out, *cstr);
      } while (*cstr++);
      ps->in++;
      return 0;
   }
   return -1;
}


static int
_parse_float(struct parse_context *ctx,
             struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);

   if (input && input->token == SL_PP_FLOAT) {
      const char *cstr = sl_pp_context_cstr(ctx->context, input->data._float);

      _emit(ctx, &ps->out, 1);
      do {
         _emit(ctx, &ps->out, *cstr);
      } while (*cstr++);
      ps->in++;
      return 0;
   }
   return -1;
}


static int
_parse_uint(struct parse_context *ctx,
            struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);

   if (input && input->token == SL_PP_UINT) {
      const char *cstr = sl_pp_context_cstr(ctx->context, input->data._uint);

      _emit(ctx, &ps->out, 1);
      do {
         _emit(ctx, &ps->out, *cstr);
      } while (*cstr++);
      ps->in++;
      return 0;
   }
   return -1;
}


/**************************************/


static int
_parse_unary_expression(struct parse_context *ctx,
                        struct parse_state *ps);

static int
_parse_conditional_expression(struct parse_context *ctx,
                              struct parse_state *ps);


static int
_parse_constant_expression(struct parse_context *ctx,
                           struct parse_state *ps);


static int
_parse_primary_expression(struct parse_context *ctx,
                          struct parse_state *ps);


static int
_parse_statement(struct parse_context *ctx,
                 struct parse_state *ps);


static int
_parse_type_specifier(struct parse_context *ctx,
                      struct parse_state *ps);


static int
_parse_declaration(struct parse_context *ctx,
                   struct parse_state *ps);


static int
_parse_statement_list(struct parse_context *ctx,
                      struct parse_state *ps);


static int
_parse_assignment_expression(struct parse_context *ctx,
                             struct parse_state *ps);


static int
_parse_precision(struct parse_context *ctx,
                 struct parse_state *ps);


static int
_parse_overriden_operator(struct parse_context *ctx,
                          struct parse_state *ps)
{
   unsigned int op;

   if (_parse_token(ctx, SL_PP_INCREMENT, ps) == 0) {
      op = OPERATOR_INCREMENT;
   } else if (_parse_token(ctx, SL_PP_ADDASSIGN, ps) == 0) {
      op = OPERATOR_ADDASSIGN;
   } else if (_parse_token(ctx, SL_PP_PLUS, ps) == 0) {
      op = OPERATOR_PLUS;
   } else if (_parse_token(ctx, SL_PP_DECREMENT, ps) == 0) {
      op = OPERATOR_DECREMENT;
   } else if (_parse_token(ctx, SL_PP_SUBASSIGN, ps) == 0) {
      op = OPERATOR_SUBASSIGN;
   } else if (_parse_token(ctx, SL_PP_MINUS, ps) == 0) {
      op = OPERATOR_MINUS;
   } else if (_parse_token(ctx, SL_PP_NOT, ps) == 0) {
      op = OPERATOR_NOT;
   } else if (_parse_token(ctx, SL_PP_MULASSIGN, ps) == 0) {
      op = OPERATOR_MULASSIGN;
   } else if (_parse_token(ctx, SL_PP_STAR, ps) == 0) {
      op = OPERATOR_MULTIPLY;
   } else if (_parse_token(ctx, SL_PP_DIVASSIGN, ps) == 0) {
      op = OPERATOR_DIVASSIGN;
   } else if (_parse_token(ctx, SL_PP_SLASH, ps) == 0) {
      op = OPERATOR_DIVIDE;
   } else if (_parse_token(ctx, SL_PP_LESSEQUAL, ps) == 0) {
      op = OPERATOR_LESSEQUAL;
   } else if (_parse_token(ctx, SL_PP_LESS, ps) == 0) {
      op = OPERATOR_LESS;
   } else if (_parse_token(ctx, SL_PP_GREATEREQUAL, ps) == 0) {
      op = OPERATOR_GREATEREQUAL;
   } else if (_parse_token(ctx, SL_PP_GREATER, ps) == 0) {
      op = OPERATOR_GREATER;
   } else if (_parse_token(ctx, SL_PP_XOR, ps) == 0) {
      op = OPERATOR_LOGICALXOR;
   } else {
      return -1;
   }

   _emit(ctx, &ps->out, op);
   return 0;
}


static int
_parse_function_decl_identifier(struct parse_context *ctx,
                                struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, 0);

   if (ctx->parsing_builtin && _parse_id(ctx, ctx->dict.__constructor, &p) == 0) {
      _update(ctx, e, FUNCTION_CONSTRUCTOR);
      *ps = p;
      return 0;
   }

   if (ctx->parsing_builtin && _parse_id(ctx, ctx->dict.__operator, &p) == 0) {
      _update(ctx, e, FUNCTION_OPERATOR);
      if (_parse_overriden_operator(ctx, &p) == 0) {
         *ps = p;
         return 0;
      }
      return -1;
   }

   if (_parse_identifier(ctx, &p) == 0) {
      _update(ctx, e, FUNCTION_ORDINARY);
      *ps = p;
      return 0;
   }

   return -1;
}


static int
_parse_invariant_qualifier(struct parse_context *ctx,
                           struct parse_state *ps)
{
   if (_parse_id(ctx, ctx->dict.invariant, ps)) {
      return -1;
   }
   _emit(ctx, &ps->out, TYPE_INVARIANT);
   return 0;
}


static int
_parse_centroid_qualifier(struct parse_context *ctx,
                          struct parse_state *ps)
{
   if (_parse_id(ctx, ctx->dict.centroid, ps)) {
      return -1;
   }
   _emit(ctx, &ps->out, TYPE_CENTROID);
   return 0;
}


static int
_parse_layout_qualifier(struct parse_context *ctx,
                        struct parse_state *ps)
{
   if (_parse_id(ctx, ctx->dict.layout, ps) == 0) {
      if (!ctx->fragment_coord_conventions) {
         _error(ctx, "GL_ARB_fragment_coord_conventions extension must be enabled "
                     "in order to use a layout qualifier");
         return -1;
      }

      /* Layout qualifiers are only defined for fragment shaders,
       * so do an early check.
       */
      if (ctx->shader_type != 1) {
         _error(ctx, "layout qualifiers are only valid for fragment shaders");
         return -1;
      }

      /* start of a parenthesised list of layout qualifiers */

      if (_parse_token(ctx, SL_PP_LPAREN, ps)) {
         _error(ctx, "expected `('");
         return -1;
      }

      /* parse comma-separated ID list */
      while (1) {
         if (_parse_id(ctx, ctx->dict.origin_upper_left, ps) == 0) {
            _emit(ctx, &ps->out, LAYOUT_QUALIFIER_UPPER_LEFT);
         }
         else if (_parse_id(ctx, ctx->dict.pixel_center_integer, ps) == 0) {
            _emit(ctx, &ps->out, LAYOUT_QUALIFIER_PIXEL_CENTER_INTEGER);
         }
         else {
            _error(ctx, "expected a layout qualifier name");
            return -1;
         }

         if (_parse_token(ctx, SL_PP_RPAREN, ps) == 0) {
            /* all done */
            break;
         }
         else if (_parse_token(ctx, SL_PP_COMMA, ps) == 0) {
            /* another layout qualifier is coming */
         }
         else {
            _error(ctx, "expected `,' or `)'");
            return -1;
         }
      }
   }

   return 0;
}


static int
_parse_storage_qualifier(struct parse_context *ctx,
                         struct parse_state *ps)
{
   struct parse_state p = *ps;
   const struct sl_pp_token_info *input = _fetch_token(ctx, p.in);
   unsigned int e = _emit(ctx, &p.out, 0);
   int id;

   if (!input || input->token != SL_PP_IDENTIFIER) {
      return -1;
   }
   id = input->data.identifier;

   if (id == ctx->dict._const) {
      _update(ctx, e, TYPE_QUALIFIER_CONST);
   } else if (ctx->shader_type == 2 && id == ctx->dict.attribute) {
      _update(ctx, e, TYPE_QUALIFIER_ATTRIBUTE);
   } else if (id == ctx->dict.varying) {
      _update(ctx, e, TYPE_QUALIFIER_VARYING);
   } else if (id == ctx->dict.uniform) {
      _update(ctx, e, TYPE_QUALIFIER_UNIFORM);
   } else if (ctx->parsing_builtin && id == ctx->dict.__fixed_output) {
      _update(ctx, e, TYPE_QUALIFIER_FIXEDOUTPUT);
   } else if (ctx->parsing_builtin && id == ctx->dict.__fixed_input) {
      _update(ctx, e, TYPE_QUALIFIER_FIXEDINPUT);
   } else {
      return -1;
   }
   _parse_token(ctx, SL_PP_IDENTIFIER, &p);
   *ps = p;
   return 0;
}


static int
_parse_struct_declarator(struct parse_context *ctx,
                         struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e;

   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   e = _emit(ctx, &p.out, FIELD_NONE);
   *ps = p;

   if (_parse_token(ctx, SL_PP_LBRACKET, &p)) {
      return 0;
   }
   if (_parse_constant_expression(ctx, &p)) {
      _error(ctx, "expected constant integral expression");
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RBRACKET, &p)) {
      _error(ctx, "expected `]'");
      return -1;
   }
   _update(ctx, e, FIELD_ARRAY);
   *ps = p;
   return 0;
}


static int
_parse_struct_declarator_list(struct parse_context *ctx,
                              struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_struct_declarator(ctx, &p)) {
      return -1;
   }

   for (;;) {
      *ps = p;
      _emit(ctx, &p.out, FIELD_NEXT);
      if (_parse_token(ctx, SL_PP_COMMA, &p)) {
         return 0;
      }
      if (_parse_struct_declarator(ctx, &p)) {
         return 0;
      }
   }
}


static int
_parse_struct_declaration(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_type_specifier(ctx, &p)) {
      return -1;
   }
   if (_parse_struct_declarator_list(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, FIELD_NONE);
   *ps = p;
   return 0;
}


static int
_parse_struct_declaration_list(struct parse_context *ctx,
                               struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_struct_declaration(ctx, &p)) {
      return -1;
   }

   for (;;) {
      *ps = p;
      _emit(ctx, &p.out, FIELD_NEXT);
      if (_parse_struct_declaration(ctx, &p)) {
         return 0;
      }
   }
}


static int
_parse_struct_specifier(struct parse_context *ctx,
                        struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_id(ctx, ctx->dict._struct, &p)) {
      return -1;
   }
   if (_parse_identifier(ctx, &p)) {
      _emit(ctx, &p.out, '\0');
   }
   if (_parse_token(ctx, SL_PP_LBRACE, &p)) {
      _error(ctx, "expected `{'");
      return -1;
   }
   if (_parse_struct_declaration_list(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RBRACE, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, FIELD_NONE);
   *ps = p;
   return 0;
}


static int
_parse_type_specifier_nonarray(struct parse_context *ctx,
                               struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, 0);
   const struct sl_pp_token_info *input;
   int id;

   if (_parse_struct_specifier(ctx, &p) == 0) {
      _update(ctx, e, TYPE_SPECIFIER_STRUCT);
      *ps = p;
      return 0;
   }

   input = _fetch_token(ctx, p.in);
   if (!input || input->token != SL_PP_IDENTIFIER) {
      return -1;
   }
   id = input->data.identifier;

   if (id == ctx->dict._void) {
      _update(ctx, e, TYPE_SPECIFIER_VOID);
   } else if (id == ctx->dict._float) {
      _update(ctx, e, TYPE_SPECIFIER_FLOAT);
   } else if (id == ctx->dict._int) {
      _update(ctx, e, TYPE_SPECIFIER_INT);
   } else if (id == ctx->dict._bool) {
      _update(ctx, e, TYPE_SPECIFIER_BOOL);
   } else if (id == ctx->dict.vec2) {
      _update(ctx, e, TYPE_SPECIFIER_VEC2);
   } else if (id == ctx->dict.vec3) {
      _update(ctx, e, TYPE_SPECIFIER_VEC3);
   } else if (id == ctx->dict.vec4) {
      _update(ctx, e, TYPE_SPECIFIER_VEC4);
   } else if (id == ctx->dict.bvec2) {
      _update(ctx, e, TYPE_SPECIFIER_BVEC2);
   } else if (id == ctx->dict.bvec3) {
      _update(ctx, e, TYPE_SPECIFIER_BVEC3);
   } else if (id == ctx->dict.bvec4) {
      _update(ctx, e, TYPE_SPECIFIER_BVEC4);
   } else if (id == ctx->dict.ivec2) {
      _update(ctx, e, TYPE_SPECIFIER_IVEC2);
   } else if (id == ctx->dict.ivec3) {
      _update(ctx, e, TYPE_SPECIFIER_IVEC3);
   } else if (id == ctx->dict.ivec4) {
      _update(ctx, e, TYPE_SPECIFIER_IVEC4);
   } else if (id == ctx->dict.mat2) {
      _update(ctx, e, TYPE_SPECIFIER_MAT2);
   } else if (id == ctx->dict.mat3) {
      _update(ctx, e, TYPE_SPECIFIER_MAT3);
   } else if (id == ctx->dict.mat4) {
      _update(ctx, e, TYPE_SPECIFIER_MAT4);
   } else if (id == ctx->dict.mat2x3) {
      _update(ctx, e, TYPE_SPECIFIER_MAT23);
   } else if (id == ctx->dict.mat3x2) {
      _update(ctx, e, TYPE_SPECIFIER_MAT32);
   } else if (id == ctx->dict.mat2x4) {
      _update(ctx, e, TYPE_SPECIFIER_MAT24);
   } else if (id == ctx->dict.mat4x2) {
      _update(ctx, e, TYPE_SPECIFIER_MAT42);
   } else if (id == ctx->dict.mat3x4) {
      _update(ctx, e, TYPE_SPECIFIER_MAT34);
   } else if (id == ctx->dict.mat4x3) {
      _update(ctx, e, TYPE_SPECIFIER_MAT43);
   } else if (id == ctx->dict.sampler1D) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER1D);
   } else if (id == ctx->dict.sampler2D) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER2D);
   } else if (id == ctx->dict.sampler3D) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER3D);
   } else if (id == ctx->dict.samplerCube) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLERCUBE);
   } else if (id == ctx->dict.sampler1DShadow) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER1DSHADOW);
   } else if (id == ctx->dict.sampler2DShadow) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER2DSHADOW);
   } else if (id == ctx->dict.sampler2DRect) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER2DRECT);
   } else if (id == ctx->dict.sampler2DRectShadow) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER2DRECTSHADOW);
   } else if (id == ctx->dict.sampler1DArray) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER_1D_ARRAY);
   } else if (id == ctx->dict.sampler2DArray) {
      /* XXX check for GL_EXT_texture_array */
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER_2D_ARRAY);
   } else if (id == ctx->dict.sampler1DArrayShadow) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER_1D_ARRAY_SHADOW);
   } else if (id == ctx->dict.sampler2DArrayShadow) {
      _update(ctx, e, TYPE_SPECIFIER_SAMPLER_2D_ARRAY_SHADOW);
   } else if (_parse_identifier(ctx, &p) == 0) {
      _update(ctx, e, TYPE_SPECIFIER_TYPENAME);
      *ps = p;
      return 0;
   } else {
      return -1;
   }

   _parse_token(ctx, SL_PP_IDENTIFIER, &p);
   *ps = p;
   return 0;
}


static int
_parse_type_specifier_array(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_token(ctx, SL_PP_LBRACKET, &p)) {
      return -1;
   }
   if (_parse_constant_expression(ctx, &p)) {
      _error(ctx, "expected constant integral expression");
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RBRACKET, &p)) {
      _error(ctx, "expected `]'");
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_type_specifier(struct parse_context *ctx,
                      struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e;

   if (_parse_type_specifier_nonarray(ctx, &p)) {
      return -1;
   }

   e = _emit(ctx, &p.out, TYPE_SPECIFIER_ARRAY);
   if (_parse_type_specifier_array(ctx, &p)) {
      _update(ctx, e, TYPE_SPECIFIER_NONARRAY);
   }
   *ps = p;
   return 0;
}


static int
_parse_fully_specified_type(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_layout_qualifier(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, LAYOUT_QUALIFIER_NONE);

   if (_parse_invariant_qualifier(ctx, &p)) {
      _emit(ctx, &p.out, TYPE_VARIANT);
   }

   if (_parse_centroid_qualifier(ctx, &p)) {
      _emit(ctx, &p.out, TYPE_CENTER);
   }
   if (_parse_storage_qualifier(ctx, &p)) {
      _emit(ctx, &p.out, TYPE_QUALIFIER_NONE);
   }
   if (_parse_precision(ctx, &p)) {
      _emit(ctx, &p.out, PRECISION_DEFAULT);
   }
   if (_parse_type_specifier(ctx, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_function_header(struct parse_context *ctx,
                       struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_fully_specified_type(ctx, &p)) {
      return -1;
   }
   if (_parse_function_decl_identifier(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_parameter_qualifier(struct parse_context *ctx,
                           struct parse_state *ps)
{
   unsigned int e = _emit(ctx, &ps->out, PARAM_QUALIFIER_IN);

   if (_parse_id(ctx, ctx->dict.out, ps) == 0) {
      _update(ctx, e, PARAM_QUALIFIER_OUT);
   } else if (_parse_id(ctx, ctx->dict.inout, ps) == 0) {
      _update(ctx, e, PARAM_QUALIFIER_INOUT);
   } else {
      _parse_id(ctx, ctx->dict.in, ps);
   }
   return 0;
}


static int
_parse_function_identifier(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p;
   unsigned int e;

   if (_parse_identifier(ctx, ps)) {
      return -1;
   }
   e = _emit(ctx, &ps->out, FUNCTION_CALL_NONARRAY);

   p = *ps;
   if (_parse_token(ctx, SL_PP_LBRACKET, &p)) {
      return 0;
   }
   if (_parse_constant_expression(ctx, &p)) {
      _error(ctx, "expected constant integral expression");
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RBRACKET, &p)) {
      _error(ctx, "expected `]'");
      return -1;
   }
   _update(ctx, e, FUNCTION_CALL_ARRAY);
   *ps = p;
   return 0;
}


static int
_parse_function_call_header(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_function_identifier(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_assign_expression(struct parse_context *ctx,
                         struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int op;

   if (_parse_unary_expression(ctx, &p)) {
      return -1;
   }

   if (_parse_token(ctx, SL_PP_ASSIGN, &p) == 0) {
      op = OP_ASSIGN;
   } else if (_parse_token(ctx, SL_PP_MULASSIGN, &p) == 0) {
      op = OP_MULASSIGN;
   } else if (_parse_token(ctx, SL_PP_DIVASSIGN, &p) == 0) {
      op = OP_DIVASSIGN;
   } else if (_parse_token(ctx, SL_PP_ADDASSIGN, &p) == 0) {
      op = OP_ADDASSIGN;
   } else if (_parse_token(ctx, SL_PP_SUBASSIGN, &p) == 0) {
      op = OP_SUBASSIGN;
   } else {
      return -1;
   }

   if (_parse_assignment_expression(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, op);

   *ps = p;
   return 0;
}


static int
_parse_assignment_expression(struct parse_context *ctx,
                             struct parse_state *ps)
{
   if (_parse_assign_expression(ctx, ps) == 0) {
      return 0;
   }

   if (_parse_conditional_expression(ctx, ps) == 0) {
      return 0;
   }

   return -1;
}


static int
_parse_function_call_header_with_parameters(struct parse_context *ctx,
                                            struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_function_call_header(ctx, &p)) {
      return -1;
   }
   if (_parse_assignment_expression(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_COMMA, &p)) {
         return 0;
      }
      if (_parse_assignment_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_END);
   }
}


static int
_parse_function_call_header_no_parameters(struct parse_context *ctx,
                                          struct parse_state *ps)
{
   if (_parse_function_call_header(ctx, ps)) {
      return -1;
   }
   _parse_id(ctx, ctx->dict._void, ps);
   return 0;
}


static int
_parse_function_call_generic(struct parse_context *ctx,
                             struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_function_call_header_with_parameters(ctx, &p) == 0) {
      if (_parse_token(ctx, SL_PP_RPAREN, &p) == 0) {
         *ps = p;
         return 0;
      }
      _error(ctx, "expected `)'");
      return -1;
   }

   p = *ps;
   if (_parse_function_call_header_no_parameters(ctx, &p) == 0) {
      if (_parse_token(ctx, SL_PP_RPAREN, &p) == 0) {
         *ps = p;
         return 0;
      }
      _error(ctx, "expected `)'");
      return -1;
   }

   return -1;
}


static int
_parse_method_call(struct parse_context *ctx,
                   struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_METHOD);
   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_DOT, &p)) {
      return -1;
   }
   if (_parse_function_call_generic(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_regular_function_call(struct parse_context *ctx,
                             struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_CALL);
   if (_parse_function_call_generic(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_function_call(struct parse_context *ctx,
                     struct parse_state *ps)
{
   if (_parse_regular_function_call(ctx, ps) == 0) {
      return 0;
   }

   if (_parse_method_call(ctx, ps) == 0) {
      return 0;
   }

   return -1;
}


static int
_parse_expression(struct parse_context *ctx,
                  struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_assignment_expression(ctx, &p)) {
      return -1;
   }

   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_COMMA, &p)) {
         return 0;
      }
      if (_parse_assignment_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_SEQUENCE);
   }
}


static int
_parse_postfix_expression(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p;

   if (_parse_function_call(ctx, ps)) {
      if (_parse_primary_expression(ctx, ps)) {
         return -1;
      }
   }

   for (p = *ps;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_INCREMENT, &p) == 0) {
         _emit(ctx, &p.out, OP_POSTINCREMENT);
      } else if (_parse_token(ctx, SL_PP_DECREMENT, &p) == 0) {
         _emit(ctx, &p.out, OP_POSTDECREMENT);
      } else if (_parse_token(ctx, SL_PP_LBRACKET, &p) == 0) {
         if (_parse_expression(ctx, &p)) {
            _error(ctx, "expected an integral expression");
            return -1;
         }
         if (_parse_token(ctx, SL_PP_RBRACKET, &p)) {
            _error(ctx, "expected `]'");
            return -1;
         }
         _emit(ctx, &p.out, OP_SUBSCRIPT);
      } else if (_parse_token(ctx, SL_PP_DOT, &p) == 0) {
         _emit(ctx, &p.out, OP_FIELD);
         if (_parse_identifier(ctx, &p)) {
            return 0;
         }
      } else {
         return 0;
      }
   }
}


static int
_parse_unary_expression(struct parse_context *ctx,
                        struct parse_state *ps)
{
   struct parse_state p;
   unsigned int op;

   if (_parse_postfix_expression(ctx, ps) == 0) {
      return 0;
   }

   p = *ps;
   if (_parse_token(ctx, SL_PP_INCREMENT, &p) == 0) {
      op = OP_PREINCREMENT;
   } else if (_parse_token(ctx, SL_PP_DECREMENT, &p) == 0) {
      op = OP_PREDECREMENT;
   } else if (_parse_token(ctx, SL_PP_PLUS, &p) == 0) {
      op = OP_PLUS;
   } else if (_parse_token(ctx, SL_PP_MINUS, &p) == 0) {
      op = OP_MINUS;
   } else if (_parse_token(ctx, SL_PP_NOT, &p) == 0) {
      op = OP_NOT;
   } else {
      return -1;
   }

   if (_parse_unary_expression(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, op);
   *ps = p;
   return 0;
}


static int
_parse_multiplicative_expression(struct parse_context *ctx,
                                 struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_unary_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      unsigned int op;

      *ps = p;
      if (_parse_token(ctx, SL_PP_STAR, &p) == 0) {
         op = OP_MULTIPLY;
      } else if (_parse_token(ctx, SL_PP_SLASH, &p) == 0) {
         op = OP_DIVIDE;
      } else {
         return 0;
      }
      if (_parse_unary_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, op);
   }
}


static int
_parse_additive_expression(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_multiplicative_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      unsigned int op;

      *ps = p;
      if (_parse_token(ctx, SL_PP_PLUS, &p) == 0) {
         op = OP_ADD;
      } else if (_parse_token(ctx, SL_PP_MINUS, &p) == 0) {
         op = OP_SUBTRACT;
      } else {
         return 0;
      }
      if (_parse_multiplicative_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, op);
   }
}


static int
_parse_relational_expression(struct parse_context *ctx,
                             struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_additive_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      unsigned int op;

      *ps = p;
      if (_parse_token(ctx, SL_PP_LESS, &p) == 0) {
         op = OP_LESS;
      } else if (_parse_token(ctx, SL_PP_GREATER, &p) == 0) {
         op = OP_GREATER;
      } else if (_parse_token(ctx, SL_PP_LESSEQUAL, &p) == 0) {
         op = OP_LESSEQUAL;
      } else if (_parse_token(ctx, SL_PP_GREATEREQUAL, &p) == 0) {
         op = OP_GREATEREQUAL;
      } else {
         return 0;
      }
      if (_parse_additive_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, op);
   }
}


static int
_parse_equality_expression(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_relational_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      unsigned int op;

      *ps = p;
      if (_parse_token(ctx, SL_PP_EQUAL, &p) == 0) {
         op = OP_EQUAL;
      } else if (_parse_token(ctx, SL_PP_NOTEQUAL, &p) == 0) {
         op = OP_NOTEQUAL;
      } else {
         return 0;
      }
      if (_parse_relational_expression(ctx, &p)) {
         return -1;
      }
      _emit(ctx, &p.out, op);
   }
}


static int
_parse_logical_and_expression(struct parse_context *ctx,
                              struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_equality_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_AND, &p)) {
         return 0;
      }
      if (_parse_equality_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_LOGICALAND);
   }
}


static int
_parse_logical_xor_expression(struct parse_context *ctx,
                              struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_logical_and_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_XOR, &p)) {
         return 0;
      }
      if (_parse_logical_and_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_LOGICALXOR);
   }
}


static int
_parse_logical_or_expression(struct parse_context *ctx,
                             struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_logical_xor_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_OR, &p)) {
         return 0;
      }
      if (_parse_logical_xor_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_LOGICALOR);
   }
}


static int
_parse_conditional_expression(struct parse_context *ctx,
                              struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_logical_or_expression(ctx, &p)) {
      return -1;
   }
   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_QUESTION, &p)) {
         return 0;
      }
      if (_parse_expression(ctx, &p)) {
         return 0;
      }
      if (_parse_token(ctx, SL_PP_COLON, &p)) {
         return 0;
      }
      if (_parse_conditional_expression(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_SELECT);
   }
}


static int
_parse_constant_expression(struct parse_context *ctx,
                           struct parse_state *ps)
{
   if (_parse_conditional_expression(ctx, ps)) {
      return -1;
   }
   _emit(ctx, &ps->out, OP_END);
   return 0;
}


static int
_parse_parameter_declarator_array(struct parse_context *ctx,
                                  struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_token(ctx, SL_PP_LBRACKET, &p)) {
      return -1;
   }
   if (_parse_constant_expression(ctx, &p)) {
      _error(ctx, "expected constant integral expression");
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RBRACKET, &p)) {
      _error(ctx, "expected `]'");
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_parameter_declarator(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e;

   if (_parse_type_specifier(ctx, &p)) {
      return -1;
   }
   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   e = _emit(ctx, &p.out, PARAMETER_ARRAY_PRESENT);
   if (_parse_parameter_declarator_array(ctx, &p)) {
      _update(ctx, e, PARAMETER_ARRAY_NOT_PRESENT);
   }
   *ps = p;
   return 0;
}


static int
_parse_parameter_type_specifier_array(struct parse_context *ctx,
                                      struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_token(ctx, SL_PP_LBRACKET, &p)) {
      return -1;
   }
   if (_parse_constant_expression(ctx, &p)) {
      _error(ctx, "expected constant integral expression");
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RBRACKET, &p)) {
      _error(ctx, "expected `]'");
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_parameter_type_specifier(struct parse_context *ctx,
                                struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e;

   if (_parse_type_specifier(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, '\0');

   e = _emit(ctx, &p.out, PARAMETER_ARRAY_PRESENT);
   if (_parse_parameter_type_specifier_array(ctx, &p)) {
      _update(ctx, e, PARAMETER_ARRAY_NOT_PRESENT);
   }
   *ps = p;
   return 0;
}


static int
_parse_parameter_declaration(struct parse_context *ctx,
                             struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, PARAMETER_NEXT);

   (void) e;

   if (_parse_storage_qualifier(ctx, &p)) {
      _emit(ctx, &p.out, TYPE_QUALIFIER_NONE);
   }
   _parse_parameter_qualifier(ctx, &p);
   if (_parse_precision(ctx, &p)) {
      _emit(ctx, &p.out, PRECISION_DEFAULT);
   }
   if (_parse_parameter_declarator(ctx, &p) == 0) {
      *ps = p;
      return 0;
   }
   if (_parse_parameter_type_specifier(ctx, &p) == 0) {
      *ps = p;
      return 0;
   }

   return -1;
}


static int
_parse_function_header_with_parameters(struct parse_context *ctx,
                                       struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_function_header(ctx, &p)) {
      return -1;
   }
   if (_parse_parameter_declaration(ctx, &p)) {
      return -1;
   }

   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_COMMA, &p)) {
         return 0;
      }
      if (_parse_parameter_declaration(ctx, &p)) {
         return 0;
      }
   }
}


static int
_parse_function_declarator(struct parse_context *ctx,
                           struct parse_state *ps)
{
   if (_parse_function_header_with_parameters(ctx, ps) == 0) {
      return 0;
   }

   if (_parse_function_header(ctx, ps) == 0) {
      return 0;
   }

   return -1;
}


static int
_parse_function_prototype(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_function_header(ctx, &p) == 0) {
      if (_parse_id(ctx, ctx->dict._void, &p) == 0) {
         if (_parse_token(ctx, SL_PP_RPAREN, &p) == 0) {
            _emit(ctx, &p.out, PARAMETER_NONE);
            *ps = p;
            return 0;
         }
         _error(ctx, "expected `)'");
         return -1;
      }
   }

   p = *ps;
   if (_parse_function_declarator(ctx, &p) == 0) {
      if (_parse_token(ctx, SL_PP_RPAREN, &p) == 0) {
         _emit(ctx, &p.out, PARAMETER_NONE);
         *ps = p;
         return 0;
      }
      _error(ctx, "expected `)'");
      return -1;
   }

   return -1;
}


static int
_parse_precision(struct parse_context *ctx,
                 struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);
   int id;
   unsigned int precision;

   if (!input || input->token != SL_PP_IDENTIFIER) {
      return -1;
   }
   id = input->data.identifier;

   if (id == ctx->dict.lowp) {
      precision = PRECISION_LOW;
   } else if (id == ctx->dict.mediump) {
      precision = PRECISION_MEDIUM;
   } else if (id == ctx->dict.highp) {
      precision = PRECISION_HIGH;
   } else {
      return -1;
   }

   _parse_token(ctx, SL_PP_IDENTIFIER, ps);
   _emit(ctx, &ps->out, precision);
   return 0;
}


static int
_parse_prectype(struct parse_context *ctx,
                 struct parse_state *ps)
{
   const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);
   int id;
   unsigned int type;

   if (!input || input->token != SL_PP_IDENTIFIER) {
      return -1;
   }
   id = input->data.identifier;

   if (id == ctx->dict._int) {
      type = TYPE_SPECIFIER_INT;
   } else if (id == ctx->dict._float) {
      type = TYPE_SPECIFIER_FLOAT;
   } else if (id == ctx->dict.sampler1D) {
      type = TYPE_SPECIFIER_SAMPLER1D;
   } else if (id == ctx->dict.sampler2D) {
      type = TYPE_SPECIFIER_SAMPLER2D;
   } else if (id == ctx->dict.sampler3D) {
      type = TYPE_SPECIFIER_SAMPLER3D;
   } else if (id == ctx->dict.samplerCube) {
      type = TYPE_SPECIFIER_SAMPLERCUBE;
   } else if (id == ctx->dict.sampler1DShadow) {
      type = TYPE_SPECIFIER_SAMPLER1DSHADOW;
   } else if (id == ctx->dict.sampler2DShadow) {
      type = TYPE_SPECIFIER_SAMPLER2DSHADOW;
   } else if (id == ctx->dict.sampler2DRect) {
      type = TYPE_SPECIFIER_SAMPLER2DRECT;
   } else if (id == ctx->dict.sampler2DRectShadow) {
      type = TYPE_SPECIFIER_SAMPLER2DRECTSHADOW;
   } else if (id == ctx->dict.sampler1DArray) {
      type = TYPE_SPECIFIER_SAMPLER_1D_ARRAY;
   } else if (id == ctx->dict.sampler2DArray) {
      type = TYPE_SPECIFIER_SAMPLER_2D_ARRAY;
   } else if (id == ctx->dict.sampler1DArrayShadow) {
      type = TYPE_SPECIFIER_SAMPLER_1D_ARRAY_SHADOW;
   } else if (id == ctx->dict.sampler2DArrayShadow) {
      type = TYPE_SPECIFIER_SAMPLER_2D_ARRAY_SHADOW;
   } else {
      return -1;
   }

   _parse_token(ctx, SL_PP_IDENTIFIER, ps);
   _emit(ctx, &ps->out, type);
   return 0;
}


static int
_parse_precision_stmt(struct parse_context *ctx,
                      struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_id(ctx, ctx->dict.precision, &p)) {
      return -1;
   }
   if (_parse_precision(ctx, &p)) {
      return -1;
   }
   if (_parse_prectype(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_floatconstant(struct parse_context *ctx,
                     struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_PUSH_FLOAT);
   if (_parse_float(ctx, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_intconstant(struct parse_context *ctx,
                   struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_PUSH_INT);
   if (_parse_uint(ctx, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_boolconstant(struct parse_context *ctx,
                    struct parse_state *ps)
{
   if (_parse_id(ctx, ctx->dict._false, ps) == 0) {
      _emit(ctx, &ps->out, OP_PUSH_BOOL);
      _emit(ctx, &ps->out, 2);  /* radix */
      _emit(ctx, &ps->out, '0');
      _emit(ctx, &ps->out, '\0');
      return 0;
   }

   if (_parse_id(ctx, ctx->dict._true, ps) == 0) {
      _emit(ctx, &ps->out, OP_PUSH_BOOL);
      _emit(ctx, &ps->out, 2);  /* radix */
      _emit(ctx, &ps->out, '1');
      _emit(ctx, &ps->out, '\0');
      return 0;
   }

   return -1;
}


static int
_parse_variable_identifier(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_PUSH_IDENTIFIER);
   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_primary_expression(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p;

   if (_parse_floatconstant(ctx, ps) == 0) {
      return 0;
   }
   if (_parse_boolconstant(ctx, ps) == 0) {
      return 0;
   }
   if (_parse_intconstant(ctx, ps) == 0) {
      return 0;
   }
   if (_parse_variable_identifier(ctx, ps) == 0) {
      return 0;
   }

   p = *ps;
   if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
      return -1;
   }
   if (_parse_expression(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RPAREN, &p)) {
      return -1;
   }

   *ps = p;
   return 0;
}


static int
_parse_asm_argument(struct parse_context *ctx,
                    struct parse_state *ps)
{
   if (_parse_variable_identifier(ctx, ps) == 0) {
      struct parse_state p = *ps;

      if (_parse_token(ctx, SL_PP_DOT, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_FIELD);
      if (_parse_identifier(ctx, &p)) {
         return 0;
      }
      *ps = p;
      return 0;
   }

   if (_parse_floatconstant(ctx, ps) == 0) {
      return 0;
   }

   return -1;
}


static int
_parse_asm_arguments(struct parse_context *ctx,
                     struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_asm_argument(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);

   for (;;) {
      *ps = p;
      if (_parse_token(ctx, SL_PP_COMMA, &p)) {
         return 0;
      }
      if (_parse_asm_argument(ctx, &p)) {
         return 0;
      }
      _emit(ctx, &p.out, OP_END);
   }
}


static int
_parse_asm_statement(struct parse_context *ctx,
                     struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_id(ctx, ctx->dict.___asm, &p)) {
      return -1;
   }
   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   if (_parse_asm_arguments(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_selection_statement(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_IF);
   if (_parse_id(ctx, ctx->dict._if, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
      _error(ctx, "expected `('");
      return -1;
   }
   if (_parse_expression(ctx, &p)) {
      _error(ctx, "expected an expression");
      return -1;
   }
   if (_parse_token(ctx, SL_PP_RPAREN, &p)) {
      _error(ctx, "expected `)'");
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   if (_parse_statement(ctx, &p)) {
      return -1;
   }

   *ps = p;
   if (_parse_id(ctx, ctx->dict._else, &p) == 0) {
      if (_parse_statement(ctx, &p) == 0) {
         *ps = p;
         return 0;
      }
   }

   _emit(ctx, &ps->out, OP_EXPRESSION);
   _emit(ctx, &ps->out, OP_PUSH_VOID);
   _emit(ctx, &ps->out, OP_END);
   return 0;
}


static int
_parse_expression_statement(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_expression(ctx, &p)) {
      _emit(ctx, &p.out, OP_PUSH_VOID);
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_for_init_statement(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, OP_EXPRESSION);

   if (_parse_expression_statement(ctx, &p) == 0) {
      *ps = p;
      return 0;
   }

   if (_parse_declaration(ctx, &p) == 0) {
      _update(ctx, e, OP_DECLARE);
      *ps = p;
      return 0;
   }

   return -1;
}


static int
_parse_initializer(struct parse_context *ctx,
                   struct parse_state *ps)
{
   if (_parse_assignment_expression(ctx, ps) == 0) {
      _emit(ctx, &ps->out, OP_END);
      return 0;
   }
   return -1;
}


static int
_parse_condition_initializer(struct parse_context *ctx,
                             struct parse_state *ps)
{
   struct parse_state p = *ps;

   _emit(ctx, &p.out, OP_DECLARE);
   _emit(ctx, &p.out, DECLARATION_INIT_DECLARATOR_LIST);
   if (_parse_fully_specified_type(ctx, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, VARIABLE_IDENTIFIER);
   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_ASSIGN, &p)) {
      _error(ctx, "expected `='");
      return -1;
   }
   _emit(ctx, &p.out, VARIABLE_INITIALIZER);
   if (_parse_initializer(ctx, &p)) {
      _error(ctx, "expected an initialiser");
      return -1;
   }
   _emit(ctx, &p.out, DECLARATOR_NONE);
   *ps = p;
   return 0;
}


static int
_parse_condition(struct parse_context *ctx,
                 struct parse_state *ps)
{
   struct parse_state p;

   if (_parse_condition_initializer(ctx, ps) == 0) {
      return 0;
   }

   p = *ps;
   _emit(ctx, &p.out, OP_EXPRESSION);
   if (_parse_expression(ctx, &p) == 0) {
      _emit(ctx, &p.out, OP_END);
      *ps = p;
      return 0;
   }

   return -1;
}


static int
_parse_for_rest_statement(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_condition(ctx, &p)) {
      _emit(ctx, &p.out, OP_EXPRESSION);
      _emit(ctx, &p.out, OP_PUSH_BOOL);
      _emit(ctx, &p.out, 2);
      _emit(ctx, &p.out, '1');
      _emit(ctx, &p.out, '\0');
      _emit(ctx, &p.out, OP_END);
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   if (_parse_expression(ctx, &p)) {
      _emit(ctx, &p.out, OP_PUSH_VOID);
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_iteration_statement(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_id(ctx, ctx->dict._while, &p) == 0) {
      _emit(ctx, &p.out, OP_WHILE);
      if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
         _error(ctx, "expected `('");
         return -1;
      }
      if (_parse_condition(ctx, &p)) {
         _error(ctx, "expected an expression");
         return -1;
      }
      if (_parse_token(ctx, SL_PP_RPAREN, &p)) {
         _error(ctx, "expected `)'");
         return -1;
      }
      if (_parse_statement(ctx, &p)) {
         return -1;
      }
      *ps = p;
      return 0;
   }

   if (_parse_id(ctx, ctx->dict._do, &p) == 0) {
      _emit(ctx, &p.out, OP_DO);
      if (_parse_statement(ctx, &p)) {
         return -1;
      }
      if (_parse_id(ctx, ctx->dict._while, &p)) {
         return -1;
      }
      if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
         _error(ctx, "expected `('");
         return -1;
      }
      if (_parse_expression(ctx, &p)) {
         _error(ctx, "expected an expression");
         return -1;
      }
      if (_parse_token(ctx, SL_PP_RPAREN, &p)) {
         _error(ctx, "expected `)'");
         return -1;
      }
      _emit(ctx, &p.out, OP_END);
      if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
         _error(ctx, "expected `;'");
         return -1;
      }
      *ps = p;
      return 0;
   }

   if (_parse_id(ctx, ctx->dict._for, &p) == 0) {
      _emit(ctx, &p.out, OP_FOR);
      if (_parse_token(ctx, SL_PP_LPAREN, &p)) {
         _error(ctx, "expected `('");
         return -1;
      }
      if (_parse_for_init_statement(ctx, &p)) {
         return -1;
      }
      if (_parse_for_rest_statement(ctx, &p)) {
         return -1;
      }
      if (_parse_token(ctx, SL_PP_RPAREN, &p)) {
         _error(ctx, "expected `)'");
         return -1;
      }
      if (_parse_statement(ctx, &p)) {
         return -1;
      }
      *ps = p;
      return 0;
   }

   return -1;
}


static int
_parse_jump_statement(struct parse_context *ctx,
                      struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, 0);

   if (_parse_id(ctx, ctx->dict._continue, &p) == 0) {
      _update(ctx, e, OP_CONTINUE);
   } else if (_parse_id(ctx, ctx->dict._break, &p) == 0) {
      _update(ctx, e, OP_BREAK);
   } else if (_parse_id(ctx, ctx->dict._return, &p) == 0) {
      _update(ctx, e, OP_RETURN);
      if (_parse_expression(ctx, &p)) {
         _emit(ctx, &p.out, OP_PUSH_VOID);
      }
      _emit(ctx, &p.out, OP_END);
   } else if (ctx->shader_type == 1 && _parse_id(ctx, ctx->dict.discard, &p) == 0) {
      _update(ctx, e, OP_DISCARD);
   } else {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_simple_statement(struct parse_context *ctx,
                        struct parse_state *ps)
{
   struct parse_state p;
   unsigned int e;

   if (_parse_selection_statement(ctx, ps) == 0) {
      return 0;
   }

   if (_parse_iteration_statement(ctx, ps) == 0) {
      return 0;
   }

   if (_parse_jump_statement(ctx, ps) == 0) {
      return 0;
   }

   p = *ps;
   e = _emit(ctx, &p.out, OP_EXPRESSION);
   if (_parse_expression_statement(ctx, &p) == 0) {
      *ps = p;
      return 0;
   }

   if (_parse_precision_stmt(ctx, &p) == 0) {
      _update(ctx, e, OP_PRECISION);
      *ps = p;
      return 0;
   }

   if (ctx->parsing_builtin && _parse_asm_statement(ctx, &p) == 0) {
      _update(ctx, e, OP_ASM);
      *ps = p;
      return 0;
   }

   if (_parse_declaration(ctx, &p) == 0) {
      _update(ctx, e, OP_DECLARE);
      *ps = p;
      return 0;
   }

   return -1;
}


static int
_parse_compound_statement(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_token(ctx, SL_PP_LBRACE, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_BLOCK_BEGIN_NEW_SCOPE);
   _parse_statement_list(ctx, &p);
   if (_parse_token(ctx, SL_PP_RBRACE, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_statement(struct parse_context *ctx,
                 struct parse_state *ps)
{
   if (_parse_compound_statement(ctx, ps) == 0) {
      return 0;
   }

   if (_parse_simple_statement(ctx, ps) == 0) {
      return 0;
   }

   return -1;
}


static int
_parse_statement_list(struct parse_context *ctx,
                      struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_statement(ctx, &p)) {
      return -1;
   }

   for (;;) {
      *ps = p;
      if (_parse_statement(ctx, &p)) {
         return 0;
      }
   }
}


static int
_parse_compound_statement_no_new_scope(struct parse_context *ctx,
                                       struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_token(ctx, SL_PP_LBRACE, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_BLOCK_BEGIN_NO_NEW_SCOPE);
   _parse_statement_list(ctx, &p);
   if (_parse_token(ctx, SL_PP_RBRACE, &p)) {
      return -1;
   }
   _emit(ctx, &p.out, OP_END);
   *ps = p;
   return 0;
}


static int
_parse_function_definition(struct parse_context *ctx,
                           struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_function_prototype(ctx, &p)) {
      return -1;
   }
   if (_parse_compound_statement_no_new_scope(ctx, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_invariant_stmt(struct parse_context *ctx,
                      struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_id(ctx, ctx->dict.invariant, &p)) {
      return -1;
   }
   if (_parse_identifier(ctx, &p)) {
      return -1;
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_single_declaration(struct parse_context *ctx,
                          struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e;

   if (_parse_fully_specified_type(ctx, &p)) {
      return -1;
   }

   e = _emit(ctx, &p.out, VARIABLE_IDENTIFIER);
   if (_parse_identifier(ctx, &p)) {
      _update(ctx, e, VARIABLE_NONE);
      *ps = p;
      return 0;
   }

   e = _emit(ctx, &p.out, VARIABLE_NONE);
   *ps = p;

   if (_parse_token(ctx, SL_PP_ASSIGN, &p) == 0) {
      _update(ctx, e, VARIABLE_INITIALIZER);
      if (_parse_initializer(ctx, &p) == 0) {
         *ps = p;
         return 0;
      }
      _error(ctx, "expected an initialiser");
      return -1;
   }
   p = *ps;

   if (_parse_token(ctx, SL_PP_LBRACKET, &p) == 0) {
      if (_parse_constant_expression(ctx, &p)) {
         _update(ctx, e, VARIABLE_ARRAY_UNKNOWN);
      } else {
         _update(ctx, e, VARIABLE_ARRAY_EXPLICIT);
      }
      if (_parse_token(ctx, SL_PP_RBRACKET, &p) == 0) {
         *ps = p;
         return 0;
      }
      _error(ctx, "expected `]'");
      return -1;
   }
   return 0;
}


static int
_parse_init_declarator_list(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;

   if (_parse_single_declaration(ctx, &p)) {
      return -1;
   }

   for (;;) {
      unsigned int e;

      *ps = p;
      if (_parse_token(ctx, SL_PP_COMMA, &p)) {
         break;
      }
      _emit(ctx, &p.out, DECLARATOR_NEXT);
      _emit(ctx, &p.out, VARIABLE_IDENTIFIER);
      if (_parse_identifier(ctx, &p)) {
         break;
      }

      e = _emit(ctx, &p.out, VARIABLE_NONE);
      *ps = p;

      if (_parse_token(ctx, SL_PP_ASSIGN, &p) == 0) {
         if (_parse_initializer(ctx, &p) == 0) {
            _update(ctx, e, VARIABLE_INITIALIZER);
            *ps = p;
            continue;
         }
         _error(ctx, "expected an initialiser");
         break;
      }
      p = *ps;

      if (_parse_token(ctx, SL_PP_LBRACKET, &p) == 0) {
         unsigned int arr;

         if (_parse_constant_expression(ctx, &p)) {
            arr = VARIABLE_ARRAY_UNKNOWN;
         } else {
            arr = VARIABLE_ARRAY_EXPLICIT;
         }
         if (_parse_token(ctx, SL_PP_RBRACKET, &p) == 0) {
            _update(ctx, e, arr);
            *ps = p;
            continue;
         }
         _error(ctx, "expected `]'");
         break;
      }
      p = *ps;
   }

   _emit(ctx, &ps->out, DECLARATOR_NONE);
   return 0;
}


static int
_parse_declaration(struct parse_context *ctx,
                   struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, DECLARATION_FUNCTION_PROTOTYPE);

   if (_parse_function_prototype(ctx, &p)) {
      if (_parse_init_declarator_list(ctx, &p)) {
         return -1;
      }
      _update(ctx, e, DECLARATION_INIT_DECLARATOR_LIST);
   }
   if (_parse_token(ctx, SL_PP_SEMICOLON, &p)) {
      _error(ctx, "expected `;'");
      return -1;
   }
   *ps = p;
   return 0;
}


static int
_parse_external_declaration(struct parse_context *ctx,
                            struct parse_state *ps)
{
   struct parse_state p = *ps;
   unsigned int e = _emit(ctx, &p.out, 0);

   if (_parse_precision_stmt(ctx, &p) == 0) {
      _update(ctx, e, DEFAULT_PRECISION);
      *ps = p;
      return 0;
   }

   if (_parse_function_definition(ctx, &p) == 0) {
      _update(ctx, e, EXTERNAL_FUNCTION_DEFINITION);
      *ps = p;
      return 0;
   }

   if (_parse_invariant_stmt(ctx, &p) == 0) {
      _update(ctx, e, INVARIANT_STMT);
      *ps = p;
      return 0;
   }

   if (_parse_declaration(ctx, &p) == 0) {
      _update(ctx, e, EXTERNAL_DECLARATION);
      *ps = p;
      return 0;
   }

   _error(ctx, "expected an identifier");
   return -1;
}


static int
_parse_extensions(struct parse_context *ctx,
                  struct parse_state *ps)
{
   for (;;) {
      const struct sl_pp_token_info *input = _fetch_token(ctx, ps->in);
      unsigned int enable;

      if (!input) {
         return -1;
      }

      switch (input->token) {
      case SL_PP_EXTENSION_REQUIRE:
      case SL_PP_EXTENSION_ENABLE:
      case SL_PP_EXTENSION_WARN:
         enable = 1;
         break;
      case SL_PP_EXTENSION_DISABLE:
         enable = 0;
         break;
      default:
         return 0;
      }

      ps->in++;
      if (input->data.extension == ctx->dict.all) {
         ctx->fragment_coord_conventions = enable;
      }
      else if (input->data.extension == ctx->dict._GL_ARB_fragment_coord_conventions) {
         ctx->fragment_coord_conventions = enable;
      }
   }
}


static int
_parse_translation_unit(struct parse_context *ctx,
                        struct parse_state *ps)
{
   _emit(ctx, &ps->out, REVISION);
   if (_parse_extensions(ctx, ps)) {
      return -1;
   }
   if (_parse_external_declaration(ctx, ps)) {
      return -1;
   }
   for (;;) {
      if (_parse_extensions(ctx, ps)) {
         return -1;
      }
      if (_parse_external_declaration(ctx, ps)) {
         break;
      }
   }
   _emit(ctx, &ps->out, EXTERNAL_NULL);
   if (_parse_token(ctx, SL_PP_EOF, ps)) {
      return -1;
   }
   return 0;
}


#define ADD_NAME_STR(CTX, NAME, STR)\
   do {\
      (CTX).dict.NAME = sl_pp_context_add_unique_str((CTX).context, (STR));\
      if ((CTX).dict.NAME == -1) {\
         return -1;\
      }\
   } while (0)

#define ADD_NAME(CTX, NAME) ADD_NAME_STR(CTX, NAME, #NAME)


int
sl_cl_compile(struct sl_pp_context *context,
              unsigned int shader_type,
              unsigned int parsing_builtin,
              unsigned char **output,
              unsigned int *cboutput,
              char *error,
              unsigned int cberror)
{
   struct parse_context ctx;
   struct parse_state ps;

   ctx.context = context;

   ADD_NAME_STR(ctx, _void, "void");
   ADD_NAME_STR(ctx, _float, "float");
   ADD_NAME_STR(ctx, _int, "int");
   ADD_NAME_STR(ctx, _bool, "bool");
   ADD_NAME(ctx, vec2);
   ADD_NAME(ctx, vec3);
   ADD_NAME(ctx, vec4);
   ADD_NAME(ctx, bvec2);
   ADD_NAME(ctx, bvec3);
   ADD_NAME(ctx, bvec4);
   ADD_NAME(ctx, ivec2);
   ADD_NAME(ctx, ivec3);
   ADD_NAME(ctx, ivec4);
   ADD_NAME(ctx, mat2);
   ADD_NAME(ctx, mat3);
   ADD_NAME(ctx, mat4);
   ADD_NAME(ctx, mat2x3);
   ADD_NAME(ctx, mat3x2);
   ADD_NAME(ctx, mat2x4);
   ADD_NAME(ctx, mat4x2);
   ADD_NAME(ctx, mat3x4);
   ADD_NAME(ctx, mat4x3);
   ADD_NAME(ctx, sampler1D);
   ADD_NAME(ctx, sampler2D);
   ADD_NAME(ctx, sampler3D);
   ADD_NAME(ctx, samplerCube);
   ADD_NAME(ctx, sampler1DShadow);
   ADD_NAME(ctx, sampler2DShadow);
   ADD_NAME(ctx, sampler2DRect);
   ADD_NAME(ctx, sampler2DRectShadow);
   ADD_NAME(ctx, sampler1DArray);
   ADD_NAME(ctx, sampler2DArray);
   ADD_NAME(ctx, sampler1DArrayShadow);
   ADD_NAME(ctx, sampler2DArrayShadow);

   ADD_NAME(ctx, invariant);

   ADD_NAME(ctx, centroid);

   ADD_NAME(ctx, precision);
   ADD_NAME(ctx, lowp);
   ADD_NAME(ctx, mediump);
   ADD_NAME(ctx, highp);

   ADD_NAME_STR(ctx, _const, "const");
   ADD_NAME(ctx, attribute);
   ADD_NAME(ctx, varying);
   ADD_NAME(ctx, uniform);
   ADD_NAME(ctx, __fixed_output);
   ADD_NAME(ctx, __fixed_input);

   ADD_NAME(ctx, in);
   ADD_NAME(ctx, out);
   ADD_NAME(ctx, inout);

   ADD_NAME(ctx, layout);
   ADD_NAME(ctx, origin_upper_left);
   ADD_NAME(ctx, pixel_center_integer);

   ADD_NAME_STR(ctx, _struct, "struct");

   ADD_NAME(ctx, __constructor);
   ADD_NAME(ctx, __operator);
   ADD_NAME_STR(ctx, ___asm, "__asm");

   ADD_NAME_STR(ctx, _if, "if");
   ADD_NAME_STR(ctx, _else, "else");
   ADD_NAME_STR(ctx, _for, "for");
   ADD_NAME_STR(ctx, _while, "while");
   ADD_NAME_STR(ctx, _do, "do");

   ADD_NAME_STR(ctx, _continue, "continue");
   ADD_NAME_STR(ctx, _break, "break");
   ADD_NAME_STR(ctx, _return, "return");
   ADD_NAME(ctx, discard);

   ADD_NAME_STR(ctx, _false, "false");
   ADD_NAME_STR(ctx, _true, "true");

   ADD_NAME(ctx, all);
   ADD_NAME_STR(ctx, _GL_ARB_fragment_coord_conventions, "GL_ARB_fragment_coord_conventions");

   ctx.out_buf = NULL;
   ctx.out_cap = 0;

   ctx.shader_type = shader_type;
   ctx.parsing_builtin = 1;

   ctx.fragment_coord_conventions = 0;

   ctx.error[0] = '\0';
   ctx.process_error = 0;

   ctx.tokens_cap = 1024;
   ctx.tokens_read = 0;
   ctx.tokens = malloc(ctx.tokens_cap * sizeof(struct sl_pp_token_info));
   if (!ctx.tokens) {
      strncpy(error, "out of memory", cberror - 1);
      error[cberror - 1] = '\0';
      return -1;
   }

   ps.in = 0;
   ps.out = 0;

   if (_parse_translation_unit(&ctx, &ps)) {
      strncpy(error, ctx.error, cberror);
      free(ctx.tokens);
      return -1;
   }

   *output = ctx.out_buf;
   *cboutput = ps.out;
   free(ctx.tokens);
   return 0;
}
