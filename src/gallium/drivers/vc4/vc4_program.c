/*
 * Copyright (c) 2014 Scott Mansell
 * Copyright © 2014 Broadcom
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#include <stdio.h>
#include <inttypes.h>
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_hash_table.h"
#include "util/u_hash.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"

#include "vc4_context.h"
#include "vc4_qpu.h"
#include "vc4_qir.h"

struct tgsi_to_qir {
        struct tgsi_parse_context parser;
        struct qcompile *c;
        struct qreg *temps;
        struct qreg *inputs;
        struct qreg *outputs;
        struct qreg *uniforms;
        struct qreg *consts;
        uint32_t num_consts;

        struct vc4_shader_state *shader_state;
        struct vc4_fs_key *fs_key;
        struct vc4_vs_key *vs_key;

        uint32_t *uniform_data;
        enum quniform_contents *uniform_contents;
        uint32_t num_uniforms;
        uint32_t num_outputs;
};

struct vc4_key {
        struct vc4_shader_state *shader_state;
};

struct vc4_fs_key {
        struct vc4_key base;
        enum pipe_format color_format;
};

struct vc4_vs_key {
        struct vc4_key base;
        enum pipe_format attr_formats[8];
};

static struct qreg
add_uniform(struct tgsi_to_qir *trans,
            enum quniform_contents contents,
            uint32_t data)
{
        uint32_t uniform = trans->num_uniforms++;
        struct qreg u = { QFILE_UNIF, uniform };

        trans->uniform_contents[uniform] = contents;
        trans->uniform_data[uniform] = data;

        return u;
}

static struct qreg
get_temp_for_uniform(struct tgsi_to_qir *trans, enum quniform_contents contents,
                     uint32_t data)
{
        struct qcompile *c = trans->c;

        for (int i = 0; i < trans->num_uniforms; i++) {
                if (trans->uniform_contents[i] == contents &&
                    trans->uniform_data[i] == data)
                        return trans->uniforms[i];
        }

        struct qreg u = add_uniform(trans, contents, data);
        struct qreg t = qir_MOV(c, u);

        trans->uniforms[u.index] = t;
        return t;
}

static struct qreg
qir_uniform_ui(struct tgsi_to_qir *trans, uint32_t ui)
{
        return get_temp_for_uniform(trans, QUNIFORM_CONSTANT, ui);
}

static struct qreg
qir_uniform_f(struct tgsi_to_qir *trans, float f)
{
        return qir_uniform_ui(trans, fui(f));
}

static struct qreg
get_src(struct tgsi_to_qir *trans, struct tgsi_src_register *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg r = c->undef;

        uint32_t s = i;
        switch (i) {
        case TGSI_SWIZZLE_X:
                s = src->SwizzleX;
                break;
        case TGSI_SWIZZLE_Y:
                s = src->SwizzleY;
                break;
        case TGSI_SWIZZLE_Z:
                s = src->SwizzleZ;
                break;
        case TGSI_SWIZZLE_W:
                s = src->SwizzleW;
                break;
        default:
                abort();
        }

        assert(!src->Indirect);

        switch (src->File) {
        case TGSI_FILE_NULL:
                return r;
        case TGSI_FILE_TEMPORARY:
                r = trans->temps[src->Index * 4 + s];
                break;
        case TGSI_FILE_IMMEDIATE:
                r = trans->consts[src->Index * 4 + s];
                break;
        case TGSI_FILE_CONSTANT:
                r = get_temp_for_uniform(trans, QUNIFORM_UNIFORM,
                                         src->Index * 4 + s);
                break;
        case TGSI_FILE_INPUT:
                r = trans->inputs[src->Index * 4 + s];
                break;
        default:
                fprintf(stderr, "unknown src file %d\n", src->File);
                abort();
        }

        if (src->Absolute)
                r = qir_FMAXABS(c, r, r);

        if (src->Negate)
                r = qir_FSUB(c, qir_uniform_f(trans, 0), r);

        return r;
};


static void
update_dst(struct tgsi_to_qir *trans, struct tgsi_full_instruction *tgsi_inst,
           int i, struct qreg val)
{
        struct tgsi_dst_register *tgsi_dst = &tgsi_inst->Dst[0].Register;

        assert(!tgsi_dst->Indirect);

        switch (tgsi_dst->File) {
        case TGSI_FILE_TEMPORARY:
                trans->temps[tgsi_dst->Index * 4 + i] = val;
                break;
        case TGSI_FILE_OUTPUT:
                trans->outputs[tgsi_dst->Index * 4 + i] = val;
                trans->num_outputs = MAX2(trans->num_outputs,
                                          tgsi_dst->Index * 4 + i + 1);
                break;
        default:
                fprintf(stderr, "unknown dst file %d\n", tgsi_dst->File);
                abort();
        }
};

static struct qreg
tgsi_to_qir_alu(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg dst = qir_get_temp(c);
        qir_emit(c, qir_inst4(op, dst,
                              src[0 * 4 + i],
                              src[1 * 4 + i],
                              src[2 * 4 + i],
                              c->undef));
        return dst;
}

static struct qreg
tgsi_to_qir_mad(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        return qir_FADD(c,
                        qir_FMUL(c,
                                 src[0 * 4 + i],
                                 src[1 * 4 + i]),
                        src[2 * 4 + i]);
}

static struct qreg
tgsi_to_qir_lit(struct tgsi_to_qir *trans,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg x = src[0 * 4 + 0];
        struct qreg y = src[0 * 4 + 1];
        struct qreg w = src[0 * 4 + 3];

        switch (i) {
        case 0:
        case 3:
                return qir_uniform_f(trans, 1.0);
        case 1:
                return qir_FMAX(c, src[0 * 4 + 0], qir_uniform_f(trans, 0.0));
        case 2: {
                struct qreg zero = qir_uniform_f(trans, 0.0);

                /* XXX: Clamp w to -128..128 */
                return qir_CMP(c,
                               x,
                               zero,
                               qir_EXP2(c, qir_FMUL(c,
                                                    w,
                                                    qir_LOG2(c,
                                                             qir_FMAX(c,
                                                                      y,
                                                                      zero)))));
        }
        default:
                assert(!"not reached");
                return c->undef;
        }
}

