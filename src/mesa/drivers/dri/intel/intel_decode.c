#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "intel_decode.h"
#include "intel_chipset.h"

static FILE *out;
static uint32_t saved_s2 = 0, saved_s4 = 0;
static char saved_s2_set = 0, saved_s4_set = 0;
static uint32_t head_offset = 0xffffffff; /* undefined */
static uint32_t tail_offset = 0xffffffff; /* undefined */

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(A) (sizeof(A)/sizeof(A[0]))
#endif

#define BUFFER_FAIL(_count, _len, _name) do {			\
    fprintf(out, "Buffer size too small in %s (%d < %d)\n",	\
	    (_name), (_count), (_len));				\
    (*failures)++;						\
    return count;						\
} while (0)


static float
int_as_float(uint32_t intval)
{
    union intfloat {
	uint32_t i;
	float f;
    } uval;

    uval.i = intval;
    return uval.f;
}

static void
instr_out(uint32_t *data, uint32_t hw_offset, unsigned int index,
	  char *fmt, ...)
{
    va_list va;
    char *parseinfo;
    uint32_t offset = hw_offset + index * 4;

    if (offset == head_offset)
	parseinfo = "HEAD";
    else if (offset == tail_offset)
	parseinfo = "TAIL";
    else
	parseinfo = "    ";

    fprintf(out, "0x%08x: %s 0x%08x: %s", offset, parseinfo,
	    data[index],
	    index == 0 ? "" : "   ");
    va_start(va, fmt);
    vfprintf(out, fmt, va);
    va_end(va);
}

static int
decode_mi(uint32_t *data, int count, uint32_t hw_offset, int *failures)
{
    unsigned int opcode;

    struct {
	uint32_t opcode;
	int len_mask;
	int min_len;
	int max_len;
	char *name;
    } opcodes_mi[] = {
	{ 0x08, 0, 1, 1, "MI_ARB_ON_OFF" },
	{ 0x0a, 0, 1, 1, "MI_BATCH_BUFFER_END" },
	{ 0x30, 0x3f, 3, 3, "MI_BATCH_BUFFER" },
	{ 0x31, 0x3f, 2, 2, "MI_BATCH_BUFFER_START" },
	{ 0x14, 0x3f, 3, 3, "MI_DISPLAY_BUFFER_INFO" },
	{ 0x04, 0, 1, 1, "MI_FLUSH" },
	{ 0x22, 0x1f, 3, 3, "MI_LOAD_REGISTER_IMM" },
	{ 0x13, 0x3f, 2, 2, "MI_LOAD_SCAN_LINES_EXCL" },
	{ 0x12, 0x3f, 2, 2, "MI_LOAD_SCAN_LINES_INCL" },
	{ 0x00, 0, 1, 1, "MI_NOOP" },
	{ 0x11, 0x3f, 2, 2, "MI_OVERLAY_FLIP" },
	{ 0x07, 0, 1, 1, "MI_REPORT_HEAD" },
	{ 0x18, 0x3f, 2, 2, "MI_SET_CONTEXT" },
	{ 0x20, 0x3f, 3, 4, "MI_STORE_DATA_IMM" },
	{ 0x21, 0x3f, 3, 4, "MI_STORE_DATA_INDEX" },
	{ 0x24, 0x3f, 3, 3, "MI_STORE_REGISTER_MEM" },
	{ 0x02, 0, 1, 1, "MI_USER_INTERRUPT" },
	{ 0x03, 0, 1, 1, "MI_WAIT_FOR_EVENT" },
	{ 0x16, 0x7f, 3, 3, "MI_SEMAPHORE_MBOX" },
	{ 0x26, 0x1f, 3, 4, "MI_FLUSH_DW" },
	{ 0x0b, 0, 1, 1, "MI_SUSPEND_FLUSH" },
    };

    switch ((data[0] & 0x1f800000) >> 23) {
    case 0x0a:
	instr_out(data, hw_offset, 0, "MI_BATCH_BUFFER_END\n");
	return -1;
    }

    for (opcode = 0; opcode < sizeof(opcodes_mi) / sizeof(opcodes_mi[0]);
	 opcode++) {
	if ((data[0] & 0x1f800000) >> 23 == opcodes_mi[opcode].opcode) {
	    unsigned int len = 1, i;

	    instr_out(data, hw_offset, 0, "%s\n", opcodes_mi[opcode].name);
	    if (opcodes_mi[opcode].max_len > 1) {
		len = (data[0] & opcodes_mi[opcode].len_mask) + 2;
		if (len < opcodes_mi[opcode].min_len ||
		    len > opcodes_mi[opcode].max_len)
		{
		    fprintf(out, "Bad length (%d) in %s, [%d, %d]\n",
			    len, opcodes_mi[opcode].name,
			    opcodes_mi[opcode].min_len,
			    opcodes_mi[opcode].max_len);
		}
	    }

	    for (i = 1; i < len; i++) {
		if (i >= count)
		    BUFFER_FAIL(count, len, opcodes_mi[opcode].name);
		instr_out(data, hw_offset, i, "dword %d\n", i);
	    }

	    return len;
	}
    }

    instr_out(data, hw_offset, 0, "MI UNKNOWN\n");
    (*failures)++;
    return 1;
}

static int
decode_2d(uint32_t *data, int count, uint32_t hw_offset, int *failures)
{
    unsigned int opcode, len;
    char *format = NULL;

    struct {
	uint32_t opcode;
	int min_len;
	int max_len;
	char *name;
    } opcodes_2d[] = {
	{ 0x40, 5, 5, "COLOR_BLT" },
	{ 0x43, 6, 6, "SRC_COPY_BLT" },
	{ 0x01, 8, 8, "XY_SETUP_BLT" },
	{ 0x11, 9, 9, "XY_SETUP_MONO_PATTERN_SL_BLT" },
	{ 0x03, 3, 3, "XY_SETUP_CLIP_BLT" },
	{ 0x24, 2, 2, "XY_PIXEL_BLT" },
	{ 0x25, 3, 3, "XY_SCANLINES_BLT" },
	{ 0x26, 4, 4, "Y_TEXT_BLT" },
	{ 0x31, 5, 134, "XY_TEXT_IMMEDIATE_BLT" },
	{ 0x50, 6, 6, "XY_COLOR_BLT" },
	{ 0x51, 6, 6, "XY_PAT_BLT" },
	{ 0x76, 8, 8, "XY_PAT_CHROMA_BLT" },
	{ 0x72, 7, 135, "XY_PAT_BLT_IMMEDIATE" },
	{ 0x77, 9, 137, "XY_PAT_CHROMA_BLT_IMMEDIATE" },
	{ 0x52, 9, 9, "XY_MONO_PAT_BLT" },
	{ 0x59, 7, 7, "XY_MONO_PAT_FIXED_BLT" },
	{ 0x53, 8, 8, "XY_SRC_COPY_BLT" },
	{ 0x54, 8, 8, "XY_MONO_SRC_COPY_BLT" },
	{ 0x71, 9, 137, "XY_MONO_SRC_COPY_IMMEDIATE_BLT" },
	{ 0x55, 9, 9, "XY_FULL_BLT" },
	{ 0x55, 9, 137, "XY_FULL_IMMEDIATE_PATTERN_BLT" },
	{ 0x56, 9, 9, "XY_FULL_MONO_SRC_BLT" },
	{ 0x75, 10, 138, "XY_FULL_MONO_SRC_IMMEDIATE_PATTERN_BLT" },
	{ 0x57, 12, 12, "XY_FULL_MONO_PATTERN_BLT" },
	{ 0x58, 12, 12, "XY_FULL_MONO_PATTERN_MONO_SRC_BLT" },
    };

    switch ((data[0] & 0x1fc00000) >> 22) {
    case 0x50:
	instr_out(data, hw_offset, 0,
		  "XY_COLOR_BLT (rgb %sabled, alpha %sabled, dst tile %d)\n",
		  (data[0] & (1 << 20)) ? "en" : "dis",
		  (data[0] & (1 << 21)) ? "en" : "dis",
		  (data[0] >> 11) & 1);

	len = (data[0] & 0x000000ff) + 2;
	if (len != 6)
	    fprintf(out, "Bad count in XY_COLOR_BLT\n");
	if (count < 6)
	    BUFFER_FAIL(count, len, "XY_COLOR_BLT");

	switch ((data[1] >> 24) & 0x3) {
	case 0:
	    format="8";
	    break;
	case 1:
	    format="565";
	    break;
	case 2:
	    format="1555";
	    break;
	case 3:
	    format="8888";
	    break;
	}

	instr_out(data, hw_offset, 1, "format %s, pitch %d, "
		  "clipping %sabled\n", format,
		  (short)(data[1] & 0xffff),
		  data[1] & (1 << 30) ? "en" : "dis");
	instr_out(data, hw_offset, 2, "(%d,%d)\n",
		  data[2] & 0xffff, data[2] >> 16);
	instr_out(data, hw_offset, 3, "(%d,%d)\n",
		  data[3] & 0xffff, data[3] >> 16);
	instr_out(data, hw_offset, 4, "offset 0x%08x\n", data[4]);
	instr_out(data, hw_offset, 5, "color\n");
	return len;
    case 0x53:
	instr_out(data, hw_offset, 0,
		  "XY_SRC_COPY_BLT (rgb %sabled, alpha %sabled, "
		  "src tile %d, dst tile %d)\n",
		  (data[0] & (1 << 20)) ? "en" : "dis",
		  (data[0] & (1 << 21)) ? "en" : "dis",
		  (data[0] >> 15) & 1,
		  (data[0] >> 11) & 1);

	len = (data[0] & 0x000000ff) + 2;
	if (len != 8)
	    fprintf(out, "Bad count in XY_SRC_COPY_BLT\n");
	if (count < 8)
	    BUFFER_FAIL(count, len, "XY_SRC_COPY_BLT");

	switch ((data[1] >> 24) & 0x3) {
	case 0:
	    format="8";
	    break;
	case 1:
	    format="565";
	    break;
	case 2:
	    format="1555";
	    break;
	case 3:
	    format="8888";
	    break;
	}

	instr_out(data, hw_offset, 1, "format %s, dst pitch %d, "
		  "clipping %sabled\n", format,
		  (short)(data[1] & 0xffff),
		  data[1] & (1 << 30) ? "en" : "dis");
	instr_out(data, hw_offset, 2, "dst (%d,%d)\n",
		  data[2] & 0xffff, data[2] >> 16);
	instr_out(data, hw_offset, 3, "dst (%d,%d)\n",
		  data[3] & 0xffff, data[3] >> 16);
	instr_out(data, hw_offset, 4, "dst offset 0x%08x\n", data[4]);
	instr_out(data, hw_offset, 5, "src (%d,%d)\n",
		  data[5] & 0xffff, data[5] >> 16);
	instr_out(data, hw_offset, 6, "src pitch %d\n",
		  (short)(data[6] & 0xffff));
	instr_out(data, hw_offset, 7, "src offset 0x%08x\n", data[7]);
	return len;
    }

    for (opcode = 0; opcode < sizeof(opcodes_2d) / sizeof(opcodes_2d[0]);
	 opcode++) {
	if ((data[0] & 0x1fc00000) >> 22 == opcodes_2d[opcode].opcode) {
	    unsigned int i;

	    len = 1;
	    instr_out(data, hw_offset, 0, "%s\n", opcodes_2d[opcode].name);
	    if (opcodes_2d[opcode].max_len > 1) {
		len = (data[0] & 0x000000ff) + 2;
		if (len < opcodes_2d[opcode].min_len ||
		    len > opcodes_2d[opcode].max_len)
		{
		    fprintf(out, "Bad count in %s\n", opcodes_2d[opcode].name);
		}
	    }

	    for (i = 1; i < len; i++) {
		if (i >= count)
		    BUFFER_FAIL(count, len, opcodes_2d[opcode].name);
		instr_out(data, hw_offset, i, "dword %d\n", i);
	    }

	    return len;
	}
    }

    instr_out(data, hw_offset, 0, "2D UNKNOWN\n");
    (*failures)++;
    return 1;
}

static int
decode_3d_1c(uint32_t *data, int count, uint32_t hw_offset, int *failures)
{
    uint32_t opcode;

    opcode = (data[0] & 0x00f80000) >> 19;

    switch (opcode) {
    case 0x11:
	instr_out(data, hw_offset, 0, "3DSTATE_DEPTH_SUBRECTANGLE_DISABLE\n");
	return 1;
    case 0x10:
	instr_out(data, hw_offset, 0, "3DSTATE_SCISSOR_ENABLE %s\n",
		data[0]&1?"enabled":"disabled");
	return 1;
    case 0x01:
	instr_out(data, hw_offset, 0, "3DSTATE_MAP_COORD_SET_I830\n");
	return 1;
    case 0x0a:
	instr_out(data, hw_offset, 0, "3DSTATE_MAP_CUBE_I830\n");
	return 1;
    case 0x05:
	instr_out(data, hw_offset, 0, "3DSTATE_MAP_TEX_STREAM_I830\n");
	return 1;
    }

    instr_out(data, hw_offset, 0, "3D UNKNOWN: 3d_1c opcode = 0x%x\n",
	      opcode);
    (*failures)++;
    return 1;
}

