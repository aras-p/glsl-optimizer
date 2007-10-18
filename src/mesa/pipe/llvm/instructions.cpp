#include "instructions.h"

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

using namespace llvm;

Instructions::Instructions(llvm::Module *mod, llvm::BasicBlock *block)
   :  m_mod(mod), m_block(block), m_idx(0)
{
   m_floatVecType = VectorType::get(Type::FloatTy, 4);

   m_llvmFSqrt = 0;
   m_llvmFAbs  = 0;
   m_llvmPow   = 0;
   m_llvmFloor = 0;
   m_llvmFlog  = 0;
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
                                  name("vecxyw"),
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

llvm::Value * Instructions::lit(llvm::Value *in1)
{
   return in1;
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

