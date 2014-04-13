/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef TOY_REG_H
#define TOY_REG_H

#include "pipe/p_compiler.h"
#include "util/u_debug.h" /* for assert() */
#include "util/u_math.h" /* for union fi */

/* a toy reg is 256-bit wide */
#define TOY_REG_WIDTH        32

/**
 * Register files.
 */
enum toy_file {
   /* virtual register file */
   TOY_FILE_VRF,

   TOY_FILE_ARF,
   TOY_FILE_GRF,
   TOY_FILE_MRF,
   TOY_FILE_IMM,

   TOY_FILE_COUNT,
};

/**
 * Register types.
 */
enum toy_type {
   TOY_TYPE_F,
   TOY_TYPE_D,
   TOY_TYPE_UD,
   TOY_TYPE_W,
   TOY_TYPE_UW,
   TOY_TYPE_V, /* only valid for immediates */

   TOY_TYPE_COUNT,
};

/**
 * Register rectangles.  The three numbers stand for vertical stride, width,
 * and horizontal stride respectively.
 */
enum toy_rect {
   TOY_RECT_LINEAR,
   TOY_RECT_041,
   TOY_RECT_010,
   TOY_RECT_220,
   TOY_RECT_440,
   TOY_RECT_240,

   TOY_RECT_COUNT,
};

/**
 * Source swizzles.  They are compatible with TGSI_SWIZZLE_x and hardware
 * values.
 */
enum toy_swizzle {
   TOY_SWIZZLE_X = 0,
   TOY_SWIZZLE_Y = 1,
   TOY_SWIZZLE_Z = 2,
   TOY_SWIZZLE_W = 3,
};

/**
 * Destination writemasks.  They are compatible with TGSI_WRITEMASK_x and
 * hardware values.
 */
enum toy_writemask {
   TOY_WRITEMASK_X    = (1 << TOY_SWIZZLE_X),
   TOY_WRITEMASK_Y    = (1 << TOY_SWIZZLE_Y),
   TOY_WRITEMASK_Z    = (1 << TOY_SWIZZLE_Z),
   TOY_WRITEMASK_W    = (1 << TOY_SWIZZLE_W),
   TOY_WRITEMASK_XY   = (TOY_WRITEMASK_X | TOY_WRITEMASK_Y),
   TOY_WRITEMASK_XZ   = (TOY_WRITEMASK_X | TOY_WRITEMASK_Z),
   TOY_WRITEMASK_XW   = (TOY_WRITEMASK_X | TOY_WRITEMASK_W),
   TOY_WRITEMASK_YZ   = (TOY_WRITEMASK_Y | TOY_WRITEMASK_Z),
   TOY_WRITEMASK_YW   = (TOY_WRITEMASK_Y | TOY_WRITEMASK_W),
   TOY_WRITEMASK_ZW   = (TOY_WRITEMASK_Z | TOY_WRITEMASK_W),
   TOY_WRITEMASK_XYZ  = (TOY_WRITEMASK_X | TOY_WRITEMASK_Y | TOY_WRITEMASK_Z),
   TOY_WRITEMASK_XYW  = (TOY_WRITEMASK_X | TOY_WRITEMASK_Y | TOY_WRITEMASK_W),
   TOY_WRITEMASK_XZW  = (TOY_WRITEMASK_X | TOY_WRITEMASK_Z | TOY_WRITEMASK_W),
   TOY_WRITEMASK_YZW  = (TOY_WRITEMASK_Y | TOY_WRITEMASK_Z | TOY_WRITEMASK_W),
   TOY_WRITEMASK_XYZW = (TOY_WRITEMASK_X | TOY_WRITEMASK_Y |
                         TOY_WRITEMASK_Z | TOY_WRITEMASK_W),
};

/**
 * Destination operand.
 */