static struct qreg
tgsi_to_qir_lrp(struct tgsi_to_qir *trans,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg src0 = src[0 * 4 + i];
        struct qreg src1 = src[1 * 4 + i];
        struct qreg src2 = src[2 * 4 + i];

        /* LRP is:
         *    src0 * src1 + (1 - src0) * src2.
         * -> src0 * src1 + src2 - src0 * src2
         * -> src2 + src0 * (src1 - src2)
         */
        return qir_FADD(c, src2, qir_FMUL(c, src0, qir_FSUB(c, src1, src2)));

}

static struct qreg
tgsi_to_qir_pow(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;

        /* Note that this instruction replicates its result from the x channel
         */
        return qir_EXP2(c, qir_FMUL(c,
                                    src[1 * 4 + 0],
                                    qir_LOG2(c, src[0 * 4 + 0])));
}

static struct qreg
tgsi_to_qir_trunc(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        return qir_ITOF(c, qir_FTOI(c, src[0 * 4 + i]));
}

/**
 * Computes x - floor(x), which is tricky because our FTOI truncates (rounds
 * to zero).
 */
static struct qreg
tgsi_to_qir_frc(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg trunc = qir_ITOF(c, qir_FTOI(c, src[0 * 4 + i]));
        struct qreg diff = qir_FSUB(c, src[0 * 4 + i], trunc);
        return qir_CMP(c,
                       diff,
                       qir_FADD(c, diff, qir_uniform_f(trans, 1.0)),
                       diff);
}

static struct qreg
tgsi_to_qir_dp(struct tgsi_to_qir *trans,
               struct tgsi_full_instruction *tgsi_inst,
               int num, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;

        struct qreg sum = qir_FMUL(c, src[0 * 4 + 0], src[1 * 4 + 0]);
        for (int j = 1; j < num; j++) {
                sum = qir_FADD(c, sum, qir_FMUL(c,
                                                src[0 * 4 + j],
                                                src[1 * 4 + j]));
        }
        return sum;
}

static struct qreg
tgsi_to_qir_dp2(struct tgsi_to_qir *trans,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return tgsi_to_qir_dp(trans, tgsi_inst, 2, src, i);
}

