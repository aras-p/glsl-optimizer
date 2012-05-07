//===-- R600CodeEmitter.cpp - TODO: Add brief description -------===//
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

#include "AMDGPU.h"
#include "AMDGPUUtil.h"
#include "AMDILCodeEmitter.h"
#include "AMDILInstrInfo.h"
#include "AMDILUtilityFunctions.h"
#include "R600RegisterInfo.h"
#include "llvm/CodeGen/MachineFunctionPass.h"
#include "llvm/CodeGen/MachineInstrBuilder.h"
#include "llvm/CodeGen/MachineRegisterInfo.h"
#include "llvm/Support/DataTypes.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Target/TargetMachine.h"

#include <stdio.h>

#define SRC_BYTE_COUNT 11
#define DST_BYTE_COUNT 5

using namespace llvm;

namespace {

  class R600CodeEmitter : public MachineFunctionPass, public AMDILCodeEmitter {

  private:

  static char ID;
  formatted_raw_ostream &_OS;
  const TargetMachine * TM;
  const MachineRegisterInfo * MRI;
  const R600RegisterInfo * TRI;
  bool evergreenEncoding;

  bool isReduction;
  unsigned reductionElement;
  bool isLast;

  unsigned section_start;

  public:

  R600CodeEmitter(formatted_raw_ostream &OS) : MachineFunctionPass(ID),
      _OS(OS), TM(NULL), evergreenEncoding(false), isReduction(false),
      isLast(true) { }

  const char *getPassName() const { return "AMDGPU Machine Code Emitter"; }

  bool runOnMachineFunction(MachineFunction &MF);
  virtual uint64_t getMachineOpValue(const MachineInstr &MI,
                                     const MachineOperand &MO) const;

  private:

  void emitALUInstr(MachineInstr  &MI);
  void emitSrc(const MachineOperand & MO);
  void emitDst(const MachineOperand & MO);
  void emitALU(MachineInstr &MI, unsigned numSrc);
  void emitTexInstr(MachineInstr &MI);
  void emitFCInstr(MachineInstr &MI);

  unsigned int getHWInst(const MachineInstr &MI);

  void emitNullBytes(unsigned int byteCount);

  void emitByte(unsigned int byte);

  void emitTwoBytes(uint32_t bytes);

  void emit(uint32_t value);
  void emit(uint64_t value);

  unsigned getHWReg(unsigned regNo) const;

  unsigned getElement(unsigned regNo);

};

} /* End anonymous namespace */

#define WRITE_MASK_X 0x1
#define WRITE_MASK_Y 0x2
#define WRITE_MASK_Z 0x4
#define WRITE_MASK_W 0x8

enum RegElement {
  ELEMENT_X = 0,
  ELEMENT_Y,
  ELEMENT_Z,
  ELEMENT_W
};

enum InstrTypes {
  INSTR_ALU = 0,
  INSTR_TEX,
  INSTR_FC,
  INSTR_NATIVE,
  INSTR_VTX
};

enum FCInstr {
  FC_IF = 0,
  FC_ELSE,
  FC_ENDIF,
  FC_BGNLOOP,
  FC_ENDLOOP,
  FC_BREAK,
  FC_BREAK_NZ_INT,
  FC_CONTINUE,
  FC_BREAK_Z_INT
};

enum TextureTypes {
  TEXTURE_1D = 1,
  TEXTURE_2D, 
  TEXTURE_3D,
  TEXTURE_CUBE,
  TEXTURE_RECT,
  TEXTURE_SHADOW1D,
  TEXTURE_SHADOW2D,
  TEXTURE_SHADOWRECT,     
  TEXTURE_1D_ARRAY,       
  TEXTURE_2D_ARRAY,
  TEXTURE_SHADOW1D_ARRAY,
  TEXTURE_SHADOW2D_ARRAY
};

char R600CodeEmitter::ID = 0;

FunctionPass *llvm::createR600CodeEmitterPass(formatted_raw_ostream &OS) {
  return new R600CodeEmitter(OS);
}

