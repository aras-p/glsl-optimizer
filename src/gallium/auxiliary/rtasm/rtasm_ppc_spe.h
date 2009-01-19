/*
 * (C) Copyright IBM Corporation 2008
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file
 * Real-time assembly generation interface for Cell B.E. SPEs.
 * For details, see /opt/cell/sdk/docs/arch/SPU_ISA_v1.2_27Jan2007_pub.pdf
 *
 * \author Ian Romanick <idr@us.ibm.com>
 * \author Brian Paul
 */

#ifndef RTASM_PPC_SPE_H
#define RTASM_PPC_SPE_H

/** 4 bytes per instruction */
#define SPE_INST_SIZE 4

/** number of general-purpose SIMD registers */
#define SPE_NUM_REGS  128

/** Return Address register (aka $lr / Link Register) */
#define SPE_REG_RA  0

/** Stack Pointer register (aka $sp) */
#define SPE_REG_SP  1


struct spe_function
{
   uint32_t *store;  /**< instruction buffer */
   uint num_inst;
   uint max_inst;

   /**
    * The "set count" reflects the number of nested register sets
    * are allowed.  In the unlikely case that we exceed the set count,
    * register allocation will start to be confused, which is critical
    * enough that we check for it.
    */
   unsigned char set_count;

   /** 
    * Flags for used and unused registers.  Each byte corresponds to a
    * register; a 0 in that byte means that the register is available.
    * A value of 1 means that the register was allocated in the current
    * register set.  Any other value N means that the register was allocated
    * N register sets ago.
     *
     * \sa
     * spe_allocate_register, spe_allocate_available_register,
     * spe_allocate_register_set, spe_release_register_set, spe_release_register, 
     */
    unsigned char regs[SPE_NUM_REGS];

    boolean print; /**< print/dump instructions as they're emitted? */
    int indent;    /**< number of spaces to indent */
};


extern void spe_init_func(struct spe_function *p, uint code_size);
extern void spe_release_func(struct spe_function *p);
extern uint spe_code_size(const struct spe_function *p);

extern int spe_allocate_available_register(struct spe_function *p);
extern int spe_allocate_register(struct spe_function *p, int reg);
extern void spe_release_register(struct spe_function *p, int reg);
extern void spe_allocate_register_set(struct spe_function *p);
extern void spe_release_register_set(struct spe_function *p);

extern uint spe_get_registers_used(const struct spe_function *p, ubyte used[]);

extern void spe_print_code(struct spe_function *p, boolean enable);
extern void spe_indent(struct spe_function *p, int spaces);
extern void spe_comment(struct spe_function *p, int rel_indent, const char *s);


#endif /* RTASM_PPC_SPE_H */

#ifndef EMIT
#define EMIT(_name, _op) \
    extern void _name (struct spe_function *p);
#define EMIT_(_name, _op) \
    extern void _name (struct spe_function *p, int rT);
#define EMIT_R(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int rA);
#define EMIT_RR(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int rA, int rB);
#define EMIT_RRR(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int rA, int rB, int rC);
#define EMIT_RI7(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int rA, int imm);
#define EMIT_RI8(_name, _op, bias) \
    extern void _name (struct spe_function *p, int rT, int rA, int imm);
#define EMIT_RI10(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int rA, int imm);
#define EMIT_RI10s(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int rA, int imm);
#define EMIT_RI16(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int imm);
#define EMIT_RI18(_name, _op) \
    extern void _name (struct spe_function *p, int rT, int imm);
#define EMIT_I16(_name, _op) \
    extern void _name (struct spe_function *p, int imm);
#define UNDEF_EMIT_MACROS
#endif /* EMIT */


/* Memory load / store instructions
 */
EMIT_RR  (spe_lqx,  0x1c4)
EMIT_RI16(spe_lqa,  0x061)
EMIT_RI16(spe_lqr,  0x067)
EMIT_RR  (spe_stqx, 0x144)
EMIT_RI16(spe_stqa, 0x041)
EMIT_RI16(spe_stqr, 0x047)
EMIT_RI7 (spe_cbd,  0x1f4)
EMIT_RR  (spe_cbx,  0x1d4)
EMIT_RI7 (spe_chd,  0x1f5)
EMIT_RI7 (spe_chx,  0x1d5)
EMIT_RI7 (spe_cwd,  0x1f6)
EMIT_RI7 (spe_cwx,  0x1d6)
EMIT_RI7 (spe_cdd,  0x1f7)
EMIT_RI7 (spe_cdx,  0x1d7)


