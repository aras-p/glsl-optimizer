/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
 develop this 3D driver.

 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:

 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keithw@vmware.com>
  */

/** @file brw_reg.h
 *
 * This file defines struct brw_reg, which is our representation for EU
 * registers.  They're not a hardware specific format, just an abstraction
 * that intends to capture the full flexibility of the hardware registers.
 *
 * The brw_eu_emit.c layer's brw_set_dest/brw_set_src[01] functions encode
 * the abstract brw_reg type into the actual hardware instruction encoding.
 */

#ifndef BRW_REG_H
#define BRW_REG_H

#include <stdbool.h>
#include "main/imports.h"
#include "main/compiler.h"
#include "program/prog_instruction.h"
#include "brw_defines.h"

#ifdef __cplusplus
extern "C" {
#endif

/** Number of general purpose registers (VS, WM, etc) */
#define BRW_MAX_GRF 128

/**
 * First GRF used for the MRF hack.
 *
 * On gen7, MRFs are no longer used, and contiguous GRFs are used instead.  We
 * haven't converted our compiler to be aware of this, so it asks for MRFs and
 * brw_eu_emit.c quietly converts them to be accesses of the top GRFs.  The
 * register allocators have to be careful of this to avoid corrupting the "MRF"s
 * with actual GRF allocations.
 */
#define GEN7_MRF_HACK_START 112

/** Number of message register file registers */
#define BRW_MAX_MRF 16

#define BRW_SWIZZLE4(a,b,c,d) (((a)<<0) | ((b)<<2) | ((c)<<4) | ((d)<<6))
#define BRW_GET_SWZ(swz, idx) (((swz) >> ((idx)*2)) & 0x3)

#define BRW_SWIZZLE_NOOP      BRW_SWIZZLE4(0,1,2,3)
#define BRW_SWIZZLE_XYZW      BRW_SWIZZLE4(0,1,2,3)
#define BRW_SWIZZLE_XXXX      BRW_SWIZZLE4(0,0,0,0)
#define BRW_SWIZZLE_YYYY      BRW_SWIZZLE4(1,1,1,1)
#define BRW_SWIZZLE_ZZZZ      BRW_SWIZZLE4(2,2,2,2)
#define BRW_SWIZZLE_WWWW      BRW_SWIZZLE4(3,3,3,3)
#define BRW_SWIZZLE_XYXY      BRW_SWIZZLE4(0,1,0,1)
#define BRW_SWIZZLE_YZXW      BRW_SWIZZLE4(1,2,0,3)
#define BRW_SWIZZLE_ZXYW      BRW_SWIZZLE4(2,0,1,3)
#define BRW_SWIZZLE_ZWZW      BRW_SWIZZLE4(2,3,2,3)

static inline bool
brw_is_single_value_swizzle(int swiz)
{
   return (swiz == BRW_SWIZZLE_XXXX ||
           swiz == BRW_SWIZZLE_YYYY ||
           swiz == BRW_SWIZZLE_ZZZZ ||
           swiz == BRW_SWIZZLE_WWWW);
}

enum PACKED brw_reg_type {
   BRW_REGISTER_TYPE_UD = 0,
   BRW_REGISTER_TYPE_D,
   BRW_REGISTER_TYPE_UW,
   BRW_REGISTER_TYPE_W,
   BRW_REGISTER_TYPE_F,

   /** Non-immediates only: @{ */
   BRW_REGISTER_TYPE_UB,
   BRW_REGISTER_TYPE_B,
   /** @} */

   /** Immediates only: @{ */
   BRW_REGISTER_TYPE_UV,
   BRW_REGISTER_TYPE_V,
   BRW_REGISTER_TYPE_VF,
   /** @} */

   BRW_REGISTER_TYPE_DF, /* Gen7+ (no immediates until Gen8+) */

   /* Gen8+ */
   BRW_REGISTER_TYPE_HF,
   BRW_REGISTER_TYPE_UQ,
   BRW_REGISTER_TYPE_Q,
};

unsigned brw_reg_type_to_hw_type(const struct brw_context *brw,
                                 enum brw_reg_type type, unsigned file);
const char *brw_reg_type_letters(unsigned brw_reg_type);

#define REG_SIZE (8*4)

/* These aren't hardware structs, just something useful for us to pass around:
 *
 * Align1 operation has a lot of control over input ranges.  Used in
 * WM programs to implement shaders decomposed into "channel serial"
 * or "structure of array" form:
 */
struct brw_reg {
   unsigned type:4;
   unsigned file:2;
   unsigned nr:8;
   unsigned subnr:5;              /* :1 in align16 */
   unsigned negate:1;             /* source only */
   unsigned abs:1;                /* source only */
   unsigned vstride:4;            /* source only */
   unsigned width:3;              /* src only, align1 only */
   unsigned hstride:2;            /* align1 only */
   unsigned address_mode:1;       /* relative addressing, hopefully! */
   unsigned pad0:1;

   union {
      struct {
         unsigned swizzle:8;      /* src only, align16 only */
         unsigned writemask:4;    /* dest only, align16 only */
         int  indirect_offset:10; /* relative addressing offset */
         unsigned pad1:10;        /* two dwords total */
      } bits;

      float f;
      int   d;
      unsigned ud;
   } dw1;
};


struct brw_indirect {
   unsigned addr_subnr:4;
   int addr_offset:10;
   unsigned pad:18;
};


static inline int
type_sz(unsigned type)
{
   switch(type) {
   case BRW_REGISTER_TYPE_UD:
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_F:
      return 4;
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_W:
      return 2;
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_B:
      return 1;
   default:
      return 0;
   }
}

static inline bool
type_is_signed(unsigned type)
{
   switch(type) {
   case BRW_REGISTER_TYPE_D:
   case BRW_REGISTER_TYPE_W:
   case BRW_REGISTER_TYPE_F:
   case BRW_REGISTER_TYPE_B:
   case BRW_REGISTER_TYPE_V:
   case BRW_REGISTER_TYPE_VF:
   case BRW_REGISTER_TYPE_DF:
   case BRW_REGISTER_TYPE_HF:
   case BRW_REGISTER_TYPE_Q:
      return true;

   case BRW_REGISTER_TYPE_UD:
   case BRW_REGISTER_TYPE_UW:
   case BRW_REGISTER_TYPE_UB:
   case BRW_REGISTER_TYPE_UV:
   case BRW_REGISTER_TYPE_UQ:
      return false;

   default:
      assert(!"Unreachable.");
      return false;
   }
}

/**
 * Construct a brw_reg.
 * \param file      one of the BRW_x_REGISTER_FILE values
 * \param nr        register number/index
 * \param subnr     register sub number
 * \param type      one of BRW_REGISTER_TYPE_x
 * \param vstride   one of BRW_VERTICAL_STRIDE_x
 * \param width     one of BRW_WIDTH_x
 * \param hstride   one of BRW_HORIZONTAL_STRIDE_x
 * \param swizzle   one of BRW_SWIZZLE_x
 * \param writemask WRITEMASK_X/Y/Z/W bitfield
 */
static inline struct brw_reg
brw_reg(unsigned file,
        unsigned nr,
        unsigned subnr,
        unsigned type,
        unsigned vstride,
        unsigned width,
        unsigned hstride,
        unsigned swizzle,
        unsigned writemask)
{
   struct brw_reg reg;
   if (file == BRW_GENERAL_REGISTER_FILE)
      assert(nr < BRW_MAX_GRF);
   else if (file == BRW_MESSAGE_REGISTER_FILE)
      assert((nr & ~(1 << 7)) < BRW_MAX_MRF);
   else if (file == BRW_ARCHITECTURE_REGISTER_FILE)
      assert(nr <= BRW_ARF_TIMESTAMP);

   reg.type = type;
   reg.file = file;
   reg.nr = nr;
   reg.subnr = subnr * type_sz(type);
   reg.negate = 0;
   reg.abs = 0;
   reg.vstride = vstride;
   reg.width = width;
   reg.hstride = hstride;
   reg.address_mode = BRW_ADDRESS_DIRECT;
   reg.pad0 = 0;

   /* Could do better: If the reg is r5.3<0;1,0>, we probably want to
    * set swizzle and writemask to W, as the lower bits of subnr will
    * be lost when converted to align16.  This is probably too much to
    * keep track of as you'd want it adjusted by suboffset(), etc.
    * Perhaps fix up when converting to align16?
    */
   reg.dw1.bits.swizzle = swizzle;
   reg.dw1.bits.writemask = writemask;
   reg.dw1.bits.indirect_offset = 0;
   reg.dw1.bits.pad1 = 0;
   return reg;
}

/** Construct float[16] register */
static inline struct brw_reg
brw_vec16_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return brw_reg(file,
                  nr,
                  subnr,
                  BRW_REGISTER_TYPE_F,
                  BRW_VERTICAL_STRIDE_16,
                  BRW_WIDTH_16,
                  BRW_HORIZONTAL_STRIDE_1,
                  BRW_SWIZZLE_XYZW,
                  WRITEMASK_XYZW);
}

