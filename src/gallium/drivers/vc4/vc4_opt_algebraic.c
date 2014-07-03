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

#include <stdio.h>
#include "vc4_qir.h"

bool
qir_opt_algebraic(struct qcompile *c)
{
        bool progress = false;
        struct simple_node *node;
        bool debug = false;

        foreach(node, &c->instructions) {
                struct qinst *inst = (struct qinst *)node;

                switch (inst->op) {
                case QOP_CMP:
                        /* Turn "dst = (a < 0) ? b : b)" into "dst = b" */
                        if (qir_reg_equals(inst->src[1], inst->src[2])) {
                                if (debug) {
                                        fprintf(stderr, "optimizing: ");
                                        qir_dump_inst(inst);
                                        fprintf(stderr, "\n");
                                }

                                inst->op = QOP_MOV;
                                inst->src[0] = inst->src[1];
                                inst->src[1] = c->undef;
                                progress = true;

                                if (debug) {
                                        fprintf(stderr, "to: ");
                                        qir_dump_inst(inst);
                                        fprintf(stderr, "\n");
                                }
                        }
                        break;

                default:
                        break;
                }
        }

        return progress;
}