bool R600CodeEmitter::runOnMachineFunction(MachineFunction &MF) {

  TM = &MF.getTarget();
  MRI = &MF.getRegInfo();
  TRI = static_cast<const R600RegisterInfo *>(TM->getRegisterInfo());
  const AMDILSubtarget &STM = TM->getSubtarget<AMDILSubtarget>();
  std::string gpu = STM.getDeviceName();
  if (!gpu.compare(0,3, "rv7")) {
    evergreenEncoding = false;
  } else {
    evergreenEncoding = true;
  }

  if (STM.dumpCode()) {
    MF.dump();
  }

  for (MachineFunction::iterator BB = MF.begin(), BB_E = MF.end();
                                                  BB != BB_E; ++BB) {
     MachineBasicBlock &MBB = *BB;
     for (MachineBasicBlock::iterator I = MBB.begin(), E = MBB.end();
                                                       I != E; ++I) {
          MachineInstr &MI = *I;
          if (MI.getNumOperands() > 1 && MI.getOperand(0).isReg() && MI.getOperand(0).isDead()) {
            continue;
          }
          if (isTexOp(MI.getOpcode())) {
            emitTexInstr(MI);
          } else if (isFCOp(MI.getOpcode())){
            emitFCInstr(MI);
          } else if (isReductionOp(MI.getOpcode())) {
            isReduction = true;
            isLast = false;
            for (reductionElement = 0; reductionElement < 4; reductionElement++) {
              isLast = (reductionElement == 3);
              emitALUInstr(MI);
            }
            isReduction = false;
          } else if (MI.getOpcode() == AMDIL::RETURN ||
                     MI.getOpcode() == AMDIL::BUNDLE ||
                     MI.getOpcode() == AMDIL::KILL) {
            continue;
          } else {
            switch(MI.getOpcode()) {
            case AMDIL::RAT_WRITE_CACHELESS_eg:
              {
                /* XXX: Support for autoencoding 64-bit instructions was added
                 * in LLVM 3.1.  Until we drop support for 3.0, we will use Magic
                 * numbers for the high bits. */
                  uint64_t high = 0x95c0100000000000;
                  uint64_t inst = getBinaryCodeForInstr(MI);
                  inst |= high;
                /* Set End Of Program bit */
                /* XXX: Need better check of end of program.  EOP should be
                 * encoded in one of the operands of the MI, and it should be
                 * set in a prior pass. */
                MachineBasicBlock::iterator NextI = llvm::next(I);
                MachineInstr &NextMI = *NextI;
                if (NextMI.getOpcode() == AMDIL::RETURN) {
                  inst |= (((uint64_t)1) << 53);
                }
                emitByte(INSTR_NATIVE);
                emit(inst);
                break;
              }
            case AMDIL::VTX_READ_eg:
              {
                emitByte(INSTR_VTX);
                /* inst */
                emitByte(0);

                /* fetch_type */
                emitByte(2);

                /* buffer_id */
                emitByte(MI.getOperand(2).getImm());

                /* src_gpr */
                emitByte(getHWReg(MI.getOperand(1).getReg()));

                /* src_sel_x */
                emitByte(TRI->getHWRegChan(MI.getOperand(1).getReg()));

                /* mega_fetch_count */
                emitByte(3);

                /* dst_gpr */
                emitByte(getHWReg(MI.getOperand(0).getReg()));

                /* dst_sel_x */
                emitByte(0);

                /* dst_sel_y */
                emitByte(7);

                /* dst_sel_z */
                emitByte(7);

                /* dst_sel_w */
                emitByte(7);

                /* use_const_fields */
                emitByte(1);

                /* data_format */
                emitByte(0);

                /* num_format_all */
                emitByte(0);

                /* format_comp_all */
                emitByte(0);

                /* srf_mode_all */
                emitByte(0);

                /* offset */
                emitByte(0);

                /* endian */
                emitByte(0);
                break;
              }

            default:
              emitALUInstr(MI);
              break;
          }
        }
    }
  }
  return false;
}

