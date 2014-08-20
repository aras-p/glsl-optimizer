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

#include "vc4_qpu.h"

static bool
writes_reg(uint64_t inst, uint32_t w)
{
        return (QPU_GET_FIELD(inst, QPU_WADDR_ADD) == w ||
                QPU_GET_FIELD(inst, QPU_WADDR_MUL) == w);
}

static bool
_reads_reg(uint64_t inst, uint32_t r, bool ignore_a, bool ignore_b)
{
        struct {
                uint32_t mux, addr;
        } src_regs[] = {
                { QPU_GET_FIELD(inst, QPU_ADD_A) },
                { QPU_GET_FIELD(inst, QPU_ADD_B) },
                { QPU_GET_FIELD(inst, QPU_MUL_A) },
                { QPU_GET_FIELD(inst, QPU_MUL_B) },
        };

        for (int i = 0; i < ARRAY_SIZE(src_regs); i++) {
                if (!ignore_a &&
                    src_regs[i].mux == QPU_MUX_A &&
                    (QPU_GET_FIELD(inst, QPU_RADDR_A) == r))
                        return true;

                if (!ignore_b &&
                    src_regs[i].mux == QPU_MUX_B &&
                    (QPU_GET_FIELD(inst, QPU_RADDR_B) == r))
                        return true;
        }

        return false;
}

static bool
reads_reg(uint64_t inst, uint32_t r)
{
        return _reads_reg(inst, r, false, false);
}

static bool
reads_a_reg(uint64_t inst, uint32_t r)
{
        return _reads_reg(inst, r, false, true);
}

static bool
reads_b_reg(uint64_t inst, uint32_t r)
{
        return _reads_reg(inst, r, true, false);
}

static bool
writes_sfu(uint64_t inst)
{
        return (writes_reg(inst, QPU_W_SFU_RECIP) ||
                writes_reg(inst, QPU_W_SFU_RECIPSQRT) ||
                writes_reg(inst, QPU_W_SFU_EXP) ||
                writes_reg(inst, QPU_W_SFU_LOG));
}

/**
 * Checks for the instruction restrictions from page 37 ("Summary of
 * Instruction Restrictions").
 */
