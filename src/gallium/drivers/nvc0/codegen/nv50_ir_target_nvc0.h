
#include "nv50/codegen/nv50_ir_target.h"

namespace nv50_ir {

#define NVC0_BUILTIN_DIV_U32 0
#define NVC0_BUILTIN_DIV_S32 1
#define NVC0_BUILTIN_RCP_F64 2
#define NVC0_BUILTIN_RSQ_F64 3

#define NVC0_BUILTIN_COUNT 4

class TargetNVC0 : public Target
{
public:
   TargetNVC0(unsigned int chipset);

   virtual CodeEmitter *getCodeEmitter(Program::Type);

   virtual bool runLegalizePass(Program *, CGStage stage) const;

   virtual void getBuiltinCode(const uint32_t **code, uint32_t *size) const;

   virtual bool insnCanLoad(const Instruction *insn, int s,
                            const Instruction *ld) const;
   virtual bool isOpSupported(operation, DataType) const;
   virtual bool isModSupported(const Instruction *, int s, Modifier) const;
   virtual bool isSatSupported(const Instruction *) const;
   virtual bool mayPredicate(const Instruction *, const Value *) const;

   virtual int getLatency(const Instruction *) const;
   virtual int getThroughput(const Instruction *) const;

   virtual unsigned int getFileSize(DataFile) const;
   virtual unsigned int getFileUnit(DataFile) const;

   virtual uint32_t getSVAddress(DataFile shaderFile, const Symbol *sv) const;

   uint32_t getBuiltinOffset(int builtin) const;

private:
   void initOpInfo();

};

} // namespace nv50_ir