/* Constant formation instructions
 */
EMIT_RI16(spe_ilh,   0x083)
EMIT_RI16(spe_ilhu,  0x082)
EMIT_RI16(spe_il,    0x081)
EMIT_RI18(spe_ila,   0x021)
EMIT_RI16(spe_iohl,  0x0c1)
EMIT_RI16(spe_fsmbi, 0x065)



/* Integer and logical instructions
 */
EMIT_RR  (spe_ah,      0x0c8)
EMIT_RI10(spe_ahi,     0x01d)
EMIT_RR  (spe_a,       0x0c0)
EMIT_RI10s(spe_ai,      0x01c)
EMIT_RR  (spe_sfh,     0x048)
EMIT_RI10(spe_sfhi,    0x00d)
EMIT_RR  (spe_sf,      0x040)
EMIT_RI10(spe_sfi,     0x00c)
EMIT_RR  (spe_addx,    0x340)
EMIT_RR  (spe_cg,      0x0c2)
EMIT_RR  (spe_cgx,     0x342)
EMIT_RR  (spe_sfx,     0x341)
EMIT_RR  (spe_bg,      0x042)
EMIT_RR  (spe_bgx,     0x343)
EMIT_RR  (spe_mpy,     0x3c4)
EMIT_RR  (spe_mpyu,    0x3cc)
EMIT_RI10(spe_mpyi,    0x074)
EMIT_RI10(spe_mpyui,   0x075)
EMIT_RRR (spe_mpya,    0x00c)
EMIT_RR  (spe_mpyh,    0x3c5)
EMIT_RR  (spe_mpys,    0x3c7)
EMIT_RR  (spe_mpyhh,   0x3c6)
EMIT_RR  (spe_mpyhha,  0x346)
EMIT_RR  (spe_mpyhhu,  0x3ce)
EMIT_RR  (spe_mpyhhau, 0x34e)
EMIT_R   (spe_clz,     0x2a5)
EMIT_R   (spe_cntb,    0x2b4)
EMIT_R   (spe_fsmb,    0x1b6)
EMIT_R   (spe_fsmh,    0x1b5)
EMIT_R   (spe_fsm,     0x1b4)
EMIT_R   (spe_gbb,     0x1b2)
EMIT_R   (spe_gbh,     0x1b1)
EMIT_R   (spe_gb,      0x1b0)
EMIT_RR  (spe_avgb,    0x0d3)
EMIT_RR  (spe_absdb,   0x053)
EMIT_RR  (spe_sumb,    0x253)
EMIT_R   (spe_xsbh,    0x2b6)
EMIT_R   (spe_xshw,    0x2ae)
EMIT_R   (spe_xswd,    0x2a6)
EMIT_RR  (spe_and,     0x0c1)
EMIT_RR  (spe_andc,    0x2c1)
EMIT_RI10s(spe_andbi,   0x016)
EMIT_RI10s(spe_andhi,   0x015)
EMIT_RI10s(spe_andi,    0x014)
EMIT_RR  (spe_or,      0x041)
EMIT_RR  (spe_orc,     0x2c9)
EMIT_RI10s(spe_orbi,    0x006)
EMIT_RI10s(spe_orhi,    0x005)
EMIT_RI10s(spe_ori,     0x004)
EMIT_R   (spe_orx,     0x1f0)
EMIT_RR  (spe_xor,     0x241)
EMIT_RI10s(spe_xorbi,   0x046)
EMIT_RI10s(spe_xorhi,   0x045)
EMIT_RI10s(spe_xori,    0x044)
EMIT_RR  (spe_nand,    0x0c9)
EMIT_RR  (spe_nor,     0x049)
EMIT_RR  (spe_eqv,     0x249)
EMIT_RRR (spe_selb,    0x008)
EMIT_RRR (spe_shufb,   0x00b)


/* Shift and rotate instructions
 */
