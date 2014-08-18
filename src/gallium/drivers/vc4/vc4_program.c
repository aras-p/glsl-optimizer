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
#ifdef USE_VC4_SIMULATOR
#include "simpenrose/simpenrose.h"
#endif

struct tgsi_to_qir {
        struct tgsi_parse_context parser;
        struct qcompile *c;
        struct qreg *temps;
        struct qreg *inputs;
        struct qreg *outputs;
        struct qreg *uniforms;
        struct qreg *consts;
        struct qreg line_x, point_x, point_y;

        uint32_t num_consts;

        struct pipe_shader_state *shader_state;
        struct vc4_key *key;
        struct vc4_fs_key *fs_key;
        struct vc4_vs_key *vs_key;

        uint32_t *uniform_data;
        enum quniform_contents *uniform_contents;
        uint32_t num_uniforms;
        uint32_t num_outputs;
        uint32_t num_texture_samples;
};

struct vc4_key {
        struct pipe_shader_state *shader_state;
        enum pipe_format tex_format[VC4_MAX_TEXTURE_SAMPLERS];
};

struct vc4_fs_key {
        struct vc4_key base;
        enum pipe_format color_format;
        bool depth_enabled;
        bool is_points;
        bool is_lines;

        struct pipe_rt_blend_state blend;
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
        case TGSI_FILE_SAMPLER:
        case TGSI_FILE_SAMPLER_VIEW:
                r = c->undef;
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
get_swizzled_channel(struct tgsi_to_qir *trans,
                     struct qreg *srcs, int swiz)
{
        switch (swiz) {
        default:
        case UTIL_FORMAT_SWIZZLE_NONE:
                fprintf(stderr, "warning: unknown swizzle\n");
                /* FALLTHROUGH */
        case UTIL_FORMAT_SWIZZLE_0:
                return qir_uniform_f(trans, 0.0);
        case UTIL_FORMAT_SWIZZLE_1:
                return qir_uniform_f(trans, 1.0);
        case UTIL_FORMAT_SWIZZLE_X:
        case UTIL_FORMAT_SWIZZLE_Y:
        case UTIL_FORMAT_SWIZZLE_Z:
        case UTIL_FORMAT_SWIZZLE_W:
                return srcs[swiz];
        }
}

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

static void
tgsi_to_qir_tex(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src)
{
        struct qcompile *c = trans->c;

        assert(!tgsi_inst->Instruction.Saturate);

        struct qreg s = src[0 * 4 + 0];
        struct qreg t = src[0 * 4 + 1];
        uint32_t unit = tgsi_inst->Src[1].Register.Index;

        if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXP) {
                struct qreg proj = qir_RCP(c, src[0 * 4 + 3]);
                s = qir_FMUL(c, s, proj);
                t = qir_FMUL(c, t, proj);
        }

        /* There is no native support for GL texture rectangle coordinates, so
         * we have to rescale from ([0, width], [0, height]) to ([0, 1], [0,
         * 1]).
         */
        if (tgsi_inst->Texture.Texture == TGSI_TEXTURE_RECT) {
                s = qir_FMUL(c, s,
                             get_temp_for_uniform(trans,
                                                  QUNIFORM_TEXRECT_SCALE_X,
                                                  unit));
                t = qir_FMUL(c, t,
                             get_temp_for_uniform(trans,
                                                  QUNIFORM_TEXRECT_SCALE_Y,
                                                  unit));
        }

        qir_TEX_T(c, t, add_uniform(trans, QUNIFORM_TEXTURE_CONFIG_P0,
                                    unit));

