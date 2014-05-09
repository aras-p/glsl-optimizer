#include "codegen/nv50_ir_target_nvc0.h"

namespace nv50_ir {

class TargetGM107 : public TargetNVC0
{
public:
   TargetGM107(unsigned int chipset) : TargetNVC0(chipset) {}

   virtual CodeEmitter *getCodeEmitter(Program::Type);
   CodeEmitter *createCodeEmitterGM107(Program::Type);

   virtual bool runLegalizePass(Program *, CGStage) const;

   virtual void getBuiltinCode(const uint32_t **, uint32_t *) const;
   virtual uint32_t getBuiltinOffset(int) const;

   virtual bool isOpSupported(operation, DataType) const;
};

} // namespace nv50_ir
