//===-- R600KernelParameters.cpp - Lower kernel function arguments --------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This pass lowers kernel function arguments to loads from the vertex buffer.
//
// Kernel arguemnts are stored in the vertex buffer at an offset of 9 dwords,
// so arg0 needs to be loaded from VTX_BUFFER[9] and arg1 is loaded from
// VTX_BUFFER[10], etc.
//
//===----------------------------------------------------------------------===//

#include "AMDGPU.h"
#include "AMDIL.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/Constants.h"
#include "llvm/Function.h"
#include "llvm/Intrinsics.h"
#include "llvm/Metadata.h"
#include "llvm/Module.h"
#include "llvm/Target/TargetData.h"
#include "llvm/Support/IRBuilder.h"
#include "llvm/Support/TypeBuilder.h"

#include <map>
#include <set>

using namespace llvm;

namespace {

#define CONSTANT_CACHE_SIZE_DW 127

class R600KernelParameters : public FunctionPass
{
  const TargetData * TD;
  LLVMContext* Context;
  Module *mod;

  struct param
  {
    param() : val(NULL), ptr_val(NULL), offset_in_dw(0), size_in_dw(0),
              indirect(true), specialID(0) {}

    Value* val;
    Value* ptr_val;
    int offset_in_dw;
    int size_in_dw;

    bool indirect;

    std::string specialType;
    int specialID;

    int end() { return offset_in_dw + size_in_dw; }
    // The first 9 dwords are reserved for the grid sizes.
    int get_rat_offset() { return 9 + offset_in_dw; }
  };

  std::vector<param> params;

  bool isOpenCLKernel(const Function* fun);
  int getLastSpecialID(const std::string& TypeName);

