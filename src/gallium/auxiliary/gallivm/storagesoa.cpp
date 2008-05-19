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
     m_immediates(0),
     m_idx(0)
{
}

void StorageSoa::addImmediate(float *vec)
{
   std::vector<float> vals(4);
   vals[0] = vec[0];
   vals[1] = vec[1];
   vals[2] = vec[2];
   vals[3] = vec[3];
   m_immediatesToFlush.push_back(vals);
}

void StorageSoa::declareImmediates()
{
   if (m_immediatesToFlush.empty())
      return;

   VectorType *vectorType = VectorType::get(Type::FloatTy, 4);
   ArrayType  *vectorChannels = ArrayType::get(vectorType, 4);
   ArrayType  *arrayType = ArrayType::get(vectorChannels, m_immediatesToFlush.size());

   m_immediates = new GlobalVariable(
      /*Type=*/arrayType,
      /*isConstant=*/false,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Initializer=*/0, // has initializer, specified below
      /*Name=*/name("immediates"),
      currentModule());

   std::vector<Constant*> arrayVals;
   for (unsigned int i = 0; i < m_immediatesToFlush.size(); ++i) {
      std::vector<float> vec = m_immediatesToFlush[i];
      std::vector<float> vals(4);
      std::vector<Constant*> channelArray;

      vals[0] = vec[0]; vals[1] = vec[0]; vals[2] = vec[0]; vals[3] = vec[0];
      llvm::Constant *xChannel = createConstGlobalVector(vals);

      vals[0] = vec[1]; vals[1] = vec[1]; vals[2] = vec[1]; vals[3] = vec[1];
      llvm::Constant *yChannel = createConstGlobalVector(vals);

      vals[0] = vec[2]; vals[1] = vec[2]; vals[2] = vec[2]; vals[3] = vec[2];
      llvm::Constant *zChannel = createConstGlobalVector(vals);

      vals[0] = vec[3]; vals[1] = vec[3]; vals[2] = vec[3]; vals[3] = vec[3];
      llvm::Constant *wChannel = createConstGlobalVector(vals);
      channelArray.push_back(xChannel);
      channelArray.push_back(yChannel);
      channelArray.push_back(zChannel);
      channelArray.push_back(wChannel);
      Constant *constChannels = ConstantArray::get(vectorChannels,
                                                   channelArray);
      arrayVals.push_back(constChannels);
   }
   Constant *constArray = ConstantArray::get(arrayType, arrayVals);
   m_immediates->setInitializer(constArray);

   m_immediatesToFlush.clear();
}

llvm::Value *StorageSoa::addrElement(int idx) const
{
   std::map<int, llvm::Value*>::const_iterator itr = m_addresses.find(idx);
   if (itr == m_addresses.end()) {
      debug_printf("Trying to access invalid shader 'address'\n");
      return 0;
   }
   llvm::Value * res = (*itr).second;

   res = new LoadInst(res, name("addr"), false, m_block);

   return res;
}

std::vector<llvm::Value*> StorageSoa::inputElement(llvm::Value *idx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_input, idx, 0);
   res[1] = element(m_input, idx, 1);
   res[2] = element(m_input, idx, 2);
   res[3] = element(m_input, idx, 3);

   return res;
}

std::vector<llvm::Value*> StorageSoa::constElement(llvm::Value *idx)
{
   std::vector<llvm::Value*> res(4);
   llvm::Value *xChannel, *yChannel, *zChannel, *wChannel;

   xChannel = elementPointer(m_consts, idx, 0);
   yChannel = elementPointer(m_consts, idx, 1);
   zChannel = elementPointer(m_consts, idx, 2);
   wChannel = elementPointer(m_consts, idx, 3);

   res[0] = alignedArrayLoad(xChannel);
   res[1] = alignedArrayLoad(yChannel);
   res[2] = alignedArrayLoad(zChannel);
   res[3] = alignedArrayLoad(wChannel);

   return res;
}

std::vector<llvm::Value*> StorageSoa::outputElement(llvm::Value *idx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_output, idx, 0);
   res[1] = element(m_output, idx, 1);
   res[2] = element(m_output, idx, 2);
   res[3] = element(m_output, idx, 3);

   return res;
}

std::vector<llvm::Value*> StorageSoa::tempElement(llvm::Value *idx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_temps, idx, 0);
   res[1] = element(m_temps, idx, 1);
   res[2] = element(m_temps, idx, 2);
   res[3] = element(m_temps, idx, 3);

   return res;
}

std::vector<llvm::Value*> StorageSoa::immediateElement(llvm::Value *idx)
{
   std::vector<llvm::Value*> res(4);

   res[0] = element(m_immediates, idx, 0);
   res[1] = element(m_immediates, idx, 1);
   res[2] = element(m_immediates, idx, 2);
   res[3] = element(m_immediates, idx, 3);

   return res;
}

