/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison implementation for Yacc-like parsers in C

   Copyright (C) 1984, 1989-1990, 2000-2015, 2018-2021 Free Software Foundation,
   Inc.

   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <https://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.

   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */

/* C LALR(1) parser skeleton written by Richard Stallman, by
   simplifying the original so-called "semantic" parser.  */

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

/* All symbols defined below should begin with yy or YY, to avoid
   infringing on user name space.  This should be done even for local
   variables, as they might otherwise be expanded by user macros.
   There are some unavoidable exceptions within include files to
   define necessary library symbols; they are noted "INFRINGES ON
   USER NAME SPACE" below.  */

/* Identify Bison output, and Bison version.  */
#define YYBISON 30802

/* Bison version string.  */
#define YYBISON_VERSION "3.8.2"

/* Skeleton name.  */
#define YYSKELETON_NAME "yacc.c"

/* Pure parsers.  */
#define YYPURE 1

/* Push parsers.  */
#define YYPUSH 0

/* Pull parsers.  */
#define YYPULL 1


/* Substitute the variable and function names.  */
#define yyparse         _mesa_glsl_parse
#define yylex           _mesa_glsl_lex
#define yyerror         _mesa_glsl_error
#define yydebug         _mesa_glsl_debug
#define yynerrs         _mesa_glsl_nerrs

/* First part of user prologue.  */
#line 1 "src/compiler/glsl/glsl_parser.yy"

/*
 * Copyright © 2008, 2009 Intel Corporation
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef _MSC_VER
#include <strings.h>
#endif
#include <assert.h>

#include "ast.h"
#include "glsl_parser_extras.h"
#include "compiler/glsl_types.h"
#include "util/blend.h"
#include "util/u_string.h"
#include "util/format/u_format.h"
#include "main/consts_exts.h"

#ifdef _MSC_VER
#pragma warning( disable : 4065 ) // switch statement contains 'default' but no 'case' labels
#endif

#undef yyerror

static void yyerror(YYLTYPE *loc, _mesa_glsl_parse_state *st, const char *msg)
{
   _mesa_glsl_error(loc, st, "%s", msg);
}

static int
_mesa_glsl_lex(YYSTYPE *val, YYLTYPE *loc, _mesa_glsl_parse_state *state)
{
   return _mesa_glsl_lexer_lex(val, loc, state->scanner);
}

static bool match_layout_qualifier(const char *s1, const char *s2,
                                   _mesa_glsl_parse_state *state)
{
   /* From the GLSL 1.50 spec, section 4.3.8 (Layout Qualifiers):
    *
    *     "The tokens in any layout-qualifier-id-list ... are not case
    *     sensitive, unless explicitly noted otherwise."
    *
    * The text "unless explicitly noted otherwise" appears to be
    * vacuous--no desktop GLSL spec (up through GLSL 4.40) notes
    * otherwise.
    *
    * However, the GLSL ES 3.00 spec says, in section 4.3.8 (Layout
    * Qualifiers):
    *
    *     "As for other identifiers, they are case sensitive."
    *
    * So we need to do a case-sensitive or a case-insensitive match,
    * depending on whether we are compiling for GLSL ES.
    */
   if (state->es_shader)
      return strcmp(s1, s2);
   else
      return strcasecmp(s1, s2);
}

#line 159 "src/compiler/glsl/glsl_parser.cpp"

# ifndef YY_CAST
#  ifdef __cplusplus
#   define YY_CAST(Type, Val) static_cast<Type> (Val)
#   define YY_REINTERPRET_CAST(Type, Val) reinterpret_cast<Type> (Val)
#  else
#   define YY_CAST(Type, Val) ((Type) (Val))
#   define YY_REINTERPRET_CAST(Type, Val) ((Type) (Val))
#  endif
# endif
# ifndef YY_NULLPTR
#  if defined __cplusplus
#   if 201103L <= __cplusplus
#    define YY_NULLPTR nullptr
#   else
#    define YY_NULLPTR 0
#   endif
#  else
#   define YY_NULLPTR ((void*)0)
#  endif
# endif

#include "glsl_parser.h"
/* Symbol kind.  */
enum yysymbol_kind_t
{
  YYSYMBOL_YYEMPTY = -2,
  YYSYMBOL_YYEOF = 0,                      /* "end of file"  */
  YYSYMBOL_YYerror = 1,                    /* error  */
  YYSYMBOL_YYUNDEF = 2,                    /* "invalid token"  */
  YYSYMBOL_ATTRIBUTE = 3,                  /* ATTRIBUTE  */
  YYSYMBOL_CONST_TOK = 4,                  /* CONST_TOK  */
  YYSYMBOL_BASIC_TYPE_TOK = 5,             /* BASIC_TYPE_TOK  */
  YYSYMBOL_BREAK = 6,                      /* BREAK  */
  YYSYMBOL_BUFFER = 7,                     /* BUFFER  */
  YYSYMBOL_CONTINUE = 8,                   /* CONTINUE  */
  YYSYMBOL_DO = 9,                         /* DO  */
  YYSYMBOL_ELSE = 10,                      /* ELSE  */
  YYSYMBOL_FOR = 11,                       /* FOR  */
  YYSYMBOL_IF = 12,                        /* IF  */
  YYSYMBOL_DEMOTE = 13,                    /* DEMOTE  */
  YYSYMBOL_DISCARD = 14,                   /* DISCARD  */
  YYSYMBOL_RETURN = 15,                    /* RETURN  */
  YYSYMBOL_SWITCH = 16,                    /* SWITCH  */
  YYSYMBOL_CASE = 17,                      /* CASE  */
  YYSYMBOL_DEFAULT = 18,                   /* DEFAULT  */
  YYSYMBOL_CENTROID = 19,                  /* CENTROID  */
  YYSYMBOL_IN_TOK = 20,                    /* IN_TOK  */
  YYSYMBOL_OUT_TOK = 21,                   /* OUT_TOK  */
  YYSYMBOL_INOUT_TOK = 22,                 /* INOUT_TOK  */
  YYSYMBOL_UNIFORM = 23,                   /* UNIFORM  */
  YYSYMBOL_VARYING = 24,                   /* VARYING  */
  YYSYMBOL_SAMPLE = 25,                    /* SAMPLE  */
  YYSYMBOL_NOPERSPECTIVE = 26,             /* NOPERSPECTIVE  */
  YYSYMBOL_FLAT = 27,                      /* FLAT  */
  YYSYMBOL_SMOOTH = 28,                    /* SMOOTH  */
  YYSYMBOL_PIXEL_LOCAL = 29,               /* PIXEL_LOCAL  */
  YYSYMBOL_PIXEL_LOCAL_IN = 30,            /* PIXEL_LOCAL_IN  */
  YYSYMBOL_PIXEL_LOCAL_OUT = 31,           /* PIXEL_LOCAL_OUT  */
  YYSYMBOL_IMAGE1DSHADOW = 32,             /* IMAGE1DSHADOW  */
  YYSYMBOL_IMAGE2DSHADOW = 33,             /* IMAGE2DSHADOW  */
  YYSYMBOL_IMAGE1DARRAYSHADOW = 34,        /* IMAGE1DARRAYSHADOW  */
  YYSYMBOL_IMAGE2DARRAYSHADOW = 35,        /* IMAGE2DARRAYSHADOW  */
  YYSYMBOL_COHERENT = 36,                  /* COHERENT  */
  YYSYMBOL_VOLATILE = 37,                  /* VOLATILE  */
  YYSYMBOL_RESTRICT = 38,                  /* RESTRICT  */
  YYSYMBOL_READONLY = 39,                  /* READONLY  */
  YYSYMBOL_WRITEONLY = 40,                 /* WRITEONLY  */
  YYSYMBOL_SHARED = 41,                    /* SHARED  */
  YYSYMBOL_TASKPAYLOAD = 42,               /* TASKPAYLOAD  */
  YYSYMBOL_PERPRIMITIVE = 43,              /* PERPRIMITIVE  */
  YYSYMBOL_STRUCT = 44,                    /* STRUCT  */
  YYSYMBOL_VOID_TOK = 45,                  /* VOID_TOK  */
  YYSYMBOL_WHILE = 46,                     /* WHILE  */
  YYSYMBOL_IDENTIFIER = 47,                /* IDENTIFIER  */
  YYSYMBOL_TYPE_IDENTIFIER = 48,           /* TYPE_IDENTIFIER  */
  YYSYMBOL_NEW_IDENTIFIER = 49,            /* NEW_IDENTIFIER  */
  YYSYMBOL_FLOATCONSTANT = 50,             /* FLOATCONSTANT  */
  YYSYMBOL_FLOAT16CONSTANT = 51,           /* FLOAT16CONSTANT  */
  YYSYMBOL_DOUBLECONSTANT = 52,            /* DOUBLECONSTANT  */
  YYSYMBOL_INTCONSTANT = 53,               /* INTCONSTANT  */
  YYSYMBOL_UINTCONSTANT = 54,              /* UINTCONSTANT  */
  YYSYMBOL_BOOLCONSTANT = 55,              /* BOOLCONSTANT  */
  YYSYMBOL_INT64CONSTANT = 56,             /* INT64CONSTANT  */
  YYSYMBOL_UINT64CONSTANT = 57,            /* UINT64CONSTANT  */
  YYSYMBOL_FIELD_SELECTION = 58,           /* FIELD_SELECTION  */
  YYSYMBOL_LEFT_OP = 59,                   /* LEFT_OP  */
  YYSYMBOL_RIGHT_OP = 60,                  /* RIGHT_OP  */
  YYSYMBOL_INC_OP = 61,                    /* INC_OP  */
  YYSYMBOL_DEC_OP = 62,                    /* DEC_OP  */
  YYSYMBOL_LE_OP = 63,                     /* LE_OP  */
  YYSYMBOL_GE_OP = 64,                     /* GE_OP  */
  YYSYMBOL_EQ_OP = 65,                     /* EQ_OP  */
  YYSYMBOL_NE_OP = 66,                     /* NE_OP  */
  YYSYMBOL_AND_OP = 67,                    /* AND_OP  */
  YYSYMBOL_OR_OP = 68,                     /* OR_OP  */
  YYSYMBOL_XOR_OP = 69,                    /* XOR_OP  */
  YYSYMBOL_MUL_ASSIGN = 70,                /* MUL_ASSIGN  */
  YYSYMBOL_DIV_ASSIGN = 71,                /* DIV_ASSIGN  */
  YYSYMBOL_ADD_ASSIGN = 72,                /* ADD_ASSIGN  */
  YYSYMBOL_MOD_ASSIGN = 73,                /* MOD_ASSIGN  */
  YYSYMBOL_LEFT_ASSIGN = 74,               /* LEFT_ASSIGN  */
  YYSYMBOL_RIGHT_ASSIGN = 75,              /* RIGHT_ASSIGN  */
  YYSYMBOL_AND_ASSIGN = 76,                /* AND_ASSIGN  */
  YYSYMBOL_XOR_ASSIGN = 77,                /* XOR_ASSIGN  */
  YYSYMBOL_OR_ASSIGN = 78,                 /* OR_ASSIGN  */
  YYSYMBOL_SUB_ASSIGN = 79,                /* SUB_ASSIGN  */
  YYSYMBOL_INVARIANT = 80,                 /* INVARIANT  */
  YYSYMBOL_PRECISE = 81,                   /* PRECISE  */
  YYSYMBOL_LOWP = 82,                      /* LOWP  */
  YYSYMBOL_MEDIUMP = 83,                   /* MEDIUMP  */
  YYSYMBOL_HIGHP = 84,                     /* HIGHP  */
  YYSYMBOL_SUPERP = 85,                    /* SUPERP  */
  YYSYMBOL_PRECISION = 86,                 /* PRECISION  */
  YYSYMBOL_VERSION_TOK = 87,               /* VERSION_TOK  */
  YYSYMBOL_EXTENSION = 88,                 /* EXTENSION  */
  YYSYMBOL_LINE = 89,                      /* LINE  */
  YYSYMBOL_COLON = 90,                     /* COLON  */
  YYSYMBOL_EOL = 91,                       /* EOL  */
  YYSYMBOL_INTERFACE_TOK = 92,             /* INTERFACE_TOK  */
  YYSYMBOL_OUTPUT = 93,                    /* OUTPUT  */
  YYSYMBOL_PRAGMA_DEBUG_ON = 94,           /* PRAGMA_DEBUG_ON  */
  YYSYMBOL_PRAGMA_DEBUG_OFF = 95,          /* PRAGMA_DEBUG_OFF  */
  YYSYMBOL_PRAGMA_OPTIMIZE_ON = 96,        /* PRAGMA_OPTIMIZE_ON  */
  YYSYMBOL_PRAGMA_OPTIMIZE_OFF = 97,       /* PRAGMA_OPTIMIZE_OFF  */
  YYSYMBOL_PRAGMA_WARNING_ON = 98,         /* PRAGMA_WARNING_ON  */
  YYSYMBOL_PRAGMA_WARNING_OFF = 99,        /* PRAGMA_WARNING_OFF  */
  YYSYMBOL_PRAGMA_INVARIANT_ALL = 100,     /* PRAGMA_INVARIANT_ALL  */
  YYSYMBOL_LAYOUT_TOK = 101,               /* LAYOUT_TOK  */
  YYSYMBOL_DOT_TOK = 102,                  /* DOT_TOK  */
  YYSYMBOL_ASM = 103,                      /* ASM  */
  YYSYMBOL_CLASS = 104,                    /* CLASS  */
  YYSYMBOL_UNION = 105,                    /* UNION  */
  YYSYMBOL_ENUM = 106,                     /* ENUM  */
  YYSYMBOL_TYPEDEF = 107,                  /* TYPEDEF  */
  YYSYMBOL_TEMPLATE = 108,                 /* TEMPLATE  */
  YYSYMBOL_THIS = 109,                     /* THIS  */
  YYSYMBOL_PACKED_TOK = 110,               /* PACKED_TOK  */
  YYSYMBOL_GOTO = 111,                     /* GOTO  */
  YYSYMBOL_INLINE_TOK = 112,               /* INLINE_TOK  */
  YYSYMBOL_NOINLINE = 113,                 /* NOINLINE  */
  YYSYMBOL_PUBLIC_TOK = 114,               /* PUBLIC_TOK  */
  YYSYMBOL_STATIC = 115,                   /* STATIC  */
  YYSYMBOL_EXTERN = 116,                   /* EXTERN  */
  YYSYMBOL_EXTERNAL = 117,                 /* EXTERNAL  */
  YYSYMBOL_LONG_TOK = 118,                 /* LONG_TOK  */
  YYSYMBOL_SHORT_TOK = 119,                /* SHORT_TOK  */
  YYSYMBOL_HALF = 120,                     /* HALF  */
  YYSYMBOL_FIXED_TOK = 121,                /* FIXED_TOK  */
  YYSYMBOL_UNSIGNED = 122,                 /* UNSIGNED  */
  YYSYMBOL_INPUT_TOK = 123,                /* INPUT_TOK  */
  YYSYMBOL_HVEC2 = 124,                    /* HVEC2  */
  YYSYMBOL_HVEC3 = 125,                    /* HVEC3  */
  YYSYMBOL_HVEC4 = 126,                    /* HVEC4  */
  YYSYMBOL_FVEC2 = 127,                    /* FVEC2  */
  YYSYMBOL_FVEC3 = 128,                    /* FVEC3  */
  YYSYMBOL_FVEC4 = 129,                    /* FVEC4  */
  YYSYMBOL_SAMPLER3DRECT = 130,            /* SAMPLER3DRECT  */
  YYSYMBOL_SIZEOF = 131,                   /* SIZEOF  */
  YYSYMBOL_CAST = 132,                     /* CAST  */
  YYSYMBOL_NAMESPACE = 133,                /* NAMESPACE  */
  YYSYMBOL_USING = 134,                    /* USING  */
  YYSYMBOL_RESOURCE = 135,                 /* RESOURCE  */
  YYSYMBOL_PATCH = 136,                    /* PATCH  */
  YYSYMBOL_SUBROUTINE = 137,               /* SUBROUTINE  */
  YYSYMBOL_ERROR_TOK = 138,                /* ERROR_TOK  */
  YYSYMBOL_COMMON = 139,                   /* COMMON  */
  YYSYMBOL_PARTITION = 140,                /* PARTITION  */
  YYSYMBOL_ACTIVE = 141,                   /* ACTIVE  */
  YYSYMBOL_FILTER = 142,                   /* FILTER  */
  YYSYMBOL_ROW_MAJOR = 143,                /* ROW_MAJOR  */
  YYSYMBOL_THEN = 144,                     /* THEN  */
  YYSYMBOL_145_ = 145,                     /* '('  */
  YYSYMBOL_146_ = 146,                     /* ')'  */
  YYSYMBOL_147_ = 147,                     /* '['  */
  YYSYMBOL_148_ = 148,                     /* ']'  */
  YYSYMBOL_149_ = 149,                     /* ','  */
  YYSYMBOL_150_ = 150,                     /* '+'  */
  YYSYMBOL_151_ = 151,                     /* '-'  */
  YYSYMBOL_152_ = 152,                     /* '!'  */
  YYSYMBOL_153_ = 153,                     /* '~'  */
  YYSYMBOL_154_ = 154,                     /* '*'  */
  YYSYMBOL_155_ = 155,                     /* '/'  */
  YYSYMBOL_156_ = 156,                     /* '%'  */
  YYSYMBOL_157_ = 157,                     /* '<'  */
  YYSYMBOL_158_ = 158,                     /* '>'  */
  YYSYMBOL_159_ = 159,                     /* '&'  */
  YYSYMBOL_160_ = 160,                     /* '^'  */
  YYSYMBOL_161_ = 161,                     /* '|'  */
  YYSYMBOL_162_ = 162,                     /* '?'  */
  YYSYMBOL_163_ = 163,                     /* ':'  */
  YYSYMBOL_164_ = 164,                     /* '='  */
  YYSYMBOL_165_ = 165,                     /* ';'  */
  YYSYMBOL_166_ = 166,                     /* '{'  */
  YYSYMBOL_167_ = 167,                     /* '}'  */
  YYSYMBOL_YYACCEPT = 168,                 /* $accept  */
  YYSYMBOL_translation_unit = 169,         /* translation_unit  */
  YYSYMBOL_170_1 = 170,                    /* $@1  */
  YYSYMBOL_version_statement = 171,        /* version_statement  */
  YYSYMBOL_pragma_statement = 172,         /* pragma_statement  */
  YYSYMBOL_extension_statement_list = 173, /* extension_statement_list  */
  YYSYMBOL_any_identifier = 174,           /* any_identifier  */
  YYSYMBOL_extension_statement = 175,      /* extension_statement  */
  YYSYMBOL_external_declaration_list = 176, /* external_declaration_list  */
  YYSYMBOL_variable_identifier = 177,      /* variable_identifier  */
  YYSYMBOL_primary_expression = 178,       /* primary_expression  */
  YYSYMBOL_postfix_expression = 179,       /* postfix_expression  */
  YYSYMBOL_integer_expression = 180,       /* integer_expression  */
  YYSYMBOL_function_call = 181,            /* function_call  */
  YYSYMBOL_function_call_or_method = 182,  /* function_call_or_method  */
  YYSYMBOL_function_call_generic = 183,    /* function_call_generic  */
  YYSYMBOL_function_call_header_no_parameters = 184, /* function_call_header_no_parameters  */
  YYSYMBOL_function_call_header_with_parameters = 185, /* function_call_header_with_parameters  */
  YYSYMBOL_function_call_header = 186,     /* function_call_header  */
  YYSYMBOL_function_identifier = 187,      /* function_identifier  */
  YYSYMBOL_unary_expression = 188,         /* unary_expression  */
  YYSYMBOL_unary_operator = 189,           /* unary_operator  */
  YYSYMBOL_multiplicative_expression = 190, /* multiplicative_expression  */
  YYSYMBOL_additive_expression = 191,      /* additive_expression  */
  YYSYMBOL_shift_expression = 192,         /* shift_expression  */
  YYSYMBOL_relational_expression = 193,    /* relational_expression  */
  YYSYMBOL_equality_expression = 194,      /* equality_expression  */
  YYSYMBOL_and_expression = 195,           /* and_expression  */
  YYSYMBOL_exclusive_or_expression = 196,  /* exclusive_or_expression  */
  YYSYMBOL_inclusive_or_expression = 197,  /* inclusive_or_expression  */
  YYSYMBOL_logical_and_expression = 198,   /* logical_and_expression  */
  YYSYMBOL_logical_xor_expression = 199,   /* logical_xor_expression  */
  YYSYMBOL_logical_or_expression = 200,    /* logical_or_expression  */
  YYSYMBOL_conditional_expression = 201,   /* conditional_expression  */
  YYSYMBOL_assignment_expression = 202,    /* assignment_expression  */
  YYSYMBOL_assignment_operator = 203,      /* assignment_operator  */
  YYSYMBOL_expression = 204,               /* expression  */
  YYSYMBOL_constant_expression = 205,      /* constant_expression  */
  YYSYMBOL_declaration = 206,              /* declaration  */
  YYSYMBOL_function_prototype = 207,       /* function_prototype  */
  YYSYMBOL_function_declarator = 208,      /* function_declarator  */
  YYSYMBOL_function_header_with_parameters = 209, /* function_header_with_parameters  */
  YYSYMBOL_function_header = 210,          /* function_header  */
  YYSYMBOL_parameter_declarator = 211,     /* parameter_declarator  */
  YYSYMBOL_parameter_declaration = 212,    /* parameter_declaration  */
  YYSYMBOL_parameter_qualifier = 213,      /* parameter_qualifier  */
  YYSYMBOL_parameter_direction_qualifier = 214, /* parameter_direction_qualifier  */
  YYSYMBOL_parameter_type_specifier = 215, /* parameter_type_specifier  */
  YYSYMBOL_init_declarator_list = 216,     /* init_declarator_list  */
  YYSYMBOL_single_declaration = 217,       /* single_declaration  */
  YYSYMBOL_fully_specified_type = 218,     /* fully_specified_type  */
  YYSYMBOL_layout_qualifier = 219,         /* layout_qualifier  */
  YYSYMBOL_layout_qualifier_id_list = 220, /* layout_qualifier_id_list  */
  YYSYMBOL_layout_qualifier_id = 221,      /* layout_qualifier_id  */
  YYSYMBOL_interface_block_layout_qualifier = 222, /* interface_block_layout_qualifier  */
  YYSYMBOL_subroutine_qualifier = 223,     /* subroutine_qualifier  */
  YYSYMBOL_subroutine_type_list = 224,     /* subroutine_type_list  */
  YYSYMBOL_interpolation_qualifier = 225,  /* interpolation_qualifier  */
  YYSYMBOL_type_qualifier = 226,           /* type_qualifier  */
  YYSYMBOL_auxiliary_storage_qualifier = 227, /* auxiliary_storage_qualifier  */
  YYSYMBOL_storage_qualifier = 228,        /* storage_qualifier  */
  YYSYMBOL_memory_qualifier = 229,         /* memory_qualifier  */
  YYSYMBOL_array_specifier = 230,          /* array_specifier  */
  YYSYMBOL_type_specifier = 231,           /* type_specifier  */
  YYSYMBOL_type_specifier_nonarray = 232,  /* type_specifier_nonarray  */
  YYSYMBOL_basic_type_specifier_nonarray = 233, /* basic_type_specifier_nonarray  */
  YYSYMBOL_precision_qualifier = 234,      /* precision_qualifier  */
  YYSYMBOL_struct_specifier = 235,         /* struct_specifier  */
  YYSYMBOL_struct_declaration_list = 236,  /* struct_declaration_list  */
  YYSYMBOL_struct_declaration = 237,       /* struct_declaration  */
  YYSYMBOL_struct_declarator_list = 238,   /* struct_declarator_list  */
  YYSYMBOL_struct_declarator = 239,        /* struct_declarator  */
  YYSYMBOL_initializer = 240,              /* initializer  */
  YYSYMBOL_initializer_list = 241,         /* initializer_list  */
  YYSYMBOL_declaration_statement = 242,    /* declaration_statement  */
  YYSYMBOL_statement = 243,                /* statement  */
  YYSYMBOL_simple_statement = 244,         /* simple_statement  */
  YYSYMBOL_compound_statement = 245,       /* compound_statement  */
  YYSYMBOL_246_2 = 246,                    /* $@2  */
  YYSYMBOL_statement_no_new_scope = 247,   /* statement_no_new_scope  */
  YYSYMBOL_compound_statement_no_new_scope = 248, /* compound_statement_no_new_scope  */
  YYSYMBOL_statement_list = 249,           /* statement_list  */
  YYSYMBOL_expression_statement = 250,     /* expression_statement  */
  YYSYMBOL_selection_statement = 251,      /* selection_statement  */
  YYSYMBOL_selection_rest_statement = 252, /* selection_rest_statement  */
  YYSYMBOL_condition = 253,                /* condition  */
  YYSYMBOL_switch_statement = 254,         /* switch_statement  */
  YYSYMBOL_switch_body = 255,              /* switch_body  */
  YYSYMBOL_case_label = 256,               /* case_label  */
  YYSYMBOL_case_label_list = 257,          /* case_label_list  */
  YYSYMBOL_case_statement = 258,           /* case_statement  */
  YYSYMBOL_case_statement_list = 259,      /* case_statement_list  */
  YYSYMBOL_iteration_statement = 260,      /* iteration_statement  */
  YYSYMBOL_for_init_statement = 261,       /* for_init_statement  */
  YYSYMBOL_conditionopt = 262,             /* conditionopt  */
  YYSYMBOL_for_rest_statement = 263,       /* for_rest_statement  */
  YYSYMBOL_jump_statement = 264,           /* jump_statement  */
  YYSYMBOL_demote_statement = 265,         /* demote_statement  */
  YYSYMBOL_external_declaration = 266,     /* external_declaration  */
  YYSYMBOL_function_definition = 267,      /* function_definition  */
  YYSYMBOL_interface_block = 268,          /* interface_block  */
  YYSYMBOL_basic_interface_block = 269,    /* basic_interface_block  */
  YYSYMBOL_interface_qualifier = 270,      /* interface_qualifier  */
  YYSYMBOL_instance_name_opt = 271,        /* instance_name_opt  */
  YYSYMBOL_member_list = 272,              /* member_list  */
  YYSYMBOL_member_declaration = 273,       /* member_declaration  */
  YYSYMBOL_layout_uniform_defaults = 274,  /* layout_uniform_defaults  */
  YYSYMBOL_layout_buffer_defaults = 275,   /* layout_buffer_defaults  */
  YYSYMBOL_layout_in_defaults = 276,       /* layout_in_defaults  */
  YYSYMBOL_layout_out_defaults = 277,      /* layout_out_defaults  */
  YYSYMBOL_layout_defaults = 278           /* layout_defaults  */
};
typedef enum yysymbol_kind_t yysymbol_kind_t;




#ifdef short
# undef short
#endif

/* On compilers that do not define __PTRDIFF_MAX__ etc., make sure
   <limits.h> and (if available) <stdint.h> are included
   so that the code can choose integer types of a good width.  */

#ifndef __PTRDIFF_MAX__
# include <limits.h> /* INFRINGES ON USER NAME SPACE */
# if defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stdint.h> /* INFRINGES ON USER NAME SPACE */
#  define YY_STDINT_H
# endif
#endif

/* Narrow types that promote to a signed type and that can represent a
   signed or unsigned integer of at least N bits.  In tables they can
   save space and decrease cache pressure.  Promoting to a signed type
   helps avoid bugs in integer arithmetic.  */

#ifdef __INT_LEAST8_MAX__
typedef __INT_LEAST8_TYPE__ yytype_int8;
#elif defined YY_STDINT_H
typedef int_least8_t yytype_int8;
#else
typedef signed char yytype_int8;
#endif

#ifdef __INT_LEAST16_MAX__
typedef __INT_LEAST16_TYPE__ yytype_int16;
#elif defined YY_STDINT_H
typedef int_least16_t yytype_int16;
#else
typedef short yytype_int16;
#endif

/* Work around bug in HP-UX 11.23, which defines these macros
   incorrectly for preprocessor constants.  This workaround can likely
   be removed in 2023, as HPE has promised support for HP-UX 11.23
   (aka HP-UX 11i v2) only through the end of 2022; see Table 2 of
   <https://h20195.www2.hpe.com/V2/getpdf.aspx/4AA4-7673ENW.pdf>.  */
#ifdef __hpux
# undef UINT_LEAST8_MAX
# undef UINT_LEAST16_MAX
# define UINT_LEAST8_MAX 255
# define UINT_LEAST16_MAX 65535
#endif

#if defined __UINT_LEAST8_MAX__ && __UINT_LEAST8_MAX__ <= __INT_MAX__
typedef __UINT_LEAST8_TYPE__ yytype_uint8;
#elif (!defined __UINT_LEAST8_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST8_MAX <= INT_MAX)
typedef uint_least8_t yytype_uint8;
#elif !defined __UINT_LEAST8_MAX__ && UCHAR_MAX <= INT_MAX
typedef unsigned char yytype_uint8;
#else
typedef short yytype_uint8;
#endif

#if defined __UINT_LEAST16_MAX__ && __UINT_LEAST16_MAX__ <= __INT_MAX__
typedef __UINT_LEAST16_TYPE__ yytype_uint16;
#elif (!defined __UINT_LEAST16_MAX__ && defined YY_STDINT_H \
       && UINT_LEAST16_MAX <= INT_MAX)