static struct qreg
tgsi_to_qir_dp3(struct tgsi_to_qir *trans,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return tgsi_to_qir_dp(trans, tgsi_inst, 3, src, i);
}

static struct qreg
tgsi_to_qir_dp4(struct tgsi_to_qir *trans,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return tgsi_to_qir_dp(trans, tgsi_inst, 4, src, i);
}

static struct qreg
tgsi_to_qir_abs(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg arg = src[0 * 4 + i];
        return qir_FMAXABS(c, arg, arg);
}

static void
emit_vertex_input(struct tgsi_to_qir *trans, int attr)
{
        enum pipe_format format = trans->vs_key->attr_formats[attr];
        struct qcompile *c = trans->c;
        struct qreg vpm_reads[4];

        /* Right now, we're setting the VPM offsets to be 16 bytes wide every
         * time, so we always read 4 32-bit VPM entries.
         */
        for (int i = 0; i < 4; i++) {
                vpm_reads[i] = qir_get_temp(c);
                qir_emit(c, qir_inst(QOP_VPM_READ,
                                     vpm_reads[i],
                                     c->undef,
                                     c->undef));
                c->num_inputs++;
        }

        bool format_warned = false;
        const struct util_format_description *desc =
                util_format_description(format);

        for (int i = 0; i < 4; i++) {
                uint8_t swiz = desc->swizzle[i];

                switch (swiz) {
                case UTIL_FORMAT_SWIZZLE_NONE:
                        if (!format_warned) {
                                fprintf(stderr,
                                        "vtx element %d NONE swizzle: %s\n",
                                        attr, util_format_name(format));
                                format_warned = true;
                        }
                        /* FALLTHROUGH */
                case UTIL_FORMAT_SWIZZLE_0:
                        trans->inputs[attr * 4 + i] = qir_uniform_ui(trans, 0);
                        break;
                case UTIL_FORMAT_SWIZZLE_1:
                        trans->inputs[attr * 4 + i] = qir_uniform_ui(trans,
                                                                     fui(1.0));
                        break;
                default:
                        if (!format_warned &&
                            (desc->channel[swiz].type != UTIL_FORMAT_TYPE_FLOAT ||
                             desc->channel[swiz].size != 32)) {
                                fprintf(stderr,
                                        "vtx element %d unsupported type: %s\n",
                                        attr, util_format_name(format));
                                format_warned = true;
                        }

                        trans->inputs[attr * 4 + i] = vpm_reads[swiz];
                        break;
                }
        }
}

static void
emit_tgsi_declaration(struct tgsi_to_qir *trans,
                      struct tgsi_full_declaration *decl)
{
        struct qcompile *c = trans->c;

        switch (decl->Declaration.File) {
        case TGSI_FILE_INPUT:
                if (c->stage == QSTAGE_FRAG) {
                        for (int i = decl->Range.First * 4;
                             i < (decl->Range.Last + 1) * 4;
                             i++) {
                                struct qreg vary = {
                                        QFILE_VARY,
                                        i
                                };
                                trans->inputs[i] =
                                        qir_VARY_ADD_C(c, qir_MOV(c, vary));

                                c->num_inputs++;
                        }
                } else {
                        for (int i = decl->Range.First;
                             i <= decl->Range.Last;
                             i++) {
                                emit_vertex_input(trans, i);
                        }
                }
                break;
        }
}

