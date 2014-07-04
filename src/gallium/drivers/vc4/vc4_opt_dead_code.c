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
 * @file vc4_opt_dead_code.c
 *
 * This is a simmple dead code eliminator for QIR with no control flow.
 *
 * It walks from the bottom of the instruction list, removing instructions
 * with a destination that is never used, and marking the sources of non-dead
 * instructions as used.
 */

#include <stdio.h>
#include <stdlib.h>
#include "vc4_qir.h"

bool
qir_opt_dead_code(struct qcompile *c)
{
        bool progress = false;
        bool debug = false;
        bool *used = calloc(c->num_temps, sizeof(bool));

        struct simple_node *node, *t;
        for (node = c->instructions.prev, t = node->prev;
             &c->instructions != node;
             node = t, t = t->prev) {
                struct qinst *inst = (struct qinst *)node;

                if (inst->dst.file == QFILE_TEMP &&
                    !used[inst->dst.index] &&
                    !qir_has_side_effects(inst)) {
                        if (debug) {
                                fprintf(stderr, "Removing: ");
                                qir_dump_inst(inst);
                                fprintf(stderr, "\n");
                        }
                        remove_from_list(&inst->link);
                        free(inst);
                        progress = true;
                        continue;
                }

                for (int i = 0; i < qir_get_op_nsrc(inst->op); i++) {
                        if (inst->src[i].file == QFILE_TEMP)
                                used[inst->src[i].index] = true;
                }
        }

        free(used);

        return progress;
}