typedef uint_least16_t yytype_uint16;
#elif !defined __UINT_LEAST16_MAX__ && USHRT_MAX <= INT_MAX
typedef unsigned short yytype_uint16;
#else
typedef int yytype_uint16;
#endif

#ifndef YYPTRDIFF_T
# if defined __PTRDIFF_TYPE__ && defined __PTRDIFF_MAX__
#  define YYPTRDIFF_T __PTRDIFF_TYPE__
#  define YYPTRDIFF_MAXIMUM __PTRDIFF_MAX__
# elif defined PTRDIFF_MAX
#  ifndef ptrdiff_t
#   include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  endif
#  define YYPTRDIFF_T ptrdiff_t
#  define YYPTRDIFF_MAXIMUM PTRDIFF_MAX
# else
#  define YYPTRDIFF_T long
#  define YYPTRDIFF_MAXIMUM LONG_MAX
# endif
#endif

#ifndef YYSIZE_T
# ifdef __SIZE_TYPE__
#  define YYSIZE_T __SIZE_TYPE__
# elif defined size_t
#  define YYSIZE_T size_t
# elif defined __STDC_VERSION__ && 199901 <= __STDC_VERSION__
#  include <stddef.h> /* INFRINGES ON USER NAME SPACE */
#  define YYSIZE_T size_t
# else
#  define YYSIZE_T unsigned
# endif
#endif

#define YYSIZE_MAXIMUM                                  \
  YY_CAST (YYPTRDIFF_T,                                 \
           (YYPTRDIFF_MAXIMUM < YY_CAST (YYSIZE_T, -1)  \
            ? YYPTRDIFF_MAXIMUM                         \
            : YY_CAST (YYSIZE_T, -1)))

#define YYSIZEOF(X) YY_CAST (YYPTRDIFF_T, sizeof (X))


/* Stored state numbers (used for stacks). */
typedef yytype_int16 yy_state_t;

/* State numbers in computations.  */
typedef int yy_state_fast_t;

#ifndef YY_
# if defined YYENABLE_NLS && YYENABLE_NLS
#  if ENABLE_NLS
#   include <libintl.h> /* INFRINGES ON USER NAME SPACE */
#   define YY_(Msgid) dgettext ("bison-runtime", Msgid)
#  endif
# endif
# ifndef YY_
#  define YY_(Msgid) Msgid
# endif
#endif


#ifndef YY_ATTRIBUTE_PURE
# if defined __GNUC__ && 2 < __GNUC__ + (96 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_PURE __attribute__ ((__pure__))
# else
#  define YY_ATTRIBUTE_PURE
# endif
#endif

#ifndef YY_ATTRIBUTE_UNUSED
# if defined __GNUC__ && 2 < __GNUC__ + (7 <= __GNUC_MINOR__)
#  define YY_ATTRIBUTE_UNUSED __attribute__ ((__unused__))
# else
#  define YY_ATTRIBUTE_UNUSED
# endif
#endif

/* Suppress unused-variable warnings by "using" E.  */
#if ! defined lint || defined __GNUC__
# define YY_USE(E) ((void) (E))
#else
# define YY_USE(E) /* empty */
#endif

/* Suppress an incorrect diagnostic about yylval being uninitialized.  */
#if defined __GNUC__ && ! defined __ICC && 406 <= __GNUC__ * 100 + __GNUC_MINOR__
# if __GNUC__ * 100 + __GNUC_MINOR__ < 407
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")
# else
#  define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN                           \
    _Pragma ("GCC diagnostic push")                                     \
    _Pragma ("GCC diagnostic ignored \"-Wuninitialized\"")              \
    _Pragma ("GCC diagnostic ignored \"-Wmaybe-uninitialized\"")
# endif
# define YY_IGNORE_MAYBE_UNINITIALIZED_END      \
    _Pragma ("GCC diagnostic pop")
#else
# define YY_INITIAL_VALUE(Value) Value
#endif
#ifndef YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
# define YY_IGNORE_MAYBE_UNINITIALIZED_END
#endif
#ifndef YY_INITIAL_VALUE
# define YY_INITIAL_VALUE(Value) /* Nothing. */
#endif

#if defined __cplusplus && defined __GNUC__ && ! defined __ICC && 6 <= __GNUC__
# define YY_IGNORE_USELESS_CAST_BEGIN                          \
    _Pragma ("GCC diagnostic push")                            \
    _Pragma ("GCC diagnostic ignored \"-Wuseless-cast\"")
# define YY_IGNORE_USELESS_CAST_END            \
    _Pragma ("GCC diagnostic pop")
#endif
#ifndef YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_BEGIN
# define YY_IGNORE_USELESS_CAST_END
#endif


#define YY_ASSERT(E) ((void) (0 && (E)))

#if 1

/* The parser invokes alloca or malloc; define the necessary symbols.  */

# ifdef YYSTACK_USE_ALLOCA
#  if YYSTACK_USE_ALLOCA
#   ifdef __GNUC__
#    define YYSTACK_ALLOC __builtin_alloca
#   elif defined __BUILTIN_VA_ARG_INCR
#    include <alloca.h> /* INFRINGES ON USER NAME SPACE */
#   elif defined _AIX
#    define YYSTACK_ALLOC __alloca
#   elif defined _MSC_VER
#    include <malloc.h> /* INFRINGES ON USER NAME SPACE */
#    define alloca _alloca
#   else
#    define YYSTACK_ALLOC alloca
#    if ! defined _ALLOCA_H && ! defined EXIT_SUCCESS
#     include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
      /* Use EXIT_SUCCESS as a witness for stdlib.h.  */
#     ifndef EXIT_SUCCESS
#      define EXIT_SUCCESS 0
#     endif
#    endif
#   endif
#  endif
# endif

# ifdef YYSTACK_ALLOC
   /* Pacify GCC's 'empty if-body' warning.  */
#  define YYSTACK_FREE(Ptr) do { /* empty */; } while (0)
#  ifndef YYSTACK_ALLOC_MAXIMUM
    /* The OS might guarantee only one guard page at the bottom of the stack,
       and a page size can be as small as 4096 bytes.  So we cannot safely
       invoke alloca (N) if N exceeds 4096.  Use a slightly smaller number
       to allow for a few compiler-allocated temporary stack slots.  */
#   define YYSTACK_ALLOC_MAXIMUM 4032 /* reasonable circa 2006 */
#  endif
# else
#  define YYSTACK_ALLOC YYMALLOC
#  define YYSTACK_FREE YYFREE
#  ifndef YYSTACK_ALLOC_MAXIMUM
#   define YYSTACK_ALLOC_MAXIMUM YYSIZE_MAXIMUM
#  endif
#  if (defined __cplusplus && ! defined EXIT_SUCCESS \
       && ! ((defined YYMALLOC || defined malloc) \
             && (defined YYFREE || defined free)))
#   include <stdlib.h> /* INFRINGES ON USER NAME SPACE */
#   ifndef EXIT_SUCCESS
#    define EXIT_SUCCESS 0
#   endif
#  endif
#  ifndef YYMALLOC
#   define YYMALLOC malloc
#   if ! defined malloc && ! defined EXIT_SUCCESS
void *malloc (YYSIZE_T); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
#  ifndef YYFREE
#   define YYFREE free
#   if ! defined free && ! defined EXIT_SUCCESS
void free (void *); /* INFRINGES ON USER NAME SPACE */
#   endif
#  endif
# endif
#endif /* 1 */

#if (! defined yyoverflow \
     && (! defined __cplusplus \
         || (defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL \
             && defined YYSTYPE_IS_TRIVIAL && YYSTYPE_IS_TRIVIAL)))

/* A type that is properly aligned for any stack member.  */
union yyalloc
{
  yy_state_t yyss_alloc;
  YYSTYPE yyvs_alloc;
  YYLTYPE yyls_alloc;
};

/* The size of the maximum gap between one aligned stack and the next.  */
# define YYSTACK_GAP_MAXIMUM (YYSIZEOF (union yyalloc) - 1)

/* The size of an array large to enough to hold all stacks, each with
   N elements.  */
# define YYSTACK_BYTES(N) \
     ((N) * (YYSIZEOF (yy_state_t) + YYSIZEOF (YYSTYPE) \
             + YYSIZEOF (YYLTYPE)) \
      + 2 * YYSTACK_GAP_MAXIMUM)

# define YYCOPY_NEEDED 1

/* Relocate STACK from its old location to the new one.  The
   local variables YYSIZE and YYSTACKSIZE give the old and new number of
   elements in the stack, and YYPTR gives the new location of the
   stack.  Advance YYPTR to a properly aligned location for the next
   stack.  */
# define YYSTACK_RELOCATE(Stack_alloc, Stack)                           \
    do                                                                  \
      {                                                                 \
        YYPTRDIFF_T yynewbytes;                                         \
        YYCOPY (&yyptr->Stack_alloc, Stack, yysize);                    \
        Stack = &yyptr->Stack_alloc;                                    \
        yynewbytes = yystacksize * YYSIZEOF (*Stack) + YYSTACK_GAP_MAXIMUM; \
        yyptr += yynewbytes / YYSIZEOF (*yyptr);                        \
      }                                                                 \
    while (0)

#endif

#if defined YYCOPY_NEEDED && YYCOPY_NEEDED
/* Copy COUNT objects from SRC to DST.  The source and destination do
   not overlap.  */
# ifndef YYCOPY
#  if defined __GNUC__ && 1 < __GNUC__
#   define YYCOPY(Dst, Src, Count) \
      __builtin_memcpy (Dst, Src, YY_CAST (YYSIZE_T, (Count)) * sizeof (*(Src)))
#  else
#   define YYCOPY(Dst, Src, Count)              \
      do                                        \
        {                                       \
          YYPTRDIFF_T yyi;                      \
          for (yyi = 0; yyi < (Count); yyi++)   \
            (Dst)[yyi] = (Src)[yyi];            \
        }                                       \
      while (0)
#  endif
# endif
#endif /* !YYCOPY_NEEDED */

/* YYFINAL -- State number of the termination state.  */
#define YYFINAL  5
/* YYLAST -- Last index in YYTABLE.  */
#define YYLAST   2673

/* YYNTOKENS -- Number of terminals.  */
#define YYNTOKENS  168
/* YYNNTS -- Number of nonterminals.  */
#define YYNNTS  111
/* YYNRULES -- Number of rules.  */
#define YYNRULES  321
/* YYNSTATES -- Number of states.  */
#define YYNSTATES  484

/* YYMAXUTOK -- Last valid token kind.  */
#define YYMAXUTOK   399


/* YYTRANSLATE(TOKEN-NUM) -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex, with out-of-bounds checking.  */
#define YYTRANSLATE(YYX)                                \
  (0 <= (YYX) && (YYX) <= YYMAXUTOK                     \
   ? YY_CAST (yysymbol_kind_t, yytranslate[YYX])        \
   : YYSYMBOL_YYUNDEF)

/* YYTRANSLATE[TOKEN-NUM] -- Symbol number corresponding to TOKEN-NUM
   as returned by yylex.  */
static const yytype_uint8 yytranslate[] =
{
       0,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   152,     2,     2,     2,   156,   159,     2,
     145,   146,   154,   150,   149,   151,     2,   155,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,   163,   165,
     157,   164,   158,   162,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,   147,     2,   148,   160,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,   166,   161,   167,   153,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     2,     2,     2,     2,     1,     2,     3,     4,
       5,     6,     7,     8,     9,    10,    11,    12,    13,    14,
      15,    16,    17,    18,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    32,    33,    34,
      35,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    58,    59,    60,    61,    62,    63,    64,
      65,    66,    67,    68,    69,    70,    71,    72,    73,    74,
      75,    76,    77,    78,    79,    80,    81,    82,    83,    84,
      85,    86,    87,    88,    89,    90,    91,    92,    93,    94,
      95,    96,    97,    98,    99,   100,   101,   102,   103,   104,
     105,   106,   107,   108,   109,   110,   111,   112,   113,   114,
     115,   116,   117,   118,   119,   120,   121,   122,   123,   124,
     125,   126,   127,   128,   129,   130,   131,   132,   133,   134,
     135,   136,   137,   138,   139,   140,   141,   142,   143,   144
};

#if YYDEBUG
/* YYRLINE[YYN] -- Source line where rule number YYN was defined.  */
static const yytype_int16 yyrline[] =
{
       0,   298,   298,   297,   321,   323,   330,   340,   341,   342,
     343,   344,   368,   373,   380,   382,   386,   387,   388,   392,
     401,   409,   417,   428,   429,   433,   440,   447,   454,   461,
     468,   475,   482,   489,   496,   503,   504,   510,   514,   521,
     527,   536,   540,   544,   548,   549,   553,   554,   558,   564,
     576,   580,   586,   600,   601,   607,   613,   623,   624,   625,
     626,   630,   631,   637,   643,   652,   653,   659,   668,   669,
     675,   684,   685,   691,   697,   703,   712,   713,   719,   728,
     729,   738,   739,   748,   749,   758,   759,   768,   769,   778,
     779,   788,   789,   798,   799,   808,   809,   810,   811,   812,
     813,   814,   815,   816,   817,   818,   822,   826,   842,   846,
     851,   855,   860,   877,   881,   882,   886,   891,   899,   917,
     928,   933,   948,   956,   973,   976,   984,   992,  1004,  1016,
    1023,  1028,  1033,  1042,  1046,  1047,  1057,  1067,  1077,  1091,
    1098,  1109,  1120,  1131,  1142,  1154,  1169,  1176,  1194,  1201,
    1202,  1212,  1757,  1935,  1961,  1966,  1971,  1979,  1984,  1993,
    2002,  2014,  2019,  2024,  2033,  2038,  2043,  2044,  2045,  2046,
    2047,  2048,  2049,  2067,  2075,  2100,  2124,  2138,  2143,  2159,
    2184,  2196,  2204,  2209,  2214,  2219,  2226,  2231,  2236,  2241,
    2246,  2271,  2283,  2288,  2293,  2298,  2303,  2308,  2313,  2321,
    2326,  2331,  2337,  2342,  2350,  2358,  2364,  2374,  2385,  2386,
    2394,  2400,  2406,  2415,  2416,  2417,  2429,  2434,  2439,  2447,
    2454,  2471,  2476,  2484,  2522,  2527,  2535,  2541,  2550,  2551,
    2555,  2562,  2569,  2576,  2582,  2583,  2587,  2588,  2589,  2590,
    2591,  2592,  2593,  2597,  2604,  2603,  2617,  2618,  2622,  2628,
    2637,  2647,  2656,  2668,  2674,  2683,  2692,  2697,  2705,  2709,
    2727,  2735,  2740,  2748,  2753,  2761,  2769,  2777,  2785,  2793,
    2801,  2809,  2816,  2823,  2833,  2834,  2838,  2840,  2846,  2851,
    2860,  2866,  2872,  2878,  2884,  2893,  2902,  2903,  2904,  2905,
    2906,  2910,  2924,  2928,  2941,  2959,  2978,  2983,  2988,  2993,
    2998,  3003,  3008,  3013,  3029,  3032,  3037,  3045,  3050,  3058,
    3082,  3089,  3093,  3100,  3104,  3114,  3123,  3133,  3142,  3154,
    3176,  3186
};
#endif

/** Accessing symbol of state STATE.  */
#define YY_ACCESSING_SYMBOL(State) YY_CAST (yysymbol_kind_t, yystos[State])

#if 1
/* The user-facing name of the symbol whose (internal) number is
   YYSYMBOL.  No bounds checking.  */
static const char *yysymbol_name (yysymbol_kind_t yysymbol) YY_ATTRIBUTE_UNUSED;

/* YYTNAME[SYMBOL-NUM] -- String name of the symbol SYMBOL-NUM.
   First, the terminals, then, starting at YYNTOKENS, nonterminals.  */
static const char *const yytname[] =
{
  "\"end of file\"", "error", "\"invalid token\"", "ATTRIBUTE",
  "CONST_TOK", "BASIC_TYPE_TOK", "BREAK", "BUFFER", "CONTINUE", "DO",
  "ELSE", "FOR", "IF", "DEMOTE", "DISCARD", "RETURN", "SWITCH", "CASE",
  "DEFAULT", "CENTROID", "IN_TOK", "OUT_TOK", "INOUT_TOK", "UNIFORM",
  "VARYING", "SAMPLE", "NOPERSPECTIVE", "FLAT", "SMOOTH", "PIXEL_LOCAL",
  "PIXEL_LOCAL_IN", "PIXEL_LOCAL_OUT", "IMAGE1DSHADOW", "IMAGE2DSHADOW",
  "IMAGE1DARRAYSHADOW", "IMAGE2DARRAYSHADOW", "COHERENT", "VOLATILE",
  "RESTRICT", "READONLY", "WRITEONLY", "SHARED", "TASKPAYLOAD",
  "PERPRIMITIVE", "STRUCT", "VOID_TOK", "WHILE", "IDENTIFIER",
  "TYPE_IDENTIFIER", "NEW_IDENTIFIER", "FLOATCONSTANT", "FLOAT16CONSTANT",
  "DOUBLECONSTANT", "INTCONSTANT", "UINTCONSTANT", "BOOLCONSTANT",
  "INT64CONSTANT", "UINT64CONSTANT", "FIELD_SELECTION", "LEFT_OP",
  "RIGHT_OP", "INC_OP", "DEC_OP", "LE_OP", "GE_OP", "EQ_OP", "NE_OP",
  "AND_OP", "OR_OP", "XOR_OP", "MUL_ASSIGN", "DIV_ASSIGN", "ADD_ASSIGN",
  "MOD_ASSIGN", "LEFT_ASSIGN", "RIGHT_ASSIGN", "AND_ASSIGN", "XOR_ASSIGN",
  "OR_ASSIGN", "SUB_ASSIGN", "INVARIANT", "PRECISE", "LOWP", "MEDIUMP",
  "HIGHP", "SUPERP", "PRECISION", "VERSION_TOK", "EXTENSION", "LINE",
  "COLON", "EOL", "INTERFACE_TOK", "OUTPUT", "PRAGMA_DEBUG_ON",
  "PRAGMA_DEBUG_OFF", "PRAGMA_OPTIMIZE_ON", "PRAGMA_OPTIMIZE_OFF",
  "PRAGMA_WARNING_ON", "PRAGMA_WARNING_OFF", "PRAGMA_INVARIANT_ALL",
  "LAYOUT_TOK", "DOT_TOK", "ASM", "CLASS", "UNION", "ENUM", "TYPEDEF",
  "TEMPLATE", "THIS", "PACKED_TOK", "GOTO", "INLINE_TOK", "NOINLINE",
  "PUBLIC_TOK", "STATIC", "EXTERN", "EXTERNAL", "LONG_TOK", "SHORT_TOK",
  "HALF", "FIXED_TOK", "UNSIGNED", "INPUT_TOK", "HVEC2", "HVEC3", "HVEC4",
  "FVEC2", "FVEC3", "FVEC4", "SAMPLER3DRECT", "SIZEOF", "CAST",
  "NAMESPACE", "USING", "RESOURCE", "PATCH", "SUBROUTINE", "ERROR_TOK",
  "COMMON", "PARTITION", "ACTIVE", "FILTER", "ROW_MAJOR", "THEN", "'('",
  "')'", "'['", "']'", "','", "'+'", "'-'", "'!'", "'~'", "'*'", "'/'",
  "'%'", "'<'", "'>'", "'&'", "'^'", "'|'", "'?'", "':'", "'='", "';'",
  "'{'", "'}'", "$accept", "translation_unit", "$@1", "version_statement",
  "pragma_statement", "extension_statement_list", "any_identifier",
  "extension_statement", "external_declaration_list",
  "variable_identifier", "primary_expression", "postfix_expression",
  "integer_expression", "function_call", "function_call_or_method",
  "function_call_generic", "function_call_header_no_parameters",
  "function_call_header_with_parameters", "function_call_header",
  "function_identifier", "unary_expression", "unary_operator",
  "multiplicative_expression", "additive_expression", "shift_expression",
  "relational_expression", "equality_expression", "and_expression",
  "exclusive_or_expression", "inclusive_or_expression",
  "logical_and_expression", "logical_xor_expression",
  "logical_or_expression", "conditional_expression",
  "assignment_expression", "assignment_operator", "expression",
  "constant_expression", "declaration", "function_prototype",
  "function_declarator", "function_header_with_parameters",
  "function_header", "parameter_declarator", "parameter_declaration",
  "parameter_qualifier", "parameter_direction_qualifier",
  "parameter_type_specifier", "init_declarator_list", "single_declaration",
  "fully_specified_type", "layout_qualifier", "layout_qualifier_id_list",
  "layout_qualifier_id", "interface_block_layout_qualifier",
  "subroutine_qualifier", "subroutine_type_list",
  "interpolation_qualifier", "type_qualifier",
  "auxiliary_storage_qualifier", "storage_qualifier", "memory_qualifier",
  "array_specifier", "type_specifier", "type_specifier_nonarray",
  "basic_type_specifier_nonarray", "precision_qualifier",
  "struct_specifier", "struct_declaration_list", "struct_declaration",
  "struct_declarator_list", "struct_declarator", "initializer",
  "initializer_list", "declaration_statement", "statement",
  "simple_statement", "compound_statement", "$@2",
  "statement_no_new_scope", "compound_statement_no_new_scope",
  "statement_list", "expression_statement", "selection_statement",
  "selection_rest_statement", "condition", "switch_statement",
  "switch_body", "case_label", "case_label_list", "case_statement",
  "case_statement_list", "iteration_statement", "for_init_statement",
  "conditionopt", "for_rest_statement", "jump_statement",
  "demote_statement", "external_declaration", "function_definition",
  "interface_block", "basic_interface_block", "interface_qualifier",
  "instance_name_opt", "member_list", "member_declaration",
  "layout_uniform_defaults", "layout_buffer_defaults",
  "layout_in_defaults", "layout_out_defaults", "layout_defaults", YY_NULLPTR
};

static const char *
yysymbol_name (yysymbol_kind_t yysymbol)
{
  return yytname[yysymbol];
}
#endif

#define YYPACT_NINF (-337)

#define yypact_value_is_default(Yyn) \
  ((Yyn) == YYPACT_NINF)

#define YYTABLE_NINF (-303)

#define yytable_value_is_error(Yyn) \
  0

/* YYPACT[STATE-NUM] -- Index in YYTABLE of the portion describing
   STATE-NUM.  */
static const yytype_int16 yypact[] =
{
     -34,   -32,    58,  -337,    -7,  -337,    10,  -337,  -337,  -337,
    -337,    13,   235,  1802,  -337,  -337,    16,  -337,  -337,  -337,
      42,  -337,    62,    99,  -337,   110,  -337,  -337,  -337,  -337,
    -337,   114,   124,   141,  -337,  -337,  -337,  -337,  -337,  -337,
    -337,  -337,   -20,  -337,  -337,  2332,  2332,  -337,  -337,  -337,
     212,    53,   115,   122,   136,   142,   147,   155,    47,   210,
    -337,   117,  -337,  -337,  1698,  -337,   -50,   118,   126,  1853,
    -115,  -337,   273,  2401,  2467,  2467,    31,  2536,  2467,  2536,
    -337,   133,  -337,  2467,  -337,  -337,  -337,  -337,  -337,   229,
    -337,  -337,  -337,  -337,  -337,   235,  2266,   119,  -337,  -337,
    -337,  -337,  -337,  -337,  -337,  -337,  -337,  2467,  2467,  -337,
    2467,  -337,  2467,  2467,  -337,  -337,    31,  -337,  -337,  -337,
    -337,  -337,  -337,  -337,   -25,  -337,   235,  -337,  -337,  -337,
     840,  -337,  -337,   583,   583,  -337,  -337,  -337,   583,  -337,
       4,   583,   583,   583,   235,  -337,   144,   146,  -110,   152,
     -37,   -30,   -29,   -18,  -337,  -337,  -337,  -337,  -337,  -337,
    -337,  -337,  -337,  -337,  -337,  -337,  2536,  -337,  -337,  2052,
     154,  -337,   138,   217,   235,   930,  -337,  2266,   158,  -337,
    -337,  -337,   186,   -49,  -337,  -337,  -337,   -27,   187,   188,
    1296,   209,   211,   190,   193,   282,   216,   218,  -337,  -337,
    -337,  -337,  -337,  -337,  -337,  -337,  1924,  1924,  1924,  -337,
    -337,  -337,  -337,  -337,   195,  -337,  -337,  -337,   204,  -337,
    -337,  -337,   221,    20,  2131,   219,   181,  1924,   192,    34,
      95,     3,   131,   198,   208,   214,   302,   301,   -55,  -337,
    -337,  -106,  -337,   207,   228,  -337,  -337,  -337,  -337,   510,
    -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,
    -337,    31,   235,  -337,  -337,  -337,  -103,  1524,  -102,  -337,
    -337,  -337,  -337,  -337,  -337,  -337,  -337,   230,  -337,  2073,
    2266,  -337,   133,   -78,  -337,  -337,  -337,   997,  -337,  1924,
    -337,   -25,  -337,   235,  -337,  -337,  -337,   330,  -337,  1611,
    1924,  -337,  -337,  -337,   -72,  1924,  2011,  -337,  -337,    37,
    -337,  1460,  -337,  -337,   319,  1924,  -337,  -337,  1924,   234,
    -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,
    -337,  -337,  -337,  1924,  -337,  1924,  1924,  1924,  1924,  1924,
    1924,  1924,  1924,  1924,  1924,  1924,  1924,  1924,  1924,  1924,
    1924,  1924,  1924,  1924,  1924,  1924,  -337,  -337,  -337,  -337,
     235,   133,  1524,   -65,  1524,  -337,  -337,  1524,  -337,  -337,
     233,   235,   215,  2266,   154,   235,  -337,  -337,  -337,  -337,
    -337,   240,  -337,  -337,  2011,    73,  -337,    74,   238,   235,
     242,  -337,   675,  -337,   241,   238,  -337,  -337,  -337,  -337,
    -337,   192,   192,    34,    34,    95,    95,    95,    95,     3,
       3,   131,   198,   208,   214,   302,   301,  -124,  -337,  -337,
     154,  -337,  1524,  -337,  -132,  -337,  -337,   -53,   341,  -337,
    -337,  1924,  -337,   227,   248,  1460,   231,   232,  1296,  -337,
    -337,  1924,  -337,  1860,  -337,  -337,   133,   236,    82,  1924,
    1296,   385,  -337,   -15,  -337,  1524,  -337,  -337,  -337,  -337,
     154,  -337,   237,   238,  -337,  1460,  1924,   243,  -337,  -337,
    1132,  1460,   -11,  -337,  -337,  -337,  -117,  -337,  -337,  -337,
    -337,  -337,  1460,  -337
};

/* YYDEFACT[STATE-NUM] -- Default reduction number in state STATE-NUM.
   Performed when YYTABLE does not specify something else to do.  Zero
   means the default is an error.  */
static const yytype_int16 yydefact[] =
{
       4,     0,     0,    14,     0,     1,     2,    16,    17,    18,
       5,     0,     0,     0,    15,     6,     0,   187,   186,   214,
     193,   182,   189,   190,   191,   192,   188,   183,   163,   162,
     161,   196,   197,   198,   199,   200,   201,   202,   203,   194,
     195,   185,     0,   213,   212,   164,   165,   218,   217,   216,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     184,   157,   290,   288,     3,   287,     0,     0,   115,   124,
       0,   134,   139,   169,   171,   168,     0,   166,   167,   170,
     146,   208,   210,   172,   211,    20,   286,   112,   292,     0,
     318,   319,   320,   321,   289,     0,     0,     0,   193,   189,
     190,   192,   196,   197,   198,    23,    24,   164,   165,   144,
     169,   174,   166,   170,   145,   173,     0,     7,     8,     9,
      10,    12,    13,    11,     0,   215,     0,    22,    21,   109,
       0,   291,   113,   124,   124,   130,   131,   132,   124,   116,
       0,   124,   124,   124,     0,   110,    16,    18,   140,     0,
     193,   189,   190,   192,   176,   293,   310,   312,   314,   316,
     177,   175,   147,   178,   303,   179,   169,   181,   294,     0,
     209,   180,     0,     0,     0,     0,   221,     0,     0,   156,
     155,   154,   151,     0,   149,   153,   159,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    31,    30,
      32,    26,    27,    33,    28,    29,     0,     0,     0,    57,
      58,    59,    60,   253,   244,   248,    25,    35,    53,    37,
      42,    43,     0,     0,    47,     0,    61,     0,    65,    68,
      71,    76,    79,    81,    83,    85,    87,    89,    91,    93,
     106,     0,   233,     0,   146,   236,   250,   235,   234,     0,
     237,   238,   239,   240,   241,   242,   117,   125,   126,   122,
     123,     0,   133,   127,   129,   128,   135,     0,   141,   118,
     313,   315,   317,   311,   204,    61,   108,     0,    51,     0,
       0,    19,   226,     0,   224,   220,   222,     0,   111,     0,
     148,     0,   158,     0,   281,   280,   247,     0,   246,     0,
       0,   285,   284,   282,     0,     0,     0,    54,    55,     0,
     243,     0,    39,    40,     0,     0,    45,    44,     0,   213,
      48,    50,    96,    97,    99,    98,   101,   102,   103,   104,
     105,   100,    95,     0,    56,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   254,   249,   252,   251,
       0,   119,     0,   136,     0,   228,   143,     0,   205,   206,
       0,     0,     0,   307,   227,     0,   223,   219,   152,   150,
     160,     0,   275,   274,   277,     0,   283,     0,   258,     0,
       0,    34,     0,    38,     0,    41,    49,    94,    62,    63,
      64,    66,    67,    69,    70,    74,    75,    72,    73,    77,
      78,    80,    82,    84,    86,    88,    90,     0,   107,   120,
     121,   138,     0,   231,     0,   142,   207,     0,   304,   308,
     225,     0,   276,     0,     0,     0,     0,     0,     0,   245,
      36,     0,   137,     0,   229,   309,   305,     0,     0,   278,
       0,   257,   255,     0,   260,     0,   271,    92,   230,   232,
     306,   295,     0,   279,   273,     0,     0,     0,   261,   265,
       0,   269,     0,   259,   272,   256,     0,   264,   267,   266,
     268,   262,   270,   263
};

