/*
 * Copyright © 2008 Keith Packard
 * Copyright © 2009-2013 Intel Corporation
 *
 * Permission to use, copy, modify, distribute, and sell this software and its
 * documentation for any purpose is hereby granted without fee, provided that
 * the above copyright notice appear in all copies and that both that copyright
 * notice and this permission notice appear in supporting documentation, and
 * that the name of the copyright holders not be used in advertising or
 * publicity pertaining to distribution of the software without specific,
 * written prior permission.  The copyright holders make no representations
 * about the suitability of this software for any purpose.  It is provided "as
 * is" without express or implied warranty.
 *
 * THE COPYRIGHT HOLDERS DISCLAIM ALL WARRANTIES WITH REGARD TO THIS SOFTWARE,
 * INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS, IN NO
 * EVENT SHALL THE COPYRIGHT HOLDERS BE LIABLE FOR ANY SPECIAL, INDIRECT OR
 * CONSEQUENTIAL DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE,
 * DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR PERFORMANCE
 * OF THIS SOFTWARE.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include <stdarg.h>

#include "brw_context.h"
#include "brw_defines.h"
#include "gen8_instruction.h"

static const struct opcode_desc *m_opcode = opcode_descs;

static const char *const m_conditional_modifier[16] = {
   [BRW_CONDITIONAL_NONE] = "",
   [BRW_CONDITIONAL_Z]    = ".e",
   [BRW_CONDITIONAL_NZ]   = ".ne",
   [BRW_CONDITIONAL_G]    = ".g",
   [BRW_CONDITIONAL_GE]   = ".ge",
   [BRW_CONDITIONAL_L]    = ".l",
   [BRW_CONDITIONAL_LE]   = ".le",
   [BRW_CONDITIONAL_O]    = ".o",
   [BRW_CONDITIONAL_U]    = ".u",
};

static const char *const m_negate[2] = { "", "-" };

static const char *const m_abs[2] = { "", "(abs)" };

static const char *const m_vert_stride[16] = {
   "0",
   "1",
   "2",
   "4",
   "8",
   "16",
   "32",
};

static const char *const width[8] = {
   "1",
   "2",
   "4",
   "8",
   "16",
};

static const char *const m_horiz_stride[4] = {
   "0",
   "1",
   "2",
   "4"
};

static const char *const m_chan_sel[4] = { "x", "y", "z", "w" };

static const char *const m_debug_ctrl[2] = { "", ".breakpoint" };

static const char *const m_saturate[2] = { "", ".sat" };

static const char *const m_accwr[2] = { "", "AccWrEnable" };

static const char *const m_maskctrl[2] = { "WE_normal", "WE_all" };

static const char *const m_exec_size[8] = {
   "1",
   "2",
   "4",
   "8",
   "16",
   "32",
};

static const char *const m_pred_inv[2] = { "+", "-" };

static const char *const m_pred_ctrl_align16[16] = {
   "",
   "",
   ".x",
   ".y",
   ".z",
   ".w",
   ".any4h",
   ".all4h",
};

static const char *const m_pred_ctrl_align1[16] = {
   "",
   "",
   ".anyv",
   ".allv",
   ".any2h",
   ".all2h",
   ".any4h",
   ".all4h",
   ".any8h",
   ".all8h",
   ".any16h",
   ".all16h",
   ".any32h",
   ".all32h",
};

static const char *const m_thread_ctrl[4] = {
   "",
   "atomic",
   "switch",
};

static const char *const m_dep_clear[4] = {
   "",
   "NoDDClr",
};

static const char *const m_dep_check[4] = {
   "",
   "NoDDChk",
};

static const char *const m_mask_ctrl[4] = {
   "",
   "nomask",
};

static const char *const m_access_mode[2] = { "align1", "align16" };

static const char *const m_reg_encoding[] = {
   [BRW_HW_REG_TYPE_UD]          = "UD",
   [BRW_HW_REG_TYPE_D]           = "D",
   [BRW_HW_REG_TYPE_UW]          = "UW",
   [BRW_HW_REG_TYPE_W]           = "W",
   [BRW_HW_REG_NON_IMM_TYPE_UB]  = "UB",
   [BRW_HW_REG_NON_IMM_TYPE_B]   = "B",
   [GEN7_HW_REG_NON_IMM_TYPE_DF] = "DF",
   [BRW_HW_REG_TYPE_F]           = "F",
   [GEN8_HW_REG_TYPE_UQ]         = "UQ",
   [GEN8_HW_REG_TYPE_Q]          = "Q",
   [GEN8_HW_REG_NON_IMM_TYPE_HF] = "HF",
};

static const char *const m_three_source_reg_encoding[] = {
   [BRW_3SRC_TYPE_F]  = "F",
   [BRW_3SRC_TYPE_D]  = "D",
   [BRW_3SRC_TYPE_UD] = "UD",
};

static const int reg_type_size[] = {
   [BRW_HW_REG_TYPE_UD]          = 4,
   [BRW_HW_REG_TYPE_D]           = 4,
   [BRW_HW_REG_TYPE_UW]          = 2,
   [BRW_HW_REG_TYPE_W]           = 2,
   [BRW_HW_REG_NON_IMM_TYPE_UB]  = 1,
   [BRW_HW_REG_NON_IMM_TYPE_B]   = 1,
   [GEN7_HW_REG_NON_IMM_TYPE_DF] = 8,
   [BRW_HW_REG_TYPE_F]           = 4,
   [GEN8_HW_REG_NON_IMM_TYPE_HF] = 2,
};

static const char *const m_reg_file[4] = {
   [BRW_ARCHITECTURE_REGISTER_FILE] = "A",
   [BRW_GENERAL_REGISTER_FILE] = "g",
   [BRW_IMMEDIATE_VALUE] = "imm",
};

static const char *const m_writemask[16] = {
   ".(none)",
   ".x",
   ".y",
   ".xy",
   ".z",
   ".xz",
   ".yz",
   ".xyz",
   ".w",
   ".xw",
   ".yw",
   ".xyw",
   ".zw",
   ".xzw",
   ".yzw",
   "",
};

static const char *const m_eot[2] = { "", "EOT" };

static const char *const m_sfid[16] = {
   [BRW_SFID_NULL]                     = "null",
   [BRW_SFID_SAMPLER]                  = "sampler",
   [BRW_SFID_MESSAGE_GATEWAY]          = "gateway",
   [GEN6_SFID_DATAPORT_SAMPLER_CACHE]  = "dp/sampler_cache",
   [GEN6_SFID_DATAPORT_RENDER_CACHE]   = "dp/render_cache",
   [BRW_SFID_URB]                      = "URB",
   [BRW_SFID_THREAD_SPAWNER]           = "thread_spawner",
   [BRW_SFID_VME]                      = "vme",
   [GEN6_SFID_DATAPORT_CONSTANT_CACHE] = "dp/constant_cache",
   [GEN7_SFID_DATAPORT_DATA_CACHE]     = "dp/data_cache",
   [GEN7_SFID_PIXEL_INTERPOLATOR]      = "pi",
   [HSW_SFID_DATAPORT_DATA_CACHE_1]    = "dp/data_cache:1",
   [HSW_SFID_CRE]                      = "cre",
};

static const char *const dp_dc1_msg_type[16] = {
   [HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ]      = "untyped surface read",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP]         = "DC untyped atomic op",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2] = "DC untyped 4x2 atomic op",
   [HSW_DATAPORT_DC_PORT1_MEDIA_BLOCK_READ]          = "DC media block read",
   [HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ]        = "DC typed surface read",
   [HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP]           = "DC typed atomic",
   [HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2]   = "DC typed 4x2 atomic op",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE]     = "DC untyped surface write",
   [HSW_DATAPORT_DC_PORT1_MEDIA_BLOCK_WRITE]         = "DC media block write",
   [HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP]         = "DC atomic counter op",
   [HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP_SIMD4X2] = "DC 4x2 atomic counter op",
   [HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE]       = "DC typed surface write",
};

static const char * const aop[16] = {
   [BRW_AOP_AND]    = "and",
   [BRW_AOP_OR]     = "or",
   [BRW_AOP_XOR]    = "xor",
   [BRW_AOP_MOV]    = "mov",
   [BRW_AOP_INC]    = "inc",
   [BRW_AOP_DEC]    = "dec",
   [BRW_AOP_ADD]    = "add",
   [BRW_AOP_SUB]    = "sub",
   [BRW_AOP_REVSUB] = "revsub",
   [BRW_AOP_IMAX]   = "imax",
   [BRW_AOP_IMIN]   = "imin",
   [BRW_AOP_UMAX]   = "umax",
   [BRW_AOP_UMIN]   = "umin",
   [BRW_AOP_CMPWR]  = "cmpwr",
   [BRW_AOP_PREDEC] = "predec",
};

static const char *const m_math_function[16] = {
   [BRW_MATH_FUNCTION_INV]                            = "inv",
   [BRW_MATH_FUNCTION_LOG]                            = "log",
   [BRW_MATH_FUNCTION_EXP]                            = "exp",
   [BRW_MATH_FUNCTION_SQRT]                           = "sqrt",
   [BRW_MATH_FUNCTION_RSQ]                            = "rsq",
   [BRW_MATH_FUNCTION_SIN]                            = "sin",
   [BRW_MATH_FUNCTION_COS]                            = "cos",
   [BRW_MATH_FUNCTION_FDIV]                           = "fdiv",
   [BRW_MATH_FUNCTION_POW]                            = "pow",
   [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER] = "intdivmod",
   [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT]               = "intdiv",
   [BRW_MATH_FUNCTION_INT_DIV_REMAINDER]              = "intmod",
   [GEN8_MATH_FUNCTION_INVM]                          = "invm",
   [GEN8_MATH_FUNCTION_RSQRTM]                        = "rsqrtm",
};

static const char *const m_urb_opcode[16] = {
   [0] = "write HWord",
   [1] = "write OWord",
   [2] = "read HWord",
   [3] = "read OWord",
   [4] = "atomic mov",
   [5] = "atomic inc",
   [6] = "atomic add",
   [7] = "SIMD8 write",
   [8] = "SIMD8 read",
   /* [9-15] - reserved */
};

