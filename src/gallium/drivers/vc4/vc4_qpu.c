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

#include <stdbool.h>
#include "vc4_qpu.h"

static uint64_t
set_src_raddr(uint64_t inst, struct qpu_reg src)
{
        if (src.mux == QPU_MUX_A) {
                /* These asserts could be better, checking to be sure we're
                 * not overwriting an actual use of a raddr of 0.
                 */
                assert(QPU_GET_FIELD(inst, QPU_RADDR_A) == 0 ||
                       QPU_GET_FIELD(inst, QPU_RADDR_A) == src.addr);
                return inst | QPU_SET_FIELD(src.addr, QPU_RADDR_A);
        }

        if (src.mux == QPU_MUX_B) {
                assert(QPU_GET_FIELD(inst, QPU_RADDR_B) == 0 ||
                       QPU_GET_FIELD(inst, QPU_RADDR_B) == src.addr);
                return inst | QPU_SET_FIELD(src.addr, QPU_RADDR_B);
        }

        return inst;
}

uint64_t
qpu_a_NOP()
{
        uint64_t inst = 0;

        inst |= QPU_SET_FIELD(QPU_A_NOP, QPU_OP_ADD);
        inst |= QPU_SET_FIELD(QPU_W_NOP, QPU_WADDR_ADD);
        inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);

        return inst;
}

uint64_t
qpu_m_NOP()
{
        uint64_t inst = 0;

        inst |= QPU_SET_FIELD(QPU_M_NOP, QPU_OP_MUL);
        inst |= QPU_SET_FIELD(QPU_W_NOP, QPU_WADDR_MUL);
        inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);

        return inst;
}

static uint64_t
qpu_a_dst(struct qpu_reg dst)
{
        uint64_t inst = 0;

        if (dst.mux <= QPU_MUX_R5) {
                /* Translate the mux to the ACCn values. */
                inst |= QPU_SET_FIELD(32 + dst.mux, QPU_WADDR_ADD);
        } else {
                inst |= QPU_SET_FIELD(dst.addr, QPU_WADDR_ADD);
                if (dst.mux == QPU_MUX_B)
                        inst |= QPU_WS;
        }

        return inst;
}

static uint64_t
qpu_m_dst(struct qpu_reg dst)
{
        uint64_t inst = 0;

        if (dst.mux <= QPU_MUX_R5) {
                /* Translate the mux to the ACCn values. */
                inst |= QPU_SET_FIELD(32 + dst.mux, QPU_WADDR_MUL);
        } else {
                inst |= QPU_SET_FIELD(dst.addr, QPU_WADDR_MUL);
                if (dst.mux == QPU_MUX_A)
                        inst |= QPU_WS;
        }

        return inst;
}

uint64_t
qpu_a_MOV(struct qpu_reg dst, struct qpu_reg src)
{
        uint64_t inst = 0;

        inst |= QPU_SET_FIELD(QPU_A_OR, QPU_OP_ADD);
        inst |= qpu_a_dst(dst);
        inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_ADD);
        inst |= QPU_SET_FIELD(src.mux, QPU_ADD_A);
        inst |= QPU_SET_FIELD(src.mux, QPU_ADD_B);
        inst |= set_src_raddr(inst, src);
        inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);

        return inst;
}

uint64_t
qpu_m_MOV(struct qpu_reg dst, struct qpu_reg src)
{
        uint64_t inst = 0;

        inst |= QPU_SET_FIELD(QPU_M_V8MIN, QPU_OP_MUL);
        inst |= qpu_m_dst(dst);
        inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_MUL);
        inst |= QPU_SET_FIELD(src.mux, QPU_MUL_A);
        inst |= QPU_SET_FIELD(src.mux, QPU_MUL_B);
        inst |= set_src_raddr(inst, src);
        inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);

        return inst;
}

uint64_t
qpu_load_imm_ui(struct qpu_reg dst, uint32_t val)
{
        uint64_t inst = 0;

        inst |= qpu_a_dst(dst);
        inst |= qpu_m_dst(qpu_rb(QPU_W_NOP));
        inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_ADD);
        inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_MUL);
        inst |= QPU_SET_FIELD(QPU_SIG_LOAD_IMM, QPU_SIG);
        inst |= val;

        return inst;
}

uint64_t
qpu_a_alu2(enum qpu_op_add op,
           struct qpu_reg dst, struct qpu_reg src0, struct qpu_reg src1)
{
        uint64_t inst = 0;

        inst |= QPU_SET_FIELD(op, QPU_OP_ADD);
        inst |= qpu_a_dst(dst);
        inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_ADD);
        inst |= QPU_SET_FIELD(src0.mux, QPU_ADD_A);
        inst |= set_src_raddr(inst, src0);
        inst |= QPU_SET_FIELD(src1.mux, QPU_ADD_B);
        inst |= set_src_raddr(inst, src1);
        inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);

        return inst;
}

uint64_t
qpu_m_alu2(enum qpu_op_mul op,
           struct qpu_reg dst, struct qpu_reg src0, struct qpu_reg src1)
{
        uint64_t inst = 0;

        set_src_raddr(inst, src0);
        set_src_raddr(inst, src1);

        inst |= QPU_SET_FIELD(op, QPU_OP_MUL);
        inst |= qpu_m_dst(dst);
        inst |= QPU_SET_FIELD(QPU_COND_ALWAYS, QPU_COND_MUL);
        inst |= QPU_SET_FIELD(src0.mux, QPU_MUL_A);
        inst |= set_src_raddr(inst, src0);
        inst |= QPU_SET_FIELD(src1.mux, QPU_MUL_B);
        inst |= set_src_raddr(inst, src1);
        inst |= QPU_SET_FIELD(QPU_SIG_NONE, QPU_SIG);

        return inst;
}

uint64_t
qpu_inst(uint64_t add, uint64_t mul)
{
        uint64_t merge = add | mul;

        /* If either one has no signal field, then use the other's signal field.
         * (since QPU_SIG_NONE != 0).
         */
        if (QPU_GET_FIELD(add, QPU_SIG) == QPU_SIG_NONE)
                merge = (merge & ~QPU_SIG_MASK) | (mul & QPU_SIG_MASK);
        else if (QPU_GET_FIELD(mul, QPU_SIG) == QPU_SIG_NONE)
                merge = (merge & ~QPU_SIG_MASK) | (add & QPU_SIG_MASK);
        else {
                assert(QPU_GET_FIELD(add, QPU_SIG) ==
                       QPU_GET_FIELD(mul, QPU_SIG));
        }

        return merge;
}

uint64_t
qpu_set_sig(uint64_t inst, uint32_t sig)
{
        assert(QPU_GET_FIELD(inst, QPU_SIG) == QPU_SIG_NONE);
        return (inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(sig, QPU_SIG);
}

