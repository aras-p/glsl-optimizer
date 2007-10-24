/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

 /*
  * Authors:
  *   Zack Rusin zack@tungstengraphics.com
  */

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
   if (!m_llvmLit) {
      m_llvmLit = makeLitFunction(m_mod);
   }
   CallInst *call = new CallInst(m_llvmLit, in, name("litres"), m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
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

llvm::Value * Instructions::arl(llvm::Value *in)
{
   return floor(in);
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
   if (!func_printf)
      func_printf = declarePrintf();
   assert(func_printf);
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
   CallInst* call = new CallInst(func_printf, params.begin(), params.end(),
                                 name("printf"), m_block);
   call->setCallingConv(CallingConv::C);
   call->setTailCall(true);
}

llvm::Function * Instructions::declarePrintf()
{
   std::vector<const Type*> args;
  ParamAttrsList *params = 0;
  FunctionType* funcTy = FunctionType::get(
     /*Result=*/IntegerType::get(32),
     /*Params=*/args,
     /*isVarArg=*/true,
     /*ParamAttrs=*/params);
  Function* func_printf = new Function(
     /*Type=*/funcTy,
     /*Linkage=*/GlobalValue::ExternalLinkage,
     /*Name=*/"printf", m_mod);
  func_printf->setCallingConv(CallingConv::C);
  return func_printf;
}


llvm::Value * Instructions::sgt(llvm::Value *in1, llvm::Value *in2)
{
   Constant *const1f = ConstantFP::get(Type::FloatTy, APFloat(1.000000e+00f));
   Constant *const0f = Constant::getNullValue(Type::FloatTy);

   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0), name("x1"), m_block);
   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0), name("x2"), m_block);
   FCmpInst *xcmp = new FCmpInst(FCmpInst::FCMP_OGT, x1, x2, name("xcmp"), m_block);
   SelectInst *x = new SelectInst(xcmp, const1f, const0f, name("xsel"), m_block);

   ExtractElementInst *y1 = new ExtractElementInst(in1, unsigned(1), name("y1"), m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1), name("y2"), m_block);
   FCmpInst *ycmp = new FCmpInst(FCmpInst::FCMP_OGT, y1, y2, name("ycmp"), m_block);
   SelectInst *y = new SelectInst(ycmp, const1f, const0f, name("ysel"), m_block);

   ExtractElementInst *z1 = new ExtractElementInst(in1, unsigned(2), name("z1"), m_block);
   ExtractElementInst *z2 = new ExtractElementInst(in2, unsigned(2), name("z2"), m_block);
   FCmpInst *zcmp = new FCmpInst(FCmpInst::FCMP_OGT, z1, z2, name("zcmp"), m_block);
   SelectInst *z = new SelectInst(zcmp, const1f, const0f, name("zsel"), m_block);

   ExtractElementInst *w1 = new ExtractElementInst(in1, unsigned(3), name("w1"), m_block);
   ExtractElementInst *w2 = new ExtractElementInst(in2, unsigned(3), name("w2"), m_block);
   FCmpInst *wcmp = new FCmpInst(FCmpInst::FCMP_OGT, w1, w2, name("wcmp"), m_block);
   SelectInst *w = new SelectInst(wcmp, const1f, const0f, name("wsel"), m_block);

   return vectorFromVals(x, y, z, w);
}
llvm::Value * Instructions::sge(llvm::Value *in1, llvm::Value *in2)
{
   Constant *const1f = ConstantFP::get(Type::FloatTy, APFloat(1.000000e+00f));
   Constant *const0f = Constant::getNullValue(Type::FloatTy);

   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0), name("x1"), m_block);
   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0), name("x2"), m_block);
   FCmpInst *xcmp = new FCmpInst(FCmpInst::FCMP_OGE, x1, x2, name("xcmp"), m_block);
   SelectInst *x = new SelectInst(xcmp, const1f, const0f, name("xsel"), m_block);

   ExtractElementInst *y1 = new ExtractElementInst(in1, unsigned(1), name("y1"), m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1), name("y2"), m_block);
   FCmpInst *ycmp = new FCmpInst(FCmpInst::FCMP_OGE, y1, y2, name("ycmp"), m_block);
   SelectInst *y = new SelectInst(ycmp, const1f, const0f, name("ysel"), m_block);

   ExtractElementInst *z1 = new ExtractElementInst(in1, unsigned(2), name("z1"), m_block);
   ExtractElementInst *z2 = new ExtractElementInst(in2, unsigned(2), name("z2"), m_block);
   FCmpInst *zcmp = new FCmpInst(FCmpInst::FCMP_OGE, z1, z2, name("zcmp"), m_block);
   SelectInst *z = new SelectInst(zcmp, const1f, const0f, name("zsel"), m_block);

   ExtractElementInst *w1 = new ExtractElementInst(in1, unsigned(3), name("w1"), m_block);
   ExtractElementInst *w2 = new ExtractElementInst(in2, unsigned(3), name("w2"), m_block);
   FCmpInst *wcmp = new FCmpInst(FCmpInst::FCMP_OGE, w1, w2, name("wcmp"), m_block);
   SelectInst *w = new SelectInst(wcmp, const1f, const0f, name("wsel"), m_block);

   return vectorFromVals(x, y, z, w);
}