/* YYPGOTO[NTERM-NUM].  */
static const yytype_int16 yypgoto[] =
{
    -337,  -337,  -337,  -337,  -337,  -337,    14,     9,  -337,    23,
    -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,  -337,
     159,  -337,   -66,     0,   -74,   -31,    50,    51,    49,    54,
      55,    56,  -337,  -159,  -130,  -337,  -107,  -232,    17,    25,
    -337,  -337,  -337,  -337,   275,   101,  -337,  -337,  -337,  -337,
     -85,     1,  -337,   120,  -337,  -337,  -337,  -337,    97,   104,
    -337,    -9,  -143,   -13,  -337,  -337,   176,  -337,   239,  -142,
      41,    38,  -254,  -337,   116,  -241,  -189,  -337,  -337,  -336,
     348,   106,   121,  -337,  -337,    35,  -337,  -337,   -52,  -337,
     -51,  -337,  -337,  -337,  -337,  -337,  -337,  -337,   359,  -337,
      -1,  -337,   347,  -337,    57,  -337,   355,   358,   367,   369,
    -337
};

/* YYDEFGOTO[NTERM-NUM].  */
static const yytype_int16 yydefgoto[] =
{
       0,     2,    13,     3,    63,     6,   282,   358,    64,   216,
     217,   218,   394,   219,   220,   221,   222,   223,   224,   225,
     226,   227,   228,   229,   230,   231,   232,   233,   234,   235,
     236,   237,   238,   239,   240,   333,   241,   277,   242,   243,
      67,    68,    69,   259,   139,   140,   141,   260,    70,    71,
      72,   110,   183,   184,   185,    74,   187,    75,    76,    77,
      78,   113,   170,   278,    81,    82,    83,    84,   175,   176,
     283,   284,   366,   424,   245,   246,   247,   248,   311,   297,
     298,   249,   250,   251,   452,   390,   252,   454,   469,   470,
     471,   472,   253,   384,   433,   434,   254,   255,    85,    86,
      87,    88,    89,   447,   372,   373,    90,    91,    92,    93,
      94
};

/* YYTABLE[YYPACT[STATE-NUM]] -- What to do in state STATE-NUM.  If
   positive, shift that token.  If negative, reduce the rule whose
   number is the opposite.  If YYTABLE_NINF, syntax error.  */
static const yytype_int16 yytable[] =
{
      80,   296,   466,   467,    79,   268,   466,   467,   359,    19,
     276,   174,  -299,   353,    73,    14,   179,   443,    11,  -296,
    -297,     4,     7,     8,     9,   355,    16,     7,     8,     9,
      65,  -298,   355,   286,   144,   444,    19,   169,    66,   441,
       7,     8,     9,   355,   169,   279,   483,   370,    42,    43,
     145,    80,    44,     1,   267,    79,    97,   378,     5,   356,
     142,   362,   367,   162,    79,    73,   342,   343,   109,   114,
      79,   375,   155,   127,    73,    42,    43,   355,   168,    44,
     166,    65,   279,    80,    10,   180,   148,   376,   304,    66,
     174,  -299,   174,   386,   320,   149,   375,   290,    12,   422,
     291,   309,   456,   178,    15,    58,    95,   354,   421,   173,
     423,  -296,   445,   425,   464,   129,   130,   244,   181,   292,
     276,    79,   293,   363,   142,   142,    59,   262,   270,   142,
     276,   166,   142,   142,   142,   271,   272,   365,   182,   374,
     186,   261,   111,   115,   117,   286,    96,   273,  -297,   112,
     112,   359,   468,    59,   340,   341,   481,    79,   266,  -298,
     344,   345,    80,  -300,    80,   155,   317,   166,   442,   318,
     154,   160,   161,  -301,   163,   165,   167,   244,   112,   112,
     171,    79,   112,   391,   338,   339,   355,   112,   396,   459,
    -302,   166,   124,   385,   451,   371,   346,   347,   387,   388,
     112,   473,   174,   397,   111,   115,   118,   154,   395,   163,
     167,   112,   112,   119,   112,   125,   112,   112,   420,   435,
     436,   389,   355,   355,   475,   418,   116,   120,   462,   478,
     480,   355,   365,   121,   365,   257,   244,   365,   122,   258,
      79,   480,   263,   264,   265,   143,   123,   417,   360,   296,
     166,   322,   323,   324,   325,   326,   327,   328,   329,   330,
     331,   296,   126,   154,   132,   312,   313,    80,   405,   406,
     407,   408,   401,   402,    80,   133,   361,   388,   172,   112,
     169,   112,     7,     8,     9,   177,   244,    19,   371,   -23,
      79,   -24,   365,   244,    47,    48,    49,   269,   244,   389,
     166,   279,    79,   460,   280,   182,   314,   380,   281,   143,
     143,   457,   166,   365,   143,   409,   410,   143,   143,   143,
     146,     8,   147,   288,   448,   365,    42,    43,   275,   105,
      44,   106,   198,   199,   200,   201,   202,   203,   204,   205,
     403,   404,   463,   206,   207,   332,   335,   336,   337,   -52,
     289,   315,   294,   295,   299,   301,   300,   348,   302,   476,
      80,   305,   310,   306,   321,   307,   308,   316,   349,   351,
     352,   244,   129,   -51,   419,   350,   381,   393,   368,   244,
     -46,   426,   428,    79,   112,   431,   334,   355,   438,   440,
     446,   112,   449,   166,   450,   465,   455,   453,   411,   413,
     412,   461,   474,   437,    59,   414,   477,   415,   256,   416,
     112,   379,   427,   430,   131,   382,   287,   392,   479,   432,
     383,   482,   244,   128,   164,   244,    79,   208,   156,    79,
     429,   157,   209,   210,   211,   212,   166,   244,   275,   166,
     158,    79,   159,     0,     0,     0,     0,   303,   275,     0,
       0,   166,   244,     0,     0,     0,    79,   244,   244,     0,
       0,    79,    79,     0,     0,     0,   166,     0,     0,   244,
       0,   166,   166,    79,     0,     0,     0,   112,     0,     0,
       0,     0,     0,   166,     0,     0,     0,     0,   112,     0,
       0,     0,     0,     0,   398,   399,   400,   275,   275,   275,
     275,   275,   275,   275,   275,   275,   275,   275,   275,   275,
     275,   275,   275,    17,    18,    19,   188,    20,   189,   190,
       0,   191,   192,   193,   194,   195,   196,     0,     0,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,     0,     0,     0,     0,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,   197,   105,    44,   106,
     198,   199,   200,   201,   202,   203,   204,   205,     0,     0,
       0,   206,   207,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   134,     0,     0,
      45,    46,    47,    48,    49,     0,    50,     0,    12,     0,
       0,     0,     0,   135,   136,   137,     0,     0,     0,     0,
       0,    58,     0,     0,     0,     0,     0,     0,     0,    34,
      35,    36,    37,    38,     0,     0,     0,     0,     0,     0,
       0,     0,    59,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    60,    61,     0,     0,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
     209,   210,   211,   212,   138,    47,    48,    49,     0,     0,
       0,     0,     0,     0,     0,   213,   214,   357,    17,    18,
      19,   188,    20,   189,   190,     0,   191,   192,   193,   194,
     195,   196,     0,     0,    21,    22,    23,    24,    25,    26,
      27,    28,    29,    30,    31,    32,    33,     0,     0,     0,
       0,    34,    35,    36,    37,    38,    39,    40,    41,    42,
      43,   197,   105,    44,   106,   198,   199,   200,   201,   202,
     203,   204,   205,     0,     0,     0,   206,   207,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    45,    46,    47,    48,    49,
       0,    50,     0,    12,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    58,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    59,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    60,    61,     0,     0,     0,     0,     0,     0,     0,
     208,     0,     0,     0,     0,   209,   210,   211,   212,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     213,   214,   439,    17,    18,    19,   188,    20,   189,   190,
       0,   191,   192,   193,   194,   195,   196,     0,     0,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,     0,     0,     0,     0,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,   197,   105,    44,   106,
     198,   199,   200,   201,   202,   203,   204,   205,     0,     0,
       0,   206,   207,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,    46,    47,    48,    49,     0,    50,     0,     0,     0,
       0,     0,     0,    17,    18,    19,     0,    98,     0,     0,
       0,    58,     0,     0,     0,     0,     0,     0,     0,    21,
      99,   100,    24,   101,    26,    27,    28,    29,    30,   102,
     103,   104,    59,     0,     0,     0,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,    60,    61,    44,     0,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
     209,   210,   211,   212,     0,     0,     0,     0,     0,     0,
      17,    18,    19,     0,    98,   213,   214,   215,     0,     0,
     107,   108,    47,    48,    49,     0,    21,    99,   100,    24,
     101,    26,    27,    28,    29,    30,   102,   103,   104,     0,
       0,    58,     0,    34,    35,    36,    37,    38,    39,    40,
      41,    42,    43,     0,     0,    44,     0,     0,     0,     0,
       0,     0,    59,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    60,    61,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   107,   108,    47,
      48,    49,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   285,    58,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    59,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    60,    61,    17,    18,    19,   188,    20,
     189,   190,     0,   191,   192,   193,   194,   195,   196,   466,
     467,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,   377,     0,     0,     0,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,   197,   105,
      44,   106,   198,   199,   200,   201,   202,   203,   204,   205,
       0,     0,     0,   206,   207,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    45,    46,    47,    48,    49,     0,    50,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    58,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    60,    61,
       0,     0,     0,     0,     0,     0,     0,   208,     0,     0,
       0,     0,   209,   210,   211,   212,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,   213,   214,    17,
      18,    19,   188,    20,   189,   190,     0,   191,   192,   193,
     194,   195,   196,     0,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,     0,     0,
       0,     0,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,   197,   105,    44,   106,   198,   199,   200,   201,
     202,   203,   204,   205,     0,     0,     0,   206,   207,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    45,    46,    47,    48,
      49,     0,    50,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    58,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    59,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    60,    61,     0,     0,     0,     0,     0,     0,
       0,   208,     0,     0,     0,     0,   209,   210,   211,   212,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   213,   130,    17,    18,    19,   188,    20,   189,   190,
       0,   191,   192,   193,   194,   195,   196,     0,     0,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      32,    33,     0,     0,     0,     0,    34,    35,    36,    37,
      38,    39,    40,    41,    42,    43,   197,   105,    44,   106,
     198,   199,   200,   201,   202,   203,   204,   205,     0,     0,
       0,   206,   207,     0,     0,     0,     0,     0,     0,    19,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      45,    46,    47,    48,    49,     0,    50,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,    58,     0,     0,     0,     0,     0,     0,    42,    43,
       0,   105,    44,   106,   198,   199,   200,   201,   202,   203,
     204,   205,    59,     0,     0,   206,   207,     0,     0,     0,
       0,     0,     0,     0,     0,     0,    60,    61,     0,     0,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
     209,   210,   211,   212,    17,    18,    19,     0,    20,     0,
       0,     0,     0,     0,     0,   213,   214,     0,     0,     0,
      21,    22,    23,    24,    25,    26,    27,    28,    29,    30,
      31,    32,    33,     0,     0,     0,    59,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,     0,   105,    44,
     106,   198,   199,   200,   201,   202,   203,   204,   205,   208,
       0,     0,   206,   207,   209,   210,   211,   212,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
     364,    45,    46,    47,    48,    49,     0,    50,     0,     0,
       0,    17,    18,    19,     0,    20,     0,     0,     0,     0,
       0,     0,    58,     0,     0,     0,     0,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    32,    33,
       0,     0,     0,    59,    34,    35,    36,    37,    38,    39,
      40,    41,    42,    43,     0,     0,    44,    60,    61,     0,
       0,     0,     0,     0,     0,     0,   208,     0,     0,     0,
       0,   209,   210,   211,   212,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   213,     0,    45,    46,
      47,    48,    49,     0,    50,     0,    12,     0,     0,     0,
       0,     0,    51,    52,    53,    54,    55,    56,    57,    58,
       0,     0,     0,     0,     0,    17,    18,    19,     0,    20,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
      59,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    32,    33,    60,    61,     0,     0,    34,    35,
      36,    37,    38,    39,    40,    41,    42,    43,     0,     0,
      44,     0,     0,     0,     0,     0,     0,   134,     0,     0,
       0,     0,     0,    62,     0,    19,     0,     0,     0,     0,
       0,     0,     0,   135,   136,   137,     0,     0,     0,     0,
       0,     0,    45,    46,    47,    48,    49,     0,    50,    34,
      35,    36,    37,    38,     0,     0,    51,    52,    53,    54,
      55,    56,    57,    58,    42,    43,     0,   105,    44,   106,
     198,   199,   200,   201,   202,   203,   204,   205,     0,     0,
       0,   206,   207,     0,    59,     0,     0,     0,     0,    19,
       0,     0,     0,     0,   138,    47,    48,    49,    60,    61,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    62,    42,    43,
       0,   105,    44,   106,   198,   199,   200,   201,   202,   203,
     204,   205,    59,     0,     0,   206,   207,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,  -114,
       0,     0,     0,     0,     0,   208,     0,     0,     0,     0,
     209,   210,   211,   212,    17,    18,    19,     0,    98,     0,
       0,     0,     0,     0,     0,     0,   364,   458,     0,     0,
      21,    99,   100,    24,   101,    26,    27,    28,    29,    30,
     102,   103,   104,     0,     0,     0,    59,    34,    35,    36,
      37,    38,    39,    40,    41,    42,    43,    19,   105,    44,
     106,   198,   199,   200,   201,   202,   203,   204,   205,   208,
       0,     0,   206,   207,   209,   210,   211,   212,    19,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,   107,   108,    47,    48,    49,    42,    43,     0,   105,
      44,   106,   198,   199,   200,   201,   202,   203,   204,   205,
       0,     0,    58,   206,   207,     0,     0,    42,    43,     0,
     105,    44,   106,   198,   199,   200,   201,   202,   203,   204,
     205,     0,     0,    59,   206,   207,    19,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    60,    61,     0,
       0,     0,     0,     0,     0,     0,   208,     0,     0,     0,
       0,   209,   210,   211,   212,     0,     0,     0,     0,     0,
       0,     0,     0,     0,    59,    42,   319,     0,   105,    44,
     106,   198,   199,   200,   201,   202,   203,   204,   205,     0,
       0,     0,   206,   207,     0,    59,     0,   208,     0,     0,
     274,     0,   209,   210,   211,   212,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,   208,     0,
       0,   369,     0,   209,   210,   211,   212,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    59,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,    17,
      18,    19,     0,    98,     0,     0,   208,     0,     0,     0,
       0,   209,   210,   211,   212,    21,    99,   100,    24,   101,
      26,    27,    28,    29,    30,   102,   103,   104,     0,     0,
       0,     0,    34,    35,    36,    37,    38,    39,    40,    41,
      42,    43,     0,     0,    44,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,    17,    18,     0,     0,    98,
       0,     0,     0,     0,     0,     0,   107,   108,    47,    48,
      49,    21,    99,   100,    24,   101,    26,    27,    28,    29,
      30,   102,   103,   104,     0,     0,     0,    58,    34,    35,
      36,    37,    38,    39,    40,    41,     0,     0,     0,   105,
       0,   106,     0,     0,     0,     0,     0,     0,    59,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    60,    61,    17,    18,     0,     0,   150,     0,
       0,     0,   107,   108,    47,    48,    49,     0,     0,     0,
      21,   151,   152,    24,   153,    26,    27,    28,    29,    30,
      31,    32,    33,    58,     0,     0,     0,    34,    35,    36,
      37,    38,    39,    40,    41,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,    60,    61,
      17,    18,     0,     0,    98,     0,     0,     0,     0,     0,
       0,   107,   108,    47,    48,    49,    21,    99,   100,    24,
     101,    26,    27,    28,    29,    30,   102,   103,   104,     0,
       0,     0,    58,    34,    35,    36,    37,    38,    39,    40,
      41,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    60,    61,    17,
      18,     0,     0,    20,     0,     0,     0,   107,   108,    47,
      48,    49,     0,     0,     0,    21,    22,    23,    24,    25,
      26,    27,    28,    29,    30,    31,    32,    33,    58,     0,
       0,     0,    34,    35,    36,    37,    38,    39,    40,    41,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,    60,    61,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,   107,   108,    47,    48,
      49,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,    58,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,     0,     0,     0,     0,     0,     0,     0,     0,
       0,     0,    60,    61
};

static const yytype_int16 yycheck[] =
{
      13,   190,    17,    18,    13,   148,    17,    18,   249,     5,
     169,    96,    49,    68,    13,     6,    41,   149,     4,    49,
      49,    53,    47,    48,    49,   149,    12,    47,    48,    49,
      13,    49,   149,   175,   149,   167,     5,   147,    13,   163,
      47,    48,    49,   149,   147,   147,   163,   279,    44,    45,
     165,    64,    48,    87,   164,    64,    42,   289,     0,   165,
      69,   164,   164,    76,    73,    64,    63,    64,    45,    46,
      79,   149,    73,    64,    73,    44,    45,   149,    79,    48,
      79,    64,   147,    96,    91,   110,    72,   165,   195,    64,
     175,    49,   177,   165,   224,    72,   149,   146,    88,   164,
     149,   208,   438,   116,    91,   101,    90,   162,   362,    95,
     364,    49,   165,   367,   450,   165,   166,   130,   143,   146,
     279,   130,   149,   266,   133,   134,   122,   140,   165,   138,
     289,   130,   141,   142,   143,   165,   165,   267,   124,   282,
     126,   140,    45,    46,    91,   287,   166,   165,    49,    45,
      46,   392,   167,   122,    59,    60,   167,   166,   144,    49,
     157,   158,   175,    49,   177,   166,   146,   166,   422,   149,
      73,    74,    75,    49,    77,    78,    79,   190,    74,    75,
      83,   190,    78,   146,   150,   151,   149,    83,   318,   443,
      49,   190,   145,   300,   435,   280,    65,    66,   305,   306,
      96,   455,   287,   333,   107,   108,    91,   110,   315,   112,
     113,   107,   108,    91,   110,     5,   112,   113,   361,   146,
     146,   306,   149,   149,   465,   355,    50,    91,   146,   470,
     471,   149,   362,    91,   364,   134,   249,   367,    91,   138,
     249,   482,   141,   142,   143,    69,    91,   354,   261,   438,
     249,    70,    71,    72,    73,    74,    75,    76,    77,    78,
      79,   450,   145,   166,   146,    61,    62,   280,   342,   343,
     344,   345,   338,   339,   287,   149,   262,   384,    49,   175,
     147,   177,    47,    48,    49,   166,   299,     5,   373,   145,
     299,   145,   422,   306,    82,    83,    84,   145,   311,   384,
     299,   147,   311,   446,   166,   291,   102,   293,    91,   133,
     134,   441,   311,   443,   138,   346,   347,   141,   142,   143,
      47,    48,    49,   165,   431,   455,    44,    45,   169,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
     340,   341,   449,    61,    62,   164,   154,   155,   156,   145,
     164,   147,   165,   165,   145,   165,   145,   159,   165,   466,
     373,   145,   167,   145,   145,   206,   207,   146,   160,    67,
      69,   384,   165,   145,   360,   161,    46,    58,   148,   392,
     146,   148,   167,   392,   280,   145,   227,   149,   146,   148,
      49,   287,   165,   392,   146,    10,   164,   166,   348,   350,
     349,   165,   165,   389,   122,   351,   163,   352,   133,   353,
     306,   291,   371,   375,    66,   299,   177,   311,   470,   384,
     299,   472,   435,    64,    77,   438,   435,   145,    73,   438,
     373,    73,   150,   151,   152,   153,   435,   450,   279,   438,
      73,   450,    73,    -1,    -1,    -1,    -1,   165,   289,    -1,
      -1,   450,   465,    -1,    -1,    -1,   465,   470,   471,    -1,
      -1,   470,   471,    -1,    -1,    -1,   465,    -1,    -1,   482,
      -1,   470,   471,   482,    -1,    -1,    -1,   373,    -1,    -1,
      -1,    -1,    -1,   482,    -1,    -1,    -1,    -1,   384,    -1,
      -1,    -1,    -1,    -1,   335,   336,   337,   338,   339,   340,
     341,   342,   343,   344,   345,   346,   347,   348,   349,   350,
     351,   352,   353,     3,     4,     5,     6,     7,     8,     9,
      -1,    11,    12,    13,    14,    15,    16,    -1,    -1,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    61,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,
      80,    81,    82,    83,    84,    -1,    86,    -1,    88,    -1,
      -1,    -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,
      -1,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    36,
      37,    38,    39,    40,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   122,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   136,   137,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,    81,    82,    83,    84,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   165,   166,   167,     3,     4,
       5,     6,     7,     8,     9,    -1,    11,    12,    13,    14,
      15,    16,    -1,    -1,    19,    20,    21,    22,    23,    24,
      25,    26,    27,    28,    29,    30,    31,    -1,    -1,    -1,
      -1,    36,    37,    38,    39,    40,    41,    42,    43,    44,
      45,    46,    47,    48,    49,    50,    51,    52,    53,    54,
      55,    56,    57,    -1,    -1,    -1,    61,    62,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    80,    81,    82,    83,    84,
      -1,    86,    -1,    88,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   101,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   136,   137,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     145,    -1,    -1,    -1,    -1,   150,   151,   152,   153,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     165,   166,   167,     3,     4,     5,     6,     7,     8,     9,
      -1,    11,    12,    13,    14,    15,    16,    -1,    -1,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    61,    62,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    81,    82,    83,    84,    -1,    86,    -1,    -1,    -1,
      -1,    -1,    -1,     3,     4,     5,    -1,     7,    -1,    -1,
      -1,   101,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,   122,    -1,    -1,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,   136,   137,    48,    -1,
      -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,    -1,    -1,    -1,    -1,    -1,    -1,
       3,     4,     5,    -1,     7,   165,   166,   167,    -1,    -1,
      80,    81,    82,    83,    84,    -1,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      -1,   101,    -1,    36,    37,    38,    39,    40,    41,    42,
      43,    44,    45,    -1,    -1,    48,    -1,    -1,    -1,    -1,
      -1,    -1,   122,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   136,   137,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,
      83,    84,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   167,   101,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   122,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   136,   137,     3,     4,     5,     6,     7,
       8,     9,    -1,    11,    12,    13,    14,    15,    16,    17,
      18,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,   167,    -1,    -1,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    46,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,    -1,    61,    62,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    82,    83,    84,    -1,    86,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   101,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   122,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,
      -1,    -1,   150,   151,   152,   153,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   165,   166,     3,
       4,     5,     6,     7,     8,     9,    -1,    11,    12,    13,
      14,    15,    16,    -1,    -1,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    46,    47,    48,    49,    50,    51,    52,    53,
      54,    55,    56,    57,    -1,    -1,    -1,    61,    62,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,    83,
      84,    -1,    86,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   101,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   136,   137,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   145,    -1,    -1,    -1,    -1,   150,   151,   152,   153,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   165,   166,     3,     4,     5,     6,     7,     8,     9,
      -1,    11,    12,    13,    14,    15,    16,    -1,    -1,    19,
      20,    21,    22,    23,    24,    25,    26,    27,    28,    29,
      30,    31,    -1,    -1,    -1,    -1,    36,    37,    38,    39,
      40,    41,    42,    43,    44,    45,    46,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    61,    62,    -1,    -1,    -1,    -1,    -1,    -1,     5,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      80,    81,    82,    83,    84,    -1,    86,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,   101,    -1,    -1,    -1,    -1,    -1,    -1,    44,    45,
      -1,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,   122,    -1,    -1,    61,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   136,   137,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,     3,     4,     5,    -1,     7,    -1,
      -1,    -1,    -1,    -1,    -1,   165,   166,    -1,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,   122,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,    -1,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,   145,
      -1,    -1,    61,    62,   150,   151,   152,   153,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     166,    80,    81,    82,    83,    84,    -1,    86,    -1,    -1,
      -1,     3,     4,     5,    -1,     7,    -1,    -1,    -1,    -1,
      -1,    -1,   101,    -1,    -1,    -1,    -1,    19,    20,    21,
      22,    23,    24,    25,    26,    27,    28,    29,    30,    31,
      -1,    -1,    -1,   122,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    -1,    -1,    48,   136,   137,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,    -1,
      -1,   150,   151,   152,   153,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   165,    -1,    80,    81,
      82,    83,    84,    -1,    86,    -1,    88,    -1,    -1,    -1,
      -1,    -1,    94,    95,    96,    97,    98,    99,   100,   101,
      -1,    -1,    -1,    -1,    -1,     3,     4,     5,    -1,     7,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
     122,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,   136,   137,    -1,    -1,    36,    37,
      38,    39,    40,    41,    42,    43,    44,    45,    -1,    -1,
      48,    -1,    -1,    -1,    -1,    -1,    -1,     4,    -1,    -1,
      -1,    -1,    -1,   165,    -1,     5,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    20,    21,    22,    -1,    -1,    -1,    -1,
      -1,    -1,    80,    81,    82,    83,    84,    -1,    86,    36,
      37,    38,    39,    40,    -1,    -1,    94,    95,    96,    97,
      98,    99,   100,   101,    44,    45,    -1,    47,    48,    49,
      50,    51,    52,    53,    54,    55,    56,    57,    -1,    -1,
      -1,    61,    62,    -1,   122,    -1,    -1,    -1,    -1,     5,
      -1,    -1,    -1,    -1,    81,    82,    83,    84,   136,   137,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   165,    44,    45,
      -1,    47,    48,    49,    50,    51,    52,    53,    54,    55,
      56,    57,   122,    -1,    -1,    61,    62,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   146,
      -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,    -1,    -1,
     150,   151,   152,   153,     3,     4,     5,    -1,     7,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   166,   167,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,    -1,    -1,    -1,   122,    36,    37,    38,
      39,    40,    41,    42,    43,    44,    45,     5,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,   145,
      -1,    -1,    61,    62,   150,   151,   152,   153,     5,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    81,    82,    83,    84,    44,    45,    -1,    47,
      48,    49,    50,    51,    52,    53,    54,    55,    56,    57,
      -1,    -1,   101,    61,    62,    -1,    -1,    44,    45,    -1,
      47,    48,    49,    50,    51,    52,    53,    54,    55,    56,
      57,    -1,    -1,   122,    61,    62,     5,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,   145,    -1,    -1,    -1,
      -1,   150,   151,   152,   153,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,   122,    44,    45,    -1,    47,    48,
      49,    50,    51,    52,    53,    54,    55,    56,    57,    -1,
      -1,    -1,    61,    62,    -1,   122,    -1,   145,    -1,    -1,
     148,    -1,   150,   151,   152,   153,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   145,    -1,
      -1,   148,    -1,   150,   151,   152,   153,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   122,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,     3,
       4,     5,    -1,     7,    -1,    -1,   145,    -1,    -1,    -1,
      -1,   150,   151,   152,   153,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,    -1,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      44,    45,    -1,    -1,    48,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,     3,     4,    -1,    -1,     7,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,    83,
      84,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    -1,    -1,    -1,   101,    36,    37,
      38,    39,    40,    41,    42,    43,    -1,    -1,    -1,    47,
      -1,    49,    -1,    -1,    -1,    -1,    -1,    -1,   122,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   136,   137,     3,     4,    -1,    -1,     7,    -1,
      -1,    -1,    80,    81,    82,    83,    84,    -1,    -1,    -1,
      19,    20,    21,    22,    23,    24,    25,    26,    27,    28,
      29,    30,    31,   101,    -1,    -1,    -1,    36,    37,    38,
      39,    40,    41,    42,    43,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,
       3,     4,    -1,    -1,     7,    -1,    -1,    -1,    -1,    -1,
      -1,    80,    81,    82,    83,    84,    19,    20,    21,    22,
      23,    24,    25,    26,    27,    28,    29,    30,    31,    -1,
      -1,    -1,   101,    36,    37,    38,    39,    40,    41,    42,
      43,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   136,   137,     3,
       4,    -1,    -1,     7,    -1,    -1,    -1,    80,    81,    82,
      83,    84,    -1,    -1,    -1,    19,    20,    21,    22,    23,
      24,    25,    26,    27,    28,    29,    30,    31,   101,    -1,
      -1,    -1,    36,    37,    38,    39,    40,    41,    42,    43,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,   136,   137,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    80,    81,    82,    83,
      84,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,   101,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,    -1,
      -1,    -1,   136,   137
};

/* YYSTOS[STATE-NUM] -- The symbol kind of the accessing symbol of
   state STATE-NUM.  */
