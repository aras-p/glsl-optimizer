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

#include "util/ralloc.h"
#include "util/register_allocate.h"
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

static void
vc4_alloc_reg_set(struct vc4_context *vc4)
{
        assert(vc4_regs[A_INDEX].addr == 0);
        assert(vc4_regs[B_INDEX].addr == 0);
        STATIC_ASSERT(ARRAY_SIZE(vc4_regs) == B_INDEX + 32);

        if (vc4->regs)
                return;

        vc4->regs = ra_alloc_reg_set(vc4, ARRAY_SIZE(vc4_regs));

        vc4->reg_class_any = ra_alloc_reg_class(vc4->regs);
        for (uint32_t i = 0; i < ARRAY_SIZE(vc4_regs); i++) {
                /* Reserve r3 for now, since we're using it for spilling-like
                 * operations in vc4_qpu_emit.c
                 */
                if (vc4_regs[i].mux == QPU_MUX_R3)
                        continue;

                /* R4 can't be written as a general purpose register. (it's
                 * TMU_NOSWAP as a write address).
                 */
                if (vc4_regs[i].mux == QPU_MUX_R4)
                        continue;

                ra_class_add_reg(vc4->regs, vc4->reg_class_any, i);
        }

        vc4->reg_class_a = ra_alloc_reg_class(vc4->regs);
        for (uint32_t i = A_INDEX; i < A_INDEX + 32; i++)
                ra_class_add_reg(vc4->regs, vc4->reg_class_a, i);

        ra_set_finalize(vc4->regs, NULL);
}

/**
 * Returns a mapping from QFILE_TEMP indices to struct qpu_regs.
 *
 * The return value should be freed by the caller.
 */
struct qpu_reg *
vc4_register_allocate(struct vc4_context *vc4, struct vc4_compile *c)
{
        struct simple_node *node;
        uint32_t def[c->num_temps];
        uint32_t use[c->num_temps];
        struct qpu_reg *temp_registers = calloc(c->num_temps,
                                                sizeof(*temp_registers));
        memset(def, 0, sizeof(def));
        memset(use, 0, sizeof(use));

        /* If things aren't ever written (undefined values), just read from
         * r0.
         */
        for (uint32_t i = 0; i < c->num_temps; i++)
                temp_registers[i] = qpu_rn(0);

        vc4_alloc_reg_set(vc4);

        struct ra_graph *g = ra_alloc_interference_graph(vc4->regs,
                                                         c->num_temps);

        for (uint32_t i = 0; i < c->num_temps; i++)
                ra_set_node_class(g, i, vc4->reg_class_any);

        /* Compute the live ranges so we can figure out interference, and
         * figure out our register classes and preallocated registers.
         */
        uint32_t ip = 0;
        foreach(node, &c->instructions) {
                struct qinst *inst = (struct qinst *)node;

                if (inst->dst.file == QFILE_TEMP) {
                        def[inst->dst.index] = ip;
                        use[inst->dst.index] = ip;
                }

                for (int i = 0; i < qir_get_op_nsrc(inst->op); i++) {
                        if (inst->src[i].file == QFILE_TEMP)
                                use[inst->src[i].index] = ip;
                }

                switch (inst->op) {
                case QOP_FRAG_Z:
                        def[inst->dst.index] = 0;
                        ra_set_node_reg(g, inst->dst.index,
                                        B_INDEX + QPU_R_FRAG_PAYLOAD_ZW);
                        break;

                case QOP_FRAG_W:
                        def[inst->dst.index] = 0;
                        ra_set_node_reg(g, inst->dst.index,
                                        A_INDEX + QPU_R_FRAG_PAYLOAD_ZW);
                        break;

                case QOP_TEX_RESULT:
                case QOP_TLB_COLOR_READ:
                        assert(vc4_regs[ACC_INDEX + 4].mux == QPU_MUX_R4);
                        ra_set_node_reg(g, inst->dst.index,
                                        ACC_INDEX + 4);
                        break;

                case QOP_PACK_SCALED:
                        /* The pack flags require an A-file dst register. */
                        ra_set_node_class(g, inst->dst.index, vc4->reg_class_a);
                        break;

                case QOP_UNPACK_8A:
                case QOP_UNPACK_8B:
                case QOP_UNPACK_8C:
                case QOP_UNPACK_8D:
                        /* The unpack flags require an A-file src register. */
                        ra_set_node_class(g, inst->src[0].index, vc4->reg_class_a);
                        break;

                default:
                        break;
                }

                ip++;
        }

        for (uint32_t i = 0; i < c->num_temps; i++) {
                for (uint32_t j = i + 1; j < c->num_temps; j++) {
                        if (!(def[i] >= use[j] || def[j] >= use[i]))
                                ra_add_node_interference(g, i, j);
                }
        }

        bool ok = ra_allocate(g);
        assert(ok);

        for (uint32_t i = 0; i < c->num_temps; i++) {
                temp_registers[i] = vc4_regs[ra_get_node_reg(g, i)];

                /* If the value's never used, just write to the NOP register
                 * for clarity in debug output.
                 */
                if (def[i] == use[i])
                        temp_registers[i] = qpu_ra(QPU_W_NOP);
        }

        ralloc_free(g);

        return temp_registers;
}