/** Construct float[8] register */
static inline struct brw_reg
brw_vec8_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return brw_reg(file,
                  nr,
                  subnr,
                  BRW_REGISTER_TYPE_F,
                  BRW_VERTICAL_STRIDE_8,
                  BRW_WIDTH_8,
                  BRW_HORIZONTAL_STRIDE_1,
                  BRW_SWIZZLE_XYZW,
                  WRITEMASK_XYZW);
}

/** Construct float[4] register */
static inline struct brw_reg
brw_vec4_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return brw_reg(file,
                  nr,
                  subnr,
                  BRW_REGISTER_TYPE_F,
                  BRW_VERTICAL_STRIDE_4,
                  BRW_WIDTH_4,
                  BRW_HORIZONTAL_STRIDE_1,
                  BRW_SWIZZLE_XYZW,
                  WRITEMASK_XYZW);
}

/** Construct float[2] register */
static inline struct brw_reg
brw_vec2_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return brw_reg(file,
                  nr,
                  subnr,
                  BRW_REGISTER_TYPE_F,
                  BRW_VERTICAL_STRIDE_2,
                  BRW_WIDTH_2,
                  BRW_HORIZONTAL_STRIDE_1,
                  BRW_SWIZZLE_XYXY,
                  WRITEMASK_XY);
}

