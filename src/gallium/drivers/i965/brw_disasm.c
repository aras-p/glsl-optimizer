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

#include "brw_disasm.h"
#include "brw_structs.h"
#include "brw_reg.h"
#include "brw_defines.h"

struct {
    char    *name;
    int	    nsrc;
    int	    ndst;
} opcode[128] = {
    [BRW_OPCODE_MOV] = { .name = "mov", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_FRC] = { .name = "frc", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDU] = { .name = "rndu", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDD] = { .name = "rndd", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDE] = { .name = "rnde", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_RNDZ] = { .name = "rndz", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_NOT] = { .name = "not", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_LZD] = { .name = "lzd", .nsrc = 1, .ndst = 1 },

    [BRW_OPCODE_MUL] = { .name = "mul", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_MAC] = { .name = "mac", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_MACH] = { .name = "mach", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_LINE] = { .name = "line", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SAD2] = { .name = "sad2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SADA2] = { .name = "sada2", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP4] = { .name = "dp4", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DPH] = { .name = "dph", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP3] = { .name = "dp3", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_DP2] = { .name = "dp2", .nsrc = 2, .ndst = 1 },

    [BRW_OPCODE_AVG] = { .name = "avg", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_ADD] = { .name = "add", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SEL] = { .name = "sel", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_AND] = { .name = "and", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_OR] = { .name = "or", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_XOR] = { .name = "xor", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SHR] = { .name = "shr", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_SHL] = { .name = "shl", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_ASR] = { .name = "asr", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_CMP] = { .name = "cmp", .nsrc = 2, .ndst = 1 },
    [BRW_OPCODE_CMPN] = { .name = "cmpn", .nsrc = 2, .ndst = 1 },

    [BRW_OPCODE_SEND] = { .name = "send", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_NOP] = { .name = "nop", .nsrc = 0, .ndst = 0 },
    [BRW_OPCODE_JMPI] = { .name = "jmpi", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_IF] = { .name = "if", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_IFF] = { .name = "iff", .nsrc = 1, .ndst = 01 },
    [BRW_OPCODE_WHILE] = { .name = "while", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_ELSE] = { .name = "else", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_BREAK] = { .name = "break", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_CONTINUE] = { .name = "cont", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_HALT] = { .name = "halt", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_MSAVE] = { .name = "msave", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_PUSH] = { .name = "push", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_MRESTORE] = { .name = "mrest", .nsrc = 1, .ndst = 1 },
    [BRW_OPCODE_POP] = { .name = "pop", .nsrc = 2, .ndst = 0 },
    [BRW_OPCODE_WAIT] = { .name = "wait", .nsrc = 1, .ndst = 0 },
    [BRW_OPCODE_DO] = { .name = "do", .nsrc = 0, .ndst = 0 },
    [BRW_OPCODE_ENDIF] = { .name = "endif", .nsrc = 2, .ndst = 0 },
};

char *conditional_modifier[16] = {
    [BRW_CONDITIONAL_NONE] = "",
    [BRW_CONDITIONAL_Z] = ".e",
    [BRW_CONDITIONAL_NZ] = ".ne",
    [BRW_CONDITIONAL_G] = ".g",
    [BRW_CONDITIONAL_GE] = ".ge",
    [BRW_CONDITIONAL_L] = ".l",
    [BRW_CONDITIONAL_LE] = ".le",
    [BRW_CONDITIONAL_R] = ".r",
    [BRW_CONDITIONAL_O] = ".o",
    [BRW_CONDITIONAL_U] = ".u",
};

char *negate[2] = {
    [0] = "",
    [1] = "-",
};

char *_abs[2] = {
    [0] = "",
    [1] = "(abs)",
};

char *vert_stride[16] = {
    [0] = "0",
    [1] = "1",
    [2] = "2",
    [3] = "4",
    [4] = "8",
    [5] = "16",
    [6] = "32",
    [15] = "VxH",
};

char *width[8] = {
    [0] = "1",
    [1] = "2",
    [2] = "4",
    [3] = "8",
    [4] = "16",
};

char *horiz_stride[4] = {
    [0] = "0",
    [1] = "1",
    [2] = "2",
    [3] = "4"
};

char *chan_sel[4] = {
    [0] = "x",
    [1] = "y",
    [2] = "z",
    [3] = "w",
};

char *dest_condmod[16] = {
   [0] = NULL
};

char *debug_ctrl[2] = {
    [0] = "",
    [1] = ".breakpoint"
};

char *saturate[2] = {
    [0] = "",
    [1] = ".sat"
};

char *exec_size[8] = {
    [0] = "1",
    [1] = "2",
    [2] = "4",
    [3] = "8",
    [4] = "16",
    [5] = "32"
};

char *pred_inv[2] = {
    [0] = "+",
    [1] = "-"
};

char *pred_ctrl_align16[16] = {
    [1] = "",
    [2] = ".x",
    [3] = ".y",
    [4] = ".z",
    [5] = ".w",
    [6] = ".any4h",
    [7] = ".all4h",
};

char *pred_ctrl_align1[16] = {
    [1] = "",
    [2] = ".anyv",
    [3] = ".allv",
    [4] = ".any2h",
    [5] = ".all2h",
    [6] = ".any4h",
    [7] = ".all4h",
    [8] = ".any8h",
    [9] = ".all8h",
    [10] = ".any16h",
    [11] = ".all16h",
};

char *thread_ctrl[4] = {
    [0] = "",
    [2] = "switch"
};

char *compr_ctrl[4] = {
    [0] = "",
    [1] = "sechalf",
    [2] = "compr",
};

char *dep_ctrl[4] = {
    [0] = "",
    [1] = "NoDDClr",
    [2] = "NoDDChk",
    [3] = "NoDDClr,NoDDChk",
};

char *mask_ctrl[4] = {
    [0] = "",
    [1] = "nomask",
};

char *access_mode[2] = {
    [0] = "align1",
    [1] = "align16",
};

char *reg_encoding[8] = {
    [0] = "UD",
    [1] = "D",
    [2] = "UW",
    [3] = "W",
    [4] = "UB",
    [5] = "B",
    [7] = "F"
};

char *imm_encoding[8] = {
    [0] = "UD",
    [1] = "D",
    [2] = "UW",
    [3] = "W",
    [5] = "VF",
    [5] = "V",
    [7] = "F"
};

char *reg_file[4] = {
    [0] = "A",
    [1] = "g",
    [2] = "m",
    [3] = "imm",
};

char *writemask[16] = {
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

char *end_of_thread[2] = {
    [0] = "",
    [1] = "EOT"
};

char *target_function[16] = {
    [BRW_MESSAGE_TARGET_NULL] = "null",
    [BRW_MESSAGE_TARGET_MATH] = "math",
    [BRW_MESSAGE_TARGET_SAMPLER] = "sampler",
    [BRW_MESSAGE_TARGET_GATEWAY] = "gateway",
    [BRW_MESSAGE_TARGET_DATAPORT_READ] = "read",
    [BRW_MESSAGE_TARGET_DATAPORT_WRITE] = "write",
    [BRW_MESSAGE_TARGET_URB] = "urb",
    [BRW_MESSAGE_TARGET_THREAD_SPAWNER] = "thread_spawner"
};

char *math_function[16] = {
    [BRW_MATH_FUNCTION_INV] = "inv",
    [BRW_MATH_FUNCTION_LOG] = "log",
    [BRW_MATH_FUNCTION_EXP] = "exp",
    [BRW_MATH_FUNCTION_SQRT] = "sqrt",
    [BRW_MATH_FUNCTION_RSQ] = "rsq",
    [BRW_MATH_FUNCTION_SIN] = "sin",
    [BRW_MATH_FUNCTION_COS] = "cos",
    [BRW_MATH_FUNCTION_SINCOS] = "sincos",
    [BRW_MATH_FUNCTION_TAN] = "tan",
    [BRW_MATH_FUNCTION_POW] = "pow",
    [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT_AND_REMAINDER] = "intdivmod",
    [BRW_MATH_FUNCTION_INT_DIV_QUOTIENT] = "intmod",
    [BRW_MATH_FUNCTION_INT_DIV_REMAINDER] = "intdiv",
};

char *math_saturate[2] = {
    [0] = "",
    [1] = "sat"
};

char *math_signed[2] = {
    [0] = "",
    [1] = "signed"
};

char *math_scalar[2] = {
    [0] = "",
    [1] = "scalar"
};

char *math_precision[2] = {
    [0] = "",
    [1] = "partial_precision"
};

char *urb_swizzle[4] = {
    [BRW_URB_SWIZZLE_NONE] = "",
    [BRW_URB_SWIZZLE_INTERLEAVE] = "interleave",
    [BRW_URB_SWIZZLE_TRANSPOSE] = "transpose",
};

char *urb_allocate[2] = {
    [0] = "",
    [1] = "allocate"
};

char *urb_used[2] = {
    [0] = "",
    [1] = "used"
};

char *urb_complete[2] = {
    [0] = "",
    [1] = "complete"
};

char *sampler_target_format[4] = {
    [0] = "F",
    [2] = "UD",
    [3] = "D"
};


static int column;

static int string (FILE *file, char *string)
{
    fputs (string, file);
    column += strlen (string);
    return 0;
}

static int format (FILE *f, char *format, ...)
{
    char    buf[1024];
    va_list	args;
    va_start (args, format);

    vsnprintf (buf, sizeof (buf) - 1, format, args);
    va_end (args);
    string (f, buf);
    return 0;
}

static int newline (FILE *f)
{
    putc ('\n', f);
    column = 0;
    return 0;
}

static int pad (FILE *f, int c)
{
    do
	string (f, " ");
    while (column < c);
    return 0;
}

static int control (FILE *file, char *name, char *ctrl[], GLuint id, int *space)
{
    if (!ctrl[id]) {
	fprintf (file, "*** invalid %s value %d ",
		 name, id);
	return 1;
    }
    if (ctrl[id][0])
    {
	if (space && *space)
	    string (file, " ");
	string (file, ctrl[id]);
	if (space)
	    *space = 1;
    }
    return 0;
}

static int print_opcode (FILE *file, int id)
{
    if (!opcode[id].name) {
	format (file, "*** invalid opcode value %d ", id);
	return 1;
    }
    string (file, opcode[id].name);
    return 0;
}

static int reg (FILE *file, GLuint _reg_file, GLuint _reg_nr)
{
    int	err = 0;
    if (_reg_file == BRW_ARCHITECTURE_REGISTER_FILE) {
	switch (_reg_nr & 0xf0) {
	case BRW_ARF_NULL:
	    string (file, "null");
	    return -1;
	case BRW_ARF_ADDRESS:
	    format (file, "a%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_ACCUMULATOR:
	    format (file, "acc%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_MASK:
	    format (file, "mask%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_MASK_STACK:
	    format (file, "msd%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_STATE:
	    format (file, "sr%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_CONTROL:
	    format (file, "cr%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_NOTIFICATION_COUNT:
	    format (file, "n%d", _reg_nr & 0x0f);
	    break;
	case BRW_ARF_IP:
	    string (file, "ip");
	    return -1;
	    break;
	default:
	    format (file, "ARF%d", _reg_nr);
	    break;
	}
    } else {
	err  |= control (file, "src reg file", reg_file, _reg_file, NULL);
	format (file, "%d", _reg_nr);
    }
    return err;
}

static int dest (FILE *file, const struct brw_instruction *inst)
{
    int	err = 0;

    if (inst->header.access_mode == BRW_ALIGN_1)
    {
	if (inst->bits1.da1.dest_address_mode == BRW_ADDRESS_DIRECT)
	{
	    err |= reg (file, inst->bits1.da1.dest_reg_file, inst->bits1.da1.dest_reg_nr);
	    if (err == -1)
		return 0;
	    if (inst->bits1.da1.dest_subreg_nr)
		format (file, ".%d", inst->bits1.da1.dest_subreg_nr);
	    format (file, "<%d>", inst->bits1.da1.dest_horiz_stride);
	    err |= control (file, "dest reg encoding", reg_encoding, inst->bits1.da1.dest_reg_type, NULL);
	}
	else
	{
	    string (file, "g[a0");
	    if (inst->bits1.ia1.dest_subreg_nr)
		format (file, ".%d", inst->bits1.ia1.dest_subreg_nr);
	    if (inst->bits1.ia1.dest_indirect_offset)
		format (file, " %d", inst->bits1.ia1.dest_indirect_offset);
	    string (file, "]");
	    format (file, "<%d>", inst->bits1.ia1.dest_horiz_stride);
	    err |= control (file, "dest reg encoding", reg_encoding, inst->bits1.ia1.dest_reg_type, NULL);
	}
    }
    else
    {
	if (inst->bits1.da16.dest_address_mode == BRW_ADDRESS_DIRECT)
	{
	    err |= reg (file, inst->bits1.da16.dest_reg_file, inst->bits1.da16.dest_reg_nr);
	    if (err == -1)
		return 0;
	    if (inst->bits1.da16.dest_subreg_nr)
		format (file, ".%d", inst->bits1.da16.dest_subreg_nr);
	    string (file, "<1>");
	    err |= control (file, "writemask", writemask, inst->bits1.da16.dest_writemask, NULL);
	    err |= control (file, "dest reg encoding", reg_encoding, inst->bits1.da16.dest_reg_type, NULL);
	}
	else
	{
	    err = 1;
	    string (file, "Indirect align16 address mode not supported");
	}
    }

    return 0;
}

static int src_align1_region (FILE *file,
			      GLuint _vert_stride, GLuint _width, GLuint _horiz_stride)
{
    int err = 0;
    string (file, "<");
    err |= control (file, "vert stride", vert_stride, _vert_stride, NULL);
    string (file, ",");
    err |= control (file, "width", width, _width, NULL);
    string (file, ",");
    err |= control (file, "horiz_stride", horiz_stride, _horiz_stride, NULL);
    string (file, ">");
    return err;
}

static int src_da1 (FILE *file, GLuint type, GLuint _reg_file,
		    GLuint _vert_stride, GLuint _width, GLuint _horiz_stride,
		    GLuint reg_num, GLuint sub_reg_num, GLuint __abs, GLuint _negate)
{
    int err = 0;
    err |= control (file, "negate", negate, _negate, NULL);
    err |= control (file, "abs", _abs, __abs, NULL);

    err |= reg (file, _reg_file, reg_num);
    if (err == -1)
	return 0;
    if (sub_reg_num)
	format (file, ".%d", sub_reg_num);
    src_align1_region (file, _vert_stride, _width, _horiz_stride);
    err |= control (file, "src reg encoding", reg_encoding, type, NULL);
    return err;
}

static int src_ia1 (FILE *file,
		    GLuint type,
		    GLuint _reg_file,
		    GLint _addr_imm,
		    GLuint _addr_subreg_nr,
		    GLuint _negate,
		    GLuint __abs,
		    GLuint _addr_mode,
		    GLuint _horiz_stride,
		    GLuint _width,
		    GLuint _vert_stride)
{
    int err = 0;
    err |= control (file, "negate", negate, _negate, NULL);
    err |= control (file, "abs", _abs, __abs, NULL);

    string (file, "g[a0");
    if (_addr_subreg_nr)
	format (file, ".%d", _addr_subreg_nr);
    if (_addr_imm)
	format (file, " %d", _addr_imm);
    string (file, "]");
    src_align1_region (file, _vert_stride, _width, _horiz_stride);
    err |= control (file, "src reg encoding", reg_encoding, type, NULL);
    return err;
}

static int src_da16 (FILE *file,
		     GLuint _reg_type,
		     GLuint _reg_file,
		     GLuint _vert_stride,
		     GLuint _reg_nr,
		     GLuint _subreg_nr,
		     GLuint __abs,
		     GLuint _negate,
		     GLuint swz_x,
		     GLuint swz_y,
		     GLuint swz_z,
		     GLuint swz_w)
{
    int err = 0;
    err |= control (file, "negate", negate, _negate, NULL);
    err |= control (file, "abs", _abs, __abs, NULL);

    err |= reg (file, _reg_file, _reg_nr);
    if (err == -1)
	return 0;
    if (_subreg_nr)
	format (file, ".%d", _subreg_nr);
    string (file, "<");
    err |= control (file, "vert stride", vert_stride, _vert_stride, NULL);
    string (file, ",1,1>");
    err |= control (file, "src da16 reg type", reg_encoding, _reg_type, NULL);
    /*
     * Three kinds of swizzle display:
     *  identity - nothing printed
     *  1->all	 - print the single channel
     *  1->1     - print the mapping
     */
    if (swz_x == BRW_CHANNEL_X &&
	swz_y == BRW_CHANNEL_Y &&
	swz_z == BRW_CHANNEL_Z &&
	swz_w == BRW_CHANNEL_W)
    {
	;
    }
    else if (swz_x == swz_y && swz_x == swz_z && swz_x == swz_w)
    {
	string (file, ".");
	err |= control (file, "channel select", chan_sel, swz_x, NULL);
    }
    else
    {
	string (file, ".");
	err |= control (file, "channel select", chan_sel, swz_x, NULL);
	err |= control (file, "channel select", chan_sel, swz_y, NULL);
	err |= control (file, "channel select", chan_sel, swz_z, NULL);
	err |= control (file, "channel select", chan_sel, swz_w, NULL);
    }
    return err;
}


static int imm (FILE *file, GLuint type, const struct brw_instruction *inst) {
    switch (type) {
    case BRW_REGISTER_TYPE_UD:
	format (file, "0x%08xUD", inst->bits3.ud);
	break;
    case BRW_REGISTER_TYPE_D:
	format (file, "%dD", inst->bits3.d);
	break;
    case BRW_REGISTER_TYPE_UW:
	format (file, "0x%04xUW", (uint16_t) inst->bits3.ud);
	break;
    case BRW_REGISTER_TYPE_W:
	format (file, "%dW", (int16_t) inst->bits3.d);
	break;
    case BRW_REGISTER_TYPE_UB:
	format (file, "0x%02xUB", (int8_t) inst->bits3.ud);
	break;
    case BRW_REGISTER_TYPE_VF:
	format (file, "Vector Float");
	break;
    case BRW_REGISTER_TYPE_V:
	format (file, "0x%08xV", inst->bits3.ud);
	break;
    case BRW_REGISTER_TYPE_F:
	format (file, "%-gF", inst->bits3.f);
    }
    return 0;
}

static int src0 (FILE *file, const struct brw_instruction *inst)
{
    if (inst->bits1.da1.src0_reg_file == BRW_IMMEDIATE_VALUE)
	return imm (file, inst->bits1.da1.src0_reg_type,
		    inst);
    else if (inst->header.access_mode == BRW_ALIGN_1)
    {
	if (inst->bits2.da1.src0_address_mode == BRW_ADDRESS_DIRECT)
	{
	    return src_da1 (file,
			    inst->bits1.da1.src0_reg_type,
			    inst->bits1.da1.src0_reg_file,
			    inst->bits2.da1.src0_vert_stride,
			    inst->bits2.da1.src0_width,
			    inst->bits2.da1.src0_horiz_stride,
			    inst->bits2.da1.src0_reg_nr,
			    inst->bits2.da1.src0_subreg_nr,
			    inst->bits2.da1.src0_abs,
			    inst->bits2.da1.src0_negate);
	}
	else
	{
	    return src_ia1 (file,
			    inst->bits1.ia1.src0_reg_type,
			    inst->bits1.ia1.src0_reg_file,
			    inst->bits2.ia1.src0_indirect_offset,
			    inst->bits2.ia1.src0_subreg_nr,
			    inst->bits2.ia1.src0_negate,
			    inst->bits2.ia1.src0_abs,
			    inst->bits2.ia1.src0_address_mode,
			    inst->bits2.ia1.src0_horiz_stride,
			    inst->bits2.ia1.src0_width,
			    inst->bits2.ia1.src0_vert_stride);
	}
    }
    else
    {
	if (inst->bits2.da16.src0_address_mode == BRW_ADDRESS_DIRECT)
	{
	    return src_da16 (file,
			     inst->bits1.da16.src0_reg_type,
			     inst->bits1.da16.src0_reg_file,
			     inst->bits2.da16.src0_vert_stride,
			     inst->bits2.da16.src0_reg_nr,
			     inst->bits2.da16.src0_subreg_nr,
			     inst->bits2.da16.src0_abs,
			     inst->bits2.da16.src0_negate,
			     inst->bits2.da16.src0_swz_x,
			     inst->bits2.da16.src0_swz_y,
			     inst->bits2.da16.src0_swz_z,
			     inst->bits2.da16.src0_swz_w);
	}
	else
	{
	    string (file, "Indirect align16 address mode not supported");
	    return 1;
	}
    }
}

static int src1 (FILE *file, const struct brw_instruction *inst)
{
    if (inst->bits1.da1.src1_reg_file == BRW_IMMEDIATE_VALUE)
	return imm (file, inst->bits1.da1.src1_reg_type,
		    inst);
    else if (inst->header.access_mode == BRW_ALIGN_1)
    {
	if (inst->bits3.da1.src1_address_mode == BRW_ADDRESS_DIRECT)
	{
	    return src_da1 (file,
			    inst->bits1.da1.src1_reg_type,
			    inst->bits1.da1.src1_reg_file,
			    inst->bits3.da1.src1_vert_stride,
			    inst->bits3.da1.src1_width,
			    inst->bits3.da1.src1_horiz_stride,
			    inst->bits3.da1.src1_reg_nr,
			    inst->bits3.da1.src1_subreg_nr,
			    inst->bits3.da1.src1_abs,
			    inst->bits3.da1.src1_negate);
	}
	else
	{
	    return src_ia1 (file,
			    inst->bits1.ia1.src1_reg_type,
			    inst->bits1.ia1.src1_reg_file,
			    inst->bits3.ia1.src1_indirect_offset,
			    inst->bits3.ia1.src1_subreg_nr,
			    inst->bits3.ia1.src1_negate,
			    inst->bits3.ia1.src1_abs,
			    inst->bits3.ia1.src1_address_mode,
			    inst->bits3.ia1.src1_horiz_stride,
			    inst->bits3.ia1.src1_width,
			    inst->bits3.ia1.src1_vert_stride);
	}
    }
    else
    {
	if (inst->bits3.da16.src1_address_mode == BRW_ADDRESS_DIRECT)
	{
	    return src_da16 (file,
			     inst->bits1.da16.src1_reg_type,
			     inst->bits1.da16.src1_reg_file,
			     inst->bits3.da16.src1_vert_stride,
			     inst->bits3.da16.src1_reg_nr,
			     inst->bits3.da16.src1_subreg_nr,
			     inst->bits3.da16.src1_abs,
			     inst->bits3.da16.src1_negate,
			     inst->bits3.da16.src1_swz_x,
			     inst->bits3.da16.src1_swz_y,
			     inst->bits3.da16.src1_swz_z,
			     inst->bits3.da16.src1_swz_w);
	}
	else
	{
	    string (file, "Indirect align16 address mode not supported");
	    return 1;
	}
    }
}

int brw_disasm_insn (FILE *file, const struct brw_instruction *inst)
{
    int	err = 0;
    int space = 0;

    if (inst->header.predicate_control) {
	string (file, "(");
	err |= control (file, "predicate inverse", pred_inv, inst->header.predicate_inverse, NULL);
	string (file, "f0");
	if (inst->bits2.da1.flag_reg_nr)
	    format (file, ".%d", inst->bits2.da1.flag_reg_nr);
	if (inst->header.access_mode == BRW_ALIGN_1)
	    err |= control (file, "predicate control align1", pred_ctrl_align1,
			    inst->header.predicate_control, NULL);
	else
	    err |= control (file, "predicate control align16", pred_ctrl_align16,
			    inst->header.predicate_control, NULL);
	string (file, ") ");
    }

    err |= print_opcode (file, inst->header.opcode);
    err |= control (file, "saturate", saturate, inst->header.saturate, NULL);
    err |= control (file, "debug control", debug_ctrl, inst->header.debug_control, NULL);

    if (inst->header.opcode != BRW_OPCODE_SEND)
	err |= control (file, "conditional modifier", conditional_modifier,
			inst->header.destreg__conditionalmod, NULL);

    if (inst->header.opcode != BRW_OPCODE_NOP) {
	string (file, "(");
	err |= control (file, "execution size", exec_size, inst->header.execution_size, NULL);
	string (file, ")");
    }

    if (inst->header.opcode == BRW_OPCODE_SEND)
	format (file, " %d", inst->header.destreg__conditionalmod);

    if (opcode[inst->header.opcode].ndst > 0) {
	pad (file, 16);
	err |= dest (file, inst);
    }
    if (opcode[inst->header.opcode].nsrc > 0) {
	pad (file, 32);
	err |= src0 (file, inst);
    }
    if (opcode[inst->header.opcode].nsrc > 1) {
	pad (file, 48);
	err |= src1 (file, inst);
    }

    if (inst->header.opcode == BRW_OPCODE_SEND) {
	newline (file);
	pad (file, 16);
	space = 0;
	err |= control (file, "target function", target_function,
			inst->bits3.generic.msg_target, &space);
	switch (inst->bits3.generic.msg_target) {
	case BRW_MESSAGE_TARGET_MATH:
	    err |= control (file, "math function", math_function,
			    inst->bits3.math.function, &space);
	    err |= control (file, "math saturate", math_saturate,
			    inst->bits3.math.saturate, &space);
	    err |= control (file, "math signed", math_signed,
			    inst->bits3.math.int_type, &space);
	    err |= control (file, "math scalar", math_scalar,
			    inst->bits3.math.data_type, &space);
	    err |= control (file, "math precision", math_precision,
			    inst->bits3.math.precision, &space);
	    break;
	case BRW_MESSAGE_TARGET_SAMPLER:
	    format (file, " (%d, %d, ",
		    inst->bits3.sampler.binding_table_index,
		    inst->bits3.sampler.sampler);
	    err |= control (file, "sampler target format", sampler_target_format,
			    inst->bits3.sampler.return_format, NULL);
	    string (file, ")");
	    break;
	case BRW_MESSAGE_TARGET_DATAPORT_WRITE:
	    format (file, " (%d, %d, %d, %d)",
		    inst->bits3.dp_write.binding_table_index,
		    (inst->bits3.dp_write.pixel_scoreboard_clear << 3) |
		    inst->bits3.dp_write.msg_control,
		    inst->bits3.dp_write.msg_type,
		    inst->bits3.dp_write.send_commit_msg);
	    break;
	case BRW_MESSAGE_TARGET_URB:
	    format (file, " %d", inst->bits3.urb.offset);
	    space = 1;
	    err |= control (file, "urb swizzle", urb_swizzle,
			    inst->bits3.urb.swizzle_control, &space);
	    err |= control (file, "urb allocate", urb_allocate,
			    inst->bits3.urb.allocate, &space);
	    err |= control (file, "urb used", urb_used,
			    inst->bits3.urb.used, &space);
	    err |= control (file, "urb complete", urb_complete,
			    inst->bits3.urb.complete, &space);
	    break;
	case BRW_MESSAGE_TARGET_THREAD_SPAWNER:
	    break;
	default:
	    format (file, "unsupported target %d", inst->bits3.generic.msg_target);
	    break;
	}
	if (space)
	    string (file, " ");
	format (file, "mlen %d",
		inst->bits3.generic.msg_length);
	format (file, " rlen %d",
		inst->bits3.generic.response_length);
    }
    pad (file, 64);
    if (inst->header.opcode != BRW_OPCODE_NOP) {
	string (file, "{");
	space = 1;
	err |= control(file, "access mode", access_mode, inst->header.access_mode, &space);
	err |= control (file, "mask control", mask_ctrl, inst->header.mask_control, &space);
	err |= control (file, "dependency control", dep_ctrl, inst->header.dependency_control, &space);
	err |= control (file, "compression control", compr_ctrl, inst->header.compression_control, &space);
	err |= control (file, "thread control", thread_ctrl, inst->header.thread_control, &space);
	if (inst->header.opcode == BRW_OPCODE_SEND)
	    err |= control (file, "end of thread", end_of_thread,
			    inst->bits3.generic.end_of_thread, &space);
	if (space)
	    string (file, " ");
	string (file, "}");
    }
    string (file, ";");
    newline (file);
    return err;
}


int brw_disasm (FILE *file, 
                const struct brw_instruction *inst,
                unsigned count)
{
   int i, err;

   for (i = 0; i < count; i++) {
      err = brw_disasm_insn(stderr, &inst[i]);
      if (err)
         return err;
   }

   fprintf(file, "\n");
   return 0;
}

