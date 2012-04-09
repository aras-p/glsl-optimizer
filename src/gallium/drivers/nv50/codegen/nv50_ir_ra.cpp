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

#include "nv50/nv50_debug.h"

namespace nv50_ir {

#define MAX_REGISTER_FILE_SIZE 256

class RegisterSet
{
public:
   RegisterSet();
   RegisterSet(const Target *);

   void init(const Target *);
   void reset(); // reset allocation status, but not max assigned regs

   void periodicMask(DataFile f, uint32_t lock, uint32_t unlock);
   void intersect(DataFile f, const RegisterSet *);

   bool assign(Value **, int nr);
   void release(const Value *);
   void occupy(const Value *);

   int getMaxAssigned(DataFile f) const { return fill[f]; }

   void print() const;

private:
   uint32_t bits[FILE_ADDRESS + 1][(MAX_REGISTER_FILE_SIZE + 31) / 32];

   int unit[FILE_ADDRESS + 1]; // log2 of allocation granularity

   int last[FILE_ADDRESS + 1];
   int fill[FILE_ADDRESS + 1];
};

void
RegisterSet::reset()
{
   memset(bits, 0, sizeof(bits));
}

RegisterSet::RegisterSet()
{
   reset();
}

void
RegisterSet::init(const Target *targ)
{
   for (unsigned int rf = 0; rf <= FILE_ADDRESS; ++rf) {
      DataFile f = static_cast<DataFile>(rf);
      last[rf] = targ->getFileSize(f) - 1;
      unit[rf] = targ->getFileUnit(f);
      fill[rf] = -1;
      assert(last[rf] < MAX_REGISTER_FILE_SIZE);
   }
}

RegisterSet::RegisterSet(const Target *targ)
{
   reset();
   init(targ);
}

void
RegisterSet::periodicMask(DataFile f, uint32_t lock, uint32_t unlock)
{
   for (int i = 0; i < (last[f] + 31) / 32; ++i)
      bits[f][i] = (bits[f][i] | lock) & ~unlock;
}

void
RegisterSet::intersect(DataFile f, const RegisterSet *set)
{
   for (int i = 0; i < (last[f] + 31) / 32; ++i)
      bits[f][i] |= set->bits[f][i];
}

void
RegisterSet::print() const
{
   INFO("GPR:");
   for (int i = 0; i < (last[FILE_GPR] + 31) / 32; ++i)
      INFO(" %08x", bits[FILE_GPR][i]);
   INFO("\n");
}

bool
RegisterSet::assign(Value **def, int nr)
{
   DataFile f = def[0]->reg.file;
   int n = nr;
   if (n == 3)
      n = 4;
   int s = (n * def[0]->reg.size) >> unit[f];
   uint32_t m = (1 << s) - 1;

   int id = last[f] + 1;
   int i;

   for (i = 0; (i * 32) < last[f]; ++i) {
      if (bits[f][i] == 0xffffffff)
         continue;

      for (id = 0; id < 32; id += s)
         if (!(bits[f][i] & (m << id)))
            break;
      if (id < 32)
         break;
   }
   id += i * 32;
   if (id > last[f])
      return false;

   bits[f][id / 32] |= m << (id % 32);

   if (id + (s - 1) > fill[f])
      fill[f] = id + (s - 1);

   for (i = 0; i < nr; ++i, ++id)
      if (!def[i]->livei.isEmpty()) // XXX: really increased id if empty ?
         def[i]->reg.data.id = id;
   return true;
}

void
RegisterSet::occupy(const Value *val)
{
   int id = val->reg.data.id;
   if (id < 0)
      return;
   unsigned int f = val->reg.file;

   uint32_t m = (1 << (val->reg.size >> unit[f])) - 1;

   INFO_DBG(0, REG_ALLOC, "reg occupy: %u[%i] %x\n", f, id, m);

   bits[f][id / 32] |= m << (id % 32);

   if (fill[f] < id)
      fill[f] = id;
}

void
RegisterSet::release(const Value *val)
{
   int id = val->reg.data.id;
   if (id < 0)
      return;
   unsigned int f = val->reg.file;

   uint32_t m = (1 << (val->reg.size >> unit[f])) - 1;

   INFO_DBG(0, REG_ALLOC, "reg release: %u[%i] %x\n", f, id, m);

   bits[f][id / 32] &= ~(m << (id % 32));
}

#define JOIN_MASK_PHI        (1 << 0)
#define JOIN_MASK_UNION      (1 << 1)
#define JOIN_MASK_MOV        (1 << 2)
#define JOIN_MASK_TEX        (1 << 3)
#define JOIN_MASK_CONSTRAINT (1 << 4)

class RegAlloc
{
public:
   RegAlloc(Program *program) : prog(program), sequence(0) { }

