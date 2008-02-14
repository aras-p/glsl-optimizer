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

#include "storagesoa.h"

#include "gallivm_p.h"

#include "pipe/p_shader_tokens.h"
#include "pipe/p_debug.h"

#include <llvm/BasicBlock.h>
#include <llvm/Module.h>
#include <llvm/Value.h>

#include <llvm/CallingConv.h>
#include <llvm/Constants.h>
#include <llvm/DerivedTypes.h>
#include <llvm/InstrTypes.h>
#include <llvm/Instructions.h>

using namespace llvm;


StorageSoa::StorageSoa(llvm::BasicBlock *block,
                       llvm::Value *input,
                       llvm::Value *output,
                       llvm::Value *consts,
                       llvm::Value *temps)
   : m_block(block),
     m_input(input),
     m_output(output),
     m_consts(consts),
     m_temps(temps),
     m_idx(0)
{
}

void StorageSoa::addImmediate(float *vec)
{
   float vals[4]; //decompose into soa

   vals[0] = vec[0]; vals[1] = vec[0]; vals[2] = vec[0]; vals[3] = vec[0];
   llvm::Value *xChannel = createConstGlobalVector(vals);

   vals[0] = vec[1]; vals[1] = vec[1]; vals[2] = vec[1]; vals[3] = vec[1];
   llvm::Value *yChannel = createConstGlobalVector(vals);


   vals[0] = vec[2]; vals[1] = vec[2]; vals[2] = vec[2]; vals[3] = vec[2];
   llvm::Value *zChannel = createConstGlobalVector(vals);

   vals[0] = vec[3]; vals[1] = vec[3]; vals[2] = vec[3]; vals[3] = vec[3];
   llvm::Value *wChannel = createConstGlobalVector(vals);

   std::vector<llvm::Value*> res(4);
   res[0] = xChannel;
   res[1] = yChannel;
   res[2] = zChannel;
   res[3] = wChannel;

   m_immediates[m_immediates.size()] = res;
}

llvm::Value *StorageSoa::addrElement(int idx) const
{
   return 0;
}

std::vector<llvm::Value*> StorageSoa::inputElement(int idx, llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_input, idx, 0);
   res[1] = element(m_input, idx, 1);
   res[2] = element(m_input, idx, 2);
   res[3] = element(m_input, idx, 3);

   return res;
}

std::vector<llvm::Value*> StorageSoa::constElement(int idx, llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);
   llvm::Value *xChannel = elementPointer(m_consts, idx, 0);
   llvm::Value *yChannel = elementPointer(m_consts, idx, 1);
   llvm::Value *zChannel = elementPointer(m_consts, idx, 2);
   llvm::Value *wChannel = elementPointer(m_consts, idx, 3);

   res[0] = alignedArrayLoad(xChannel);
   res[1] = alignedArrayLoad(yChannel);
   res[2] = alignedArrayLoad(zChannel);
   res[3] = alignedArrayLoad(wChannel);

   return res;
}

std::vector<llvm::Value*> StorageSoa::outputElement(int idx, llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_output, idx, 0);
   res[1] = element(m_output, idx, 1);
   res[2] = element(m_output, idx, 2);
   res[3] = element(m_output, idx, 3);

   return res;
}

std::vector<llvm::Value*> StorageSoa::tempElement(int idx, llvm::Value *indIdx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_temps, idx, 0);
   res[1] = element(m_temps, idx, 1);
   res[2] = element(m_temps, idx, 2);
   res[3] = element(m_temps, idx, 3);

   return res;
}

std::vector<llvm::Value*> StorageSoa::immediateElement(int idx)
{
   std::vector<llvm::Value*> res(4);
   res = m_immediates[idx];

   res[0] = new LoadInst(res[0], name("immx"), false, m_block);
   res[1] = new LoadInst(res[1], name("immy"), false, m_block);
   res[2] = new LoadInst(res[2], name("immz"), false, m_block);
   res[3] = new LoadInst(res[3], name("immw"), false, m_block);

   return res;
}

llvm::Value * StorageSoa::extractIndex(llvm::Value *vec)
{
   return 0;
}

llvm::Value * StorageSoa::elementPointer(llvm::Value *ptr, int index,
                                         int channel) const
{
   std::vector<Value*> indices;
   indices.push_back(constantInt(index));
   indices.push_back(constantInt(channel));

   GetElementPtrInst *getElem = new GetElementPtrInst(ptr,
                                                      indices.begin(),
                                                      indices.end(),
                                                      name("ptr"),
                                                      m_block);
   return getElem;
}

llvm::Value * StorageSoa::element(llvm::Value *ptr, int index,
                                  int channel) const
{
   llvm::Value *res = elementPointer(ptr, index, channel);
   LoadInst *load = new LoadInst(res, name("element"), false, m_block);
   //load->setAlignment(8);
   return load;
}

