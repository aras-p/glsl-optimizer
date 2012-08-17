
#ifndef AMDGPU_ASMPRINTER_H
#define AMDGPU_ASMPRINTER_H

#include "llvm/CodeGen/AsmPrinter.h"

namespace llvm {

class AMDGPUAsmPrinter : public AsmPrinter {

public:
  explicit AMDGPUAsmPrinter(TargetMachine &TM, MCStreamer &Streamer)
    : AsmPrinter(TM, Streamer) { }

  virtual const char *getPassName() const {
    return "AMDGPU Assembly Printer";
  }

  /// EmitInstuction - Implemented in AMDGPUMCInstLower.cpp
  virtual void EmitInstruction(const MachineInstr *MI);
};

} // End anonymous llvm

#endif //AMDGPU_ASMPRINTER_H
