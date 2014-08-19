/*
 * Copyright © 2008 Keith Packard
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
#include "brw_reg.h"
#include "brw_inst.h"

const struct opcode_desc opcode_descs[128] = {
   [BRW_OPCODE_MOV]      = { .name = "mov",     .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_FRC]      = { .name = "frc",     .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_RNDU]     = { .name = "rndu",    .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_RNDD]     = { .name = "rndd",    .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_RNDE]     = { .name = "rnde",    .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_RNDZ]     = { .name = "rndz",    .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_NOT]      = { .name = "not",     .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_LZD]      = { .name = "lzd",     .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_F32TO16]  = { .name = "f32to16", .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_F16TO32]  = { .name = "f16to32", .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_BFREV]    = { .name = "bfrev",   .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_FBH]      = { .name = "fbh",     .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_FBL]      = { .name = "fbl",     .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_CBIT]     = { .name = "cbit",    .nsrc = 1, .ndst = 1 },

   [BRW_OPCODE_MUL]      = { .name = "mul",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_MAC]      = { .name = "mac",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_MACH]     = { .name = "mach",    .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_LINE]     = { .name = "line",    .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_PLN]      = { .name = "pln",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_MAD]      = { .name = "mad",     .nsrc = 3, .ndst = 1 },
   [BRW_OPCODE_LRP]      = { .name = "lrp",     .nsrc = 3, .ndst = 1 },
   [BRW_OPCODE_SAD2]     = { .name = "sad2",    .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_SADA2]    = { .name = "sada2",   .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_DP4]      = { .name = "dp4",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_DPH]      = { .name = "dph",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_DP3]      = { .name = "dp3",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_DP2]      = { .name = "dp2",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_MATH]     = { .name = "math",    .nsrc = 2, .ndst = 1 },

   [BRW_OPCODE_AVG]      = { .name = "avg",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_ADD]      = { .name = "add",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_SEL]      = { .name = "sel",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_AND]      = { .name = "and",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_OR]       = { .name = "or",      .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_XOR]      = { .name = "xor",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_SHR]      = { .name = "shr",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_SHL]      = { .name = "shl",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_ASR]      = { .name = "asr",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_CMP]      = { .name = "cmp",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_CMPN]     = { .name = "cmpn",    .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_CSEL]     = { .name = "csel",    .nsrc = 3, .ndst = 1 },
   [BRW_OPCODE_BFE]      = { .name = "bfe",     .nsrc = 3, .ndst = 1 },
   [BRW_OPCODE_BFI1]     = { .name = "bfi1",    .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_BFI2]     = { .name = "bfi2",    .nsrc = 3, .ndst = 1 },
   [BRW_OPCODE_ADDC]     = { .name = "addc",    .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_SUBB]     = { .name = "subb",    .nsrc = 2, .ndst = 1 },

   [BRW_OPCODE_SEND]     = { .name = "send",    .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_SENDC]    = { .name = "sendc",   .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_NOP]      = { .name = "nop",     .nsrc = 0, .ndst = 0 },
   [BRW_OPCODE_JMPI]     = { .name = "jmpi",    .nsrc = 0, .ndst = 0 },
   [BRW_OPCODE_IF]       = { .name = "if",      .nsrc = 2, .ndst = 0 },
   [BRW_OPCODE_IFF]      = { .name = "iff",     .nsrc = 2, .ndst = 1 },
   [BRW_OPCODE_WHILE]    = { .name = "while",   .nsrc = 2, .ndst = 0 },
   [BRW_OPCODE_ELSE]     = { .name = "else",    .nsrc = 2, .ndst = 0 },
   [BRW_OPCODE_BREAK]    = { .name = "break",   .nsrc = 2, .ndst = 0 },
   [BRW_OPCODE_CONTINUE] = { .name = "cont",    .nsrc = 1, .ndst = 0 },
   [BRW_OPCODE_HALT]     = { .name = "halt",    .nsrc = 1, .ndst = 0 },
   [BRW_OPCODE_MSAVE]    = { .name = "msave",   .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_PUSH]     = { .name = "push",    .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_MRESTORE] = { .name = "mrest",   .nsrc = 1, .ndst = 1 },
   [BRW_OPCODE_POP]      = { .name = "pop",     .nsrc = 2, .ndst = 0 },
   [BRW_OPCODE_WAIT]     = { .name = "wait",    .nsrc = 1, .ndst = 0 },
   [BRW_OPCODE_DO]       = { .name = "do",      .nsrc = 0, .ndst = 0 },
   [BRW_OPCODE_ENDIF]    = { .name = "endif",   .nsrc = 2, .ndst = 0 },
};

static bool
has_jip(struct brw_context *brw, enum opcode opcode)
{
   if (brw->gen < 6)
      return false;

   return opcode == BRW_OPCODE_IF ||
          opcode == BRW_OPCODE_ELSE ||
          opcode == BRW_OPCODE_ENDIF ||
          opcode == BRW_OPCODE_WHILE;
}

static bool
has_uip(struct brw_context *brw, enum opcode opcode)
{
   if (brw->gen < 6)
      return false;

   return (brw->gen >= 7 && opcode == BRW_OPCODE_IF) ||
          (brw->gen >= 8 && opcode == BRW_OPCODE_ELSE) ||
          opcode == BRW_OPCODE_BREAK ||
          opcode == BRW_OPCODE_CONTINUE ||
          opcode == BRW_OPCODE_HALT;
}

static bool
is_logic_instruction(unsigned opcode)
{
   return opcode == BRW_OPCODE_AND ||
          opcode == BRW_OPCODE_NOT ||
          opcode == BRW_OPCODE_OR ||
          opcode == BRW_OPCODE_XOR;
}

const char *const conditional_modifier[16] = {
   [BRW_CONDITIONAL_NONE] = "",
   [BRW_CONDITIONAL_Z]    = ".e",
   [BRW_CONDITIONAL_NZ]   = ".ne",
   [BRW_CONDITIONAL_G]    = ".g",
   [BRW_CONDITIONAL_GE]   = ".ge",
   [BRW_CONDITIONAL_L]    = ".l",
   [BRW_CONDITIONAL_LE]   = ".le",
   [BRW_CONDITIONAL_R]    = ".r",
   [BRW_CONDITIONAL_O]    = ".o",
   [BRW_CONDITIONAL_U]    = ".u",
};

static const char *const m_negate[2] = {
   [0] = "",
   [1] = "-",
};

static const char *const _abs[2] = {
   [0] = "",
   [1] = "(abs)",
};

static const char *const m_bitnot[2] = { "", "~" };

static const char *const vert_stride[16] = {
   [0] = "0",
   [1] = "1",
   [2] = "2",
   [3] = "4",
   [4] = "8",
   [5] = "16",
   [6] = "32",
   [15] = "VxH",
};

static const char *const width[8] = {
   [0] = "1",
   [1] = "2",
   [2] = "4",
   [3] = "8",
   [4] = "16",
};

static const char *const horiz_stride[4] = {
   [0] = "0",
   [1] = "1",
   [2] = "2",
   [3] = "4"
};

static const char *const chan_sel[4] = {
   [0] = "x",
   [1] = "y",
   [2] = "z",
   [3] = "w",
};

static const char *const debug_ctrl[2] = {
   [0] = "",
   [1] = ".breakpoint"
};

static const char *const saturate[2] = {
   [0] = "",
   [1] = ".sat"
};

static const char *const cmpt_ctrl[2] = {
   [0] = "",
   [1] = "compacted"
};

static const char *const accwr[2] = {
   [0] = "",
   [1] = "AccWrEnable"
};

static const char *const wectrl[2] = {
   [0] = "",
   [1] = "WE_all"
};

static const char *const exec_size[8] = {
   [0] = "1",
   [1] = "2",
   [2] = "4",
   [3] = "8",
   [4] = "16",
   [5] = "32"
};

static const char *const pred_inv[2] = {
   [0] = "+",
   [1] = "-"
};

static const char *const pred_ctrl_align16[16] = {
   [1] = "",
   [2] = ".x",
   [3] = ".y",
   [4] = ".z",
   [5] = ".w",
   [6] = ".any4h",
   [7] = ".all4h",
};

static const char *const pred_ctrl_align1[16] = {
   [BRW_PREDICATE_NORMAL]        = "",
   [BRW_PREDICATE_ALIGN1_ANYV]   = ".anyv",
   [BRW_PREDICATE_ALIGN1_ALLV]   = ".allv",
   [BRW_PREDICATE_ALIGN1_ANY2H]  = ".any2h",
   [BRW_PREDICATE_ALIGN1_ALL2H]  = ".all2h",
   [BRW_PREDICATE_ALIGN1_ANY4H]  = ".any4h",
   [BRW_PREDICATE_ALIGN1_ALL4H]  = ".all4h",
   [BRW_PREDICATE_ALIGN1_ANY8H]  = ".any8h",
   [BRW_PREDICATE_ALIGN1_ALL8H]  = ".all8h",
   [BRW_PREDICATE_ALIGN1_ANY16H] = ".any16h",
   [BRW_PREDICATE_ALIGN1_ALL16H] = ".all16h",
   [BRW_PREDICATE_ALIGN1_ANY32H] = ".any32h",
   [BRW_PREDICATE_ALIGN1_ANY32H] = ".all32h",
};

static const char *const thread_ctrl[4] = {
   [BRW_THREAD_NORMAL] = "",
   [BRW_THREAD_ATOMIC] = "atomic",
   [BRW_THREAD_SWITCH] = "switch",
};

static const char *const compr_ctrl[4] = {
   [0] = "",
   [1] = "sechalf",
   [2] = "compr",
   [3] = "compr4",
};

static const char *const dep_ctrl[4] = {
   [0] = "",
   [1] = "NoDDClr",
   [2] = "NoDDChk",
   [3] = "NoDDClr,NoDDChk",
};

static const char *const mask_ctrl[4] = {
   [0] = "",
   [1] = "nomask",
};

static const char *const access_mode[2] = {
   [0] = "align1",
   [1] = "align16",
};

static const char * const reg_encoding[] = {
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

static const char *const three_source_reg_encoding[] = {
   [BRW_3SRC_TYPE_F]  = "F",
   [BRW_3SRC_TYPE_D]  = "D",
   [BRW_3SRC_TYPE_UD] = "UD",
};

const int reg_type_size[] = {
   [BRW_HW_REG_TYPE_UD]          = 4,
   [BRW_HW_REG_TYPE_D]           = 4,
   [BRW_HW_REG_TYPE_UW]          = 2,
   [BRW_HW_REG_TYPE_W]           = 2,
   [BRW_HW_REG_NON_IMM_TYPE_UB]  = 1,
   [BRW_HW_REG_NON_IMM_TYPE_B]   = 1,
   [GEN7_HW_REG_NON_IMM_TYPE_DF] = 8,
   [BRW_HW_REG_TYPE_F]           = 4,
   [GEN8_HW_REG_TYPE_UQ]         = 8,
   [GEN8_HW_REG_TYPE_Q]          = 8,
   [GEN8_HW_REG_NON_IMM_TYPE_HF] = 2,
};

static const char *const reg_file[4] = {
   [0] = "A",
   [1] = "g",
   [2] = "m",
   [3] = "imm",
};

static const char *const writemask[16] = {
   [0x0] = ".",
   [0x1] = ".x",
   [0x2] = ".y",
   [0x3] = ".xy",
   [0x4] = ".z",
   [0x5] = ".xz",
   [0x6] = ".yz",
   [0x7] = ".xyz",
   [0x8] = ".w",
   [0x9] = ".xw",
   [0xa] = ".yw",
   [0xb] = ".xyw",
   [0xc] = ".zw",
   [0xd] = ".xzw",
   [0xe] = ".yzw",
   [0xf] = "",
};

static const char *const end_of_thread[2] = {
   [0] = "",
   [1] = "EOT"
};

/* SFIDs on Gen4-5 */
static const char *const gen4_sfid[16] = {
   [BRW_SFID_NULL]            = "null",
   [BRW_SFID_MATH]            = "math",
   [BRW_SFID_SAMPLER]         = "sampler",
   [BRW_SFID_MESSAGE_GATEWAY] = "gateway",
   [BRW_SFID_DATAPORT_READ]   = "read",
   [BRW_SFID_DATAPORT_WRITE]  = "write",
   [BRW_SFID_URB]             = "urb",
   [BRW_SFID_THREAD_SPAWNER]  = "thread_spawner",
   [BRW_SFID_VME]             = "vme",
};