        struct qreg sampler_p1 = add_uniform(trans, QUNIFORM_TEXTURE_CONFIG_P1,
                                             unit);
        if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXB) {
                qir_TEX_B(c, src[0 * 4 + 3], sampler_p1);
                qir_TEX_S(c, s, add_uniform(trans, QUNIFORM_CONSTANT, 0));
        } else {
                qir_TEX_S(c, s, sampler_p1);
        }

        trans->num_texture_samples++;
        qir_emit(c, qir_inst(QOP_TEX_RESULT, c->undef, c->undef, c->undef));

        struct qreg unpacked[4];
        for (int i = 0; i < 4; i++)
                unpacked[i] = qir_R4_UNPACK(c, i);

        bool format_warned = false;
        for (int i = 0; i < 4; i++) {
                if (!(tgsi_inst->Dst[0].Register.WriteMask & (1 << i)))
                        continue;

                enum pipe_format format = trans->key->tex_format[unit];
                const struct util_format_description *desc =
                        util_format_description(format);

                uint8_t swiz = desc->swizzle[i];
                if (!format_warned &&
                    swiz <= UTIL_FORMAT_SWIZZLE_W &&
                    (desc->channel[swiz].type != UTIL_FORMAT_TYPE_UNSIGNED ||
                     desc->channel[swiz].size != 8)) {
                        fprintf(stderr,
                                "tex channel %d unsupported type: %s\n",
                                i, util_format_name(format));
                        format_warned = true;
                }

                update_dst(trans, tgsi_inst, i,
                           get_swizzled_channel(trans, unpacked, swiz));
        }
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

/**
 * Computes floor(x), which is tricky because our FTOI truncates (rounds to
 * zero).
 */
static struct qreg
tgsi_to_qir_flr(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        struct qreg trunc = qir_ITOF(c, qir_FTOI(c, src[0 * 4 + i]));
        return qir_CMP(c,
                       src[0 * 4 + i],
                       qir_FSUB(c, trunc, qir_uniform_f(trans, 1.0)),
                       trunc);
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

/* Note that this instruction replicates its result from the x channel */
static struct qreg
tgsi_to_qir_sin(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        float coeff[] = {
                2.0 * M_PI,
                -pow(2.0 * M_PI, 3) / (3 * 2 * 1),
                pow(2.0 * M_PI, 5) / (5 * 4 * 3 * 2 * 1),
                -pow(2.0 * M_PI, 7) / (7 * 6 * 5 * 4 * 3 * 2 * 1),
        };

        struct qreg scaled_x =
                qir_FMUL(c,
                         src[0 * 4 + 0],
                         qir_uniform_f(trans, 1.0f / (M_PI * 2.0f)));


        struct qreg x = tgsi_to_qir_frc(trans, NULL, 0, &scaled_x, 0);
        struct qreg x2 = qir_FMUL(c, x, x);
        struct qreg sum = qir_FMUL(c, x, qir_uniform_f(trans, coeff[0]));
        for (int i = 1; i < ARRAY_SIZE(coeff); i++) {
                x = qir_FMUL(c, x, x2);
                sum = qir_FADD(c,
                               sum,
                               qir_FMUL(c,
                                        x,
                                        qir_uniform_f(trans, coeff[i])));
        }
        return sum;
}

/* Note that this instruction replicates its result from the x channel */
static struct qreg
tgsi_to_qir_cos(struct tgsi_to_qir *trans,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qcompile *c = trans->c;
        float coeff[] = {
                1.0f,
                -pow(2.0 * M_PI, 2) / (2 * 1),
                pow(2.0 * M_PI, 4) / (4 * 3 * 2 * 1),
                -pow(2.0 * M_PI, 6) / (6 * 5 * 4 * 3 * 2 * 1),
        };

        struct qreg scaled_x =
                qir_FMUL(c, src[0 * 4 + 0],
                         qir_uniform_f(trans, 1.0f / (M_PI * 2.0f)));
        struct qreg x_frac = tgsi_to_qir_frc(trans, NULL, 0, &scaled_x, 0);

        struct qreg sum = qir_uniform_f(trans, coeff[0]);
        struct qreg x2 = qir_FMUL(c, x_frac, x_frac);
        struct qreg x = x2; /* Current x^2, x^4, or x^6 */
        for (int i = 1; i < ARRAY_SIZE(coeff); i++) {
                if (i != 1)
                        x = qir_FMUL(c, x, x2);

                struct qreg mul = qir_FMUL(c,
                                           x,
                                           qir_uniform_f(trans, coeff[i]));
                if (i == 0)
                        sum = mul;
                else
                        sum = qir_FADD(c, sum, mul);
        }
        return sum;
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

                if (swiz <= UTIL_FORMAT_SWIZZLE_W &&
                    !format_warned &&
                    (desc->channel[swiz].type != UTIL_FORMAT_TYPE_FLOAT ||
                     desc->channel[swiz].size != 32)) {
                        fprintf(stderr,
                                "vtx element %d unsupported type: %s\n",
                                attr, util_format_name(format));
                        format_warned = true;
                }

                trans->inputs[attr * 4 + i] =
                        get_swizzled_channel(trans, vpm_reads, swiz);
        }
}

