/* A Bison parser, made by GNU Bison 3.8.2.  */

/* Bison interface for Yacc-like parsers in C

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

/* DO NOT RELY ON FEATURES THAT ARE NOT DOCUMENTED in the manual,
   especially those whose name start with YY_ or yy_.  They are
   private implementation details that can be changed or removed.  */

#ifndef YY__MESA_GLSL_SRC_COMPILER_GLSL_GLSL_PARSER_H_INCLUDED
# define YY__MESA_GLSL_SRC_COMPILER_GLSL_GLSL_PARSER_H_INCLUDED
/* Debug traces.  */
#ifndef YYDEBUG
# define YYDEBUG 0
#endif
#if YYDEBUG
extern int _mesa_glsl_debug;
#endif

/* Token kinds.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
  enum yytokentype
  {
    YYEMPTY = -2,
    YYEOF = 0,                     /* "end of file"  */
    YYerror = 256,                 /* error  */
    YYUNDEF = 257,                 /* "invalid token"  */
    ATTRIBUTE = 258,               /* ATTRIBUTE  */
    CONST_TOK = 259,               /* CONST_TOK  */
    BASIC_TYPE_TOK = 260,          /* BASIC_TYPE_TOK  */
    BREAK = 261,                   /* BREAK  */
    BUFFER = 262,                  /* BUFFER  */
    CONTINUE = 263,                /* CONTINUE  */
    DO = 264,                      /* DO  */
    ELSE = 265,                    /* ELSE  */
    FOR = 266,                     /* FOR  */
    IF = 267,                      /* IF  */
    DEMOTE = 268,                  /* DEMOTE  */
    DISCARD = 269,                 /* DISCARD  */
    RETURN = 270,                  /* RETURN  */
    SWITCH = 271,                  /* SWITCH  */
    CASE = 272,                    /* CASE  */
    DEFAULT = 273,                 /* DEFAULT  */
    CENTROID = 274,                /* CENTROID  */
    IN_TOK = 275,                  /* IN_TOK  */
    OUT_TOK = 276,                 /* OUT_TOK  */
    INOUT_TOK = 277,               /* INOUT_TOK  */
    UNIFORM = 278,                 /* UNIFORM  */
    VARYING = 279,                 /* VARYING  */
    SAMPLE = 280,                  /* SAMPLE  */
    NOPERSPECTIVE = 281,           /* NOPERSPECTIVE  */
    FLAT = 282,                    /* FLAT  */
    SMOOTH = 283,                  /* SMOOTH  */
    PIXEL_LOCAL = 284,             /* PIXEL_LOCAL  */
    PIXEL_LOCAL_IN = 285,          /* PIXEL_LOCAL_IN  */
    PIXEL_LOCAL_OUT = 286,         /* PIXEL_LOCAL_OUT  */
    IMAGE1DSHADOW = 287,           /* IMAGE1DSHADOW  */
    IMAGE2DSHADOW = 288,           /* IMAGE2DSHADOW  */
    IMAGE1DARRAYSHADOW = 289,      /* IMAGE1DARRAYSHADOW  */
    IMAGE2DARRAYSHADOW = 290,      /* IMAGE2DARRAYSHADOW  */
    COHERENT = 291,                /* COHERENT  */
    VOLATILE = 292,                /* VOLATILE  */
    RESTRICT = 293,                /* RESTRICT  */
    READONLY = 294,                /* READONLY  */
    WRITEONLY = 295,               /* WRITEONLY  */
    SHARED = 296,                  /* SHARED  */
    TASKPAYLOAD = 297,             /* TASKPAYLOAD  */
    PERPRIMITIVE = 298,            /* PERPRIMITIVE  */
    STRUCT = 299,                  /* STRUCT  */
    VOID_TOK = 300,                /* VOID_TOK  */
    WHILE = 301,                   /* WHILE  */
    IDENTIFIER = 302,              /* IDENTIFIER  */
    TYPE_IDENTIFIER = 303,         /* TYPE_IDENTIFIER  */
    NEW_IDENTIFIER = 304,          /* NEW_IDENTIFIER  */
    FLOATCONSTANT = 305,           /* FLOATCONSTANT  */
    FLOAT16CONSTANT = 306,         /* FLOAT16CONSTANT  */
    DOUBLECONSTANT = 307,          /* DOUBLECONSTANT  */
    INTCONSTANT = 308,             /* INTCONSTANT  */
    UINTCONSTANT = 309,            /* UINTCONSTANT  */
    BOOLCONSTANT = 310,            /* BOOLCONSTANT  */
    INT64CONSTANT = 311,           /* INT64CONSTANT  */
    UINT64CONSTANT = 312,          /* UINT64CONSTANT  */
    FIELD_SELECTION = 313,         /* FIELD_SELECTION  */
    LEFT_OP = 314,                 /* LEFT_OP  */
    RIGHT_OP = 315,                /* RIGHT_OP  */
    INC_OP = 316,                  /* INC_OP  */
    DEC_OP = 317,                  /* DEC_OP  */
    LE_OP = 318,                   /* LE_OP  */
    GE_OP = 319,                   /* GE_OP  */
    EQ_OP = 320,                   /* EQ_OP  */
    NE_OP = 321,                   /* NE_OP  */
    AND_OP = 322,                  /* AND_OP  */
    OR_OP = 323,                   /* OR_OP  */
    XOR_OP = 324,                  /* XOR_OP  */
    MUL_ASSIGN = 325,              /* MUL_ASSIGN  */
    DIV_ASSIGN = 326,              /* DIV_ASSIGN  */
    ADD_ASSIGN = 327,              /* ADD_ASSIGN  */
    MOD_ASSIGN = 328,              /* MOD_ASSIGN  */
    LEFT_ASSIGN = 329,             /* LEFT_ASSIGN  */
    RIGHT_ASSIGN = 330,            /* RIGHT_ASSIGN  */
    AND_ASSIGN = 331,              /* AND_ASSIGN  */
    XOR_ASSIGN = 332,              /* XOR_ASSIGN  */
    OR_ASSIGN = 333,               /* OR_ASSIGN  */
    SUB_ASSIGN = 334,              /* SUB_ASSIGN  */
    INVARIANT = 335,               /* INVARIANT  */
    PRECISE = 336,                 /* PRECISE  */
    LOWP = 337,                    /* LOWP  */
    MEDIUMP = 338,                 /* MEDIUMP  */
    HIGHP = 339,                   /* HIGHP  */
    SUPERP = 340,                  /* SUPERP  */
    PRECISION = 341,               /* PRECISION  */
    VERSION_TOK = 342,             /* VERSION_TOK  */
    EXTENSION = 343,               /* EXTENSION  */
    LINE = 344,                    /* LINE  */
    COLON = 345,                   /* COLON  */
    EOL = 346,                     /* EOL  */
    INTERFACE_TOK = 347,           /* INTERFACE_TOK  */
    OUTPUT = 348,                  /* OUTPUT  */
    PRAGMA_DEBUG_ON = 349,         /* PRAGMA_DEBUG_ON  */
    PRAGMA_DEBUG_OFF = 350,        /* PRAGMA_DEBUG_OFF  */
    PRAGMA_OPTIMIZE_ON = 351,      /* PRAGMA_OPTIMIZE_ON  */
    PRAGMA_OPTIMIZE_OFF = 352,     /* PRAGMA_OPTIMIZE_OFF  */
    PRAGMA_WARNING_ON = 353,       /* PRAGMA_WARNING_ON  */
    PRAGMA_WARNING_OFF = 354,      /* PRAGMA_WARNING_OFF  */
    PRAGMA_INVARIANT_ALL = 355,    /* PRAGMA_INVARIANT_ALL  */
    LAYOUT_TOK = 356,              /* LAYOUT_TOK  */
    DOT_TOK = 357,                 /* DOT_TOK  */
    ASM = 358,                     /* ASM  */
    CLASS = 359,                   /* CLASS  */
    UNION = 360,                   /* UNION  */
    ENUM = 361,                    /* ENUM  */
    TYPEDEF = 362,                 /* TYPEDEF  */
    TEMPLATE = 363,                /* TEMPLATE  */
    THIS = 364,                    /* THIS  */
    PACKED_TOK = 365,              /* PACKED_TOK  */
    GOTO = 366,                    /* GOTO  */
    INLINE_TOK = 367,              /* INLINE_TOK  */
    NOINLINE = 368,                /* NOINLINE  */
    PUBLIC_TOK = 369,              /* PUBLIC_TOK  */
    STATIC = 370,                  /* STATIC  */
    EXTERN = 371,                  /* EXTERN  */
    EXTERNAL = 372,                /* EXTERNAL  */
    LONG_TOK = 373,                /* LONG_TOK  */
    SHORT_TOK = 374,               /* SHORT_TOK  */
    HALF = 375,                    /* HALF  */
    FIXED_TOK = 376,               /* FIXED_TOK  */
    UNSIGNED = 377,                /* UNSIGNED  */
    INPUT_TOK = 378,               /* INPUT_TOK  */
    HVEC2 = 379,                   /* HVEC2  */
    HVEC3 = 380,                   /* HVEC3  */
    HVEC4 = 381,                   /* HVEC4  */
    FVEC2 = 382,                   /* FVEC2  */
    FVEC3 = 383,                   /* FVEC3  */
    FVEC4 = 384,                   /* FVEC4  */
    SAMPLER3DRECT = 385,           /* SAMPLER3DRECT  */
    SIZEOF = 386,                  /* SIZEOF  */
    CAST = 387,                    /* CAST  */
    NAMESPACE = 388,               /* NAMESPACE  */
    USING = 389,                   /* USING  */
    RESOURCE = 390,                /* RESOURCE  */
    PATCH = 391,                   /* PATCH  */
    SUBROUTINE = 392,              /* SUBROUTINE  */
    ERROR_TOK = 393,               /* ERROR_TOK  */
    COMMON = 394,                  /* COMMON  */
    PARTITION = 395,               /* PARTITION  */
    ACTIVE = 396,                  /* ACTIVE  */
    FILTER = 397,                  /* FILTER  */
    ROW_MAJOR = 398,               /* ROW_MAJOR  */
    THEN = 399                     /* THEN  */
  };
  typedef enum yytokentype yytoken_kind_t;