static const char *const gen6_sfid[16] = {
   [BRW_SFID_NULL]                     = "null",
   [BRW_SFID_MATH]                     = "math",
   [BRW_SFID_SAMPLER]                  = "sampler",
   [BRW_SFID_MESSAGE_GATEWAY]          = "gateway",
   [BRW_SFID_URB]                      = "urb",
   [BRW_SFID_THREAD_SPAWNER]           = "thread_spawner",
   [GEN6_SFID_DATAPORT_SAMPLER_CACHE]  = "sampler",
   [GEN6_SFID_DATAPORT_RENDER_CACHE]   = "render",
   [GEN6_SFID_DATAPORT_CONSTANT_CACHE] = "const",
   [GEN7_SFID_DATAPORT_DATA_CACHE]     = "data",
   [GEN7_SFID_PIXEL_INTERPOLATOR]      = "pixel interp",
   [HSW_SFID_DATAPORT_DATA_CACHE_1]    = "dp data 1",
   [HSW_SFID_CRE]                      = "cre",
};

static const char *const dp_write_port_msg_type[8] = {
   [0b000] = "OWord block write",
   [0b001] = "OWord dual block write",
   [0b010] = "media block write",
   [0b011] = "DWord scattered write",
   [0b100] = "RT write",
   [0b101] = "streamed VB write",
   [0b110] = "RT UNORM write", /* G45+ */
   [0b111] = "flush render cache",
};

