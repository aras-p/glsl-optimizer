/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
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

#include "glheader.h"
#include "context.h"
#include "hash.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "nvprogram.h"
#include "nvvertparse.h"
#include "nvvertprog.h"

#include "arbvertparse.h"

/**
 * Overview:
 *
 * This is a simple top-down predictive parser. In a nutshell, there are two key
 * elements to a predictive parser. First, is the 'look ahead' symbol. This is simply 
 * the next token in the input stream. The other component is a stack of symbols. 
 *
 * Given the grammar, we can derive a look ahead table. This is just a 2D array, 
 * where one axis is the non-terminal on top of the stack and the other axis is 
 * the look ahead. Each entry in the array is the production to apply when the pair
 * of stack symbol & look ahead is encountered. If no production is listed at 
 * a given entry in the table, a parse error occurs if this combination
 * is seen.
 *
 * Since we want to drive the parsing from a couple of huge tables, we have to
 * save some information later on for processing (e.g. for code generation).
 * For this, we explicitly recreate the parse tree representing the derivation. 
 * We can then make several passes over the parse tree to perform all additional 
 * processing, and we can be sure the the parse is valid.
 *   
 * The stack is initialized by pushing the start symbol onto it. The look ahead
 * symbol is initially the first token in the program string. 
 *
 * If there is a non-terminal symbol on top of the stack, the look ahead table 
 * is consulted to find if a production exists for the top of the stack
 * and the look ahead.
 * 
 * When we find a matching production, we pop the non-terminal off the top of 
 * the stack (the LHS of the production), and push the tokens on the RHS of 
 * the production onto the stack. Note that we store a pointer to the parse_tree_node
 * containing the LHS token on the stack. This way, we can setup the children in 
 * the parse tree as we apply the production.
 *   
 * If there is a terminal symbol on top of the stack, we compare it with the
 * look ahead symbol. If they match, or we don't care about the value of the
 * terminal (e.g. we know its an integer, but don't necessairly care what 
 * integer), the terminal symbol is popped off the stack and the look ahead 
 * is advanced.
 *
 * There are a few special nasty cases of productions for which we make special 
 * cases. These aren't found in the production/look-ahead tables, but listed 
 * out explicitly.
 * 
 * After the parse tree has been constructed, we make several recusive passes 
 * over it to perform various tasks.
 *  
 * The first pass is made to clean up the state bindings.  This is done in 
 * parse_tree_fold_bindings(). The goal is to reduce the big mess of a parse tree
 * created by strings such as:
 * 
 *           result.color.secondary
 *
 * and roll them up into one token and fill out some information in a symbol table.
 * In this case, the token at the root of the derivation becomes BINDING_TOKEN,
 * and the token attribute is an index into the binding table where this state
 * is held.
 *
 * The next two passes go about associating variables with BINDING_TOKENs. This 
 * takes care of the cases like:
 *
 *           OUTPUT foo = result.color.secondary;
 *
 * by inserting the index in the binding table for result.color.secondary into 
 * the attr field in the identifier table where the 'foo' variable sits. 
 * The second such pass deals with setting up arrays of program parameters, 
 * while the first only deals with scalars.
 * 
 * We then examine all the information on variables and state that we have 
 * gathered, and layout which 'register' each variable or bit-of-state should use.
 *  
 *
 * Finally, we make a recursive pass of the parse tree and generate opcodes 
 * for Mesa to interpret while executing the program.
 * 
 * It should be noted that each input/stack token has two parts, an 'identifier' 
 * and an 'attribute'. The identifier tells what class the token is, e.g. INTEGER_TOKEN,
 * or NT_PROGRAM_SINGLE_ITEM_TOKEN. For some tokens, e.g. INTEGER_TOKEN, ID_TOKEN,
 * FLOAT_TOKEN, or BINDING_TOKEN, the attribute for the token is an index into a table
 * giving various properties about the token.
 * 
 */

/**
 * Here are all of the structs used to hold parse state and symbol
 * tables used.
 *
 * All strings which are not reserved words, floats, ints, or misc
 * puncuation (ie .) are considered to be [potential] identifiers.
 * When we encounter such a string while lex'ing, insert it into 
 * the id symbol table, shown below. 
 *
 * Sometime later, we'll initialize the variable types for identifiers
 * which really are variables. This gets shoved into the 'type' field.
 *
 * For variables, we'll need additional info (e.g. state it is bound to,
 * variable we're aliased to, etc). This is stored in the 'attr' field.
 * 	- For alias variables, the attr is the idx in the id table of
 * 	   the variable we are bound to.
 * 	- For other variables, we need a whole mess of info. This can be 
 * 	   found in the binding symbol table, below. In this case, the
 * 	   attr field is the idx into the binding table that describes us.
 * 	- For uninitialized ids, attr is -1.
 *
 * The data field holds the string of the id.
 *
 * len is the number of identifiers in the table.
 */
typedef struct st_id_table
{
   GLint len;
   GLint *type;
   GLint *attr;
   GLubyte **data;
}
id_table;

/**
 * For PARAM arrays, we need to record the contents for use when
 * laying out registers and loading state. 
 *
 * len is the number of arrays in the table.
 *
 * num_elements[i] is the number of items in array i. In other words, this
 * is the number of registers it would require when allocating.
 *
 * data[i][n] is the state bound to element n in array i. It is an idx into
 * the binding table
 */
typedef struct st_array_table
{
   GLint len;
   GLint *num_elements;
   GLint **data;
}
array_table;

/**
 * For holding all of the data used to describe an identifier, we have the catch
 * all binding symbol table.
 *
 * len is the number of bound items;
 *
 * type[i] tells what we are binding too, e.g. ATTRIB_POSITION, FOG_COLOR, or CONSTANT
 *
 * offset[i] gives the matrix number for matrix bindings, e.g. MATRIXROWS_MODELVIEW.
 * Alternativly, it gives the number of the first parameter for PROGRAM_ENV_* and 
 * PROGRAM_LOCAL_*.
 *
 * num_rows[i] gives the number of rows for multiple matrix rows, or the number 
 * of parameters in a env/local array.
 *
 * consts gives the 4-GLfloat constant value for bindings of type CONSTANT
 *
 * reg_num gives the register number which this binding is held in.
 */
typedef struct st_binding_table
{
   GLint len;
   GLint *type;
   GLint *offset;
   GLint *row;
   GLint *num_rows;
   GLfloat **consts;
   GLint *reg_num;
}
binding_table;

/**
 * Integers and floats are stored here. 
 */
typedef struct st_int_table
{
   GLint len;
   GLint *data;
}
int_table;

typedef struct st_float_table
{
   GLint len;
   GLdouble *data;
}
float_table;

/**
 * To avoid writing tons of mindless parser code, the parsing is driven by
 * a few big tables of rules, plus a few special cases. However, this means
 * that we have to do all of our analysis outside of the parsing step.
 *
 * So, the parser will contruct a parse tree describing the program string
 * which we can then operate on to do things like code generation.
 *
 * This struct represents a node in the parse tree.
 *
 * If tok is a non-terminal token, tok_attr is not relevant.
 * If tok is BINDING_TOKEN, tok_attr is the index into the binding table.
 * if tok is INTEGER_TOKEN or FLOAT_TOKEN, tok_attr is the index in the integer/GLfloat table
 *
 * prod_applied is the production number applied to this token in the derivation of
 * the program string. See arbvp_grammar.txt for a listing of the productions, and
 * their numbers.
 */
typedef struct st_parse_tree_node
{
   GLint tok, tok_attr, is_terminal;
   GLint prod_applied;
   struct st_parse_tree_node *children[4];
}
parse_tree_node;

/** This stores tokens that we lex out 
 */
typedef struct st_token_list
{
   GLint tok, tok_attr;
   parse_tree_node *pt;

   struct st_token_list *next;
}
token_list;

/**
 * This holds all of the productions in the grammar. 
 *
 * lhs is a non-terminal token, e.g. NT_PROGRAM_TOKEN.
 * rhs is either NULL_TOKEN, another non-terminal token, or a terminal token.
 * 	In some cases, we need the RHS to be a certain value, e.g. for the dst reg write masks.
 * 	For this, key is used to specify the string. If we don't care about the key, just
 * 	specify "".
 * 	Int/floats are slightly different, "-1" specifies 'we don't care'.
 *
 * lhs is not very useful here, but is is convient for sanity sake when specifing productions.
 */
typedef struct st_prod_table
{
   GLint lhs;
   GLint rhs[4];
   GLubyte *key[4];
}
prod_table;

/**
 * This holds the look ahead table to drive the parser. We examine the token on 
 * the top of the stack, as well as the next token in the input stream (the look ahead).
 * We then match this against the table, and apply the approprate production. If nothing
 * matches, we have a parse error.
 *
 * Here, LHS is the (non-terminal) token to match against the top of the stack.
 *
 * la is the token to match against the look ahead.
 *
 * If la is ID_TOKEN, we have to match a given keyword (e.g. 'ambient'). This is specified in 
 * la_kw.
 *
 * prod_idx is the idx into the prod_table of the production that we are to apply if this
 * rule matches.
 */
typedef struct st_look_ahead_table
{
   GLint lhs;
   GLint la;
   GLubyte *la_kw;
   GLint prod_idx;
}
look_ahead_table;

/**
 * This is the general catch-all for the parse state 
 */
typedef struct st_parse_state
{
   GLubyte *str;
   GLint len;

   /* lex stuff ------ */
   GLint start_pos, curr_pos;
   GLint curr_state;
   /* ---------------- */

   id_table idents;
   int_table ints;
   float_table floats;
   binding_table binds;
   array_table arrays;

   token_list *stack_head, *stack_free_list;

   parse_tree_node *pt_head;
}
parse_state;

/* local prototypes */
static GLint float_table_add(float_table * tab, GLubyte * str, GLint start,
			     GLint end);
static GLint int_table_add(int_table * tab, GLubyte * str, GLint start,
			   GLint end);
static GLint id_table_add(id_table * tab, GLubyte * str, GLint start,
			  GLint end);
static void parse_tree_free_children(parse_tree_node * ptn);

/** 
 * Here we have a ton of defined terms that we use to identify productions, 
 * terminals, and nonterminals.
 */

/**
 * Terminal tokens
 */
#define EOF_TOKEN 		0
#define ID_TOKEN			1
#define ABS_TOKEN			2
#define ADD_TOKEN			3
#define ADDRESS_TOKEN	4
#define ALIAS_TOKEN		5
#define ARL_TOKEN			6
#define ATTRIB_TOKEN		7

#define DP3_TOKEN			8
#define DP4_TOKEN			9
#define DPH_TOKEN			10
#define DST_TOKEN			11

#define END_TOKEN			12
#define EX2_TOKEN			13
#define EXP_TOKEN			14

#define FLR_TOKEN			15
#define FRC_TOKEN			16

#define LG2_TOKEN			17
#define LIT_TOKEN			18
#define LOG_TOKEN			19

#define MAD_TOKEN			20
#define MAX_TOKEN			21
#define MIN_TOKEN			22
#define MOV_TOKEN			23
#define MUL_TOKEN			24

#define OPTION_TOKEN    25
#define OUTPUT_TOKEN		26

#define PARAM_TOKEN		27
#define POW_TOKEN			28

#define RCP_TOKEN			29
#define RSQ_TOKEN			30

#define SGE_TOKEN			31
#define SLT_TOKEN			32
#define SUB_TOKEN			33
#define SWZ_TOKEN			34

#define TEMP_TOKEN		35

#define XPD_TOKEN			36

#define SEMICOLON_TOKEN	37
#define COMMA_TOKEN		38
#define PLUS_TOKEN		39
#define MINUS_TOKEN		40
#define PERIOD_TOKEN		41
#define DOTDOT_TOKEN		42
#define LBRACKET_TOKEN	43
#define RBRACKET_TOKEN	44
#define LBRACE_TOKEN		45
#define RBRACE_TOKEN		46
#define EQUAL_TOKEN		47

#define INTEGER_TOKEN	48
#define FLOAT_TOKEN		49

#define PROGRAM_TOKEN	50
#define RESULT_TOKEN		51
#define STATE_TOKEN		52
#define VERTEX_TOKEN		53

#define NULL_TOKEN		54

#define BINDING_TOKEN	55

/** 
 * Non-terminal tokens
 */
#define NT_PROGRAM_TOKEN					100
#define NT_OPTION_SEQUENCE_TOKEN			101
#define NT_OPTION_SEQUENCE2_TOKEN		102
#define NT_OPTION_TOKEN						103
#define NT_STATEMENT_SEQUENCE_TOKEN		104
#define NT_STATEMENT_SEQUENCE2_TOKEN	105
#define NT_STATEMENT_TOKEN					106

#define NT_INSTRUCTION_TOKEN				107
#define NT_ARL_INSTRUCTION_TOKEN			108
#define NT_VECTOROP_INSTRUCTION_TOKEN	109
#define NT_VECTOROP_TOKEN					110
#define NT_SCALAROP_INSTRUCTION_TOKEN	111
#define NT_SCALAROP_TOKEN					112
#define NT_BINSCOP_INSTRUCTION_TOKEN	113
#define NT_BINSCOP_INSTRUCTION2_TOKEN	114
#define NT_BINSCOP_TOKEN					115
#define NT_BINOP_INSTRUCTION_TOKEN		116
#define NT_BINOP_INSTRUCTION2_TOKEN		117
#define NT_BINOP_TOKEN						118
#define NT_TRIOP_INSTRUCTION_TOKEN		119
#define NT_TRIOP_INSTRUCTION2_TOKEN		120
#define NT_TRIOP_INSTRUCTION3_TOKEN		121
#define NT_TRIOP_TOKEN						122
#define NT_SWZ_INSTRUCTION_TOKEN			123
#define NT_SWZ_INSTRUCTION2_TOKEN		124

#define NT_SCALAR_SRC_REG_TOKEN			130
#define NT_SWIZZLE_SRC_REG_TOKEN			131
#define NT_MASKED_DST_REG_TOKEN			132
#define NT_MASKED_ADDR_REG_TOKEN			133
#define NT_EXTENDED_SWIZZLE_TOKEN		134
#define NT_EXTENDED_SWIZZLE2_TOKEN		135
#define NT_EXT_SWIZ_COMP_TOKEN			136
#define NT_EXT_SWIZ_SEL_TOKEN				137
#define NT_SRC_REG_TOKEN					138
#define NT_DST_REG_TOKEN					139
#define NT_VERTEX_ATTRIB_REG_TOKEN		140

#define NT_TEMPORARY_REG_TOKEN 			150
#define NT_PROG_PARAM_REG_TOKEN			151
#define NT_PROG_PARAM_SINGLE_TOKEN		152
#define NT_PROG_PARAM_ARRAY_TOKEN		153
#define NT_PROG_PARAM_ARRAY_MEM_TOKEN	154
#define NT_PROG_PARAM_ARRAY_ABS_TOKEN	155
#define NT_PROG_PARAM_ARRAY_REL_TOKEN	156

#define NT_ADDR_REG_REL_OFFSET_TOKEN	157
#define NT_ADDR_REG_POS_OFFSET_TOKEN	158
#define NT_ADDR_REG_NEG_OFFSET_TOKEN	159

#define NT_VERTEX_RESULT_REG_TOKEN		160
#define NT_ADDR_REG_TOKEN					161
#define NT_ADDR_COMPONENT_TOKEN			162
#define NT_ADDR_WRITE_MASK_TOKEN			163
#define NT_SCALAR_SUFFIX_TOKEN			164
#define NT_SWIZZLE_SUFFIX_TOKEN			165

#define NT_COMPONENT_TOKEN					166
#define NT_OPTIONAL_MASK_TOKEN			167
#define NT_OPTIONAL_MASK2_TOKEN			168
#define NT_NAMING_STATEMENT_TOKEN		169

#define NT_ATTRIB_STATEMENT_TOKEN		170
#define NT_VTX_ATTRIB_BINDING_TOKEN		171
#define NT_VTX_ATTRIB_ITEM_TOKEN			172
#define NT_VTX_ATTRIB_NUM_TOKEN			173
#define NT_VTX_OPT_WEIGHT_NUM_TOKEN		174
#define NT_VTX_WEIGHT_NUM_TOKEN			175
#define NT_PARAM_STATEMENT_TOKEN			176
#define NT_PARAM_STATEMENT2_TOKEN		177
#define NT_OPT_ARRAY_SIZE_TOKEN			178
#define NT_PARAM_SINGLE_INIT_TOKEN		179
#define NT_PARAM_MULTIPLE_INIT_TOKEN	180
#define NT_PARAM_MULT_INIT_LIST_TOKEN	181
#define NT_PARAM_MULT_INIT_LIST2_TOKEN	182
#define NT_PARAM_SINGLE_ITEM_DECL_TOKEN 183
#define NT_PARAM_SINGLE_ITEM_USE_TOKEN	184
#define NT_PARAM_MULTIPLE_ITEM_TOKEN	185
#define NT_STATE_MULTIPLE_ITEM_TOKEN	186
#define NT_STATE_MULTIPLE_ITEM2_TOKEN	187
#define NT_FOO_TOKEN							188
#define NT_FOO2_TOKEN						189
#define NT_FOO3_TOKEN						190
#define NT_FOO35_TOKEN						191
#define NT_FOO4_TOKEN						192
#define NT_STATE_SINGLE_ITEM_TOKEN		193
#define NT_STATE_SINGLE_ITEM2_TOKEN		194
#define NT_STATE_MATERIAL_ITEM_TOKEN	195
#define NT_STATE_MATERIAL_ITEM2_TOKEN	196
#define NT_STATE_MAT_PROPERTY_TOKEN		197
#define NT_STATE_LIGHT_ITEM_TOKEN		198
#define NT_STATE_LIGHT_ITEM2_TOKEN		199
#define NT_STATE_LIGHT_PROPERTY_TOKEN	200
#define NT_STATE_SPOT_PROPERTY_TOKEN	201
#define NT_STATE_LIGHT_MODEL_ITEM_TOKEN 202
#define NT_STATE_LMOD_PROPERTY_TOKEN	 203
#define NT_STATE_LMOD_PROPERTY2_TOKEN	 204

#define NT_STATE_LIGHT_PROD_ITEM_TOKEN	   207
#define NT_STATE_LIGHT_PROD_ITEM15_TOKEN	208
#define NT_STATE_LIGHT_PROD_ITEM2_TOKEN	209
#define NT_STATE_LPROD_PROPERTY_TOKEN	210
#define NT_STATE_LIGHT_NUMBER_TOKEN		211
#define NT_STATE_TEX_GEN_ITEM_TOKEN		212
#define NT_STATE_TEX_GEN_ITEM2_TOKEN	213
#define NT_STATE_TEX_GEN_TYPE_TOKEN		214
#define NT_STATE_TEX_GEN_COORD_TOKEN	215
#define NT_STATE_FOG_ITEM_TOKEN			216
#define NT_STATE_FOG_PROPERTY_TOKEN		217

#define NT_STATE_CLIP_PLANE_ITEM_TOKEN	218
#define NT_STATE_CLIP_PLANE_ITEM2_TOKEN	219
#define NT_STATE_CLIP_PLANE_NUM_TOKEN	220
#define NT_STATE_POINT_ITEM_TOKEN		221
#define NT_STATE_POINT_PROPERTY_TOKEN	222
#define NT_STATE_MATRIX_ROW_TOKEN		223
#define NT_STATE_MATRIX_ROW15_TOKEN		224
#define NT_STATE_MATRIX_ROW2_TOKEN		225
#define NT_STATE_MATRIX_ROW3_TOKEN		226
#define NT_STATE_MAT_MODIFIER_TOKEN		227
#define NT_STATE_MATRIX_ROW_NUM_TOKEN	228
#define NT_STATE_MATRIX_NAME_TOKEN		229
#define NT_STATE_OPT_MOD_MAT_NUM_TOKEN	230
#define NT_STATE_MOD_MAT_NUM_TOKEN		231
#define NT_STATE_PALETTE_MAT_NUM_TOKEN	232
#define NT_STATE_PROGRAM_MAT_NUM_TOKEN	233

#define NT_PROGRAM_SINGLE_ITEM_TOKEN	234
#define NT_PROGRAM_SINGLE_ITEM2_TOKEN	235
#define NT_PROGRAM_MULTIPLE_ITEM_TOKEN	236
#define NT_PROGRAM_MULTIPLE_ITEM2_TOKEN	237
#define NT_PROG_ENV_PARAMS_TOKEN			238
#define NT_PROG_ENV_PARAM_NUMS_TOKEN	239
#define NT_PROG_ENV_PARAM_NUMS2_TOKEN	240
#define NT_PROG_ENV_PARAM_TOKEN			250
#define NT_PROG_LOCAL_PARAMS_TOKEN		251
#define NT_PROG_LOCAL_PARAM_NUMS_TOKEN	252
#define NT_PROG_LOCAL_PARAM_NUMS2_TOKEN	253
#define NT_PROG_LOCAL_PARAM_TOKEN		254
#define NT_PROG_ENV_PARAM_NUM_TOKEN		255
#define NT_PROG_LOCAL_PARAM_NUM_TOKEN	256

#define NT_PARAM_CONST_DECL_TOKEN		257
#define NT_PARAM_CONST_USE_TOKEN			258
#define NT_PARAM_CONST_SCALAR_DECL_TOKEN	259
#define NT_PARAM_CONST_SCALAR_USE_TOKEN	260
#define NT_PARAM_CONST_VECTOR_TOKEN		261
#define NT_PARAM_CONST_VECTOR2_TOKEN	262
#define NT_PARAM_CONST_VECTOR3_TOKEN	263
#define NT_PARAM_CONST_VECTOR4_TOKEN	264

#define NT_SIGNED_FLOAT_CONSTANT_TOKEN	265
#define NT_FLOAT_CONSTANT_TOKEN			266
#define NT_OPTIONAL_SIGN_TOKEN			267

#define NT_TEMP_STATEMENT_TOKEN			268
#define NT_ADDRESS_STATEMENT_TOKEN		269
#define NT_VAR_NAME_LIST_TOKEN			270
#define NT_OUTPUT_STATEMENT_TOKEN		271
#define NT_RESULT_BINDING_TOKEN			272
#define NT_RESULT_BINDING2_TOKEN			273
#define NT_RESULT_COL_BINDING_TOKEN		274
#define NT_RESULT_COL_BINDING2_TOKEN	275
#define NT_RESULT_COL_BINDING3_TOKEN	276
#define NT_RESULT_COL_BINDING4_TOKEN	277
#define NT_RESULT_COL_BINDING5_TOKEN	278

#define NT_OPT_FACE_TYPE2_TOKEN			279
#define NT_OPT_COLOR_TYPE_TOKEN			280
#define NT_OPT_COLOR_TYPE2_TOKEN			281
#define NT_OPT_TEX_COORD_NUM_TOKEN		282
#define NT_TEX_COORD_NUM_TOKEN			283

#define NT_ALIAS_STATEMENT_TOKEN			284
#define NT_ESTABLISH_NAME_TOKEN			285
#define NT_ESTABLISHED_NAME_TOKEN		286

#define NT_SWIZZLE_SUFFIX2_TOKEN			287
#define NT_COMPONENT4_TOKEN				288

/**
 * FSA States for lex
 *
 * XXX: These can be turned into enums 
 */
#define STATE_BASE		0
#define STATE_IDENT		1

#define STATE_A			2
#define STATE_AB			3
#define STATE_ABS			4
#define STATE_AD			5
#define STATE_ADD			6
#define STATE_ADDR		7
#define STATE_ADDRE		8
#define STATE_ADDRES		9
#define STATE_ADDRESS	10
#define STATE_AL			11
#define STATE_ALI			12
#define STATE_ALIA		13
#define STATE_ALIAS		14
#define STATE_AR			15
#define STATE_ARL			16
#define STATE_AT			17
#define STATE_ATT			18
#define STATE_ATTR		19
#define STATE_ATTRI		20
#define STATE_ATTRIB		21

#define STATE_D			22
#define STATE_DP			23
#define STATE_DP3			24
#define STATE_DP4			25
#define STATE_DPH			26
#define STATE_DS			27
#define STATE_DST			28

#define STATE_E			29
#define STATE_EN			30
#define STATE_END			31
#define STATE_EX			32
#define STATE_EX2			33
#define STATE_EXP			34

#define STATE_F			35
#define STATE_FL			36
#define STATE_FLR			37
#define STATE_FR			38
#define STATE_FRC			39

#define STATE_L			40
#define STATE_LG			41
#define STATE_LG2			42
#define STATE_LI			43
#define STATE_LIT			44
#define STATE_LO			45
#define STATE_LOG			46

#define STATE_M			47
#define STATE_MA			48
#define STATE_MAD			49
#define STATE_MAX			50
#define STATE_MI			51
#define STATE_MIN			52
#define STATE_MO			53
#define STATE_MOV			54
#define STATE_MU			55
#define STATE_MUL			56

#define STATE_O			57
#define STATE_OP			58
#define STATE_OPT			59
#define STATE_OPTI		60
#define STATE_OPTIO		61
#define STATE_OPTION		62
#define STATE_OU			63
#define STATE_OUT			64
#define STATE_OUTP		65
#define STATE_OUTPU		66
#define STATE_OUTPUT		67

#define STATE_P			68
#define STATE_PA			69
#define STATE_PAR			70
#define STATE_PARA		71
#define STATE_PARAM		72
#define STATE_PO			73
#define STATE_POW			74

#define STATE_R			75
#define STATE_RC			76
#define STATE_RCP			77
#define STATE_RS			78
#define STATE_RSQ			79

#define STATE_S			80
#define STATE_SG			81
#define STATE_SGE			82
#define STATE_SL			83
#define STATE_SLT			84
#define STATE_SU			85
#define STATE_SUB			86
#define STATE_SW			87
#define STATE_SWZ			88

#define STATE_T			89
#define STATE_TE			90
#define STATE_TEM			91
#define STATE_TEMP		92

#define STATE_X			93
#define STATE_XP			94
#define STATE_XPD			95

#define STATE_N1			96
#define STATE_N2			97
#define STATE_N3			98
#define STATE_N4			99
#define STATE_N5			100
#define STATE_N6			101
#define STATE_N7			102

#define STATE_COMMENT	103

/* LC == lower case, as in 'program' */
#define STATE_LC_P		104
#define STATE_LC_PR		105
#define STATE_LC_PRO		106
#define STATE_LC_PROG	107
#define STATE_LC_PROGR	108
#define STATE_LC_PROGRA	109

#define STATE_LC_R		110
#define STATE_LC_RE		111
#define STATE_LC_RES		112
#define STATE_LC_RESU	113
#define STATE_LC_RESUL	114
#define STATE_LC_RESULT	115

#define STATE_LC_S		116
#define STATE_LC_ST		117
#define STATE_LC_STA		118
#define STATE_LC_STAT	119
#define STATE_LC_STATE	120

#define STATE_LC_V			121
#define STATE_LC_VE			122
#define STATE_LC_VER			123
#define STATE_LC_VERT		124
#define STATE_LC_VERTE		125
#define STATE_LC_VERTEX		126
#define STATE_LC_PROGRAM	127

/**
 * Error codes 
 */
#define ARB_VP_ERROR			-1
#define ARB_VP_SUCESS		 0

/** 
 * Variable types 
 */
#define TYPE_NONE				0
#define TYPE_ATTRIB			1
#define TYPE_PARAM			2
#define TYPE_PARAM_SINGLE	3
#define TYPE_PARAM_ARRAY	4
#define TYPE_TEMP				5
#define TYPE_ADDRESS			6
#define TYPE_OUTPUT			7
#define TYPE_ALIAS			8

/** 
 * Vertex Attrib Bindings
 */
#define ATTRIB_POSITION				1
#define ATTRIB_WEIGHT				2
#define ATTRIB_NORMAL				3
#define ATTRIB_COLOR_PRIMARY		4
#define ATTRIB_COLOR_SECONDARY	5
#define ATTRIB_FOGCOORD				6
#define ATTRIB_TEXCOORD				7
#define ATTRIB_MATRIXINDEX			8
#define ATTRIB_ATTRIB				9

/** 
 * Result Bindings
 */
#define RESULT_POSITION						10
#define RESULT_FOGCOORD						11
#define RESULT_POINTSIZE					12
#define RESULT_COLOR_FRONT_PRIMARY		13
#define RESULT_COLOR_FRONT_SECONDARY	14
#define RESULT_COLOR_BACK_PRIMARY		15
#define RESULT_COLOR_BACK_SECONDARY		16
#define RESULT_TEXCOORD						17

/** 
 * Material Property Bindings
 */
#define MATERIAL_FRONT_AMBIENT			18
#define MATERIAL_FRONT_DIFFUSE			19
#define MATERIAL_FRONT_SPECULAR			20
#define MATERIAL_FRONT_EMISSION			21
#define MATERIAL_FRONT_SHININESS			22
#define MATERIAL_BACK_AMBIENT				23
#define MATERIAL_BACK_DIFFUSE				24
#define MATERIAL_BACK_SPECULAR			25
#define MATERIAL_BACK_EMISSION			26
#define MATERIAL_BACK_SHININESS			27

/** 
 * Light Property Bindings
 */
#define LIGHT_AMBIENT						28
#define LIGHT_DIFFUSE						29
#define LIGHT_SPECULAR						30
#define LIGHT_POSITION						31
#define LIGHT_ATTENUATION					32
#define LIGHT_SPOT_DIRECTION				33
#define LIGHT_HALF							34
#define LIGHTMODEL_AMBIENT					35
#define LIGHTMODEL_FRONT_SCENECOLOR		36
#define LIGHTMODEL_BACK_SCENECOLOR		37
#define LIGHTPROD_FRONT_AMBIENT			38
#define LIGHTPROD_FRONT_DIFFUSE			39
#define LIGHTPROD_FRONT_SPECULAR			40
#define LIGHTPROD_BACK_AMBIENT			41
#define LIGHTPROD_BACK_DIFFUSE			42
#define LIGHTPROD_BACK_SPECULAR			43

/**
 * Texgen Property Bindings
 */
#define TEXGEN_EYE_S							44
#define TEXGEN_EYE_T							45
#define TEXGEN_EYE_R							46
#define TEXGEN_EYE_Q							47
#define TEXGEN_OBJECT_S						48
#define TEXGEN_OBJECT_T						49
#define TEXGEN_OBJECT_R						50
#define TEXGEN_OBJECT_Q						51

/**
 * Fog Property Bindings
 */
#define FOG_COLOR								52
#define FOG_PARAMS							53

/** 
 * Clip Property Bindings
 */
#define CLIP_PLANE							54

/** 
 * Point Property Bindings
 */
#define POINT_SIZE							55
#define POINT_ATTENUATION					56

/**
 * Matrix Row Property Bindings
 */
#define MATRIXROW_MODELVIEW					57
#define MATRIXROW_MODELVIEW_INVERSE			58
#define MATRIXROW_MODELVIEW_INVTRANS		59
#define MATRIXROW_MODELVIEW_TRANSPOSE		60
#define MATRIXROW_PROJECTION					61
#define MATRIXROW_PROJECTION_INVERSE		62
#define MATRIXROW_PROJECTION_INVTRANS		63
#define MATRIXROW_PROJECTION_TRANSPOSE		64
#define MATRIXROW_MVP							65
#define MATRIXROW_MVP_INVERSE					66
#define MATRIXROW_MVP_INVTRANS				67
#define MATRIXROW_MVP_TRANSPOSE				68
#define MATRIXROW_TEXTURE						69
#define MATRIXROW_TEXTURE_INVERSE			70
#define MATRIXROW_TEXTURE_INVTRANS			71
#define MATRIXROW_TEXTURE_TRANSPOSE			72
#define MATRIXROW_PALETTE						73
#define MATRIXROW_PALETTE_INVERSE			74
#define MATRIXROW_PALETTE_INVTRANS			75
#define MATRIXROW_PALETTE_TRANSPOSE			76
#define MATRIXROW_PROGRAM						77
#define MATRIXROW_PROGRAM_INVERSE			78
#define MATRIXROW_PROGRAM_INVTRANS			79
#define MATRIXROW_PROGRAM_TRANSPOSE			80

