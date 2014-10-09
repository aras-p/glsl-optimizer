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

#include "vc4_qir.h"

static bool debug;

static void
dce(struct vc4_compile *c, struct qinst *inst)
{
        if (debug) {
                fprintf(stderr, "Removing: ");
                qir_dump_inst(c, inst);
                fprintf(stderr, "\n");
        }
        qir_remove_instruction(inst);
}

bool
qir_opt_dead_code(struct vc4_compile *c)
{
        bool progress = false;
        bool *used = calloc(c->num_temps, sizeof(bool));
        bool sf_used = false;

        struct simple_node *node, *t;
        for (node = c->instructions.prev, t = node->prev;
             &c->instructions != node;
             node = t, t = t->prev) {
                struct qinst *inst = (struct qinst *)node;

                if (inst->dst.file == QFILE_TEMP &&
                    !used[inst->dst.index] &&
                    !qir_has_side_effects(inst)) {
                        dce(c, inst);
                        progress = true;
                        continue;
                }

                if (qir_depends_on_flags(inst))
                        sf_used = true;
                if (inst->op == QOP_SF) {
                        if (!sf_used) {
                                dce(c, inst);
                                progress = true;
                                continue;
                        }
                        sf_used = false;
                }

                for (int i = 0; i < qir_get_op_nsrc(inst->op); i++) {
                        if (inst->src[i].file == QFILE_TEMP)
                                used[inst->src[i].index] = true;
                }
        }

        free(used);

        return progress;
}
