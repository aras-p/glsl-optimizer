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

#ifndef __NV50_IR_H__
#define __NV50_IR_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "nv50_ir_util.h"
#include "nv50_ir_graph.h"

#include "nv50_ir_driver.h"

namespace nv50_ir {

enum operation
{
   OP_NOP = 0,
   OP_PHI,
   OP_UNION, // unify a new definition and several source values
   OP_SPLIT, // $r0d -> { $r0, $r1 } ($r0d and $r0/$r1 will be coalesced)
   OP_MERGE, // opposite of split, e.g. combine 2 32 bit into a 64 bit value
   OP_CONSTRAINT, // copy values into consecutive registers
   OP_MOV,
   OP_LOAD,
   OP_STORE,
   OP_ADD,
   OP_SUB,
   OP_MUL,
   OP_DIV,
   OP_MOD,
   OP_MAD,
   OP_FMA,
   OP_SAD, // abs(src0 - src1) + src2
   OP_ABS,
   OP_NEG,
   OP_NOT,
   OP_AND,
   OP_OR,
   OP_XOR,
   OP_SHL,
   OP_SHR,
   OP_MAX,
   OP_MIN,
   OP_SAT,  // CLAMP(f32, 0.0, 1.0)
   OP_CEIL,
   OP_FLOOR,
   OP_TRUNC,
   OP_CVT,
   OP_SET_AND, // dst = (src0 CMP src1) & src2
   OP_SET_OR,
   OP_SET_XOR,
   OP_SET,
   OP_SELP, // dst = src2 ? src0 : src1
   OP_SLCT, // dst = (src2 CMP 0) ? src0 : src1
   OP_RCP,
   OP_RSQ,
   OP_LG2,
   OP_SIN,
   OP_COS,
   OP_EX2,
   OP_EXP, // exponential (base M_E)
   OP_LOG, // natural logarithm
   OP_PRESIN,
   OP_PREEX2,
   OP_SQRT,
   OP_POW,
   OP_BRA,
   OP_CALL,
   OP_RET,
   OP_CONT,
   OP_BREAK,
   OP_PRERET,
   OP_PRECONT,
   OP_PREBREAK,
   OP_BRKPT,     // breakpoint (not related to loops)
   OP_JOINAT,    // push control flow convergence point
   OP_JOIN,      // converge
   OP_DISCARD,
   OP_EXIT,
   OP_MEMBAR,
   OP_VFETCH, // indirection 0 in attribute space, indirection 1 is vertex base
   OP_PFETCH, // fetch base address of vertex src0 (immediate) [+ src1]
   OP_EXPORT,
   OP_LINTERP,
   OP_PINTERP,
   OP_EMIT,    // emit vertex
   OP_RESTART, // restart primitive
   OP_TEX,
   OP_TXB, // texture bias
   OP_TXL, // texure lod
   OP_TXF, // texel fetch
   OP_TXQ, // texture size query
   OP_TXD, // texture derivatives
   OP_TXG, // texture gather
   OP_TEXCSAA,
   OP_SULD, // surface load
   OP_SUST, // surface store
   OP_DFDX,
   OP_DFDY,
   OP_RDSV, // read system value
   OP_WRSV, // write system value
   OP_PIXLD,
   OP_QUADOP,
   OP_QUADON,
   OP_QUADPOP,
   OP_POPCNT, // bitcount(src0 & src1)
   OP_INSBF,  // insert first src1[8:15] bits of src0 into src2 at src1[0:7]
   OP_EXTBF,
   OP_LAST
};

#define NV50_IR_SUBOP_MUL_HIGH     1
#define NV50_IR_SUBOP_EMIT_RESTART 1
#define NV50_IR_SUBOP_LDC_IL       1
#define NV50_IR_SUBOP_LDC_IS       2
#define NV50_IR_SUBOP_LDC_ISL      3
#define NV50_IR_SUBOP_SHIFT_WRAP   1

enum DataType
{
   TYPE_NONE,
   TYPE_U8,
   TYPE_S8,
   TYPE_U16,
   TYPE_S16,
   TYPE_U32,
   TYPE_S32,
   TYPE_U64, // 64 bit operations are only lowered after register allocation
   TYPE_S64,
   TYPE_F16,
   TYPE_F32,
   TYPE_F64,
   TYPE_B96,
   TYPE_B128
};

enum CondCode
{
   CC_FL = 0,
   CC_NEVER = CC_FL, // when used with FILE_FLAGS
   CC_LT = 1,
   CC_EQ = 2,
   CC_NOT_P = CC_EQ, // when used with FILE_PREDICATE
   CC_LE = 3,
   CC_GT = 4,
   CC_NE = 5,
   CC_P  = CC_NE,
   CC_GE = 6,
   CC_TR = 7,
   CC_ALWAYS = CC_TR,
   CC_U  = 8,
   CC_LTU = 9,
   CC_EQU = 10,
   CC_LEU = 11,
   CC_GTU = 12,
   CC_NEU = 13,
   CC_GEU = 14,
   CC_NO = 0x10,
   CC_NC = 0x11,
   CC_NS = 0x12,
   CC_NA = 0x13,
   CC_A  = 0x14,
   CC_S  = 0x15,
   CC_C  = 0x16,
   CC_O  = 0x17
};

enum RoundMode
{
   ROUND_N, // nearest
   ROUND_M, // towards -inf
   ROUND_Z, // towards 0
   ROUND_P, // towards +inf
   ROUND_NI, // nearest integer
   ROUND_MI, // to integer towards -inf
   ROUND_ZI, // to integer towards 0
   ROUND_PI, // to integer towards +inf
};

enum CacheMode
{
   CACHE_CA,            // cache at all levels
   CACHE_WB = CACHE_CA, // cache write back
   CACHE_CG,            // cache at global level
   CACHE_CS,            // cache streaming
   CACHE_CV,            // cache as volatile
   CACHE_WT = CACHE_CV  // cache write-through
};

enum DataFile
{
   FILE_NULL = 0,
   FILE_GPR,
   FILE_PREDICATE,       // boolean predicate
   FILE_FLAGS,           // zero/sign/carry/overflow bits
   FILE_ADDRESS,
   FILE_IMMEDIATE,
   FILE_MEMORY_CONST,
   FILE_SHADER_INPUT,
   FILE_SHADER_OUTPUT,
   FILE_MEMORY_GLOBAL,
   FILE_MEMORY_SHARED,
   FILE_MEMORY_LOCAL,
   FILE_SYSTEM_VALUE,
   DATA_FILE_COUNT
};

enum TexTarget
{
   TEX_TARGET_1D,
   TEX_TARGET_2D,
   TEX_TARGET_2D_MS,
   TEX_TARGET_3D,
   TEX_TARGET_CUBE,
   TEX_TARGET_1D_SHADOW,
   TEX_TARGET_2D_SHADOW,
   TEX_TARGET_CUBE_SHADOW,
   TEX_TARGET_1D_ARRAY,
   TEX_TARGET_2D_ARRAY,
   TEX_TARGET_2D_MS_ARRAY,
   TEX_TARGET_CUBE_ARRAY,
   TEX_TARGET_1D_ARRAY_SHADOW,
   TEX_TARGET_2D_ARRAY_SHADOW,
   TEX_TARGET_RECT,
   TEX_TARGET_RECT_SHADOW,
   TEX_TARGET_CUBE_ARRAY_SHADOW,
   TEX_TARGET_BUFFER,
   TEX_TARGET_COUNT
};

enum SVSemantic
{
   SV_POSITION, // WPOS
   SV_VERTEX_ID,
   SV_INSTANCE_ID,
   SV_INVOCATION_ID,
   SV_PRIMITIVE_ID,
   SV_VERTEX_COUNT, // gl_PatchVerticesIn
   SV_LAYER,
   SV_VIEWPORT_INDEX,
   SV_YDIR,
   SV_FACE,
   SV_POINT_SIZE,
   SV_POINT_COORD,
   SV_CLIP_DISTANCE,
   SV_SAMPLE_INDEX,
   SV_TESS_FACTOR,
   SV_TESS_COORD,
   SV_TID,
   SV_CTAID,
   SV_NTID,
   SV_GRIDID,
   SV_NCTAID,
   SV_LANEID,
   SV_PHYSID,
   SV_NPHYSID,
   SV_CLOCK,
   SV_LBASE,
   SV_SBASE,
   SV_UNDEFINED,
   SV_LAST
};

class Program;
class Function;
class BasicBlock;

class Target;

class Instruction;
class CmpInstruction;
class TexInstruction;
class FlowInstruction;

class Value;
class LValue;
class Symbol;
class ImmediateValue;

struct Storage
{
   DataFile file;
   int8_t fileIndex; // signed, may be indirect for CONST[]
   uint8_t size; // this should match the Instruction type's size
   DataType type; // mainly for pretty printing
   union {
      uint64_t u64;    // immediate values
      uint32_t u32;
      uint16_t u16;
      uint8_t u8;
      int64_t s64;
      int32_t s32;
      int16_t s16;
      int8_t s8;
      float f32;
      double f64;
      int32_t offset; // offset from 0 (base of address space)
      int32_t id;     // register id (< 0 if virtual/unassigned)
      struct {
         SVSemantic sv;
         int index;
      } sv;
   } data;
};

// precedence: NOT after SAT after NEG after ABS
#define NV50_IR_MOD_ABS (1 << 0)
#define NV50_IR_MOD_NEG (1 << 1)
#define NV50_IR_MOD_SAT (1 << 2)
#define NV50_IR_MOD_NOT (1 << 3)
#define NV50_IR_MOD_NEG_ABS (NV50_IR_MOD_NEG | NV50_IR_MOD_ABS)

#define NV50_IR_INTERP_MODE_MASK   0x3
#define NV50_IR_INTERP_LINEAR      (0 << 0)
#define NV50_IR_INTERP_PERSPECTIVE (1 << 0)
#define NV50_IR_INTERP_FLAT        (2 << 0)
#define NV50_IR_INTERP_SC          (3 << 0) // what exactly is that ?
#define NV50_IR_INTERP_SAMPLE_MASK 0xc
#define NV50_IR_INTERP_DEFAULT     (0 << 2)
#define NV50_IR_INTERP_CENTROID    (1 << 2)
#define NV50_IR_INTERP_OFFSET      (2 << 2)
#define NV50_IR_INTERP_SAMPLEID    (3 << 2)

// do we really want this to be a class ?
class Modifier
{
public:
   Modifier() : bits(0) { }
   Modifier(unsigned int m) : bits(m) { }
   Modifier(operation op);

