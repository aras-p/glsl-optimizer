/*
 * Copyright 2010 Jerome Glisse <glisse@freedesktop.org>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHOR(S) AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <tgsi/tgsi_parse.h>
#include <tgsi/tgsi_scan.h>
#include "r600_shader.h"
#include "r600_context.h"

struct tgsi_shader {
	struct c_vector				**v[TGSI_FILE_COUNT];
	struct tgsi_shader_info			info;
	struct tgsi_parse_context		parser;
	const struct tgsi_token			*tokens;
	struct c_shader				*shader;
	struct c_node				*node;
};

static unsigned tgsi_file_to_c_file(unsigned file);
static unsigned tgsi_sname_to_c_sname(unsigned sname);
static int tgsi_opcode_to_c_opcode(unsigned opcode, unsigned *copcode);

static int tgsi_shader_init(struct tgsi_shader *ts,
				const struct tgsi_token *tokens,
				struct c_shader *shader)
{
	int i;

	ts->shader = shader;
	ts->tokens = tokens;
	tgsi_scan_shader(ts->tokens, &ts->info);
	tgsi_parse_init(&ts->parser, ts->tokens);
	/* initialize to NULL in case of error */
	for (i = 0; i < C_FILE_COUNT; i++) {
		ts->v[i] = NULL;
	}
	for (i = 0; i < TGSI_FILE_COUNT; i++) {
		if (ts->info.file_count[i] > 0) {
			ts->v[i] = calloc(ts->info.file_count[i], sizeof(void*));
			if (ts->v[i] == NULL) {
				fprintf(stderr, "%s:%d unsupported %d %d\n", __func__, __LINE__, i, ts->info.file_count[i]);
				return -ENOMEM;
			}
		}
	}
	return 0;
}

static void tgsi_shader_destroy(struct tgsi_shader *ts)
{
	int i;

	for (i = 0; i < TGSI_FILE_COUNT; i++) {
		free(ts->v[i]);
	}
	tgsi_parse_free(&ts->parser);
}