llvm::Value * StorageSoa::elementPointer(llvm::Value *ptr, llvm::Value *index,
                                         int channel) const
{
   std::vector<Value*> indices;
   if (m_immediates == ptr)
      indices.push_back(constantInt(0));
   indices.push_back(index);
   indices.push_back(constantInt(channel));

   GetElementPtrInst *getElem = GetElementPtrInst::Create(ptr,
                                                          indices.begin(),
                                                          indices.end(),
                                                          name("ptr"),
                                                          m_block);
   return getElem;
}

llvm::Value * StorageSoa::element(llvm::Value *ptr, llvm::Value *index,
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

llvm::Constant * StorageSoa::createConstGlobalVector(const std::vector<float> &vec)
{
   VectorType *vectorType = VectorType::get(Type::FloatTy, 4);
   std::vector<Constant*> immValues;
   ConstantFP *constx = ConstantFP::get(APFloat(vec[0]));
   ConstantFP *consty = ConstantFP::get(APFloat(vec[1]));
   ConstantFP *constz = ConstantFP::get(APFloat(vec[2]));
   ConstantFP *constw = ConstantFP::get(APFloat(vec[3]));
   immValues.push_back(constx);
   immValues.push_back(consty);
   immValues.push_back(constz);
   immValues.push_back(constw);
   Constant  *constVector = ConstantVector::get(vectorType, immValues);

   return constVector;
}

std::vector<llvm::Value*> StorageSoa::load(enum tgsi_file_type type, int idx, int swizzle,
                                           llvm::Value *indIdx)
{
   std::vector<llvm::Value*> val(4);

   //if we have an indirect index, always use that
   //   if not use the integer offset to create one
   llvm::Value *realIndex = 0;
   if (indIdx)
      realIndex = indIdx;
   else
      realIndex = constantInt(idx);
   debug_printf("XXXXXXXXX realIdx = %p, indIdx = %p\n", realIndex, indIdx);

   switch(type) {
   case TGSI_FILE_INPUT:
      val = inputElement(realIndex);
      break;
   case TGSI_FILE_OUTPUT:
      val = outputElement(realIndex);
      break;
   case TGSI_FILE_TEMPORARY:
      val = tempElement(realIndex);
      break;
   case TGSI_FILE_CONSTANT:
      val = constElement(realIndex);
      break;
   case TGSI_FILE_IMMEDIATE:
      val = immediateElement(realIndex);
      break;
   case TGSI_FILE_ADDRESS:
      debug_printf("Address not handled in the load phase!\n");
      assert(0);
      break;
   default:
      debug_printf("Unknown load!\n");
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

void StorageSoa::store(enum tgsi_file_type type, int idx, const std::vector<llvm::Value*> &val,
                       int mask)
{
   llvm::Value *out = 0;
   switch(type) {
   case TGSI_FILE_OUTPUT:
      out = m_output;
      break;
   case TGSI_FILE_TEMPORARY:
      out = m_temps;
      break;
   case TGSI_FILE_INPUT:
      out = m_input;
      break;
   case TGSI_FILE_ADDRESS: {
      llvm::Value *addr = m_addresses[idx];
      if (!addr) {
         addAddress(idx);
         addr = m_addresses[idx];
         assert(addr);
      }
      new StoreInst(val[0], addr, false, m_block);
      return;
      break;
   }
   default:
      debug_printf("Can't save output of this type: %d !\n", type);
      assert(0);
      break;
   }
   llvm::Value *realIndex = constantInt(idx);
   if ((mask & TGSI_WRITEMASK_X)) {
      llvm::Value *xChannel = elementPointer(out, realIndex, 0);
      new StoreInst(val[0], xChannel, false, m_block);
   }
   if ((mask & TGSI_WRITEMASK_Y)) {
      llvm::Value *yChannel = elementPointer(out, realIndex, 1);
      new StoreInst(val[1], yChannel, false, m_block);
   }
   if ((mask & TGSI_WRITEMASK_Z)) {
      llvm::Value *zChannel = elementPointer(out, realIndex, 2);
      new StoreInst(val[2], zChannel, false, m_block);
   }
   if ((mask & TGSI_WRITEMASK_W)) {
      llvm::Value *wChannel = elementPointer(out, realIndex, 3);
      new StoreInst(val[3], wChannel, false, m_block);
   }
}

void StorageSoa::addAddress(int idx)
{
   GlobalVariable *val = new GlobalVariable(
      /*Type=*/IntegerType::get(32),
      /*isConstant=*/false,
      /*Linkage=*/GlobalValue::ExternalLinkage,
      /*Initializer=*/0, // has initializer, specified below
      /*Name=*/name("address"),
      currentModule());
   val->setInitializer(Constant::getNullValue(IntegerType::get(32)));

   debug_printf("adding to %d\n", idx);
   m_addresses[idx] = val;
}