llvm::Value * Instructions::slt(llvm::Value *in1, llvm::Value *in2)
{
   Constant *const1f = ConstantFP::get(Type::FloatTy, APFloat(1.000000e+00f));
   Constant *const0f = Constant::getNullValue(Type::FloatTy);

   ExtractElementInst *x1 = new ExtractElementInst(in1, unsigned(0), name("x1"), m_block);
   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0), name("x2"), m_block);
   FCmpInst *xcmp = new FCmpInst(FCmpInst::FCMP_OLT, x1, x2, name("xcmp"), m_block);
   SelectInst *x = new SelectInst(xcmp, const1f, const0f, name("xsel"), m_block);

   ExtractElementInst *y1 = new ExtractElementInst(in1, unsigned(1), name("y1"), m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1), name("y2"), m_block);
   FCmpInst *ycmp = new FCmpInst(FCmpInst::FCMP_OLT, y1, y2, name("ycmp"), m_block);
   SelectInst *y = new SelectInst(ycmp, const1f, const0f, name("ysel"), m_block);

   ExtractElementInst *z1 = new ExtractElementInst(in1, unsigned(2), name("z1"), m_block);
   ExtractElementInst *z2 = new ExtractElementInst(in2, unsigned(2), name("z2"), m_block);
   FCmpInst *zcmp = new FCmpInst(FCmpInst::FCMP_OLT, z1, z2, name("zcmp"), m_block);
   SelectInst *z = new SelectInst(zcmp, const1f, const0f, name("zsel"), m_block);

   ExtractElementInst *w1 = new ExtractElementInst(in1, unsigned(3), name("w1"), m_block);
   ExtractElementInst *w2 = new ExtractElementInst(in2, unsigned(3), name("w2"), m_block);
   FCmpInst *wcmp = new FCmpInst(FCmpInst::FCMP_OLT, w1, w2, name("wcmp"), m_block);
   SelectInst *w = new SelectInst(wcmp, const1f, const0f, name("wsel"), m_block);

   return vectorFromVals(x, y, z, w);
}

llvm::Value * Instructions::cross(llvm::Value *in1, llvm::Value *in2)
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

   ExtractElementInst *x2 = new ExtractElementInst(in2, unsigned(0),
                                                   name("x2"),
                                                   m_block);
   ExtractElementInst *y2 = new ExtractElementInst(in2, unsigned(1),
                                                   name("y2"),
                                                   m_block);
   ExtractElementInst *z2 = new ExtractElementInst(in2, unsigned(2),
                                                   name("z2"),
                                                   m_block);
   Value *y1z2 = mul(y1, z2);
   Value *z1y2 = mul(z1, y2);

   Value *z1x2 = mul(z1, x2);
   Value *x1z2 = mul(x1, z2);

   Value *x1y2 = mul(x1, y2);
   Value *y1x2 = mul(y1, x2);

   return vectorFromVals(sub(y1z2, z1y2), sub(z1x2, x1z2), sub(x1y2, y1x2));
}