#define MATRIXROWS_MODELVIEW					81
#define MATRIXROWS_MODELVIEW_INVERSE		82
#define MATRIXROWS_MODELVIEW_INVTRANS		83
#define MATRIXROWS_MODELVIEW_TRANSPOSE		84
#define MATRIXROWS_PROJECTION					85
#define MATRIXROWS_PROJECTION_INVERSE		86
#define MATRIXROWS_PROJECTION_INVTRANS		87
#define MATRIXROWS_PROJECTION_TRANSPOSE	88
#define MATRIXROWS_MVP							89
#define MATRIXROWS_MVP_INVERSE				90
#define MATRIXROWS_MVP_INVTRANS				91
#define MATRIXROWS_MVP_TRANSPOSE				92
#define MATRIXROWS_TEXTURE						93
#define MATRIXROWS_TEXTURE_INVERSE			94
#define MATRIXROWS_TEXTURE_INVTRANS			95
#define MATRIXROWS_TEXTURE_TRANSPOSE		96
#define MATRIXROWS_PALETTE						97
#define MATRIXROWS_PALETTE_INVERSE			98
#define MATRIXROWS_PALETTE_INVTRANS			99
#define MATRIXROWS_PALETTE_TRANSPOSE		100
#define MATRIXROWS_PROGRAM						101
#define MATRIXROWS_PROGRAM_INVERSE			102
#define MATRIXROWS_PROGRAM_INVTRANS			103
#define MATRIXROWS_PROGRAM_TRANSPOSE		104

#define PROGRAM_ENV_SINGLE					105
#define PROGRAM_LOCAL_SINGLE				106
#define PROGRAM_ENV_MULTI					107
#define PROGRAM_LOCAL_MULTI				108

#define CONSTANT								109




#define IS_WHITESPACE(c)	(c == ' ') || (c == '\t') || (c == '\n')
#define IS_IDCHAR(c)			((c >= 'A') && (c <= 'Z')) || \
									((c >= 'a') && (c <= 'z')) || \
									(c == '_') || (c == '$')
#define IS_DIGIT(c)			(c >= '0') && (c <= '9')
#define IS_CD(c)				(IS_DIGIT(c)) || (IS_IDCHAR(c))

#define ADV_TO_STATE(state) s->curr_state = state; s->curr_pos++;

#define ADV_OR_FALLBACK(c, state) if (curr == c) { \
                                     ADV_TO_STATE(state); \
                                  } else {\
                                     if (IS_CD(curr)) { \
                                        ADV_TO_STATE(STATE_IDENT); \
                                     } else \
                                        s->curr_state = 1;\
                                  }

#define FINISH(tok) *token = tok; s->start_pos = s->curr_pos; s->curr_state = STATE_BASE; return ARB_VP_SUCESS;
#define ADV_AND_FINISH(tok) *token = tok; s->start_pos = s->curr_pos+1; s->curr_pos++; \
															s->curr_state = STATE_BASE; return ARB_VP_SUCESS;

#define FINISH_OR_FALLBACK(tok) if (IS_CD(curr)) {\
                                   ADV_TO_STATE(STATE_IDENT); \
                                } else { \
                                   FINISH(tok)\
                                }

#define NO_KW {"", "", "", ""}
#define NULL2 NULL_TOKEN, NULL_TOKEN
#define NULL3 NULL_TOKEN, NULL2
#define NULL4 NULL2, NULL2