/** Construct float[1] register */
static inline struct brw_reg
brw_vec1_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return brw_reg(file,
                  nr,
                  subnr,
                  BRW_REGISTER_TYPE_F,
                  BRW_VERTICAL_STRIDE_0,
                  BRW_WIDTH_1,
                  BRW_HORIZONTAL_STRIDE_0,
                  BRW_SWIZZLE_XXXX,
                  WRITEMASK_X);
}

static inline struct brw_reg
brw_vecn_reg(unsigned width, unsigned file, unsigned nr, unsigned subnr)
{
   switch (width) {
   case 1:
      return brw_vec1_reg(file, nr, subnr);
   case 2:
      return brw_vec2_reg(file, nr, subnr);
   case 4:
      return brw_vec4_reg(file, nr, subnr);
   case 8:
      return brw_vec8_reg(file, nr, subnr);
   case 16:
      return brw_vec16_reg(file, nr, subnr);
   default:
      assert(!"Invalid register width");
   }
   unreachable();
}

static inline struct brw_reg
retype(struct brw_reg reg, unsigned type)
{
   reg.type = type;
   return reg;
}

static inline struct brw_reg
sechalf(struct brw_reg reg)
{
   if (reg.vstride)
      reg.nr++;
   return reg;
}

static inline struct brw_reg
suboffset(struct brw_reg reg, unsigned delta)
{
   reg.subnr += delta * type_sz(reg.type);
   return reg;
}


static inline struct brw_reg
offset(struct brw_reg reg, unsigned delta)
{
   reg.nr += delta;
   return reg;
}


static inline struct brw_reg
byte_offset(struct brw_reg reg, unsigned bytes)
{
   unsigned newoffset = reg.nr * REG_SIZE + reg.subnr + bytes;
   reg.nr = newoffset / REG_SIZE;
   reg.subnr = newoffset % REG_SIZE;
   return reg;
}


/** Construct unsigned word[16] register */
static inline struct brw_reg
brw_uw16_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return suboffset(retype(brw_vec16_reg(file, nr, 0), BRW_REGISTER_TYPE_UW), subnr);
}

/** Construct unsigned word[8] register */
static inline struct brw_reg
brw_uw8_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return suboffset(retype(brw_vec8_reg(file, nr, 0), BRW_REGISTER_TYPE_UW), subnr);
}

/** Construct unsigned word[1] register */
static inline struct brw_reg
brw_uw1_reg(unsigned file, unsigned nr, unsigned subnr)
{
   return suboffset(retype(brw_vec1_reg(file, nr, 0), BRW_REGISTER_TYPE_UW), subnr);
}