static const yytype_int16 yystos[] =
{
       0,    87,   169,   171,    53,     0,   173,    47,    48,    49,
      91,   174,    88,   170,   175,    91,   174,     3,     4,     5,
       7,    19,    20,    21,    22,    23,    24,    25,    26,    27,
      28,    29,    30,    31,    36,    37,    38,    39,    40,    41,
      42,    43,    44,    45,    48,    80,    81,    82,    83,    84,
      86,    94,    95,    96,    97,    98,    99,   100,   101,   122,
     136,   137,   165,   172,   176,   206,   207,   208,   209,   210,
     216,   217,   218,   219,   223,   225,   226,   227,   228,   229,
     231,   232,   233,   234,   235,   266,   267,   268,   269,   270,
     274,   275,   276,   277,   278,    90,   166,   174,     7,    20,
      21,    23,    29,    30,    31,    47,    49,    80,    81,   177,
     219,   226,   227,   229,   177,   226,   234,    91,    91,    91,
      91,    91,    91,    91,   145,     5,   145,   175,   266,   165,
     166,   248,   146,   149,     4,    20,    21,    22,    81,   212,
     213,   214,   229,   234,   149,   165,    47,    49,   174,   177,
       7,    20,    21,    23,   226,   268,   274,   275,   276,   277,
     226,   226,   231,   226,   270,   226,   219,   226,   268,   147,
     230,   226,    49,   174,   218,   236,   237,   166,   231,    41,
     110,   143,   174,   220,   221,   222,   174,   224,     6,     8,
       9,    11,    12,    13,    14,    15,    16,    46,    50,    51,
      52,    53,    54,    55,    56,    57,    61,    62,   145,   150,
     151,   152,   153,   165,   166,   167,   177,   178,   179,   181,
     182,   183,   184,   185,   186,   187,   188,   189,   190,   191,
     192,   193,   194,   195,   196,   197,   198,   199,   200,   201,
     202,   204,   206,   207,   231,   242,   243,   244,   245,   249,
     250,   251,   254,   260,   264,   265,   212,   213,   213,   211,
     215,   219,   231,   213,   213,   213,   174,   164,   230,   145,
     165,   165,   165,   165,   148,   188,   201,   205,   231,   147,
     166,    91,   174,   238,   239,   167,   237,   236,   165,   164,
     146,   149,   146,   149,   165,   165,   244,   247,   248,   145,
     145,   165,   165,   165,   204,   145,   145,   188,   188,   204,
     167,   246,    61,    62,   102,   147,   146,   146,   149,    45,
     202,   145,    70,    71,    72,    73,    74,    75,    76,    77,
      78,    79,   164,   203,   188,   154,   155,   156,   150,   151,
      59,    60,    63,    64,   157,   158,    65,    66,   159,   160,
     161,    67,    69,    68,   162,   149,   165,   167,   175,   243,
     231,   174,   164,   230,   166,   202,   240,   164,   148,   148,
     205,   218,   272,   273,   230,   149,   165,   167,   205,   221,
     174,    46,   242,   250,   261,   204,   165,   204,   204,   218,
     253,   146,   249,    58,   180,   204,   202,   202,   188,   188,
     188,   190,   190,   191,   191,   192,   192,   192,   192,   193,
     193,   194,   195,   196,   197,   198,   199,   204,   202,   174,
     230,   240,   164,   240,   241,   240,   148,   238,   167,   272,
     239,   145,   253,   262,   263,   146,   146,   174,   146,   167,
     148,   163,   240,   149,   167,   165,    49,   271,   204,   165,
     146,   243,   252,   166,   255,   164,   247,   202,   167,   240,
     230,   165,   146,   204,   247,    10,    17,    18,   167,   256,
     257,   258,   259,   240,   165,   243,   204,   163,   243,   256,
     243,   167,   258,   163
};

/* YYR1[RULE-NUM] -- Symbol kind of the left-hand side of rule RULE-NUM.  */
static const yytype_int16 yyr1[] =
{
       0,   168,   170,   169,   171,   171,   171,   172,   172,   172,
     172,   172,   172,   172,   173,   173,   174,   174,   174,   175,
     176,   176,   176,   177,   177,   178,   178,   178,   178,   178,
     178,   178,   178,   178,   178,   179,   179,   179,   179,   179,
     179,   180,   181,   182,   183,   183,   184,   184,   185,   185,
     186,   187,   187,   188,   188,   188,   188,   189,   189,   189,
     189,   190,   190,   190,   190,   191,   191,   191,   192,   192,
     192,   193,   193,   193,   193,   193,   194,   194,   194,   195,
     195,   196,   196,   197,   197,   198,   198,   199,   199,   200,
     200,   201,   201,   202,   202,   203,   203,   203,   203,   203,
     203,   203,   203,   203,   203,   203,   204,   204,   205,   206,
     206,   206,   206,   207,   208,   208,   209,   209,   210,   211,
     211,   211,   212,   212,   213,   213,   213,   213,   213,   213,
     214,   214,   214,   215,   216,   216,   216,   216,   216,   217,
     217,   217,   217,   217,   217,   217,   218,   218,   219,   220,
     220,   221,   221,   221,   222,   222,   222,   223,   223,   224,
     224,   225,   225,   225,   226,   226,   226,   226,   226,   226,
     226,   226,   226,   226,   226,   226,   226,   226,   226,   226,
     226,   226,   227,   227,   227,   227,   228,   228,   228,   228,
     228,   228,   228,   228,   228,   228,   228,   228,   228,   229,
     229,   229,   229,   229,   230,   230,   230,   230,   231,   231,
     232,   232,   232,   233,   233,   233,   234,   234,   234,   235,
     235,   236,   236,   237,   238,   238,   239,   239,   240,   240,
     240,   241,   241,   242,   243,   243,   244,   244,   244,   244,
     244,   244,   244,   245,   246,   245,   247,   247,   248,   248,
     249,   249,   249,   250,   250,   251,   252,   252,   253,   253,
     254,   255,   255,   256,   256,   257,   257,   258,   258,   259,
     259,   260,   260,   260,   261,   261,   262,   262,   263,   263,
     264,   264,   264,   264,   264,   265,   266,   266,   266,   266,
     266,   267,   268,   268,   268,   269,   270,   270,   270,   270,
     270,   270,   270,   270,   271,   271,   271,   272,   272,   273,
     274,   274,   275,   275,   276,   276,   277,   277,   278,   278,
     278,   278
};

/* YYR2[RULE-NUM] -- Number of symbols on the right-hand side of rule RULE-NUM.  */
static const yytype_int8 yyr2[] =
{
       0,     2,     0,     4,     0,     3,     4,     2,     2,     2,
       2,     2,     2,     2,     0,     2,     1,     1,     1,     5,
       1,     2,     2,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     3,     1,     4,     1,     3,     2,
       2,     1,     1,     1,     2,     2,     2,     1,     2,     3,
       2,     1,     1,     1,     2,     2,     2,     1,     1,     1,
       1,     1,     3,     3,     3,     1,     3,     3,     1,     3,
       3,     1,     3,     3,     3,     3,     1,     3,     3,     1,
       3,     1,     3,     1,     3,     1,     3,     1,     3,     1,
       3,     1,     5,     1,     3,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     3,     1,     2,
       2,     4,     1,     2,     1,     1,     2,     3,     3,     2,
       3,     3,     2,     2,     0,     2,     2,     2,     2,     2,
       1,     1,     1,     1,     1,     3,     4,     6,     5,     1,
       2,     3,     5,     4,     2,     2,     1,     2,     4,     1,
       3,     1,     3,     1,     1,     1,     1,     1,     4,     1,
       3,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     2,     2,     2,     2,     2,     2,
       2,     2,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     1,     2,     3,     3,     4,     1,     2,
       1,     1,     1,     1,     1,     2,     1,     1,     1,     5,
       4,     1,     2,     3,     1,     3,     1,     2,     1,     3,
       4,     1,     3,     1,     1,     1,     1,     1,     1,     1,
       1,     1,     1,     2,     0,     4,     1,     1,     2,     3,
       1,     2,     2,     1,     2,     5,     3,     1,     1,     4,
       5,     2,     3,     3,     2,     1,     2,     2,     2,     1,
       2,     5,     7,     6,     1,     1,     1,     0,     2,     3,
       2,     2,     2,     3,     2,     2,     1,     1,     1,     1,
       1,     2,     1,     2,     2,     7,     1,     1,     1,     1,
       1,     1,     1,     2,     0,     1,     2,     1,     2,     3,
       2,     3,     2,     3,     2,     3,     2,     3,     1,     1,
       1,     1
};


enum { YYENOMEM = -2 };

#define yyerrok         (yyerrstatus = 0)
#define yyclearin       (yychar = YYEMPTY)

#define YYACCEPT        goto yyacceptlab
#define YYABORT         goto yyabortlab
#define YYERROR         goto yyerrorlab
#define YYNOMEM         goto yyexhaustedlab


#define YYRECOVERING()  (!!yyerrstatus)

#define YYBACKUP(Token, Value)                                    \
  do                                                              \
    if (yychar == YYEMPTY)                                        \
      {                                                           \
        yychar = (Token);                                         \
        yylval = (Value);                                         \
        YYPOPSTACK (yylen);                                       \
        yystate = *yyssp;                                         \
        goto yybackup;                                            \
      }                                                           \
    else                                                          \
      {                                                           \
        yyerror (&yylloc, state, YY_("syntax error: cannot back up")); \
        YYERROR;                                                  \
      }                                                           \
  while (0)

/* Backward compatibility with an undocumented macro.
   Use YYerror or YYUNDEF. */
#define YYERRCODE YYUNDEF

/* YYLLOC_DEFAULT -- Set CURRENT to span from RHS[1] to RHS[N].
   If N is 0, then set CURRENT to the empty location which ends
   the previous symbol: RHS[0] (always defined).  */

#ifndef YYLLOC_DEFAULT
# define YYLLOC_DEFAULT(Current, Rhs, N)                                \
    do                                                                  \
      if (N)                                                            \
        {                                                               \
          (Current).first_line   = YYRHSLOC (Rhs, 1).first_line;        \
          (Current).first_column = YYRHSLOC (Rhs, 1).first_column;      \
          (Current).last_line    = YYRHSLOC (Rhs, N).last_line;         \
          (Current).last_column  = YYRHSLOC (Rhs, N).last_column;       \
        }                                                               \
      else                                                              \
        {                                                               \
          (Current).first_line   = (Current).last_line   =              \
            YYRHSLOC (Rhs, 0).last_line;                                \
          (Current).first_column = (Current).last_column =              \
            YYRHSLOC (Rhs, 0).last_column;                              \
        }                                                               \
    while (0)
#endif

#define YYRHSLOC(Rhs, K) ((Rhs)[K])


/* Enable debugging if requested.  */
#if YYDEBUG

# ifndef YYFPRINTF
#  include <stdio.h> /* INFRINGES ON USER NAME SPACE */
#  define YYFPRINTF fprintf
# endif

# define YYDPRINTF(Args)                        \
do {                                            \
  if (yydebug)                                  \
    YYFPRINTF Args;                             \
} while (0)


/* YYLOCATION_PRINT -- Print the location on the stream.
   This macro was not mandated originally: define only if we know
   we won't break user code: when these are the locations we know.  */

# ifndef YYLOCATION_PRINT

#  if defined YY_LOCATION_PRINT

   /* Temporary convenience wrapper in case some people defined the
      undocumented and private YY_LOCATION_PRINT macros.  */
#   define YYLOCATION_PRINT(File, Loc)  YY_LOCATION_PRINT(File, *(Loc))

#  elif defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL

/* Print *YYLOCP on YYO.  Private, do not rely on its existence. */

YY_ATTRIBUTE_UNUSED
static int
yy_location_print_ (FILE *yyo, YYLTYPE const * const yylocp)
{
  int res = 0;
  int end_col = 0 != yylocp->last_column ? yylocp->last_column - 1 : 0;
  if (0 <= yylocp->first_line)
    {
      res += YYFPRINTF (yyo, "%d", yylocp->first_line);
      if (0 <= yylocp->first_column)
        res += YYFPRINTF (yyo, ".%d", yylocp->first_column);
    }
  if (0 <= yylocp->last_line)
    {
      if (yylocp->first_line < yylocp->last_line)
        {
          res += YYFPRINTF (yyo, "-%d", yylocp->last_line);
          if (0 <= end_col)
            res += YYFPRINTF (yyo, ".%d", end_col);
        }
      else if (0 <= end_col && yylocp->first_column < end_col)
        res += YYFPRINTF (yyo, "-%d", end_col);
    }
  return res;
}

#   define YYLOCATION_PRINT  yy_location_print_

    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT(File, Loc)  YYLOCATION_PRINT(File, &(Loc))

#  else

#   define YYLOCATION_PRINT(File, Loc) ((void) 0)
    /* Temporary convenience wrapper in case some people defined the
       undocumented and private YY_LOCATION_PRINT macros.  */
#   define YY_LOCATION_PRINT  YYLOCATION_PRINT

#  endif
# endif /* !defined YYLOCATION_PRINT */


# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)                    \
do {                                                                      \
  if (yydebug)                                                            \
    {                                                                     \
      YYFPRINTF (stderr, "%s ", Title);                                   \
      yy_symbol_print (stderr,                                            \
                  Kind, Value, Location, state); \
      YYFPRINTF (stderr, "\n");                                           \
    }                                                                     \
} while (0)


/*-----------------------------------.
| Print this symbol's value on YYO.  |
`-----------------------------------*/

static void
yy_symbol_value_print (FILE *yyo,
                       yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct _mesa_glsl_parse_state *state)
{
  FILE *yyoutput = yyo;
  YY_USE (yyoutput);
  YY_USE (yylocationp);
  YY_USE (state);
  if (!yyvaluep)
    return;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}


/*---------------------------.
| Print this symbol on YYO.  |
`---------------------------*/

static void
yy_symbol_print (FILE *yyo,
                 yysymbol_kind_t yykind, YYSTYPE const * const yyvaluep, YYLTYPE const * const yylocationp, struct _mesa_glsl_parse_state *state)
{
  YYFPRINTF (yyo, "%s %s (",
             yykind < YYNTOKENS ? "token" : "nterm", yysymbol_name (yykind));

  YYLOCATION_PRINT (yyo, yylocationp);
  YYFPRINTF (yyo, ": ");
  yy_symbol_value_print (yyo, yykind, yyvaluep, yylocationp, state);
  YYFPRINTF (yyo, ")");
}

/*------------------------------------------------------------------.
| yy_stack_print -- Print the state stack from its BOTTOM up to its |
| TOP (included).                                                   |
`------------------------------------------------------------------*/

static void
yy_stack_print (yy_state_t *yybottom, yy_state_t *yytop)
{
  YYFPRINTF (stderr, "Stack now");
  for (; yybottom <= yytop; yybottom++)
    {
      int yybot = *yybottom;
      YYFPRINTF (stderr, " %d", yybot);
    }
  YYFPRINTF (stderr, "\n");
}

# define YY_STACK_PRINT(Bottom, Top)                            \
do {                                                            \
  if (yydebug)                                                  \
    yy_stack_print ((Bottom), (Top));                           \
} while (0)


/*------------------------------------------------.
| Report that the YYRULE is going to be reduced.  |
`------------------------------------------------*/

static void
yy_reduce_print (yy_state_t *yyssp, YYSTYPE *yyvsp, YYLTYPE *yylsp,
                 int yyrule, struct _mesa_glsl_parse_state *state)
{
  int yylno = yyrline[yyrule];
  int yynrhs = yyr2[yyrule];
  int yyi;
  YYFPRINTF (stderr, "Reducing stack by rule %d (line %d):\n",
             yyrule - 1, yylno);
  /* The symbols being reduced.  */
  for (yyi = 0; yyi < yynrhs; yyi++)
    {
      YYFPRINTF (stderr, "   $%d = ", yyi + 1);
      yy_symbol_print (stderr,
                       YY_ACCESSING_SYMBOL (+yyssp[yyi + 1 - yynrhs]),
                       &yyvsp[(yyi + 1) - (yynrhs)],
                       &(yylsp[(yyi + 1) - (yynrhs)]), state);
      YYFPRINTF (stderr, "\n");
    }
}

# define YY_REDUCE_PRINT(Rule)          \
do {                                    \
  if (yydebug)                          \
    yy_reduce_print (yyssp, yyvsp, yylsp, Rule, state); \
} while (0)

/* Nonzero means print parse trace.  It is left uninitialized so that
   multiple parsers can coexist.  */
int yydebug;
#else /* !YYDEBUG */
# define YYDPRINTF(Args) ((void) 0)
# define YY_SYMBOL_PRINT(Title, Kind, Value, Location)
# define YY_STACK_PRINT(Bottom, Top)
# define YY_REDUCE_PRINT(Rule)
#endif /* !YYDEBUG */


/* YYINITDEPTH -- initial size of the parser's stacks.  */
#ifndef YYINITDEPTH
# define YYINITDEPTH 200
#endif

/* YYMAXDEPTH -- maximum size the stacks can grow to (effective only
   if the built-in stack extension method is used).

   Do not make this value too large; the results are undefined if
   YYSTACK_ALLOC_MAXIMUM < YYSTACK_BYTES (YYMAXDEPTH)
   evaluated with infinite-precision integer arithmetic.  */

#ifndef YYMAXDEPTH
# define YYMAXDEPTH 10000
#endif


/* Context of a parse error.  */
typedef struct
{
  yy_state_t *yyssp;
  yysymbol_kind_t yytoken;
  YYLTYPE *yylloc;
} yypcontext_t;

/* Put in YYARG at most YYARGN of the expected tokens given the
   current YYCTX, and return the number of tokens stored in YYARG.  If
   YYARG is null, return the number of expected tokens (guaranteed to
   be less than YYNTOKENS).  Return YYENOMEM on memory exhaustion.
   Return 0 if there are more than YYARGN expected tokens, yet fill
   YYARG up to YYARGN. */
static int
yypcontext_expected_tokens (const yypcontext_t *yyctx,
                            yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  int yyn = yypact[+*yyctx->yyssp];
  if (!yypact_value_is_default (yyn))
    {
      /* Start YYX at -YYN if negative to avoid negative indexes in
         YYCHECK.  In other words, skip the first -YYN actions for
         this state because they are default actions.  */
      int yyxbegin = yyn < 0 ? -yyn : 0;
      /* Stay within bounds of both yycheck and yytname.  */
      int yychecklim = YYLAST - yyn + 1;
      int yyxend = yychecklim < YYNTOKENS ? yychecklim : YYNTOKENS;
      int yyx;
      for (yyx = yyxbegin; yyx < yyxend; ++yyx)
        if (yycheck[yyx + yyn] == yyx && yyx != YYSYMBOL_YYerror
            && !yytable_value_is_error (yytable[yyx + yyn]))
          {
            if (!yyarg)
              ++yycount;
            else if (yycount == yyargn)
              return 0;
            else
              yyarg[yycount++] = YY_CAST (yysymbol_kind_t, yyx);
          }
    }
  if (yyarg && yycount == 0 && 0 < yyargn)
    yyarg[0] = YYSYMBOL_YYEMPTY;
  return yycount;
}




#ifndef yystrlen
# if defined __GLIBC__ && defined _STRING_H
#  define yystrlen(S) (YY_CAST (YYPTRDIFF_T, strlen (S)))
# else
/* Return the length of YYSTR.  */
static YYPTRDIFF_T
yystrlen (const char *yystr)
{
  YYPTRDIFF_T yylen;
  for (yylen = 0; yystr[yylen]; yylen++)
    continue;
  return yylen;
}
# endif
#endif

#ifndef yystpcpy
# if defined __GLIBC__ && defined _STRING_H && defined _GNU_SOURCE
#  define yystpcpy stpcpy
# else
/* Copy YYSRC to YYDEST, returning the address of the terminating '\0' in
   YYDEST.  */
static char *
yystpcpy (char *yydest, const char *yysrc)
{
  char *yyd = yydest;
  const char *yys = yysrc;

  while ((*yyd++ = *yys++) != '\0')
    continue;

  return yyd - 1;
}
# endif
#endif

#ifndef yytnamerr
/* Copy to YYRES the contents of YYSTR after stripping away unnecessary
   quotes and backslashes, so that it's suitable for yyerror.  The
   heuristic is that double-quoting is unnecessary unless the string
   contains an apostrophe, a comma, or backslash (other than
   backslash-backslash).  YYSTR is taken from yytname.  If YYRES is
   null, do not copy; instead, return the length of what the result
   would have been.  */
static YYPTRDIFF_T
yytnamerr (char *yyres, const char *yystr)
{
  if (*yystr == '"')
    {
      YYPTRDIFF_T yyn = 0;
      char const *yyp = yystr;
      for (;;)
        switch (*++yyp)
          {
          case '\'':
          case ',':
            goto do_not_strip_quotes;

          case '\\':
            if (*++yyp != '\\')
              goto do_not_strip_quotes;
            else
              goto append;

          append:
          default:
            if (yyres)
              yyres[yyn] = *yyp;
            yyn++;
            break;

          case '"':
            if (yyres)
              yyres[yyn] = '\0';
            return yyn;
          }
    do_not_strip_quotes: ;
    }

  if (yyres)
    return yystpcpy (yyres, yystr) - yyres;
  else
    return yystrlen (yystr);
}
#endif


static int
yy_syntax_error_arguments (const yypcontext_t *yyctx,
                           yysymbol_kind_t yyarg[], int yyargn)
{
  /* Actual size of YYARG. */
  int yycount = 0;
  /* There are many possibilities here to consider:
     - If this state is a consistent state with a default action, then
       the only way this function was invoked is if the default action
       is an error action.  In that case, don't check for expected
       tokens because there are none.
     - The only way there can be no lookahead present (in yychar) is if
       this state is a consistent state with a default action.  Thus,
       detecting the absence of a lookahead is sufficient to determine
       that there is no unexpected or expected token to report.  In that
       case, just report a simple "syntax error".
     - Don't assume there isn't a lookahead just because this state is a
       consistent state with a default action.  There might have been a
       previous inconsistent state, consistent state with a non-default
       action, or user semantic action that manipulated yychar.
     - Of course, the expected token list depends on states to have
       correct lookahead information, and it depends on the parser not
       to perform extra reductions after fetching a lookahead from the
       scanner and before detecting a syntax error.  Thus, state merging
       (from LALR or IELR) and default reductions corrupt the expected
       token list.  However, the list is correct for canonical LR with
       one exception: it will still contain any token that will not be
       accepted due to an error action in a later state.
  */
  if (yyctx->yytoken != YYSYMBOL_YYEMPTY)
    {
      int yyn;
      if (yyarg)
        yyarg[yycount] = yyctx->yytoken;
      ++yycount;
      yyn = yypcontext_expected_tokens (yyctx,
                                        yyarg ? yyarg + 1 : yyarg, yyargn - 1);
      if (yyn == YYENOMEM)
        return YYENOMEM;
      else
        yycount += yyn;
    }
  return yycount;
}

/* Copy into *YYMSG, which is of size *YYMSG_ALLOC, an error message
   about the unexpected token YYTOKEN for the state stack whose top is
   YYSSP.

   Return 0 if *YYMSG was successfully written.  Return -1 if *YYMSG is
   not large enough to hold the message.  In that case, also set
   *YYMSG_ALLOC to the required number of bytes.  Return YYENOMEM if the
   required number of bytes is too large to store.  */
static int
yysyntax_error (YYPTRDIFF_T *yymsg_alloc, char **yymsg,
                const yypcontext_t *yyctx)
{
  enum { YYARGS_MAX = 5 };
  /* Internationalized format string. */
  const char *yyformat = YY_NULLPTR;
  /* Arguments of yyformat: reported tokens (one for the "unexpected",
     one per "expected"). */
  yysymbol_kind_t yyarg[YYARGS_MAX];
  /* Cumulated lengths of YYARG.  */
  YYPTRDIFF_T yysize = 0;

  /* Actual size of YYARG. */
  int yycount = yy_syntax_error_arguments (yyctx, yyarg, YYARGS_MAX);
  if (yycount == YYENOMEM)
    return YYENOMEM;

  switch (yycount)
    {
#define YYCASE_(N, S)                       \
      case N:                               \
        yyformat = S;                       \
        break
    default: /* Avoid compiler warnings. */
      YYCASE_(0, YY_("syntax error"));
      YYCASE_(1, YY_("syntax error, unexpected %s"));
      YYCASE_(2, YY_("syntax error, unexpected %s, expecting %s"));
      YYCASE_(3, YY_("syntax error, unexpected %s, expecting %s or %s"));
      YYCASE_(4, YY_("syntax error, unexpected %s, expecting %s or %s or %s"));
      YYCASE_(5, YY_("syntax error, unexpected %s, expecting %s or %s or %s or %s"));
#undef YYCASE_
    }

  /* Compute error message size.  Don't count the "%s"s, but reserve
     room for the terminator.  */
  yysize = yystrlen (yyformat) - 2 * yycount + 1;
  {
    int yyi;
    for (yyi = 0; yyi < yycount; ++yyi)
      {
        YYPTRDIFF_T yysize1
          = yysize + yytnamerr (YY_NULLPTR, yytname[yyarg[yyi]]);
        if (yysize <= yysize1 && yysize1 <= YYSTACK_ALLOC_MAXIMUM)
          yysize = yysize1;
        else
          return YYENOMEM;
      }
  }

  if (*yymsg_alloc < yysize)
    {
      *yymsg_alloc = 2 * yysize;
      if (! (yysize <= *yymsg_alloc
             && *yymsg_alloc <= YYSTACK_ALLOC_MAXIMUM))
        *yymsg_alloc = YYSTACK_ALLOC_MAXIMUM;
      return -1;
    }

  /* Avoid sprintf, as that infringes on the user's name space.
     Don't have undefined behavior even if the translation
     produced a string with the wrong number of "%s"s.  */
  {
    char *yyp = *yymsg;
    int yyi = 0;
    while ((*yyp = *yyformat) != '\0')
      if (*yyp == '%' && yyformat[1] == 's' && yyi < yycount)
        {
          yyp += yytnamerr (yyp, yytname[yyarg[yyi++]]);
          yyformat += 2;
        }
      else
        {
          ++yyp;
          ++yyformat;
        }
  }
  return 0;
}


/*-----------------------------------------------.
| Release the memory associated to this symbol.  |
`-----------------------------------------------*/

static void
yydestruct (const char *yymsg,
            yysymbol_kind_t yykind, YYSTYPE *yyvaluep, YYLTYPE *yylocationp, struct _mesa_glsl_parse_state *state)
{
  YY_USE (yyvaluep);
  YY_USE (yylocationp);
  YY_USE (state);
  if (!yymsg)
    yymsg = "Deleting";
  YY_SYMBOL_PRINT (yymsg, yykind, yyvaluep, yylocationp);

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  YY_USE (yykind);
  YY_IGNORE_MAYBE_UNINITIALIZED_END
}






/*----------.
| yyparse.  |
`----------*/

