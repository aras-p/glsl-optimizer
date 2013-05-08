#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "glsl_optimizer.h"

#if _MSC_VER
#define snprintf _snprintf
#endif

static glslopt_ctx* gContext = 0;

static int printhelp(const char* msg)
{
	if (msg) printf("%s\n\n\n", msg);
	printf("Usage: glslopt <-f|-v> {<-es|-noes> <-scalar|-vector>} <input shader> [<output shader>]\n");
	printf("\t-f\t\t: fragment shader\n");
	printf("\t-v\t\t: vertex shader\n");
    printf("\t-es\t\t: use OpenGL ES\n");
    printf("\t-noes\t\t: use regular OpenGL\n");
    printf("\t-scalar\t\t: optimize for scalar architecture\n");
    printf("\t-vector\t\t: optimize for vector architecture\n");
    printf("\t-fancy\t\t: enables 4.0 and 4.1 binding and location features\n");
    printf("\t-repeat N\t: rerun optimizations N times\n");
	printf("\n\tIf no output specified, output is to [input].out.\n");
	return 1;
}

static bool init(bool openglES)
{
	gContext = glslopt_initialize(openglES);
	if( !gContext )
		return false;
	return true;
}

static void term()
{
	glslopt_cleanup(gContext);
}

static char* loadFile(const char* filename)
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

static bool saveFile(const char* filename, const char* data)
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

static bool compileShader(const char* dstfilename, const char* srcfilename, bool vertexShader, bool scalar, bool fancy, int repeat)
{
	const char* originalShader = loadFile(srcfilename);
	if( !originalShader )
		return false;

	const glslopt_shader_type type = vertexShader ? kGlslOptShaderVertex : kGlslOptShaderFragment;
    
    unsigned options = 0;
    if ( scalar )   options |= kGlslOptionScalar;
    if ( fancy )    options |= kGlslOptionFancy;

	glslopt_shader* shader = glslopt_optimize(gContext, type, originalShader, options);
	if( !glslopt_get_status(shader) )
	{
		printf( "Failed to compile %s:\n\n%s\n", srcfilename, glslopt_get_log(shader));
		return false;
	}
    
    printf ( "%s\n", glslopt_get_log(shader));

	const char* optimizedShader = strdup(glslopt_get_output(shader));
    
    for ( int pass=0; repeat>0; pass++,repeat-- )
    {
        glslopt_shader* new_shader = glslopt_optimize(gContext, type, optimizedShader, options | kGlslOptionSkipPreprocessor);
        if( !glslopt_get_status(new_shader) )
        {
            printf( "Failed to compile during repeat pass %i\n\n%s\n\n%s\n",
                   pass+1, optimizedShader, glslopt_get_log(new_shader));
            return false;
        }
        
        free((void *)optimizedShader);
        optimizedShader = strdup(glslopt_get_output(new_shader));
        
        glslopt_shader_delete(new_shader);
    }

	if( !saveFile(dstfilename, optimizedShader) )
		return false;

	delete[] originalShader;
    free((void *)optimizedShader);
	return true;
}

int main(int argc, char* argv[])
{
	if( argc < 3 )
		return printhelp(NULL);

	bool vertexShader = false, freename = false, openglES = false, scalar = false, fancy = false;
	const char* source = 0;
	char* dest = 0;
    int repeat = 0;

	for( int i=1; i < argc; i++ )
	{
		if( argv[i][0] == '-' )
		{
			if( 0 == strcmp("-v", argv[i]) )
				vertexShader = true;
			else if( 0 == strcmp("-f", argv[i]) )
				vertexShader = false;
			else if( 0 == strcmp("-noes", argv[i]) )
				openglES = false;
			else if( 0 == strcmp("-es", argv[i]) )
				openglES = true;
			else if( 0 == strcmp("-scalar", argv[i]) )
				scalar = true;
			else if( 0 == strcmp("-vector", argv[i]) )
				scalar = false;
			else if( 0 == strcmp("-fancy", argv[i]) )
				fancy = true;
            else if( 0 == strcmp("-repeat", argv[i]) )
            {
                if ( i+1 >= argc )
                    return printhelp("-repeat needs a count");
                else
                    repeat = atoi(argv[i+1]), i++;
            }
		}
		else
		{
			if( source == 0 )
				source = argv[i];
			else if( dest == 0 )
				dest = argv[i];
		}
	}

	if( !source )
		return printhelp("Must give a source");

	if( !init(openglES) )
	{
		printf("Failed to initialize glslopt!\n");
		return 1;
	}

	if ( !dest ) {
		dest = (char *) calloc(strlen(source)+5, sizeof(char));
		snprintf(dest, strlen(source)+5, "%s.out", source);
		freename = true;
	}

	int result = 0;
	if( !compileShader(dest, source, vertexShader, scalar, fancy, repeat) )
		result = 1;

	if( freename ) free(dest);

	term();
	return result;
}