static void
emit_tgsi_instruction(struct tgsi_to_qir *trans,
                      struct tgsi_full_instruction *tgsi_inst)
{
        struct qcompile *c = trans->c;
        struct {
                enum qop op;
                struct qreg (*func)(struct tgsi_to_qir *trans,
                                    struct tgsi_full_instruction *tgsi_inst,
                                    enum qop op,
                                    struct qreg *src, int i);
        } op_trans[] = {
                [TGSI_OPCODE_MOV] = { QOP_MOV, tgsi_to_qir_alu },
                [TGSI_OPCODE_ABS] = { 0, tgsi_to_qir_abs },
                [TGSI_OPCODE_MUL] = { QOP_FMUL, tgsi_to_qir_alu },
                [TGSI_OPCODE_ADD] = { QOP_FADD, tgsi_to_qir_alu },
                [TGSI_OPCODE_SUB] = { QOP_FSUB, tgsi_to_qir_alu },
                [TGSI_OPCODE_MIN] = { QOP_FMIN, tgsi_to_qir_alu },
                [TGSI_OPCODE_MAX] = { QOP_FMAX, tgsi_to_qir_alu },
                [TGSI_OPCODE_RSQ] = { QOP_RSQ, tgsi_to_qir_alu },
                [TGSI_OPCODE_SEQ] = { QOP_SEQ, tgsi_to_qir_alu },
                [TGSI_OPCODE_SNE] = { QOP_SNE, tgsi_to_qir_alu },
                [TGSI_OPCODE_SGE] = { QOP_SGE, tgsi_to_qir_alu },
                [TGSI_OPCODE_SLT] = { QOP_SLT, tgsi_to_qir_alu },
                [TGSI_OPCODE_CMP] = { QOP_CMP, tgsi_to_qir_alu },
                [TGSI_OPCODE_MAD] = { 0, tgsi_to_qir_mad },
                [TGSI_OPCODE_DP2] = { 0, tgsi_to_qir_dp2 },
                [TGSI_OPCODE_DP3] = { 0, tgsi_to_qir_dp3 },
                [TGSI_OPCODE_DP4] = { 0, tgsi_to_qir_dp4 },
                [TGSI_OPCODE_RCP] = { QOP_RCP, tgsi_to_qir_alu },
                [TGSI_OPCODE_RSQ] = { QOP_RSQ, tgsi_to_qir_alu },
                [TGSI_OPCODE_EX2] = { QOP_EXP2, tgsi_to_qir_alu },
                [TGSI_OPCODE_LG2] = { QOP_LOG2, tgsi_to_qir_alu },
                [TGSI_OPCODE_LIT] = { 0, tgsi_to_qir_lit },
                [TGSI_OPCODE_LRP] = { 0, tgsi_to_qir_lrp },
                [TGSI_OPCODE_POW] = { 0, tgsi_to_qir_pow },
                [TGSI_OPCODE_TRUNC] = { 0, tgsi_to_qir_trunc },
                [TGSI_OPCODE_FRC] = { 0, tgsi_to_qir_frc },
        };
        static int asdf = 0;
        uint32_t tgsi_op = tgsi_inst->Instruction.Opcode;

        if (tgsi_op == TGSI_OPCODE_END)
                return;

        if (tgsi_op > ARRAY_SIZE(op_trans) || !op_trans[tgsi_op].func) {
                fprintf(stderr, "unknown tgsi inst: ");
                tgsi_dump_instruction(tgsi_inst, asdf++);
                fprintf(stderr, "\n");
                abort();
        }

        struct qreg src_regs[12];
        for (int s = 0; s < 3; s++) {
                for (int i = 0; i < 4; i++) {
                        src_regs[4 * s + i] =
                                get_src(trans, &tgsi_inst->Src[s].Register, i);
                }
        }

        for (int i = 0; i < 4; i++) {
                if (!(tgsi_inst->Dst[0].Register.WriteMask & (1 << i)))
                        continue;

                struct qreg result;

                result = op_trans[tgsi_op].func(trans, tgsi_inst,
                                                op_trans[tgsi_op].op,
                                                src_regs, i);

                if (tgsi_inst->Instruction.Saturate) {
                        float low = (tgsi_inst->Instruction.Saturate ==
                                     TGSI_SAT_MINUS_PLUS_ONE ? -1.0 : 0.0);
                        result = qir_FMAX(c,
                                          qir_FMIN(c,
                                                   result,
                                                   qir_uniform_f(trans, 1.0)),
                                          qir_uniform_f(trans, low));
                }

                update_dst(trans, tgsi_inst, i, result);
        }
}

static void
parse_tgsi_immediate(struct tgsi_to_qir *trans, struct tgsi_full_immediate *imm)
{
        for (int i = 0; i < 4; i++) {
                unsigned n = trans->num_consts++;
                trans->consts[n] = qir_uniform_ui(trans, imm->u[i].Uint);
        }
}