static const char *const m_urb_interleave[2] = { "", "interleaved" };

static const char *const m_rt_opcode[] = {
   [0] = "SIMD16",
   [1] = "RepData",
   [2] = "DualLow",
   [3] = "DualHigh",
   [4] = "SIMD8",
   [5] = "ImageWrite",
   [6] = "Reserved",
   [7] = "RepData(7)",
};

static int column;

static int
string(FILE *file, const char *string)
{
   fputs(string, file);
   column += strlen(string);
   return 0;
}

static int
format(FILE *f, const char *format, ...)
{
   char buf[1024];
   va_list args;
   va_start(args, format);

   vsnprintf(buf, sizeof(buf) - 1, format, args);
   va_end(args);
   string(f, buf);
   return 0;
}

static int
newline(FILE *f)
{
   putc('\n', f);
   column = 0;
   return 0;
}

static int
pad(FILE *f, int c)
{
   do
      string(f, " ");
   while (column < c);
   return 0;
}

static int
control(FILE *file, const char *name, const char *const ctrl[],
        unsigned id, int *space)
{
   if (!ctrl[id]) {
      fprintf(file, "*** invalid %s value %d ", name, id);
      return 1;
   }
   if (ctrl[id][0]) {
      if (space && *space)
         string(file, " ");
      string(file, ctrl[id]);
      if (space)
         *space = 1;
   }
   return 0;
}

