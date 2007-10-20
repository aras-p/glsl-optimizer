#include "instructions.h"

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

using namespace llvm;


Function* makeLitFunction(Module *mod);

Instructions::Instructions(llvm::Module *mod, llvm::Function *func, llvm::BasicBlock *block)
   :  m_mod(mod), m_func(func), m_block(block), m_idx(0)
{
   m_floatVecType = VectorType::get(Type::FloatTy, 4);

   m_llvmFSqrt = 0;
   m_llvmFAbs  = 0;
   m_llvmPow   = 0;
   m_llvmFloor = 0;
   m_llvmFlog  = 0;
   m_llvmLit  = 0;
   m_fmtPtr = 0;
}

llvm::Value * Instructions::add(llvm::Value *in1, llvm::Value *in2)
{
   BinaryOperator *res = BinaryOperator::create(Instruction::Add, in1, in2,
                                                name("add"),
                                                m_block);
   return res;
}

llvm::Value * Instructions::madd(llvm::Value *in1, llvm::Value *in2,
                                 llvm::Value *in3)
{
   Value *mulRes = mul(in1, in2);
   return add(mulRes, in3);
}

llvm::Value * Instructions::mul(llvm::Value *in1, llvm::Value *in2)
{
   BinaryOperator *res = BinaryOperator::create(Instruction::Mul, in1, in2,
                                                name("mul"),
                                                m_block);
   return res;
}

const char * Instructions::name(const char *prefix)
{
   ++m_idx;
   snprintf(m_name, 32, "%s%d", prefix, m_idx);
   return m_name;
}

