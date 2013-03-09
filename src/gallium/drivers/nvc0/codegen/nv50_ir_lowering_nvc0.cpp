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

#include "nv50/codegen/nv50_ir.h"
#include "nv50/codegen/nv50_ir_build_util.h"

#include "nv50_ir_target_nvc0.h"

#include <limits>

namespace nv50_ir {

#define QOP_ADD  0
#define QOP_SUBR 1
#define QOP_SUB  2
#define QOP_MOV2 3

//             UL UR LL LR
#define QUADOP(q, r, s, t)                      \
   ((QOP_##q << 6) | (QOP_##r << 4) |           \
    (QOP_##s << 2) | (QOP_##t << 0))

class NVC0LegalizeSSA : public Pass
{
private:
   virtual bool visit(BasicBlock *);
   virtual bool visit(Function *);

   // we want to insert calls to the builtin library only after optimization
   void handleDIV(Instruction *); // integer division, modulus
   void handleRCPRSQ(Instruction *); // double precision float recip/rsqrt

private:
   BuildUtil bld;
};

void
NVC0LegalizeSSA::handleDIV(Instruction *i)
{
   FlowInstruction *call;
   int builtin;
   Value *def[2];

   bld.setPosition(i, false);
   def[0] = bld.mkMovToReg(0, i->getSrc(0))->getDef(0);
   def[1] = bld.mkMovToReg(1, i->getSrc(1))->getDef(0);
   switch (i->dType) {
   case TYPE_U32: builtin = NVC0_BUILTIN_DIV_U32; break;
   case TYPE_S32: builtin = NVC0_BUILTIN_DIV_S32; break;
   default:
      return;
   }
   call = bld.mkFlow(OP_CALL, NULL, CC_ALWAYS, NULL);
   bld.mkMov(i->getDef(0), def[(i->op == OP_DIV) ? 0 : 1]);
   bld.mkClobber(FILE_GPR, (i->op == OP_DIV) ? 0xe : 0xd, 2);
   bld.mkClobber(FILE_PREDICATE, (i->dType == TYPE_S32) ? 0xf : 0x3, 0);

   call->fixed = 1;
   call->absolute = call->builtin = 1;
   call->target.builtin = builtin;
   delete_Instruction(prog, i);
}

void
NVC0LegalizeSSA::handleRCPRSQ(Instruction *i)
{
   // TODO
}

bool
NVC0LegalizeSSA::visit(Function *fn)
{
   bld.setProgram(fn->getProgram());
   return true;
}

bool
NVC0LegalizeSSA::visit(BasicBlock *bb)
{
   Instruction *next;
   for (Instruction *i = bb->getEntry(); i; i = next) {
      next = i->next;
      if (i->dType == TYPE_F32)
         continue;
      switch (i->op) {
      case OP_DIV:
      case OP_MOD:
         handleDIV(i);
         break;
      case OP_RCP:
      case OP_RSQ:
         if (i->dType == TYPE_F64)
            handleRCPRSQ(i);
         break;
      default:
         break;
      }
   }
   return true;
}

class NVC0LegalizePostRA : public Pass
{
public:
   NVC0LegalizePostRA(const Program *);

private:
   virtual bool visit(Function *);
   virtual bool visit(BasicBlock *);

   void replaceZero(Instruction *);
   bool tryReplaceContWithBra(BasicBlock *);
   void propagateJoin(BasicBlock *);

   struct TexUse
   {
      TexUse(Instruction *use, const Instruction *tex)
         : insn(use), tex(tex), level(-1) { }
      Instruction *insn;
      const Instruction *tex; // or split / mov
      int level;
   };
   struct Limits
   {
      Limits() { }
      Limits(int min, int max) : min(min), max(max) { }
      int min, max;
   };
   bool insertTextureBarriers(Function *);
   inline bool insnDominatedBy(const Instruction *, const Instruction *) const;
   void findFirstUses(const Instruction *tex, const Instruction *def,
                      std::list<TexUse>&);
   void findOverwritingDefs(const Instruction *tex, Instruction *insn,
                            const BasicBlock *term,
                            std::list<TexUse>&);
   void addTexUse(std::list<TexUse>&, Instruction *, const Instruction *);
   const Instruction *recurseDef(const Instruction *);

private:
   LValue *rZero;
   LValue *carry;
   const bool needTexBar;
};

NVC0LegalizePostRA::NVC0LegalizePostRA(const Program *prog)
   : needTexBar(prog->getTarget()->getChipset() >= 0xe0)
{
}

bool
NVC0LegalizePostRA::insnDominatedBy(const Instruction *later,
                                    const Instruction *early) const
{
   if (early->bb == later->bb)
      return early->serial < later->serial;
   return later->bb->dominatedBy(early->bb);
}

void
NVC0LegalizePostRA::addTexUse(std::list<TexUse> &uses,
                              Instruction *usei, const Instruction *insn)
{
   bool add = true;
   for (std::list<TexUse>::iterator it = uses.begin();
        it != uses.end();) {
      if (insnDominatedBy(usei, it->insn)) {
         add = false;
         break;
      }
      if (insnDominatedBy(it->insn, usei))
         it = uses.erase(it);
      else
         ++it;
   }
   if (add)
      uses.push_back(TexUse(usei, insn));
}

void
NVC0LegalizePostRA::findOverwritingDefs(const Instruction *texi,
                                        Instruction *insn,
                                        const BasicBlock *term,
                                        std::list<TexUse> &uses)
{
   while (insn->op == OP_MOV && insn->getDef(0)->equals(insn->getSrc(0)))
      insn = insn->getSrc(0)->getUniqueInsn();

   if (!insn || !insn->bb->reachableBy(texi->bb, term))
      return;

   switch (insn->op) {
   /* Values not connected to the tex's definition through any of these should
    * not be conflicting.
    */
   case OP_SPLIT:
   case OP_MERGE:
   case OP_PHI:
   case OP_UNION:
      /* recurse again */
      for (int s = 0; insn->srcExists(s); ++s)
         findOverwritingDefs(texi, insn->getSrc(s)->getUniqueInsn(), term,
                             uses);
      break;
   default:
      // if (!isTextureOp(insn->op)) // TODO: are TEXes always ordered ?
      addTexUse(uses, insn, texi);
      break;
   }
}

void
NVC0LegalizePostRA::findFirstUses(const Instruction *texi,
                                  const Instruction *insn,
                                  std::list<TexUse> &uses)
{
   for (int d = 0; insn->defExists(d); ++d) {
      Value *v = insn->getDef(d);
      for (Value::UseIterator u = v->uses.begin(); u != v->uses.end(); ++u) {
         Instruction *usei = (*u)->getInsn();

         if (usei->op == OP_PHI || usei->op == OP_UNION) {
            // need a barrier before WAW cases
            for (int s = 0; usei->srcExists(s); ++s) {
               Instruction *defi = usei->getSrc(s)->getUniqueInsn();
               if (defi && &usei->src(s) != *u)
                  findOverwritingDefs(texi, defi, usei->bb, uses);
            }
         }

         if (usei->op == OP_SPLIT ||
             usei->op == OP_MERGE ||
             usei->op == OP_PHI ||
             usei->op == OP_UNION) {
            // these uses don't manifest in the machine code
            findFirstUses(texi, usei, uses);
         } else
         if (usei->op == OP_MOV && usei->getDef(0)->equals(usei->getSrc(0)) &&
             usei->subOp != NV50_IR_SUBOP_MOV_FINAL) {
            findFirstUses(texi, usei, uses);
         } else {
            addTexUse(uses, usei, insn);
         }
      }
   }
}

// Texture barriers:
// This pass is a bit long and ugly and can probably be optimized.
//
// 1. obtain a list of TEXes and their outputs' first use(s)
// 2. calculate the barrier level of each first use (minimal number of TEXes,
//    over all paths, between the TEX and the use in question)
// 3. for each barrier, if all paths from the source TEX to that barrier
//    contain a barrier of lesser level, it can be culled
bool
NVC0LegalizePostRA::insertTextureBarriers(Function *fn)
{
   std::list<TexUse> *uses;
   std::vector<Instruction *> texes;
   std::vector<int> bbFirstTex;
   std::vector<int> bbFirstUse;
   std::vector<int> texCounts;
   std::vector<TexUse> useVec;
   ArrayList insns;

   fn->orderInstructions(insns);

   texCounts.resize(fn->allBBlocks.getSize(), 0);
   bbFirstTex.resize(fn->allBBlocks.getSize(), insns.getSize());
   bbFirstUse.resize(fn->allBBlocks.getSize(), insns.getSize());

   // tag BB CFG nodes by their id for later
   for (ArrayList::Iterator i = fn->allBBlocks.iterator(); !i.end(); i.next()) {
      BasicBlock *bb = reinterpret_cast<BasicBlock *>(i.get());
      if (bb)
         bb->cfg.tag = bb->getId();
   }

   // gather the first uses for each TEX
   for (int i = 0; i < insns.getSize(); ++i) {
      Instruction *tex = reinterpret_cast<Instruction *>(insns.get(i));
      if (isTextureOp(tex->op)) {
         texes.push_back(tex);
         if (!texCounts.at(tex->bb->getId()))
            bbFirstTex[tex->bb->getId()] = texes.size() - 1;
         texCounts[tex->bb->getId()]++;
      }
   }
   insns.clear();
   if (texes.empty())
      return false;
   uses = new std::list<TexUse>[texes.size()];
   if (!uses)
      return false;
   for (size_t i = 0; i < texes.size(); ++i)
      findFirstUses(texes[i], texes[i], uses[i]);

   // determine the barrier level at each use
   for (size_t i = 0; i < texes.size(); ++i) {
      for (std::list<TexUse>::iterator u = uses[i].begin(); u != uses[i].end();
           ++u) {
         BasicBlock *tb = texes[i]->bb;
         BasicBlock *ub = u->insn->bb;
         if (tb == ub) {
            u->level = 0;
            for (size_t j = i + 1; j < texes.size() &&
                    texes[j]->bb == tb && texes[j]->serial < u->insn->serial;
                 ++j)
               u->level++;
         } else {
            u->level = fn->cfg.findLightestPathWeight(&tb->cfg,
                                                      &ub->cfg, texCounts);
            if (u->level < 0) {
               WARN("Failed to find path TEX -> TEXBAR\n");
               u->level = 0;
               continue;
            }
            // this counted all TEXes in the origin block, correct that
            u->level -= i - bbFirstTex.at(tb->getId()) + 1 /* this TEX */;
            // and did not count the TEXes in the destination block, add those
            for (size_t j = bbFirstTex.at(ub->getId()); j < texes.size() &&
                    texes[j]->bb == ub && texes[j]->serial < u->insn->serial;
                 ++j)
               u->level++;
         }
         assert(u->level >= 0);
         useVec.push_back(*u);
      }
   }
   delete[] uses;
   uses = NULL;

   // insert the barriers
   for (size_t i = 0; i < useVec.size(); ++i) {
      Instruction *prev = useVec[i].insn->prev;
      if (useVec[i].level < 0)
         continue;
      if (prev && prev->op == OP_TEXBAR) {
         if (prev->subOp > useVec[i].level)
            prev->subOp = useVec[i].level;
         prev->setSrc(prev->srcCount(), useVec[i].tex->getDef(0));
      } else {
         Instruction *bar = new_Instruction(func, OP_TEXBAR, TYPE_NONE);
         bar->fixed = 1;
         bar->subOp = useVec[i].level;
         // make use explicit to ease latency calculation
         bar->setSrc(bar->srcCount(), useVec[i].tex->getDef(0));
         useVec[i].insn->bb->insertBefore(useVec[i].insn, bar);
      }
   }

   if (fn->getProgram()->optLevel < 3) {
      if (uses)
         delete[] uses;
      return true;
   }

   std::vector<Limits> limitT, limitB, limitS; // entry, exit, single

   limitT.resize(fn->allBBlocks.getSize(), Limits(0, 0));
   limitB.resize(fn->allBBlocks.getSize(), Limits(0, 0));
   limitS.resize(fn->allBBlocks.getSize());

   // cull unneeded barriers (should do that earlier, but for simplicity)
   IteratorRef bi = fn->cfg.iteratorCFG();
   // first calculate min/max outstanding TEXes for each BB
   for (bi->reset(); !bi->end(); bi->next()) {
      Graph::Node *n = reinterpret_cast<Graph::Node *>(bi->get());
      BasicBlock *bb = BasicBlock::get(n);
      int min = 0;
      int max = std::numeric_limits<int>::max();
      for (Instruction *i = bb->getFirst(); i; i = i->next) {
         if (isTextureOp(i->op)) {
            min++;
            if (max < std::numeric_limits<int>::max())
               max++;
         } else
         if (i->op == OP_TEXBAR) {
            min = MIN2(min, i->subOp);
            max = MIN2(max, i->subOp);
         }
      }
      // limits when looking at an isolated block
      limitS[bb->getId()].min = min;
      limitS[bb->getId()].max = max;
   }
   // propagate the min/max values
   for (unsigned int l = 0; l <= fn->loopNestingBound; ++l) {
      for (bi->reset(); !bi->end(); bi->next()) {
         Graph::Node *n = reinterpret_cast<Graph::Node *>(bi->get());
         BasicBlock *bb = BasicBlock::get(n);
         const int bbId = bb->getId();
         for (Graph::EdgeIterator ei = n->incident(); !ei.end(); ei.next()) {
            BasicBlock *in = BasicBlock::get(ei.getNode());
            const int inId = in->getId();
            limitT[bbId].min = MAX2(limitT[bbId].min, limitB[inId].min);
            limitT[bbId].max = MAX2(limitT[bbId].max, limitB[inId].max);
         }
         // I just hope this is correct ...
         if (limitS[bbId].max == std::numeric_limits<int>::max()) {
            // no barrier
            limitB[bbId].min = limitT[bbId].min + limitS[bbId].min;
            limitB[bbId].max = limitT[bbId].max + limitS[bbId].min;
         } else {
            // block contained a barrier
            limitB[bbId].min = MIN2(limitS[bbId].max,
                                    limitT[bbId].min + limitS[bbId].min);
            limitB[bbId].max = MIN2(limitS[bbId].max,
                                    limitT[bbId].max + limitS[bbId].min);
         }
      }
   }
   // finally delete unnecessary barriers
   for (bi->reset(); !bi->end(); bi->next()) {
      Graph::Node *n = reinterpret_cast<Graph::Node *>(bi->get());
      BasicBlock *bb = BasicBlock::get(n);
      Instruction *prev = NULL;
      Instruction *next;
      int max = limitT[bb->getId()].max;
      for (Instruction *i = bb->getFirst(); i; i = next) {
         next = i->next;
         if (i->op == OP_TEXBAR) {
            if (i->subOp >= max) {
               delete_Instruction(prog, i);
            } else {
               max = i->subOp;
               if (prev && prev->op == OP_TEXBAR && prev->subOp >= max) {
                  delete_Instruction(prog, prev);
                  prev = NULL;
               }
            }
         } else
         if (isTextureOp(i->op)) {
            max++;
         }
         if (!i->isNop())
            prev = i;
      }
   }
   if (uses)
      delete[] uses;
   return true;
}

bool
NVC0LegalizePostRA::visit(Function *fn)
{
   if (needTexBar)
      insertTextureBarriers(fn);

   rZero = new_LValue(fn, FILE_GPR);
   carry = new_LValue(fn, FILE_FLAGS);

   rZero->reg.data.id = prog->getTarget()->getFileSize(FILE_GPR);
   carry->reg.data.id = 0;

   return true;
}

void
NVC0LegalizePostRA::replaceZero(Instruction *i)
{
   for (int s = 0; i->srcExists(s); ++s) {
      if (s == 2 && i->op == OP_SUCLAMP)
         continue;
      ImmediateValue *imm = i->getSrc(s)->asImm();
      if (imm && imm->reg.data.u64 == 0)
         i->setSrc(s, rZero);
   }
}

// replace CONT with BRA for single unconditional continue
bool
NVC0LegalizePostRA::tryReplaceContWithBra(BasicBlock *bb)
{
   if (bb->cfg.incidentCount() != 2 || bb->getEntry()->op != OP_PRECONT)
      return false;
   Graph::EdgeIterator ei = bb->cfg.incident();
   if (ei.getType() != Graph::Edge::BACK)
      ei.next();
   if (ei.getType() != Graph::Edge::BACK)
      return false;
   BasicBlock *contBB = BasicBlock::get(ei.getNode());

   if (!contBB->getExit() || contBB->getExit()->op != OP_CONT ||
       contBB->getExit()->getPredicate())
      return false;
   contBB->getExit()->op = OP_BRA;
   bb->remove(bb->getEntry()); // delete PRECONT

   ei.next();
   assert(ei.end() || ei.getType() != Graph::Edge::BACK);
   return true;
}

// replace branches to join blocks with join ops
void
NVC0LegalizePostRA::propagateJoin(BasicBlock *bb)
{
   if (bb->getEntry()->op != OP_JOIN || bb->getEntry()->asFlow()->limit)
      return;
   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      BasicBlock *in = BasicBlock::get(ei.getNode());
      Instruction *exit = in->getExit();
      if (!exit) {
         in->insertTail(new FlowInstruction(func, OP_JOIN, bb));
         // there should always be a terminator instruction
         WARN("inserted missing terminator in BB:%i\n", in->getId());
      } else
      if (exit->op == OP_BRA) {
         exit->op = OP_JOIN;
         exit->asFlow()->limit = 1; // must-not-propagate marker
      }
   }
   bb->remove(bb->getEntry());
}

bool
NVC0LegalizePostRA::visit(BasicBlock *bb)
{
   Instruction *i, *next;

   // remove pseudo operations and non-fixed no-ops, split 64 bit operations
   for (i = bb->getFirst(); i; i = next) {
      next = i->next;
      if (i->op == OP_EMIT || i->op == OP_RESTART) {
         if (!i->getDef(0)->refCount())
            i->setDef(0, NULL);
         if (i->src(0).getFile() == FILE_IMMEDIATE)
            i->setSrc(0, rZero); // initial value must be 0
      } else
      if (i->isNop()) {
         bb->remove(i);
      } else {
         // TODO: Move this to before register allocation for operations that
         // need the $c register !
         if (typeSizeof(i->dType) == 8) {
            Instruction *hi;
            hi = BuildUtil::split64BitOpPostRA(func, i, rZero, carry);
            if (hi)
               next = hi;
         }

         if (i->op != OP_MOV && i->op != OP_PFETCH)
            replaceZero(i);
      }
   }
   if (!bb->getEntry())
      return true;

   if (!tryReplaceContWithBra(bb))
      propagateJoin(bb);

   return true;
}

class NVC0LoweringPass : public Pass
{
public:
   NVC0LoweringPass(Program *);

private:
   virtual bool visit(Function *);
   virtual bool visit(BasicBlock *);
   virtual bool visit(Instruction *);

   bool handleRDSV(Instruction *);
   bool handleWRSV(Instruction *);
   bool handleEXPORT(Instruction *);
   bool handleOUT(Instruction *);
   bool handleDIV(Instruction *);
   bool handleMOD(Instruction *);
   bool handleSQRT(Instruction *);
   bool handlePOW(Instruction *);
   bool handleTEX(TexInstruction *);
   bool handleTXD(TexInstruction *);
   bool handleTXQ(TexInstruction *);
   bool handleManualTXD(TexInstruction *);
   bool handleATOM(Instruction *);
   bool handleCasExch(Instruction *, bool needCctl);
   void handleSurfaceOpNVE4(TexInstruction *);

   void checkPredicate(Instruction *);

   void readTessCoord(LValue *dst, int c);

   Value *loadResInfo32(Value *ptr, uint32_t off);
   Value *loadMsInfo32(Value *ptr, uint32_t off);
   Value *loadTexHandle(Value *ptr, unsigned int slot);

   void adjustCoordinatesMS(TexInstruction *);
   void processSurfaceCoordsNVE4(TexInstruction *);

private:
   const Target *const targ;

   BuildUtil bld;

   Symbol *gMemBase;
   LValue *gpEmitAddress;
};

NVC0LoweringPass::NVC0LoweringPass(Program *prog) : targ(prog->getTarget())
{
   bld.setProgram(prog);
   gMemBase = NULL;
}

bool
NVC0LoweringPass::visit(Function *fn)
{
   if (prog->getType() == Program::TYPE_GEOMETRY) {
      assert(!strncmp(fn->getName(), "MAIN", 4));
      // TODO: when we generate actual functions pass this value along somehow
      bld.setPosition(BasicBlock::get(fn->cfg.getRoot()), false);
      gpEmitAddress = bld.loadImm(NULL, 0)->asLValue();
      if (fn->cfgExit) {
         bld.setPosition(BasicBlock::get(fn->cfgExit)->getExit(), false);
         bld.mkMovToReg(0, gpEmitAddress);
      }
   }
   return true;
}

bool
NVC0LoweringPass::visit(BasicBlock *bb)
{
   return true;
}

inline Value *
NVC0LoweringPass::loadTexHandle(Value *ptr, unsigned int slot)
{
   uint8_t b = prog->driver->io.resInfoCBSlot;
   uint32_t off = prog->driver->io.texBindBase + slot * 4;
   return bld.
      mkLoadv(TYPE_U32, bld.mkSymbol(FILE_MEMORY_CONST, b, TYPE_U32, off), ptr);
}

// move array source to first slot, convert to u16, add indirections
bool
NVC0LoweringPass::handleTEX(TexInstruction *i)
{
   const int dim = i->tex.target.getDim() + i->tex.target.isCube();
   const int arg = i->tex.target.getArgCount();

   if (prog->getTarget()->getChipset() >= NVISA_GK104_CHIPSET) {
      if (i->tex.rIndirectSrc >= 0 || i->tex.sIndirectSrc >= 0) {
         WARN("indirect TEX not implemented\n");
      }
      if (i->tex.r == i->tex.s) {
         i->tex.r += prog->driver->io.texBindBase / 4;
         i->tex.s  = 0; // only a single cX[] value possible here
      } else {
         Value *hnd = bld.getScratch();
         Value *rHnd = loadTexHandle(NULL, i->tex.r);
         Value *sHnd = loadTexHandle(NULL, i->tex.s);

         bld.mkOp3(OP_INSBF, TYPE_U32, hnd, rHnd, bld.mkImm(0x1400), sHnd);

         i->tex.r = 0; // not used for indirect tex
         i->tex.s = 0;
         i->setIndirectR(hnd);
      }
      if (i->tex.target.isArray()) {
         LValue *layer = new_LValue(func, FILE_GPR);
         Value *src = i->getSrc(arg - 1);
         const int sat = (i->op == OP_TXF) ? 1 : 0;
         DataType sTy = (i->op == OP_TXF) ? TYPE_U32 : TYPE_F32;
         bld.mkCvt(OP_CVT, TYPE_U16, layer, sTy, src)->saturate = sat;
         for (int s = dim; s >= 1; --s)
            i->setSrc(s, i->getSrc(s - 1));
         i->setSrc(0, layer);
      }
   } else
   // (nvc0) generate and move the tsc/tic/array source to the front
   if (dim != arg || i->tex.rIndirectSrc >= 0 || i->tex.sIndirectSrc >= 0) {
      LValue *src = new_LValue(func, FILE_GPR); // 0xttxsaaaa

      Value *arrayIndex = i->tex.target.isArray() ? i->getSrc(arg - 1) : NULL;
      for (int s = dim; s >= 1; --s)
         i->setSrc(s, i->getSrc(s - 1));
      i->setSrc(0, arrayIndex);

      Value *ticRel = i->getIndirectR();
      Value *tscRel = i->getIndirectS();

      if (arrayIndex) {
         int sat = (i->op == OP_TXF) ? 1 : 0;
         DataType sTy = (i->op == OP_TXF) ? TYPE_U32 : TYPE_F32;
         bld.mkCvt(OP_CVT, TYPE_U16, src, sTy, arrayIndex)->saturate = sat;
      } else {
         bld.loadImm(src, 0);
      }

      if (ticRel) {
         i->setSrc(i->tex.rIndirectSrc, NULL);
         bld.mkOp3(OP_INSBF, TYPE_U32, src, ticRel, bld.mkImm(0x0917), src);
      }
      if (tscRel) {
         i->setSrc(i->tex.sIndirectSrc, NULL);
         bld.mkOp3(OP_INSBF, TYPE_U32, src, tscRel, bld.mkImm(0x0710), src);
      }

      i->setSrc(0, src);
   }

   // offset is last source (lod 1st, dc 2nd)
   if (i->tex.useOffsets) {
      uint32_t value = 0;
      int n, c;
      int s = i->srcCount(0xff);
      for (n = 0; n < i->tex.useOffsets; ++n)
         for (c = 0; c < 3; ++c)
            value |= (i->tex.offset[n][c] & 0xf) << (n * 12 + c * 4);
      i->setSrc(s, bld.loadImm(NULL, value));
   }

   return true;
}

bool
NVC0LoweringPass::handleManualTXD(TexInstruction *i)
{
   static const uint8_t qOps[4][2] =
   {
      { QUADOP(MOV2, ADD,  MOV2, ADD),  QUADOP(MOV2, MOV2, ADD,  ADD) }, // l0
      { QUADOP(SUBR, MOV2, SUBR, MOV2), QUADOP(MOV2, MOV2, ADD,  ADD) }, // l1
      { QUADOP(MOV2, ADD,  MOV2, ADD),  QUADOP(SUBR, SUBR, MOV2, MOV2) }, // l2
      { QUADOP(SUBR, MOV2, SUBR, MOV2), QUADOP(SUBR, SUBR, MOV2, MOV2) }, // l3
   };
   Value *def[4][4];
   Value *crd[3];
   Instruction *tex;
   Value *zero = bld.loadImm(bld.getSSA(), 0);
   int l, c;
   const int dim = i->tex.target.getDim();

   i->op = OP_TEX; // no need to clone dPdx/dPdy later

   for (c = 0; c < dim; ++c)
      crd[c] = bld.getScratch();

   bld.mkOp(OP_QUADON, TYPE_NONE, NULL);
   for (l = 0; l < 4; ++l) {
      // mov coordinates from lane l to all lanes
      for (c = 0; c < dim; ++c)
         bld.mkQuadop(0x00, crd[c], l, i->getSrc(c), zero);
      // add dPdx from lane l to lanes dx
      for (c = 0; c < dim; ++c)
         bld.mkQuadop(qOps[l][0], crd[c], l, i->dPdx[c].get(), crd[c]);
      // add dPdy from lane l to lanes dy
      for (c = 0; c < dim; ++c)
         bld.mkQuadop(qOps[l][1], crd[c], l, i->dPdy[c].get(), crd[c]);
      // texture
      bld.insert(tex = cloneForward(func, i));
      for (c = 0; c < dim; ++c)
         tex->setSrc(c, crd[c]);
      // save results
      for (c = 0; i->defExists(c); ++c) {
         Instruction *mov;
         def[c][l] = bld.getSSA();
         mov = bld.mkMov(def[c][l], tex->getDef(c));
         mov->fixed = 1;
         mov->lanes = 1 << l;
      }
   }
   bld.mkOp(OP_QUADPOP, TYPE_NONE, NULL);

   for (c = 0; i->defExists(c); ++c) {
      Instruction *u = bld.mkOp(OP_UNION, TYPE_U32, i->getDef(c));
      for (l = 0; l < 4; ++l)
         u->setSrc(l, def[c][l]);
   }

   i->bb->remove(i);
   return true;
}

bool
NVC0LoweringPass::handleTXD(TexInstruction *txd)
{
   int dim = txd->tex.target.getDim();
   int arg = txd->tex.target.getArgCount();

   handleTEX(txd);
   while (txd->srcExists(arg))
      ++arg;

   txd->tex.derivAll = true;
   if (dim > 2 ||
       txd->tex.target.isCube() ||
       arg > 4 ||
       txd->tex.target.isShadow())
      return handleManualTXD(txd);

   for (int c = 0; c < dim; ++c) {
      txd->setSrc(arg + c * 2 + 0, txd->dPdx[c]);
      txd->setSrc(arg + c * 2 + 1, txd->dPdy[c]);
      txd->dPdx[c].set(NULL);
      txd->dPdy[c].set(NULL);
   }
   return true;
}

bool
NVC0LoweringPass::handleTXQ(TexInstruction *txq)
{
   // TODO: indirect resource/sampler index
   return true;
}

bool
NVC0LoweringPass::handleATOM(Instruction *atom)
{
   SVSemantic sv;

   switch (atom->src(0).getFile()) {
   case FILE_MEMORY_LOCAL:
      sv = SV_LBASE;
      break;
   case FILE_MEMORY_SHARED:
      sv = SV_SBASE;
      break;
   default:
      assert(atom->src(0).getFile() == FILE_MEMORY_GLOBAL);
      return true;
   }
   Value *base =
      bld.mkOp1v(OP_RDSV, TYPE_U32, bld.getScratch(), bld.mkSysVal(sv, 0));
   Value *ptr = atom->getIndirect(0, 0);

   atom->setSrc(0, cloneShallow(func, atom->getSrc(0)));
   atom->getSrc(0)->reg.file = FILE_MEMORY_GLOBAL;
   if (ptr)
      base = bld.mkOp2v(OP_ADD, TYPE_U32, base, base, ptr);
   atom->setIndirect(0, 0, base);

   return true;
}

bool
NVC0LoweringPass::handleCasExch(Instruction *cas, bool needCctl)
{
   if (cas->subOp != NV50_IR_SUBOP_ATOM_CAS &&
       cas->subOp != NV50_IR_SUBOP_ATOM_EXCH)
      return false;
   bld.setPosition(cas, true);

   if (needCctl) {
      Instruction *cctl = bld.mkOp1(OP_CCTL, TYPE_NONE, NULL, cas->getSrc(0));
      cctl->setIndirect(0, 0, cas->getIndirect(0, 0));
      cctl->fixed = 1;
      cctl->subOp = NV50_IR_SUBOP_CCTL_IV;
      if (cas->isPredicated())
         cctl->setPredicate(cas->cc, cas->getPredicate());
   }

   if (cas->defExists(0) && cas->subOp == NV50_IR_SUBOP_ATOM_CAS) {
      // CAS is crazy. It's 2nd source is a double reg, and the 3rd source
      // should be set to the high part of the double reg or bad things will
      // happen elsewhere in the universe.
      // Also, it sometimes returns the new value instead of the old one
      // under mysterious circumstances.
      Value *dreg = bld.getSSA(8);
      bld.setPosition(cas, false);
      bld.mkOp2(OP_MERGE, TYPE_U64, dreg, cas->getSrc(1), cas->getSrc(2));
      cas->setSrc(1, dreg);
   }

   return true;
}

inline Value *
NVC0LoweringPass::loadResInfo32(Value *ptr, uint32_t off)
{
   uint8_t b = prog->driver->io.resInfoCBSlot;
   off += prog->driver->io.suInfoBase;
   return bld.
      mkLoadv(TYPE_U32, bld.mkSymbol(FILE_MEMORY_CONST, b, TYPE_U32, off), ptr);
}

inline Value *
NVC0LoweringPass::loadMsInfo32(Value *ptr, uint32_t off)
{
   uint8_t b = prog->driver->io.msInfoCBSlot;
   off += prog->driver->io.msInfoBase;
   return bld.
      mkLoadv(TYPE_U32, bld.mkSymbol(FILE_MEMORY_CONST, b, TYPE_U32, off), ptr);
}

/* On nvc0, surface info is obtained via the surface binding points passed
 * to the SULD/SUST instructions.
 * On nve4, surface info is stored in c[] and is used by various special
 * instructions, e.g. for clamping coordiantes or generating an address.
 * They couldn't just have added an equivalent to TIC now, couldn't they ?
 */
#define NVE4_SU_INFO_ADDR   0x00
#define NVE4_SU_INFO_FMT    0x04
#define NVE4_SU_INFO_DIM_X  0x08
#define NVE4_SU_INFO_PITCH  0x0c
#define NVE4_SU_INFO_DIM_Y  0x10
#define NVE4_SU_INFO_ARRAY  0x14
#define NVE4_SU_INFO_DIM_Z  0x18
#define NVE4_SU_INFO_UNK1C  0x1c
#define NVE4_SU_INFO_WIDTH  0x20
#define NVE4_SU_INFO_HEIGHT 0x24
#define NVE4_SU_INFO_DEPTH  0x28
#define NVE4_SU_INFO_TARGET 0x2c
#define NVE4_SU_INFO_CALL   0x30
#define NVE4_SU_INFO_RAW_X  0x34
#define NVE4_SU_INFO_MS_X   0x38
#define NVE4_SU_INFO_MS_Y   0x3c

#define NVE4_SU_INFO__STRIDE 0x40

#define NVE4_SU_INFO_DIM(i)  (0x08 + (i) * 8)
#define NVE4_SU_INFO_SIZE(i) (0x20 + (i) * 4)
#define NVE4_SU_INFO_MS(i)   (0x38 + (i) * 4)

static inline uint16_t getSuClampSubOp(const TexInstruction *su, int c)
{
   switch (su->tex.target.getEnum()) {
   case TEX_TARGET_BUFFER:      return NV50_IR_SUBOP_SUCLAMP_PL(0, 1);
   case TEX_TARGET_RECT:        return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_1D:          return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_1D_ARRAY:    return (c == 1) ?
                                   NV50_IR_SUBOP_SUCLAMP_PL(0, 2) :
                                   NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_2D:          return NV50_IR_SUBOP_SUCLAMP_BL(0, 2);
   case TEX_TARGET_2D_MS:       return NV50_IR_SUBOP_SUCLAMP_BL(0, 2);
   case TEX_TARGET_2D_ARRAY:    return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_2D_MS_ARRAY: return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_3D:          return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_CUBE:        return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   case TEX_TARGET_CUBE_ARRAY:  return NV50_IR_SUBOP_SUCLAMP_SD(0, 2);
   default:
      assert(0);
      return 0;
   }
}

void
NVC0LoweringPass::adjustCoordinatesMS(TexInstruction *tex)
{
   const uint16_t base = tex->tex.r * NVE4_SU_INFO__STRIDE;
   const int arg = tex->tex.target.getArgCount();

   if (tex->tex.target == TEX_TARGET_2D_MS)
      tex->tex.target = TEX_TARGET_2D;
   else
   if (tex->tex.target == TEX_TARGET_2D_MS_ARRAY)
      tex->tex.target = TEX_TARGET_2D_ARRAY;
   else
      return;

   Value *x = tex->getSrc(0);
   Value *y = tex->getSrc(1);
   Value *s = tex->getSrc(arg - 1);

   Value *tx = bld.getSSA(), *ty = bld.getSSA(), *ts = bld.getSSA();

   Value *ms_x = loadResInfo32(NULL, base + NVE4_SU_INFO_MS(0));
   Value *ms_y = loadResInfo32(NULL, base + NVE4_SU_INFO_MS(1));

   bld.mkOp2(OP_SHL, TYPE_U32, tx, x, ms_x);
   bld.mkOp2(OP_SHL, TYPE_U32, ty, y, ms_y);

   s = bld.mkOp2v(OP_AND, TYPE_U32, ts, s, bld.loadImm(NULL, 0x7));
   s = bld.mkOp2v(OP_SHL, TYPE_U32, ts, ts, bld.mkImm(3));

   Value *dx = loadMsInfo32(ts, 0x0);
   Value *dy = loadMsInfo32(ts, 0x4);

   bld.mkOp2(OP_ADD, TYPE_U32, tx, tx, dx);
   bld.mkOp2(OP_ADD, TYPE_U32, ty, ty, dy);

   tex->setSrc(0, tx);
   tex->setSrc(1, ty);
   tex->moveSources(arg, -1);
}

// Sets 64-bit "generic address", predicate and format sources for SULD/SUST.
// They're computed from the coordinates using the surface info in c[] space.
void
NVC0LoweringPass::processSurfaceCoordsNVE4(TexInstruction *su)
{
   Instruction *insn;
   const bool atom = su->op == OP_SUREDB || su->op == OP_SUREDP;
   const bool raw =
      su->op == OP_SULDB || su->op == OP_SUSTB || su->op == OP_SUREDB;
   const int idx = su->tex.r;
   const int dim = su->tex.target.getDim();
   const int arg = dim + (su->tex.target.isArray() ? 1 : 0);
   const uint16_t base = idx * NVE4_SU_INFO__STRIDE;
   int c;
   Value *zero = bld.mkImm(0);
   Value *p1 = NULL;
   Value *v;
   Value *src[3];
   Value *bf, *eau, *off;
   Value *addr, *pred;

   off = bld.getScratch(4);
   bf = bld.getScratch(4);
   addr = bld.getSSA(8);
   pred = bld.getScratch(1, FILE_PREDICATE);

   bld.setPosition(su, false);

   adjustCoordinatesMS(su);

   // calculate clamped coordinates
   for (c = 0; c < arg; ++c) {
      src[c] = bld.getScratch();
      if (c == 0 && raw)
         v = loadResInfo32(NULL, base + NVE4_SU_INFO_RAW_X);
      else
         v = loadResInfo32(NULL, base + NVE4_SU_INFO_DIM(c));
      bld.mkOp3(OP_SUCLAMP, TYPE_S32, src[c], su->getSrc(c), v, zero)
         ->subOp = getSuClampSubOp(su, c);
   }
   for (; c < 3; ++c)
      src[c] = zero;

   // set predicate output
   if (su->tex.target == TEX_TARGET_BUFFER) {
      src[0]->getInsn()->setFlagsDef(1, pred);
   } else
   if (su->tex.target.isArray()) {
      p1 = bld.getSSA(1, FILE_PREDICATE);
      src[dim]->getInsn()->setFlagsDef(1, p1);
   }

   // calculate pixel offset
   if (dim == 1) {
      if (su->tex.target != TEX_TARGET_BUFFER)
         bld.mkOp2(OP_AND, TYPE_U32, off, src[0], bld.loadImm(NULL, 0xffff));
   } else
   if (dim == 3) {
      v = loadResInfo32(NULL, base + NVE4_SU_INFO_UNK1C);
      bld.mkOp3(OP_MADSP, TYPE_U32, off, src[2], v, src[1])
         ->subOp = NV50_IR_SUBOP_MADSP(4,2,8); // u16l u16l u16l

      v = loadResInfo32(NULL, base + NVE4_SU_INFO_PITCH);
      bld.mkOp3(OP_MADSP, TYPE_U32, off, off, v, src[0])
         ->subOp = NV50_IR_SUBOP_MADSP(0,2,8); // u32 u16l u16l
   } else {
      assert(dim == 2);
      v = loadResInfo32(NULL, base + NVE4_SU_INFO_PITCH);
      bld.mkOp3(OP_MADSP, TYPE_U32, off, src[1], v, src[0])
         ->subOp = su->tex.target.isArray() ?
         NV50_IR_SUBOP_MADSP_SD : NV50_IR_SUBOP_MADSP(4,2,8); // u16l u16l u16l
   }

   // calculate effective address part 1
   if (su->tex.target == TEX_TARGET_BUFFER) {
      if (raw) {
         bf = src[0];
      } else {
         v = loadResInfo32(NULL, base + NVE4_SU_INFO_FMT);
         bld.mkOp3(OP_VSHL, TYPE_U32, bf, src[0], v, zero)
            ->subOp = NV50_IR_SUBOP_V1(7,6,8|2);
      }
   } else {
      Value *y = src[1];
      Value *z = src[2];
      uint16_t subOp = 0;

      switch (dim) {
      case 1:
         y = zero;
         z = zero;
         break;
      case 2:
         z = off;
         if (!su->tex.target.isArray()) {
            z = loadResInfo32(NULL, base + NVE4_SU_INFO_UNK1C);
            subOp = NV50_IR_SUBOP_SUBFM_3D;
         }
         break;
      default:
         subOp = NV50_IR_SUBOP_SUBFM_3D;
         assert(dim == 3);
         break;
      }
      insn = bld.mkOp3(OP_SUBFM, TYPE_U32, bf, src[0], y, z);
      insn->subOp = subOp;
      insn->setFlagsDef(1, pred);
   }

   // part 2
   v = loadResInfo32(NULL, base + NVE4_SU_INFO_ADDR);

   if (su->tex.target == TEX_TARGET_BUFFER) {
      eau = v;
   } else {
      eau = bld.mkOp3v(OP_SUEAU, TYPE_U32, bld.getScratch(4), off, bf, v);
   }
   // add array layer offset
   if (su->tex.target.isArray()) {
      v = loadResInfo32(NULL, base + NVE4_SU_INFO_ARRAY);
      if (dim == 1)
         bld.mkOp3(OP_MADSP, TYPE_U32, eau, src[1], v, eau)
            ->subOp = NV50_IR_SUBOP_MADSP(4,0,0); // u16 u24 u32
      else
         bld.mkOp3(OP_MADSP, TYPE_U32, eau, v, src[2], eau)
            ->subOp = NV50_IR_SUBOP_MADSP(0,0,0); // u32 u24 u32
      // combine predicates
      assert(p1);
      bld.mkOp2(OP_OR, TYPE_U8, pred, pred, p1);
   }

   if (atom) {
      Value *lo = bf;
      if (su->tex.target == TEX_TARGET_BUFFER) {
         lo = zero;
         bld.mkMov(off, bf);
      }
      //  bf == g[] address & 0xff
      // eau == g[] address >> 8
      bld.mkOp3(OP_PERMT, TYPE_U32,  bf,   lo, bld.loadImm(NULL, 0x6540), eau);
      bld.mkOp3(OP_PERMT, TYPE_U32, eau, zero, bld.loadImm(NULL, 0x0007), eau);
   } else
   if (su->op == OP_SULDP && su->tex.target == TEX_TARGET_BUFFER) {
      // Convert from u32 to u8 address format, which is what the library code
      // doing SULDP currently uses.
      // XXX: can SUEAU do this ?
      // XXX: does it matter that we don't mask high bytes in bf ?
      // Grrr.
      bld.mkOp2(OP_SHR, TYPE_U32, off, bf, bld.mkImm(8));
      bld.mkOp2(OP_ADD, TYPE_U32, eau, eau, off);
   }

   bld.mkOp2(OP_MERGE, TYPE_U64, addr, bf, eau);

   if (atom && su->tex.target == TEX_TARGET_BUFFER)
      bld.mkOp2(OP_ADD, TYPE_U64, addr, addr, off);

   // let's just set it 0 for raw access and hope it works
   v = raw ?
      bld.mkImm(0) : loadResInfo32(NULL, base + NVE4_SU_INFO_FMT);

   // get rid of old coordinate sources, make space for fmt info and predicate
   su->moveSources(arg, 3 - arg);
   // set 64 bit address and 32-bit format sources
   su->setSrc(0, addr);
   su->setSrc(1, v);
   su->setSrc(2, pred);
}

void
NVC0LoweringPass::handleSurfaceOpNVE4(TexInstruction *su)
{
   processSurfaceCoordsNVE4(su);

   // Who do we hate more ? The person who decided that nvc0's SULD doesn't
   // have to support conversion or the person who decided that, in OpenCL,
   // you don't have to specify the format here like you do in OpenGL ?

   if (su->op == OP_SULDP) {
      // We don't patch shaders. Ever.
      // You get an indirect call to our library blob here.
      // But at least it's uniform.
      FlowInstruction *call;
      LValue *p[3];
      LValue *r[5];
      uint16_t base = su->tex.r * NVE4_SU_INFO__STRIDE + NVE4_SU_INFO_CALL;

      for (int i = 0; i < 4; ++i)
         (r[i] = bld.getScratch(4, FILE_GPR))->reg.data.id = i;
      for (int i = 0; i < 3; ++i)
         (p[i] = bld.getScratch(1, FILE_PREDICATE))->reg.data.id = i;
      (r[4] = bld.getScratch(8, FILE_GPR))->reg.data.id = 4;

      bld.mkMov(p[1], bld.mkImm((su->cache == CACHE_CA) ? 1 : 0), TYPE_U8);
      bld.mkMov(p[2], bld.mkImm((su->cache == CACHE_CG) ? 1 : 0), TYPE_U8);
      bld.mkMov(p[0], su->getSrc(2), TYPE_U8);
      bld.mkMov(r[4], su->getSrc(0), TYPE_U64);
      bld.mkMov(r[2], su->getSrc(1), TYPE_U32);

      call = bld.mkFlow(OP_CALL, NULL, su->cc, su->getPredicate());

      call->indirect = 1;
      call->absolute = 1;
      call->setSrc(0, bld.mkSymbol(FILE_MEMORY_CONST,
                                   prog->driver->io.resInfoCBSlot, TYPE_U32,
                                   prog->driver->io.suInfoBase + base));
      call->setSrc(1, r[2]);
      call->setSrc(2, r[4]);
      for (int i = 0; i < 3; ++i)
         call->setSrc(3 + i, p[i]);
      for (int i = 0; i < 4; ++i) {
         call->setDef(i, r[i]);
         bld.mkMov(su->getDef(i), r[i]);
      }
      call->setDef(4, p[1]);
      delete_Instruction(bld.getProgram(), su);
   }

   if (su->op == OP_SUREDB || su->op == OP_SUREDP) {
      // FIXME: for out of bounds access, destination value will be undefined !
      Value *pred = su->getSrc(2);
      CondCode cc = CC_NOT_P;
      if (su->getPredicate()) {
         pred = bld.getScratch(1, FILE_PREDICATE);
         cc = su->cc;
         if (cc == CC_NOT_P) {
            bld.mkOp2(OP_OR, TYPE_U8, pred, su->getPredicate(), su->getSrc(2));
         } else {
            bld.mkOp2(OP_AND, TYPE_U8, pred, su->getPredicate(), su->getSrc(2));
            pred->getInsn()->src(1).mod = Modifier(NV50_IR_MOD_NOT);
         }
      }
      Instruction *red = bld.mkOp(OP_ATOM, su->dType, su->getDef(0));
      red->subOp = su->subOp;
      if (!gMemBase)
         gMemBase = bld.mkSymbol(FILE_MEMORY_GLOBAL, 0, TYPE_U32, 0);
      red->setSrc(0, gMemBase);
      red->setSrc(1, su->getSrc(3));
      if (su->subOp == NV50_IR_SUBOP_ATOM_CAS)
         red->setSrc(2, su->getSrc(4));
      red->setIndirect(0, 0, su->getSrc(0));
      red->setPredicate(cc, pred);
      delete_Instruction(bld.getProgram(), su);
      handleCasExch(red, true);
   } else {
      su->sType = (su->tex.target == TEX_TARGET_BUFFER) ? TYPE_U32 : TYPE_U8;
   }
}

bool
NVC0LoweringPass::handleWRSV(Instruction *i)
{
   Instruction *st;
   Symbol *sym;
   uint32_t addr;

   // must replace, $sreg are not writeable
   addr = targ->getSVAddress(FILE_SHADER_OUTPUT, i->getSrc(0)->asSym());
   if (addr >= 0x400)
      return false;
   sym = bld.mkSymbol(FILE_SHADER_OUTPUT, 0, i->sType, addr);

   st = bld.mkStore(OP_EXPORT, i->dType, sym, i->getIndirect(0, 0),
                    i->getSrc(1));
   st->perPatch = i->perPatch;

   bld.getBB()->remove(i);
   return true;
}

void
NVC0LoweringPass::readTessCoord(LValue *dst, int c)
{
   Value *laneid = bld.getSSA();
   Value *x, *y;

   bld.mkOp1(OP_RDSV, TYPE_U32, laneid, bld.mkSysVal(SV_LANEID, 0));

   if (c == 0) {
      x = dst;
      y = NULL;
   } else
   if (c == 1) {
      x = NULL;
      y = dst;
   } else {
      assert(c == 2);
      x = bld.getSSA();
      y = bld.getSSA();
   }
   if (x)
      bld.mkFetch(x, TYPE_F32, FILE_SHADER_OUTPUT, 0x2f0, NULL, laneid);
   if (y)
      bld.mkFetch(y, TYPE_F32, FILE_SHADER_OUTPUT, 0x2f4, NULL, laneid);

   if (c == 2) {
      bld.mkOp2(OP_ADD, TYPE_F32, dst, x, y);
      bld.mkOp2(OP_SUB, TYPE_F32, dst, bld.loadImm(NULL, 1.0f), dst);
   }
}

bool
NVC0LoweringPass::handleRDSV(Instruction *i)
{
   Symbol *sym = i->getSrc(0)->asSym();
   const SVSemantic sv = sym->reg.data.sv.sv;
   Value *vtx = NULL;
   Instruction *ld;
   uint32_t addr = targ->getSVAddress(FILE_SHADER_INPUT, sym);

   if (addr >= 0x400) {
      // mov $sreg
      if (sym->reg.data.sv.index == 3) {
         // TGSI backend may use 4th component of TID,NTID,CTAID,NCTAID
         i->op = OP_MOV;
         i->setSrc(0, bld.mkImm((sv == SV_NTID || sv == SV_NCTAID) ? 1 : 0));
      }
      return true;
   }

   switch (sv) {
   case SV_POSITION:
      assert(prog->getType() == Program::TYPE_FRAGMENT);
      bld.mkInterp(NV50_IR_INTERP_LINEAR, i->getDef(0), addr, NULL);
      break;
   case SV_FACE:
   {
      Value *face = i->getDef(0);
      bld.mkInterp(NV50_IR_INTERP_FLAT, face, addr, NULL);
      if (i->dType == TYPE_F32) {
         bld.mkOp2(OP_AND, TYPE_U32, face, face, bld.mkImm(0x80000000));
         bld.mkOp2(OP_XOR, TYPE_U32, face, face, bld.mkImm(0xbf800000));
      }
   }
      break;
   case SV_TESS_COORD:
      assert(prog->getType() == Program::TYPE_TESSELLATION_EVAL);
      readTessCoord(i->getDef(0)->asLValue(), i->getSrc(0)->reg.data.sv.index);
      break;
   case SV_NTID:
   case SV_NCTAID:
   case SV_GRIDID:
      assert(targ->getChipset() >= NVISA_GK104_CHIPSET); // mov $sreg otherwise
      if (sym->reg.data.sv.index == 3) {
         i->op = OP_MOV;
         i->setSrc(0, bld.mkImm(sv == SV_GRIDID ? 0 : 1));
         return true;
      }
      addr += prog->driver->prop.cp.gridInfoBase;
      bld.mkLoad(TYPE_U32, i->getDef(0),
                 bld.mkSymbol(FILE_MEMORY_CONST, 0, TYPE_U32, addr), NULL);
      break;
   default:
      if (prog->getType() == Program::TYPE_TESSELLATION_EVAL)
         vtx = bld.mkOp1v(OP_PFETCH, TYPE_U32, bld.getSSA(), bld.mkImm(0));
      ld = bld.mkFetch(i->getDef(0), i->dType,
                       FILE_SHADER_INPUT, addr, i->getIndirect(0, 0), vtx);
      ld->perPatch = i->perPatch;
      break;
   }
   bld.getBB()->remove(i);
   return true;
}

bool
NVC0LoweringPass::handleDIV(Instruction *i)
{
   if (!isFloatType(i->dType))
      return true;
   bld.setPosition(i, false);
   Instruction *rcp = bld.mkOp1(OP_RCP, i->dType, bld.getSSA(), i->getSrc(1));
   i->op = OP_MUL;
   i->setSrc(1, rcp->getDef(0));
   return true;
}

bool
NVC0LoweringPass::handleMOD(Instruction *i)
{
   if (i->dType != TYPE_F32)
      return true;
   LValue *value = bld.getScratch();
   bld.mkOp1(OP_RCP, TYPE_F32, value, i->getSrc(1));
   bld.mkOp2(OP_MUL, TYPE_F32, value, i->getSrc(0), value);
   bld.mkOp1(OP_TRUNC, TYPE_F32, value, value);
   bld.mkOp2(OP_MUL, TYPE_F32, value, i->getSrc(1), value);
   i->op = OP_SUB;
   i->setSrc(1, value);
   return true;
}

bool
NVC0LoweringPass::handleSQRT(Instruction *i)
{
   Instruction *rsq = bld.mkOp1(OP_RSQ, TYPE_F32,
                                bld.getSSA(), i->getSrc(0));
   i->op = OP_MUL;
   i->setSrc(1, rsq->getDef(0));

   return true;
}

bool
NVC0LoweringPass::handlePOW(Instruction *i)
{
   LValue *val = bld.getScratch();

   bld.mkOp1(OP_LG2, TYPE_F32, val, i->getSrc(0));
   bld.mkOp2(OP_MUL, TYPE_F32, val, i->getSrc(1), val)->dnz = 1;
   bld.mkOp1(OP_PREEX2, TYPE_F32, val, val);

   i->op = OP_EX2;
   i->setSrc(0, val);
   i->setSrc(1, NULL);

   return true;
}

bool
NVC0LoweringPass::handleEXPORT(Instruction *i)
{
   if (prog->getType() == Program::TYPE_FRAGMENT) {
      int id = i->getSrc(0)->reg.data.offset / 4;

      if (i->src(0).isIndirect(0)) // TODO, ugly
         return false;
      i->op = OP_MOV;
      i->subOp = NV50_IR_SUBOP_MOV_FINAL;
      i->src(0).set(i->src(1));
      i->setSrc(1, NULL);
      i->setDef(0, new_LValue(func, FILE_GPR));
      i->getDef(0)->reg.data.id = id;

      prog->maxGPR = MAX2(prog->maxGPR, id);
   } else
   if (prog->getType() == Program::TYPE_GEOMETRY) {
      i->setIndirect(0, 1, gpEmitAddress);
   }
   return true;
}

bool
NVC0LoweringPass::handleOUT(Instruction *i)
{
   if (i->op == OP_RESTART && i->prev && i->prev->op == OP_EMIT) {
      i->prev->subOp = NV50_IR_SUBOP_EMIT_RESTART;
      delete_Instruction(prog, i);
   } else {
      assert(gpEmitAddress);
      i->setDef(0, gpEmitAddress);
      if (i->srcExists(0))
         i->setSrc(1, i->getSrc(0));
      i->setSrc(0, gpEmitAddress);
   }
   return true;
}

// Generate a binary predicate if an instruction is predicated by
// e.g. an f32 value.
void
NVC0LoweringPass::checkPredicate(Instruction *insn)
{
   Value *pred = insn->getPredicate();
   Value *pdst;

   if (!pred || pred->reg.file == FILE_PREDICATE)
      return;
   pdst = new_LValue(func, FILE_PREDICATE);

   // CAUTION: don't use pdst->getInsn, the definition might not be unique,
   //  delay turning PSET(FSET(x,y),0) into PSET(x,y) to a later pass

   bld.mkCmp(OP_SET, CC_NEU, TYPE_U32, pdst, bld.mkImm(0), pred);

   insn->setPredicate(insn->cc, pdst);
}

//
// - add quadop dance for texturing
// - put FP outputs in GPRs
// - convert instruction sequences
//
bool
NVC0LoweringPass::visit(Instruction *i)
{
   bld.setPosition(i, false);

   if (i->cc != CC_ALWAYS)
      checkPredicate(i);

   switch (i->op) {
   case OP_TEX:
   case OP_TXB:
   case OP_TXL:
   case OP_TXF:
   case OP_TXG:
      return handleTEX(i->asTex());
   case OP_TXD:
      return handleTXD(i->asTex());
   case OP_TXQ:
     return handleTXQ(i->asTex());
   case OP_EX2:
      bld.mkOp1(OP_PREEX2, TYPE_F32, i->getDef(0), i->getSrc(0));
      i->setSrc(0, i->getDef(0));
      break;
   case OP_POW:
      return handlePOW(i);
   case OP_DIV:
      return handleDIV(i);
   case OP_MOD:
      return handleMOD(i);
   case OP_SQRT:
      return handleSQRT(i);
   case OP_EXPORT:
      return handleEXPORT(i);
   case OP_EMIT:
   case OP_RESTART:
      return handleOUT(i);
   case OP_RDSV:
      return handleRDSV(i);
   case OP_WRSV:
      return handleWRSV(i);
   case OP_LOAD:
      if (i->src(0).getFile() == FILE_SHADER_INPUT) {
         if (prog->getType() == Program::TYPE_COMPUTE) {
            i->getSrc(0)->reg.file = FILE_MEMORY_CONST;
            i->getSrc(0)->reg.fileIndex = 0;
         } else {
            i->op = OP_VFETCH;
            assert(prog->getType() != Program::TYPE_FRAGMENT); // INTERP
         }
      }
      break;
   case OP_ATOM:
   {
      const bool cctl = i->src(0).getFile() == FILE_MEMORY_GLOBAL;
      handleATOM(i);
      handleCasExch(i, cctl);
   }
      break;
   case OP_SULDB:
   case OP_SULDP:
   case OP_SUSTB:
   case OP_SUSTP:
   case OP_SUREDB:
   case OP_SUREDP:
      if (targ->getChipset() >= NVISA_GK104_CHIPSET)
         handleSurfaceOpNVE4(i->asTex());
      break;
   default:
      break;
   }
   return true;
}

bool
TargetNVC0::runLegalizePass(Program *prog, CGStage stage) const
{
   if (stage == CG_STAGE_PRE_SSA) {
      NVC0LoweringPass pass(prog);
      return pass.run(prog, false, true);
   } else
   if (stage == CG_STAGE_POST_RA) {
      NVC0LegalizePostRA pass(prog);
      return pass.run(prog, false, true);
   } else
   if (stage == CG_STAGE_SSA) {
      NVC0LegalizeSSA pass;
      return pass.run(prog, false, true);
   }
   return false;
}

} // namespace nv50_ir
