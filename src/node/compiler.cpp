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

NAN_METHOD(Compiler::New)
{
	if(info.IsConstructCall())
	{
		glslopt_target target = kGlslTargetOpenGL;

		if (info[0]->IsInt32())
			target = (glslopt_target)info[0]->Int32Value();

		else if (info[0]->IsBoolean())
			target = (glslopt_target)((int)info[0]->BooleanValue());

		Compiler* obj = new Compiler(target);
    obj->Wrap(info.This());
    info.GetReturnValue().Set(info.This());
  }
	else
	{
		const int argc = 1;
    Local<Value> argv[argc] = {info[0]};
    Local<Function> cons = Nan::New(constructor());
    info.GetReturnValue().Set(Nan::NewInstance(cons, argc, argv).ToLocalChecked());
  }
}

//----------------------------------------------------------------------

NAN_METHOD(Compiler::Dispose)
{
	Compiler* obj = ObjectWrap::Unwrap<Compiler>(info.This());
	obj->release();

	info.GetReturnValue().Set(Nan::Undefined());
}
