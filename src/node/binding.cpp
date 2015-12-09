#include <node.h>
#include <node_version.h>
#include <nan.h>
#include "shader.h"
#include <glsl_optimizer.h>

using namespace v8;

void InitAll(Local<Object> exports)
{
	// Export constants on node v4 and v5
#if defined(NODE_MAJOR_VERSION) && (NODE_MAJOR_VERSION > 4)
	Isolate *isolate = exports->GetIsolate();

	Local<Context> ctx = isolate->GetCurrentContext();

	exports->CreateDataProperty(ctx, String::NewFromUtf8(isolate, "VERTEX_SHADER"), Int32::New(isolate, kGlslOptShaderVertex));
	exports->CreateDataProperty(ctx, String::NewFromUtf8(isolate, "FRAGMENT_SHADER"), Int32::New(isolate, kGlslOptShaderFragment));
	exports->CreateDataProperty(ctx, String::NewFromUtf8(isolate, "TARGET_OPENGL"), Int32::New(isolate, kGlslTargetOpenGL));
	exports->CreateDataProperty(ctx, String::NewFromUtf8(isolate, "TARGET_OPENGLES20"), Int32::New(isolate, kGlslTargetOpenGLES20));
	exports->CreateDataProperty(ctx, String::NewFromUtf8(isolate, "TARGET_OPENGLES30"), Int32::New(isolate, kGlslTargetOpenGLES30));

#else
	// Export constants on node v0.12
	Isolate *isolate = Isolate::GetCurrent();

	//TODO need to set these as read only properties
	exports->Set(String::NewFromUtf8(isolate, "VERTEX_SHADER"), Int32::New(isolate, kGlslOptShaderVertex));
	exports->Set(String::NewFromUtf8(isolate, "FRAGMENT_SHADER"), Int32::New(isolate, kGlslOptShaderFragment));
	exports->Set(String::NewFromUtf8(isolate, "TARGET_OPENGL"), Int32::New(isolate, kGlslTargetOpenGL));
	exports->Set(String::NewFromUtf8(isolate, "TARGET_OPENGLES20"), Int32::New(isolate, kGlslTargetOpenGLES20));
	exports->Set(String::NewFromUtf8(isolate, "TARGET_OPENGLES30"), Int32::New(isolate, kGlslTargetOpenGLES30));
#endif

	// Export classes
	Compiler::Init(exports);
	Shader::Init(exports);
}

/*
NAN_MODULE_INIT(InitAll)
{
	Compiler::Init(target);
	Shader::Init(target);
}
*/

NODE_MODULE(glslOptimizer, InitAll);