static void
emit_fragcoord_input(struct tgsi_to_qir *trans, int attr)
{
        struct qcompile *c = trans->c;

        trans->inputs[attr * 4 + 0] = qir_FRAG_X(c);
        trans->inputs[attr * 4 + 1] = qir_FRAG_Y(c);
        trans->inputs[attr * 4 + 2] =
                qir_FMUL(c,
                         qir_FRAG_Z(c),
                         qir_uniform_f(trans, 1.0 / 0xffffff));
        trans->inputs[attr * 4 + 3] = qir_FRAG_RCP_W(c);
}

static struct qreg
emit_fragment_varying(struct tgsi_to_qir *trans, int index)
{
        struct qcompile *c = trans->c;

        struct qreg vary = {
                QFILE_VARY,
                index
        };

        /* XXX: multiply by W */
        return qir_VARY_ADD_C(c, qir_MOV(c, vary));
}

static void
emit_fragment_input(struct tgsi_to_qir *trans, int attr)
{
        struct qcompile *c = trans->c;

        for (int i = 0; i < 4; i++) {
                trans->inputs[attr * 4 + i] =
                        emit_fragment_varying(trans, attr * 4 + i);
                c->num_inputs++;
        }
}

static void
emit_tgsi_declaration(struct tgsi_to_qir *trans,
                      struct tgsi_full_declaration *decl)
{
        struct qcompile *c = trans->c;

