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

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "ir_print_visitor.h"


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


int
main(int argc, char **argv)
{
   struct _mesa_glsl_parse_state state;
   char *shader;
   size_t shader_len;
   exec_list instructions;

   if (argc < 3) {
      printf("Usage: %s [v|g|f|i] <shader_file>\n", argv[0]);
      return EXIT_FAILURE;
   }

   memset(& state, 0, sizeof(state));

   switch (argv[1][0]) {
   case 'v':
      state.target = vertex_shader;
      break;
   case 'g':
      state.target = geometry_shader;
      break;
   case 'f':
      state.target = fragment_shader;
      break;
   default:
      printf("Usage: %s [v|g|f] <shader_file>\n", argv[0]);
      return EXIT_FAILURE;
   }

   shader = load_text_file(argv[2], & shader_len);

   state.scanner = NULL;
   state.translation_unit.make_empty();
   state.symbols = new glsl_symbol_table;
   state.error = false;
   state.temp_index = 0;
   state.loop_or_switch_nesting = NULL;
   state.ARB_texture_rectangle_enable = true;

   _mesa_glsl_lexer_ctor(& state, shader, shader_len);
   _mesa_glsl_parse(& state);
   _mesa_glsl_lexer_dtor(& state);

   foreach_list_const(n, &state.translation_unit) {
      ast_node *ast = exec_node_data(ast_node, n, link);
      ast->print();
   }

   if (!state.error && !state.translation_unit.is_empty())
      _mesa_ast_to_hir(&instructions, &state);

   /* Optimization passes */
   if (!state.error && !instructions.is_empty()) {
      bool progress;
      do {
	 progress = false;

	 progress = do_function_inlining(&instructions) || progress;
	 progress = do_if_simplification(&instructions) || progress;
	 progress = do_copy_propagation(&instructions) || progress;
	 progress = do_dead_code_local(&instructions) || progress;
	 progress = do_dead_code_unlinked(&instructions) || progress;
	 progress = do_constant_variable_unlinked(&instructions) || progress;
	 progress = do_constant_folding(&instructions) || progress;
	 progress = do_vec_index_to_swizzle(&instructions) || progress;
	 progress = do_swizzle_swizzle(&instructions) || progress;
      } while (progress);
   }

   /* Print out the resulting IR */
   printf("\n\n");

   if (!state.error) {
      _mesa_print_ir(&instructions, &state);
   }

   delete state.symbols;

   return state.error != 0;
}
