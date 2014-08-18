/*
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

#ifndef VC4_QIR_H
#define VC4_QIR_H

#include <stdbool.h>
#include <stdint.h>

#include "util/u_simple_list.h"

enum qfile {
        QFILE_NULL,
        QFILE_TEMP,
        QFILE_VARY,
        QFILE_UNIF,
};

struct qreg {
        enum qfile file;
        uint32_t index;
};

enum qop {
        QOP_UNDEF,
        QOP_MOV,
        QOP_FADD,
        QOP_FSUB,
        QOP_FMUL,
        QOP_FMIN,
        QOP_FMAX,
        QOP_FMINABS,
        QOP_FMAXABS,

        QOP_SEQ,
        QOP_SNE,
        QOP_SGE,
        QOP_SLT,
        QOP_CMP,

        QOP_FTOI,
        QOP_ITOF,
        QOP_RCP,
        QOP_RSQ,
        QOP_EXP2,
        QOP_LOG2,
        QOP_VW_SETUP,
        QOP_VR_SETUP,
        QOP_PACK_SCALED,
        QOP_PACK_COLORS,
        QOP_VPM_WRITE,
        QOP_VPM_READ,
        QOP_TLB_PASSTHROUGH_Z_WRITE,
        QOP_TLB_COLOR_WRITE,
        QOP_TLB_COLOR_READ,
        QOP_VARY_ADD_C,

        QOP_FRAG_X,
        QOP_FRAG_Y,
        QOP_FRAG_Z,
        QOP_FRAG_RCP_W,

        /** Texture x coordinate parameter write */
        QOP_TEX_S,
        /** Texture y coordinate parameter write */
        QOP_TEX_T,
        /** Texture border color parameter or cube map z coordinate write */
        QOP_TEX_R,
        /** Texture LOD bias parameter write */
        QOP_TEX_B,
        /**
         * Signal of texture read being necessary and then reading r4 into
         * the destination
         */
        QOP_TEX_RESULT,
        QOP_R4_UNPACK_A,
        QOP_R4_UNPACK_B,
        QOP_R4_UNPACK_C,
        QOP_R4_UNPACK_D
};

struct simple_node {
        struct simple_node *next;
        struct simple_node *prev;
};

struct qinst {
        struct simple_node link;

        enum qop op;
        struct qreg dst;
        struct qreg *src;
};

enum qstage {
        /**
         * Coordinate shader, runs during binning, before the VS, and just
         * outputs position.
         */
        QSTAGE_COORD,
        QSTAGE_VERT,
        QSTAGE_FRAG,
};

enum quniform_contents {
        /**
         * Indicates that a constant 32-bit value is copied from the program's
         * uniform contents.
         */
        QUNIFORM_CONSTANT,
        /**
         * Indicates that the program's uniform contents are used as an index
         * into the GL uniform storage.
         */
        QUNIFORM_UNIFORM,

        /** @{
         * Scaling factors from clip coordinates to relative to the viewport
         * center.
         *
         * This is used by the coordinate and vertex shaders to produce the
         * 32-bit entry consisting of 2 16-bit fields with 12.4 signed fixed
         * point offsets from the viewport ccenter.
         */
        QUNIFORM_VIEWPORT_X_SCALE,
        QUNIFORM_VIEWPORT_Y_SCALE,
        /** @} */

        QUNIFORM_VIEWPORT_Z_OFFSET,
        QUNIFORM_VIEWPORT_Z_SCALE,

        /**
         * A reference to a texture config parameter 0 uniform.
         *
         * This is a uniform implicitly loaded with a QPU_W_TMU* write, which
         * defines texture type, miplevels, and such.  It will be found as a
         * parameter to the first QOP_TEX_[STRB] instruction in a sequence.
         */
        QUNIFORM_TEXTURE_CONFIG_P0,

        /**
         * A reference to a texture config parameter 1 uniform.
         *
         * This is a uniform implicitly loaded with a QPU_W_TMU* write, which
         * defines texture width, height, filters, and wrap modes.  It will be
         * found as a parameter to the second QOP_TEX_[STRB] instruction in a
         * sequence.
         */
        QUNIFORM_TEXTURE_CONFIG_P1,

        QUNIFORM_TEXRECT_SCALE_X,
        QUNIFORM_TEXRECT_SCALE_Y,

        QUNIFORM_BLEND_CONST_COLOR,
};

struct qcompile {
        struct qreg undef;
        enum qstage stage;
        uint32_t num_temps;
        struct simple_node instructions;
        uint32_t immediates[1024];