struct toy_dst {
   unsigned file:3;              /* TOY_FILE_x */
   unsigned type:3;              /* TOY_TYPE_x */
   unsigned rect:3;              /* TOY_RECT_x */
   unsigned indirect:1;          /* true or false */
   unsigned indirect_subreg:6;   /* which subreg of a0? */

   unsigned writemask:4;         /* TOY_WRITEMASK_x */
   unsigned pad:12;

   uint32_t val32;
};

/**
 * Source operand.
 */
struct toy_src {
   unsigned file:3;              /* TOY_FILE_x */
   unsigned type:3;              /* TOY_TYPE_x */
   unsigned rect:3;              /* TOY_RECT_x */
   unsigned indirect:1;          /* true or false */
   unsigned indirect_subreg:6;   /* which subreg of a0? */

   unsigned swizzle_x:2;         /* TOY_SWIZZLE_x */
   unsigned swizzle_y:2;         /* TOY_SWIZZLE_x */
   unsigned swizzle_z:2;         /* TOY_SWIZZLE_x */
   unsigned swizzle_w:2;         /* TOY_SWIZZLE_x */
   unsigned absolute:1;          /* true or false */
   unsigned negate:1;            /* true or false */
   unsigned pad:6;

   uint32_t val32;
};

/**
 * Return true if the file is virtual.
 */
static inline bool
toy_file_is_virtual(enum toy_file file)
{
   return (file == TOY_FILE_VRF);
}

/**
 * Return true if the file is a hardware one.
 */
static inline bool
toy_file_is_hw(enum toy_file file)
{
   return !toy_file_is_virtual(file);
}

/**
 * Return the size of the file.
 */
static inline uint32_t
toy_file_size(enum toy_file file)
{
   switch (file) {
   case TOY_FILE_GRF:
      return 256 * TOY_REG_WIDTH;
   case TOY_FILE_MRF:
      /* there is no MRF on GEN7+ */
      return 256 * TOY_REG_WIDTH;
   default:
      assert(!"invalid toy file");
      return 0;
   }
}

/**
 * Return the size of the type.
 */
static inline int
toy_type_size(enum toy_type type)
{
   switch (type) {
   case TOY_TYPE_F:
   case TOY_TYPE_D:
   case TOY_TYPE_UD:
      return 4;
   case TOY_TYPE_W:
   case TOY_TYPE_UW:
      return 2;
   case TOY_TYPE_V:
   default:
      assert(!"invalid toy type");
      return 0;
   }
}

/**
 * Return true if the destination operand is null.
 */
static inline bool
tdst_is_null(struct toy_dst dst)
{
   /* GEN6_ARF_NULL happens to be 0 */
   return (dst.file == TOY_FILE_ARF && dst.val32 == 0);
}

/**
 * Validate the destination operand.
 */
static inline struct toy_dst
tdst_validate(struct toy_dst dst)
{
   switch (dst.file) {
   case TOY_FILE_VRF:
   case TOY_FILE_ARF:
   case TOY_FILE_MRF:
      assert(!dst.indirect);
      if (dst.file == TOY_FILE_MRF)
         assert(dst.val32 < toy_file_size(dst.file));
      break;
   case TOY_FILE_GRF:
      if (!dst.indirect)
         assert(dst.val32 < toy_file_size(dst.file));
      break;
   case TOY_FILE_IMM:
      /* yes, dst can be IMM of type W (for IF/ELSE/ENDIF/WHILE) */
      assert(!dst.indirect);
      assert(dst.type == TOY_TYPE_W);
      break;
   default:
      assert(!"invalid dst file");
      break;
   }

   switch (dst.type) {
   case TOY_TYPE_V:
      assert(!"invalid dst type");
      break;
   default:
      break;
   }

   assert(dst.rect == TOY_RECT_LINEAR);
   if (dst.file != TOY_FILE_IMM)
      assert(dst.val32 % toy_type_size(dst.type) == 0);

   assert(dst.writemask <= TOY_WRITEMASK_XYZW);

   return dst;
}

/**
 * Change the type of the destination operand.
 */
