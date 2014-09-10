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

#include <inttypes.h>
#include "pipe/p_state.h"
#include "util/u_format.h"
#include "util/u_hash_table.h"
#include "util/u_hash.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_info.h"

#include "vc4_context.h"
#include "vc4_qpu.h"
#include "vc4_qir.h"
#ifdef USE_VC4_SIMULATOR
#include "simpenrose/simpenrose.h"
#endif

struct vc4_key {
        struct pipe_shader_state *shader_state;
        struct {
                enum pipe_format format;
                unsigned compare_mode:1;
                unsigned compare_func:3;
                uint8_t swizzle[4];
        } tex[VC4_MAX_TEXTURE_SAMPLERS];
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
add_uniform(struct vc4_compile *c,
            enum quniform_contents contents,
            uint32_t data)
{
        uint32_t uniform = c->num_uniforms++;
        struct qreg u = { QFILE_UNIF, uniform };

        c->uniform_contents[uniform] = contents;
        c->uniform_data[uniform] = data;

        return u;
}

static struct qreg
get_temp_for_uniform(struct vc4_compile *c, enum quniform_contents contents,
                     uint32_t data)
{
        for (int i = 0; i < c->num_uniforms; i++) {
                if (c->uniform_contents[i] == contents &&
                    c->uniform_data[i] == data)
                        return c->uniforms[i];
        }

        struct qreg u = add_uniform(c, contents, data);
        struct qreg t = qir_MOV(c, u);

        c->uniforms[u.index] = t;
        return t;
}

static struct qreg
qir_uniform_ui(struct vc4_compile *c, uint32_t ui)
{
        return get_temp_for_uniform(c, QUNIFORM_CONSTANT, ui);
}

static struct qreg
qir_uniform_f(struct vc4_compile *c, float f)
{
        return qir_uniform_ui(c, fui(f));
}

static struct qreg
get_src(struct vc4_compile *c, unsigned tgsi_op,
        struct tgsi_src_register *src, int i)
{
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
                r = c->temps[src->Index * 4 + s];
                break;
        case TGSI_FILE_IMMEDIATE:
                r = c->consts[src->Index * 4 + s];
                break;
        case TGSI_FILE_CONSTANT:
                r = get_temp_for_uniform(c, QUNIFORM_UNIFORM,
                                         src->Index * 4 + s);
                break;
        case TGSI_FILE_INPUT:
                r = c->inputs[src->Index * 4 + s];
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

        if (src->Negate) {
                switch (tgsi_opcode_infer_src_type(tgsi_op)) {
                case TGSI_TYPE_SIGNED:
                case TGSI_TYPE_UNSIGNED:
                        r = qir_SUB(c, qir_uniform_ui(c, 0), r);
                        break;
                default:
                        r = qir_FSUB(c, qir_uniform_f(c, 0.0), r);
                        break;
                }
        }