int
yyparse (struct _mesa_glsl_parse_state *state)
{
/* Lookahead token kind.  */
int yychar;


/* The semantic value of the lookahead symbol.  */
/* Default value used for initialization, for pacifying older GCCs
   or non-GCC compilers.  */
YY_INITIAL_VALUE (static YYSTYPE yyval_default;)
YYSTYPE yylval YY_INITIAL_VALUE (= yyval_default);

/* Location data for the lookahead symbol.  */
static YYLTYPE yyloc_default
# if defined YYLTYPE_IS_TRIVIAL && YYLTYPE_IS_TRIVIAL
  = { 1, 1, 1, 1 }
# endif
;
YYLTYPE yylloc = yyloc_default;

    /* Number of syntax errors so far.  */
    int yynerrs = 0;

    yy_state_fast_t yystate = 0;
    /* Number of tokens to shift before error messages enabled.  */
    int yyerrstatus = 0;

    /* Refer to the stacks through separate pointers, to allow yyoverflow
       to reallocate them elsewhere.  */

    /* Their size.  */
    YYPTRDIFF_T yystacksize = YYINITDEPTH;

    /* The state stack: array, bottom, top.  */
    yy_state_t yyssa[YYINITDEPTH];
    yy_state_t *yyss = yyssa;
    yy_state_t *yyssp = yyss;

    /* The semantic value stack: array, bottom, top.  */
    YYSTYPE yyvsa[YYINITDEPTH];
    YYSTYPE *yyvs = yyvsa;
    YYSTYPE *yyvsp = yyvs;

    /* The location stack: array, bottom, top.  */
    YYLTYPE yylsa[YYINITDEPTH];
    YYLTYPE *yyls = yylsa;
    YYLTYPE *yylsp = yyls;

  int yyn;
  /* The return value of yyparse.  */
  int yyresult;
  /* Lookahead symbol kind.  */
  yysymbol_kind_t yytoken = YYSYMBOL_YYEMPTY;
  /* The variables used to return semantic value and location from the
     action routines.  */
  YYSTYPE yyval;
  YYLTYPE yyloc;

  /* The locations where the error started and ended.  */
  YYLTYPE yyerror_range[3];

  /* Buffer for error messages, and its allocated size.  */
  char yymsgbuf[128];
  char *yymsg = yymsgbuf;
  YYPTRDIFF_T yymsg_alloc = sizeof yymsgbuf;

#define YYPOPSTACK(N)   (yyvsp -= (N), yyssp -= (N), yylsp -= (N))

  /* The number of symbols on the RHS of the reduced rule.
     Keep to zero when no symbol should be popped.  */
  int yylen = 0;

  YYDPRINTF ((stderr, "Starting parse\n"));

  yychar = YYEMPTY; /* Cause a token to be read.  */


/* User initialization code.  */
#line 90 "src/compiler/glsl/glsl_parser.yy"
{
   yylloc.first_line = 1;
   yylloc.first_column = 1;
   yylloc.last_line = 1;
   yylloc.last_column = 1;
   yylloc.source = 0;
   yylloc.path = NULL;
}

#line 2486 "src/compiler/glsl/glsl_parser.cpp"

  yylsp[0] = yylloc;
  goto yysetstate;


/*------------------------------------------------------------.
| yynewstate -- push a new state, which is found in yystate.  |
`------------------------------------------------------------*/
yynewstate:
  /* In all cases, when you get here, the value and location stacks
     have just been pushed.  So pushing a state here evens the stacks.  */
  yyssp++;


/*--------------------------------------------------------------------.
| yysetstate -- set current state (the top of the stack) to yystate.  |
`--------------------------------------------------------------------*/
yysetstate:
  YYDPRINTF ((stderr, "Entering state %d\n", yystate));
  YY_ASSERT (0 <= yystate && yystate < YYNSTATES);
  YY_IGNORE_USELESS_CAST_BEGIN
  *yyssp = YY_CAST (yy_state_t, yystate);
  YY_IGNORE_USELESS_CAST_END
  YY_STACK_PRINT (yyss, yyssp);

  if (yyss + yystacksize - 1 <= yyssp)
#if !defined yyoverflow && !defined YYSTACK_RELOCATE
    YYNOMEM;
#else
    {
      /* Get the current used size of the three stacks, in elements.  */
      YYPTRDIFF_T yysize = yyssp - yyss + 1;

# if defined yyoverflow
      {
        /* Give user a chance to reallocate the stack.  Use copies of
           these so that the &'s don't force the real ones into
           memory.  */
        yy_state_t *yyss1 = yyss;
        YYSTYPE *yyvs1 = yyvs;
        YYLTYPE *yyls1 = yyls;

        /* Each stack pointer address is followed by the size of the
           data in use in that stack, in bytes.  This used to be a
           conditional around just the two extra args, but that might
           be undefined if yyoverflow is a macro.  */
        yyoverflow (YY_("memory exhausted"),
                    &yyss1, yysize * YYSIZEOF (*yyssp),
                    &yyvs1, yysize * YYSIZEOF (*yyvsp),
                    &yyls1, yysize * YYSIZEOF (*yylsp),
                    &yystacksize);
        yyss = yyss1;
        yyvs = yyvs1;
        yyls = yyls1;
      }
# else /* defined YYSTACK_RELOCATE */
      /* Extend the stack our own way.  */
      if (YYMAXDEPTH <= yystacksize)
        YYNOMEM;
      yystacksize *= 2;
      if (YYMAXDEPTH < yystacksize)
        yystacksize = YYMAXDEPTH;

      {
        yy_state_t *yyss1 = yyss;
        union yyalloc *yyptr =
          YY_CAST (union yyalloc *,
                   YYSTACK_ALLOC (YY_CAST (YYSIZE_T, YYSTACK_BYTES (yystacksize))));
        if (! yyptr)
          YYNOMEM;
        YYSTACK_RELOCATE (yyss_alloc, yyss);
        YYSTACK_RELOCATE (yyvs_alloc, yyvs);
        YYSTACK_RELOCATE (yyls_alloc, yyls);
#  undef YYSTACK_RELOCATE
        if (yyss1 != yyssa)
          YYSTACK_FREE (yyss1);
      }
# endif

      yyssp = yyss + yysize - 1;
      yyvsp = yyvs + yysize - 1;
      yylsp = yyls + yysize - 1;

      YY_IGNORE_USELESS_CAST_BEGIN
      YYDPRINTF ((stderr, "Stack size increased to %ld\n",
                  YY_CAST (long, yystacksize)));
      YY_IGNORE_USELESS_CAST_END

      if (yyss + yystacksize - 1 <= yyssp)
        YYABORT;
    }
#endif /* !defined yyoverflow && !defined YYSTACK_RELOCATE */


  if (yystate == YYFINAL)
    YYACCEPT;

  goto yybackup;


/*-----------.
| yybackup.  |
`-----------*/
yybackup:
  /* Do appropriate processing given the current state.  Read a
     lookahead token if we need one and don't already have one.  */

  /* First try to decide what to do without reference to lookahead token.  */
  yyn = yypact[yystate];
  if (yypact_value_is_default (yyn))
    goto yydefault;

  /* Not known => get a lookahead token if don't already have one.  */

  /* YYCHAR is either empty, or end-of-input, or a valid lookahead.  */
  if (yychar == YYEMPTY)
    {
      YYDPRINTF ((stderr, "Reading a token\n"));
      yychar = yylex (&yylval, &yylloc, state);
    }

  if (yychar <= YYEOF)
    {
      yychar = YYEOF;
      yytoken = YYSYMBOL_YYEOF;
      YYDPRINTF ((stderr, "Now at end of input.\n"));
    }
  else if (yychar == YYerror)
    {
      /* The scanner already issued an error message, process directly
         to error recovery.  But do not keep the error token as
         lookahead, it is too special and may lead us to an endless
         loop in error recovery. */
      yychar = YYUNDEF;
      yytoken = YYSYMBOL_YYerror;
      yyerror_range[1] = yylloc;
      goto yyerrlab1;
    }
  else
    {
      yytoken = YYTRANSLATE (yychar);
      YY_SYMBOL_PRINT ("Next token is", yytoken, &yylval, &yylloc);
    }

  /* If the proper action on seeing token YYTOKEN is to reduce or to
     detect an error, take that action.  */
  yyn += yytoken;
  if (yyn < 0 || YYLAST < yyn || yycheck[yyn] != yytoken)
    goto yydefault;
  yyn = yytable[yyn];
  if (yyn <= 0)
    {
      if (yytable_value_is_error (yyn))
        goto yyerrlab;
      yyn = -yyn;
      goto yyreduce;
    }

  /* Count tokens shifted since error; after three, turn off error
     status.  */
  if (yyerrstatus)
    yyerrstatus--;

  /* Shift the lookahead token.  */
  YY_SYMBOL_PRINT ("Shifting", yytoken, &yylval, &yylloc);
  yystate = yyn;
  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END
  *++yylsp = yylloc;

  /* Discard the shifted token.  */
  yychar = YYEMPTY;
  goto yynewstate;


/*-----------------------------------------------------------.
| yydefault -- do the default action for the current state.  |
`-----------------------------------------------------------*/
yydefault:
  yyn = yydefact[yystate];
  if (yyn == 0)
    goto yyerrlab;
  goto yyreduce;


/*-----------------------------.
| yyreduce -- do a reduction.  |
`-----------------------------*/
yyreduce:
  /* yyn is the number of a rule to reduce with.  */
  yylen = yyr2[yyn];

  /* If YYLEN is nonzero, implement the default value of the action:
     '$$ = $1'.

     Otherwise, the following line sets YYVAL to garbage.
     This behavior is undocumented and Bison
     users should not rely upon it.  Assigning to YYVAL
     unconditionally makes the parser a bit smaller, and it avoids a
     GCC warning that YYVAL may be used uninitialized.  */
  yyval = yyvsp[1-yylen];

  /* Default location. */
  YYLLOC_DEFAULT (yyloc, (yylsp - yylen), yylen);
  yyerror_range[1] = yyloc;
  YY_REDUCE_PRINT (yyn);
  switch (yyn)
    {
  case 2: /* $@1: %empty  */
#line 298 "src/compiler/glsl/glsl_parser.yy"
   {
      _mesa_glsl_initialize_types(state);
   }
#line 2701 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 3: /* translation_unit: version_statement extension_statement_list $@1 external_declaration_list  */
#line 302 "src/compiler/glsl/glsl_parser.yy"
   {
      delete state->symbols;
      state->symbols = new(ralloc_parent(state)) glsl_symbol_table;
      if (state->es_shader) {
         if (state->stage == MESA_SHADER_FRAGMENT) {
            state->symbols->add_default_precision_qualifier("int", ast_precision_medium);
         } else {
            state->symbols->add_default_precision_qualifier("float", ast_precision_high);
            state->symbols->add_default_precision_qualifier("int", ast_precision_high);
         }
         state->symbols->add_default_precision_qualifier("sampler2D", ast_precision_low);
         state->symbols->add_default_precision_qualifier("samplerExternalOES", ast_precision_low);
         state->symbols->add_default_precision_qualifier("samplerCube", ast_precision_low);
         state->symbols->add_default_precision_qualifier("atomic_uint", ast_precision_high);
      }
      _mesa_glsl_initialize_types(state);
   }
#line 2723 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 5: /* version_statement: VERSION_TOK INTCONSTANT EOL  */
#line 324 "src/compiler/glsl/glsl_parser.yy"
   {
      state->process_version_directive(&(yylsp[-1]), (yyvsp[-1].n), NULL);
      if (state->error) {
         YYERROR;
      }
   }
#line 2734 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 6: /* version_statement: VERSION_TOK INTCONSTANT any_identifier EOL  */
#line 331 "src/compiler/glsl/glsl_parser.yy"
   {
      state->process_version_directive(&(yylsp[-2]), (yyvsp[-2].n), (yyvsp[-1].identifier));
      if (state->error) {
         YYERROR;
      }
   }
#line 2745 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 7: /* pragma_statement: PRAGMA_DEBUG_ON EOL  */
#line 340 "src/compiler/glsl/glsl_parser.yy"
                       { (yyval.node) = NULL; }
#line 2751 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 8: /* pragma_statement: PRAGMA_DEBUG_OFF EOL  */
#line 341 "src/compiler/glsl/glsl_parser.yy"
                          { (yyval.node) = NULL; }
#line 2757 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 9: /* pragma_statement: PRAGMA_OPTIMIZE_ON EOL  */
#line 342 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.node) = NULL; }
#line 2763 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 10: /* pragma_statement: PRAGMA_OPTIMIZE_OFF EOL  */
#line 343 "src/compiler/glsl/glsl_parser.yy"
                             { (yyval.node) = NULL; }
#line 2769 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 11: /* pragma_statement: PRAGMA_INVARIANT_ALL EOL  */
#line 345 "src/compiler/glsl/glsl_parser.yy"
   {
      /* Pragma invariant(all) cannot be used in a fragment shader.
       *
       * Page 27 of the GLSL 1.20 spec, Page 53 of the GLSL ES 3.00 spec:
       *
       *     "It is an error to use this pragma in a fragment shader."
       */
      if (state->is_version(120, 300) &&
          state->stage == MESA_SHADER_FRAGMENT) {
         _mesa_glsl_error(& (yylsp[-1]), state,
                          "pragma `invariant(all)' cannot be used "
                          "in a fragment shader.");
      } else if (!state->is_version(120, 100)) {
         _mesa_glsl_warning(& (yylsp[-1]), state,
                            "pragma `invariant(all)' not supported in %s "
                            "(GLSL ES 1.00 or GLSL 1.20 required)",
                            state->get_version_string());
      } else {
         state->all_invariant = true;
      }

      (yyval.node) = NULL;
   }
#line 2797 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 12: /* pragma_statement: PRAGMA_WARNING_ON EOL  */
#line 369 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *mem_ctx = state->linalloc;
      (yyval.node) = new(mem_ctx) ast_warnings_toggle(true);
   }
#line 2806 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 13: /* pragma_statement: PRAGMA_WARNING_OFF EOL  */
#line 374 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *mem_ctx = state->linalloc;
      (yyval.node) = new(mem_ctx) ast_warnings_toggle(false);
   }
#line 2815 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 19: /* extension_statement: EXTENSION any_identifier COLON any_identifier EOL  */
#line 393 "src/compiler/glsl/glsl_parser.yy"
   {
      if (!_mesa_glsl_process_extension((yyvsp[-3].identifier), & (yylsp[-3]), (yyvsp[-1].identifier), & (yylsp[-1]), state)) {
         YYERROR;
      }
   }
#line 2825 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 20: /* external_declaration_list: external_declaration  */
#line 402 "src/compiler/glsl/glsl_parser.yy"
   {
      /* FINISHME: The NULL test is required because pragmas are set to
       * FINISHME: NULL. (See production rule for external_declaration.)
       */
      if ((yyvsp[0].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[0].node)->link);
   }
#line 2837 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 21: /* external_declaration_list: external_declaration_list external_declaration  */
#line 410 "src/compiler/glsl/glsl_parser.yy"
   {
      /* FINISHME: The NULL test is required because pragmas are set to
       * FINISHME: NULL. (See production rule for external_declaration.)
       */
      if ((yyvsp[0].node) != NULL)
         state->translation_unit.push_tail(& (yyvsp[0].node)->link);
   }
#line 2849 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 22: /* external_declaration_list: external_declaration_list extension_statement  */
#line 417 "src/compiler/glsl/glsl_parser.yy"
                                                   {
      if (!state->allow_extension_directive_midshader) {
         _mesa_glsl_error(& (yylsp[0]), state,
                          "#extension directive is not allowed "
                          "in the middle of a shader");
         YYERROR;
      }
   }
#line 2862 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 25: /* primary_expression: variable_identifier  */
#line 434 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_identifier, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.identifier = (yyvsp[0].identifier);
   }
#line 2873 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 26: /* primary_expression: INTCONSTANT  */
#line 441 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_int_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.int_constant = (yyvsp[0].n);
   }
#line 2884 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 27: /* primary_expression: UINTCONSTANT  */
#line 448 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_uint_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.uint_constant = (yyvsp[0].n);
   }
#line 2895 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 28: /* primary_expression: INT64CONSTANT  */
#line 455 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_int64_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.int64_constant = (yyvsp[0].n64);
   }
#line 2906 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 29: /* primary_expression: UINT64CONSTANT  */
#line 462 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_uint64_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.uint64_constant = (yyvsp[0].n64);
   }
#line 2917 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 30: /* primary_expression: FLOAT16CONSTANT  */
#line 469 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_float16_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.float16_constant = (yyvsp[0].real);
   }
#line 2928 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 31: /* primary_expression: FLOATCONSTANT  */
#line 476 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_float_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.float_constant = (yyvsp[0].real);
   }
#line 2939 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 32: /* primary_expression: DOUBLECONSTANT  */
#line 483 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_double_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.double_constant = (yyvsp[0].dreal);
   }
#line 2950 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 33: /* primary_expression: BOOLCONSTANT  */
#line 490 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_bool_constant, NULL, NULL, NULL);
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->primary_expression.bool_constant = (yyvsp[0].n);
   }
#line 2961 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 34: /* primary_expression: '(' expression ')'  */
#line 497 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[-1].expression);
   }
#line 2969 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 36: /* postfix_expression: postfix_expression '[' integer_expression ']'  */
#line 505 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_array_index, (yyvsp[-3].expression), (yyvsp[-1].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[-3]), (yylsp[0]));
   }
#line 2979 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 37: /* postfix_expression: function_call  */
#line 511 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[0].expression);
   }
#line 2987 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 38: /* postfix_expression: postfix_expression DOT_TOK FIELD_SELECTION  */
#line 515 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_field_selection, (yyvsp[-2].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
      (yyval.expression)->primary_expression.identifier = (yyvsp[0].identifier);
   }
#line 2998 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 39: /* postfix_expression: postfix_expression INC_OP  */
#line 522 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_post_inc, (yyvsp[-1].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 3008 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 40: /* postfix_expression: postfix_expression DEC_OP  */
#line 528 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_post_dec, (yyvsp[-1].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 3018 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 48: /* function_call_header_with_parameters: function_call_header assignment_expression  */
#line 559 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[-1].expression);
      (yyval.expression)->set_location((yylsp[-1]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
   }
#line 3028 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 49: /* function_call_header_with_parameters: function_call_header_with_parameters ',' assignment_expression  */
#line 565 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[-2].expression);
      (yyval.expression)->set_location((yylsp[-2]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
   }
#line 3038 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 51: /* function_identifier: type_specifier  */
#line 581 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_function_expression((yyvsp[0].type_specifier));
      (yyval.expression)->set_location((yylsp[0]));
      }
#line 3048 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 52: /* function_identifier: postfix_expression  */
#line 587 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_function_expression((yyvsp[0].expression));
      (yyval.expression)->set_location((yylsp[0]));
      }
#line 3058 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 54: /* unary_expression: INC_OP unary_expression  */
#line 602 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_pre_inc, (yyvsp[0].expression), NULL, NULL);
      (yyval.expression)->set_location((yylsp[-1]));
   }
#line 3068 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 55: /* unary_expression: DEC_OP unary_expression  */
#line 608 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_pre_dec, (yyvsp[0].expression), NULL, NULL);
      (yyval.expression)->set_location((yylsp[-1]));
   }
#line 3078 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 56: /* unary_expression: unary_operator unary_expression  */
#line 614 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression((yyvsp[-1].n), (yyvsp[0].expression), NULL, NULL);
      (yyval.expression)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 3088 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 57: /* unary_operator: '+'  */
#line 623 "src/compiler/glsl/glsl_parser.yy"
         { (yyval.n) = ast_plus; }
#line 3094 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 58: /* unary_operator: '-'  */
#line 624 "src/compiler/glsl/glsl_parser.yy"
         { (yyval.n) = ast_neg; }
#line 3100 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 59: /* unary_operator: '!'  */
#line 625 "src/compiler/glsl/glsl_parser.yy"
         { (yyval.n) = ast_logic_not; }
#line 3106 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 60: /* unary_operator: '~'  */
#line 626 "src/compiler/glsl/glsl_parser.yy"
         { (yyval.n) = ast_bit_not; }
#line 3112 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 62: /* multiplicative_expression: multiplicative_expression '*' unary_expression  */
#line 632 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_mul, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3122 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 63: /* multiplicative_expression: multiplicative_expression '/' unary_expression  */
#line 638 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_div, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3132 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 64: /* multiplicative_expression: multiplicative_expression '%' unary_expression  */
#line 644 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_mod, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3142 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 66: /* additive_expression: additive_expression '+' multiplicative_expression  */
#line 654 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_add, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3152 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 67: /* additive_expression: additive_expression '-' multiplicative_expression  */
#line 660 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_sub, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3162 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 69: /* shift_expression: shift_expression LEFT_OP additive_expression  */
#line 670 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_lshift, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3172 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 70: /* shift_expression: shift_expression RIGHT_OP additive_expression  */
#line 676 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_rshift, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3182 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 72: /* relational_expression: relational_expression '<' shift_expression  */
#line 686 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_less, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3192 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 73: /* relational_expression: relational_expression '>' shift_expression  */
#line 692 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_greater, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3202 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 74: /* relational_expression: relational_expression LE_OP shift_expression  */
#line 698 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_lequal, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3212 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 75: /* relational_expression: relational_expression GE_OP shift_expression  */
#line 704 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_gequal, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3222 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 77: /* equality_expression: equality_expression EQ_OP relational_expression  */
#line 714 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_equal, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3232 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 78: /* equality_expression: equality_expression NE_OP relational_expression  */
#line 720 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_nequal, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3242 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 80: /* and_expression: and_expression '&' equality_expression  */
#line 730 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_and, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3252 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 82: /* exclusive_or_expression: exclusive_or_expression '^' and_expression  */
#line 740 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_xor, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3262 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 84: /* inclusive_or_expression: inclusive_or_expression '|' exclusive_or_expression  */
#line 750 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_bit_or, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3272 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 86: /* logical_and_expression: logical_and_expression AND_OP inclusive_or_expression  */
#line 760 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_and, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3282 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 88: /* logical_xor_expression: logical_xor_expression XOR_OP logical_and_expression  */
#line 770 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_xor, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3292 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 90: /* logical_or_expression: logical_or_expression OR_OP logical_xor_expression  */
#line 780 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression_bin(ast_logic_or, (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3302 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 92: /* conditional_expression: logical_or_expression '?' expression ':' assignment_expression  */
#line 790 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression(ast_conditional, (yyvsp[-4].expression), (yyvsp[-2].expression), (yyvsp[0].expression));
      (yyval.expression)->set_location_range((yylsp[-4]), (yylsp[0]));
   }
#line 3312 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 94: /* assignment_expression: unary_expression assignment_operator assignment_expression  */
#line 800 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_expression((yyvsp[-1].n), (yyvsp[-2].expression), (yyvsp[0].expression), NULL);
      (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 3322 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 95: /* assignment_operator: '='  */
#line 808 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_assign; }
#line 3328 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 96: /* assignment_operator: MUL_ASSIGN  */
#line 809 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_mul_assign; }
#line 3334 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 97: /* assignment_operator: DIV_ASSIGN  */
#line 810 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_div_assign; }
#line 3340 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 98: /* assignment_operator: MOD_ASSIGN  */
#line 811 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_mod_assign; }
#line 3346 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 99: /* assignment_operator: ADD_ASSIGN  */
#line 812 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_add_assign; }
#line 3352 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 100: /* assignment_operator: SUB_ASSIGN  */
#line 813 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_sub_assign; }
#line 3358 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 101: /* assignment_operator: LEFT_ASSIGN  */
#line 814 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_ls_assign; }
#line 3364 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 102: /* assignment_operator: RIGHT_ASSIGN  */
#line 815 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_rs_assign; }
#line 3370 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 103: /* assignment_operator: AND_ASSIGN  */
#line 816 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_and_assign; }
#line 3376 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 104: /* assignment_operator: XOR_ASSIGN  */
#line 817 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_xor_assign; }
#line 3382 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 105: /* assignment_operator: OR_ASSIGN  */
#line 818 "src/compiler/glsl/glsl_parser.yy"
                      { (yyval.n) = ast_or_assign; }
#line 3388 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 106: /* expression: assignment_expression  */
#line 823 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[0].expression);
   }
#line 3396 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 107: /* expression: expression ',' assignment_expression  */
#line 827 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      if ((yyvsp[-2].expression)->oper != ast_sequence) {
         (yyval.expression) = new(ctx) ast_expression(ast_sequence, NULL, NULL, NULL);
         (yyval.expression)->set_location_range((yylsp[-2]), (yylsp[0]));
         (yyval.expression)->expressions.push_tail(& (yyvsp[-2].expression)->link);
      } else {
         (yyval.expression) = (yyvsp[-2].expression);
      }

      (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
   }
#line 3413 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 109: /* declaration: function_prototype ';'  */
#line 847 "src/compiler/glsl/glsl_parser.yy"
   {
      state->symbols->pop_scope();
      (yyval.node) = (yyvsp[-1].function);
   }
#line 3422 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 110: /* declaration: init_declarator_list ';'  */
#line 852 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = (yyvsp[-1].declarator_list);
   }
#line 3430 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 111: /* declaration: PRECISION precision_qualifier type_specifier ';'  */
#line 856 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyvsp[-1].type_specifier)->default_precision = (yyvsp[-2].n);
      (yyval.node) = (yyvsp[-1].type_specifier);
   }
#line 3439 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 112: /* declaration: interface_block  */
#line 861 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_interface_block *block = (ast_interface_block *) (yyvsp[0].node);
      if (block->layout.has_layout() || block->layout.has_memory()) {
         if (!block->default_layout.merge_qualifier(& (yylsp[0]), state, block->layout, false)) {
            YYERROR;
         }
      }
      block->layout = block->default_layout;
      if (!block->layout.push_to_global(& (yylsp[0]), state)) {
         YYERROR;
      }
      (yyval.node) = (yyvsp[0].node);
   }
#line 3457 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 116: /* function_header_with_parameters: function_header parameter_declaration  */
#line 887 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.function) = (yyvsp[-1].function);
      (yyval.function)->parameters.push_tail(& (yyvsp[0].parameter_declarator)->link);
   }
#line 3466 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 117: /* function_header_with_parameters: function_header_with_parameters ',' parameter_declaration  */
#line 892 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.function) = (yyvsp[-2].function);
      (yyval.function)->parameters.push_tail(& (yyvsp[0].parameter_declarator)->link);
   }
#line 3475 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 118: /* function_header: fully_specified_type variable_identifier '('  */
#line 900 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.function) = new(ctx) ast_function();
      (yyval.function)->set_location((yylsp[-1]));
      (yyval.function)->return_type = (yyvsp[-2].fully_specified_type);
      (yyval.function)->identifier = (yyvsp[-1].identifier);

      if ((yyvsp[-2].fully_specified_type)->qualifier.is_subroutine_decl()) {
         /* add type for IDENTIFIER search */
         state->symbols->add_type((yyvsp[-1].identifier), glsl_subroutine_type((yyvsp[-1].identifier)));
      } else
         state->symbols->add_function(new(ctx) ir_function((yyvsp[-1].identifier)));
      state->symbols->push_scope();
   }
#line 3494 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 119: /* parameter_declarator: type_specifier any_identifier  */
#line 918 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location((yylsp[-1]));
      (yyval.parameter_declarator)->type->specifier = (yyvsp[-1].type_specifier);
      (yyval.parameter_declarator)->identifier = (yyvsp[0].identifier);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[0].identifier), ir_var_auto));
   }
#line 3509 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 120: /* parameter_declarator: layout_qualifier type_specifier any_identifier  */
#line 929 "src/compiler/glsl/glsl_parser.yy"
   {
      _mesa_glsl_error(&(yylsp[-2]), state, "is is not allowed on function parameter");
      YYERROR;
   }
#line 3518 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 121: /* parameter_declarator: type_specifier any_identifier array_specifier  */
#line 934 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location_range((yylsp[-2]), (yylsp[0]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location((yylsp[-2]));
      (yyval.parameter_declarator)->type->specifier = (yyvsp[-2].type_specifier);
      (yyval.parameter_declarator)->identifier = (yyvsp[-1].identifier);
      (yyval.parameter_declarator)->array_specifier = (yyvsp[0].array_specifier);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-1].identifier), ir_var_auto));
   }
#line 3534 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 122: /* parameter_declaration: parameter_qualifier parameter_declarator  */
#line 949 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.parameter_declarator) = (yyvsp[0].parameter_declarator);
      (yyval.parameter_declarator)->type->qualifier = (yyvsp[-1].type_qualifier);
      if (!(yyval.parameter_declarator)->type->qualifier.push_to_global(& (yylsp[-1]), state)) {
         YYERROR;
      }
   }
#line 3546 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 123: /* parameter_declaration: parameter_qualifier parameter_type_specifier  */
#line 957 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.parameter_declarator) = new(ctx) ast_parameter_declarator();
      (yyval.parameter_declarator)->set_location((yylsp[0]));
      (yyval.parameter_declarator)->type = new(ctx) ast_fully_specified_type();
      (yyval.parameter_declarator)->type->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.parameter_declarator)->type->qualifier = (yyvsp[-1].type_qualifier);
      if (!(yyval.parameter_declarator)->type->qualifier.push_to_global(& (yylsp[-1]), state)) {
         YYERROR;
      }
      (yyval.parameter_declarator)->type->specifier = (yyvsp[0].type_specifier);
   }
#line 3563 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 124: /* parameter_qualifier: %empty  */
#line 973 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
   }
#line 3571 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 125: /* parameter_qualifier: CONST_TOK parameter_qualifier  */
#line 977 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).flags.q.constant)
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate const qualifier");

      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).flags.q.constant = 1;
   }
#line 3583 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 126: /* parameter_qualifier: PRECISE parameter_qualifier  */
#line 985 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate precise qualifier");

      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).flags.q.precise = 1;
   }
#line 3595 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 127: /* parameter_qualifier: parameter_direction_qualifier parameter_qualifier  */
#line 993 "src/compiler/glsl/glsl_parser.yy"
   {
      if (((yyvsp[-1].type_qualifier).flags.q.in || (yyvsp[-1].type_qualifier).flags.q.out) && ((yyvsp[0].type_qualifier).flags.q.in || (yyvsp[0].type_qualifier).flags.q.out))
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate in/out/inout qualifier");

      if (!state->has_420pack_or_es31() && (yyvsp[0].type_qualifier).flags.q.constant)
         _mesa_glsl_error(&(yylsp[-1]), state, "in/out/inout must come after const "
                                      "or precise");

      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 3611 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 128: /* parameter_qualifier: precision_qualifier parameter_qualifier  */
#line 1005 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).precision != ast_precision_none)
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate precision qualifier");

      if (!state->has_420pack_or_es31() &&
          (yyvsp[0].type_qualifier).flags.i != 0)
         _mesa_glsl_error(&(yylsp[-1]), state, "precision qualifiers must come last");

      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).precision = (yyvsp[-1].n);
   }
#line 3627 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 129: /* parameter_qualifier: memory_qualifier parameter_qualifier  */
#line 1017 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 3636 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 130: /* parameter_direction_qualifier: IN_TOK  */
#line 1024 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
   }
#line 3645 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 131: /* parameter_direction_qualifier: OUT_TOK  */
#line 1029 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.out = 1;
   }
#line 3654 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 132: /* parameter_direction_qualifier: INOUT_TOK  */
#line 1034 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
      (yyval.type_qualifier).flags.q.out = 1;
   }
#line 3664 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 135: /* init_declarator_list: init_declarator_list ',' any_identifier  */
#line 1048 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), NULL, NULL);
      decl->set_location((yylsp[0]));

      (yyval.declarator_list) = (yyvsp[-2].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[0].identifier), ir_var_auto));
   }
#line 3678 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 136: /* init_declarator_list: init_declarator_list ',' any_identifier array_specifier  */
#line 1058 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-1].identifier), (yyvsp[0].array_specifier), NULL);
      decl->set_location_range((yylsp[-1]), (yylsp[0]));

      (yyval.declarator_list) = (yyvsp[-3].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-1].identifier), ir_var_auto));
   }
