#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>
#include <time.h>
#include "glsl_optimizer.h"

#define GL_GLEXT_PROTOTYPES 1

#ifndef GOT_GFX
#define GOT_GFX 0
#endif

#if GOT_GFX

// ---- Windows GL bits
#ifdef _MSC_VER
#define GOT_MORE_THAN_GLSL_120 1
#include <windows.h>
#include <gl/GL.h>
extern "C" {
typedef char GLchar;		/* native character */
typedef unsigned int GLuint;	/* shader object handle */
#define GL_VERTEX_SHADER              0x8B31
#define GL_FRAGMENT_SHADER            0x8B30
#define GL_COMPILE_STATUS             0x8B81
typedef void (WINAPI * PFNGLDELETESHADERPROC) (GLuint shader);
typedef GLuint (WINAPI * PFNGLCREATESHADERPROC) (GLenum type);
typedef void (WINAPI * PFNGLSHADERSOURCEPROC) (GLuint shader, GLsizei count, const GLchar *const*string, const GLint *length);
typedef void (WINAPI * PFNGLCOMPILESHADERPROC) (GLuint shader);
typedef void (WINAPI * PFNGLGETSHADERINFOLOGPROC) (GLuint shader, GLsizei bufSize, GLsizei *length, GLchar *infoLog);
typedef void (WINAPI * PFNGLGETSHADERIVPROC) (GLuint shader, GLenum pname, GLint *params);
static PFNGLDELETESHADERPROC glDeleteShader;
static PFNGLCREATESHADERPROC glCreateShader;
static PFNGLSHADERSOURCEPROC glShaderSource;
static PFNGLCOMPILESHADERPROC glCompileShader;
static PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
static PFNGLGETSHADERIVPROC glGetShaderiv;
}
#endif // #ifdef _MSC_VER


// ---- Apple GL bits
#ifdef __APPLE__

#define GOT_MORE_THAN_GLSL_120 0
#include <OpenGL/OpenGL.h>
#include <OpenGL/gl.h>
#include <OpenGL/CGLTypes.h>
#include <dirent.h>
#include <stdlib.h>
static CGLContextObj s_GLContext;
static CGLContextObj s_GLContext3;
static bool s_GL3Active = false;

#endif // ifdef __APPLE__


#endif // GOT_GFX