llvm::Value * Instructions::dp3(llvm::Value *in1, llvm::Value *in2)
{
   Value *mulRes = mul(in1, in2);
   ExtractElementInst *x = new ExtractElementInst(mulRes, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   ExtractElementInst *y = new ExtractElementInst(mulRes, unsigned(1),
                                                  name("extracty"),
                                                  m_block);
   ExtractElementInst *z = new ExtractElementInst(mulRes, unsigned(2),
                                                  name("extractz"),
                                                  m_block);
   BinaryOperator *xy = BinaryOperator::create(Instruction::Add, x, y,
                                                name("xy"),
                                                m_block);
   BinaryOperator *dot3 = BinaryOperator::create(Instruction::Add, xy, z,
                                                name("dot3"),
                                                m_block);
   return vectorFromVals(dot3, dot3, dot3, dot3);
}

llvm::Value *Instructions::callFSqrt(llvm::Value *val)
{
   if (!m_llvmFSqrt) {
      // predeclare the intrinsic
      std::vector<const Type*> fsqrtArgs;
      fsqrtArgs.push_back(Type::FloatTy);
      ParamAttrsList *fsqrtPal = 0;
      FunctionType* fsqrtType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/fsqrtArgs,
         /*isVarArg=*/false,
         /*ParamAttrs=*/fsqrtPal);
      m_llvmFSqrt = new Function(
         /*Type=*/fsqrtType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"llvm.sqrt.f32", m_mod);
      m_llvmFSqrt->setCallingConv(CallingConv::C);
   }
   CallInst *call = new CallInst(m_llvmFSqrt, val,
                                    name("sqrt"),
                                    m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::rsq(llvm::Value *in1)
{
   ExtractElementInst *x = new ExtractElementInst(in1, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   Value *abs  = callFAbs(x);
   Value *sqrt = callFSqrt(abs);

   BinaryOperator *rsqrt = BinaryOperator::create(Instruction::FDiv,
                                                  ConstantFP::get(Type::FloatTy,
                                                                  APFloat(1.f)),
                                                  sqrt,
                                                  name("rsqrt"),
                                                  m_block);
   return vectorFromVals(rsqrt, rsqrt, rsqrt, rsqrt);
}

llvm::Value * Instructions::vectorFromVals(llvm::Value *x, llvm::Value *y,
                                           llvm::Value *z, llvm::Value *w)
{
   Constant *const_vec = Constant::getNullValue(m_floatVecType);
   InsertElementInst *res = new InsertElementInst(const_vec, x, unsigned(0),
                                                  name("vecx"), m_block);
   res = new InsertElementInst(res, y, unsigned(1),
                               name("vecxy"),
                               m_block);
   res = new InsertElementInst(res, z, unsigned(2),
                               name("vecxyz"),
                               m_block);
   if (w)
      res = new InsertElementInst(res, w, unsigned(3),
                                  name("vecxyzw"),
                                  m_block);
   return res;
}

llvm::Value *Instructions::callFAbs(llvm::Value *val)
{
   if (!m_llvmFAbs) {
      // predeclare the intrinsic
      std::vector<const Type*> fabsArgs;
      fabsArgs.push_back(Type::FloatTy);
      ParamAttrsList *fabsPal = 0;
      FunctionType* fabsType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/fabsArgs,
         /*isVarArg=*/false,
         /*ParamAttrs=*/fabsPal);
      m_llvmFAbs = new Function(
         /*Type=*/fabsType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"fabs", m_mod);
      m_llvmFAbs->setCallingConv(CallingConv::C);
   }
   CallInst *call = new CallInst(m_llvmFAbs, val,
                                 name("fabs"),
                                 m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::lit(llvm::Value *in)
{
#if 0
   printVector(in);
   return in;

   ExtractElementInst *x = new ExtractElementInst(in, unsigned(0),
                                                  name("x"),
                                                  m_block);
   
   ExtractElementInst *y = new ExtractElementInst(in, unsigned(1),
                                                  name("y"),
                                                  m_block);
   
   ExtractElementInst *w = new ExtractElementInst(in, unsigned(3),
                                                  name("w"),
                                                  m_block);
   return vectorFromVals(ConstantFP::get(Type::FloatTy, APFloat(1.f)),
                         ConstantFP::get(Type::FloatTy, APFloat(0.f)),
                         ConstantFP::get(Type::FloatTy, APFloat(0.f)),
                         ConstantFP::get(Type::FloatTy, APFloat(1.f)));
#else
   if (!m_llvmLit) {
      m_llvmLit = makeLitFunction(m_mod);
   }
   CallInst *call = new CallInst(m_llvmLit, in, name("litres"), m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
#endif
}

llvm::Value * Instructions::sub(llvm::Value *in1, llvm::Value *in2)
{
   BinaryOperator *res = BinaryOperator::create(Instruction::Sub, in1, in2,
                                                name("sub"),
                                                m_block);
   return res;
}

llvm::Value * Instructions::callPow(llvm::Value *val1, llvm::Value *val2)
{
   if (!m_llvmPow) {
      // predeclare the intrinsic
      std::vector<const Type*> powArgs;
      powArgs.push_back(Type::FloatTy);
      powArgs.push_back(Type::FloatTy);
      ParamAttrsList *powPal = 0;
      FunctionType* powType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/powArgs,
         /*isVarArg=*/false,
         /*ParamAttrs=*/powPal);
      m_llvmPow = new Function(
         /*Type=*/powType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"llvm.pow.f32", m_mod);
      m_llvmPow->setCallingConv(CallingConv::C);
   }
   std::vector<Value*> params;
   params.push_back(val1);
   params.push_back(val2);
   CallInst *call = new CallInst(m_llvmPow, params.begin(), params.end(),
                                 name("pow"),
                                 m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::pow(llvm::Value *in1, llvm::Value *in2)
{
   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0),
                                                   name("x1"),
                                                   m_block);
   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0),
                                                   name("x2"),
                                                   m_block);
   llvm::Value *val = callPow(x1, x2);
   return vectorFromVals(val, val, val, val);
}

llvm::Value * Instructions::rcp(llvm::Value *in1)
{
   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0),
                                                   name("x1"),
                                                   m_block);
   BinaryOperator *res = BinaryOperator::create(Instruction::FDiv,
                                                ConstantFP::get(Type::FloatTy,
                                                                APFloat(1.f)),
                                                x1,
                                                name("rcp"),
                                                m_block);
   return vectorFromVals(res, res, res, res);
}

