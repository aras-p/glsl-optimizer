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

#pragma once
#ifndef GLSL_PARSER_EXTRAS_H
#define GLSL_PARSER_EXTRAS_H

#include <cstdlib>
#include "glsl_symbol_table.h"

enum _mesa_glsl_parser_targets {
   vertex_shader,
   geometry_shader,
   fragment_shader,
   ir_shader
};

struct _mesa_glsl_parse_state {
   void *scanner;
   exec_list translation_unit;
   glsl_symbol_table *symbols;

   unsigned language_version;
   enum _mesa_glsl_parser_targets target;

   /**
    * Implementation defined limits that affect built-in variables, etc.
    *
    * \sa struct gl_constants (in mtypes.h)
    */
   struct {
      unsigned MaxDrawBuffers;
      unsigned MaxTextureCoords;
   } Const;

   /**
    * During AST to IR conversion, pointer to current IR function
    *
    * Will be \c NULL whenever the AST to IR conversion is not inside a
    * function definition.
    */
   class ir_function_signature *current_function;

   /** Have we found a return statement in this function? */
   bool found_return;

   /** Was there an error during compilation? */
   bool error;

   /** Index of last generated anonymous temporary. */
   unsigned temp_index;

   /** Loop or switch statement containing the current instructions. */
   class ir_instruction *loop_or_switch_nesting;

   /** List of structures defined in user code. */
   const glsl_type **user_structures;
   unsigned num_user_structures;

   char *info_log;

   /**
    * \name Enable bits for GLSL extensions
    */
   /*@{*/
   unsigned ARB_draw_buffers_enable:1;
   unsigned ARB_draw_buffers_warn:1;
   unsigned ARB_texture_rectangle_enable:1;
   unsigned ARB_texture_rectangle_warn:1;
   unsigned EXT_texture_array_enable:1;
   unsigned EXT_texture_array_warn:1;
   /*@}*/

   /** Extensions supported by the OpenGL implementation. */
   const struct gl_extensions *extensions;
};

typedef struct YYLTYPE {
   int first_line;
   int first_column;
   int last_line;
   int last_column;
   unsigned source;
} YYLTYPE;
# define YYLTYPE_IS_DECLARED 1
# define YYLTYPE_IS_TRIVIAL 1

extern void _mesa_glsl_error(YYLTYPE *locp, _mesa_glsl_parse_state *state,
			     const char *fmt, ...);

/**
 * Emit a warning to the shader log
 *
 * \sa _mesa_glsl_error
 */
extern void _mesa_glsl_warning(const YYLTYPE *locp,
			       _mesa_glsl_parse_state *state,
			       const char *fmt, ...);

extern "C" {
extern int preprocess(void *ctx, const char **shader, char **info_log,
		      const struct gl_extensions *extensions);
}

extern void _mesa_glsl_lexer_ctor(struct _mesa_glsl_parse_state *state,
				  const char *string);

extern void _mesa_glsl_lexer_dtor(struct _mesa_glsl_parse_state *state);

union YYSTYPE;
extern int _mesa_glsl_lex(union YYSTYPE *yylval, YYLTYPE *yylloc, 
			  void *scanner);

extern int _mesa_glsl_parse(struct _mesa_glsl_parse_state *);

/**
 * Process elements of the #extension directive
 *
 * \return
 * If \c name and \c behavior are valid, \c true is returned.  Otherwise
 * \c false is returned.
 */
extern bool _mesa_glsl_process_extension(const char *name, YYLTYPE *name_locp,
					 const char *behavior,
					 YYLTYPE *behavior_locp,
					 _mesa_glsl_parse_state *state);

/**
 * Get the textual name of the specified shader target
 */
extern const char *
_mesa_glsl_shader_target_name(enum _mesa_glsl_parser_targets target);

void do_ir_to_mesa(exec_list *instructions);

#endif /* GLSL_PARSER_EXTRAS_H */
