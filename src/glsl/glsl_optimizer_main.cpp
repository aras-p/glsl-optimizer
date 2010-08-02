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

int dump_ast = 0;
int dump_hir = 0;
int dump_lir = 0;

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
   //state->error = preprocess(state, &source, &state->info_log,
	//		     state->extensions);
	state->error = 0;

   if (!state->error) {
      _mesa_glsl_lexer_ctor(state, source);
      _mesa_glsl_parse(state);
      _mesa_glsl_lexer_dtor(state);
   }

   if (dump_ast) {
	   printf ("******** AST:\n");
      foreach_list_const(n, &state->translation_unit) {
	 ast_node *ast = exec_node_data(ast_node, n, link);
	 ast->print();
      }
      printf("\n\n");
   }

   shader->ir = new(statePtr) exec_list;
   if (!state->error && !state->translation_unit.is_empty())
      _mesa_ast_to_hir(shader->ir, state);

   // Print out the unoptimized IR.
   if (!state->error && dump_hir) {
      validate_ir_tree(shader->ir);
	  printf ("******** Unoptimized:\n");
      _mesa_print_ir_glsl(shader->ir, state);
   }

   // Optimization passes
   if (!state->error && !shader->ir->is_empty()) {
      bool progress;
      do {
	 progress = false;

	 progress = do_function_inlining(shader->ir) || progress;
	 progress = do_if_simplification(shader->ir) || progress;
	 progress = do_copy_propagation(shader->ir) || progress;
	 progress = do_dead_code_local(shader->ir) || progress;
	 progress = do_dead_code_unlinked(shader->ir) || progress;
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
   if (!state->error && dump_lir) {
	   printf ("******** Optimized:\n");
      _mesa_print_ir_glsl(shader->ir, state);
   }

   //shader->symbols = state->symbols;
   shader->CompileStatus = !state->error;
   //shader->Version = state->language_version;
   //memcpy(shader->builtins_to_link, state->builtins_to_link,
   //	 sizeof(shader->builtins_to_link[0]) * state->num_builtins_to_link);
   //shader->num_builtins_to_link = state->num_builtins_to_link;

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

   int optind = 1;
   for (int i = 1; i < argc; ++i)
   {
	   if (!strcmp (argv[i], "--dump-ast")) dump_ast = 1, ++optind;
	   else if (!strcmp (argv[i], "--dump-hir")) dump_hir = 1, ++optind;
	   else if (!strcmp (argv[i], "--dump-lir")) dump_lir = 1, ++optind;
   }

   if (argc <= optind)
      usage_fail(argv[0]);

   //struct gl_shader_program *whole_program;

   //whole_program = talloc_zero (NULL, struct gl_shader_program);
   //assert(whole_program != NULL);

   for (/* empty */; argc > optind; optind++)
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

   //talloc_free(whole_program);
   _mesa_glsl_release_types();
   _mesa_glsl_release_functions();

   return status;
}
