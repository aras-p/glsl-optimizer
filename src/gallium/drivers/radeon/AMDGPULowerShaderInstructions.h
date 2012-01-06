//===-- AMDGPULowerShaderInstructions.h - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// TODO: Add full description
//
//===----------------------------------------------------------------------===//


#ifndef AMDGPU_LOWER_SHADER_INSTRUCTIONS
#define AMDGPU_LOWER_SHADER_INSTRUCTIONS

namespace llvm {

class MachineFunction;
class MachineRegisterInfo;
class TargetInstrInfo;

class AMDGPULowerShaderInstructionsPass {

  protected:
    MachineRegisterInfo * MRI;
    /**
     * @param physReg The physical register that will be preloaded.
     * @param virtReg The virtual register that currently holds the
     *                preloaded value.
     */
    void preloadRegister(MachineFunction * MF, const TargetInstrInfo * TII,
                         unsigned physReg, unsigned virtReg) const;
};

} // end namespace llvm


#endif // AMDGPU_LOWER_SHADER_INSTRUCTIONS
