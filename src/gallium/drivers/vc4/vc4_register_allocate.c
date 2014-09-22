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

#include <inttypes.h>

#include "vc4_context.h"
#include "vc4_qir.h"
#include "vc4_qpu.h"

#define QPU_R(file, index) { QPU_MUX_##file, index }

static const struct qpu_reg vc4_regs[] = {
        { QPU_MUX_R0, 0},
        { QPU_MUX_R1, 0},
        { QPU_MUX_R2, 0},
        { QPU_MUX_R3, 0},
        { QPU_MUX_R4, 0},
        QPU_R(A, 0),
        QPU_R(A, 1),
        QPU_R(A, 2),
        QPU_R(A, 3),
        QPU_R(A, 4),
        QPU_R(A, 5),
        QPU_R(A, 6),
        QPU_R(A, 7),
        QPU_R(A, 8),
        QPU_R(A, 9),
        QPU_R(A, 10),
        QPU_R(A, 11),
        QPU_R(A, 12),
        QPU_R(A, 13),
        QPU_R(A, 14),
        QPU_R(A, 15),
        QPU_R(A, 16),
        QPU_R(A, 17),
        QPU_R(A, 18),
        QPU_R(A, 19),
        QPU_R(A, 20),
        QPU_R(A, 21),
        QPU_R(A, 22),
        QPU_R(A, 23),
        QPU_R(A, 24),
        QPU_R(A, 25),
        QPU_R(A, 26),
        QPU_R(A, 27),
        QPU_R(A, 28),
        QPU_R(A, 29),
        QPU_R(A, 30),
        QPU_R(A, 31),
        QPU_R(B, 0),
        QPU_R(B, 1),
        QPU_R(B, 2),
        QPU_R(B, 3),
        QPU_R(B, 4),
        QPU_R(B, 5),
        QPU_R(B, 6),
        QPU_R(B, 7),
        QPU_R(B, 8),
        QPU_R(B, 9),
        QPU_R(B, 10),
        QPU_R(B, 11),
        QPU_R(B, 12),
        QPU_R(B, 13),
        QPU_R(B, 14),
        QPU_R(B, 15),
        QPU_R(B, 16),
        QPU_R(B, 17),
        QPU_R(B, 18),
        QPU_R(B, 19),
        QPU_R(B, 20),
        QPU_R(B, 21),
        QPU_R(B, 22),
        QPU_R(B, 23),
        QPU_R(B, 24),
        QPU_R(B, 25),
        QPU_R(B, 26),
        QPU_R(B, 27),
        QPU_R(B, 28),
        QPU_R(B, 29),
        QPU_R(B, 30),
        QPU_R(B, 31),
};
#define ACC_INDEX     0
#define A_INDEX       (ACC_INDEX + 5)
#define B_INDEX       (A_INDEX + 32)

/**
 * Returns a mapping from QFILE_TEMP indices to struct qpu_regs.
 *
 * The return value should be freed by the caller.
 */
struct qpu_reg *
vc4_register_allocate(struct vc4_compile *c)
{
        struct simple_node *node;
        bool reg_in_use[ARRAY_SIZE(vc4_regs)];
        int *reg_allocated = calloc(c->num_temps, sizeof(*reg_allocated));
        int *reg_uses_remaining =
                calloc(c->num_temps, sizeof(*reg_uses_remaining));
        struct qpu_reg *temp_registers = calloc(c->num_temps,
                                                sizeof(*temp_registers));

        for (int i = 0; i < ARRAY_SIZE(reg_in_use); i++)
                reg_in_use[i] = false;
        for (int i = 0; i < c->num_temps; i++)
                reg_allocated[i] = -1;

        /* If things aren't ever written (undefined values), just read from
         * r0.
         */
        for (int i = 0; i < c->num_temps; i++)
                temp_registers[i] = qpu_rn(0);

        /* Reserve r3 for spilling-like operations in vc4_qpu_emit.c */
        reg_in_use[ACC_INDEX + 3] = true;

        foreach(node, &c->instructions) {
                struct qinst *qinst = (struct qinst *)node;

                if (qinst->dst.file == QFILE_TEMP)
                        reg_uses_remaining[qinst->dst.index]++;
                for (int i = 0; i < qir_get_op_nsrc(qinst->op); i++) {
                        if (qinst->src[i].file == QFILE_TEMP)
                                reg_uses_remaining[qinst->src[i].index]++;
                }
                if (qinst->op == QOP_FRAG_Z)
                        reg_in_use[3 + 32 + QPU_R_FRAG_PAYLOAD_ZW] = true;
                if (qinst->op == QOP_FRAG_W)
                        reg_in_use[3 + QPU_R_FRAG_PAYLOAD_ZW] = true;
        }

        foreach(node, &c->instructions) {
                struct qinst *qinst = (struct qinst *)node;

                for (int i = 0; i < qir_get_op_nsrc(qinst->op); i++) {
                        int index = qinst->src[i].index;

                        if (qinst->src[i].file != QFILE_TEMP)
                                continue;

                        if (reg_allocated[index] == -1) {
                                fprintf(stderr, "undefined reg use: ");
                                qir_dump_inst(qinst);
                                fprintf(stderr, "\n");
                        } else {
                                reg_uses_remaining[index]--;
                                if (reg_uses_remaining[index] == 0)
                                        reg_in_use[reg_allocated[index]] = false;
                        }
                }

                if (qinst->dst.file == QFILE_TEMP) {
                        if (reg_allocated[qinst->dst.index] == -1) {
                                int alloc;
                                for (alloc = 0;
                                     alloc < ARRAY_SIZE(reg_in_use);
                                     alloc++) {
                                        struct qpu_reg reg = vc4_regs[alloc];

                                        switch (qinst->op) {
                                        case QOP_PACK_SCALED:
                                                /* The pack flags require an
                                                 * A-file register.
                                                 */
                                                if (reg.mux != QPU_MUX_A)
                                                        continue;
                                                break;
                                        case QOP_TEX_RESULT:
                                        case QOP_TLB_COLOR_READ:
                                                /* Only R4-generating
                                                 * instructions get to store
                                                 * values in R4 for now, until
                                                 * we figure out how to do
                                                 * interference.
                                                 */
                                                if (reg.mux != QPU_MUX_R4)
                                                        continue;
                                                break;
                                        case QOP_FRAG_Z:
                                                if (reg.mux != QPU_MUX_B ||
                                                    reg.addr != QPU_R_FRAG_PAYLOAD_ZW) {
                                                        continue;
                                                }
                                                break;
                                        case QOP_FRAG_W:
                                                if (reg.mux != QPU_MUX_A ||
                                                    reg.addr != QPU_R_FRAG_PAYLOAD_ZW) {
                                                        continue;
                                                }
                                                break;
                                        default:
                                                if (reg.mux == QPU_MUX_R4)
                                                        continue;
                                                break;
                                        }

                                        if (!reg_in_use[alloc])
                                                break;
                                }
                                assert(alloc != ARRAY_SIZE(reg_in_use) && "need better reg alloc");
                                reg_in_use[alloc] = true;
                                reg_allocated[qinst->dst.index] = alloc;
                                temp_registers[qinst->dst.index] = vc4_regs[alloc];
                        }

                        reg_uses_remaining[qinst->dst.index]--;
                        if (reg_uses_remaining[qinst->dst.index] == 0) {
                                reg_in_use[reg_allocated[qinst->dst.index]] =
                                        false;
                        }
                }
        }

        free(reg_allocated);
        free(reg_uses_remaining);

        return temp_registers;
}