llvm::Value * Instructions::dp4(llvm::Value *in1, llvm::Value *in2)
{
   Value *mulRes = mul(in1, in2);
   ExtractElementInst *x = new ExtractElementInst(mulRes, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   ExtractElementInst *y = new ExtractElementInst(mulRes, unsigned(1),
                                                  name("extracty"),
                                                  m_block);
   ExtractElementInst *z = new ExtractElementInst(mulRes, unsigned(2),
                                                  name("extractz"),
                                                  m_block);
   ExtractElementInst *w = new ExtractElementInst(mulRes, unsigned(3),
                                                  name("extractw"),
                                                  m_block);
   BinaryOperator *xy = BinaryOperator::create(Instruction::Add, x, y,
                                                name("xy"),
                                                m_block);
   BinaryOperator *xyz = BinaryOperator::create(Instruction::Add, xy, z,
                                                name("xyz"),
                                                m_block);
   BinaryOperator *dot4 = BinaryOperator::create(Instruction::Add, xyz, w,
                                                 name("dot4"),
                                                 m_block);
   return vectorFromVals(dot4, dot4, dot4, dot4);
}

llvm::Value * Instructions::dph(llvm::Value *in1, llvm::Value *in2)
{
   Value *mulRes = mul(in1, in2);
   ExtractElementInst *x = new ExtractElementInst(mulRes, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   ExtractElementInst *y = new ExtractElementInst(mulRes, unsigned(1),
                                                  name("extracty"),
                                                  m_block);
   ExtractElementInst *z = new ExtractElementInst(mulRes, unsigned(2),
                                                  name("extractz"),
                                                  m_block);
   ExtractElementInst *w = new ExtractElementInst(in2, unsigned(3),
                                                  name("val2w"),
                                                  m_block);
   BinaryOperator *xy = BinaryOperator::create(Instruction::Add, x, y,
                                                name("xy"),
                                                m_block);
   BinaryOperator *xyz = BinaryOperator::create(Instruction::Add, xy, z,
                                                name("xyz"),
                                                m_block);
   BinaryOperator *dph = BinaryOperator::create(Instruction::Add, xyz, w,
                                                name("dph"),
                                                m_block);
   return vectorFromVals(dph, dph, dph, dph);
}

llvm::Value * Instructions::dst(llvm::Value *in1, llvm::Value *in2)
{
   ExtractElementInst *y1 = new ExtractElementInst(in1, unsigned(1),
                                                   name("y1"),
                                                   m_block);
   ExtractElementInst *z = new ExtractElementInst(in1, unsigned(2),
                                                  name("z"),
                                                  m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1),
                                                   name("y2"),
                                                   m_block);
   ExtractElementInst *w = new ExtractElementInst(in2, unsigned(3),
                                                  name("w"),
                                                  m_block);
   BinaryOperator *ry = BinaryOperator::create(Instruction::Mul,
                                               y1, y2,
                                               name("tyuy"),
                                               m_block);
   return vectorFromVals(ConstantFP::get(Type::FloatTy, APFloat(1.f)),
                         ry, z, w);
}

llvm::Value * Instructions::ex2(llvm::Value *in)
{
   llvm::Value *val = callPow(ConstantFP::get(Type::FloatTy, APFloat(2.f)),
                              new ExtractElementInst(in, unsigned(0),
                                                     name("x1"),
                                                     m_block));
   return vectorFromVals(val, val, val, val);
}

llvm::Value * Instructions::callFloor(llvm::Value *val)
{
   if (!m_llvmFloor) {
      // predeclare the intrinsic
      std::vector<const Type*> floorArgs;
      floorArgs.push_back(Type::FloatTy);
      ParamAttrsList *floorPal = 0;
      FunctionType* floorType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/floorArgs,
         /*isVarArg=*/false,
         /*ParamAttrs=*/floorPal);
      m_llvmFloor = new Function(
         /*Type=*/floorType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"floorf", m_mod);
      m_llvmFloor->setCallingConv(CallingConv::C);
   }
   CallInst *call = new CallInst(m_llvmFloor, val,
                                 name("floorf"),
                                 m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::floor(llvm::Value *in)
{
   ExtractElementInst *x = new ExtractElementInst(in, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   ExtractElementInst *y = new ExtractElementInst(in, unsigned(1),
                                                  name("extracty"),
                                                  m_block);
   ExtractElementInst *z = new ExtractElementInst(in, unsigned(2),
                                                  name("extractz"),
                                                  m_block);
   ExtractElementInst *w = new ExtractElementInst(in, unsigned(3),
                                                  name("extractw"),
                                                  m_block);
   return vectorFromVals(callFloor(x), callFloor(y),
                         callFloor(z), callFloor(w));
}

llvm::Value * Instructions::frc(llvm::Value *in)
{
   llvm::Value *flr = floor(in);
   return sub(in, flr);
}

llvm::Value * Instructions::callFLog(llvm::Value *val)
{
   if (!m_llvmFlog) {
      // predeclare the intrinsic
      std::vector<const Type*> flogArgs;
      flogArgs.push_back(Type::FloatTy);
      ParamAttrsList *flogPal = 0;
      FunctionType* flogType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/flogArgs,
         /*isVarArg=*/false,
         /*ParamAttrs=*/flogPal);
      m_llvmFlog = new Function(
         /*Type=*/flogType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"logf", m_mod);
      m_llvmFlog->setCallingConv(CallingConv::C);
   }
   CallInst *call = new CallInst(m_llvmFlog, val,
                                 name("logf"),
                                 m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::lg2(llvm::Value *in)
{
   ExtractElementInst *x = new ExtractElementInst(in, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   ExtractElementInst *y = new ExtractElementInst(in, unsigned(1),
                                                  name("extracty"),
                                                  m_block);
   ExtractElementInst *z = new ExtractElementInst(in, unsigned(2),
                                                  name("extractz"),
                                                  m_block);
   ExtractElementInst *w = new ExtractElementInst(in, unsigned(3),
                                                  name("extractw"),
                                                  m_block);
   llvm::Value *const_vec = vectorFromVals(
      ConstantFP::get(Type::FloatTy, APFloat(1.442695f)),
      ConstantFP::get(Type::FloatTy, APFloat(1.442695f)),
      ConstantFP::get(Type::FloatTy, APFloat(1.442695f)),
      ConstantFP::get(Type::FloatTy, APFloat(1.442695f))
      );
   return mul(vectorFromVals(callFLog(x), callFLog(y),
                             callFLog(z), callFLog(w)), const_vec);
}

llvm::Value * Instructions::min(llvm::Value *in1, llvm::Value *in2)
{
   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0),
                                                  name("x1"),
                                                  m_block);
   ExtractElementInst *y1 = new ExtractElementInst(in1, unsigned(1),
                                                  name("y1"),
                                                  m_block);
   ExtractElementInst *z1 = new ExtractElementInst(in1, unsigned(2),
                                                  name("z1"),
                                                  m_block);
   ExtractElementInst *w1 = new ExtractElementInst(in1, unsigned(3),
                                                  name("w1"),
                                                  m_block);

   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0),
                                                   name("x2"),
                                                   m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1),
                                                   name("y2"),
                                                   m_block);
   ExtractElementInst *z2 = new ExtractElementInst(in2, unsigned(2),
                                                   name("z2"),
                                                   m_block);
   ExtractElementInst *w2 = new ExtractElementInst(in2, unsigned(3),
                                                   name("w2"),
                                                   m_block);

   FCmpInst *xcmp  = new FCmpInst(FCmpInst::FCMP_OLT, x1, x2,
                                  name("xcmp"), m_block);
   SelectInst *selx = new SelectInst(xcmp, x1, x2,
                                     name("selx"), m_block);

   FCmpInst *ycmp  = new FCmpInst(FCmpInst::FCMP_OLT, y1, y2,
                                  name("ycmp"), m_block);
   SelectInst *sely = new SelectInst(ycmp, y1, y2,
                                     name("sely"), m_block);

   FCmpInst *zcmp  = new FCmpInst(FCmpInst::FCMP_OLT, z1, z2,
                                  name("zcmp"), m_block);
   SelectInst *selz = new SelectInst(zcmp, z1, z2,
                                     name("selz"), m_block);

   FCmpInst *wcmp  = new FCmpInst(FCmpInst::FCMP_OLT, w1, w2,
                                  name("wcmp"), m_block);
   SelectInst *selw = new SelectInst(wcmp, w1, w2,
                                     name("selw"), m_block);

   return vectorFromVals(selx, sely, selz, selw);
}