   bool exec();
   bool execFunc();

private:
   bool coalesceValues(unsigned int mask);
   bool linearScan();
   bool allocateConstrainedValues();

private:
   class PhiMovesPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
      inline bool needNewElseBlock(BasicBlock *b, BasicBlock *p);
   };

   class BuildIntervalsPass : public Pass {
   private:
      virtual bool visit(BasicBlock *);
      void collectLiveValues(BasicBlock *);
      void addLiveRange(Value *, const BasicBlock *, int end);
   };

   class InsertConstraintsPass : public Pass {
   public:
      bool exec(Function *func);
   private:
      virtual bool visit(BasicBlock *);

      bool insertConstraintMoves();

      void addHazard(Instruction *i, const ValueRef *src);
      void textureMask(TexInstruction *);
      void addConstraint(Instruction *, int s, int n);
      bool detectConflict(Instruction *, int s);

      DLList constrList;
   };

   bool buildLiveSets(BasicBlock *);
   void collectLValues(DLList&, bool assignedOnly);

   void insertOrderedTail(DLList&, Value *);
   inline Instruction *insnBySerial(int);

private:
   Program *prog;
   Function *func;

   // instructions in control flow / chronological order
   ArrayList insns;

   int sequence; // for manual passes through CFG
};

Instruction *
RegAlloc::insnBySerial(int serial)
{
   return reinterpret_cast<Instruction *>(insns.get(serial));
}

void
RegAlloc::BuildIntervalsPass::addLiveRange(Value *val,
                                           const BasicBlock *bb,
                                           int end)
{
   Instruction *insn = val->getUniqueInsn();

   if (!insn)
      return;
   assert(bb->getFirst()->serial <= bb->getExit()->serial);
   assert(bb->getExit()->serial + 1 >= end);

   int begin = insn->serial;
   if (begin < bb->getEntry()->serial || begin > bb->getExit()->serial)
      begin = bb->getEntry()->serial;

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "%%%i <- live range [%i(%i), %i)\n",
            val->id, begin, insn->serial, end);

   if (begin != end) // empty ranges are only added as hazards for fixed regs
      val->livei.extend(begin, end);
}

bool
RegAlloc::PhiMovesPass::needNewElseBlock(BasicBlock *b, BasicBlock *p)
{
   if (b->cfg.incidentCount() <= 1)
      return false;

   int n = 0;
   for (Graph::EdgeIterator ei = p->cfg.outgoing(); !ei.end(); ei.next())
      if (ei.getType() == Graph::Edge::TREE ||
          ei.getType() == Graph::Edge::FORWARD)
         ++n;
   return (n == 2);
}

// For each operand of each PHI in b, generate a new value by inserting a MOV
// at the end of the block it is coming from and replace the operand with its
// result. This eliminates liveness conflicts and enables us to let values be
// copied to the right register if such a conflict exists nonetheless.
//
// These MOVs are also crucial in making sure the live intervals of phi srces
// are extended until the end of the loop, since they are not included in the
// live-in sets.
bool
RegAlloc::PhiMovesPass::visit(BasicBlock *bb)
{
   Instruction *phi, *mov;
   BasicBlock *pb, *pn;

   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      pb = pn = BasicBlock::get(ei.getNode());
      assert(pb);

      if (needNewElseBlock(bb, pb)) {
         pn = new BasicBlock(func);

         // deletes an edge, iterator is invalid after this:
         pb->cfg.detach(&bb->cfg);
         pb->cfg.attach(&pn->cfg, Graph::Edge::TREE);
         pn->cfg.attach(&bb->cfg, Graph::Edge::FORWARD); // XXX: check order !

         assert(pb->getExit()->op != OP_CALL);
         if (pb->getExit()->asFlow()->target.bb == bb)
            pb->getExit()->asFlow()->target.bb = pn;
         break;
      }
   }

   // insert MOVs (phi->src(j) should stem from j-th in-BB)
   int j = 0;
   for (Graph::EdgeIterator ei = bb->cfg.incident(); !ei.end(); ei.next()) {
      pb = BasicBlock::get(ei.getNode());
      if (!pb->isTerminated())
         pb->insertTail(new_FlowInstruction(func, OP_BRA, bb));

      for (phi = bb->getPhi(); phi && phi->op == OP_PHI; phi = phi->next) {
         mov = new_Instruction(func, OP_MOV, TYPE_U32);

         mov->setSrc(0, phi->getSrc(j));
         mov->setDef(0, new_LValue(func, phi->getDef(0)->asLValue()));
         phi->setSrc(j, mov->getDef(0));

         pb->insertBefore(pb->getExit(), mov);
      }
      ++j;
   }

   return true;
}