/** Sets the string dstname to describe the destination of the PS instruction */
static void
i915_get_instruction_dst(uint32_t *data, int i, char *dstname, int do_mask)
{
    uint32_t a0 = data[i];
    int dst_nr = (a0 >> 14) & 0xf;
    char dstmask[8];
    char *sat;

    if (do_mask) {
	if (((a0 >> 10) & 0xf) == 0xf) {
	    dstmask[0] = 0;
	} else {
	    int dstmask_index = 0;

	    dstmask[dstmask_index++] = '.';
	    if (a0 & (1 << 10))
		dstmask[dstmask_index++] = 'x';
	    if (a0 & (1 << 11))
		dstmask[dstmask_index++] = 'y';
	    if (a0 & (1 << 12))
		dstmask[dstmask_index++] = 'z';
	    if (a0 & (1 << 13))
		dstmask[dstmask_index++] = 'w';
	    dstmask[dstmask_index++] = 0;
	}

	if (a0 & (1 << 22))
	    sat = ".sat";
	else
	    sat = "";
    } else {
	dstmask[0] = 0;
	sat = "";
    }

    switch ((a0 >> 19) & 0x7) {
    case 0:
	if (dst_nr > 15)
	    fprintf(out, "bad destination reg R%d\n", dst_nr);
	sprintf(dstname, "R%d%s%s", dst_nr, dstmask, sat);
	break;
    case 4:
	if (dst_nr > 0)
	    fprintf(out, "bad destination reg oC%d\n", dst_nr);
	sprintf(dstname, "oC%s%s", dstmask, sat);
	break;
    case 5:
	if (dst_nr > 0)
	    fprintf(out, "bad destination reg oD%d\n", dst_nr);
	sprintf(dstname, "oD%s%s",  dstmask, sat);
	break;
    case 6:
	if (dst_nr > 3)
	    fprintf(out, "bad destination reg U%d\n", dst_nr);
	sprintf(dstname, "U%d%s%s", dst_nr, dstmask, sat);
	break;
    default:
	sprintf(dstname, "RESERVED");
	break;
    }
}

static char *
i915_get_channel_swizzle(uint32_t select)
{
    switch (select & 0x7) {
    case 0:
	return (select & 8) ? "-x" : "x";
    case 1:
	return (select & 8) ? "-y" : "y";
    case 2:
	return (select & 8) ? "-z" : "z";
    case 3:
	return (select & 8) ? "-w" : "w";
    case 4:
	return (select & 8) ? "-0" : "0";
    case 5:
	return (select & 8) ? "-1" : "1";
    default:
	return (select & 8) ? "-bad" : "bad";
    }
}

static void
i915_get_instruction_src_name(uint32_t src_type, uint32_t src_nr, char *name)
{
    switch (src_type) {
    case 0:
	sprintf(name, "R%d", src_nr);
	if (src_nr > 15)
	    fprintf(out, "bad src reg %s\n", name);
	break;
    case 1:
	if (src_nr < 8)
	    sprintf(name, "T%d", src_nr);
	else if (src_nr == 8)
	    sprintf(name, "DIFFUSE");
	else if (src_nr == 9)
	    sprintf(name, "SPECULAR");
	else if (src_nr == 10)
	    sprintf(name, "FOG");
	else {
	    fprintf(out, "bad src reg T%d\n", src_nr);
	    sprintf(name, "RESERVED");
	}
	break;
    case 2:
	sprintf(name, "C%d", src_nr);
	if (src_nr > 31)
	    fprintf(out, "bad src reg %s\n", name);
	break;
    case 4:
	sprintf(name, "oC");
	if (src_nr > 0)
	    fprintf(out, "bad src reg oC%d\n", src_nr);
	break;
    case 5:
	sprintf(name, "oD");
	if (src_nr > 0)
	    fprintf(out, "bad src reg oD%d\n", src_nr);
	break;
    case 6:
	sprintf(name, "U%d", src_nr);
	if (src_nr > 3)
	    fprintf(out, "bad src reg %s\n", name);
	break;
    default:
	fprintf(out, "bad src reg type %d\n", src_type);
	sprintf(name, "RESERVED");
	break;
    }
}

static void
i915_get_instruction_src0(uint32_t *data, int i, char *srcname)
{
    uint32_t a0 = data[i];
    uint32_t a1 = data[i + 1];
    int src_nr = (a0 >> 2) & 0x1f;
    char *swizzle_x = i915_get_channel_swizzle((a1 >> 28) & 0xf);
    char *swizzle_y = i915_get_channel_swizzle((a1 >> 24) & 0xf);
    char *swizzle_z = i915_get_channel_swizzle((a1 >> 20) & 0xf);
    char *swizzle_w = i915_get_channel_swizzle((a1 >> 16) & 0xf);
    char swizzle[100];

    i915_get_instruction_src_name((a0 >> 7) & 0x7, src_nr, srcname);
    sprintf(swizzle, ".%s%s%s%s", swizzle_x, swizzle_y, swizzle_z, swizzle_w);
    if (strcmp(swizzle, ".xyzw") != 0)
	strcat(srcname, swizzle);
}

static void
i915_get_instruction_src1(uint32_t *data, int i, char *srcname)
{
    uint32_t a1 = data[i + 1];
    uint32_t a2 = data[i + 2];
    int src_nr = (a1 >> 8) & 0x1f;
    char *swizzle_x = i915_get_channel_swizzle((a1 >> 4) & 0xf);
    char *swizzle_y = i915_get_channel_swizzle((a1 >> 0) & 0xf);
    char *swizzle_z = i915_get_channel_swizzle((a2 >> 28) & 0xf);
    char *swizzle_w = i915_get_channel_swizzle((a2 >> 24) & 0xf);
    char swizzle[100];

    i915_get_instruction_src_name((a1 >> 13) & 0x7, src_nr, srcname);
    sprintf(swizzle, ".%s%s%s%s", swizzle_x, swizzle_y, swizzle_z, swizzle_w);
    if (strcmp(swizzle, ".xyzw") != 0)
	strcat(srcname, swizzle);
}

static void
i915_get_instruction_src2(uint32_t *data, int i, char *srcname)
{
    uint32_t a2 = data[i + 2];
    int src_nr = (a2 >> 16) & 0x1f;
    char *swizzle_x = i915_get_channel_swizzle((a2 >> 12) & 0xf);
    char *swizzle_y = i915_get_channel_swizzle((a2 >> 8) & 0xf);
    char *swizzle_z = i915_get_channel_swizzle((a2 >> 4) & 0xf);
    char *swizzle_w = i915_get_channel_swizzle((a2 >> 0) & 0xf);
    char swizzle[100];

    i915_get_instruction_src_name((a2 >> 21) & 0x7, src_nr, srcname);
    sprintf(swizzle, ".%s%s%s%s", swizzle_x, swizzle_y, swizzle_z, swizzle_w);
    if (strcmp(swizzle, ".xyzw") != 0)
	strcat(srcname, swizzle);
}

static void
i915_get_instruction_addr(uint32_t src_type, uint32_t src_nr, char *name)
{
    switch (src_type) {
    case 0:
	sprintf(name, "R%d", src_nr);
	if (src_nr > 15)
	    fprintf(out, "bad src reg %s\n", name);
	break;
    case 1:
	if (src_nr < 8)
	    sprintf(name, "T%d", src_nr);
	else if (src_nr == 8)
	    sprintf(name, "DIFFUSE");
	else if (src_nr == 9)
	    sprintf(name, "SPECULAR");
	else if (src_nr == 10)
	    sprintf(name, "FOG");
	else {
	    fprintf(out, "bad src reg T%d\n", src_nr);
	    sprintf(name, "RESERVED");
	}
	break;
    case 4:
	sprintf(name, "oC");
	if (src_nr > 0)
	    fprintf(out, "bad src reg oC%d\n", src_nr);
	break;
    case 5:
	sprintf(name, "oD");
	if (src_nr > 0)
	    fprintf(out, "bad src reg oD%d\n", src_nr);
	break;
    default:
	fprintf(out, "bad src reg type %d\n", src_type);
	sprintf(name, "RESERVED");
	break;
    }
}

static void
i915_decode_alu1(uint32_t *data, uint32_t hw_offset,
		 int i, char *instr_prefix, char *op_name)
{
    char dst[100], src0[100];

    i915_get_instruction_dst(data, i, dst, 1);
    i915_get_instruction_src0(data, i, src0);

    instr_out(data, hw_offset, i++, "%s: %s %s, %s\n", instr_prefix,
	      op_name, dst, src0);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
}

static void
i915_decode_alu2(uint32_t *data, uint32_t hw_offset,
		 int i, char *instr_prefix, char *op_name)
{
    char dst[100], src0[100], src1[100];

    i915_get_instruction_dst(data, i, dst, 1);
    i915_get_instruction_src0(data, i, src0);
    i915_get_instruction_src1(data, i, src1);

    instr_out(data, hw_offset, i++, "%s: %s %s, %s, %s\n", instr_prefix,
	      op_name, dst, src0, src1);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
}

static void
i915_decode_alu3(uint32_t *data, uint32_t hw_offset,
		 int i, char *instr_prefix, char *op_name)
{
    char dst[100], src0[100], src1[100], src2[100];

    i915_get_instruction_dst(data, i, dst, 1);
    i915_get_instruction_src0(data, i, src0);
    i915_get_instruction_src1(data, i, src1);
    i915_get_instruction_src2(data, i, src2);

    instr_out(data, hw_offset, i++, "%s: %s %s, %s, %s, %s\n", instr_prefix,
	      op_name, dst, src0, src1, src2);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
}

static void
i915_decode_tex(uint32_t *data, uint32_t hw_offset, int i, char *instr_prefix,
		char *tex_name)
{
    uint32_t t0 = data[i];
    uint32_t t1 = data[i + 1];
    char dst_name[100];
    char addr_name[100];
    int sampler_nr;

    i915_get_instruction_dst(data, i, dst_name, 0);
    i915_get_instruction_addr((t1 >> 24) & 0x7,
			      (t1 >> 17) & 0xf,
			      addr_name);
    sampler_nr = t0 & 0xf;

    instr_out(data, hw_offset, i++, "%s: %s %s, S%d, %s\n", instr_prefix,
	      tex_name, dst_name, sampler_nr, addr_name);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
    instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
}

static void
i915_decode_dcl(uint32_t *data, uint32_t hw_offset, int i, char *instr_prefix)
{
    uint32_t d0 = data[i];
    char *sampletype;
    int dcl_nr = (d0 >> 14) & 0xf;
    char *dcl_x = d0 & (1 << 10) ? "x" : "";
    char *dcl_y = d0 & (1 << 11) ? "y" : "";
    char *dcl_z = d0 & (1 << 12) ? "z" : "";
    char *dcl_w = d0 & (1 << 13) ? "w" : "";
    char dcl_mask[10];

    switch ((d0 >> 19) & 0x3) {
    case 1:
	sprintf(dcl_mask, ".%s%s%s%s", dcl_x, dcl_y, dcl_z, dcl_w);
	if (strcmp(dcl_mask, ".") == 0)
	    fprintf(out, "bad (empty) dcl mask\n");

	if (dcl_nr > 10)
	    fprintf(out, "bad T%d dcl register number\n", dcl_nr);
	if (dcl_nr < 8) {
	    if (strcmp(dcl_mask, ".x") != 0 &&
		strcmp(dcl_mask, ".xy") != 0 &&
		strcmp(dcl_mask, ".xz") != 0 &&
		strcmp(dcl_mask, ".w") != 0 &&
		strcmp(dcl_mask, ".xyzw") != 0) {
		fprintf(out, "bad T%d.%s dcl mask\n", dcl_nr, dcl_mask);
	    }
	    instr_out(data, hw_offset, i++, "%s: DCL T%d%s\n", instr_prefix,
		      dcl_nr, dcl_mask);
	} else {
	    if (strcmp(dcl_mask, ".xz") == 0)
		fprintf(out, "errataed bad dcl mask %s\n", dcl_mask);
	    else if (strcmp(dcl_mask, ".xw") == 0)
		fprintf(out, "errataed bad dcl mask %s\n", dcl_mask);
	    else if (strcmp(dcl_mask, ".xzw") == 0)
		fprintf(out, "errataed bad dcl mask %s\n", dcl_mask);

	    if (dcl_nr == 8) {
		instr_out(data, hw_offset, i++, "%s: DCL DIFFUSE%s\n", instr_prefix,
			  dcl_mask);
	    } else if (dcl_nr == 9) {
		instr_out(data, hw_offset, i++, "%s: DCL SPECULAR%s\n", instr_prefix,
			  dcl_mask);
	    } else if (dcl_nr == 10) {
		instr_out(data, hw_offset, i++, "%s: DCL FOG%s\n", instr_prefix,
			  dcl_mask);
	    }
	}
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	break;
    case 3:
	switch ((d0 >> 22) & 0x3) {
	case 0:
	    sampletype = "2D";
	    break;
	case 1:
	    sampletype = "CUBE";
	    break;
	case 2:
	    sampletype = "3D";
	    break;
	default:
	    sampletype = "RESERVED";
	    break;
	}
	if (dcl_nr > 15)
	    fprintf(out, "bad S%d dcl register number\n", dcl_nr);
	instr_out(data, hw_offset, i++, "%s: DCL S%d %s\n", instr_prefix,
		  dcl_nr, sampletype);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	break;
    default:
	instr_out(data, hw_offset, i++, "%s: DCL RESERVED%d\n", instr_prefix, dcl_nr);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
    }
}

static void
i915_decode_instruction(uint32_t *data, uint32_t hw_offset,
			int i, char *instr_prefix)
{
    switch ((data[i] >> 24) & 0x1f) {
    case 0x0:
	instr_out(data, hw_offset, i++, "%s: NOP\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	break;
    case 0x01:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "ADD");
	break;
    case 0x02:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "MOV");
	break;
    case 0x03:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "MUL");
	break;
    case 0x04:
	i915_decode_alu3(data, hw_offset, i, instr_prefix, "MAD");
	break;
    case 0x05:
	i915_decode_alu3(data, hw_offset, i, instr_prefix, "DP2ADD");
	break;
    case 0x06:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "DP3");
	break;
    case 0x07:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "DP4");
	break;
    case 0x08:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "FRC");
	break;
    case 0x09:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "RCP");
	break;
    case 0x0a:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "RSQ");
	break;
    case 0x0b:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "EXP");
	break;
    case 0x0c:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "LOG");
	break;
    case 0x0d:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "CMP");
	break;
    case 0x0e:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "MIN");
	break;
    case 0x0f:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "MAX");
	break;
    case 0x10:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "FLR");
	break;
    case 0x11:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "MOD");
	break;
    case 0x12:
	i915_decode_alu1(data, hw_offset, i, instr_prefix, "TRC");
	break;
    case 0x13:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "SGE");
	break;
    case 0x14:
	i915_decode_alu2(data, hw_offset, i, instr_prefix, "SLT");
	break;
    case 0x15:
	i915_decode_tex(data, hw_offset, i, instr_prefix, "TEXLD");
	break;
    case 0x16:
	i915_decode_tex(data, hw_offset, i, instr_prefix, "TEXLDP");
	break;
    case 0x17:
	i915_decode_tex(data, hw_offset, i, instr_prefix, "TEXLDB");
	break;
    case 0x19:
	i915_decode_dcl(data, hw_offset, i, instr_prefix);
	break;
    default:
	instr_out(data, hw_offset, i++, "%s: unknown\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	instr_out(data, hw_offset, i++, "%s\n", instr_prefix);
	break;
    }
}

