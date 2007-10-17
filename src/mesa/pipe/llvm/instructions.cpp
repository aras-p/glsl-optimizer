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
   m_llvmFAbs = 0;
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