llvm::Value * Instructions::max(llvm::Value *in1, llvm::Value *in2)
{
   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0),
                                                  name("x1"),
                                                  m_block);
   ExtractElementInst *y1 = new ExtractElementInst(in1, unsigned(1),
                                                  name("y1"),
                                                  m_block);
   ExtractElementInst *z1 = new ExtractElementInst(in1, unsigned(2),
                                                  name("z1"),
                                                  m_block);
   ExtractElementInst *w1 = new ExtractElementInst(in1, unsigned(3),
                                                  name("w1"),
                                                  m_block);

   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0),
                                                   name("x2"),
                                                   m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1),
                                                   name("y2"),
                                                   m_block);
   ExtractElementInst *z2 = new ExtractElementInst(in2, unsigned(2),
                                                   name("z2"),
                                                   m_block);
   ExtractElementInst *w2 = new ExtractElementInst(in2, unsigned(3),
                                                   name("w2"),
                                                   m_block);

   FCmpInst *xcmp  = new FCmpInst(FCmpInst::FCMP_OGT, x1, x2,
                                  name("xcmp"), m_block);
   SelectInst *selx = new SelectInst(xcmp, x1, x2,
                                     name("selx"), m_block);

   FCmpInst *ycmp  = new FCmpInst(FCmpInst::FCMP_OGT, y1, y2,
                                  name("ycmp"), m_block);
   SelectInst *sely = new SelectInst(ycmp, y1, y2,
                                     name("sely"), m_block);

   FCmpInst *zcmp  = new FCmpInst(FCmpInst::FCMP_OGT, z1, z2,
                                  name("zcmp"), m_block);
   SelectInst *selz = new SelectInst(zcmp, z1, z2,
                                     name("selz"), m_block);

   FCmpInst *wcmp  = new FCmpInst(FCmpInst::FCMP_OGT, w1, w2,
                                  name("wcmp"), m_block);
   SelectInst *selw = new SelectInst(wcmp, w1, w2,
                                     name("selw"), m_block);

   return vectorFromVals(selx, sely, selz, selw);
}