// Build the set of live-in variables of bb.
bool
RegAlloc::buildLiveSets(BasicBlock *bb)
{
   BasicBlock *bn;
   Instruction *i;
   unsigned int s, d;

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "buildLiveSets(BB:%i)\n", bb->getId());

   bb->liveSet.allocate(func->allLValues.getSize(), false);

   int n = 0;
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      bn = BasicBlock::get(ei.getNode());
      if (bn == bb)
         continue;
      if (bn->cfg.visit(sequence))
         if (!buildLiveSets(bn))
            return false;
      if (n++ == 0)
         bb->liveSet = bn->liveSet;
      else
         bb->liveSet |= bn->liveSet;
   }
   if (!n && !bb->liveSet.marker)
      bb->liveSet.fill(0);
   bb->liveSet.marker = true;

   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("BB:%i live set of out blocks:\n", bb->getId());
      bb->liveSet.print();
   }

   // if (!bb->getEntry())
   //   return true;

   for (i = bb->getExit(); i && i != bb->getEntry()->prev; i = i->prev) {
      for (d = 0; i->defExists(d); ++d)
         bb->liveSet.clr(i->getDef(d)->id);
      for (s = 0; i->srcExists(s); ++s)
         if (i->getSrc(s)->asLValue())
            bb->liveSet.set(i->getSrc(s)->id);
   }
   for (i = bb->getPhi(); i && i->op == OP_PHI; i = i->next)
      bb->liveSet.clr(i->getDef(0)->id);

   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      INFO("BB:%i live set after propagation:\n", bb->getId());
      bb->liveSet.print();
   }

   return true;
}

void
RegAlloc::BuildIntervalsPass::collectLiveValues(BasicBlock *bb)
{
   BasicBlock *bbA = NULL, *bbB = NULL;

   assert(bb->cfg.incidentCount() || bb->liveSet.popCount() == 0);

   if (bb->cfg.outgoingCount()) {
      // trickery to save a loop of OR'ing liveSets
      // aliasing works fine with BitSet::setOr
      for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
         if (ei.getType() == Graph::Edge::DUMMY)
            continue;
         if (bbA) {
            bb->liveSet.setOr(&bbA->liveSet, &bbB->liveSet);
            bbA = bb;
         } else {
            bbA = bbB;
         }
         bbB = BasicBlock::get(ei.getNode());
      }
      bb->liveSet.setOr(&bbB->liveSet, bbA ? &bbA->liveSet : NULL);
   } else
   if (bb->cfg.incidentCount()) {
      bb->liveSet.fill(0);
   }
}