        switch (decl->Declaration.File) {
        case TGSI_FILE_INPUT:
                for (int i = decl->Range.First;
                     i <= decl->Range.Last;
                     i++) {
                        if (c->stage == QSTAGE_FRAG) {
                                if (decl->Semantic.Name ==
                                    TGSI_SEMANTIC_POSITION) {
                                        emit_fragcoord_input(trans, i);
                                } else {
                                        emit_fragment_input(trans, i);
                                }
                        } else {
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
                [TGSI_OPCODE_FLR] = { 0, tgsi_to_qir_flr },
                [TGSI_OPCODE_SIN] = { 0, tgsi_to_qir_sin },
                [TGSI_OPCODE_COS] = { 0, tgsi_to_qir_cos },
        };
        static int asdf = 0;
        uint32_t tgsi_op = tgsi_inst->Instruction.Opcode;

        if (tgsi_op == TGSI_OPCODE_END)
                return;

        struct qreg src_regs[12];
        for (int s = 0; s < 3; s++) {
                for (int i = 0; i < 4; i++) {
                        src_regs[4 * s + i] =
                                get_src(trans, &tgsi_inst->Src[s].Register, i);
                }
        }

        switch (tgsi_op) {
        case TGSI_OPCODE_TEX:
        case TGSI_OPCODE_TXP:
        case TGSI_OPCODE_TXB:
                tgsi_to_qir_tex(trans, tgsi_inst,
                                op_trans[tgsi_op].op, src_regs);
                return;
        default:
                break;
        }

        if (tgsi_op > ARRAY_SIZE(op_trans) || !(op_trans[tgsi_op].func)) {
                fprintf(stderr, "unknown tgsi inst: ");
                tgsi_dump_instruction(tgsi_inst, asdf++);
                fprintf(stderr, "\n");
                abort();
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

static struct qreg
vc4_blend_channel(struct tgsi_to_qir *trans,
                  struct qreg *dst,
                  struct qreg *src,
                  struct qreg val,
                  unsigned factor,
                  int channel)
{
        struct qcompile *c = trans->c;

        switch(factor) {
        case PIPE_BLENDFACTOR_ONE:
                return val;
        case PIPE_BLENDFACTOR_SRC_COLOR:
                return qir_FMUL(c, val, src[channel]);
        case PIPE_BLENDFACTOR_SRC_ALPHA:
                return qir_FMUL(c, val, src[3]);
        case PIPE_BLENDFACTOR_DST_ALPHA:
                return qir_FMUL(c, val, dst[3]);
        case PIPE_BLENDFACTOR_DST_COLOR:
                return qir_FMUL(c, val, dst[channel]);
        case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
                return qir_FMIN(c, src[3], qir_FSUB(c,
                                                    qir_uniform_f(trans, 1.0),
                                                    dst[3]));
        case PIPE_BLENDFACTOR_CONST_COLOR:
                return qir_FMUL(c, val,
                                get_temp_for_uniform(trans,
                                                     QUNIFORM_BLEND_CONST_COLOR,
                                                     channel));
        case PIPE_BLENDFACTOR_CONST_ALPHA:
                return qir_FMUL(c, val,
                                get_temp_for_uniform(trans,
                                                     QUNIFORM_BLEND_CONST_COLOR,
                                                     3));
        case PIPE_BLENDFACTOR_ZERO:
                return qir_uniform_f(trans, 0.0);
        case PIPE_BLENDFACTOR_INV_SRC_COLOR:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(trans, 1.0),
                                                 src[channel]));
        case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(trans, 1.0),
                                                 src[3]));
        case PIPE_BLENDFACTOR_INV_DST_ALPHA:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(trans, 1.0),
                                                 dst[3]));
        case PIPE_BLENDFACTOR_INV_DST_COLOR:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(trans, 1.0),
                                                 dst[channel]));
        case PIPE_BLENDFACTOR_INV_CONST_COLOR:
                return qir_FMUL(c, val,
                                qir_FSUB(c, qir_uniform_f(trans, 1.0),
                                         get_temp_for_uniform(trans,
                                                              QUNIFORM_BLEND_CONST_COLOR,
                                                              channel)));
        case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
                return qir_FMUL(c, val,
                                qir_FSUB(c, qir_uniform_f(trans, 1.0),
                                         get_temp_for_uniform(trans,
                                                              QUNIFORM_BLEND_CONST_COLOR,
                                                              3)));

        default:
        case PIPE_BLENDFACTOR_SRC1_COLOR:
        case PIPE_BLENDFACTOR_SRC1_ALPHA:
        case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
        case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
                /* Unsupported. */
                fprintf(stderr, "Unknown blend factor %d\n", factor);
                return val;
        }
}

static struct qreg
vc4_blend_func(struct tgsi_to_qir *trans,
               struct qreg src, struct qreg dst,
               unsigned func)
{
        struct qcompile *c = trans->c;

        switch (func) {
        case PIPE_BLEND_ADD:
                return qir_FADD(c, src, dst);
        case PIPE_BLEND_SUBTRACT:
                return qir_FSUB(c, src, dst);
        case PIPE_BLEND_REVERSE_SUBTRACT:
                return qir_FSUB(c, dst, src);
        case PIPE_BLEND_MIN:
                return qir_FMIN(c, src, dst);
        case PIPE_BLEND_MAX:
                return qir_FMAX(c, src, dst);

        default:
                /* Unsupported. */
                fprintf(stderr, "Unknown blend func %d\n", func);
                return src;

        }
}

/**
 * Implements fixed function blending in shader code.
 *
 * VC4 doesn't have any hardware support for blending.  Instead, you read the
 * current contents of the destination from the tile buffer after having
 * waited for the scoreboard (which is handled by vc4_qpu_emit.c), then do
 * math using your output color and that destination value, and update the
 * output color appropriately.
 */
static void
vc4_blend(struct tgsi_to_qir *trans, struct qreg *result,
          struct qreg *dst_color, struct qreg *src_color)
{
        struct pipe_rt_blend_state *blend = &trans->fs_key->blend;