EMIT_RR  (spe_shlh,      0x05f)
EMIT_RI7 (spe_shlhi,     0x07f)
EMIT_RR  (spe_shl,       0x05b)
EMIT_RI7 (spe_shli,      0x07b)
EMIT_RR  (spe_shlqbi,    0x1db)
EMIT_RI7 (spe_shlqbii,   0x1fb)
EMIT_RR  (spe_shlqby,    0x1df)
EMIT_RI7 (spe_shlqbyi,   0x1ff)
EMIT_RR  (spe_shlqbybi,  0x1cf)
EMIT_RR  (spe_roth,      0x05c)
EMIT_RI7 (spe_rothi,     0x07c)
EMIT_RR  (spe_rot,       0x058)
EMIT_RI7 (spe_roti,      0x078)
EMIT_RR  (spe_rotqby,    0x1dc)
EMIT_RI7 (spe_rotqbyi,   0x1fc)
EMIT_RR  (spe_rotqbybi,  0x1cc)
EMIT_RR  (spe_rotqbi,    0x1d8)
EMIT_RI7 (spe_rotqbii,   0x1f8)
EMIT_RR  (spe_rothm,     0x05d)
EMIT_RI7 (spe_rothmi,    0x07d)
EMIT_RR  (spe_rotm,      0x059)
EMIT_RI7 (spe_rotmi,     0x079)
EMIT_RR  (spe_rotqmby,   0x1dd)
EMIT_RI7 (spe_rotqmbyi,  0x1fd)
EMIT_RR  (spe_rotqmbybi, 0x1cd)
EMIT_RR  (spe_rotqmbi,   0x1c9)
EMIT_RI7 (spe_rotqmbii,  0x1f9)
EMIT_RR  (spe_rotmah,    0x05e)
EMIT_RI7 (spe_rotmahi,   0x07e)
EMIT_RR  (spe_rotma,     0x05a)
EMIT_RI7 (spe_rotmai,    0x07a)


/* Compare, branch, and halt instructions
 */
EMIT_RR  (spe_heq,       0x3d8)
EMIT_RI10(spe_heqi,      0x07f)
EMIT_RR  (spe_hgt,       0x258)
EMIT_RI10(spe_hgti,      0x04f)
EMIT_RR  (spe_hlgt,      0x2d8)
EMIT_RI10(spe_hlgti,     0x05f)
EMIT_RR  (spe_ceqb,      0x3d0)
EMIT_RI10(spe_ceqbi,     0x07e)
EMIT_RR  (spe_ceqh,      0x3c8)
EMIT_RI10(spe_ceqhi,     0x07d)
EMIT_RR  (spe_ceq,       0x3c0)
EMIT_RI10(spe_ceqi,      0x07c)
EMIT_RR  (spe_cgtb,      0x250)
EMIT_RI10(spe_cgtbi,     0x04e)
EMIT_RR  (spe_cgth,      0x248)
EMIT_RI10(spe_cgthi,     0x04d)
EMIT_RR  (spe_cgt,       0x240)
EMIT_RI10(spe_cgti,      0x04c)
EMIT_RR  (spe_clgtb,     0x2d0)
EMIT_RI10(spe_clgtbi,    0x05e)
EMIT_RR  (spe_clgth,     0x2c8)
EMIT_RI10(spe_clgthi,    0x05d)
EMIT_RR  (spe_clgt,      0x2c0)
EMIT_RI10(spe_clgti,     0x05c)
EMIT_I16 (spe_br,        0x064)
EMIT_I16 (spe_bra,       0x060)
EMIT_RI16(spe_brsl,      0x066)
EMIT_RI16(spe_brasl,     0x062)
EMIT_RI16(spe_brnz,      0x042)
EMIT_RI16(spe_brz,       0x040)
EMIT_RI16(spe_brhnz,     0x046)
EMIT_RI16(spe_brhz,      0x044)

/* Control instructions
 */
EMIT     (spe_lnop,      0x001)

extern void
spe_lqd(struct spe_function *p, int rT, int rA, int offset);

extern void
spe_stqd(struct spe_function *p, int rT, int rA, int offset);

extern void spe_bi(struct spe_function *p, int rA, int d, int e);
extern void spe_iret(struct spe_function *p, int rA, int d, int e);
extern void spe_bisled(struct spe_function *p, int rT, int rA,
    int d, int e);
extern void spe_bisl(struct spe_function *p, int rT, int rA,
    int d, int e);
extern void spe_biz(struct spe_function *p, int rT, int rA,
    int d, int e);
extern void spe_binz(struct spe_function *p, int rT, int rA,
    int d, int e);
extern void spe_bihz(struct spe_function *p, int rT, int rA,
    int d, int e);
extern void spe_bihnz(struct spe_function *p, int rT, int rA,
    int d, int e);


