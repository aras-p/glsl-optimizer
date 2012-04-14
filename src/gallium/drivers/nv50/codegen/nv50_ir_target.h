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

#ifndef __NV50_IR_TARGET_H__
#define __NV50_IR_TARGET_H__

#include "nv50_ir.h"

namespace nv50_ir {

struct RelocInfo;

struct RelocEntry
{
   enum Type
   {
      TYPE_CODE,
      TYPE_BUILTIN,
      TYPE_DATA
   };

   uint32_t data;
   uint32_t mask;
   uint32_t offset;
   int8_t bitPos;
   Type type;

   inline void apply(uint32_t *binary, const RelocInfo *info) const;
};

struct RelocInfo
{
   uint32_t codePos;
   uint32_t libPos;
   uint32_t dataPos;

   uint32_t count;

   RelocEntry entry[0];
};

class CodeEmitter
{
public:
   CodeEmitter(const Target *);

   // returns whether the instruction was encodable and written
   virtual bool emitInstruction(Instruction *) = 0;

   virtual uint32_t getMinEncodingSize(const Instruction *) const = 0;

   void setCodeLocation(void *, uint32_t size);
   inline void *getCodeLocation() const { return code; }
   inline uint32_t getCodeSize() const { return codeSize; }

   bool addReloc(RelocEntry::Type, int w, uint32_t data, uint32_t m,
                 int s);

   inline void *getRelocInfo() const { return relocInfo; }

   void prepareEmission(Program *);
   virtual void prepareEmission(Function *);
   virtual void prepareEmission(BasicBlock *);

   void printBinary() const;

protected:
   const Target *targ;

   uint32_t *code;
   uint32_t codeSize;
   uint32_t codeSizeLimit;

   RelocInfo *relocInfo;
};

class Target
{
public:
   static Target *create(uint32_t chipset);
   static void destroy(Target *);

   // 0x50 and 0x84 to 0xaf for nv50
   // 0xc0 to 0xdf for nvc0
   inline uint32_t getChipset() const { return chipset; }

   virtual CodeEmitter *getCodeEmitter(Program::Type) = 0;

   // Drivers should upload this so we can use it from all programs.
   // The address chosen is supplied to the relocation routine.
   virtual void getBuiltinCode(const uint32_t **code, uint32_t *size) const = 0;

   virtual void parseDriverInfo(const struct nv50_ir_prog_info *info) { }

   virtual bool runLegalizePass(Program *, CGStage stage) const = 0;

public:
   struct OpInfo
   {
      OpInfo *variants;
      operation op;
      uint16_t srcTypes;
      uint16_t dstTypes;
      uint32_t immdBits;
      uint8_t srcNr;
      uint8_t srcMods[3];
      uint8_t dstMods;
      uint8_t srcFiles[3];
      uint8_t dstFiles;
      unsigned int minEncSize  : 4;
      unsigned int vector      : 1;
      unsigned int predicate   : 1;
      unsigned int commutative : 1;
      unsigned int pseudo      : 1;
      unsigned int flow        : 1;
      unsigned int hasDest     : 1;
      unsigned int terminator  : 1;
   };

   inline const OpInfo& getOpInfo(const Instruction *) const;
   inline const OpInfo& getOpInfo(const operation) const;

   inline DataFile nativeFile(DataFile f) const;

   virtual bool insnCanLoad(const Instruction *insn, int s,
                            const Instruction *ld) const = 0;
   virtual bool isOpSupported(operation, DataType) const = 0;
   virtual bool isAccessSupported(DataFile, DataType) const = 0;
   virtual bool isModSupported(const Instruction *,
                               int s, Modifier) const = 0;
   virtual bool isSatSupported(const Instruction *) const = 0;
   virtual bool isPostMultiplySupported(operation op, float f,
                                        int& e) const { return false; }
   virtual bool mayPredicate(const Instruction *,
                             const Value *) const = 0;

   virtual int getLatency(const Instruction *) const { return 1; }
   virtual int getThroughput(const Instruction *) const { return 1; }

   virtual unsigned int getFileSize(DataFile) const = 0;
   virtual unsigned int getFileUnit(DataFile) const = 0;

   virtual uint32_t getSVAddress(DataFile, const Symbol *) const = 0;

public:
   bool joinAnterior; // true if join is executed before the op

   static const uint8_t operationSrcNr[OP_LAST + 1];

protected:
   uint32_t chipset;

   DataFile nativeFileMap[DATA_FILE_COUNT];

   OpInfo opInfo[OP_LAST + 1];
};

const Target::OpInfo& Target::getOpInfo(const Instruction *insn) const
{
   return opInfo[MIN2(insn->op, OP_LAST)];
}

const Target::OpInfo& Target::getOpInfo(const operation op) const
{
   return opInfo[op];
}

inline DataFile Target::nativeFile(DataFile f) const
{
   return nativeFileMap[f];
}

} // namespace nv50_ir

#endif // __NV50_IR_TARGET_H__