static bool InitializeOpenGL ()
{
	bool hasGLSL = false;

#if GOT_GFX

#ifdef _MSC_VER
	// setup minimal required GL
	HWND wnd = CreateWindowA(
		"STATIC",
		"GL",
		WS_OVERLAPPEDWINDOW | WS_CLIPSIBLINGS |	WS_CLIPCHILDREN,
		0, 0, 16, 16,
		NULL, NULL,
		GetModuleHandle(NULL), NULL );
	HDC dc = GetDC( wnd );

	PIXELFORMATDESCRIPTOR pfd = {
		sizeof(PIXELFORMATDESCRIPTOR), 1,
		PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL,
		PFD_TYPE_RGBA, 32,
		0, 0, 0, 0, 0, 0,
		0, 0, 0, 0, 0, 0, 0,
		16, 0,
		0, PFD_MAIN_PLANE, 0, 0, 0, 0
	};

	int fmt = ChoosePixelFormat( dc, &pfd );
	SetPixelFormat( dc, fmt, &pfd );

	HGLRC rc = wglCreateContext( dc );
	wglMakeCurrent( dc, rc );
	
#elif defined(__APPLE__)
	
	CGLPixelFormatAttribute attributes[] = {
		kCGLPFAAccelerated,   // no software rendering
		(CGLPixelFormatAttribute) 0
	};
	CGLPixelFormatAttribute attributes3[] = {
		kCGLPFAAccelerated,   // no software rendering
		kCGLPFAOpenGLProfile, // core profile with the version stated below
		(CGLPixelFormatAttribute) kCGLOGLPVersion_3_2_Core,
		(CGLPixelFormatAttribute) 0
	};
	GLint num;
	CGLPixelFormatObj pix;
	
	// create legacy context
	CGLChoosePixelFormat(attributes, &pix, &num);
	if (pix == NULL)
		return false;
	CGLCreateContext(pix, NULL, &s_GLContext);
	if (s_GLContext == NULL)
		return false;
	CGLDestroyPixelFormat(pix);
	CGLSetCurrentContext(s_GLContext);
	
	// create core 3.2 context
	CGLChoosePixelFormat(attributes3, &pix, &num);
	if (pix == NULL)
		return false;
	CGLCreateContext(pix, NULL, &s_GLContext3);
	if (s_GLContext3 == NULL)
		return false;
	CGLDestroyPixelFormat(pix);

#endif

	// check if we have GLSL
	const char* extensions = (const char*)glGetString(GL_EXTENSIONS);
	hasGLSL = extensions != NULL && strstr(extensions, "GL_ARB_shader_objects") && strstr(extensions, "GL_ARB_vertex_shader") && strstr(extensions, "GL_ARB_fragment_shader");
	
	#if defined(__APPLE__)
	// using core profile; always has GLSL
	hasGLSL = true;
	#endif
	
	
#ifdef _MSC_VER
	if (hasGLSL)
	{
		glDeleteShader = (PFNGLDELETESHADERPROC)wglGetProcAddress("glDeleteShader");
		glCreateShader = (PFNGLCREATESHADERPROC)wglGetProcAddress("glCreateShader");
		glShaderSource = (PFNGLSHADERSOURCEPROC)wglGetProcAddress("glShaderSource");
		glCompileShader = (PFNGLCOMPILESHADERPROC)wglGetProcAddress("glCompileShader");
		glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)wglGetProcAddress("glGetShaderInfoLog");
		glGetShaderiv = (PFNGLGETSHADERIVPROC)wglGetProcAddress("glGetShaderiv");
	}
#endif

#endif
	return hasGLSL;
}

static void CleanupGL()
{
#if GOT_GFX
	#ifdef __APPLE__
	CGLSetCurrentContext(NULL);
	if (s_GLContext)
		CGLDestroyContext(s_GLContext);
	if (s_GLContext3)
		CGLDestroyContext(s_GLContext3);
	#endif // #ifdef __APPLE__
#endif // #if GOT_GFX
}


static void replace_string (std::string& target, const std::string& search, const std::string& replace, size_t startPos)
{
	if (search.empty())
		return;
	
	std::string::size_type p = startPos;
	while ((p = target.find (search, p)) != std::string::npos)
	{
		target.replace (p, search.size (), replace);
		p += replace.size ();
	}
}

