
#ifndef AMDGPU_MCINSTLOWER_H
#define AMDGPU_MCINSTLOWER_H

namespace llvm {

class MCInst;
class MachineInstr;

class AMDGPUMCInstLower {

public:
  AMDGPUMCInstLower();

  /// Lower - Lower a MachineInstr to an MCInst
  void Lower(const MachineInstr *MI, MCInst &OutMI) const;

};

} // End namespace llvm

#endif //AMDGPU_MCINSTLOWER_H
