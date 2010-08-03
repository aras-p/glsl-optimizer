#include <string>

static bool ReadStringFromFile (const char* pathName, std::string& output)
{
	FILE* file = fopen( pathName, "rb" );
	if (file == NULL)
		return false;
	fseek(file, 0, SEEK_END);
	int length = ftell(file);
	fseek(file, 0, SEEK_SET);
	if (length < 0)
	{
		fclose( file );
		return false;
	}
	output.resize(length);
	int readLength = fread(&*output.begin(), 1, length, file);
	fclose(file);
	if (readLength != length)
	{
		output.clear();
		return false;
	}
	return true;
}


#include "ast.h"
#include "glsl_parser_extras.h"
#include "glsl_parser.h"
#include "ir_optimization.h"
#include "ir_print_glsl_visitor.h"
#include "program.h"

extern "C" struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type);

// Copied from shader_api.c for the stand-alone compiler.
struct gl_shader *
_mesa_new_shader(GLcontext *ctx, GLuint name, GLenum type)
{
   struct gl_shader *shader;
   assert(type == GL_FRAGMENT_SHADER || type == GL_VERTEX_SHADER);
   shader = talloc_zero(NULL, struct gl_shader);
   if (shader) {
      shader->Type = type;
      shader->Name = name;
      shader->RefCount = 1;
   }
   return shader;
}


void
usage_fail(const char *name)
{
      printf("%s <filename.frag|filename.vert>\n", name);
      exit(EXIT_FAILURE);
}

struct Shader {
	Shader () : ir(0), InfoLog(0) { }

	GLenum type;
	std::string source;
	GLboolean CompileStatus;
	struct exec_list *ir;
	GLchar *InfoLog;
};

void compile_shader (Shader* shader)
{
	_mesa_glsl_parse_state *statePtr = talloc_zero (NULL, _mesa_glsl_parse_state);
	struct _mesa_glsl_parse_state *state = new(statePtr) _mesa_glsl_parse_state(NULL, shader->type, statePtr);

	const char* source = shader->source.c_str();
	state->error = 0;

   if (!state->error) {
      _mesa_glsl_lexer_ctor(state, source);
      _mesa_glsl_parse(state);
      _mesa_glsl_lexer_dtor(state);
   }

   shader->ir = new(statePtr) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
      _mesa_ast_to_hir(shader->ir, state);

   // Print out the unoptimized IR.
   if (!state->error) {
      validate_ir_tree(shader->ir);
	  printf ("******** Unoptimized:\n");
	  char* buffer = talloc_strdup(statePtr, "");
      buffer = _mesa_print_ir_glsl(shader->ir, state, buffer);
	  printf (buffer);
   }

   // Optimization passes
   if (!state->error && !shader->ir->is_empty()) {
      bool progress;
      do {
	 progress = false;

	 progress = do_function_inlining(shader->ir) || progress;
	 progress = do_unused_function_removal(shader->ir) || progress;
	 progress = do_if_simplification(shader->ir) || progress;
	 progress = do_copy_propagation(shader->ir) || progress;
	 progress = do_dead_code_local(shader->ir) || progress;
	 progress = do_dead_code_unlinked(shader->ir) || progress;
	 progress = do_tree_grafting(shader->ir) || progress;
	 progress = do_constant_variable_unlinked(shader->ir) || progress;
	 progress = do_constant_folding(shader->ir) || progress;
	 progress = do_algebraic(shader->ir) || progress;
	 progress = do_vec_index_to_swizzle(shader->ir) || progress;
	 progress = do_vec_index_to_cond_assign(shader->ir) || progress;
	 progress = do_swizzle_swizzle(shader->ir) || progress;
      } while (progress);

      validate_ir_tree(shader->ir);
   }


   // Print out the resulting IR
   if (!state->error) {
	   printf ("******** Optimized:\n");
	   char* buffer = talloc_strdup(statePtr, "");
	   buffer = _mesa_print_ir_glsl(shader->ir, state, buffer);
	   printf (buffer);
   }

   shader->CompileStatus = !state->error;

   if (shader->InfoLog)
      talloc_free(shader->InfoLog);

   shader->InfoLog = state->info_log;

   // Retain any live IR, but trash the rest.
   reparent_ir(shader->ir, statePtr);

   talloc_free(state);

   return;
}

int main(int argc, char **argv)
{
   int status = EXIT_SUCCESS;

   if (argc <= 1)
      usage_fail(argv[0]);

   for (int optind = 1; optind < argc; ++optind)
   {
	  Shader shader;

      const unsigned len = strlen(argv[optind]);
      if (len < 6)
		  usage_fail(argv[0]);

      const char *const ext = & argv[optind][len - 5];
      if (strncmp(".vert", ext, 5) == 0)
		shader.type = GL_VERTEX_SHADER;
      else if (strncmp(".geom", ext, 5) == 0)
		shader.type = GL_GEOMETRY_SHADER;
      else if (strncmp(".frag", ext, 5) == 0)
		shader.type = GL_FRAGMENT_SHADER;
      else
		usage_fail(argv[0]);

	  if (!ReadStringFromFile (argv[optind], shader.source))
	  {
		  printf("File \"%s\" does not exist.\n", argv[optind]);
		  exit(EXIT_FAILURE);
	  }

      compile_shader(&shader);

      if (!shader.CompileStatus) {
	 printf("Info log for %s:\n%s\n", argv[optind], shader.InfoLog);
	 status = EXIT_FAILURE;
	 break;
      }
   }

   _mesa_glsl_release_types();
   _mesa_glsl_release_functions();

   return status;
}