bool
RegAlloc::BuildIntervalsPass::visit(BasicBlock *bb)
{
   collectLiveValues(bb);

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "BuildIntervals(BB:%i)\n", bb->getId());

   // go through out blocks and delete phi sources that do not originate from
   // the current block from the live set
   for (Graph::EdgeIterator ei = bb->cfg.outgoing(); !ei.end(); ei.next()) {
      BasicBlock *out = BasicBlock::get(ei.getNode());

      for (Instruction *i = out->getPhi(); i && i->op == OP_PHI; i = i->next) {
         bb->liveSet.clr(i->getDef(0)->id);

         for (int s = 0; i->srcExists(s); ++s) {
            assert(i->src(s).getInsn());
            if (i->getSrc(s)->getUniqueInsn()->bb == bb) // XXX: reachableBy ?
               bb->liveSet.set(i->getSrc(s)->id);
            else
               bb->liveSet.clr(i->getSrc(s)->id);
         }
      }
   }

   // remaining live-outs are live until end
   if (bb->getExit()) {
      for (unsigned int j = 0; j < bb->liveSet.getSize(); ++j)
         if (bb->liveSet.test(j))
            addLiveRange(func->getLValue(j), bb, bb->getExit()->serial + 1);
   }

   for (Instruction *i = bb->getExit(); i && i->op != OP_PHI; i = i->prev) {
      for (int d = 0; i->defExists(d); ++d) {
         bb->liveSet.clr(i->getDef(d)->id);
         if (i->getDef(d)->reg.data.id >= 0) // add hazard for fixed regs
            i->getDef(d)->livei.extend(i->serial, i->serial);
      }

      for (int s = 0; i->srcExists(s); ++s) {
         if (!i->getSrc(s)->asLValue())
            continue;
         if (!bb->liveSet.test(i->getSrc(s)->id)) {
            bb->liveSet.set(i->getSrc(s)->id);
            addLiveRange(i->getSrc(s), bb, i->serial);
         }
      }
   }

   return true;
}

bool
RegAlloc::coalesceValues(unsigned int mask)
{
   int c, n;

   for (n = 0; n < insns.getSize(); ++n) {
      Instruction *i;
      Instruction *insn = insnBySerial(n);

      switch (insn->op) {
      case OP_PHI:
         if (!(mask & JOIN_MASK_PHI))
            break;
         for (c = 0; insn->srcExists(c); ++c)
            if (!insn->getDef(0)->coalesce(insn->getSrc(c), false)) {
               ERROR("failed to coalesce phi operands\n");
               return false;
            }
         break;
      case OP_UNION:
         if (!(mask & JOIN_MASK_UNION))
            break;
         for (c = 0; insn->srcExists(c); ++c)
            insn->getDef(0)->coalesce(insn->getSrc(c), true);
         break;
      case OP_CONSTRAINT:
         if (!(mask & JOIN_MASK_CONSTRAINT))
            break;
         for (c = 0; c < 4 && insn->srcExists(c); ++c)
            insn->getDef(c)->coalesce(insn->getSrc(c), true);
         break;
      case OP_MOV:
         if (!(mask & JOIN_MASK_MOV))
            break;
         i = NULL;
         if (!insn->getDef(0)->uses.empty())
            i = insn->getDef(0)->uses.front()->getInsn();
         // if this is a contraint-move there will only be a single use
         if (i && i->op == OP_CONSTRAINT)
            break;
         i = insn->getSrc(0)->getUniqueInsn();
         if (i && !i->constrainedDefs())
            insn->getDef(0)->coalesce(insn->getSrc(0), false);
         break;
      case OP_TEX:
      case OP_TXB:
      case OP_TXL:
      case OP_TXF:
      case OP_TXQ:
      case OP_TXD:
      case OP_TXG:
      case OP_TEXCSAA:
         if (!(mask & JOIN_MASK_TEX))
            break;
         for (c = 0; c < 4 && insn->srcExists(c); ++c)
            insn->getDef(c)->coalesce(insn->getSrc(c), true);
         break;
      default:
         break;
      }
   }
   return true;
}

void
RegAlloc::insertOrderedTail(DLList &list, Value *val)
{
   // we insert the live intervals in order, so this should be short
   DLList::Iterator iter = list.revIterator();
   const int begin = val->livei.begin();
   for (; !iter.end(); iter.next()) {
      if (reinterpret_cast<Value *>(iter.get())->livei.begin() <= begin)
         break;
   }
   iter.insert(val);
}

static void
checkList(DLList &list)
{
   Value *prev = NULL;
   Value *next = NULL;

   for (DLList::Iterator iter = list.iterator(); !iter.end(); iter.next()) {
      next = Value::get(iter);
      assert(next);
      if (prev) {
         assert(prev->livei.begin() <= next->livei.begin());
      }
      assert(next->join == next);
      prev = next;
   }
}

