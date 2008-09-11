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
#ifdef MESA_LLVM

#include "instructions.h"

#include "storage.h"

#include "util/u_memory.h"

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/Function.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>
#include <llvm/ParameterAttributes.h>
#include <llvm/Support/MemoryBuffer.h>
#include <llvm/Bitcode/ReaderWriter.h>

#include <sstream>
#include <fstream>
#include <iostream>

using namespace llvm;

#include "gallivm_builtins.cpp"

#if 0
llvm::Value *arrayFromChannels(std::vector<llvm::Value*> &vals)
{
   VectorType *vectorType = VectorType::get(Type::FloatTy, 4);
   ArrayType *vectorArray = ArrayType::get(vectorType, 4);
}
#endif

static inline std::string createFuncName(int label)
{
   std::ostringstream stream;
   stream << "function";
   stream << label;
   return stream.str();
}

Instructions::Instructions(llvm::Module *mod, llvm::Function *func, llvm::BasicBlock *block,
                           Storage *storage)
   :  m_mod(mod), m_func(func), m_builder(block), m_idx(0),
      m_storage(storage)
{
   m_floatVecType = VectorType::get(Type::FloatTy, 4);

   m_llvmFSqrt = 0;
   m_llvmFAbs  = 0;
   m_llvmPow   = 0;
   m_llvmFloor = 0;
   m_llvmFlog  = 0;
   m_llvmLit  = 0;
   m_fmtPtr = 0;

   MemoryBuffer *buffer = MemoryBuffer::getMemBuffer(
      (const char*)&llvm_builtins_data[0],
      (const char*)&llvm_builtins_data[Elements(llvm_builtins_data)-1]);
   m_mod = ParseBitcodeFile(buffer);
}

llvm::Value * Instructions::add(llvm::Value *in1, llvm::Value *in2)
{
   return m_builder.CreateAdd(in1, in2, name("add"));
}

llvm::Value * Instructions::madd(llvm::Value *in1, llvm::Value *in2,
                                 llvm::Value *in3)
{
   Value *mulRes = mul(in1, in2);
   return add(mulRes, in3);
}
 
llvm::Value * Instructions::mul(llvm::Value *in1, llvm::Value *in2)
{
   return m_builder.CreateMul(in1, in2, name("mul"));
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
   Value *x = m_builder.CreateExtractElement(mulRes,
                                                          m_storage->constantInt(0),
                                                          name("extractx"));
   Value *y = m_builder.CreateExtractElement(mulRes,
                                                          m_storage->constantInt(1),
                                                          name("extracty"));
   Value *z = m_builder.CreateExtractElement(mulRes,
                                                          m_storage->constantInt(2),
                                                          name("extractz"));
   Value *xy = m_builder.CreateAdd(x, y,name("xy"));
   Value *dot3 = m_builder.CreateAdd(xy, z, name("dot3"));
   return vectorFromVals(dot3, dot3, dot3, dot3);
}

