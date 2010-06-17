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
#include <cstdlib>
#include <cstdio>
#include <getopt.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "ir_print_visitor.h"
#include "program.h"


static char *
load_text_file(const char *file_name, size_t *size)
{
	char *text = NULL;
	struct stat st;
	ssize_t total_read = 0;
	int fd = open(file_name, O_RDONLY);

	*size = 0;
	if (fd < 0) {
		return NULL;
	}

	if (fstat(fd, & st) == 0) {
	   text = (char *) malloc(st.st_size + 1);
		if (text != NULL) {
			do {
				ssize_t bytes = read(fd, text + total_read,
						     st.st_size - total_read);
				if (bytes < 0) {
					free(text);
					text = NULL;
					break;
				}

				if (bytes == 0) {
					break;
				}

				total_read += bytes;
			} while (total_read < st.st_size);

			text[total_read] = '\0';
			*size = total_read;
		}
	}

	close(fd);

	return text;
}


void
usage_fail(const char *name)
{
      printf("%s <filename.frag|filename.vert>\n", name);
      exit(EXIT_FAILURE);
}


int dump_ast = 0;
int dump_lir = 0;

const struct option compiler_opts[] = {
   { "dump-ast", 0, &dump_ast, 1 },
   { "dump-lir", 0, &dump_lir, 1 },
   { NULL, 0, NULL, 0 }
};

void
compile_shader(struct glsl_shader *prog)
{
   struct _mesa_glsl_parse_state state;

   memset(& state, 0, sizeof(state));
   switch (prog->Type) {
   case GL_VERTEX_SHADER:   state.target = vertex_shader; break;
   case GL_FRAGMENT_SHADER: state.target = fragment_shader; break;
   case GL_GEOMETRY_SHADER: state.target = geometry_shader; break;
   }

   state.scanner = NULL;
   state.translation_unit.make_empty();
   state.symbols = new glsl_symbol_table;
   state.error = false;
   state.temp_index = 0;
   state.loop_or_switch_nesting = NULL;
   state.ARB_texture_rectangle_enable = true;

   _mesa_glsl_lexer_ctor(& state, prog->Source, prog->SourceLen);
   _mesa_glsl_parse(& state);
   _mesa_glsl_lexer_dtor(& state);

   if (dump_ast) {
      foreach_list_const(n, &state.translation_unit) {
	 ast_node *ast = exec_node_data(ast_node, n, link);
	 ast->print();
      }
      printf("\n\n");
   }

   prog->ir.make_empty();
   if (!state.error && !state.translation_unit.is_empty())
      _mesa_ast_to_hir(&prog->ir, &state);

   /* Optimization passes */
   if (!state.error && !prog->ir.is_empty()) {
      bool progress;
      do {
	 progress = false;

	 progress = do_function_inlining(&prog->ir) || progress;
	 progress = do_if_simplification(&prog->ir) || progress;
	 progress = do_copy_propagation(&prog->ir) || progress;
	 progress = do_dead_code_local(&prog->ir) || progress;
	 progress = do_dead_code_unlinked(&prog->ir) || progress;
	 progress = do_constant_variable_unlinked(&prog->ir) || progress;
	 progress = do_constant_folding(&prog->ir) || progress;
	 progress = do_vec_index_to_swizzle(&prog->ir) || progress;
	 progress = do_swizzle_swizzle(&prog->ir) || progress;
      } while (progress);
   }

   /* Print out the resulting IR */
   if (!state.error && dump_lir) {
      _mesa_print_ir(&prog->ir, &state);
   }

   prog->symbols = state.symbols;
   prog->CompileStatus = !state.error;
   return;
}

int
main(int argc, char **argv)
{
   int status = EXIT_SUCCESS;

   int c;
   int idx = 0;
   while ((c = getopt_long(argc, argv, "", compiler_opts, &idx)) != -1)
      /* empty */ ;


   if (argc <= optind)
      usage_fail(argv[0]);

   struct glsl_program whole_program;
   memset(&whole_program, 0, sizeof(whole_program));

   for (/* empty */; argc > optind; optind++) {
      whole_program.Shaders = (struct glsl_shader **)
	 realloc(whole_program.Shaders,
		 sizeof(struct glsl_shader *) * (whole_program.NumShaders + 1));
      assert(whole_program.Shaders != NULL);

      struct glsl_shader *prog = new glsl_shader;
      memset(prog, 0, sizeof(*prog));

      whole_program.Shaders[whole_program.NumShaders] = prog;
      whole_program.NumShaders++;

      const unsigned len = strlen(argv[optind]);
      if (len < 6)
	 usage_fail(argv[0]);

      const char *const ext = & argv[optind][len - 5];
      if (strncmp(".vert", ext, 5) == 0)
	 prog->Type = GL_VERTEX_SHADER;
      else if (strncmp(".geom", ext, 5) == 0)
	 prog->Type = GL_GEOMETRY_SHADER;
      else if (strncmp(".frag", ext, 5) == 0)
	 prog->Type = GL_FRAGMENT_SHADER;
      else
	 usage_fail(argv[0]);

      prog->Source = load_text_file(argv[optind], &prog->SourceLen);

      compile_shader(prog);

      if (!prog->CompileStatus) {
	 status = EXIT_FAILURE;
	 break;
      }
   }

   return status;
}
