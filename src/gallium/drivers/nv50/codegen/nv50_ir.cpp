/*
 * Copyright 2011 Christoph Bumiller
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "nv50_ir.h"
#include "nv50_ir_target.h"
#include "nv50_ir_driver.h"

extern "C" {
#include "nv50/nv50_program.h"
#include "nv50/nv50_debug.h"
}

namespace nv50_ir {

Modifier::Modifier(operation op)
{
   switch (op) {
   case OP_NEG: bits = NV50_IR_MOD_NEG; break;
   case OP_ABS: bits = NV50_IR_MOD_ABS; break;
   case OP_SAT: bits = NV50_IR_MOD_SAT; break;
   case OP_NOT: bits = NV50_IR_MOD_NOT; break;
   default:
      bits = 0;
      break;
   }
}

Modifier Modifier::operator*(const Modifier m) const
{
   unsigned int a, b, c;

   b = m.bits;
   if (this->bits & NV50_IR_MOD_ABS)
      b &= ~NV50_IR_MOD_NEG;

   a = (this->bits ^ b)      & (NV50_IR_MOD_NOT | NV50_IR_MOD_NEG);
   c = (this->bits | m.bits) & (NV50_IR_MOD_ABS | NV50_IR_MOD_SAT);

   return Modifier(a | c);
}

ValueRef::ValueRef() : value(NULL), insn(NULL)
{
   indirect[0] = -1;
   indirect[1] = -1;
   usedAsPtr = false;
}

ValueRef::ValueRef(const ValueRef& ref) : value(NULL), insn(ref.insn)
{
   set(ref);
   usedAsPtr = ref.usedAsPtr;
}

ValueRef::~ValueRef()
{
   this->set(NULL);
}

ImmediateValue *ValueRef::getImmediate() const
{
   Value *src = value;

   while (src) {
      if (src->reg.file == FILE_IMMEDIATE)
         return src->asImm();

      Instruction *insn = src->getUniqueInsn();

      src = (insn && insn->op == OP_MOV) ? insn->getSrc(0) : NULL;
   }
   return NULL;
}

ValueDef::ValueDef() : value(NULL), insn(NULL)
{
   // nothing to do
}

ValueDef::ValueDef(const ValueDef& def) : value(NULL), insn(NULL)
{
   set(def.get());
}

ValueDef::~ValueDef()
{
   this->set(NULL);
}

void
ValueRef::set(const ValueRef &ref)
{
   this->set(ref.get());
   mod = ref.mod;
   indirect[0] = ref.indirect[0];
   indirect[1] = ref.indirect[1];
}

void
ValueRef::set(Value *refVal)
{
   if (value == refVal)
      return;
   if (value)
      value->uses.remove(this);
   if (refVal)
      refVal->uses.push_back(this);

   value = refVal;
}

void
ValueDef::set(Value *defVal)
{
   if (value == defVal)
      return;
   if (value)
      value->defs.remove(this);
   if (defVal)
      defVal->defs.push_back(this);

   value = defVal;
}

void
ValueDef::replace(Value *repVal, bool doSet)
{
   if (value == repVal)
      return;

   while (value->refCount())
      value->uses.front()->set(repVal);

   if (doSet)
      set(repVal);
}

Value::Value()
{
  join = this;
  memset(&reg, 0, sizeof(reg));
  reg.size = 4;
}

bool
Value::coalesce(Value *jval, bool force)
{
   Value *repr = this->join; // new representative
   Value *jrep = jval->join;

   if (reg.file != jval->reg.file || reg.size != jval->reg.size) {
      if (!force)
         return false;
      ERROR("forced coalescing of values of different sizes/files");
   }

   if (!force && (repr->reg.data.id != jrep->reg.data.id)) {
      if (repr->reg.data.id >= 0 &&
          jrep->reg.data.id >= 0)
            return false;
      if (jrep->reg.data.id >= 0) {
         repr = jval->join;
         jrep = this->join;
         jval = this;
      }

      // need to check all fixed register values of the program for overlap
      Function *func = defs.front()->getInsn()->bb->getFunction();

      // TODO: put values in by register-id bins per function
      ArrayList::Iterator iter = func->allLValues.iterator();
      for (; !iter.end(); iter.next()) {
         Value *fixed = reinterpret_cast<Value *>(iter.get());
         assert(fixed);
         if (fixed->reg.data.id == repr->reg.data.id)
            if (fixed->livei.overlaps(jrep->livei))
               return false;
      }
   }
   if (repr->livei.overlaps(jrep->livei)) {
      if (!force)
         return false;
      // do we really want this ? if at all, only for constraint ops
      INFO("NOTE: forced coalescing with live range overlap\n");
   }

   for (DefIterator it = jrep->defs.begin(); it != jrep->defs.end(); ++it)
      (*it)->get()->join = repr;

   repr->defs.insert(repr->defs.end(),
                     jrep->defs.begin(), jrep->defs.end());
   repr->livei.unify(jrep->livei);

   assert(repr->join == repr && jval->join == repr);
   return true;
}

LValue::LValue(Function *fn, DataFile file)
{
   reg.file = file;
   reg.size = (file != FILE_PREDICATE) ? 4 : 1;
   reg.data.id = -1;

   affinity = -1;

   fn->add(this, this->id);
}

LValue::LValue(Function *fn, LValue *lval)
{
   assert(lval);

   reg.file = lval->reg.file;
   reg.size = lval->reg.size;
   reg.data.id = -1;

   affinity = -1;

   fn->add(this, this->id);
}

Value *LValue::clone(Function *func) const
{
   LValue *that = new_LValue(func, reg.file);

   that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
   that->reg.data = this->reg.data;

   return that;
}

Symbol::Symbol(Program *prog, DataFile f, ubyte fidx)
{
   baseSym = NULL;

   reg.file = f;
   reg.fileIndex = fidx;
   reg.data.offset = 0;

   prog->add(this, this->id);
}

Value *
Symbol::clone(Function *func) const
{
   Program *prog = func->getProgram();

   Symbol *that = new_Symbol(prog, reg.file, reg.fileIndex);

   that->reg.size = this->reg.size;
   that->reg.type = this->reg.type;
   that->reg.data = this->reg.data;

   that->baseSym = this->baseSym;

   return that;
}

ImmediateValue::ImmediateValue(Program *prog, uint32_t uval)
{
   memset(&reg, 0, sizeof(reg));

   reg.file = FILE_IMMEDIATE;
   reg.size = 4;
   reg.type = TYPE_U32;

   reg.data.u32 = uval;

   prog->add(this, this->id);
}

ImmediateValue::ImmediateValue(Program *prog, float fval)
{
   memset(&reg, 0, sizeof(reg));

   reg.file = FILE_IMMEDIATE;
   reg.size = 4;
   reg.type = TYPE_F32;

   reg.data.f32 = fval;

   prog->add(this, this->id);
}

ImmediateValue::ImmediateValue(Program *prog, double dval)
{
   memset(&reg, 0, sizeof(reg));

   reg.file = FILE_IMMEDIATE;
   reg.size = 8;
   reg.type = TYPE_F64;

   reg.data.f64 = dval;

   prog->add(this, this->id);
}

ImmediateValue::ImmediateValue(const ImmediateValue *proto, DataType ty)
{
   reg = proto->reg;

   reg.type = ty;
   reg.size = typeSizeof(ty);
}

bool
ImmediateValue::isInteger(const int i) const
{
   switch (reg.type) {
   case TYPE_S8:
      return reg.data.s8 == i;
   case TYPE_U8:
      return reg.data.u8 == i;
   case TYPE_S16:
      return reg.data.s16 == i;
   case TYPE_U16:
      return reg.data.u16 == i;
   case TYPE_S32:
   case TYPE_U32:
      return reg.data.s32 == i; // as if ...
   case TYPE_F32:
      return reg.data.f32 == static_cast<float>(i);
   case TYPE_F64:
      return reg.data.f64 == static_cast<double>(i);
   default:
      return false;
   }
}

bool
ImmediateValue::isNegative() const
{
   switch (reg.type) {
   case TYPE_S8:  return reg.data.s8 < 0;
   case TYPE_S16: return reg.data.s16 < 0;
   case TYPE_S32:
   case TYPE_U32: return reg.data.s32 < 0;
   case TYPE_F32: return reg.data.u32 & (1 << 31);
   case TYPE_F64: return reg.data.u64 & (1ULL << 63);
   default:
      return false;
   }
}

bool
ImmediateValue::isPow2() const
{
   switch (reg.type) {
   case TYPE_U8:
   case TYPE_U16:
   case TYPE_U32: return util_is_power_of_two(reg.data.u32);
   default:
      return false;
   }
}

void
ImmediateValue::applyLog2()
{
   switch (reg.type) {
   case TYPE_S8:
   case TYPE_S16:
   case TYPE_S32:
      assert(!this->isNegative());
      // fall through
   case TYPE_U8:
   case TYPE_U16:
   case TYPE_U32:
      reg.data.u32 = util_logbase2(reg.data.u32);
      break;
   case TYPE_F32:
      reg.data.f32 = log2f(reg.data.f32);
      break;
   case TYPE_F64:
      reg.data.f64 = log2(reg.data.f64);
      break;
   default:
      assert(0);
      break;
   }
}

bool
ImmediateValue::compare(CondCode cc, float fval) const
{
   if (reg.type != TYPE_F32)
      ERROR("immediate value is not of type f32");

   switch (static_cast<CondCode>(cc & 7)) {
   case CC_TR: return true;
   case CC_FL: return false;
   case CC_LT: return reg.data.f32 <  fval;
   case CC_LE: return reg.data.f32 <= fval;
   case CC_GT: return reg.data.f32 >  fval;
   case CC_GE: return reg.data.f32 >= fval;
   case CC_EQ: return reg.data.f32 == fval;
   case CC_NE: return reg.data.f32 != fval;
   default:
      assert(0);
      return false;
   }
}

bool
Value::interfers(const Value *that) const
{
   uint32_t idA, idB;

   if (that->reg.file != reg.file || that->reg.fileIndex != reg.fileIndex)
      return false;
   if (this->asImm())
      return false;

   if (this->asSym()) {
      idA = this->join->reg.data.offset;
      idB = that->join->reg.data.offset;
   } else {
      idA = this->join->reg.data.id * this->reg.size;
      idB = that->join->reg.data.id * that->reg.size;
   }

   if (idA < idB)
      return (idA + this->reg.size > idB);
   else
   if (idA > idB)
      return (idB + that->reg.size > idA);
   else
      return (idA == idB);
}

bool
Value::equals(const Value *that, bool strict) const
{
   that = that->join;

   if (strict)
      return this == that;

   if (that->reg.file != reg.file || that->reg.fileIndex != reg.fileIndex)
      return false;
   if (that->reg.size != this->reg.size)
      return false;

   if (that->reg.data.id != this->reg.data.id)
      return false;

   return true;
}

bool
ImmediateValue::equals(const Value *that, bool strict) const
{
   const ImmediateValue *imm = that->asImm();
   if (!imm)
      return false;
   return reg.data.u64 == imm->reg.data.u64;
}

bool
Symbol::equals(const Value *that, bool strict) const
{
   if (reg.file != that->reg.file || reg.fileIndex != that->reg.fileIndex)
      return false;
   assert(that->asSym());

   if (this->baseSym != that->asSym()->baseSym)
      return false;

   return this->reg.data.offset == that->reg.data.offset;
}

void Instruction::init()
{
   next = prev = 0;

   cc = CC_ALWAYS;
   rnd = ROUND_N;
   cache = CACHE_CA;
   subOp = 0;

   saturate = 0;
   join = terminator = 0;
   ftz = dnz = 0;
   atomic = 0;
   perPatch = 0;
   fixed = 0;
   encSize = 0;
   ipa = 0;

   lanes = 0xf;

   postFactor = 0;

   predSrc = -1;
   flagsDef = -1;
   flagsSrc = -1;
}

Instruction::Instruction()
{
   init();

   op = OP_NOP;
   dType = sType = TYPE_F32;

   id = -1;
   bb = 0;
}

Instruction::Instruction(Function *fn, operation opr, DataType ty)
{
   init();

   op = opr;
   dType = sType = ty;

   fn->add(this, id);
}

Instruction::~Instruction()
{
   if (bb) {
      Function *fn = bb->getFunction();
      bb->remove(this);
      fn->allInsns.remove(id);
   }

   for (int s = 0; srcExists(s); ++s)
      setSrc(s, NULL);
   // must unlink defs too since the list pointers will get deallocated
   for (int d = 0; defExists(d); ++d)
      setDef(d, NULL);
}

void
Instruction::setDef(int i, Value *val)
{
   int size = def.size();
   if (i >= size) {
      def.resize(i + 1);
      while (size <= i)
         def[size++].setInsn(this);
   }
   def[i].set(val);
}

void
Instruction::setSrc(int s, Value *val)
{
   int size = src.size();
   if (s >= size) {
      src.resize(s + 1);
      while (size <= s)
         src[size++].setInsn(this);
   }
   src[s].set(val);
}

void
Instruction::setSrc(int s, const ValueRef& ref)
{
   setSrc(s, ref.get());
   src[s].mod = ref.mod;
}

void
Instruction::swapSources(int a, int b)
{
   Value *value = src[a].get();
   Modifier m = src[a].mod;

   setSrc(a, src[b]);

   src[b].set(value);
   src[b].mod = m;
}

void
Instruction::takeExtraSources(int s, Value *values[3])
{
   values[0] = getIndirect(s, 0);
   if (values[0])
      setIndirect(s, 0, NULL);

   values[1] = getIndirect(s, 1);
   if (values[1])
      setIndirect(s, 1, NULL);

   values[2] = getPredicate();
   if (values[2])
      setPredicate(cc, NULL);
}

void
Instruction::putExtraSources(int s, Value *values[3])
{
   if (values[0])
      setIndirect(s, 0, values[0]);
   if (values[1])
      setIndirect(s, 1, values[1]);
   if (values[2])
      setPredicate(cc, values[2]);
}

Instruction *
Instruction::clone(bool deep) const
{
   Instruction *insn = new_Instruction(bb->getFunction(), op, dType);
   assert(!asCmp() && !asFlow());
   cloneBase(insn, deep);
   return insn;
}

void
Instruction::cloneBase(Instruction *insn, bool deep) const
{
   insn->sType = this->sType;

   insn->cc = this->cc;
   insn->rnd = this->rnd;
   insn->cache = this->cache;
   insn->subOp = this->subOp;

   insn->saturate = this->saturate;
   insn->atomic = this->atomic;
   insn->ftz = this->ftz;
   insn->dnz = this->dnz;
   insn->ipa = this->ipa;
   insn->lanes = this->lanes;
   insn->perPatch = this->perPatch;

   insn->postFactor = this->postFactor;

   if (deep) {
      if (!bb)
         return;
      Function *fn = bb->getFunction();
      for (int d = 0; this->defExists(d); ++d)
         insn->setDef(d, this->getDef(d)->clone(fn));
   } else {
      for (int d = 0; this->defExists(d); ++d)
         insn->setDef(d, this->getDef(d));
   }

   for (int s = 0; this->srcExists(s); ++s)
      insn->setSrc(s, this->src[s]);

   insn->predSrc = this->predSrc;
   insn->flagsDef = this->flagsDef;
   insn->flagsSrc = this->flagsSrc;
}

unsigned int
Instruction::defCount(unsigned int mask) const
{
   unsigned int i, n;

   for (n = 0, i = 0; this->defExists(i); ++i, mask >>= 1)
      n += mask & 1;
   return n;
}

unsigned int
Instruction::srcCount(unsigned int mask) const
{
   unsigned int i, n;

   for (n = 0, i = 0; this->srcExists(i); ++i, mask >>= 1)
      n += mask & 1;
   return n;
}

bool
Instruction::setIndirect(int s, int dim, Value *value)
{
   assert(this->srcExists(s));

   int p = src[s].indirect[dim];
   if (p < 0) {
      if (!value)
         return true;
      p = src.size();
   }
   setSrc(p, value);
   src[p].usedAsPtr = (value != 0);
   src[s].indirect[dim] = value ? p : -1;
   return true;
}

bool
Instruction::setPredicate(CondCode ccode, Value *value)
{
   cc = ccode;

   if (!value) {
      if (predSrc >= 0) {
         src[predSrc] = 0;
         predSrc = -1;
      }
      return true;
   }

   if (predSrc < 0)
      predSrc = src.size();

   setSrc(predSrc, value);
   return true;
}

bool
Instruction::writesPredicate() const
{
   for (int d = 0; defExists(d); ++d)
      if (getDef(d)->inFile(FILE_PREDICATE) || getDef(d)->inFile(FILE_FLAGS))
         return true;
   return false;
}

static bool
insnCheckCommutation(const Instruction *a, const Instruction *b)
{
   for (int d = 0; a->defExists(d); ++d)
      for (int s = 0; b->srcExists(s); ++s)
         if (a->getDef(d)->interfers(b->getSrc(s)))
            return false;
   return true;
}

bool
Instruction::isCommutationLegal(const Instruction *i) const
{
   bool ret = true;
   ret = ret && insnCheckCommutation(this, i);
   ret = ret && insnCheckCommutation(i, this);
   return ret;
}

TexInstruction::TexInstruction(Function *fn, operation op)
   : Instruction(fn, op, TYPE_F32)
{
   memset(&tex, 0, sizeof(tex));

   tex.rIndirectSrc = -1;
   tex.sIndirectSrc = -1;
}

TexInstruction::~TexInstruction()
{
   for (int c = 0; c < 3; ++c) {
      dPdx[c].set(NULL);
      dPdy[c].set(NULL);
   }
}

Instruction *
TexInstruction::clone(bool deep) const
{
   TexInstruction *tex = new_TexInstruction(bb->getFunction(), op);
   cloneBase(tex, deep);

   tex->tex = this->tex;

   if (op == OP_TXD) {
      for (unsigned int c = 0; c < tex->tex.target.getDim(); ++c) {
         tex->dPdx[c].set(dPdx[c]);
         tex->dPdy[c].set(dPdy[c]);
      }
   }

   return tex;
}

const struct TexInstruction::Target::Desc TexInstruction::Target::descTable[] =
{
   { "1D",                1, 1, false, false, false },
   { "2D",                2, 2, false, false, false },
   { "2D_MS",             2, 2, false, false, false },
   { "3D",                3, 3, false, false, false },
   { "CUBE",              2, 3, false, true,  false },
   { "1D_SHADOW",         1, 1, false, false, true  },
   { "2D_SHADOW",         2, 2, false, false, true  },
   { "CUBE_SHADOW",       2, 3, false, true,  true  },
   { "1D_ARRAY",          1, 2, true,  false, false },
   { "2D_ARRAY",          2, 3, true,  false, false },
   { "2D_MS_ARRAY",       2, 3, true,  false, false },
   { "CUBE_ARRAY",        2, 4, true,  true,  false },
   { "1D_ARRAY_SHADOW",   1, 2, true,  false, true  },
   { "2D_ARRAY_SHADOW",   2, 3, true,  false, true  },
   { "RECT",              2, 2, false, false, false },
   { "RECT_SHADOW",       2, 2, false, false, true  },
   { "CUBE_ARRAY_SHADOW", 2, 4, true,  true,  true  },
   { "BUFFER",            1, 1, false, false, false },
};

CmpInstruction::CmpInstruction(Function *fn, operation op)
   : Instruction(fn, op, TYPE_F32)
{
   setCond = CC_ALWAYS;
}

Instruction *
CmpInstruction::clone(bool deep) const
{
   CmpInstruction *cmp = new_CmpInstruction(bb->getFunction(), op);
   cloneBase(cmp, deep);
   cmp->setCond = setCond;
   cmp->dType = dType;
   return cmp;
}

FlowInstruction::FlowInstruction(Function *fn, operation op,
                                 BasicBlock *targ)
   : Instruction(fn, op, TYPE_NONE)
{
   target.bb = targ;

   if (op == OP_BRA ||
       op == OP_CONT || op == OP_BREAK ||
       op == OP_RET || op == OP_EXIT)
      terminator = 1;
   else
   if (op == OP_JOIN)
      terminator = targ ? 1 : 0;

   allWarp = absolute = limit = 0;
}

Program::Program(Type type, Target *arch)
   : progType(type),
     target(arch),
     mem_Instruction(sizeof(Instruction), 6),
     mem_CmpInstruction(sizeof(CmpInstruction), 4),
     mem_TexInstruction(sizeof(TexInstruction), 4),
     mem_FlowInstruction(sizeof(FlowInstruction), 4),
     mem_LValue(sizeof(LValue), 8),
     mem_Symbol(sizeof(Symbol), 7),
     mem_ImmediateValue(sizeof(ImmediateValue), 7)
{
   code = NULL;
   binSize = 0;

   maxGPR = -1;

   main = new Function(this, "MAIN");

   dbgFlags = 0;
}

Program::~Program()
{
   if (main)
      delete main;
}

void Program::releaseInstruction(Instruction *insn)
{
   // TODO: make this not suck so much

   insn->~Instruction();

   if (insn->asCmp())
      mem_CmpInstruction.release(insn);
   else
   if (insn->asTex())
      mem_TexInstruction.release(insn);
   else
   if (insn->asFlow())
      mem_FlowInstruction.release(insn);
   else
      mem_Instruction.release(insn);
}

void Program::releaseValue(Value *value)
{
   if (value->asLValue())
      mem_LValue.release(value);
   else
   if (value->asImm())
      mem_ImmediateValue.release(value);
   else
   if (value->asSym())
      mem_Symbol.release(value);
}


} // namespace nv50_ir

extern "C" {

static void
nv50_ir_init_prog_info(struct nv50_ir_prog_info *info)
{
#if defined(PIPE_SHADER_HULL) && defined(PIPE_SHADER_DOMAIN)
   if (info->type == PIPE_SHADER_HULL || info->type == PIPE_SHADER_DOMAIN) {
      info->prop.tp.domain = PIPE_PRIM_MAX;
      info->prop.tp.outputPrim = PIPE_PRIM_MAX;
   }
#endif
   if (info->type == PIPE_SHADER_GEOMETRY) {
      info->prop.gp.instanceCount = 1;
      info->prop.gp.maxVertices = 1;
   }
   info->io.clipDistance = 0xff;
   info->io.pointSize = 0xff;
   info->io.vertexId = 0xff;
   info->io.edgeFlagIn = 0xff;
   info->io.edgeFlagOut = 0xff;
   info->io.fragDepth = 0xff;
   info->io.sampleMask = 0xff;
   info->io.backFaceColor[0] = info->io.backFaceColor[1] = 0xff;
}
   
int
nv50_ir_generate_code(struct nv50_ir_prog_info *info)
{
   int ret = 0;

   nv50_ir::Program::Type type;

   nv50_ir_init_prog_info(info);

#define PROG_TYPE_CASE(a, b)                                      \
   case PIPE_SHADER_##a: type = nv50_ir::Program::TYPE_##b; break

   switch (info->type) {
   PROG_TYPE_CASE(VERTEX, VERTEX);
// PROG_TYPE_CASE(HULL, TESSELLATION_CONTROL);
// PROG_TYPE_CASE(DOMAIN, TESSELLATION_EVAL);
   PROG_TYPE_CASE(GEOMETRY, GEOMETRY);
   PROG_TYPE_CASE(FRAGMENT, FRAGMENT);
   default:
      type = nv50_ir::Program::TYPE_COMPUTE;
      break;
   }
   INFO_DBG(info->dbgFlags, VERBOSE, "translating program of type %u\n", type);

   nv50_ir::Target *targ = nv50_ir::Target::create(info->target);
   if (!targ)
      return -1;

   nv50_ir::Program *prog = new nv50_ir::Program(type, targ);
   if (!prog)
      return -1;
   prog->dbgFlags = info->dbgFlags;

   switch (info->bin.sourceRep) {
#if 0
   case PIPE_IR_LLVM:
   case PIPE_IR_GLSL:
      return -1;
   case PIPE_IR_SM4:
      ret = prog->makeFromSM4(info) ? 0 : -2;
      break;
   case PIPE_IR_TGSI:
#endif
   default:
      ret = prog->makeFromTGSI(info) ? 0 : -2;
      break;
   }
   if (ret < 0)
      goto out;
   if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
      prog->print();

   prog->getTarget()->runLegalizePass(prog, nv50_ir::CG_STAGE_PRE_SSA);

   prog->convertToSSA();

   if (prog->dbgFlags & NV50_IR_DEBUG_VERBOSE)
      prog->print();

   prog->optimizeSSA(info->optLevel);
   prog->getTarget()->runLegalizePass(prog, nv50_ir::CG_STAGE_SSA);

   if (prog->dbgFlags & NV50_IR_DEBUG_BASIC)
      prog->print();

   if (!prog->registerAllocation()) {
      ret = -4;
      goto out;
   }
   prog->getTarget()->runLegalizePass(prog, nv50_ir::CG_STAGE_POST_RA);

   prog->optimizePostRA(info->optLevel);

   if (!prog->emitBinary(info)) {
      ret = -5;
      goto out;
   }

out:
   INFO_DBG(prog->dbgFlags, VERBOSE, "nv50_ir_generate_code: ret = %i\n", ret);

   info->bin.maxGPR = prog->maxGPR;
   info->bin.code = prog->code;
   info->bin.codeSize = prog->binSize;

   delete prog;
   nv50_ir::Target::destroy(targ);

   return ret;
}

} // extern "C"