static inline struct brw_reg
brw_imm_reg(unsigned type)
{
   return brw_reg(BRW_IMMEDIATE_VALUE,
                  0,
                  0,
                  type,
                  BRW_VERTICAL_STRIDE_0,
                  BRW_WIDTH_1,
                  BRW_HORIZONTAL_STRIDE_0,
                  0,
                  0);
}

/** Construct float immediate register */
static inline struct brw_reg
brw_imm_f(float f)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_F);
   imm.dw1.f = f;
   return imm;
}

/** Construct integer immediate register */
static inline struct brw_reg
brw_imm_d(int d)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_D);
   imm.dw1.d = d;
   return imm;
}

/** Construct uint immediate register */
static inline struct brw_reg
brw_imm_ud(unsigned ud)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_UD);
   imm.dw1.ud = ud;
   return imm;
}

/** Construct ushort immediate register */
static inline struct brw_reg
brw_imm_uw(uint16_t uw)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_UW);
   imm.dw1.ud = uw | (uw << 16);
   return imm;
}

/** Construct short immediate register */
static inline struct brw_reg
brw_imm_w(int16_t w)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_W);
   imm.dw1.d = w | (w << 16);
   return imm;
}

/* brw_imm_b and brw_imm_ub aren't supported by hardware - the type
 * numbers alias with _V and _VF below:
 */

/** Construct vector of eight signed half-byte values */
static inline struct brw_reg
brw_imm_v(unsigned v)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_V);
   imm.vstride = BRW_VERTICAL_STRIDE_0;
   imm.width = BRW_WIDTH_8;
   imm.hstride = BRW_HORIZONTAL_STRIDE_1;
   imm.dw1.ud = v;
   return imm;
}

/** Construct vector of four 8-bit float values */
static inline struct brw_reg
brw_imm_vf(unsigned v)
{
   struct brw_reg imm = brw_imm_reg(BRW_REGISTER_TYPE_VF);
   imm.vstride = BRW_VERTICAL_STRIDE_0;
   imm.width = BRW_WIDTH_4;
   imm.hstride = BRW_HORIZONTAL_STRIDE_1;
   imm.dw1.ud = v;
   return imm;
}

/**
 * Convert an integer into a "restricted" 8-bit float, used in vector
 * immediates.  The 8-bit floating point format has a sign bit, an
 * excess-3 3-bit exponent, and a 4-bit mantissa.  All integer values
 * from -31 to 31 can be represented exactly.
 */
static inline uint8_t
int_to_float8(int x)
{
   if (x == 0) {
      return 0;
   } else if (x < 0) {
      return 1 << 7 | int_to_float8(-x);
   } else {
      const unsigned exponent = _mesa_logbase2(x);
      const unsigned mantissa = (x - (1 << exponent)) << (4 - exponent);
      assert(exponent <= 4);
      return (exponent + 3) << 4 | mantissa;
   }
}

/**
 * Construct a floating-point packed vector immediate from its integer
 * values. \sa int_to_float8()
 */
static inline struct brw_reg
brw_imm_vf4(int v0, int v1, int v2, int v3)
{
   return brw_imm_vf((int_to_float8(v0) << 0) |
                     (int_to_float8(v1) << 8) |
                     (int_to_float8(v2) << 16) |
                     (int_to_float8(v3) << 24));
}


static inline struct brw_reg
brw_address(struct brw_reg reg)
{
   return brw_imm_uw(reg.nr * REG_SIZE + reg.subnr);
}