   // @return new Modifier applying a after b (asserts if unrepresentable)
   Modifier operator*(const Modifier) const;
   Modifier operator==(const Modifier m) const { return m.bits == bits; }
   Modifier operator!=(const Modifier m) const { return m.bits != bits; }

   inline Modifier operator&(const Modifier m) const { return bits & m.bits; }
   inline Modifier operator|(const Modifier m) const { return bits | m.bits; }
   inline Modifier operator^(const Modifier m) const { return bits ^ m.bits; }

   operation getOp() const;

   inline int neg() const { return (bits & NV50_IR_MOD_NEG) ? 1 : 0; }
   inline int abs() const { return (bits & NV50_IR_MOD_ABS) ? 1 : 0; }

   inline operator bool() { return bits ? true : false; }

   void applyTo(ImmediateValue &imm) const;

   int print(char *buf, size_t size) const;

private:
   uint8_t bits;
};

class ValueRef
{
public:
   ValueRef();
   ~ValueRef();

   inline ValueRef& operator=(Value *val) { this->set(val); return *this; }

   inline bool exists() const { return value != NULL; }

   void set(Value *);
   void set(const ValueRef&);
   inline Value *get() const { return value; }
   inline Value *rep() const;

   inline Instruction *getInsn() const { return insn; }
   inline void setInsn(Instruction *inst) { insn = inst; }