static int ntransform_declaration(struct tgsi_shader *ts)
{
	struct tgsi_full_declaration *fd = &ts->parser.FullToken.FullDeclaration;
	struct c_vector *v;
	unsigned file;
	unsigned name;
	int sid;
	int i;

	if (fd->Declaration.Dimension) {
		fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	for (i = fd->Range.First ; i <= fd->Range.Last; i++) {
		sid = i;
		name = C_SEMANTIC_GENERIC;
		file = tgsi_file_to_c_file(fd->Declaration.File);
		if (file == TGSI_FILE_NULL) {
			fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
			return -EINVAL;
		}
		if (fd->Declaration.Semantic) {
			name = tgsi_sname_to_c_sname(fd->Semantic.Name);
			sid = fd->Semantic.Index;
		}
		v = c_shader_vector_new(ts->shader, file, name, sid);
		if (v == NULL) {
			fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
			return -ENOMEM;
		}
		ts->v[fd->Declaration.File][i] = v;
	}
	return 0;
}

static int ntransform_immediate(struct tgsi_shader *ts)
{
	struct tgsi_full_immediate *fd = &ts->parser.FullToken.FullImmediate;
	struct c_vector *v;
	unsigned file;
	unsigned name;

	if (fd->Immediate.DataType != TGSI_IMM_FLOAT32) {
		fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	name = C_SEMANTIC_GENERIC;
	file = C_FILE_IMMEDIATE;
	v = c_shader_vector_new(ts->shader, file, name, 0);
	if (v == NULL) {
		fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
			return -ENOMEM;
	}
	v->channel[0]->value = fd->u[0].Uint;
	v->channel[1]->value = fd->u[1].Uint;
	v->channel[2]->value = fd->u[2].Uint;
	v->channel[3]->value = fd->u[3].Uint;
	ts->v[TGSI_FILE_IMMEDIATE][0] = v;
	return 0;
}

static int ntransform_instruction(struct tgsi_shader *ts)
{
	struct tgsi_full_instruction *fi = &ts->parser.FullToken.FullInstruction;
	struct c_shader *shader = ts->shader;
	struct c_instruction instruction;
	unsigned opcode;
	int i, r;

	if (fi->Instruction.NumDstRegs > 1) {
		fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (fi->Instruction.Saturate) {
		fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (fi->Instruction.Predicate) {
		fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (fi->Instruction.Label) {
		fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	if (fi->Instruction.Texture) {
		fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
		return -EINVAL;
	}
	for (i = 0; i < fi->Instruction.NumSrcRegs; i++) {
		if (fi->Src[i].Register.Indirect ||
			fi->Src[i].Register.Dimension ||
			fi->Src[i].Register.Absolute) {
			fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
			return -EINVAL;
		}
	}
	for (i = 0; i < fi->Instruction.NumDstRegs; i++) {
		if (fi->Dst[i].Register.Indirect || fi->Dst[i].Register.Dimension) {
			fprintf(stderr, "%s %d unsupported\n", __func__, __LINE__);
			return -EINVAL;
		}
	}
	r = tgsi_opcode_to_c_opcode(fi->Instruction.Opcode, &opcode);
	if (r) {
		fprintf(stderr, "%s:%d unsupported\n", __func__, __LINE__);
		return r;
	}
	if (opcode == C_OPCODE_END) {
		return c_node_cfg_link(ts->node, &shader->end);
	}
	/* FIXME add flow instruction handling */
	memset(&instruction, 0, sizeof(struct c_instruction));
	instruction.opcode = opcode;
	instruction.ninput = fi->Instruction.NumSrcRegs;
	instruction.write_mask = fi->Dst[0].Register.WriteMask;
	for (i = 0; i < fi->Instruction.NumSrcRegs; i++) {
		instruction.input[i].vector = ts->v[fi->Src[i].Register.File][fi->Src[i].Register.Index];
		instruction.input[i].swizzle[0] = fi->Src[i].Register.SwizzleX;
		instruction.input[i].swizzle[1] = fi->Src[i].Register.SwizzleY;
		instruction.input[i].swizzle[2] = fi->Src[i].Register.SwizzleZ;
		instruction.input[i].swizzle[3] = fi->Src[i].Register.SwizzleW;
	}
	instruction.output.vector = ts->v[fi->Dst[0].Register.File][fi->Dst[0].Register.Index];
	instruction.output.swizzle[0] = (fi->Dst[0].Register.WriteMask & 0x1) ? C_SWIZZLE_X : C_SWIZZLE_D;
	instruction.output.swizzle[1] = (fi->Dst[0].Register.WriteMask & 0x2) ? C_SWIZZLE_Y : C_SWIZZLE_D;
	instruction.output.swizzle[2] = (fi->Dst[0].Register.WriteMask & 0x4) ? C_SWIZZLE_Z : C_SWIZZLE_D;
	instruction.output.swizzle[3] = (fi->Dst[0].Register.WriteMask & 0x8) ? C_SWIZZLE_W : C_SWIZZLE_D;
	return c_node_add_new_instruction(ts->node, &instruction);
}

int c_shader_from_tgsi(struct c_shader *shader, unsigned type,
			const struct tgsi_token *tokens)
{
	struct tgsi_shader ts;
	int r = 0;

	c_shader_init(shader, type);
	r = tgsi_shader_init(&ts, tokens, shader);
	if (r)
		goto out_err;
	ts.shader = shader;
	ts.node = &shader->entry;
	while (!tgsi_parse_end_of_tokens(&ts.parser)) {
		tgsi_parse_token(&ts.parser);
		switch (ts.parser.FullToken.Token.Type) {
		case TGSI_TOKEN_TYPE_IMMEDIATE:
			r = ntransform_immediate(&ts);
			if (r)
				goto out_err;
			break;
		case TGSI_TOKEN_TYPE_DECLARATION:
			r = ntransform_declaration(&ts);
			if (r)
				goto out_err;
			break;
		case TGSI_TOKEN_TYPE_INSTRUCTION:
			r = ntransform_instruction(&ts);
			if (r)
				goto out_err;
			break;
		default:
			r = -EINVAL;
			goto out_err;
		}
	}
	tgsi_shader_destroy(&ts);
	return 0;
out_err:
	c_shader_destroy(shader);
	tgsi_shader_destroy(&ts);
	return r;
}

static unsigned tgsi_file_to_c_file(unsigned file)
{
	switch (file) {
	case TGSI_FILE_CONSTANT:
		return C_FILE_CONSTANT;
	case TGSI_FILE_INPUT:
		return C_FILE_INPUT;
	case TGSI_FILE_OUTPUT:
		return C_FILE_OUTPUT;
	case TGSI_FILE_TEMPORARY:
		return C_FILE_TEMPORARY;
	case TGSI_FILE_SAMPLER:
		return C_FILE_SAMPLER;
	case TGSI_FILE_ADDRESS:
		return C_FILE_ADDRESS;
	case TGSI_FILE_IMMEDIATE:
		return C_FILE_IMMEDIATE;
	case TGSI_FILE_PREDICATE:
		return C_FILE_PREDICATE;
	case TGSI_FILE_SYSTEM_VALUE:
		return C_FILE_SYSTEM_VALUE;
	case TGSI_FILE_NULL:
		return C_FILE_NULL;
	default:
		fprintf(stderr, "%s:%d unsupported file %d\n", __func__, __LINE__, file);
		return C_FILE_NULL;
	}
}

static unsigned tgsi_sname_to_c_sname(unsigned sname)
{
	switch (sname) {
	case TGSI_SEMANTIC_POSITION:
		return C_SEMANTIC_POSITION;
	case TGSI_SEMANTIC_COLOR:
		return C_SEMANTIC_COLOR;
	case TGSI_SEMANTIC_BCOLOR:
		return C_SEMANTIC_BCOLOR;
	case TGSI_SEMANTIC_FOG:
		return C_SEMANTIC_FOG;
	case TGSI_SEMANTIC_PSIZE:
		return C_SEMANTIC_PSIZE;
	case TGSI_SEMANTIC_GENERIC:
		return C_SEMANTIC_GENERIC;
	case TGSI_SEMANTIC_NORMAL:
		return C_SEMANTIC_NORMAL;
	case TGSI_SEMANTIC_FACE:
		return C_SEMANTIC_FACE;
	case TGSI_SEMANTIC_EDGEFLAG:
		return C_SEMANTIC_EDGEFLAG;
	case TGSI_SEMANTIC_PRIMID:
		return C_SEMANTIC_PRIMID;
	case TGSI_SEMANTIC_INSTANCEID:
		return C_SEMANTIC_INSTANCEID;
	default:
		return C_SEMANTIC_GENERIC;
	}
}

static int tgsi_opcode_to_c_opcode(unsigned opcode, unsigned *copcode)
{
	switch (opcode) {
	case TGSI_OPCODE_MOV:
		*copcode = C_OPCODE_MOV;
		return 0;
	case TGSI_OPCODE_MUL:
		*copcode = C_OPCODE_MUL;
		return 0;
	case TGSI_OPCODE_MAD:
		*copcode = C_OPCODE_MAD;
		return 0;
	case TGSI_OPCODE_END:
		*copcode = C_OPCODE_END;
		return 0;
	case TGSI_OPCODE_ARL:
		*copcode = C_OPCODE_ARL;
		return 0;
	case TGSI_OPCODE_LIT:
		*copcode = C_OPCODE_LIT;
		return 0;
	case TGSI_OPCODE_RCP:
		*copcode = C_OPCODE_RCP;
		return 0;
	case TGSI_OPCODE_RSQ:
		*copcode = C_OPCODE_RSQ;
		return 0;
	case TGSI_OPCODE_EXP:
		*copcode = C_OPCODE_EXP;
		return 0;
	case TGSI_OPCODE_LOG:
		*copcode = C_OPCODE_LOG;
		return 0;
	case TGSI_OPCODE_ADD:
		*copcode = C_OPCODE_ADD;
		return 0;
	case TGSI_OPCODE_DP3:
		*copcode = C_OPCODE_DP3;
		return 0;
	case TGSI_OPCODE_DP4:
		*copcode = C_OPCODE_DP4;
		return 0;
	case TGSI_OPCODE_DST:
		*copcode = C_OPCODE_DST;
		return 0;
	case TGSI_OPCODE_MIN:
		*copcode = C_OPCODE_MIN;
		return 0;
	case TGSI_OPCODE_MAX:
		*copcode = C_OPCODE_MAX;
		return 0;
	case TGSI_OPCODE_SLT:
		*copcode = C_OPCODE_SLT;
		return 0;
	case TGSI_OPCODE_SGE:
		*copcode = C_OPCODE_SGE;
		return 0;
	case TGSI_OPCODE_SUB:
		*copcode = C_OPCODE_SUB;
		return 0;
	case TGSI_OPCODE_LRP:
		*copcode = C_OPCODE_LRP;
		return 0;
	case TGSI_OPCODE_CND:
		*copcode = C_OPCODE_CND;
		return 0;
	case TGSI_OPCODE_DP2A:
		*copcode = C_OPCODE_DP2A;
		return 0;
	case TGSI_OPCODE_FRC:
		*copcode = C_OPCODE_FRC;
		return 0;
	case TGSI_OPCODE_CLAMP:
		*copcode = C_OPCODE_CLAMP;
		return 0;
	case TGSI_OPCODE_FLR:
		*copcode = C_OPCODE_FLR;
		return 0;
	case TGSI_OPCODE_ROUND:
		*copcode = C_OPCODE_ROUND;
		return 0;
	case TGSI_OPCODE_EX2:
		*copcode = C_OPCODE_EX2;
		return 0;
	case TGSI_OPCODE_LG2:
		*copcode = C_OPCODE_LG2;
		return 0;
	case TGSI_OPCODE_POW:
		*copcode = C_OPCODE_POW;
		return 0;
	case TGSI_OPCODE_XPD:
		*copcode = C_OPCODE_XPD;
		return 0;
	case TGSI_OPCODE_ABS:
		*copcode = C_OPCODE_ABS;
		return 0;
	case TGSI_OPCODE_RCC:
		*copcode = C_OPCODE_RCC;
		return 0;
	case TGSI_OPCODE_DPH:
		*copcode = C_OPCODE_DPH;
		return 0;
	case TGSI_OPCODE_COS:
		*copcode = C_OPCODE_COS;
		return 0;
	case TGSI_OPCODE_DDX:
		*copcode = C_OPCODE_DDX;
		return 0;
	case TGSI_OPCODE_DDY:
		*copcode = C_OPCODE_DDY;
		return 0;
	case TGSI_OPCODE_KILP:
		*copcode = C_OPCODE_KILP;
		return 0;
	case TGSI_OPCODE_PK2H:
		*copcode = C_OPCODE_PK2H;
		return 0;
	case TGSI_OPCODE_PK2US:
		*copcode = C_OPCODE_PK2US;
		return 0;
	case TGSI_OPCODE_PK4B:
		*copcode = C_OPCODE_PK4B;
		return 0;
	case TGSI_OPCODE_PK4UB:
		*copcode = C_OPCODE_PK4UB;
		return 0;
	case TGSI_OPCODE_RFL:
		*copcode = C_OPCODE_RFL;
		return 0;
	case TGSI_OPCODE_SEQ:
		*copcode = C_OPCODE_SEQ;
		return 0;
	case TGSI_OPCODE_SFL:
		*copcode = C_OPCODE_SFL;
		return 0;
	case TGSI_OPCODE_SGT:
		*copcode = C_OPCODE_SGT;
		return 0;
	case TGSI_OPCODE_SIN:
		*copcode = C_OPCODE_SIN;
		return 0;
	case TGSI_OPCODE_SLE:
		*copcode = C_OPCODE_SLE;
		return 0;
	case TGSI_OPCODE_SNE:
		*copcode = C_OPCODE_SNE;
		return 0;
	case TGSI_OPCODE_STR:
		*copcode = C_OPCODE_STR;
		return 0;
	case TGSI_OPCODE_TEX:
		*copcode = C_OPCODE_TEX;
		return 0;
	case TGSI_OPCODE_TXD:
		*copcode = C_OPCODE_TXD;
		return 0;
	case TGSI_OPCODE_TXP:
		*copcode = C_OPCODE_TXP;
		return 0;
	case TGSI_OPCODE_UP2H:
		*copcode = C_OPCODE_UP2H;
		return 0;
	case TGSI_OPCODE_UP2US:
		*copcode = C_OPCODE_UP2US;
		return 0;
	case TGSI_OPCODE_UP4B:
		*copcode = C_OPCODE_UP4B;
		return 0;
	case TGSI_OPCODE_UP4UB:
		*copcode = C_OPCODE_UP4UB;
		return 0;
	case TGSI_OPCODE_X2D:
		*copcode = C_OPCODE_X2D;
		return 0;
	case TGSI_OPCODE_ARA:
		*copcode = C_OPCODE_ARA;
		return 0;
	case TGSI_OPCODE_ARR:
		*copcode = C_OPCODE_ARR;
		return 0;
	case TGSI_OPCODE_BRA:
		*copcode = C_OPCODE_BRA;
		return 0;
	case TGSI_OPCODE_CAL:
		*copcode = C_OPCODE_CAL;
		return 0;
	case TGSI_OPCODE_RET:
		*copcode = C_OPCODE_RET;
		return 0;
	case TGSI_OPCODE_SSG:
		*copcode = C_OPCODE_SSG;
		return 0;
	case TGSI_OPCODE_CMP:
		*copcode = C_OPCODE_CMP;
		return 0;
	case TGSI_OPCODE_SCS:
		*copcode = C_OPCODE_SCS;
		return 0;
	case TGSI_OPCODE_TXB:
		*copcode = C_OPCODE_TXB;
		return 0;
	case TGSI_OPCODE_NRM:
		*copcode = C_OPCODE_NRM;
		return 0;
	case TGSI_OPCODE_DIV:
		*copcode = C_OPCODE_DIV;
		return 0;
	case TGSI_OPCODE_DP2:
		*copcode = C_OPCODE_DP2;
		return 0;
	case TGSI_OPCODE_TXL:
		*copcode = C_OPCODE_TXL;
		return 0;
	case TGSI_OPCODE_BRK:
		*copcode = C_OPCODE_BRK;
		return 0;
	case TGSI_OPCODE_IF:
		*copcode = C_OPCODE_IF;
		return 0;
	case TGSI_OPCODE_ELSE:
		*copcode = C_OPCODE_ELSE;
		return 0;
	case TGSI_OPCODE_ENDIF:
		*copcode = C_OPCODE_ENDIF;
		return 0;
	case TGSI_OPCODE_PUSHA:
		*copcode = C_OPCODE_PUSHA;
		return 0;
	case TGSI_OPCODE_POPA:
		*copcode = C_OPCODE_POPA;
		return 0;
	case TGSI_OPCODE_CEIL:
		*copcode = C_OPCODE_CEIL;
		return 0;
	case TGSI_OPCODE_I2F:
		*copcode = C_OPCODE_I2F;
		return 0;
	case TGSI_OPCODE_NOT:
		*copcode = C_OPCODE_NOT;
		return 0;
	case TGSI_OPCODE_TRUNC:
		*copcode = C_OPCODE_TRUNC;
		return 0;
	case TGSI_OPCODE_SHL:
		*copcode = C_OPCODE_SHL;
		return 0;
	case TGSI_OPCODE_AND:
		*copcode = C_OPCODE_AND;
		return 0;
	case TGSI_OPCODE_OR:
		*copcode = C_OPCODE_OR;
		return 0;
	case TGSI_OPCODE_MOD:
		*copcode = C_OPCODE_MOD;
		return 0;
	case TGSI_OPCODE_XOR:
		*copcode = C_OPCODE_XOR;
		return 0;
	case TGSI_OPCODE_SAD:
		*copcode = C_OPCODE_SAD;
		return 0;
	case TGSI_OPCODE_TXF:
		*copcode = C_OPCODE_TXF;
		return 0;
	case TGSI_OPCODE_TXQ:
		*copcode = C_OPCODE_TXQ;
		return 0;
	case TGSI_OPCODE_CONT:
		*copcode = C_OPCODE_CONT;
		return 0;
	case TGSI_OPCODE_EMIT:
		*copcode = C_OPCODE_EMIT;
		return 0;
	case TGSI_OPCODE_ENDPRIM:
		*copcode = C_OPCODE_ENDPRIM;
		return 0;
	case TGSI_OPCODE_BGNLOOP:
		*copcode = C_OPCODE_BGNLOOP;
		return 0;
	case TGSI_OPCODE_BGNSUB:
		*copcode = C_OPCODE_BGNSUB;
		return 0;
	case TGSI_OPCODE_ENDLOOP:
		*copcode = C_OPCODE_ENDLOOP;
		return 0;
	case TGSI_OPCODE_ENDSUB:
		*copcode = C_OPCODE_ENDSUB;
		return 0;
	case TGSI_OPCODE_NOP:
		*copcode = C_OPCODE_NOP;
		return 0;
	case TGSI_OPCODE_NRM4:
		*copcode = C_OPCODE_NRM4;
		return 0;
	case TGSI_OPCODE_CALLNZ:
		*copcode = C_OPCODE_CALLNZ;
		return 0;
	case TGSI_OPCODE_IFC:
		*copcode = C_OPCODE_IFC;
		return 0;
	case TGSI_OPCODE_BREAKC:
		*copcode = C_OPCODE_BREAKC;
		return 0;
	case TGSI_OPCODE_KIL:
		*copcode = C_OPCODE_KIL;
		return 0;
	case TGSI_OPCODE_F2I:
		*copcode = C_OPCODE_F2I;
		return 0;
	case TGSI_OPCODE_IDIV:
		*copcode = C_OPCODE_IDIV;
		return 0;
	case TGSI_OPCODE_IMAX:
		*copcode = C_OPCODE_IMAX;
		return 0;
	case TGSI_OPCODE_IMIN:
		*copcode = C_OPCODE_IMIN;
		return 0;
	case TGSI_OPCODE_INEG:
		*copcode = C_OPCODE_INEG;
		return 0;
	case TGSI_OPCODE_ISGE:
		*copcode = C_OPCODE_ISGE;
		return 0;
	case TGSI_OPCODE_ISHR:
		*copcode = C_OPCODE_ISHR;
		return 0;
	case TGSI_OPCODE_ISLT:
		*copcode = C_OPCODE_ISLT;
		return 0;
	case TGSI_OPCODE_F2U:
		*copcode = C_OPCODE_F2U;
		return 0;
	case TGSI_OPCODE_U2F:
		*copcode = C_OPCODE_U2F;
		return 0;
	case TGSI_OPCODE_UADD:
		*copcode = C_OPCODE_UADD;
		return 0;
	case TGSI_OPCODE_UDIV:
		*copcode = C_OPCODE_UDIV;
		return 0;
	case TGSI_OPCODE_UMAD:
		*copcode = C_OPCODE_UMAD;
		return 0;
	case TGSI_OPCODE_UMAX:
		*copcode = C_OPCODE_UMAX;
		return 0;
	case TGSI_OPCODE_UMIN:
		*copcode = C_OPCODE_UMIN;
		return 0;
	case TGSI_OPCODE_UMOD:
		*copcode = C_OPCODE_UMOD;
		return 0;
	case TGSI_OPCODE_UMUL:
		*copcode = C_OPCODE_UMUL;
		return 0;
	case TGSI_OPCODE_USEQ:
		*copcode = C_OPCODE_USEQ;
		return 0;
	case TGSI_OPCODE_USGE:
		*copcode = C_OPCODE_USGE;
		return 0;
	case TGSI_OPCODE_USHR:
		*copcode = C_OPCODE_USHR;
		return 0;
	case TGSI_OPCODE_USLT:
		*copcode = C_OPCODE_USLT;
		return 0;
	case TGSI_OPCODE_USNE:
		*copcode = C_OPCODE_USNE;
		return 0;
	case TGSI_OPCODE_SWITCH:
		*copcode = C_OPCODE_SWITCH;
		return 0;
	case TGSI_OPCODE_CASE:
		*copcode = C_OPCODE_CASE;
		return 0;
	case TGSI_OPCODE_DEFAULT:
		*copcode = C_OPCODE_DEFAULT;
		return 0;
	case TGSI_OPCODE_ENDSWITCH:
		*copcode = C_OPCODE_ENDSWITCH;
		return 0;
	default:
		fprintf(stderr, "%s:%d unsupported opcode %d\n", __func__, __LINE__, opcode);
		return -EINVAL;
	}
}
