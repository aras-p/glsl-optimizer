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

static bool debug;

static void
dump_from(struct qinst *inst)
{
        if (!debug)
                return;

        fprintf(stderr, "optimizing: ");
        qir_dump_inst(inst);
        fprintf(stderr, "\n");
}

static void
dump_to(struct qinst *inst)
{
        if (!debug)
                return;

        fprintf(stderr, "to: ");
        qir_dump_inst(inst);
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
                case QOP_SEL_X_Y_ZS:
                case QOP_SEL_X_Y_ZC:
                case QOP_SEL_X_Y_NS:
                case QOP_SEL_X_Y_NC:
                        if (qir_reg_equals(inst->src[0], inst->src[1])) {
                                /* Turn "dst = (sf == x) ? a : a)" into
                                 * "dst = a"
                                 */
                                dump_from(inst);
                                inst->op = QOP_MOV;
                                inst->src[0] = inst->src[1];
                                inst->src[1] = c->undef;
                                progress = true;
                                dump_to(inst);
                        } else if (is_zero(c, defs, inst->src[1])) {
                                /* Replace references to a 0 uniform value
                                 * with the SEL_X_0 equivalent.
                                 */
                                dump_from(inst);
                                inst->op -= (QOP_SEL_X_Y_ZS - QOP_SEL_X_0_ZS);
                                inst->src[1] = c->undef;
                                progress = true;
                                dump_to(inst);
                        }
                        break;

                default:
                        break;
                }
        }

        return progress;
}