   inline bool isIndirect(int dim) const { return indirect[dim] >= 0; }
   inline const ValueRef *getIndirect(int dim) const;

   inline DataFile getFile() const;
   inline unsigned getSize() const;

   // SSA: return eventual (traverse MOVs) literal value, if it exists
   ImmediateValue *getImmediate() const;

   class Iterator
   {
   public:
      Iterator(ValueRef *ref) : pos(ref), ini(ref) { }

      inline ValueRef *get() const { return pos; }
      inline bool end() const { return pos == NULL; }
      inline void next() { pos = (pos->next != ini) ? pos->next : 0; }

   private:
      ValueRef *pos, *ini;
   };

   inline Iterator iterator() { return Iterator(this); }

public:
   Modifier mod;
   int8_t indirect[2]; // >= 0 if relative to lvalue in insn->src[indirect[i]]
   uint8_t swizzle;

   bool usedAsPtr; // for printing

private:
   Value *value;
   Instruction *insn;
   ValueRef *next; // to link uses of the value
   ValueRef *prev;
};

class ValueDef
{
public:
   ValueDef();
   ~ValueDef();

   inline ValueDef& operator=(Value *val) { this->set(val); return *this; }

   inline bool exists() const { return value != NULL; }

   inline Value *get() const { return value; }
   inline Value *rep() const;
   void set(Value *);
   void replace(Value *, bool doSet); // replace all uses of the old value