/** Construct float[1] general-purpose register */
static inline struct brw_reg
brw_vec1_grf(unsigned nr, unsigned subnr)
{
   return brw_vec1_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

/** Construct float[2] general-purpose register */
static inline struct brw_reg
brw_vec2_grf(unsigned nr, unsigned subnr)
{
   return brw_vec2_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

/** Construct float[4] general-purpose register */
static inline struct brw_reg
brw_vec4_grf(unsigned nr, unsigned subnr)
{
   return brw_vec4_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

/** Construct float[8] general-purpose register */
static inline struct brw_reg
brw_vec8_grf(unsigned nr, unsigned subnr)
{
   return brw_vec8_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}


static inline struct brw_reg
brw_uw8_grf(unsigned nr, unsigned subnr)
{
   return brw_uw8_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}

static inline struct brw_reg
brw_uw16_grf(unsigned nr, unsigned subnr)
{
   return brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, nr, subnr);
}


/** Construct null register (usually used for setting condition codes) */
static inline struct brw_reg
brw_null_reg(void)
{
   return brw_vec8_reg(BRW_ARCHITECTURE_REGISTER_FILE, BRW_ARF_NULL, 0);
}

static inline struct brw_reg
brw_address_reg(unsigned subnr)
{
   return brw_uw1_reg(BRW_ARCHITECTURE_REGISTER_FILE, BRW_ARF_ADDRESS, subnr);
}

/* If/else instructions break in align16 mode if writemask & swizzle
 * aren't xyzw.  This goes against the convention for other scalar
 * regs:
 */
static inline struct brw_reg
brw_ip_reg(void)
{
   return brw_reg(BRW_ARCHITECTURE_REGISTER_FILE,
                  BRW_ARF_IP,
                  0,
                  BRW_REGISTER_TYPE_UD,
                  BRW_VERTICAL_STRIDE_4, /* ? */
                  BRW_WIDTH_1,
                  BRW_HORIZONTAL_STRIDE_0,
                  BRW_SWIZZLE_XYZW, /* NOTE! */
                  WRITEMASK_XYZW); /* NOTE! */
}

static inline struct brw_reg
brw_acc_reg(void)
{
   return brw_vec8_reg(BRW_ARCHITECTURE_REGISTER_FILE, BRW_ARF_ACCUMULATOR, 0);
}

static inline struct brw_reg
brw_notification_1_reg(void)
{

   return brw_reg(BRW_ARCHITECTURE_REGISTER_FILE,
                  BRW_ARF_NOTIFICATION_COUNT,
                  1,
                  BRW_REGISTER_TYPE_UD,
                  BRW_VERTICAL_STRIDE_0,
                  BRW_WIDTH_1,
                  BRW_HORIZONTAL_STRIDE_0,
                  BRW_SWIZZLE_XXXX,
                  WRITEMASK_X);
}


static inline struct brw_reg
brw_flag_reg(int reg, int subreg)
{
   return brw_uw1_reg(BRW_ARCHITECTURE_REGISTER_FILE,
                      BRW_ARF_FLAG + reg, subreg);
}


static inline struct brw_reg
brw_mask_reg(unsigned subnr)
{
   return brw_uw1_reg(BRW_ARCHITECTURE_REGISTER_FILE, BRW_ARF_MASK, subnr);
}

static inline struct brw_reg
brw_message_reg(unsigned nr)
{
   assert((nr & ~(1 << 7)) < BRW_MAX_MRF);
   return brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, nr, 0);
}

static inline struct brw_reg
brw_uvec_mrf(unsigned width, unsigned nr, unsigned subnr)
{
   return retype(brw_vecn_reg(width, BRW_MESSAGE_REGISTER_FILE, nr, subnr),
                 BRW_REGISTER_TYPE_UD);
}

/* This is almost always called with a numeric constant argument, so
 * make things easy to evaluate at compile time:
 */
static inline unsigned cvt(unsigned val)
{
   switch (val) {
   case 0: return 0;
   case 1: return 1;
   case 2: return 2;
   case 4: return 3;
   case 8: return 4;
   case 16: return 5;
   case 32: return 6;
   }
   return 0;
}

static inline struct brw_reg
stride(struct brw_reg reg, unsigned vstride, unsigned width, unsigned hstride)
{
   reg.vstride = cvt(vstride);
   reg.width = cvt(width) - 1;
   reg.hstride = cvt(hstride);
   return reg;
}


static inline struct brw_reg
vec16(struct brw_reg reg)
{
   return stride(reg, 16,16,1);
}

static inline struct brw_reg
vec8(struct brw_reg reg)
{
   return stride(reg, 8,8,1);
}

static inline struct brw_reg
vec4(struct brw_reg reg)
{
   return stride(reg, 4,4,1);
}

static inline struct brw_reg
vec2(struct brw_reg reg)
{
   return stride(reg, 2,2,1);
}

static inline struct brw_reg
vec1(struct brw_reg reg)
{
   return stride(reg, 0,1,0);
}


static inline struct brw_reg
get_element(struct brw_reg reg, unsigned elt)
{
   return vec1(suboffset(reg, elt));
}

static inline struct brw_reg
get_element_ud(struct brw_reg reg, unsigned elt)
{
   return vec1(suboffset(retype(reg, BRW_REGISTER_TYPE_UD), elt));
}

static inline struct brw_reg
get_element_d(struct brw_reg reg, unsigned elt)
{
   return vec1(suboffset(retype(reg, BRW_REGISTER_TYPE_D), elt));
}


static inline struct brw_reg
brw_swizzle(struct brw_reg reg, unsigned x, unsigned y, unsigned z, unsigned w)
{
   assert(reg.file != BRW_IMMEDIATE_VALUE);

   reg.dw1.bits.swizzle = BRW_SWIZZLE4(BRW_GET_SWZ(reg.dw1.bits.swizzle, x),
                                       BRW_GET_SWZ(reg.dw1.bits.swizzle, y),
                                       BRW_GET_SWZ(reg.dw1.bits.swizzle, z),
                                       BRW_GET_SWZ(reg.dw1.bits.swizzle, w));
   return reg;
}


static inline struct brw_reg
brw_swizzle1(struct brw_reg reg, unsigned x)
{
   return brw_swizzle(reg, x, x, x, x);
}

static inline struct brw_reg
brw_writemask(struct brw_reg reg, unsigned mask)
{
   assert(reg.file != BRW_IMMEDIATE_VALUE);
   reg.dw1.bits.writemask &= mask;
   return reg;
}

static inline struct brw_reg
brw_set_writemask(struct brw_reg reg, unsigned mask)
{
   assert(reg.file != BRW_IMMEDIATE_VALUE);
   reg.dw1.bits.writemask = mask;
   return reg;
}

static inline struct brw_reg
negate(struct brw_reg reg)
{
   reg.negate ^= 1;
   return reg;
}

static inline struct brw_reg
brw_abs(struct brw_reg reg)
{
   reg.abs = 1;
   reg.negate = 0;
   return reg;
}

/************************************************************************/

static inline struct brw_reg
brw_vec4_indirect(unsigned subnr, int offset)
{
   struct brw_reg reg =  brw_vec4_grf(0, 0);
   reg.subnr = subnr;
   reg.address_mode = BRW_ADDRESS_REGISTER_INDIRECT_REGISTER;
   reg.dw1.bits.indirect_offset = offset;
   return reg;
}

static inline struct brw_reg
brw_vec1_indirect(unsigned subnr, int offset)
{
   struct brw_reg reg =  brw_vec1_grf(0, 0);
   reg.subnr = subnr;
   reg.address_mode = BRW_ADDRESS_REGISTER_INDIRECT_REGISTER;
   reg.dw1.bits.indirect_offset = offset;
   return reg;
}

static inline struct brw_reg
deref_4f(struct brw_indirect ptr, int offset)
{
   return brw_vec4_indirect(ptr.addr_subnr, ptr.addr_offset + offset);
}

static inline struct brw_reg
deref_1f(struct brw_indirect ptr, int offset)
{
   return brw_vec1_indirect(ptr.addr_subnr, ptr.addr_offset + offset);
}

static inline struct brw_reg
deref_4b(struct brw_indirect ptr, int offset)
{
   return retype(deref_4f(ptr, offset), BRW_REGISTER_TYPE_B);
}

static inline struct brw_reg
deref_1uw(struct brw_indirect ptr, int offset)
{
   return retype(deref_1f(ptr, offset), BRW_REGISTER_TYPE_UW);
}

static inline struct brw_reg
deref_1d(struct brw_indirect ptr, int offset)
{
   return retype(deref_1f(ptr, offset), BRW_REGISTER_TYPE_D);
}

static inline struct brw_reg
deref_1ud(struct brw_indirect ptr, int offset)
{
   return retype(deref_1f(ptr, offset), BRW_REGISTER_TYPE_UD);
}

static inline struct brw_reg
get_addr_reg(struct brw_indirect ptr)
{
   return brw_address_reg(ptr.addr_subnr);
}

static inline struct brw_indirect
brw_indirect_offset(struct brw_indirect ptr, int offset)
{
   ptr.addr_offset += offset;
   return ptr;
}

static inline struct brw_indirect
brw_indirect(unsigned addr_subnr, int offset)
{
   struct brw_indirect ptr;
   ptr.addr_subnr = addr_subnr;
   ptr.addr_offset = offset;
   ptr.pad = 0;
   return ptr;
}

#ifdef __cplusplus
}
#endif

#endif