static inline struct toy_dst
tdst_type(struct toy_dst dst, enum toy_type type)
{
   dst.type = type;
   return tdst_validate(dst);
}

/**
 * Change the type of the destination operand to TOY_TYPE_D.
 */
static inline struct toy_dst
tdst_d(struct toy_dst dst)
{
   return tdst_type(dst, TOY_TYPE_D);
}

/**
 * Change the type of the destination operand to TOY_TYPE_UD.
 */
static inline struct toy_dst
tdst_ud(struct toy_dst dst)
{
   return tdst_type(dst, TOY_TYPE_UD);
}

/**
 * Change the type of the destination operand to TOY_TYPE_W.
 */
static inline struct toy_dst
tdst_w(struct toy_dst dst)
{
   return tdst_type(dst, TOY_TYPE_W);
}

/**
 * Change the type of the destination operand to TOY_TYPE_UW.
 */
static inline struct toy_dst
tdst_uw(struct toy_dst dst)
{
   return tdst_type(dst, TOY_TYPE_UW);
}

/**
 * Change the rectangle of the destination operand.
 */
static inline struct toy_dst
tdst_rect(struct toy_dst dst, enum toy_rect rect)
{
   dst.rect = rect;
   return tdst_validate(dst);
}

/**
 * Apply writemask to the destination operand.  Note that the current
 * writemask is honored.
 */
static inline struct toy_dst
tdst_writemask(struct toy_dst dst, enum toy_writemask writemask)
{
   dst.writemask &= writemask;
   return tdst_validate(dst);
}

/**
 * Offset the destination operand.
 */
static inline struct toy_dst
tdst_offset(struct toy_dst dst, int reg, int subreg)
{
   dst.val32 += reg * TOY_REG_WIDTH + subreg * toy_type_size(dst.type);
   return tdst_validate(dst);
}

/**
 * Construct a destination operand.
 */
static inline struct toy_dst
tdst_full(enum toy_file file, enum toy_type type, enum toy_rect rect,
          bool indirect, unsigned indirect_subreg,
          enum toy_writemask writemask, uint32_t val32)
{
   struct toy_dst dst;

   dst.file = file;
   dst.type = type;
   dst.rect = rect;
   dst.indirect = indirect;
   dst.indirect_subreg = indirect_subreg;
   dst.writemask = writemask;
   dst.pad = 0;

   dst.val32 = val32;

   return tdst_validate(dst);
}

/**
 * Construct a null destination operand.
 */
static inline struct toy_dst
tdst_null(void)
{
   static const struct toy_dst null_dst = {
      .file = TOY_FILE_ARF,
      .type = TOY_TYPE_F,
      .rect = TOY_RECT_LINEAR,
      .indirect = false,
      .indirect_subreg = 0,
      .writemask = TOY_WRITEMASK_XYZW,
      .pad = 0,
      .val32 = 0,
   };

   return null_dst;
}

/**
 * Construct a destination operand from a source operand.
 */
static inline struct toy_dst
tdst_from(struct toy_src src)
{
   const enum toy_writemask writemask =
      (1 << src.swizzle_x) |
      (1 << src.swizzle_y) |
      (1 << src.swizzle_z) |
      (1 << src.swizzle_w);

   return tdst_full(src.file, src.type, src.rect,
         src.indirect, src.indirect_subreg, writemask, src.val32);
}

/**
 * Construct a destination operand, assuming the type is TOY_TYPE_F, the
 * rectangle is TOY_RECT_LINEAR, and the writemask is TOY_WRITEMASK_XYZW.
 */
static inline struct toy_dst
tdst(enum toy_file file, unsigned reg, unsigned subreg_in_bytes)
{
   const enum toy_type type = TOY_TYPE_F;
   const enum toy_rect rect = TOY_RECT_LINEAR;
   const uint32_t val32 = reg * TOY_REG_WIDTH + subreg_in_bytes;

   return tdst_full(file, type, rect,
         false, 0, TOY_WRITEMASK_XYZW, val32);
}