const char * StorageSoa::name(const char *prefix) const
{
   ++m_idx;
   snprintf(m_name, 32, "%s%d", prefix, m_idx);
   return m_name;
}

llvm::ConstantInt * StorageSoa::constantInt(int idx) const
{
   if (m_constInts.find(idx) != m_constInts.end()) {
      return m_constInts[idx];
   }
   ConstantInt *constInt = ConstantInt::get(APInt(32,  idx));
   m_constInts[idx] = constInt;
   return constInt;
}

llvm::Value *StorageSoa::alignedArrayLoad(llvm::Value *val)
{
   VectorType  *vectorType = VectorType::get(Type::FloatTy, 4);
   PointerType *vectorPtr  = PointerType::get(vectorType, 0);

   CastInst *cast = new BitCastInst(val, vectorPtr, name("toVector"), m_block);
   LoadInst *load = new LoadInst(cast, name("alignLoad"), false, m_block);
   load->setAlignment(8);
   return load;
}

llvm::Module * StorageSoa::currentModule() const
{
    if (!m_block || !m_block->getParent())
       return 0;

    return m_block->getParent()->getParent();
}

llvm::Value * StorageSoa::createConstGlobalVector(float *vec)
{
   VectorType *vectorType = VectorType::get(Type::FloatTy, 4);
   GlobalVariable *immediate = new GlobalVariable(
      /*Type=*/vectorType,
      /*isConstant=*/true,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Initializer=*/0, // has initializer, specified below
      /*Name=*/name("immediate"),
      currentModule());

   std::vector<Constant*> immValues;
   ConstantFP *constx = ConstantFP::get(Type::FloatTy, APFloat(vec[0]));
   ConstantFP *consty = ConstantFP::get(Type::FloatTy, APFloat(vec[1]));
   ConstantFP *constz = ConstantFP::get(Type::FloatTy, APFloat(vec[2]));
   ConstantFP *constw = ConstantFP::get(Type::FloatTy, APFloat(vec[3]));
   immValues.push_back(constx);
   immValues.push_back(consty);
   immValues.push_back(constz);
   immValues.push_back(constw);
   Constant  *constVector = ConstantVector::get(vectorType, immValues);

   // Global Variable Definitions
   immediate->setInitializer(constVector);

   return immediate;
}

std::vector<llvm::Value*> StorageSoa::load(Argument type, int idx, int swizzle,
                                           llvm::Value *indIdx )
{
   std::vector<llvm::Value*> val(4);
   switch(type) {
   case Input:
      val = inputElement(idx, indIdx);
      break;
   case Output:
      val = outputElement(idx, indIdx);
      break;
   case Temp:
      val = tempElement(idx, indIdx);
      break;
   case Const:
      val = constElement(idx, indIdx);
      break;
   case Immediate:
      val = immediateElement(idx);
      break;
   case Address:
      debug_printf("Address not handled in the fetch phase!\n");
      assert(0);
      break;
   }
   if (!gallivm_is_swizzle(swizzle))
      return val;

   std::vector<llvm::Value*> res(4);

   res[0] = val[gallivm_x_swizzle(swizzle)];
   res[1] = val[gallivm_y_swizzle(swizzle)];
   res[2] = val[gallivm_z_swizzle(swizzle)];
   res[3] = val[gallivm_w_swizzle(swizzle)];
   return res;
}

void StorageSoa::store(Argument type, int idx, const std::vector<llvm::Value*> &val,
                       int mask)
{
   llvm::Value *out = 0;
   switch(type) {
   case Output:
      out = m_output;
      break;
   case Temp:
      out = m_temps;
      break;
   case Input:
      out = m_input;
      break;
   default:
      debug_printf("Can't save output of this type: %d !\n", type);
      assert(0);
      break;
   }

   if ((mask & TGSI_WRITEMASK_X)) {
      llvm::Value *xChannel = elementPointer(out, idx, 0);
      new StoreInst(val[0], xChannel, false, m_block);
   }
   if ((mask & TGSI_WRITEMASK_Y)) {
      llvm::Value *yChannel = elementPointer(out, idx, 1);
      new StoreInst(val[1], yChannel, false, m_block);
   }
   if ((mask & TGSI_WRITEMASK_Z)) {
      llvm::Value *zChannel = elementPointer(out, idx, 2);
      new StoreInst(val[2], zChannel, false, m_block);
   }
   if ((mask & TGSI_WRITEMASK_W)) {
      llvm::Value *wChannel = elementPointer(out, idx, 3);
      new StoreInst(val[3], wChannel, false, m_block);
   }
}
