//===-- AMDIL.td - TODO: Add brief description -------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//==-----------------------------------------------------------------------===//
// This file specifies where the base TD file exists
// and where the version specific TD file exists.
include "AMDILBase.td"
include "AMDILVersion.td"

include "R600Schedule.td"
include "SISchedule.td"
include "Processors.td"
include "AMDGPUIntrinsics.td"
include "AMDGPURegisterInfo.td"
include "AMDGPUInstructions.td"
