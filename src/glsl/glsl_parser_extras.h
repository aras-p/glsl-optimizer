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

/*
 * Most of the definitions here only apply to C++
 */
#ifdef __cplusplus


#include <cstdlib>
#include "glsl_symbol_table.h"

enum _mesa_glsl_parser_targets {
   vertex_shader,
   geometry_shader,
   fragment_shader
};

struct gl_context;

struct _mesa_glsl_parse_state {
   _mesa_glsl_parse_state(struct gl_context *ctx, GLenum target,
			  void *mem_ctx);

   /* Callers of this talloc-based new need not call delete. It's
    * easier to just talloc_free 'ctx' (or any of its ancestors). */
   static void* operator new(size_t size, void *ctx)
   {
      void *mem = talloc_zero_size(ctx, size);
      assert(mem != NULL);

      return mem;
   }

   /* If the user *does* call delete, that's OK, we will just
    * talloc_free in that case. */
   static void operator delete(void *mem)
   {
      talloc_free(mem);
   }

   void *scanner;
   exec_list translation_unit;
   glsl_symbol_table *symbols;

   bool es_shader;
   unsigned language_version;
   const char *version_string;
   enum _mesa_glsl_parser_targets target;

   /**
    * Implementation defined limits that affect built-in variables, etc.
    *
    * \sa struct gl_constants (in mtypes.h)
    */
   struct {
      /* 1.10 */
      unsigned MaxLights;
      unsigned MaxClipPlanes;
      unsigned MaxTextureUnits;
      unsigned MaxTextureCoords;
      unsigned MaxVertexAttribs;
      unsigned MaxVertexUniformComponents;
      unsigned MaxVaryingFloats;
      unsigned MaxVertexTextureImageUnits;
      unsigned MaxCombinedTextureImageUnits;
      unsigned MaxTextureImageUnits;
      unsigned MaxFragmentUniformComponents;

      /* ARB_draw_buffers */
      unsigned MaxDrawBuffers;
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

   /**
    * Are all shader inputs / outputs invariant?
    *
    * This is set when the 'STDGL invariant(all)' pragma is used.
    */
   bool all_invariant;

   /** Loop or switch statement containing the current instructions. */
   class ir_instruction *loop_or_switch_nesting;
   class ast_iteration_statement *loop_or_switch_nesting_ast;

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
   unsigned ARB_draw_instanced_enable:1;
   unsigned ARB_draw_instanced_warn:1;
   unsigned ARB_explicit_attrib_location_enable:1;
   unsigned ARB_explicit_attrib_location_warn:1;
   unsigned ARB_fragment_coord_conventions_enable:1;
   unsigned ARB_fragment_coord_conventions_warn:1;
   unsigned ARB_texture_rectangle_enable:1;
   unsigned ARB_texture_rectangle_warn:1;
   unsigned EXT_texture_array_enable:1;
   unsigned EXT_texture_array_warn:1;
   unsigned ARB_shader_stencil_export_enable:1;
   unsigned ARB_shader_stencil_export_warn:1;
   /*@}*/

   /** Extensions supported by the OpenGL implementation. */
   const struct gl_extensions *extensions;

   /** Shaders containing built-in functions that are used for linking. */
   struct gl_shader *builtins_to_link[16];
   unsigned num_builtins_to_link;
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

# define YYLLOC_DEFAULT(Current, Rhs, N)			\
do {								\
   if (N)							\
   {								\
      (Current).first_line   = YYRHSLOC(Rhs, 1).first_line;	\
      (Current).first_column = YYRHSLOC(Rhs, 1).first_column;	\
      (Current).last_line    = YYRHSLOC(Rhs, N).last_line;	\
      (Current).last_column  = YYRHSLOC(Rhs, N).last_column;	\
   }								\
   else								\
   {								\
      (Current).first_line   = (Current).last_line =		\
	 YYRHSLOC(Rhs, 0).last_line;				\
      (Current).first_column = (Current).last_column =		\
	 YYRHSLOC(Rhs, 0).last_column;				\
   }								\
   (Current).source = 0;					\
} while (0)

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


#endif /* __cplusplus */


/*
 * These definitions apply to C and C++
 */
#ifdef __cplusplus
extern "C" {
#endif

extern int preprocess(void *ctx, const char **shader, char **info_log,
                      const struct gl_extensions *extensions, int api);

extern void _mesa_destroy_shader_compiler();
extern void _mesa_destroy_shader_compiler_caches();

#ifdef __cplusplus
}
#endif


#endif /* GLSL_PARSER_EXTRAS_H */