static void
emit_frag_end(struct tgsi_to_qir *trans)
{
        struct qcompile *c = trans->c;

        struct qreg t = qir_get_temp(c);

        const struct util_format_description *format_desc =
                util_format_description(trans->fs_key->color_format);

        /* Debug: Sometimes you're getting a black output and just want to see
         * if the FS is getting executed at all.  Spam magenta into the color
         * output.
         */
        if (0) {
                trans->outputs[format_desc->swizzle[0]] =
                        qir_uniform_ui(trans, fui(1.0));
                trans->outputs[format_desc->swizzle[1]] =
                        qir_uniform_ui(trans, fui(0.0));
                trans->outputs[format_desc->swizzle[2]] =
                        qir_uniform_ui(trans, fui(1.0));
                trans->outputs[format_desc->swizzle[3]] =
                        qir_uniform_ui(trans, fui(0.5));
        }

        struct qreg swizzled_outputs[4] = {
                trans->outputs[format_desc->swizzle[0]],
                trans->outputs[format_desc->swizzle[1]],
                trans->outputs[format_desc->swizzle[2]],
                trans->outputs[format_desc->swizzle[3]],
        };

        qir_emit(c, qir_inst4(QOP_PACK_COLORS, t,
                              swizzled_outputs[0],
                              swizzled_outputs[1],
                              swizzled_outputs[2],
                              swizzled_outputs[3]));
        qir_emit(c, qir_inst(QOP_TLB_COLOR_WRITE, c->undef,
                             t, c->undef));
}

static void
emit_scaled_viewport_write(struct tgsi_to_qir *trans, struct qreg rcp_w)
{
        struct qcompile *c = trans->c;
        struct qreg xyi[2];

        for (int i = 0; i < 2; i++) {
                struct qreg scale =
                        add_uniform(trans, QUNIFORM_VIEWPORT_X_SCALE + i, 0);

                xyi[i] = qir_FTOI(c, qir_FMUL(c,
                                              qir_FMUL(c,
                                                       trans->outputs[i],
                                                       scale),
                                              rcp_w));
        }

        qir_VPM_WRITE(c, qir_PACK_SCALED(c, xyi[0], xyi[1]));
}

static void
emit_zs_write(struct tgsi_to_qir *trans, struct qreg rcp_w)
{
        struct qcompile *c = trans->c;

        qir_VPM_WRITE(c, qir_FMUL(c, trans->outputs[2], rcp_w));
}

static void
emit_rcp_wc_write(struct tgsi_to_qir *trans, struct qreg rcp_w)
{
        struct qcompile *c = trans->c;

        qir_VPM_WRITE(c, rcp_w);
}

static void
emit_vert_end(struct tgsi_to_qir *trans)
{
        struct qcompile *c = trans->c;

        struct qreg rcp_w = qir_RCP(c, trans->outputs[3]);

        emit_scaled_viewport_write(trans, rcp_w);
        emit_zs_write(trans, rcp_w);
        emit_rcp_wc_write(trans, rcp_w);

        for (int i = 4; i < trans->num_outputs; i++) {
                qir_VPM_WRITE(c, trans->outputs[i]);
        }
}

static void
emit_coord_end(struct tgsi_to_qir *trans)
{
        struct qcompile *c = trans->c;

        struct qreg rcp_w = qir_RCP(c, trans->outputs[3]);

        for (int i = 0; i < 4; i++)
                qir_VPM_WRITE(c, trans->outputs[i]);

        emit_scaled_viewport_write(trans, rcp_w);
        emit_zs_write(trans, rcp_w);
        emit_rcp_wc_write(trans, rcp_w);
}

static struct tgsi_to_qir *
vc4_shader_tgsi_to_qir(struct vc4_compiled_shader *shader, enum qstage stage,
                       struct vc4_key *key)
{
        struct tgsi_to_qir *trans = CALLOC_STRUCT(tgsi_to_qir);
        struct qcompile *c;
        int ret;

        c = qir_compile_init();
        c->stage = stage;