/* This uglyness is the production table. See the prod_table struct definition for a description */
prod_table ptab[] = {
   {NT_PROGRAM_TOKEN,
        {NT_OPTION_SEQUENCE_TOKEN, NT_STATEMENT_SEQUENCE_TOKEN, END_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_OPTION_SEQUENCE_TOKEN,  {NT_OPTION_SEQUENCE2_TOKEN, NULL3},                      NO_KW},
   {NT_OPTION_SEQUENCE2_TOKEN, {NT_OPTION_TOKEN, NT_OPTION_SEQUENCE2_TOKEN, NULL2},     NO_KW},
   {NT_OPTION_SEQUENCE2_TOKEN, {NULL4},                                                 NO_KW},
   {NT_OPTION_TOKEN,           {OPTION_TOKEN, ID_TOKEN, SEMICOLON_TOKEN, NULL_TOKEN},   NO_KW},
    

   /* 5: */
   {NT_STATEMENT_SEQUENCE_TOKEN,  {NT_STATEMENT_SEQUENCE2_TOKEN, NULL3},                     NO_KW},
   {NT_STATEMENT_SEQUENCE2_TOKEN, {NT_STATEMENT_TOKEN, NT_STATEMENT_SEQUENCE2_TOKEN, NULL2}, NO_KW},
   {NT_STATEMENT_SEQUENCE2_TOKEN, {NULL4},                                                   NO_KW},
   {NT_STATEMENT_TOKEN, {NT_INSTRUCTION_TOKEN, SEMICOLON_TOKEN, NULL2},                      NO_KW},
   {NT_STATEMENT_TOKEN, {NT_NAMING_STATEMENT_TOKEN, SEMICOLON_TOKEN, NULL2},                 NO_KW},

   /* 10: */
   {NT_INSTRUCTION_TOKEN, {NT_ARL_INSTRUCTION_TOKEN, NULL3},      NO_KW},
   {NT_INSTRUCTION_TOKEN, {NT_VECTOROP_INSTRUCTION_TOKEN, NULL3}, NO_KW},
   {NT_INSTRUCTION_TOKEN, {NT_SCALAROP_INSTRUCTION_TOKEN, NULL3}, NO_KW},
   {NT_INSTRUCTION_TOKEN, {NT_BINSCOP_INSTRUCTION_TOKEN, NULL3},  NO_KW},
   {NT_INSTRUCTION_TOKEN, {NT_BINOP_INSTRUCTION_TOKEN, NULL3},    NO_KW},

   /* 15: */
   {NT_INSTRUCTION_TOKEN,          {NT_TRIOP_INSTRUCTION_TOKEN, NULL3},      NO_KW},
   {NT_INSTRUCTION_TOKEN,          {NT_SWZ_INSTRUCTION_TOKEN, NULL3},        NO_KW},
   {NT_ARL_INSTRUCTION_TOKEN,      {ARL_TOKEN, NT_MASKED_ADDR_REG_TOKEN, 
												  COMMA_TOKEN, NT_SCALAR_SRC_REG_TOKEN}, NO_KW},
   {NT_VECTOROP_INSTRUCTION_TOKEN, {NT_VECTOROP_TOKEN, NT_MASKED_DST_REG_TOKEN, COMMA_TOKEN,
                                     NT_SWIZZLE_SRC_REG_TOKEN},              NO_KW},
   {NT_VECTOROP_TOKEN,             {ABS_TOKEN, NULL3},                       NO_KW},

   /* 20: */
   {NT_VECTOROP_TOKEN,             {FLR_TOKEN, NULL3}, NO_KW},
   {NT_VECTOROP_TOKEN,             {FRC_TOKEN, NULL3}, NO_KW},
   {NT_VECTOROP_TOKEN,             {LIT_TOKEN, NULL3}, NO_KW},
   {NT_VECTOROP_TOKEN,             {MOV_TOKEN, NULL3}, NO_KW},
   {NT_SCALAROP_INSTRUCTION_TOKEN, {NT_SCALAROP_TOKEN, NT_MASKED_DST_REG_TOKEN, COMMA_TOKEN,
                             NT_SCALAR_SRC_REG_TOKEN}, NO_KW},

   /* 25: */
   {NT_SCALAROP_TOKEN, {EX2_TOKEN, NULL3}, NO_KW},
   {NT_SCALAROP_TOKEN, {EXP_TOKEN, NULL3}, NO_KW},
   {NT_SCALAROP_TOKEN, {LG2_TOKEN, NULL3}, NO_KW},
   {NT_SCALAROP_TOKEN, {LOG_TOKEN, NULL3}, NO_KW},
   {NT_SCALAROP_TOKEN, {RCP_TOKEN, NULL3}, NO_KW},

   /* 30: */
   {NT_SCALAROP_TOKEN, {RSQ_TOKEN, NULL3},         NO_KW},
   {NT_BINSCOP_INSTRUCTION_TOKEN,
                       {NT_BINSCOP_TOKEN, NT_MASKED_DST_REG_TOKEN, NT_BINSCOP_INSTRUCTION2_TOKEN,
                         NULL_TOKEN},              NO_KW},
   {NT_BINSCOP_INSTRUCTION2_TOKEN,
                       {COMMA_TOKEN, NT_SCALAR_SRC_REG_TOKEN, COMMA_TOKEN,
                         NT_SCALAR_SRC_REG_TOKEN}, NO_KW},
   {NT_BINSCOP_TOKEN,  {POW_TOKEN, NULL3},         NO_KW},
   {NT_BINOP_INSTRUCTION_TOKEN,
                       {NT_BINOP_TOKEN, NT_MASKED_DST_REG_TOKEN, NT_BINOP_INSTRUCTION2_TOKEN,
                         NULL_TOKEN},              NO_KW},

   /* 35: */
   {NT_BINOP_INSTRUCTION2_TOKEN,
                       {COMMA_TOKEN, NT_SWIZZLE_SRC_REG_TOKEN, COMMA_TOKEN,
                        NT_SWIZZLE_SRC_REG_TOKEN}, NO_KW},
   {NT_BINOP_TOKEN,    {ADD_TOKEN, NULL3},         NO_KW},
   {NT_BINOP_TOKEN,    {DP3_TOKEN, NULL3},         NO_KW},
   {NT_BINOP_TOKEN,    {DP4_TOKEN, NULL3},         NO_KW},
   {NT_BINOP_TOKEN,    {DPH_TOKEN, NULL3},         NO_KW},

   /* 40: */
   {NT_BINOP_TOKEN, {DST_TOKEN, NULL3}, NO_KW},
   {NT_BINOP_TOKEN, {MAX_TOKEN, NULL3}, NO_KW},
   {NT_BINOP_TOKEN, {MIN_TOKEN, NULL3}, NO_KW},
   {NT_BINOP_TOKEN, {MUL_TOKEN, NULL3}, NO_KW},
   {NT_BINOP_TOKEN, {SGE_TOKEN, NULL3}, NO_KW},

   /* 45: */
   {NT_BINOP_TOKEN, {SLT_TOKEN, NULL3}, NO_KW},
   {NT_BINOP_TOKEN, {SUB_TOKEN, NULL3}, NO_KW},
   {NT_BINOP_TOKEN, {XPD_TOKEN, NULL3}, NO_KW},
   {NT_TRIOP_INSTRUCTION_TOKEN,
                    {NT_TRIOP_TOKEN, NT_MASKED_DST_REG_TOKEN, NT_TRIOP_INSTRUCTION2_TOKEN,
                      NULL_TOKEN},      NO_KW},
   {NT_TRIOP_INSTRUCTION2_TOKEN,
                    {COMMA_TOKEN, NT_SWIZZLE_SRC_REG_TOKEN, NT_TRIOP_INSTRUCTION3_TOKEN,
                      NULL_TOKEN},      NO_KW},

   /* 50: */
   {NT_TRIOP_INSTRUCTION3_TOKEN,
                     {COMMA_TOKEN, NT_SWIZZLE_SRC_REG_TOKEN, COMMA_TOKEN,
                       NT_SWIZZLE_SRC_REG_TOKEN}, NO_KW},
   {NT_TRIOP_TOKEN,  {MAD_TOKEN, NULL3},          NO_KW},
   {NT_SWZ_INSTRUCTION_TOKEN,
                     {SWZ_TOKEN, NT_MASKED_DST_REG_TOKEN, NT_SWZ_INSTRUCTION2_TOKEN,
                       NULL_TOKEN},               NO_KW},
   {NT_SWZ_INSTRUCTION2_TOKEN,
                     {COMMA_TOKEN, NT_SRC_REG_TOKEN, COMMA_TOKEN, NT_EXTENDED_SWIZZLE_TOKEN},
                                                  NO_KW},
   {NT_SCALAR_SRC_REG_TOKEN,
                     {NT_OPTIONAL_SIGN_TOKEN, NT_SRC_REG_TOKEN, NT_SCALAR_SUFFIX_TOKEN,
                       NULL_TOKEN},               NO_KW},

   /* 55 */
   {NT_SWIZZLE_SRC_REG_TOKEN,
                      {NT_OPTIONAL_SIGN_TOKEN, NT_SRC_REG_TOKEN, NT_SWIZZLE_SUFFIX_TOKEN,
                        NULL_TOKEN},                                        NO_KW},
   {NT_MASKED_DST_REG_TOKEN,
                      {NT_DST_REG_TOKEN, NT_OPTIONAL_MASK_TOKEN, NULL2},    NO_KW},
   {NT_MASKED_ADDR_REG_TOKEN,
                      {NT_ADDR_REG_TOKEN, NT_ADDR_WRITE_MASK_TOKEN, NULL2}, NO_KW},
   {NT_EXTENDED_SWIZZLE_TOKEN,
                      {NT_EXT_SWIZ_COMP_TOKEN, COMMA_TOKEN, NT_EXT_SWIZ_COMP_TOKEN,
                        NT_EXTENDED_SWIZZLE2_TOKEN},                        NO_KW},
   {NT_EXTENDED_SWIZZLE2_TOKEN,
                      {COMMA_TOKEN, NT_EXT_SWIZ_COMP_TOKEN, COMMA_TOKEN,
                        NT_EXT_SWIZ_COMP_TOKEN},                            NO_KW},

   /* 60 */
   {NT_EXT_SWIZ_COMP_TOKEN,{NT_OPTIONAL_SIGN_TOKEN, NT_EXT_SWIZ_SEL_TOKEN, NULL2},
                                                                NO_KW},
   {NT_EXT_SWIZ_SEL_TOKEN, {INTEGER_TOKEN, NULL3},              {"0", "", "", ""}},
   {NT_EXT_SWIZ_SEL_TOKEN, {INTEGER_TOKEN, NULL3},              {"1", "", "", ""}},
   {NT_EXT_SWIZ_SEL_TOKEN, {NT_COMPONENT_TOKEN, NULL3},         NO_KW},
   {NT_SRC_REG_TOKEN,      {NT_VERTEX_ATTRIB_REG_TOKEN, NULL3}, NO_KW},

   /* 65: */
   {NT_SRC_REG_TOKEN,           {NT_TEMPORARY_REG_TOKEN, NULL3},     NO_KW},
   {NT_SRC_REG_TOKEN,           {NT_PROG_PARAM_REG_TOKEN, NULL3},    NO_KW},
   {NT_DST_REG_TOKEN,           {NT_TEMPORARY_REG_TOKEN, NULL3},     NO_KW},
   {NT_DST_REG_TOKEN,           {NT_VERTEX_RESULT_REG_TOKEN, NULL3}, NO_KW},
   {NT_VERTEX_ATTRIB_REG_TOKEN, {NT_ESTABLISHED_NAME_TOKEN, NULL3},  NO_KW},

   /* 70: */
   {NT_VERTEX_ATTRIB_REG_TOKEN, {NT_VTX_ATTRIB_BINDING_TOKEN, NULL3},    NO_KW},
   {NT_TEMPORARY_REG_TOKEN,     {NT_ESTABLISHED_NAME_TOKEN, NULL3},      NO_KW},
   {NT_PROG_PARAM_REG_TOKEN,    {NT_PROG_PARAM_SINGLE_TOKEN, NULL3},     NO_KW},
   {NT_PROG_PARAM_REG_TOKEN,
                                {NT_PROG_PARAM_ARRAY_TOKEN, LBRACKET_TOKEN, NT_PROG_PARAM_ARRAY_MEM_TOKEN,
                                  RBRACKET_TOKEN},                       NO_KW},
   {NT_PROG_PARAM_REG_TOKEN,    {NT_PARAM_SINGLE_ITEM_USE_TOKEN, NULL3}, NO_KW},

   /* 75: */
   {NT_PROG_PARAM_SINGLE_TOKEN,    {NT_ESTABLISHED_NAME_TOKEN, NULL3},     NO_KW},
   {NT_PROG_PARAM_ARRAY_TOKEN,     {NT_ESTABLISHED_NAME_TOKEN, NULL3},     NO_KW},
   {NT_PROG_PARAM_ARRAY_MEM_TOKEN, {NT_PROG_PARAM_ARRAY_ABS_TOKEN, NULL3}, NO_KW},
   {NT_PROG_PARAM_ARRAY_MEM_TOKEN, {NT_PROG_PARAM_ARRAY_REL_TOKEN, NULL3}, NO_KW},
                                                                           /* -1 matches all */
   {NT_PROG_PARAM_ARRAY_ABS_TOKEN, {INTEGER_TOKEN, NULL3},                 {"-1", "", "", ""}},	


   /* 80: */
   {NT_PROG_PARAM_ARRAY_REL_TOKEN,
              {NT_ADDR_REG_TOKEN, NT_ADDR_COMPONENT_TOKEN, NT_ADDR_REG_REL_OFFSET_TOKEN,
                NULL_TOKEN},                                      NO_KW},
   {NT_ADDR_REG_REL_OFFSET_TOKEN, {NULL4},                        NO_KW},
   {NT_ADDR_REG_REL_OFFSET_TOKEN,
              {PLUS_TOKEN, NT_ADDR_REG_POS_OFFSET_TOKEN, NULL2},  NO_KW},
   {NT_ADDR_REG_REL_OFFSET_TOKEN,
              {MINUS_TOKEN, NT_ADDR_REG_NEG_OFFSET_TOKEN, NULL2}, NO_KW},
   {NT_ADDR_REG_POS_OFFSET_TOKEN, {INTEGER_TOKEN, NULL3},         {"-1", "", "", ""}},

   /* 85: */
   {NT_ADDR_REG_NEG_OFFSET_TOKEN, {INTEGER_TOKEN, NULL3},             {"-1", "", "", ""}},
   {NT_VERTEX_RESULT_REG_TOKEN,   {NT_ESTABLISHED_NAME_TOKEN, NULL3}, NO_KW},
   {NT_VERTEX_RESULT_REG_TOKEN,   {NT_RESULT_BINDING_TOKEN, NULL3},   NO_KW},
   {NT_ADDR_REG_TOKEN,            {NT_ESTABLISHED_NAME_TOKEN, NULL3}, NO_KW},
   {NT_ADDR_COMPONENT_TOKEN,      {PERIOD_TOKEN, ID_TOKEN, NULL2},    {"", "x", "", ""}},
    
   /* 90: */
   {NT_ADDR_WRITE_MASK_TOKEN, {PERIOD_TOKEN, ID_TOKEN, NULL2},            {"", "x", "", ""}},
   {NT_SCALAR_SUFFIX_TOKEN,   {PERIOD_TOKEN, NT_COMPONENT_TOKEN, NULL2},  {"", "x", "", ""}},
   {NT_SWIZZLE_SUFFIX_TOKEN,  {NULL4},                                    NO_KW},
   {NT_COMPONENT_TOKEN,       {ID_TOKEN, NULL3},                          {"x", "", "", ""}},
   {NT_COMPONENT_TOKEN,       {ID_TOKEN, NULL3},                          {"y", "", "", ""}},

   /* 95: */
   {NT_COMPONENT_TOKEN,      {ID_TOKEN, NULL3},                              {"z", "", "", ""}},
   {NT_COMPONENT_TOKEN,      {ID_TOKEN, NULL3},                              {"w", "", "", ""}},
   {NT_OPTIONAL_MASK_TOKEN,  {PERIOD_TOKEN, NT_OPTIONAL_MASK2_TOKEN, NULL2}, NO_KW},
   {NT_OPTIONAL_MASK_TOKEN,  {NULL4},                                        NO_KW},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3},                              {"x", "", "", ""}},

   /* 100: */
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"y", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"xy", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"z", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"xz", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"yz", "", "", ""}},

   /* 105: */
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"xyz", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"w", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"xw", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"yw", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN, {ID_TOKEN, NULL3}, {"xyw", "", "", ""}},

   /* 110: */
   {NT_OPTIONAL_MASK2_TOKEN,  {ID_TOKEN, NULL3}, {"zw", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN,  {ID_TOKEN, NULL3}, {"xzw", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN,  {ID_TOKEN, NULL3}, {"yzw", "", "", ""}},
   {NT_OPTIONAL_MASK2_TOKEN,  {ID_TOKEN, NULL3}, {"xyzw", "", "", ""}},
   {NT_NAMING_STATEMENT_TOKEN, {NT_ATTRIB_STATEMENT_TOKEN, NULL3}, NO_KW},

   /* 115: */
   {NT_NAMING_STATEMENT_TOKEN, {NT_PARAM_STATEMENT_TOKEN, NULL3},   NO_KW},
   {NT_NAMING_STATEMENT_TOKEN, {NT_TEMP_STATEMENT_TOKEN, NULL3},    NO_KW},
   {NT_NAMING_STATEMENT_TOKEN, {NT_ADDRESS_STATEMENT_TOKEN, NULL3}, NO_KW},
   {NT_NAMING_STATEMENT_TOKEN, {NT_OUTPUT_STATEMENT_TOKEN, NULL3},  NO_KW},
   {NT_NAMING_STATEMENT_TOKEN, {NT_ALIAS_STATEMENT_TOKEN, NULL3},   NO_KW},

   /* 120: */
   {NT_ATTRIB_STATEMENT_TOKEN,
           {ATTRIB_TOKEN, NT_ESTABLISH_NAME_TOKEN, EQUAL_TOKEN, NT_VTX_ATTRIB_BINDING_TOKEN}, NO_KW},
   {NT_VTX_ATTRIB_BINDING_TOKEN,
           {VERTEX_TOKEN, PERIOD_TOKEN, NT_VTX_ATTRIB_ITEM_TOKEN, NULL_TOKEN},                NO_KW},
   {NT_VTX_ATTRIB_ITEM_TOKEN, {ID_TOKEN, NULL3},                             {"position", "", "", ""}},
   {NT_VTX_ATTRIB_ITEM_TOKEN, {ID_TOKEN, NT_VTX_OPT_WEIGHT_NUM_TOKEN, NULL2},{"weight", "", "", ""}},
   {NT_VTX_ATTRIB_ITEM_TOKEN, {ID_TOKEN, NULL3},                             {"normal", "", "", ""}},

   /* 125: */
   {NT_VTX_ATTRIB_ITEM_TOKEN, {ID_TOKEN, NT_OPT_COLOR_TYPE_TOKEN, NULL2},
                                                 {"color", "", "", ""}},
   {NT_VTX_ATTRIB_ITEM_TOKEN, {ID_TOKEN, NULL3}, {"fogcoord", "", "", ""}},
   {NT_VTX_ATTRIB_ITEM_TOKEN, {ID_TOKEN, NT_OPT_TEX_COORD_NUM_TOKEN, NULL2},
                                                 {"texcoord", "", "", ""}},
   {NT_VTX_ATTRIB_ITEM_TOKEN,
                              {ID_TOKEN, LBRACKET_TOKEN, NT_VTX_WEIGHT_NUM_TOKEN, RBRACKET_TOKEN},
                                                 {"matrixindex", "", "", ""}},
   {NT_VTX_ATTRIB_ITEM_TOKEN,
                              {ID_TOKEN, LBRACKET_TOKEN, NT_VTX_ATTRIB_NUM_TOKEN, RBRACKET_TOKEN},
                                                 {"attrib", "", "", ""}},

   /* 130: */
   {NT_VTX_ATTRIB_NUM_TOKEN,     {INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_VTX_OPT_WEIGHT_NUM_TOKEN, {NULL4},                NO_KW},
   {NT_VTX_OPT_WEIGHT_NUM_TOKEN, {LBRACKET_TOKEN, NT_VTX_WEIGHT_NUM_TOKEN, RBRACKET_TOKEN, NULL_TOKEN},
                                                         NO_KW},
   {NT_VTX_WEIGHT_NUM_TOKEN,     {INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_PARAM_STATEMENT_TOKEN,
                                 {PARAM_TOKEN, NT_ESTABLISH_NAME_TOKEN, NT_PARAM_STATEMENT2_TOKEN,
                                   NULL_TOKEN},          NO_KW},

   /* 135: */
   {NT_PARAM_STATEMENT2_TOKEN,  {NT_PARAM_SINGLE_INIT_TOKEN, NULL3},     NO_KW},
   {NT_PARAM_STATEMENT2_TOKEN,  {LBRACKET_TOKEN, NT_OPT_ARRAY_SIZE_TOKEN, RBRACKET_TOKEN,
                                    NT_PARAM_MULTIPLE_INIT_TOKEN},       NO_KW},
   {NT_OPT_ARRAY_SIZE_TOKEN,    {NULL4},                                 NO_KW},
   {NT_OPT_ARRAY_SIZE_TOKEN,    {INTEGER_TOKEN, NULL3},                  {"-1", "", "", ""}},
   {NT_PARAM_SINGLE_INIT_TOKEN, {EQUAL_TOKEN, NT_PARAM_SINGLE_ITEM_DECL_TOKEN, NULL2}, 
			                                                                NO_KW},
    

   /* 140: */
   {NT_PARAM_MULTIPLE_INIT_TOKEN,
                     {EQUAL_TOKEN, LBRACE_TOKEN, NT_PARAM_MULT_INIT_LIST_TOKEN, RBRACE_TOKEN},
                                              NO_KW},
   {NT_PARAM_MULT_INIT_LIST_TOKEN,
                     {NT_PARAM_MULTIPLE_ITEM_TOKEN, NT_PARAM_MULT_INIT_LIST2_TOKEN, NULL2},
                                              NO_KW},
   {NT_PARAM_MULT_INIT_LIST2_TOKEN,
                     {COMMA_TOKEN, NT_PARAM_MULT_INIT_LIST_TOKEN, NULL2}, 
							                         NO_KW},
   {NT_PARAM_MULT_INIT_LIST2_TOKEN,  {NULL4}, NO_KW},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, {NT_STATE_SINGLE_ITEM_TOKEN, NULL3},
                                              NO_KW},

   /* 145: */
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, {NT_PROGRAM_SINGLE_ITEM_TOKEN, NULL3}, NO_KW},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, {NT_PARAM_CONST_DECL_TOKEN, NULL3},    NO_KW},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, {NT_STATE_SINGLE_ITEM_TOKEN, NULL3},    NO_KW},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, {NT_PROGRAM_SINGLE_ITEM_TOKEN, NULL3},  NO_KW},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, {NT_PARAM_CONST_USE_TOKEN, NULL3},      NO_KW},

   /* 150: */
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, {NT_STATE_MULTIPLE_ITEM_TOKEN, NULL3},    NO_KW},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, {NT_PROGRAM_MULTIPLE_ITEM_TOKEN, NULL3},  NO_KW},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, {NT_PARAM_CONST_DECL_TOKEN, NULL3},       NO_KW},
   {NT_STATE_MULTIPLE_ITEM_TOKEN,
        {STATE_TOKEN, PERIOD_TOKEN, NT_STATE_MULTIPLE_ITEM2_TOKEN, NULL_TOKEN},
                                                                            NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_MATERIAL_ITEM_TOKEN, NULL3},   NO_KW},

   /* 155: */
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_LIGHT_ITEM_TOKEN, NULL3},       NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_LIGHT_MODEL_ITEM_TOKEN, NULL3}, NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_LIGHT_PROD_ITEM_TOKEN, NULL3},  NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_TEX_GEN_ITEM_TOKEN, NULL3},     NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_FOG_ITEM_TOKEN, NULL3},         NO_KW},

   /* 160: */
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_CLIP_PLANE_ITEM_TOKEN, NULL3},  NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, {NT_STATE_POINT_ITEM_TOKEN, NULL3},       NO_KW},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN,
                  {ID_TOKEN, PERIOD_TOKEN, NT_STATE_MATRIX_NAME_TOKEN, NT_FOO_TOKEN},
                                                               {"matrix", "", "", ""}},
   {NT_FOO_TOKEN, {PERIOD_TOKEN, NT_FOO2_TOKEN, NULL2},                      NO_KW},
   {NT_FOO2_TOKEN, {NT_STATE_MAT_MODIFIER_TOKEN, NT_FOO3_TOKEN, NULL2},      NO_KW},

   /* 165: */
   {NT_FOO2_TOKEN,  {ID_TOKEN, LBRACKET_TOKEN, NT_STATE_MATRIX_ROW_NUM_TOKEN, NT_FOO4_TOKEN},
                             {"row", "", "", ""}},
   {NT_FOO3_TOKEN,  {NULL4}, NO_KW},
   {NT_FOO3_TOKEN,  {PERIOD_TOKEN, ID_TOKEN, LBRACKET_TOKEN, NT_FOO35_TOKEN},
                            {"", "row", "", ""}},
   {NT_FOO35_TOKEN, {NT_STATE_MATRIX_ROW_NUM_TOKEN, NT_FOO4_TOKEN, NULL2},
                            NO_KW},
   {NT_FOO4_TOKEN,  {RBRACKET_TOKEN, NULL3}, NO_KW},

   /* 170: */
   {NT_FOO4_TOKEN, {DOTDOT_TOKEN, NT_STATE_MATRIX_ROW_NUM_TOKEN, RBRACKET_TOKEN, NULL_TOKEN},
                     NO_KW},
   {NT_STATE_SINGLE_ITEM_TOKEN,
                   {STATE_TOKEN, PERIOD_TOKEN, NT_STATE_SINGLE_ITEM2_TOKEN, NULL_TOKEN},
                     NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_MATERIAL_ITEM_TOKEN, NULL3},
                     NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_LIGHT_ITEM_TOKEN, NULL3}, 
			            NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_LIGHT_MODEL_ITEM_TOKEN, NULL3},
                     NO_KW},

   /* 175: */
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_LIGHT_PROD_ITEM_TOKEN, NULL3}, NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_TEX_GEN_ITEM_TOKEN, NULL3},    NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_FOG_ITEM_TOKEN, NULL3},        NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_CLIP_PLANE_ITEM_TOKEN, NULL3}, NO_KW},
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_POINT_ITEM_TOKEN, NULL3},      NO_KW},

   /* 180: */
   {NT_STATE_SINGLE_ITEM2_TOKEN, {NT_STATE_MATRIX_ROW_TOKEN, NULL3}, NO_KW},
   {NT_STATE_MATERIAL_ITEM_TOKEN,
         {ID_TOKEN, PERIOD_TOKEN, NT_STATE_MATERIAL_ITEM2_TOKEN, NULL_TOKEN},
                                                                     {"material", "", "", ""}},
   {NT_STATE_MATERIAL_ITEM2_TOKEN, {NT_STATE_MAT_PROPERTY_TOKEN, NULL3},
                                                                     NO_KW},
   {NT_STATE_MATERIAL_ITEM2_TOKEN,
         {PERIOD_TOKEN, NT_OPT_FACE_TYPE2_TOKEN, PERIOD_TOKEN,
            NT_STATE_MAT_PROPERTY_TOKEN},                            NO_KW},
   {NT_STATE_MAT_PROPERTY_TOKEN, {ID_TOKEN, NULL3},                  {"ambient", "", "", ""}},

   /* 185 */
   {NT_STATE_MAT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"diffuse", "", "", ""}},
   {NT_STATE_MAT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"specular", "", "", ""}},
   {NT_STATE_MAT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"emission", "", "", ""}},
   {NT_STATE_MAT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"shininess", "", "", ""}},
   {NT_STATE_LIGHT_ITEM_TOKEN,
                                 {ID_TOKEN, LBRACKET_TOKEN, NT_STATE_LIGHT_NUMBER_TOKEN,
                      NT_STATE_LIGHT_ITEM2_TOKEN},  {"light", "", "", ""}},

   /* 190: */
   {NT_STATE_LIGHT_ITEM2_TOKEN, {RBRACKET_TOKEN, PERIOD_TOKEN, NT_STATE_LIGHT_PROPERTY_TOKEN, NULL_TOKEN},
                                                      NO_KW},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"ambient", "", "", ""}},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"diffuse", "", "", ""}},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"specular", "", "", ""}},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"position", "", "", ""}},

   /* 195: */
   {NT_STATE_LIGHT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"attenuation", "", "", ""}},
   {NT_STATE_LIGHT_PROPERTY_TOKEN,
               {ID_TOKEN, PERIOD_TOKEN, NT_STATE_SPOT_PROPERTY_TOKEN, NULL_TOKEN},
                                                      {"spot", "", "", ""}},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"half", "", "", ""}},
   {NT_STATE_SPOT_PROPERTY_TOKEN,  {ID_TOKEN, NULL3}, {"direction", "", "", ""}},
   {NT_STATE_LIGHT_MODEL_ITEM_TOKEN,
     {ID_TOKEN, NT_STATE_LMOD_PROPERTY_TOKEN, NULL2}, {"lightmodel", "", "", ""}},
						     
   /* 200: */
   {NT_STATE_LMOD_PROPERTY_TOKEN,  {PERIOD_TOKEN, NT_STATE_LMOD_PROPERTY2_TOKEN, NULL2}, NO_KW},
   {NT_STATE_LMOD_PROPERTY2_TOKEN, {ID_TOKEN, NULL3}, {"ambient", "", "", ""}},
   {NT_STATE_LMOD_PROPERTY2_TOKEN, {ID_TOKEN, NULL3}, {"scenecolor", "", "", ""}},
   {NT_STATE_LMOD_PROPERTY2_TOKEN,
                 {NT_OPT_FACE_TYPE2_TOKEN, PERIOD_TOKEN, ID_TOKEN, NULL_TOKEN},
                                                      {"scenecolor", "", "", ""}},
   {NT_STATE_LIGHT_PROD_ITEM_TOKEN,
                 {ID_TOKEN, LBRACKET_TOKEN, NT_STATE_LIGHT_NUMBER_TOKEN,
                   NT_STATE_LIGHT_PROD_ITEM15_TOKEN}, {"lightprod", "", "", ""}},

   /* 205: */
   {NT_STATE_LIGHT_PROD_ITEM15_TOKEN,
    {RBRACKET_TOKEN, PERIOD_TOKEN, NT_STATE_LIGHT_PROD_ITEM2_TOKEN, NULL_TOKEN},        NO_KW},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN, {NT_STATE_LPROD_PROPERTY_TOKEN, NULL3},            NO_KW},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN,
   {NT_OPT_FACE_TYPE2_TOKEN, PERIOD_TOKEN, NT_STATE_LPROD_PROPERTY_TOKEN, NULL_TOKEN},  NO_KW},
   {NT_STATE_LPROD_PROPERTY_TOKEN, {ID_TOKEN, NULL3},  {"diffuse", "", "", ""}},
   {NT_STATE_LPROD_PROPERTY_TOKEN, {ID_TOKEN, NULL3},  {"ambient", "", "", ""}},
    
   /* 210: */
   {NT_STATE_LPROD_PROPERTY_TOKEN, {ID_TOKEN, NULL3},    {"specular", "", "", ""}},
   {NT_STATE_LIGHT_NUMBER_TOKEN, {INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_STATE_TEX_GEN_ITEM_TOKEN, {ID_TOKEN, NT_OPT_TEX_COORD_NUM_TOKEN, 
              NT_STATE_TEX_GEN_ITEM2_TOKEN, NULL_TOKEN}, {"texgen", "", "", ""}},
   {NT_STATE_TEX_GEN_ITEM2_TOKEN,{PERIOD_TOKEN, NT_STATE_TEX_GEN_TYPE_TOKEN, PERIOD_TOKEN,
                          NT_STATE_TEX_GEN_COORD_TOKEN}, NO_KW},
   {NT_STATE_TEX_GEN_TYPE_TOKEN, {ID_TOKEN, NULL3},      {"eye", "", "", ""}},

   /* 215: */
   {NT_STATE_TEX_GEN_TYPE_TOKEN,  {ID_TOKEN, NULL3}, {"object", "", "", ""}},
   {NT_STATE_TEX_GEN_COORD_TOKEN, {ID_TOKEN, NULL3}, {"s", "", "", ""}},
   {NT_STATE_TEX_GEN_COORD_TOKEN, {ID_TOKEN, NULL3}, {"t", "", "", ""}},
   {NT_STATE_TEX_GEN_COORD_TOKEN, {ID_TOKEN, NULL3}, {"r", "", "", ""}},
   {NT_STATE_TEX_GEN_COORD_TOKEN, {ID_TOKEN, NULL3}, {"q", "", "", ""}},

   /* 220: */
   {NT_STATE_FOG_ITEM_TOKEN,     {ID_TOKEN, PERIOD_TOKEN, NT_STATE_FOG_PROPERTY_TOKEN, NULL_TOKEN},
                                                    {"fog", "","",""}},
   {NT_STATE_FOG_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"color", "", "", ""}},
   {NT_STATE_FOG_PROPERTY_TOKEN, {ID_TOKEN, NULL3}, {"params", "", "", ""}},
   {NT_STATE_CLIP_PLANE_ITEM_TOKEN,
                {ID_TOKEN, LBRACKET_TOKEN, NT_STATE_CLIP_PLANE_NUM_TOKEN,
                  NT_STATE_CLIP_PLANE_ITEM2_TOKEN}, {"clip", "", "", ""}},
   {NT_STATE_CLIP_PLANE_ITEM2_TOKEN,
                {RBRACKET_TOKEN, PERIOD_TOKEN, ID_TOKEN, NULL_TOKEN}, 
					                                     {"", "", "plane", ""}},
																	 
   /* 225: */
   {NT_STATE_CLIP_PLANE_NUM_TOKEN,{INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_STATE_POINT_ITEM_TOKEN,    {ID_TOKEN, PERIOD_TOKEN, NT_STATE_POINT_PROPERTY_TOKEN, NULL_TOKEN},
                                                          {"point", "", "", ""}},
   {NT_STATE_POINT_PROPERTY_TOKEN, {ID_TOKEN, NULL3},     {"size", "", "", ""}},
   {NT_STATE_POINT_PROPERTY_TOKEN, {ID_TOKEN, NULL3},     {"attenuation", "", "", ""}},
   {NT_STATE_MATRIX_ROW_TOKEN,     {ID_TOKEN, PERIOD_TOKEN, NT_STATE_MATRIX_NAME_TOKEN,
                            NT_STATE_MATRIX_ROW15_TOKEN}, {"matrix", "", "", ""}},

   /* 230: */
   {NT_STATE_MATRIX_ROW15_TOKEN, {PERIOD_TOKEN, NT_STATE_MATRIX_ROW2_TOKEN, NULL2}, NO_KW},
   {NT_STATE_MATRIX_ROW2_TOKEN,  {ID_TOKEN, LBRACKET_TOKEN, 
                           NT_STATE_MATRIX_ROW_NUM_TOKEN, RBRACKET_TOKEN},         
									{"row", "", "", ""}},
   {NT_STATE_MATRIX_ROW2_TOKEN, {NT_STATE_MAT_MODIFIER_TOKEN, PERIOD_TOKEN, ID_TOKEN,
                           NT_STATE_MATRIX_ROW3_TOKEN}, 
									{"", "", "row", ""}},
   {NT_STATE_MATRIX_ROW3_TOKEN, {LBRACKET_TOKEN, NT_STATE_MATRIX_ROW_NUM_TOKEN, RBRACKET_TOKEN,
                          NULL_TOKEN}, NO_KW},
   {NT_STATE_MAT_MODIFIER_TOKEN, {ID_TOKEN, NULL3}, {"inverse", "", "", ""}},

   /* 235: */
   {NT_STATE_MAT_MODIFIER_TOKEN,   {ID_TOKEN, NULL3},      {"transpose", "", "", ""}},
   {NT_STATE_MAT_MODIFIER_TOKEN,   {ID_TOKEN, NULL3},      {"invtrans", "", "", ""}},
   {NT_STATE_MATRIX_ROW_NUM_TOKEN, {INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_STATE_MATRIX_NAME_TOKEN,    {ID_TOKEN, NT_STATE_OPT_MOD_MAT_NUM_TOKEN, NULL2},
			                                                  {"modelview", "", "", ""}},
   {NT_STATE_MATRIX_NAME_TOKEN, {ID_TOKEN, NULL3},         {"projection", "", "", ""}},

   /* 240: */
   {NT_STATE_MATRIX_NAME_TOKEN, {ID_TOKEN, NULL3}, {"mvp", "", "", ""}},
   {NT_STATE_MATRIX_NAME_TOKEN, {ID_TOKEN, NT_OPT_TEX_COORD_NUM_TOKEN, NULL2},
                                                   {"texture", "", "", ""}},
   {NT_STATE_MATRIX_NAME_TOKEN, {ID_TOKEN, LBRACKET_TOKEN, NT_STATE_PALETTE_MAT_NUM_TOKEN,
                                  RBRACKET_TOKEN}, {"palette", "", "", ""}},
   {NT_STATE_MATRIX_NAME_TOKEN, {PROGRAM_TOKEN, LBRACKET_TOKEN, NT_STATE_PROGRAM_MAT_NUM_TOKEN,
                                  RBRACKET_TOKEN}, NO_KW},
   {NT_STATE_OPT_MOD_MAT_NUM_TOKEN, {NULL4},       NO_KW},

   /* 245: */
   {NT_STATE_OPT_MOD_MAT_NUM_TOKEN,
    {LBRACKET_TOKEN, NT_STATE_MOD_MAT_NUM_TOKEN, RBRACKET_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_STATE_MOD_MAT_NUM_TOKEN, {INTEGER_TOKEN, NULL3},                       {"-1", "", "", ""}},
   {NT_STATE_PALETTE_MAT_NUM_TOKEN, {INTEGER_TOKEN, NULL3},                   {"-1", "", "", ""}},
   {NT_STATE_PROGRAM_MAT_NUM_TOKEN, {INTEGER_TOKEN, NULL3},                   {"-1", "", "", ""}},
   {NT_PROGRAM_SINGLE_ITEM_TOKEN, 
     {PROGRAM_TOKEN, PERIOD_TOKEN, NT_PROGRAM_SINGLE_ITEM2_TOKEN, NULL_TOKEN}, NO_KW},

   /* 250: */
   {NT_PROGRAM_SINGLE_ITEM2_TOKEN, {NT_PROG_ENV_PARAM_TOKEN, NULL3},            NO_KW},
   {NT_PROGRAM_SINGLE_ITEM2_TOKEN, {NT_PROG_LOCAL_PARAM_TOKEN, NULL3},          NO_KW},
   {NT_PROGRAM_MULTIPLE_ITEM_TOKEN,
    {PROGRAM_TOKEN, PERIOD_TOKEN, NT_PROGRAM_MULTIPLE_ITEM2_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_PROGRAM_MULTIPLE_ITEM2_TOKEN, {NT_PROG_ENV_PARAMS_TOKEN, NULL3},         NO_KW},
   {NT_PROGRAM_MULTIPLE_ITEM2_TOKEN, {NT_PROG_LOCAL_PARAMS_TOKEN, NULL3},       NO_KW},

   /* 255: */
   {NT_PROG_ENV_PARAMS_TOKEN, {ID_TOKEN, LBRACKET_TOKEN, NT_PROG_ENV_PARAM_NUMS_TOKEN, RBRACKET_TOKEN},
                                   {"env", "", "", ""}},
   {NT_PROG_ENV_PARAM_NUMS_TOKEN, {NT_PROG_ENV_PARAM_NUM_TOKEN, NT_PROG_ENV_PARAM_NUMS2_TOKEN, NULL2},
                                   NO_KW},
   {NT_PROG_ENV_PARAM_NUMS2_TOKEN, {DOTDOT_TOKEN, NT_PROG_ENV_PARAM_NUM_TOKEN, NULL2}, NO_KW},
   {NT_PROG_ENV_PARAM_NUMS2_TOKEN, {NULL4},                                            NO_KW},
   {NT_PROG_ENV_PARAM_TOKEN,
     {ID_TOKEN, LBRACKET_TOKEN, NT_PROG_ENV_PARAM_NUM_TOKEN, RBRACKET_TOKEN},
                                   {"env", "", "", ""}},

   /* 260: */
   {NT_PROG_LOCAL_PARAMS_TOKEN, {ID_TOKEN, LBRACKET_TOKEN, NT_PROG_LOCAL_PARAM_NUMS_TOKEN,
                  RBRACKET_TOKEN}, {"local", "", "", ""}},
   {NT_PROG_LOCAL_PARAM_NUMS_TOKEN,
    {NT_PROG_LOCAL_PARAM_NUM_TOKEN, NT_PROG_LOCAL_PARAM_NUMS2_TOKEN, NULL2},
                                   NO_KW},
   {NT_PROG_LOCAL_PARAM_NUMS2_TOKEN, {DOTDOT_TOKEN, NT_PROG_LOCAL_PARAM_NUM_TOKEN, NULL2}, NO_KW},
   {NT_PROG_LOCAL_PARAM_NUMS2_TOKEN, {NULL4},                                              NO_KW},
   {NT_PROG_LOCAL_PARAM_TOKEN, {ID_TOKEN, LBRACKET_TOKEN, NT_PROG_LOCAL_PARAM_NUM_TOKEN, RBRACKET_TOKEN},
                                   {"local", "", "", ""}},

   /* 265: */
   {NT_PROG_ENV_PARAM_NUM_TOKEN, {INTEGER_TOKEN, NULL3},   {"-1", "", "", ""}},
   {NT_PROG_LOCAL_PARAM_NUM_TOKEN, {INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_PARAM_CONST_DECL_TOKEN, {NT_PARAM_CONST_SCALAR_DECL_TOKEN, NULL3}, NO_KW},
   {NT_PARAM_CONST_DECL_TOKEN, {NT_PARAM_CONST_VECTOR_TOKEN, NULL3},      NO_KW},
   {NT_PARAM_CONST_USE_TOKEN, {NT_PARAM_CONST_SCALAR_USE_TOKEN, NULL3},   NO_KW},
    
   /* 270: */
   {NT_PARAM_CONST_USE_TOKEN,         {NT_PARAM_CONST_VECTOR_TOKEN, NULL3},    NO_KW},
   {NT_PARAM_CONST_SCALAR_DECL_TOKEN, {NT_SIGNED_FLOAT_CONSTANT_TOKEN, NULL3}, NO_KW},
   {NT_PARAM_CONST_SCALAR_USE_TOKEN,  {FLOAT_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_PARAM_CONST_VECTOR_TOKEN, {LBRACE_TOKEN, NT_SIGNED_FLOAT_CONSTANT_TOKEN,
                                    NT_PARAM_CONST_VECTOR2_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_PARAM_CONST_VECTOR2_TOKEN,{RBRACE_TOKEN, NULL3},                        NO_KW},

   /* 275: */
   {NT_PARAM_CONST_VECTOR2_TOKEN, {COMMA_TOKEN, NT_SIGNED_FLOAT_CONSTANT_TOKEN,
                                    NT_PARAM_CONST_VECTOR3_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_PARAM_CONST_VECTOR3_TOKEN, {RBRACE_TOKEN, NULL3},                       NO_KW},
   {NT_PARAM_CONST_VECTOR3_TOKEN, {COMMA_TOKEN, NT_SIGNED_FLOAT_CONSTANT_TOKEN,
                                    NT_PARAM_CONST_VECTOR4_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_PARAM_CONST_VECTOR4_TOKEN, {RBRACE_TOKEN, NULL3},                       NO_KW},
   {NT_PARAM_CONST_VECTOR4_TOKEN, {COMMA_TOKEN, NT_SIGNED_FLOAT_CONSTANT_TOKEN, 
														RBRACE_TOKEN, NULL_TOKEN},           NO_KW},

   /* 280: */
   {NT_SIGNED_FLOAT_CONSTANT_TOKEN, {NT_OPTIONAL_SIGN_TOKEN, FLOAT_TOKEN, NULL2}, 
                                                  {"", "-1", "", ""}},
   {NT_OPTIONAL_SIGN_TOKEN, {NULL4},              NO_KW},
   {NT_OPTIONAL_SIGN_TOKEN, {MINUS_TOKEN, NULL3}, NO_KW},
   {NT_OPTIONAL_SIGN_TOKEN, {PLUS_TOKEN, NULL3},  NO_KW},
   {NT_TEMP_STATEMENT_TOKEN, {TEMP_TOKEN, NT_VAR_NAME_LIST_TOKEN, NULL2},
                                                  NO_KW},

   /* 285: */
   {NT_ADDRESS_STATEMENT_TOKEN, {ADDRESS_TOKEN, NT_VAR_NAME_LIST_TOKEN, NULL2}, NO_KW},
   {NT_VAR_NAME_LIST_TOKEN,     {NT_ESTABLISH_NAME_TOKEN, NULL3},               NO_KW},
   {NT_VAR_NAME_LIST_TOKEN,     {NT_ESTABLISH_NAME_TOKEN, COMMA_TOKEN, NT_VAR_NAME_LIST_TOKEN,
                                  NULL_TOKEN},                                  NO_KW},
   {NT_OUTPUT_STATEMENT_TOKEN,  {OUTPUT_TOKEN, NT_ESTABLISH_NAME_TOKEN, EQUAL_TOKEN,
                                  NT_RESULT_BINDING_TOKEN},                     NO_KW},
   {NT_RESULT_BINDING_TOKEN,    {RESULT_TOKEN, PERIOD_TOKEN, NT_RESULT_BINDING2_TOKEN, NULL_TOKEN},
                                                                                NO_KW},

   /* 290: */
   {NT_RESULT_BINDING2_TOKEN, {ID_TOKEN, NULL3},                    {"position", "", "", ""}},
   {NT_RESULT_BINDING2_TOKEN, {ID_TOKEN, NULL3},                    {"fogcoord", "", "", ""}},
   {NT_RESULT_BINDING2_TOKEN, {ID_TOKEN, NULL3},                    {"pointsize", "", "", ""}},
   {NT_RESULT_BINDING2_TOKEN, {NT_RESULT_COL_BINDING_TOKEN, NULL3}, NO_KW},
   {NT_RESULT_BINDING2_TOKEN, {ID_TOKEN, NT_OPT_TEX_COORD_NUM_TOKEN, NULL2},
                                                                    {"texcoord", "", "", ""}},

   /* 295: */
   {NT_RESULT_COL_BINDING_TOKEN, {ID_TOKEN, NT_RESULT_COL_BINDING2_TOKEN, NULL2}, {"color", "", "", ""}},
   {NT_RESULT_COL_BINDING2_TOKEN, {NULL4},                                        NO_KW},
   {NT_RESULT_COL_BINDING2_TOKEN, {PERIOD_TOKEN, NT_RESULT_COL_BINDING3_TOKEN, NULL2}, NO_KW},
   {NT_RESULT_COL_BINDING3_TOKEN, {ID_TOKEN, NT_RESULT_COL_BINDING4_TOKEN, NULL2}, {"front", "", "", ""}},
   {NT_RESULT_COL_BINDING3_TOKEN, {ID_TOKEN, NT_RESULT_COL_BINDING4_TOKEN, NULL2}, {"back", "", "", ""}},
    
   /* 300: */
   {NT_RESULT_COL_BINDING4_TOKEN, {NULL4},           NO_KW},
   {NT_RESULT_COL_BINDING4_TOKEN, {PERIOD_TOKEN, NT_RESULT_COL_BINDING5_TOKEN, NULL2}, NO_KW},
   {NT_RESULT_COL_BINDING5_TOKEN, {ID_TOKEN, NULL3}, {"primary", "", "", ""}},
   {NT_RESULT_COL_BINDING5_TOKEN, {ID_TOKEN, NULL3}, {"secondary", "", "", ""}},
   {NT_OPT_FACE_TYPE2_TOKEN,      {ID_TOKEN, NULL3}, {"front", "", "", ""}},

   /* 305: */
   {NT_OPT_FACE_TYPE2_TOKEN, {ID_TOKEN, NULL3},  {"back", "", "", ""}},
   {NT_OPT_COLOR_TYPE_TOKEN, {PERIOD_TOKEN, NT_OPT_COLOR_TYPE2_TOKEN, NULL2}, NO_KW},
   {NT_OPT_COLOR_TYPE_TOKEN, {NULL4},            NO_KW},
   {NT_OPT_COLOR_TYPE2_TOKEN, {ID_TOKEN, NULL3}, {"primary", "", "", ""}},
   {NT_OPT_COLOR_TYPE2_TOKEN, {ID_TOKEN, NULL3}, {"secondary", "", "", ""}},

   /* 310: */
   {NT_OPT_TEX_COORD_NUM_TOKEN, {NULL4},                                  NO_KW},
   {NT_OPT_TEX_COORD_NUM_TOKEN,
    {LBRACKET_TOKEN, NT_TEX_COORD_NUM_TOKEN, RBRACKET_TOKEN, NULL_TOKEN}, NO_KW},
   {NT_TEX_COORD_NUM_TOKEN,     {INTEGER_TOKEN, NULL3}, {"-1", "", "", ""}},
   {NT_ALIAS_STATEMENT_TOKEN,   {ALIAS_TOKEN, NT_ESTABLISH_NAME_TOKEN, EQUAL_TOKEN,
                             NT_ESTABLISHED_NAME_TOKEN}, NO_KW},
   {NT_ESTABLISH_NAME_TOKEN, {ID_TOKEN, NULL3},          {"-1", "", "", ""}},

   /* 315: */
   {NT_ESTABLISHED_NAME_TOKEN, {ID_TOKEN, NULL3},           {"-1", "", "", ""}},
   {NT_FOO_TOKEN,              {NULL4},                     NO_KW},
   {NT_SWIZZLE_SUFFIX_TOKEN, {PERIOD_TOKEN, NT_SWIZZLE_SUFFIX2_TOKEN, NULL2},
                                                            NO_KW},
   {NT_SWIZZLE_SUFFIX2_TOKEN, {NT_COMPONENT_TOKEN, NULL3},  NO_KW},
   {NT_SWIZZLE_SUFFIX2_TOKEN, {NT_COMPONENT4_TOKEN, NULL3}, NO_KW},
};

/* This is the look ahead table. See the look_ahead_table struct def for a description */
look_ahead_table latab[] = {
   {NT_PROGRAM_TOKEN, ABS_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, ADD_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, ADDRESS_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, ALIAS_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, ARL_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, ATTRIB_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, DP3_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, DP4_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, DPH_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, DST_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, END_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, EOF_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, EX2_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, EXP_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, FLR_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, FRC_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, LIT_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, LG2_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, LOG_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, MAD_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, MAX_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, MIN_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, MOV_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, MUL_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, OPTION_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, OUTPUT_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, PARAM_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, POW_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, RCP_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, RSQ_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, SGE_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, SLT_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, SUB_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, SWZ_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, XPD_TOKEN, "", 0},
   {NT_PROGRAM_TOKEN, TEMP_TOKEN, "", 0},

   {NT_OPTION_SEQUENCE_TOKEN, ABS_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, ADD_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, ADDRESS_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, ALIAS_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, ARL_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, ATTRIB_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, DP3_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, DP4_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, DPH_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, DST_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, END_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, EX2_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, EXP_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, FLR_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, FRC_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, LIT_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, LG2_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, LOG_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, MAD_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, MAX_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, MIN_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, MOV_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, MUL_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, OPTION_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, OUTPUT_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, PARAM_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, POW_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, RCP_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, RSQ_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, SGE_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, SLT_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, SUB_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, SWZ_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, XPD_TOKEN, "", 1},
   {NT_OPTION_SEQUENCE_TOKEN, TEMP_TOKEN, "", 1},

   {NT_OPTION_SEQUENCE2_TOKEN, OPTION_TOKEN, "", 2},

   {NT_OPTION_SEQUENCE2_TOKEN, ABS_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, ADD_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, ADDRESS_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, ALIAS_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, ARL_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, ATTRIB_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, DP3_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, DP4_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, DPH_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, DST_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, END_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, EX2_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, EXP_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, FLR_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, FRC_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, LIT_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, LG2_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, LOG_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, MAD_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, MAX_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, MIN_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, MOV_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, MUL_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, OUTPUT_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, PARAM_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, POW_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, RCP_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, RSQ_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, SGE_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, SLT_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, SUB_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, SWZ_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, XPD_TOKEN, "", 3},
   {NT_OPTION_SEQUENCE2_TOKEN, TEMP_TOKEN, "", 3},

   {NT_OPTION_TOKEN, OPTION_TOKEN, "", 4},

   {NT_STATEMENT_SEQUENCE_TOKEN, ABS_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, ADD_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, ADDRESS_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, ALIAS_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, ARL_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, ATTRIB_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, DP3_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, DP4_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, DPH_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, DST_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, END_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, EX2_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, EXP_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, FLR_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, FRC_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, LIT_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, LG2_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, LOG_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, MAD_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, MAX_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, MIN_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, MOV_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, MUL_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, OUTPUT_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, PARAM_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, POW_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, RCP_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, RSQ_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, SGE_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, SLT_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, SUB_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, SWZ_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, XPD_TOKEN, "", 5},
   {NT_STATEMENT_SEQUENCE_TOKEN, TEMP_TOKEN, "", 5},

   {NT_STATEMENT_SEQUENCE2_TOKEN, ABS_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, ADD_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, ADDRESS_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, ALIAS_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, ARL_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, ATTRIB_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, DP3_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, DP4_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, DPH_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, DST_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, EX2_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, EXP_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, FLR_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, FRC_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, LIT_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, LG2_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, LOG_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, MAD_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, MAX_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, MIN_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, MOV_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, MUL_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, OUTPUT_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, PARAM_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, POW_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, RCP_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, RSQ_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, SGE_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, SLT_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, SUB_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, SWZ_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, XPD_TOKEN, "", 6},
   {NT_STATEMENT_SEQUENCE2_TOKEN, TEMP_TOKEN, "", 6},

   {NT_STATEMENT_SEQUENCE2_TOKEN, END_TOKEN, "", 7},


   {NT_STATEMENT_TOKEN, ABS_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, ADD_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, ARL_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, DP3_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, DP4_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, DPH_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, DST_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, EX2_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, EXP_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, FLR_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, FRC_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, LIT_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, LG2_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, LOG_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, MAD_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, MAX_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, MIN_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, MOV_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, MUL_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, POW_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, RCP_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, RSQ_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, SGE_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, SLT_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, SUB_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, SWZ_TOKEN, "", 8},
   {NT_STATEMENT_TOKEN, XPD_TOKEN, "", 8},

   {NT_STATEMENT_TOKEN, ADDRESS_TOKEN, "", 9},
   {NT_STATEMENT_TOKEN, ALIAS_TOKEN, "", 9},
   {NT_STATEMENT_TOKEN, ATTRIB_TOKEN, "", 9},
   {NT_STATEMENT_TOKEN, OUTPUT_TOKEN, "", 9},
   {NT_STATEMENT_TOKEN, PARAM_TOKEN, "", 9},
   {NT_STATEMENT_TOKEN, TEMP_TOKEN, "", 9},

   {NT_INSTRUCTION_TOKEN, ARL_TOKEN, "", 10},

   {NT_INSTRUCTION_TOKEN, ABS_TOKEN, "", 11},
   {NT_INSTRUCTION_TOKEN, FLR_TOKEN, "", 11},
   {NT_INSTRUCTION_TOKEN, FRC_TOKEN, "", 11},
   {NT_INSTRUCTION_TOKEN, LIT_TOKEN, "", 11},
   {NT_INSTRUCTION_TOKEN, MOV_TOKEN, "", 11},

   {NT_INSTRUCTION_TOKEN, EX2_TOKEN, "", 12},
   {NT_INSTRUCTION_TOKEN, EXP_TOKEN, "", 12},
   {NT_INSTRUCTION_TOKEN, LG2_TOKEN, "", 12},
   {NT_INSTRUCTION_TOKEN, LOG_TOKEN, "", 12},
   {NT_INSTRUCTION_TOKEN, RCP_TOKEN, "", 12},
   {NT_INSTRUCTION_TOKEN, RSQ_TOKEN, "", 12},

   {NT_INSTRUCTION_TOKEN, POW_TOKEN, "", 13},

   {NT_INSTRUCTION_TOKEN, ADD_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, DP3_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, DP4_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, DPH_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, DST_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, MAX_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, MIN_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, MUL_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, SGE_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, SLT_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, SUB_TOKEN, "", 14},
   {NT_INSTRUCTION_TOKEN, XPD_TOKEN, "", 14},

   {NT_INSTRUCTION_TOKEN, MAD_TOKEN, "", 15},

   {NT_INSTRUCTION_TOKEN, SWZ_TOKEN, "", 16},

   {NT_ARL_INSTRUCTION_TOKEN, ARL_TOKEN, "", 17},

   {NT_VECTOROP_INSTRUCTION_TOKEN, ABS_TOKEN, "", 18},
   {NT_VECTOROP_INSTRUCTION_TOKEN, FLR_TOKEN, "", 18},
   {NT_VECTOROP_INSTRUCTION_TOKEN, FRC_TOKEN, "", 18},
   {NT_VECTOROP_INSTRUCTION_TOKEN, LIT_TOKEN, "", 18},
   {NT_VECTOROP_INSTRUCTION_TOKEN, MOV_TOKEN, "", 18},

   {NT_VECTOROP_TOKEN, ABS_TOKEN, "", 19},

   {NT_VECTOROP_TOKEN, FLR_TOKEN, "", 20},

   {NT_VECTOROP_TOKEN, FRC_TOKEN, "", 21},

   {NT_VECTOROP_TOKEN, LIT_TOKEN, "", 22},

   {NT_VECTOROP_TOKEN, MOV_TOKEN, "", 23},

   {NT_SCALAROP_INSTRUCTION_TOKEN, EX2_TOKEN, "", 24},
   {NT_SCALAROP_INSTRUCTION_TOKEN, EXP_TOKEN, "", 24},
   {NT_SCALAROP_INSTRUCTION_TOKEN, LG2_TOKEN, "", 24},
   {NT_SCALAROP_INSTRUCTION_TOKEN, LOG_TOKEN, "", 24},
   {NT_SCALAROP_INSTRUCTION_TOKEN, RCP_TOKEN, "", 24},
   {NT_SCALAROP_INSTRUCTION_TOKEN, RSQ_TOKEN, "", 24},

   {NT_SCALAROP_TOKEN, EX2_TOKEN, "", 25},

   {NT_SCALAROP_TOKEN, EXP_TOKEN, "", 26},

   {NT_SCALAROP_TOKEN, LG2_TOKEN, "", 27},

   {NT_SCALAROP_TOKEN, LOG_TOKEN, "", 28},

   {NT_SCALAROP_TOKEN, RCP_TOKEN, "", 29},

   {NT_SCALAROP_TOKEN, RSQ_TOKEN, "", 30},

   {NT_BINSCOP_INSTRUCTION_TOKEN, POW_TOKEN, "", 31},

   {NT_BINSCOP_INSTRUCTION2_TOKEN, COMMA_TOKEN, "", 32},

   {NT_BINSCOP_TOKEN, POW_TOKEN, "", 33},

   {NT_BINOP_INSTRUCTION_TOKEN, ADD_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, DP3_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, DP4_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, DPH_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, DST_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, MAX_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, MIN_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, MUL_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, SGE_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, SLT_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, SUB_TOKEN, "", 34},
   {NT_BINOP_INSTRUCTION_TOKEN, XPD_TOKEN, "", 34},

   {NT_BINOP_INSTRUCTION2_TOKEN, COMMA_TOKEN, "", 35},

   {NT_BINOP_TOKEN, ADD_TOKEN, "", 36},
   {NT_BINOP_TOKEN, DP3_TOKEN, "", 37},
   {NT_BINOP_TOKEN, DP4_TOKEN, "", 38},
   {NT_BINOP_TOKEN, DPH_TOKEN, "", 39},
   {NT_BINOP_TOKEN, DST_TOKEN, "", 40},
   {NT_BINOP_TOKEN, MAX_TOKEN, "", 41},
   {NT_BINOP_TOKEN, MIN_TOKEN, "", 42},
   {NT_BINOP_TOKEN, MUL_TOKEN, "", 43},
   {NT_BINOP_TOKEN, SGE_TOKEN, "", 44},
   {NT_BINOP_TOKEN, SLT_TOKEN, "", 45},
   {NT_BINOP_TOKEN, SUB_TOKEN, "", 46},
   {NT_BINOP_TOKEN, XPD_TOKEN, "", 47},

   {NT_TRIOP_INSTRUCTION_TOKEN, MAD_TOKEN, "", 48},
   {NT_TRIOP_INSTRUCTION2_TOKEN, COMMA_TOKEN, "", 49},
   {NT_TRIOP_INSTRUCTION3_TOKEN, COMMA_TOKEN, "", 50},

   {NT_TRIOP_TOKEN, MAD_TOKEN, "", 51},
   {NT_SWZ_INSTRUCTION_TOKEN, SWZ_TOKEN, "", 52},
   {NT_SWZ_INSTRUCTION2_TOKEN, COMMA_TOKEN, "", 53},

   {NT_SCALAR_SRC_REG_TOKEN, PLUS_TOKEN, "", 54},
   {NT_SCALAR_SRC_REG_TOKEN, MINUS_TOKEN, "", 54},
   {NT_SCALAR_SRC_REG_TOKEN, VERTEX_TOKEN, "", 54},
   {NT_SCALAR_SRC_REG_TOKEN, STATE_TOKEN, "", 54},
   {NT_SCALAR_SRC_REG_TOKEN, PROGRAM_TOKEN, "", 54},
   {NT_SCALAR_SRC_REG_TOKEN, LBRACE_TOKEN, "", 54},
   {NT_SCALAR_SRC_REG_TOKEN, FLOAT_TOKEN, "-1", 54},
   {NT_SCALAR_SRC_REG_TOKEN, INTEGER_TOKEN, "-1", 54},
   {NT_SCALAR_SRC_REG_TOKEN, ID_TOKEN, "-1", 54},

   {NT_SWIZZLE_SRC_REG_TOKEN, PLUS_TOKEN, "", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, MINUS_TOKEN, "", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, VERTEX_TOKEN, "", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, STATE_TOKEN, "", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, PROGRAM_TOKEN, "", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, LBRACE_TOKEN, "", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, FLOAT_TOKEN, "-1", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, INTEGER_TOKEN, "-1", 55},
   {NT_SWIZZLE_SRC_REG_TOKEN, ID_TOKEN, "-1", 55},

   {NT_MASKED_DST_REG_TOKEN, ID_TOKEN, "-1", 56},
   {NT_MASKED_DST_REG_TOKEN, RESULT_TOKEN, "", 56},
   {NT_MASKED_ADDR_REG_TOKEN, ID_TOKEN, "-1", 57},

   {NT_EXTENDED_SWIZZLE_TOKEN, PLUS_TOKEN, "", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, MINUS_TOKEN, "", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, INTEGER_TOKEN, "0", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, INTEGER_TOKEN, "1", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, ID_TOKEN, "x", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, ID_TOKEN, "y", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, ID_TOKEN, "z", 58},
   {NT_EXTENDED_SWIZZLE_TOKEN, ID_TOKEN, "w", 58},

   {NT_EXTENDED_SWIZZLE2_TOKEN, COMMA_TOKEN, "", 59},

   {NT_EXT_SWIZ_COMP_TOKEN, PLUS_TOKEN, "", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, MINUS_TOKEN, "", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, INTEGER_TOKEN, "0", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, INTEGER_TOKEN, "1", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, ID_TOKEN, "x", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, ID_TOKEN, "y", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, ID_TOKEN, "z", 60},
   {NT_EXT_SWIZ_COMP_TOKEN, ID_TOKEN, "w", 60},

   {NT_EXT_SWIZ_SEL_TOKEN, INTEGER_TOKEN, "0", 61},
   {NT_EXT_SWIZ_SEL_TOKEN, INTEGER_TOKEN, "1", 62},
   {NT_EXT_SWIZ_SEL_TOKEN, ID_TOKEN, "x", 63},
   {NT_EXT_SWIZ_SEL_TOKEN, ID_TOKEN, "y", 63},
   {NT_EXT_SWIZ_SEL_TOKEN, ID_TOKEN, "z", 63},
   {NT_EXT_SWIZ_SEL_TOKEN, ID_TOKEN, "w", 63},

   /* Special case for 64 - 68 */

   {NT_DST_REG_TOKEN, RESULT_TOKEN, "", 68},
   {NT_VERTEX_ATTRIB_REG_TOKEN, ID_TOKEN, "-1", 69},
   {NT_VERTEX_ATTRIB_REG_TOKEN, VERTEX_TOKEN, "", 70},
   {NT_TEMPORARY_REG_TOKEN, ID_TOKEN, "-1", 71},

   /* Special case for 72 - 73 */

   {NT_PROG_PARAM_REG_TOKEN, STATE_TOKEN, "", 74},
   {NT_PROG_PARAM_REG_TOKEN, PROGRAM_TOKEN, "", 74},
   {NT_PROG_PARAM_REG_TOKEN, LBRACE_TOKEN, "", 74},
   {NT_PROG_PARAM_REG_TOKEN, FLOAT_TOKEN, "-1", 74},

   {NT_PROG_PARAM_SINGLE_TOKEN, ID_TOKEN, "-1", 75},
   {NT_PROG_PARAM_ARRAY_TOKEN, ID_TOKEN, "-1", 76},
   {NT_PROG_PARAM_ARRAY_MEM_TOKEN, INTEGER_TOKEN, "-1", 77},
   {NT_PROG_PARAM_ARRAY_MEM_TOKEN, ID_TOKEN, "-1", 78},
   {NT_PROG_PARAM_ARRAY_ABS_TOKEN, INTEGER_TOKEN, "-1", 79},
   {NT_PROG_PARAM_ARRAY_REL_TOKEN, ID_TOKEN, "-1", 80},

   {NT_ADDR_REG_REL_OFFSET_TOKEN, RBRACKET_TOKEN, "", 81},
   {NT_ADDR_REG_REL_OFFSET_TOKEN, PLUS_TOKEN, "", 82},
   {NT_ADDR_REG_REL_OFFSET_TOKEN, MINUS_TOKEN, "", 83},
   {NT_ADDR_REG_POS_OFFSET_TOKEN, INTEGER_TOKEN, "-1", 84},
   {NT_ADDR_REG_NEG_OFFSET_TOKEN, INTEGER_TOKEN, "-1", 85},


   {NT_VERTEX_RESULT_REG_TOKEN, ID_TOKEN, "-1", 86},
   {NT_VERTEX_RESULT_REG_TOKEN, RESULT_TOKEN, "", 87},
   {NT_ADDR_REG_TOKEN, ID_TOKEN, "-1", 88},
   {NT_ADDR_COMPONENT_TOKEN, PERIOD_TOKEN, "", 89},
   {NT_ADDR_WRITE_MASK_TOKEN, PERIOD_TOKEN, "", 90},

   {NT_SCALAR_SUFFIX_TOKEN, PERIOD_TOKEN, "", 91},

   {NT_SWIZZLE_SUFFIX_TOKEN, COMMA_TOKEN, "", 92},
   {NT_SWIZZLE_SUFFIX_TOKEN, SEMICOLON_TOKEN, "", 92},
   {NT_SWIZZLE_SUFFIX_TOKEN, PERIOD_TOKEN, "", 317},

   {NT_COMPONENT_TOKEN, ID_TOKEN, "x", 93},
   {NT_COMPONENT_TOKEN, ID_TOKEN, "y", 94},
   {NT_COMPONENT_TOKEN, ID_TOKEN, "z", 95},
   {NT_COMPONENT_TOKEN, ID_TOKEN, "w", 96},

   {NT_OPTIONAL_MASK_TOKEN, PERIOD_TOKEN, "", 97},
   {NT_OPTIONAL_MASK_TOKEN, COMMA_TOKEN, "", 98},

   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "x", 99},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "y", 100},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xy", 101},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "z", 102},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xz", 103},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "yz", 104},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xyz", 105},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "w", 106},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xw", 107},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "yw", 108},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xyw", 109},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "zw", 110},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xzw", 111},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "yzw", 112},
   {NT_OPTIONAL_MASK2_TOKEN, ID_TOKEN, "xyzw", 113},


   {NT_NAMING_STATEMENT_TOKEN, ATTRIB_TOKEN, "", 114},
   {NT_NAMING_STATEMENT_TOKEN, PARAM_TOKEN, "", 115},
   {NT_NAMING_STATEMENT_TOKEN, TEMP_TOKEN, "", 116},
   {NT_NAMING_STATEMENT_TOKEN, ADDRESS_TOKEN, "", 117},
   {NT_NAMING_STATEMENT_TOKEN, OUTPUT_TOKEN, "", 118},
   {NT_NAMING_STATEMENT_TOKEN, ALIAS_TOKEN, "", 119},

   {NT_ATTRIB_STATEMENT_TOKEN, ATTRIB_TOKEN, "", 120},
   {NT_VTX_ATTRIB_BINDING_TOKEN, VERTEX_TOKEN, "", 121},

   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "position", 122},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "weight", 123},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "normal", 124},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "color", 125},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "fogcoord", 126},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "texcoord", 127},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "matrixindex", 128},
   {NT_VTX_ATTRIB_ITEM_TOKEN, ID_TOKEN, "attrib", 129},

   {NT_VTX_ATTRIB_NUM_TOKEN, INTEGER_TOKEN, "-1", 130},
   {NT_VTX_OPT_WEIGHT_NUM_TOKEN, SEMICOLON_TOKEN, "", 131},
   {NT_VTX_OPT_WEIGHT_NUM_TOKEN, COMMA_TOKEN, "", 131},
   {NT_VTX_OPT_WEIGHT_NUM_TOKEN, PERIOD_TOKEN, "", 131},
   {NT_VTX_OPT_WEIGHT_NUM_TOKEN, LBRACKET_TOKEN, "", 132},

   {NT_VTX_WEIGHT_NUM_TOKEN, INTEGER_TOKEN, "-1", 133},
   {NT_PARAM_STATEMENT_TOKEN, PARAM_TOKEN, "", 134},
   {NT_PARAM_STATEMENT2_TOKEN, EQUAL_TOKEN, "", 135},
   {NT_PARAM_STATEMENT2_TOKEN, LBRACKET_TOKEN, "", 136},

   {NT_OPT_ARRAY_SIZE_TOKEN, RBRACKET_TOKEN, "", 137},
   {NT_OPT_ARRAY_SIZE_TOKEN, INTEGER_TOKEN, "-1", 138},

   {NT_PARAM_SINGLE_INIT_TOKEN, EQUAL_TOKEN, "", 139},
   {NT_PARAM_MULTIPLE_INIT_TOKEN, EQUAL_TOKEN, "", 140},

   {NT_PARAM_MULT_INIT_LIST_TOKEN, STATE_TOKEN, "", 141},
   {NT_PARAM_MULT_INIT_LIST_TOKEN, PROGRAM_TOKEN, "", 141},
   {NT_PARAM_MULT_INIT_LIST_TOKEN, PLUS_TOKEN, "", 141},
   {NT_PARAM_MULT_INIT_LIST_TOKEN, MINUS_TOKEN, "", 141},
   {NT_PARAM_MULT_INIT_LIST_TOKEN, FLOAT_TOKEN, "-1", 141},
   {NT_PARAM_MULT_INIT_LIST_TOKEN, INTEGER_TOKEN, "-1", 141},
   {NT_PARAM_MULT_INIT_LIST_TOKEN, LBRACE_TOKEN, "", 141},


   {NT_PARAM_MULT_INIT_LIST2_TOKEN, COMMA_TOKEN, "", 142},
   {NT_PARAM_MULT_INIT_LIST2_TOKEN, RBRACE_TOKEN, "", 143},


   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, STATE_TOKEN, "", 144},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, PROGRAM_TOKEN, "", 145},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, PLUS_TOKEN, "", 146},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, MINUS_TOKEN, "", 146},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, FLOAT_TOKEN, "-1", 146},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, INTEGER_TOKEN, "-1", 146},
   {NT_PARAM_SINGLE_ITEM_DECL_TOKEN, LBRACE_TOKEN, "", 146},


   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, STATE_TOKEN, "", 147},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, PROGRAM_TOKEN, "", 148},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, LBRACE_TOKEN, "", 149},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, FLOAT_TOKEN, "-1", 149},
   {NT_PARAM_SINGLE_ITEM_USE_TOKEN, INTEGER_TOKEN, "-1", 149},

   {NT_PARAM_MULTIPLE_ITEM_TOKEN, STATE_TOKEN, "", 150},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, PROGRAM_TOKEN, "", 151},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, PLUS_TOKEN, "", 152},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, MINUS_TOKEN, "", 152},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, FLOAT_TOKEN, "-1", 152},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, INTEGER_TOKEN, "-1", 152},
   {NT_PARAM_MULTIPLE_ITEM_TOKEN, LBRACE_TOKEN, "", 152},

   {NT_STATE_MULTIPLE_ITEM_TOKEN, STATE_TOKEN, "", 153},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "material", 154},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "light", 155},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "lightmodel", 156},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "lightprod", 157},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "texgen", 158},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "fog", 159},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "clip", 160},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "point", 161},
   {NT_STATE_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "matrix", 162},

   {NT_FOO_TOKEN, PERIOD_TOKEN, "", 163},
   {NT_FOO_TOKEN, COMMA_TOKEN, "", 316},
   {NT_FOO_TOKEN, RBRACE_TOKEN, "", 316},
   {NT_FOO2_TOKEN, ID_TOKEN, "inverse", 164},
   {NT_FOO2_TOKEN, ID_TOKEN, "transpose", 164},
   {NT_FOO2_TOKEN, ID_TOKEN, "invtrans", 164},
   {NT_FOO2_TOKEN, ID_TOKEN, "row", 165},
   {NT_FOO3_TOKEN, COMMA_TOKEN, "", 166},
   {NT_FOO3_TOKEN, RBRACE_TOKEN, "", 166},
   {NT_FOO3_TOKEN, PERIOD_TOKEN, "", 167},

   {NT_FOO35_TOKEN, INTEGER_TOKEN, "-1", 168},
   {NT_FOO4_TOKEN, RBRACKET_TOKEN, "", 169},
   {NT_FOO4_TOKEN, DOTDOT_TOKEN, "", 170},

   {NT_STATE_SINGLE_ITEM_TOKEN, STATE_TOKEN, "", 171},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "material", 172},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "light", 173},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "lightmodel", 174},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "lightprod", 175},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "texgen", 176},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "fog", 177},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "clip", 178},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "point", 179},
   {NT_STATE_SINGLE_ITEM2_TOKEN, ID_TOKEN, "matrix", 180},


   {NT_STATE_MATERIAL_ITEM_TOKEN, ID_TOKEN, "material", 181},
   {NT_STATE_MATERIAL_ITEM2_TOKEN, ID_TOKEN, "ambient", 182},
   {NT_STATE_MATERIAL_ITEM2_TOKEN, ID_TOKEN, "diffuse", 182},
   {NT_STATE_MATERIAL_ITEM2_TOKEN, ID_TOKEN, "specular", 182},
   {NT_STATE_MATERIAL_ITEM2_TOKEN, ID_TOKEN, "emission", 182},
   {NT_STATE_MATERIAL_ITEM2_TOKEN, ID_TOKEN, "shininess", 182},

   {NT_STATE_MATERIAL_ITEM2_TOKEN, PERIOD_TOKEN, "", 183},
   {NT_STATE_MAT_PROPERTY_TOKEN, ID_TOKEN, "ambient", 184},
   {NT_STATE_MAT_PROPERTY_TOKEN, ID_TOKEN, "diffuse", 185},
   {NT_STATE_MAT_PROPERTY_TOKEN, ID_TOKEN, "specular", 186},
   {NT_STATE_MAT_PROPERTY_TOKEN, ID_TOKEN, "emission", 187},
   {NT_STATE_MAT_PROPERTY_TOKEN, ID_TOKEN, "shininess", 188},


   {NT_STATE_LIGHT_ITEM_TOKEN, ID_TOKEN, "light", 189},
   {NT_STATE_LIGHT_ITEM2_TOKEN, RBRACKET_TOKEN, "", 190},


   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "ambient", 191},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "diffuse", 192},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "specular", 193},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "position", 194},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "attenuation", 195},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "spot", 196},
   {NT_STATE_LIGHT_PROPERTY_TOKEN, ID_TOKEN, "half", 197},

   {NT_STATE_SPOT_PROPERTY_TOKEN, ID_TOKEN, "direction", 198},
   {NT_STATE_LIGHT_MODEL_ITEM_TOKEN, ID_TOKEN, "lightmodel", 199},


   {NT_STATE_LMOD_PROPERTY_TOKEN, PERIOD_TOKEN, "", 200},
   {NT_STATE_LMOD_PROPERTY2_TOKEN, ID_TOKEN, "ambient", 201},
   {NT_STATE_LMOD_PROPERTY2_TOKEN, ID_TOKEN, "scenecolor", 202},
   {NT_STATE_LMOD_PROPERTY2_TOKEN, ID_TOKEN, "front", 203},
   {NT_STATE_LMOD_PROPERTY2_TOKEN, ID_TOKEN, "back", 203},


   {NT_STATE_LIGHT_PROD_ITEM_TOKEN, ID_TOKEN, "lightprod", 204},
   {NT_STATE_LIGHT_PROD_ITEM15_TOKEN, RBRACKET_TOKEN, "", 205},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN, ID_TOKEN, "ambient", 206},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN, ID_TOKEN, "diffuse", 206},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN, ID_TOKEN, "specular", 206},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN, ID_TOKEN, "front", 207},
   {NT_STATE_LIGHT_PROD_ITEM2_TOKEN, ID_TOKEN, "back", 207},

   {NT_STATE_LPROD_PROPERTY_TOKEN, ID_TOKEN, "ambient", 208},
   {NT_STATE_LPROD_PROPERTY_TOKEN, ID_TOKEN, "diffuse", 209},
   {NT_STATE_LPROD_PROPERTY_TOKEN, ID_TOKEN, "specular", 210},

   {NT_STATE_LIGHT_NUMBER_TOKEN, INTEGER_TOKEN, "-1", 211},
   {NT_STATE_TEX_GEN_ITEM_TOKEN, ID_TOKEN, "texgen", 212},
   {NT_STATE_TEX_GEN_ITEM2_TOKEN, PERIOD_TOKEN, "", 213},
   {NT_STATE_TEX_GEN_TYPE_TOKEN, ID_TOKEN, "eye", 214},
   {NT_STATE_TEX_GEN_TYPE_TOKEN, ID_TOKEN, "object", 215},


   {NT_STATE_TEX_GEN_COORD_TOKEN, ID_TOKEN, "s", 216},
   {NT_STATE_TEX_GEN_COORD_TOKEN, ID_TOKEN, "t", 217},
   {NT_STATE_TEX_GEN_COORD_TOKEN, ID_TOKEN, "r", 218},
   {NT_STATE_TEX_GEN_COORD_TOKEN, ID_TOKEN, "q", 219},

   {NT_STATE_FOG_ITEM_TOKEN, ID_TOKEN, "fog", 220},

   {NT_STATE_FOG_PROPERTY_TOKEN, ID_TOKEN, "color", 221},
   {NT_STATE_FOG_PROPERTY_TOKEN, ID_TOKEN, "params", 222},

   {NT_STATE_CLIP_PLANE_ITEM_TOKEN, ID_TOKEN, "clip", 223},
   {NT_STATE_CLIP_PLANE_ITEM2_TOKEN, RBRACKET_TOKEN, "", 224},
   {NT_STATE_CLIP_PLANE_NUM_TOKEN, INTEGER_TOKEN, "-1", 225},
   {NT_STATE_POINT_ITEM_TOKEN, ID_TOKEN, "point", 226},
   {NT_STATE_POINT_PROPERTY_TOKEN, ID_TOKEN, "size", 227},
   {NT_STATE_POINT_PROPERTY_TOKEN, ID_TOKEN, "attenuation", 228},


   {NT_STATE_MATRIX_ROW_TOKEN, ID_TOKEN, "matrix", 229},
   {NT_STATE_MATRIX_ROW15_TOKEN, PERIOD_TOKEN, "", 230},
   {NT_STATE_MATRIX_ROW2_TOKEN, ID_TOKEN, "row", 231},
   {NT_STATE_MATRIX_ROW2_TOKEN, ID_TOKEN, "inverse", 232},
   {NT_STATE_MATRIX_ROW2_TOKEN, ID_TOKEN, "transpose", 232},
   {NT_STATE_MATRIX_ROW2_TOKEN, ID_TOKEN, "invtrans", 232},
   {NT_STATE_MATRIX_ROW3_TOKEN, LBRACKET_TOKEN, "", 233},

   {NT_STATE_MAT_MODIFIER_TOKEN, ID_TOKEN, "inverse", 234},
   {NT_STATE_MAT_MODIFIER_TOKEN, ID_TOKEN, "transpose", 235},
   {NT_STATE_MAT_MODIFIER_TOKEN, ID_TOKEN, "invtrans", 236},
   {NT_STATE_MATRIX_ROW_NUM_TOKEN, INTEGER_TOKEN, "-1", 237},


   {NT_STATE_MATRIX_NAME_TOKEN, ID_TOKEN, "modelview", 238},
   {NT_STATE_MATRIX_NAME_TOKEN, ID_TOKEN, "projection", 239},
   {NT_STATE_MATRIX_NAME_TOKEN, ID_TOKEN, "mvp", 240},
   {NT_STATE_MATRIX_NAME_TOKEN, ID_TOKEN, "texture", 241},
   {NT_STATE_MATRIX_NAME_TOKEN, ID_TOKEN, "palette", 242},
   {NT_STATE_MATRIX_NAME_TOKEN, PROGRAM_TOKEN, "", 243},

   {NT_STATE_OPT_MOD_MAT_NUM_TOKEN, PERIOD_TOKEN, "", 244},
   {NT_STATE_OPT_MOD_MAT_NUM_TOKEN, COMMA_TOKEN, "", 244},
   {NT_STATE_OPT_MOD_MAT_NUM_TOKEN, RBRACE_TOKEN, "", 244},
   {NT_STATE_OPT_MOD_MAT_NUM_TOKEN, LBRACKET_TOKEN, "", 245},

   {NT_STATE_MOD_MAT_NUM_TOKEN, INTEGER_TOKEN, "-1", 246},
   {NT_STATE_PALETTE_MAT_NUM_TOKEN, INTEGER_TOKEN, "-1", 247},
   {NT_STATE_PROGRAM_MAT_NUM_TOKEN, INTEGER_TOKEN, "-1", 248},

   {NT_PROGRAM_SINGLE_ITEM_TOKEN, PROGRAM_TOKEN, "", 249},
   {NT_PROGRAM_SINGLE_ITEM2_TOKEN, ID_TOKEN, "env", 250},
   {NT_PROGRAM_SINGLE_ITEM2_TOKEN, ID_TOKEN, "local", 251},


   {NT_PROGRAM_MULTIPLE_ITEM_TOKEN, PROGRAM_TOKEN, "", 252},
   {NT_PROGRAM_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "env", 253},
   {NT_PROGRAM_MULTIPLE_ITEM2_TOKEN, ID_TOKEN, "local", 254},
   {NT_PROG_ENV_PARAMS_TOKEN, ID_TOKEN, "env", 255},
   {NT_PROG_ENV_PARAM_NUMS_TOKEN, INTEGER_TOKEN, "-1", 256},
   {NT_PROG_ENV_PARAM_NUMS2_TOKEN, DOTDOT_TOKEN, "", 257},
   {NT_PROG_ENV_PARAM_NUMS2_TOKEN, RBRACKET_TOKEN, "", 258},
   {NT_PROG_ENV_PARAM_TOKEN, ID_TOKEN, "env", 259},

   {NT_PROG_LOCAL_PARAMS_TOKEN, ID_TOKEN, "local", 260},
   {NT_PROG_LOCAL_PARAM_NUMS_TOKEN, INTEGER_TOKEN, "-1", 261},
   {NT_PROG_LOCAL_PARAM_NUMS2_TOKEN, DOTDOT_TOKEN, "", 262},
   {NT_PROG_LOCAL_PARAM_NUMS2_TOKEN, RBRACKET_TOKEN, "", 263},
   {NT_PROG_LOCAL_PARAM_TOKEN, ID_TOKEN, "local", 264},
   {NT_PROG_ENV_PARAM_NUM_TOKEN, INTEGER_TOKEN, "-1", 265},
   {NT_PROG_LOCAL_PARAM_NUM_TOKEN, INTEGER_TOKEN, "-1", 266},
   {NT_PARAM_CONST_DECL_TOKEN, PLUS_TOKEN, "", 267},
   {NT_PARAM_CONST_DECL_TOKEN, MINUS_TOKEN, "", 267},
   {NT_PARAM_CONST_DECL_TOKEN, FLOAT_TOKEN, "-1", 267},
   {NT_PARAM_CONST_DECL_TOKEN, INTEGER_TOKEN, "-1", 267},
   {NT_PARAM_CONST_DECL_TOKEN, LBRACE_TOKEN, "", 268},

   {NT_PARAM_CONST_USE_TOKEN, FLOAT_TOKEN, "-1", 269},
   {NT_PARAM_CONST_USE_TOKEN, INTEGER_TOKEN, "-1", 269},
   {NT_PARAM_CONST_USE_TOKEN, LBRACE_TOKEN, "", 270},


   {NT_PARAM_CONST_SCALAR_DECL_TOKEN, PLUS_TOKEN, "", 271},
   {NT_PARAM_CONST_SCALAR_DECL_TOKEN, MINUS_TOKEN, "", 271},
   {NT_PARAM_CONST_SCALAR_DECL_TOKEN, FLOAT_TOKEN, "-1", 271},
   {NT_PARAM_CONST_SCALAR_DECL_TOKEN, INTEGER_TOKEN, "-1", 271},

   {NT_PARAM_CONST_SCALAR_USE_TOKEN, FLOAT_TOKEN, "-1", 272},
   {NT_PARAM_CONST_SCALAR_USE_TOKEN, INTEGER_TOKEN, "-1", 272},
   {NT_PARAM_CONST_VECTOR_TOKEN, LBRACE_TOKEN, "", 273},
   {NT_PARAM_CONST_VECTOR2_TOKEN, RBRACE_TOKEN, "", 274},
   {NT_PARAM_CONST_VECTOR2_TOKEN, COMMA_TOKEN, "", 275},
   {NT_PARAM_CONST_VECTOR3_TOKEN, RBRACE_TOKEN, "", 276},
   {NT_PARAM_CONST_VECTOR3_TOKEN, COMMA_TOKEN, "", 277},
   {NT_PARAM_CONST_VECTOR4_TOKEN, RBRACE_TOKEN, "", 278},
   {NT_PARAM_CONST_VECTOR4_TOKEN, COMMA_TOKEN, "", 279},

   {NT_SIGNED_FLOAT_CONSTANT_TOKEN, PLUS_TOKEN, "", 280},
   {NT_SIGNED_FLOAT_CONSTANT_TOKEN, MINUS_TOKEN, "", 280},
   {NT_SIGNED_FLOAT_CONSTANT_TOKEN, FLOAT_TOKEN, "-1", 280},
   {NT_SIGNED_FLOAT_CONSTANT_TOKEN, INTEGER_TOKEN, "-1", 280},

   {NT_OPTIONAL_SIGN_TOKEN, FLOAT_TOKEN, "-1", 281},
   {NT_OPTIONAL_SIGN_TOKEN, INTEGER_TOKEN, "0", 281},
   {NT_OPTIONAL_SIGN_TOKEN, INTEGER_TOKEN, "1", 281},
   {NT_OPTIONAL_SIGN_TOKEN, INTEGER_TOKEN, "-1", 281},
   {NT_OPTIONAL_SIGN_TOKEN, ID_TOKEN, "-1", 281},
   {NT_OPTIONAL_SIGN_TOKEN, STATE_TOKEN, "", 281},
   {NT_OPTIONAL_SIGN_TOKEN, PROGRAM_TOKEN, "", 281},
   {NT_OPTIONAL_SIGN_TOKEN, VERTEX_TOKEN, "", 281},
   {NT_OPTIONAL_SIGN_TOKEN, LBRACE_TOKEN, "", 281},


   {NT_OPTIONAL_SIGN_TOKEN, MINUS_TOKEN, "", 282},
   {NT_OPTIONAL_SIGN_TOKEN, PLUS_TOKEN, "", 283},

   {NT_TEMP_STATEMENT_TOKEN, TEMP_TOKEN, "", 284},
   {NT_ADDRESS_STATEMENT_TOKEN, ADDRESS_TOKEN, "", 285},

   /* Special case 286-7 */

   {NT_OUTPUT_STATEMENT_TOKEN, OUTPUT_TOKEN, "", 288},
   {NT_RESULT_BINDING_TOKEN, RESULT_TOKEN, "", 289},
   {NT_RESULT_BINDING2_TOKEN, ID_TOKEN, "position", 290},
   {NT_RESULT_BINDING2_TOKEN, ID_TOKEN, "fogcoord", 291},
   {NT_RESULT_BINDING2_TOKEN, ID_TOKEN, "pointsize", 292},
   {NT_RESULT_BINDING2_TOKEN, ID_TOKEN, "color", 293},
   {NT_RESULT_BINDING2_TOKEN, ID_TOKEN, "texcoord", 294},

   {NT_RESULT_COL_BINDING_TOKEN, ID_TOKEN, "color", 295},

   /* Special case 296-7 */

   {NT_RESULT_COL_BINDING3_TOKEN, ID_TOKEN, "front", 298},
   {NT_RESULT_COL_BINDING3_TOKEN, ID_TOKEN, "back", 299},

   /* Special case 300-301 */

   {NT_RESULT_COL_BINDING5_TOKEN, ID_TOKEN, "primary", 302},
   {NT_RESULT_COL_BINDING5_TOKEN, ID_TOKEN, "secondary", 303},
   {NT_OPT_FACE_TYPE2_TOKEN, ID_TOKEN, "front", 304},
   {NT_OPT_FACE_TYPE2_TOKEN, ID_TOKEN, "back", 305},

   /* Special case 306-7 */

   {NT_OPT_COLOR_TYPE2_TOKEN, ID_TOKEN, "primary", 308},
   {NT_OPT_COLOR_TYPE2_TOKEN, ID_TOKEN, "secondary", 309},

   {NT_OPT_TEX_COORD_NUM_TOKEN, PERIOD_TOKEN, "", 310},
   {NT_OPT_TEX_COORD_NUM_TOKEN, SEMICOLON_TOKEN, "", 310},
   {NT_OPT_TEX_COORD_NUM_TOKEN, COMMA_TOKEN, "", 310},
   {NT_OPT_TEX_COORD_NUM_TOKEN, RBRACE_TOKEN, "", 310},
   {NT_OPT_TEX_COORD_NUM_TOKEN, LBRACKET_TOKEN, "", 311},

   {NT_TEX_COORD_NUM_TOKEN, INTEGER_TOKEN, "-1", 312},

   /* Special case for 313 to get aliasing correct */

   {NT_ESTABLISH_NAME_TOKEN, ID_TOKEN, "-1", 314},
   {NT_ESTABLISHED_NAME_TOKEN, ID_TOKEN, "-1", 315},

   /* Special case for 318-9 */
};

