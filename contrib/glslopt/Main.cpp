#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "glsl_optimizer.h"

static glslopt_ctx* gContext = 0;

int printhelp(const char* format, ...)
{
	char buffer[4096];

	va_list args;
	va_start(args, format);
	vsnprintf(buffer, sizeof(buffer), format, args);

	printf("%s\n", buffer);
	printf("\n\nUsage: glslopt <-f|-v> <input shader> <output shader>\n");
	printf("\t-f : fragment shader\n");
	printf("\t-v : vertex shader\n");
	return 1;
}

bool init()
{
	gContext = glslopt_initialize();
	if( !gContext )
		return false;
	return true;
}

void term()
{
	glslopt_cleanup(gContext);
}

char* loadFile(const char* filename)
{
	FILE* file = fopen(filename, "rt");
	if( !file )
	{
		printf("Failed to open %s for reading\n", filename);
		return 0;
	}

	fseek(file, 0, SEEK_END);
	const int size = ftell(file);
	fseek(file, 0, SEEK_SET);

	char* result = new char[size+1];
	const int count = (int)fread(result, 1, size, file);
	result[count] = 0;

	fclose(file);
	return result;
}

bool saveFile(const char* filename, const char* data)
{
	int size = (int)strlen(data)+1;
	FILE* file = fopen(filename, "wt");
	if( !file )
	{
		printf( "Failed to open %s for writing\n", filename);
		return false;
	}

	if( 1 != fwrite(data,size,1,file) )
	{
		printf( "Failed to write to %s\n", filename);
		fclose(file);
		return false;
	}

	fclose(file);
	return true;
}

bool compileShader(const char* dstfilename, const char* srcfilename, bool vertexShader)
{
	const char* originalShader = loadFile(srcfilename);
	if( !originalShader )
		return false;

	const glslopt_shader_type type = vertexShader ? kGlslOptShaderVertex : kGlslOptShaderFragment;

	glslopt_shader* shader = glslopt_optimize(gContext, type, originalShader);
	if( !glslopt_get_status(shader) )
	{
		printf( "Failed to compile %s:\n\n%s\n", srcfilename, glslopt_get_log(shader));
		return false;
	}

	const char* optimizedShader = glslopt_get_output(shader);

	if( !saveFile(dstfilename, optimizedShader) )
		return false;

	delete[] originalShader;
	return true;
}

int main(int argc, char* argv[])
{
	if( argc < 4 )
		return printhelp("");

	bool vertexShader = false;
	const char* source = 0;
	const char* dest = 0;

	for( int i=1; i < argc; i++ )
	{
		if( argv[i][0] == '-' )
		{
			if( 0 == stricmp("-v", argv[i]) )
				vertexShader = true;
			if( 0 == stricmp("-f", argv[i]) )
				vertexShader = false;
		}
		else
		{
			if( source == 0 )
				source = argv[i];
			else if( dest == 0 )
				dest = argv[i];
		}
	}

	if( !dest || !source )
		return printhelp("Must give both source and dest");

	if( !init() )
	{
		printf("Failed to initialize glslopt!\n");
		return 1;
	}

	int result = 0;
	if( !compileShader(dest, source, vertexShader) )
		result = 1;

	term();
	return result;
}