/*
  Generated from:
extern float exp2f(float x);
extern float log2f(float x);

float4 lit(float4 tmp)
{
    tmp.w = (tmp.w < -128.0) ? -128.0f : ((tmp.w > 128.0f) ? 128.f : tmp.w);
    float4 result;
    result.x = 1.0;
    result.y = tmp.x;
    result.z = (tmp.x > 0) ? exp2f(tmp.w * log2f(tmp.y)) : 0.0;
    result.w = 1.0;
    return result;
}
with:
clang --emit-llvm lit.c |llvm-as|opt -std-compile-opts|llvm2cpp -gen-function -for=lit

*/
Function* makeLitFunction(Module *mod) {

// Type Definitions
ArrayType* ArrayTy_0 = ArrayType::get(IntegerType::get(8), 27);

PointerType* PointerTy_1 = PointerType::get(ArrayTy_0);

ArrayType* ArrayTy_2 = ArrayType::get(IntegerType::get(8), 28);

PointerType* PointerTy_3 = PointerType::get(ArrayTy_2);

ArrayType* ArrayTy_4 = ArrayType::get(IntegerType::get(8), 8);

PointerType* PointerTy_5 = PointerType::get(ArrayTy_4);

ArrayType* ArrayTy_6 = ArrayType::get(IntegerType::get(8), 33);

PointerType* PointerTy_7 = PointerType::get(ArrayTy_6);

std::vector<const Type*>FuncTy_8_args;
FuncTy_8_args.push_back(Type::FloatTy);
FuncTy_8_args.push_back(Type::FloatTy);
ParamAttrsList *FuncTy_8_PAL = 0;
FunctionType* FuncTy_8 = FunctionType::get(
  /*Result=*/Type::FloatTy,
  /*Params=*/FuncTy_8_args,
  /*isVarArg=*/false,
  /*ParamAttrs=*/FuncTy_8_PAL);

std::vector<const Type*>FuncTy_10_args;
ParamAttrsList *FuncTy_10_PAL = 0;
FunctionType* FuncTy_10 = FunctionType::get(
  /*Result=*/IntegerType::get(32),
  /*Params=*/FuncTy_10_args,
  /*isVarArg=*/true,
  /*ParamAttrs=*/FuncTy_10_PAL);

PointerType* PointerTy_9 = PointerType::get(FuncTy_10);

PointerType* PointerTy_11 = PointerType::get(IntegerType::get(8));

PointerType* PointerTy_12 = PointerType::get(FuncTy_8);

VectorType* VectorTy_13 = VectorType::get(Type::FloatTy, 4);

std::vector<const Type*>FuncTy_14_args;
FuncTy_14_args.push_back(VectorTy_13);
ParamAttrsList *FuncTy_14_PAL = 0;
FunctionType* FuncTy_14 = FunctionType::get(
  /*Result=*/VectorTy_13,
  /*Params=*/FuncTy_14_args,
  /*isVarArg=*/false,
  /*ParamAttrs=*/FuncTy_14_PAL);


// Function Declarations

Function* func_approx = new Function(
  /*Type=*/FuncTy_8,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"approx", mod); 
func_approx->setCallingConv(CallingConv::C);

Function* func_printf = mod->getFunction("printf");

Function* func_powf = new Function(
  /*Type=*/FuncTy_8,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"powf", mod); // (external, no body)
func_powf->setCallingConv(CallingConv::C);

Function* func_lit = new Function(
  /*Type=*/FuncTy_14,
  /*Linkage=*/GlobalValue::ExternalLinkage,
  /*Name=*/"lit", mod); 
func_lit->setCallingConv(CallingConv::C);

// Global Variable Declarations


GlobalVariable* gvar_array__str = new GlobalVariable(
/*Type=*/ArrayTy_0,
/*isConstant=*/true,
/*Linkage=*/GlobalValue::InternalLinkage,
/*Initializer=*/0, // has initializer, specified below
/*Name=*/".str",
mod);

GlobalVariable* gvar_array__str1 = new GlobalVariable(
/*Type=*/ArrayTy_2,
/*isConstant=*/true,
/*Linkage=*/GlobalValue::InternalLinkage,
/*Initializer=*/0, // has initializer, specified below
/*Name=*/".str1",
mod);

GlobalVariable* gvar_array__str2 = new GlobalVariable(
/*Type=*/ArrayTy_4,
/*isConstant=*/true,
/*Linkage=*/GlobalValue::InternalLinkage,
/*Initializer=*/0, // has initializer, specified below
/*Name=*/".str2",
mod);

GlobalVariable* gvar_array__str3 = new GlobalVariable(
/*Type=*/ArrayTy_6,
/*isConstant=*/true,
/*Linkage=*/GlobalValue::InternalLinkage,
/*Initializer=*/0, // has initializer, specified below
/*Name=*/".str3",
mod);

// Constant Definitions
Constant* const_array_15 = ConstantArray::get("After test with '%f' '%f'\x0A", true);
Constant* const_array_16 = ConstantArray::get("Calling pow with '%f' '%f'\x0A", true);
Constant* const_array_17 = ConstantArray::get("IN LIT\x0A", true);
Constant* const_array_18 = ConstantArray::get("About to approx  with '%f' '%f'\x0A", true);
ConstantFP* const_float_19 = ConstantFP::get(Type::FloatTy, APFloat(-1.280000e+02f));
ConstantFP* const_float_20 = ConstantFP::get(Type::FloatTy, APFloat(1.280000e+02f));
std::vector<Constant*> const_ptr_21_indices;
Constant* const_int32_22 = Constant::getNullValue(IntegerType::get(32));
const_ptr_21_indices.push_back(const_int32_22);
const_ptr_21_indices.push_back(const_int32_22);
Constant* const_ptr_21 = ConstantExpr::getGetElementPtr(gvar_array__str, &const_ptr_21_indices[0], const_ptr_21_indices.size() );
Constant* const_float_23 = Constant::getNullValue(Type::FloatTy);
std::vector<Constant*> const_ptr_24_indices;
const_ptr_24_indices.push_back(const_int32_22);
const_ptr_24_indices.push_back(const_int32_22);
Constant* const_ptr_24 = ConstantExpr::getGetElementPtr(gvar_array__str1, &const_ptr_24_indices[0], const_ptr_24_indices.size() );
std::vector<Constant*> const_ptr_25_indices;
const_ptr_25_indices.push_back(const_int32_22);
const_ptr_25_indices.push_back(const_int32_22);
Constant* const_ptr_25 = ConstantExpr::getGetElementPtr(gvar_array__str2, &const_ptr_25_indices[0], const_ptr_25_indices.size() );
std::vector<Constant*> const_packed_26_elems;
ConstantFP* const_float_27 = ConstantFP::get(Type::FloatTy, APFloat(1.000000e+00f));
const_packed_26_elems.push_back(const_float_27);
UndefValue* const_float_28 = UndefValue::get(Type::FloatTy);
const_packed_26_elems.push_back(const_float_28);
const_packed_26_elems.push_back(const_float_28);
const_packed_26_elems.push_back(const_float_27);
Constant* const_packed_26 = ConstantVector::get(VectorTy_13, const_packed_26_elems);
ConstantInt* const_int32_29 = ConstantInt::get(APInt(32,  "1", 10));
ConstantInt* const_int32_30 = ConstantInt::get(APInt(32,  "3", 10));
std::vector<Constant*> const_ptr_31_indices;
const_ptr_31_indices.push_back(const_int32_22);
const_ptr_31_indices.push_back(const_int32_22);
Constant* const_ptr_31 = ConstantExpr::getGetElementPtr(gvar_array__str3, &const_ptr_31_indices[0], const_ptr_31_indices.size() );
ConstantInt* const_int32_32 = ConstantInt::get(APInt(32,  "2", 10));
std::vector<Constant*> const_packed_33_elems;
const_packed_33_elems.push_back(const_float_27);
const_packed_33_elems.push_back(const_float_23);
const_packed_33_elems.push_back(const_float_23);
const_packed_33_elems.push_back(const_float_27);
Constant* const_packed_33 = ConstantVector::get(VectorTy_13, const_packed_33_elems);

// Global Variable Definitions
gvar_array__str->setInitializer(const_array_15);
gvar_array__str1->setInitializer(const_array_16);
gvar_array__str2->setInitializer(const_array_17);
gvar_array__str3->setInitializer(const_array_18);

// Function Definitions

// Function: approx (func_approx)
{
  Function::arg_iterator args = func_approx->arg_begin();
  Value* float_a = args++;
  float_a->setName("a");
  Value* float_b = args++;
  float_b->setName("b");
  
  BasicBlock* label_entry = new BasicBlock("entry",func_approx,0);
  
  // Block entry (label_entry)
  FCmpInst* int1_cmp = new FCmpInst(FCmpInst::FCMP_OLT, float_b, const_float_19, "cmp", label_entry);
  SelectInst* float_b_addr_0 = new SelectInst(int1_cmp, const_float_19, float_b, "b.addr.0", label_entry);
  FCmpInst* int1_cmp3 = new FCmpInst(FCmpInst::FCMP_OGT, float_b_addr_0, const_float_20, "cmp3", label_entry);
  SelectInst* float_b_addr_1 = new SelectInst(int1_cmp3, const_float_20, float_b_addr_0, "b.addr.1", label_entry);
  CastInst* double_conv = new FPExtInst(float_a, Type::DoubleTy, "conv", label_entry);
  CastInst* double_conv8 = new FPExtInst(float_b_addr_1, Type::DoubleTy, "conv8", label_entry);
  std::vector<Value*> int32_call_params;
  int32_call_params.push_back(const_ptr_21);
  int32_call_params.push_back(double_conv);
  int32_call_params.push_back(double_conv8);
  CallInst* int32_call = new CallInst(func_printf, int32_call_params.begin(), int32_call_params.end(), "call", label_entry);
  int32_call->setCallingConv(CallingConv::C);
  int32_call->setTailCall(true);
  FCmpInst* int1_cmp11 = new FCmpInst(FCmpInst::FCMP_OLT, float_a, const_float_23, "cmp11", label_entry);
  SelectInst* float_a_addr_0 = new SelectInst(int1_cmp11, const_float_23, float_a, "a.addr.0", label_entry);
  CastInst* double_conv16 = new FPExtInst(float_a_addr_0, Type::DoubleTy, "conv16", label_entry);
  std::vector<Value*> int32_call19_params;
  int32_call19_params.push_back(const_ptr_24);
  int32_call19_params.push_back(double_conv16);
  int32_call19_params.push_back(double_conv8);
  CallInst* int32_call19 = new CallInst(func_printf, int32_call19_params.begin(), int32_call19_params.end(), "call19", label_entry);
  int32_call19->setCallingConv(CallingConv::C);
  int32_call19->setTailCall(true);
  std::vector<Value*> float_call22_params;
  float_call22_params.push_back(float_a_addr_0);
  float_call22_params.push_back(float_b_addr_1);
  CallInst* float_call22 = new CallInst(func_powf, float_call22_params.begin(), float_call22_params.end(), "call22", label_entry);
  float_call22->setCallingConv(CallingConv::C);
  float_call22->setTailCall(true);
  new ReturnInst(float_call22, label_entry);
  
}

// Function: lit (func_lit)
{
  Function::arg_iterator args = func_lit->arg_begin();
  Value* packed_tmp = args++;
  packed_tmp->setName("tmp");
  
  BasicBlock* label_entry_35 = new BasicBlock("entry",func_lit,0);
  BasicBlock* label_ifthen = new BasicBlock("ifthen",func_lit,0);
  BasicBlock* label_UnifiedReturnBlock = new BasicBlock("UnifiedReturnBlock",func_lit,0);
  
  // Block entry (label_entry_35)
  CallInst* int32_call_36 = new CallInst(func_printf, const_ptr_25, "call", label_entry_35);
  int32_call_36->setCallingConv(CallingConv::C);
  int32_call_36->setTailCall(true);
  ExtractElementInst* float_tmp7 = new ExtractElementInst(packed_tmp, const_int32_22, "tmp7", label_entry_35);
  FCmpInst* int1_cmp_37 = new FCmpInst(FCmpInst::FCMP_OGT, float_tmp7, const_float_23, "cmp", label_entry_35);
  new BranchInst(label_ifthen, label_UnifiedReturnBlock, int1_cmp_37, label_entry_35);
  
  // Block ifthen (label_ifthen)
  InsertElementInst* packed_tmp12 = new InsertElementInst(const_packed_26, float_tmp7, const_int32_29, "tmp12", label_ifthen);
  ExtractElementInst* float_tmp14 = new ExtractElementInst(packed_tmp, const_int32_29, "tmp14", label_ifthen);
  CastInst* double_conv15 = new FPExtInst(float_tmp14, Type::DoubleTy, "conv15", label_ifthen);
  ExtractElementInst* float_tmp17 = new ExtractElementInst(packed_tmp, const_int32_30, "tmp17", label_ifthen);
  CastInst* double_conv18 = new FPExtInst(float_tmp17, Type::DoubleTy, "conv18", label_ifthen);
  std::vector<Value*> int32_call19_39_params;
  int32_call19_39_params.push_back(const_ptr_31);
  int32_call19_39_params.push_back(double_conv15);
  int32_call19_39_params.push_back(double_conv18);
  CallInst* int32_call19_39 = new CallInst(func_printf, int32_call19_39_params.begin(), int32_call19_39_params.end(), "call19", label_ifthen);
  int32_call19_39->setCallingConv(CallingConv::C);
  int32_call19_39->setTailCall(true);
  FCmpInst* int1_cmp_i = new FCmpInst(FCmpInst::FCMP_OLT, float_tmp17, const_float_19, "cmp.i", label_ifthen);
  SelectInst* float_b_addr_0_i = new SelectInst(int1_cmp_i, const_float_19, float_tmp17, "b.addr.0.i", label_ifthen);
  FCmpInst* int1_cmp3_i = new FCmpInst(FCmpInst::FCMP_OGT, float_b_addr_0_i, const_float_20, "cmp3.i", label_ifthen);
  SelectInst* float_b_addr_1_i = new SelectInst(int1_cmp3_i, const_float_20, float_b_addr_0_i, "b.addr.1.i", label_ifthen);
  CastInst* double_conv8_i = new FPExtInst(float_b_addr_1_i, Type::DoubleTy, "conv8.i", label_ifthen);
  std::vector<Value*> int32_call_i_params;
  int32_call_i_params.push_back(const_ptr_21);
  int32_call_i_params.push_back(double_conv15);
  int32_call_i_params.push_back(double_conv8_i);
  CallInst* int32_call_i = new CallInst(func_printf, int32_call_i_params.begin(), int32_call_i_params.end(), "call.i", label_ifthen);
  int32_call_i->setCallingConv(CallingConv::C);
  int32_call_i->setTailCall(true);
  FCmpInst* int1_cmp11_i = new FCmpInst(FCmpInst::FCMP_OLT, float_tmp14, const_float_23, "cmp11.i", label_ifthen);
  SelectInst* float_a_addr_0_i = new SelectInst(int1_cmp11_i, const_float_23, float_tmp14, "a.addr.0.i", label_ifthen);
  CastInst* double_conv16_i = new FPExtInst(float_a_addr_0_i, Type::DoubleTy, "conv16.i", label_ifthen);
  std::vector<Value*> int32_call19_i_params;
  int32_call19_i_params.push_back(const_ptr_24);
  int32_call19_i_params.push_back(double_conv16_i);
  int32_call19_i_params.push_back(double_conv8_i);
  CallInst* int32_call19_i = new CallInst(func_printf, int32_call19_i_params.begin(), int32_call19_i_params.end(), "call19.i", label_ifthen);
  int32_call19_i->setCallingConv(CallingConv::C);
  int32_call19_i->setTailCall(true);
  std::vector<Value*> float_call22_i_params;
  float_call22_i_params.push_back(float_a_addr_0_i);
  float_call22_i_params.push_back(float_b_addr_1_i);
  CallInst* float_call22_i = new CallInst(func_powf, float_call22_i_params.begin(), float_call22_i_params.end(), "call22.i", label_ifthen);
  float_call22_i->setCallingConv(CallingConv::C);
  float_call22_i->setTailCall(true);
  InsertElementInst* packed_tmp26 = new InsertElementInst(packed_tmp12, float_call22_i, const_int32_32, "tmp26", label_ifthen);
  new ReturnInst(packed_tmp26, label_ifthen);
  
  // Block UnifiedReturnBlock (label_UnifiedReturnBlock)
  new ReturnInst(const_packed_33, label_UnifiedReturnBlock);
  
}

return func_lit;

}