        memset(trans, 0, sizeof(*trans));
        /* XXX sizing */
        trans->temps = calloc(sizeof(struct qreg), 1024);
        trans->inputs = calloc(sizeof(struct qreg), 8 * 4);
        trans->outputs = calloc(sizeof(struct qreg), 1024);
        trans->uniforms = calloc(sizeof(struct qreg), 1024);
        trans->consts = calloc(sizeof(struct qreg), 1024);

        trans->uniform_data = calloc(sizeof(uint32_t), 1024);
        trans->uniform_contents = calloc(sizeof(enum quniform_contents), 1024);

        trans->shader_state = key->shader_state;
        trans->c = c;
        ret = tgsi_parse_init(&trans->parser, trans->shader_state->base.tokens);
        assert(ret == TGSI_PARSE_OK);

        if (vc4_debug & VC4_DEBUG_TGSI) {
                fprintf(stderr, "TGSI:\n");
                tgsi_dump(trans->shader_state->base.tokens, 0);
        }

        switch (stage) {
        case QSTAGE_FRAG:
                trans->fs_key = (struct vc4_fs_key *)key;
                break;
        case QSTAGE_VERT:
                trans->vs_key = (struct vc4_vs_key *)key;
                break;
        case QSTAGE_COORD:
                trans->vs_key = (struct vc4_vs_key *)key;
                break;
        }

        while (!tgsi_parse_end_of_tokens(&trans->parser)) {
                tgsi_parse_token(&trans->parser);

                switch (trans->parser.FullToken.Token.Type) {
                case TGSI_TOKEN_TYPE_DECLARATION:
                        emit_tgsi_declaration(trans,
                                              &trans->parser.FullToken.FullDeclaration);
                        break;

                case TGSI_TOKEN_TYPE_INSTRUCTION:
                        emit_tgsi_instruction(trans,
                                              &trans->parser.FullToken.FullInstruction);
                        break;

                case TGSI_TOKEN_TYPE_IMMEDIATE:
                        parse_tgsi_immediate(trans,
                                             &trans->parser.FullToken.FullImmediate);
                        break;
                }
        }

        switch (stage) {
        case QSTAGE_FRAG:
                emit_frag_end(trans);
                break;
        case QSTAGE_VERT:
                emit_vert_end(trans);
                break;
        case QSTAGE_COORD:
                emit_coord_end(trans);
                break;
        }

        tgsi_parse_free(&trans->parser);
        free(trans->temps);

        qir_optimize(c);

        if (vc4_debug & VC4_DEBUG_QIR) {
                fprintf(stderr, "QIR:\n");
                qir_dump(c);
        }
        vc4_generate_code(c);

        if (vc4_debug & VC4_DEBUG_SHADERDB) {
                fprintf(stderr, "SHADER-DB: %s: %d instructions\n",
                        qir_get_stage_name(c->stage), c->qpu_inst_count);
                fprintf(stderr, "SHADER-DB: %s: %d uniforms\n",
                        qir_get_stage_name(c->stage), trans->num_uniforms);
        }

        return trans;
}

static void *
vc4_shader_state_create(struct pipe_context *pctx,
                        const struct pipe_shader_state *cso)
{
        struct vc4_shader_state *so = CALLOC_STRUCT(vc4_shader_state);
        if (!so)
                return NULL;

        so->base.tokens = tgsi_dup_tokens(cso->tokens);

        return so;
}

static void
copy_uniform_state_to_shader(struct vc4_compiled_shader *shader,
                             int shader_index,
                             struct tgsi_to_qir *trans)
{
        int count = trans->num_uniforms;
        struct vc4_shader_uniform_info *uinfo = &shader->uniforms[shader_index];

        uinfo->count = count;
        uinfo->data = malloc(count * sizeof(*uinfo->data));
        memcpy(uinfo->data, trans->uniform_data,
               count * sizeof(*uinfo->data));
        uinfo->contents = malloc(count * sizeof(*uinfo->contents));
        memcpy(uinfo->contents, trans->uniform_contents,
               count * sizeof(*uinfo->contents));
}

static void
vc4_fs_compile(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
               struct vc4_fs_key *key)
{
        struct tgsi_to_qir *trans = vc4_shader_tgsi_to_qir(shader, QSTAGE_FRAG,
                                                           &key->base);
        shader->num_inputs = trans->c->num_inputs;
        copy_uniform_state_to_shader(shader, 0, trans);
        shader->bo = vc4_bo_alloc_mem(vc4->screen, trans->c->qpu_insts,
                                      trans->c->qpu_inst_count * sizeof(uint64_t),
                                      "fs_code");