static GLint nprods = sizeof(ptab) / sizeof(prod_table);
static GLint nlas = sizeof(latab) / sizeof(look_ahead_table);

/** 
 * This is a gigantic FSA to recognize the keywords put forth in the
 * GL_ARB_vertex_program spec. 
 *
 * All other tokens are deemed 'identifiers', and shoved into the 
 * identifier symbol table (as opposed to the GLint and GLfloat symbol tables)
 *
 * \param s				The parse state
 * \param token		The next token seen is returned in this value
 * \param token_attr	The token attribute for the next token is returned here. This
 *                    is the index into the approprate symbol table if token is INTEGER_TOKEN,
 *                    FLOAT_TOKEN, or ID_TOKEN
 *                   
 * \return ARB_VP_ERROR on lex error, ARB_VP_SUCESS on sucess
 */
static GLint
get_next_token(parse_state * s, GLint * token, GLint * token_attr)
{
   GLubyte curr;

   while (s->start_pos < s->len) {
      curr = s->str[s->curr_pos];

      switch (s->curr_state) {
	 /* Default state */
      case STATE_BASE:
	 if (IS_WHITESPACE(curr)) {
	    s->start_pos++;
	    s->curr_pos++;
	 }
	 else {
	    if (IS_IDCHAR(curr)) {
	       switch (curr) {
	       case 'A':
		  ADV_TO_STATE(STATE_A);
		  break;

	       case 'D':
		  ADV_TO_STATE(STATE_D);
		  break;

	       case 'E':
		  ADV_TO_STATE(STATE_E);
		  break;

	       case 'F':
		  ADV_TO_STATE(STATE_F);
		  break;

	       case 'L':
		  ADV_TO_STATE(STATE_L);
		  break;

	       case 'M':
		  ADV_TO_STATE(STATE_M);
		  break;

	       case 'O':
		  ADV_TO_STATE(STATE_O);
		  break;

	       case 'P':
		  ADV_TO_STATE(STATE_P);
		  break;

	       case 'R':
		  ADV_TO_STATE(STATE_R);
		  break;

	       case 'S':
		  ADV_TO_STATE(STATE_S);
		  break;

	       case 'T':
		  ADV_TO_STATE(STATE_T);
		  break;

	       case 'X':
		  ADV_TO_STATE(STATE_X);
		  break;

	       case 'p':
		  ADV_TO_STATE(STATE_LC_P);
		  break;

	       case 'r':
		  ADV_TO_STATE(STATE_LC_R);
		  break;

	       case 's':
		  ADV_TO_STATE(STATE_LC_S);
		  break;

	       case 'v':
		  ADV_TO_STATE(STATE_LC_V);
		  break;

	       default:
		  s->curr_state = 1;
		  s->start_pos = s->curr_pos;
		  s->curr_pos++;
	       }
	    }
	    else if (IS_DIGIT(curr)) {
	       ADV_TO_STATE(STATE_N4);
	    }
	    else {
	       switch (curr) {
	       case '#':
		  ADV_TO_STATE(STATE_COMMENT);
		  break;
	       case ';':
		  ADV_AND_FINISH(SEMICOLON_TOKEN);
		  break;
	       case ',':
		  ADV_AND_FINISH(COMMA_TOKEN);
		  break;
	       case '+':
		  ADV_AND_FINISH(PLUS_TOKEN);
		  break;
	       case '-':
		  ADV_AND_FINISH(MINUS_TOKEN);
		  break;
	       case '[':
		  ADV_AND_FINISH(LBRACKET_TOKEN);
		  break;
	       case ']':
		  ADV_AND_FINISH(RBRACKET_TOKEN);
		  break;
	       case '{':
		  ADV_AND_FINISH(LBRACE_TOKEN);
		  break;
	       case '}':
		  ADV_AND_FINISH(RBRACE_TOKEN);
		  break;
	       case '=':
		  ADV_AND_FINISH(EQUAL_TOKEN);
		  break;

	       case '.':
		  ADV_TO_STATE(STATE_N1);
		  break;

	       default:
		  return ARB_VP_ERROR;
		  break;
	       }
	    }
	 }
	 break;


	 /* Main identifier state */
      case STATE_IDENT:
	 if (IS_CD(curr)) {
	    s->curr_pos++;
	 }
	 else {
	    *token = ID_TOKEN;
	    *token_attr =
	       id_table_add(&s->idents, s->str, s->start_pos, s->curr_pos);

	    s->curr_state = 0;
	    s->start_pos = s->curr_pos;
	    return ARB_VP_SUCESS;
	 }
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the A* keywords
	  */
      case STATE_A:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'B':
	       ADV_TO_STATE(STATE_AB);
	       break;
	    case 'D':
	       ADV_TO_STATE(STATE_AD);
	       break;
	    case 'L':
	       ADV_TO_STATE(STATE_AL);
	       break;
	    case 'R':
	       ADV_TO_STATE(STATE_AR);
	       break;
	    case 'T':
	       ADV_TO_STATE(STATE_AT);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_AB:
	 ADV_OR_FALLBACK('S', STATE_ABS);
	 break;

      case STATE_ABS:
	 FINISH_OR_FALLBACK(ABS_TOKEN);
	 break;

      case STATE_AD:
	 ADV_OR_FALLBACK('D', STATE_ADD);
	 break;

      case STATE_ADD:
	 if (curr == 'R') {
	    ADV_TO_STATE(STATE_ADDR);
	 }
	 else if (IS_CD(curr)) {
	    ADV_TO_STATE(STATE_IDENT);
	 }
	 else {
	    FINISH(ADD_TOKEN);
	 }
	 break;

      case STATE_ADDR:
	 ADV_OR_FALLBACK('E', STATE_ADDRE);
	 break;

      case STATE_ADDRE:
	 ADV_OR_FALLBACK('S', STATE_ADDRES);
	 break;

      case STATE_ADDRES:
	 ADV_OR_FALLBACK('S', STATE_ADDRESS);
	 break;

      case STATE_ADDRESS:
	 FINISH_OR_FALLBACK(ADDRESS_TOKEN);
	 break;

      case STATE_AL:
	 ADV_OR_FALLBACK('I', STATE_ALI);
	 break;

      case STATE_ALI:
	 ADV_OR_FALLBACK('A', STATE_ALIA);
	 break;

      case STATE_ALIA:
	 ADV_OR_FALLBACK('S', STATE_ALIAS);
	 break;

      case STATE_ALIAS:
	 FINISH_OR_FALLBACK(ALIAS_TOKEN);
	 break;

      case STATE_AR:
	 ADV_OR_FALLBACK('L', STATE_ARL);
	 break;

      case STATE_ARL:
	 FINISH_OR_FALLBACK(ARL_TOKEN);
	 break;

      case STATE_AT:
	 ADV_OR_FALLBACK('T', STATE_ATT);
	 break;

      case STATE_ATT:
	 ADV_OR_FALLBACK('R', STATE_ATTR);
	 break;

      case STATE_ATTR:
	 ADV_OR_FALLBACK('I', STATE_ATTRI);
	 break;

      case STATE_ATTRI:
	 ADV_OR_FALLBACK('B', STATE_ATTRIB);
	 break;

      case STATE_ATTRIB:
	 FINISH_OR_FALLBACK(ATTRIB_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the D* keywords
	  */
      case STATE_D:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'P':
	       ADV_TO_STATE(STATE_DP);
	       break;
	    case 'S':
	       ADV_TO_STATE(STATE_DS);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_DP:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case '3':
	       ADV_TO_STATE(STATE_DP3);
	       break;
	    case '4':
	       ADV_TO_STATE(STATE_DP4);
	       break;
	    case 'H':
	       ADV_TO_STATE(STATE_DPH);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_DP3:
	 FINISH_OR_FALLBACK(DP3_TOKEN);
	 break;

      case STATE_DP4:
	 FINISH_OR_FALLBACK(DP4_TOKEN);
	 break;

      case STATE_DPH:
	 FINISH_OR_FALLBACK(DPH_TOKEN);
	 break;

      case STATE_DS:
	 ADV_OR_FALLBACK('T', STATE_DST);
	 break;

      case STATE_DST:
	 FINISH_OR_FALLBACK(DST_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the E* keywords
	  */
      case STATE_E:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'N':
	       ADV_TO_STATE(STATE_EN);
	       break;
	    case 'X':
	       ADV_TO_STATE(STATE_EX);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_EN:
	 ADV_OR_FALLBACK('D', STATE_END);
	 break;

      case STATE_END:
	 FINISH_OR_FALLBACK(END_TOKEN);
	 break;

      case STATE_EX:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case '2':
	       ADV_TO_STATE(STATE_EX2);
	       break;
	    case 'P':
	       ADV_TO_STATE(STATE_EXP);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_EX2:
	 FINISH_OR_FALLBACK(EX2_TOKEN);
	 break;

      case STATE_EXP:
	 FINISH_OR_FALLBACK(EXP_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the F* keywords
	  */
      case STATE_F:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'L':
	       ADV_TO_STATE(STATE_FL);
	       break;
	    case 'R':
	       ADV_TO_STATE(STATE_FR);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_FL:
	 ADV_OR_FALLBACK('R', STATE_FLR);
	 break;

      case STATE_FLR:
	 FINISH_OR_FALLBACK(FLR_TOKEN);
	 break;

      case STATE_FR:
	 ADV_OR_FALLBACK('C', STATE_FRC);
	 break;

      case STATE_FRC:
	 FINISH_OR_FALLBACK(FRC_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the L* keywords
	  */
      case STATE_L:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'G':
	       ADV_TO_STATE(STATE_LG);
	       break;
	    case 'I':
	       ADV_TO_STATE(STATE_LI);
	       break;
	    case 'O':
	       ADV_TO_STATE(STATE_LO);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_LG:
	 ADV_OR_FALLBACK('2', STATE_LG2);
	 break;

      case STATE_LG2:
	 FINISH_OR_FALLBACK(LG2_TOKEN);
	 break;

      case STATE_LI:
	 ADV_OR_FALLBACK('T', STATE_LIT);
	 break;

      case STATE_LIT:
	 FINISH_OR_FALLBACK(LIT_TOKEN);
	 break;

      case STATE_LO:
	 ADV_OR_FALLBACK('G', STATE_LOG);
	 break;

      case STATE_LOG:
	 FINISH_OR_FALLBACK(LOG_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the M* keywords
	  */
      case STATE_M:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'A':
	       ADV_TO_STATE(STATE_MA);
	       break;
	    case 'I':
	       ADV_TO_STATE(STATE_MI);
	       break;
	    case 'O':
	       ADV_TO_STATE(STATE_MO);
	       break;
	    case 'U':
	       ADV_TO_STATE(STATE_MU);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_MA:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'D':
	       ADV_TO_STATE(STATE_MAD);
	       break;
	    case 'X':
	       ADV_TO_STATE(STATE_MAX);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_MAD:
	 FINISH_OR_FALLBACK(MAD_TOKEN);
	 break;

      case STATE_MAX:
	 FINISH_OR_FALLBACK(MAX_TOKEN);
	 break;

      case STATE_MI:
	 ADV_OR_FALLBACK('N', STATE_MIN);
	 break;

      case STATE_MIN:
	 FINISH_OR_FALLBACK(MIN_TOKEN);
	 break;

      case STATE_MO:
	 ADV_OR_FALLBACK('V', STATE_MOV);
	 break;

      case STATE_MOV:
	 FINISH_OR_FALLBACK(MOV_TOKEN);
	 break;

      case STATE_MU:
	 ADV_OR_FALLBACK('L', STATE_MUL);
	 break;

      case STATE_MUL:
	 FINISH_OR_FALLBACK(MUL_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the O* keywords
	  */
      case STATE_O:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'P':
	       ADV_TO_STATE(STATE_OP);
	       break;
	    case 'U':
	       ADV_TO_STATE(STATE_OU);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_OP:
	 ADV_OR_FALLBACK('T', STATE_OPT);
	 break;

      case STATE_OPT:
	 ADV_OR_FALLBACK('I', STATE_OPTI);
	 break;

      case STATE_OPTI:
	 ADV_OR_FALLBACK('O', STATE_OPTIO);
	 break;

      case STATE_OPTIO:
	 ADV_OR_FALLBACK('N', STATE_OPTION);
	 break;

      case STATE_OPTION:
	 FINISH_OR_FALLBACK(OPTION_TOKEN);
	 break;

      case STATE_OU:
	 ADV_OR_FALLBACK('T', STATE_OUT);
	 break;

      case STATE_OUT:
	 ADV_OR_FALLBACK('P', STATE_OUTP);
	 break;

      case STATE_OUTP:
	 ADV_OR_FALLBACK('U', STATE_OUTPU);
	 break;

      case STATE_OUTPU:
	 ADV_OR_FALLBACK('T', STATE_OUTPUT);
	 break;

      case STATE_OUTPUT:
	 FINISH_OR_FALLBACK(OUTPUT_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the P* keywords
	  */
      case STATE_P:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'A':
	       ADV_TO_STATE(STATE_PA);
	       break;
	    case 'O':
	       ADV_TO_STATE(STATE_PO);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_PA:
	 ADV_OR_FALLBACK('R', STATE_PAR);
	 break;

      case STATE_PAR:
	 ADV_OR_FALLBACK('A', STATE_PARA);
	 break;

      case STATE_PARA:
	 ADV_OR_FALLBACK('M', STATE_PARAM);
	 break;

      case STATE_PARAM:
	 FINISH_OR_FALLBACK(PARAM_TOKEN);
	 break;

      case STATE_PO:
	 ADV_OR_FALLBACK('W', STATE_POW);
	 break;

      case STATE_POW:
	 FINISH_OR_FALLBACK(POW_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the R* keywords
	  */
      case STATE_R:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'C':
	       ADV_TO_STATE(STATE_RC);
	       break;
	    case 'S':
	       ADV_TO_STATE(STATE_RS);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_RC:
	 ADV_OR_FALLBACK('P', STATE_RCP);
	 break;

      case STATE_RCP:
	 FINISH_OR_FALLBACK(RCP_TOKEN);
	 break;

      case STATE_RS:
	 ADV_OR_FALLBACK('Q', STATE_RSQ);
	 break;

      case STATE_RSQ:
	 FINISH_OR_FALLBACK(RSQ_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the S* keywords
	  */
      case STATE_S:
	 if (IS_CD(curr)) {
	    switch (curr) {
	    case 'G':
	       ADV_TO_STATE(STATE_SG);
	       break;
	    case 'L':
	       ADV_TO_STATE(STATE_SL);
	       break;
	    case 'U':
	       ADV_TO_STATE(STATE_SU);
	       break;
	    case 'W':
	       ADV_TO_STATE(STATE_SW);
	       break;
	    default:
	       ADV_TO_STATE(STATE_IDENT);
	       break;
	    }
	 }
	 else {
	    s->curr_state = 1;
	 }
	 break;

      case STATE_SG:
	 ADV_OR_FALLBACK('E', STATE_SGE);
	 break;

      case STATE_SGE:
	 FINISH_OR_FALLBACK(SGE_TOKEN);
	 break;

      case STATE_SL:
	 ADV_OR_FALLBACK('T', STATE_SLT);
	 break;

      case STATE_SLT:
	 FINISH_OR_FALLBACK(SLT_TOKEN);
	 break;

      case STATE_SU:
	 ADV_OR_FALLBACK('B', STATE_SUB);
	 break;

      case STATE_SUB:
	 FINISH_OR_FALLBACK(SUB_TOKEN);
	 break;

      case STATE_SW:
	 ADV_OR_FALLBACK('Z', STATE_SWZ);
	 break;

      case STATE_SWZ:
	 FINISH_OR_FALLBACK(SWZ_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the T* keywords
	  */
      case STATE_T:
	 ADV_OR_FALLBACK('E', STATE_TE);
	 break;

      case STATE_TE:
	 ADV_OR_FALLBACK('M', STATE_TEM);
	 break;

      case STATE_TEM:
	 ADV_OR_FALLBACK('P', STATE_TEMP);
	 break;

      case STATE_TEMP:
	 FINISH_OR_FALLBACK(TEMP_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the X* keywords
	  */
      case STATE_X:
	 ADV_OR_FALLBACK('P', STATE_XP);
	 break;

      case STATE_XP:
	 ADV_OR_FALLBACK('D', STATE_XPD);
	 break;

      case STATE_XPD:
	 FINISH_OR_FALLBACK(XPD_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the p* keywords
	  */
      case STATE_LC_P:
	 ADV_OR_FALLBACK('r', STATE_LC_PR);
	 break;

      case STATE_LC_PR:
	 ADV_OR_FALLBACK('o', STATE_LC_PRO);
	 break;

      case STATE_LC_PRO:
	 ADV_OR_FALLBACK('g', STATE_LC_PROG);
	 break;

      case STATE_LC_PROG:
	 ADV_OR_FALLBACK('r', STATE_LC_PROGR);
	 break;

      case STATE_LC_PROGR:
	 ADV_OR_FALLBACK('a', STATE_LC_PROGRA);
	 break;

      case STATE_LC_PROGRA:
	 ADV_OR_FALLBACK('m', STATE_LC_PROGRAM);
	 break;

      case STATE_LC_PROGRAM:
	 FINISH_OR_FALLBACK(PROGRAM_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the r* keywords
	  */
      case STATE_LC_R:
	 ADV_OR_FALLBACK('e', STATE_LC_RE);
	 break;

      case STATE_LC_RE:
	 ADV_OR_FALLBACK('s', STATE_LC_RES);
	 break;

      case STATE_LC_RES:
	 ADV_OR_FALLBACK('u', STATE_LC_RESU);
	 break;

      case STATE_LC_RESU:
	 ADV_OR_FALLBACK('l', STATE_LC_RESUL);
	 break;

      case STATE_LC_RESUL:
	 ADV_OR_FALLBACK('t', STATE_LC_RESULT);
	 break;

      case STATE_LC_RESULT:
	 FINISH_OR_FALLBACK(RESULT_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the s* keywords
	  */
      case STATE_LC_S:
	 ADV_OR_FALLBACK('t', STATE_LC_ST);
	 break;

      case STATE_LC_ST:
	 ADV_OR_FALLBACK('a', STATE_LC_STA);
	 break;

      case STATE_LC_STA:
	 ADV_OR_FALLBACK('t', STATE_LC_STAT);
	 break;

      case STATE_LC_STAT:
	 ADV_OR_FALLBACK('e', STATE_LC_STATE);
	 break;

      case STATE_LC_STATE:
	 FINISH_OR_FALLBACK(STATE_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the v* keywords
	  */
      case STATE_LC_V:
	 ADV_OR_FALLBACK('e', STATE_LC_VE);
	 break;

      case STATE_LC_VE:
	 ADV_OR_FALLBACK('r', STATE_LC_VER);
	 break;

      case STATE_LC_VER:
	 ADV_OR_FALLBACK('t', STATE_LC_VERT);
	 break;

      case STATE_LC_VERT:
	 ADV_OR_FALLBACK('e', STATE_LC_VERTE);
	 break;

      case STATE_LC_VERTE:
	 ADV_OR_FALLBACK('x', STATE_LC_VERTEX);
	 break;

      case STATE_LC_VERTEX:
	 FINISH_OR_FALLBACK(VERTEX_TOKEN);
	 break;

	 /* -----------------------------------------------------
	  *  Beginning of the number & comment FSAs
	  */
      case STATE_N1:
	 if (curr == '.') {
	    ADV_TO_STATE(STATE_N2);
	 }
	 else if (IS_DIGIT(curr)) {
	    ADV_TO_STATE(STATE_N3);
	 }
	 else {
	    //ADV_AND_FINISH(PERIOD_TOKEN);                                 
	    FINISH(PERIOD_TOKEN);
	 }
	 break;

      case STATE_N2:
#if 1
	 //ADV_AND_FINISH(DOTDOT_TOKEN);
	 FINISH(DOTDOT_TOKEN);
#else
	 FINISH(PERIOD_TOKEN);
#endif
	 break;

      case STATE_N3:
	 if (IS_DIGIT(curr)) {
	    ADV_TO_STATE(STATE_N3);
	 }
	 else if ((curr == 'E') || (curr == 'e')) {
	    ADV_TO_STATE(STATE_N5);
	 }
	 else if (curr == '.') {
	    /* Blech! we have something like 1.. -> have to backup */
	    s->curr_pos -= 1;
	    *token_attr =
	       int_table_add(&s->ints, s->str, s->start_pos, s->curr_pos);
	    FINISH(INTEGER_TOKEN);
	 }
	 else {
	    *token_attr =
	       float_table_add(&s->floats, s->str, s->start_pos, s->curr_pos);
	    //ADV_AND_FINISH(FLOAT_TOKEN);
	    FINISH(FLOAT_TOKEN);
	 }
	 break;

      case STATE_N4:
	 if (IS_DIGIT(curr)) {
	    ADV_TO_STATE(STATE_N4);
	 }
	 else if ((curr == 'E') || (curr == 'e')) {
	    ADV_TO_STATE(STATE_N5);
	 }
	 else if (curr == '.') {
	    ADV_TO_STATE(STATE_N3);
	 }
	 else {
	    *token_attr =
	       int_table_add(&s->ints, s->str, s->start_pos, s->curr_pos);
	    //ADV_AND_FINISH(INTEGER_TOKEN);
	    FINISH(INTEGER_TOKEN);
	 }
	 break;

      case STATE_N5:
	 if (IS_DIGIT(curr)) {
	    ADV_TO_STATE(STATE_N6);
	 }
	 else if ((curr == '+') || (curr == '-')) {
	    ADV_TO_STATE(STATE_N7)
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

      case STATE_N6:
	 if (IS_DIGIT(curr)) {
	    ADV_TO_STATE(STATE_N6);
	 }
	 else {
	    *token_attr =
	       float_table_add(&s->floats, s->str, s->start_pos, s->curr_pos);
	    //ADV_AND_FINISH(FLOAT_TOKEN);
	    FINISH(FLOAT_TOKEN);
	 }
	 break;

      case STATE_N7:
	 if (IS_DIGIT(curr)) {
	    ADV_TO_STATE(STATE_N6);
	 }
	 else {
	    return ARB_VP_ERROR;
	 }

	 break;

      case STATE_COMMENT:
	 if ((curr == '\n') || (curr == '\r')) {
	    s->start_pos = s->curr_pos + 1;
	    s->curr_pos++;
	    s->curr_state = 0;
	 }
	 else {
	    ADV_TO_STATE(STATE_COMMENT);
	 }
      }
   }

   *token = EOF_TOKEN;
   return ARB_VP_SUCESS;
}

/**
 * This does the same as get_next_token(), but it does not advance the 
 * position pointers (Err, rather it does, but then it resets them)
 *
 * \param s          The parse state
 * \param token      The next token seen is returned in this value
 * \param token_attr	The token attribute for the next token is returned here. This
 *                    is the index into the approprate symbol table if token is INTEGER_TOKEN,
 *                    FLOAT_TOKEN, or ID_TOKEN
 * \param how_many   How many tokens to peek ahead
 *                   
 * \return ARB_VP_ERROR on lex error, ARB_VP_SUCESS on sucess
 */
static GLint
peek_next_token(parse_state * s, GLint * token, GLint * token_attr,
		GLint how_many)
{
   GLint tmp_state = s->curr_state;
   GLint tmp_sp = s->start_pos;
   GLint tmp_cp = s->curr_pos;
   GLint a, retval;

   for (a = 0; a < how_many; a++) {
      retval = get_next_token(s, token, token_attr);

      if (retval == ARB_VP_ERROR)
	 return retval;
   }

   s->curr_state = tmp_state;
   s->start_pos = tmp_sp;
   s->curr_pos = tmp_cp;

   return retval;
}

/**
 * Print out the value of a token
 */
static void
debug_token(parse_state * state, GLint t, GLint ta)
{
   switch (t) {
   case EOF_TOKEN:
      printf("EOF\n");
      break;
   case ID_TOKEN:
      printf("|%s|  ", state->idents.data[ta]);
      break;
   case INTEGER_TOKEN:
      printf("|%d|  ", state->ints.data[ta]);
      break;
   case FLOAT_TOKEN:
      printf("|%f|  ", state->floats.data[ta]);
      break;

   case ABS_TOKEN:
      printf("ABS  ");
      break;
   case ADD_TOKEN:
      printf("ADD  ");
      break;
   case ADDRESS_TOKEN:
      printf("ADDRESS  ");
      break;
   case ALIAS_TOKEN:
      printf("ALIAS  ");
      break;
   case ARL_TOKEN:
      printf("ARL  ");
      break;
   case ATTRIB_TOKEN:
      printf("ATTRIB  ");
      break;

   case DP3_TOKEN:
      printf("DP3  ");
      break;
   case DP4_TOKEN:
      printf("DP4  ");
      break;
   case DPH_TOKEN:
      printf("DPH  ");
      break;
   case DST_TOKEN:
      printf("DST  ");
      break;

   case END_TOKEN:
      printf("END  ");
      break;
   case EX2_TOKEN:
      printf("EX2  ");
      break;
   case EXP_TOKEN:
      printf("EXP  ");
      break;

   case FLR_TOKEN:
      printf("FLR  ");
      break;
   case FRC_TOKEN:
      printf("FRC  ");
      break;

   case LG2_TOKEN:
      printf("LG2  ");
      break;
   case LIT_TOKEN:
      printf("LIT  ");
      break;
   case LOG_TOKEN:
      printf("LOG  ");
      break;

   case MAD_TOKEN:
      printf("MAD  ");
      break;
   case MAX_TOKEN:
      printf("MAX  ");
      break;
   case MIN_TOKEN:
      printf("MIN  ");
      break;
   case MOV_TOKEN:
      printf("MOV  ");
      break;
   case MUL_TOKEN:
      printf("MUL  ");
      break;

   case OPTION_TOKEN:
      printf("OPTION  ");
      break;
   case OUTPUT_TOKEN:
      printf("OUTPUT  ");
      break;

   case PARAM_TOKEN:
      printf("PARAM  ");
      break;
   case POW_TOKEN:
      printf("POW  ");
      break;

   case RCP_TOKEN:
      printf("RCP  ");
      break;
   case RSQ_TOKEN:
      printf("RSQ  ");
      break;

   case SGE_TOKEN:
      printf("SGE  ");
      break;
   case SLT_TOKEN:
      printf("SLT  ");
      break;
   case SUB_TOKEN:
      printf("SUB  ");
      break;
   case SWZ_TOKEN:
      printf("SWZ  ");
      break;

   case TEMP_TOKEN:
      printf("TEMP  ");
      break;

   case XPD_TOKEN:
      printf("XPD  ");
      break;

   case SEMICOLON_TOKEN:
      printf(";  ");
      break;
   case COMMA_TOKEN:
      printf(",  ");
      break;
   case PLUS_TOKEN:
      printf("+  ");
      break;
   case MINUS_TOKEN:
      printf("-  ");
      break;
   case PERIOD_TOKEN:
      printf(".  ");
      break;
   case DOTDOT_TOKEN:
      printf("..  ");
      break;
   case LBRACKET_TOKEN:
      printf("[  ");
      break;
   case RBRACKET_TOKEN:
      printf("]  ");
      break;
   case LBRACE_TOKEN:
      printf("{  ");
      break;
   case RBRACE_TOKEN:
      printf("}  ");
      break;
   case EQUAL_TOKEN:
      printf("=  ");
      break;

   case PROGRAM_TOKEN:
      printf("program ");
      break;
   case RESULT_TOKEN:
      printf("result ");
      break;
   case STATE_TOKEN:
      printf("state ");
      break;
   case VERTEX_TOKEN:
      printf("vertex ");
      break;
   default:
      printf("Unknown token type %d\n", t);
   }
}

/**
 * Setup the state used by the parser / lex 
 *
 * \param str	The program string, with the !!ARBvp1.0 token stripped off
 * \param len	The lenght of the given string
 *
 * \return 		A parse_state struct to keep track of all the things we need while parsing
 */
static parse_state *
parse_state_init(GLubyte * str, GLint len)
{
   parse_state *s = (parse_state *) _mesa_malloc(sizeof(parse_state));

   s->str = _mesa_strdup(str);
   s->len = len;
   s->curr_pos = 0;
   s->start_pos = 0;

   s->curr_state = 0;

   s->idents.len = 0;
   s->idents.data = NULL;

   s->ints.len = 0;
   s->ints.data = NULL;

   s->floats.len = 0;
   s->floats.data = NULL;
   printf("%s\n", s->str);

   s->binds.len = 0;
   s->binds.type = NULL;
   s->binds.offset = NULL;
   s->binds.row = NULL;
   s->binds.consts = NULL;

   s->arrays.len = 0;
   s->arrays.num_elements = NULL;
   s->arrays.data = NULL;

   s->stack_head = NULL;
   s->stack_free_list = NULL;

   s->pt_head = NULL;

   return s;
}

/**
 * This frees all the things we hold while parsing.
 *
 * \param s		The struct created by parse_state_init()
 */
static void
parse_state_cleanup(parse_state * s)
{
   GLint a;
   token_list *tl, *next;

   /* Free our copy of the string [Mesa has its own] */
   _mesa_free(s->str);

   /* Free all the tables ident, int, float, bind, mat */
   _mesa_free(s->idents.type);
   _mesa_free(s->idents.attr);
   for (a = 0; a < s->idents.len; a++)
      _mesa_free(s->idents.data[a]);

   _mesa_free(s->ints.data);
   _mesa_free(s->floats.data);

   _mesa_free(s->arrays.num_elements);
   for (a = 0; a < s->arrays.len; a++)
      _mesa_free(s->arrays.data[a]);

   _mesa_free(s->binds.type);
   _mesa_free(s->binds.offset);
   _mesa_free(s->binds.row);
   _mesa_free(s->binds.num_rows);
   _mesa_free(s->binds.reg_num);
#if 0
   for (a = 0; a < s->binds.len; a++) {
      printf("6: %d/%d\n", a, s->binds.len - 1);
      _mesa_free(s->binds.consts[a]);
   }
#endif

   /* Free the stack */
   tl = s->stack_head;
   while (tl) {
      next = tl->next;
      free(tl);
      tl = next;
   }
   tl = s->stack_free_list;
   while (tl) {
      next = tl->next;
      free(tl);
      tl = next;
   }
   printf("freed stack free list\n");

#if 0
   /* Free the parse tree */
   parse_tree_free_children(s->pt_head);
   printf("freed parse_tree\n");
#endif
   free(s);
   printf("freed state\n");
}

/**
 *	Alloc a new node for a parse tree.
 *
 *	\return	An empty node to fill and stick into the parse tree
 */
static parse_tree_node *
parse_tree_node_init(void)
{
   GLint a;
   parse_tree_node *pt;

   pt = (parse_tree_node *) _mesa_malloc(sizeof(parse_tree_node));
   pt->tok = -1;
   pt->tok_attr = -1;
   pt->is_terminal = 0;
   pt->prod_applied = -1;

   for (a = 0; a < 4; a++)
      pt->children[a] = NULL;

   return pt;
}

/**
 * We maintain a stack of nonterminals used for predictive parsing. To keep
 * from thrashing malloc/free, we keep a free list of token_list structs
 * to be used for this stack. This function is called to refill the free
 * list, when we try to grab a new token_list struct, and find that there are none
 * available
 *
 * \param s	The parse state
 */
static void
_fill_free_list(parse_state * s)
{
   GLint a;
   token_list *tl;

   if (s->stack_free_list)
      return;

   for (a = 0; a < 20; a++) {
      tl = (token_list *) _mesa_malloc(sizeof(token_list));

      tl->next = s->stack_free_list;
      s->stack_free_list = tl;
   }
}

/**
 * Peek at the head of the nonterminal stack,
 *
 * \param s          The parse state
 * \param token      Return for the nonterminal token on the top of the stack
 * \param token_attr Return for the the token attribute on the top of the stack
 *
 * \return ARB_VP_ERROR on an empty stack [not necessarily a bad thing], else ARB_VP_SUCESS
 */
static GLint
_stack_peek(parse_state * s, GLint * token, GLint * token_attr)
{
   if (!s->stack_head) {
      fprintf(stderr, "ACK! Empty stack on peek!\n");
      return ARB_VP_ERROR;
   }

   *token = s->stack_head->tok;
   *token_attr = s->stack_head->tok_attr;

   return ARB_VP_SUCESS;
}

/**
 * Remove the token at the head of the nonterminal stack
 * \param s          The parse state
 * \param token      Return for the nonterminal token on the top of the stack
 * \param token_attr Return for the the token attribute on the top of the stack
 * \param ptn        Return for a pointer to the place in the parse tree where 
 *                    the token lives
 *
 * \return           ARB_VP_ERROR on an empty stack [not necessarily a bad thing], else ARB_VP_SUCESS
 */
static GLint
_stack_pop(parse_state * s, GLint * token, GLint * token_attr,
	   parse_tree_node ** ptn)
{
   token_list *tl;

   if (!s->stack_head) {
      fprintf(stderr, "ACK! Empty stack!\n");
      return ARB_VP_ERROR;
   }

   *token = s->stack_head->tok;
   *token_attr = s->stack_head->tok_attr;
   if (ptn)
      *ptn = s->stack_head->pt;
   tl = s->stack_head;

   s->stack_head = tl->next;
   tl->next = s->stack_free_list;
   s->stack_free_list = tl;

   return ARB_VP_SUCESS;
}

/**
 * Put a token, its attribute, and the the parse tree node where the token is stored, onto
 * the parse stack.
 * 
 * \param s          The parse state
 * \param token      Return for the nonterminal token on the top of the stack
 * \param token_attr Return for the the token attribute on the top of the stack
 * \param ptn        Return for a pointer to the place in the parse tree where 
 *                    the token lives
 *
 * \return           ARB_VP_ERROR on out of memory while allocing more storage for the stack,
 *                    else ARB_VP_SUCESS
 */
static GLint
_stack_push(parse_state * s, GLint token, GLint token_attr,
	    parse_tree_node * ptn)
{
   token_list *tl;

   if (!s->stack_free_list) {
      _fill_free_list(s);
      if (!s->stack_free_list) {
	 fprintf(stderr, "ACK! Error filling stack free list\n");
	 return ARB_VP_ERROR;
      }
   }

   tl = s->stack_free_list;

   s->stack_free_list = tl->next;

   tl->tok = token;
   tl->tok_attr = token_attr;
   tl->pt = ptn;
   tl->next = s->stack_head;

   s->stack_head = tl;

   return ARB_VP_SUCESS;
}

/**
 * Allocate a new entry in the array table
 *
 * \param tab   The array table to add a new element too
 *
 * \return      The index into the array table where the new element is.
 */
static GLint
array_table_new(array_table * tab)
{
   GLint idx;
   if (tab->len == 0) {
      tab->num_elements = (GLint *) _mesa_malloc(sizeof(GLint));
      tab->data = (GLint **) _mesa_malloc(sizeof(GLint *));
      idx = 0;
   }
   else {
      tab->num_elements =
	 (GLint *) _mesa_realloc(tab->num_elements, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
      tab->data =
	 (GLint **) _mesa_realloc(tab->data, tab->len * sizeof(GLint *),
				  (tab->len + 1) * sizeof(GLint *));
      idx = tab->len;
   }

   tab->len++;
   tab->num_elements[idx] = 0;
   tab->data[idx] = NULL;

   return idx;
}

/**
 * Add a new element to a array in a array table
 *
 * \param tab   The array table
 * \param idx   The index into the array table of the array we want to append an item onto
 * \param data  The program parameter that goes into the idx-th array 
 */
static void
array_table_add_data(array_table * tab, GLint idx, GLint data)
{
   if ((idx < 0) || (idx >= tab->len)) {
      printf("Bad matrix index %d!\n", idx);
      return;
   }

   if (tab->data[idx] == NULL) {
      tab->data[idx] = (GLint *) _mesa_malloc(sizeof(GLint));
   }
   else {
      tab->data[idx] =
	 (GLint *) _mesa_realloc(tab->data[idx],
				 tab->num_elements[idx] * sizeof(GLint),
				 (tab->num_elements[idx] +
				  1) * sizeof(GLint));
   }

   tab->data[idx][tab->num_elements[idx]] = data;
   tab->num_elements[idx]++;
}


/**
 * This adds a new entry into the binding table. 
 *
 * \param tab    The binding table
 * \param type   The type of the state
 * \param offset For matrix bindings, e.g. MATRIXROWS_MODELVIEW, this gives the matrix number. 
 *               For PROGRAM_ENV_* and PROGRAM_LOCAL_* bindings, this gives the number of the first parameter
 *               
 * \param row    For MATRIXROWS bindings, this is the first row in the matrix we're bound to
 * \param nrows  For MATRIXROWS bindings, this is the number of rows of the matrix we have. 
 *               For PROGRAM_ENV/LOCAL bindings, this is the number of parameters in the array we're bound to
 * \param values For CONSTANT bindings, these are the constant values we're bound to
 * \return       The index into the binding table where this state is bound
 */
static GLint
binding_table_add(binding_table * tab, GLint type, GLint offset, GLint row,
		  GLint nrows, GLfloat * values)
{
   GLint key, a;

   key = tab->len;

   /* test for existance */
   for (a = 0; a < tab->len; a++) {
      if ((tab->type[a] == type) && (tab->offset[a] == offset)
	  && (tab->row[a] == row) && (tab->num_rows[a] == nrows) &&
	  (fabs(tab->consts[a][0] - values[0]) < .0001) &&
	  (fabs(tab->consts[a][1] - values[1]) < .0001) &&
	  (fabs(tab->consts[a][2] - values[2]) < .0001) &&
	  (fabs(tab->consts[a][3] - values[3]) < .0001))
	 return a;
   }

   if (tab->len == 0) {
      tab->type = (GLint *) _mesa_malloc(sizeof(GLint));
      tab->offset = (GLint *) _mesa_malloc(sizeof(GLint));
      tab->row = (GLint *) _mesa_malloc(sizeof(GLint));
      tab->num_rows = (GLint *) _mesa_malloc(sizeof(GLint));
      tab->consts = (GLfloat **) _mesa_malloc(sizeof(GLfloat *));
      tab->consts[0] = (GLfloat *) _mesa_malloc(4 * sizeof(GLfloat));
      tab->reg_num = (GLint *) _mesa_malloc(sizeof(GLint));
   }
   else {
      tab->type =
	 (GLint *) _mesa_realloc(tab->type, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
      tab->offset =
	 (GLint *) _mesa_realloc(tab->offset, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
      tab->row =
	 (GLint *) _mesa_realloc(tab->row, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
      tab->num_rows =
	 (GLint *) _mesa_realloc(tab->num_rows, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
      tab->consts =
	 (GLfloat **) _mesa_realloc(tab->consts, tab->len * sizeof(GLfloat),
				    (tab->len + 1) * sizeof(GLfloat *));
      tab->consts[tab->len] = (GLfloat *) _mesa_malloc(4 * sizeof(GLfloat));
      tab->reg_num =
	 (GLint *) _mesa_realloc(tab->reg_num, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
   }

   tab->type[key] = type;
   tab->offset[key] = offset;
   tab->row[key] = row;		//key;
   tab->num_rows[key] = nrows;
   tab->reg_num[key] = 0;
   _mesa_memcpy(tab->consts[key], values, 4 * sizeof(GLfloat));
   tab->len++;

   return key;
}

/** 
 * Given a string and a start/end point, add a string into the float
 * symbol table (and convert it into a float)
 *
 * If we already have this GLfloat in the table, don't bother
 * adding another, just return the key to the existing one
 *
 * \param tab     The float table
 * \param str     The string containing the float
 * \param start   The starting position of the float in str
 * \param end     The ending position of the float in str
 *
 * \return        The index of the float, after we insert it, in the float table
 */
static GLint
float_table_add(float_table * tab, GLubyte * str, GLint start, GLint end)
{
   GLint key, a;
   GLubyte *newstr;

   key = tab->len;

   newstr = (GLubyte *) _mesa_malloc(end - start + 2);
   _mesa_memset(newstr, 0, end - start + 2);
   _mesa_memcpy(newstr, str + start, end - start);

   /* test for existance */
   for (a = 0; a < tab->len; a++) {
      if (tab->data[a] == atof(newstr)) {
	 _mesa_free(newstr);
	 return a;
      }
   }

   if (tab->len == 0) {
      tab->data = (GLdouble *) _mesa_malloc(sizeof(GLdouble));
   }
   else {
      tab->data =
	 (GLdouble *) _mesa_realloc(tab->data, tab->len * sizeof(GLdouble),
				    (tab->len + 1) * sizeof(GLdouble));
   }

   tab->data[key] = atof(newstr);
   tab->len++;

   _mesa_free(newstr);
   return key;
}

/** 
 * Given a string and a start/end point, add a string into the int
 * symbol table (and convert it into a int)
 *
 * If we already have this int in the table, don't bother
 * adding another, just return the key to the existing one
 *
 * \param tab     The int table
 * \param str     The string containing the int
 * \param start   The starting position of the int in str
 * \param end     The ending position of the int in str
 *
 * \return        The index of the int, after we insert it, in the int table
 */
static GLint
int_table_add(int_table * tab, GLubyte * str, GLint start, GLint end)
{
   GLint key, a;
   GLubyte *newstr;

   key = tab->len;

   newstr = (GLubyte *) _mesa_malloc(end - start + 2);
   _mesa_memset(newstr, 0, end - start + 2);
   _mesa_memcpy(newstr, str + start, end - start);

   for (a = 0; a < tab->len; a++) {
      if (tab->data[a] == _mesa_atoi(newstr)) {
	 _mesa_free(newstr);
	 return a;
      }
   }

   if (tab->len == 0) {
      tab->data = (GLint *) _mesa_malloc(sizeof(GLint));
   }
   else {
      tab->data =
	 (GLint *) _mesa_realloc(tab->data, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
   }

   tab->data[key] = _mesa_atoi(newstr);
   tab->len++;

   _mesa_free(newstr);
   return key;
}

/**
 * Insert an identifier into the identifier symbol table
 * 
 * If we already have this id in the table, don't bother
 * adding another, just return the key to the existing one
 *
 * If we already have this id in the table, and it has been
 * initialized to an ALIAS, return what the alias points
 * to, not the alias var
 *
 * \param tab    The ID table
 * \param str    The string containing the id
 * \param start  The position in str where the id begins
 * \param end    The position in str where the id ends
 *
 * \return       either:
 *                1) The index into the id table where the id is
 *                2) The index into the id table where the alias of id, if we already have id
 *                     in the table, and it has been initialized to type ALIAS
 */
static GLint
id_table_add(id_table * tab, GLubyte * str, GLint start, GLint end)
{
   GLint key, a;
   GLubyte *newstr;

   key = tab->len;

   if (tab->len == 0) {
      tab->data = (GLubyte **) _mesa_malloc(sizeof(GLubyte *));
      tab->type = (GLint *) _mesa_malloc(sizeof(GLint));
      tab->attr = (GLint *) _mesa_malloc(sizeof(GLint));
   }
   else {
      tab->data =
	 (GLubyte **) _mesa_realloc(tab->data, tab->len * sizeof(GLubyte *),
				    (tab->len + 1) * sizeof(GLubyte *));
      tab->type =
	 (GLint *) _mesa_realloc(tab->type, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
      tab->attr =
	 (GLint *) _mesa_realloc(tab->attr, tab->len * sizeof(GLint),
				 (tab->len + 1) * sizeof(GLint));
   }

   //tab->type[key] = TYPE_NONE;

   newstr = (GLubyte *) _mesa_malloc((end - start + 2) * sizeof(GLubyte));
   _mesa_memset(newstr, 0, end - start + 2);
   _mesa_memcpy(newstr, str + start, end - start);

   for (a = 0; a < tab->len; a++) {
      /* aha! we found it in the table */
      if (!_mesa_strcmp(tab->data[a], newstr)) {
	 _mesa_free(newstr);

	 key = a;
	 while ((tab->type[key] == TYPE_ALIAS) && (tab->attr[key] != -1)) {
	    printf("----------- %s is an alias, renaming to %s\n",
		   tab->data[key], tab->data[tab->attr[key]]);
	    key = tab->attr[key];
	 }

	 return key;
      }
   }

   /* oh, we really have a new id */
   tab->data[key] = newstr;
   tab->type[key] = TYPE_NONE;
   tab->attr[key] = -1;
   tab->len++;

   return key;
}

//#define SHOW_STEPS 1


/**
 * Apply the specified production number to the parse state. This handles
 * looking at the production table and sticking new tokens onto the 
 * parse stack. 
 *
 * It also handles the construction of the parse tree
 *
 * \param s   The parse state
 * \param num The production number to apply [the idx in the production table]
 *
 */
static void
apply_production(parse_state * s, int num)
{
   GLint a, str_key, stack_tok, stack_tok_attr;
   GLint tok, nnptr = 0;
   parse_tree_node *ptn;
   parse_tree_node *pt_ptr_new[4];

   _stack_pop(s, &stack_tok, &stack_tok_attr, &ptn);
   /*printf("apply prod %d\n", num); */

   ptn->prod_applied = num;
   for (a = 3; a >= 0; a--) {
      str_key = 0;
      tok = ptab[num].rhs[a];

      if (tok == NULL_TOKEN)
	 continue;

      /* If we are pushing an identifier or a number, we need to translate the string literal
       * in the production table into an entry in the approprate symbol table
       */
      if (tok == ID_TOKEN) {
	 str_key =
	    id_table_add(&s->idents, ptab[num].key[a], 0,
			 strlen(ptab[num].key[a]));
      }
      else if (tok == INTEGER_TOKEN) {
	 str_key =
	    int_table_add(&s->ints, ptab[num].key[a], 0,
			  strlen(ptab[num].key[a]));
      }
      else if (tok == FLOAT_TOKEN) {
	 str_key =
	    float_table_add(&s->floats, ptab[num].key[a], 0,
			    strlen(ptab[num].key[a]));
      }

      /* "-1" is a wildcard flag, accept any id/float/int */
      if ((!_mesa_strcmp(ptab[num].key[a], "-1")) &&
	  ((tok == FLOAT_TOKEN) || (tok == INTEGER_TOKEN)
	   || (tok == ID_TOKEN))) {
	 pt_ptr_new[nnptr] = parse_tree_node_init();
	 pt_ptr_new[nnptr]->is_terminal = 0;
	 pt_ptr_new[nnptr]->tok = tok;
	 pt_ptr_new[nnptr]->tok_attr = str_key;
	 nnptr++;
	 _stack_push(s, ptab[num].rhs[a], str_key, pt_ptr_new[nnptr - 1]);
      }
      else if (tok >= NT_PROGRAM_TOKEN) {
	 pt_ptr_new[nnptr] = parse_tree_node_init();
	 pt_ptr_new[nnptr]->is_terminal = 0;
	 pt_ptr_new[nnptr]->tok = tok;
	 nnptr++;
	 _stack_push(s, ptab[num].rhs[a], str_key, pt_ptr_new[nnptr - 1]);
      }
      else
	 _stack_push(s, ptab[num].rhs[a], str_key, NULL);
   }

   tok = 0;
   if (ptn) {
      /*printf("%x %d:[%x %x %x %x]\n", ptn, nnptr, pt_ptr_new[0], pt_ptr_new[1], pt_ptr_new[2], pt_ptr_new[3]); */

      for (a = nnptr - 1; a >= 0; a--) {
	 ptn->children[tok] = pt_ptr_new[a];
	 /*printf("%x->children[%d] = %x\n", ptn, tok, pt_ptr_new[a]); */
	 tok++;
      }
   }
}

/**
 * Here we initialize a bunch of variables to a given type (e.g. TEMP). 
 *
 * We stick the variable type into the 0-th element in the var_queue array, followed by var_queue_size
 * indicies into the identifier table which designate the variables we are to init.
 * 
 * \param s              The parse state
 * \param var_queue      The queue of variables to initialize. The first element in the array is variable
 *                        type. It is followed by var_queue_size indicies into the identifier table 
 *                        who we are supposed to init to type var_queue[0].
 * \param var_queue_size The number of variables listed in var_queue[].
 *
 * \return               ARB_VP_ERROR if we have already initilized a variable in var_queue, otherwise
 *                        ARB_VP_SUCESS
 */
static GLint
init_vars(parse_state * s, GLint * var_queue, GLint var_queue_size)
{
   GLint a;

   a = 1;
   while (a < var_queue_size) {
      /* Make sure we haven't already init'd this var. This will
       * catch multiple definations of the same symbol
       */
      if (s->idents.type[var_queue[a]] != TYPE_NONE) {
	 printf("%s already init'd! (%d)\n", s->idents.data[var_queue[a]],
		s->idents.type[var_queue[a]]);
	 return ARB_VP_ERROR;
      }

      s->idents.type[var_queue[a]] = var_queue[0];
      s->idents.attr[var_queue[a]] = -1;
      a++;
   }

   return ARB_VP_SUCESS;
}

/**
 * The main parsing loop. This applies productions and builds the parse tree.
 *
 * \param s    The parse state
 *
 * \return ARB_VP_ERROR on a parse error, else ARB_VP_SUCESS
 */
static GLint
parse(parse_state * s)
{
   GLint input_tok, input_tok_attr;
   GLint stack_tok, stack_tok_attr;
   GLint peek_tok, peek_tok_attr, ret;
   GLint idx, str_key;
   GLint var_queue_size = 0;
   GLint *var_queue;
   parse_tree_node *ptn, *ptn2;

   /* This should be MAX_VAR + 1 */
   var_queue = (GLint *) _mesa_malloc(1000 * sizeof(int));

   s->stack_head = NULL;

   /* init the system by pushing the start symbol onto the stack */
   ptn = parse_tree_node_init();
   ptn->is_terminal = 0;
   ptn->tok = NT_PROGRAM_TOKEN;
   s->pt_head = ptn;
   _stack_push(s, NT_PROGRAM_TOKEN, 0, ptn);

   /* and get the first token */
   if (get_next_token(s, &input_tok, &input_tok_attr) == ARB_VP_ERROR) {
      fprintf(stderr, "LEX ERROR!!\n");
      return ARB_VP_ERROR;
   }

   while (1) {
      GLint la, is_nonterm;

      /* If the stack is empty, and we've eaten all the input, we're done */
      if ((_stack_peek(s, &stack_tok, &stack_tok_attr) == ARB_VP_ERROR) &&
	  (input_tok == EOF_TOKEN))
	 break;

#ifdef SHOW_STEPS
      printf("stack: %d input: ", stack_tok);
      debug_token(s, input_tok, input_tok_attr);
      printf("\n");
#endif

      /* We [may] have a non-terminal on top of the stack, apply
       * productions!
       */
      switch (stack_tok) {
	 /* Special case non-terminals */

	 /* productions 64-66 */
      case NT_SRC_REG_TOKEN:
	 if ((input_tok == VERTEX_TOKEN) ||
	     ((input_tok == ID_TOKEN)
	      && (s->idents.type[input_tok_attr] == TYPE_ATTRIB))) {
	    apply_production(s, 64);
	 }
	 else
	    if ((input_tok == ID_TOKEN)
		&& (s->idents.type[input_tok_attr] == TYPE_TEMP)) {
	    apply_production(s, 65);
	 }
	 else
	    if ((input_tok == STATE_TOKEN) ||
		(input_tok == PROGRAM_TOKEN) ||
		(input_tok == LBRACE_TOKEN) ||
		(input_tok == FLOAT_TOKEN) ||
		(input_tok == INTEGER_TOKEN) ||
		((input_tok == ID_TOKEN)
		 && (s->idents.type[input_tok_attr] == TYPE_PARAM_SINGLE))
		|| ((input_tok == ID_TOKEN)
		    && (s->idents.type[input_tok_attr] ==
			TYPE_PARAM_ARRAY))) {
	    apply_production(s, 66);
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* productions 67-68 */
      case NT_DST_REG_TOKEN:
	 /* We can only write to result.*, TEMP vars, or OUTPUT vars */
	 if (input_tok == RESULT_TOKEN) {
	    apply_production(s, 68);
	 }
	 else if (input_tok == ID_TOKEN) {
	    if (s->idents.type[input_tok_attr] == TYPE_TEMP) {
	       apply_production(s, 67);
	    }
	    else if (s->idents.type[input_tok_attr] == TYPE_OUTPUT) {
	       apply_production(s, 68);
	    }
	    else {
	       return ARB_VP_ERROR;
	    }
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* productions 72-4 */
      case NT_PROG_PARAM_REG_TOKEN:
	 if ((input_tok == ID_TOKEN)
	     && (s->idents.type[input_tok_attr] == TYPE_PARAM_SINGLE)) {
	    apply_production(s, 72);
	 }
	 else
	    if ((input_tok == ID_TOKEN)
		&& (s->idents.type[input_tok_attr] == TYPE_PARAM_ARRAY)) {
	    apply_production(s, 73);
	 }
	 else
	    if ((input_tok == STATE_TOKEN) ||
		(input_tok == PROGRAM_TOKEN) ||
		(input_tok == FLOAT_TOKEN) ||
		(input_tok == INTEGER_TOKEN) || (input_tok == LBRACE_TOKEN)) {
	    apply_production(s, 74);
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* productions 286-7 */
      case NT_VAR_NAME_LIST_TOKEN:
	 ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);

	 var_queue[var_queue_size] = input_tok_attr;
	 var_queue_size++;

	 if ((ret == ARB_VP_ERROR) || (peek_tok != COMMA_TOKEN)) {
	    apply_production(s, 286);

	    /* Dump the var_queue & assign types */
	    if (init_vars(s, var_queue, var_queue_size) == ARB_VP_ERROR)
	       return ARB_VP_ERROR;
	 }
	 else {
	    apply_production(s, 287);
	 }
	 break;

	 /* productions 296-7 */
      case NT_RESULT_COL_BINDING2_TOKEN:
	 if ((input_tok == SEMICOLON_TOKEN) || (input_tok == COMMA_TOKEN)) {
	    apply_production(s, 296);
	 }
	 else if (input_tok == PERIOD_TOKEN) {
	    ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	    if ((peek_tok == ID_TOKEN) &&
		((!_mesa_strcmp(s->idents.data[peek_tok_attr], "back")) ||
		 (!_mesa_strcmp(s->idents.data[peek_tok_attr], "front")))) {
	       apply_production(s, 297);
	    }
	    else {
	       apply_production(s, 296);
	    }
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* productions 300-1 */
      case NT_RESULT_COL_BINDING4_TOKEN:
	 if ((input_tok == SEMICOLON_TOKEN) || (input_tok == COMMA_TOKEN)) {
	    apply_production(s, 300);
	 }
	 else if (input_tok == PERIOD_TOKEN) {
	    ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	    if ((ret == ARB_VP_SUCESS) && (peek_tok == ID_TOKEN) &&
		((!_mesa_strcmp(s->idents.data[peek_tok_attr], "primary")) ||
		 (!_mesa_strcmp(s->idents.data[peek_tok_attr], "secondary"))))
	    {
	       apply_production(s, 301);
	    }
	    else {
	       apply_production(s, 300);
	    }
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* productions 306-7 */
      case NT_OPT_COLOR_TYPE_TOKEN:
	 if ((input_tok == SEMICOLON_TOKEN) || (input_tok == COMMA_TOKEN)) {
	    apply_production(s, 307);
	 }
	 else if (input_tok == PERIOD_TOKEN) {
	    ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	    if ((ret == ARB_VP_SUCESS) && (peek_tok == ID_TOKEN) &&
		((!_mesa_strcmp(s->idents.data[peek_tok_attr], "primary")) ||
		 (!_mesa_strcmp(s->idents.data[peek_tok_attr], "secondary"))))
	    {
	       apply_production(s, 306);
	    }
	    else {
	       apply_production(s, 307);
	    }
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* production 313 -- Do this so we can mangle IDs as they are 
	  *                                              added into the ID table for the lex
	  */
      case NT_ALIAS_STATEMENT_TOKEN:
	 if (input_tok == ALIAS_TOKEN) {
	    GLint alias_to;

	    apply_production(s, 313);

	    /* stack ALIAS */
	    var_queue_size = 1;
	    var_queue[0] = TYPE_ALIAS;
	    ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	    var_queue[var_queue_size] = peek_tok_attr;
	    var_queue_size++;

	    if (init_vars(s, var_queue, var_queue_size) == ARB_VP_ERROR)
	       return ARB_VP_ERROR;

	    /* Now, peek ahead and see what we are aliasing _to_ */
	    alias_to = peek_tok_attr;
	    ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 3);
	    if (ret == ARB_VP_SUCESS) {
	       s->idents.attr[alias_to] = peek_tok_attr;
	    }
	    else
	       return ARB_VP_ERROR;
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* productions 314 (ESTABLISH_NAME) duplicates are caught by the ID symbol table */

	 /* productions 315 */
      case NT_ESTABLISHED_NAME_TOKEN:
	 if (input_tok == ID_TOKEN) {
	    if (s->idents.type[input_tok_attr] == TYPE_NONE) {
	       printf("Trying to use variable %s before initializing!\n",
		      s->idents.data[input_tok_attr]);
	       return ARB_VP_ERROR;
	    }
	    else {
	       apply_production(s, 315);
	    }
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;


	 /* productions 318-9 */
      case NT_SWIZZLE_SUFFIX2_TOKEN:
	 if (strlen(s->idents.data[input_tok_attr]) == 1) {
	    apply_production(s, 318);
	 }
	 else if (strlen(s->idents.data[input_tok_attr]) == 4) {
	    apply_production(s, 319);
	 }
	 else {
	    return ARB_VP_ERROR;
	 }
	 break;

	 /* 4-component swizzle mask -- this is a total hack */
#define IS_SWZ_CMP(foo) ((foo == 'x') || (foo == 'y') || (foo == 'z') || (foo == 'w'))
      case NT_COMPONENT4_TOKEN:
	 {
	    GLubyte *str = s->idents.data[input_tok_attr];

	    if (IS_SWZ_CMP(str[0]) && IS_SWZ_CMP(str[1]) && IS_SWZ_CMP(str[2])
		&& IS_SWZ_CMP(str[3])) {
	       _stack_pop(s, &stack_tok, &stack_tok_attr, &ptn);
//                                              _stack_push(s, input_tok, input_tok_attr, NULL);

	       ptn2 = parse_tree_node_init();
	       ptn2->tok = input_tok;
	       ptn2->tok_attr = input_tok_attr;
	       ptn2->is_terminal = 1;
	       ptn->children[0] = ptn2;
	       _stack_push(s, input_tok, input_tok_attr, ptn2);
	    }
	    else {
	       return ARB_VP_ERROR;
	    }
	 }
	 break;

	 /* Handle general non-terminals using tables, and terminals */
      default:
	 is_nonterm = 0;
	 for (la = 0; la < nlas; la++) {
	    if (latab[la].lhs != stack_tok)
	       continue;

	    if (latab[la].la != input_tok)
	       continue;

	    if (input_tok == ID_TOKEN) {
	       str_key =
		  id_table_add(&s->idents, latab[la].la_kw, 0,
			       strlen(latab[la].la_kw));
	       if ((str_key != input_tok_attr)
		   && (_mesa_strcmp(latab[la].la_kw, "-1")))
		  continue;
	    }
	    else if (input_tok == INTEGER_TOKEN) {
	       if ((s->ints.data[input_tok_attr] !=
		    _mesa_atoi(latab[la].la_kw))
		   && (_mesa_atoi(latab[la].la_kw) != -1)) {
		  continue;
	       }
	    }
	    else if (input_tok == FLOAT_TOKEN) {
	       if ((s->floats.data[input_tok_attr] != atof(latab[la].la_kw))
		   && (atof(latab[la].la_kw) != -1))
		  continue;
	    }
	    idx = latab[la].prod_idx;

	    /* Here we stack identifiers onto the var_queue */
	    switch (idx) {
	       /* setup ATTRIB */
	    case 120:
	       var_queue_size = 1;
	       var_queue[0] = TYPE_ATTRIB;
	       ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	       var_queue[var_queue_size] = peek_tok_attr;
	       var_queue_size++;

	       if (init_vars(s, var_queue, var_queue_size) == ARB_VP_ERROR)
		  return ARB_VP_ERROR;
	       break;

	       /* stack PARAM */
	    case 134:
	       var_queue_size = 1;
	       var_queue[0] = TYPE_PARAM;

	       ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	       var_queue[var_queue_size] = peek_tok_attr;
	       var_queue_size++;

	       break;

	    case 135:
	       var_queue[0] = TYPE_PARAM_SINGLE;

	       if (init_vars(s, var_queue, var_queue_size) == ARB_VP_ERROR)
		  return ARB_VP_ERROR;
	       break;

	    case 136:
	       var_queue[0] = TYPE_PARAM_ARRAY;

	       if (init_vars(s, var_queue, var_queue_size) == ARB_VP_ERROR)
		  return ARB_VP_ERROR;
	       break;

	       /* stack TEMP */
	    case 284:
	       var_queue_size = 1;
	       var_queue[0] = TYPE_TEMP;
	       break;

	       /* stack ADDRESS */
	    case 285:
	       var_queue_size = 1;
	       var_queue[0] = TYPE_ADDRESS;
	       break;

	       /* stack OUTPUT */
	    case 288:
	       var_queue_size = 1;
	       var_queue[0] = TYPE_OUTPUT;
	       ret = peek_next_token(s, &peek_tok, &peek_tok_attr, 1);
	       var_queue[var_queue_size] = peek_tok_attr;
	       var_queue_size++;

	       if (init_vars(s, var_queue, var_queue_size) == ARB_VP_ERROR)
		  return ARB_VP_ERROR;
	       break;


	       /* stack opts for varNameList  -- see above */

	    }

	    /* finally.. we match! apply the production */
	    if ((idx < 0) || (idx >= nprods)) {
	       fprintf(stderr,
		       "Prod IDX of %d in look ahead table [%d] is horked!\n",
		       idx, la);
	       exit(1);
	    }

	    apply_production(s, idx);
	    is_nonterm = 1;
	    break;
	 }

	 if (!is_nonterm) {
	    if ((stack_tok == EOF_TOKEN) && (s->stack_head == NULL))
	       return ARB_VP_SUCESS;

	    if ((input_tok == stack_tok) ||
		((stack_tok == FLOAT_TOKEN)
		 && (input_tok == INTEGER_TOKEN))) {
	       /* XXX: Need to check values for int/id/GLfloat tokens here -- yes! */

	       _stack_pop(s, &stack_tok, &stack_tok_attr, &ptn);
	       if ((ptn)
		   && ((stack_tok == FLOAT_TOKEN)
		       || (stack_tok == INTEGER_TOKEN)
		       || (stack_tok == ID_TOKEN))) {
		  ptn->is_terminal = 1;
		  ptn->tok = input_tok;
		  ptn->tok_attr = input_tok_attr;
	       }

	       if (get_next_token(s, &input_tok, &input_tok_attr) ==
		   ARB_VP_ERROR) {
		  fprintf(stderr, "PARSE ERROR!!\n");
		  return ARB_VP_ERROR;
	       }
	    }
	    else {
	       fprintf(stderr, "PARSE ERROR!\n");
	       return ARB_VP_ERROR;
	    }
	 }
	 break;
      }
   }

   return ARB_VP_SUCESS;


}

/**
 * Print out the parse tree from a given point. Just for debugging.
 *
 * \param s    The parse state
 * \param ptn  The root to start printing from
 */
static void
print_parse_tree(parse_state * s, parse_tree_node * ptn)
{
   GLint a;

   /* If we're terminal, prGLint and exit */
   if (ptn->is_terminal) {
      debug_token(s, ptn->tok, ptn->tok_attr);
      printf("\n");
      return;
   }

   /* Else, recurse on all our children */
   for (a = 0; a < 4; a++) {
      if (!ptn->children[a])
	 return;

      print_parse_tree(s, ptn->children[a]);
   }
}

/**
 * Free all of the children of a given parse tree node. 
 *
 * \param ptn    The root node whose children we recursively free
 */
static void
parse_tree_free_children(parse_tree_node * ptn)
{
   GLint a;

   if (!ptn)
      return;
   if (!ptn->children[0])
      return;

   for (a = 0; a < 4; a++) {
      if (ptn->children[a]) {
	 parse_tree_free_children(ptn->children[a]);
	 _mesa_free(ptn->children[a]);
	 ptn->children[a] = NULL;
      }
   }
}

/**
 * Given the name of a matrix, and a modifier, expand into a binding type.
 *
 * names:  0 -- modelview
 *         1 -- projection
 *         2 -- mvp
 *         3 -- texture
 *         4 -- palette
 *         5 -- program
 *
 * mods:   0 -- normal
 *         1 -- inverse
 *         2 -- invtrans
 *         3 -- transpose
 *
 * \param name  The number of the matrix type
 * \param mod   The number of the matrix modifier
 *
 * \return The binding state corresponding to name & mod
 */
static GLint
name_and_mod_to_matrixrows(GLint name, GLint mod)
{
   switch (name) {
   case 0:			/* modelview */
      switch (mod) {
      case 0:
	 return MATRIXROWS_MODELVIEW;
      case 1:
	 return MATRIXROWS_MODELVIEW_INVERSE;
      case 2:
	 return MATRIXROWS_MODELVIEW_INVTRANS;
      case 3:
	 return MATRIXROWS_MODELVIEW_TRANSPOSE;
      }
      break;
   case 1:			/* projection */
      switch (mod) {
      case 0:
	 return MATRIXROWS_PROJECTION;
      case 1:
	 return MATRIXROWS_PROJECTION_INVERSE;
      case 2:
	 return MATRIXROWS_PROJECTION_INVTRANS;
      case 3:
	 return MATRIXROWS_PROJECTION_TRANSPOSE;
      }

      break;
   case 2:			/* mvp */
      switch (mod) {
      case 0:
	 return MATRIXROWS_MVP;
      case 1:
	 return MATRIXROWS_MVP_INVERSE;
      case 2:
	 return MATRIXROWS_MVP_INVTRANS;
      case 3:
	 return MATRIXROWS_MVP_TRANSPOSE;
      }

      break;
   case 3:			/* texture */
      switch (mod) {
      case 0:
	 return MATRIXROWS_TEXTURE;
      case 1:
	 return MATRIXROWS_TEXTURE_INVERSE;
      case 2:
	 return MATRIXROWS_TEXTURE_INVTRANS;
      case 3:
	 return MATRIXROWS_TEXTURE_TRANSPOSE;
      }
      break;
   case 4:			/* palette */
      switch (mod) {
      case 0:
	 return MATRIXROWS_PALETTE;
      case 1:
	 return MATRIXROWS_PALETTE_INVERSE;
      case 2:
	 return MATRIXROWS_PALETTE_INVTRANS;
      case 3:
	 return MATRIXROWS_PALETTE_TRANSPOSE;
      }
      break;
   case 5:			/* program */
      switch (mod) {
      case 0:
	 return MATRIXROWS_PROGRAM;
      case 1:
	 return MATRIXROWS_PROGRAM_INVERSE;
      case 2:
	 return MATRIXROWS_PROGRAM_INVTRANS;
      case 3:
	 return MATRIXROWS_PROGRAM_TRANSPOSE;
      }
      break;
   }

   return 0;
}

/**
 * This takes a node in the parse tree for an OPTIONAL_MASK token
 * being derived into a write mask for a register. 
 *
 * This will expand the production number into a 4-component
 * write mask.
 *
 * \param mask_node  The parse tree node for the optional_mask non-termina
 * \param mask       4-component write mask
 */
static void
get_optional_mask(parse_tree_node * mask_node, GLint * mask)
{
   if (mask_node->prod_applied == 97) {
      switch (mask_node->children[0]->prod_applied) {
      case 99:			/* x  */
	 mask[0] = 1;
	 mask[1] = 0;
	 mask[2] = 0;
	 mask[3] = 0;
	 break;

      case 100:		/* y */
	 mask[0] = 0;
	 mask[1] = 1;
	 mask[2] = 0;
	 mask[3] = 0;
	 break;

      case 101:		/* xy */
	 mask[0] = 1;
	 mask[1] = 1;
	 mask[2] = 0;
	 mask[3] = 0;
	 break;

      case 102:		/* z */
	 mask[0] = 0;
	 mask[1] = 0;
	 mask[2] = 1;
	 mask[3] = 0;
	 break;

      case 103:		/* xz */
	 mask[0] = 1;
	 mask[1] = 0;
	 mask[2] = 1;
	 mask[3] = 0;
	 break;

      case 104:		/* yz */
	 mask[0] = 0;
	 mask[1] = 1;
	 mask[2] = 1;
	 mask[3] = 0;
	 break;

      case 105:		/* xyz */
	 mask[0] = 1;
	 mask[1] = 1;
	 mask[2] = 1;
	 mask[3] = 0;
	 break;

      case 106:		/* w */
	 mask[0] = 0;
	 mask[1] = 0;
	 mask[2] = 0;
	 mask[3] = 1;
	 break;

      case 107:		/* xw */
	 mask[0] = 1;
	 mask[1] = 0;
	 mask[2] = 0;
	 mask[3] = 1;
	 break;

      case 108:		/* yw */
	 mask[0] = 0;
	 mask[1] = 1;
	 mask[2] = 0;
	 mask[3] = 1;
	 break;

      case 109:		/* xyw */
	 mask[0] = 1;
	 mask[1] = 1;
	 mask[2] = 0;
	 mask[3] = 1;
	 break;

      case 110:		/* zw */
	 mask[0] = 0;
	 mask[1] = 0;
	 mask[2] = 1;
	 mask[3] = 1;
	 break;

      case 111:		/* xzw */
	 mask[0] = 1;
	 mask[1] = 0;
	 mask[2] = 1;
	 mask[3] = 1;
	 break;

      case 112:		/* yzw */
	 mask[0] = 0;
	 mask[1] = 1;
	 mask[2] = 1;
	 mask[3] = 1;
	 break;

      case 113:		/* xyzw */
	 mask[0] = 1;
	 mask[1] = 1;
	 mask[2] = 1;
	 mask[3] = 1;
	 break;
      }
   }
}

/**
 * Given a MASKED_DST_REG token in a parse tree node, figure out what 
 * register number and write mask the production results in.
 *
 * \param  s         The parse state
 * \param  mdr       The parse tree node
 * \param  dest      The destination register number
 * \param  dest_mask The 4-component write mask
 */
static void
get_masked_dst_reg(parse_state * s, parse_tree_node * mdr, GLint * dest,
		   GLint * dest_mask)
{
   GLint a;

   /* dest is a TEMP variable */
   if (mdr->children[0]->prod_applied == 67) {
      a = mdr->children[0]->children[0]->children[0]->children[0]->tok_attr;
      *dest = s->binds.reg_num[s->idents.attr[a]];
      printf("dst reg: %d (%s)\n", *dest, s->idents.data[a]);
   }
   else {
      /* dest is a result variable */
      if (mdr->children[0]->children[0]->prod_applied == 86) {
	 a = mdr->children[0]->children[0]->children[0]->children[0]->
	    tok_attr;
	 *dest = s->binds.reg_num[s->idents.attr[a]];
	 printf("dest reg: %d (%s)\n", *dest, s->idents.data[a]);
      }
      /* dest is an implicit binding to result/output state */
      else {
	 a = mdr->children[0]->children[0]->children[0]->tok_attr;
	 *dest = s->binds.reg_num[a];
	 printf("dst: %d\n", *dest);
      }
   }

   /* mdr->children[1] is the write mask */
   get_optional_mask(mdr->children[1], dest_mask);
}


/**
 * Given a parse tree node with a swizzled src token, figure out the swizzle
 * mask.
 *
 * \param s       The parse state
 * \param ssr     The parse tree node
 * \param swz     The 4-component swizzle, 0 - x, 1 - y, 2 - z, 3 - w
 */
static void
get_src_swizzle(parse_state * s, parse_tree_node * ssr, GLint * swz)
{
   GLint a;
   GLubyte *c;

   if (ssr->prod_applied == 317) {
      if (ssr->children[0]->prod_applied == 318) {	/* single component */
	 switch (ssr->children[0]->children[0]->prod_applied) {
	 case 93:		/* x */
	    for (a = 0; a < 4; a++)
	       swz[a] = 0;
	    break;

	 case 94:		/* y */
	    for (a = 0; a < 4; a++)
	       swz[a] = 1;
	    break;

	 case 95:		/* z */
	    for (a = 0; a < 4; a++)
	       swz[a] = 2;
	    break;

	 case 96:		/* w */
	    for (a = 0; a < 4; a++)
	       swz[a] = 3;
	    break;
	 }
      }
      else {			/* 4-component */

	 c = s->idents.data[ssr->children[0]->children[0]->children[0]->
			    tok_attr];
	 for (a = 0; a < 4; a++) {
	    switch (c[a]) {
	    case 'x':
	       swz[a] = 0;
	       break;
	    case 'y':
	       swz[a] = 1;
	       break;
	    case 'z':
	       swz[a] = 2;
	       break;
	    case 'w':
	       swz[a] = 3;
	       break;
	    }
	 }
      }
   }
}


/**
 * Given a parse tree node for a src register with an optional sign, figure out
 * what register the src maps to, and what the sign is
 *
 * \param s     The parse state
 * \param ssr   The parse tree node to work from
 * \param sign  The sign (1 or -1)
 * \param ssrc  The src register number
 */
static void
get_optional_sign_and_src_reg(parse_state * s, parse_tree_node * ssr,
			      int *sign, int *ssrc)
{
   GLint a;

   /* ssr->children[0] is the optionalSign  */
   if (ssr->children[0]->prod_applied == 282) {	/* - */
      *sign = -1;
   }

   /* ssr->children[1] is the srcReg        */

   /* The src is a vertex attrib */
   if (ssr->children[1]->prod_applied == 64) {
      if (ssr->children[1]->children[0]->prod_applied == 69) {	/* variable */
	 a = ssr->children[1]->children[0]->children[0]->children[0]->
	    tok_attr;
	 *ssrc = s->binds.reg_num[s->idents.attr[a]];
	 printf("src reg: %d (%s)\n", *ssrc, s->idents.data[a]);
      }
      else {			/* implicit binding */

	 a = ssr->children[1]->children[0]->children[0]->tok_attr;
	 *ssrc = s->binds.reg_num[a];
	 printf("src reg: %d %d (implicit binding)\n",
		*ssrc, s->binds.type[a]);
      }
   }
   else
      /* The src is a temp variable */
   if (ssr->children[1]->prod_applied == 65) {
      a = ssr->children[1]->children[0]->children[0]->children[0]->tok_attr;
      *ssrc = s->binds.reg_num[s->idents.attr[a]];
      printf("src reg: %d (%s)\n", *ssrc, s->idents.data[a]);
   }
   /* The src is a param */
   else {
      /* We have a single item */
      if (ssr->children[1]->children[0]->prod_applied == 72) {
	 a = ssr->children[1]->children[0]->children[0]->children[0]->
	    children[0]->tok_attr;
	 *ssrc = s->binds.reg_num[s->idents.attr[a]];
	 printf("src reg: %d (%s)\n", *ssrc, s->idents.data[a]);
      }
      else
	 /* We have an array of items */
      if (ssr->children[1]->children[0]->prod_applied == 73) {
	 a = ssr->children[1]->children[0]->children[0]->children[0]->
	    children[0]->tok_attr;
	 *ssrc = s->binds.reg_num[s->idents.attr[a]];

	 /* We have an absolute offset into the array */
	 if (ssr->children[1]->children[0]->children[1]->prod_applied == 77) {
	    /* Ok, are array will be layed out fully in registers, so we can compute the reg */
	    printf("array base: %s\n", s->idents.data[a]);
	    a = ssr->children[1]->children[0]->children[1]->children[0]->
	       children[0]->tok_attr;
	    printf("array absolute offset: %d\n", s->ints.data[a]);
	    *ssrc += s->ints.data[a];
	 }
	 /* Otherwise, we have to grab the offset register */
	 else {			/* progParamArrayRel */

	    /* XXX: We don't know the offset, so we have to grab the offset register # */
	 }
      }
      /* Otherwise, we have an implicit binding */
      else {			/* paramSingleItemUse */

	 if (ssr->children[1]->children[0]->children[0]->prod_applied == 148) {	/* programSingleItem */
	    a = ssr->children[1]->children[0]->children[0]->children[0]->
	       tok_attr;
	 }
	 else {
	    a = ssr->children[1]->children[0]->children[0]->children[0]->
	       children[0]->tok_attr;
	 }
	 *ssrc = s->binds.reg_num[a];
	 printf("src reg: %d %d (implicit binding)\n", *ssrc,
		s->binds.type[a]);
      }
   }
}


/**
 * Figure out what register a src reg is in, as well as the swizzle mask and the 
 * sign
 *
 * \param s    The parse state
 * \param ssr  The swizzeled src reg parse tree node
 * \param sign The return value for the sign {1, -1}
 * \param ssrc The return value for the register number
 * \param swz  The 4-component swizzle mask
 */
static void
get_swizzle_src_reg(parse_state * s, parse_tree_node * ssr, GLint * sign,
		    GLint * ssrc, GLint * swz)
{
   get_optional_sign_and_src_reg(s, ssr, sign, ssrc);

   /* ssr->children[2] is the swizzleSuffix */
   get_src_swizzle(s, ssr->children[2], swz);
}

/**
 *	Just like get_swizzle_src_reg, but find the scalar value to use from the register instead
 *	of the swizzle mask
 *
 * \param s       The parse state
 * \param ssr     The swizzeled src reg parse tree node
 * \param sign    The return value for the sign {1, -1}
 * \param ssrc    The return value for the register number
 * \param scalar  The 1-component scalar number
 */
static void
get_scalar_src_reg(parse_state * s, parse_tree_node * ssr, GLint * sign,
		   GLint * ssrc, GLint * scalar)
{
   get_optional_sign_and_src_reg(s, ssr, sign, ssrc);

   /* sn->children[2] is a scalarSuffix  */
   switch (ssr->children[2]->children[0]->prod_applied) {
   case 93:
      *scalar = 0;
      break;
   case 94:
      *scalar = 1;
      break;
   case 95:
      *scalar = 2;
      break;
   case 96:
      *scalar = 3;
      break;
   }
}

/**
 * Recursivly traverse the parse tree and generate Mesa opcodes 
 *
 * \param s    The parse state
 * \param ptn  The parse tree node to process
 */
static void
parse_tree_generate_opcodes(parse_state * s, parse_tree_node * ptn)
{
   GLint a;
   GLint opcode, dst, src[3];
   GLint dst_mask[4], src_swz[3][4], src_scalar[2], src_sign[3];
   parse_tree_node *dn, *sn[3];

   src_sign[0] = src_sign[1] = src_sign[2] = 1;
   for (a = 0; a < 4; a++) {
      src_swz[0][a] = a;
      src_swz[1][a] = a;
      src_swz[2][a] = a;
   }
   src_scalar[0] = src_scalar[1] = src_scalar[2] = 0;
   dst_mask[0] = dst_mask[1] = dst_mask[2] = dst_mask[3] = 1;

   switch (ptn->prod_applied) {
   case 17:			/* ARL */
      opcode = VP_OPCODE_ARL;

      dn = ptn->children[0];
      sn[0] = ptn->children[1];

      /* dn is a maskedAddrReg */
      /* dn->children[0] is an addrReg */
      /* dn->children[1] is an addrWriteMask */
      /* XXX: do this.. */
      break;

   case 18:			/* VECTORop */
      switch (ptn->children[0]->prod_applied) {
      case 19:			/* ABS */
	 opcode = VP_OPCODE_ABS;
	 break;
      case 20:			/* FLR */
	 opcode = VP_OPCODE_FLR;
	 break;
      case 21:			/* FRC */
	 opcode = VP_OPCODE_FRC;
	 break;
      case 22:			/* LIT */
	 opcode = VP_OPCODE_LIT;
	 break;
      case 23:			/* MOV */
	 opcode = VP_OPCODE_MOV;
	 break;
      }
      printf("opcode: %d\n", opcode);

      /* dn is a maskedDstReg */
      dn = ptn->children[1];

      /* sn is a swizzleSrcReg */
      sn[0] = ptn->children[2];

      get_masked_dst_reg(s, dn, &dst, dst_mask);
      printf("dst: %d mask: %d %d %d %d\n", dst, dst_mask[0], dst_mask[1],
	     dst_mask[2], dst_mask[3]);

      get_swizzle_src_reg(s, sn[0], &src_sign[0], &src[0], src_swz[0]);

      printf("src sign: %d reg: %d swz: %d %d %d %d\n",
	     src_sign[0], src[0], src_swz[0][0], src_swz[0][1], src_swz[0][2],
	     src_swz[0][3]);
      break;

   case 24:			/* SCALARop */
      switch (ptn->children[0]->prod_applied) {
      case 25:			/* EX2 */
	 opcode = VP_OPCODE_EX2;
	 break;
      case 26:			/* EXP */
	 opcode = VP_OPCODE_EXP;
	 break;
      case 27:			/* LG2 */
	 opcode = VP_OPCODE_LG2;
	 break;
      case 28:			/* LOG */
	 opcode = VP_OPCODE_LOG;
	 break;
      case 29:			/* RCP */
	 opcode = VP_OPCODE_RCP;
	 break;
      case 30:			/* RSQ */
	 opcode = VP_OPCODE_RSQ;
	 break;
      }

      printf("opcode: %d\n", opcode);
      /* dn is a maskedDstReg */
      dn = ptn->children[1];

      get_masked_dst_reg(s, dn, &dst, dst_mask);
      printf("dst: %d mask: %d %d %d %d\n", dst, dst_mask[0], dst_mask[1],
	     dst_mask[2], dst_mask[3]);

      /* sn is a scalarSrcReg */
      sn[0] = ptn->children[2];

      get_scalar_src_reg(s, sn[0], &src_sign[0], &src[0], &src_scalar[0]);
      printf("src sign: %d reg: %d scalar: %d\n", src_sign[0], src[0],
	     src_scalar[0]);
      break;

   case 31:			/* BINSC */
      opcode = VP_OPCODE_POW;

      printf("opcode: %d\n", opcode);
      /* maskedDstReg */
      dn = ptn->children[1];
      get_masked_dst_reg(s, dn, &dst, dst_mask);
      printf("dst: %d mask: %d %d %d %d\n", dst, dst_mask[0], dst_mask[1],
	     dst_mask[2], dst_mask[3]);

      /* sn are scalarSrcReg's */
      sn[0] = ptn->children[2]->children[0];
      sn[1] = ptn->children[2]->children[1];

      get_scalar_src_reg(s, sn[0], &src_sign[0], &src[0], &src_scalar[0]);
      get_scalar_src_reg(s, sn[1], &src_sign[1], &src[1], &src_scalar[1]);

      printf("src0 sign: %d reg: %d scalar: %d\n", src_sign[0], src[0],
	     src_scalar[0]);
      printf("src1 sign: %d reg: %d scalar: %d\n", src_sign[1], src[1],
	     src_scalar[1]);
      break;


   case 34:			/* BIN */
      switch (ptn->children[0]->prod_applied) {
      case 36:			/* ADD */
	 opcode = VP_OPCODE_ADD;
	 break;
      case 37:			/* DP3 */
	 opcode = VP_OPCODE_DP3;
	 break;
      case 38:			/* DP4 */
	 opcode = VP_OPCODE_DP4;
	 break;
      case 39:			/* DPH */
	 opcode = VP_OPCODE_DPH;
	 break;
      case 40:			/* DST */
	 opcode = VP_OPCODE_DST;
	 break;
      case 41:			/* MAX */
	 opcode = VP_OPCODE_MAX;
	 break;
      case 42:			/* MIN */
	 opcode = VP_OPCODE_MIN;
	 break;
      case 43:			/* MUL */
	 opcode = VP_OPCODE_MUL;
	 break;
      case 44:			/* SGE */
	 opcode = VP_OPCODE_SGE;
	 break;
      case 45:			/* SLT */
	 opcode = VP_OPCODE_SLT;
	 break;
      case 46:			/* SUB */
	 opcode = VP_OPCODE_SUB;
	 break;
      case 47:			/* XPD */
	 opcode = VP_OPCODE_XPD;
	 break;
      }

      printf("opcode: %d\n", opcode);

      /* maskedDstReg */
      dn = ptn->children[1];
      get_masked_dst_reg(s, dn, &dst, dst_mask);
      printf("dst: %d mask: %d %d %d %d\n", dst, dst_mask[0], dst_mask[1],
	     dst_mask[2], dst_mask[3]);

      /* sn are scalarSrcReg's */
      sn[0] = ptn->children[2]->children[0];
      sn[1] = ptn->children[2]->children[1];

      get_swizzle_src_reg(s, sn[0], &src_sign[0], &src[0], src_swz[0]);
      get_swizzle_src_reg(s, sn[1], &src_sign[1], &src[1], src_swz[1]);

      printf("src0 sign: %d reg: %d swz: %d %d %d %d\n",
	     src_sign[0], src[0], src_swz[0][0], src_swz[0][1], src_swz[0][2],
	     src_swz[0][3]);
      printf("src1 sign: %d reg: %d swz: %d %d %d %d\n", src_sign[1], src[1],
	     src_swz[1][0], src_swz[1][1], src_swz[1][2], src_swz[1][3]);
      break;

   case 48:			/* TRI */
      opcode = VP_OPCODE_MAD;

      printf("opcode: %d\n", opcode);

      /* maskedDstReg */
      dn = ptn->children[1];
      get_masked_dst_reg(s, dn, &dst, dst_mask);
      printf("dst: %d mask: %d %d %d %d\n", dst, dst_mask[0], dst_mask[1],
	     dst_mask[2], dst_mask[3]);

      /* sn are scalarSrcReg's */
      sn[0] = ptn->children[2]->children[0];
      sn[1] = ptn->children[2]->children[1]->children[0];
      sn[2] = ptn->children[2]->children[1]->children[1];

      get_swizzle_src_reg(s, sn[0], &src_sign[0], &src[0], src_swz[0]);
      get_swizzle_src_reg(s, sn[1], &src_sign[1], &src[1], src_swz[1]);
      get_swizzle_src_reg(s, sn[2], &src_sign[2], &src[2], src_swz[2]);

      printf("src0 sign: %d reg: %d swz: %d %d %d %d\n",
	     src_sign[0], src[0], src_swz[0][0], src_swz[0][1], src_swz[0][2],
	     src_swz[0][3]);
      printf("src1 sign: %d reg: %d swz: %d %d %d %d\n", src_sign[1], src[1],
	     src_swz[1][0], src_swz[1][1], src_swz[1][2], src_swz[1][3]);
      printf("src2 sign: %d reg: %d swz: %d %d %d %d\n", src_sign[2], src[2],
	     src_swz[2][0], src_swz[2][1], src_swz[2][2], src_swz[2][3]);

   }

   for (a = 0; a < 4; a++) {
      if (!ptn->children[a])
	 return;
      parse_tree_generate_opcodes(s, ptn->children[a]);
   }
}

/**
 * When we go to examine the parse tree to generate opcodes, things are not exactly pretty to deal with.
 * Parameters, constants, matricies, attribute bindings, and the like are represented by large numbers
 * of nodes.
 *
 * In order to keep the code generation code cleaner, we make a recursive pass over the parse tree and 'roll up' these deep
 * derivations of the attribs, and replace them with a single token, BINDING_TOKEN. The token attribute for 
 * BINDING_TOKEN is a index in the 'binding table' where all the relavant info on the chunk of state is stored, 
 * e.g its type.
 *
 * For example, the string 'vertex.color.secondary' is represented by 4 productions, and 4 nodes in the parse
 * tree. The token at the root of this derivation is NT_VTX_ATTRIB_BINDING_TOKEN. After this folding, 
 * the token at the root is BINDING_TOKEN, and s->binds[token_attr_at_the_root].type = ATTRIB_COLOR_SECONDARY.
 *
 * \param s    The parse state
 * \param ptn  The root parse tree node to start folding bindings 
 */
static void
parse_tree_fold_bindings(parse_state * s, parse_tree_node * ptn)
{
   GLint a, b;
   GLint eat_children, bind_type, bind_idx, bind_row, bind_nrows;
   GLfloat bind_vals[4];
   parse_tree_node *ptmp;

   eat_children = 0;
   bind_row = 0;
   bind_nrows = 1;
   bind_vals[0] = bind_vals[1] = bind_vals[2] = bind_vals[3];
   switch (ptn->prod_applied) {
      /* vertex */
   case 121:
      eat_children = 1;
      bind_idx = 0;
      switch (ptn->children[0]->prod_applied) {
      case 122:		/* position */
	 bind_type = ATTRIB_POSITION;
	 break;
      case 123:		/* weight */
	 bind_type = ATTRIB_WEIGHT;
	 if (ptn->children[0]->children[0]->prod_applied == 132) {
	    bind_idx =
	       s->ints.data[ptn->children[0]->children[0]->children[0]->
			    children[0]->tok_attr];
	 }
	 break;
      case 124:		/* normal */
	 bind_type = ATTRIB_NORMAL;
	 break;
      case 125:		/* color */
	 bind_type = ATTRIB_COLOR_PRIMARY;
	 if (ptn->children[0]->children[0]->prod_applied == 306) {
	    if (ptn->children[0]->children[0]->children[0]->prod_applied ==
		309)
	       bind_type = ATTRIB_COLOR_SECONDARY;
	 }
	 break;
      case 126:		/* fogcoord */
	 bind_type = ATTRIB_FOGCOORD;
	 break;
      case 127:		/* texcoord */
	 bind_type = ATTRIB_TEXCOORD;
	 if (ptn->children[0]->children[0]->prod_applied == 311) {
	    bind_idx =
	       s->ints.data[ptn->children[0]->children[0]->children[0]->
			    children[0]->tok_attr];
	 }
	 break;
      case 128:		/* matrixindex */
	 bind_type = ATTRIB_MATRIXINDEX;
	 bind_idx =
	    s->ints.data[ptn->children[0]->children[0]->children[0]->
			 tok_attr];
	 break;
      case 129:		/* attrib */
	 bind_type = ATTRIB_ATTRIB;
	 bind_idx =
	    s->ints.data[ptn->children[0]->children[0]->children[0]->
			 tok_attr];
	 break;
      }
      break;

      /* state */
   case 154:
   case 172:			/* material */
      eat_children = 2;
      bind_idx = 0;
      ptmp = ptn->children[0]->children[0];

      a = 0;
      if (ptmp->prod_applied == 182) {
	 a = 1;
	 b = 0;
      }
      else
	 if ((ptmp->prod_applied == 183)
	     && (ptmp->children[0]->prod_applied == 305)) {
	 a = 1;
	 b = 1;
      }

      /* no explicit face, or explicit front */
      if (a) {
	 switch (ptmp->children[b]->prod_applied) {
	 case 184:		/* ambient */
	    bind_type = MATERIAL_FRONT_AMBIENT;
	    break;
	 case 185:		/* diffuse */
	    bind_type = MATERIAL_FRONT_DIFFUSE;
	    break;
	 case 186:		/* specular */
	    bind_type = MATERIAL_FRONT_SPECULAR;
	    break;
	 case 187:		/* emission */
	    bind_type = MATERIAL_FRONT_EMISSION;
	    break;
	 case 188:		/* shininess */
	    bind_type = MATERIAL_FRONT_SHININESS;
	    break;
	 }
      }
      /* has explicit back face */
      else {
	 switch (ptmp->children[1]->prod_applied) {
	 case 184:		/* ambient */
	    bind_type = MATERIAL_BACK_AMBIENT;
	    break;
	 case 185:		/* diffuse */
	    bind_type = MATERIAL_BACK_DIFFUSE;
	    break;
	 case 186:		/* specular */
	    bind_type = MATERIAL_BACK_SPECULAR;
	    break;
	 case 187:		/* emission */
	    bind_type = MATERIAL_BACK_EMISSION;
	    break;
	 case 188:		/* shininess */
	    bind_type = MATERIAL_BACK_SHININESS;
	    break;
	 }
      }
      break;
   case 155:
   case 173:			/* light */
      eat_children = 2;
      bind_idx = 0;
      printf("FOLDING LIGHT!\n");
      ptmp = ptn->children[0];
      bind_idx = s->ints.data[ptmp->children[0]->children[0]->tok_attr];
      switch (ptmp->children[1]->children[0]->prod_applied) {
      case 191:		/* ambient */
	 bind_type = LIGHT_AMBIENT;
	 break;
      case 192:		/* diffuse */
	 bind_type = LIGHT_DIFFUSE;
	 break;
      case 193:		/* specular */
	 bind_type = LIGHT_SPECULAR;
	 break;
      case 194:		/* position */
	 bind_type = LIGHT_POSITION;
	 break;
      case 195:		/* attenuation */
	 bind_type = LIGHT_ATTENUATION;
	 break;
      case 196:		/* spot */
	 bind_type = LIGHT_SPOT_DIRECTION;
	 break;
      case 197:		/* half */
	 bind_type = LIGHT_HALF;
	 break;
      }
      break;

   case 156:
   case 174:			/* lightmodel */
      eat_children = 2;
      bind_idx = 0;

      ptmp = ptn->children[0];
      switch (ptmp->prod_applied) {
      case 201:		/* ambient */
	 bind_type = LIGHTMODEL_AMBIENT;
	 break;
      case 202:		/* scenecolor */
	 bind_type = LIGHTMODEL_FRONT_SCENECOLOR;
	 break;
      case 203:		/* foo.scenecolor */
	 if (ptmp->children[0]->prod_applied == 304)
	    bind_type = LIGHTMODEL_FRONT_SCENECOLOR;
	 else
	    bind_type = LIGHTMODEL_BACK_SCENECOLOR;
      }
      break;
   case 157:
   case 175:			/* lightprod */
      eat_children = 2;
      bind_idx = 0;

      ptmp = ptn->children[0];
      bind_idx = s->ints.data[ptmp->children[0]->children[0]->tok_attr];
      /* No explicit face */
      if (ptmp->children[1]->children[0]->prod_applied == 206) {
	 a = 1;			/* front */
	 b = 0;			/* 0-th child */
      }
      else
	 if ((ptmp->children[1]->children[0]->prod_applied == 207) &&
	     (ptmp->children[1]->children[0]->children[0]->prod_applied ==
	      304)) {
	 a = 1;			/* front */
	 b = 1;			/* 1-th child */
      }
      else {
	 a = 0;
	 b = 1;
      }
      if (a) {
	 switch (ptmp->children[1]->children[0]->children[b]->prod_applied) {
	 case 208:		/* ambient */
	    bind_type = LIGHTPROD_FRONT_AMBIENT;
	    break;
	 case 209:		/* diffuse */
	    bind_type = LIGHTPROD_FRONT_DIFFUSE;
	    break;
	 case 210:		/* specular */
	    bind_type = LIGHTPROD_FRONT_SPECULAR;
	    break;
	 }
      }
      else {
	 switch (ptmp->children[1]->children[0]->children[b]->prod_applied) {
	 case 208:		/* ambient */
	    bind_type = LIGHTPROD_BACK_AMBIENT;
	    break;
	 case 209:		/* diffuse */
	    bind_type = LIGHTPROD_BACK_DIFFUSE;
	    break;
	 case 210:		/* specular */
	    bind_type = LIGHTPROD_BACK_SPECULAR;
	    break;
	 }
      }
      break;
   case 158:
   case 176:			/* texgen */
      eat_children = 2;
      bind_idx = 0;

      ptmp = ptn->children[0];
      if (ptmp->children[0]->prod_applied == 311)
	 bind_idx =
	    s->ints.data[ptmp->children[0]->children[0]->children[0]->
			 tok_attr];
      ptmp = ptn->children[0]->children[1];
      if (ptmp->children[0]->prod_applied == 214)
	 a = 1;			/* eye */
      else
	 a = 0;			/* object */
      b = ptmp->children[1]->prod_applied - 216;
      if (a == 1) {
	 switch (b) {
	 case 0:
	    bind_type = TEXGEN_EYE_S;
	    break;
	 case 1:
	    bind_type = TEXGEN_EYE_T;
	    break;
	 case 2:
	    bind_type = TEXGEN_EYE_R;
	    break;
	 case 3:
	    bind_type = TEXGEN_EYE_Q;
	    break;
	 }
      }
      else {
	 switch (b) {
	 case 0:
	    bind_type = TEXGEN_OBJECT_S;
	    break;
	 case 1:
	    bind_type = TEXGEN_OBJECT_T;
	    break;
	 case 2:
	    bind_type = TEXGEN_OBJECT_R;
	    break;
	 case 3:
	    bind_type = TEXGEN_OBJECT_Q;
	    break;
	 }
      }
      break;
   case 159:
   case 177:			/* fog */
      eat_children = 2;
      bind_idx = 0;

      ptmp = ptn->children[0];
      if (ptmp->children[0]->prod_applied == 221)
	 bind_type = FOG_COLOR;
      else
	 bind_type = FOG_PARAMS;
      break;
   case 160:
   case 178:			/* clip */
      eat_children = 2;
      bind_idx = 0;

      ptmp = ptn->children[0];
      bind_idx = s->ints.data[ptmp->children[0]->children[0]->tok_attr];
      bind_type = CLIP_PLANE;
      break;
   case 161:
   case 179:			/* point */
      eat_children = 2;
      bind_idx = 0;

      ptmp = ptn->children[0];
      if (ptmp->children[0]->prod_applied == 227)
	 bind_type = POINT_SIZE;
      else
	 bind_type = POINT_ATTENUATION;
      break;

   case 162:			/* matrix rows/whole matrix */
      eat_children = 2;
      bind_idx = 0;
      {
	 parse_tree_node *mname;
	 GLint mod = 0;
	 GLint name = 0;

	 mname = ptn->children[0];
	 switch (mname->prod_applied) {
	 case 238:		/* modelview */
	    name = 0;
	    if (mname->children[0]->prod_applied == 245)
	       bind_idx =
		  s->ints.data[mname->children[0]->children[0]->children[0]->
			       tok_attr];
	    break;
	 case 239:		/* projection */
	    name = 1;
	    break;
	 case 240:		/* mvp */
	    name = 2;
	    break;
	 case 241:		/* texture */
	    if (mname->children[0]->prod_applied == 311)
	       bind_idx =
		  s->ints.data[mname->children[0]->children[0]->children[0]->
			       tok_attr];
	    name = 3;
	    break;
	 case 242:		/* palette */
	    bind_idx =
	       s->ints.data[mname->children[0]->children[0]->tok_attr];
	    name = 4;
	    break;
	 case 243:		/* program */
	    bind_idx =
	       s->ints.data[mname->children[0]->children[0]->tok_attr];
	    name = 5;
	    break;
	 }

	 ptmp = ptn->children[1];
	 if (ptmp->prod_applied == 316) {
	    bind_type = name_and_mod_to_matrixrows(name, mod);
	    bind_row = 0;
	    bind_nrows = 4;
	 }
	 else {
	    if (ptmp->children[0]->prod_applied == 164) {
	       switch (ptmp->children[0]->children[0]->prod_applied) {
	       case 234:	/* inverse */
		  mod = 1;
		  break;
	       case 235:	/* transpose */
		  mod = 3;
		  break;
	       case 236:	/* invtrans */
		  mod = 2;
		  break;
	       }
	       if (ptmp->children[0]->children[1]->prod_applied == 166) {
		  bind_type = name_and_mod_to_matrixrows(name, mod);
		  bind_row = 0;
		  bind_nrows = 4;
	       }
	       else {		/* prod 167 */

		  bind_type = name_and_mod_to_matrixrows(name, mod);
		  bind_row =
		     s->ints.data[ptmp->children[0]->children[1]->
				  children[0]->children[0]->children[0]->
				  tok_attr];
		  if (ptmp->children[0]->children[1]->children[0]->
		      children[1]->prod_applied == 169)
		     bind_nrows = 1;
		  else {
		     bind_nrows =
			s->ints.data[ptmp->children[0]->children[1]->
				     children[0]->children[1]->children[0]->
				     children[0]->tok_attr] - bind_row + 1;
		  }
	       }
	    }
	    else {		/* prod 165 */

	       bind_type = name_and_mod_to_matrixrows(name, mod);

	       bind_row =
		  s->ints.data[ptmp->children[0]->children[0]->children[0]->
			       tok_attr];
	       if (ptmp->children[0]->children[1]->prod_applied == 169)
		  bind_nrows = 1;
	       else
		  bind_nrows =
		     s->ints.data[ptmp->children[0]->children[1]->
				  children[0]->children[0]->tok_attr] -
		     bind_row + 1;
	    }
	 }
      }

      printf("folding matrixrows: %d %d %d %d\n", bind_type, bind_idx,
	     bind_row, bind_nrows);
      break;

   case 180:			/* matrix row */
      eat_children = 2;
      bind_idx = 0;

      {
	 GLint mod;
	 parse_tree_node *mname, *mrow;

	 ptmp = ptn->children[0];
	 mname = ptmp->children[0];
	 mod = 0;
	 if (ptmp->children[1]->children[0]->prod_applied == 232) {
	    mrow =
	       ptmp->children[1]->children[0]->children[1]->children[0]->
	       children[0];
	    switch (ptmp->children[1]->children[0]->children[0]->prod_applied) {
	    case 234:
	       mod = 1;		/* inverse */
	       break;
	    case 235:
	       mod = 2;		/* transpose */
	       break;
	    case 236:
	       mod = 3;		/* invtrans */
	       break;
	    }
	 }
	 else {
	    mrow = ptmp->children[1]->children[0]->children[0]->children[0];
	 }
	 bind_row = s->ints.data[mrow->tok_attr];

	 switch (mname->prod_applied) {
	 case 238:		/* modelview */
	    if (mname->children[0]->prod_applied == 245) {
	       bind_idx =
		  s->ints.data[mname->children[0]->children[0]->children[0]->
			       tok_attr];
	    }
	    switch (mod) {
	    case 0:
	       bind_type = MATRIXROW_MODELVIEW;
	       break;
	    case 1:
	       bind_type = MATRIXROW_MODELVIEW_INVERSE;
	       break;
	    case 2:
	       bind_type = MATRIXROW_MODELVIEW_TRANSPOSE;
	       break;
	    case 3:
	       bind_type = MATRIXROW_MODELVIEW_INVTRANS;
	    }
	    break;

	 case 239:		/* projection */
	    switch (mod) {
	    case 0:
	       bind_type = MATRIXROW_PROJECTION;
	       break;
	    case 1:
	       bind_type = MATRIXROW_PROJECTION_INVERSE;
	       break;
	    case 2:
	       bind_type = MATRIXROW_PROJECTION_TRANSPOSE;
	       break;
	    case 3:
	       bind_type = MATRIXROW_PROJECTION_INVTRANS;
	    }
	    break;

	 case 240:		/* mvp */
	    switch (mod) {
	    case 0:
	       bind_type = MATRIXROW_MVP;
	       break;
	    case 1:
	       bind_type = MATRIXROW_MVP_INVERSE;
	       break;
	    case 2:
	       bind_type = MATRIXROW_MVP_TRANSPOSE;
	       break;
	    case 3:
	       bind_type = MATRIXROW_MVP_INVTRANS;
	    }
	    break;

	 case 241:		/* texture */
	    if (mname->children[0]->prod_applied == 311) {
	       bind_idx =
		  s->ints.data[mname->children[0]->children[0]->children[0]->
			       tok_attr];
	    }
	    switch (mod) {
	    case 0:
	       bind_type = MATRIXROW_TEXTURE;
	       break;
	    case 1:
	       bind_type = MATRIXROW_TEXTURE_INVERSE;
	       break;
	    case 2:
	       bind_type = MATRIXROW_TEXTURE_TRANSPOSE;
	       break;
	    case 3:
	       bind_type = MATRIXROW_TEXTURE_INVTRANS;
	    }
	    break;

	 case 242:		/* palette */
	    bind_idx =
	       s->ints.data[mname->children[0]->children[0]->tok_attr];
	    switch (mod) {
	    case 0:
	       bind_type = MATRIXROW_PALETTE;
	       break;
	    case 1:
	       bind_type = MATRIXROW_PALETTE_INVERSE;
	       break;
	    case 2:
	       bind_type = MATRIXROW_PALETTE_TRANSPOSE;
	       break;
	    case 3:
	       bind_type = MATRIXROW_PALETTE_INVTRANS;
	    }
	    break;

	 case 243:		/* program */
	    bind_idx =
	       s->ints.data[mname->children[0]->children[0]->tok_attr];
	    switch (mod) {
	    case 0:
	       bind_type = MATRIXROW_PROGRAM;
	       break;
	    case 1:
	       bind_type = MATRIXROW_PROGRAM_INVERSE;
	       break;
	    case 2:
	       bind_type = MATRIXROW_PROGRAM_TRANSPOSE;
	       break;
	    case 3:
	       bind_type = MATRIXROW_PROGRAM_INVTRANS;
	    }
	    break;
	 }
      }
      break;

      /* program (single) */
   case 249:
      eat_children = 1;
      bind_idx = 0;
      switch (ptn->children[0]->prod_applied) {
      case 250:		/* env */
	 bind_type = PROGRAM_ENV_SINGLE;
	 break;
      case 251:		/* local */
	 bind_type = PROGRAM_LOCAL_SINGLE;
	 break;
      }
      bind_idx =
	 s->ints.data[ptn->children[0]->children[0]->children[0]->
		      children[0]->tok_attr];
      break;

      /* program (multi) */
   case 252:
      eat_children = 1;
      bind_idx = 0;
      switch (ptn->children[0]->prod_applied) {
      case 253:		/* env */
      case 254:		/* local */
	 if (ptn->children[0]->prod_applied == 253)
	    bind_type = PROGRAM_ENV_MULTI;
	 else
	    bind_type = PROGRAM_LOCAL_MULTI;

	 ptmp = ptn->children[0]->children[0]->children[0];
	 bind_idx = bind_row =
	    s->ints.data[ptmp->children[0]->children[0]->tok_attr];
	 bind_nrows = 1;

	 ptmp = ptn->children[0]->children[0]->children[0]->children[1];
	 if ((ptmp->prod_applied == 257) || (ptmp->prod_applied == 262))
	    bind_nrows =
	       s->ints.data[ptmp->children[0]->children[0]->tok_attr] -
	       bind_idx;
	 break;
      }
      break;

#define FOLD_FLOAT_CONSTANT(float_ptr, bind_vals_idx, sign) \
		if (float_ptr->tok == 49) /* GLfloat */ {\
			bind_vals[bind_vals_idx] = sign * s->floats.data[float_ptr->tok_attr];\
		}\
		else /* GLint */ {\
			bind_vals[bind_vals_idx] = sign * s->ints.data[float_ptr->tok_attr];\
		}

#define FOLD_SIGNED_FLOAT_CONSTANT(sf_ptr, bind_vals_idx) \
		{\
			GLfloat __mul = 1.;\
			if (sf_ptr->children[0]->prod_applied == 282) \
				__mul = -1.;\
			FOLD_FLOAT_CONSTANT(sf_ptr->children[1], bind_vals_idx, __mul);\
		}

      /* const scalar decl */
   case 271:
      eat_children = 1;
      bind_idx = 0;
      bind_type = CONSTANT;

      FOLD_SIGNED_FLOAT_CONSTANT(ptn->children[0], 0);
#if 0
      {
	 GLfloat mul = 1.;
	 if (ptn->children[0]->children[0]->prod_applied == 282) {
	    mul = -1;
	 }

	 FOLD_FLOAT_CONSTANT(ptn->children[0]->children[1], 0, mul);
      }
#endif
      break;

      /* const vector */
   case 273:
      eat_children = 1;
      bind_idx = 0;
      bind_type = CONSTANT;

      FOLD_SIGNED_FLOAT_CONSTANT(ptn->children[0], 0);
      if (ptn->children[1]->prod_applied == 275) {
	 FOLD_SIGNED_FLOAT_CONSTANT(ptn->children[1]->children[0], 1);
	 if (ptn->children[1]->children[1]->prod_applied == 277) {
	    FOLD_SIGNED_FLOAT_CONSTANT(ptn->children[1]->children[1]->
				       children[0], 2);
	    if (ptn->children[1]->children[1]->children[1]->prod_applied ==
		279) {
	       FOLD_SIGNED_FLOAT_CONSTANT(ptn->children[1]->children[1]->
					  children[1]->children[0], 3);
	    }
	 }
      }
      break;

      /* result */
   case 289:
      eat_children = 1;
      bind_idx = 0;
      switch (ptn->children[0]->prod_applied) {
      case 290:		/* position */
	 bind_type = RESULT_POSITION;
	 break;
      case 291:		/* fogcoord */
	 bind_type = RESULT_FOGCOORD;
	 break;
      case 292:		/* pointsize */
	 bind_type = RESULT_POINTSIZE;
	 break;
      case 293:		/* color */
	 bind_type = RESULT_COLOR_FRONT_PRIMARY;
	 ptmp = ptn->children[0]->children[0]->children[0];
	 if (ptmp->prod_applied == 297) {
	    if (ptmp->children[0]->prod_applied == 298) {	/* front */
	       if (ptmp->children[0]->children[0]->prod_applied == 301) {
		  if (ptmp->children[0]->children[0]->children[0]->prod_applied == 303)	/* secondary */
		     bind_type = RESULT_COLOR_FRONT_SECONDARY;
	       }
	    }
	    else {		/* back */

	       bind_type = RESULT_COLOR_BACK_PRIMARY;
	       if (ptmp->children[0]->children[0]->prod_applied == 301) {
		  if (ptmp->children[0]->children[0]->children[0]->prod_applied == 303)	/* secondary */
		     bind_type = RESULT_COLOR_BACK_SECONDARY;
	       }

	    }
	 }
	 break;
      case 294:		/* texcoord */
	 bind_type = RESULT_TEXCOORD;
	 if (ptn->children[0]->children[0]->prod_applied == 311) {
	    bind_idx =
	       s->ints.data[ptn->children[0]->children[0]->children[0]->
			    children[0]->tok_attr];
	 }
	 break;
      }
      break;
   }

   /* Mmmmm... baaaaby */
   if (eat_children) {
      if (eat_children == 2)
	 parse_tree_free_children(ptn->children[0]);
      else
	 parse_tree_free_children(ptn);

      /* Insert the binding into the binding table */
      ptn->tok = BINDING_TOKEN;
      ptn->tok_attr =
	 binding_table_add(&s->binds, bind_type, bind_idx, bind_row,
			   bind_nrows, bind_vals);

      printf("Got binding %d %d %d %d at pos %d in bind tab [%f %f %f %f]\n",
	     bind_type, bind_idx, bind_row, bind_nrows, ptn->tok_attr,
	     bind_vals[0], bind_vals[1], bind_vals[2], bind_vals[3]);
   }


   for (a = 0; a < 4; a++) {
      if (!ptn->children[a])
	 return;

      parse_tree_fold_bindings(s, ptn->children[a]);
   }
}

/**
 * After we have figured out what mess of parse tree actually represents GL state (or constants, or 
 * whatnot), we have to line up variables with the state.  For example, a line something like
 *
 *    OUTPUT foo = result.position;
 *
 * We would have 'foo' in the identifier table at some position foo_idx, and 'result.position' in the 
 * binding table at some position res_pos_idx. To set things up such that 'foo' is associated with 
 * the result position state, we need to set ident[foo_idx].attr = res_pos_idx so we can generate
 * opcodes without going bonkers.
 *
 * This function works on OUTPUT, ATTRIB, and PARAM single bindings. PARAM array bindings are handled in  
 * parse_tree_assign_param_arrays().
 *
 * \param  s   The parse state
 * \param  ptn The root of the parse tree from which to start lining up state and variables
 */
static void
parse_tree_assign_bindings(parse_state * s, parse_tree_node * ptn)
{
   GLint a;
   parse_tree_node *var_name, *attr_item;

   /* OUTPUT, ATTRIB */
   if ((ptn->prod_applied == 288) || (ptn->prod_applied == 120)) {
      var_name = ptn->children[0]->children[0];
      attr_item = ptn->children[1];

      if (attr_item->tok != BINDING_TOKEN) {
	 fprintf(stderr,
		 "sanity check: trying to bind an output variable to something funky!\n");
	 return;
      }

      s->idents.attr[var_name->tok_attr] = attr_item->tok_attr;
      printf("result: %s bound to %d\n", s->idents.data[var_name->tok_attr],
	     s->binds.type[s->idents.attr[var_name->tok_attr]]);
      return;
   }

   /* stateSingleItemDecl */
   if (ptn->prod_applied == 134) {
      var_name = ptn->children[0]->children[0];
      if (ptn->children[1]->prod_applied == 135) {
	 if (ptn->children[1]->children[0]->prod_applied == 139) {
	    if (ptn->children[1]->children[0]->children[0]->prod_applied ==
		144)
	       attr_item =
		  ptn->children[1]->children[0]->children[0]->children[0]->
		  children[0];
	    else if (ptn->children[1]->children[0]->children[0]->
		     prod_applied == 145)
	       attr_item =
		  ptn->children[1]->children[0]->children[0]->children[0];
	    else
	       attr_item =
		  ptn->children[1]->children[0]->children[0]->children[0]->
		  children[0];

	    if (attr_item->tok != BINDING_TOKEN) {
	       fprintf(stderr,
		       "sanity check: trying to bind an param variable (%s) to something funky! [%d]\n",
		       s->idents.data[var_name->tok_attr], attr_item->tok);
	       exit(1);
	    }

	    s->idents.attr[var_name->tok_attr] = attr_item->tok_attr;
	    printf("result: %s bound to %d\n",
		   s->idents.data[var_name->tok_attr],
		   s->binds.type[s->idents.attr[var_name->tok_attr]]);
	    return;
	 }
      }

   }

   /* else, recurse on all our children */
   for (a = 0; a < 4; a++) {
      if (!ptn->children[a])
	 return;

      parse_tree_assign_bindings(s, ptn->children[a]);
   }

}

/**
 * This handles lining up PARAM arrays with variables, much like parse_tree_assign_bindings().
 *
 * In parse_tree_assign_bindings, we set the identifier attr to the index into the binding table of 
 * the bound state.
 *
 * Here, instead, we allocate a slot in the 'array table' to stick the bound state into. Instead
 * of an index into the binding table, the identifier attr now holds the index into the array table.
 *
 * \param s   The parse state
 * \param pnt The root parse tree node to handle arrays from
 *
 */
static void
parse_tree_assign_param_arrays(parse_state * s, parse_tree_node * ptn)
{
   GLint a, is_mult, array_len;
   parse_tree_node *var_name, *binding, *arraysize, *ptmp;

   /* If we're a param */
   if (ptn->prod_applied == 134) {
      /* establish name */
      var_name = ptn->children[0];

      /* param_statement2 */
      binding = ptn->children[1];
      if (binding->prod_applied == 136) {
	 /* optarraysize */
	 arraysize = binding->children[0];

	 is_mult = 0;

	 /* foo[3] */
	 if (arraysize->prod_applied == 138) {
	    debug_token(s, var_name->children[0]->tok,
			var_name->children[0]->tok_attr);
	    debug_token(s, arraysize->children[0]->tok,
			arraysize->children[0]->tok_attr);
	    printf("\n");
	    is_mult = 1;
	 }
	 else
	    /* foo[] */
	 if (arraysize->prod_applied == 137) {
	    arraysize = NULL;
	    printf("How do I init a PARAM array like foo[]?? \n");
	    is_mult = 1;
	 }

	 if (!is_mult)
	    return;

	 s->idents.attr[var_name->tok_attr] = array_table_new(&s->arrays);

	 binding = binding->children[1]->children[0];
	 ptmp = binding->children[0];
	 array_len = 0;

	 if (ptmp->prod_applied == 150) {	/* state */
	    printf("matrix 0 [state]:\n");
	    printf("%d %d\n", ptmp->children[0]->children[0]->tok,
		   ptmp->children[0]->children[0]->tok_attr);
	    array_table_add_data(&s->arrays,
				 s->idents.attr[var_name->tok_attr],
				 ptmp->children[0]->children[0]->tok_attr);
	    array_len +=
	       s->binds.num_rows[ptmp->children[0]->children[0]->tok_attr];
	 }
	 else if (ptmp->prod_applied == 151) {	/* program */
	    printf("matrix 0 [program]:\n");
	    printf("%d %d\n", ptmp->children[0]->tok,
		   ptmp->children[0]->tok_attr);
	    array_table_add_data(&s->arrays,
				 s->idents.attr[var_name->tok_attr],
				 ptmp->children[0]->tok_attr);
	    array_len += s->binds.num_rows[ptmp->children[0]->tok_attr];
	 }
	 else {			/* constant */

	    printf("matrix 0 [constant]:\n");
	    printf("%d %d\n", ptmp->children[0]->children[0]->tok,
		   ptmp->children[0]->children[0]->tok_attr);
	    array_table_add_data(&s->arrays,
				 s->idents.attr[var_name->tok_attr],
				 ptmp->children[0]->children[0]->tok_attr);
	    array_len +=
	       s->binds.num_rows[ptmp->children[0]->children[0]->tok_attr];
	 }
	 binding = binding->children[1];

	 while (binding->prod_applied != 143) {
	    ptmp = binding->children[0]->children[0];
	    printf("mat: %d\n", ptmp->prod_applied);
	    if (ptmp->prod_applied == 150) {	/* state */
	       printf("matrix %d:\n", array_len);
	       printf("%d %d\n", ptmp->children[0]->children[0]->tok,
		      ptmp->children[0]->children[0]->tok_attr);
	       array_table_add_data(&s->arrays,
				    s->idents.attr[var_name->tok_attr],
				    ptmp->children[0]->children[0]->tok_attr);
	       array_len +=
		  s->binds.num_rows[ptmp->children[0]->children[0]->tok_attr];
	    }
	    else if (ptmp->prod_applied == 151) {	/* program */
	       printf("matrix %d [program]:\n", array_len);
	       printf("%d %d\n", ptmp->children[0]->tok,
		      ptmp->children[0]->tok_attr);
	       array_table_add_data(&s->arrays,
				    s->idents.attr[var_name->tok_attr],
				    ptmp->children[0]->tok_attr);
	       array_len += s->binds.num_rows[ptmp->children[0]->tok_attr];
	    }
	    else {		/* constant */

	       printf("matrix %d [constant]:\n", array_len);
	       printf("%d %d\n", ptmp->children[0]->children[0]->tok,
		      ptmp->children[0]->children[0]->tok_attr);
	       array_table_add_data(&s->arrays,
				    s->idents.attr[var_name->tok_attr],
				    ptmp->children[0]->children[0]->tok_attr);
	       array_len +=
		  s->binds.num_rows[ptmp->children[0]->children[0]->tok_attr];
	    }
	    binding = binding->children[0]->children[1];
	 }

	 /* XXX: have to compare the requested size, and the actual 
	  * size, and fix up any inconsistancies
	  */
	 if (arraysize) {
	    printf("matrix wants to get %d rows\n",
		   s->ints.data[arraysize->children[0]->tok_attr]);
	 }
	 printf("matrix num rows: %d\n", array_len);
      }

      return;
   }

   /* Else, recurse on all our children */
   for (a = 0; a < 4; a++) {
      if (!ptn->children[a])
	 return;

      parse_tree_assign_param_arrays(s, ptn->children[a]);
   }

}

/* XXX: This needs to be written properly. */
/**
 * Here we allocate 'registers' for all of the various variables and bound state. 
 *
 * The 'register' number is given by the reg_num field in the binding table. Note that this field
 * is not stored in the identifier table. If it were, we would need a different mechanism for handling 
 * implicit bindings.
 *
 * However, after some discussion with Brian, implicit bindings may be handled by grabbing state
 * directly from Mesa's state structs. This might be a little hairy here, maybe not.. Implicit 
 * bindings are those in the binding table that are not pointed to by any ident.attr or array.data
 *
 * This should also do various error checking, like the multiple vertex attrib error, or 'too many bindings'
 * error.
 * 
 * \param s The parse state
 */
static void
assign_regs(parse_state * s)
{
   GLint a;
   GLfloat foo[4];

   for (a = 0; a < s->idents.len; a++) {
      if (s->idents.type[a] == TYPE_TEMP) {
	 s->idents.attr[a] =
	    binding_table_add(&s->binds, TYPE_TEMP, 0, 0, 0, foo);
      }
   }
}

/**
 * Parse/compile the 'str' returning the compiled 'program'.
 * ctx->Program.ErrorPos will be -1 if successful.  Otherwise, ErrorPos
 * indicates the position of the error in 'str'.
 */
void
_mesa_parse_arb_vertex_program(GLcontext * ctx, GLenum target,
			       const GLubyte * string, GLsizei len,
			       struct vertex_program *program)
{
   GLubyte *our_string;
   parse_state *state;

   printf("len: %d\n", len);

   /* XXX: How do I handle these errors? */
   if (len < 10)
      return;
   if (_mesa_strncmp(string, "!!ARBvp1.0", 10))
      return;

   /* Make a null-terminated copy of the program string */
   our_string = (GLubyte *) MALLOC(len + 1);
   if (!our_string) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glProgramStringARB");
      return;
   }
   MEMCPY(our_string, string, len);
   our_string[len] = 0;

   state = parse_state_init(our_string + 10, strlen(our_string) - 10);

   if (parse(state) == ARB_VP_SUCESS) {
      printf("parse sucess!\n");
   }
   else {
      printf("*** error\n");
      parse_state_cleanup(state);
      return;
   }

   /* First, we 'fold' bindings from a big mess of productions and 
    * tokens into one BINDING_TOKEN, which points to an entry
    * in the binding sym table that holds all of the relevant
    * info for the binding destination.
    */
   parse_tree_fold_bindings(state, state->pt_head);

   /* Now, for each type of binding, walk the parse tree and stick
    * the index into the binding sym table 
    */
   parse_tree_assign_bindings(state, state->pt_head);

   /* XXX: this still needs a' fixin to get folded bindings 
    *                   -- it does? wtf do I mean? */
   parse_tree_assign_param_arrays(state, state->pt_head);

   /* XXX: Now, assign registers. For this, we'll need to create
    * bindings for all temps (and what else?? )
    */
   assign_regs(state);

   /* Ok, now generate code */
   parse_tree_generate_opcodes(state, state->pt_head);

   /* Just for testing.. */
   program->Base.Target = target;
   if (program->Base.String) {
      FREE(program->Base.String);
   }
   program->Base.String = our_string;
   program->Base.Format = GL_PROGRAM_FORMAT_ASCII_ARB;

   if (program->Instructions) {
      FREE(program->Instructions);
   }

   program->Instructions =
      (struct vp_instruction *) _mesa_malloc(sizeof(struct vp_instruction));
   program->Instructions[0].Opcode = VP_OPCODE_END;
   program->InputsRead = 0;
   program->OutputsWritten = 0;
   program->IsPositionInvariant = 0;

   parse_state_cleanup(state);

   /* TODO:
    *    - handle implicit state grabbing & register allocation like discussed
    *      - implicit param declarations -- see above
    *
    *      - variable bindings -- ADDRESS
    *      - deal with explicit array sizes & size mismatches
    *      - shuddup all my debugging crap
    *      - grep for XXX
    *      - multiple vtx attrib binding error
    *      - What do I do on look ahead for prod 54 & 55? (see arbvp_grammar.txt)
    *      - misc errors
    *         - check integer ranges
    *         - check array index ranges
    *         - check variable counts
    *      - param register allocation
    *      - exercise swizzles and masks
    *      - error handling
    *      - generate opcodes
    *          + Get addres registers for relative offsets in PARAM arrays 
    *          + Properly encode implicit PARAMs and ATTRIBs.
    *          + ARL
    *          + SWZ
    *          + actually emit Mesa opcodes
    *      - segfaults while freeing stuff
    *      - OPTION
    */
}