llvm::Value *Instructions::callFSqrt(llvm::Value *val)
{
   if (!m_llvmFSqrt) {
      // predeclare the intrinsic
      std::vector<const Type*> fsqrtArgs;
      fsqrtArgs.push_back(Type::FloatTy);
      PAListPtr fsqrtPal;
      FunctionType* fsqrtType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/fsqrtArgs,
         /*isVarArg=*/false);
      m_llvmFSqrt = Function::Create(
         /*Type=*/fsqrtType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"llvm.sqrt.f32", m_mod);
      m_llvmFSqrt->setCallingConv(CallingConv::C);
      m_llvmFSqrt->setParamAttrs(fsqrtPal);
   }
   CallInst *call = m_builder.CreateCall(m_llvmFSqrt, val,
                                         name("sqrt"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::rsq(llvm::Value *in1)
{
   Value *x = m_builder.CreateExtractElement(in1,
                                             m_storage->constantInt(0),
                                             name("extractx"));
   Value *abs  = callFAbs(x);
   Value *sqrt = callFSqrt(abs);

   Value *rsqrt = m_builder.CreateFDiv(ConstantFP::get(APFloat(1.f)),
                                       sqrt,
                                       name("rsqrt"));
   return vectorFromVals(rsqrt, rsqrt, rsqrt, rsqrt);
}

llvm::Value * Instructions::vectorFromVals(llvm::Value *x, llvm::Value *y,
                                           llvm::Value *z, llvm::Value *w)
{
   Constant *const_vec = Constant::getNullValue(m_floatVecType);
   Value *res = m_builder.CreateInsertElement(const_vec, x,
                                              m_storage->constantInt(0),
                                              name("vecx"));
   res = m_builder.CreateInsertElement(res, y, m_storage->constantInt(1),
                               name("vecxy"));
   res = m_builder.CreateInsertElement(res, z, m_storage->constantInt(2),
                               name("vecxyz"));
   if (w)
      res = m_builder.CreateInsertElement(res, w, m_storage->constantInt(3),
                                          name("vecxyzw"));
   return res;
}

llvm::Value *Instructions::callFAbs(llvm::Value *val)
{
   if (!m_llvmFAbs) {
      // predeclare the intrinsic
      std::vector<const Type*> fabsArgs;
      fabsArgs.push_back(Type::FloatTy);
      PAListPtr fabsPal;
      FunctionType* fabsType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/fabsArgs,
         /*isVarArg=*/false);
      m_llvmFAbs = Function::Create(
         /*Type=*/fabsType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"fabs", m_mod);
      m_llvmFAbs->setCallingConv(CallingConv::C);
      m_llvmFAbs->setParamAttrs(fabsPal);
   }
   CallInst *call = m_builder.CreateCall(m_llvmFAbs, val,
                                         name("fabs"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::lit(llvm::Value *in)
{
   if (!m_llvmLit) {
      m_llvmLit = m_mod->getFunction("lit");
   }
   CallInst *call = m_builder.CreateCall(m_llvmLit, in, name("litres"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::sub(llvm::Value *in1, llvm::Value *in2)
{
   Value *res = m_builder.CreateSub(in1, in2, name("sub"));
   return res;
}

llvm::Value * Instructions::callPow(llvm::Value *val1, llvm::Value *val2)
{
   if (!m_llvmPow) {
      // predeclare the intrinsic
      std::vector<const Type*> powArgs;
      powArgs.push_back(Type::FloatTy);
      powArgs.push_back(Type::FloatTy);
      PAListPtr powPal;
      FunctionType* powType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/powArgs,
         /*isVarArg=*/false);
      m_llvmPow = Function::Create(
         /*Type=*/powType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"llvm.pow.f32", m_mod);
      m_llvmPow->setCallingConv(CallingConv::C);
      m_llvmPow->setParamAttrs(powPal);
   }
   std::vector<Value*> params;
   params.push_back(val1);
   params.push_back(val2);
   CallInst *call = m_builder.CreateCall(m_llvmPow, params.begin(), params.end(),
                                         name("pow"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::pow(llvm::Value *in1, llvm::Value *in2)
{
   Value *x1 = m_builder.CreateExtractElement(in1,
                                              m_storage->constantInt(0),
                                              name("x1"));
   Value *x2 = m_builder.CreateExtractElement(in2,
                                              m_storage->constantInt(0),
                                              name("x2"));
   llvm::Value *val = callPow(x1, x2);
   return vectorFromVals(val, val, val, val);
}

llvm::Value * Instructions::rcp(llvm::Value *in1)
{
   Value *x1 = m_builder.CreateExtractElement(in1,
                                              m_storage->constantInt(0),
                                              name("x1"));
   Value *res = m_builder.CreateFDiv(ConstantFP::get(APFloat(1.f)),
                                     x1, name("rcp"));
   return vectorFromVals(res, res, res, res);
}

llvm::Value * Instructions::dp4(llvm::Value *in1, llvm::Value *in2)
{
   Value *mulRes = mul(in1, in2);
   std::vector<llvm::Value*> vec = extractVector(mulRes);
   Value *xy = m_builder.CreateAdd(vec[0], vec[1], name("xy"));
   Value *xyz = m_builder.CreateAdd(xy, vec[2], name("xyz"));
   Value *dot4 = m_builder.CreateAdd(xyz, vec[3], name("dot4"));
   return vectorFromVals(dot4, dot4, dot4, dot4);
}

llvm::Value * Instructions::dph(llvm::Value *in1, llvm::Value *in2)
{
   Value *mulRes = mul(in1, in2);
   std::vector<llvm::Value*> vec1 = extractVector(mulRes);
   Value *xy = m_builder.CreateAdd(vec1[0], vec1[1], name("xy"));
   Value *xyz = m_builder.CreateAdd(xy, vec1[2], name("xyz"));
   Value *dph = m_builder.CreateAdd(xyz, vec1[3], name("dph"));
   return vectorFromVals(dph, dph, dph, dph);
}

llvm::Value * Instructions::dst(llvm::Value *in1, llvm::Value *in2)
{
   Value *y1 = m_builder.CreateExtractElement(in1,
                                              m_storage->constantInt(1),
                                              name("y1"));
   Value *z = m_builder.CreateExtractElement(in1,
                                             m_storage->constantInt(2),
                                             name("z"));
   Value *y2 = m_builder.CreateExtractElement(in2,
                                              m_storage->constantInt(1),
                                              name("y2"));
   Value *w = m_builder.CreateExtractElement(in2,
                                             m_storage->constantInt(3),
                                             name("w"));
   Value *ry = m_builder.CreateMul(y1, y2, name("tyuy"));
   return vectorFromVals(ConstantFP::get(APFloat(1.f)),
                         ry, z, w);
}

llvm::Value * Instructions::ex2(llvm::Value *in)
{
   llvm::Value *val = callPow(ConstantFP::get(APFloat(2.f)),
                              m_builder.CreateExtractElement(
                                 in, m_storage->constantInt(0),
                                 name("x1")));
   return vectorFromVals(val, val, val, val);
}

llvm::Value * Instructions::callFloor(llvm::Value *val)
{
   if (!m_llvmFloor) {
      // predeclare the intrinsic
      std::vector<const Type*> floorArgs;
      floorArgs.push_back(Type::FloatTy);
      PAListPtr floorPal;
      FunctionType* floorType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/floorArgs,
         /*isVarArg=*/false);
      m_llvmFloor = Function::Create(
         /*Type=*/floorType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"floorf", m_mod);
      m_llvmFloor->setCallingConv(CallingConv::C);
      m_llvmFloor->setParamAttrs(floorPal);
   }
   CallInst *call =  m_builder.CreateCall(m_llvmFloor, val,
                                          name("floorf"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::floor(llvm::Value *in)
{
   std::vector<llvm::Value*> vec = extractVector(in);
   return vectorFromVals(callFloor(vec[0]), callFloor(vec[1]),
                         callFloor(vec[2]), callFloor(vec[3]));
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
      PAListPtr flogPal;
      FunctionType* flogType = FunctionType::get(
         /*Result=*/Type::FloatTy,
         /*Params=*/flogArgs,
         /*isVarArg=*/false);
      m_llvmFlog = Function::Create(
         /*Type=*/flogType,
         /*Linkage=*/GlobalValue::ExternalLinkage,
         /*Name=*/"logf", m_mod);
      m_llvmFlog->setCallingConv(CallingConv::C);
      m_llvmFlog->setParamAttrs(flogPal);
   }
   CallInst *call = m_builder.CreateCall(m_llvmFlog, val,
                                         name("logf"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::lg2(llvm::Value *in)
{
   std::vector<llvm::Value*> vec = extractVector(in);
   llvm::Value *const_vec = constVector(1.442695f, 1.442695f,
                                        1.442695f, 1.442695f);
   return mul(vectorFromVals(callFLog(vec[0]), callFLog(vec[1]),
                             callFLog(vec[2]), callFLog(vec[3])), const_vec);
}

llvm::Value * Instructions::min(llvm::Value *in1, llvm::Value *in2)
{
   std::vector<llvm::Value*> vec1 = extractVector(in1);
   std::vector<llvm::Value*> vec2 = extractVector(in2);

   Value *xcmp  = m_builder.CreateFCmpOLT(vec1[0], vec2[0], name("xcmp"));
   Value *selx = m_builder.CreateSelect(xcmp, vec1[0], vec2[0],
                                        name("selx"));

   Value *ycmp  = m_builder.CreateFCmpOLT(vec1[1], vec2[1], name("ycmp"));
   Value *sely = m_builder.CreateSelect(ycmp, vec1[1], vec2[1],
                                        name("sely"));

   Value *zcmp  = m_builder.CreateFCmpOLT(vec1[2], vec2[2], name("zcmp"));
   Value *selz = m_builder.CreateSelect(zcmp, vec1[2], vec2[2],
                                        name("selz"));

   Value *wcmp  = m_builder.CreateFCmpOLT(vec1[3], vec2[3], name("wcmp"));
   Value *selw = m_builder.CreateSelect(wcmp, vec1[3], vec2[3],
                                        name("selw"));

   return vectorFromVals(selx, sely, selz, selw);
}

llvm::Value * Instructions::max(llvm::Value *in1, llvm::Value *in2)
{
   std::vector<llvm::Value*> vec1 = extractVector(in1);
   std::vector<llvm::Value*> vec2 = extractVector(in2);

   Value *xcmp  = m_builder.CreateFCmpOGT(vec1[0], vec2[0],
                                          name("xcmp"));
   Value *selx = m_builder.CreateSelect(xcmp, vec1[0], vec2[0],
                                        name("selx"));

   Value *ycmp  = m_builder.CreateFCmpOGT(vec1[1], vec2[1],
                                          name("ycmp"));
   Value *sely = m_builder.CreateSelect(ycmp, vec1[1], vec2[1],
                                        name("sely"));

   Value *zcmp  = m_builder.CreateFCmpOGT(vec1[2], vec2[2],
                                          name("zcmp"));
   Value *selz = m_builder.CreateSelect(zcmp, vec1[2], vec2[2],
                                        name("selz"));

   Value *wcmp  = m_builder.CreateFCmpOGT(vec1[3], vec2[3],
                                          name("wcmp"));
   Value *selw = m_builder.CreateSelect(wcmp, vec1[3], vec2[3],
                                        name("selw"));

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
   std::vector<llvm::Value*> vec = extractVector(val);
   Value *dx = m_builder.CreateFPExt(vec[0], Type::DoubleTy, name("dx"));
   Value *dy = m_builder.CreateFPExt(vec[1], Type::DoubleTy, name("dy"));
   Value *dz = m_builder.CreateFPExt(vec[2], Type::DoubleTy, name("dz"));
   Value *dw = m_builder.CreateFPExt(vec[3], Type::DoubleTy, name("dw"));
   std::vector<Value*> params;
   params.push_back(m_fmtPtr);
   params.push_back(dx);
   params.push_back(dy);
   params.push_back(dz);
   params.push_back(dw);
   CallInst *call = m_builder.CreateCall(func_printf, params.begin(), params.end(),
                                         name("printf"));
   call->setCallingConv(CallingConv::C);
   call->setTailCall(true);
}

llvm::Function * Instructions::declarePrintf()
{
   std::vector<const Type*> args;
   PAListPtr params;
   FunctionType* funcTy = FunctionType::get(
      /*Result=*/IntegerType::get(32),
      /*Params=*/args,
      /*isVarArg=*/true);
   Function* func_printf = Function::Create(
      /*Type=*/funcTy,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Name=*/"printf", m_mod);
   func_printf->setCallingConv(CallingConv::C);
   func_printf->setParamAttrs(params);
   return func_printf;
}


llvm::Value * Instructions::sgt(llvm::Value *in1, llvm::Value *in2)
{
   Constant *const1f = ConstantFP::get(APFloat(1.000000e+00f));
   Constant *const0f = Constant::getNullValue(Type::FloatTy);

   std::vector<llvm::Value*> vec1 = extractVector(in1);
   std::vector<llvm::Value*> vec2 = extractVector(in2);
   Value *xcmp = m_builder.CreateFCmpOGT(vec1[0], vec2[0], name("xcmp"));
   Value *x = m_builder.CreateSelect(xcmp, const1f, const0f, name("xsel"));

   Value *ycmp = m_builder.CreateFCmpOGT(vec1[1], vec2[1], name("ycmp"));
   Value *y = m_builder.CreateSelect(ycmp, const1f, const0f, name("ysel"));

   Value *zcmp = m_builder.CreateFCmpOGT(vec1[2], vec2[2], name("zcmp"));
   Value *z = m_builder.CreateSelect(zcmp, const1f, const0f, name("zsel"));

   Value *wcmp = m_builder.CreateFCmpOGT(vec1[3], vec2[3], name("wcmp"));
   Value *w = m_builder.CreateSelect(wcmp, const1f, const0f, name("wsel"));

   return vectorFromVals(x, y, z, w);
}
llvm::Value * Instructions::sge(llvm::Value *in1, llvm::Value *in2)
{
   Constant *const1f = ConstantFP::get(APFloat(1.000000e+00f));
   Constant *const0f = Constant::getNullValue(Type::FloatTy);

   std::vector<llvm::Value*> vec1 = extractVector(in1);
   std::vector<llvm::Value*> vec2 = extractVector(in2);

   Value *xcmp = m_builder.CreateFCmpOGE(vec1[0], vec2[0], name("xcmp"));
   Value *x = m_builder.CreateSelect(xcmp, const1f, const0f, name("xsel"));

   Value *ycmp = m_builder.CreateFCmpOGE(vec1[1], vec2[1], name("ycmp"));
   Value *y = m_builder.CreateSelect(ycmp, const1f, const0f, name("ysel"));

   Value *zcmp = m_builder.CreateFCmpOGE(vec1[2], vec2[2], name("zcmp"));
   Value *z = m_builder.CreateSelect(zcmp, const1f, const0f, name("zsel"));

   Value *wcmp = m_builder.CreateFCmpOGE(vec1[3], vec2[3], name("wcmp"));
   Value *w = m_builder.CreateSelect(wcmp, const1f, const0f, name("wsel"));

   return vectorFromVals(x, y, z, w);
}


llvm::Value * Instructions::slt(llvm::Value *in1, llvm::Value *in2)
{
   Constant *const1f = ConstantFP::get(APFloat(1.000000e+00f));
   Constant *const0f = Constant::getNullValue(Type::FloatTy);

   std::vector<llvm::Value*> vec1 = extractVector(in1);
   std::vector<llvm::Value*> vec2 = extractVector(in2);

   Value *xcmp = m_builder.CreateFCmpOLT(vec1[0], vec2[0], name("xcmp"));
   Value *x = m_builder.CreateSelect(xcmp, const1f, const0f, name("xsel"));

   Value *ycmp = m_builder.CreateFCmpOLT(vec1[1], vec2[1], name("ycmp"));
   Value *y = m_builder.CreateSelect(ycmp, const1f, const0f, name("ysel"));

   Value *zcmp = m_builder.CreateFCmpOLT(vec1[2], vec2[2], name("zcmp"));
   Value *z = m_builder.CreateSelect(zcmp, const1f, const0f, name("zsel"));

   Value *wcmp = m_builder.CreateFCmpOLT(vec1[3], vec2[3], name("wcmp"));
   Value *w = m_builder.CreateSelect(wcmp, const1f, const0f, name("wsel"));

   return vectorFromVals(x, y, z, w);
}

llvm::Value * Instructions::cross(llvm::Value *in1, llvm::Value *in2)
{
   Value *x1 = m_builder.CreateExtractElement(in1,
                                              m_storage->constantInt(0),
                                              name("x1"));
   Value *y1 = m_builder.CreateExtractElement(in1,
                                              m_storage->constantInt(1),
                                              name("y1"));
   Value *z1 = m_builder.CreateExtractElement(in1,
                                              m_storage->constantInt(2),
                                              name("z1"));

   Value *x2 = m_builder.CreateExtractElement(in2,
                                              m_storage->constantInt(0),
                                              name("x2"));
   Value *y2 = m_builder.CreateExtractElement(in2,
                                              m_storage->constantInt(1),
                                              name("y2"));
   Value *z2 = m_builder.CreateExtractElement(in2,
                                              m_storage->constantInt(2),
                                              name("z2"));
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
   std::vector<llvm::Value*> vec = extractVector(in);
   Value *xabs  = callFAbs(vec[0]);
   Value *yabs  = callFAbs(vec[1]);
   Value *zabs  = callFAbs(vec[2]);
   Value *wabs  = callFAbs(vec[3]);
   return vectorFromVals(xabs, yabs, zabs, wabs);
}

void Instructions::ifop(llvm::Value *in)
{
   BasicBlock *ifthen = BasicBlock::Create(name("ifthen"), m_func,0);
   BasicBlock *ifend = BasicBlock::Create(name("ifthenend"), m_func,0);

   //BasicBlock *yblock = new BasicBlock(name("yblock"), m_func,0);
   //BasicBlock *zblock = new BasicBlock(name("zblock"), m_func,0);
   //BasicBlock *wblock = new BasicBlock(name("wblock"), m_func,0);

   Constant *float0 = Constant::getNullValue(Type::FloatTy);

   Value *x = m_builder.CreateExtractElement(in, m_storage->constantInt(0),
                                             name("extractx"));
   Value *xcmp = m_builder.CreateFCmpUNE(x, float0, name("xcmp"));
   m_builder.CreateCondBr(xcmp, ifthen, ifend);
   //m_builder.SetInsertPoint(yblock);

   m_builder.SetInsertPoint(ifthen);
   m_ifStack.push(ifend);
}

llvm::BasicBlock * Instructions::currentBlock() const
{
   return m_builder.GetInsertBlock();
}

void Instructions::elseop()
{
   assert(!m_ifStack.empty());
   BasicBlock *ifend = BasicBlock::Create(name("ifend"), m_func,0);
   m_builder.CreateBr(ifend);
   m_builder.SetInsertPoint(m_ifStack.top());
   currentBlock()->setName(name("ifelse"));
   m_ifStack.pop();
   m_ifStack.push(ifend);
}

void Instructions::endif()
{
   assert(!m_ifStack.empty());
   m_builder.CreateBr(m_ifStack.top());
   m_builder.SetInsertPoint(m_ifStack.top());
   m_ifStack.pop();
}

llvm::Value * Instructions::lerp(llvm::Value *in1, llvm::Value *in2,
                                 llvm::Value *in3)
{
   llvm::Value *m = mul(in1, in2);
   llvm::Value *vec1 = constVector(1.f, 1.f, 1.f, 1.f);
   llvm::Value *s = sub(vec1, in1);
   return add(m, mul(s, in3));
}

void Instructions::beginLoop()
{
   BasicBlock *begin = BasicBlock::Create(name("loop"), m_func,0);
   BasicBlock *end = BasicBlock::Create(name("endloop"), m_func,0);

   m_builder.CreateBr(begin);
   Loop loop;
   loop.begin = begin;
   loop.end   = end;
   m_builder.SetInsertPoint(begin);
   m_loopStack.push(loop);
}

void Instructions::endLoop()
{
   assert(!m_loopStack.empty());
   Loop loop = m_loopStack.top();
   m_builder.CreateBr(loop.begin);
   loop.end->moveAfter(currentBlock());
   m_builder.SetInsertPoint(loop.end);
   m_loopStack.pop();
}

void Instructions::brk()
{
   assert(!m_loopStack.empty());
   BasicBlock *unr = BasicBlock::Create(name("unreachable"), m_func,0);
   m_builder.CreateBr(m_loopStack.top().end);
   m_builder.SetInsertPoint(unr);
}

llvm::Value * Instructions::trunc(llvm::Value *in)
{
   std::vector<llvm::Value*> vec = extractVector(in);
   Value *icastx = m_builder.CreateFPToSI(vec[0], IntegerType::get(32),
                                          name("ftoix"));
   Value *icasty = m_builder.CreateFPToSI(vec[1], IntegerType::get(32),
                                          name("ftoiy"));
   Value *icastz = m_builder.CreateFPToSI(vec[2], IntegerType::get(32),
                                          name("ftoiz"));
   Value *icastw = m_builder.CreateFPToSI(vec[3], IntegerType::get(32),
                                          name("ftoiw"));
   Value *fx = m_builder.CreateSIToFP(icastx, Type::FloatTy,
                                      name("fx"));
   Value *fy = m_builder.CreateSIToFP(icasty, Type::FloatTy,
                                      name("fy"));
   Value *fz = m_builder.CreateSIToFP(icastz, Type::FloatTy,
                                      name("fz"));
   Value *fw = m_builder.CreateSIToFP(icastw, Type::FloatTy,
                                      name("fw"));
   return vectorFromVals(fx, fy, fz, fw);
}

void Instructions::end()
{
   m_builder.CreateRetVoid();
}

void Instructions::cal(int label, llvm::Value *input)
{
   std::vector<Value*> params;
   params.push_back(input);
   llvm::Function *func = findFunction(label);

   m_builder.CreateCall(func, params.begin(), params.end());
}

llvm::Function * Instructions::declareFunc(int label)
{
   PointerType *vecPtr = PointerType::getUnqual(m_floatVecType);
   std::vector<const Type*> args;
   args.push_back(vecPtr);
   args.push_back(vecPtr);
   args.push_back(vecPtr);
   args.push_back(vecPtr);
   PAListPtr params;
   FunctionType *funcType = FunctionType::get(
      /*Result=*/Type::VoidTy,
      /*Params=*/args,
      /*isVarArg=*/false);
   std::string name = createFuncName(label);
   Function *func = Function::Create(
      /*Type=*/funcType,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Name=*/name.c_str(), m_mod);
   func->setCallingConv(CallingConv::C);
   func->setParamAttrs(params);
   return func;
}

void Instructions::bgnSub(unsigned label)
{
   llvm::Function *func = findFunction(label);

   Function::arg_iterator args = func->arg_begin();
   Value *ptr_INPUT = args++;
   ptr_INPUT->setName("INPUT");
   m_storage->pushArguments(ptr_INPUT);

   llvm::BasicBlock *entry = BasicBlock::Create("entry", func, 0);

   m_func = func;
   m_builder.SetInsertPoint(entry);
}

void Instructions::endSub()
{
   m_func = 0;
   m_builder.SetInsertPoint(0);
}

llvm::Function * Instructions::findFunction(int label)
{
   llvm::Function *func = m_functions[label];
   if (!func) {
      func = declareFunc(label);
      m_functions[label] = func;
   }
   return func;
}

llvm::Value * Instructions::constVector(float x, float y, float z, float w)
{
   std::vector<Constant*> vec(4);
   vec[0] = ConstantFP::get(APFloat(x));
   vec[1] = ConstantFP::get(APFloat(y));
   vec[2] = ConstantFP::get(APFloat(z));
   vec[3] = ConstantFP::get(APFloat(w));
   return ConstantVector::get(m_floatVecType, vec);
}


std::vector<llvm::Value*> Instructions::extractVector(llvm::Value *vec)
{
   std::vector<llvm::Value*> elems(4);
   elems[0] = m_builder.CreateExtractElement(vec, m_storage->constantInt(0),
                                             name("x"));
   elems[1] = m_builder.CreateExtractElement(vec, m_storage->constantInt(1),
                                             name("y"));
   elems[2] = m_builder.CreateExtractElement(vec, m_storage->constantInt(2),
                                             name("z"));
   elems[3] = m_builder.CreateExtractElement(vec, m_storage->constantInt(3),
                                             name("w"));
   return elems;
}

llvm::Value * Instructions::cmp(llvm::Value *in1, llvm::Value *in2, llvm::Value *in3)
{
   llvm::Function *func = m_mod->getFunction("cmp");
   assert(func);

   std::vector<Value*> params;
   params.push_back(in1);
   params.push_back(in2);
   params.push_back(in3);
   CallInst *call = m_builder.CreateCall(func, params.begin(), params.end(), name("cmpres"));
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::cos(llvm::Value *in)
{
#if 0
   llvm::Function *func = m_mod->getFunction("vcos");
   assert(func);

   CallInst *call = m_builder.CreateCall(func, in, name("cosres"));
   call->setTailCall(false);
   return call;
#else
   std::vector<llvm::Value*> elems = extractVector(in);
   Function *func = m_mod->getFunction("cosf");
   assert(func);
   CallInst *cos = m_builder.CreateCall(func, elems[0], name("cosres"));
   cos->setCallingConv(CallingConv::C);
   cos->setTailCall(true);
   return vectorFromVals(cos, cos, cos, cos);
#endif
}

llvm::Value * Instructions::scs(llvm::Value *in)
{
   llvm::Function *func = m_mod->getFunction("scs");
   assert(func);

   CallInst *call = m_builder.CreateCall(func, in, name("scsres"));
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::kil(llvm::Value *in)
{
   llvm::Function *func = m_mod->getFunction("kil");
   assert(func);

   CallInst *call = m_builder.CreateCall(func, in, name("kilpres"));
   call->setTailCall(false);
   return call;
}

llvm::Value * Instructions::sin(llvm::Value *in)
{
   llvm::Function *func = m_mod->getFunction("vsin");
   assert(func);

   CallInst *call = m_builder.CreateCall(func, in, name("sinres"));
   call->setTailCall(false);
   return call;
}
#endif //MESA_LLVM


