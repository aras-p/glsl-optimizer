#include <node.h>
#include <node_version.h>
#include <nan.h>
#include "shader.h"
#include <glsl_optimizer.h>

using namespace v8;

void InitAll(Local<Object> exports)
{
	Nan::Set(exports, Nan::New("VERTEX_SHADER").ToLocalChecked(), Nan::New(kGlslOptShaderVertex));
	Nan::Set(exports, Nan::New("FRAGMENT_SHADER").ToLocalChecked(), Nan::New(kGlslOptShaderFragment));
	Nan::Set(exports, Nan::New("COMPUTE_SHADER").ToLocalChecked(), Nan::New(kGlslOptShaderCompute));
	Nan::Set(exports, Nan::New("TARGET_OPENGL").ToLocalChecked(), Nan::New(kGlslTargetOpenGL));
	Nan::Set(exports, Nan::New("TARGET_OPENGLES30").ToLocalChecked(), Nan::New(kGlslTargetOpenGLES30));

	// Export classes
	Compiler::Init(exports);
	Shader::Init(exports);
}

NODE_MODULE(glslOptimizer, InitAll);