void Instructions::printVector(llvm::Value *val)
{
   static const char *frmt = "Vector is [%f, %f, %f, %f]\x0A";

   if (!m_fmtPtr) {
      Constant *format = ConstantArray::get(frmt, true);
      ArrayType *arrayTy = ArrayType::get(IntegerType::get(8), strlen(frmt) + 1);
      GlobalVariable* globalFormat = new GlobalVariable(
         /*Type=*/arrayTy,
         /*isConstant=*/true,
         /*Linkage=*/GlobalValue::InternalLinkage,
         /*Initializer=*/0, // has initializer, specified below
         /*Name=*/name(".str"),
         m_mod);
      globalFormat->setInitializer(format);

      Constant* const_int0 = Constant::getNullValue(IntegerType::get(32));
      std::vector<Constant*> const_ptr_21_indices;
      const_ptr_21_indices.push_back(const_int0);
      const_ptr_21_indices.push_back(const_int0);
      m_fmtPtr = ConstantExpr::getGetElementPtr(globalFormat,
                                                &const_ptr_21_indices[0], const_ptr_21_indices.size());
   }

   Function *func_printf = m_mod->getFunction("printf");
   ExtractElementInst *x = new ExtractElementInst(val, unsigned(0),
                                                  name("extractx"),
                                                  m_block);
   ExtractElementInst *y = new ExtractElementInst(val, unsigned(1),
                                                  name("extracty"),
                                                  m_block);
   ExtractElementInst *z = new ExtractElementInst(val, unsigned(2),
                                                  name("extractz"),
                                                  m_block);
   ExtractElementInst *w = new ExtractElementInst(val, unsigned(3),
                                                  name("extractw"),
                                                  m_block);
   CastInst *dx = new FPExtInst(x, Type::DoubleTy, name("dx"), m_block);
   CastInst *dy = new FPExtInst(y, Type::DoubleTy, name("dy"), m_block);
   CastInst *dz = new FPExtInst(z, Type::DoubleTy, name("dz"), m_block);
   CastInst *dw = new FPExtInst(w, Type::DoubleTy, name("dw"), m_block);
   std::vector<Value*> params;
   params.push_back(m_fmtPtr);
   params.push_back(dx);
   params.push_back(dy);
   params.push_back(dz);
   params.push_back(dw);
   CallInst* call = new CallInst(func_printf, params.begin(), params.end(), "printf", m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(true);
}
