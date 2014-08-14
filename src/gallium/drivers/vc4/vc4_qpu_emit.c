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

        for (int i = 0; i < c->qpu_inst_count; i++) {
                fprintf(stderr, "0x%016"PRIx64" ", c->qpu_insts[i]);
                vc4_qpu_disasm(&c->qpu_insts[i], 1);
                fprintf(stderr, "\n");
        }
}

struct queued_qpu_inst {
        struct simple_node link;
        uint64_t inst;
};

static void
queue(struct qcompile *c, uint64_t inst)
{
        struct queued_qpu_inst *q = calloc(1, sizeof(*q));
        q->inst = inst;
        insert_at_tail(&c->qpu_inst_list, &q->link);
}

static uint64_t *
last_inst(struct qcompile *c)
{
        struct queued_qpu_inst *q =
                (struct queued_qpu_inst *)last_elem(&c->qpu_inst_list);
        return &q->inst;
}

/**
 * This is used to resolve the fact that we might register-allocate two
 * different operands of an instruction to the same physical register file
 * even though instructions have only one field for the register file source
 * address.
 *
 * In that case, we need to move one to a temporary that can be used in the
 * instruction, instead.
 */
static void
fixup_raddr_conflict(struct qcompile *c,
               struct qpu_reg src0, struct qpu_reg *src1)
{
        if ((src0.mux == QPU_MUX_A || src0.mux == QPU_MUX_B) &&
            (src1->mux == QPU_MUX_A || src1->mux == QPU_MUX_B) &&
            src0.addr != src1->addr) {
                queue(c, qpu_inst(qpu_a_MOV(qpu_r3(), *src1),
                                  qpu_m_NOP()));
                *src1 = qpu_r3();
        }
}

static void
serialize_one_inst(struct qcompile *c, uint64_t inst)
{
        if (c->qpu_inst_count >= c->qpu_inst_size) {
                c->qpu_inst_size = MAX2(16, c->qpu_inst_size * 2);
                c->qpu_insts = realloc(c->qpu_insts,
                                       c->qpu_inst_size * sizeof(uint64_t));
        }
        c->qpu_insts[c->qpu_inst_count++] = inst;
}

static void
serialize_insts(struct qcompile *c)
{
        int last_sfu_write = -10;

        while (!is_empty_list(&c->qpu_inst_list)) {
                struct queued_qpu_inst *q =
                        (struct queued_qpu_inst *)first_elem(&c->qpu_inst_list);
                uint32_t last_waddr_a = QPU_W_NOP, last_waddr_b = QPU_W_NOP;
                uint32_t raddr_a = QPU_GET_FIELD(q->inst, QPU_RADDR_A);
                uint32_t raddr_b = QPU_GET_FIELD(q->inst, QPU_RADDR_B);

                if (c->qpu_inst_count > 0) {
                        uint64_t last_inst = c->qpu_insts[c->qpu_inst_count -
                                                          1];
                        uint32_t last_waddr_add = QPU_GET_FIELD(last_inst,
                                                                QPU_WADDR_ADD);
                        uint32_t last_waddr_mul = QPU_GET_FIELD(last_inst,
                                                                QPU_WADDR_MUL);

                        if (last_inst & QPU_WS) {
                                last_waddr_a = last_waddr_mul;
                                last_waddr_b = last_waddr_add;
                        } else {
                                last_waddr_a = last_waddr_add;
                                last_waddr_b = last_waddr_mul;
                        }
                }

                uint32_t src_muxes[] = {
                        QPU_GET_FIELD(q->inst, QPU_ADD_A),
                        QPU_GET_FIELD(q->inst, QPU_ADD_B),
                        QPU_GET_FIELD(q->inst, QPU_MUL_A),
                        QPU_GET_FIELD(q->inst, QPU_MUL_B),
                };

                /* "An instruction must not read from a location in physical
                 *  regfile A or B that was written to by the previous
                 *  instruction."
                 */
                bool needs_raddr_vs_waddr_nop = false;
                bool reads_r4 = false;
                for (int i = 0; i < ARRAY_SIZE(src_muxes); i++) {
                        if ((raddr_a < 32 &&
                             src_muxes[i] == QPU_MUX_A &&
                             last_waddr_a == raddr_a) ||
                            (raddr_b < 32 &&
                             src_muxes[i] == QPU_MUX_B &&
                             last_waddr_b == raddr_b)) {
                                needs_raddr_vs_waddr_nop = true;
                        }
                        if (src_muxes[i] == QPU_MUX_R4)
                                reads_r4 = true;
                }

                if (needs_raddr_vs_waddr_nop) {
                        serialize_one_inst(c, qpu_inst(qpu_a_NOP(),
                                                       qpu_m_NOP()));
                }

                /* "After an SFU lookup instruction, accumulator r4 must not
                 *  be read in the following two instructions. Any other
                 *  instruction that results in r4 being written (that is, TMU
                 *  read, TLB read, SFU lookup) cannot occur in the two
                 *  instructions following an SFU lookup."
                 */
                if (reads_r4) {
                        while (c->qpu_inst_count - last_sfu_write < 3) {
                                serialize_one_inst(c, qpu_inst(qpu_a_NOP(),
                                                               qpu_m_NOP()));
                        }
                }

                uint32_t waddr_a = QPU_GET_FIELD(q->inst, QPU_WADDR_ADD);
                uint32_t waddr_m = QPU_GET_FIELD(q->inst, QPU_WADDR_MUL);
                if ((waddr_a >= QPU_W_SFU_RECIP && waddr_a <= QPU_W_SFU_LOG) ||
                    (waddr_m >= QPU_W_SFU_RECIP && waddr_m <= QPU_W_SFU_LOG)) {
                        last_sfu_write = c->qpu_inst_count;
                }

                serialize_one_inst(c, q->inst);

                remove_from_list(&q->link);
                free(q);
        }
}