#line 3692 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 137: /* init_declarator_list: init_declarator_list ',' any_identifier array_specifier '=' initializer  */
#line 1068 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-3].identifier), (yyvsp[-2].array_specifier), (yyvsp[0].expression));
      decl->set_location_range((yylsp[-3]), (yylsp[-2]));

      (yyval.declarator_list) = (yyvsp[-5].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-3].identifier), ir_var_auto));
   }
#line 3706 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 138: /* init_declarator_list: init_declarator_list ',' any_identifier '=' initializer  */
#line 1078 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), NULL, (yyvsp[0].expression));
      decl->set_location((yylsp[-2]));

      (yyval.declarator_list) = (yyvsp[-4].declarator_list);
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-2].identifier), ir_var_auto));
   }
#line 3720 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 139: /* single_declaration: fully_specified_type  */
#line 1092 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      /* Empty declaration list is valid. */
      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[0].fully_specified_type));
      (yyval.declarator_list)->set_location((yylsp[0]));
   }
#line 3731 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 140: /* single_declaration: fully_specified_type any_identifier  */
#line 1099 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), NULL, NULL);
      decl->set_location((yylsp[0]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-1].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[0].identifier), ir_var_auto));
   }
#line 3746 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 141: /* single_declaration: fully_specified_type any_identifier array_specifier  */
#line 1110 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-1].identifier), (yyvsp[0].array_specifier), NULL);
      decl->set_location_range((yylsp[-1]), (yylsp[0]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-2].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[-2]), (yylsp[0]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-1].identifier), ir_var_auto));
   }
#line 3761 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 142: /* single_declaration: fully_specified_type any_identifier array_specifier '=' initializer  */
#line 1121 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-3].identifier), (yyvsp[-2].array_specifier), (yyvsp[0].expression));
      decl->set_location_range((yylsp[-3]), (yylsp[-2]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-4].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[-4]), (yylsp[-2]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-3].identifier), ir_var_auto));
   }
#line 3776 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 143: /* single_declaration: fully_specified_type any_identifier '=' initializer  */
#line 1132 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), NULL, (yyvsp[0].expression));
      decl->set_location((yylsp[-2]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list((yyvsp[-3].fully_specified_type));
      (yyval.declarator_list)->set_location_range((yylsp[-3]), (yylsp[-2]));
      (yyval.declarator_list)->declarations.push_tail(&decl->link);
      state->symbols->add_variable(new(ctx) ir_variable(NULL, (yyvsp[-2].identifier), ir_var_auto));
   }
#line 3791 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 144: /* single_declaration: INVARIANT variable_identifier  */
#line 1143 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), NULL, NULL);
      decl->set_location((yylsp[0]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
      (yyval.declarator_list)->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.declarator_list)->invariant = true;

      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   }
#line 3807 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 145: /* single_declaration: PRECISE variable_identifier  */
#line 1155 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[0].identifier), NULL, NULL);
      decl->set_location((yylsp[0]));

      (yyval.declarator_list) = new(ctx) ast_declarator_list(NULL);
      (yyval.declarator_list)->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.declarator_list)->precise = true;

      (yyval.declarator_list)->declarations.push_tail(&decl->link);
   }
#line 3823 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 146: /* fully_specified_type: type_specifier  */
#line 1170 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
      (yyval.fully_specified_type)->set_location((yylsp[0]));
      (yyval.fully_specified_type)->specifier = (yyvsp[0].type_specifier);
   }
#line 3834 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 147: /* fully_specified_type: type_qualifier type_specifier  */
#line 1177 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.fully_specified_type) = new(ctx) ast_fully_specified_type();
      (yyval.fully_specified_type)->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.fully_specified_type)->qualifier = (yyvsp[-1].type_qualifier);
      if (!(yyval.fully_specified_type)->qualifier.push_to_global(& (yylsp[-1]), state)) {
         YYERROR;
      }
      (yyval.fully_specified_type)->specifier = (yyvsp[0].type_specifier);
      if ((yyval.fully_specified_type)->specifier->structure != NULL &&
          (yyval.fully_specified_type)->specifier->structure->is_declaration) {
            (yyval.fully_specified_type)->specifier->structure->layout = &(yyval.fully_specified_type)->qualifier;
      }
   }
#line 3853 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 148: /* layout_qualifier: LAYOUT_TOK '(' layout_qualifier_id_list ')'  */
#line 1195 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
   }
#line 3861 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 150: /* layout_qualifier_id_list: layout_qualifier_id_list ',' layout_qualifier_id  */
#line 1203 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-2].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[0]), state, (yyvsp[0].type_qualifier), true)) {
         YYERROR;
      }
   }
#line 3872 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 151: /* layout_qualifier_id: any_identifier  */
#line 1213 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));

      /* Layout qualifiers for ARB_fragment_coord_conventions. */
      if (!(yyval.type_qualifier).flags.i && (state->ARB_fragment_coord_conventions_enable ||
                          state->is_version(150, 0))) {
         if (match_layout_qualifier((yyvsp[0].identifier), "origin_upper_left", state) == 0) {
            (yyval.type_qualifier).flags.q.origin_upper_left = 1;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "pixel_center_integer",
                                           state) == 0) {
            (yyval.type_qualifier).flags.q.pixel_center_integer = 1;
         }

         if ((yyval.type_qualifier).flags.i && state->ARB_fragment_coord_conventions_warn) {
            _mesa_glsl_warning(& (yylsp[0]), state,
                               "GL_ARB_fragment_coord_conventions layout "
                               "identifier `%s' used", (yyvsp[0].identifier));
         }
      }

      /* Layout qualifiers for AMD/ARB_conservative_depth. */
      if (!(yyval.type_qualifier).flags.i &&
          (state->AMD_conservative_depth_enable ||
           state->ARB_conservative_depth_enable ||
           state->EXT_conservative_depth_enable ||
           state->is_version(420, 0))) {
         if (match_layout_qualifier((yyvsp[0].identifier), "depth_any", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_any;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "depth_greater", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_greater;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "depth_less", state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_less;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "depth_unchanged",
                                           state) == 0) {
            (yyval.type_qualifier).flags.q.depth_type = 1;
            (yyval.type_qualifier).depth_type = ast_depth_unchanged;
         }

         if ((yyval.type_qualifier).flags.i && state->AMD_conservative_depth_warn) {
            _mesa_glsl_warning(& (yylsp[0]), state,
                               "GL_AMD_conservative_depth "
                               "layout qualifier `%s' is used", (yyvsp[0].identifier));
         }
         if ((yyval.type_qualifier).flags.i && state->ARB_conservative_depth_warn) {
            _mesa_glsl_warning(& (yylsp[0]), state,
                               "GL_ARB_conservative_depth "
                               "layout qualifier `%s' is used", (yyvsp[0].identifier));
         }
         if ((yyval.type_qualifier).flags.i && state->EXT_conservative_depth_warn) {
            _mesa_glsl_warning(& (yylsp[0]), state,
                               "GL_EXT_conservative_depth "
                               "layout qualifier `%s' is used", (yyvsp[0].identifier));
         }
      }

      /* See also interface_block_layout_qualifier. */
      if (!(yyval.type_qualifier).flags.i &&
          (state->has_uniform_buffer_objects() ||
           state->has_shader_storage_buffer_objects())) {
         if (match_layout_qualifier((yyvsp[0].identifier), "std140", state) == 0) {
            (yyval.type_qualifier).flags.q.std140 = 1;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "shared", state) == 0) {
            (yyval.type_qualifier).flags.q.shared = 1;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "std430", state) == 0) {
            (yyval.type_qualifier).flags.q.std430 = 1;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "column_major", state) == 0) {
            (yyval.type_qualifier).flags.q.column_major = 1;
         /* "row_major" is a reserved word in GLSL 1.30+. Its token is parsed
          * below in the interface_block_layout_qualifier rule.
          *
          * It is not a reserved word in GLSL ES 3.00, so it's handled here as
          * an identifier.
          *
          * Also, this takes care of alternate capitalizations of
          * "row_major" (which is necessary because layout qualifiers
          * are case-insensitive in desktop GLSL).
          */
         } else if (match_layout_qualifier((yyvsp[0].identifier), "row_major", state) == 0) {
            (yyval.type_qualifier).flags.q.row_major = 1;
         /* "packed" is a reserved word in GLSL, and its token is
          * parsed below in the interface_block_layout_qualifier rule.
          * However, we must take care of alternate capitalizations of
          * "packed", because layout qualifiers are case-insensitive
          * in desktop GLSL.
          */
         } else if (match_layout_qualifier((yyvsp[0].identifier), "packed", state) == 0) {
           (yyval.type_qualifier).flags.q.packed = 1;
         }

         if ((yyval.type_qualifier).flags.i &&
             (state->ARB_uniform_buffer_object_warn ||
              state->ARB_shader_storage_buffer_object_warn)) {
            _mesa_glsl_warning(& (yylsp[0]), state,
                               "#version 140 / GL_ARB_uniform_buffer_object "
                               "/ GL_ARB_shader_storage_buffer_object"
                               "layout qualifier `%s' is used", (yyvsp[0].identifier));
         }
      }

      /* Layout qualifiers for GLSL 1.50 geometry shaders. */
      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            GLenum e;
         } map[] = {
                 { "points", GL_POINTS },
                 { "lines", GL_LINES },
                 { "lines_adjacency", GL_LINES_ADJACENCY },
                 { "line_strip", GL_LINE_STRIP },
                 { "triangles", GL_TRIANGLES },
                 { "triangles_adjacency", GL_TRIANGLES_ADJACENCY },
                 { "triangle_strip", GL_TRIANGLE_STRIP },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[0].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.prim_type = 1;
               (yyval.type_qualifier).prim_type = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->has_geometry_shader() &&
             !state->has_tessellation_shader() && !state->EXT_mesh_shader_enable) {
            _mesa_glsl_error(& (yylsp[0]), state, "#version 150 layout "
                             "qualifier `%s' used", (yyvsp[0].identifier));
         }
      }

      /* Layout qualifiers for ARB_shader_image_load_store. */
      if (state->has_shader_image_load_store() ||
          state->EXT_shader_pixel_local_storage_enable) {
         if (!(yyval.type_qualifier).flags.i) {
            static const struct {
               const char *name;
               enum pipe_format format;
               glsl_base_type base_type;
               /** Minimum desktop GLSL version required for the image
                * format.  Use 130 if already present in the original
                * ARB extension.
                */
               unsigned required_glsl;
               /** Minimum GLSL ES version required for the image format. */
               unsigned required_essl;
               /* NV_image_formats */
               bool nv_image_formats;
               bool ext_qualifiers;
               /* for __pixel_localEXT */
               bool pixel_local_qualifiers;
            } map[] = {
               { "rgba32f", PIPE_FORMAT_R32G32B32A32_FLOAT, GLSL_TYPE_FLOAT, 130, 310, false, false, false },
               { "rgba16f", PIPE_FORMAT_R16G16B16A16_FLOAT, GLSL_TYPE_FLOAT, 130, 310, false, false, false },
               { "rg32f", PIPE_FORMAT_R32G32_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "rg16f", PIPE_FORMAT_R16G16_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false, true },
               { "r11f_g11f_b10f", PIPE_FORMAT_R11G11B10_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false, true },
               { "r32f", PIPE_FORMAT_R32_FLOAT, GLSL_TYPE_FLOAT, 130, 310, false, false, true },
               { "r16f", PIPE_FORMAT_R16_FLOAT, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "rgba32ui", PIPE_FORMAT_R32G32B32A32_UINT, GLSL_TYPE_UINT, 130, 310, false, false, false },
               { "rgba16ui", PIPE_FORMAT_R16G16B16A16_UINT, GLSL_TYPE_UINT, 130, 310, false, false, false },
               { "rgb10_a2ui", PIPE_FORMAT_R10G10B10A2_UINT, GLSL_TYPE_UINT, 130, 0, true, false, true },
               { "rgba8ui", PIPE_FORMAT_R8G8B8A8_UINT, GLSL_TYPE_UINT, 130, 310, false, false, true },
               { "rg32ui", PIPE_FORMAT_R32G32_UINT, GLSL_TYPE_UINT, 130, 0, true, false, false },
               { "rg16ui", PIPE_FORMAT_R16G16_UINT, GLSL_TYPE_UINT, 130, 0, true, false, true },
               { "rg8ui", PIPE_FORMAT_R8G8_UINT, GLSL_TYPE_UINT, 130, 0, true, false, false },
               { "r32ui", PIPE_FORMAT_R32_UINT, GLSL_TYPE_UINT, 130, 310, false, false, true },
               { "r16ui", PIPE_FORMAT_R16_UINT, GLSL_TYPE_UINT, 130, 0, true, false, false },
               { "r8ui", PIPE_FORMAT_R8_UINT, GLSL_TYPE_UINT, 130, 0, true, false, false },
               { "rgba32i", PIPE_FORMAT_R32G32B32A32_SINT, GLSL_TYPE_INT, 130, 310, false, false, false },
               { "rgba16i", PIPE_FORMAT_R16G16B16A16_SINT, GLSL_TYPE_INT, 130, 310, false, false, false },
               { "rgba8i", PIPE_FORMAT_R8G8B8A8_SINT, GLSL_TYPE_INT, 130, 310, false, false, true },
               { "rg32i", PIPE_FORMAT_R32G32_SINT, GLSL_TYPE_INT, 130, 0, true, false, false },
               { "rg16i", PIPE_FORMAT_R16G16_SINT, GLSL_TYPE_INT, 130, 0, true, false, true },
               { "rg8i", PIPE_FORMAT_R8G8_SINT, GLSL_TYPE_INT, 130, 0, true, false, false },
               { "r32i", PIPE_FORMAT_R32_SINT, GLSL_TYPE_INT, 130, 310, false, false, true },
               { "r16i", PIPE_FORMAT_R16_SINT, GLSL_TYPE_INT, 130, 0, true, false, false },
               { "r8i", PIPE_FORMAT_R8_SINT, GLSL_TYPE_INT, 130, 0, true, false, false },
               { "rgba16", PIPE_FORMAT_R16G16B16A16_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "rgb10_a2", PIPE_FORMAT_R10G10B10A2_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, true },
               { "rgba8", PIPE_FORMAT_R8G8B8A8_UNORM, GLSL_TYPE_FLOAT, 130, 310, false, false, true },
               { "rg16", PIPE_FORMAT_R16G16_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, true },
               { "rg8", PIPE_FORMAT_R8G8_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "r16", PIPE_FORMAT_R16_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "r8", PIPE_FORMAT_R8_UNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "rgba16_snorm", PIPE_FORMAT_R16G16B16A16_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "rgba8_snorm", PIPE_FORMAT_R8G8B8A8_SNORM, GLSL_TYPE_FLOAT, 130, 310, false, false, false },
               { "rg16_snorm", PIPE_FORMAT_R16G16_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "rg8_snorm", PIPE_FORMAT_R8G8_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "r16_snorm", PIPE_FORMAT_R16_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },
               { "r8_snorm", PIPE_FORMAT_R8_SNORM, GLSL_TYPE_FLOAT, 130, 0, true, false, false },

               /* From GL_EXT_shader_image_load_store: */
               /* base_type is incorrect but it'll be patched later when we know
                * the variable type. See ast_to_hir.cpp */
               { "size1x8", PIPE_FORMAT_R8_SINT, GLSL_TYPE_VOID, 130, 0, false, true, false },
               { "size1x16", PIPE_FORMAT_R16_SINT, GLSL_TYPE_VOID, 130, 0, false, true, false },
               { "size1x32", PIPE_FORMAT_R32_SINT, GLSL_TYPE_VOID, 130, 0, false, true, false },
               { "size2x32", PIPE_FORMAT_R32G32_SINT, GLSL_TYPE_VOID, 130, 0, false, true, false },
               { "size4x32", PIPE_FORMAT_R32G32B32A32_SINT, GLSL_TYPE_VOID, 130, 0, false, true, false },
            };

            for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
               if (state->EXT_shader_pixel_local_storage_enable &&
                   map[i].pixel_local_qualifiers &&
                   match_layout_qualifier((yyvsp[0].identifier), map[i].name, state) == 0) {
                  (yyval.type_qualifier).flags.q.explicit_image_format = 1;
                  (yyval.type_qualifier).image_format = map[i].format;
                  (yyval.type_qualifier).image_base_type = map[i].base_type;
                  break;
               }
               if ((state->is_version(map[i].required_glsl,
                                      map[i].required_essl) ||
                    (state->NV_image_formats_enable &&
                     map[i].nv_image_formats)) &&
                   match_layout_qualifier((yyvsp[0].identifier), map[i].name, state) == 0) {
                  /* Skip ARB_shader_image_load_store qualifiers if not enabled */
                  if (!map[i].ext_qualifiers && !(state->ARB_shader_image_load_store_enable ||
                                                  state->is_version(420, 310))) {
                     continue;
                  }
                  /* Skip EXT_shader_image_load_store qualifiers if not enabled */
                  if (map[i].ext_qualifiers && !state->EXT_shader_image_load_store_enable) {
                     continue;
                  }
                  (yyval.type_qualifier).flags.q.explicit_image_format = 1;
                  (yyval.type_qualifier).image_format = map[i].format;
                  (yyval.type_qualifier).image_base_type = map[i].base_type;
                  break;
               }
            }
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[0].identifier), "early_fragment_tests", state) == 0) {
            /* From section 4.4.1.3 of the GLSL 4.50 specification
             * (Fragment Shader Inputs):
             *
             *  "Fragment shaders also allow the following layout
             *   qualifier on in only (not with variable declarations)
             *     layout-qualifier-id
             *        early_fragment_tests
             *   [...]"
             */
            if (state->stage != MESA_SHADER_FRAGMENT) {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "early_fragment_tests layout qualifier only "
                                "valid in fragment shaders");
            }

            (yyval.type_qualifier).flags.q.early_fragment_tests = 1;
         }

         if (match_layout_qualifier((yyvsp[0].identifier), "inner_coverage", state) == 0) {
            if (state->stage != MESA_SHADER_FRAGMENT) {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "inner_coverage layout qualifier only "
                                "valid in fragment shaders");
            }

            if (state->INTEL_conservative_rasterization_enable) {
               (yyval.type_qualifier).flags.q.inner_coverage = 1;
            } else {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "inner_coverage layout qualifier present, "
                                "but the INTEL_conservative_rasterization extension "
                                "is not enabled.");
            }
         }

         if (match_layout_qualifier((yyvsp[0].identifier), "post_depth_coverage", state) == 0) {
            if (state->stage != MESA_SHADER_FRAGMENT) {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "post_depth_coverage layout qualifier only "
                                "valid in fragment shaders");
            }

            if (state->ARB_post_depth_coverage_enable ||
                state->INTEL_conservative_rasterization_enable) {
               (yyval.type_qualifier).flags.q.post_depth_coverage = 1;
            } else {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "post_depth_coverage layout qualifier present, "
                                "but the GL_ARB_post_depth_coverage extension "
                                "is not enabled.");
            }
         }

         if ((yyval.type_qualifier).flags.q.post_depth_coverage && (yyval.type_qualifier).flags.q.inner_coverage) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "post_depth_coverage & inner_coverage layout qualifiers "
                             "are mutually exclusive");
         }
      }

      const bool pixel_interlock_ordered = match_layout_qualifier((yyvsp[0].identifier),
         "pixel_interlock_ordered", state) == 0;
      const bool pixel_interlock_unordered = match_layout_qualifier((yyvsp[0].identifier),
         "pixel_interlock_unordered", state) == 0;
      const bool sample_interlock_ordered = match_layout_qualifier((yyvsp[0].identifier),
         "sample_interlock_ordered", state) == 0;
      const bool sample_interlock_unordered = match_layout_qualifier((yyvsp[0].identifier),
         "sample_interlock_unordered", state) == 0;

      if (pixel_interlock_ordered + pixel_interlock_unordered +
          sample_interlock_ordered + sample_interlock_unordered > 0 &&
          state->stage != MESA_SHADER_FRAGMENT) {
         _mesa_glsl_error(& (yylsp[0]), state, "interlock layout qualifiers: "
                          "pixel_interlock_ordered, pixel_interlock_unordered, "
                          "sample_interlock_ordered and sample_interlock_unordered, "
                          "only valid in fragment shader input layout declaration.");
      } else if (pixel_interlock_ordered + pixel_interlock_unordered +
                 sample_interlock_ordered + sample_interlock_unordered > 0 &&
                 !state->ARB_fragment_shader_interlock_enable &&
                 !state->NV_fragment_shader_interlock_enable) {
         _mesa_glsl_error(& (yylsp[0]), state,
                          "interlock layout qualifier present, but the "
                          "GL_ARB_fragment_shader_interlock or "
                          "GL_NV_fragment_shader_interlock extension is not "
                          "enabled.");
      } else {
         (yyval.type_qualifier).flags.q.pixel_interlock_ordered = pixel_interlock_ordered;
         (yyval.type_qualifier).flags.q.pixel_interlock_unordered = pixel_interlock_unordered;
         (yyval.type_qualifier).flags.q.sample_interlock_ordered = sample_interlock_ordered;
         (yyval.type_qualifier).flags.q.sample_interlock_unordered = sample_interlock_unordered;
      }

      /* Layout qualifiers for tessellation evaluation shaders. */
      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            GLenum e;
         } map[] = {
                 /* triangles already parsed by gs-specific code */
                 { "quads", GL_QUADS },
                 { "isolines", GL_ISOLINES },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[0].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.prim_type = 1;
               (yyval.type_qualifier).prim_type = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "primitive mode qualifier `%s' requires "
                             "GLSL 4.00 or ARB_tessellation_shader", (yyvsp[0].identifier));
         }
      }
      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            enum gl_tess_spacing e;
         } map[] = {
                 { "equal_spacing", TESS_SPACING_EQUAL },
                 { "fractional_odd_spacing", TESS_SPACING_FRACTIONAL_ODD },
                 { "fractional_even_spacing", TESS_SPACING_FRACTIONAL_EVEN },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[0].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.vertex_spacing = 1;
               (yyval.type_qualifier).vertex_spacing = map[i].e;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "vertex spacing qualifier `%s' requires "
                             "GLSL 4.00 or ARB_tessellation_shader", (yyvsp[0].identifier));
         }
      }
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[0].identifier), "cw", state) == 0) {
            (yyval.type_qualifier).flags.q.ordering = 1;
            (yyval.type_qualifier).ordering = GL_CW;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "ccw", state) == 0) {
            (yyval.type_qualifier).flags.q.ordering = 1;
            (yyval.type_qualifier).ordering = GL_CCW;
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "ordering qualifier `%s' requires "
                             "GLSL 4.00 or ARB_tessellation_shader", (yyvsp[0].identifier));
         }
      }
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[0].identifier), "point_mode", state) == 0) {
            (yyval.type_qualifier).flags.q.point_mode = 1;
            (yyval.type_qualifier).point_mode = true;
         }

         if ((yyval.type_qualifier).flags.i && !state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "qualifier `point_mode' requires "
                             "GLSL 4.00 or ARB_tessellation_shader");
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         static const struct {
            const char *s;
            uint32_t mask;
         } map[] = {
                 { "blend_support_multiply",       BITFIELD_BIT(PIPE_ADVANCED_BLEND_MULTIPLY) },
                 { "blend_support_screen",         BITFIELD_BIT(PIPE_ADVANCED_BLEND_SCREEN) },
                 { "blend_support_overlay",        BITFIELD_BIT(PIPE_ADVANCED_BLEND_OVERLAY) },
                 { "blend_support_darken",         BITFIELD_BIT(PIPE_ADVANCED_BLEND_DARKEN) },
                 { "blend_support_lighten",        BITFIELD_BIT(PIPE_ADVANCED_BLEND_LIGHTEN) },
                 { "blend_support_colordodge",     BITFIELD_BIT(PIPE_ADVANCED_BLEND_COLORDODGE) },
                 { "blend_support_colorburn",      BITFIELD_BIT(PIPE_ADVANCED_BLEND_COLORBURN) },
                 { "blend_support_hardlight",      BITFIELD_BIT(PIPE_ADVANCED_BLEND_HARDLIGHT) },
                 { "blend_support_softlight",      BITFIELD_BIT(PIPE_ADVANCED_BLEND_SOFTLIGHT) },
                 { "blend_support_difference",     BITFIELD_BIT(PIPE_ADVANCED_BLEND_DIFFERENCE) },
                 { "blend_support_exclusion",      BITFIELD_BIT(PIPE_ADVANCED_BLEND_EXCLUSION) },
                 { "blend_support_hsl_hue",        BITFIELD_BIT(PIPE_ADVANCED_BLEND_HSL_HUE) },
                 { "blend_support_hsl_saturation", BITFIELD_BIT(PIPE_ADVANCED_BLEND_HSL_SATURATION) },
                 { "blend_support_hsl_color",      BITFIELD_BIT(PIPE_ADVANCED_BLEND_HSL_COLOR) },
                 { "blend_support_hsl_luminosity", BITFIELD_BIT(PIPE_ADVANCED_BLEND_HSL_LUMINOSITY) },
                 { "blend_support_all_equations",  (1u << (PIPE_ADVANCED_BLEND_HSL_LUMINOSITY + 1)) - 2 },
         };
         for (unsigned i = 0; i < ARRAY_SIZE(map); i++) {
            if (match_layout_qualifier((yyvsp[0].identifier), map[i].s, state) == 0) {
               (yyval.type_qualifier).flags.q.blend_support = 1;
               state->fs_blend_support |= map[i].mask;
               break;
            }
         }

         if ((yyval.type_qualifier).flags.i &&
             !state->KHR_blend_equation_advanced_enable &&
             !state->is_version(0, 320)) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "advanced blending layout qualifiers require "
                             "ESSL 3.20 or KHR_blend_equation_advanced");
         }

         if ((yyval.type_qualifier).flags.i && state->stage != MESA_SHADER_FRAGMENT) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "advanced blending layout qualifiers only "
                             "valid in fragment shaders");
         }
      }

      /* Layout qualifiers for ARB_compute_variable_group_size. */
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[0].identifier), "local_size_variable", state) == 0) {
            (yyval.type_qualifier).flags.q.local_size_variable = 1;
         }

         if ((yyval.type_qualifier).flags.i && !state->ARB_compute_variable_group_size_enable) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "qualifier `local_size_variable` requires "
                             "ARB_compute_variable_group_size");
         }
      }

      /* Layout qualifiers for ARB_bindless_texture. */
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[0].identifier), "bindless_sampler", state) == 0)
            (yyval.type_qualifier).flags.q.bindless_sampler = 1;
         if (match_layout_qualifier((yyvsp[0].identifier), "bound_sampler", state) == 0)
            (yyval.type_qualifier).flags.q.bound_sampler = 1;

         if (state->has_shader_image_load_store()) {
            if (match_layout_qualifier((yyvsp[0].identifier), "bindless_image", state) == 0)
               (yyval.type_qualifier).flags.q.bindless_image = 1;
            if (match_layout_qualifier((yyvsp[0].identifier), "bound_image", state) == 0)
               (yyval.type_qualifier).flags.q.bound_image = 1;
         }

         if ((yyval.type_qualifier).flags.i && !state->has_bindless()) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "qualifier `%s` requires "
                             "ARB_bindless_texture", (yyvsp[0].identifier));
         }
      }

      if (!(yyval.type_qualifier).flags.i &&
          state->EXT_shader_framebuffer_fetch_non_coherent_enable) {
         if (match_layout_qualifier((yyvsp[0].identifier), "noncoherent", state) == 0)
            (yyval.type_qualifier).flags.q.non_coherent = 1;
      }

      // Layout qualifiers for NV_compute_shader_derivatives.
      if (!(yyval.type_qualifier).flags.i) {
         if (match_layout_qualifier((yyvsp[0].identifier), "derivative_group_quadsNV", state) == 0) {
            (yyval.type_qualifier).flags.q.derivative_group = 1;
            (yyval.type_qualifier).derivative_group = DERIVATIVE_GROUP_QUADS;
         } else if (match_layout_qualifier((yyvsp[0].identifier), "derivative_group_linearNV", state) == 0) {
            (yyval.type_qualifier).flags.q.derivative_group = 1;
            (yyval.type_qualifier).derivative_group = DERIVATIVE_GROUP_LINEAR;
         }

         if ((yyval.type_qualifier).flags.i) {
            if (!state->has_compute_shader()) {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "qualifier `%s' requires "
                                "a compute shader", (yyvsp[0].identifier));
            }

            if (!state->NV_compute_shader_derivatives_enable) {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "qualifier `%s' requires "
                                "NV_compute_shader_derivatives", (yyvsp[0].identifier));
            }

            if (state->NV_compute_shader_derivatives_warn) {
               _mesa_glsl_warning(& (yylsp[0]), state,
                                  "NV_compute_shader_derivatives layout "
                                  "qualifier `%s' used", (yyvsp[0].identifier));
            }
         }
      }

      /* Layout qualifier for NV_viewport_array2. */
      if (!(yyval.type_qualifier).flags.i && state->stage != MESA_SHADER_FRAGMENT) {
         if (match_layout_qualifier((yyvsp[0].identifier), "viewport_relative", state) == 0) {
            (yyval.type_qualifier).flags.q.viewport_relative = 1;
         }

         if ((yyval.type_qualifier).flags.i && !state->NV_viewport_array2_enable) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "qualifier `%s' requires "
                             "GL_NV_viewport_array2", (yyvsp[0].identifier));
         }

         if ((yyval.type_qualifier).flags.i && state->NV_viewport_array2_warn) {
            _mesa_glsl_warning(& (yylsp[0]), state,
                               "GL_NV_viewport_array2 layout "
                               "identifier `%s' used", (yyvsp[0].identifier));
         }
      }

      if (!(yyval.type_qualifier).flags.i) {
         _mesa_glsl_error(& (yylsp[0]), state, "unrecognized layout identifier "
                          "`%s'", (yyvsp[0].identifier));
         YYERROR;
      }
   }