   inline Instruction *getInsn() const { return insn; }
   inline void setInsn(Instruction *inst) { insn = inst; }

   inline DataFile getFile() const;
   inline unsigned getSize() const;

   // HACK: save the pre-SSA value in 'prev', in SSA we don't need the def list
   //  but we'll use it again for coalescing in register allocation
   inline void setSSA(LValue *);
   inline const LValue *preSSA() const;
   inline void restoreDefList(); // after having been abused for SSA hack
   void mergeDefs(ValueDef *);

   class Iterator
   {
   public:
      Iterator(ValueDef *def) : pos(def), ini(def) { }

      inline ValueDef *get() const { return pos; }
      inline bool end() const { return pos == NULL; }
      inline void next() { pos = (pos->next != ini) ? pos->next : NULL; }

   private:
      ValueDef *pos, *ini;
   };

   inline Iterator iterator() { return Iterator(this); }

private:
   Value *value;   // should make this LValue * ...
   Instruction *insn;
   ValueDef *next; // circular list of all definitions of the same value
   ValueDef *prev;
};

class Value
{
public:
   Value();

   virtual Value *clone(Function *) const { return NULL; }

   virtual int print(char *, size_t, DataType ty = TYPE_NONE) const = 0;

   virtual bool equals(const Value *, bool strict = false) const;
   virtual bool interfers(const Value *) const;

   inline Instruction *getUniqueInsn() const;
   inline Instruction *getInsn() const; // use when uniqueness is certain

   inline int refCount() { return refCnt; }
   inline int ref() { return ++refCnt; }
   inline int unref() { --refCnt; assert(refCnt >= 0); return refCnt; }

   inline LValue *asLValue();
   inline Symbol *asSym();
   inline ImmediateValue *asImm();
   inline const Symbol *asSym() const;
   inline const ImmediateValue *asImm() const;

   bool coalesce(Value *, bool force = false);

   inline bool inFile(DataFile f) { return reg.file == f; }

   static inline Value *get(Iterator&);

protected:
   int refCnt;

   friend class ValueDef;
   friend class ValueRef;

public:
   int id;
   ValueRef *uses;
   ValueDef *defs;
   Storage reg;

   // TODO: these should be in LValue:
   Interval livei;
   Value *join;
};

class LValue : public Value
{
public:
   LValue(Function *, DataFile file);
   LValue(Function *, LValue *);

   virtual Value *clone(Function *) const;

   virtual int print(char *, size_t, DataType ty = TYPE_NONE) const;

public:
   unsigned ssa : 1;

   int affinity;
};

class Symbol : public Value
{
public:
   Symbol(Program *, DataFile file = FILE_MEMORY_CONST, ubyte fileIdx = 0);

   virtual Value *clone(Function *) const;

   virtual bool equals(const Value *that, bool strict) const;

   virtual int print(char *, size_t, DataType ty = TYPE_NONE) const;

   // print with indirect values
   int print(char *, size_t, Value *, Value *, DataType ty = TYPE_NONE) const;

   inline void setFile(DataFile file, ubyte fileIndex = 0)
   {
      reg.file = file;
      reg.fileIndex = fileIndex;
   }

   inline void setOffset(int32_t offset);
   inline void setAddress(Symbol *base, int32_t offset);
   inline void setSV(SVSemantic sv, uint32_t idx = 0);

   inline const Symbol *getBase() const { return baseSym; }

private:
   Symbol *baseSym; // array base for Symbols representing array elements
};

class ImmediateValue : public Value
{
public:
   ImmediateValue(Program *, uint32_t);
   ImmediateValue(Program *, float);
   ImmediateValue(Program *, double);

   // NOTE: not added to program with
   ImmediateValue(const ImmediateValue *, DataType ty);

   virtual bool equals(const Value *that, bool strict) const;

   // these only work if 'type' is valid (we mostly use untyped literals):
   bool isInteger(const int ival) const; // ival is cast to this' type
   bool isNegative() const;
   bool isPow2() const;

   void applyLog2();

