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
 * @file vc4_opt_copy_propagation.c
 *
 * This implements simple copy propagation for QIR without control flow.
 *
 * For each temp, it keeps a qreg of which source it was MOVed from, if it
 * was.  If we see that used later, we can just reuse the source value, since
 * we know we don't have control flow, and we have SSA for our values so
 * there's no killing to worry about.
 */

#include <stdlib.h>
#include <stdio.h>
#include "vc4_qir.h"

bool
qir_opt_copy_propagation(struct qcompile *c)
{
        bool progress = false;
        struct simple_node *node;
        bool debug = false;
        struct qreg *movs = calloc(c->num_temps, sizeof(struct qreg));

        foreach(node, &c->instructions) {
                struct qinst *inst = (struct qinst *)node;

                for (int i = 0; i < qir_get_op_nsrc(inst->op); i++) {
                        int index = inst->src[i].index;
                        if (inst->src[i].file == QFILE_TEMP &&
                            movs[index].file == QFILE_TEMP) {
                                if (debug) {
                                        fprintf(stderr, "Copy propagate: ");
                                        qir_dump_inst(inst);
                                        fprintf(stderr, "\n");
                                }

                                inst->src[i] = movs[index];

                                if (debug) {
                                        fprintf(stderr, "to: ");
                                        qir_dump_inst(inst);
                                        fprintf(stderr, "\n");
                                }

                                progress = true;
                        }
                }

                if (inst->op == QOP_MOV && inst->dst.file == QFILE_TEMP)
                        movs[inst->dst.index] = inst->src[0];
        }

        free(movs);
        return progress;
}