static const char *const dp_rc_msg_type_gen6[16] = {
   [BRW_DATAPORT_READ_MESSAGE_OWORD_BLOCK_READ] = "OWORD block read",
   [GEN6_DATAPORT_READ_MESSAGE_RENDER_UNORM_READ] = "RT UNORM read",
   [GEN6_DATAPORT_READ_MESSAGE_OWORD_DUAL_BLOCK_READ] = "OWORD dual block read",
   [GEN6_DATAPORT_READ_MESSAGE_MEDIA_BLOCK_READ] = "media block read",
   [GEN6_DATAPORT_READ_MESSAGE_OWORD_UNALIGN_BLOCK_READ] =
      "OWORD unaligned block read",
   [GEN6_DATAPORT_READ_MESSAGE_DWORD_SCATTERED_READ] = "DWORD scattered read",
   [GEN6_DATAPORT_WRITE_MESSAGE_DWORD_ATOMIC_WRITE] = "DWORD atomic write",
   [GEN6_DATAPORT_WRITE_MESSAGE_OWORD_BLOCK_WRITE] = "OWORD block write",
   [GEN6_DATAPORT_WRITE_MESSAGE_OWORD_DUAL_BLOCK_WRITE] =
      "OWORD dual block write",
   [GEN6_DATAPORT_WRITE_MESSAGE_MEDIA_BLOCK_WRITE] = "media block write",
   [GEN6_DATAPORT_WRITE_MESSAGE_DWORD_SCATTERED_WRITE] =
      "DWORD scattered write",
   [GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE] = "RT write",
   [GEN6_DATAPORT_WRITE_MESSAGE_STREAMED_VB_WRITE] = "streamed VB write",
   [GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_UNORM_WRITE] = "RT UNORM write",
};

static const char *const m_rt_write_subtype[] = {
   [0b000] = "SIMD16",
   [0b001] = "SIMD16/RepData",
   [0b010] = "SIMD8/DualSrcLow",
   [0b011] = "SIMD8/DualSrcHigh",
   [0b100] = "SIMD8",
   [0b101] = "SIMD8/ImageWrite",   /* Gen6+ */
   [0b111] = "SIMD16/RepData-111", /* no idea how this is different than 1 */
};

static const char *const dp_dc0_msg_type_gen7[16] = {
   [GEN7_DATAPORT_DC_OWORD_BLOCK_READ] = "DC OWORD block read",
   [GEN7_DATAPORT_DC_UNALIGNED_OWORD_BLOCK_READ] =
      "DC unaligned OWORD block read",
   [GEN7_DATAPORT_DC_OWORD_DUAL_BLOCK_READ] = "DC OWORD dual block read",
   [GEN7_DATAPORT_DC_DWORD_SCATTERED_READ] = "DC DWORD scattered read",
   [GEN7_DATAPORT_DC_BYTE_SCATTERED_READ] = "DC byte scattered read",
   [GEN7_DATAPORT_DC_UNTYPED_ATOMIC_OP] = "DC untyped atomic",
   [GEN7_DATAPORT_DC_MEMORY_FENCE] = "DC mfence",
   [GEN7_DATAPORT_DC_OWORD_BLOCK_WRITE] = "DC OWORD block write",
   [GEN7_DATAPORT_DC_OWORD_DUAL_BLOCK_WRITE] = "DC OWORD dual block write",
   [GEN7_DATAPORT_DC_DWORD_SCATTERED_WRITE] = "DC DWORD scatterd write",
   [GEN7_DATAPORT_DC_BYTE_SCATTERED_WRITE] = "DC byte scattered write",
   [GEN7_DATAPORT_DC_UNTYPED_SURFACE_WRITE] = "DC untyped surface write",
};

static const char *const dp_dc1_msg_type_hsw[16] = {
   [HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ] = "untyped surface read",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP] = "DC untyped atomic op",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2] =
      "DC untyped 4x2 atomic op",
   [HSW_DATAPORT_DC_PORT1_MEDIA_BLOCK_READ] = "DC media block read",
   [HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ] = "DC typed surface read",
   [HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP] = "DC typed atomic",
   [HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2] = "DC typed 4x2 atomic op",
   [HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE] = "DC untyped surface write",
   [HSW_DATAPORT_DC_PORT1_MEDIA_BLOCK_WRITE] = "DC media block write",
   [HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP] = "DC atomic counter op",
   [HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP_SIMD4X2] =
      "DC 4x2 atomic counter op",
   [HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE] = "DC typed surface write",
};

