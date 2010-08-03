#include <string>
#include "glsl_optimizer.h"

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


static void usage_fail(const char *name)
{
	printf("%s <filename.frag|filename.vert>\n", name);
	exit(EXIT_FAILURE);
}


int main(int argc, char **argv)
{
   int status = EXIT_SUCCESS;

   if (argc <= 1)
      usage_fail(argv[0]);

   glslopt_ctx* ctx = glslopt_initialize();

   for (int optind = 1; optind < argc; ++optind)
   {
      const unsigned len = strlen(argv[optind]);
      if (len < 6)
		  usage_fail(argv[0]);

	  glslopt_shader_type type;

      const char *const ext = & argv[optind][len - 5];
      if (strncmp(".vert", ext, 5) == 0)
		  type = kGlslOptShaderVertex;
      else if (strncmp(".frag", ext, 5) == 0)
		  type = kGlslOptShaderFragment;
      else
		  usage_fail(argv[0]);

	  std::string shaderSource;
	  if (!ReadStringFromFile (argv[optind], shaderSource))
	  {
		  printf("File \"%s\" does not exist.\n", argv[optind]);
		  exit(EXIT_FAILURE);
	  }

	  glslopt_shader* shader = glslopt_optimize (ctx, type, shaderSource.c_str());
	  if (!glslopt_get_status(shader))
	  {
		  printf ("ERRROR, info log for %s:\n%s\n", argv[optind], glslopt_get_log(shader));
		  status = EXIT_FAILURE;
		  break;
	  }
	  else
	  {
		  printf ("******** Raw       output for %s:\n%s\n", argv[optind], glslopt_get_raw_output(shader));
		  printf ("******** Optimized output for %s:\n%s\n", argv[optind], glslopt_get_output(shader));
	  }

	  glslopt_shader_delete (shader);
   }

   glslopt_cleanup (ctx);

   return status;
}