void R600CodeEmitter::emitALUInstr(MachineInstr &MI)
{

  unsigned numOperands = MI.getNumExplicitOperands();

   /* Some instructions are just place holder instructions that represent
    * operations that the GPU does automatically.  They should be ignored. */
  if (isPlaceHolderOpcode(MI.getOpcode())) {
    return;
  }

  /* We need to handle some opcodes differently */
  switch (MI.getOpcode()) {
    default: break;

    /* XXX: Temp Hack */
    case AMDIL::STORE_OUTPUT:
      numOperands = 2;
      break;
  }

  /* XXX Check if instruction writes a result */
  if (numOperands < 1) {
    return;
  }
  const MachineOperand dstOp = MI.getOperand(0);

  /* Emit instruction type */
  emitByte(0);

  unsigned int opIndex;
  for (opIndex = 1; opIndex < numOperands; opIndex++) {
    /* Literal constants are always stored as the last operand. */
    if (MI.getOperand(opIndex).isImm() || MI.getOperand(opIndex).isFPImm()) {
      break;
    }
    emitSrc(MI.getOperand(opIndex));
  }

    /* Emit zeros for unused sources */
  for ( ; opIndex < 4; opIndex++) {
    emitNullBytes(SRC_BYTE_COUNT);
  }

  emitDst(dstOp);

  emitALU(MI, numOperands - 1);
}

void R600CodeEmitter::emitSrc(const MachineOperand & MO)
{
  uint32_t value = 0;
  /* Emit the source select (2 bytes).  For GPRs, this is the register index.
   * For other potential instruction operands, (e.g. constant registers) the
   * value of the source select is defined in the r600isa docs. */
  if (MO.isReg()) {
    unsigned reg = MO.getReg();
    emitTwoBytes(getHWReg(reg));
    if (reg == AMDIL::ALU_LITERAL_X) {
      const MachineInstr * parent = MO.getParent();
      unsigned immOpIndex = parent->getNumExplicitOperands() - 1;
      MachineOperand immOp = parent->getOperand(immOpIndex);
      if (immOp.isFPImm()) {
        value = immOp.getFPImm()->getValueAPF().bitcastToAPInt().getZExtValue();
      } else {
        assert(immOp.isImm());
        value = immOp.getImm();
      }
    }
  } else {
    /* XXX: Handle other operand types. */
    emitTwoBytes(0);
  }

  /* Emit the source channel (1 byte) */
  if (isReduction) {
    emitByte(reductionElement);
  } else if (MO.isReg()) {
    emitByte(TRI->getHWRegChan(MO.getReg()));
  } else {
    emitByte(0);
  }

  /* XXX: Emit isNegated (1 byte) */
  if ((!(MO.getTargetFlags() & MO_FLAG_ABS))
      && (MO.getTargetFlags() & MO_FLAG_NEG ||
     (MO.isReg() &&
      (MO.getReg() == AMDIL::NEG_ONE || MO.getReg() == AMDIL::NEG_HALF)))){
    emitByte(1);
  } else {
    emitByte(0);
  }

  /* Emit isAbsolute (1 byte) */
  if (MO.getTargetFlags() & MO_FLAG_ABS) {
    emitByte(1);
  } else {
    emitByte(0);
  }

  /* XXX: Emit relative addressing mode (1 byte) */
  emitByte(0);

  /* Emit kc_bank, This will be adjusted later by r600_asm */
  emitByte(0);

  /* Emit the literal value, if applicable (4 bytes).  */
  emit(value);

}