/**
 * Construct an immediate destination operand of type TOY_TYPE_W.
 */
static inline struct toy_dst
tdst_imm_w(int16_t w)
{
   const union fi fi = { .i = w };

   return tdst_full(TOY_FILE_IMM, TOY_TYPE_W, TOY_RECT_LINEAR,
         false, 0, TOY_WRITEMASK_XYZW, fi.ui);
}

/**
 * Return true if the source operand is null.
 */
static inline bool
tsrc_is_null(struct toy_src src)
{
   /* GEN6_ARF_NULL happens to be 0 */
   return (src.file == TOY_FILE_ARF && src.val32 == 0);
}

/**
 * Return true if the source operand is swizzled.
 */
static inline bool
tsrc_is_swizzled(struct toy_src src)
{
   return (src.swizzle_x != TOY_SWIZZLE_X ||
           src.swizzle_y != TOY_SWIZZLE_Y ||
           src.swizzle_z != TOY_SWIZZLE_Z ||
           src.swizzle_w != TOY_SWIZZLE_W);
}

/**
 * Return true if the source operand is swizzled to the same channel.
 */
static inline bool
tsrc_is_swizzle1(struct toy_src src)
{
   return (src.swizzle_x == src.swizzle_y &&
           src.swizzle_x == src.swizzle_z &&
           src.swizzle_x == src.swizzle_w);
}

/**
 * Validate the source operand.
 */
static inline struct toy_src
tsrc_validate(struct toy_src src)
{
   switch (src.file) {
   case TOY_FILE_VRF:
   case TOY_FILE_ARF:
   case TOY_FILE_MRF:
      assert(!src.indirect);
      if (src.file == TOY_FILE_MRF)
         assert(src.val32 < toy_file_size(src.file));
      break;
   case TOY_FILE_GRF:
      if (!src.indirect)
         assert(src.val32 < toy_file_size(src.file));
      break;
   case TOY_FILE_IMM:
      assert(!src.indirect);
      break;
   default:
      assert(!"invalid src file");
      break;
   }

   switch (src.type) {
   case TOY_TYPE_V:
      assert(src.file == TOY_FILE_IMM);
      break;
   default:
      break;
   }

   if (src.file != TOY_FILE_IMM)
      assert(src.val32 % toy_type_size(src.type) == 0);

   assert(src.swizzle_x < 4 && src.swizzle_y < 4 &&
          src.swizzle_z < 4 && src.swizzle_w < 4);

   return src;
}

/**
 * Change the type of the source operand.
 */
static inline struct toy_src
tsrc_type(struct toy_src src, enum toy_type type)
{
   src.type = type;
   return tsrc_validate(src);
}

/**
 * Change the type of the source operand to TOY_TYPE_D.
 */
static inline struct toy_src
tsrc_d(struct toy_src src)
{
   return tsrc_type(src, TOY_TYPE_D);
}

/**
 * Change the type of the source operand to TOY_TYPE_UD.
 */
static inline struct toy_src
tsrc_ud(struct toy_src src)
{
   return tsrc_type(src, TOY_TYPE_UD);
}

/**
 * Change the type of the source operand to TOY_TYPE_W.
 */
static inline struct toy_src
tsrc_w(struct toy_src src)
{
   return tsrc_type(src, TOY_TYPE_W);
}

/**
 * Change the type of the source operand to TOY_TYPE_UW.
 */
static inline struct toy_src
tsrc_uw(struct toy_src src)
{
   return tsrc_type(src, TOY_TYPE_UW);
}

/**
 * Change the rectangle of the source operand.
 */
static inline struct toy_src
tsrc_rect(struct toy_src src, enum toy_rect rect)
{
   src.rect = rect;
   return tsrc_validate(src);
}

/**
 * Swizzle the source operand.  Note that the current swizzles are honored.
 */
