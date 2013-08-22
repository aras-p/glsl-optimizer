#include <glsl/glsl_optimizer.h>

#include <fstream>
#include <iostream>
#include <string>


static void usage()
{
	std::cerr << "Usage: glsl_optimizer (--vertex|--fragment) [--opengl-es] SHADER_FILE" << std::endl;
}


int main(int argc, char* argv[])
{
	std::string shaderSource;

	int isOpenGLES = -999;
	int shaderType = -999;

	if (argc < 3)
	{
		usage();
		return 1;
	}

	// Parse options only
	for (int i = 1; i < argc-1; ++i)
	{
		if (shaderType == -999 && strcmp(argv[i], "--vertex") == 0)
			shaderType = (int)kGlslOptShaderVertex;
		else if (shaderType == -999 && strcmp(argv[i], "--fragment") == 0)
			shaderType = (int)kGlslOptShaderFragment;
		else if (isOpenGLES == -999 && strcmp(argv[i], "--opengl-es") == 0)
			isOpenGLES = 1;
		else
		{
			usage();
			return 1;
		}
	}

	// Default to desktop OpenGL
	if (isOpenGLES == -999)
		isOpenGLES = 0;

	const char* shaderFilePath = argv[argc-1];

	std::ifstream shaderFile(shaderFilePath, std::ios::binary);
	if (!shaderFile)
	{
		std::cerr << "Could not open file " << shaderFilePath;
		return 1;
	}
	shaderFile.seekg(0, std::ios::end);
	const std::ifstream::pos_type fileLength = shaderFile.tellg();

	if (fileLength < 5)
	{
		std::cerr << "Shader file empty or too small (" << shaderFilePath << ")";
		return 1;
	}

	shaderFile.seekg(0);
	shaderSource.resize((size_t)fileLength);
	shaderFile.read(&shaderSource[0], (std::streamsize)fileLength);
	if (shaderFile.gcount() != fileLength)
	{
		std::cerr << "Failed to read complete shader file (" << shaderFilePath << ")";
		return 1;
	}

	shaderFile.close();

	glslopt_ctx* ctx = glslopt_initialize(!!isOpenGLES);

	int res = 1;

	glslopt_shader* shader = glslopt_optimize(ctx, (glslopt_shader_type)shaderType, shaderSource.c_str(), 0);
	if (glslopt_get_status(shader))
	{
		const char* newSource = glslopt_get_output(shader);
		std::cout << newSource << std::endl;
		res = 0;
	}
	else
	{
		const char* errorLog = glslopt_get_log(shader);
		std::cerr << "Error during optimization: " << errorLog << std::endl;
	}
	glslopt_shader_delete(shader);

	glslopt_cleanup(ctx);

	return res;
}