static const char *const aop[16] = {
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

static const char * const pixel_interpolator_msg_types[4] = {
    [GEN7_PIXEL_INTERPOLATOR_LOC_SHARED_OFFSET] = "per_message_offset",
    [GEN7_PIXEL_INTERPOLATOR_LOC_SAMPLE] = "sample_position",
    [GEN7_PIXEL_INTERPOLATOR_LOC_CENTROID] = "centroid",
    [GEN7_PIXEL_INTERPOLATOR_LOC_PER_SLOT_OFFSET] = "per_slot_offset",
};

static const char *const math_function[16] = {
   [BRW_MATH_FUNCTION_INV]    = "inv",
   [BRW_MATH_FUNCTION_LOG]    = "log",
   [BRW_MATH_FUNCTION_EXP]    = "exp",
   [BRW_MATH_FUNCTION_SQRT]   = "sqrt",
   [BRW_MATH_FUNCTION_RSQ]    = "rsq",
   [BRW_MATH_FUNCTION_SIN]    = "sin",
   [BRW_MATH_FUNCTION_COS]    = "cos",
   [BRW_MATH_FUNCTION_SINCOS] = "sincos",
   [BRW_MATH_FUNCTION_FDIV]   = "fdiv",
   [BRW_MATH_FUNCTION_POW]    = "pow",
   [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER] = "intdivmod",
   [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT]  = "intdiv",
   [BRW_MATH_FUNCTION_INT_DIV_REMAINDER] = "intmod",
   [GEN8_MATH_FUNCTION_INVM]  = "invm",
   [GEN8_MATH_FUNCTION_RSQRTM] = "rsqrtm",
};

static const char *const math_saturate[2] = {
   [0] = "",
   [1] = "sat"
};

static const char *const math_signed[2] = {
   [0] = "",
   [1] = "signed"
};

static const char *const math_scalar[2] = {
   [0] = "",
   [1] = "scalar"
};

static const char *const math_precision[2] = {
   [0] = "",
   [1] = "partial_precision"
};

static const char *const gen5_urb_opcode[] = {
   [0] = "urb_write",
   [1] = "ff_sync",
};

static const char *const gen7_urb_opcode[] = {
   [0] = "write HWord",
   [1] = "write OWord",
   [2] = "read HWord",
   [3] = "read OWord",
   [4] = "atomic mov",  /* Gen7+ */
   [5] = "atomic inc",  /* Gen7+ */
   [6] = "atomic add",  /* Gen8+ */
   [7] = "SIMD8 write", /* Gen8+ */
   [8] = "SIMD8 read",  /* Gen8+ */
   /* [9-15] - reserved */
};

static const char *const urb_swizzle[4] = {
   [BRW_URB_SWIZZLE_NONE]       = "",
   [BRW_URB_SWIZZLE_INTERLEAVE] = "interleave",
   [BRW_URB_SWIZZLE_TRANSPOSE]  = "transpose",
};

static const char *const urb_allocate[2] = {
   [0] = "",
   [1] = "allocate"
};

static const char *const urb_used[2] = {
   [0] = "",
   [1] = "used"
};

static const char *const urb_complete[2] = {
   [0] = "",
   [1] = "complete"
};

static const char *const sampler_target_format[4] = {
   [0] = "F",
   [2] = "UD",
   [3] = "D"
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
   if (!opcode_descs[id].name) {
      format(file, "*** invalid opcode value %d ", id);
      return 1;
   }
   string(file, opcode_descs[id].name);
   return 0;
}

static int
reg(FILE *file, unsigned _reg_file, unsigned _reg_nr)
{
   int err = 0;

   /* Clear the Compr4 instruction compression bit. */
   if (_reg_file == BRW_MESSAGE_REGISTER_FILE)
      _reg_nr &= ~(1 << 7);

   if (_reg_file == BRW_ARCHITECTURE_REGISTER_FILE) {
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
      err |= control(file, "src reg file", reg_file, _reg_file, NULL);
      format(file, "%d", _reg_nr);
   }
   return err;
}

static int
dest(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   int err = 0;

   if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
      if (brw_inst_dst_address_mode(brw, inst) == BRW_ADDRESS_DIRECT) {
         err |= reg(file, brw_inst_dst_reg_file(brw, inst),
                    brw_inst_dst_da_reg_nr(brw, inst));
         if (err == -1)
            return 0;
         if (brw_inst_dst_da1_subreg_nr(brw, inst))
            format(file, ".%d", brw_inst_dst_da1_subreg_nr(brw, inst) /
                   reg_type_size[brw_inst_dst_reg_type(brw, inst)]);
         string(file, "<");
         err |= control(file, "horiz stride", horiz_stride,
                        brw_inst_dst_hstride(brw, inst), NULL);
         string(file, ">");
         err |= control(file, "dest reg encoding", reg_encoding,
                        brw_inst_dst_reg_type(brw, inst), NULL);
      } else {
         string(file, "g[a0");
         if (brw_inst_dst_ia_subreg_nr(brw, inst))
            format(file, ".%d", brw_inst_dst_ia_subreg_nr(brw, inst) /
                   reg_type_size[brw_inst_dst_reg_type(brw, inst)]);
         if (brw_inst_dst_ia1_addr_imm(brw, inst))
            format(file, " %d", brw_inst_dst_ia1_addr_imm(brw, inst));
         string(file, "]<");
         err |= control(file, "horiz stride", horiz_stride,
                        brw_inst_dst_hstride(brw, inst), NULL);
         string(file, ">");
         err |= control(file, "dest reg encoding", reg_encoding,
                        brw_inst_dst_reg_type(brw, inst), NULL);
      }
   } else {
      if (brw_inst_dst_address_mode(brw, inst) == BRW_ADDRESS_DIRECT) {
         err |= reg(file, brw_inst_dst_reg_file(brw, inst),
                    brw_inst_dst_da_reg_nr(brw, inst));
         if (err == -1)
            return 0;
         if (brw_inst_dst_da16_subreg_nr(brw, inst))
            format(file, ".%d", brw_inst_dst_da16_subreg_nr(brw, inst) /
                   reg_type_size[brw_inst_dst_reg_type(brw, inst)]);
         string(file, "<1>");
         err |= control(file, "writemask", writemask,
                        brw_inst_da16_writemask(brw, inst), NULL);
         err |= control(file, "dest reg encoding", reg_encoding,
                        brw_inst_dst_reg_type(brw, inst), NULL);
      } else {
         err = 1;
         string(file, "Indirect align16 address mode not supported");
      }
   }

   return 0;
}

static int
dest_3src(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   int err = 0;
   uint32_t reg_file;

   if (brw->gen == 6 && brw_inst_3src_dst_reg_file(brw, inst))
      reg_file = BRW_MESSAGE_REGISTER_FILE;
   else
      reg_file = BRW_GENERAL_REGISTER_FILE;

   err |= reg(file, reg_file, brw_inst_3src_dst_reg_nr(brw, inst));
   if (err == -1)
      return 0;
   if (brw_inst_3src_dst_subreg_nr(brw, inst))
      format(file, ".%d", brw_inst_3src_dst_subreg_nr(brw, inst));
   string(file, "<1>");
   err |= control(file, "writemask", writemask,
                  brw_inst_3src_dst_writemask(brw, inst), NULL);
   err |= control(file, "dest reg encoding", three_source_reg_encoding,
                  brw_inst_3src_dst_type(brw, inst), NULL);

   return 0;
}

static int
src_align1_region(FILE *file,
                  unsigned _vert_stride, unsigned _width,
                  unsigned _horiz_stride)
{
   int err = 0;
   string(file, "<");
   err |= control(file, "vert stride", vert_stride, _vert_stride, NULL);
   string(file, ",");
   err |= control(file, "width", width, _width, NULL);
   string(file, ",");
   err |= control(file, "horiz_stride", horiz_stride, _horiz_stride, NULL);
   string(file, ">");
   return err;
}

static int
src_da1(FILE *file,
        const struct brw_context *brw,
        unsigned opcode,
        unsigned type, unsigned _reg_file,
        unsigned _vert_stride, unsigned _width, unsigned _horiz_stride,
        unsigned reg_num, unsigned sub_reg_num, unsigned __abs,
        unsigned _negate)
{
   int err = 0;

   if (brw->gen >= 8 && is_logic_instruction(opcode))
      err |= control(file, "bitnot", m_bitnot, _negate, NULL);
   else
      err |= control(file, "negate", m_negate, _negate, NULL);

   err |= control(file, "abs", _abs, __abs, NULL);

   err |= reg(file, _reg_file, reg_num);
   if (err == -1)
      return 0;
   if (sub_reg_num)
      format(file, ".%d", sub_reg_num / reg_type_size[type]);   /* use formal style like spec */
   src_align1_region(file, _vert_stride, _width, _horiz_stride);
   err |= control(file, "src reg encoding", reg_encoding, type, NULL);
   return err;
}

static int
src_ia1(FILE *file,
        const struct brw_context *brw,
        unsigned opcode,
        unsigned type,
        unsigned _reg_file,
        int _addr_imm,
        unsigned _addr_subreg_nr,
        unsigned _negate,
        unsigned __abs,
        unsigned _addr_mode,
        unsigned _horiz_stride, unsigned _width, unsigned _vert_stride)
{
   int err = 0;

   if (brw->gen >= 8 && is_logic_instruction(opcode))
      err |= control(file, "bitnot", m_bitnot, _negate, NULL);
   else
      err |= control(file, "negate", m_negate, _negate, NULL);

   err |= control(file, "abs", _abs, __abs, NULL);

   string(file, "g[a0");
   if (_addr_subreg_nr)
      format(file, ".%d", _addr_subreg_nr);
   if (_addr_imm)
      format(file, " %d", _addr_imm);
   string(file, "]");
   src_align1_region(file, _vert_stride, _width, _horiz_stride);
   err |= control(file, "src reg encoding", reg_encoding, type, NULL);
   return err;
}