void R600CodeEmitter::emitDst(const MachineOperand & MO)
{
  if (MO.isReg()) {
    /* Emit the destination register index (1 byte) */
    emitByte(getHWReg(MO.getReg()));

    /* Emit the element of the destination register (1 byte)*/
    if (isReduction) {
      emitByte(reductionElement);
    } else {
      emitByte(TRI->getHWRegChan(MO.getReg()));
    }

    /* Emit isClamped (1 byte) */
    if (MO.getTargetFlags() & MO_FLAG_CLAMP) {
      emitByte(1);
    } else {
      emitByte(0);
    }

    /* Emit writemask (1 byte).  */
    if ((isReduction && reductionElement != TRI->getHWRegChan(MO.getReg()))
         || MO.getTargetFlags() & MO_FLAG_MASK) {
      emitByte(0);
    } else {
      emitByte(1);
    }

    /* XXX: Emit relative addressing mode */
    emitByte(0);
  } else {
    /* XXX: Handle other operand types.  Are there any for destination regs? */
    emitNullBytes(DST_BYTE_COUNT);
  }
}

void R600CodeEmitter::emitALU(MachineInstr &MI, unsigned numSrc)
{
  /* Emit the instruction (2 bytes) */
  emitTwoBytes(getHWInst(MI));

  /* Emit isLast (for this instruction group) (1 byte) */
  if (isLast) {
    emitByte(1);
  } else {
    emitByte(0);
  }
  /* Emit isOp3 (1 byte) */
  if (numSrc == 3) {
    emitByte(1);
  } else {
    emitByte(0);
  }

  /* XXX: Emit predicate (1 byte) */
  emitByte(0);

  /* XXX: Emit bank swizzle. (1 byte)  Do we need this?  It looks like
   * r600_asm.c sets it. */
  emitByte(0);

  /* XXX: Emit bank_swizzle_force (1 byte) Not sure what this is for. */
  emitByte(0);

  /* XXX: Emit OMOD (1 byte) Not implemented. */
  emitByte(0);

  /* XXX: Emit index_mode.  I think this is for indirect addressing, so we
   * don't need to worry about it. */
  emitByte(0);
}

void R600CodeEmitter::emitTexInstr(MachineInstr &MI)
{

  int64_t sampler = MI.getOperand(2).getImm();
  int64_t textureType = MI.getOperand(3).getImm();
  unsigned opcode = MI.getOpcode();
  unsigned srcSelect[4] = {0, 1, 2, 3};

  /* Emit instruction type */
  emitByte(1);

  /* Emit instruction */
  emitByte(getHWInst(MI));

  /* XXX: Emit resource id r600_shader.c uses sampler + 1.  Why? */
  emitByte(sampler + 1 + 1);

  /* Emit source register */
  emitByte(getHWReg(MI.getOperand(1).getReg()));

  /* XXX: Emit src isRelativeAddress */
  emitByte(0);

  /* Emit destination register */
  emitByte(getHWReg(MI.getOperand(0).getReg()));

  /* XXX: Emit dst isRealtiveAddress */
  emitByte(0);

  /* XXX: Emit dst select */
  emitByte(0); /* X */
  emitByte(1); /* Y */
  emitByte(2); /* Z */
  emitByte(3); /* W */

  /* XXX: Emit lod bias */
  emitByte(0);

  /* XXX: Emit coord types */
  unsigned coordType[4] = {1, 1, 1, 1};

  if (textureType == TEXTURE_RECT
      || textureType == TEXTURE_SHADOWRECT) {
    coordType[ELEMENT_X] = 0;
    coordType[ELEMENT_Y] = 0;
  }

  if (textureType == TEXTURE_1D_ARRAY
      || textureType == TEXTURE_SHADOW1D_ARRAY) {
    if (opcode == AMDIL::TEX_SAMPLE_C_L || opcode == AMDIL::TEX_SAMPLE_C_LB) {
      coordType[ELEMENT_Y] = 0;
    } else {
      coordType[ELEMENT_Z] = 0;
      srcSelect[ELEMENT_Z] = ELEMENT_Y;
    }
  } else if (textureType == TEXTURE_2D_ARRAY
             || textureType == TEXTURE_SHADOW2D_ARRAY) {
    coordType[ELEMENT_Z] = 0;
  }

  for (unsigned i = 0; i < 4; i++) {
    emitByte(coordType[i]);
  }

  /* XXX: Emit offsets */
  emitByte(0); /* X */
  emitByte(0); /* Y */
  emitByte(0); /* Z */
  /* There is no OFFSET_W */

  /* Emit sampler id */
  emitByte(sampler);

  /* XXX:Emit source select */
  if ((textureType == TEXTURE_SHADOW1D
      || textureType == TEXTURE_SHADOW2D
      || textureType == TEXTURE_SHADOWRECT
      || textureType == TEXTURE_SHADOW1D_ARRAY)
      && opcode != AMDIL::TEX_SAMPLE_C_L
      && opcode != AMDIL::TEX_SAMPLE_C_LB) {
    srcSelect[ELEMENT_W] = ELEMENT_Z;
  }

  for (unsigned i = 0; i < 4; i++) {
    emitByte(srcSelect[i]);
  }
}