/** Load/splat immediate float into rT. */
extern void
spe_load_float(struct spe_function *p, int rT, float x);

/** Load/splat immediate int into rT. */
extern void
spe_load_int(struct spe_function *p, int rT, int i);

/** Load/splat immediate unsigned int into rT. */
extern void
spe_load_uint(struct spe_function *p, int rT, uint ui);

/** And immediate value into rT. */
extern void
spe_and_uint(struct spe_function *p, int rT, int rA, uint ui);

/** Xor immediate value into rT. */
extern void
spe_xor_uint(struct spe_function *p, int rT, int rA, uint ui);

/** Compare equal with immediate value. */
extern void
spe_compare_equal_uint(struct spe_function *p, int rT, int rA, uint ui);

/** Compare greater with immediate value. */
extern void
spe_compare_greater_uint(struct spe_function *p, int rT, int rA, uint ui);

/** Replicate word 0 of rA across rT. */
extern void
spe_splat(struct spe_function *p, int rT, int rA);

/** rT = complement_all_bits(rA). */
extern void
spe_complement(struct spe_function *p, int rT, int rA);

/** rT = rA. */
extern void
spe_move(struct spe_function *p, int rT, int rA);

/** rT = {0,0,0,0}. */
extern void
spe_zero(struct spe_function *p, int rT);

/** rT = splat(rA, word) */
extern void
spe_splat_word(struct spe_function *p, int rT, int rA, int word);

/** rT = float min(rA, rB) */
extern void
spe_float_min(struct spe_function *p, int rT, int rA, int rB);

/** rT = float max(rA, rB) */
extern void
spe_float_max(struct spe_function *p, int rT, int rA, int rB);


/* Floating-point instructions
 */
EMIT_RR  (spe_fa,         0x2c4)
EMIT_RR  (spe_dfa,        0x2cc)
EMIT_RR  (spe_fs,         0x2c5)
EMIT_RR  (spe_dfs,        0x2cd)
EMIT_RR  (spe_fm,         0x2c6)
EMIT_RR  (spe_dfm,        0x2ce)
EMIT_RRR (spe_fma,        0x00e)
EMIT_RR  (spe_dfma,       0x35c)
EMIT_RRR (spe_fnms,       0x00d)
EMIT_RR  (spe_dfnms,      0x35e)
EMIT_RRR (spe_fms,        0x00f)
EMIT_RR  (spe_dfms,       0x35d)
EMIT_RR  (spe_dfnma,      0x35f)
EMIT_R   (spe_frest,      0x1b8)
EMIT_R   (spe_frsqest,    0x1b9)
EMIT_RR  (spe_fi,         0x3d4)
EMIT_RI8 (spe_csflt,      0x1da, 155)
EMIT_RI8 (spe_cflts,      0x1d8, 173)
EMIT_RI8 (spe_cuflt,      0x1db, 155)
EMIT_RI8 (spe_cfltu,      0x1d9, 173)
EMIT_R   (spe_frds,       0x3b9)
EMIT_R   (spe_fesd,       0x3b8)
EMIT_RR  (spe_dfceq,      0x3c3)
EMIT_RR  (spe_dfcmeq,     0x3cb)
EMIT_RR  (spe_dfcgt,      0x2c3)
EMIT_RR  (spe_dfcmgt,     0x2cb)
EMIT_RI7 (spe_dftsv,      0x3bf)
EMIT_RR  (spe_fceq,       0x3c2)
EMIT_RR  (spe_fcmeq,      0x3ca)
EMIT_RR  (spe_fcgt,       0x2c2)
EMIT_RR  (spe_fcmgt,      0x2ca)
EMIT_R   (spe_fscrwr,     0x3ba)
EMIT_    (spe_fscrrd,     0x398)


/* Channel instructions
 */
EMIT_R   (spe_rdch,       0x00d)
EMIT_R   (spe_rdchcnt,    0x00f)
EMIT_R   (spe_wrch,       0x10d)


#ifdef UNDEF_EMIT_MACROS
#undef EMIT
#undef EMIT_
#undef EMIT_R
#undef EMIT_RR
#undef EMIT_RRR
#undef EMIT_RI7
#undef EMIT_RI8
#undef EMIT_RI10
#undef EMIT_RI10s
#undef EMIT_RI16
#undef EMIT_RI18
#undef EMIT_I16
#undef UNDEF_EMIT_MACROS
#endif /* EMIT_ */
