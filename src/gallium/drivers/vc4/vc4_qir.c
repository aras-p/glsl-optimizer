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

#include <stdio.h>

#include "util/u_memory.h"
#include "util/u_simple_list.h"

#include "vc4_qir.h"
#include "vc4_qpu.h"

struct qir_op_info {
        const char *name;
        uint8_t ndst, nsrc;
        bool has_side_effects;
};

static const struct qir_op_info qir_op_info[] = {
        [QOP_MOV] = { "mov", 1, 1 },
        [QOP_FADD] = { "fadd", 1, 2 },
        [QOP_FSUB] = { "fsub", 1, 2 },
        [QOP_FMUL] = { "fmul", 1, 2 },
        [QOP_FMIN] = { "fmin", 1, 2 },
        [QOP_FMAX] = { "fmax", 1, 2 },
        [QOP_FMINABS] = { "fminabs", 1, 2 },
        [QOP_FMAXABS] = { "fmaxabs", 1, 2 },

        [QOP_SEQ] = { "seq", 1, 2 },
        [QOP_SNE] = { "sne", 1, 2 },
        [QOP_SGE] = { "sge", 1, 2 },
        [QOP_SLT] = { "slt", 1, 2 },
        [QOP_CMP] = { "cmp", 1, 3 },

        [QOP_FTOI] = { "ftoi", 1, 1 },
        [QOP_ITOF] = { "itof", 1, 1 },
        [QOP_RCP] = { "rcp", 1, 1 },
        [QOP_RSQ] = { "rsq", 1, 1 },
        [QOP_EXP2] = { "exp2", 1, 2 },
        [QOP_LOG2] = { "log2", 1, 2 },
        [QOP_PACK_COLORS] = { "pack_colors", 1, 4 },
        [QOP_PACK_SCALED] = { "pack_scaled", 1, 2 },
        [QOP_VPM_WRITE] = { "vpm_write", 0, 1, true },
        [QOP_VPM_READ] = { "vpm_read", 0, 1, true },
        [QOP_TLB_PASSTHROUGH_Z_WRITE] = { "tlb_passthrough_z", 0, 0, true },
        [QOP_TLB_COLOR_WRITE] = { "tlb_color", 0, 1, true },
        [QOP_TLB_COLOR_READ] = { "tlb_color_read", 0, 0, true },
        [QOP_VARY_ADD_C] = { "vary_add_c", 1, 1 },

        [QOP_FRAG_X] = { "frag_x", 1, 0 },
        [QOP_FRAG_Y] = { "frag_y", 1, 0 },
        [QOP_FRAG_Z] = { "frag_z", 1, 0 },
        [QOP_FRAG_RCP_W] = { "frag_rcp_w", 1, 0 },

        [QOP_TEX_S] = { "tex_s", 0, 2 },
        [QOP_TEX_T] = { "tex_t", 0, 2 },
        [QOP_TEX_R] = { "tex_r", 0, 2 },
        [QOP_TEX_B] = { "tex_b", 0, 2 },
        [QOP_TEX_RESULT] = { "tex_result", 0, 0 },
        [QOP_R4_UNPACK_A] = { "r4_unpack_a", 1, 0 },
        [QOP_R4_UNPACK_B] = { "r4_unpack_b", 1, 0 },
        [QOP_R4_UNPACK_C] = { "r4_unpack_c", 1, 0 },
        [QOP_R4_UNPACK_D] = { "r4_unpack_d", 1, 0 },
};

static const char *
qir_get_op_name(enum qop qop)
{
        if (qop < ARRAY_SIZE(qir_op_info) && qir_op_info[qop].name)
                return qir_op_info[qop].name;
        else
                return "???";
}

int
qir_get_op_nsrc(enum qop qop)
{
        if (qop < ARRAY_SIZE(qir_op_info) && qir_op_info[qop].name)
                return qir_op_info[qop].nsrc;
        else
                abort();
}

