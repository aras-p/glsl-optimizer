#include "shader.h"

using namespace v8;
using namespace node;

//----------------------------------------------------------------------

Shader::Shader(Compiler* compiler, int type, const char* source)
{
	if (compiler)
	{
		_binding = glslopt_optimize(compiler->getBinding(), (glslopt_shader_type)type, source, 0);
		_compiled = glslopt_get_status(_binding);
	}
	else
	{
		_binding = 0;
		_compiled = false;
	}
}

//----------------------------------------------------------------------

Shader::~Shader()
{
	release();
}

//----------------------------------------------------------------------

void Shader::release()
{
	if (_binding)
	{
		glslopt_shader_delete(_binding);
		_binding = 0;
		_compiled = false;
	}
}

//----------------------------------------------------------------------

const char* Shader::getOutput() const
{
	return (_compiled) ? glslopt_get_output(_binding) : "";
}

//----------------------------------------------------------------------

const char* Shader::getRawOutput() const
{
	return (_compiled) ? glslopt_get_raw_output(_binding) : "";
}

//----------------------------------------------------------------------

const char* Shader::getLog() const
{
	return (_compiled) ? glslopt_get_log(_binding) : "";
}

//----------------------------------------------------------------------

NAN_METHOD(Shader::New)
{
	/*
	//TODO throwing exceptions with Nan
	// Checking arguments
	if(info.Length() != 3)
	{
		Nan::ThrowTypeError("Need three arguments");
	  info.GetReturnValue().Set(Nan::Undefined());
	}

	// Checking compiler
	if(!info[0]->IsObject())
	{
		Nan::ThrowTypeError("The compiler is not a valid Object");
		info.GetReturnValue().Set(Nan::Undefined());
	}

	// Checking shader type
	if (!info[1]->IsInt32())
	{
		Nan::ThrowTypeError("The shader type is not a valid Integer");
		info.GetReturnValue().Set(Nan::Undefined());
	}

	// Checking the shader source code
	if (!info[2]->IsString())
	{
		Nan::ThrowTypeError("The source code is not a valid String");
		info.GetReturnValue().Set(Nan::Undefined());
	}
	*/

	//TODO switch to Nan ASA exceptions get fixed
	if(info.Length() == 3) {
		if(info[0]->IsObject() && info[1]->IsInt32() && info[2]->IsString())
		{
			Compiler* compiler = ObjectWrap::Unwrap<Compiler>(info[0]->ToObject());
			int type = info[1]->Int32Value();
			String::Utf8Value sourceCode(info[2]->ToString());

			Shader* obj = new Shader(compiler, type, *sourceCode);

			obj->Wrap(info.This());
			info.GetReturnValue().Set(info.This());
		}
		else
		{
			std::cerr << "Error: Invalid arguments for Shader" << std::endl;
			info.GetReturnValue().Set(Nan::Undefined());
			exit(1);
		}
	} else {
		std::cerr << "Error: Invalid number of arguments for Shader" << std::endl;
		info.GetReturnValue().Set(Nan::Undefined());
		exit(1);
	}
}

//----------------------------------------------------------------------

NAN_METHOD(Shader::Dispose)
{
	Shader* obj = ObjectWrap::Unwrap<Shader>(info.This());
	obj->release();

	info.GetReturnValue().Set(Nan::Undefined());
}

//----------------------------------------------------------------------

NAN_METHOD(Shader::Compiled)
{
	Shader* obj = ObjectWrap::Unwrap<Shader>(info.This());

	info.GetReturnValue().Set(Nan::New<Boolean>(obj->isCompiled()));
}

//----------------------------------------------------------------------

NAN_METHOD(Shader::Output)
{
	Shader* obj = ObjectWrap::Unwrap<Shader>(info.This());

	info.GetReturnValue().Set(Nan::New<String>(obj->getOutput()).ToLocalChecked());
}

//----------------------------------------------------------------------

NAN_METHOD(Shader::RawOutput)
{
	Shader* obj = ObjectWrap::Unwrap<Shader>(info.This());

	info.GetReturnValue().Set(Nan::New<String>(obj->getRawOutput()).ToLocalChecked());
}

//----------------------------------------------------------------------

NAN_METHOD(Shader::Log)
{
	Shader* obj = ObjectWrap::Unwrap<Shader>(info.This());

	info.GetReturnValue().Set(Nan::New<String>(obj->getLog()).ToLocalChecked());
}