   // for constant folding:
   ImmediateValue operator+(const ImmediateValue&) const;
   ImmediateValue operator-(const ImmediateValue&) const;
   ImmediateValue operator*(const ImmediateValue&) const;
   ImmediateValue operator/(const ImmediateValue&) const;

   bool compare(CondCode cc, float fval) const;

   virtual int print(char *, size_t, DataType ty = TYPE_NONE) const;
};


#define NV50_IR_MAX_DEFS 4
#define NV50_IR_MAX_SRCS 8

class Instruction
{
public:
   Instruction();
   Instruction(Function *, operation, DataType);
   virtual ~Instruction();

   virtual Instruction *clone(bool deep) const;

   inline void setDef(int i, Value *val) { def[i].set(val); }
   inline void setSrc(int s, Value *val) { src[s].set(val); }
   void setSrc(int s, ValueRef&);
   void swapSources(int a, int b);
   bool setIndirect(int s, int dim, Value *);

   inline Value *getDef(int d) const { return def[d].get(); }
   inline Value *getSrc(int s) const { return src[s].get(); }
   inline Value *getIndirect(int s, int dim) const;

   inline bool defExists(int d) const { return d < 4 && def[d].exists(); }
   inline bool srcExists(int s) const { return s < 8 && src[s].exists(); }

   inline bool constrainedDefs() const { return def[1].exists(); }

   bool setPredicate(CondCode ccode, Value *);
   inline Value *getPredicate() const;
   bool writesPredicate() const;

   unsigned int defCount(unsigned int mask) const;
   unsigned int srcCount(unsigned int mask) const;

   // save & remove / set indirect[0,1] and predicate source
   void takeExtraSources(int s, Value *[3]);
   void putExtraSources(int s, Value *[3]);

   inline void setType(DataType type) { dType = sType = type; }

   inline void setType(DataType dtype, DataType stype)
   {
      dType = dtype;
      sType = stype;
   }

   inline bool isPseudo() const { return op < OP_MOV; }
   bool isDead() const;
   bool isNop() const;
   bool isCommutationLegal(const Instruction *) const; // must be adjacent !
   bool isActionEqual(const Instruction *) const;
   bool isResultEqual(const Instruction *) const;

   void print() const;

   inline CmpInstruction *asCmp();
   inline TexInstruction *asTex();
   inline FlowInstruction *asFlow();
   inline const TexInstruction *asTex() const;
   inline const CmpInstruction *asCmp() const;
   inline const FlowInstruction *asFlow() const;

public:
   Instruction *next;
   Instruction *prev;
   int id;
   int serial; // CFG order

   operation op;
   DataType dType; // destination or defining type
   DataType sType; // source or secondary type
   CondCode cc;
   RoundMode rnd;
   CacheMode cache;

   uint8_t subOp; // quadop, 1 for mul-high, etc.

   unsigned encSize    : 4; // encoding size in bytes
   unsigned saturate   : 1; // to [0.0f, 1.0f]
   unsigned join       : 1; // converge control flow (use OP_JOIN until end)
   unsigned fixed      : 1; // prevent dead code elimination
   unsigned terminator : 1; // end of basic block
   unsigned atomic     : 1;
   unsigned ftz        : 1; // flush denormal to zero
   unsigned dnz        : 1; // denormals, NaN are zero
   unsigned ipa        : 4; // interpolation mode
   unsigned lanes      : 4;
   unsigned perPatch   : 1;
   unsigned exit       : 1; // terminate program after insn

   int8_t postFactor; // MUL/DIV(if < 0) by 1 << postFactor

   int8_t predSrc;
   int8_t flagsDef;
   int8_t flagsSrc;

   // NOTE: should make these pointers, saves space and work on shuffling
   ValueDef def[NV50_IR_MAX_DEFS]; // no gaps !
   ValueRef src[NV50_IR_MAX_SRCS]; // no gaps !

   BasicBlock *bb;

   // instruction specific methods:
   // (don't want to subclass, would need more constructors and memory pools)
public:
   inline void setInterpolate(unsigned int mode) { ipa = mode; }

   unsigned int getInterpMode() const { return ipa & 0x3; }
   unsigned int getSampleMode() const { return ipa & 0xc; }

private:
   void init();
protected:
   void cloneBase(Instruction *clone, bool deep) const;
};

enum TexQuery
{
   TXQ_DIMS,
   TXQ_TYPE,
   TXQ_SAMPLE_POSITION,
   TXQ_FILTER,
   TXQ_LOD,
   TXQ_WRAP,
   TXQ_BORDER_COLOUR
};

class TexInstruction : public Instruction
{
public:
   class Target
   {
   public:
      Target(TexTarget targ = TEX_TARGET_2D) : target(targ) { }