#line 4421 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 152: /* layout_qualifier_id: any_identifier '=' constant_expression  */
#line 1758 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      linear_ctx *ctx = state->linalloc;

      if ((yyvsp[0].expression)->oper != ast_int_constant &&
          (yyvsp[0].expression)->oper != ast_uint_constant &&
          !state->has_enhanced_layouts()) {
         _mesa_glsl_error(& (yylsp[-2]), state,
                          "compile-time constant expressions require "
                          "GLSL 4.40 or ARB_enhanced_layouts");
      }

      if (match_layout_qualifier("align", (yyvsp[-2].identifier), state) == 0) {
         if (!state->has_enhanced_layouts()) {
            _mesa_glsl_error(& (yylsp[-2]), state,
                             "align qualifier requires "
                             "GLSL 4.40 or ARB_enhanced_layouts");
         } else {
            (yyval.type_qualifier).flags.q.explicit_align = 1;
            (yyval.type_qualifier).align = (yyvsp[0].expression);
         }
      }

      if (match_layout_qualifier("location", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_location = 1;

         if ((yyval.type_qualifier).flags.q.attribute == 1 &&
             state->ARB_explicit_attrib_location_warn) {
            _mesa_glsl_warning(& (yylsp[-2]), state,
                               "GL_ARB_explicit_attrib_location layout "
                               "identifier `%s' used", (yyvsp[-2].identifier));
         }
         (yyval.type_qualifier).location = (yyvsp[0].expression);
      }

      if (match_layout_qualifier("num_views", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_numviews = 1;
         (yyval.type_qualifier).num_views = (yyvsp[0].expression);
      }

      if (match_layout_qualifier("component", (yyvsp[-2].identifier), state) == 0) {
         if (!state->has_enhanced_layouts()) {
            _mesa_glsl_error(& (yylsp[-2]), state,
                             "component qualifier requires "
                             "GLSL 4.40 or ARB_enhanced_layouts");
         } else {
            (yyval.type_qualifier).flags.q.explicit_component = 1;
            (yyval.type_qualifier).component = (yyvsp[0].expression);
         }
      }

      if (match_layout_qualifier("index", (yyvsp[-2].identifier), state) == 0) {
         if (state->es_shader && !state->EXT_blend_func_extended_enable) {
            _mesa_glsl_error(& (yylsp[0]), state, "index layout qualifier requires EXT_blend_func_extended");
            YYERROR;
         }

         (yyval.type_qualifier).flags.q.explicit_index = 1;
         (yyval.type_qualifier).index = (yyvsp[0].expression);
      }

      if ((state->has_420pack_or_es31() ||
           state->has_atomic_counters() ||
           state->has_shader_storage_buffer_objects()) &&
          match_layout_qualifier("binding", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_binding = 1;
         (yyval.type_qualifier).binding = (yyvsp[0].expression);
      }

      if ((state->has_atomic_counters() ||
           state->has_enhanced_layouts()) &&
          match_layout_qualifier("offset", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.explicit_offset = 1;
         (yyval.type_qualifier).offset = (yyvsp[0].expression);
      }

      if (match_layout_qualifier("max_vertices", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.max_vertices = 1;
         (yyval.type_qualifier).max_vertices = new(ctx) ast_layout_expression((yylsp[-2]), (yyvsp[0].expression));
         if (!state->has_geometry_shader() && !state->EXT_mesh_shader_enable) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "max_vertices qualifier specified");
         }
      }

      if (match_layout_qualifier("max_primitives", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.max_primitives = 1;
         (yyval.type_qualifier).max_primitives = new(ctx) ast_layout_expression((yylsp[-2]), (yyvsp[0].expression));
         if (!state->EXT_mesh_shader_enable) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "max_primitives qualifier specified");
         }
      }

      if (state->stage == MESA_SHADER_GEOMETRY) {
         if (match_layout_qualifier("stream", (yyvsp[-2].identifier), state) == 0 &&
             state->check_explicit_attrib_stream_allowed(& (yylsp[0]))) {
            (yyval.type_qualifier).flags.q.stream = 1;
            (yyval.type_qualifier).flags.q.explicit_stream = 1;
            (yyval.type_qualifier).stream = (yyvsp[0].expression);
         }
      }

      if (state->has_enhanced_layouts()) {
         if (match_layout_qualifier("xfb_buffer", (yyvsp[-2].identifier), state) == 0) {
            (yyval.type_qualifier).flags.q.xfb_buffer = 1;
            (yyval.type_qualifier).flags.q.explicit_xfb_buffer = 1;
            (yyval.type_qualifier).xfb_buffer = (yyvsp[0].expression);
         }

         if (match_layout_qualifier("xfb_offset", (yyvsp[-2].identifier), state) == 0) {
            (yyval.type_qualifier).flags.q.explicit_xfb_offset = 1;
            (yyval.type_qualifier).offset = (yyvsp[0].expression);
         }

         if (match_layout_qualifier("xfb_stride", (yyvsp[-2].identifier), state) == 0) {
            (yyval.type_qualifier).flags.q.xfb_stride = 1;
            (yyval.type_qualifier).flags.q.explicit_xfb_stride = 1;
            (yyval.type_qualifier).xfb_stride = (yyvsp[0].expression);
         }
      }

      static const char * const local_size_qualifiers[3] = {
         "local_size_x",
         "local_size_y",
         "local_size_z",
      };
      for (int i = 0; i < 3; i++) {
         if (match_layout_qualifier(local_size_qualifiers[i], (yyvsp[-2].identifier),
                                    state) == 0) {
            if (!state->has_compute_shader() && !state->EXT_mesh_shader_enable) {
               _mesa_glsl_error(& (yylsp[0]), state,
                                "%s qualifier requires GLSL 4.30 or "
                                "GLSL ES 3.10 or ARB_compute_shader or EXT_mesh_shader",
                                local_size_qualifiers[i]);
               YYERROR;
            } else {
               (yyval.type_qualifier).flags.q.local_size |= (1 << i);
               (yyval.type_qualifier).local_size[i] = new(ctx) ast_layout_expression((yylsp[-2]), (yyvsp[0].expression));
            }
            break;
         }
      }

      if (match_layout_qualifier("invocations", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.invocations = 1;
         (yyval.type_qualifier).invocations = new(ctx) ast_layout_expression((yylsp[-2]), (yyvsp[0].expression));
         if (!state->is_version(400, 320) &&
             !state->ARB_gpu_shader5_enable &&
             !state->OES_geometry_shader_enable &&
             !state->EXT_geometry_shader_enable) {
            _mesa_glsl_error(& (yylsp[0]), state,
                             "GL_ARB_gpu_shader5 invocations "
                             "qualifier specified");
         }
      }

      /* Layout qualifiers for tessellation control shaders. */
      if (match_layout_qualifier("vertices", (yyvsp[-2].identifier), state) == 0) {
         (yyval.type_qualifier).flags.q.vertices = 1;
         (yyval.type_qualifier).vertices = new(ctx) ast_layout_expression((yylsp[-2]), (yyvsp[0].expression));
         if (!state->has_tessellation_shader()) {
            _mesa_glsl_error(& (yylsp[-2]), state,
                             "vertices qualifier requires GLSL 4.00 or "
                             "ARB_tessellation_shader");
         }
      }

      /* If the identifier didn't match any known layout identifiers,
       * emit an error.
       */
      if (!(yyval.type_qualifier).flags.i) {
         _mesa_glsl_error(& (yylsp[-2]), state, "unrecognized layout identifier "
                          "`%s'", (yyvsp[-2].identifier));
         YYERROR;
      }
   }
#line 4603 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 153: /* layout_qualifier_id: interface_block_layout_qualifier  */
#line 1936 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      /* Layout qualifiers for ARB_uniform_buffer_object. */
      if ((yyval.type_qualifier).flags.q.uniform && !state->has_uniform_buffer_objects()) {
         _mesa_glsl_error(& (yylsp[0]), state,
                          "#version 140 / GL_ARB_uniform_buffer_object "
                          "layout qualifier `uniform' is used");
      } else if ((yyval.type_qualifier).flags.q.uniform && state->ARB_uniform_buffer_object_warn) {
         _mesa_glsl_warning(& (yylsp[0]), state,
                            "#version 140 / GL_ARB_uniform_buffer_object "
                            "layout qualifier `uniform' is used");
      }
   }
#line 4621 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 154: /* interface_block_layout_qualifier: ROW_MAJOR  */
#line 1962 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.row_major = 1;
   }
#line 4630 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 155: /* interface_block_layout_qualifier: PACKED_TOK  */
#line 1967 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.packed = 1;
   }
#line 4639 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 156: /* interface_block_layout_qualifier: SHARED  */
#line 1972 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.shared = 1;
   }
#line 4648 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 157: /* subroutine_qualifier: SUBROUTINE  */
#line 1980 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.subroutine = 1;
   }
#line 4657 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 158: /* subroutine_qualifier: SUBROUTINE '(' subroutine_type_list ')'  */
#line 1985 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.subroutine = 1;
      (yyval.type_qualifier).subroutine_list = (yyvsp[-1].subroutine_list);
   }
#line 4667 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 159: /* subroutine_type_list: any_identifier  */
#line 1994 "src/compiler/glsl/glsl_parser.yy"
   {
        linear_ctx *ctx = state->linalloc;
        ast_declaration *decl = new(ctx)  ast_declaration((yyvsp[0].identifier), NULL, NULL);
        decl->set_location((yylsp[0]));

        (yyval.subroutine_list) = new(ctx) ast_subroutine_list();
        (yyval.subroutine_list)->declarations.push_tail(&decl->link);
   }
#line 4680 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 160: /* subroutine_type_list: subroutine_type_list ',' any_identifier  */
#line 2003 "src/compiler/glsl/glsl_parser.yy"
   {
        linear_ctx *ctx = state->linalloc;
        ast_declaration *decl = new(ctx)  ast_declaration((yyvsp[0].identifier), NULL, NULL);
        decl->set_location((yylsp[0]));

        (yyval.subroutine_list) = (yyvsp[-2].subroutine_list);
        (yyval.subroutine_list)->declarations.push_tail(&decl->link);
   }
#line 4693 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 161: /* interpolation_qualifier: SMOOTH  */
#line 2015 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.smooth = 1;
   }
#line 4702 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 162: /* interpolation_qualifier: FLAT  */
#line 2020 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.flat = 1;
   }
#line 4711 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 163: /* interpolation_qualifier: NOPERSPECTIVE  */
#line 2025 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.noperspective = 1;
   }
#line 4720 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 164: /* type_qualifier: INVARIANT  */
#line 2034 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.invariant = 1;
   }
#line 4729 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 165: /* type_qualifier: PRECISE  */
#line 2039 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.precise = 1;
   }
#line 4738 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 172: /* type_qualifier: precision_qualifier  */
#line 2050 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(&(yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).precision = (yyvsp[0].n);
   }
#line 4747 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 173: /* type_qualifier: PRECISE type_qualifier  */
#line 2068 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate \"precise\" qualifier");

      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).flags.q.precise = 1;
   }
#line 4759 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 174: /* type_qualifier: INVARIANT type_qualifier  */
#line 2076 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).flags.q.invariant)
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate \"invariant\" qualifier");

      if (!state->has_420pack_or_es31() && (yyvsp[0].type_qualifier).flags.q.precise)
         _mesa_glsl_error(&(yylsp[-1]), state,
                          "\"invariant\" must come after \"precise\"");

      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).flags.q.invariant = 1;

      /* GLSL ES 3.00 spec, section 4.6.1 "The Invariant Qualifier":
       *
       * "Only variables output from a shader can be candidates for invariance.
       * This includes user-defined output variables and the built-in output
       * variables. As only outputs can be declared as invariant, an invariant
       * output from one shader stage will still match an input of a subsequent
       * stage without the input being declared as invariant."
       *
       * On the desktop side, this text first appears in GLSL 4.20.
       */
      if (state->is_version(420, 300) && (yyval.type_qualifier).flags.q.in)
         _mesa_glsl_error(&(yylsp[-1]), state, "invariant qualifiers cannot be used with shader inputs");
   }
#line 4788 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 175: /* type_qualifier: interpolation_qualifier type_qualifier  */
#line 2101 "src/compiler/glsl/glsl_parser.yy"
   {
      /* Section 4.3 of the GLSL 1.40 specification states:
       * "...qualified with one of these interpolation qualifiers"
       *
       * GLSL 1.30 claims to allow "one or more", but insists that:
       * "These interpolation qualifiers may only precede the qualifiers in,
       *  centroid in, out, or centroid out in a declaration."
       *
       * ...which means that e.g. smooth can't precede smooth, so there can be
       * only one after all, and the 1.40 text is a clarification, not a change.
       */
      if ((yyvsp[0].type_qualifier).has_interpolation())
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate interpolation qualifier");

      if (!state->has_420pack_or_es31() &&
          ((yyvsp[0].type_qualifier).flags.q.precise || (yyvsp[0].type_qualifier).flags.q.invariant)) {
         _mesa_glsl_error(&(yylsp[-1]), state, "interpolation qualifiers must come "
                          "after \"precise\" or \"invariant\"");
      }

      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 4816 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 176: /* type_qualifier: layout_qualifier type_qualifier  */
#line 2125 "src/compiler/glsl/glsl_parser.yy"
   {
      /* In the absence of ARB_shading_language_420pack, layout qualifiers may
       * appear no later than auxiliary storage qualifiers. There is no
       * particularly clear spec language mandating this, but in all examples
       * the layout qualifier precedes the storage qualifier.
       *
       * We allow combinations of layout with interpolation, invariant or
       * precise qualifiers since these are useful in ARB_separate_shader_objects.
       * There is no clear spec guidance on this either.
       */
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(& (yylsp[-1]), state, (yyvsp[0].type_qualifier), false, (yyvsp[0].type_qualifier).has_layout());
   }
#line 4834 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 177: /* type_qualifier: subroutine_qualifier type_qualifier  */
#line 2139 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 4843 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 178: /* type_qualifier: auxiliary_storage_qualifier type_qualifier  */
#line 2144 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).has_auxiliary_storage()) {
         _mesa_glsl_error(&(yylsp[-1]), state,
                          "duplicate auxiliary storage qualifier (centroid or sample)");
      }

      if ((!state->has_420pack_or_es31() && !state->EXT_gpu_shader4_enable) &&
          ((yyvsp[0].type_qualifier).flags.q.precise || (yyvsp[0].type_qualifier).flags.q.invariant ||
           (yyvsp[0].type_qualifier).has_interpolation() || (yyvsp[0].type_qualifier).has_layout())) {
         _mesa_glsl_error(&(yylsp[-1]), state, "auxiliary storage qualifiers must come "
                          "just before storage qualifiers");
      }
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 4863 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 179: /* type_qualifier: storage_qualifier type_qualifier  */
#line 2160 "src/compiler/glsl/glsl_parser.yy"
   {
      /* Section 4.3 of the GLSL 1.20 specification states:
       * "Variable declarations may have a storage qualifier specified..."
       *  1.30 clarifies this to "may have one storage qualifier".
       *
       * GL_EXT_gpu_shader4 allows "varying out" in fragment shaders.
       */
      if ((yyvsp[0].type_qualifier).has_storage() &&
          (!state->EXT_gpu_shader4_enable ||
           state->stage != MESA_SHADER_FRAGMENT ||
           !(yyvsp[-1].type_qualifier).flags.q.varying || !(yyvsp[0].type_qualifier).flags.q.out))
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate storage qualifier");

      if (!state->has_420pack_or_es31() &&
          ((yyvsp[0].type_qualifier).flags.q.precise || (yyvsp[0].type_qualifier).flags.q.invariant || (yyvsp[0].type_qualifier).has_interpolation() ||
           (yyvsp[0].type_qualifier).has_layout() || (yyvsp[0].type_qualifier).has_auxiliary_storage())) {
         _mesa_glsl_error(&(yylsp[-1]), state, "storage qualifiers must come after "
                          "precise, invariant, interpolation, layout and auxiliary "
                          "storage qualifiers");
      }

      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 4892 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 180: /* type_qualifier: precision_qualifier type_qualifier  */
#line 2185 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type_qualifier).precision != ast_precision_none)
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate precision qualifier");

      if (!(state->has_420pack_or_es31()) &&
          (yyvsp[0].type_qualifier).flags.i != 0)
         _mesa_glsl_error(&(yylsp[-1]), state, "precision qualifiers must come last");

      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).precision = (yyvsp[-1].n);
   }
#line 4908 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 181: /* type_qualifier: memory_qualifier type_qualifier  */
#line 2197 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      (yyval.type_qualifier).merge_qualifier(&(yylsp[-1]), state, (yyvsp[0].type_qualifier), false);
   }
#line 4917 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 182: /* auxiliary_storage_qualifier: CENTROID  */
#line 2205 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.centroid = 1;
   }
#line 4926 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 183: /* auxiliary_storage_qualifier: SAMPLE  */
#line 2210 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.sample = 1;
   }
#line 4935 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 184: /* auxiliary_storage_qualifier: PATCH  */
#line 2215 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.patch = 1;
   }
#line 4944 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 185: /* auxiliary_storage_qualifier: PERPRIMITIVE  */
#line 2220 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.per_primitive = 1;
   }
#line 4953 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 186: /* storage_qualifier: CONST_TOK  */
#line 2227 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.constant = 1;
   }
#line 4962 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 187: /* storage_qualifier: ATTRIBUTE  */
#line 2232 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.attribute = 1;
   }
#line 4971 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 188: /* storage_qualifier: VARYING  */
#line 2237 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.varying = 1;
   }
#line 4980 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 189: /* storage_qualifier: IN_TOK  */
#line 2242 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
   }
#line 4989 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 190: /* storage_qualifier: OUT_TOK  */
#line 2247 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.out = 1;

      if (state->stage == MESA_SHADER_GEOMETRY &&
          state->has_explicit_attrib_stream()) {
         /* Section 4.3.8.2 (Output Layout Qualifiers) of the GLSL 4.00
          * spec says:
          *
          *     "If the block or variable is declared with the stream
          *     identifier, it is associated with the specified stream;
          *     otherwise, it is associated with the current default stream."
          */
          (yyval.type_qualifier).flags.q.stream = 1;
          (yyval.type_qualifier).flags.q.explicit_stream = 0;
          (yyval.type_qualifier).stream = state->out_qualifier->stream;
      }

      if (state->has_enhanced_layouts() && state->exts->ARB_transform_feedback3) {
          (yyval.type_qualifier).flags.q.xfb_buffer = 1;
          (yyval.type_qualifier).flags.q.explicit_xfb_buffer = 0;
          (yyval.type_qualifier).xfb_buffer = state->out_qualifier->xfb_buffer;
      }
   }
#line 5018 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 191: /* storage_qualifier: INOUT_TOK  */
#line 2272 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
      (yyval.type_qualifier).flags.q.out = 1;

      if (!state->has_framebuffer_fetch() ||
          !state->is_version(130, 300) ||
          state->stage != MESA_SHADER_FRAGMENT)
         _mesa_glsl_error(&(yylsp[0]), state, "A single interface variable cannot be "
                          "declared as both input and output");
   }
#line 5034 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 192: /* storage_qualifier: UNIFORM  */
#line 2284 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.uniform = 1;
   }
#line 5043 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 193: /* storage_qualifier: BUFFER  */
#line 2289 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.buffer = 1;
   }
#line 5052 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 194: /* storage_qualifier: SHARED  */
#line 2294 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.shared_storage = 1;
   }
#line 5061 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 195: /* storage_qualifier: TASKPAYLOAD  */
#line 2299 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.task_payload = 1;
   }
#line 5070 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 196: /* storage_qualifier: PIXEL_LOCAL  */
#line 2304 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.pixel_local_storage = GLSL_PIXEL_LOCAL_STORAGE_INOUT;
   }
#line 5079 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 197: /* storage_qualifier: PIXEL_LOCAL_IN  */
#line 2309 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.pixel_local_storage = GLSL_PIXEL_LOCAL_STORAGE_IN;
   }
#line 5088 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 198: /* storage_qualifier: PIXEL_LOCAL_OUT  */
#line 2314 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.pixel_local_storage = GLSL_PIXEL_LOCAL_STORAGE_OUT;
   }
#line 5097 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 199: /* memory_qualifier: COHERENT  */
#line 2322 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.coherent = 1;
   }
#line 5106 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 200: /* memory_qualifier: VOLATILE  */
#line 2327 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q._volatile = 1;
   }
#line 5115 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 201: /* memory_qualifier: RESTRICT  */
#line 2332 "src/compiler/glsl/glsl_parser.yy"
   {
      STATIC_ASSERT(sizeof((yyval.type_qualifier).flags.q) <= sizeof((yyval.type_qualifier).flags.i));
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.restrict_flag = 1;
   }
#line 5125 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 202: /* memory_qualifier: READONLY  */
#line 2338 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.read_only = 1;
   }
#line 5134 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 203: /* memory_qualifier: WRITEONLY  */
#line 2343 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.write_only = 1;
   }
#line 5143 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 204: /* array_specifier: '[' ']'  */
#line 2351 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.array_specifier) = new(ctx) ast_array_specifier((yylsp[-1]), new(ctx) ast_expression(
                                                  ast_unsized_array_dim, NULL,
                                                  NULL, NULL));
      (yyval.array_specifier)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 5155 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 205: /* array_specifier: '[' constant_expression ']'  */
#line 2359 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.array_specifier) = new(ctx) ast_array_specifier((yylsp[-2]), (yyvsp[-1].expression));
      (yyval.array_specifier)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 5165 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 206: /* array_specifier: array_specifier '[' ']'  */
#line 2365 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.array_specifier) = (yyvsp[-2].array_specifier);

      if (state->check_arrays_of_arrays_allowed(& (yylsp[-2]))) {
         (yyval.array_specifier)->add_dimension(new(ctx) ast_expression(ast_unsized_array_dim, NULL,
                                                   NULL, NULL));
      }
   }
#line 5179 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 207: /* array_specifier: array_specifier '[' constant_expression ']'  */
#line 2375 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.array_specifier) = (yyvsp[-3].array_specifier);

      if (state->check_arrays_of_arrays_allowed(& (yylsp[-3]))) {
         (yyval.array_specifier)->add_dimension((yyvsp[-1].expression));
      }
   }
#line 5191 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 209: /* type_specifier: type_specifier_nonarray array_specifier  */
#line 2387 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_specifier) = (yyvsp[-1].type_specifier);
      (yyval.type_specifier)->array_specifier = (yyvsp[0].array_specifier);
   }
#line 5200 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 210: /* type_specifier_nonarray: basic_type_specifier_nonarray  */
#line 2395 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[0].type));
      (yyval.type_specifier)->set_location((yylsp[0]));
   }
#line 5210 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 211: /* type_specifier_nonarray: struct_specifier  */
#line 2401 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[0].struct_specifier));
      (yyval.type_specifier)->set_location((yylsp[0]));
   }
#line 5220 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 212: /* type_specifier_nonarray: TYPE_IDENTIFIER  */
#line 2407 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.type_specifier) = new(ctx) ast_type_specifier((yyvsp[0].identifier));
      (yyval.type_specifier)->set_location((yylsp[0]));
   }
#line 5230 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 213: /* basic_type_specifier_nonarray: VOID_TOK  */
#line 2415 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.type) = &glsl_type_builtin_void; }
#line 5236 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 214: /* basic_type_specifier_nonarray: BASIC_TYPE_TOK  */
#line 2416 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.type) = (yyvsp[0].type); }
#line 5242 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 215: /* basic_type_specifier_nonarray: UNSIGNED BASIC_TYPE_TOK  */
#line 2418 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].type) == &glsl_type_builtin_int) {
         (yyval.type) = &glsl_type_builtin_uint;
      } else {
         _mesa_glsl_error(&(yylsp[-1]), state,
                          "\"unsigned\" is only allowed before \"int\"");
      }
   }
#line 5255 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 216: /* precision_qualifier: HIGHP  */
#line 2430 "src/compiler/glsl/glsl_parser.yy"
   {
      state->check_precision_qualifiers_allowed(&(yylsp[0]));
      (yyval.n) = ast_precision_high;
   }
#line 5264 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 217: /* precision_qualifier: MEDIUMP  */
#line 2435 "src/compiler/glsl/glsl_parser.yy"
   {
      state->check_precision_qualifiers_allowed(&(yylsp[0]));
      (yyval.n) = ast_precision_medium;
   }
#line 5273 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 218: /* precision_qualifier: LOWP  */
#line 2440 "src/compiler/glsl/glsl_parser.yy"
   {
      state->check_precision_qualifiers_allowed(&(yylsp[0]));
      (yyval.n) = ast_precision_low;
   }
#line 5282 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 219: /* struct_specifier: STRUCT any_identifier '{' struct_declaration_list '}'  */
#line 2448 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.struct_specifier) = new(ctx) ast_struct_specifier((yyvsp[-3].identifier), (yyvsp[-1].declarator_list));
      (yyval.struct_specifier)->set_location_range((yylsp[-3]), (yylsp[0]));
      state->symbols->add_type((yyvsp[-3].identifier), &glsl_type_builtin_void);
   }
#line 5293 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 220: /* struct_specifier: STRUCT '{' struct_declaration_list '}'  */
#line 2455 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;

      /* All anonymous structs have the same name. This simplifies matching of
       * globals whose type is an unnamed struct.
       *
       * It also avoids a memory leak when the same shader is compiled over and
       * over again.
       */
      (yyval.struct_specifier) = new(ctx) ast_struct_specifier("#anon_struct", (yyvsp[-1].declarator_list));

      (yyval.struct_specifier)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 5311 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 221: /* struct_declaration_list: struct_declaration  */
#line 2472 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.declarator_list) = (yyvsp[0].declarator_list);
      (yyvsp[0].declarator_list)->link.self_link();
   }
#line 5320 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 222: /* struct_declaration_list: struct_declaration_list struct_declaration  */
#line 2477 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.declarator_list) = (yyvsp[-1].declarator_list);
      (yyval.declarator_list)->link.insert_before(& (yyvsp[0].declarator_list)->link);
   }
#line 5329 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 223: /* struct_declaration: fully_specified_type struct_declarator_list ';'  */
#line 2485 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_fully_specified_type *const type = (yyvsp[-2].fully_specified_type);
      type->set_location((yylsp[-2]));

      if (state->has_bindless()) {
         ast_type_qualifier input_layout_mask;

         /* Allow to declare qualifiers for images. */
         input_layout_mask.flags.i = 0;
         input_layout_mask.flags.q.coherent = 1;
         input_layout_mask.flags.q._volatile = 1;
         input_layout_mask.flags.q.restrict_flag = 1;
         input_layout_mask.flags.q.read_only = 1;
         input_layout_mask.flags.q.write_only = 1;
         input_layout_mask.flags.q.explicit_image_format = 1;

         if ((type->qualifier.flags.i & ~input_layout_mask.flags.i) != 0) {
            _mesa_glsl_error(&(yylsp[-2]), state,
                             "only precision and image qualifiers may be "
                             "applied to structure members");
         }
      } else {
         if (type->qualifier.flags.i != 0)
            _mesa_glsl_error(&(yylsp[-2]), state,
                             "only precision qualifiers may be applied to "
                             "structure members");
      }

      (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
      (yyval.declarator_list)->set_location((yylsp[-1]));

      (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[-1].declaration)->link);
   }
#line 5368 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 224: /* struct_declarator_list: struct_declarator  */
#line 2523 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.declaration) = (yyvsp[0].declaration);
      (yyvsp[0].declaration)->link.self_link();
   }
#line 5377 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 225: /* struct_declarator_list: struct_declarator_list ',' struct_declarator  */
#line 2528 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.declaration) = (yyvsp[-2].declaration);
      (yyval.declaration)->link.insert_before(& (yyvsp[0].declaration)->link);
   }
#line 5386 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 226: /* struct_declarator: any_identifier  */
#line 2536 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.declaration) = new(ctx) ast_declaration((yyvsp[0].identifier), NULL, NULL);
      (yyval.declaration)->set_location((yylsp[0]));
   }
#line 5396 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 227: /* struct_declarator: any_identifier array_specifier  */
#line 2542 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.declaration) = new(ctx) ast_declaration((yyvsp[-1].identifier), (yyvsp[0].array_specifier), NULL);
      (yyval.declaration)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 5406 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 229: /* initializer: '{' initializer_list '}'  */
#line 2552 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[-1].expression);
   }
#line 5414 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 230: /* initializer: '{' initializer_list ',' '}'  */
#line 2556 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.expression) = (yyvsp[-2].expression);
   }
#line 5422 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 231: /* initializer_list: initializer  */
#line 2563 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.expression) = new(ctx) ast_aggregate_initializer();
      (yyval.expression)->set_location((yylsp[0]));
      (yyval.expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
   }
#line 5433 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 232: /* initializer_list: initializer_list ',' initializer  */
#line 2570 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyvsp[-2].expression)->expressions.push_tail(& (yyvsp[0].expression)->link);
   }