        qir_compile_destroy(trans->c);
        free(trans);
}

static void
vc4_vs_compile(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
               struct vc4_vs_key *key)
{
        struct tgsi_to_qir *vs_trans = vc4_shader_tgsi_to_qir(shader,
                                                              QSTAGE_VERT,
                                                              &key->base);
        copy_uniform_state_to_shader(shader, 0, vs_trans);

        struct tgsi_to_qir *cs_trans = vc4_shader_tgsi_to_qir(shader,
                                                              QSTAGE_COORD,
                                                              &key->base);
        copy_uniform_state_to_shader(shader, 1, cs_trans);

        uint32_t vs_size = vs_trans->c->qpu_inst_count * sizeof(uint64_t);
        uint32_t cs_size = cs_trans->c->qpu_inst_count * sizeof(uint64_t);
        shader->coord_shader_offset = vs_size; /* XXX: alignment? */
        shader->bo = vc4_bo_alloc(vc4->screen,
                                  shader->coord_shader_offset + cs_size,
                                  "vs_code");

        void *map = vc4_bo_map(shader->bo);
        memcpy(map, vs_trans->c->qpu_insts, vs_size);
        memcpy(map + shader->coord_shader_offset,
               cs_trans->c->qpu_insts, cs_size);

        qir_compile_destroy(vs_trans->c);
        qir_compile_destroy(cs_trans->c);
}

static void
vc4_update_compiled_fs(struct vc4_context *vc4)
{
        struct vc4_fs_key local_key;
        struct vc4_fs_key *key = &local_key;

        memset(key, 0, sizeof(*key));
        key->base.shader_state = vc4->prog.bind_fs;

        if (vc4->framebuffer.cbufs[0])
                key->color_format = vc4->framebuffer.cbufs[0]->format;

        vc4->prog.fs = util_hash_table_get(vc4->fs_cache, key);
        if (vc4->prog.fs)
                return;

        key = malloc(sizeof(*key));
        memcpy(key, &local_key, sizeof(*key));

        struct vc4_compiled_shader *shader = CALLOC_STRUCT(vc4_compiled_shader);
        vc4_fs_compile(vc4, shader, key);
        util_hash_table_set(vc4->fs_cache, key, shader);

        vc4->prog.fs = shader;
}

static void
vc4_update_compiled_vs(struct vc4_context *vc4)
{
        struct vc4_vs_key local_key;
        struct vc4_vs_key *key = &local_key;

        memset(key, 0, sizeof(*key));
        key->base.shader_state = vc4->prog.bind_vs;

        for (int i = 0; i < ARRAY_SIZE(key->attr_formats); i++)
                key->attr_formats[i] = vc4->vtx->pipe[i].src_format;

        vc4->prog.vs = util_hash_table_get(vc4->vs_cache, key);
        if (vc4->prog.vs)
                return;

        key = malloc(sizeof(*key));
        memcpy(key, &local_key, sizeof(*key));

        struct vc4_compiled_shader *shader = CALLOC_STRUCT(vc4_compiled_shader);
        vc4_vs_compile(vc4, shader, key);
        util_hash_table_set(vc4->vs_cache, key, shader);

        vc4->prog.vs = shader;
}

void
vc4_update_compiled_shaders(struct vc4_context *vc4)
{
        vc4_update_compiled_fs(vc4);
        vc4_update_compiled_vs(vc4);
}

static unsigned
fs_cache_hash(void *key)
{
        return util_hash_crc32(key, sizeof(struct vc4_fs_key));
}

static unsigned
vs_cache_hash(void *key)
{
        return util_hash_crc32(key, sizeof(struct vc4_vs_key));
}

static int
fs_cache_compare(void *key1, void *key2)
{
        return memcmp(key1, key2, sizeof(struct vc4_fs_key));
}

static int
vs_cache_compare(void *key1, void *key2)
{
        return memcmp(key1, key2, sizeof(struct vc4_vs_key));
}