void R600CodeEmitter::emitFCInstr(MachineInstr &MI)
{
  /* Emit instruction type */
  emitByte(INSTR_FC);

  /* Emit SRC */
  unsigned numOperands = MI.getNumOperands();
  if (numOperands > 0) {
    assert(numOperands == 1);
    emitSrc(MI.getOperand(0));
  } else {
    emitNullBytes(SRC_BYTE_COUNT);
  }

  /* Emit FC Instruction */
  enum FCInstr instr;
  switch (MI.getOpcode()) {
  case AMDIL::BREAK_LOGICALZ_f32:
    instr = FC_BREAK;
    break;
  case AMDIL::BREAK_LOGICALNZ_f32:
  case AMDIL::BREAK_LOGICALNZ_i32:
    instr = FC_BREAK_NZ_INT;
    break;
  case AMDIL::BREAK_LOGICALZ_i32:
    instr = FC_BREAK_Z_INT;
    break;
  case AMDIL::CONTINUE_LOGICALNZ_f32:
  case AMDIL::CONTINUE_LOGICALNZ_i32:
    instr = FC_CONTINUE;
    break;
  /* XXX: This assumes that all IFs will be if (x != 0).  If we add
   * optimizations this might not be the case */
  case AMDIL::IF_LOGICALNZ_f32:
  case AMDIL::IF_LOGICALNZ_i32:
    instr = FC_IF;
    break;
  case AMDIL::IF_LOGICALZ_f32:
    abort();
    break;
  case AMDIL::ELSE:
    instr = FC_ELSE;
    break;
  case AMDIL::ENDIF:
    instr = FC_ENDIF;
    break;
  case AMDIL::ENDLOOP:
    instr = FC_ENDLOOP;
    break;
  case AMDIL::WHILELOOP:
    instr = FC_BGNLOOP;
    break;
  default:
    abort();
    break;
  }
  emitByte(instr);
}

#define INSTR_FLOAT2_V(inst, hw) \
  case AMDIL:: inst##_v4f32: \
  case AMDIL:: inst##_v2f32: return HW_INST2(hw);

#define INSTR_FLOAT2_S(inst, hw) \
  case AMDIL:: inst##_f32: return HW_INST2(hw);

#define INSTR_FLOAT2(inst, hw) \
  INSTR_FLOAT2_V(inst, hw) \
  INSTR_FLOAT2_S(inst, hw)

unsigned int R600CodeEmitter::getHWInst(const MachineInstr &MI)
{

  /* XXX: Lower these to MOV before the code emitter. */
  switch (MI.getOpcode()) {
    case AMDIL::STORE_OUTPUT:
    case AMDIL::VCREATE_v4i32:
    case AMDIL::LOADCONST_i32:
    case AMDIL::LOADCONST_f32:
    case AMDIL::MOVE_v4i32:
    /* Instructons to reinterpret bits as ... */
    case AMDIL::IL_ASINT_f32:
    case AMDIL::IL_ASINT_i32:
    case AMDIL::IL_ASFLOAT_f32:
    case AMDIL::IL_ASFLOAT_i32:
      return 0x19;

  default:
    return getBinaryCodeForInstr(MI);
  }
}