void
RegAlloc::collectLValues(DLList &list, bool assignedOnly)
{
   for (int n = 0; n < insns.getSize(); ++n) {
      Instruction *i = insnBySerial(n);

      for (int d = 0; i->defExists(d); ++d)
         if (!i->getDef(d)->livei.isEmpty())
            if (!assignedOnly || i->getDef(d)->reg.data.id >= 0)
               insertOrderedTail(list, i->getDef(d));
   }
   checkList(list);
}

bool
RegAlloc::allocateConstrainedValues()
{
   Value *defs[4];
   RegisterSet regSet[4];
   DLList regVals;

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "RA: allocating constrained values\n");

   collectLValues(regVals, true);

   for (int c = 0; c < 4; ++c)
      regSet[c].init(prog->getTarget());

   for (int n = 0; n < insns.getSize(); ++n) {
      Instruction *i = insnBySerial(n);

      const int vecSize = i->defCount(0xf);
      if (vecSize < 2)
         continue;
      assert(vecSize <= 4);

      for (int c = 0; c < vecSize; ++c)
         defs[c] = i->def(c).rep();

      if (defs[0]->reg.data.id >= 0) {
         for (int c = 1; c < vecSize; ++c) {
            assert(defs[c]->reg.data.id >= 0);
         }
         continue;
      }

      for (int c = 0; c < vecSize; ++c) {
         uint32_t mask;
         regSet[c].reset();

         for (DLList::Iterator it = regVals.iterator(); !it.end(); it.next()) {
            Value *rVal = Value::get(it);
            if (rVal->reg.data.id >= 0 && rVal->livei.overlaps(defs[c]->livei))
               regSet[c].occupy(rVal);
         }
         mask = 0x11111111;
         if (vecSize == 2) // granularity is 2 instead of 4
            mask |= 0x11111111 << 2;
         regSet[c].periodicMask(defs[0]->reg.file, 0, ~(mask << c));

         if (!defs[c]->livei.isEmpty())
            insertOrderedTail(regVals, defs[c]);
      }
      for (int c = 1; c < vecSize; ++c)
         regSet[0].intersect(defs[0]->reg.file, &regSet[c]);

      if (!regSet[0].assign(&defs[0], vecSize)) // TODO: spilling
         return false;
   }
   for (int c = 0; c < 4; c += 2)
      if (regSet[c].getMaxAssigned(FILE_GPR) > prog->maxGPR)
         prog->maxGPR = regSet[c].getMaxAssigned(FILE_GPR);
   return true;
}

bool
RegAlloc::linearScan()
{
   Value *cur, *val;
   DLList unhandled, active, inactive;
   RegisterSet f(prog->getTarget()), free(prog->getTarget());

   INFO_DBG(prog->dbgFlags, REG_ALLOC, "RA: linear scan\n");

   collectLValues(unhandled, false);

   for (DLList::Iterator cI = unhandled.iterator(); !cI.end();) {
      cur = Value::get(cI);
      cI.erase();

      for (DLList::Iterator aI = active.iterator(); !aI.end();) {
         val = Value::get(aI);
         if (val->livei.end() <= cur->livei.begin()) {
            free.release(val);
            aI.erase();
         } else
         if (!val->livei.contains(cur->livei.begin())) {
            free.release(val);
            aI.moveToList(inactive);
         } else {
            aI.next();
         }
      }

      for (DLList::Iterator iI = inactive.iterator(); !iI.end();) {
         val = Value::get(iI);
         if (val->livei.end() <= cur->livei.begin()) {
            iI.erase();
         } else
         if (val->livei.contains(cur->livei.begin())) {
            free.occupy(val);
            iI.moveToList(active);
         } else {
            iI.next();
         }
      }
      f = free;

      for (DLList::Iterator iI = inactive.iterator(); !iI.end(); iI.next()) {
         val = Value::get(iI);
         if (val->livei.overlaps(cur->livei))
            f.occupy(val);
      }

      for (DLList::Iterator uI = unhandled.iterator(); !uI.end(); uI.next()) {
         val = Value::get(uI);
         if (val->reg.data.id >= 0 && val->livei.overlaps(cur->livei))
            f.occupy(val);
      }

      if (cur->reg.data.id < 0) {
         bool spill = !f.assign(&cur, 1);
         if (spill) {
            ERROR("out of registers of file %u\n", cur->reg.file);
            abort();
         }
      }
      free.occupy(cur);
      active.insert(cur);
   }

   if (f.getMaxAssigned(FILE_GPR) > prog->maxGPR)
      prog->maxGPR = f.getMaxAssigned(FILE_GPR);
   if (free.getMaxAssigned(FILE_GPR) > prog->maxGPR)
      prog->maxGPR = free.getMaxAssigned(FILE_GPR);
   return true;
}