static char *
decode_compare_func(uint32_t op)
{
    switch (op&0x7) {
    case 0: return "always";
    case 1: return "never";
    case 2: return "less";
    case 3: return "equal";
    case 4: return "lequal";
    case 5: return "greater";
    case 6: return "notequal";
    case 7: return "gequal";
    }
    return "";
}

static char *
decode_stencil_op(uint32_t op)
{
    switch (op&0x7) {
    case 0: return "keep";
    case 1: return "zero";
    case 2: return "replace";
    case 3: return "incr_sat";
    case 4: return "decr_sat";
    case 5: return "greater";
    case 6: return "incr";
    case 7: return "decr";
    }
    return "";
}

static char *
decode_blend_fact(uint32_t op)
{
    switch (op&0xf) {
    case 1: return "zero";
    case 2: return "one";
    case 3: return "src_colr";
    case 4: return "inv_src_colr";
    case 5: return "src_alpha";
    case 6: return "inv_src_alpha";
    case 7: return "dst_alpha";
    case 8: return "inv_dst_alpha";
    case 9: return "dst_colr";
    case 10: return "inv_dst_colr";
    case 11: return "src_alpha_sat";
    case 12: return "cnst_colr";
    case 13: return "inv_cnst_colr";
    case 14: return "cnst_alpha";
    case 15: return "inv_const_alpha";
    }
    return "";
}

static char *
decode_tex_coord_mode(uint32_t mode)
{
    switch (mode&0x7) {
    case 0: return "wrap";
    case 1: return "mirror";
    case 2: return "clamp_edge";
    case 3: return "cube";
    case 4: return "clamp_border";
    case 5: return "mirror_once";
    }
    return "";
}

static char *
decode_sample_filter(uint32_t mode)
{
    switch (mode&0x7) {
    case 0: return "nearest";
    case 1: return "linear";
    case 2: return "anisotropic";
    case 3: return "4x4_1";
    case 4: return "4x4_2";
    case 5: return "4x4_flat";
    case 6: return "6x5_mono";
    }
    return "";
}

