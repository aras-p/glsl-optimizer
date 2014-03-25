#include "compiler.h"

using namespace v8;
using namespace node;

//----------------------------------------------------------------------

Compiler::Compiler(glslopt_target target)
{
	_binding = glslopt_initialize(target);
}

//----------------------------------------------------------------------

Compiler::~Compiler()
{
	release();
}

//----------------------------------------------------------------------

void Compiler::release()
{
	if (_binding)
	{
		glslopt_cleanup(_binding);

		_binding = 0;
	}
}

//----------------------------------------------------------------------

void Compiler::Init(Handle<Object> exports)
{
	// Prepare constructor template
	Local<FunctionTemplate> tpl = FunctionTemplate::New(New);
	tpl->SetClassName(String::NewSymbol("Compiler"));
	tpl->InstanceTemplate()->SetInternalFieldCount(1);

	// Prototype
	SetPrototypeMethod(tpl, "dispose", Dispose);

	// Export the class
	Persistent<Function> constructor = Persistent<Function>::New(tpl->GetFunction());
	exports->Set(String::NewSymbol("Compiler"), constructor);
}

//----------------------------------------------------------------------

Handle<Value> Compiler::New(const Arguments& args)
{
	HandleScope scope;

	glslopt_target target = kGlslTargetOpenGLES20;
	if (!args[0]->IsUndefined())
	{
		glslopt_target target_value = (glslopt_target)args[0]->Uint32Value();
		switch (target_value)
		{
			case kGlslTargetOpenGL:
			case kGlslTargetOpenGLES20:
			case kGlslTargetOpenGLES30:
				target = target_value;
				break;
			default:
				return ThrowException(Exception::TypeError(
					String::New("Invalid optimization target"))
				);
		}
	}

	Compiler* obj = new Compiler(target);

	obj->Wrap(args.This());

	return args.This();
}

//----------------------------------------------------------------------

Handle<Value> Compiler::Dispose(const Arguments& args)
{
	HandleScope scope;

	Compiler* obj = ObjectWrap::Unwrap<Compiler>(args.This());
	obj->release();

	return scope.Close(Undefined());
}
