//===-- R600Schedule.td - TODO: Add brief description -------===//
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


def ALU_X : FuncUnit;
def ALU_Y : FuncUnit;
def ALU_Z : FuncUnit;
def ALU_W : FuncUnit;
def TRANS : FuncUnit;


def AnyALU : InstrItinClass;
def VecALU : InstrItinClass;
def TransALU : InstrItinClass;

def R600_EG_Itin : ProcessorItineraries <
  [ALU_X, ALU_Y, ALU_Z, ALU_W, TRANS],
  [],
  [
    InstrItinData<AnyALU, [InstrStage<1, [ALU_X, ALU_Y, ALU_Z, ALU_W, TRANS]>]>,
    InstrItinData<VecALU, [InstrStage<1, [ALU_X, ALU_Y, ALU_X, ALU_W]>]>,
    InstrItinData<TransALU, [InstrStage<1, [TRANS]>]>
  ]
>;