static bool CheckGLSL (bool vertex, bool gles, const std::string& testName, const char* prefix, const std::string& source)
{
	#if !GOT_GFX
	return false;
	#else

	#if !GOT_MORE_THAN_GLSL_120
	if (source.find("#version 140") != std::string::npos)
		return true;
	#endif

	const bool need3 =
		(source.find("#version 150") != std::string::npos) ||
		(source.find("#version 300") != std::string::npos);
	
#	ifdef __APPLE__
	// Mac core context does not accept any older shader versions, so need to switch to
	// either legacy context or core one.
	if (need3)
	{
		if (!s_GL3Active)
			CGLSetCurrentContext(s_GLContext3);
		s_GL3Active = true;
	}
	else
	{
		if (s_GL3Active)
			CGLSetCurrentContext(s_GLContext);
		s_GL3Active = false;
	}
#	endif // ifdef __APPLE__
	
	
	std::string src;
	if (gles)
	{
		src += "#define lowp\n";
		src += "#define mediump\n";
		src += "#define highp\n";
		src += "#define texture2DLodEXT texture2DLod\n";
		src += "#define texture2DProjLodEXT texture2DProjLod\n";
		src += "#define texture2DGradEXT texture2DGradARB\n";
		src += "#define textureCubeGradEXT textureCubeGradARB\n";
		src += "#define gl_FragDepthEXT gl_FragDepth\n";
		if (!need3)
		{
			src += "#define gl_LastFragData _glesLastFragData\n";
			src += "varying lowp vec4 _glesLastFragData[4];\n";
		}
		if (!need3)
		{
			src += "float shadow2DEXT (sampler2DShadow s, vec3 p) { return shadow2D(s,p).r; }\n";
			src += "float shadow2DProjEXT (sampler2DShadow s, vec4 p) { return shadow2DProj(s,p).r; }\n";
		}
	}
	src += source;
	if (gles)
	{
		replace_string (src, "GL_EXT_shader_texture_lod", "GL_ARB_shader_texture_lod", 0);
		replace_string (src, "GL_EXT_draw_instanced", "GL_ARB_draw_instanced", 0);
		replace_string (src, "gl_InstanceIDEXT", "gl_InstanceIDARB	", 0);
		replace_string (src, "#extension GL_OES_standard_derivatives : require", "", 0);
		replace_string (src, "#extension GL_EXT_shadow_samplers : require", "", 0);
		replace_string (src, "#extension GL_EXT_frag_depth : require", "", 0);
		replace_string (src, "#extension GL_OES_standard_derivatives : enable", "", 0);
		replace_string (src, "#extension GL_EXT_shadow_samplers : enable", "", 0);
		replace_string (src, "#extension GL_EXT_frag_depth : enable", "", 0);
		replace_string (src, "#extension GL_EXT_draw_buffers : enable", "", 0);
		replace_string (src, "#extension GL_EXT_draw_buffers : require", "", 0);
		replace_string (src, "precision ", "// precision ", 0);
		replace_string (src, "#version 300 es", "", 0);
	}
	
	// Desktop GL has no equivalent external-image sampler extension.
	if (gles && src.find("#extension GL_OES_EGL_image_external_essl3") != std::string::npos) {
		if (!strcmp(prefix, "input"))
			printf("\n  %s: skipping desktop OpenGL validation of external textures\n", testName.c_str());
		return true;
	}

	// can't check FB fetch on PC
	if (src.find("#extension GL_EXT_shader_framebuffer_fetch") != std::string::npos)
		return true;

	if (gles && need3)
	{
		src = "#version 330\n" + src;
	}
	const char* sourcePtr = src.c_str();

	
	GLuint shader = glCreateShader (vertex ? GL_VERTEX_SHADER : GL_FRAGMENT_SHADER);
	glShaderSource (shader, 1, &sourcePtr, NULL);
	glCompileShader (shader);
	GLint status;
	
	glGetShaderiv (shader, GL_COMPILE_STATUS, &status);
	
	bool res = true;
	if (status != GL_TRUE)
	{
		char log[20000];
		log[0] = 0;
		GLsizei logLength;
		glGetShaderInfoLog (shader, sizeof(log), &logLength, log);
		printf ("\n  %s: real glsl compiler error on %s:\n%s\n", testName.c_str(), prefix, log);
		res = false;
	}
	glDeleteShader (shader);
	return res;
#	endif
}

#ifdef __APPLE__