        struct simple_node qpu_inst_list;
        uint64_t *qpu_insts;
        uint32_t qpu_inst_count;
        uint32_t qpu_inst_size;
        uint32_t num_inputs;
};

struct qcompile *qir_compile_init(void);
void qir_compile_destroy(struct qcompile *c);
struct qinst *qir_inst(enum qop op, struct qreg dst,
                       struct qreg src0, struct qreg src1);
struct qinst *qir_inst4(enum qop op, struct qreg dst,
                        struct qreg a,
                        struct qreg b,
                        struct qreg c,
                        struct qreg d);
void qir_emit(struct qcompile *c, struct qinst *inst);
struct qreg qir_get_temp(struct qcompile *c);
int qir_get_op_nsrc(enum qop qop);
bool qir_reg_equals(struct qreg a, struct qreg b);
bool qir_has_side_effects(struct qinst *inst);

void qir_dump(struct qcompile *c);
void qir_dump_inst(struct qinst *inst);
const char *qir_get_stage_name(enum qstage stage);

void qir_optimize(struct qcompile *c);
bool qir_opt_algebraic(struct qcompile *c);
bool qir_opt_copy_propagation(struct qcompile *c);
bool qir_opt_dead_code(struct qcompile *c);

#define QIR_ALU0(name)                                                   \
static inline struct qreg                                                \
qir_##name(struct qcompile *c)                                           \
{                                                                        \
        struct qreg t = qir_get_temp(c);                                 \
        qir_emit(c, qir_inst(QOP_##name, t, c->undef, c->undef));        \
        return t;                                                        \
}

#define QIR_ALU1(name)                                                   \
static inline struct qreg                                                \
qir_##name(struct qcompile *c, struct qreg a)                            \
{                                                                        \
        struct qreg t = qir_get_temp(c);                                 \
        qir_emit(c, qir_inst(QOP_##name, t, a, c->undef));               \
        return t;                                                        \
}

#define QIR_ALU2(name)                                                   \
static inline struct qreg                                                \
qir_##name(struct qcompile *c, struct qreg a, struct qreg b)             \
{                                                                        \
        struct qreg t = qir_get_temp(c);                                 \
        qir_emit(c, qir_inst(QOP_##name, t, a, b));                      \
        return t;                                                        \
}

#define QIR_NODST_1(name)                                               \
static inline void                                                      \
qir_##name(struct qcompile *c, struct qreg a)                           \
{                                                                       \
        qir_emit(c, qir_inst(QOP_##name, c->undef, a, c->undef));       \
}

#define QIR_NODST_2(name)                                               \
static inline void                                                      \
qir_##name(struct qcompile *c, struct qreg a, struct qreg b)            \
{                                                                       \
        qir_emit(c, qir_inst(QOP_##name, c->undef, a, b));       \
}

QIR_ALU1(MOV)
QIR_ALU2(FADD)
QIR_ALU2(FSUB)
QIR_ALU2(FMUL)
QIR_ALU2(FMIN)
QIR_ALU2(FMAX)
QIR_ALU2(FMINABS)
QIR_ALU2(FMAXABS)
QIR_ALU1(FTOI)
QIR_ALU1(ITOF)
QIR_ALU1(RCP)
QIR_ALU1(RSQ)
QIR_ALU1(EXP2)
QIR_ALU1(LOG2)
QIR_ALU2(PACK_SCALED)
QIR_ALU1(VARY_ADD_C)
QIR_NODST_1(VPM_WRITE)
QIR_NODST_2(TEX_S)
QIR_NODST_2(TEX_T)
QIR_NODST_2(TEX_R)
QIR_NODST_2(TEX_B)
QIR_ALU0(FRAG_X)
QIR_ALU0(FRAG_Y)
QIR_ALU0(FRAG_Z)
QIR_ALU0(FRAG_RCP_W)

static inline struct qreg
qir_CMP(struct qcompile *c, struct qreg cmp, struct qreg a, struct qreg b)
{
        struct qreg t = qir_get_temp(c);
        qir_emit(c, qir_inst4(QOP_CMP, t, cmp, a, b, c->undef));
        return t;
}

static inline struct qreg
qir_R4_UNPACK(struct qcompile *c, int i)
{
        struct qreg t = qir_get_temp(c);
        qir_emit(c, qir_inst(QOP_R4_UNPACK_A + i, t, c->undef, c->undef));
        return t;
}

#endif /* VC4_QIR_H */
