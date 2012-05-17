//===-- AMDGPUInstrInfo.td - AMDGPU DAG nodes --------------*- tablegen -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains DAG node defintions for the AMDGPU target.
//
//===----------------------------------------------------------------------===//

//===----------------------------------------------------------------------===//
// AMDGPU DAG Nodes
//

// out = max(a, b) a and b are floats
def AMDGPUfmax : SDNode<"AMDGPUISD::FMAX", SDTFPBinOp,
  [SDNPCommutative, SDNPAssociative]
>;

// out = max(a, b) a and b are signed ints
def AMDGPUsmax : SDNode<"AMDGPUISD::SMAX", SDTIntBinOp,
  [SDNPCommutative, SDNPAssociative]
>;

// out = max(a, b) a and b are unsigned ints
def AMDGPUumax : SDNode<"AMDGPUISD::UMAX", SDTIntBinOp,
  [SDNPCommutative, SDNPAssociative]
>;

// out = min(a, b) a and b are floats
def AMDGPUfmin : SDNode<"AMDGPUISD::FMIN", SDTFPBinOp,
  [SDNPCommutative, SDNPAssociative]
>;

// out = min(a, b) a snd b are signed ints
def AMDGPUsmin : SDNode<"AMDGPUISD::SMIN", SDTIntBinOp,
  [SDNPCommutative, SDNPAssociative]
>;

// out = min(a, b) a and b are unsigned ints
def AMDGPUumin : SDNode<"AMDGPUISD::UMIN", SDTIntBinOp,
  [SDNPCommutative, SDNPAssociative]
>;