      const char *getName() const { return descTable[target].name; }
      unsigned int getArgCount() const { return descTable[target].argc; }
      unsigned int getDim() const { return descTable[target].dim; }
      int isArray() const { return descTable[target].array ? 1 : 0; }
      int isCube() const { return descTable[target].cube ? 1 : 0; }
      int isShadow() const { return descTable[target].shadow ? 1 : 0; }

      Target& operator=(TexTarget targ)
      {
         assert(targ < TEX_TARGET_COUNT);
         return *this;
      }

      inline bool operator==(TexTarget targ) const { return target == targ; }

   private:
      struct Desc
      {
         char name[19];
         uint8_t dim;
         uint8_t argc;
         bool array;
         bool cube;
         bool shadow;
      };

      static const struct Desc descTable[TEX_TARGET_COUNT];

   private:
      enum TexTarget target;
   };

public:
   TexInstruction(Function *, operation);
   virtual ~TexInstruction();

   virtual Instruction *clone(bool deep) const;

   inline void setTexture(Target targ, uint8_t r, uint8_t s)
   {
      tex.r = r;
      tex.s = s;
      tex.target = targ;
   }

   inline Value *getIndirectR() const;
   inline Value *getIndirectS() const;

public:
   struct {
      Target target;

      uint8_t r;
      int8_t rIndirectSrc;
      uint8_t s;
      int8_t sIndirectSrc;

      uint8_t mask;
      uint8_t gatherComp;

      bool liveOnly; // only execute on live pixels of a quad (optimization)
      bool levelZero;
      bool derivAll;

      int8_t useOffsets; // 0, 1, or 4 for textureGatherOffsets
      int8_t offset[4][3];

      enum TexQuery query;
   } tex;

   ValueRef dPdx[3];
   ValueRef dPdy[3];
};

class CmpInstruction : public Instruction
{
public:
   CmpInstruction(Function *, operation);

   virtual Instruction *clone(bool deep) const;

   void setCondition(CondCode cond) { setCond = cond; }
   CondCode getCondition() const { return setCond; }

public:
   CondCode setCond;
};

class FlowInstruction : public Instruction
{
public:
   FlowInstruction(Function *, operation, BasicBlock *target);

public:
   unsigned allWarp  : 1;
   unsigned absolute : 1;
   unsigned limit    : 1;
   unsigned builtin  : 1; // true for calls to emulation code

   union {
      BasicBlock *bb;
      int builtin;
      Function *fn;
   } target;
};

class BasicBlock
{
public:
   BasicBlock(Function *);
   ~BasicBlock();

   inline int getId() const { return id; }
   inline unsigned int getInsnCount() const { return numInsns; }
   inline bool isTerminated() const { return exit && exit->terminator; }

   bool dominatedBy(BasicBlock *bb);
   inline bool reachableBy(BasicBlock *by, BasicBlock *term);

   // returns mask of conditional out blocks
   // e.g. 3 for IF { .. } ELSE { .. } ENDIF, 1 for IF { .. } ENDIF
   unsigned int initiatesSimpleConditional() const;

public:
   Function *getFunction() const { return func; }
   Program *getProgram() const { return program; }

   Instruction *getEntry() const { return entry; } // first non-phi instruction
   Instruction *getPhi() const { return phi; }
   Instruction *getFirst() const { return phi ? phi : entry; }
   Instruction *getExit() const { return exit; }

   void insertHead(Instruction *);
   void insertTail(Instruction *);
   void insertBefore(Instruction *, Instruction *);
   void insertAfter(Instruction *, Instruction *);
   void remove(Instruction *);
   void permuteAdjacent(Instruction *, Instruction *);

   BasicBlock *idom() const;

   // NOTE: currently does not rebuild the dominator tree
   BasicBlock *splitBefore(Instruction *, bool attach = true);
   BasicBlock *splitAfter(Instruction *, bool attach = true);

   DLList& getDF() { return df; }
   DLList::Iterator iterDF() { return df.iterator(); }