llvm::Value * Instructions::abs(llvm::Value *in)
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
   Value *xabs  = callFAbs(x);
   Value *yabs  = callFAbs(y);
   Value *zabs  = callFAbs(z);
   Value *wabs  = callFAbs(w);
   return vectorFromVals(xabs, yabs, zabs, wabs);
}

/*
FIXME: hand write the lit function. Currently it's
generated from:
typedef __attribute__(( ocu_vector_type(4) )) float float4;

extern float powf(float a, float b);

inline float approx(float a, float b)
{
    if (b < -128.0f) b = -128.0f;
    if (b > 128.0f)   b = 128.0f;
    if (a < 0) a = 0;
    return powf(a, b);
}

float4 lit(float4 tmp)
{
    float4 result;
    result.x = 1.0;
    result.w = 1.0;
    if (tmp.x > 0) {
        result.y = tmp.x;
        result.z = approx(tmp.y, tmp.w);
    } else {
        result.y = 0;
        result.z = 0;
    }
    return result;
}
with:
clang --emit-llvm lit.c |llvm-as|opt -std-compile-opts|llvm2cpp -gen-contents -for=lit
*/
Function* makeLitFunction(Module *mod) {

   // Type Definitions
   std::vector<const Type*>FuncTy_0_args;
   FuncTy_0_args.push_back(Type::FloatTy);
   FuncTy_0_args.push_back(Type::FloatTy);
   ParamAttrsList *FuncTy_0_PAL = 0;
   FunctionType* FuncTy_0 = FunctionType::get(
      /*Result=*/Type::FloatTy,
      /*Params=*/FuncTy_0_args,
      /*isVarArg=*/false,
      /*ParamAttrs=*/FuncTy_0_PAL);

   PointerType* PointerTy_1 = PointerType::get(FuncTy_0);

   VectorType* VectorTy_2 = VectorType::get(Type::FloatTy, 4);

   std::vector<const Type*>FuncTy_3_args;
   FuncTy_3_args.push_back(VectorTy_2);
   ParamAttrsList *FuncTy_3_PAL = 0;
   FunctionType* FuncTy_3 = FunctionType::get(
      /*Result=*/VectorTy_2,
      /*Params=*/FuncTy_3_args,
      /*isVarArg=*/false,
      /*ParamAttrs=*/FuncTy_3_PAL);


   // Function Declarations

   Function* func_approx = new Function(
      /*Type=*/FuncTy_0,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Name=*/"approx", mod);
   func_approx->setCallingConv(CallingConv::C);

   Function* func_powf = new Function(
      /*Type=*/FuncTy_0,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Name=*/"powf", mod); // (external, no body)
   func_powf->setCallingConv(CallingConv::C);

   Function* func_lit = new Function(
      /*Type=*/FuncTy_3,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Name=*/"lit", mod);
   func_lit->setCallingConv(CallingConv::C);

   // Global Variable Declarations


   // Constant Definitions
   ConstantFP* const_float_4 = ConstantFP::get(Type::FloatTy, APFloat(-1.280000e+02f));
   ConstantFP* const_float_5 = ConstantFP::get(Type::FloatTy, APFloat(1.280000e+02f));
   Constant* const_float_6 = Constant::getNullValue(Type::FloatTy);
   Constant* const_int32_7 = Constant::getNullValue(IntegerType::get(32));
   std::vector<Constant*> const_packed_8_elems;
   ConstantFP* const_float_9 = ConstantFP::get(Type::FloatTy, APFloat(1.000000e+00f));
   const_packed_8_elems.push_back(const_float_9);
   UndefValue* const_float_10 = UndefValue::get(Type::FloatTy);
   const_packed_8_elems.push_back(const_float_10);
   const_packed_8_elems.push_back(const_float_10);
   const_packed_8_elems.push_back(const_float_9);
   Constant* const_packed_8 = ConstantVector::get(VectorTy_2, const_packed_8_elems);
   ConstantInt* const_int32_11 = ConstantInt::get(APInt(32,  "1", 10));
   ConstantInt* const_int32_12 = ConstantInt::get(APInt(32,  "3", 10));
   ConstantInt* const_int32_13 = ConstantInt::get(APInt(32,  "2", 10));
   std::vector<Constant*> const_packed_14_elems;
   const_packed_14_elems.push_back(const_float_9);
   const_packed_14_elems.push_back(const_float_6);
   const_packed_14_elems.push_back(const_float_6);
   const_packed_14_elems.push_back(const_float_9);
   Constant* const_packed_14 = ConstantVector::get(VectorTy_2, const_packed_14_elems);

   // Global Variable Definitions

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
      FCmpInst* int1_cmp = new FCmpInst(FCmpInst::FCMP_OLT, float_b, const_float_4, "cmp", label_entry);
      SelectInst* float_b_addr_0 = new SelectInst(int1_cmp, const_float_4, float_b, "b.addr.0", label_entry);
      FCmpInst* int1_cmp3 = new FCmpInst(FCmpInst::FCMP_OGT, float_b_addr_0, const_float_5, "cmp3", label_entry);
      SelectInst* float_b_addr_1 = new SelectInst(int1_cmp3, const_float_5, float_b_addr_0, "b.addr.1", label_entry);
      FCmpInst* int1_cmp7 = new FCmpInst(FCmpInst::FCMP_OLT, float_a, const_float_6, "cmp7", label_entry);
      SelectInst* float_a_addr_0 = new SelectInst(int1_cmp7, const_float_6, float_a, "a.addr.0", label_entry);
      std::vector<Value*> float_call_params;
      float_call_params.push_back(float_a_addr_0);
      float_call_params.push_back(float_b_addr_1);
      CallInst* float_call = new CallInst(func_powf, float_call_params.begin(), float_call_params.end(), "call", label_entry);
      float_call->setCallingConv(CallingConv::C);
      float_call->setTailCall(true);
      new ReturnInst(float_call, label_entry);
   }

   // Function: lit (func_lit)
   {
      Function::arg_iterator args = func_lit->arg_begin();
      Value* packed_tmp = args++;
      packed_tmp->setName("tmp");

      BasicBlock* label_entry_16 = new BasicBlock("entry",func_lit,0);
      BasicBlock* label_ifthen = new BasicBlock("ifthen",func_lit,0);
      BasicBlock* label_UnifiedReturnBlock = new BasicBlock("UnifiedReturnBlock",func_lit,0);

      // Block entry (label_entry_16)
      ExtractElementInst* float_tmp7 = new ExtractElementInst(packed_tmp, const_int32_7, "tmp7", label_entry_16);
      FCmpInst* int1_cmp_17 = new FCmpInst(FCmpInst::FCMP_OGT, float_tmp7, const_float_6, "cmp", label_entry_16);
      new BranchInst(label_ifthen, label_UnifiedReturnBlock, int1_cmp_17, label_entry_16);

      // Block ifthen (label_ifthen)
      InsertElementInst* packed_tmp12 = new InsertElementInst(const_packed_8, float_tmp7, const_int32_11, "tmp12", label_ifthen);
      ExtractElementInst* float_tmp14 = new ExtractElementInst(packed_tmp, const_int32_11, "tmp14", label_ifthen);
      ExtractElementInst* float_tmp16 = new ExtractElementInst(packed_tmp, const_int32_12, "tmp16", label_ifthen);
      FCmpInst* int1_cmp_i = new FCmpInst(FCmpInst::FCMP_OLT, float_tmp16, const_float_4, "cmp.i", label_ifthen);
      SelectInst* float_b_addr_0_i = new SelectInst(int1_cmp_i, const_float_4, float_tmp16, "b.addr.0.i", label_ifthen);
      FCmpInst* int1_cmp3_i = new FCmpInst(FCmpInst::FCMP_OGT, float_b_addr_0_i, const_float_5, "cmp3.i", label_ifthen);
      SelectInst* float_b_addr_1_i = new SelectInst(int1_cmp3_i, const_float_5, float_b_addr_0_i, "b.addr.1.i", label_ifthen);
      FCmpInst* int1_cmp7_i = new FCmpInst(FCmpInst::FCMP_OLT, float_tmp14, const_float_6, "cmp7.i", label_ifthen);
      SelectInst* float_a_addr_0_i = new SelectInst(int1_cmp7_i, const_float_6, float_tmp14, "a.addr.0.i", label_ifthen);
      std::vector<Value*> float_call_i_params;
      float_call_i_params.push_back(float_a_addr_0_i);
      float_call_i_params.push_back(float_b_addr_1_i);
      CallInst* float_call_i = new CallInst(func_powf, float_call_i_params.begin(), float_call_i_params.end(), "call.i", label_ifthen);
      float_call_i->setCallingConv(CallingConv::C);
      float_call_i->setTailCall(true);
      InsertElementInst* packed_tmp18 = new InsertElementInst(packed_tmp12, float_call_i, const_int32_13, "tmp18", label_ifthen);
      new ReturnInst(packed_tmp18, label_ifthen);

      // Block UnifiedReturnBlock (label_UnifiedReturnBlock)
      new ReturnInst(const_packed_14, label_UnifiedReturnBlock);

   }
   return func_lit;
}