static int
print_opcode(FILE *file, int id)
{
   if (!m_opcode[id].name) {
      format(file, "*** invalid opcode value %d ", id);
      return 1;
   }
   string(file, m_opcode[id].name);
   return 0;
}

static int
reg(FILE *file, unsigned reg_file, unsigned _reg_nr)
{
   int err = 0;

   if (reg_file == BRW_ARCHITECTURE_REGISTER_FILE) {
      switch (_reg_nr & 0xf0) {
      case BRW_ARF_NULL:
         string(file, "null");
         return -1;
      case BRW_ARF_ADDRESS:
         format(file, "a%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_ACCUMULATOR:
         format(file, "acc%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_FLAG:
         format(file, "f%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_MASK:
         format(file, "mask%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_MASK_STACK:
         format(file, "msd%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_STATE:
         format(file, "sr%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_CONTROL:
         format(file, "cr%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_NOTIFICATION_COUNT:
         format(file, "n%d", _reg_nr & 0x0f);
         break;
      case BRW_ARF_IP:
         string(file, "ip");
         return -1;
         break;
      default:
         format(file, "ARF%d", _reg_nr);
         break;
      }
   } else {
      err |= control(file, "src reg file", m_reg_file, reg_file, NULL);
      format(file, "%d", _reg_nr);
   }
   return err;
}

static int
dest(FILE *file, struct gen8_instruction *inst)
{
   int err = 0;

   if (gen8_access_mode(inst) == BRW_ALIGN_1) {
      assert(gen8_dst_address_mode(inst) == BRW_ADDRESS_DIRECT);
      err |= reg(file, gen8_dst_reg_file(inst), gen8_dst_da_reg_nr(inst));
      if (err == -1)
         return 0;
      if (gen8_dst_da1_subreg_nr(inst))
         format(file, ".%d", gen8_dst_da1_subreg_nr(inst) /
                reg_type_size[gen8_dst_reg_type(inst)]);
      string(file, "<");
      err |= control(file, "horiz stride", m_horiz_stride, gen8_dst_da1_hstride(inst), NULL);
      string(file, ">");
      err |= control(file, "dest reg encoding", m_reg_encoding, gen8_dst_reg_type(inst), NULL);
   } else {
      assert(gen8_dst_address_mode(inst) == BRW_ADDRESS_DIRECT);
      err |= reg(file, gen8_dst_reg_file(inst), gen8_dst_da_reg_nr(inst));
      if (err == -1)
         return 0;
      if (gen8_dst_da16_subreg_nr(inst))
         format(file, ".%d", gen8_dst_da16_subreg_nr(inst) /
                reg_type_size[gen8_dst_reg_type(inst)]);
      string(file, "<1>");
      err |= control(file, "writemask", m_writemask, gen8_da16_writemask(inst), NULL);
      err |= control(file, "dest reg encoding", m_reg_encoding, gen8_dst_reg_type(inst), NULL);
   }

   return 0;
}

static int
dest_3src(FILE *file, struct gen8_instruction *inst)
{
   int      err = 0;

   err |= reg(file, BRW_GENERAL_REGISTER_FILE, gen8_dst_3src_reg_nr(inst));
   if (err == -1)
      return 0;
   if (gen8_dst_3src_subreg_nr(inst))
      format(file, ".%d", gen8_dst_3src_subreg_nr(inst));
   string(file, "<1>");
   err |= control(file, "writemask", m_writemask, gen8_dst_3src_writemask(inst),
                  NULL);
   err |= control(file, "dest reg encoding", m_three_source_reg_encoding,
                  gen8_dst_3src_type(inst), NULL);

   return 0;
}

static int
src_align1_region(FILE *file, unsigned vert_stride, unsigned _width,
                  unsigned horiz_stride)
{
   int err = 0;
   string(file, "<");
   err |= control(file, "vert stride", m_vert_stride, vert_stride, NULL);
   string(file, ",");
   err |= control(file, "width", width, _width, NULL);
   string(file, ",");
   err |= control(file, "horiz_stride", m_horiz_stride, horiz_stride, NULL);
   string(file, ">");
   return err;
}

static int
src_swizzle(FILE *file, unsigned x, unsigned y, unsigned z, unsigned w)
{
   int err = 0;

   /* Three kinds of swizzle display:
    * - identity - nothing printed
    * - 1->all   - print the single channel
    * - 1->1     - print the mapping
    */
   if (x == BRW_CHANNEL_X &&
       y == BRW_CHANNEL_Y &&
       z == BRW_CHANNEL_Z &&
       w == BRW_CHANNEL_W) {
      ; /* Print nothing */
   } else if (x == y && x == z && x == w) {
      string(file, ".");
      err |= control(file, "channel select", m_chan_sel, x, NULL);
   } else {
      string(file, ".");
      err |= control(file, "channel select", m_chan_sel, x, NULL);
      err |= control(file, "channel select", m_chan_sel, y, NULL);
      err |= control(file, "channel select", m_chan_sel, z, NULL);
      err |= control(file, "channel select", m_chan_sel, w, NULL);
   }
   return err;
}

static int
src_da1(FILE *file, unsigned type, unsigned reg_file,
        unsigned vert_stride, unsigned _width, unsigned horiz_stride,
        unsigned reg_num, unsigned sub_reg_num, unsigned _abs, unsigned negate)
{
   int err = 0;
   err |= control(file, "negate", m_negate, negate, NULL);
   err |= control(file, "abs", m_abs, _abs, NULL);

   err |= reg(file, reg_file, reg_num);
   if (err == -1)
      return 0;
   if (sub_reg_num)
      format(file, ".%d", sub_reg_num / reg_type_size[type]); /* use formal style like spec */
   src_align1_region(file, vert_stride, _width, horiz_stride);
   err |= control(file, "src reg encoding", m_reg_encoding, type, NULL);
   return err;
}

static int
src_da16(FILE *file,
         unsigned _reg_type,
         unsigned reg_file,
         unsigned vert_stride,
         unsigned _reg_nr,
         unsigned _subreg_nr,
         unsigned _abs,
         unsigned negate,
         unsigned swz_x,
         unsigned swz_y,
         unsigned swz_z,
         unsigned swz_w)
{
   int err = 0;
   err |= control(file, "negate", m_negate, negate, NULL);
   err |= control(file, "abs", m_abs, _abs, NULL);

   err |= reg(file, reg_file, _reg_nr);
   if (err == -1)
      return 0;
   if (_subreg_nr)
      /* bit4 for subreg number byte addressing. Make this same meaning as
       * in da1 case, so output looks consistent.
       */
      format(file, ".%d", 16 / reg_type_size[_reg_type]);
   string(file, "<");
   err |= control(file, "vert stride", m_vert_stride, vert_stride, NULL);
   string(file, ",4,1>");
   /*
    * Three kinds of swizzle display:
    *  identity - nothing printed
    *  1->all       - print the single channel
    *  1->1    - print the mapping
    */
   if (swz_x == BRW_CHANNEL_X &&
      swz_y == BRW_CHANNEL_Y &&
      swz_z == BRW_CHANNEL_Z &&
      swz_w == BRW_CHANNEL_W) {
      ; /* Print nothing */
   } else if (swz_x == swz_y && swz_x == swz_z && swz_x == swz_w) {
      string(file, ".");
      err |= control(file, "channel select", m_chan_sel, swz_x, NULL);
   } else {
      string(file, ".");
      err |= control(file, "channel select", m_chan_sel, swz_x, NULL);
      err |= control(file, "channel select", m_chan_sel, swz_y, NULL);
      err |= control(file, "channel select", m_chan_sel, swz_z, NULL);
      err |= control(file, "channel select", m_chan_sel, swz_w, NULL);
   }
   err |= control(file, "src da16 reg type", m_reg_encoding, _reg_type, NULL);
   return err;
}

static int
src0_3src(FILE *file, struct gen8_instruction *inst)
{
   int err = 0;
   unsigned swiz = gen8_src0_3src_swizzle(inst);
   unsigned swz_x = (swiz >> 0) & 0x3;
   unsigned swz_y = (swiz >> 2) & 0x3;
   unsigned swz_z = (swiz >> 4) & 0x3;
   unsigned swz_w = (swiz >> 6) & 0x3;

   err |= control(file, "negate", m_negate, gen8_src0_3src_negate(inst), NULL);
   err |= control(file, "abs", m_abs, gen8_src0_3src_abs(inst), NULL);

   err |= reg(file, BRW_GENERAL_REGISTER_FILE, gen8_src0_3src_reg_nr(inst));
   if (err == -1)
      return 0;
   if (gen8_src0_3src_subreg_nr(inst))
      format(file, ".%d", gen8_src0_3src_subreg_nr(inst));
   if (gen8_src0_3src_rep_ctrl(inst))
      string(file, "<0,1,0>");
   else
      string(file, "<4,4,1>");
   err |= control(file, "src da16 reg type", m_three_source_reg_encoding,
                  gen8_src_3src_type(inst), NULL);
   err |= src_swizzle(file, swz_x, swz_y, swz_z, swz_w);
   return err;
}

static int
src1_3src(FILE *file, struct gen8_instruction *inst)
{
   int err = 0;
   unsigned swiz = gen8_src1_3src_swizzle(inst);
   unsigned swz_x = (swiz >> 0) & 0x3;
   unsigned swz_y = (swiz >> 2) & 0x3;
   unsigned swz_z = (swiz >> 4) & 0x3;
   unsigned swz_w = (swiz >> 6) & 0x3;
   unsigned src1_subreg_nr = gen8_src1_3src_subreg_nr(inst);

   err |= control(file, "negate", m_negate, gen8_src1_3src_negate(inst), NULL);
   err |= control(file, "abs", m_abs, gen8_src1_3src_abs(inst), NULL);

   err |= reg(file, BRW_GENERAL_REGISTER_FILE, gen8_src1_3src_reg_nr(inst));
   if (err == -1)
      return 0;
   if (src1_subreg_nr)
      format(file, ".%d", src1_subreg_nr);
   if (gen8_src1_3src_rep_ctrl(inst))
      string(file, "<0,1,0>");
   else
      string(file, "<4,4,1>");
   err |= control(file, "src da16 reg type", m_three_source_reg_encoding,
                  gen8_src_3src_type(inst), NULL);
   err |= src_swizzle(file, swz_x, swz_y, swz_z, swz_w);
   return err;
}

static int
src2_3src(FILE *file, struct gen8_instruction *inst)
{
   int err = 0;
   unsigned swiz = gen8_src2_3src_swizzle(inst);
   unsigned swz_x = (swiz >> 0) & 0x3;
   unsigned swz_y = (swiz >> 2) & 0x3;
   unsigned swz_z = (swiz >> 4) & 0x3;
   unsigned swz_w = (swiz >> 6) & 0x3;

   err |= control(file, "negate", m_negate, gen8_src2_3src_negate(inst), NULL);
   err |= control(file, "abs", m_abs, gen8_src2_3src_abs(inst), NULL);

   err |= reg(file, BRW_GENERAL_REGISTER_FILE, gen8_src2_3src_reg_nr(inst));
   if (err == -1)
      return 0;
   if (gen8_src2_3src_subreg_nr(inst))
      format(file, ".%d", gen8_src2_3src_subreg_nr(inst));
   if (gen8_src2_3src_rep_ctrl(inst))
      string(file, "<0,1,0>");
   else
      string(file, "<4,4,1>");
   err |= control(file, "src da16 reg type", m_three_source_reg_encoding,
                  gen8_src_3src_type(inst), NULL);
   err |= src_swizzle(file, swz_x, swz_y, swz_z, swz_w);
   return err;
}

static int
imm(FILE *file, unsigned type, struct gen8_instruction *inst)
{
   switch (type) {
   case BRW_HW_REG_TYPE_UD:
      format(file, "0x%08xUD", gen8_src1_imm_ud(inst));
      break;
   case BRW_HW_REG_TYPE_D:
      format(file, "%dD", gen8_src1_imm_d(inst));
      break;
   case BRW_HW_REG_TYPE_UW:
      format(file, "0x%04xUW", (uint16_t) gen8_src1_imm_ud(inst));
      break;
   case BRW_HW_REG_TYPE_W:
      format(file, "%dW", (int16_t) gen8_src1_imm_d(inst));
      break;
   case BRW_HW_REG_IMM_TYPE_UV:
      format(file, "0x%08xUV", gen8_src1_imm_ud(inst));
      break;
   case BRW_HW_REG_IMM_TYPE_VF:
      format(file, "Vector Float");
      break;
   case BRW_HW_REG_IMM_TYPE_V:
      format(file, "0x%08xV", gen8_src1_imm_ud(inst));
      break;
   case BRW_HW_REG_TYPE_F:
      format(file, "%-gF", gen8_src1_imm_f(inst));
      break;
   case GEN8_HW_REG_IMM_TYPE_DF:
   case GEN8_HW_REG_IMM_TYPE_HF:
      assert(!"Not implemented yet");
      break;
   }
   return 0;
}

static int
src0(FILE *file, struct gen8_instruction *inst)
{
   if (gen8_src0_reg_file(inst) == BRW_IMMEDIATE_VALUE)
      return imm(file, gen8_src0_reg_type(inst), inst);

   if (gen8_access_mode(inst) == BRW_ALIGN_1) {
      assert(gen8_src0_address_mode(inst) == BRW_ADDRESS_DIRECT);
      return src_da1(file,
                     gen8_src0_reg_type(inst),
                     gen8_src0_reg_file(inst),
                     gen8_src0_vert_stride(inst),
                     gen8_src0_da1_width(inst),
                     gen8_src0_da1_hstride(inst),
                     gen8_src0_da_reg_nr(inst),
                     gen8_src0_da1_subreg_nr(inst),
                     gen8_src0_abs(inst),
                     gen8_src0_negate(inst));
   } else {
      assert(gen8_src0_address_mode(inst) == BRW_ADDRESS_DIRECT);
      return src_da16(file,
                      gen8_src0_reg_type(inst),
                      gen8_src0_reg_file(inst),
                      gen8_src0_vert_stride(inst),
                      gen8_src0_da_reg_nr(inst),
                      gen8_src0_da16_subreg_nr(inst),
                      gen8_src0_abs(inst),
                      gen8_src0_negate(inst),
                      gen8_src0_da16_swiz_x(inst),
                      gen8_src0_da16_swiz_y(inst),
                      gen8_src0_da16_swiz_z(inst),
                      gen8_src0_da16_swiz_w(inst));
   }
}

static int
src1(FILE *file, struct gen8_instruction *inst)
{
   if (gen8_src1_reg_file(inst) == BRW_IMMEDIATE_VALUE)
      return imm(file, gen8_src1_reg_type(inst), inst);

   if (gen8_access_mode(inst) == BRW_ALIGN_1) {
      assert(gen8_src1_address_mode(inst) == BRW_ADDRESS_DIRECT);
      return src_da1(file,
                     gen8_src1_reg_type(inst),
                     gen8_src1_reg_file(inst),
                     gen8_src1_vert_stride(inst),
                     gen8_src1_da1_width(inst),
                     gen8_src1_da1_hstride(inst),
                     gen8_src1_da_reg_nr(inst),
                     gen8_src1_da1_subreg_nr(inst),
                     gen8_src1_abs(inst),
                     gen8_src1_negate(inst));
   } else {
      assert(gen8_src1_address_mode(inst) == BRW_ADDRESS_DIRECT);
      return src_da16(file,
                      gen8_src1_reg_type(inst),
                      gen8_src1_reg_file(inst),
                      gen8_src1_vert_stride(inst),
                      gen8_src1_da_reg_nr(inst),
                      gen8_src1_da16_subreg_nr(inst),
                      gen8_src1_abs(inst),
                      gen8_src1_negate(inst),
                      gen8_src1_da16_swiz_x(inst),
                      gen8_src1_da16_swiz_y(inst),
                      gen8_src1_da16_swiz_z(inst),
                      gen8_src1_da16_swiz_w(inst));
   }
}

static int
qtr_ctrl(FILE *file, struct gen8_instruction *inst)
{
   int qtr_ctl = gen8_qtr_control(inst);
   int exec_size = 1 << gen8_exec_size(inst);

   if (exec_size == 8) {
      switch (qtr_ctl) {
      case 0:
         string(file, " 1Q");
         break;
      case 1:
         string(file, " 2Q");
         break;
      case 2:
         string(file, " 3Q");
         break;
      case 3:
         string(file, " 4Q");
         break;
      }
   } else if (exec_size == 16) {
      if (qtr_ctl < 2)
         string(file, " 1H");
      else
         string(file, " 2H");
   }
   return 0;
}

int
gen8_disassemble(FILE *file, struct gen8_instruction *inst, int gen)
{
   int err = 0;
   int space = 0;

   const int opcode = gen8_opcode(inst);

   if (gen8_pred_control(inst)) {
      string(file, "(");
      err |= control(file, "predicate inverse", m_pred_inv, gen8_pred_inv(inst), NULL);
      format(file, "f%d", gen8_flag_reg_nr(inst));
      if (gen8_flag_subreg_nr(inst))
         format(file, ".%d", gen8_flag_subreg_nr(inst));
      if (gen8_access_mode(inst) == BRW_ALIGN_1) {
         err |= control(file, "predicate control align1", m_pred_ctrl_align1,
                        gen8_pred_control(inst), NULL);
      } else {
         err |= control(file, "predicate control align16", m_pred_ctrl_align16,
                        gen8_pred_control(inst), NULL);
      }
      string(file, ") ");
   }

   err |= print_opcode(file, opcode);
   err |= control(file, "saturate", m_saturate, gen8_saturate(inst), NULL);
   err |= control(file, "debug control", m_debug_ctrl, gen8_debug_control(inst), NULL);

   if (opcode == BRW_OPCODE_MATH) {
      string(file, " ");
      err |= control(file, "function", m_math_function, gen8_math_function(inst),
                     NULL);
   } else if (opcode != BRW_OPCODE_SEND && opcode != BRW_OPCODE_SENDC) {
      err |= control(file, "conditional modifier", m_conditional_modifier,
                     gen8_cond_modifier(inst), NULL);

      /* If we're using the conditional modifier, print the flag reg used. */
      if (gen8_cond_modifier(inst) && opcode != BRW_OPCODE_SEL) {
         format(file, ".f%d", gen8_flag_reg_nr(inst));
         if (gen8_flag_subreg_nr(inst))
            format(file, ".%d", gen8_flag_subreg_nr(inst));
      }
   }

   if (opcode != BRW_OPCODE_NOP) {
      string(file, "(");
      err |= control(file, "execution size", m_exec_size, gen8_exec_size(inst), NULL);
      string(file, ")");
   }

   if (m_opcode[opcode].nsrc == 3) {
      pad(file, 16);
      err |= dest_3src(file, inst);

      pad(file, 32);
      err |= src0_3src(file, inst);

      pad(file, 48);
      err |= src1_3src(file, inst);

      pad(file, 64);
      err |= src2_3src(file, inst);
   } else {
      if (opcode == BRW_OPCODE_ENDIF || opcode == BRW_OPCODE_WHILE) {
         pad(file, 16);
         format(file, "JIP: %d", gen8_jip(inst));
      } else if (opcode == BRW_OPCODE_IF ||
                 opcode == BRW_OPCODE_ELSE ||
                 opcode == BRW_OPCODE_BREAK ||
                 opcode == BRW_OPCODE_CONTINUE ||
                 opcode == BRW_OPCODE_HALT) {
         pad(file, 16);
         format(file, "JIP: %d", gen8_jip(inst));
         pad(file, 32);
         format(file, "UIP: %d", gen8_uip(inst));
      } else {
         if (m_opcode[opcode].ndst > 0) {
            pad(file, 16);
            err |= dest(file, inst);
         }
         if (m_opcode[opcode].nsrc > 0) {
            pad(file, 32);
            err |= src0(file, inst);
         }
         if (m_opcode[opcode].nsrc > 1) {
            pad(file, 48);
            err |= src1(file, inst);
         }
      }
   }

   if (opcode == BRW_OPCODE_SEND || opcode == BRW_OPCODE_SENDC) {
      const int sfid = gen8_sfid(inst);

      newline(file);
      pad(file, 16);
      space = 0;

      err |= control(file, "SFID", m_sfid, sfid, &space);

      switch (sfid) {
      case BRW_SFID_SAMPLER:
         format(file, " (%d, %d, %d, %d)",
                gen8_binding_table_index(inst),
                gen8_sampler(inst),
                gen8_sampler_msg_type(inst),
                gen8_sampler_simd_mode(inst));
         break;

      case BRW_SFID_URB:
         space = 1;
         err |= control(file, "urb opcode", m_urb_opcode,
                        gen8_urb_opcode(inst), &space);
         err |= control(file, "urb interleave", m_urb_interleave,
                        gen8_urb_interleave(inst), &space);
         format(file, " %d %d",
                gen8_urb_global_offset(inst), gen8_urb_per_slot_offset(inst));
         break;

      case GEN6_SFID_DATAPORT_RENDER_CACHE: {
         err |= control(file, "rt message", m_rt_opcode,
                        gen8_rt_message_type(inst), &space);
         format(file, " %s%sSurface = %d",
                gen8_rt_slot_group(inst) ? "Hi " : "",
                gen8_rt_last(inst) ? "LastRT " : "",
                gen8_binding_table_index(inst));
         break;
      }

      case GEN6_SFID_DATAPORT_SAMPLER_CACHE:
      case GEN6_SFID_DATAPORT_CONSTANT_CACHE:
      case GEN7_SFID_DATAPORT_DATA_CACHE:
         format(file, " (%d, 0x%x)",
                gen8_binding_table_index(inst),
                gen8_function_control(inst));
         break;

      case HSW_SFID_DATAPORT_DATA_CACHE_1:
         err |= control(file, "DP DC1 message type",
                        dp_dc1_msg_type, gen8_dp_message_type(inst), &space);
         format(file, ", Surface = %d, ", gen8_binding_table_index(inst));
         switch (gen8_dp_message_type(inst)) {
         case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP:
         case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP:
         case HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP:
            format(file, "SIMD%d,",
                   (gen8_dp_message_control(inst) & (1 << 4)) ? 8 : 16);
            /* fallthrough */
         case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2:
         case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2:
         case HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP_SIMD4X2:
            control(file, "atomic op", aop,
                    gen8_dp_message_control(inst) & 0xf, &space);
            break;
         case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ:
         case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE:
         case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ:
         case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE: {
            static const char *simd_modes[] = { "4x2", "16", "8" };
            unsigned msg_ctrl = gen8_dp_message_control(inst);
            format(file, "SIMD%s, Mask = 0x%x",
                   simd_modes[msg_ctrl >> 4], msg_ctrl & 0xf);
            break;
         }
         default:
            format(file, "0x%x", gen8_dp_message_control(inst));
         }
         break;

      default:
         format(file, "unsupported shared function ID (%d)", sfid);
         break;
      }
      if (space)
         string(file, " ");
      format(file, "mlen %d", gen8_mlen(inst));
      format(file, " rlen %d", gen8_rlen(inst));
   }
   pad(file, 64);
   if (opcode != BRW_OPCODE_NOP) {
      string(file, "{");
      space = 1;
      err |= control(file, "access mode", m_access_mode, gen8_access_mode(inst), &space);
      err |= control(file, "mask control", m_maskctrl, gen8_mask_control(inst), &space);
      err |= control(file, "DDClr", m_dep_clear, gen8_no_dd_clear(inst), &space);
      err |= control(file, "DDChk", m_dep_check, gen8_no_dd_check(inst), &space);

      err |= qtr_ctrl(file, inst);

      err |= control(file, "thread control", m_thread_ctrl, gen8_thread_control(inst), &space);
      err |= control(file, "acc write control", m_accwr, gen8_acc_wr_control(inst), &space);
      if (opcode == BRW_OPCODE_SEND || opcode == BRW_OPCODE_SENDC)
         err |= control(file, "end of thread", m_eot, gen8_eot(inst), &space);
      if (space)
         string(file, " ");
      string(file, "}");
   }
   string(file, ";");
   newline(file);
   return err;
}