        return r;
};


static void
update_dst(struct vc4_compile *c, struct tgsi_full_instruction *tgsi_inst,
           int i, struct qreg val)
{
        struct tgsi_dst_register *tgsi_dst = &tgsi_inst->Dst[0].Register;

        assert(!tgsi_dst->Indirect);

        switch (tgsi_dst->File) {
        case TGSI_FILE_TEMPORARY:
                c->temps[tgsi_dst->Index * 4 + i] = val;
                break;
        case TGSI_FILE_OUTPUT:
                c->outputs[tgsi_dst->Index * 4 + i] = val;
                c->num_outputs = MAX2(c->num_outputs,
                                      tgsi_dst->Index * 4 + i + 1);
                break;
        default:
                fprintf(stderr, "unknown dst file %d\n", tgsi_dst->File);
                abort();
        }
};

static struct qreg
get_swizzled_channel(struct vc4_compile *c,
                     struct qreg *srcs, int swiz)
{
        switch (swiz) {
        default:
        case UTIL_FORMAT_SWIZZLE_NONE:
                fprintf(stderr, "warning: unknown swizzle\n");
                /* FALLTHROUGH */
        case UTIL_FORMAT_SWIZZLE_0:
                return qir_uniform_f(c, 0.0);
        case UTIL_FORMAT_SWIZZLE_1:
                return qir_uniform_f(c, 1.0);
        case UTIL_FORMAT_SWIZZLE_X:
        case UTIL_FORMAT_SWIZZLE_Y:
        case UTIL_FORMAT_SWIZZLE_Z:
        case UTIL_FORMAT_SWIZZLE_W:
                return srcs[swiz];
        }
}

static struct qreg
tgsi_to_qir_alu(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qreg dst = qir_get_temp(c);
        qir_emit(c, qir_inst4(op, dst,
                              src[0 * 4 + i],
                              src[1 * 4 + i],
                              src[2 * 4 + i],
                              c->undef));
        return dst;
}

static struct qreg
tgsi_to_qir_umul(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        struct qreg src0_hi = qir_SHR(c, src[0 * 4 + i],
                                      qir_uniform_ui(c, 16));
        struct qreg src0_lo = qir_AND(c, src[0 * 4 + i],
                                      qir_uniform_ui(c, 0xffff));
        struct qreg src1_hi = qir_SHR(c, src[1 * 4 + i],
                                      qir_uniform_ui(c, 16));
        struct qreg src1_lo = qir_AND(c, src[1 * 4 + i],
                                      qir_uniform_ui(c, 0xffff));

        struct qreg hilo = qir_MUL24(c, src0_hi, src1_lo);
        struct qreg lohi = qir_MUL24(c, src0_lo, src1_hi);
        struct qreg lolo = qir_MUL24(c, src0_lo, src1_lo);

        return qir_ADD(c, lolo, qir_SHL(c,
                                        qir_ADD(c, hilo, lohi),
                                        qir_uniform_ui(c, 16)));
}

static struct qreg
tgsi_to_qir_idiv(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return qir_FTOI(c, qir_FMUL(c,
                                    qir_ITOF(c, src[0 * 4 + i]),
                                    qir_RCP(c, qir_ITOF(c, src[1 * 4 + i]))));
}

static struct qreg
tgsi_to_qir_ineg(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return qir_SUB(c, qir_uniform_ui(c, 0), src[0 * 4 + i]);
}

static struct qreg
tgsi_to_qir_seq(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_ZS(c, qir_uniform_f(c, 1.0));
}

static struct qreg
tgsi_to_qir_sne(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_ZC(c, qir_uniform_f(c, 1.0));
}

static struct qreg
tgsi_to_qir_slt(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_NS(c, qir_uniform_f(c, 1.0));
}

static struct qreg
tgsi_to_qir_sge(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_NC(c, qir_uniform_f(c, 1.0));
}

static struct qreg
tgsi_to_qir_fseq(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_ZS(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_fsne(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_ZC(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_fslt(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_NS(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_fsge(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_NC(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_useq(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_SUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_ZS(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_usne(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_SUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_ZC(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_islt(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_SUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_NS(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_isge(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        qir_SF(c, qir_SUB(c, src[0 * 4 + i], src[1 * 4 + i]));
        return qir_SEL_X_0_NC(c, qir_uniform_ui(c, ~0));
}

static struct qreg
tgsi_to_qir_cmp(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        qir_SF(c, src[0 * 4 + i]);
        return qir_SEL_X_Y_NS(c,
                              src[1 * 4 + i],
                              src[2 * 4 + i]);
}

static struct qreg
tgsi_to_qir_mad(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        return qir_FADD(c,
                        qir_FMUL(c,
                                 src[0 * 4 + i],
                                 src[1 * 4 + i]),
                        src[2 * 4 + i]);
}

static struct qreg
tgsi_to_qir_lit(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        struct qreg x = src[0 * 4 + 0];
        struct qreg y = src[0 * 4 + 1];
        struct qreg w = src[0 * 4 + 3];

        switch (i) {
        case 0:
        case 3:
                return qir_uniform_f(c, 1.0);
        case 1:
                return qir_FMAX(c, src[0 * 4 + 0], qir_uniform_f(c, 0.0));
        case 2: {
                struct qreg zero = qir_uniform_f(c, 0.0);

                qir_SF(c, x);
                /* XXX: Clamp w to -128..128 */
                return qir_SEL_X_0_NC(c,
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
tgsi_to_qir_lrp(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
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
tgsi_to_qir_tex(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src)
{
        assert(!tgsi_inst->Instruction.Saturate);

        struct qreg s = src[0 * 4 + 0];
        struct qreg t = src[0 * 4 + 1];
        uint32_t unit = tgsi_inst->Src[1].Register.Index;

        struct qreg proj = c->undef;
        if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXP) {
                proj = qir_RCP(c, src[0 * 4 + 3]);
                s = qir_FMUL(c, s, proj);
                t = qir_FMUL(c, t, proj);
        }

        /* There is no native support for GL texture rectangle coordinates, so
         * we have to rescale from ([0, width], [0, height]) to ([0, 1], [0,
         * 1]).
         */
        if (tgsi_inst->Texture.Texture == TGSI_TEXTURE_RECT ||
            tgsi_inst->Texture.Texture == TGSI_TEXTURE_SHADOWRECT) {
                s = qir_FMUL(c, s,
                             get_temp_for_uniform(c,
                                                  QUNIFORM_TEXRECT_SCALE_X,
                                                  unit));
                t = qir_FMUL(c, t,
                             get_temp_for_uniform(c,
                                                  QUNIFORM_TEXRECT_SCALE_Y,
                                                  unit));
        }

        qir_TEX_T(c, t, add_uniform(c, QUNIFORM_TEXTURE_CONFIG_P0, unit));

        struct qreg sampler_p1 = add_uniform(c, QUNIFORM_TEXTURE_CONFIG_P1,
                                             unit);
        if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXB) {
                qir_TEX_B(c, src[0 * 4 + 3], sampler_p1);
                qir_TEX_S(c, s, add_uniform(c, QUNIFORM_CONSTANT, 0));
        } else {
                qir_TEX_S(c, s, sampler_p1);
        }

        c->num_texture_samples++;
        struct qreg r4 = qir_TEX_RESULT(c);

        enum pipe_format format = c->key->tex[unit].format;

        struct qreg unpacked[4];
        if (util_format_is_depth_or_stencil(format)) {
                struct qreg depthf = qir_ITOF(c, qir_SHR(c, r4,
                                                         qir_uniform_ui(c, 8)));
                struct qreg normalized = qir_FMUL(c, depthf,
                                                  qir_uniform_f(c, 1.0f/0xffffff));

                struct qreg depth_output;

                struct qreg compare = src[0 * 4 + 2];

                if (tgsi_inst->Instruction.Opcode == TGSI_OPCODE_TXP)
                        compare = qir_FMUL(c, compare, proj);

                struct qreg one = qir_uniform_f(c, 1.0f);
                if (c->key->tex[unit].compare_mode) {
                        switch (c->key->tex[unit].compare_func) {
                        case PIPE_FUNC_NEVER:
                                depth_output = qir_uniform_f(c, 0.0f);
                                break;
                        case PIPE_FUNC_ALWAYS:
                                depth_output = one;
                                break;
                        case PIPE_FUNC_EQUAL:
                                qir_SF(c, qir_FSUB(c, compare, normalized));
                                depth_output = qir_SEL_X_0_ZS(c, one);
                                break;
                        case PIPE_FUNC_NOTEQUAL:
                                qir_SF(c, qir_FSUB(c, compare, normalized));
                                depth_output = qir_SEL_X_0_ZC(c, one);
                                break;
                        case PIPE_FUNC_GREATER:
                                qir_SF(c, qir_FSUB(c, compare, normalized));
                                depth_output = qir_SEL_X_0_NC(c, one);
                                break;
                        case PIPE_FUNC_GEQUAL:
                                qir_SF(c, qir_FSUB(c, normalized, compare));
                                depth_output = qir_SEL_X_0_NS(c, one);
                                break;
                        case PIPE_FUNC_LESS:
                                qir_SF(c, qir_FSUB(c, compare, normalized));
                                depth_output = qir_SEL_X_0_NS(c, one);
                                break;
                        case PIPE_FUNC_LEQUAL:
                                qir_SF(c, qir_FSUB(c, normalized, compare));
                                depth_output = qir_SEL_X_0_NC(c, one);
                                break;
                        }
                } else {
                        depth_output = normalized;
                }

                for (int i = 0; i < 4; i++)
                        unpacked[i] = depth_output;
        } else {
                for (int i = 0; i < 4; i++)
                        unpacked[i] = qir_R4_UNPACK(c, r4, i);
        }

        const uint8_t *format_swiz = vc4_get_format_swizzle(format);
        uint8_t swiz[4];
        util_format_compose_swizzles(format_swiz, c->key->tex[unit].swizzle, swiz);
        for (int i = 0; i < 4; i++) {
                if (!(tgsi_inst->Dst[0].Register.WriteMask & (1 << i)))
                        continue;

                update_dst(c, tgsi_inst, i,
                           get_swizzled_channel(c, unpacked, swiz[i]));
        }
}

static struct qreg
tgsi_to_qir_pow(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        /* Note that this instruction replicates its result from the x channel
         */
        return qir_EXP2(c, qir_FMUL(c,
                                    src[1 * 4 + 0],
                                    qir_LOG2(c, src[0 * 4 + 0])));
}

static struct qreg
tgsi_to_qir_trunc(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        return qir_ITOF(c, qir_FTOI(c, src[0 * 4 + i]));
}

/**
 * Computes x - floor(x), which is tricky because our FTOI truncates (rounds
 * to zero).
 */
static struct qreg
tgsi_to_qir_frc(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qreg trunc = qir_ITOF(c, qir_FTOI(c, src[0 * 4 + i]));
        struct qreg diff = qir_FSUB(c, src[0 * 4 + i], trunc);
        qir_SF(c, diff);
        return qir_SEL_X_Y_NS(c,
                              qir_FADD(c, diff, qir_uniform_f(c, 1.0)),
                              diff);
}

/**
 * Computes floor(x), which is tricky because our FTOI truncates (rounds to
 * zero).
 */
static struct qreg
tgsi_to_qir_flr(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qreg trunc = qir_ITOF(c, qir_FTOI(c, src[0 * 4 + i]));

        /* This will be < 0 if we truncated and the truncation was of a value
         * that was < 0 in the first place.
         */
        qir_SF(c, qir_FSUB(c, src[0 * 4 + i], trunc));

        return qir_SEL_X_Y_NS(c,
                              qir_FSUB(c, trunc, qir_uniform_f(c, 1.0)),
                              trunc);
}

static struct qreg
tgsi_to_qir_dp(struct vc4_compile *c,
               struct tgsi_full_instruction *tgsi_inst,
               int num, struct qreg *src, int i)
{
        struct qreg sum = qir_FMUL(c, src[0 * 4 + 0], src[1 * 4 + 0]);
        for (int j = 1; j < num; j++) {
                sum = qir_FADD(c, sum, qir_FMUL(c,
                                                src[0 * 4 + j],
                                                src[1 * 4 + j]));
        }
        return sum;
}

static struct qreg
tgsi_to_qir_dp2(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return tgsi_to_qir_dp(c, tgsi_inst, 2, src, i);
}

static struct qreg
tgsi_to_qir_dp3(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return tgsi_to_qir_dp(c, tgsi_inst, 3, src, i);
}

static struct qreg
tgsi_to_qir_dp4(struct vc4_compile *c,
                 struct tgsi_full_instruction *tgsi_inst,
                 enum qop op, struct qreg *src, int i)
{
        return tgsi_to_qir_dp(c, tgsi_inst, 4, src, i);
}

static struct qreg
tgsi_to_qir_abs(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        struct qreg arg = src[0 * 4 + i];
        return qir_FMAXABS(c, arg, arg);
}

/* Note that this instruction replicates its result from the x channel */
static struct qreg
tgsi_to_qir_sin(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        float coeff[] = {
                2.0 * M_PI,
                -pow(2.0 * M_PI, 3) / (3 * 2 * 1),
                pow(2.0 * M_PI, 5) / (5 * 4 * 3 * 2 * 1),
                -pow(2.0 * M_PI, 7) / (7 * 6 * 5 * 4 * 3 * 2 * 1),
        };

        struct qreg scaled_x =
                qir_FMUL(c,
                         src[0 * 4 + 0],
                         qir_uniform_f(c, 1.0f / (M_PI * 2.0f)));


        struct qreg x = tgsi_to_qir_frc(c, NULL, 0, &scaled_x, 0);
        struct qreg x2 = qir_FMUL(c, x, x);
        struct qreg sum = qir_FMUL(c, x, qir_uniform_f(c, coeff[0]));
        for (int i = 1; i < ARRAY_SIZE(coeff); i++) {
                x = qir_FMUL(c, x, x2);
                sum = qir_FADD(c,
                               sum,
                               qir_FMUL(c,
                                        x,
                                        qir_uniform_f(c, coeff[i])));
        }
        return sum;
}

/* Note that this instruction replicates its result from the x channel */
static struct qreg
tgsi_to_qir_cos(struct vc4_compile *c,
                struct tgsi_full_instruction *tgsi_inst,
                enum qop op, struct qreg *src, int i)
{
        float coeff[] = {
                1.0f,
                -pow(2.0 * M_PI, 2) / (2 * 1),
                pow(2.0 * M_PI, 4) / (4 * 3 * 2 * 1),
                -pow(2.0 * M_PI, 6) / (6 * 5 * 4 * 3 * 2 * 1),
        };

        struct qreg scaled_x =
                qir_FMUL(c, src[0 * 4 + 0],
                         qir_uniform_f(c, 1.0f / (M_PI * 2.0f)));
        struct qreg x_frac = tgsi_to_qir_frc(c, NULL, 0, &scaled_x, 0);

        struct qreg sum = qir_uniform_f(c, coeff[0]);
        struct qreg x2 = qir_FMUL(c, x_frac, x_frac);
        struct qreg x = x2; /* Current x^2, x^4, or x^6 */
        for (int i = 1; i < ARRAY_SIZE(coeff); i++) {
                if (i != 1)
                        x = qir_FMUL(c, x, x2);

                struct qreg mul = qir_FMUL(c,
                                           x,
                                           qir_uniform_f(c, coeff[i]));
                if (i == 0)
                        sum = mul;
                else
                        sum = qir_FADD(c, sum, mul);
        }
        return sum;
}

static void
emit_vertex_input(struct vc4_compile *c, int attr)
{
        enum pipe_format format = c->vs_key->attr_formats[attr];
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

                c->inputs[attr * 4 + i] =
                        get_swizzled_channel(c, vpm_reads, swiz);
        }
}

static void
tgsi_to_qir_kill_if(struct vc4_compile *c, struct qreg *src, int i)
{
        if (c->discard.file == QFILE_NULL)
                c->discard = qir_uniform_f(c, 0.0);
        qir_SF(c, src[0 * 4 + i]);
        c->discard = qir_SEL_X_Y_NS(c, qir_uniform_f(c, 1.0),
                                    c->discard);
}

static void
emit_fragcoord_input(struct vc4_compile *c, int attr)
{
        c->inputs[attr * 4 + 0] = qir_FRAG_X(c);
        c->inputs[attr * 4 + 1] = qir_FRAG_Y(c);
        c->inputs[attr * 4 + 2] =
                qir_FMUL(c,
                         qir_FRAG_Z(c),
                         qir_uniform_f(c, 1.0 / 0xffffff));
        c->inputs[attr * 4 + 3] = qir_FRAG_RCP_W(c);
}

static struct qreg
emit_fragment_varying(struct vc4_compile *c, int index)
{
        struct qreg vary = {
                QFILE_VARY,
                index
        };

        /* XXX: multiply by W */
        return qir_VARY_ADD_C(c, qir_MOV(c, vary));
}

static void
emit_fragment_input(struct vc4_compile *c, int attr)
{
        for (int i = 0; i < 4; i++) {
                c->inputs[attr * 4 + i] =
                        emit_fragment_varying(c, attr * 4 + i);
                c->num_inputs++;
        }
}

static void
emit_tgsi_declaration(struct vc4_compile *c,
                      struct tgsi_full_declaration *decl)
{
        switch (decl->Declaration.File) {
        case TGSI_FILE_INPUT:
                for (int i = decl->Range.First;
                     i <= decl->Range.Last;
                     i++) {
                        if (c->stage == QSTAGE_FRAG) {
                                if (decl->Semantic.Name ==
                                    TGSI_SEMANTIC_POSITION) {
                                        emit_fragcoord_input(c, i);
                                } else {
                                        emit_fragment_input(c, i);
                                }
                        } else {
                                emit_vertex_input(c, i);
                        }
                }
                break;
        }
}

static void
emit_tgsi_instruction(struct vc4_compile *c,
                      struct tgsi_full_instruction *tgsi_inst)
{
        struct {
                enum qop op;
                struct qreg (*func)(struct vc4_compile *c,
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
                [TGSI_OPCODE_F2I] = { QOP_FTOI, tgsi_to_qir_alu },
                [TGSI_OPCODE_I2F] = { QOP_ITOF, tgsi_to_qir_alu },
                [TGSI_OPCODE_UADD] = { QOP_ADD, tgsi_to_qir_alu },
                [TGSI_OPCODE_USHR] = { QOP_SHR, tgsi_to_qir_alu },
                [TGSI_OPCODE_ISHR] = { QOP_ASR, tgsi_to_qir_alu },
                [TGSI_OPCODE_SHL] = { QOP_SHL, tgsi_to_qir_alu },
                [TGSI_OPCODE_IMIN] = { QOP_MIN, tgsi_to_qir_alu },
                [TGSI_OPCODE_IMAX] = { QOP_MAX, tgsi_to_qir_alu },
                [TGSI_OPCODE_AND] = { QOP_AND, tgsi_to_qir_alu },
                [TGSI_OPCODE_OR] = { QOP_OR, tgsi_to_qir_alu },
                [TGSI_OPCODE_XOR] = { QOP_XOR, tgsi_to_qir_alu },
                [TGSI_OPCODE_NOT] = { QOP_NOT, tgsi_to_qir_alu },

                [TGSI_OPCODE_UMUL] = { 0, tgsi_to_qir_umul },
                [TGSI_OPCODE_IDIV] = { 0, tgsi_to_qir_idiv },
                [TGSI_OPCODE_INEG] = { 0, tgsi_to_qir_ineg },

                [TGSI_OPCODE_RSQ] = { QOP_RSQ, tgsi_to_qir_alu },
                [TGSI_OPCODE_SEQ] = { 0, tgsi_to_qir_seq },
                [TGSI_OPCODE_SNE] = { 0, tgsi_to_qir_sne },
                [TGSI_OPCODE_SGE] = { 0, tgsi_to_qir_sge },
                [TGSI_OPCODE_SLT] = { 0, tgsi_to_qir_slt },
                [TGSI_OPCODE_FSEQ] = { 0, tgsi_to_qir_fseq },
                [TGSI_OPCODE_FSNE] = { 0, tgsi_to_qir_fsne },
                [TGSI_OPCODE_FSGE] = { 0, tgsi_to_qir_fsge },
                [TGSI_OPCODE_FSLT] = { 0, tgsi_to_qir_fslt },
                [TGSI_OPCODE_USEQ] = { 0, tgsi_to_qir_useq },
                [TGSI_OPCODE_USNE] = { 0, tgsi_to_qir_usne },
                [TGSI_OPCODE_ISGE] = { 0, tgsi_to_qir_isge },
                [TGSI_OPCODE_ISLT] = { 0, tgsi_to_qir_islt },

                [TGSI_OPCODE_CMP] = { 0, tgsi_to_qir_cmp },
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
                                get_src(c, tgsi_inst->Instruction.Opcode,
                                        &tgsi_inst->Src[s].Register, i);
                }
        }

        switch (tgsi_op) {
        case TGSI_OPCODE_TEX:
        case TGSI_OPCODE_TXP:
        case TGSI_OPCODE_TXB:
                tgsi_to_qir_tex(c, tgsi_inst,
                                op_trans[tgsi_op].op, src_regs);
                return;
        case TGSI_OPCODE_KILL:
                c->discard = qir_uniform_f(c, 1.0);
                return;
        case TGSI_OPCODE_KILL_IF:
                for (int i = 0; i < 4; i++)
                        tgsi_to_qir_kill_if(c, src_regs, i);
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

                result = op_trans[tgsi_op].func(c, tgsi_inst,
                                                op_trans[tgsi_op].op,
                                                src_regs, i);

                if (tgsi_inst->Instruction.Saturate) {
                        float low = (tgsi_inst->Instruction.Saturate ==
                                     TGSI_SAT_MINUS_PLUS_ONE ? -1.0 : 0.0);
                        result = qir_FMAX(c,
                                          qir_FMIN(c,
                                                   result,
                                                   qir_uniform_f(c, 1.0)),
                                          qir_uniform_f(c, low));
                }

                update_dst(c, tgsi_inst, i, result);
        }
}

static void
parse_tgsi_immediate(struct vc4_compile *c, struct tgsi_full_immediate *imm)
{
        for (int i = 0; i < 4; i++) {
                unsigned n = c->num_consts++;
                c->consts[n] = qir_uniform_ui(c, imm->u[i].Uint);
        }
}

static struct qreg
vc4_blend_channel(struct vc4_compile *c,
                  struct qreg *dst,
                  struct qreg *src,
                  struct qreg val,
                  unsigned factor,
                  int channel)
{
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
                                                    qir_uniform_f(c, 1.0),
                                                    dst[3]));
        case PIPE_BLENDFACTOR_CONST_COLOR:
                return qir_FMUL(c, val,
                                get_temp_for_uniform(c,
                                                     QUNIFORM_BLEND_CONST_COLOR,
                                                     channel));
        case PIPE_BLENDFACTOR_CONST_ALPHA:
                return qir_FMUL(c, val,
                                get_temp_for_uniform(c,
                                                     QUNIFORM_BLEND_CONST_COLOR,
                                                     3));
        case PIPE_BLENDFACTOR_ZERO:
                return qir_uniform_f(c, 0.0);
        case PIPE_BLENDFACTOR_INV_SRC_COLOR:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(c, 1.0),
                                                 src[channel]));
        case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(c, 1.0),
                                                 src[3]));
        case PIPE_BLENDFACTOR_INV_DST_ALPHA:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(c, 1.0),
                                                 dst[3]));
        case PIPE_BLENDFACTOR_INV_DST_COLOR:
                return qir_FMUL(c, val, qir_FSUB(c, qir_uniform_f(c, 1.0),
                                                 dst[channel]));
        case PIPE_BLENDFACTOR_INV_CONST_COLOR:
                return qir_FMUL(c, val,
                                qir_FSUB(c, qir_uniform_f(c, 1.0),
                                         get_temp_for_uniform(c,
                                                              QUNIFORM_BLEND_CONST_COLOR,
                                                              channel)));
        case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
                return qir_FMUL(c, val,
                                qir_FSUB(c, qir_uniform_f(c, 1.0),
                                         get_temp_for_uniform(c,
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
vc4_blend_func(struct vc4_compile *c,
               struct qreg src, struct qreg dst,
               unsigned func)
{
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
vc4_blend(struct vc4_compile *c, struct qreg *result,
          struct qreg *dst_color, struct qreg *src_color)
{
        struct pipe_rt_blend_state *blend = &c->fs_key->blend;

        if (!blend->blend_enable) {
                for (int i = 0; i < 4; i++)
                        result[i] = src_color[i];
                return;
        }

        struct qreg src_blend[4], dst_blend[4];
        for (int i = 0; i < 3; i++) {
                src_blend[i] = vc4_blend_channel(c,
                                                 dst_color, src_color,
                                                 src_color[i],
                                                 blend->rgb_src_factor, i);
                dst_blend[i] = vc4_blend_channel(c,
                                                 dst_color, src_color,
                                                 dst_color[i],
                                                 blend->rgb_dst_factor, i);
        }
        src_blend[3] = vc4_blend_channel(c,
                                         dst_color, src_color,
                                         src_color[3],
                                         blend->alpha_src_factor, 3);
        dst_blend[3] = vc4_blend_channel(c,
                                         dst_color, src_color,
                                         dst_color[3],
                                         blend->alpha_dst_factor, 3);

        for (int i = 0; i < 3; i++) {
                result[i] = vc4_blend_func(c,
                                           src_blend[i], dst_blend[i],
                                           blend->rgb_func);
        }
        result[3] = vc4_blend_func(c,
                                   src_blend[3], dst_blend[3],
                                   blend->alpha_func);
}

static void
emit_frag_end(struct vc4_compile *c)
{
        struct qreg src_color[4] = {
                c->outputs[0], c->outputs[1], c->outputs[2], c->outputs[3],
        };

        enum pipe_format color_format = c->fs_key->color_format;
        const uint8_t *format_swiz = vc4_get_format_swizzle(color_format);
        struct qreg tlb_read_color[4] = { c->undef, c->undef, c->undef, c->undef };
        struct qreg dst_color[4] = { c->undef, c->undef, c->undef, c->undef };
        if (c->fs_key->blend.blend_enable ||
            c->fs_key->blend.colormask != 0xf) {
                struct qreg r4 = qir_TLB_COLOR_READ(c);
                for (int i = 0; i < 4; i++)
                        tlb_read_color[i] = qir_R4_UNPACK(c, r4, i);
                for (int i = 0; i < 4; i++)
                        dst_color[i] = get_swizzled_channel(c,
                                                            tlb_read_color,
                                                            format_swiz[i]);
        }

        struct qreg blend_color[4];
        vc4_blend(c, blend_color, dst_color, src_color);

        /* If the bit isn't set in the color mask, then just return the
         * original dst color, instead.
         */
        for (int i = 0; i < 4; i++) {
                if (!(c->fs_key->blend.colormask & (1 << i))) {
                        blend_color[i] = dst_color[i];
                }
        }

        /* Debug: Sometimes you're getting a black output and just want to see
         * if the FS is getting executed at all.  Spam magenta into the color
         * output.
         */
        if (0) {
                blend_color[0] = qir_uniform_f(c, 1.0);
                blend_color[1] = qir_uniform_f(c, 0.0);
                blend_color[2] = qir_uniform_f(c, 1.0);
                blend_color[3] = qir_uniform_f(c, 0.5);
        }

        struct qreg swizzled_outputs[4];
        for (int i = 0; i < 4; i++) {
                swizzled_outputs[i] = get_swizzled_channel(c, blend_color,
                                                           format_swiz[i]);
        }

        if (c->discard.file != QFILE_NULL)
                qir_TLB_DISCARD_SETUP(c, c->discard);

        if (c->fs_key->depth_enabled) {
                qir_emit(c, qir_inst(QOP_TLB_PASSTHROUGH_Z_WRITE, c->undef,
                                     c->undef, c->undef));
        }

        bool color_written = false;
        for (int i = 0; i < 4; i++) {
                if (swizzled_outputs[i].file != QFILE_NULL)
                        color_written = true;
        }

        struct qreg packed_color;
        if (color_written) {
                /* Fill in any undefined colors.  The simulator will assertion
                 * fail if we read something that wasn't written, and I don't
                 * know what hardware does.
                 */
                for (int i = 0; i < 4; i++) {
                        if (swizzled_outputs[i].file == QFILE_NULL)
                                swizzled_outputs[i] = qir_uniform_f(c, 0.0);
                }
                packed_color = qir_get_temp(c);
                qir_emit(c, qir_inst4(QOP_PACK_COLORS, packed_color,
                                      swizzled_outputs[0],
                                      swizzled_outputs[1],
                                      swizzled_outputs[2],
                                      swizzled_outputs[3]));
        } else {
                packed_color = qir_uniform_ui(c, 0);
        }

        qir_emit(c, qir_inst(QOP_TLB_COLOR_WRITE, c->undef,
                             packed_color, c->undef));
}

static void
emit_scaled_viewport_write(struct vc4_compile *c, struct qreg rcp_w)
{
        struct qreg xyi[2];

        for (int i = 0; i < 2; i++) {
                struct qreg scale =
                        add_uniform(c, QUNIFORM_VIEWPORT_X_SCALE + i, 0);

                xyi[i] = qir_FTOI(c, qir_FMUL(c,
                                              qir_FMUL(c,
                                                       c->outputs[i],
                                                       scale),
                                              rcp_w));
        }

        qir_VPM_WRITE(c, qir_PACK_SCALED(c, xyi[0], xyi[1]));
}

static void
emit_zs_write(struct vc4_compile *c, struct qreg rcp_w)
{
        struct qreg zscale = add_uniform(c, QUNIFORM_VIEWPORT_Z_SCALE, 0);
        struct qreg zoffset = add_uniform(c, QUNIFORM_VIEWPORT_Z_OFFSET, 0);

        qir_VPM_WRITE(c, qir_FMUL(c, qir_FADD(c, qir_FMUL(c,
                                                          c->outputs[2],
                                                          zscale),
                                              zoffset),
                                  rcp_w));
}

static void
emit_rcp_wc_write(struct vc4_compile *c, struct qreg rcp_w)
{
        qir_VPM_WRITE(c, rcp_w);
}

static void
emit_vert_end(struct vc4_compile *c)
{
        struct qreg rcp_w = qir_RCP(c, c->outputs[3]);

        emit_scaled_viewport_write(c, rcp_w);
        emit_zs_write(c, rcp_w);
        emit_rcp_wc_write(c, rcp_w);

        for (int i = 4; i < c->num_outputs; i++) {
                qir_VPM_WRITE(c, c->outputs[i]);
        }
}

static void
emit_coord_end(struct vc4_compile *c)
{
        struct qreg rcp_w = qir_RCP(c, c->outputs[3]);

        for (int i = 0; i < 4; i++)
                qir_VPM_WRITE(c, c->outputs[i]);

        emit_scaled_viewport_write(c, rcp_w);
        emit_zs_write(c, rcp_w);
        emit_rcp_wc_write(c, rcp_w);
}

static struct vc4_compile *
vc4_shader_tgsi_to_qir(struct vc4_compiled_shader *shader, enum qstage stage,
                       struct vc4_key *key)
{
        struct vc4_compile *c = qir_compile_init();
        int ret;

        c->stage = stage;

        /* XXX sizing */
        c->temps = calloc(sizeof(struct qreg), 1024);
        c->inputs = calloc(sizeof(struct qreg), 8 * 4);
        c->outputs = calloc(sizeof(struct qreg), 1024);
        c->uniforms = calloc(sizeof(struct qreg), 1024);
        c->consts = calloc(sizeof(struct qreg), 1024);

        c->uniform_data = calloc(sizeof(uint32_t), 1024);
        c->uniform_contents = calloc(sizeof(enum quniform_contents), 1024);

        c->shader_state = key->shader_state;
        ret = tgsi_parse_init(&c->parser, c->shader_state->tokens);
        assert(ret == TGSI_PARSE_OK);

        if (vc4_debug & VC4_DEBUG_TGSI) {
                fprintf(stderr, "TGSI:\n");
                tgsi_dump(c->shader_state->tokens, 0);
        }

        c->key = key;
        switch (stage) {
        case QSTAGE_FRAG:
                c->fs_key = (struct vc4_fs_key *)key;
                if (c->fs_key->is_points) {
                        c->point_x = emit_fragment_varying(c, 0);
                        c->point_y = emit_fragment_varying(c, 0);
                } else if (c->fs_key->is_lines) {
                        c->line_x = emit_fragment_varying(c, 0);
                }
                break;
        case QSTAGE_VERT:
                c->vs_key = (struct vc4_vs_key *)key;
                break;
        case QSTAGE_COORD:
                c->vs_key = (struct vc4_vs_key *)key;
                break;
        }

        while (!tgsi_parse_end_of_tokens(&c->parser)) {
                tgsi_parse_token(&c->parser);

                switch (c->parser.FullToken.Token.Type) {
                case TGSI_TOKEN_TYPE_DECLARATION:
                        emit_tgsi_declaration(c,
                                              &c->parser.FullToken.FullDeclaration);
                        break;

                case TGSI_TOKEN_TYPE_INSTRUCTION:
                        emit_tgsi_instruction(c,
                                              &c->parser.FullToken.FullInstruction);
                        break;

                case TGSI_TOKEN_TYPE_IMMEDIATE:
                        parse_tgsi_immediate(c,
                                             &c->parser.FullToken.FullImmediate);
                        break;
                }
        }

        switch (stage) {
        case QSTAGE_FRAG:
                emit_frag_end(c);
                break;
        case QSTAGE_VERT:
                emit_vert_end(c);
                break;
        case QSTAGE_COORD:
                emit_coord_end(c);
                break;
        }

        tgsi_parse_free(&c->parser);
        free(c->temps);

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
                        qir_get_stage_name(c->stage), c->num_uniforms);
        }

        return c;
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
                             struct vc4_compile *c)
{
        int count = c->num_uniforms;
        struct vc4_shader_uniform_info *uinfo = &shader->uniforms[shader_index];

        uinfo->count = count;
        uinfo->data = malloc(count * sizeof(*uinfo->data));
        memcpy(uinfo->data, c->uniform_data,
               count * sizeof(*uinfo->data));
        uinfo->contents = malloc(count * sizeof(*uinfo->contents));
        memcpy(uinfo->contents, c->uniform_contents,
               count * sizeof(*uinfo->contents));
        uinfo->num_texture_samples = c->num_texture_samples;
}

static void
vc4_fs_compile(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
               struct vc4_fs_key *key)
{
        struct vc4_compile *c = vc4_shader_tgsi_to_qir(shader, QSTAGE_FRAG,
                                                       &key->base);
        shader->num_inputs = c->num_inputs;
        copy_uniform_state_to_shader(shader, 0, c);
        shader->bo = vc4_bo_alloc_mem(vc4->screen, c->qpu_insts,
                                      c->qpu_inst_count * sizeof(uint64_t),
                                      "fs_code");

        qir_compile_destroy(c);
}

static void
vc4_vs_compile(struct vc4_context *vc4, struct vc4_compiled_shader *shader,
               struct vc4_vs_key *key)
{
        struct vc4_compile *vs_c = vc4_shader_tgsi_to_qir(shader,
                                                          QSTAGE_VERT,
                                                          &key->base);
        copy_uniform_state_to_shader(shader, 0, vs_c);

        struct vc4_compile *cs_c = vc4_shader_tgsi_to_qir(shader,
                                                          QSTAGE_COORD,
                                                          &key->base);
        copy_uniform_state_to_shader(shader, 1, cs_c);

        uint32_t vs_size = vs_c->qpu_inst_count * sizeof(uint64_t);
        uint32_t cs_size = cs_c->qpu_inst_count * sizeof(uint64_t);
        shader->coord_shader_offset = vs_size; /* XXX: alignment? */
        shader->bo = vc4_bo_alloc(vc4->screen,
                                  shader->coord_shader_offset + cs_size,
                                  "vs_code");

        void *map = vc4_bo_map(shader->bo);
        memcpy(map, vs_c->qpu_insts, vs_size);
        memcpy(map + shader->coord_shader_offset,
               cs_c->qpu_insts, cs_size);

        qir_compile_destroy(vs_c);
        qir_compile_destroy(cs_c);
}

static void
vc4_setup_shared_key(struct vc4_key *key, struct vc4_texture_stateobj *texstate)
{
        for (int i = 0; i < texstate->num_textures; i++) {
                struct pipe_sampler_view *sampler = texstate->textures[i];
                struct pipe_sampler_state *sampler_state =
texstate->samplers[i];

                if (sampler) {
                        struct pipe_resource *prsc = sampler->texture;
                        key->tex[i].format = prsc->format;
                        key->tex[i].swizzle[0] = sampler->swizzle_r;
                        key->tex[i].swizzle[1] = sampler->swizzle_g;
                        key->tex[i].swizzle[2] = sampler->swizzle_b;
                        key->tex[i].swizzle[3] = sampler->swizzle_a;
                        key->tex[i].compare_mode = sampler_state->compare_mode;
                        key->tex[i].compare_func = sampler_state->compare_func;
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
                 rsc->slices[0].offset | texture->u.tex.last_level |
                 ((rsc->vc4_format & 7) << 4));
}

static void
write_texture_p1(struct vc4_context *vc4,
                 struct vc4_texture_stateobj *texstate,
                 uint32_t unit)
{
        struct pipe_sampler_view *texture = texstate->textures[unit];
        struct vc4_resource *rsc = vc4_resource(texture->texture);
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
               ((rsc->vc4_format >> 4) << 31) |
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