struct delete_state {
        struct vc4_context *vc4;
        struct vc4_shader_state *shader_state;
};

static enum pipe_error
fs_delete_from_cache(void *in_key, void *in_value, void *data)
{
        struct delete_state *del = data;
        struct vc4_fs_key *key = in_key;
        struct vc4_compiled_shader *shader = in_value;

        if (key->base.shader_state == data) {
                util_hash_table_remove(del->vc4->fs_cache, key);
                vc4_bo_unreference(&shader->bo);
                free(shader);
        }

        return 0;
}

static enum pipe_error
vs_delete_from_cache(void *in_key, void *in_value, void *data)
{
        struct delete_state *del = data;
        struct vc4_vs_key *key = in_key;
        struct vc4_compiled_shader *shader = in_value;

        if (key->base.shader_state == data) {
                util_hash_table_remove(del->vc4->vs_cache, key);
                vc4_bo_unreference(&shader->bo);
                free(shader);
        }

        return 0;
}

static void
vc4_shader_state_delete(struct pipe_context *pctx, void *hwcso)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        struct vc4_shader_state *so = hwcso;
        struct delete_state del;

        del.vc4 = vc4;
        del.shader_state = so;
        util_hash_table_foreach(vc4->fs_cache, fs_delete_from_cache, &del);
        util_hash_table_foreach(vc4->vs_cache, vs_delete_from_cache, &del);

        free((void *)so->base.tokens);
        free(so);
}

void
vc4_get_uniform_bo(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
                   struct vc4_constbuf_stateobj *cb,
                   int shader_index, struct vc4_bo **out_bo,
                   uint32_t *out_offset)
{
        struct vc4_shader_uniform_info *uinfo = &shader->uniforms[shader_index];
        struct vc4_bo *ubo = vc4_bo_alloc(vc4->screen,
                                          MAX2(1, uinfo->count * 4), "ubo");
        uint32_t *map = vc4_bo_map(ubo);

        for (int i = 0; i < uinfo->count; i++) {
                switch (uinfo->contents[i]) {
                case QUNIFORM_CONSTANT:
                        map[i] = uinfo->data[i];
                        break;
                case QUNIFORM_UNIFORM:
                        map[i] = ((uint32_t *)cb->cb[0].user_buffer)[uinfo->data[i]];
                        break;
                case QUNIFORM_VIEWPORT_X_SCALE:
                        map[i] = fui(vc4->framebuffer.width * 16.0f / 2.0f);
                        break;
                case QUNIFORM_VIEWPORT_Y_SCALE:
                        map[i] = fui(vc4->framebuffer.height * -16.0f / 2.0f);
                        break;
                }
#if 0
                fprintf(stderr, "%p/%d: %d: 0x%08x (%f)\n",
                        shader, shader_index, i, map[i], uif(map[i]));
#endif
        }

        *out_bo = ubo;
        *out_offset = 0;
}

static void
vc4_fp_state_bind(struct pipe_context *pctx, void *hwcso)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        vc4->prog.bind_fs = hwcso;
        vc4->prog.dirty |= VC4_SHADER_DIRTY_FP;
        vc4->dirty |= VC4_DIRTY_PROG;
}

static void
vc4_vp_state_bind(struct pipe_context *pctx, void *hwcso)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        vc4->prog.bind_vs = hwcso;
        vc4->prog.dirty |= VC4_SHADER_DIRTY_VP;
        vc4->dirty |= VC4_DIRTY_PROG;
}

void
vc4_program_init(struct pipe_context *pctx)
{
        struct vc4_context *vc4 = vc4_context(pctx);

        pctx->create_vs_state = vc4_shader_state_create;
        pctx->delete_vs_state = vc4_shader_state_delete;

        pctx->create_fs_state = vc4_shader_state_create;
        pctx->delete_fs_state = vc4_shader_state_delete;

        pctx->bind_fs_state = vc4_fp_state_bind;
        pctx->bind_vs_state = vc4_vp_state_bind;

        vc4->fs_cache = util_hash_table_create(fs_cache_hash, fs_cache_compare);
        vc4->vs_cache = util_hash_table_create(vs_cache_hash, vs_cache_compare);
}