static bool CheckMetal (bool vertex, bool gles, const std::string& testName, const char* prefix, const std::string& source)
{
	FILE* f = fopen ("metalTemp.metal", "wb");
	if (!f)
	{
		printf ("\n  %s: could not write Metal compiler input\n", testName.c_str());
		return false;
	}
	size_t written = fwrite (source.c_str(), 1, source.size(), f);
	int closed = fclose (f);
	if (written != source.size() || closed != 0)
		return false;

	const char* command = (source.find("atomic_thread_fence(") != std::string::npos ||
		source.find("memory_coherence_device") != std::string::npos)
		? "xcrun --sdk iphoneos metal -std=metal3.2 -c metalTemp.metal -o metalTemp.air -Wno-parentheses-equality"
		: source.find("#error Image atomics require Metal 3.1") != std::string::npos
		? "xcrun --sdk iphoneos metal -std=metal3.1 -c metalTemp.metal -o metalTemp.air -Wno-parentheses-equality"
		: "xcrun --sdk iphoneos metal -c metalTemp.metal -o metalTemp.air -Wno-parentheses-equality";
	int res = system(command);
	if (res != 0)
	{
		printf ("\n  %s: Metal compiler failed\n", testName.c_str());
		return false;
	}
	return true;
}

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
	size_t readLength = length ? fread(&output[0], 1, length, file) : 0;
	fclose(file);
	if (readLength != length)
	{
		output.clear();
		return false;
	}

	replace_string(output, "\r\n", "\n", 0);
	return true;
}

namespace fs = std::filesystem;
typedef std::vector<std::string> StringVector;

static bool GetFiles (const std::string& folder, const std::string& endsWith, StringVector& files)
{
	std::error_code error;
	fs::directory_iterator it(folder, error), end;
	while (!error && it != end)
	{
		std::string name = it->path().filename().string();
		if (name.size() >= endsWith.size() && name.compare(name.size() - endsWith.size(), endsWith.size(), endsWith) == 0)
			files.push_back(name);
		it.increment(error);
	}
	if (error)
	{
		printf("Could not read test directory %s: %s\n", folder.c_str(), error.message().c_str());
		return false;
	}
	std::sort(files.begin(), files.end());
	return true;
}

static bool WriteStringToFile (const std::string& path, const std::string& text)
{
	FILE* file = fopen(path.c_str(), "wb");
	if (!file)
		return false;
	size_t written = fwrite(text.data(), 1, text.size(), file);
	int closed = fclose(file);
	return written == text.size() && closed == 0;
}

static void MassageVertexForGLES (std::string& s)
{
	if (s.find ("_glesVertex") != std::string::npos)
		return;
	std::string pre;
	size_t insertPoint = s.find("#version");
	int version = 0;
	char profile[3] = {};
	if (insertPoint != std::string::npos &&
		sscanf(s.c_str() + insertPoint, "#version %d %2s", &version, profile) == 2 &&
		version >= 300 && strcmp(profile, "es") == 0 &&
		s.find('\n', insertPoint) != std::string::npos)
	{
		insertPoint = s.find('\n', insertPoint) + 1;
		pre += "#define gl_Vertex _glesVertex\nin highp vec4 _glesVertex;\n";
		pre += "#define gl_Normal _glesNormal\nin mediump vec3 _glesNormal;\n";
		pre += "#define gl_MultiTexCoord0 _glesMultiTexCoord0\nin highp vec4 _glesMultiTexCoord0;\n";
		pre += "#define gl_MultiTexCoord1 _glesMultiTexCoord1\nin highp vec4 _glesMultiTexCoord1;\n";
		pre += "#define gl_Color _glesColor\nin lowp vec4 _glesColor;\n";
	}
	else
	{
		insertPoint = 0;
		pre += "#define gl_Vertex _glesVertex\nattribute highp vec4 _glesVertex;\n";
		pre += "#define gl_Normal _glesNormal\nattribute mediump vec3 _glesNormal;\n";
		pre += "#define gl_MultiTexCoord0 _glesMultiTexCoord0\nattribute highp vec4 _glesMultiTexCoord0;\n";
		pre += "#define gl_MultiTexCoord1 _glesMultiTexCoord1\nattribute highp vec4 _glesMultiTexCoord1;\n";
		pre += "#define gl_Color _glesColor\nattribute lowp vec4 _glesColor;\n";
	}
	
	s.insert (insertPoint, pre);
}

static void MassageFragmentForGLES (std::string& s)
{
	std::string pre;
	s = pre + s;
}