static int
src_swizzle(FILE *file, unsigned swiz)
{
   unsigned x = BRW_GET_SWZ(swiz, BRW_CHANNEL_X);
   unsigned y = BRW_GET_SWZ(swiz, BRW_CHANNEL_Y);
   unsigned z = BRW_GET_SWZ(swiz, BRW_CHANNEL_Z);
   unsigned w = BRW_GET_SWZ(swiz, BRW_CHANNEL_W);
   int err = 0;

   if (x == y && x == z && x == w) {
      string(file, ".");
      err |= control(file, "channel select", chan_sel, x, NULL);
   } else if (swiz != BRW_SWIZZLE_XYZW) {
      string(file, ".");
      err |= control(file, "channel select", chan_sel, x, NULL);
      err |= control(file, "channel select", chan_sel, y, NULL);
      err |= control(file, "channel select", chan_sel, z, NULL);
      err |= control(file, "channel select", chan_sel, w, NULL);
   }
   return err;
}

static int
src_da16(FILE *file,
         const struct brw_context *brw,
         unsigned opcode,
         unsigned _reg_type,
         unsigned _reg_file,
         unsigned _vert_stride,
         unsigned _reg_nr,
         unsigned _subreg_nr,
         unsigned __abs,
         unsigned _negate,
         unsigned swz_x, unsigned swz_y, unsigned swz_z, unsigned swz_w)
{
   int err = 0;

   if (brw->gen >= 8 && is_logic_instruction(opcode))
      err |= control(file, "bitnot", m_bitnot, _negate, NULL);
   else
      err |= control(file, "negate", m_negate, _negate, NULL);

   err |= control(file, "abs", _abs, __abs, NULL);

   err |= reg(file, _reg_file, _reg_nr);
   if (err == -1)
      return 0;
   if (_subreg_nr)
      /* bit4 for subreg number byte addressing. Make this same meaning as
         in da1 case, so output looks consistent. */
      format(file, ".%d", 16 / reg_type_size[_reg_type]);
   string(file, "<");
   err |= control(file, "vert stride", vert_stride, _vert_stride, NULL);
   string(file, ",4,1>");
   err |= src_swizzle(file, BRW_SWIZZLE4(swz_x, swz_y, swz_z, swz_w));
   err |= control(file, "src da16 reg type", reg_encoding, _reg_type, NULL);
   return err;
}

static int
src0_3src(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   int err = 0;
   unsigned src0_subreg_nr = brw_inst_3src_src0_subreg_nr(brw, inst);

   err |= control(file, "negate", m_negate,
                  brw_inst_3src_src0_negate(brw, inst), NULL);
   err |= control(file, "abs", _abs, brw_inst_3src_src0_abs(brw, inst), NULL);

   err |= reg(file, BRW_GENERAL_REGISTER_FILE,
              brw_inst_3src_src0_reg_nr(brw, inst));
   if (err == -1)
      return 0;
   if (src0_subreg_nr)
      format(file, ".%d", src0_subreg_nr);
   if (brw_inst_3src_src0_rep_ctrl(brw, inst))
      string(file, "<0,1,0>");
   else
      string(file, "<4,4,1>");
   err |= control(file, "src da16 reg type", three_source_reg_encoding,
                  brw_inst_3src_src_type(brw, inst), NULL);
   err |= src_swizzle(file, brw_inst_3src_src0_swizzle(brw, inst));
   return err;
}

static int
src1_3src(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   int err = 0;
   unsigned src1_subreg_nr = brw_inst_3src_src1_subreg_nr(brw, inst);

   err |= control(file, "negate", m_negate,
                  brw_inst_3src_src1_negate(brw, inst), NULL);
   err |= control(file, "abs", _abs, brw_inst_3src_src1_abs(brw, inst), NULL);

   err |= reg(file, BRW_GENERAL_REGISTER_FILE,
              brw_inst_3src_src1_reg_nr(brw, inst));
   if (err == -1)
      return 0;
   if (src1_subreg_nr)
      format(file, ".%d", src1_subreg_nr);
   if (brw_inst_3src_src1_rep_ctrl(brw, inst))
      string(file, "<0,1,0>");
   else
      string(file, "<4,4,1>");
   err |= control(file, "src da16 reg type", three_source_reg_encoding,
                  brw_inst_3src_src_type(brw, inst), NULL);
   err |= src_swizzle(file, brw_inst_3src_src1_swizzle(brw, inst));
   return err;
}


static int
src2_3src(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   int err = 0;
   unsigned src2_subreg_nr = brw_inst_3src_src2_subreg_nr(brw, inst);

   err |= control(file, "negate", m_negate,
                  brw_inst_3src_src2_negate(brw, inst), NULL);
   err |= control(file, "abs", _abs, brw_inst_3src_src2_abs(brw, inst), NULL);

   err |= reg(file, BRW_GENERAL_REGISTER_FILE,
              brw_inst_3src_src2_reg_nr(brw, inst));
   if (err == -1)
      return 0;
   if (src2_subreg_nr)
      format(file, ".%d", src2_subreg_nr);
   if (brw_inst_3src_src2_rep_ctrl(brw, inst))
      string(file, "<0,1,0>");
   else
      string(file, "<4,4,1>");
   err |= control(file, "src da16 reg type", three_source_reg_encoding,
                  brw_inst_3src_src_type(brw, inst), NULL);
   err |= src_swizzle(file, brw_inst_3src_src2_swizzle(brw, inst));
   return err;
}

static int
imm(FILE *file, struct brw_context *brw, unsigned type, brw_inst *inst)
{
   switch (type) {
   case BRW_HW_REG_TYPE_UD:
      format(file, "0x%08xUD", brw_inst_imm_ud(brw, inst));
      break;
   case BRW_HW_REG_TYPE_D:
      format(file, "%dD", brw_inst_imm_d(brw, inst));
      break;
   case BRW_HW_REG_TYPE_UW:
      format(file, "0x%04xUW", (uint16_t) brw_inst_imm_ud(brw, inst));
      break;
   case BRW_HW_REG_TYPE_W:
      format(file, "%dW", (int16_t) brw_inst_imm_d(brw, inst));
      break;
   case BRW_HW_REG_IMM_TYPE_UV:
      format(file, "0x%08xUV", brw_inst_imm_ud(brw, inst));
      break;
   case BRW_HW_REG_IMM_TYPE_VF:
      format(file, "Vector Float");
      break;
   case BRW_HW_REG_IMM_TYPE_V:
      format(file, "0x%08xV", brw_inst_imm_ud(brw, inst));
      break;
   case BRW_HW_REG_TYPE_F:
      format(file, "%-gF", brw_inst_imm_f(brw, inst));
      break;
   case GEN8_HW_REG_IMM_TYPE_DF:
      string(file, "Double IMM");
      break;
   case GEN8_HW_REG_IMM_TYPE_HF:
      string(file, "Half Float IMM");
      break;
   }
   return 0;
}

