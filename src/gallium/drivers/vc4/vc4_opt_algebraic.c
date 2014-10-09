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

/**
 * @file vc4_opt_algebraic.c
 *
 * This is the optimization pass for miscellaneous changes to instructions
 * where we can simplify the operation by some knowledge about the specific
 * operations.
 *
 * Mostly this will be a matter of turning things into MOVs so that they can
 * later be copy-propagated out.
 */

#include "vc4_qir.h"
#include "util/u_math.h"

static bool debug;

static void
dump_from(struct vc4_compile *c, struct qinst *inst)
{
        if (!debug)
                return;

        fprintf(stderr, "optimizing: ");
        qir_dump_inst(c, inst);
        fprintf(stderr, "\n");
}

static void
dump_to(struct vc4_compile *c, struct qinst *inst)
{
        if (!debug)
                return;

        fprintf(stderr, "to: ");
        qir_dump_inst(c, inst);
        fprintf(stderr, "\n");
}

static struct qreg
follow_movs(struct qinst **defs, struct qreg reg)
{
        while (reg.file == QFILE_TEMP && defs[reg.index]->op == QOP_MOV)
                reg = defs[reg.index]->src[0];

        return reg;
}

static bool
is_zero(struct vc4_compile *c, struct qinst **defs, struct qreg reg)
{
        reg = follow_movs(defs, reg);

        return (reg.file == QFILE_UNIF &&
                c->uniform_contents[reg.index] == QUNIFORM_CONSTANT &&
                c->uniform_data[reg.index] == 0);
}

static bool
is_1f(struct vc4_compile *c, struct qinst **defs, struct qreg reg)
{
        reg = follow_movs(defs, reg);

        return (reg.file == QFILE_UNIF &&
                c->uniform_contents[reg.index] == QUNIFORM_CONSTANT &&
                c->uniform_data[reg.index] == fui(1.0));
}

static void
replace_with_mov(struct vc4_compile *c, struct qinst *inst, struct qreg arg)
{
        dump_from(c, inst);
        inst->op = QOP_MOV;
        inst->src[0] = arg;
        inst->src[1] = c->undef;
        dump_to(c, inst);
}

static bool
add_replace_zero(struct vc4_compile *c,
                 struct qinst **defs,
                 struct qinst *inst,
                 int arg)
{
        if (!is_zero(c, defs, inst->src[arg]))
                return false;
        replace_with_mov(c, inst, inst->src[1 - arg]);
        return true;
}

static bool
fmul_replace_zero(struct vc4_compile *c,
                  struct qinst **defs,
                  struct qinst *inst,
                  int arg)
{
        if (!is_zero(c, defs, inst->src[arg]))
                return false;
        replace_with_mov(c, inst, inst->src[arg]);
        return true;
}

static bool
fmul_replace_one(struct vc4_compile *c,
                 struct qinst **defs,
                 struct qinst *inst,
                 int arg)
{
        if (!is_1f(c, defs, inst->src[arg]))
                return false;
        replace_with_mov(c, inst, inst->src[1 - arg]);
        return true;
}

bool
qir_opt_algebraic(struct vc4_compile *c)
{
        bool progress = false;
        struct simple_node *node;
        struct qinst *defs[c->num_temps];

        foreach(node, &c->instructions) {
                struct qinst *inst = (struct qinst *)node;

                if (inst->dst.file == QFILE_TEMP)
                        defs[inst->dst.index] = inst;

                switch (inst->op) {
                case QOP_SF:
                        /* SF just looks at the sign bit, or whether all the
                         * bits are 0.  This is preserved across an itof
                         * transformation.
                         */
                        if (inst->src[0].file == QFILE_TEMP &&
                            defs[inst->src[0].index]->op == QOP_ITOF) {
                                dump_from(c, inst);
                                inst->src[0] =
                                        defs[inst->src[0].index]->src[0];
                                progress =  true;
                                dump_to(c, inst);
                                break;
                        }
                        break;

                case QOP_SEL_X_Y_ZS:
                case QOP_SEL_X_Y_ZC:
                case QOP_SEL_X_Y_NS:
                case QOP_SEL_X_Y_NC:
                        if (qir_reg_equals(inst->src[0], inst->src[1])) {
                                /* Turn "dst = (sf == x) ? a : a)" into
                                 * "dst = a"
                                 */
                                replace_with_mov(c, inst, inst->src[1]);
                                progress = true;
                                break;
                        }

                        if (is_zero(c, defs, inst->src[1])) {
                                /* Replace references to a 0 uniform value
                                 * with the SEL_X_0 equivalent.
                                 */
                                dump_from(c, inst);
                                inst->op -= (QOP_SEL_X_Y_ZS - QOP_SEL_X_0_ZS);
                                inst->src[1] = c->undef;
                                progress = true;
                                dump_to(c, inst);
                                break;
                        }

                        if (is_zero(c, defs, inst->src[0])) {
                                /* Replace references to a 0 uniform value
                                 * with the SEL_X_0 equivalent, flipping the
                                 * condition being evaluated since the operand
                                 * order is flipped.
                                 */
                                dump_from(c, inst);
                                inst->op -= QOP_SEL_X_Y_ZS;
                                inst->op ^= 1;
                                inst->op += QOP_SEL_X_0_ZS;
                                inst->src[0] = inst->src[1];
                                inst->src[1] = c->undef;
                                progress = true;
                                dump_to(c, inst);
                                break;
                        }

                        break;

                case QOP_FSUB:
                case QOP_SUB:
                        if (is_zero(c, defs, inst->src[1])) {
                                replace_with_mov(c, inst, inst->src[0]);
                        }
                        break;

                case QOP_ADD:
                        if (add_replace_zero(c, defs, inst, 0) ||
                            add_replace_zero(c, defs, inst, 1)) {
                                progress = true;
                                break;
                        }
                        break;

                case QOP_FADD:
                        if (add_replace_zero(c, defs, inst, 0) ||
                            add_replace_zero(c, defs, inst, 1)) {
                                progress = true;
                                break;
                        }

                        /* FADD(a, FSUB(0, b)) -> FSUB(a, b) */
                        if (inst->src[1].file == QFILE_TEMP &&
                            defs[inst->src[1].index]->op == QOP_FSUB) {
                                struct qinst *fsub = defs[inst->src[1].index];
                                if (is_zero(c, defs, fsub->src[0])) {
                                        dump_from(c, inst);
                                        inst->op = QOP_FSUB;
                                        inst->src[1] = fsub->src[1];
                                        progress = true;
                                        dump_to(c, inst);
                                        break;
                                }
                        }

                        /* FADD(FSUB(0, b), a) -> FSUB(a, b) */
                        if (inst->src[0].file == QFILE_TEMP &&
                            defs[inst->src[0].index]->op == QOP_FSUB) {
                                struct qinst *fsub = defs[inst->src[0].index];
                                if (is_zero(c, defs, fsub->src[0])) {
                                        dump_from(c, inst);
                                        inst->op = QOP_FSUB;
                                        inst->src[0] = inst->src[1];
                                        inst->src[1] = fsub->src[1];
                                        dump_to(c, inst);
                                        progress = true;
                                        break;
                                }
                        }
                        break;

                case QOP_FMUL:
                        if (fmul_replace_zero(c, defs, inst, 0) ||
                            fmul_replace_zero(c, defs, inst, 1) ||
                            fmul_replace_one(c, defs, inst, 0) ||
                            fmul_replace_one(c, defs, inst, 1)) {
                                progress = true;
                                break;
                        }
                        break;

                default:
                        break;
                }
        }

        return progress;
}