  int getListSize();
  void AddParam(Argument* arg);
  int calculateArgumentSize(Argument* arg);
  void RunAna(Function* fun);
  void Replace(Function* fun);
  bool isIndirect(Value* val, std::set<Value*>& visited);
  void Propagate(Function* fun);
  void Propagate(Value* v, const Twine& name, bool indirect = true);
  Value* ConstantRead(Function* fun, param& p);
  Value* handleSpecial(Function* fun, param& p);
  bool isSpecialType(Type*);
  std::string getSpecialTypeName(Type*);
public:
  static char ID;
  R600KernelParameters() : FunctionPass(ID) {};
  R600KernelParameters(const TargetData* TD) : FunctionPass(ID), TD(TD) {}
  bool runOnFunction (Function &F);
  void getAnalysisUsage(AnalysisUsage &AU) const;
  const char *getPassName() const;
  bool doInitialization(Module &M);
  bool doFinalization(Module &M);
};

char R600KernelParameters::ID = 0;

static RegisterPass<R600KernelParameters> X("kerparam",
                            "OpenCL Kernel Parameter conversion", false, false);

bool R600KernelParameters::isOpenCLKernel(const Function* fun)
{
  Module *mod = const_cast<Function*>(fun)->getParent();
  NamedMDNode * md = mod->getOrInsertNamedMetadata("opencl.kernels");

  if (!md or !md->getNumOperands())
  {
    return false;
  }

  for (int i = 0; i < int(md->getNumOperands()); i++)
  {
    if (!md->getOperand(i) or !md->getOperand(i)->getOperand(0))
    {
      continue;
    }
    
    assert(md->getOperand(i)->getNumOperands() == 1);

    if (md->getOperand(i)->getOperand(0)->getName() == fun->getName())
    {
      return true;
    }
  }

  return false;
}

int R600KernelParameters::getLastSpecialID(const std::string& TypeName)
{
  int lastID = -1;

  for (std::vector<param>::iterator i = params.begin(); i != params.end(); i++)
  {
    if (i->specialType == TypeName)
    {
      lastID = i->specialID;
    }
  }

  return lastID;
}

int R600KernelParameters::getListSize()
{
  if (params.size() == 0)
  {
    return 0;
  }

  return params.back().end();
}

bool R600KernelParameters::isIndirect(Value* val, std::set<Value*>& visited)
{
  //XXX Direct parameters are not supported yet, so return true here.
  return true;
#if 0
  if (isa<LoadInst>(val))
  {
    return false;
  }

  if (isa<IntegerType>(val->getType()))
  {
    assert(0 and "Internal error");
    return false;
  }

  if (visited.count(val))
  {
    return false;
  }

  visited.insert(val);

  if (isa<GetElementPtrInst>(val))
  {
    GetElementPtrInst* GEP = dyn_cast<GetElementPtrInst>(val);
    GetElementPtrInst::op_iterator i = GEP->op_begin();

    for (i++; i != GEP->op_end(); i++)
    {
      if (!isa<Constant>(*i))
      {
        return true;
      }
    }
  }

  for (Value::use_iterator i = val->use_begin(); i != val->use_end(); i++)
  {
    Value* v2 = dyn_cast<Value>(*i);

    if (v2)
    {
      if (isIndirect(v2, visited))
      {
        return true;
      }
    }
  }

  return false;
#endif
}

void R600KernelParameters::AddParam(Argument* arg)
{
  param p;

  p.val = dyn_cast<Value>(arg);
  p.offset_in_dw = getListSize();
  p.size_in_dw = calculateArgumentSize(arg);

  if (isa<PointerType>(arg->getType()) and arg->hasByValAttr())
  {
    std::set<Value*> visited;
    p.indirect = isIndirect(p.val, visited);
  }

  params.push_back(p);
}

int R600KernelParameters::calculateArgumentSize(Argument* arg)
{
  Type* t = arg->getType();

  if (arg->hasByValAttr() and dyn_cast<PointerType>(t))
  {
    t = dyn_cast<PointerType>(t)->getElementType();
  }

  int store_size_in_dw = (TD->getTypeStoreSize(t) + 3)/4;

  assert(store_size_in_dw);

  return store_size_in_dw;
}


void R600KernelParameters::RunAna(Function* fun)
{
  assert(isOpenCLKernel(fun));

  for (Function::arg_iterator i = fun->arg_begin(); i != fun->arg_end(); i++)
  {
    AddParam(i);
  }

}

void R600KernelParameters::Replace(Function* fun)
{
  for (std::vector<param>::iterator i = params.begin(); i != params.end(); i++)
  {
    Value *new_val;

    if (isSpecialType(i->val->getType()))
    {
      new_val = handleSpecial(fun, *i);
    }
    else
    {
      new_val = ConstantRead(fun, *i);
    }
    if (new_val)
    {
      i->val->replaceAllUsesWith(new_val);
    }
  }
}

void R600KernelParameters::Propagate(Function* fun)
{
  for (std::vector<param>::iterator i = params.begin(); i != params.end(); i++)
  {
    if (i->ptr_val)
    {
      Propagate(i->ptr_val, i->val->getName(), i->indirect);
   }
  }
}

void R600KernelParameters::Propagate(Value* v, const Twine& name, bool indirect)
{
  LoadInst* load = dyn_cast<LoadInst>(v);
  GetElementPtrInst *GEP = dyn_cast<GetElementPtrInst>(v);

  unsigned addrspace;

  if (indirect)
  {
    addrspace = AMDILAS::PARAM_I_ADDRESS;
  }
  else
  {
    addrspace = AMDILAS::PARAM_D_ADDRESS;
  }

  if (GEP and GEP->getType()->getAddressSpace() != addrspace)
  {
    Value* op = GEP->getPointerOperand();

    if (dyn_cast<PointerType>(op->getType())->getAddressSpace() != addrspace)
    {
      op = new BitCastInst(op, PointerType::get(dyn_cast<PointerType>(
                           op->getType())->getElementType(), addrspace),
                           name, dyn_cast<Instruction>(v));
    }

    std::vector<Value*> params(GEP->idx_begin(), GEP->idx_end());

    GetElementPtrInst* GEP2 = GetElementPtrInst::Create(op, params, name,
                                                      dyn_cast<Instruction>(v));
    GEP2->setIsInBounds(GEP->isInBounds());
    v = dyn_cast<Value>(GEP2);
    GEP->replaceAllUsesWith(GEP2);
    GEP->eraseFromParent();
    load = NULL;
  }

  if (load)
  {
    ///normally at this point we have the right address space
    if (load->getPointerAddressSpace() != addrspace)
    {
      Value *orig_ptr = load->getPointerOperand();
      PointerType *orig_ptr_type = dyn_cast<PointerType>(orig_ptr->getType());

      Type* new_ptr_type = PointerType::get(orig_ptr_type->getElementType(),
                                            addrspace);

      Value* new_ptr = orig_ptr;

      if (orig_ptr->getType() != new_ptr_type)
      {
        new_ptr = new BitCastInst(orig_ptr, new_ptr_type, "prop_cast", load);
      }

      Value* new_load = new LoadInst(new_ptr, name, load);
      load->replaceAllUsesWith(new_load);
      load->eraseFromParent();
    }

    return;
  }

  std::vector<User*> users(v->use_begin(), v->use_end());

  for (int i = 0; i < int(users.size()); i++)
  {
    Value* v2 = dyn_cast<Value>(users[i]);

    if (v2)
    {
      Propagate(v2, name, indirect);
    }
  }
}

Value* R600KernelParameters::ConstantRead(Function* fun, param& p)
{
  assert(fun->front().begin() != fun->front().end());

  Instruction *first_inst = fun->front().begin();
  IRBuilder <> builder (first_inst);
/* First 3 dwords are reserved for the dimmension info */

  if (!p.val->hasNUsesOrMore(1))
  {
    return NULL;
  }
  unsigned addrspace;

  if (p.indirect)
  {
    addrspace = AMDILAS::PARAM_I_ADDRESS;
  }
  else
  {
    addrspace = AMDILAS::PARAM_D_ADDRESS;
  }

  Argument *arg = dyn_cast<Argument>(p.val);
  Type * argType = p.val->getType();
  PointerType * argPtrType = dyn_cast<PointerType>(p.val->getType());

  if (argPtrType and arg->hasByValAttr())
  {
    Value* param_addr_space_ptr = ConstantPointerNull::get(
                                    PointerType::get(Type::getInt32Ty(*Context),
                                    addrspace));
    Value* param_ptr = GetElementPtrInst::Create(param_addr_space_ptr,
                                    ConstantInt::get(Type::getInt32Ty(*Context),
                                    p.get_rat_offset()), arg->getName(),
                                    first_inst);
    param_ptr = new BitCastInst(param_ptr,
                                PointerType::get(argPtrType->getElementType(),
                                                 addrspace),
                                arg->getName(), first_inst);
    p.ptr_val = param_ptr;
    return param_ptr;
  }
  else
  {
    Value* param_addr_space_ptr = ConstantPointerNull::get(PointerType::get(
                                                        argType, addrspace));

    Value* param_ptr = builder.CreateGEP(param_addr_space_ptr,
             ConstantInt::get(Type::getInt32Ty(*Context), p.get_rat_offset()),
                              arg->getName());

    Value* param_value = builder.CreateLoad(param_ptr, arg->getName());

    return param_value;
  }
}

Value* R600KernelParameters::handleSpecial(Function* fun, param& p)
{
  std::string name = getSpecialTypeName(p.val->getType());
  int ID;

  assert(!name.empty());

  if (name == "image2d_t" or name == "image3d_t")
  {
    int lastID = std::max(getLastSpecialID("image2d_t"),
                     getLastSpecialID("image3d_t"));

    if (lastID == -1)
    {
      ID = 2; ///ID0 and ID1 are used internally by the driver
    }
    else
    {
      ID = lastID + 1;
    }
  }
  else if (name == "sampler_t")
  {
    int lastID = getLastSpecialID("sampler_t");

    if (lastID == -1)
    {
      ID = 0;
    }
    else
    {
      ID = lastID + 1;
    }
  }
  else
  {
    ///TODO: give some error message
    return NULL;
  }

  p.specialType = name;
  p.specialID = ID;

  Instruction *first_inst = fun->front().begin();

  return new IntToPtrInst(ConstantInt::get(Type::getInt32Ty(*Context),
                                           p.specialID), p.val->getType(),
                                           "resourceID", first_inst);
}


bool R600KernelParameters::isSpecialType(Type* t)
{
  return !getSpecialTypeName(t).empty();
}

std::string R600KernelParameters::getSpecialTypeName(Type* t)
{
  PointerType *pt = dyn_cast<PointerType>(t);
  StructType *st = NULL;

  if (pt)
  {
    st = dyn_cast<StructType>(pt->getElementType());
  }

  if (st)
  {
    std::string prefix = "struct.opencl_builtin_type_";

    std::string name = st->getName().str();

    if (name.substr(0, prefix.length()) == prefix)
    {
      return name.substr(prefix.length(), name.length());
    }
  }

  return "";
}


bool R600KernelParameters::runOnFunction (Function &F)
{
  if (!isOpenCLKernel(&F))
  {
    return false;
  }

  RunAna(&F);
  Replace(&F);
  Propagate(&F);

  return false;
}

void R600KernelParameters::getAnalysisUsage(AnalysisUsage &AU) const
{
  FunctionPass::getAnalysisUsage(AU);
  AU.setPreservesAll();
}

const char *R600KernelParameters::getPassName() const
{
  return "OpenCL Kernel parameter conversion to memory";
}

bool R600KernelParameters::doInitialization(Module &M)
{
  Context = &M.getContext();
  mod = &M;

  return false;
}

bool R600KernelParameters::doFinalization(Module &M)
{
  return false;
}

} // End anonymous namespace

FunctionPass* llvm::createR600KernelParametersPass(const TargetData* TD)
{
  FunctionPass *p = new R600KernelParameters(TD);

  return p;
}


