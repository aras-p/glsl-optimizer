/*
 * Copyright (c) 2014 Scott Mansell
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
#include "pipe/p_state.h"
#include "util/u_memory.h"
#include "tgsi/tgsi_parse.h"

#include "vc4_context.h"
#include "vc4_qpu.h"

static void
vc4_dump_program(const uint64_t *insts, uint count)
{
        for (int i = 0; i < count; i++) {
                fprintf(stderr, "0x%016"PRIx64" ", insts[i]);
                vc4_qpu_disasm(&insts[i], 1);
                fprintf(stderr, "\n");
        }
}

static struct vc4_shader_state *
vc4_shader_state_create(struct pipe_context *pctx,
                        const struct pipe_shader_state *cso)
{
        struct vc4_shader_state *so = CALLOC_STRUCT(vc4_shader_state);
        if (!so)
                return NULL;

        so->base.tokens = tgsi_dup_tokens(cso->tokens);

        return so;
}

static void *
vc4_fs_state_create(struct pipe_context *pctx,
                    const struct pipe_shader_state *cso)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        struct vc4_shader_state *so = vc4_shader_state_create(pctx, cso);
        if (!so)
                return NULL;

        uint64_t gen_fsc[100];
        uint64_t cur_inst;
        int gen_fsc_len = 0;
#if 1
        cur_inst = qpu_load_imm_f(qpu_r5(), 0.0f);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_MOV(qpu_r0(), qpu_vary()),
                            qpu_m_MOV(qpu_r3(), qpu_r5()));
        cur_inst |= QPU_PM;
        cur_inst |= QPU_SET_FIELD(QPU_PACK_MUL_8D, QPU_PACK);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_FADD(qpu_r0(), qpu_r0(), qpu_r5()),
                            qpu_m_MOV(qpu_r1(), qpu_vary()));
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_FADD(qpu_r1(), qpu_r1(), qpu_r5()),
                            qpu_m_MOV(qpu_r2(), qpu_vary()));
        cur_inst = (cur_inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(QPU_SIG_WAIT_FOR_SCOREBOARD, QPU_SIG);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_FADD(qpu_r2(), qpu_r2(), qpu_r5()),
                            qpu_m_MOV(qpu_r3(), qpu_r0()));
        cur_inst |= QPU_PM;
        cur_inst |= QPU_SET_FIELD(QPU_PACK_MUL_8A, QPU_PACK);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_NOP(),
                            qpu_m_MOV(qpu_r3(), qpu_r1()));
        cur_inst |= QPU_PM;
        cur_inst |= QPU_SET_FIELD(QPU_PACK_MUL_8B, QPU_PACK);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_NOP(),
                            qpu_m_MOV(qpu_r3(), qpu_r2()));
        cur_inst |= QPU_PM;
        cur_inst |= QPU_SET_FIELD(QPU_PACK_MUL_8C, QPU_PACK);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_MOV(qpu_tlbc(), qpu_r3()),
                            qpu_m_NOP());
        cur_inst = (cur_inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(QPU_SIG_PROG_END, QPU_SIG);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        cur_inst = (cur_inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(QPU_SIG_SCOREBOARD_UNLOCK, QPU_SIG);
        gen_fsc[gen_fsc_len++] = cur_inst;

#else

        /* drain the varyings. */
        for (int i = 0; i < 3; i++) {
                cur_inst = qpu_inst(qpu_a_MOV(qpu_ra(QPU_W_NOP), qpu_rb(QPU_R_NOP)),
                                    qpu_m_NOP());
                if (i == 1)
                        cur_inst = (cur_inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(QPU_SIG_WAIT_FOR_SCOREBOARD, QPU_SIG);
                gen_fsc[gen_fsc_len++] = cur_inst;

                cur_inst = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
                gen_fsc[gen_fsc_len++] = cur_inst;
        }

        /* some colors */
#if 1
        for (int i = 0; i < 4; i++) {
                cur_inst = qpu_load_imm_f(qpu_rn(i), .2 + i / 4.0);
                gen_fsc[gen_fsc_len++] = cur_inst;
        }

        for (int i = 0; i < 4; i++) {
                cur_inst = qpu_inst(qpu_a_NOP(),
                                    qpu_m_FMUL(qpu_ra(1),
                                               qpu_rn(i), qpu_rn(i)));
                cur_inst |= QPU_PM;
                cur_inst |= QPU_SET_FIELD(QPU_PACK_A_8A + i, QPU_PACK);
                gen_fsc[gen_fsc_len++] = cur_inst;
        }
#else
        cur_inst = qpu_load_imm_ui(qpu_ra(1), 0x22446688);
        gen_fsc[gen_fsc_len++] = cur_inst;
#endif

        cur_inst = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_MOV(qpu_tlbc(), qpu_ra(1)),
                            qpu_m_NOP());
        cur_inst = (cur_inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(QPU_SIG_PROG_END, QPU_SIG);
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        gen_fsc[gen_fsc_len++] = cur_inst;

        cur_inst = qpu_inst(qpu_a_NOP(), qpu_m_NOP());
        cur_inst = (cur_inst & ~QPU_SIG_MASK) | QPU_SET_FIELD(QPU_SIG_SCOREBOARD_UNLOCK, QPU_SIG);
        gen_fsc[gen_fsc_len++] = cur_inst;
#endif


        if (1)
                vc4_dump_program(gen_fsc, gen_fsc_len);
        vc4_qpu_validate(gen_fsc, gen_fsc_len);

        so->bo = vc4_bo_alloc_mem(vc4->screen, gen_fsc,
                                  gen_fsc_len * sizeof(uint64_t), "fs_code");

        return so;
}

static void *
vc4_vs_state_create(struct pipe_context *pctx,
                    const struct pipe_shader_state *cso)
{
        struct vc4_shader_state *so = vc4_shader_state_create(pctx, cso);
        if (!so)
                return NULL;


        return so;
}

static void
vc4_shader_state_delete(struct pipe_context *pctx, void *hwcso)
{
        struct pipe_shader_state *so = hwcso;

        free((void *)so->tokens);
        free(so);
}

static void
vc4_fp_state_bind(struct pipe_context *pctx, void *hwcso)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        vc4->prog.fs = hwcso;
        vc4->prog.dirty |= VC4_SHADER_DIRTY_FP;
        vc4->dirty |= VC4_DIRTY_PROG;
}

static void
vc4_vp_state_bind(struct pipe_context *pctx, void *hwcso)
{
        struct vc4_context *vc4 = vc4_context(pctx);
        vc4->prog.vs = hwcso;
        vc4->prog.dirty |= VC4_SHADER_DIRTY_VP;
        vc4->dirty |= VC4_DIRTY_PROG;
}

void
vc4_program_init(struct pipe_context *pctx)
{
        pctx->create_vs_state = vc4_vs_state_create;
        pctx->delete_vs_state = vc4_shader_state_delete;

        pctx->create_fs_state = vc4_fs_state_create;
        pctx->delete_fs_state = vc4_shader_state_delete;

        pctx->bind_fs_state = vc4_fp_state_bind;
        pctx->bind_vs_state = vc4_vp_state_bind;
}