static int
src0(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   if (brw_inst_src0_reg_file(brw, inst) == BRW_IMMEDIATE_VALUE) {
      return imm(file, brw, brw_inst_src0_reg_type(brw, inst), inst);
   } else if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
      if (brw_inst_src0_address_mode(brw, inst) == BRW_ADDRESS_DIRECT) {
         return src_da1(file,
                        brw,
                        brw_inst_opcode(brw, inst),
                        brw_inst_src0_reg_type(brw, inst),
                        brw_inst_src0_reg_file(brw, inst),
                        brw_inst_src0_vstride(brw, inst),
                        brw_inst_src0_width(brw, inst),
                        brw_inst_src0_hstride(brw, inst),
                        brw_inst_src0_da_reg_nr(brw, inst),
                        brw_inst_src0_da1_subreg_nr(brw, inst),
                        brw_inst_src0_abs(brw, inst),
                        brw_inst_src0_negate(brw, inst));
      } else {
         return src_ia1(file,
                        brw,
                        brw_inst_opcode(brw, inst),
                        brw_inst_src0_reg_type(brw, inst),
                        brw_inst_src0_reg_file(brw, inst),
                        brw_inst_src0_ia1_addr_imm(brw, inst),
                        brw_inst_src0_ia_subreg_nr(brw, inst),
                        brw_inst_src0_negate(brw, inst),
                        brw_inst_src0_abs(brw, inst),
                        brw_inst_src0_address_mode(brw, inst),
                        brw_inst_src0_hstride(brw, inst),
                        brw_inst_src0_width(brw, inst),
                        brw_inst_src0_vstride(brw, inst));
      }
   } else {
      if (brw_inst_src0_address_mode(brw, inst) == BRW_ADDRESS_DIRECT) {
         return src_da16(file,
                         brw,
                         brw_inst_opcode(brw, inst),
                         brw_inst_src0_reg_type(brw, inst),
                         brw_inst_src0_reg_file(brw, inst),
                         brw_inst_src0_vstride(brw, inst),
                         brw_inst_src0_da_reg_nr(brw, inst),
                         brw_inst_src0_da16_subreg_nr(brw, inst),
                         brw_inst_src0_abs(brw, inst),
                         brw_inst_src0_negate(brw, inst),
                         brw_inst_src0_da16_swiz_x(brw, inst),
                         brw_inst_src0_da16_swiz_y(brw, inst),
                         brw_inst_src0_da16_swiz_z(brw, inst),
                         brw_inst_src0_da16_swiz_w(brw, inst));
      } else {
         string(file, "Indirect align16 address mode not supported");
         return 1;
      }
   }
}

static int
src1(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   if (brw_inst_src1_reg_file(brw, inst) == BRW_IMMEDIATE_VALUE) {
      return imm(file, brw, brw_inst_src1_reg_type(brw, inst), inst);
   } else if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
      if (brw_inst_src1_address_mode(brw, inst) == BRW_ADDRESS_DIRECT) {
         return src_da1(file,
                        brw,
                        brw_inst_opcode(brw, inst),
                        brw_inst_src1_reg_type(brw, inst),
                        brw_inst_src1_reg_file(brw, inst),
                        brw_inst_src1_vstride(brw, inst),
                        brw_inst_src1_width(brw, inst),
                        brw_inst_src1_hstride(brw, inst),
                        brw_inst_src1_da_reg_nr(brw, inst),
                        brw_inst_src1_da1_subreg_nr(brw, inst),
                        brw_inst_src1_abs(brw, inst),
                        brw_inst_src1_negate(brw, inst));
      } else {
         return src_ia1(file,
                        brw,
                        brw_inst_opcode(brw, inst),
                        brw_inst_src1_reg_type(brw, inst),
                        brw_inst_src1_reg_file(brw, inst),
                        brw_inst_src1_ia1_addr_imm(brw, inst),
                        brw_inst_src1_ia_subreg_nr(brw, inst),
                        brw_inst_src1_negate(brw, inst),
                        brw_inst_src1_abs(brw, inst),
                        brw_inst_src1_address_mode(brw, inst),
                        brw_inst_src1_hstride(brw, inst),
                        brw_inst_src1_width(brw, inst),
                        brw_inst_src1_vstride(brw, inst));
      }
   } else {
      if (brw_inst_src1_address_mode(brw, inst) == BRW_ADDRESS_DIRECT) {
         return src_da16(file,
                         brw,
                         brw_inst_opcode(brw, inst),
                         brw_inst_src1_reg_type(brw, inst),
                         brw_inst_src1_reg_file(brw, inst),
                         brw_inst_src1_vstride(brw, inst),
                         brw_inst_src1_da_reg_nr(brw, inst),
                         brw_inst_src1_da16_subreg_nr(brw, inst),
                         brw_inst_src1_abs(brw, inst),
                         brw_inst_src1_negate(brw, inst),
                         brw_inst_src1_da16_swiz_x(brw, inst),
                         brw_inst_src1_da16_swiz_y(brw, inst),
                         brw_inst_src1_da16_swiz_z(brw, inst),
                         brw_inst_src1_da16_swiz_w(brw, inst));
      } else {
         string(file, "Indirect align16 address mode not supported");
         return 1;
      }
   }
}