   static inline BasicBlock *get(Iterator&);
   static inline BasicBlock *get(Graph::Node *);

public:
   Graph::Node cfg; // first edge is branch *taken* (the ELSE branch)
   Graph::Node dom;

   BitSet liveSet;

   uint32_t binPos;
   uint32_t binSize;

   Instruction *joinAt; // for quick reference

   bool explicitCont; // loop headers: true if loop contains continue stmts

private:
   int id;
   DLList df;

   Instruction *phi;
   Instruction *entry;
   Instruction *exit;

   unsigned int numInsns;

private:
   Function *func;
   Program *program;

   void splitCommon(Instruction *, BasicBlock *, bool attach);
};

class Function
{
public:
   Function(Program *, const char *name);
   ~Function();

   inline Program *getProgram() const { return prog; }
   inline const char *getName() const { return name; }
   inline int getId() const { return id; }

   void print();
   void printLiveIntervals() const;
   void printCFGraph(const char *filePath);

   bool setEntry(BasicBlock *);
   bool setExit(BasicBlock *);

   unsigned int orderInstructions(ArrayList&);

   inline void add(BasicBlock *bb, int& id) { allBBlocks.insert(bb, id); }
   inline void add(Instruction *insn, int& id) { allInsns.insert(insn, id); }
   inline void add(LValue *lval, int& id) { allLValues.insert(lval, id); }

   inline LValue *getLValue(int id);

   bool convertToSSA();

public:
   Graph cfg;
   Graph::Node *cfgExit;
   Graph *domTree;
   Graph::Node call; // node in the call graph

   BasicBlock **bbArray; // BBs in emission order
   int bbCount;

   unsigned int loopNestingBound;
   int regClobberMax;

   uint32_t binPos;
   uint32_t binSize;

   ArrayList allBBlocks;
   ArrayList allInsns;
   ArrayList allLValues;

private:
   void buildLiveSetsPreSSA(BasicBlock *, const int sequence);

private:
   int id;
   const char *const name;
   Program *prog;
};

enum CGStage
{
   CG_STAGE_PRE_SSA,
   CG_STAGE_SSA, // expected directly before register allocation
   CG_STAGE_POST_RA
};

class Program
{
public:
   enum Type
   {
      TYPE_VERTEX,
      TYPE_TESSELLATION_CONTROL,
      TYPE_TESSELLATION_EVAL,
      TYPE_GEOMETRY,
      TYPE_FRAGMENT,
      TYPE_COMPUTE
   };

   Program(Type type, Target *targ);
   ~Program();

   void print();

   Type getType() const { return progType; }

   inline void add(Function *fn, int& id) { allFuncs.insert(fn, id); }
   inline void add(Value *rval, int& id) { allRValues.insert(rval, id); }

   bool makeFromTGSI(struct nv50_ir_prog_info *);
   bool makeFromSM4(struct nv50_ir_prog_info *);
   bool convertToSSA();
   bool optimizeSSA(int level);
   bool optimizePostRA(int level);
   bool registerAllocation();
   bool emitBinary(struct nv50_ir_prog_info *);

   const Target *getTarget() const { return target; }

private:
   Type progType;
   Target *target;

public:
   Function *main;
   Graph calls;

   ArrayList allFuncs;
   ArrayList allRValues;

   uint32_t *code;
   uint32_t binSize;

   int maxGPR;

   MemoryPool mem_Instruction;
   MemoryPool mem_CmpInstruction;
   MemoryPool mem_TexInstruction;
   MemoryPool mem_FlowInstruction;
   MemoryPool mem_LValue;
   MemoryPool mem_Symbol;
   MemoryPool mem_ImmediateValue;

   uint32_t dbgFlags;

   void releaseInstruction(Instruction *);
   void releaseValue(Value *);
};

// TODO: add const version
class Pass
{
public:
   bool run(Program *, bool ordered = false, bool skipPhi = false);
   bool run(Function *, bool ordered = false, bool skipPhi = false);

private:
   // return false to continue with next entity on next higher level
   virtual bool visit(Function *) { return true; }
   virtual bool visit(BasicBlock *) { return true; }
   virtual bool visit(Instruction *) { return false; }

   bool doRun(Program *, bool ordered, bool skipPhi);
   bool doRun(Function *, bool ordered, bool skipPhi);

protected:
   bool err;
   Function *func;
   Program *prog;
};

// =============================================================================

#include "nv50_ir_inlines.h"

} // namespace nv50_ir

#endif // __NV50_IR_H__