static int
decode_3d_1d(uint32_t *data, int count,
	     uint32_t hw_offset,
	     uint32_t devid,
	     int *failures)
{
    unsigned int len, i, c, idx, word, map, sampler, instr;
    char *format = "", *zformat, *type;
    uint32_t opcode;

    struct {
	uint32_t opcode;
	int i830_only;
	int min_len;
	int max_len;
	char *name;
    } opcodes_3d_1d[] = {
	{ 0x86, 0, 4, 4, "3DSTATE_CHROMA_KEY" },
	{ 0x88, 0, 2, 2, "3DSTATE_CONSTANT_BLEND_COLOR" },
	{ 0x99, 0, 2, 2, "3DSTATE_DEFAULT_DIFFUSE" },
	{ 0x9a, 0, 2, 2, "3DSTATE_DEFAULT_SPECULAR" },
	{ 0x98, 0, 2, 2, "3DSTATE_DEFAULT_Z" },
	{ 0x97, 0, 2, 2, "3DSTATE_DEPTH_OFFSET_SCALE" },
	{ 0x9d, 0, 65, 65, "3DSTATE_FILTER_COEFFICIENTS_4X4" },
	{ 0x9e, 0, 4, 4, "3DSTATE_MONO_FILTER" },
	{ 0x89, 0, 4, 4, "3DSTATE_FOG_MODE" },
	{ 0x8f, 0, 2, 16, "3DSTATE_MAP_PALLETE_LOAD_32" },
	{ 0x83, 0, 2, 2, "3DSTATE_SPAN_STIPPLE" },
	{ 0x8c, 1, 2, 2, "3DSTATE_MAP_COORD_TRANSFORM_I830" },
	{ 0x8b, 1, 2, 2, "3DSTATE_MAP_VERTEX_TRANSFORM_I830" },
	{ 0x8d, 1, 3, 3, "3DSTATE_W_STATE_I830" },
	{ 0x01, 1, 2, 2, "3DSTATE_COLOR_FACTOR_I830" },
	{ 0x02, 1, 2, 2, "3DSTATE_MAP_COORD_SETBIND_I830" },
    }, *opcode_3d_1d;

    opcode = (data[0] & 0x00ff0000) >> 16;

    switch (opcode) {
    case 0x07:
	/* This instruction is unusual.  A 0 length means just 1 DWORD instead of
	 * 2.  The 0 length is specified in one place to be unsupported, but
	 * stated to be required in another, and 0 length LOAD_INDIRECTs appear
	 * to cause no harm at least.
	 */
	instr_out(data, hw_offset, 0, "3DSTATE_LOAD_INDIRECT\n");
	len = (data[0] & 0x000000ff) + 1;
	i = 1;
	if (data[0] & (0x01 << 8)) {
	    if (i + 2 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_LOAD_INDIRECT");
	    instr_out(data, hw_offset, i++, "SIS.0\n");
	    instr_out(data, hw_offset, i++, "SIS.1\n");
	}
	if (data[0] & (0x02 << 8)) {
	    if (i + 1 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_LOAD_INDIRECT");
	    instr_out(data, hw_offset, i++, "DIS.0\n");
	}
	if (data[0] & (0x04 << 8)) {
	    if (i + 2 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_LOAD_INDIRECT");
	    instr_out(data, hw_offset, i++, "SSB.0\n");
	    instr_out(data, hw_offset, i++, "SSB.1\n");
	}
	if (data[0] & (0x08 << 8)) {
	    if (i + 2 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_LOAD_INDIRECT");
	    instr_out(data, hw_offset, i++, "MSB.0\n");
	    instr_out(data, hw_offset, i++, "MSB.1\n");
	}
	if (data[0] & (0x10 << 8)) {
	    if (i + 2 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_LOAD_INDIRECT");
	    instr_out(data, hw_offset, i++, "PSP.0\n");
	    instr_out(data, hw_offset, i++, "PSP.1\n");
	}
	if (data[0] & (0x20 << 8)) {
	    if (i + 2 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_LOAD_INDIRECT");
	    instr_out(data, hw_offset, i++, "PSC.0\n");
	    instr_out(data, hw_offset, i++, "PSC.1\n");
	}
	if (len != i) {
	    fprintf(out, "Bad count in 3DSTATE_LOAD_INDIRECT\n");
	    (*failures)++;
	    return len;
	}
	return len;
    case 0x04:
	instr_out(data, hw_offset, 0, "3DSTATE_LOAD_STATE_IMMEDIATE_1\n");
	len = (data[0] & 0x0000000f) + 2;
	i = 1;
	for (word = 0; word <= 8; word++) {
	    if (data[0] & (1 << (4 + word))) {
		if (i >= count)
		    BUFFER_FAIL(count, len, "3DSTATE_LOAD_STATE_IMMEDIATE_1");

		/* save vertex state for decode */
		if (!IS_GEN2(devid)) {
			if (word == 2) {
				saved_s2_set = 1;
				saved_s2 = data[i];
			}
			if (word == 4) {
				saved_s4_set = 1;
				saved_s4 = data[i];
			}

			switch (word) {
			case 0:
				instr_out(data, hw_offset, i, "S0: vbo offset: 0x%08x%s\n",
					  data[i]&(~1),data[i]&1?", auto cache invalidate disabled":"");
				break;
			case 1:
				instr_out(data, hw_offset, i, "S1: vertex width: %i, vertex pitch: %i\n",
					  (data[i]>>24)&0x3f,(data[i]>>16)&0x3f);
				break;
			case 2:
				instr_out(data, hw_offset, i, "S2: texcoord formats: ");
				for (int tex_num = 0; tex_num < 8; tex_num++) {
					switch((data[i]>>tex_num*4)&0xf) {
					case 0: fprintf(out, "%i=2D ", tex_num); break;
					case 1: fprintf(out, "%i=3D ", tex_num); break;
					case 2: fprintf(out, "%i=4D ", tex_num); break;
					case 3: fprintf(out, "%i=1D ", tex_num); break;
					case 4: fprintf(out, "%i=2D_16 ", tex_num); break;
					case 5: fprintf(out, "%i=4D_16 ", tex_num); break;
					case 0xf: fprintf(out, "%i=NP ", tex_num); break;
					}
				}
				fprintf(out, "\n");

				break;
			case 3:
				instr_out(data, hw_offset, i, "S3: not documented\n", word);
				break;
			case 4:
				{
					char *cullmode = "";
					char *vfmt_xyzw = "";
					switch((data[i]>>13)&0x3) {
					case 0: cullmode = "both"; break;
					case 1: cullmode = "none"; break;
					case 2: cullmode = "cw"; break;
					case 3: cullmode = "ccw"; break;
					}
					switch(data[i] & (7<<6 | 1<<2)) {
					case 1<<6: vfmt_xyzw = "XYZ,"; break;
					case 2<<6: vfmt_xyzw = "XYZW,"; break;
					case 3<<6: vfmt_xyzw = "XY,"; break;
					case 4<<6: vfmt_xyzw = "XYW,"; break;
					case 1<<6 | 1<<2: vfmt_xyzw = "XYZF,"; break;
					case 2<<6 | 1<<2: vfmt_xyzw = "XYZWF,"; break;
					case 3<<6 | 1<<2: vfmt_xyzw = "XYF,"; break;
					case 4<<6 | 1<<2: vfmt_xyzw = "XYWF,"; break;
					}
					instr_out(data, hw_offset, i, "S4: point_width=%i, line_width=%.1f,"
						  "%s%s%s%s%s cullmode=%s, vfmt=%s%s%s%s%s%s "
						  "%s%s\n",
						  (data[i]>>23)&0x1ff,
						  ((data[i]>>19)&0xf) / 2.0,
						  data[i]&(0xf<<15)?" flatshade=":"",
						  data[i]&(1<<18)?"Alpha,":"",
						  data[i]&(1<<17)?"Fog,":"",
						  data[i]&(1<<16)?"Specular,":"",
						  data[i]&(1<<15)?"Color,":"",
						  cullmode,
						  data[i]&(1<<12)?"PointWidth,":"",
						  data[i]&(1<<11)?"SpecFog,":"",
						  data[i]&(1<<10)?"Color,":"",
						  data[i]&(1<<9)?"DepthOfs,":"",
						  vfmt_xyzw,
						  data[i]&(1<<9)?"FogParam,":"",
						  data[i]&(1<<5)?"force default diffuse, ":"",
						  data[i]&(1<<4)?"force default specular, ":"",
						  data[i]&(1<<3)?"local depth ofs enable, ":"",
						  data[i]&(1<<1)?"point sprite enable, ":"",
						  data[i]&(1<<0)?"line AA enable, ":"");
					break;
				}
			case 5:
				{
					instr_out(data, hw_offset, i, "S5:%s%s%s%s%s"
						  "%s%s%s%s stencil_ref=0x%x, stencil_test=%s, "
						  "stencil_fail=%s, stencil_pass_z_fail=%s, "
						  "stencil_pass_z_pass=%s, %s%s%s%s\n",
						  data[i]&(0xf<<28)?" write_disable=":"",
						  data[i]&(1<<31)?"Alpha,":"",
						  data[i]&(1<<30)?"Red,":"",
						  data[i]&(1<<29)?"Green,":"",
						  data[i]&(1<<28)?"Blue,":"",
						  data[i]&(1<<27)?" force default point size,":"",
						  data[i]&(1<<26)?" last pixel enable,":"",
						  data[i]&(1<<25)?" global depth ofs enable,":"",
						  data[i]&(1<<24)?" fog enable,":"",
						  (data[i]>>16)&0xff,
						  decode_compare_func(data[i]>>13),
						  decode_stencil_op(data[i]>>10),
						  decode_stencil_op(data[i]>>7),
						  decode_stencil_op(data[i]>>4),
						  data[i]&(1<<3)?"stencil write enable, ":"",
						  data[i]&(1<<2)?"stencil test enable, ":"",
						  data[i]&(1<<1)?"color dither enable, ":"",
						  data[i]&(1<<0)?"logicop enable, ":"");
				}
				break;
			case 6:
				instr_out(data, hw_offset, i, "S6: %salpha_test=%s, alpha_ref=0x%x, "
					  "depth_test=%s, %ssrc_blnd_fct=%s, dst_blnd_fct=%s, "
					  "%s%stristrip_provoking_vertex=%i\n",
					  data[i]&(1<<31)?"alpha test enable, ":"",
					  decode_compare_func(data[i]>>28),
					  data[i]&(0xff<<20),
					  decode_compare_func(data[i]>>16),
					  data[i]&(1<<15)?"cbuf blend enable, ":"",
					  decode_blend_fact(data[i]>>8),
					  decode_blend_fact(data[i]>>4),
					  data[i]&(1<<3)?"depth write enable, ":"",
					  data[i]&(1<<2)?"cbuf write enable, ":"",
					  data[i]&(0x3));
				break;
			case 7:
				instr_out(data, hw_offset, i, "S7: depth offset constant: 0x%08x\n", data[i]);
				break;
			}
		} else {
			instr_out(data, hw_offset, i, "S%d: 0x%08x\n", i, data[i]);
		}
		i++;
	    }
	}
	if (len != i) {
	    fprintf(out, "Bad count in 3DSTATE_LOAD_STATE_IMMEDIATE_1\n");
	    (*failures)++;
	}
	return len;
    case 0x03:
	instr_out(data, hw_offset, 0, "3DSTATE_LOAD_STATE_IMMEDIATE_2\n");
	len = (data[0] & 0x0000000f) + 2;
	i = 1;
	for (word = 6; word <= 14; word++) {
	    if (data[0] & (1 << word)) {
		if (i >= count)
		    BUFFER_FAIL(count, len, "3DSTATE_LOAD_STATE_IMMEDIATE_2");

		if (word == 6)
		    instr_out(data, hw_offset, i++, "TBCF\n");
		else if (word >= 7 && word <= 10) {
		    instr_out(data, hw_offset, i++, "TB%dC\n", word - 7);
		    instr_out(data, hw_offset, i++, "TB%dA\n", word - 7);
		} else if (word >= 11 && word <= 14) {
		    instr_out(data, hw_offset, i, "TM%dS0: offset=0x%08x, %s\n",
			      word - 11,
			      data[i]&0xfffffffe,
			      data[i]&1?"use fence":"");
		    i++;
		    instr_out(data, hw_offset, i, "TM%dS1: height=%i, width=%i, %s\n",
			      word - 11,
			      data[i]>>21, (data[i]>>10)&0x3ff,
			      data[i]&2?(data[i]&1?"y-tiled":"x-tiled"):"");
		    i++;
		    instr_out(data, hw_offset, i, "TM%dS2: pitch=%i, \n",
			      word - 11,
			      ((data[i]>>21) + 1)*4);
		    i++;
		    instr_out(data, hw_offset, i++, "TM%dS3\n", word - 11);
		    instr_out(data, hw_offset, i++, "TM%dS4: dflt color\n", word - 11);
		}
	    }
	}
	if (len != i) {
	    fprintf(out, "Bad count in 3DSTATE_LOAD_STATE_IMMEDIATE_2\n");
	    (*failures)++;
	}
	return len;
    case 0x00:
	instr_out(data, hw_offset, 0, "3DSTATE_MAP_STATE\n");
	len = (data[0] & 0x0000003f) + 2;
	instr_out(data, hw_offset, 1, "mask\n");

	i = 2;
	for (map = 0; map <= 15; map++) {
	    if (data[1] & (1 << map)) {
		int width, height, pitch, dword;
		const char *tiling;

		if (i + 3 >= count)
		    BUFFER_FAIL(count, len, "3DSTATE_MAP_STATE");

		dword = data[i];
		instr_out(data, hw_offset, i++, "map %d MS2 %s%s%s\n", map,
			  dword&(1<<31)?"untrusted surface, ":"",
			  dword&(1<<1)?"vertical line stride enable, ":"",
			  dword&(1<<0)?"vertical ofs enable, ":"");

		dword = data[i];
		width = ((dword >> 10) & ((1 << 11) - 1))+1;
		height = ((dword >> 21) & ((1 << 11) - 1))+1;

		tiling = "none";
		if (dword & (1 << 2))
			tiling = "fenced";
		else if (dword & (1 << 1))
			tiling = dword & (1 << 0) ? "Y" : "X";
		type = " BAD";
		switch ((dword>>7) & 0x7) {
		case 1:
		    type = "8b";
		    switch ((dword>>3) & 0xf) {
		    case 0: format = "I"; break;
		    case 1: format = "L"; break;
		    case 2: format = "A"; break;
		    case 3: format = " mono"; break; }
		    break;
		case 2:
		    type = "16b";
		    switch ((dword>>3) & 0xf) {
		    case 0: format = " rgb565"; break;
		    case 1: format = " argb1555"; break;
		    case 2: format = " argb4444"; break;
		    case 5: format = " ay88"; break;
		    case 6: format = " bump655"; break;
		    case 7: format = "I"; break;
		    case 8: format = "L"; break;
		    case 9: format = "A"; break; }
		    break;
		case 3:
		    type = "32b";
		    switch ((dword>>3) & 0xf) {
		    case 0: format = " argb8888"; break;
		    case 1: format = " abgr8888"; break;
		    case 2: format = " xrgb8888"; break;
		    case 3: format = " xbgr8888"; break;
		    case 4: format = " qwvu8888"; break;
		    case 5: format = " axvu8888"; break;
		    case 6: format = " lxvu8888"; break;
		    case 7: format = " xlvu8888"; break;
		    case 8: format = " argb2101010"; break;
		    case 9: format = " abgr2101010"; break;
		    case 10: format = " awvu2101010"; break;
		    case 11: format = " gr1616"; break;
		    case 12: format = " vu1616"; break;
		    case 13: format = " xI824"; break;
		    case 14: format = " xA824"; break;
		    case 15: format = " xL824"; break; }
		    break;
		case 5:
		    type = "422";
		    switch ((dword>>3) & 0xf) {
		    case 0: format = " yuv_swapy"; break;
		    case 1: format = " yuv"; break;
		    case 2: format = " yuv_swapuv"; break;
		    case 3: format = " yuv_swapuvy"; break; }
		    break;
		case 6:
		    type = "compressed";
		    switch ((dword>>3) & 0x7) {
		    case 0: format = " dxt1"; break;
		    case 1: format = " dxt2_3"; break;
		    case 2: format = " dxt4_5"; break;
		    case 3: format = " fxt1"; break;
		    case 4: format = " dxt1_rb"; break; }
		    break;
		case 7:
		    type = "4b indexed";
		    switch ((dword>>3) & 0xf) {
		    case 7: format = " argb8888"; break; }
		    break;
		default:
		    format = "BAD";
		}
		dword = data[i];
		instr_out(data, hw_offset, i++, "map %d MS3 [width=%d, height=%d, format=%s%s, tiling=%s%s]\n",
			  map, width, height, type, format, tiling,
			  dword&(1<<9)?" palette select":"");

		dword = data[i];
		pitch = 4*(((dword >> 21) & ((1 << 11) - 1))+1);
		instr_out(data, hw_offset, i++, "map %d MS4 [pitch=%d, max_lod=%i, vol_depth=%i, cube_face_ena=%x, %s]\n",
			  map, pitch,
			  (dword>>9)&0x3f, dword&0xff, (dword>>15)&0x3f,
			  dword&(1<<8)?"miplayout legacy":"miplayout right");
	    }
	}
	if (len != i) {
	    fprintf(out, "Bad count in 3DSTATE_MAP_STATE\n");
	    (*failures)++;
	    return len;
	}
	return len;
    case 0x06:
	instr_out(data, hw_offset, 0, "3DSTATE_PIXEL_SHADER_CONSTANTS\n");
	len = (data[0] & 0x000000ff) + 2;

	i = 2;
	for (c = 0; c <= 31; c++) {
	    if (data[1] & (1 << c)) {
		if (i + 4 >= count)
		    BUFFER_FAIL(count, len, "3DSTATE_PIXEL_SHADER_CONSTANTS");
		instr_out(data, hw_offset, i, "C%d.X = %f\n",
			  c, int_as_float(data[i]));
		i++;
		instr_out(data, hw_offset, i, "C%d.Y = %f\n",
			  c, int_as_float(data[i]));
		i++;
		instr_out(data, hw_offset, i, "C%d.Z = %f\n",
			  c, int_as_float(data[i]));
		i++;
		instr_out(data, hw_offset, i, "C%d.W = %f\n",
			  c, int_as_float(data[i]));
		i++;
	    }
	}
	if (len != i) {
	    fprintf(out, "Bad count in 3DSTATE_PIXEL_SHADER_CONSTANTS\n");
	    (*failures)++;
	}
	return len;
    case 0x05:
	instr_out(data, hw_offset, 0, "3DSTATE_PIXEL_SHADER_PROGRAM\n");
	len = (data[0] & 0x000000ff) + 2;
	if ((len - 1) % 3 != 0 || len > 370) {
	    fprintf(out, "Bad count in 3DSTATE_PIXEL_SHADER_PROGRAM\n");
	    (*failures)++;
	}
	i = 1;
	for (instr = 0; instr < (len - 1) / 3; instr++) {
	    char instr_prefix[10];

	    if (i + 3 >= count)
		BUFFER_FAIL(count, len, "3DSTATE_PIXEL_SHADER_PROGRAM");
	    sprintf(instr_prefix, "PS%03d", instr);
	    i915_decode_instruction(data, hw_offset, i, instr_prefix);
	    i += 3;
	}
	return len;
    case 0x01:
	if (IS_GEN2(devid))
		break;
	instr_out(data, hw_offset, 0, "3DSTATE_SAMPLER_STATE\n");
	instr_out(data, hw_offset, 1, "mask\n");
	len = (data[0] & 0x0000003f) + 2;
	i = 2;
	for (sampler = 0; sampler <= 15; sampler++) {
	    if (data[1] & (1 << sampler)) {
		uint32_t dword;
		char *mip_filter = "";
		if (i + 3 >= count)
		    BUFFER_FAIL(count, len, "3DSTATE_SAMPLER_STATE");
		dword = data[i];
		switch ((dword>>20)&0x3) {
		case 0: mip_filter = "none"; break;
		case 1: mip_filter = "nearest"; break;
		case 3: mip_filter = "linear"; break;
		}
		instr_out(data, hw_offset, i++, "sampler %d SS2:%s%s%s "
			  "base_mip_level=%i, mip_filter=%s, mag_filter=%s, min_filter=%s "
			  "lod_bias=%.2f,%s max_aniso=%i, shadow_func=%s\n", sampler,
			  dword&(1<<31)?" reverse gamma,":"",
			  dword&(1<<30)?" packed2planar,":"",
			  dword&(1<<29)?" colorspace conversion,":"",
			  (dword>>22)&0x1f,
			  mip_filter,
			  decode_sample_filter(dword>>17),
			  decode_sample_filter(dword>>14),
			  ((dword>>5)&0x1ff)/(0x10*1.0),
			  dword&(1<<4)?" shadow,":"",
			  dword&(1<<3)?4:2,
			  decode_compare_func(dword));
		dword = data[i];
		instr_out(data, hw_offset, i++, "sampler %d SS3: min_lod=%.2f,%s "
			  "tcmode_x=%s, tcmode_y=%s, tcmode_z=%s,%s texmap_idx=%i,%s\n",
			  sampler, ((dword>>24)&0xff)/(0x10*1.0),
			  dword&(1<<17)?" kill pixel enable,":"",
			  decode_tex_coord_mode(dword>>12),
			  decode_tex_coord_mode(dword>>9),
			  decode_tex_coord_mode(dword>>6),
			  dword&(1<<5)?" normalized coords,":"",
			  (dword>>1)&0xf,
			  dword&(1<<0)?" deinterlacer,":"");
		dword = data[i];
		instr_out(data, hw_offset, i++, "sampler %d SS4: border color\n",
			  sampler, ((dword>>24)&0xff)/(0x10*1.0),
			  dword);
	    }
	}
	if (len != i) {
	    fprintf(out, "Bad count in 3DSTATE_SAMPLER_STATE\n");
	    (*failures)++;
	}
	return len;
    case 0x85:
	len = (data[0] & 0x0000000f) + 2;

	if (len != 2)
	    fprintf(out, "Bad count in 3DSTATE_DEST_BUFFER_VARIABLES\n");
	if (count < 2)
	    BUFFER_FAIL(count, len, "3DSTATE_DEST_BUFFER_VARIABLES");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_DEST_BUFFER_VARIABLES\n");

	switch ((data[1] >> 8) & 0xf) {
	case 0x0: format = "g8"; break;
	case 0x1: format = "x1r5g5b5"; break;
	case 0x2: format = "r5g6b5"; break;
	case 0x3: format = "a8r8g8b8"; break;
	case 0x4: format = "ycrcb_swapy"; break;
	case 0x5: format = "ycrcb_normal"; break;
	case 0x6: format = "ycrcb_swapuv"; break;
	case 0x7: format = "ycrcb_swapuvy"; break;
	case 0x8: format = "a4r4g4b4"; break;
	case 0x9: format = "a1r5g5b5"; break;
	case 0xa: format = "a2r10g10b10"; break;
	default: format = "BAD"; break;
	}
	switch ((data[1] >> 2) & 0x3) {
	case 0x0: zformat = "u16"; break;
	case 0x1: zformat = "f16"; break;
	case 0x2: zformat = "u24x8"; break;
	default: zformat = "BAD"; break;
	}
	instr_out(data, hw_offset, 1, "%s format, %s depth format, early Z %sabled\n",
		  format, zformat,
		  (data[1] & (1 << 31)) ? "en" : "dis");
	return len;

    case 0x8e:
	{
	    const char *name, *tiling;

	    len = (data[0] & 0x0000000f) + 2;
	    if (len != 3)
		fprintf(out, "Bad count in 3DSTATE_BUFFER_INFO\n");
	    if (count < 3)
		BUFFER_FAIL(count, len, "3DSTATE_BUFFER_INFO");

	    switch((data[1] >> 24) & 0x7) {
	    case 0x3: name = "color"; break;
	    case 0x7: name = "depth"; break;
	    default: name = "unknown"; break;
	    }

	    tiling = "none";
	    if (data[1] & (1 << 23))
		tiling = "fenced";
	    else if (data[1] & (1 << 22))
		tiling = data[1] & (1 << 21) ? "Y" : "X";

	    instr_out(data, hw_offset, 0, "3DSTATE_BUFFER_INFO\n");
	    instr_out(data, hw_offset, 1, "%s, tiling = %s, pitch=%d\n", name, tiling, data[1]&0xffff);

	    instr_out(data, hw_offset, 2, "address\n");
	    return len;
	}
    case 0x81:
	len = (data[0] & 0x0000000f) + 2;

	if (len != 3)
	    fprintf(out, "Bad count in 3DSTATE_SCISSOR_RECTANGLE\n");
	if (count < 3)
	    BUFFER_FAIL(count, len, "3DSTATE_SCISSOR_RECTANGLE");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_SCISSOR_RECTANGLE\n");
	instr_out(data, hw_offset, 1, "(%d,%d)\n",
		  data[1] & 0xffff, data[1] >> 16);
	instr_out(data, hw_offset, 2, "(%d,%d)\n",
		  data[2] & 0xffff, data[2] >> 16);

	return len;
    case 0x80:
	len = (data[0] & 0x0000000f) + 2;

	if (len != 5)
	    fprintf(out, "Bad count in 3DSTATE_DRAWING_RECTANGLE\n");
	if (count < 5)
	    BUFFER_FAIL(count, len, "3DSTATE_DRAWING_RECTANGLE");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_DRAWING_RECTANGLE\n");
	instr_out(data, hw_offset, 1, "%s\n",
		  data[1]&(1<<30)?"depth ofs disabled ":"");
	instr_out(data, hw_offset, 2, "(%d,%d)\n",
		  data[2] & 0xffff, data[2] >> 16);
	instr_out(data, hw_offset, 3, "(%d,%d)\n",
		  data[3] & 0xffff, data[3] >> 16);
	instr_out(data, hw_offset, 4, "(%d,%d)\n",
		  data[4] & 0xffff, data[4] >> 16);

	return len;
    case 0x9c:
	len = (data[0] & 0x0000000f) + 2;

	if (len != 7)
	    fprintf(out, "Bad count in 3DSTATE_CLEAR_PARAMETERS\n");
	if (count < 7)
	    BUFFER_FAIL(count, len, "3DSTATE_CLEAR_PARAMETERS");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_CLEAR_PARAMETERS\n");
	instr_out(data, hw_offset, 1, "prim_type=%s, clear=%s%s%s\n",
		  data[1]&(1<<16)?"CLEAR_RECT":"ZONE_INIT",
		  data[1]&(1<<2)?"color,":"",
		  data[1]&(1<<1)?"depth,":"",
		  data[1]&(1<<0)?"stencil,":"");
	instr_out(data, hw_offset, 2, "clear color\n");
	instr_out(data, hw_offset, 3, "clear depth/stencil\n");
	instr_out(data, hw_offset, 4, "color value (rgba8888)\n");
	instr_out(data, hw_offset, 5, "depth value %f\n",
		  int_as_float(data[5]));
	instr_out(data, hw_offset, 6, "clear stencil\n");
	return len;
    }

    for (idx = 0; idx < ARRAY_SIZE(opcodes_3d_1d); idx++)
    {
	opcode_3d_1d = &opcodes_3d_1d[idx];
	if (opcode_3d_1d->i830_only && !IS_GEN2(devid))
	    continue;

	if (((data[0] & 0x00ff0000) >> 16) == opcode_3d_1d->opcode) {
	    len = 1;

	    instr_out(data, hw_offset, 0, "%s\n", opcode_3d_1d->name);
	    if (opcode_3d_1d->max_len > 1) {
		len = (data[0] & 0x0000ffff) + 2;
		if (len < opcode_3d_1d->min_len ||
		    len > opcode_3d_1d->max_len)
		{
		    fprintf(out, "Bad count in %s\n",
			    opcode_3d_1d->name);
		    (*failures)++;
		}
	    }

	    for (i = 1; i < len; i++) {
		if (i >= count)
		    BUFFER_FAIL(count, len,  opcode_3d_1d->name);
		instr_out(data, hw_offset, i, "dword %d\n", i);
	    }

	    return len;
	}
    }

    instr_out(data, hw_offset, 0, "3D UNKNOWN: 3d_1d opcode = 0x%x\n", opcode);
    (*failures)++;
    return 1;
}

static int
decode_3d_primitive(uint32_t *data, int count, uint32_t hw_offset,
		    int *failures)
{
    char immediate = (data[0] & (1 << 23)) == 0;
    unsigned int len, i, j, ret;
    char *primtype;
    int original_s2 = saved_s2;
    int original_s4 = saved_s4;

    switch ((data[0] >> 18) & 0xf) {
    case 0x0: primtype = "TRILIST"; break;
    case 0x1: primtype = "TRISTRIP"; break;
    case 0x2: primtype = "TRISTRIP_REVERSE"; break;
    case 0x3: primtype = "TRIFAN"; break;
    case 0x4: primtype = "POLYGON"; break;
    case 0x5: primtype = "LINELIST"; break;
    case 0x6: primtype = "LINESTRIP"; break;
    case 0x7: primtype = "RECTLIST"; break;
    case 0x8: primtype = "POINTLIST"; break;
    case 0x9: primtype = "DIB"; break;
    case 0xa: primtype = "CLEAR_RECT"; saved_s4 = 3 << 6; saved_s2 = ~0; break;
    default: primtype = "unknown"; break;
    }

    /* XXX: 3DPRIM_DIB not supported */
    if (immediate) {
	len = (data[0] & 0x0003ffff) + 2;
	instr_out(data, hw_offset, 0, "3DPRIMITIVE inline %s\n", primtype);
	if (count < len)
	    BUFFER_FAIL(count, len,  "3DPRIMITIVE inline");
	if (!saved_s2_set || !saved_s4_set) {
	    fprintf(out, "unknown vertex format\n");
	    for (i = 1; i < len; i++) {
		instr_out(data, hw_offset, i,
			  "           vertex data (%f float)\n",
			  int_as_float(data[i]));
	    }
	} else {
	    unsigned int vertex = 0;
	    for (i = 1; i < len;) {
		unsigned int tc;

#define VERTEX_OUT(fmt, ...) do {					\
    if (i < len)							\
	instr_out(data, hw_offset, i, " V%d."fmt"\n", vertex, __VA_ARGS__); \
    else								\
	fprintf(out, " missing data in V%d\n", vertex);			\
    i++;								\
} while (0)

		VERTEX_OUT("X = %f", int_as_float(data[i]));
		VERTEX_OUT("Y = %f", int_as_float(data[i]));
	        switch (saved_s4 >> 6 & 0x7) {
		case 0x1:
		    VERTEX_OUT("Z = %f", int_as_float(data[i]));
		    break;
		case 0x2:
		    VERTEX_OUT("Z = %f", int_as_float(data[i]));
		    VERTEX_OUT("W = %f", int_as_float(data[i]));
		    break;
		case 0x3:
		    break;
		case 0x4:
		    VERTEX_OUT("W = %f", int_as_float(data[i]));
		    break;
		default:
		    fprintf(out, "bad S4 position mask\n");
		}

		if (saved_s4 & (1 << 10)) {
		    VERTEX_OUT("color = (A=0x%02x, R=0x%02x, G=0x%02x, "
			       "B=0x%02x)",
			       data[i] >> 24,
			       (data[i] >> 16) & 0xff,
			       (data[i] >> 8) & 0xff,
			       data[i] & 0xff);
		}
		if (saved_s4 & (1 << 11)) {
		    VERTEX_OUT("spec = (A=0x%02x, R=0x%02x, G=0x%02x, "
			       "B=0x%02x)",
			       data[i] >> 24,
			       (data[i] >> 16) & 0xff,
			       (data[i] >> 8) & 0xff,
			       data[i] & 0xff);
		}
		if (saved_s4 & (1 << 12))
		    VERTEX_OUT("width = 0x%08x)", data[i]);

		for (tc = 0; tc <= 7; tc++) {
		    switch ((saved_s2 >> (tc * 4)) & 0xf) {
		    case 0x0:
			VERTEX_OUT("T%d.X = %f", tc, int_as_float(data[i]));
			VERTEX_OUT("T%d.Y = %f", tc, int_as_float(data[i]));
			break;
		    case 0x1:
			VERTEX_OUT("T%d.X = %f", tc, int_as_float(data[i]));
			VERTEX_OUT("T%d.Y = %f", tc, int_as_float(data[i]));
			VERTEX_OUT("T%d.Z = %f", tc, int_as_float(data[i]));
			break;
		    case 0x2:
			VERTEX_OUT("T%d.X = %f", tc, int_as_float(data[i]));
			VERTEX_OUT("T%d.Y = %f", tc, int_as_float(data[i]));
			VERTEX_OUT("T%d.Z = %f", tc, int_as_float(data[i]));
			VERTEX_OUT("T%d.W = %f", tc, int_as_float(data[i]));
			break;
		    case 0x3:
			VERTEX_OUT("T%d.X = %f", tc, int_as_float(data[i]));
			break;
		    case 0x4:
			VERTEX_OUT("T%d.XY = 0x%08x half-float", tc, data[i]);
			break;
		    case 0x5:
			VERTEX_OUT("T%d.XY = 0x%08x half-float", tc, data[i]);
			VERTEX_OUT("T%d.ZW = 0x%08x half-float", tc, data[i]);
			break;
		    case 0xf:
			break;
		    default:
			fprintf(out, "bad S2.T%d format\n", tc);
		    }
		}
		vertex++;
	    }
	}

	ret = len;
    } else {
	/* indirect vertices */
	len = data[0] & 0x0000ffff; /* index count */
	if (data[0] & (1 << 17)) {
	    /* random vertex access */
	    if (count < (len + 1) / 2 + 1) {
		BUFFER_FAIL(count, (len + 1) / 2 + 1,
			    "3DPRIMITIVE random indirect");
	    }
	    instr_out(data, hw_offset, 0,
		      "3DPRIMITIVE random indirect %s (%d)\n", primtype, len);
	    if (len == 0) {
		/* vertex indices continue until 0xffff is found */
		for (i = 1; i < count; i++) {
		    if ((data[i] & 0xffff) == 0xffff) {
			instr_out(data, hw_offset, i,
				  "    indices: (terminator)\n");
			ret = i;
			goto out;
		    } else if ((data[i] >> 16) == 0xffff) {
			instr_out(data, hw_offset, i,
				  "    indices: 0x%04x, (terminator)\n",
				  data[i] & 0xffff);
			ret = i;
			goto out;
		    } else {
			instr_out(data, hw_offset, i,
				  "    indices: 0x%04x, 0x%04x\n",
				  data[i] & 0xffff, data[i] >> 16);
		    }
		}
		fprintf(out,
			"3DPRIMITIVE: no terminator found in index buffer\n");
		(*failures)++;
		ret = count;
		goto out;
	    } else {
		/* fixed size vertex index buffer */
		for (j = 1, i = 0; i < len; i += 2, j++) {
		    if (i * 2 == len - 1) {
			instr_out(data, hw_offset, j,
				  "    indices: 0x%04x\n",
				  data[j] & 0xffff);
		    } else {
			instr_out(data, hw_offset, j,
				  "    indices: 0x%04x, 0x%04x\n",
				  data[j] & 0xffff, data[j] >> 16);
		    }
		}
	    }
	    ret = (len + 1) / 2 + 1;
	    goto out;
	} else {
	    /* sequential vertex access */
	    if (count < 2)
		BUFFER_FAIL(count, 2, "3DPRIMITIVE seq indirect");
	    instr_out(data, hw_offset, 0,
		      "3DPRIMITIVE sequential indirect %s, %d starting from "
		      "%d\n", primtype, len, data[1] & 0xffff);
	    instr_out(data, hw_offset, 1, "           start\n");
	    ret = 2;
	    goto out;
	}
    }

out:
    saved_s2 = original_s2;
    saved_s4 = original_s4;
    return ret;
}

static int
decode_3d(uint32_t *data, int count, uint32_t hw_offset, uint32_t devid, int *failures)
{
    uint32_t opcode;
    unsigned int idx;

    struct {
	uint32_t opcode;
	int min_len;
	int max_len;
	char *name;
    } opcodes_3d[] = {
	{ 0x06, 1, 1, "3DSTATE_ANTI_ALIASING" },
	{ 0x08, 1, 1, "3DSTATE_BACKFACE_STENCIL_OPS" },
	{ 0x09, 1, 1, "3DSTATE_BACKFACE_STENCIL_MASKS" },
	{ 0x16, 1, 1, "3DSTATE_COORD_SET_BINDINGS" },
	{ 0x15, 1, 1, "3DSTATE_FOG_COLOR" },
	{ 0x0b, 1, 1, "3DSTATE_INDEPENDENT_ALPHA_BLEND" },
	{ 0x0d, 1, 1, "3DSTATE_MODES_4" },
	{ 0x0c, 1, 1, "3DSTATE_MODES_5" },
	{ 0x07, 1, 1, "3DSTATE_RASTERIZATION_RULES" },
    }, *opcode_3d;

    opcode = (data[0] & 0x1f000000) >> 24;

    switch (opcode) {
    case 0x1f:
	return decode_3d_primitive(data, count, hw_offset, failures);
    case 0x1d:
	return decode_3d_1d(data, count, hw_offset, devid, failures);
    case 0x1c:
	return decode_3d_1c(data, count, hw_offset, failures);
    }

    for (idx = 0; idx < ARRAY_SIZE(opcodes_3d); idx++) {
	opcode_3d = &opcodes_3d[idx];
	if (opcode == opcode_3d->opcode) {
	    unsigned int len = 1, i;

	    instr_out(data, hw_offset, 0, "%s\n", opcode_3d->name);
	    if (opcode_3d->max_len > 1) {
		len = (data[0] & 0xff) + 2;
		if (len < opcode_3d->min_len ||
		    len > opcode_3d->max_len)
		{
		    fprintf(out, "Bad count in %s\n", opcode_3d->name);
		}
	    }

	    for (i = 1; i < len; i++) {
		if (i >= count)
		    BUFFER_FAIL(count, len, opcode_3d->name);
		instr_out(data, hw_offset, i, "dword %d\n", i);
	    }
	    return len;
	}
    }

    instr_out(data, hw_offset, 0, "3D UNKNOWN: 3d opcode = 0x%x\n", opcode);
    (*failures)++;
    return 1;
}

static const char *
get_965_surfacetype(unsigned int surfacetype)
{
    switch (surfacetype) {
    case 0: return "1D";
    case 1: return "2D";
    case 2: return "3D";
    case 3: return "CUBE";
    case 4: return "BUFFER";
    case 7: return "NULL";
    default: return "unknown";
    }
}

static const char *
get_965_depthformat(unsigned int depthformat)
{
    switch (depthformat) {
    case 0: return "s8_z24float";
    case 1: return "z32float";
    case 2: return "z24s8";
    case 5: return "z16";
    default: return "unknown";
    }
}

static const char *
get_965_element_component(uint32_t data, int component)
{
    uint32_t component_control = (data >> (16 + (3 - component) * 4)) & 0x7;

    switch (component_control) {
    case 0:
	return "nostore";
    case 1:
	switch (component) {
	case 0: return "X";
	case 1: return "Y";
	case 2: return "Z";
	case 3: return "W";
	default: return "fail";
	}
    case 2:
	return "0.0";
    case 3:
	return "1.0";
    case 4:
	return "0x1";
    case 5:
	return "VID";
    default:
	return "fail";
    }
}

static const char *
get_965_prim_type(uint32_t data)
{
    uint32_t primtype = (data >> 10) & 0x1f;

    switch (primtype) {
    case 0x01: return "point list";
    case 0x02: return "line list";
    case 0x03: return "line strip";
    case 0x04: return "tri list";
    case 0x05: return "tri strip";
    case 0x06: return "tri fan";
    case 0x07: return "quad list";
    case 0x08: return "quad strip";
    case 0x09: return "line list adj";
    case 0x0a: return "line strip adj";
    case 0x0b: return "tri list adj";
    case 0x0c: return "tri strip adj";
    case 0x0d: return "tri strip reverse";
    case 0x0e: return "polygon";
    case 0x0f: return "rect list";
    case 0x10: return "line loop";
    case 0x11: return "point list bf";
    case 0x12: return "line strip cont";
    case 0x13: return "line strip bf";
    case 0x14: return "line strip cont bf";
    case 0x15: return "tri fan no stipple";
    default: return "fail";
    }
}

static int
i965_decode_urb_fence(uint32_t *data, uint32_t hw_offset, int len, int count,
		      int *failures)
{
	uint32_t vs_fence, clip_fence, gs_fence, sf_fence, vfe_fence, cs_fence;

	if (len != 3)
	    fprintf(out, "Bad count in URB_FENCE\n");
	if (count < 3)
	    BUFFER_FAIL(count, len, "URB_FENCE");

	vs_fence = data[1] & 0x3ff;
	gs_fence = (data[1] >> 10) & 0x3ff;
	clip_fence = (data[1] >> 20) & 0x3ff;
	sf_fence = data[2] & 0x3ff;
	vfe_fence = (data[2] >> 10) & 0x3ff;
	cs_fence = (data[2] >> 20) & 0x7ff;

	instr_out(data, hw_offset, 0, "URB_FENCE: %s%s%s%s%s%s\n",
			(data[0] >> 13) & 1 ? "cs " : "",
			(data[0] >> 12) & 1 ? "vfe " : "",
			(data[0] >> 11) & 1 ? "sf " : "",
			(data[0] >> 10) & 1 ? "clip " : "",
			(data[0] >> 9)  & 1 ? "gs " : "",
			(data[0] >> 8)  & 1 ? "vs " : "");
	instr_out(data, hw_offset, 1,
		  "vs fence: %d, clip_fence: %d, gs_fence: %d\n",
		  vs_fence, clip_fence, gs_fence);
	instr_out(data, hw_offset, 2,
		  "sf fence: %d, vfe_fence: %d, cs_fence: %d\n",
		  sf_fence, vfe_fence, cs_fence);
	if (gs_fence < vs_fence)
	    fprintf(out, "gs fence < vs fence!\n");
	if (clip_fence < gs_fence)
	    fprintf(out, "clip fence < gs fence!\n");
	if (sf_fence < clip_fence)
	    fprintf(out, "sf fence < clip fence!\n");
	if (cs_fence < sf_fence)
	    fprintf(out, "cs fence < sf fence!\n");

	return len;
}

static void
state_base_out(uint32_t *data, uint32_t hw_offset, unsigned int index,
	       char *name)
{
    if (data[index] & 1) {
	instr_out(data, hw_offset, index, "%s state base address 0x%08x\n",
		  name, data[index] & ~1);
    } else {
	instr_out(data, hw_offset, index, "%s state base not updated\n",
		  name);
    }
}

static void
state_max_out(uint32_t *data, uint32_t hw_offset, unsigned int index,
	      char *name)
{
    if (data[index] & 1) {
	if (data[index] == 1) {
	    instr_out(data, hw_offset, index,
		      "%s state upper bound disabled\n", name);
	} else {
	    instr_out(data, hw_offset, index, "%s state upper bound 0x%08x\n",
		      name, data[index] & ~1);
	}
    } else {
	instr_out(data, hw_offset, index, "%s state upper bound not updated\n",
		  name);
    }
}

static int
decode_3d_965(uint32_t *data, int count, uint32_t hw_offset, uint32_t devid, int *failures)
{
    uint32_t opcode;
    unsigned int idx, len;
    int i, sba_len;
    char *desc1 = NULL;

    struct {
	uint32_t opcode;
	int min_len;
	int max_len;
	char *name;
    } opcodes_3d[] = {
	{ 0x6000, 3, 3, "URB_FENCE" },
	{ 0x6001, 2, 2, "CS_URB_STATE" },
	{ 0x6002, 2, 2, "CONSTANT_BUFFER" },
	{ 0x6101, 6, 6, "STATE_BASE_ADDRESS" },
	{ 0x6102, 2, 2 , "STATE_SIP" },
	{ 0x6104, 1, 1, "3DSTATE_PIPELINE_SELECT" },
	{ 0x680b, 1, 1, "3DSTATE_VF_STATISTICS" },
	{ 0x6904, 1, 1, "3DSTATE_PIPELINE_SELECT" },
	{ 0x7800, 7, 7, "3DSTATE_PIPELINED_POINTERS" },
	{ 0x7801, 6, 6, "3DSTATE_BINDING_TABLE_POINTERS" },
	{ 0x7808, 5, 257, "3DSTATE_VERTEX_BUFFERS" },
	{ 0x7809, 3, 256, "3DSTATE_VERTEX_ELEMENTS" },
	{ 0x780a, 3, 3, "3DSTATE_INDEX_BUFFER" },
	{ 0x780b, 1, 1, "3DSTATE_VF_STATISTICS" },
	{ 0x7900, 4, 4, "3DSTATE_DRAWING_RECTANGLE" },
	{ 0x7901, 5, 5, "3DSTATE_CONSTANT_COLOR" },
	{ 0x7905, 5, 7, "3DSTATE_DEPTH_BUFFER" },
	{ 0x7906, 2, 2, "3DSTATE_POLY_STIPPLE_OFFSET" },
	{ 0x7907, 33, 33, "3DSTATE_POLY_STIPPLE_PATTERN" },
	{ 0x7908, 3, 3, "3DSTATE_LINE_STIPPLE" },
	{ 0x7909, 2, 2, "3DSTATE_GLOBAL_DEPTH_OFFSET_CLAMP" },
	{ 0x7909, 2, 2, "3DSTATE_CLEAR_PARAMS" },
	{ 0x790a, 3, 3, "3DSTATE_AA_LINE_PARAMETERS" },
	{ 0x790b, 4, 4, "3DSTATE_GS_SVB_INDEX" },
	{ 0x790d, 3, 3, "3DSTATE_MULTISAMPLE" },
	{ 0x7910, 2, 2, "3DSTATE_CLEAR_PARAMS" },
	{ 0x7b00, 6, 6, "3DPRIMITIVE" },
	{ 0x7802, 4, 4, "3DSTATE_SAMPLER_STATE_POINTERS" },
	{ 0x7805, 3, 3, "3DSTATE_URB" },
	{ 0x780d, 4, 4, "3DSTATE_VIEWPORT_STATE_POINTERS" },
	{ 0x780e, 4, 4, "3DSTATE_CC_STATE_POINTERS" },
	{ 0x780f, 2, 2, "3DSTATE_SCISSOR_STATE_POINTERS" },
	{ 0x7810, 6, 6, "3DSTATE_VS_STATE" },
	{ 0x7811, 7, 7, "3DSTATE_GS_STATE" },
	{ 0x7812, 4, 4, "3DSTATE_CLIP_STATE" },
	{ 0x7813, 20, 20, "3DSTATE_SF_STATE" },
	{ 0x7814, 9, 9, "3DSTATE_WM_STATE" },
	{ 0x7815, 5, 5, "3DSTATE_CONSTANT_VS_STATE" },
	{ 0x7816, 5, 5, "3DSTATE_CONSTANT_GS_STATE" },
	{ 0x7817, 5, 5, "3DSTATE_CONSTANT_PS_STATE" },
	{ 0x7818, 2, 2, "3DSTATE_SAMPLE_MASK" },
   }, *opcode_3d;

    len = (data[0] & 0x0000ffff) + 2;

    opcode = (data[0] & 0xffff0000) >> 16;
    switch (opcode) {
    case 0x6000:
	len = (data[0] & 0x000000ff) + 2;
	return i965_decode_urb_fence(data, hw_offset, len, count, failures);
    case 0x6001:
	instr_out(data, hw_offset, 0, "CS_URB_STATE\n");
	instr_out(data, hw_offset, 1, "entry_size: %d [%d bytes], n_entries: %d\n",
			(data[1] >> 4) & 0x1f,
			(((data[1] >> 4) & 0x1f) + 1) * 64,
			data[1] & 0x7);
	return len;
    case 0x6002:
	len = (data[0] & 0x000000ff) + 2;
	instr_out(data, hw_offset, 0, "CONSTANT_BUFFER: %s\n",
			(data[0] >> 8) & 1 ? "valid" : "invalid");
	instr_out(data, hw_offset, 1, "offset: 0x%08x, length: %d bytes\n",
			data[1] & ~0x3f, ((data[1] & 0x3f) + 1) * 64);
	return len;
    case 0x6101:
	if (IS_GEN6(devid))
	    sba_len = 10;
	else if (IS_GEN5(devid))
	    sba_len = 8;
	else
	    sba_len = 6;
	if (len != sba_len)
	    fprintf(out, "Bad count in STATE_BASE_ADDRESS\n");
	if (len != sba_len)
	    BUFFER_FAIL(count, len, "STATE_BASE_ADDRESS");

	i = 0;
	instr_out(data, hw_offset, 0,
		  "STATE_BASE_ADDRESS\n");
	i++;

	state_base_out(data, hw_offset, i++, "general");
	state_base_out(data, hw_offset, i++, "surface");
	if (IS_GEN6(devid))
	    state_base_out(data, hw_offset, i++, "dynamic");
	state_base_out(data, hw_offset, i++, "indirect");
	if (IS_GEN5(devid) || IS_GEN6(devid))
	    state_base_out(data, hw_offset, i++, "instruction");

	state_max_out(data, hw_offset, i++, "general");
	if (IS_GEN6(devid))
	    state_max_out(data, hw_offset, i++, "dynamic");
	state_max_out(data, hw_offset, i++, "indirect");
	if (IS_GEN5(devid) || IS_GEN6(devid))
	    state_max_out(data, hw_offset, i++, "instruction");

	return len;
    case 0x7800:
	if (len != 7)
	    fprintf(out, "Bad count in 3DSTATE_PIPELINED_POINTERS\n");
	if (count < 7)
	    BUFFER_FAIL(count, len, "3DSTATE_PIPELINED_POINTERS");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_PIPELINED_POINTERS\n");
	instr_out(data, hw_offset, 1, "VS state\n");
	instr_out(data, hw_offset, 2, "GS state\n");
	instr_out(data, hw_offset, 3, "Clip state\n");
	instr_out(data, hw_offset, 4, "SF state\n");
	instr_out(data, hw_offset, 5, "WM state\n");
	instr_out(data, hw_offset, 6, "CC state\n");
	return len;
    case 0x7801:
	len = (data[0] & 0x000000ff) + 2;
	if (len != 6 && len != 4)
	    fprintf(out, "Bad count in 3DSTATE_BINDING_TABLE_POINTERS\n");
	if (len == 6) {
	    if (count < 6)
		BUFFER_FAIL(count, len, "3DSTATE_BINDING_TABLE_POINTERS");
	    instr_out(data, hw_offset, 0,
		      "3DSTATE_BINDING_TABLE_POINTERS\n");
	    instr_out(data, hw_offset, 1, "VS binding table\n");
	    instr_out(data, hw_offset, 2, "GS binding table\n");
	    instr_out(data, hw_offset, 3, "Clip binding table\n");
	    instr_out(data, hw_offset, 4, "SF binding table\n");
	    instr_out(data, hw_offset, 5, "WM binding table\n");
	} else {
	    if (count < 4)
		BUFFER_FAIL(count, len, "3DSTATE_BINDING_TABLE_POINTERS");

	    instr_out(data, hw_offset, 0,
		      "3DSTATE_BINDING_TABLE_POINTERS: VS mod %d, "
		      "GS mod %d, PS mod %d\n",
		      (data[0] & (1 << 8)) != 0,
		      (data[0] & (1 << 9)) != 0,
		      (data[0] & (1 << 12)) != 0);
	    instr_out(data, hw_offset, 1, "VS binding table\n");
	    instr_out(data, hw_offset, 2, "GS binding table\n");
	    instr_out(data, hw_offset, 3, "WM binding table\n");
	}

	return len;
    case 0x7802:
        len = (data[0] & 0xff) + 2;
        if (len != 4)
            fprintf(out, "Bad count in 3DSTATE_SAMPLER_STATE_POINTERS\n");
	if (count < 4)
	    BUFFER_FAIL(count, len, "3DSTATE_SAMPLER_STATE_POINTERS");
        instr_out(data, hw_offset, 0, "3DSTATE_SAMPLER_STATE_POINTERS: VS mod %d, "
                  "GS mod %d, PS mod %d\n",
                  (data[0] & (1 << 8)) != 0,
                  (data[0] & (1 << 9)) != 0,
                  (data[0] & (1 << 12)) != 0);
        instr_out(data, hw_offset, 1, "VS sampler state\n");
        instr_out(data, hw_offset, 2, "GS sampler state\n");
        instr_out(data, hw_offset, 3, "WM sampler state\n");
        return len;
    case 0x7805:
        len = (data[0] & 0xff) + 2;
        if (len != 3)
            fprintf(out, "Bad count in 3DSTATE_URB\n");
	if (count < 3)
	    BUFFER_FAIL(count, len, "3DSTATE_URB");
        instr_out(data, hw_offset, 0, "3DSTATE_URB\n");
        instr_out(data, hw_offset, 1, "VS entries %d, alloc size %d (1024bit row)\n",
                        data[1] & 0xffff, ((data[1] >> 16) & 0x07f) + 1);
        instr_out(data, hw_offset, 2, "GS entries %d, alloc size %d (1024bit row)\n",
                        (data[2] >> 8) & 0x3ff, (data[2] & 7) + 1);
        return len;

    case 0x7808:
	len = (data[0] & 0xff) + 2;
	if ((len - 1) % 4 != 0)
	    fprintf(out, "Bad count in 3DSTATE_VERTEX_BUFFERS\n");
	if (count < len)
	    BUFFER_FAIL(count, len, "3DSTATE_VERTEX_BUFFERS");
	instr_out(data, hw_offset, 0, "3DSTATE_VERTEX_BUFFERS\n");

	for (i = 1; i < len;) {
	    int idx, access;
	    if (IS_GEN6(devid)) {
                idx = 26;
                access = 20;
            } else {
                idx = 27;
                access = 26;
            }
	    instr_out(data, hw_offset, i, "buffer %d: %s, pitch %db\n",
		      data[i] >> idx,
		      data[i] & (1 << access) ? "random" : "sequential",
		      data[i] & 0x07ff);
	    i++;
	    instr_out(data, hw_offset, i++, "buffer address\n");
	    instr_out(data, hw_offset, i++, "max index\n");
	    instr_out(data, hw_offset, i++, "mbz\n");
	}
	return len;

    case 0x7809:
	len = (data[0] & 0xff) + 2;
	if ((len + 1) % 2 != 0)
	    fprintf(out, "Bad count in 3DSTATE_VERTEX_ELEMENTS\n");
	if (count < len)
	    BUFFER_FAIL(count, len, "3DSTATE_VERTEX_ELEMENTS");
	instr_out(data, hw_offset, 0, "3DSTATE_VERTEX_ELEMENTS\n");

	for (i = 1; i < len;) {
	    instr_out(data, hw_offset, i, "buffer %d: %svalid, type 0x%04x, "
		      "src offset 0x%04x bytes\n",
		      data[i] >> (IS_GEN6(devid) ? 26 : 27),
		      data[i] & (1 << (IS_GEN6(devid) ? 25 : 26)) ? "" : "in",
		      (data[i] >> 16) & 0x1ff,
		      data[i] & 0x07ff);
	    i++;
	    instr_out(data, hw_offset, i, "(%s, %s, %s, %s), "
		      "dst offset 0x%02x bytes\n",
		      get_965_element_component(data[i], 0),
		      get_965_element_component(data[i], 1),
		      get_965_element_component(data[i], 2),
		      get_965_element_component(data[i], 3),
		      (data[i] & 0xff) * 4);
	    i++;
	}
	return len;

    case 0x780d:
	len = (data[0] & 0xff) + 2;
	if (len != 4)
	    fprintf(out, "Bad count in 3DSTATE_VIEWPORT_STATE_POINTERS\n");
	if (count < len)
	    BUFFER_FAIL(count, len, "3DSTATE_VIEWPORT_STATE_POINTERS");
	instr_out(data, hw_offset, 0, "3DSTATE_VIEWPORT_STATE_POINTERS\n");
	instr_out(data, hw_offset, 1, "clip\n");
	instr_out(data, hw_offset, 2, "sf\n");
	instr_out(data, hw_offset, 3, "cc\n");
	return len;

    case 0x780a:
	len = (data[0] & 0xff) + 2;
	if (len != 3)
	    fprintf(out, "Bad count in 3DSTATE_INDEX_BUFFER\n");
	if (count < len)
	    BUFFER_FAIL(count, len, "3DSTATE_INDEX_BUFFER");
	instr_out(data, hw_offset, 0, "3DSTATE_INDEX_BUFFER\n");
	instr_out(data, hw_offset, 1, "beginning buffer address\n");
	instr_out(data, hw_offset, 2, "ending buffer address\n");
	return len;

    case 0x780e:
        len = (data[0] & 0xff) + 2;
        if (len != 4)
            fprintf(out, "Bad count in 3DSTATE_CC_STATE_POINTERS\n");
	if (count < 4)
	    BUFFER_FAIL(count, len, "3DSTATE_CC_STATE_POINTERS");
        instr_out(data, hw_offset, 0, "3DSTATE_CC_STATE_POINTERS\n");
        instr_out(data, hw_offset, 1, "blend change %d\n", data[1] & 1);
        instr_out(data, hw_offset, 2, "depth stencil change %d\n", data[2] & 1);
        instr_out(data, hw_offset, 3, "cc change %d\n", data[3] & 1);
        return len;

    case 0x780f:
        len = (data[0] & 0xff) + 2;
        if (len != 2)
            fprintf(out, "Bad count in 3DSTATE_SCISSOR_POINTERS\n");
	if (count < 2)
	    BUFFER_FAIL(count, len, "3DSTATE_SCISSOR_POINTERS");
        instr_out(data, hw_offset, 0, "3DSTATE_SCISSOR_POINTERS\n");
        instr_out(data, hw_offset, 1, "scissor rect offset\n");
        return len;

    case 0x7810:
        len = (data[0] & 0xff) + 2;
        if (len != 6)
            fprintf(out, "Bad count in 3DSTATE_VS\n");
	if (count < 6)
	    BUFFER_FAIL(count, len, "3DSTATE_VS");
        instr_out(data, hw_offset, 0, "3DSTATE_VS\n");
        instr_out(data, hw_offset, 1, "kernel pointer\n");
        instr_out(data, hw_offset, 2, "SPF=%d, VME=%d, Sampler Count %d, "
                  "Binding table count %d\n",
                  (data[2] >> 31) & 1,
                  (data[2] >> 30) & 1,
                  (data[2] >> 27) & 7,
                  (data[2] >> 18) & 0xff);
        instr_out(data, hw_offset, 3, "scratch offset\n");
        instr_out(data, hw_offset, 4, "Dispatch GRF start %d, VUE read length %d, "
                  "VUE read offset %d\n",
                  (data[4] >> 20) & 0x1f,
                  (data[4] >> 11) & 0x3f,
                  (data[4] >> 4) & 0x3f);
        instr_out(data, hw_offset, 5, "Max Threads %d, Vertex Cache %sable, "
                  "VS func %sable\n",
                  ((data[5] >> 25) & 0x7f) + 1,
                  (data[5] & (1 << 1)) != 0 ? "dis" : "en",
                  (data[5] & 1) != 0 ? "en" : "dis");
        return len;

    case 0x7811:
        len = (data[0] & 0xff) + 2;
        if (len != 7)
            fprintf(out, "Bad count in 3DSTATE_GS\n");
	if (count < 7)
	    BUFFER_FAIL(count, len, "3DSTATE_GS");
        instr_out(data, hw_offset, 0, "3DSTATE_GS\n");
        instr_out(data, hw_offset, 1, "kernel pointer\n");
        instr_out(data, hw_offset, 2, "SPF=%d, VME=%d, Sampler Count %d, "
                  "Binding table count %d\n",
                  (data[2] >> 31) & 1,
                  (data[2] >> 30) & 1,
                  (data[2] >> 27) & 7,
                  (data[2] >> 18) & 0xff);
        instr_out(data, hw_offset, 3, "scratch offset\n");
        instr_out(data, hw_offset, 4, "Dispatch GRF start %d, VUE read length %d, "
                  "VUE read offset %d\n",
                  (data[4] & 0xf),
                  (data[4] >> 11) & 0x3f,
                  (data[4] >> 4) & 0x3f);
        instr_out(data, hw_offset, 5, "Max Threads %d, Rendering %sable\n",
                  ((data[5] >> 25) & 0x7f) + 1,
                  (data[5] & (1 << 8)) != 0 ? "en" : "dis");
        instr_out(data, hw_offset, 6, "Reorder %sable, Discard Adjaceny %sable, "
                  "GS %sable\n",
                  (data[6] & (1 << 30)) != 0 ? "en" : "dis",
                  (data[6] & (1 << 29)) != 0 ? "en" : "dis",
                  (data[6] & (1 << 15)) != 0 ? "en" : "dis");
        return len;

    case 0x7812:
        len = (data[0] & 0xff) + 2;
        if (len != 4)
            fprintf(out, "Bad count in 3DSTATE_CLIP\n");
	if (count < 4)
	    BUFFER_FAIL(count, len, "3DSTATE_CLIP");
        instr_out(data, hw_offset, 0, "3DSTATE_CLIP\n");
        instr_out(data, hw_offset, 1, "UserClip distance cull test mask 0x%x\n",
                  data[1] & 0xff);
        instr_out(data, hw_offset, 2, "Clip %sable, API mode %s, Viewport XY test %sable, "
                  "Viewport Z test %sable, Guardband test %sable, Clip mode %d, "
                  "Perspective Divide %sable, Non-Perspective Barycentric %sable, "
                  "Tri Provoking %d, Line Provoking %d, Trifan Provoking %d\n",
                  (data[2] & (1 << 31)) != 0 ? "en" : "dis",
                  (data[2] & (1 << 30)) != 0 ? "D3D" : "OGL",
                  (data[2] & (1 << 28)) != 0 ? "en" : "dis",
                  (data[2] & (1 << 27)) != 0 ? "en" : "dis",
                  (data[2] & (1 << 26)) != 0 ? "en" : "dis",
                  (data[2] >> 13) & 7,
                  (data[2] & (1 << 9)) != 0 ? "dis" : "en",
                  (data[2] & (1 << 8)) != 0 ? "en" : "dis",
                  (data[2] >> 4) & 3,
                  (data[2] >> 2) & 3,
                  (data[2] & 3));
        instr_out(data, hw_offset, 3, "Min PointWidth %d, Max PointWidth %d, "
                  "Force Zero RTAIndex %sable, Max VPIndex %d\n",
                  (data[3] >> 17) & 0x7ff,
                  (data[3] >> 6) & 0x7ff,
                  (data[3] & (1 << 5)) != 0 ? "en" : "dis",
                  (data[3] & 0xf));
        return len;

    case 0x7813:
        len = (data[0] & 0xff) + 2;
        if (len != 20)
            fprintf(out, "Bad count in 3DSTATE_SF\n");
	if (count < 20)
	    BUFFER_FAIL(count, len, "3DSTATE_SF");
        instr_out(data, hw_offset, 0, "3DSTATE_SF\n");
        instr_out(data, hw_offset, 1, "Attrib Out %d, Attrib Swizzle %sable, VUE read length %d, "
                  "VUE read offset %d\n",
                  (data[1] >> 22) & 0x3f,
                  (data[1] & (1 << 21)) != 0 ? "en" : "dis",
                  (data[1] >> 11) & 0x1f,
                  (data[1] >> 4) & 0x3f);
        instr_out(data, hw_offset, 2, "Legacy Global DepthBias %sable, FrontFace fill %d, BF fill %d, "
                  "VP transform %sable, FrontWinding_%s\n",
                  (data[2] & (1 << 11)) != 0 ? "en" : "dis",
                  (data[2] >> 5) & 3,
                  (data[2] >> 3) & 3,
                  (data[2] & (1 << 1)) != 0 ? "en" : "dis",
                  (data[2] & 1) != 0 ? "CCW" : "CW");
        instr_out(data, hw_offset, 3, "AA %sable, CullMode %d, Scissor %sable, Multisample m ode %d\n",
                  (data[3] & (1 << 31)) != 0 ? "en" : "dis",
                  (data[3] >> 29) & 3,
                  (data[3] & (1 << 11)) != 0 ? "en" : "dis",
                  (data[3] >> 8) & 3);
        instr_out(data, hw_offset, 4, "Last Pixel %sable, SubPixel Precision %d, Use PixelWidth %d\n",
                  (data[4] & (1 << 31)) != 0 ? "en" : "dis",
                  (data[4] & (1 << 12)) != 0 ? 4 : 8,
                  (data[4] & (1 << 11)) != 0);
        instr_out(data, hw_offset, 5, "Global Depth Offset Constant %f\n", data[5]);
        instr_out(data, hw_offset, 6, "Global Depth Offset Scale %f\n", data[6]);
        instr_out(data, hw_offset, 7, "Global Depth Offset Clamp %f\n", data[7]);
        int i, j;
        for (i = 0, j = 0; i < 8; i++, j+=2)
            instr_out(data, hw_offset, i+8, "Attrib %d (Override %s%s%s%s, Const Source %d, Swizzle Select %d, "
                  "Source %d); Attrib %d (Override %s%s%s%s, Const Source %d, Swizzle Select %d, Source %d)\n",
                  j+1,
                  (data[8+i] & (1 << 31)) != 0 ? "W":"",
                  (data[8+i] & (1 << 30)) != 0 ? "Z":"",
                  (data[8+i] & (1 << 29)) != 0 ? "Y":"",
                  (data[8+i] & (1 << 28)) != 0 ? "X":"",
                  (data[8+i] >> 25) & 3, (data[8+i] >> 22) & 3,
                  (data[8+i] >> 16) & 0x1f,
                  j,
                  (data[8+i] & (1 << 15)) != 0 ? "W":"",
                  (data[8+i] & (1 << 14)) != 0 ? "Z":"",
                  (data[8+i] & (1 << 13)) != 0 ? "Y":"",
                  (data[8+i] & (1 << 12)) != 0 ? "X":"",
                  (data[8+i] >> 9) & 3, (data[8+i] >> 6) & 3,
                  (data[8+i] & 0x1f));
	instr_out(data, hw_offset, 16, "Point Sprite TexCoord Enable\n");
        instr_out(data, hw_offset, 17, "Const Interp Enable\n");
        instr_out(data, hw_offset, 18, "Attrib 7-0 WrapShortest Enable\n");
        instr_out(data, hw_offset, 19, "Attrib 15-8 WrapShortest Enable\n");

        return len;

    case 0x7814:
        len = (data[0] & 0xff) + 2;
        if (len != 9)
            fprintf(out, "Bad count in 3DSTATE_WM\n");
	if (count < 9)
	    BUFFER_FAIL(count, len, "3DSTATE_WM");
        instr_out(data, hw_offset, 0, "3DSTATE_WM\n");
        instr_out(data, hw_offset, 1, "kernel start pointer 0\n");
        instr_out(data, hw_offset, 2, "SPF=%d, VME=%d, Sampler Count %d, "
                  "Binding table count %d\n",
                  (data[2] >> 31) & 1,
                  (data[2] >> 30) & 1,
                  (data[2] >> 27) & 7,
                  (data[2] >> 18) & 0xff);
        instr_out(data, hw_offset, 3, "scratch offset\n");
        instr_out(data, hw_offset, 4, "Depth Clear %d, Depth Resolve %d, HiZ Resolve %d, "
                  "Dispatch GRF start[0] %d, start[1] %d, start[2] %d\n",
                  (data[4] & (1 << 30)) != 0,
                  (data[4] & (1 << 28)) != 0,
                  (data[4] & (1 << 27)) != 0,
                  (data[4] >> 16) & 0x7f,
                  (data[4] >> 8) & 0x7f,
                  (data[4] & 0x7f));
        instr_out(data, hw_offset, 5, "MaxThreads %d, PS KillPixel %d, PS computed Z %d, "
                  "PS use sourceZ %d, Thread Dispatch %d, PS use sourceW %d, Dispatch32 %d, "
                  "Dispatch16 %d, Dispatch8 %d\n",
                  ((data[5] >> 25) & 0x7f) + 1,
                  (data[5] & (1 << 22)) != 0,
                  (data[5] & (1 << 21)) != 0,
                  (data[5] & (1 << 20)) != 0,
                  (data[5] & (1 << 19)) != 0,
                  (data[5] & (1 << 8)) != 0,
                  (data[5] & (1 << 2)) != 0,
                  (data[5] & (1 << 1)) != 0,
                  (data[5] & (1 << 0)) != 0);
        instr_out(data, hw_offset, 6, "Num SF output %d, Pos XY offset %d, ZW interp mode %d , "
                  "Barycentric interp mode 0x%x, Point raster rule %d, Multisample mode %d, "
                  "Multisample Dispatch mode %d\n",
                  (data[6] >> 20) & 0x3f,
                  (data[6] >> 18) & 3,
                  (data[6] >> 16) & 3,
                  (data[6] >> 10) & 0x3f,
                  (data[6] & (1 << 9)) != 0,
                  (data[6] >> 1) & 3,
                  (data[6] & 1));
        instr_out(data, hw_offset, 7, "kernel start pointer 1\n");
        instr_out(data, hw_offset, 8, "kernel start pointer 2\n");

        return len;

    case 0x7900:
	if (len != 4)
	    fprintf(out, "Bad count in 3DSTATE_DRAWING_RECTANGLE\n");
	if (count < 4)
	    BUFFER_FAIL(count, len, "3DSTATE_DRAWING_RECTANGLE");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_DRAWING_RECTANGLE\n");
	instr_out(data, hw_offset, 1, "top left: %d,%d\n",
		  data[1] & 0xffff,
		  (data[1] >> 16) & 0xffff);
	instr_out(data, hw_offset, 2, "bottom right: %d,%d\n",
		  data[2] & 0xffff,
		  (data[2] >> 16) & 0xffff);
	instr_out(data, hw_offset, 3, "origin: %d,%d\n",
		  (int)data[3] & 0xffff,
		  ((int)data[3] >> 16) & 0xffff);

	return len;

    case 0x7905:
	if (len < 5 || len > 7)
	    fprintf(out, "Bad count in 3DSTATE_DEPTH_BUFFER\n");
	if (count < len)
	    BUFFER_FAIL(count, len, "3DSTATE_DEPTH_BUFFER");

	instr_out(data, hw_offset, 0,
		  "3DSTATE_DEPTH_BUFFER\n");
	if (IS_GEN5(devid) || IS_GEN6(devid))
            instr_out(data, hw_offset, 1, "%s, %s, pitch = %d bytes, %stiled, HiZ %d, Seperate Stencil %d\n",
                    get_965_surfacetype(data[1] >> 29),
                    get_965_depthformat((data[1] >> 18) & 0x7),
                    (data[1] & 0x0001ffff) + 1,
                    data[1] & (1 << 27) ? "" : "not ",
                    (data[1] & (1 << 22)) != 0,
                    (data[1] & (1 << 21)) != 0);
        else
            instr_out(data, hw_offset, 1, "%s, %s, pitch = %d bytes, %stiled\n",
                    get_965_surfacetype(data[1] >> 29),
                    get_965_depthformat((data[1] >> 18) & 0x7),
                    (data[1] & 0x0001ffff) + 1,
                    data[1] & (1 << 27) ? "" : "not ");
	instr_out(data, hw_offset, 2, "depth offset\n");
	instr_out(data, hw_offset, 3, "%dx%d\n",
		  ((data[3] & 0x0007ffc0) >> 6) + 1,
		  ((data[3] & 0xfff80000) >> 19) + 1);
	instr_out(data, hw_offset, 4, "volume depth\n");
	if (len >= 6)
	    instr_out(data, hw_offset, 5, "\n");
	if (len >= 7) {
            if (IS_GEN6(devid))
                instr_out(data, hw_offset, 6, "\n");
            else
                instr_out(data, hw_offset, 6, "render target view extent\n");
        }


	return len;

    case 0x7a00:
	if (IS_GEN6(devid)) {
		int i;
		len = (data[0] & 0xff) + 2;
		if (len != 4 && len != 5)
			fprintf(out, "Bad count in PIPE_CONTROL\n");
		if (count < len)
			BUFFER_FAIL(count, len, "PIPE_CONTROL");

		switch ((data[1] >> 14) & 0x3) {
		case 0: desc1 = "no write"; break;
		case 1: desc1 = "qword write"; break;
		case 2: desc1 = "PS_DEPTH_COUNT write"; break;
		case 3: desc1 = "TIMESTAMP write"; break;
		}
		instr_out(data, hw_offset, 0, "PIPE_CONTROL\n");
		instr_out(data, hw_offset, 1,
			  "%s, %scs stall, %stlb invalidate, "
			  "%ssync gfdt, %sdepth stall, %sRC write flush, "
			  "%sinst flush, %sTC flush\n",
			  desc1,
			  data[1] & (1 << 20) ? "" : "no ",
			  data[1] & (1 << 18) ? "" : "no ",
			  data[1] & (1 << 17) ? "" : "no ",
			  data[1] & (1 << 13) ? "" : "no ",
			  data[1] & (1 << 12) ? "" : "no ",
			  data[1] & (1 << 11) ? "" : "no ",
			  data[1] & (1 << 10) ? "" : "no ");
		if (len == 5) {
		    instr_out(data, hw_offset, 2, "destination address\n");
		    instr_out(data, hw_offset, 3, "immediate dword low\n");
		    instr_out(data, hw_offset, 4, "immediate dword high\n");
		} else {
		    for (i = 2; i < len; i++) {
			instr_out(data, hw_offset, i, "\n");
		    }
		}
		return len;
	} else {
		len = (data[0] & 0xff) + 2;
		if (len != 4)
			fprintf(out, "Bad count in PIPE_CONTROL\n");
		if (count < len)
			BUFFER_FAIL(count, len, "PIPE_CONTROL");

		switch ((data[0] >> 14) & 0x3) {
		case 0: desc1 = "no write"; break;
		case 1: desc1 = "qword write"; break;
		case 2: desc1 = "PS_DEPTH_COUNT write"; break;
		case 3: desc1 = "TIMESTAMP write"; break;
		}
		instr_out(data, hw_offset, 0,
			  "PIPE_CONTROL: %s, %sdepth stall, %sRC write flush, "
			  "%sinst flush\n",
			  desc1,
			  data[0] & (1 << 13) ? "" : "no ",
			  data[0] & (1 << 12) ? "" : "no ",
			  data[0] & (1 << 11) ? "" : "no ");
		instr_out(data, hw_offset, 1, "destination address\n");
		instr_out(data, hw_offset, 2, "immediate dword low\n");
		instr_out(data, hw_offset, 3, "immediate dword high\n");
		return len;
	}
    case 0x7b00:
	len = (data[0] & 0xff) + 2;
	if (len != 6)
	    fprintf(out, "Bad count in 3DPRIMITIVE\n");
	if (count < len)
	    BUFFER_FAIL(count, len, "3DPRIMITIVE");

	instr_out(data, hw_offset, 0,
		  "3DPRIMITIVE: %s %s\n",
		  get_965_prim_type(data[0]),
		  (data[0] & (1 << 15)) ? "random" : "sequential");
	instr_out(data, hw_offset, 1, "vertex count\n");
	instr_out(data, hw_offset, 2, "start vertex\n");
	instr_out(data, hw_offset, 3, "instance count\n");
	instr_out(data, hw_offset, 4, "start instance\n");
	instr_out(data, hw_offset, 5, "index bias\n");
	return len;
    }

    for (idx = 0; idx < ARRAY_SIZE(opcodes_3d); idx++) {
	opcode_3d = &opcodes_3d[idx];
	if ((data[0] & 0xffff0000) >> 16 == opcode_3d->opcode) {
	    unsigned int i;
	    len = 1;

	    instr_out(data, hw_offset, 0, "%s\n", opcode_3d->name);
	    if (opcode_3d->max_len > 1) {
		len = (data[0] & 0xff) + 2;
		if (len < opcode_3d->min_len ||
		    len > opcode_3d->max_len)
		{
		    fprintf(out, "Bad count in %s\n", opcode_3d->name);
		}
	    }

	    for (i = 1; i < len; i++) {
		if (i >= count)
		    BUFFER_FAIL(count, len, opcode_3d->name);
		instr_out(data, hw_offset, i, "dword %d\n", i);
	    }
	    return len;
	}
    }

    instr_out(data, hw_offset, 0, "3D UNKNOWN: 3d_965 opcode = 0x%x\n", opcode);
    (*failures)++;
    return 1;
}

static int
decode_3d_i830(uint32_t *data, int count, uint32_t hw_offset, uint32_t devid, int *failures)
{
    unsigned int idx;
    uint32_t opcode;

    struct {
	uint32_t opcode;
	int min_len;
	int max_len;
	char *name;
    } opcodes_3d[] = {
	{ 0x02, 1, 1, "3DSTATE_MODES_3" },
	{ 0x03, 1, 1, "3DSTATE_ENABLES_1"},
	{ 0x04, 1, 1, "3DSTATE_ENABLES_2"},
	{ 0x05, 1, 1, "3DSTATE_VFT0"},
	{ 0x06, 1, 1, "3DSTATE_AA"},
	{ 0x07, 1, 1, "3DSTATE_RASTERIZATION_RULES" },
	{ 0x08, 1, 1, "3DSTATE_MODES_1" },
	{ 0x09, 1, 1, "3DSTATE_STENCIL_TEST" },
	{ 0x0a, 1, 1, "3DSTATE_VFT1"},
	{ 0x0b, 1, 1, "3DSTATE_INDPT_ALPHA_BLEND" },
	{ 0x0c, 1, 1, "3DSTATE_MODES_5" },
	{ 0x0d, 1, 1, "3DSTATE_MAP_BLEND_OP" },
	{ 0x0e, 1, 1, "3DSTATE_MAP_BLEND_ARG" },
	{ 0x0f, 1, 1, "3DSTATE_MODES_2" },
	{ 0x15, 1, 1, "3DSTATE_FOG_COLOR" },
	{ 0x16, 1, 1, "3DSTATE_MODES_4" },
    }, *opcode_3d;

    opcode = (data[0] & 0x1f000000) >> 24;

    switch (opcode) {
    case 0x1f:
	return decode_3d_primitive(data, count, hw_offset, failures);
    case 0x1d:
	return decode_3d_1d(data, count, hw_offset, devid, failures);
    case 0x1c:
	return decode_3d_1c(data, count, hw_offset, failures);
    }

    for (idx = 0; idx < ARRAY_SIZE(opcodes_3d); idx++) {
	opcode_3d = &opcodes_3d[idx];
	if ((data[0] & 0x1f000000) >> 24 == opcode_3d->opcode) {
	    unsigned int len = 1, i;

	    instr_out(data, hw_offset, 0, "%s\n", opcode_3d->name);
	    if (opcode_3d->max_len > 1) {
		len = (data[0] & 0xff) + 2;
		if (len < opcode_3d->min_len ||
		    len > opcode_3d->max_len)
		{
		    fprintf(out, "Bad count in %s\n", opcode_3d->name);
		}
	    }

	    for (i = 1; i < len; i++) {
		if (i >= count)
		    BUFFER_FAIL(count, len, opcode_3d->name);
		instr_out(data, hw_offset, i, "dword %d\n", i);
	    }
	    return len;
	}
    }

    instr_out(data, hw_offset, 0, "3D UNKNOWN: 3d_i830 opcode = 0x%x\n", opcode);
    (*failures)++;
    return 1;
}

/**
 * Decodes an i830-i915 batch buffer, writing the output to stdout.
 *
 * \param data batch buffer contents
 * \param count number of DWORDs to decode in the batch buffer
 * \param hw_offset hardware address for the buffer
 */
int
intel_decode(uint32_t *data, int count,
	     uint32_t hw_offset,
	     uint32_t devid,
	     uint32_t ignore_end_of_batchbuffer)
{
    int ret;
    int index = 0;
    int failures = 0;

    out = stdout;

    while (index < count) {
	switch ((data[index] & 0xe0000000) >> 29) {
	case 0x0:
	    ret = decode_mi(data + index, count - index,
			       hw_offset + index * 4, &failures);

	    /* If MI_BATCHBUFFER_END happened, then dump the rest of the
	     * output in case we some day want it in debugging, but don't
	     * decode it since it'll just confuse in the common case.
	     */
	    if (ret == -1) {
		if (ignore_end_of_batchbuffer) {
		    index++;
		} else {
		    for (index = index + 1; index < count; index++) {
			instr_out(data, hw_offset, index, "\n");
		    }
		}
	    } else
		index += ret;
	    break;
	case 0x2:
	    index += decode_2d(data + index, count - index,
			       hw_offset + index * 4, &failures);
	    break;
	case 0x3:
	    if (IS_965(devid)) {
		index += decode_3d_965(data + index, count - index,
				       hw_offset + index * 4,
				       devid, &failures);
	    } else if (IS_GEN3(devid)) {
		index += decode_3d(data + index, count - index,
				   hw_offset + index * 4,
				   devid, &failures);
	    } else {
		index += decode_3d_i830(data + index, count - index,
					hw_offset + index * 4,
					devid, &failures);
	    }
	    break;
	default:
	    instr_out(data, hw_offset, index, "UNKNOWN\n");
	    failures++;
	    index++;
	    break;
	}
	fflush(out);
    }

    return failures;
}

void intel_decode_context_reset(void)
{
    saved_s2_set = 0;
    saved_s4_set = 1;
}

void intel_decode_context_set_head_tail(uint32_t head, uint32_t tail)
{
	head_offset = head;
	tail_offset = tail;
}