static int
qtr_ctrl(FILE *file, struct brw_context *brw, brw_inst *inst)
{
   int qtr_ctl = brw_inst_qtr_control(brw, inst);
   int exec_size = 1 << brw_inst_exec_size(brw, inst);

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
brw_disassemble_inst(FILE *file, struct brw_context *brw, brw_inst *inst,
                     bool is_compacted)
{
   int err = 0;
   int space = 0;

   const enum opcode opcode = brw_inst_opcode(brw, inst);

   if (brw_inst_pred_control(brw, inst)) {
      string(file, "(");
      err |= control(file, "predicate inverse", pred_inv,
                     brw_inst_pred_inv(brw, inst), NULL);
      format(file, "f%d", brw->gen >= 7 ? brw_inst_flag_reg_nr(brw, inst) : 0);
      if (brw_inst_flag_subreg_nr(brw, inst))
         format(file, ".%d", brw_inst_flag_subreg_nr(brw, inst));
      if (brw_inst_access_mode(brw, inst) == BRW_ALIGN_1) {
         err |= control(file, "predicate control align1", pred_ctrl_align1,
                        brw_inst_pred_control(brw, inst), NULL);
      } else {
         err |= control(file, "predicate control align16", pred_ctrl_align16,
                        brw_inst_pred_control(brw, inst), NULL);
      }
      string(file, ") ");
   }

   err |= print_opcode(file, opcode);
   err |= control(file, "saturate", saturate, brw_inst_saturate(brw, inst),
                  NULL);

   err |= control(file, "debug control", debug_ctrl,
                  brw_inst_debug_control(brw, inst), NULL);

   if (opcode == BRW_OPCODE_MATH) {
      string(file, " ");
      err |= control(file, "function", math_function,
                     brw_inst_math_function(brw, inst), NULL);
   } else if (opcode != BRW_OPCODE_SEND && opcode != BRW_OPCODE_SENDC) {
      err |= control(file, "conditional modifier", conditional_modifier,
                     brw_inst_cond_modifier(brw, inst), NULL);

      /* If we're using the conditional modifier, print which flags reg is
       * used for it.  Note that on gen6+, the embedded-condition SEL and
       * control flow doesn't update flags.
       */
      if (brw_inst_cond_modifier(brw, inst) &&
          (brw->gen < 6 || (opcode != BRW_OPCODE_SEL &&
                            opcode != BRW_OPCODE_IF &&
                            opcode != BRW_OPCODE_WHILE))) {
         format(file, ".f%d",
                brw->gen >= 7 ? brw_inst_flag_reg_nr(brw, inst) : 0);
         if (brw_inst_flag_subreg_nr(brw, inst))
            format(file, ".%d", brw_inst_flag_subreg_nr(brw, inst));
      }
   }

   if (opcode != BRW_OPCODE_NOP) {
      string(file, "(");
      err |= control(file, "execution size", exec_size,
                     brw_inst_exec_size(brw, inst), NULL);
      string(file, ")");
   }

   if (opcode == BRW_OPCODE_SEND && brw->gen < 6)
      format(file, " %d", brw_inst_base_mrf(brw, inst));

   if (has_uip(brw, opcode)) {
      /* Instructions that have UIP also have JIP. */
      pad(file, 16);
      format(file, "JIP: %d", brw_inst_jip(brw, inst));
      pad(file, 32);
      format(file, "UIP: %d", brw_inst_uip(brw, inst));
   } else if (has_jip(brw, opcode)) {
      pad(file, 16);
      if (brw->gen >= 7) {
         format(file, "JIP: %d", brw_inst_jip(brw, inst));
      } else {
         format(file, "JIP: %d", brw_inst_gen6_jump_count(brw, inst));
      }
   } else if (brw->gen < 6 && (opcode == BRW_OPCODE_BREAK ||
                               opcode == BRW_OPCODE_CONTINUE ||
                               opcode == BRW_OPCODE_ELSE)) {
      pad(file, 16);
      format(file, "Jump: %d", brw_inst_gen4_jump_count(brw, inst));
      pad(file, 32);
      format(file, "Pop: %d", brw_inst_gen4_pop_count(brw, inst));
   } else if (brw->gen < 6 && (opcode == BRW_OPCODE_IF ||
                               opcode == BRW_OPCODE_IFF ||
                               opcode == BRW_OPCODE_HALT)) {
      pad(file, 16);
      format(file, "Jump: %d", brw_inst_gen4_pop_count(brw, inst));
   } else if (brw->gen < 6 && opcode == BRW_OPCODE_ENDIF) {
      pad(file, 16);
      format(file, "Pop: %d", brw_inst_gen4_pop_count(brw, inst));
   } else if (opcode == BRW_OPCODE_JMPI) {
      format(file, " %d", brw_inst_imm_d(brw, inst));
   } else if (opcode_descs[opcode].nsrc == 3) {
      pad(file, 16);
      err |= dest_3src(file, brw, inst);

      pad(file, 32);
      err |= src0_3src(file, brw, inst);

      pad(file, 48);
      err |= src1_3src(file, brw, inst);

      pad(file, 64);
      err |= src2_3src(file, brw, inst);
   } else {
      if (opcode_descs[opcode].ndst > 0) {
         pad(file, 16);
         err |= dest(file, brw, inst);
      }

      if (opcode_descs[opcode].nsrc > 0) {
         pad(file, 32);
         err |= src0(file, brw, inst);
      }

      if (opcode_descs[opcode].nsrc > 1) {
         pad(file, 48);
         err |= src1(file, brw, inst);
      }
   }

   if (opcode == BRW_OPCODE_SEND || opcode == BRW_OPCODE_SENDC) {
      enum brw_message_target sfid = brw_inst_sfid(brw, inst);

      if (brw_inst_src1_reg_file(brw, inst) != BRW_IMMEDIATE_VALUE) {
         /* show the indirect descriptor source */
         pad(file, 48);
         err |= src1(file, brw, inst);
      }

      newline(file);
      pad(file, 16);
      space = 0;

      fprintf(file, "            ");
      err |= control(file, "SFID", brw->gen >= 6 ? gen6_sfid : gen4_sfid,
                     sfid, &space);


      if (brw_inst_src1_reg_file(brw, inst) != BRW_IMMEDIATE_VALUE) {
         format(file, " indirect");
      } else {
         switch (sfid) {
         case BRW_SFID_MATH:
            err |= control(file, "math function", math_function,
                           brw_inst_math_msg_function(brw, inst), &space);
            err |= control(file, "math saturate", math_saturate,
                           brw_inst_math_msg_saturate(brw, inst), &space);
            err |= control(file, "math signed", math_signed,
                           brw_inst_math_msg_signed_int(brw, inst), &space);
            err |= control(file, "math scalar", math_scalar,
                           brw_inst_math_msg_data_type(brw, inst), &space);
            err |= control(file, "math precision", math_precision,
                           brw_inst_math_msg_precision(brw, inst), &space);
            break;
         case BRW_SFID_SAMPLER:
            if (brw->gen >= 5) {
               format(file, " (%d, %d, %d, %d)",
                      brw_inst_binding_table_index(brw, inst),
                      brw_inst_sampler(brw, inst),
                      brw_inst_sampler_msg_type(brw, inst),
                      brw_inst_sampler_simd_mode(brw, inst));
            } else {
               format(file, " (%d, %d, %d, ",
                      brw_inst_binding_table_index(brw, inst),
                      brw_inst_sampler(brw, inst),
                      brw_inst_sampler_msg_type(brw, inst));
               if (!brw->is_g4x) {
                  err |= control(file, "sampler target format",
                                 sampler_target_format,
                                 brw_inst_sampler_return_format(brw, inst), NULL);
               }
               string(file, ")");
            }
            break;
         case GEN6_SFID_DATAPORT_SAMPLER_CACHE:
            /* aka BRW_SFID_DATAPORT_READ on Gen4-5 */
            if (brw->gen >= 6) {
               format(file, " (%d, %d, %d, %d)",
                      brw_inst_binding_table_index(brw, inst),
                      brw_inst_dp_msg_control(brw, inst),
                      brw_inst_dp_msg_type(brw, inst),
                      brw->gen >= 7 ? 0 : brw_inst_dp_write_commit(brw, inst));
            } else {
               format(file, " (%d, %d, %d)",
                      brw_inst_binding_table_index(brw, inst),
                      brw_inst_dp_read_msg_control(brw, inst),
                      brw_inst_dp_read_msg_type(brw, inst));
            }
            break;

         case GEN6_SFID_DATAPORT_RENDER_CACHE: {
            /* aka BRW_SFID_DATAPORT_WRITE on Gen4-5 */
            unsigned msg_type = brw_inst_dp_write_msg_type(brw, inst);

            err |= control(file, "DP rc message type",
                           brw->gen >= 6 ? dp_rc_msg_type_gen6
                                         : dp_write_port_msg_type,
                           msg_type, &space);

            bool is_rt_write = msg_type ==
               (brw->gen >= 6 ? GEN6_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE
                              : BRW_DATAPORT_WRITE_MESSAGE_RENDER_TARGET_WRITE);

            if (is_rt_write) {
               err |= control(file, "RT message type", m_rt_write_subtype,
                              brw_inst_rt_message_type(brw, inst), &space);
               if (brw->gen >= 6 && brw_inst_rt_slot_group(brw, inst))
                  string(file, " Hi");
               if (brw_inst_rt_last(brw, inst))
                  string(file, " LastRT");
               if (brw->gen < 7 && brw_inst_dp_write_commit(brw, inst))
                  string(file, " WriteCommit");
            } else {
               format(file, " MsgCtrl = 0x%x",
                      brw_inst_dp_write_msg_control(brw, inst));
            }

            format(file, " Surface = %d", brw_inst_binding_table_index(brw, inst));
            break;
         }

         case BRW_SFID_URB:
            format(file, " %d", brw_inst_urb_global_offset(brw, inst));

            space = 1;
            if (brw->gen >= 7) {
               err |= control(file, "urb opcode", gen7_urb_opcode,
                              brw_inst_urb_opcode(brw, inst), &space);
            } else if (brw->gen >= 5) {
               err |= control(file, "urb opcode", gen5_urb_opcode,
                              brw_inst_urb_opcode(brw, inst), &space);
            }
            err |= control(file, "urb swizzle", urb_swizzle,
                           brw_inst_urb_swizzle_control(brw, inst), &space);
            if (brw->gen < 7) {
               err |= control(file, "urb allocate", urb_allocate,
                              brw_inst_urb_allocate(brw, inst), &space);
               err |= control(file, "urb used", urb_used,
                              brw_inst_urb_used(brw, inst), &space);
            }
            if (brw->gen < 8) {
               err |= control(file, "urb complete", urb_complete,
                              brw_inst_urb_complete(brw, inst), &space);
            }
            break;
         case BRW_SFID_THREAD_SPAWNER:
            break;
         case GEN7_SFID_DATAPORT_DATA_CACHE:
            if (brw->gen >= 7) {
               format(file, " (");

               err |= control(file, "DP DC0 message type",
                              dp_dc0_msg_type_gen7,
                              brw_inst_dp_msg_type(brw, inst), &space);

               format(file, ", %d, ", brw_inst_binding_table_index(brw, inst));

               switch (brw_inst_dp_msg_type(brw, inst)) {
               case GEN7_DATAPORT_DC_UNTYPED_ATOMIC_OP:
                  control(file, "atomic op", aop,
                          brw_inst_imm_ud(brw, inst) >> 8 & 0xf, &space);
                  break;
               default:
                  format(file, "%d", brw_inst_dp_msg_control(brw, inst));
               }
               format(file, ")");
               break;
            }
            /* FALLTHROUGH */

         case HSW_SFID_DATAPORT_DATA_CACHE_1: {
            if (brw->gen >= 7) {
               format(file, " (");

               unsigned msg_ctrl = brw_inst_dp_msg_control(brw, inst);

               err |= control(file, "DP DC1 message type",
                              dp_dc1_msg_type_hsw,
                              brw_inst_dp_msg_type(brw, inst), &space);

               format(file, ", Surface = %d, ",
                      brw_inst_binding_table_index(brw, inst));

               switch (brw_inst_dp_msg_type(brw, inst)) {
               case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP:
               case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP:
               case HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP:
                  format(file, "SIMD%d,", (msg_ctrl & (1 << 4)) ? 8 : 16);
                  /* fallthrough */
               case HSW_DATAPORT_DC_PORT1_UNTYPED_ATOMIC_OP_SIMD4X2:
               case HSW_DATAPORT_DC_PORT1_TYPED_ATOMIC_OP_SIMD4X2:
               case HSW_DATAPORT_DC_PORT1_ATOMIC_COUNTER_OP_SIMD4X2:
                  control(file, "atomic op", aop, msg_ctrl & 0xf, &space);
                  break;
               case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_READ:
               case HSW_DATAPORT_DC_PORT1_UNTYPED_SURFACE_WRITE:
               case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_READ:
               case HSW_DATAPORT_DC_PORT1_TYPED_SURFACE_WRITE: {
                  static const char *simd_modes[] = { "4x2", "16", "8" };
                  format(file, "SIMD%s, Mask = 0x%x",
                         simd_modes[msg_ctrl >> 4], msg_ctrl & 0xf);
                  break;
               }
               default:
                  format(file, "0x%x", msg_ctrl);
               }
               format(file, ")");
               break;
            }
            /* FALLTHROUGH */
         }

         case GEN7_SFID_PIXEL_INTERPOLATOR:
            if (brw->gen >= 7) {
               format(file, " (%s, %s, 0x%02x)",
                      brw_inst_pi_nopersp(brw, inst) ? "linear" : "persp",
                      pixel_interpolator_msg_types[brw_inst_pi_message_type(brw, inst)],
                      brw_inst_pi_message_data(brw, inst));
               break;
            }
            /* FALLTHROUGH */

         default:
            format(file, "unsupported shared function ID %d", sfid);
            break;
         }

         if (space)
            string(file, " ");
         format(file, "mlen %d", brw_inst_mlen(brw, inst));
         format(file, " rlen %d", brw_inst_rlen(brw, inst));
      }
   }
   pad(file, 64);
   if (opcode != BRW_OPCODE_NOP) {
      string(file, "{");
      space = 1;
      err |= control(file, "access mode", access_mode,
                     brw_inst_access_mode(brw, inst), &space);
      if (brw->gen >= 6) {
         err |= control(file, "write enable control", wectrl,
                        brw_inst_mask_control(brw, inst), &space);
      } else {
         err |= control(file, "mask control", mask_ctrl,
                        brw_inst_mask_control(brw, inst), &space);
      }
      err |= control(file, "dependency control", dep_ctrl,
                     ((brw_inst_no_dd_check(brw, inst) << 1) |
                      brw_inst_no_dd_clear(brw, inst)), &space);

      if (brw->gen >= 6)
         err |= qtr_ctrl(file, brw, inst);
      else {
         if (brw_inst_qtr_control(brw, inst) == BRW_COMPRESSION_COMPRESSED &&
             opcode_descs[opcode].ndst > 0 &&
             brw_inst_dst_reg_file(brw, inst) == BRW_MESSAGE_REGISTER_FILE &&
             brw_inst_dst_da_reg_nr(brw, inst) & (1 << 7)) {
            format(file, " compr4");
         } else {
            err |= control(file, "compression control", compr_ctrl,
                           brw_inst_qtr_control(brw, inst), &space);
         }
      }

      err |= control(file, "compaction", cmpt_ctrl, is_compacted, &space);
      err |= control(file, "thread control", thread_ctrl,
                     brw_inst_thread_control(brw, inst), &space);
      if (brw->gen >= 6)
         err |= control(file, "acc write control", accwr,
                        brw_inst_acc_wr_control(brw, inst), &space);
      if (opcode == BRW_OPCODE_SEND || opcode == BRW_OPCODE_SENDC)
         err |= control(file, "end of thread", end_of_thread,
                        brw_inst_eot(brw, inst), &space);
      if (space)
         string(file, " ");
      string(file, "}");
   }
   string(file, ";");
   newline(file);
   return err;
}
