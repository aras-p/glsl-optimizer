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

/**
 * Returns a mapping from QFILE_TEMP indices to struct qpu_regs.
 *
 * The return value should be freed by the caller.
 */
struct qpu_reg *
vc4_register_allocate(struct vc4_compile *c)
{
        struct simple_node *node;
        struct qpu_reg allocate_to_qpu_reg[4 + 32 + 32];
        bool reg_in_use[ARRAY_SIZE(allocate_to_qpu_reg)];
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

        uint32_t next_reg = 0;
        for (int i = 0; i < 4; i++)
                allocate_to_qpu_reg[next_reg++] = qpu_rn(i == 3 ? 4 : i);
        for (int i = 0; i < 32; i++)
                allocate_to_qpu_reg[next_reg++] = qpu_ra(i);
        for (int i = 0; i < 32; i++)
                allocate_to_qpu_reg[next_reg++] = qpu_rb(i);
        assert(next_reg == ARRAY_SIZE(allocate_to_qpu_reg));

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
                                        struct qpu_reg reg = allocate_to_qpu_reg[alloc];

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
                                temp_registers[qinst->dst.index] = allocate_to_qpu_reg[alloc];
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
