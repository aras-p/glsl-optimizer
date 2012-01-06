//===-- R600InstrFormats.td - TODO: Add brief description -------===//
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


class ALUInst <bits<10> op, dag outs, dag ins, string asm, list<dag> pattern>
  : InstR600 <, outs, ins , asm, pattern>