        if (!blend->blend_enable) {
                for (int i = 0; i < 4; i++)
                        result[i] = src_color[i];
                return;
        }

        struct qreg src_blend[4], dst_blend[4];
        for (int i = 0; i < 3; i++) {
                src_blend[i] = vc4_blend_channel(trans,
                                                 dst_color, src_color,
                                                 src_color[i],
                                                 blend->rgb_src_factor, i);
                dst_blend[i] = vc4_blend_channel(trans,
                                                 dst_color, src_color,
                                                 dst_color[i],
                                                 blend->rgb_dst_factor, i);
        }
        src_blend[3] = vc4_blend_channel(trans,
                                         dst_color, src_color,
                                         src_color[3],
                                         blend->alpha_src_factor, 3);
        dst_blend[3] = vc4_blend_channel(trans,
                                         dst_color, src_color,
                                         dst_color[3],
                                         blend->alpha_dst_factor, 3);

        for (int i = 0; i < 3; i++) {
                result[i] = vc4_blend_func(trans,
                                           src_blend[i], dst_blend[i],
                                           blend->rgb_func);
        }
        result[3] = vc4_blend_func(trans,
                                   src_blend[3], dst_blend[3],
                                   blend->alpha_func);
}

static void
emit_frag_end(struct tgsi_to_qir *trans)
{
        struct qcompile *c = trans->c;

        struct qreg t = qir_get_temp(c);

        const struct util_format_description *format_desc =
                util_format_description(trans->fs_key->color_format);

        struct qreg src_color[4] = {
                trans->outputs[0], trans->outputs[1],
                trans->outputs[2], trans->outputs[3],
        };

        struct qreg dst_color[4] = { c->undef, c->undef, c->undef, c->undef };
        if (trans->fs_key->blend.blend_enable ||
            trans->fs_key->blend.colormask != 0xf) {
                qir_emit(c, qir_inst(QOP_TLB_COLOR_READ, c->undef,
                                     c->undef, c->undef));
                for (int i = 0; i < 4; i++) {
                        dst_color[i] = qir_R4_UNPACK(c, i);

                        /* XXX: Swizzles? */
                }
        }

        struct qreg blend_color[4];
        vc4_blend(trans, blend_color, dst_color, src_color);

        /* If the bit isn't set in the color mask, then just return the
         * original dst color, instead.
         */
        for (int i = 0; i < 4; i++) {
                if (!(trans->fs_key->blend.colormask & (1 << i))) {
                        blend_color[i] = dst_color[i];
                }
        }

        /* Debug: Sometimes you're getting a black output and just want to see
         * if the FS is getting executed at all.  Spam magenta into the color
         * output.
         */
        if (0) {
                blend_color[0] = qir_uniform_f(trans, 1.0);
                blend_color[1] = qir_uniform_f(trans, 0.0);
                blend_color[2] = qir_uniform_f(trans, 1.0);
                blend_color[3] = qir_uniform_f(trans, 0.5);
        }

        struct qreg swizzled_outputs[4];
        for (int i = 0; i < 4; i++) {
                swizzled_outputs[i] =
                        get_swizzled_channel(trans, blend_color,
                                             format_desc->swizzle[i]);
        }

        if (trans->fs_key->depth_enabled) {
                qir_emit(c, qir_inst(QOP_TLB_PASSTHROUGH_Z_WRITE, c->undef,
                                     c->undef, c->undef));
        }

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

        struct qreg zscale = add_uniform(trans, QUNIFORM_VIEWPORT_Z_SCALE, 0);
        struct qreg zoffset = add_uniform(trans, QUNIFORM_VIEWPORT_Z_OFFSET, 0);

        qir_VPM_WRITE(c, qir_FMUL(c, qir_FADD(c, qir_FMUL(c,
                                                          trans->outputs[2],
                                                          zscale),
                                              zoffset),
                                  rcp_w));
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
        ret = tgsi_parse_init(&trans->parser, trans->shader_state->tokens);
        assert(ret == TGSI_PARSE_OK);

        if (vc4_debug & VC4_DEBUG_TGSI) {
                fprintf(stderr, "TGSI:\n");
                tgsi_dump(trans->shader_state->tokens, 0);
        }

        trans->key = key;
        switch (stage) {
        case QSTAGE_FRAG:
                trans->fs_key = (struct vc4_fs_key *)key;
                if (trans->fs_key->is_points) {
                        trans->point_x = emit_fragment_varying(trans, 0);
                        trans->point_y = emit_fragment_varying(trans, 0);
                } else if (trans->fs_key->is_lines) {
                        trans->line_x = emit_fragment_varying(trans, 0);
                }
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
        struct pipe_shader_state *so = CALLOC_STRUCT(pipe_shader_state);
        if (!so)
                return NULL;

        so->tokens = tgsi_dup_tokens(cso->tokens);

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
        uinfo->num_texture_samples = trans->num_texture_samples;
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
vc4_setup_shared_key(struct vc4_key *key, struct vc4_texture_stateobj *texstate)
{
        for (int i = 0; i < texstate->num_textures; i++) {
                struct pipe_sampler_view *sampler = texstate->textures[i];
                if (sampler) {
                        struct pipe_resource *prsc = sampler->texture;
                        key->tex_format[i] = prsc->format;
                }
        }
}

static void
vc4_update_compiled_fs(struct vc4_context *vc4, uint8_t prim_mode)
{
        struct vc4_fs_key local_key;
        struct vc4_fs_key *key = &local_key;

        memset(key, 0, sizeof(*key));
        vc4_setup_shared_key(&key->base, &vc4->fragtex);
        key->base.shader_state = vc4->prog.bind_fs;
        key->is_points = (prim_mode == PIPE_PRIM_POINTS);
        key->is_lines = (prim_mode >= PIPE_PRIM_LINES &&
                         prim_mode <= PIPE_PRIM_LINE_STRIP);
        key->blend = vc4->blend->rt[0];

        if (vc4->framebuffer.cbufs[0])
                key->color_format = vc4->framebuffer.cbufs[0]->format;

        key->depth_enabled = vc4->zsa->base.depth.enabled;

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
        vc4_setup_shared_key(&key->base, &vc4->verttex);
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
vc4_update_compiled_shaders(struct vc4_context *vc4, uint8_t prim_mode)
{
        vc4_update_compiled_fs(vc4, prim_mode);
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
        struct pipe_shader_state *shader_state;
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
        struct pipe_shader_state *so = hwcso;
        struct delete_state del;

        del.vc4 = vc4;
        del.shader_state = so;
        util_hash_table_foreach(vc4->fs_cache, fs_delete_from_cache, &del);
        util_hash_table_foreach(vc4->vs_cache, vs_delete_from_cache, &del);

        free((void *)so->tokens);
        free(so);
}

static uint32_t translate_wrap(uint32_t p_wrap)
{
        switch (p_wrap) {
        case PIPE_TEX_WRAP_REPEAT:
                return 0;
        case PIPE_TEX_WRAP_CLAMP:
        case PIPE_TEX_WRAP_CLAMP_TO_EDGE:
                return 1;
        case PIPE_TEX_WRAP_MIRROR_REPEAT:
                return 2;
        case PIPE_TEX_WRAP_CLAMP_TO_BORDER:
                return 3;
        default:
                fprintf(stderr, "Unknown wrap mode %d\n", p_wrap);
                assert(!"not reached");
                return 0;
        }
}

static void
write_texture_p0(struct vc4_context *vc4,
                 struct vc4_texture_stateobj *texstate,
                 uint32_t unit)
{
        struct pipe_sampler_view *texture = texstate->textures[unit];
        struct vc4_resource *rsc = vc4_resource(texture->texture);

        cl_reloc(vc4, &vc4->uniforms, rsc->bo,
                 rsc->slices[0].offset | texture->u.tex.last_level);
}

static void
write_texture_p1(struct vc4_context *vc4,
                 struct vc4_texture_stateobj *texstate,
                 uint32_t unit)
{
        struct pipe_sampler_view *texture = texstate->textures[unit];
        struct pipe_sampler_state *sampler = texstate->samplers[unit];
        static const uint32_t mipfilter_map[] = {
                [PIPE_TEX_MIPFILTER_NEAREST] = 2,
                [PIPE_TEX_MIPFILTER_LINEAR] = 4,
                [PIPE_TEX_MIPFILTER_NONE] = 0
        };
        static const uint32_t imgfilter_map[] = {
                [PIPE_TEX_FILTER_NEAREST] = 1,
                [PIPE_TEX_FILTER_LINEAR] = 0,
        };

        cl_u32(&vc4->uniforms,
               (1 << 31) /* XXX: data type */|
               (texture->texture->height0 << 20) |
               (texture->texture->width0 << 8) |
               (imgfilter_map[sampler->mag_img_filter] << 7) |
               ((imgfilter_map[sampler->min_img_filter] +
                 mipfilter_map[sampler->min_mip_filter]) << 4) |
               (translate_wrap(sampler->wrap_t) << 2) |
               (translate_wrap(sampler->wrap_s) << 0));
}

static uint32_t
get_texrect_scale(struct vc4_texture_stateobj *texstate,
                  enum quniform_contents contents,
                  uint32_t data)
{
        struct pipe_sampler_view *texture = texstate->textures[data];
        uint32_t dim;

        if (contents == QUNIFORM_TEXRECT_SCALE_X)
                dim = texture->texture->width0;
        else
                dim = texture->texture->height0;

        return fui(1.0f / dim);
}

void
vc4_write_uniforms(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
                   struct vc4_constbuf_stateobj *cb,
                   struct vc4_texture_stateobj *texstate,
                   int shader_index)
{
        struct vc4_shader_uniform_info *uinfo = &shader->uniforms[shader_index];
        const uint32_t *gallium_uniforms = cb->cb[0].user_buffer;

        cl_start_shader_reloc(&vc4->uniforms, uinfo->num_texture_samples);

        for (int i = 0; i < uinfo->count; i++) {

                switch (uinfo->contents[i]) {
                case QUNIFORM_CONSTANT:
                        cl_u32(&vc4->uniforms, uinfo->data[i]);
                        break;
                case QUNIFORM_UNIFORM:
                        cl_u32(&vc4->uniforms,
                               gallium_uniforms[uinfo->data[i]]);
                        break;
                case QUNIFORM_VIEWPORT_X_SCALE:
                        cl_f(&vc4->uniforms, vc4->viewport.scale[0] * 16.0f);
                        break;
                case QUNIFORM_VIEWPORT_Y_SCALE:
                        cl_f(&vc4->uniforms, vc4->viewport.scale[1] * 16.0f);
                        break;

                case QUNIFORM_VIEWPORT_Z_OFFSET:
                        cl_f(&vc4->uniforms, vc4->viewport.translate[2]);
                        break;
                case QUNIFORM_VIEWPORT_Z_SCALE:
                        cl_f(&vc4->uniforms, vc4->viewport.scale[2]);
                        break;

                case QUNIFORM_TEXTURE_CONFIG_P0:
                        write_texture_p0(vc4, texstate, uinfo->data[i]);
                        break;

                case QUNIFORM_TEXTURE_CONFIG_P1:
                        write_texture_p1(vc4, texstate, uinfo->data[i]);
                        break;

                case QUNIFORM_TEXRECT_SCALE_X:
                case QUNIFORM_TEXRECT_SCALE_Y:
                        cl_u32(&vc4->uniforms,
                               get_texrect_scale(texstate,
                                                 uinfo->contents[i],
                                                 uinfo->data[i]));
                        break;

                case QUNIFORM_BLEND_CONST_COLOR:
                        cl_f(&vc4->uniforms,
                             vc4->blend_color.color[uinfo->data[i]]);
                        break;
                }
#if 0
                uint32_t written_val = *(uint32_t *)(vc4->uniforms.next - 4);
                fprintf(stderr, "%p/%d: %d: 0x%08x (%f)\n",
                        shader, shader_index, i, written_val, uif(written_val));
#endif
        }
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