static inline struct toy_src
tsrc_swizzle(struct toy_src src,
             enum toy_swizzle swizzle_x, enum toy_swizzle swizzle_y,
             enum toy_swizzle swizzle_z, enum toy_swizzle swizzle_w)
{
   const enum toy_swizzle current[4] = {
      src.swizzle_x, src.swizzle_y,
      src.swizzle_z, src.swizzle_w,
   };

   src.swizzle_x = current[swizzle_x];
   src.swizzle_y = current[swizzle_y];
   src.swizzle_z = current[swizzle_z];
   src.swizzle_w = current[swizzle_w];

   return tsrc_validate(src);
}

/**
 * Swizzle the source operand to the same channel.  Note that the current
 * swizzles are honored.
 */
static inline struct toy_src
tsrc_swizzle1(struct toy_src src, enum toy_swizzle swizzle)
{
   return tsrc_swizzle(src, swizzle, swizzle, swizzle, swizzle);
}

/**
 * Set absolute and unset negate of the source operand.
 */
static inline struct toy_src
tsrc_absolute(struct toy_src src)
{
   src.absolute = true;
   src.negate = false;
   return tsrc_validate(src);
}

/**
 * Negate the source operand.
 */
static inline struct toy_src
tsrc_negate(struct toy_src src)
{
   src.negate = !src.negate;
   return tsrc_validate(src);
}

/**
 * Offset the source operand.
 */
static inline struct toy_src
tsrc_offset(struct toy_src src, int reg, int subreg)
{
   src.val32 += reg * TOY_REG_WIDTH + subreg * toy_type_size(src.type);
   return tsrc_validate(src);
}

/**
 * Construct a source operand.
 */
static inline struct toy_src
tsrc_full(enum toy_file file, enum toy_type type,
          enum toy_rect rect, bool indirect, unsigned indirect_subreg,
          enum toy_swizzle swizzle_x, enum toy_swizzle swizzle_y,
          enum toy_swizzle swizzle_z, enum toy_swizzle swizzle_w,
          bool absolute, bool negate,
          uint32_t val32)
{
   struct toy_src src;

   src.file = file;
   src.type = type;
   src.rect = rect;
   src.indirect = indirect;
   src.indirect_subreg = indirect_subreg;
   src.swizzle_x = swizzle_x;
   src.swizzle_y = swizzle_y;
   src.swizzle_z = swizzle_z;
   src.swizzle_w = swizzle_w;
   src.absolute = absolute;
   src.negate = negate;
   src.pad = 0;

   src.val32 = val32;

   return tsrc_validate(src);
}

/**
 * Construct a null source operand.
 */
static inline struct toy_src
tsrc_null(void)
{
   static const struct toy_src null_src = {
      .file = TOY_FILE_ARF,
      .type = TOY_TYPE_F,
      .rect = TOY_RECT_LINEAR,
      .indirect = false,
      .indirect_subreg = 0,
      .swizzle_x = TOY_SWIZZLE_X,
      .swizzle_y = TOY_SWIZZLE_Y,
      .swizzle_z = TOY_SWIZZLE_Z,
      .swizzle_w = TOY_SWIZZLE_W,
      .absolute = false,
      .negate = false,
      .pad = 0,
      .val32 = 0,
   };

   return null_src;
}

/**
 * Construct a source operand from a destination operand.
 */