void Instructions::ifop(llvm::Value *in)
{
   BasicBlock *ifthen = new BasicBlock(name("ifthen"), m_func,0);
   BasicBlock *ifend = new BasicBlock(name("ifend"), m_func,0);

   //BasicBlock *yblock = new BasicBlock(name("yblock"), m_func,0);
   //BasicBlock *zblock = new BasicBlock(name("zblock"), m_func,0);
   //BasicBlock *wblock = new BasicBlock(name("wblock"), m_func,0);

   Constant *float0 = Constant::getNullValue(Type::FloatTy);

   ExtractElementInst *x = new ExtractElementInst(in, unsigned(0), name("extractx"),
                                                  m_block);
   FCmpInst *xcmp = new FCmpInst(FCmpInst::FCMP_UNE, x, float0,
                                 name("xcmp"), m_block);
   new BranchInst(ifend, ifthen, xcmp, m_block);
   //m_block = yblock;


   m_block = ifthen;
   m_ifStack.push(ifend);
}

llvm::BasicBlock * Instructions::currentBlock() const
{
   return m_block;
}

void Instructions::endif()
{
   assert(!m_ifStack.empty());
   new BranchInst(m_ifStack.top(), m_block);
   m_block = m_ifStack.top();
   m_ifStack.pop();
}

llvm::Value * Instructions::lerp(llvm::Value *in1, llvm::Value *in2,
                                 llvm::Value *in3)
{
   llvm::Value *m = mul(in1, in2);
   llvm::Value *vec1 = vectorFromVals(ConstantFP::get(Type::FloatTy,
                                                      APFloat(1.f)),
                                      ConstantFP::get(Type::FloatTy,
                                                      APFloat(1.f)),
                                      ConstantFP::get(Type::FloatTy,
                                                      APFloat(1.f)),
                                      ConstantFP::get(Type::FloatTy,
                                                      APFloat(1.f)));
   llvm::Value *s = sub(vec1, in1);
   return add(m, mul(s, in3));
}