#line 5441 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 234: /* statement: compound_statement  */
#line 2582 "src/compiler/glsl/glsl_parser.yy"
                             { (yyval.node) = (ast_node *) (yyvsp[0].compound_statement); }
#line 5447 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 243: /* compound_statement: '{' '}'  */
#line 2598 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(true, NULL);
      (yyval.compound_statement)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 5457 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 244: /* $@2: %empty  */
#line 2604 "src/compiler/glsl/glsl_parser.yy"
   {
      state->symbols->push_scope();
   }
#line 5465 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 245: /* compound_statement: '{' $@2 statement_list '}'  */
#line 2608 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(true, (yyvsp[-1].node));
      (yyval.compound_statement)->set_location_range((yylsp[-3]), (yylsp[0]));
      state->symbols->pop_scope();
   }
#line 5476 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 246: /* statement_no_new_scope: compound_statement_no_new_scope  */
#line 2617 "src/compiler/glsl/glsl_parser.yy"
                                   { (yyval.node) = (ast_node *) (yyvsp[0].compound_statement); }
#line 5482 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 248: /* compound_statement_no_new_scope: '{' '}'  */
#line 2623 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(false, NULL);
      (yyval.compound_statement)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 5492 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 249: /* compound_statement_no_new_scope: '{' statement_list '}'  */
#line 2629 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.compound_statement) = new(ctx) ast_compound_statement(false, (yyvsp[-1].node));
      (yyval.compound_statement)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 5502 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 250: /* statement_list: statement  */
#line 2638 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].node) == NULL) {
         _mesa_glsl_error(& (yylsp[0]), state, "<nil> statement");
         assert((yyvsp[0].node) != NULL);
      }

      (yyval.node) = (yyvsp[0].node);
      (yyval.node)->link.self_link();
   }
#line 5516 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 251: /* statement_list: statement_list statement  */
#line 2648 "src/compiler/glsl/glsl_parser.yy"
   {
      if ((yyvsp[0].node) == NULL) {
         _mesa_glsl_error(& (yylsp[0]), state, "<nil> statement");
         assert((yyvsp[0].node) != NULL);
      }
      (yyval.node) = (yyvsp[-1].node);
      (yyval.node)->link.insert_before(& (yyvsp[0].node)->link);
   }
#line 5529 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 252: /* statement_list: statement_list extension_statement  */
#line 2657 "src/compiler/glsl/glsl_parser.yy"
   {
      if (!state->allow_extension_directive_midshader) {
         _mesa_glsl_error(& (yylsp[-1]), state,
                          "#extension directive is not allowed "
                          "in the middle of a shader");
         YYERROR;
      }
   }
#line 5542 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 253: /* expression_statement: ';'  */
#line 2669 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_expression_statement(NULL);
      (yyval.node)->set_location((yylsp[0]));
   }
#line 5552 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 254: /* expression_statement: expression ';'  */
#line 2675 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_expression_statement((yyvsp[-1].expression));
      (yyval.node)->set_location((yylsp[-1]));
   }
#line 5562 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 255: /* selection_statement: IF '(' expression ')' selection_rest_statement  */
#line 2684 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = new(state->linalloc) ast_selection_statement((yyvsp[-2].expression), (yyvsp[0].selection_rest_statement).then_statement,
                                                        (yyvsp[0].selection_rest_statement).else_statement);
      (yyval.node)->set_location_range((yylsp[-4]), (yylsp[0]));
   }
#line 5572 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 256: /* selection_rest_statement: statement ELSE statement  */
#line 2693 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.selection_rest_statement).then_statement = (yyvsp[-2].node);
      (yyval.selection_rest_statement).else_statement = (yyvsp[0].node);
   }
#line 5581 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 257: /* selection_rest_statement: statement  */
#line 2698 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.selection_rest_statement).then_statement = (yyvsp[0].node);
      (yyval.selection_rest_statement).else_statement = NULL;
   }
#line 5590 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 258: /* condition: expression  */
#line 2706 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = (ast_node *) (yyvsp[0].expression);
   }
#line 5598 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 259: /* condition: fully_specified_type any_identifier '=' initializer  */
#line 2710 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_declaration *decl = new(ctx) ast_declaration((yyvsp[-2].identifier), NULL, (yyvsp[0].expression));
      ast_declarator_list *declarator = new(ctx) ast_declarator_list((yyvsp[-3].fully_specified_type));
      decl->set_location_range((yylsp[-2]), (yylsp[0]));
      declarator->set_location((yylsp[-3]));

      declarator->declarations.push_tail(&decl->link);
      (yyval.node) = declarator;
   }
#line 5613 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 260: /* switch_statement: SWITCH '(' expression ')' switch_body  */
#line 2728 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = new(state->linalloc) ast_switch_statement((yyvsp[-2].expression), (yyvsp[0].switch_body));
      (yyval.node)->set_location_range((yylsp[-4]), (yylsp[0]));
   }
#line 5622 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 261: /* switch_body: '{' '}'  */
#line 2736 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.switch_body) = new(state->linalloc) ast_switch_body(NULL);
      (yyval.switch_body)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 5631 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 262: /* switch_body: '{' case_statement_list '}'  */
#line 2741 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.switch_body) = new(state->linalloc) ast_switch_body((yyvsp[-1].case_statement_list));
      (yyval.switch_body)->set_location_range((yylsp[-2]), (yylsp[0]));
   }
#line 5640 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 263: /* case_label: CASE expression ':'  */
#line 2749 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.case_label) = new(state->linalloc) ast_case_label((yyvsp[-1].expression));
      (yyval.case_label)->set_location((yylsp[-1]));
   }
#line 5649 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 264: /* case_label: DEFAULT ':'  */
#line 2754 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.case_label) = new(state->linalloc) ast_case_label(NULL);
      (yyval.case_label)->set_location((yylsp[0]));
   }
#line 5658 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 265: /* case_label_list: case_label  */
#line 2762 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_case_label_list *labels = new(state->linalloc) ast_case_label_list();

      labels->labels.push_tail(& (yyvsp[0].case_label)->link);
      (yyval.case_label_list) = labels;
      (yyval.case_label_list)->set_location((yylsp[0]));
   }
#line 5670 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 266: /* case_label_list: case_label_list case_label  */
#line 2770 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.case_label_list) = (yyvsp[-1].case_label_list);
      (yyval.case_label_list)->labels.push_tail(& (yyvsp[0].case_label)->link);
   }
#line 5679 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 267: /* case_statement: case_label_list statement  */
#line 2778 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_case_statement *stmts = new(state->linalloc) ast_case_statement((yyvsp[-1].case_label_list));
      stmts->set_location((yylsp[0]));

      stmts->stmts.push_tail(& (yyvsp[0].node)->link);
      (yyval.case_statement) = stmts;
   }
#line 5691 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 268: /* case_statement: case_statement statement  */
#line 2786 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.case_statement) = (yyvsp[-1].case_statement);
      (yyval.case_statement)->stmts.push_tail(& (yyvsp[0].node)->link);
   }
#line 5700 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 269: /* case_statement_list: case_statement  */
#line 2794 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_case_statement_list *cases= new(state->linalloc) ast_case_statement_list();
      cases->set_location((yylsp[0]));

      cases->cases.push_tail(& (yyvsp[0].case_statement)->link);
      (yyval.case_statement_list) = cases;
   }
#line 5712 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 270: /* case_statement_list: case_statement_list case_statement  */
#line 2802 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.case_statement_list) = (yyvsp[-1].case_statement_list);
      (yyval.case_statement_list)->cases.push_tail(& (yyvsp[0].case_statement)->link);
   }
#line 5721 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 271: /* iteration_statement: WHILE '(' condition ')' statement_no_new_scope  */
#line 2810 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_while,
                                            NULL, (yyvsp[-2].node), NULL, (yyvsp[0].node));
      (yyval.node)->set_location_range((yylsp[-4]), (yylsp[-1]));
   }
#line 5732 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 272: /* iteration_statement: DO statement_no_new_scope WHILE '(' expression ')' ';'  */
#line 2817 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_do_while,
                                            NULL, (yyvsp[-2].expression), NULL, (yyvsp[-5].node));
      (yyval.node)->set_location_range((yylsp[-6]), (yylsp[-1]));
   }
#line 5743 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 273: /* iteration_statement: FOR '(' for_init_statement for_rest_statement ')' statement_no_new_scope  */
#line 2824 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_iteration_statement(ast_iteration_statement::ast_for,
                                            (yyvsp[-3].node), (yyvsp[-2].for_rest_statement).cond, (yyvsp[-2].for_rest_statement).rest, (yyvsp[0].node));
      (yyval.node)->set_location_range((yylsp[-5]), (yylsp[0]));
   }
#line 5754 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 277: /* conditionopt: %empty  */
#line 2840 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = NULL;
   }
#line 5762 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 278: /* for_rest_statement: conditionopt ';'  */
#line 2847 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.for_rest_statement).cond = (yyvsp[-1].node);
      (yyval.for_rest_statement).rest = NULL;
   }
#line 5771 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 279: /* for_rest_statement: conditionopt ';' expression  */
#line 2852 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.for_rest_statement).cond = (yyvsp[-2].node);
      (yyval.for_rest_statement).rest = (yyvsp[0].expression);
   }
#line 5780 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 280: /* jump_statement: CONTINUE ';'  */
#line 2861 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_continue, NULL);
      (yyval.node)->set_location((yylsp[-1]));
   }
#line 5790 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 281: /* jump_statement: BREAK ';'  */
#line 2867 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_break, NULL);
      (yyval.node)->set_location((yylsp[-1]));
   }
#line 5800 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 282: /* jump_statement: RETURN ';'  */
#line 2873 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, NULL);
      (yyval.node)->set_location((yylsp[-1]));
   }
#line 5810 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 283: /* jump_statement: RETURN expression ';'  */
#line 2879 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_return, (yyvsp[-1].expression));
      (yyval.node)->set_location_range((yylsp[-2]), (yylsp[-1]));
   }
#line 5820 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 284: /* jump_statement: DISCARD ';'  */
#line 2885 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_jump_statement(ast_jump_statement::ast_discard, NULL);
      (yyval.node)->set_location((yylsp[-1]));
   }
#line 5830 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 285: /* demote_statement: DEMOTE ';'  */
#line 2894 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.node) = new(ctx) ast_demote_statement();
      (yyval.node)->set_location((yylsp[-1]));
   }
#line 5840 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 286: /* external_declaration: function_definition  */
#line 2902 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.node) = (yyvsp[0].function_definition); }
#line 5846 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 287: /* external_declaration: declaration  */
#line 2903 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.node) = (yyvsp[0].node); }
#line 5852 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 288: /* external_declaration: pragma_statement  */
#line 2904 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.node) = (yyvsp[0].node); }
#line 5858 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 289: /* external_declaration: layout_defaults  */
#line 2905 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.node) = (yyvsp[0].node); }
#line 5864 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 290: /* external_declaration: ';'  */
#line 2906 "src/compiler/glsl/glsl_parser.yy"
                            { (yyval.node) = NULL; }
#line 5870 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 291: /* function_definition: function_prototype compound_statement_no_new_scope  */
#line 2911 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      (yyval.function_definition) = new(ctx) ast_function_definition();
      (yyval.function_definition)->set_location_range((yylsp[-1]), (yylsp[0]));
      (yyval.function_definition)->prototype = (yyvsp[-1].function);
      (yyval.function_definition)->body = (yyvsp[0].compound_statement);

      state->symbols->pop_scope();
   }
#line 5884 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 292: /* interface_block: basic_interface_block  */
#line 2925 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = (yyvsp[0].interface_block);
   }
#line 5892 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 293: /* interface_block: layout_qualifier interface_block  */
#line 2929 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_interface_block *block = (ast_interface_block *) (yyvsp[0].node);

      if (!(yyvsp[-1].type_qualifier).merge_qualifier(& (yylsp[-1]), state, block->layout, false,
                              block->layout.has_layout())) {
         YYERROR;
      }

      block->layout = (yyvsp[-1].type_qualifier);

      (yyval.node) = block;
   }
#line 5909 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 294: /* interface_block: memory_qualifier interface_block  */
#line 2942 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_interface_block *block = (ast_interface_block *)(yyvsp[0].node);

      if (!block->default_layout.flags.q.buffer) {
            _mesa_glsl_error(& (yylsp[-1]), state,
                             "memory qualifiers can only be used in the "
                             "declaration of shader storage blocks");
      }
      if (!(yyvsp[-1].type_qualifier).merge_qualifier(& (yylsp[-1]), state, block->layout, false)) {
         YYERROR;
      }
      block->layout = (yyvsp[-1].type_qualifier);
      (yyval.node) = block;
   }
#line 5928 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 295: /* basic_interface_block: interface_qualifier NEW_IDENTIFIER '{' member_list '}' instance_name_opt ';'  */
#line 2960 "src/compiler/glsl/glsl_parser.yy"
   {
      ast_interface_block *const block = (yyvsp[-1].interface_block);

      if ((yyvsp[-6].type_qualifier).flags.q.uniform) {
         block->default_layout = *state->default_uniform_qualifier;
      } else if ((yyvsp[-6].type_qualifier).flags.q.buffer) {
         block->default_layout = *state->default_shader_storage_qualifier;
      }
      block->block_name = (yyvsp[-5].identifier);
      block->declarations.push_degenerate_list_at_head(& (yyvsp[-3].declarator_list)->link);

      _mesa_ast_process_interface_block(& (yylsp[-6]), state, block, (yyvsp[-6].type_qualifier));

      (yyval.interface_block) = block;
   }
#line 5948 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 296: /* interface_qualifier: IN_TOK  */
#line 2979 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.in = 1;
   }
#line 5957 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 297: /* interface_qualifier: OUT_TOK  */
#line 2984 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.out = 1;
   }
#line 5966 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 298: /* interface_qualifier: UNIFORM  */
#line 2989 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.uniform = 1;
   }
#line 5975 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 299: /* interface_qualifier: BUFFER  */
#line 2994 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.buffer = 1;
   }
#line 5984 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 300: /* interface_qualifier: PIXEL_LOCAL  */
#line 2999 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.pixel_local_storage = GLSL_PIXEL_LOCAL_STORAGE_INOUT;
   }
#line 5993 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 301: /* interface_qualifier: PIXEL_LOCAL_IN  */
#line 3004 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.pixel_local_storage = GLSL_PIXEL_LOCAL_STORAGE_IN;
   }
#line 6002 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 302: /* interface_qualifier: PIXEL_LOCAL_OUT  */
#line 3009 "src/compiler/glsl/glsl_parser.yy"
   {
      memset(& (yyval.type_qualifier), 0, sizeof((yyval.type_qualifier)));
      (yyval.type_qualifier).flags.q.pixel_local_storage = GLSL_PIXEL_LOCAL_STORAGE_OUT;
   }
#line 6011 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 303: /* interface_qualifier: auxiliary_storage_qualifier interface_qualifier  */
#line 3014 "src/compiler/glsl/glsl_parser.yy"
   {
      if (!(yyvsp[-1].type_qualifier).flags.q.patch && !(yyvsp[-1].type_qualifier).flags.q.per_primitive) {
         _mesa_glsl_error(&(yylsp[-1]), state, "invalid interface qualifier");
      }
      if ((yyvsp[0].type_qualifier).has_auxiliary_storage()) {
         _mesa_glsl_error(&(yylsp[-1]), state, "duplicate auxiliary storage qualifier");
      }
      (yyval.type_qualifier) = (yyvsp[0].type_qualifier);
      (yyval.type_qualifier).flags.q.patch = (yyvsp[-1].type_qualifier).flags.q.patch;
      (yyval.type_qualifier).flags.q.per_primitive = (yyvsp[-1].type_qualifier).flags.q.per_primitive;
   }
#line 6027 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 304: /* instance_name_opt: %empty  */
#line 3029 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.interface_block) = new(state->linalloc) ast_interface_block(NULL, NULL);
   }
#line 6035 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 305: /* instance_name_opt: NEW_IDENTIFIER  */
#line 3033 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.interface_block) = new(state->linalloc) ast_interface_block((yyvsp[0].identifier), NULL);
      (yyval.interface_block)->set_location((yylsp[0]));
   }
#line 6044 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 306: /* instance_name_opt: NEW_IDENTIFIER array_specifier  */
#line 3038 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.interface_block) = new(state->linalloc) ast_interface_block((yyvsp[-1].identifier), (yyvsp[0].array_specifier));
      (yyval.interface_block)->set_location_range((yylsp[-1]), (yylsp[0]));
   }
#line 6053 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 307: /* member_list: member_declaration  */
#line 3046 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.declarator_list) = (yyvsp[0].declarator_list);
      (yyvsp[0].declarator_list)->link.self_link();
   }
#line 6062 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 308: /* member_list: member_declaration member_list  */
#line 3051 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.declarator_list) = (yyvsp[-1].declarator_list);
      (yyvsp[0].declarator_list)->link.insert_before(& (yyval.declarator_list)->link);
   }
#line 6071 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 309: /* member_declaration: fully_specified_type struct_declarator_list ';'  */
#line 3059 "src/compiler/glsl/glsl_parser.yy"
   {
      linear_ctx *ctx = state->linalloc;
      ast_fully_specified_type *type = (yyvsp[-2].fully_specified_type);
      type->set_location((yylsp[-2]));

      if (type->qualifier.flags.q.attribute) {
         _mesa_glsl_error(& (yylsp[-2]), state,
                          "keyword 'attribute' cannot be used with "
                          "interface block member");
      } else if (type->qualifier.flags.q.varying) {
         _mesa_glsl_error(& (yylsp[-2]), state,
                          "keyword 'varying' cannot be used with "
                          "interface block member");
      }

      (yyval.declarator_list) = new(ctx) ast_declarator_list(type);
      (yyval.declarator_list)->set_location((yylsp[-1]));

      (yyval.declarator_list)->declarations.push_degenerate_list_at_head(& (yyvsp[-1].declaration)->link);
   }
#line 6096 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 310: /* layout_uniform_defaults: layout_qualifier layout_uniform_defaults  */
#line 3083 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[-1]), state, (yyvsp[0].type_qualifier), false, true)) {
         YYERROR;
      }
   }
#line 6107 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 312: /* layout_buffer_defaults: layout_qualifier layout_buffer_defaults  */
#line 3094 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[-1]), state, (yyvsp[0].type_qualifier), false, true)) {
         YYERROR;
      }
   }
#line 6118 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 314: /* layout_in_defaults: layout_qualifier layout_in_defaults  */
#line 3105 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[-1]), state, (yyvsp[0].type_qualifier), false, true)) {
         YYERROR;
      }
      if (!(yyval.type_qualifier).validate_in_qualifier(& (yylsp[-1]), state)) {
         YYERROR;
      }
   }
#line 6132 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 315: /* layout_in_defaults: layout_qualifier IN_TOK ';'  */
#line 3115 "src/compiler/glsl/glsl_parser.yy"
   {
      if (!(yyvsp[-2].type_qualifier).validate_in_qualifier(& (yylsp[-2]), state)) {
         YYERROR;
      }
   }
#line 6142 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 316: /* layout_out_defaults: layout_qualifier layout_out_defaults  */
#line 3124 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.type_qualifier) = (yyvsp[-1].type_qualifier);
      if (!(yyval.type_qualifier).merge_qualifier(& (yylsp[-1]), state, (yyvsp[0].type_qualifier), false, true)) {
         YYERROR;
      }
      if (!(yyval.type_qualifier).validate_out_qualifier(& (yylsp[-1]), state)) {
         YYERROR;
      }
   }
#line 6156 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 317: /* layout_out_defaults: layout_qualifier OUT_TOK ';'  */
#line 3134 "src/compiler/glsl/glsl_parser.yy"
   {
      if (!(yyvsp[-2].type_qualifier).validate_out_qualifier(& (yylsp[-2]), state)) {
         YYERROR;
      }
   }
#line 6166 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 318: /* layout_defaults: layout_uniform_defaults  */
#line 3143 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = NULL;
      if (!state->default_uniform_qualifier->
             merge_qualifier(& (yylsp[0]), state, (yyvsp[0].type_qualifier), false)) {
         YYERROR;
      }
      if (!state->default_uniform_qualifier->
             push_to_global(& (yylsp[0]), state)) {
         YYERROR;
      }
   }
#line 6182 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 319: /* layout_defaults: layout_buffer_defaults  */
#line 3155 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = NULL;
      if (!state->default_shader_storage_qualifier->
             merge_qualifier(& (yylsp[0]), state, (yyvsp[0].type_qualifier), false)) {
         YYERROR;
      }
      if (!state->default_shader_storage_qualifier->
             push_to_global(& (yylsp[0]), state)) {
         YYERROR;
      }

      /* From the GLSL 4.50 spec, section 4.4.5:
       *
       *     "It is a compile-time error to specify the binding identifier for
       *     the global scope or for block member declarations."
       */
      if (state->default_shader_storage_qualifier->flags.q.explicit_binding) {
         _mesa_glsl_error(& (yylsp[0]), state,
                          "binding qualifier cannot be set for default layout");
      }
   }
#line 6208 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 320: /* layout_defaults: layout_in_defaults  */
#line 3177 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = NULL;
      if (!(yyvsp[0].type_qualifier).merge_into_in_qualifier(& (yylsp[0]), state, (yyval.node))) {
         YYERROR;
      }
      if (!state->in_qualifier->push_to_global(& (yylsp[0]), state)) {
         YYERROR;
      }
   }
#line 6222 "src/compiler/glsl/glsl_parser.cpp"
    break;

  case 321: /* layout_defaults: layout_out_defaults  */
#line 3187 "src/compiler/glsl/glsl_parser.yy"
   {
      (yyval.node) = NULL;
      if (!(yyvsp[0].type_qualifier).merge_into_out_qualifier(& (yylsp[0]), state, (yyval.node))) {
         YYERROR;
      }
      if (!state->out_qualifier->push_to_global(& (yylsp[0]), state)) {
         YYERROR;
      }

      (void)yynerrs;
   }
#line 6238 "src/compiler/glsl/glsl_parser.cpp"
    break;


#line 6242 "src/compiler/glsl/glsl_parser.cpp"

      default: break;
    }
  /* User semantic actions sometimes alter yychar, and that requires
     that yytoken be updated with the new translation.  We take the
     approach of translating immediately before every use of yytoken.
     One alternative is translating here after every semantic action,
     but that translation would be missed if the semantic action invokes
     YYABORT, YYACCEPT, or YYERROR immediately after altering yychar or
     if it invokes YYBACKUP.  In the case of YYABORT or YYACCEPT, an
     incorrect destructor might then be invoked immediately.  In the
     case of YYERROR or YYBACKUP, subsequent parser actions might lead
     to an incorrect destructor call or verbose syntax error message
     before the lookahead is translated.  */
  YY_SYMBOL_PRINT ("-> $$ =", YY_CAST (yysymbol_kind_t, yyr1[yyn]), &yyval, &yyloc);

  YYPOPSTACK (yylen);
  yylen = 0;

  *++yyvsp = yyval;
  *++yylsp = yyloc;

  /* Now 'shift' the result of the reduction.  Determine what state
     that goes to, based on the state we popped back to and the rule
     number reduced by.  */
  {
    const int yylhs = yyr1[yyn] - YYNTOKENS;
    const int yyi = yypgoto[yylhs] + *yyssp;
    yystate = (0 <= yyi && yyi <= YYLAST && yycheck[yyi] == *yyssp
               ? yytable[yyi]
               : yydefgoto[yylhs]);
  }

  goto yynewstate;


/*--------------------------------------.
| yyerrlab -- here on detecting error.  |
`--------------------------------------*/
yyerrlab:
  /* Make sure we have latest lookahead translation.  See comments at
     user semantic actions for why this is necessary.  */
  yytoken = yychar == YYEMPTY ? YYSYMBOL_YYEMPTY : YYTRANSLATE (yychar);
  /* If not already recovering from an error, report this error.  */
  if (!yyerrstatus)
    {
      ++yynerrs;
      {
        yypcontext_t yyctx
          = {yyssp, yytoken, &yylloc};
        char const *yymsgp = YY_("syntax error");
        int yysyntax_error_status;
        yysyntax_error_status = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
        if (yysyntax_error_status == 0)
          yymsgp = yymsg;
        else if (yysyntax_error_status == -1)
          {
            if (yymsg != yymsgbuf)
              YYSTACK_FREE (yymsg);
            yymsg = YY_CAST (char *,
                             YYSTACK_ALLOC (YY_CAST (YYSIZE_T, yymsg_alloc)));
            if (yymsg)
              {
                yysyntax_error_status
                  = yysyntax_error (&yymsg_alloc, &yymsg, &yyctx);
                yymsgp = yymsg;
              }
            else
              {
                yymsg = yymsgbuf;
                yymsg_alloc = sizeof yymsgbuf;
                yysyntax_error_status = YYENOMEM;
              }
          }
        yyerror (&yylloc, state, yymsgp);
        if (yysyntax_error_status == YYENOMEM)
          YYNOMEM;
      }
    }

  yyerror_range[1] = yylloc;
  if (yyerrstatus == 3)
    {
      /* If just tried and failed to reuse lookahead token after an
         error, discard it.  */

      if (yychar <= YYEOF)
        {
          /* Return failure if at end of input.  */
          if (yychar == YYEOF)
            YYABORT;
        }
      else
        {
          yydestruct ("Error: discarding",
                      yytoken, &yylval, &yylloc, state);
          yychar = YYEMPTY;
        }
    }

  /* Else will try to reuse lookahead token after shifting the error
     token.  */
  goto yyerrlab1;


/*---------------------------------------------------.
| yyerrorlab -- error raised explicitly by YYERROR.  |
`---------------------------------------------------*/
yyerrorlab:
  /* Pacify compilers when the user code never invokes YYERROR and the
     label yyerrorlab therefore never appears in user code.  */
  if (0)
    YYERROR;
  ++yynerrs;

  /* Do not reclaim the symbols of the rule whose action triggered
     this YYERROR.  */
  YYPOPSTACK (yylen);
  yylen = 0;
  YY_STACK_PRINT (yyss, yyssp);
  yystate = *yyssp;
  goto yyerrlab1;


/*-------------------------------------------------------------.
| yyerrlab1 -- common code for both syntax error and YYERROR.  |
`-------------------------------------------------------------*/
yyerrlab1:
  yyerrstatus = 3;      /* Each real token shifted decrements this.  */

  /* Pop stack until we find a state that shifts the error token.  */
  for (;;)
    {
      yyn = yypact[yystate];
      if (!yypact_value_is_default (yyn))
        {
          yyn += YYSYMBOL_YYerror;
          if (0 <= yyn && yyn <= YYLAST && yycheck[yyn] == YYSYMBOL_YYerror)
            {
              yyn = yytable[yyn];
              if (0 < yyn)
                break;
            }
        }

      /* Pop the current state because it cannot handle the error token.  */
      if (yyssp == yyss)
        YYABORT;

      yyerror_range[1] = *yylsp;
      yydestruct ("Error: popping",
                  YY_ACCESSING_SYMBOL (yystate), yyvsp, yylsp, state);
      YYPOPSTACK (1);
      yystate = *yyssp;
      YY_STACK_PRINT (yyss, yyssp);
    }

  YY_IGNORE_MAYBE_UNINITIALIZED_BEGIN
  *++yyvsp = yylval;
  YY_IGNORE_MAYBE_UNINITIALIZED_END

  yyerror_range[2] = yylloc;
  ++yylsp;
  YYLLOC_DEFAULT (*yylsp, yyerror_range, 2);

  /* Shift the error token.  */
  YY_SYMBOL_PRINT ("Shifting", YY_ACCESSING_SYMBOL (yyn), yyvsp, yylsp);

  yystate = yyn;
  goto yynewstate;


/*-------------------------------------.
| yyacceptlab -- YYACCEPT comes here.  |
`-------------------------------------*/
yyacceptlab:
  yyresult = 0;
  goto yyreturnlab;


/*-----------------------------------.
| yyabortlab -- YYABORT comes here.  |
`-----------------------------------*/
yyabortlab:
  yyresult = 1;
  goto yyreturnlab;


/*-----------------------------------------------------------.
| yyexhaustedlab -- YYNOMEM (memory exhaustion) comes here.  |
`-----------------------------------------------------------*/
yyexhaustedlab:
  yyerror (&yylloc, state, YY_("memory exhausted"));
  yyresult = 2;
  goto yyreturnlab;


/*----------------------------------------------------------.
| yyreturnlab -- parsing is finished, clean up and return.  |
`----------------------------------------------------------*/
yyreturnlab:
  if (yychar != YYEMPTY)
    {
      /* Make sure we have latest lookahead translation.  See comments at
         user semantic actions for why this is necessary.  */
      yytoken = YYTRANSLATE (yychar);
      yydestruct ("Cleanup: discarding lookahead",
                  yytoken, &yylval, &yylloc, state);
    }
  /* Do not reclaim the symbols of the rule whose action triggered
     this YYABORT or YYACCEPT.  */
  YYPOPSTACK (yylen);
  YY_STACK_PRINT (yyss, yyssp);
  while (yyssp != yyss)
    {
      yydestruct ("Cleanup: popping",
                  YY_ACCESSING_SYMBOL (+*yyssp), yyvsp, yylsp, state);
      YYPOPSTACK (1);
    }
#ifndef yyoverflow
  if (yyss != yyssa)
    YYSTACK_FREE (yyss);
#endif
  if (yymsg != yymsgbuf)
    YYSTACK_FREE (yymsg);
  return yyresult;
}