static const char* kGlslTypeNames[kGlslTypeCount] = {
	"float",
	"int",
	"bool",
	"2d",
	"3d",
	"cube",
	"2dshadow",
	"2darray",
	"other",
	"image2d",
	"image2darray",
	"image3d",
};
static const char* kGlslPrecNames[kGlslPrecCount] = {
	"high",
	"medium",
	"low",
};


static bool TestFile (glslopt_ctx* ctx, glslopt_shader_type type,
	const std::string& testName,
	const std::string& inputPath,
	const std::string& outputPath,
	const std::string& actualPath,
	bool updateExpected,
	bool gles,
	bool doCheckGLSL,
	bool doCheckMetal, bool reparse = false)
{
	const bool vertex = type == kGlslOptShaderVertex;
	if (type == kGlslOptShaderCompute && doCheckGLSL) {
		printf("\n  %s: compute validation is unavailable in the legacy OpenGL test context.\n", testName.c_str());
		return false;
	}
	std::string input;
	if (!ReadStringFromFile (inputPath.c_str(), input))
	{
		printf ("\n  %s: failed to read input file\n", testName.c_str());
		return false;
	}
	if (doCheckGLSL)
	{
		if (!CheckGLSL (vertex, gles, testName, "input", input.c_str()))
			return false;
	}

	if (gles)
	{
		if (vertex)
			MassageVertexForGLES (input);
		else if (type == kGlslOptShaderFragment)
			MassageFragmentForGLES (input);
	}

	bool res = true;

	glslopt_shader* shader = glslopt_optimize (ctx, type, input.c_str(), 0);

	bool optimizeOk = glslopt_get_status(shader);
	if (optimizeOk && (!glslopt_get_raw_output(shader) || !glslopt_get_output(shader))) {
		printf("\n  %s: optimizer returned no source output.\n", testName.c_str());
		glslopt_shader_delete(shader);
		return false;
	}
	if (optimizeOk)
	{
		std::string textHir = glslopt_get_raw_output (shader);
		std::string textOpt = glslopt_get_output (shader);

		if (reparse) {
			for (const std::string* output : {&textHir, &textOpt}) {
				glslopt_shader* parsed = glslopt_optimize(ctx, type, output->c_str(), output == &textHir ? kGlslOptionNotFullShader : 0);
				if (!glslopt_get_status(parsed)) {
					printf("\n  %s: printed %s does not compile: %s\n", testName.c_str(),
						output == &textHir ? "raw" : "optimized", glslopt_get_log(parsed));
					WriteStringToFile(actualPath + (output == &textHir ? ".raw" : ".invalid"), *output);
					res = false;
				}
				glslopt_shader_delete(parsed);
			}
		}
		// append stats
		char buffer[1000];
		int statsAlu, statsTex, statsFlow;
		glslopt_shader_get_stats (shader, &statsAlu, &statsTex, &statsFlow);
		sprintf(buffer, "\n// stats: %i alu %i tex %i flow\n", statsAlu, statsTex, statsFlow);
		textOpt += buffer;
		
		// append inputs
		int localX, localY, localZ;
		glslopt_shader_get_compute_local_size(shader, &localX, &localY, &localZ);
		if (type == kGlslOptShaderCompute) {
			snprintf(buffer, sizeof(buffer), "// workgroup size: %d %d %d\n", localX, localY, localZ);
			textOpt += buffer;
		} else if (localX || localY || localZ) {
			printf("\n  %s: non-compute shader reports a workgroup size.\n", testName.c_str());
			res = false;
		}

		const int inputCount = glslopt_shader_get_input_count (shader);
		if (inputCount > 0)
		{
			sprintf(buffer, "// inputs: %i\n", inputCount);
			textOpt += buffer;
		}
		for (int i = 0; i < inputCount; ++i)
		{
			const char* parName;
			glslopt_basic_type parType;
			glslopt_precision parPrec;
			int parVecSize, parMatSize, parArrSize, location;
			glslopt_shader_get_input_desc(shader, i, &parName, &parType, &parPrec, &parVecSize, &parMatSize, &parArrSize, &location);
			if (location >= 0)
				sprintf(buffer, "//  #%i: %s (%s %s) %ix%i [%i] loc %i\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize, location);
			else
				sprintf(buffer, "//  #%i: %s (%s %s) %ix%i [%i]\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize);
			textOpt += buffer;
		}
		// append uniforms
		const int uniformCount = glslopt_shader_get_uniform_count (shader);
		const int uniformSize = glslopt_shader_get_uniform_total_size (shader);
		if (uniformCount > 0)
		{
			sprintf(buffer, "// uniforms: %i (total size: %i)\n", uniformCount, uniformSize);
			textOpt += buffer;
		}
		for (int i = 0; i < uniformCount; ++i)
		{
			const char* parName;
			glslopt_basic_type parType;
			glslopt_precision parPrec;
			int parVecSize, parMatSize, parArrSize, location;
			glslopt_shader_get_uniform_desc(shader, i, &parName, &parType, &parPrec, &parVecSize, &parMatSize, &parArrSize, &location);
			if (location >= 0)
				sprintf(buffer, "//  #%i: %s (%s %s) %ix%i [%i] loc %i\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize, location);
			else
				sprintf(buffer, "//  #%i: %s (%s %s) %ix%i [%i]\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize);
			textOpt += buffer;
		}
		// Buffer reflection includes inactive layout members and is separate from uniforms.
		const int bufferCount = glslopt_shader_get_buffer_count(shader);
		auto appendField = [&](const char* name, int value) {
			textOpt += std::string(" ") + name + " " + std::to_string(value);
		};
		for (int i = 0; i < bufferCount; ++i) {
			const auto* b = glslopt_shader_get_buffer_desc(shader, i);
			const char* layouts[] = { "std140", "shared", "packed", "std430", "atomic-counter" };
			snprintf(buffer, sizeof(buffer), "// buffer #%d: %s", i, b->name);
			textOpt += buffer;
			if (b->kind == kGlslBufferStorage) textOpt += " storage";
			if (b->kind == kGlslBufferAtomicCounter) textOpt += " atomic-counter";
			if (b->instanceName) textOpt += std::string(" instance ") + b->instanceName;
			textOpt += std::string(" layout ") + layouts[b->layout];
			if (b->binding >= 0) appendField("binding", b->binding);
			if (b->arraySize > 1) appendField("array", b->arraySize);
			textOpt += " (size: " + (b->size >= 0 ? std::to_string(b->size) : "unknown") + ")";
			if (b->metalBufferIndex >= 0) appendField("metal", b->metalBufferIndex);
			if (b->metalSizeBufferIndex >= 0) appendField("metal-size-buffer", b->metalSizeBufferIndex);
			textOpt += "\n";
			for (int j = 0; j < glslopt_shader_get_buffer_member_count(shader, i); ++j) {
				const auto* m = glslopt_shader_get_buffer_member_desc(shader, i, j);
				snprintf(buffer, sizeof(buffer), "//  member #%d: %s %s", j, m->typeName, m->name);
				textOpt += buffer;
				if (m->parentIndex >= 0) appendField("parent", m->parentIndex);
				if (m->offset >= 0) appendField("offset", m->offset);
				if (m->arraySize < 0) textOpt += " array runtime";
				if (m->arraySize > 0) appendField("array", m->arraySize);
				if (m->arrayStride > 0) appendField("stride", m->arrayStride);
				if (m->matrixStride > 0) appendField("matrix-stride", m->matrixStride);
				if (m->rowMajor) textOpt += " row-major";
				if (m->readOnly) textOpt += " readonly";
				if (m->writeOnly) textOpt += " writeonly";
				if (!m->used) textOpt += " (unused)";
				textOpt += "\n";
			}
		}
		// append textures
		const int textureCount = glslopt_shader_get_texture_count (shader);
		if (textureCount > 0)
		{
			sprintf(buffer, "// textures: %i\n", textureCount);
			textOpt += buffer;
		}
		for (int i = 0; i < textureCount; ++i)
		{
			const char* parName;
			glslopt_basic_type parType;
			glslopt_precision parPrec;
			int parVecSize, parMatSize, parArrSize, location;
			glslopt_shader_get_texture_desc(shader, i, &parName, &parType, &parPrec, &parVecSize, &parMatSize, &parArrSize, &location);
			if (location >= 0)
				sprintf(buffer, "//  #%i: %s (%s %s) %ix%i [%i] loc %i\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize, location);
			else
				sprintf(buffer, "//  #%i: %s (%s %s) %ix%i [%i]\n", i, parName, kGlslPrecNames[parPrec], kGlslTypeNames[parType], parVecSize, parMatSize, parArrSize);
			textOpt += buffer;
		}

		std::string outputOpt;
		bool haveExpected = ReadStringFromFile (outputPath.c_str(), outputOpt);

#		ifdef __APPLE__

		if (res && doCheckMetal && !CheckMetal (vertex, gles, testName, "metal", textOpt.c_str()))
			res = false;

#		endif
		
		if (!haveExpected || textOpt != outputOpt)
		{
			if (updateExpected && res)
			{
				if (!WriteStringToFile(outputPath, textOpt))
				{
					printf("\n  %s: could not update %s\n", testName.c_str(), outputPath.c_str());
					res = false;
				}
				else
					printf("\n  Updated %s\n", outputPath.c_str());
			}
			else
			{
				printf("\n  %s: %s (%s)\n", testName.c_str(),
					haveExpected ? "does not match optimized output" : "missing or unreadable expected output", outputPath.c_str());
				if (WriteStringToFile(actualPath, textOpt))
					printf("  Actual output: %s\n", actualPath.c_str());
				else
					printf("  Could not write actual output: %s\n", actualPath.c_str());
				res = false;
			}
		}
		if (res && doCheckGLSL && !CheckGLSL (vertex, gles, testName, "raw", textHir.c_str()))
			res = false;
		if (res && doCheckGLSL && !CheckGLSL (vertex, gles, testName, "optimized", textOpt.c_str()))
			res = false;
	}
	else
	{
		printf ("\n  %s: optimize error: %s\n", testName.c_str(), glslopt_get_log(shader));
		res = false;
	}

	glslopt_shader_delete (shader);

	return res;
}


int main (int argc, const char** argv)
{
	if (argc < 2)
	{
		printf ("USAGE: glsl_optimizer_tests testfolder [--update-expected] [--check-glsl] [--check-metal]\n");
		return 1;
	}

	bool updateExpected = false;
	bool hasOpenGL = false;
	bool hasMetal = false;
	for (int i = 2; i < argc; ++i)
	{
		if (strcmp(argv[i], "--update-expected") == 0)
			updateExpected = true;
		else if (strcmp(argv[i], "--check-glsl") == 0)
			hasOpenGL = true;
		else if (strcmp(argv[i], "--check-metal") == 0)
			hasMetal = true;
		else
		{
			printf("Unknown option: %s\n", argv[i]);
			return 1;
		}
	}
	if (hasOpenGL && !InitializeOpenGL())
	{
		printf("OpenGL validation unavailable; enable GLSLOPT_TEST_OPENGL and provide a working OpenGL context.\n");
		CleanupGL();
		return 1;
	}
	if (hasMetal)
	{
#ifdef __APPLE__
		if (system("xcrun --sdk iphoneos metal --version > /dev/null 2>&1") != 0)
#endif
		{
			printf("Metal validation requires macOS and an Xcode iPhoneOS Metal compiler.\n");
			CleanupGL();
			return 1;
		}
	}

	glslopt_ctx* ctx[2] = {
		glslopt_initialize(kGlslTargetOpenGLES30),
		glslopt_initialize(kGlslTargetOpenGL),
	};
	glslopt_ctx* ctxMetal = glslopt_initialize(kGlslTargetMetal);

	std::string baseFolder = argv[1];

	clock_t time0 = clock();

	// 2.39s
	// ralloc fix 256 initial: 1.35s

	static const char* kTypeName[] = { "vertex", "fragment", "compute" };
	static const glslopt_shader_type kTypes[] = { kGlslOptShaderVertex, kGlslOptShaderFragment, kGlslOptShaderCompute };
	size_t tests = 0;
	size_t errors = 0;
	for (int type = 0; type < 3; ++type)
	{
		std::string testFolder = baseFolder + "/" + kTypeName[type];
		std::string actualFolder = std::string("test-output/") + kTypeName[type];
		std::error_code error;
		fs::create_directories(actualFolder, error);
		if (error)
		{
			printf("Could not create output directory %s: %s\n", actualFolder.c_str(), error.message().c_str());
			++errors;
			continue;
		}
		size_t stageTests = 0;

		static const char* kAPIName[2] = { "OpenGL ES 3.0", "OpenGL" };
		static const char* kApiIn [2] = {"-inES3.txt", "-in.txt"};
		static const char* kApiOut[2] = {"-outES3.txt", "-out.txt"};
		static const char* kApiOutMetal[2] = {"-outES3Metal.txt", "-outMetal.txt"};
		for (int api = 0; api < 2; ++api)
		{
			printf ("\n** running %s tests for %s...\n", kTypeName[type], kAPIName[api]);
			StringVector inputFiles;
			if (!GetFiles(testFolder, kApiIn[api], inputFiles))
			{
				++errors;
				break;
			}

			size_t n = inputFiles.size();
			for (size_t i = 0; i < n; ++i)
			{
				std::string inname = inputFiles[i];
				//if (inname != "ast-in.txt")
				//	continue;
				std::string outname = inname.substr (0,inname.size()-strlen(kApiIn[api])) + kApiOut[api];
				std::string outnameMetal = inname.substr (0,inname.size()-strlen(kApiIn[api])) + kApiOutMetal[api];
				const bool useMetal = api == 0 || fs::is_regular_file(testFolder + "/" + outnameMetal, error);
				bool ok = TestFile (ctx[api], kTypes[type], inname, testFolder + "/" + inname, testFolder + "/" + outname, actualFolder + "/" + outname + ".actual", updateExpected, api==0, hasOpenGL, false, true);
				if (!ok)
				{
					++errors;
				}
				if (useMetal)
				{
					ok = TestFile (ctxMetal, kTypes[type], inname, testFolder + "/" + inname, testFolder + "/" + outnameMetal, actualFolder + "/" + outnameMetal + ".actual", updateExpected, false, false, hasMetal);
					if (!ok)
					{
						++errors;
					}
				}
				++tests;
				++stageTests;
			}
		}
		if (stageTests == 0)
		{
			printf("No shader inputs found in %s\n", testFolder.c_str());
			++errors;
		}
	}
	clock_t time1 = clock();
	float timeDelta = float(time1-time0)/CLOCKS_PER_SEC;

	if (errors != 0)
		printf ("\n**** %i tests (%.2fsec), %i !!!FAILED!!!\n", (int)tests, timeDelta, (int)errors);
	else
		printf ("\n**** %i tests (%.2fsec) succeeded\n", (int)tests, timeDelta);
	
	// 3.25s
	// with builtin call linking, 3.84s

	for (int i = 0; i < 2; ++i)
		glslopt_cleanup (ctx[i]);
	glslopt_cleanup (ctxMetal);
	CleanupGL();

	return errors ? 1 : 0;
}
