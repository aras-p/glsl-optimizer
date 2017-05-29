#ifndef SHADER_H
#define SHADER_H

#include "compiler.h"
#include <node.h>
#include <nan.h>
#include <glsl_optimizer.h>

#include <iostream>

class Shader : public Nan::ObjectWrap {
public:
	static NAN_MODULE_INIT(Init) {
		v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Shader").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

		Nan::SetPrototypeMethod(tpl, "dispose", Dispose);
		Nan::SetPrototypeMethod(tpl, "compiled", Compiled);
		Nan::SetPrototypeMethod(tpl, "output", Output);
		Nan::SetPrototypeMethod(tpl, "rawOutput", RawOutput);
		Nan::SetPrototypeMethod(tpl, "log", Log);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Shader").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
  }

	inline bool isCompiled() const { return _compiled; }
	const char* getOutput() const;
	const char* getRawOutput() const;
	const char* getLog() const;

	void release();

private:
	Shader(Compiler* compiler, int type, const char* source);
	~Shader();

	static NAN_METHOD(New);
	static NAN_METHOD(Dispose);

	static NAN_METHOD(Compiled);
	static NAN_METHOD(Output);
	static NAN_METHOD(RawOutput);
	static NAN_METHOD(Log);

	static inline Nan::Persistent<v8::Function> & constructor() {
		static Nan::Persistent<v8::Function> my_constructor;
		return my_constructor;
 	}

	glslopt_shader* _binding;
	bool _compiled;
};

#endif