bool
RegAlloc::exec()
{
   for (IteratorRef it = prog->calls.iteratorDFS(false);
        !it->end(); it->next()) {
      func = Function::get(reinterpret_cast<Graph::Node *>(it->get()));
      if (!execFunc())
         return false;
   }
   return true;
}

bool
RegAlloc::execFunc()
{
   InsertConstraintsPass insertConstr;
   PhiMovesPass insertMoves;
   BuildIntervalsPass buildIntervals;

   unsigned int i;
   bool ret;

   ret = insertConstr.exec(func);
   if (!ret)
      goto out;

   ret = insertMoves.run(func);
   if (!ret)
      goto out;

   for (sequence = func->cfg.nextSequence(), i = 0;
        ret && i <= func->loopNestingBound;
        sequence = func->cfg.nextSequence(), ++i)
      ret = buildLiveSets(BasicBlock::get(func->cfg.getRoot()));
   if (!ret)
      goto out;

   func->orderInstructions(this->insns);

   ret = buildIntervals.run(func);
   if (!ret)
      goto out;

   ret = coalesceValues(JOIN_MASK_PHI);
   if (!ret)
      goto out;
   switch (prog->getTarget()->getChipset() & 0xf0) {
   case 0x50:
      ret = coalesceValues(JOIN_MASK_UNION | JOIN_MASK_TEX);
      break;
   case 0xc0:
      ret = coalesceValues(JOIN_MASK_UNION | JOIN_MASK_CONSTRAINT);
      break;
   default:
      break;
   }
   if (!ret)
      goto out;
   ret = coalesceValues(JOIN_MASK_MOV);
   if (!ret)
      goto out;

   if (prog->dbgFlags & NV50_IR_DEBUG_REG_ALLOC) {
      func->print();
      func->printLiveIntervals();
   }

   ret = allocateConstrainedValues() && linearScan();
   if (!ret)
      goto out;

out:
   // TODO: should probably call destructor on LValues later instead
   for (ArrayList::Iterator it = func->allLValues.iterator();
        !it.end(); it.next())
      reinterpret_cast<LValue *>(it.get())->livei.clear();

   return ret;
}

bool Program::registerAllocation()
{
   RegAlloc ra(this);
   return ra.exec();
}

bool
RegAlloc::InsertConstraintsPass::exec(Function *ir)
{
   constrList.clear();

   bool ret = run(ir, true, true);
   if (ret)
      ret = insertConstraintMoves();
   return ret;
}

// TODO: make part of texture insn
void
RegAlloc::InsertConstraintsPass::textureMask(TexInstruction *tex)
{
   Value *def[4];
   int c, k, d;
   uint8_t mask = 0;

   for (d = 0, k = 0, c = 0; c < 4; ++c) {
      if (!(tex->tex.mask & (1 << c)))
         continue;
      if (tex->getDef(k)->refCount()) {
         mask |= 1 << c;
         def[d++] = tex->getDef(k);
      }
      ++k;
   }
   tex->tex.mask = mask;

#if 0 // reorder or set the unused ones NULL ?
   for (c = 0; c < 4; ++c)
      if (!(tex->tex.mask & (1 << c)))
         def[d++] = tex->getDef(c);
#endif
   for (c = 0; c < d; ++c)
      tex->setDef(c, def[c]);
#if 1
   for (; c < 4; ++c)
      tex->setDef(c, NULL);
#endif
}

bool
RegAlloc::InsertConstraintsPass::detectConflict(Instruction *cst, int s)
{
   Value *v = cst->getSrc(s);

   // current register allocation can't handle it if a value participates in
   // multiple constraints
   for (Value::UseIterator it = v->uses.begin(); it != v->uses.end(); ++it) {
      if (cst != (*it)->getInsn())
         return true;
   }

   // can start at s + 1 because detectConflict is called on all sources
   for (int c = s + 1; cst->srcExists(c); ++c)
      if (v == cst->getSrc(c))
         return true;

   Instruction *defi = v->getInsn();

   return (!defi || defi->constrainedDefs());
}