void R600CodeEmitter::emitNullBytes(unsigned int byteCount)
{
  for (unsigned int i = 0; i < byteCount; i++) {
    emitByte(0);
  }
}

void R600CodeEmitter::emitByte(unsigned int byte)
{
  _OS.write((uint8_t) byte & 0xff);
}
void R600CodeEmitter::emitTwoBytes(unsigned int bytes)
{
  _OS.write((uint8_t) (bytes & 0xff));
  _OS.write((uint8_t) ((bytes >> 8) & 0xff));
}

void R600CodeEmitter::emit(uint32_t value)
{
  for (unsigned i = 0; i < 4; i++) {
    _OS.write((uint8_t) ((value >> (8 * i)) & 0xff));
  }
}

void R600CodeEmitter::emit(uint64_t value)
{
  for (unsigned i = 0; i < 8; i++) {
    emitByte((value >> (8 * i)) & 0xff);
  }
}

unsigned R600CodeEmitter::getHWReg(unsigned regNo) const
{
  unsigned hwReg;

  hwReg = TRI->getHWRegIndex(regNo);
  if (AMDIL::R600_CReg32RegClass.contains(regNo)) {
    hwReg += 512;
  }
  return hwReg;
}

uint64_t R600CodeEmitter::getMachineOpValue(const MachineInstr &MI,
                                            const MachineOperand &MO) const
{
  if (MO.isReg()) {
    return getHWReg(MO.getReg());
  } else {
    return MO.getImm();
  }
}


RegElement maskBitToElement(unsigned int maskBit)
{
  switch (maskBit) {
    case WRITE_MASK_X: return ELEMENT_X;
    case WRITE_MASK_Y: return ELEMENT_Y;
    case WRITE_MASK_Z: return ELEMENT_Z;
    case WRITE_MASK_W: return ELEMENT_W;
    default:
      assert("Invalid maskBit");
      return ELEMENT_X;
  }
}

unsigned int dstSwizzleToWriteMask(unsigned swizzle)
{
  switch(swizzle) {
  default:
  case AMDIL_DST_SWIZZLE_DEFAULT:
    return WRITE_MASK_X | WRITE_MASK_Y | WRITE_MASK_Z | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE_X___:
    return WRITE_MASK_X;
  case AMDIL_DST_SWIZZLE_XY__:
    return WRITE_MASK_X | WRITE_MASK_Y;
  case AMDIL_DST_SWIZZLE_XYZ_:
    return WRITE_MASK_X | WRITE_MASK_Y | WRITE_MASK_Z;
  case AMDIL_DST_SWIZZLE_XYZW:
    return WRITE_MASK_X | WRITE_MASK_Y | WRITE_MASK_Z | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE__Y__:
    return WRITE_MASK_Y;
  case AMDIL_DST_SWIZZLE__YZ_:
    return WRITE_MASK_Y | WRITE_MASK_Z;
  case AMDIL_DST_SWIZZLE__YZW:
    return WRITE_MASK_Y | WRITE_MASK_Z | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE___Z_:
    return WRITE_MASK_Z;
  case AMDIL_DST_SWIZZLE___ZW:
    return WRITE_MASK_Z | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE____W:
    return WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE_X_ZW:
    return WRITE_MASK_X | WRITE_MASK_Z | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE_XY_W:
    return WRITE_MASK_X | WRITE_MASK_Y | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE_X_Z_:
    return WRITE_MASK_X | WRITE_MASK_Z;
  case AMDIL_DST_SWIZZLE_X__W:
    return WRITE_MASK_X | WRITE_MASK_W;
  case AMDIL_DST_SWIZZLE__Y_W:
    return WRITE_MASK_Y | WRITE_MASK_W;
  }
}

#include "AMDILGenCodeEmitter.inc"