static inline struct toy_src
tsrc_from(struct toy_dst dst)
{
   enum toy_swizzle swizzle[4];

   if (dst.writemask == TOY_WRITEMASK_XYZW) {
      swizzle[0] = TOY_SWIZZLE_X;
      swizzle[1] = TOY_SWIZZLE_Y;
      swizzle[2] = TOY_SWIZZLE_Z;
      swizzle[3] = TOY_SWIZZLE_W;
   }
   else {
      const enum toy_swizzle first =
         (dst.writemask & TOY_WRITEMASK_X) ? TOY_SWIZZLE_X :
         (dst.writemask & TOY_WRITEMASK_Y) ? TOY_SWIZZLE_Y :
         (dst.writemask & TOY_WRITEMASK_Z) ? TOY_SWIZZLE_Z :
         (dst.writemask & TOY_WRITEMASK_W) ? TOY_SWIZZLE_W :
         TOY_SWIZZLE_X;

      swizzle[0] = (dst.writemask & TOY_WRITEMASK_X) ? TOY_SWIZZLE_X : first;
      swizzle[1] = (dst.writemask & TOY_WRITEMASK_Y) ? TOY_SWIZZLE_Y : first;
      swizzle[2] = (dst.writemask & TOY_WRITEMASK_Z) ? TOY_SWIZZLE_Z : first;
      swizzle[3] = (dst.writemask & TOY_WRITEMASK_W) ? TOY_SWIZZLE_W : first;
   }

   return tsrc_full(dst.file, dst.type, dst.rect,
                    dst.indirect, dst.indirect_subreg,
                    swizzle[0], swizzle[1], swizzle[2], swizzle[3],
                    false, false, dst.val32);
}

/**
 * Construct a source operand, assuming the type is TOY_TYPE_F, the
 * rectangle is TOY_RECT_LINEAR, and no swizzles/absolute/negate.
 */
static inline struct toy_src
tsrc(enum toy_file file, unsigned reg, unsigned subreg_in_bytes)
{
   const enum toy_type type = TOY_TYPE_F;
   const enum toy_rect rect = TOY_RECT_LINEAR;
   const uint32_t val32 = reg * TOY_REG_WIDTH + subreg_in_bytes;

   return tsrc_full(file, type, rect, false, 0,
                    TOY_SWIZZLE_X, TOY_SWIZZLE_Y,
                    TOY_SWIZZLE_Z, TOY_SWIZZLE_W,
                    false, false, val32);
}

/**
 * Construct an immediate source operand.
 */
static inline struct toy_src
tsrc_imm(enum toy_type type, uint32_t val32)
{
   return tsrc_full(TOY_FILE_IMM, type, TOY_RECT_LINEAR, false, 0,
                    TOY_SWIZZLE_X, TOY_SWIZZLE_Y,
                    TOY_SWIZZLE_Z, TOY_SWIZZLE_W,
                    false, false, val32);
}

/**
 * Construct an immediate source operand of type TOY_TYPE_F.
 */
static inline struct toy_src
tsrc_imm_f(float f)
{
   const union fi fi = { .f = f };
   return tsrc_imm(TOY_TYPE_F, fi.ui);
}

/**
 * Construct an immediate source operand of type TOY_TYPE_D.
 */
static inline struct toy_src
tsrc_imm_d(int32_t d)
{
   const union fi fi = { .i = d };
   return tsrc_imm(TOY_TYPE_D, fi.ui);
}

/**
 * Construct an immediate source operand of type TOY_TYPE_UD.
 */
static inline struct toy_src
tsrc_imm_ud(uint32_t ud)
{
   const union fi fi = { .ui = ud };
   return tsrc_imm(TOY_TYPE_UD, fi.ui);
}

/**
 * Construct an immediate source operand of type TOY_TYPE_W.
 */
static inline struct toy_src
tsrc_imm_w(int16_t w)
{
   const union fi fi = { .i = w };
   return tsrc_imm(TOY_TYPE_W, fi.ui);
}

/**
 * Construct an immediate source operand of type TOY_TYPE_UW.
 */
static inline struct toy_src
tsrc_imm_uw(uint16_t uw)
{
   const union fi fi = { .ui = uw };
   return tsrc_imm(TOY_TYPE_UW, fi.ui);
}

/**
 * Construct an immediate source operand of type TOY_TYPE_V.
 */
static inline struct toy_src
tsrc_imm_v(uint32_t v)
{
   return tsrc_imm(TOY_TYPE_V, v);
}

#endif /* TOY_REG_H */
