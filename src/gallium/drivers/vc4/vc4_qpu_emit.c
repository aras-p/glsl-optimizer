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
#include <inttypes.h>

#include "vc4_context.h"
#include "vc4_qir.h"
#include "vc4_qpu.h"

static void
vc4_dump_program(struct qcompile *c)
{
        fprintf(stderr, "%s:\n", qir_get_stage_name(c->stage));

        for (int i = 0; i < c->num_qpu_insts; i++) {
                fprintf(stderr, "0x%016"PRIx64" ", c->qpu_insts[i]);
                vc4_qpu_disasm(&c->qpu_insts[i], 1);
                fprintf(stderr, "\n");
        }
}

void
vc4_generate_code(struct qcompile *c)
{
        uint64_t *insts = malloc(sizeof(uint64_t) * 1024); /* XXX: sizing */
        uint32_t ni = 0;
        struct qpu_reg allocate_to_qpu_reg[4 + 32 + 32];
        bool reg_in_use[ARRAY_SIZE(allocate_to_qpu_reg)];
        int *reg_allocated = calloc(c->num_temps, sizeof(*reg_allocated));
        int *reg_uses_remaining =
                calloc(c->num_temps, sizeof(*reg_uses_remaining));

        for (int i = 0; i < ARRAY_SIZE(reg_in_use); i++)
                reg_in_use[i] = false;
        for (int i = 0; i < c->num_temps; i++)
                reg_allocated[i] = -1;
        for (int i = 0; i < 4; i++)
                allocate_to_qpu_reg[i] = qpu_rn(i);
        for (int i = 0; i < 32; i++)
                allocate_to_qpu_reg[i + 4] = qpu_ra(i);
        for (int i = 0; i < 32; i++)
                allocate_to_qpu_reg[i + 4 + 32] = qpu_rb(i);

        struct simple_node *node;
        foreach(node, &c->instructions) {
                struct qinst *qinst = (struct qinst *)node;

                if (qinst->dst.file == QFILE_TEMP)
                        reg_uses_remaining[qinst->dst.index]++;
                for (int i = 0; i < qir_get_op_nsrc(qinst->op); i++) {
                        if (qinst->src[i].file == QFILE_TEMP)
                                reg_uses_remaining[qinst->src[i].index]++;
                }
        }

        switch (c->stage) {
        case QSTAGE_VERT:
        case QSTAGE_COORD:
                insts[ni++] = qpu_load_imm_ui(qpu_vrsetup(), 0x00401a00);
                insts[ni++] = qpu_load_imm_ui(qpu_vwsetup(), 0x00001a00);
                break;
        case QSTAGE_FRAG:
                break;
        }

        foreach(node, &c->instructions) {
                struct qinst *qinst = (struct qinst *)node;

#if 0
                fprintf(stderr, "translating qinst to qpu: ");
                qir_dump_inst(qinst);
                fprintf(stderr, "\n");
#endif

                static const struct {
                        uint32_t op;
                        bool is_mul;
                } translate[] = {
#define A(name) [QOP_##name] = {QPU_A_##name, false}
#define M(name) [QOP_##name] = {QPU_M_##name, true}
                        A(FADD),
                        A(FSUB),
                        A(FMIN),
                        A(FMAX),
                        A(FMINABS),
                        A(FMAXABS),
                        A(FTOI),

                        M(FMUL),
                };

                struct qpu_reg src[4];
                for (int i = 0; i < qir_get_op_nsrc(qinst->op); i++) {
                        int index = qinst->src[i].index;
                        switch (qinst->src[i].file) {
                        case QFILE_NULL:
                                src[i] = qpu_rn(0);
                                break;
                        case QFILE_TEMP:
                                assert(reg_allocated[index] != -1);
                                src[i] = allocate_to_qpu_reg[reg_allocated[index]];
                                reg_uses_remaining[index]--;
                                if (reg_uses_remaining[index] == 0)
                                        reg_in_use[reg_allocated[index]] = false;
                                break;
                        case QFILE_UNIF:
                                src[i] = qpu_unif();
                                break;
                        case QFILE_VARY:
                                src[i] = qpu_vary();
                                break;
                        }
                }

                struct qpu_reg dst;
                switch (qinst->dst.file) {
                case QFILE_NULL:
                        dst = qpu_ra(QPU_W_NOP);
                        break;

                case QFILE_TEMP:
                        if (reg_allocated[qinst->dst.index] == -1) {
                                int alloc;
                                for (alloc = 0;
                                     alloc < ARRAY_SIZE(reg_in_use);
                                     alloc++) {
                                        /* The pack flags require an A-file register. */
                                        if (qinst->op == QOP_PACK_SCALED &&
                                            allocate_to_qpu_reg[alloc].mux != QPU_MUX_A) {
                                                continue;
                                        }

                                        if (!reg_in_use[alloc])
                                                break;
                                }
                                assert(alloc != ARRAY_SIZE(reg_in_use) && "need better reg alloc");
                                reg_in_use[alloc] = true;
                                reg_allocated[qinst->dst.index] = alloc;
                        }

                        dst = allocate_to_qpu_reg[reg_allocated[qinst->dst.index]];

                        reg_uses_remaining[qinst->dst.index]--;
                        if (reg_uses_remaining[qinst->dst.index] == 0) {
                                reg_in_use[reg_allocated[qinst->dst.index]] =
                                        false;
                        }
                        break;

                case QFILE_VARY:
                case QFILE_UNIF:
                        assert(!"not reached");
                        break;
                }

                switch (qinst->op) {
                case QOP_MOV:
                        /* Skip emitting the MOV if it's a no-op. */
                        if (dst.mux == QPU_MUX_A || dst.mux == QPU_MUX_B ||
                            dst.mux != src[0].mux || dst.addr != src[0].addr) {
                                insts[ni++] = qpu_inst(qpu_a_MOV(dst, src[0]),
                                                       qpu_m_NOP());
                        }
                        break;

                case QOP_VPM_WRITE:
                        insts[ni++] = qpu_inst(qpu_a_MOV(qpu_ra(QPU_W_VPM),
                                                         src[0]),
                                               qpu_m_NOP());
                        break;

                case QOP_VPM_READ:
                        insts[ni++] = qpu_inst(qpu_a_MOV(dst,
                                                         qpu_ra(QPU_R_VPM)),
                                               qpu_m_NOP());
                        break;

                case QOP_PACK_COLORS:
                        for (int i = 0; i < 4; i++) {
                                insts[ni++] = qpu_inst(qpu_a_NOP(),
                                                       qpu_m_MOV(qpu_r5(), src[i]));
                                insts[ni - 1] |= QPU_PM;
                                insts[ni - 1] |= QPU_SET_FIELD(QPU_PACK_MUL_8A + i,
                                                               QPU_PACK);
                        }

                        insts[ni++] = qpu_inst(qpu_a_MOV(dst, qpu_r5()),
                                               qpu_m_NOP());

                        break;

                case QOP_TLB_COLOR_WRITE:
                        insts[ni++] = qpu_inst(qpu_a_MOV(qpu_tlbc(),
                                                         src[0]),
                                               qpu_m_NOP());
                        break;

                case QOP_PACK_SCALED:
                        insts[ni++] = qpu_inst(qpu_a_MOV(dst, src[0]),
                                               qpu_m_NOP());
                        insts[ni - 1] |= QPU_SET_FIELD(QPU_PACK_A_16A, QPU_PACK);

                        insts[ni++] = qpu_inst(qpu_a_MOV(dst, src[1]),
                                               qpu_m_NOP());
                        insts[ni - 1] |= QPU_SET_FIELD(QPU_PACK_A_16B, QPU_PACK);

                        break;

                default:
                        assert(qinst->op < ARRAY_SIZE(translate));
                        assert(translate[qinst->op].op != 0); /* NOPs */

                        /* If we have only one source, put it in the second
                         * argument slot as well so that we don't take up
                         * another raddr just to get unused data.
                         */
                        if (qir_get_op_nsrc(qinst->op) == 1)
                                src[1] = src[0];

                        if ((src[0].mux == QPU_MUX_A || src[0].mux == QPU_MUX_B) &&
                            (src[1].mux == QPU_MUX_A || src[1].mux == QPU_MUX_B) &&
                            src[0].addr != src[1].addr) {
                                insts[ni++] = qpu_inst(qpu_a_MOV(qpu_r5(), src[1]),
                                                       qpu_m_NOP());
                                src[1] = qpu_r5();
                        }

                        if (translate[qinst->op].is_mul) {
                                insts[ni++] = qpu_inst(qpu_a_NOP(),
                                                       qpu_m_alu2(translate[qinst->op].op,
                                                                  dst, src[0], src[1]));
                        } else {
                                insts[ni++] = qpu_inst(qpu_a_alu2(translate[qinst->op].op,
                                                                  dst, src[0], src[1]),
                                                       qpu_m_NOP());
                        }
                        break;
                }

                if ((dst.mux == QPU_MUX_A || dst.mux == QPU_MUX_B) &&
                    dst.addr < 32)
                        insts[ni++] = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        }

        /* thread end can't have VPM write */
        if (QPU_GET_FIELD(insts[ni - 1], QPU_WADDR_ADD) == QPU_W_VPM ||
            QPU_GET_FIELD(insts[ni - 1], QPU_WADDR_MUL) == QPU_W_VPM)
                insts[ni++] = qpu_inst(qpu_a_NOP(), qpu_m_NOP());

        insts[ni - 1] = qpu_set_sig(insts[ni - 1], QPU_SIG_PROG_END);
        insts[ni++] = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        insts[ni++] = qpu_inst(qpu_a_NOP(), qpu_m_NOP());

        switch (c->stage) {
        case QSTAGE_VERT:
        case QSTAGE_COORD:
                break;
        case QSTAGE_FRAG:
                insts[2] = qpu_set_sig(insts[2], QPU_SIG_WAIT_FOR_SCOREBOARD);
                insts[ni - 1] = qpu_set_sig(insts[ni - 1],
                                            QPU_SIG_SCOREBOARD_UNLOCK);
                break;
        }

        c->qpu_insts = insts;
        c->num_qpu_insts = ni;

        vc4_dump_program(c);
        vc4_qpu_validate(insts, ni);
}

