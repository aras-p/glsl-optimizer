#ifndef COMPILER_H
#define COMPILER_H

#include <node.h>
#include <nan.h>
#include <glsl_optimizer.h>

class Compiler : public Nan::ObjectWrap {
public:
	static NAN_MODULE_INIT(Init) {
  	v8::Local<v8::FunctionTemplate> tpl = Nan::New<v8::FunctionTemplate>(New);
    tpl->SetClassName(Nan::New("Compiler").ToLocalChecked());
    tpl->InstanceTemplate()->SetInternalFieldCount(1);

    Nan::SetPrototypeMethod(tpl, "dispose", Dispose);

    constructor().Reset(Nan::GetFunction(tpl).ToLocalChecked());
    Nan::Set(target, Nan::New("Compiler").ToLocalChecked(), Nan::GetFunction(tpl).ToLocalChecked());
  }

	inline glslopt_ctx* getBinding() const { return _binding; }

	void release();

private:
	Compiler(glslopt_target target);
	~Compiler();

	static NAN_METHOD(New);
	static NAN_METHOD(Dispose);

	static inline Nan::Persistent<v8::Function> & constructor() {
		static Nan::Persistent<v8::Function> my_constructor;
		return my_constructor;
 	}

	glslopt_ctx* _binding;
};

#endif