bool
qir_has_side_effects(struct qinst *inst)
{
        for (int i = 0; i < qir_get_op_nsrc(inst->op); i++) {
                if (inst->src[i].file == QFILE_VARY ||
                    inst->src[i].file == QFILE_UNIF)
                        return true;
        }

        return qir_op_info[inst->op].has_side_effects;
}

static void
qir_print_reg(struct qreg reg)
{
        const char *files[] = {
                [QFILE_TEMP] = "t",
                [QFILE_VARY] = "v",
                [QFILE_UNIF] = "u",
        };

        if (reg.file == QFILE_NULL)
                fprintf(stderr, "null");
        else
                fprintf(stderr, "%s%d", files[reg.file], reg.index);
}

void
qir_dump_inst(struct qinst *inst)
{
        fprintf(stderr, "%s ", qir_get_op_name(inst->op));

        qir_print_reg(inst->dst);
        for (int i = 0; i < qir_get_op_nsrc(inst->op); i++) {
                fprintf(stderr, ", ");
                qir_print_reg(inst->src[i]);
        }
}

void
qir_dump(struct qcompile *c)
{
        struct simple_node *node;

        foreach(node, &c->instructions) {
                struct qinst *inst = (struct qinst *)node;
                qir_dump_inst(inst);
                fprintf(stderr, "\n");
        }
}

struct qreg
qir_get_temp(struct qcompile *c)
{
        struct qreg reg;

        reg.file = QFILE_TEMP;
        reg.index = c->num_temps++;

        return reg;
}

struct qinst *
qir_inst(enum qop op, struct qreg dst, struct qreg src0, struct qreg src1)
{
        struct qinst *inst = CALLOC_STRUCT(qinst);

        inst->op = op;
        inst->dst = dst;
        inst->src = calloc(2, sizeof(inst->src[0]));
        inst->src[0] = src0;
        inst->src[1] = src1;

        return inst;
}

struct qinst *
qir_inst4(enum qop op, struct qreg dst,
          struct qreg a,
          struct qreg b,
          struct qreg c,
          struct qreg d)
{
        struct qinst *inst = CALLOC_STRUCT(qinst);

        inst->op = op;
        inst->dst = dst;
        inst->src = calloc(4, sizeof(*inst->src));
        inst->src[0] = a;
        inst->src[1] = b;
        inst->src[2] = c;
        inst->src[3] = d;

        return inst;
}

void
qir_emit(struct qcompile *c, struct qinst *inst)
{
        insert_at_tail(&c->instructions, &inst->link);
}

bool
qir_reg_equals(struct qreg a, struct qreg b)
{
        return a.file == b.file && a.index == b.index;
}

struct qcompile *
qir_compile_init(void)
{
        struct qcompile *c = CALLOC_STRUCT(qcompile);

        make_empty_list(&c->instructions);

        return c;
}

void
qir_compile_destroy(struct qcompile *c)
{
        free(c);
}

const char *
qir_get_stage_name(enum qstage stage)
{
        static const char *names[] = {
                [QSTAGE_FRAG] = "FS",
                [QSTAGE_VERT] = "VS",
                [QSTAGE_COORD] = "CS",
        };

        return names[stage];
}

#define OPTPASS(func)                                                   \
        do {                                                            \
                bool stage_progress = func(c);                          \
                if (stage_progress) {                                   \
                        progress = true;                                \
                        if (print_opt_debug) {                          \
                                fprintf(stderr,                         \
                                        "QIR opt pass %2d: %s progress\n", \
                                        pass, #func);                   \
                        }                                               \
                }                                                       \
        } while (0)

void
qir_optimize(struct qcompile *c)
{
        bool print_opt_debug = false;
        int pass = 1;

        while (true) {
                bool progress = false;

                OPTPASS(qir_opt_algebraic);
                OPTPASS(qir_opt_copy_propagation);
                OPTPASS(qir_opt_dead_code);

                if (!progress)
                        break;

                pass++;
        }
}