#endif

/* Value type.  */
#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
union YYSTYPE
{
#line 102 "src/compiler/glsl/glsl_parser.yy"

   int n;
   int64_t n64;
   float real;
   double dreal;
   const char *identifier;

   struct ast_type_qualifier type_qualifier;

   ast_node *node;
   ast_type_specifier *type_specifier;
   ast_array_specifier *array_specifier;
   ast_fully_specified_type *fully_specified_type;
   ast_function *function;
   ast_parameter_declarator *parameter_declarator;
   ast_function_definition *function_definition;
   ast_compound_statement *compound_statement;
   ast_expression *expression;
   ast_declarator_list *declarator_list;
   ast_struct_specifier *struct_specifier;
   ast_declaration *declaration;
   ast_switch_body *switch_body;
   ast_case_label *case_label;
   ast_case_label_list *case_label_list;
   ast_case_statement *case_statement;
   ast_case_statement_list *case_statement_list;
   ast_interface_block *interface_block;
   ast_subroutine_list *subroutine_list;
   struct {
      ast_node *cond;
      ast_expression *rest;
   } for_rest_statement;

   struct {
      ast_node *then_statement;
      ast_node *else_statement;
   } selection_rest_statement;

   const glsl_type *type;

#line 249 "src/compiler/glsl/glsl_parser.h"

};
typedef union YYSTYPE YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define YYSTYPE_IS_DECLARED 1
#endif

/* Location type.  */
#if ! defined YYLTYPE && ! defined YYLTYPE_IS_DECLARED
typedef struct YYLTYPE YYLTYPE;
struct YYLTYPE
{
  int first_line;
  int first_column;
  int last_line;
  int last_column;
};
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1
#endif




int _mesa_glsl_parse (struct _mesa_glsl_parse_state *state);


#endif /* !YY__MESA_GLSL_SRC_COMPILER_GLSL_GLSL_PARSER_H_INCLUDED  */