void
RegAlloc::InsertConstraintsPass::addConstraint(Instruction *i, int s, int n)
{
   Instruction *cst;
   int d;

   // first, look for an existing identical constraint op
   for (DLList::Iterator it = constrList.iterator(); !it.end(); it.next()) {
      cst = reinterpret_cast<Instruction *>(it.get());
      if (!i->bb->dominatedBy(cst->bb))
         break;
      for (d = 0; d < n; ++d)
         if (cst->getSrc(d) != i->getSrc(d + s))
            break;
      if (d >= n) {
         for (d = 0; d < n; ++d, ++s)
            i->setSrc(s, cst->getDef(d));
         return;
      }
   }
   cst = new_Instruction(func, OP_CONSTRAINT, i->dType);

   for (d = 0; d < n; ++s, ++d) {
      cst->setDef(d, new_LValue(func, FILE_GPR));
      cst->setSrc(d, i->getSrc(s));
      i->setSrc(s, cst->getDef(d));
   }
   i->bb->insertBefore(i, cst);

   constrList.insert(cst);
}

// Add a dummy use of the pointer source of >= 8 byte loads after the load
// to prevent it from being assigned a register which overlapping the load's
// destination, which would produce random corruptions.
void
RegAlloc::InsertConstraintsPass::addHazard(Instruction *i, const ValueRef *src)
{
   Instruction *hzd = new_Instruction(func, OP_NOP, TYPE_NONE);
   hzd->setSrc(0, src->get());
   i->bb->insertAfter(i, hzd);

}

// Insert constraint markers for instructions whose multiple sources must be
// located in consecutive registers.
bool
RegAlloc::InsertConstraintsPass::visit(BasicBlock *bb)
{
   TexInstruction *tex;
   Instruction *next;
   int s, n, size;

   for (Instruction *i = bb->getEntry(); i; i = next) {
      next = i->next;

      if ((tex = i->asTex())) {
         textureMask(tex);

         // FIXME: this is target specific
         if (tex->op == OP_TXQ) {
            s = tex->srcCount(0xff);
            n = 0;
         } else {
            s = tex->tex.target.getArgCount();
            if (!tex->tex.target.isArray() &&
                (tex->tex.rIndirectSrc >= 0 || tex->tex.sIndirectSrc >= 0))
               ++s;
	    if (tex->op == OP_TXD && tex->tex.useOffsets)
               ++s;
            n = tex->srcCount(0xff) - s;
            assert(n <= 4);
         }

         if (s > 1)
            addConstraint(i, 0, s);
         if (n > 1)
            addConstraint(i, s, n);
      } else
      if (i->op == OP_EXPORT || i->op == OP_STORE) {
         for (size = typeSizeof(i->dType), s = 1; size > 0; ++s) {
            assert(i->srcExists(s));
            size -= i->getSrc(s)->reg.size;
         }
         if ((s - 1) > 1)
            addConstraint(i, 1, s - 1);
      } else
      if (i->op == OP_LOAD) {
         if (i->src(0).isIndirect(0) && typeSizeof(i->dType) >= 8)
            addHazard(i, i->src(0).getIndirect(0));
      }
   }
   return true;
}

// Insert extra moves so that, if multiple register constraints on a value are
// in conflict, these conflicts can be resolved.
bool
RegAlloc::InsertConstraintsPass::insertConstraintMoves()
{
   for (DLList::Iterator it = constrList.iterator(); !it.end(); it.next()) {
      Instruction *cst = reinterpret_cast<Instruction *>(it.get());

      for (int s = 0; cst->srcExists(s); ++s) {
         if (!detectConflict(cst, s))
             continue;
         Instruction *mov = new_Instruction(func, OP_MOV,
                                            typeOfSize(cst->src(s).getSize()));
         mov->setSrc(0, cst->getSrc(s));
         mov->setDef(0, new_LValue(func, FILE_GPR));
         cst->setSrc(s, mov->getDef(0));

         cst->bb->insertBefore(cst, mov);
      }
   }
   return true;
}

} // namespace nv50_ir