void
vc4_generate_code(struct qcompile *c)
{
        struct qpu_reg allocate_to_qpu_reg[3 + 32 + 32];
        bool reg_in_use[ARRAY_SIZE(allocate_to_qpu_reg)];
        int *reg_allocated = calloc(c->num_temps, sizeof(*reg_allocated));
        int *reg_uses_remaining =
                calloc(c->num_temps, sizeof(*reg_uses_remaining));

        for (int i = 0; i < ARRAY_SIZE(reg_in_use); i++)
                reg_in_use[i] = false;
        for (int i = 0; i < c->num_temps; i++)
                reg_allocated[i] = -1;
        for (int i = 0; i < 3; i++)
                allocate_to_qpu_reg[i] = qpu_rn(i);
        for (int i = 0; i < 32; i++)
                allocate_to_qpu_reg[i + 3] = qpu_ra(i);
        for (int i = 0; i < 32; i++)
                allocate_to_qpu_reg[i + 3 + 32] = qpu_rb(i);

        make_empty_list(&c->qpu_inst_list);

        struct simple_node *node;
        foreach(node, &c->instructions) {
                struct qinst *qinst = (struct qinst *)node;

                if (qinst->dst.file == QFILE_TEMP)
                        reg_uses_remaining[qinst->dst.index]++;
                for (int i = 0; i < qir_get_op_nsrc(qinst->op); i++) {
                        if (qinst->src[i].file == QFILE_TEMP)
                                reg_uses_remaining[qinst->src[i].index]++;
                }
                if (qinst->op == QOP_TLB_PASSTHROUGH_Z_WRITE ||
                    qinst->op == QOP_FRAG_Z)
                        reg_in_use[3 + 32 + QPU_R_FRAG_PAYLOAD_ZW] = true;
        }

        switch (c->stage) {
        case QSTAGE_VERT:
        case QSTAGE_COORD:
                queue(c, qpu_load_imm_ui(qpu_vrsetup(),
                                         (0x00001a00 +
                                          0x00100000 * c->num_inputs)));
                queue(c, qpu_load_imm_ui(qpu_vwsetup(), 0x00001a00));
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
                        A(ITOF),

                        M(FMUL),
                };

                static const uint32_t compareflags[] = {
                        [QOP_SEQ - QOP_SEQ] = QPU_COND_ZS,
                        [QOP_SNE - QOP_SEQ] = QPU_COND_ZC,
                        [QOP_SLT - QOP_SEQ] = QPU_COND_NS,
                        [QOP_SGE - QOP_SEQ] = QPU_COND_NC,
                };

                struct qpu_reg src[4];
                for (int i = 0; i < qir_get_op_nsrc(qinst->op); i++) {
                        int index = qinst->src[i].index;
                        switch (qinst->src[i].file) {
                        case QFILE_NULL:
                                src[i] = qpu_rn(0);
                                break;
                        case QFILE_TEMP:
                                if (reg_allocated[index] == -1) {
                                        fprintf(stderr, "undefined reg use: ");
                                        qir_dump_inst(qinst);
                                        fprintf(stderr, "\n");

                                        src[i] = qpu_rn(0);
                                } else {
                                        src[i] = allocate_to_qpu_reg[reg_allocated[index]];
                                        reg_uses_remaining[index]--;
                                        if (reg_uses_remaining[index] == 0)
                                                reg_in_use[reg_allocated[index]] = false;
                                }
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
                                queue(c, qpu_inst(qpu_a_MOV(dst, src[0]),
                                                  qpu_m_NOP()));
                        }
                        break;

                case QOP_CMP:
                        queue(c, qpu_inst(qpu_a_MOV(qpu_ra(QPU_W_NOP),
                                                    src[0]),
                                          qpu_m_NOP()));
                        *last_inst(c) |= QPU_SF;

                        if (dst.mux <= QPU_MUX_R3) {
                                fixup_raddr_conflict(c, src[1], &src[2]);
                                queue(c, qpu_inst(qpu_a_MOV(dst, src[1]),
                                                  qpu_m_MOV(dst, src[2])));
                                *last_inst(c) = ((*last_inst(c) & ~(QPU_COND_ADD_MASK |
                                                                    QPU_COND_MUL_MASK))
                                                 | QPU_SET_FIELD(QPU_COND_NS,
                                                                 QPU_COND_ADD)
                                                 | QPU_SET_FIELD(QPU_COND_NC,
                                                                 QPU_COND_MUL));
                        } else {
                                if (dst.mux == src[1].mux &&
                                    dst.addr == src[1].addr) {
                                        queue(c, qpu_inst(qpu_a_MOV(dst, src[1]),
                                                          qpu_m_NOP()));

                                        queue(c, qpu_inst(qpu_a_MOV(dst, src[2]),
                                                          qpu_m_NOP()));
                                        *last_inst(c) = ((*last_inst(c) & ~(QPU_COND_ADD_MASK))
                                                         | QPU_SET_FIELD(QPU_COND_NC,
                                                                         QPU_COND_ADD));
                                } else {
                                        queue(c, qpu_inst(qpu_a_MOV(dst, src[2]),
                                                          qpu_m_NOP()));

                                        queue(c, qpu_inst(qpu_a_MOV(dst, src[1]),
                                                          qpu_m_NOP()));
                                        *last_inst(c) = ((*last_inst(c) & ~(QPU_COND_ADD_MASK))
                                                         | QPU_SET_FIELD(QPU_COND_NS,
                                                                         QPU_COND_ADD));
                                }
                        }
                        break;

                case QOP_SEQ:
                case QOP_SNE:
                case QOP_SGE:
                case QOP_SLT:
                        fixup_raddr_conflict(c, src[0], &src[1]);
                        queue(c, qpu_inst(qpu_a_SUB(qpu_ra(QPU_W_NOP),
                                                    src[0], src[1]),
                                          qpu_m_NOP()));
                        *last_inst(c) |= QPU_SF;

                        queue(c, qpu_load_imm_f(dst, 0.0));
                        queue(c, qpu_load_imm_f(dst, 1.0));
                        *last_inst(c) = ((*last_inst(c) & ~QPU_COND_ADD_MASK)
                                         | QPU_SET_FIELD(compareflags[qinst->op - QOP_SEQ],
                                                         QPU_COND_ADD));

                        break;

                case QOP_VPM_WRITE:
                        queue(c, qpu_inst(qpu_a_MOV(qpu_ra(QPU_W_VPM), src[0]),
                                          qpu_m_NOP()));
                        break;

                case QOP_VPM_READ:
                        queue(c, qpu_inst(qpu_a_MOV(dst, qpu_ra(QPU_R_VPM)),
                                          qpu_m_NOP()));
                        break;

                case QOP_RCP:
                case QOP_RSQ:
                case QOP_EXP2:
                case QOP_LOG2:
                        switch (qinst->op) {
                        case QOP_RCP:
                                queue(c, qpu_inst(qpu_a_MOV(qpu_rb(QPU_W_SFU_RECIP),
                                                            src[0]),
                                                  qpu_m_NOP()));
                                break;
                        case QOP_RSQ:
                                queue(c, qpu_inst(qpu_a_MOV(qpu_rb(QPU_W_SFU_RECIPSQRT),
                                                            src[0]),
                                                  qpu_m_NOP()));
                                break;
                        case QOP_EXP2:
                                queue(c, qpu_inst(qpu_a_MOV(qpu_rb(QPU_W_SFU_EXP),
                                                            src[0]),
                                                  qpu_m_NOP()));
                                break;
                        case QOP_LOG2:
                                queue(c, qpu_inst(qpu_a_MOV(qpu_rb(QPU_W_SFU_LOG),
                                                            src[0]),
                                                  qpu_m_NOP()));
                                break;
                        default:
                                abort();
                        }

                        queue(c, qpu_inst(qpu_a_MOV(dst, qpu_r4()),
                                          qpu_m_NOP()));

                        break;

                case QOP_PACK_COLORS:
                        for (int i = 0; i < 4; i++) {
                                queue(c, qpu_inst(qpu_a_NOP(),
                                                  qpu_m_MOV(qpu_r3(), src[i])));
                                *last_inst(c) |= QPU_PM;
                                *last_inst(c) |= QPU_SET_FIELD(QPU_PACK_MUL_8A + i,
                                                               QPU_PACK);
                        }

                        queue(c, qpu_inst(qpu_a_MOV(dst, qpu_r3()),
                                          qpu_m_NOP()));

                        break;

                case QOP_FRAG_X:
                        queue(c, qpu_inst(qpu_a_ITOF(dst,
                                                     qpu_ra(QPU_R_XY_PIXEL_COORD)),
                                          qpu_m_NOP()));
                        break;

                case QOP_FRAG_Y:
                        queue(c, qpu_inst(qpu_a_ITOF(dst,
                                                     qpu_rb(QPU_R_XY_PIXEL_COORD)),
                                          qpu_m_NOP()));
                        break;

                case QOP_FRAG_Z:
                        queue(c, qpu_inst(qpu_a_ITOF(dst,
                                                     qpu_rb(QPU_R_FRAG_PAYLOAD_ZW)),
                                          qpu_m_NOP()));
                        break;

                case QOP_FRAG_RCP_W:
                        queue(c, qpu_inst(qpu_a_MOV(qpu_rb(QPU_W_SFU_RECIP),
                                                    qpu_ra(QPU_R_FRAG_PAYLOAD_ZW)),
                                          qpu_m_NOP()));

                        queue(c, qpu_inst(qpu_a_MOV(dst, qpu_r4()),
                                          qpu_m_NOP()));
                        break;

                case QOP_TLB_PASSTHROUGH_Z_WRITE:
                        queue(c, qpu_inst(qpu_a_MOV(qpu_ra(QPU_W_TLB_Z),
                                                    qpu_rb(QPU_R_FRAG_PAYLOAD_ZW)),
                                          qpu_m_NOP()));
                        break;

                case QOP_TLB_COLOR_READ:
                        queue(c, qpu_inst(qpu_a_NOP(), qpu_m_NOP()));
                        *last_inst(c) = qpu_set_sig(*last_inst(c),
                                                    QPU_SIG_COLOR_LOAD);

                        break;

                case QOP_TLB_COLOR_WRITE:
                        queue(c, qpu_inst(qpu_a_MOV(qpu_tlbc(),
                                                    src[0]),
                                          qpu_m_NOP()));
                        break;

                case QOP_VARY_ADD_C:
                        queue(c, qpu_inst(qpu_a_FADD(dst,
                                                     src[0], qpu_r5()),
                                          qpu_m_NOP()));
                        break;

                case QOP_PACK_SCALED: {
                        uint64_t a = (qpu_inst(qpu_a_MOV(dst, src[0]),
                                               qpu_m_NOP()) |
                                      QPU_SET_FIELD(QPU_PACK_A_16A,
                                                    QPU_PACK));
                        uint64_t b = (qpu_inst(qpu_a_MOV(dst, src[1]),
                                               qpu_m_NOP()) |
                                      QPU_SET_FIELD(QPU_PACK_A_16B,
                                                    QPU_PACK));

                        if (dst.mux == src[1].mux && dst.addr == src[1].addr) {
                                queue(c, b);
                                queue(c, a);
                        } else {
                                queue(c, a);
                                queue(c, b);
                        }
                        break;
                }

                case QOP_TEX_S:
                case QOP_TEX_T:
                case QOP_TEX_R:
                case QOP_TEX_B:
                        queue(c, qpu_inst(qpu_a_MOV(qpu_rb(QPU_W_TMU0_S +
                                                           (qinst->op -
                                                            QOP_TEX_S)),
                                                    src[0]),
                                          qpu_m_NOP()));
                        break;

                case QOP_TEX_RESULT:
                        queue(c, qpu_inst(qpu_a_NOP(), qpu_m_NOP()));
                        *last_inst(c) = qpu_set_sig(*last_inst(c),
                                                    QPU_SIG_LOAD_TMU0);

                        break;

                case QOP_R4_UNPACK_A:
                case QOP_R4_UNPACK_B:
                case QOP_R4_UNPACK_C:
                case QOP_R4_UNPACK_D:
                        queue(c, qpu_inst(qpu_a_MOV(dst, qpu_r4()),
                                          qpu_m_NOP()));
                        *last_inst(c) |= QPU_PM;
                        *last_inst(c) |= QPU_SET_FIELD(QPU_UNPACK_R4_8A +
                                                       (qinst->op -
                                                        QOP_R4_UNPACK_A),
                                                       QPU_UNPACK);

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

                        fixup_raddr_conflict(c, src[0], &src[1]);

                        if (translate[qinst->op].is_mul) {
                                queue(c, qpu_inst(qpu_a_NOP(),
                                                  qpu_m_alu2(translate[qinst->op].op,
                                                             dst,
                                                             src[0], src[1])));
                        } else {
                                queue(c, qpu_inst(qpu_a_alu2(translate[qinst->op].op,
                                                             dst,
                                                             src[0], src[1]),
                                                  qpu_m_NOP()));
                        }
                        break;
                }
        }

        serialize_insts(c);

        /* thread end can't have VPM write */
        if (QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
                          QPU_WADDR_ADD) == QPU_W_VPM ||
            QPU_GET_FIELD(c->qpu_insts[c->qpu_inst_count - 1],
                          QPU_WADDR_MUL) == QPU_W_VPM) {
                serialize_one_inst(c, qpu_inst(qpu_a_NOP(), qpu_m_NOP()));
        }

        c->qpu_insts[c->qpu_inst_count - 1] =
                qpu_set_sig(c->qpu_insts[c->qpu_inst_count - 1],
                            QPU_SIG_PROG_END);
        serialize_one_inst(c, qpu_inst(qpu_a_NOP(), qpu_m_NOP()));
        serialize_one_inst(c, qpu_inst(qpu_a_NOP(), qpu_m_NOP()));

        switch (c->stage) {
        case QSTAGE_VERT:
        case QSTAGE_COORD:
                break;
        case QSTAGE_FRAG:
                c->qpu_insts[2] = qpu_set_sig(c->qpu_insts[2],
                                              QPU_SIG_WAIT_FOR_SCOREBOARD);
                c->qpu_insts[c->qpu_inst_count - 1] =
                        qpu_set_sig(c->qpu_insts[c->qpu_inst_count - 1],
                                    QPU_SIG_SCOREBOARD_UNLOCK);
                break;
        }

        if (vc4_debug & VC4_DEBUG_QPU)
                vc4_dump_program(c);

        vc4_qpu_validate(c->qpu_insts, c->qpu_inst_count);
}