void
vc4_qpu_validate(uint64_t *insts, uint32_t num_inst)
{
        for (int i = 0; i < num_inst; i++) {
                uint64_t inst = insts[i];

                if (QPU_GET_FIELD(inst, QPU_SIG) != QPU_SIG_PROG_END)
                        continue;

                /* "The Thread End instruction must not write to either physical
                 *  regfile A or B."
                 */
                assert(QPU_GET_FIELD(inst, QPU_WADDR_ADD) >= 32);
                assert(QPU_GET_FIELD(inst, QPU_WADDR_MUL) >= 32);

                /* Two delay slots will be executed. */
                assert(i + 2 <= num_inst);

                 for (int j = i; j < i + 2; j++) {
                         /* "The last three instructions of any program
                          *  (Thread End plus the following two delay-slot
                          *  instructions) must not do varyings read, uniforms
                          *  read or any kind of VPM, VDR, or VDW read or
                          *  write."
                          */
                         assert(!writes_reg(insts[j], QPU_W_VPM));
                         assert(!reads_reg(insts[j], QPU_R_VARY));
                         assert(!reads_reg(insts[j], QPU_R_UNIF));
                         assert(!reads_reg(insts[j], QPU_R_VPM));

                         /* "The Thread End instruction and the following two
                          *  delay slot instructions must not write or read
                          *  address 14 in either regfile A or B."
                          */
                         assert(!writes_reg(insts[j], 14));
                         assert(!reads_reg(insts[j], 14));

                 }

                 /* "The final program instruction (the second delay slot
                  *  instruction) must not do a TLB Z write."
                  */
                 assert(!writes_reg(insts[i + 2], QPU_W_TLB_Z));
        }

        /* "A scoreboard wait must not occur in the first two instructions of
         *  a fragment shader. This is either the explicit Wait for Scoreboard
         *  signal or an implicit wait with the first tile-buffer read or
         *  write instruction."
         */
        for (int i = 0; i < 2; i++) {
                uint64_t inst = insts[i];

                assert(QPU_GET_FIELD(inst, QPU_SIG) != QPU_SIG_COLOR_LOAD);
                assert(QPU_GET_FIELD(inst, QPU_SIG) !=
                       QPU_SIG_WAIT_FOR_SCOREBOARD);
                assert(!writes_reg(inst, QPU_W_TLB_COLOR_MS));
                assert(!writes_reg(inst, QPU_W_TLB_COLOR_ALL));
                assert(!writes_reg(inst, QPU_W_TLB_Z));

        }

        /* "If TMU_NOSWAP is written, the write must be three instructions
         *  before the first TMU write instruction.  For example, if
         *  TMU_NOSWAP is written in the first shader instruction, the first
         *  TMU write cannot occur before the 4th shader instruction."
         */
        int last_tmu_noswap = -10;
        for (int i = 0; i < num_inst; i++) {
                uint64_t inst = insts[i];

                assert((i - last_tmu_noswap) > 3 ||
                       (!writes_reg(inst, QPU_W_TMU0_S) &&
                        !writes_reg(inst, QPU_W_TMU1_S)));

                if (writes_reg(inst, QPU_W_TMU_NOSWAP))
                    last_tmu_noswap = i;
        }

        /* "An instruction must not read from a location in physical regfile A
         *  or B that was written to by the previous instruction."
         */
        for (int i = 0; i < num_inst - 1; i++) {
                uint64_t inst = insts[i];
                uint32_t add_waddr = QPU_GET_FIELD(inst, QPU_WADDR_ADD);
                uint32_t mul_waddr = QPU_GET_FIELD(inst, QPU_WADDR_MUL);
                uint32_t waddr_a, waddr_b;

                if (inst & QPU_WS) {
                        waddr_b = add_waddr;
                        waddr_a = mul_waddr;
                } else {
                        waddr_a = add_waddr;
                        waddr_b = mul_waddr;
                }

                assert(waddr_a >= 32 || !reads_a_reg(insts[i + 1], waddr_a));
                assert(waddr_b >= 32 || !reads_b_reg(insts[i + 1], waddr_b));
        }

        /* "After an SFU lookup instruction, accumulator r4 must not be read
         *  in the following two instructions. Any other instruction that
         *  results in r4 being written (that is, TMU read, TLB read, SFU
         *  lookup) cannot occur in the two instructions following an SFU
         *  lookup."
         */
        int last_sfu_inst = -10;
        for (int i = 0; i < num_inst - 1; i++) {
                uint64_t inst = insts[i];

                assert(i - last_sfu_inst > 2 ||
                       (!writes_sfu(inst) &&
                        !writes_reg(inst, QPU_W_TMU0_S) &&
                        !writes_reg(inst, QPU_W_TMU1_S) &&
                        QPU_GET_FIELD(inst, QPU_SIG) != QPU_SIG_COLOR_LOAD));

                if (writes_sfu(inst))
                        last_sfu_inst = i;
        }

        int last_r5_write = -10;
        for (int i = 0; i < num_inst - 1; i++) {
                uint64_t inst = insts[i];

                /* "An instruction that does a vector rotate by r5 must not
                 *  immediately follow an instruction that writes to r5."
                 */
                assert(last_r5_write != i - 1 ||
                       QPU_GET_FIELD(inst, QPU_SIG) != QPU_SIG_SMALL_IMM ||
                       QPU_GET_FIELD(inst, QPU_SMALL_IMM) != 48);
        }

        /* "An instruction that does a vector rotate must not immediately
         *  follow an instruction that writes to the accumulator that is being
         *  rotated.
         *
         * XXX: TODO.
         */

        /* "After an instruction that does a TLB Z write, the multisample mask
         *  must not be read as an instruction input argument in the following
         *  two instruction. The TLB Z write instruction can, however, be
         *  followed immediately by a TLB color write."
         */
        for (int i = 0; i < num_inst - 1; i++) {
                uint64_t inst = insts[i];
                if (writes_reg(inst, QPU_W_TLB_Z)) {
                        assert(!reads_a_reg(insts[i + 1], QPU_R_MS_REV_FLAGS));
                        assert(!reads_a_reg(insts[i + 2], QPU_R_MS_REV_FLAGS));
                }
        }

        /*
         * "A single instruction can only perform a maximum of one of the
         *  following closely coupled peripheral accesses in a single
         *  instruction: TMU write, TMU read, TLB write, TLB read, TLB
         *  combined color read and write, SFU write, Mutex read or Semaphore
         *  access."
         */
        for (int i = 0; i < num_inst - 1; i++) {
                uint64_t inst = insts[i];
                int accesses = 0;
                static const uint32_t specials[] = {
                        QPU_W_TLB_COLOR_MS,
                        QPU_W_TLB_COLOR_ALL,
                        QPU_W_TLB_Z,
                        QPU_W_TMU0_S,
                        QPU_W_TMU0_T,
                        QPU_W_TMU0_R,
                        QPU_W_TMU0_B,
                        QPU_W_TMU1_S,
                        QPU_W_TMU1_T,
                        QPU_W_TMU1_R,
                        QPU_W_TMU1_B,
                        QPU_W_SFU_RECIP,
                        QPU_W_SFU_RECIPSQRT,
                        QPU_W_SFU_EXP,
                        QPU_W_SFU_LOG,
                };

                for (int j = 0; j < ARRAY_SIZE(specials); j++) {
                        if (writes_reg(inst, specials[j]))
                                accesses++;
                }

                if (reads_reg(inst, QPU_R_MUTEX_ACQUIRE))
                        accesses++;

                /* XXX: semaphore, combined color read/write? */
                switch (QPU_GET_FIELD(inst, QPU_SIG)) {
                case QPU_SIG_COLOR_LOAD:
                case QPU_SIG_COLOR_LOAD_END:
                case QPU_SIG_LOAD_TMU0:
                case QPU_SIG_LOAD_TMU1:
                        accesses++;
                }

                assert(accesses <= 1);
        }
}
