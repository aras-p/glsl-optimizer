#include <string>
#include <vector>
#include "glsl_optimizer.h"

#ifdef _MSC_VER
#include <windows.h>
#else
#include <dirent.h>
#endif


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

typedef std::vector<std::string> StringVector;

static StringVector GetFiles (const std::string& folder, const std::string& endsWith)
{
	StringVector res;

	#ifdef _MSC_VER
	WIN32_FIND_DATAA FindFileData;
	HANDLE hFind = FindFirstFileA ((folder+"/*"+endsWith).c_str(), &FindFileData);
	if (hFind == INVALID_HANDLE_VALUE)
		return res;

	do {
		res.push_back (FindFileData.cFileName);
	} while (FindNextFileA (hFind, &FindFileData));

	FindClose (hFind);
	
	#else
	
	DIR *dirp;
	struct dirent *dp;

	if ((dirp = opendir(folder.c_str())) == NULL)
		return res;

	while ( (dp = readdir(dirp)) )
	{
		std::string fname = dp->d_name;
		if (fname == "." || fname == "..")
			continue;
		if (!EndsWith (fname, endsWith))
			continue;
		res.push_back (fname);
	}
	closedir(dirp);
	
	#endif

	return res;
}

static void DeleteFile (const std::string& path)
{
	#ifdef _MSC_VER
	DeleteFileA (path.c_str());
	#else
	unlink (path.c_str());
	#endif
}


static bool TestFile (glslopt_ctx* ctx, bool vertex, const std::string& inputPath, const std::string& hirPath, const std::string& outputPath, bool doCheckGLSL)
{
	std::string input;
	if (!ReadStringFromFile (inputPath.c_str(), input))
	{
		printf ("  failed to read input file\n");
		return false;
	}

	bool res = true;

	glslopt_shader_type type = vertex ? kGlslOptShaderVertex : kGlslOptShaderFragment;
	glslopt_shader* shader = glslopt_optimize (ctx, type, input.c_str());

	bool optimizeOk = glslopt_get_status(shader);
	if (optimizeOk)
	{
		std::string textHir = glslopt_get_raw_output (shader);
		std::string textOpt = glslopt_get_output (shader);
		std::string outputHir;
		ReadStringFromFile (hirPath.c_str(), outputHir);
		std::string outputOpt;
		ReadStringFromFile (outputPath.c_str(), outputOpt);

		if (textHir != outputHir)
		{
			// write output
			FILE* f = fopen (hirPath.c_str(), "wb");
			fwrite (textHir.c_str(), 1, textHir.size(), f);
			fclose (f);
			printf ("  does not match raw output\n");
			res = false;
		}

		if (textOpt != outputOpt)
		{
			// write output
			FILE* f = fopen (outputPath.c_str(), "wb");
			fwrite (textOpt.c_str(), 1, textOpt.size(), f);
			fclose (f);
			printf ("  does not match optimized output\n");
			res = false;
		}
		//if (doCheckGLSL && !CheckGLSL (vertex, text.c_str()))
		//	res = false;
	}
	else
	{
		printf ("  optimize error: %s\n", glslopt_get_log(shader));
		res = false;
	}

	glslopt_shader_delete (shader);

	return res;
}


int main (int argc, const char** argv)
{
	if (argc < 2)
	{
		printf ("USAGE: glsloptimizer testfolder\n");
		return 1;
	}

	//bool hasOpenGL = InitializeOpenGL ();
	bool hasOpenGL = false;
	glslopt_ctx* ctx = glslopt_initialize();

	std::string baseFolder = argv[1];

	static const char* kTypeName[2] = { "vertex", "fragment" };
	size_t tests = 0;
	size_t errors = 0;
	for (int type = 0; type < 2; ++type)
	{
		printf ("testing %s...\n", kTypeName[type]);
		std::string testFolder = baseFolder + "/" + kTypeName[type];
		StringVector inputFiles = GetFiles (testFolder, "-in.txt");

		size_t n = inputFiles.size();
		tests += n;
		for (size_t i = 0; i < n; ++i)
		{
			std::string inname = inputFiles[i];
			printf ("test %s\n", inname.c_str());
			std::string hirname = inname.substr (0,inname.size()-7) + "-ir.txt";
			std::string outname = inname.substr (0,inname.size()-7) + "-out.txt";
			bool ok = TestFile (ctx, type==0, testFolder + "/" + inname, testFolder + "/" + hirname, testFolder + "/" + outname, hasOpenGL);
			if (!ok)
			{
				++errors;
			}
		}
	}
	if (errors != 0)
		printf ("%i tests, %i FAILED\n", tests, errors);
	else
		printf ("%i tests succeeded\n", tests);

	glslopt_cleanup (ctx);

	return errors ? 1 : 0;
}
