/*
 * Copyright (C) 2008-2009  Advanced Micro Devices, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * Contacts:
 *   Richard Li <RichardZ.Li@amd.com>, <richardradeon@gmail.com>
 */

#if !defined (_SQ_MICRO_REG_H)
#define _SQ_MICRO_REG_H

#if defined(LITTLEENDIAN_CPU)
#elif defined(BIGENDIAN_CPU)
#else
#error "BIGENDIAN_CPU or LITTLEENDIAN_CPU must be defined"
#endif

/*
 * SQ_ALU_SRC_GPR_BASE value
 */

#define SQ_ALU_SRC_GPR_BASE            0x00000000

/*
 * SQ_ALU_SRC_GPR_SIZE value
 */

#define SQ_ALU_SRC_GPR_SIZE            0x00000080

/*
 * SQ_ALU_SRC_KCACHE0_BASE value
 */

#define SQ_ALU_SRC_KCACHE0_BASE        0x00000080

/*
 * SQ_ALU_SRC_KCACHE0_SIZE value
 */

#define SQ_ALU_SRC_KCACHE0_SIZE        0x00000020

/*
 * SQ_ALU_SRC_KCACHE1_BASE value
 */

#define SQ_ALU_SRC_KCACHE1_BASE        0x000000a0

/*
 * SQ_ALU_SRC_KCACHE1_SIZE value
 */

#define SQ_ALU_SRC_KCACHE1_SIZE        0x00000020

/*
 * SQ_ALU_SRC_CFILE_BASE value
 */

#define SQ_ALU_SRC_CFILE_BASE          0x00000100

/*
 * SQ_ALU_SRC_CFILE_SIZE value
 */

#define SQ_ALU_SRC_CFILE_SIZE          0x00000100

/*
 * SQ_SP_OP_REDUC_BEGIN value
 */

#define SQ_SP_OP_REDUC_BEGIN           0x00000050

/*
 * SQ_SP_OP_REDUC_END value
 */

#define SQ_SP_OP_REDUC_END             0x00000053

/*
 * SQ_SP_OP_TRANS_BEGIN value
 */

#define SQ_SP_OP_TRANS_BEGIN           0x00000060

/*
 * SQ_SP_OP_TRANS_END value
 */

#define SQ_SP_OP_TRANS_END             0x0000007f

/*
 * SQ_CF_WORD0 struct
 */

#define SQ_CF_WORD0_ADDR_SIZE          32

#define SQ_CF_WORD0_ADDR_SHIFT         0

#define SQ_CF_WORD0_ADDR_MASK          0xffffffff

#define SQ_CF_WORD0_MASK \
     (SQ_CF_WORD0_ADDR_MASK)

#define SQ_CF_WORD0_DEFAULT            0xcdcdcdcd

#define SQ_CF_WORD0_GET_ADDR(sq_cf_word0) \
     ((sq_cf_word0 & SQ_CF_WORD0_ADDR_MASK) >> SQ_CF_WORD0_ADDR_SHIFT)

#define SQ_CF_WORD0_SET_ADDR(sq_cf_word0_reg, addr) \
     sq_cf_word0_reg = (sq_cf_word0_reg & ~SQ_CF_WORD0_ADDR_MASK) | (addr << SQ_CF_WORD0_ADDR_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_word0_t {
          unsigned int addr                           : SQ_CF_WORD0_ADDR_SIZE;
     } sq_cf_word0_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_word0_t {
          unsigned int addr                           : SQ_CF_WORD0_ADDR_SIZE;
     } sq_cf_word0_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_word0_t f;
} sq_cf_word0_u;


/*
 * SQ_CF_WORD1 struct
 */

#define SQ_CF_WORD1_POP_COUNT_SIZE     3
#define SQ_CF_WORD1_CF_CONST_SIZE      5
#define SQ_CF_WORD1_COND_SIZE          2
#define SQ_CF_WORD1_COUNT_SIZE         3
#define SQ_CF_WORD1_CALL_COUNT_SIZE    6
#define SQ_CF_WORD1_COUNT_3_SIZE       1
#define SQ_CF_WORD1_END_OF_PROGRAM_SIZE 1
#define SQ_CF_WORD1_VALID_PIXEL_MODE_SIZE 1
#define SQ_CF_WORD1_CF_INST_SIZE       7
#define SQ_CF_WORD1_WHOLE_QUAD_MODE_SIZE 1
#define SQ_CF_WORD1_BARRIER_SIZE       1

#define SQ_CF_WORD1_POP_COUNT_SHIFT    0
#define SQ_CF_WORD1_CF_CONST_SHIFT     3
#define SQ_CF_WORD1_COND_SHIFT         8
#define SQ_CF_WORD1_COUNT_SHIFT        10
#define SQ_CF_WORD1_CALL_COUNT_SHIFT   13
#define SQ_CF_WORD1_COUNT_3_SHIFT      19
#define SQ_CF_WORD1_END_OF_PROGRAM_SHIFT 21
#define SQ_CF_WORD1_VALID_PIXEL_MODE_SHIFT 22
#define SQ_CF_WORD1_CF_INST_SHIFT      23
#define SQ_CF_WORD1_WHOLE_QUAD_MODE_SHIFT 30
#define SQ_CF_WORD1_BARRIER_SHIFT      31

#define SQ_CF_WORD1_POP_COUNT_MASK     0x00000007
#define SQ_CF_WORD1_CF_CONST_MASK      0x000000f8
#define SQ_CF_WORD1_COND_MASK          0x00000300
#define SQ_CF_WORD1_COUNT_MASK         0x00001c00
#define SQ_CF_WORD1_CALL_COUNT_MASK    0x0007e000
#define SQ_CF_WORD1_COUNT_3_MASK       0x00080000
#define SQ_CF_WORD1_END_OF_PROGRAM_MASK 0x00200000
#define SQ_CF_WORD1_VALID_PIXEL_MODE_MASK 0x00400000
#define SQ_CF_WORD1_CF_INST_MASK       0x3f800000
#define SQ_CF_WORD1_WHOLE_QUAD_MODE_MASK 0x40000000
#define SQ_CF_WORD1_BARRIER_MASK       0x80000000

#define SQ_CF_WORD1_MASK \
     (SQ_CF_WORD1_POP_COUNT_MASK | \
      SQ_CF_WORD1_CF_CONST_MASK | \
      SQ_CF_WORD1_COND_MASK | \
      SQ_CF_WORD1_COUNT_MASK | \
      SQ_CF_WORD1_CALL_COUNT_MASK | \
      SQ_CF_WORD1_COUNT_3_MASK | \
      SQ_CF_WORD1_END_OF_PROGRAM_MASK | \
      SQ_CF_WORD1_VALID_PIXEL_MODE_MASK | \
      SQ_CF_WORD1_CF_INST_MASK | \
      SQ_CF_WORD1_WHOLE_QUAD_MODE_MASK | \
      SQ_CF_WORD1_BARRIER_MASK)

#define SQ_CF_WORD1_DEFAULT            0xcdcdcdcd

#define SQ_CF_WORD1_GET_POP_COUNT(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_POP_COUNT_MASK) >> SQ_CF_WORD1_POP_COUNT_SHIFT)
#define SQ_CF_WORD1_GET_CF_CONST(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_CF_CONST_MASK) >> SQ_CF_WORD1_CF_CONST_SHIFT)
#define SQ_CF_WORD1_GET_COND(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_COND_MASK) >> SQ_CF_WORD1_COND_SHIFT)
#define SQ_CF_WORD1_GET_COUNT(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_COUNT_MASK) >> SQ_CF_WORD1_COUNT_SHIFT)
#define SQ_CF_WORD1_GET_CALL_COUNT(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_CALL_COUNT_MASK) >> SQ_CF_WORD1_CALL_COUNT_SHIFT)
#define SQ_CF_WORD1_GET_COUNT_3(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_COUNT_3_MASK) >> SQ_CF_WORD1_COUNT_3_SHIFT)
#define SQ_CF_WORD1_GET_END_OF_PROGRAM(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_END_OF_PROGRAM_MASK) >> SQ_CF_WORD1_END_OF_PROGRAM_SHIFT)
#define SQ_CF_WORD1_GET_VALID_PIXEL_MODE(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_VALID_PIXEL_MODE_MASK) >> SQ_CF_WORD1_VALID_PIXEL_MODE_SHIFT)
#define SQ_CF_WORD1_GET_CF_INST(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_CF_INST_MASK) >> SQ_CF_WORD1_CF_INST_SHIFT)
#define SQ_CF_WORD1_GET_WHOLE_QUAD_MODE(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_WHOLE_QUAD_MODE_MASK) >> SQ_CF_WORD1_WHOLE_QUAD_MODE_SHIFT)
#define SQ_CF_WORD1_GET_BARRIER(sq_cf_word1) \
     ((sq_cf_word1 & SQ_CF_WORD1_BARRIER_MASK) >> SQ_CF_WORD1_BARRIER_SHIFT)

#define SQ_CF_WORD1_SET_POP_COUNT(sq_cf_word1_reg, pop_count) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_POP_COUNT_MASK) | (pop_count << SQ_CF_WORD1_POP_COUNT_SHIFT)
#define SQ_CF_WORD1_SET_CF_CONST(sq_cf_word1_reg, cf_const) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_CF_CONST_MASK) | (cf_const << SQ_CF_WORD1_CF_CONST_SHIFT)
#define SQ_CF_WORD1_SET_COND(sq_cf_word1_reg, cond) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_COND_MASK) | (cond << SQ_CF_WORD1_COND_SHIFT)
#define SQ_CF_WORD1_SET_COUNT(sq_cf_word1_reg, count) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_COUNT_MASK) | (count << SQ_CF_WORD1_COUNT_SHIFT)
#define SQ_CF_WORD1_SET_CALL_COUNT(sq_cf_word1_reg, call_count) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_CALL_COUNT_MASK) | (call_count << SQ_CF_WORD1_CALL_COUNT_SHIFT)
#define SQ_CF_WORD1_SET_COUNT_3(sq_cf_word1_reg, count_3) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_COUNT_3_MASK) | (count_3 << SQ_CF_WORD1_COUNT_3_SHIFT)
#define SQ_CF_WORD1_SET_END_OF_PROGRAM(sq_cf_word1_reg, end_of_program) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_END_OF_PROGRAM_MASK) | (end_of_program << SQ_CF_WORD1_END_OF_PROGRAM_SHIFT)
#define SQ_CF_WORD1_SET_VALID_PIXEL_MODE(sq_cf_word1_reg, valid_pixel_mode) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_VALID_PIXEL_MODE_MASK) | (valid_pixel_mode << SQ_CF_WORD1_VALID_PIXEL_MODE_SHIFT)
#define SQ_CF_WORD1_SET_CF_INST(sq_cf_word1_reg, cf_inst) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_CF_INST_MASK) | (cf_inst << SQ_CF_WORD1_CF_INST_SHIFT)
#define SQ_CF_WORD1_SET_WHOLE_QUAD_MODE(sq_cf_word1_reg, whole_quad_mode) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_WHOLE_QUAD_MODE_MASK) | (whole_quad_mode << SQ_CF_WORD1_WHOLE_QUAD_MODE_SHIFT)
#define SQ_CF_WORD1_SET_BARRIER(sq_cf_word1_reg, barrier) \
     sq_cf_word1_reg = (sq_cf_word1_reg & ~SQ_CF_WORD1_BARRIER_MASK) | (barrier << SQ_CF_WORD1_BARRIER_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_word1_t {
          unsigned int pop_count                      : SQ_CF_WORD1_POP_COUNT_SIZE;
          unsigned int cf_const                       : SQ_CF_WORD1_CF_CONST_SIZE;
          unsigned int cond                           : SQ_CF_WORD1_COND_SIZE;
          unsigned int count                          : SQ_CF_WORD1_COUNT_SIZE;
          unsigned int call_count                     : SQ_CF_WORD1_CALL_COUNT_SIZE;
          unsigned int count_3                        : SQ_CF_WORD1_COUNT_3_SIZE;
          unsigned int                                : 1;
          unsigned int end_of_program                 : SQ_CF_WORD1_END_OF_PROGRAM_SIZE;
          unsigned int valid_pixel_mode               : SQ_CF_WORD1_VALID_PIXEL_MODE_SIZE;
          unsigned int cf_inst                        : SQ_CF_WORD1_CF_INST_SIZE;
          unsigned int whole_quad_mode                : SQ_CF_WORD1_WHOLE_QUAD_MODE_SIZE;
          unsigned int barrier                        : SQ_CF_WORD1_BARRIER_SIZE;
     } sq_cf_word1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_word1_t {
          unsigned int barrier                        : SQ_CF_WORD1_BARRIER_SIZE;
          unsigned int whole_quad_mode                : SQ_CF_WORD1_WHOLE_QUAD_MODE_SIZE;
          unsigned int cf_inst                        : SQ_CF_WORD1_CF_INST_SIZE;
          unsigned int valid_pixel_mode               : SQ_CF_WORD1_VALID_PIXEL_MODE_SIZE;
          unsigned int end_of_program                 : SQ_CF_WORD1_END_OF_PROGRAM_SIZE;
          unsigned int                                : 1;
          unsigned int count_3                        : SQ_CF_WORD1_COUNT_3_SIZE;
          unsigned int call_count                     : SQ_CF_WORD1_CALL_COUNT_SIZE;
          unsigned int count                          : SQ_CF_WORD1_COUNT_SIZE;
          unsigned int cond                           : SQ_CF_WORD1_COND_SIZE;
          unsigned int cf_const                       : SQ_CF_WORD1_CF_CONST_SIZE;
          unsigned int pop_count                      : SQ_CF_WORD1_POP_COUNT_SIZE;
     } sq_cf_word1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_word1_t f;
} sq_cf_word1_u;


/*
 * SQ_CF_ALU_WORD0 struct
 */

#define SQ_CF_ALU_WORD0_ADDR_SIZE      22
#define SQ_CF_ALU_WORD0_KCACHE_BANK0_SIZE 4
#define SQ_CF_ALU_WORD0_KCACHE_BANK1_SIZE 4
#define SQ_CF_ALU_WORD0_KCACHE_MODE0_SIZE 2

#define SQ_CF_ALU_WORD0_ADDR_SHIFT     0
#define SQ_CF_ALU_WORD0_KCACHE_BANK0_SHIFT 22
#define SQ_CF_ALU_WORD0_KCACHE_BANK1_SHIFT 26
#define SQ_CF_ALU_WORD0_KCACHE_MODE0_SHIFT 30

#define SQ_CF_ALU_WORD0_ADDR_MASK      0x003fffff
#define SQ_CF_ALU_WORD0_KCACHE_BANK0_MASK 0x03c00000
#define SQ_CF_ALU_WORD0_KCACHE_BANK1_MASK 0x3c000000
#define SQ_CF_ALU_WORD0_KCACHE_MODE0_MASK 0xc0000000

#define SQ_CF_ALU_WORD0_MASK \
     (SQ_CF_ALU_WORD0_ADDR_MASK | \
      SQ_CF_ALU_WORD0_KCACHE_BANK0_MASK | \
      SQ_CF_ALU_WORD0_KCACHE_BANK1_MASK | \
      SQ_CF_ALU_WORD0_KCACHE_MODE0_MASK)

#define SQ_CF_ALU_WORD0_DEFAULT        0xcdcdcdcd

#define SQ_CF_ALU_WORD0_GET_ADDR(sq_cf_alu_word0) \
     ((sq_cf_alu_word0 & SQ_CF_ALU_WORD0_ADDR_MASK) >> SQ_CF_ALU_WORD0_ADDR_SHIFT)
#define SQ_CF_ALU_WORD0_GET_KCACHE_BANK0(sq_cf_alu_word0) \
     ((sq_cf_alu_word0 & SQ_CF_ALU_WORD0_KCACHE_BANK0_MASK) >> SQ_CF_ALU_WORD0_KCACHE_BANK0_SHIFT)
#define SQ_CF_ALU_WORD0_GET_KCACHE_BANK1(sq_cf_alu_word0) \
     ((sq_cf_alu_word0 & SQ_CF_ALU_WORD0_KCACHE_BANK1_MASK) >> SQ_CF_ALU_WORD0_KCACHE_BANK1_SHIFT)
#define SQ_CF_ALU_WORD0_GET_KCACHE_MODE0(sq_cf_alu_word0) \
     ((sq_cf_alu_word0 & SQ_CF_ALU_WORD0_KCACHE_MODE0_MASK) >> SQ_CF_ALU_WORD0_KCACHE_MODE0_SHIFT)

#define SQ_CF_ALU_WORD0_SET_ADDR(sq_cf_alu_word0_reg, addr) \
     sq_cf_alu_word0_reg = (sq_cf_alu_word0_reg & ~SQ_CF_ALU_WORD0_ADDR_MASK) | (addr << SQ_CF_ALU_WORD0_ADDR_SHIFT)
#define SQ_CF_ALU_WORD0_SET_KCACHE_BANK0(sq_cf_alu_word0_reg, kcache_bank0) \
     sq_cf_alu_word0_reg = (sq_cf_alu_word0_reg & ~SQ_CF_ALU_WORD0_KCACHE_BANK0_MASK) | (kcache_bank0 << SQ_CF_ALU_WORD0_KCACHE_BANK0_SHIFT)
#define SQ_CF_ALU_WORD0_SET_KCACHE_BANK1(sq_cf_alu_word0_reg, kcache_bank1) \
     sq_cf_alu_word0_reg = (sq_cf_alu_word0_reg & ~SQ_CF_ALU_WORD0_KCACHE_BANK1_MASK) | (kcache_bank1 << SQ_CF_ALU_WORD0_KCACHE_BANK1_SHIFT)
#define SQ_CF_ALU_WORD0_SET_KCACHE_MODE0(sq_cf_alu_word0_reg, kcache_mode0) \
     sq_cf_alu_word0_reg = (sq_cf_alu_word0_reg & ~SQ_CF_ALU_WORD0_KCACHE_MODE0_MASK) | (kcache_mode0 << SQ_CF_ALU_WORD0_KCACHE_MODE0_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_alu_word0_t {
          unsigned int addr                           : SQ_CF_ALU_WORD0_ADDR_SIZE;
          unsigned int kcache_bank0                   : SQ_CF_ALU_WORD0_KCACHE_BANK0_SIZE;
          unsigned int kcache_bank1                   : SQ_CF_ALU_WORD0_KCACHE_BANK1_SIZE;
          unsigned int kcache_mode0                   : SQ_CF_ALU_WORD0_KCACHE_MODE0_SIZE;
     } sq_cf_alu_word0_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_alu_word0_t {
          unsigned int kcache_mode0                   : SQ_CF_ALU_WORD0_KCACHE_MODE0_SIZE;
          unsigned int kcache_bank1                   : SQ_CF_ALU_WORD0_KCACHE_BANK1_SIZE;
          unsigned int kcache_bank0                   : SQ_CF_ALU_WORD0_KCACHE_BANK0_SIZE;
          unsigned int addr                           : SQ_CF_ALU_WORD0_ADDR_SIZE;
     } sq_cf_alu_word0_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_alu_word0_t f;
} sq_cf_alu_word0_u;


/*
 * SQ_CF_ALU_WORD1 struct
 */

#define SQ_CF_ALU_WORD1_KCACHE_MODE1_SIZE 2
#define SQ_CF_ALU_WORD1_KCACHE_ADDR0_SIZE 8
#define SQ_CF_ALU_WORD1_KCACHE_ADDR1_SIZE 8
#define SQ_CF_ALU_WORD1_COUNT_SIZE     7
#define SQ_CF_ALU_WORD1_ALT_CONST_SIZE 1
#define SQ_CF_ALU_WORD1_CF_INST_SIZE   4
#define SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_SIZE 1
#define SQ_CF_ALU_WORD1_BARRIER_SIZE   1

#define SQ_CF_ALU_WORD1_KCACHE_MODE1_SHIFT 0
#define SQ_CF_ALU_WORD1_KCACHE_ADDR0_SHIFT 2
#define SQ_CF_ALU_WORD1_KCACHE_ADDR1_SHIFT 10
#define SQ_CF_ALU_WORD1_COUNT_SHIFT    18
#define SQ_CF_ALU_WORD1_ALT_CONST_SHIFT 25
#define SQ_CF_ALU_WORD1_CF_INST_SHIFT  26
#define SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_SHIFT 30
#define SQ_CF_ALU_WORD1_BARRIER_SHIFT  31

#define SQ_CF_ALU_WORD1_KCACHE_MODE1_MASK 0x00000003
#define SQ_CF_ALU_WORD1_KCACHE_ADDR0_MASK 0x000003fc
#define SQ_CF_ALU_WORD1_KCACHE_ADDR1_MASK 0x0003fc00
#define SQ_CF_ALU_WORD1_COUNT_MASK     0x01fc0000
#define SQ_CF_ALU_WORD1_ALT_CONST_MASK 0x02000000
#define SQ_CF_ALU_WORD1_CF_INST_MASK   0x3c000000
#define SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_MASK 0x40000000
#define SQ_CF_ALU_WORD1_BARRIER_MASK   0x80000000

#define SQ_CF_ALU_WORD1_MASK \
     (SQ_CF_ALU_WORD1_KCACHE_MODE1_MASK | \
      SQ_CF_ALU_WORD1_KCACHE_ADDR0_MASK | \
      SQ_CF_ALU_WORD1_KCACHE_ADDR1_MASK | \
      SQ_CF_ALU_WORD1_COUNT_MASK | \
      SQ_CF_ALU_WORD1_ALT_CONST_MASK | \
      SQ_CF_ALU_WORD1_CF_INST_MASK | \
      SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_MASK | \
      SQ_CF_ALU_WORD1_BARRIER_MASK)

#define SQ_CF_ALU_WORD1_DEFAULT        0xcdcdcdcd

#define SQ_CF_ALU_WORD1_GET_KCACHE_MODE1(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_KCACHE_MODE1_MASK) >> SQ_CF_ALU_WORD1_KCACHE_MODE1_SHIFT)
#define SQ_CF_ALU_WORD1_GET_KCACHE_ADDR0(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_KCACHE_ADDR0_MASK) >> SQ_CF_ALU_WORD1_KCACHE_ADDR0_SHIFT)
#define SQ_CF_ALU_WORD1_GET_KCACHE_ADDR1(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_KCACHE_ADDR1_MASK) >> SQ_CF_ALU_WORD1_KCACHE_ADDR1_SHIFT)
#define SQ_CF_ALU_WORD1_GET_COUNT(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_COUNT_MASK) >> SQ_CF_ALU_WORD1_COUNT_SHIFT)
#define SQ_CF_ALU_WORD1_GET_ALT_CONST(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_ALT_CONST_MASK) >> SQ_CF_ALU_WORD1_ALT_CONST_SHIFT)
#define SQ_CF_ALU_WORD1_GET_CF_INST(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_CF_INST_MASK) >> SQ_CF_ALU_WORD1_CF_INST_SHIFT)
#define SQ_CF_ALU_WORD1_GET_WHOLE_QUAD_MODE(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_MASK) >> SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_SHIFT)
#define SQ_CF_ALU_WORD1_GET_BARRIER(sq_cf_alu_word1) \
     ((sq_cf_alu_word1 & SQ_CF_ALU_WORD1_BARRIER_MASK) >> SQ_CF_ALU_WORD1_BARRIER_SHIFT)

#define SQ_CF_ALU_WORD1_SET_KCACHE_MODE1(sq_cf_alu_word1_reg, kcache_mode1) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_KCACHE_MODE1_MASK) | (kcache_mode1 << SQ_CF_ALU_WORD1_KCACHE_MODE1_SHIFT)
#define SQ_CF_ALU_WORD1_SET_KCACHE_ADDR0(sq_cf_alu_word1_reg, kcache_addr0) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_KCACHE_ADDR0_MASK) | (kcache_addr0 << SQ_CF_ALU_WORD1_KCACHE_ADDR0_SHIFT)
#define SQ_CF_ALU_WORD1_SET_KCACHE_ADDR1(sq_cf_alu_word1_reg, kcache_addr1) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_KCACHE_ADDR1_MASK) | (kcache_addr1 << SQ_CF_ALU_WORD1_KCACHE_ADDR1_SHIFT)
#define SQ_CF_ALU_WORD1_SET_COUNT(sq_cf_alu_word1_reg, count) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_COUNT_MASK) | (count << SQ_CF_ALU_WORD1_COUNT_SHIFT)
#define SQ_CF_ALU_WORD1_SET_ALT_CONST(sq_cf_alu_word1_reg, alt_const) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_ALT_CONST_MASK) | (alt_const << SQ_CF_ALU_WORD1_ALT_CONST_SHIFT)
#define SQ_CF_ALU_WORD1_SET_CF_INST(sq_cf_alu_word1_reg, cf_inst) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_CF_INST_MASK) | (cf_inst << SQ_CF_ALU_WORD1_CF_INST_SHIFT)
#define SQ_CF_ALU_WORD1_SET_WHOLE_QUAD_MODE(sq_cf_alu_word1_reg, whole_quad_mode) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_MASK) | (whole_quad_mode << SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_SHIFT)
#define SQ_CF_ALU_WORD1_SET_BARRIER(sq_cf_alu_word1_reg, barrier) \
     sq_cf_alu_word1_reg = (sq_cf_alu_word1_reg & ~SQ_CF_ALU_WORD1_BARRIER_MASK) | (barrier << SQ_CF_ALU_WORD1_BARRIER_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_alu_word1_t {
          unsigned int kcache_mode1                   : SQ_CF_ALU_WORD1_KCACHE_MODE1_SIZE;
          unsigned int kcache_addr0                   : SQ_CF_ALU_WORD1_KCACHE_ADDR0_SIZE;
          unsigned int kcache_addr1                   : SQ_CF_ALU_WORD1_KCACHE_ADDR1_SIZE;
          unsigned int count                          : SQ_CF_ALU_WORD1_COUNT_SIZE;
          unsigned int alt_const                      : SQ_CF_ALU_WORD1_ALT_CONST_SIZE;
          unsigned int cf_inst                        : SQ_CF_ALU_WORD1_CF_INST_SIZE;
          unsigned int whole_quad_mode                : SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_SIZE;
          unsigned int barrier                        : SQ_CF_ALU_WORD1_BARRIER_SIZE;
     } sq_cf_alu_word1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_alu_word1_t {
          unsigned int barrier                        : SQ_CF_ALU_WORD1_BARRIER_SIZE;
          unsigned int whole_quad_mode                : SQ_CF_ALU_WORD1_WHOLE_QUAD_MODE_SIZE;
          unsigned int cf_inst                        : SQ_CF_ALU_WORD1_CF_INST_SIZE;
          unsigned int alt_const                      : SQ_CF_ALU_WORD1_ALT_CONST_SIZE;
          unsigned int count                          : SQ_CF_ALU_WORD1_COUNT_SIZE;
          unsigned int kcache_addr1                   : SQ_CF_ALU_WORD1_KCACHE_ADDR1_SIZE;
          unsigned int kcache_addr0                   : SQ_CF_ALU_WORD1_KCACHE_ADDR0_SIZE;
          unsigned int kcache_mode1                   : SQ_CF_ALU_WORD1_KCACHE_MODE1_SIZE;
     } sq_cf_alu_word1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_alu_word1_t f;
} sq_cf_alu_word1_u;


/*
 * SQ_CF_ALLOC_EXPORT_WORD0 struct
 */

#define SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_SIZE 13
#define SQ_CF_ALLOC_EXPORT_WORD0_TYPE_SIZE 2
#define SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_SIZE 7
#define SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_SIZE 1
#define SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_SIZE 7
#define SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_SIZE 2

#define SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_SHIFT 0
#define SQ_CF_ALLOC_EXPORT_WORD0_TYPE_SHIFT 13
#define SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_SHIFT 15
#define SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_SHIFT 22
#define SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_SHIFT 23
#define SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_SHIFT 30

#define SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_MASK 0x00001fff
#define SQ_CF_ALLOC_EXPORT_WORD0_TYPE_MASK 0x00006000
#define SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_MASK 0x003f8000
#define SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_MASK 0x00400000
#define SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_MASK 0x3f800000
#define SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_MASK 0xc0000000

#define SQ_CF_ALLOC_EXPORT_WORD0_MASK \
     (SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD0_TYPE_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_MASK)

#define SQ_CF_ALLOC_EXPORT_WORD0_DEFAULT 0xcdcdcdcd

#define SQ_CF_ALLOC_EXPORT_WORD0_GET_ARRAY_BASE(sq_cf_alloc_export_word0) \
     ((sq_cf_alloc_export_word0 & SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_MASK) >> SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_GET_TYPE(sq_cf_alloc_export_word0) \
     ((sq_cf_alloc_export_word0 & SQ_CF_ALLOC_EXPORT_WORD0_TYPE_MASK) >> SQ_CF_ALLOC_EXPORT_WORD0_TYPE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_GET_RW_GPR(sq_cf_alloc_export_word0) \
     ((sq_cf_alloc_export_word0 & SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_MASK) >> SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_GET_RW_REL(sq_cf_alloc_export_word0) \
     ((sq_cf_alloc_export_word0 & SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_MASK) >> SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_GET_INDEX_GPR(sq_cf_alloc_export_word0) \
     ((sq_cf_alloc_export_word0 & SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_MASK) >> SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_GET_ELEM_SIZE(sq_cf_alloc_export_word0) \
     ((sq_cf_alloc_export_word0 & SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_MASK) >> SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_SHIFT)

#define SQ_CF_ALLOC_EXPORT_WORD0_SET_ARRAY_BASE(sq_cf_alloc_export_word0_reg, array_base) \
     sq_cf_alloc_export_word0_reg = (sq_cf_alloc_export_word0_reg & ~SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_MASK) | (array_base << SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_SET_TYPE(sq_cf_alloc_export_word0_reg, type) \
     sq_cf_alloc_export_word0_reg = (sq_cf_alloc_export_word0_reg & ~SQ_CF_ALLOC_EXPORT_WORD0_TYPE_MASK) | (type << SQ_CF_ALLOC_EXPORT_WORD0_TYPE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_SET_RW_GPR(sq_cf_alloc_export_word0_reg, rw_gpr) \
     sq_cf_alloc_export_word0_reg = (sq_cf_alloc_export_word0_reg & ~SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_MASK) | (rw_gpr << SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_SET_RW_REL(sq_cf_alloc_export_word0_reg, rw_rel) \
     sq_cf_alloc_export_word0_reg = (sq_cf_alloc_export_word0_reg & ~SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_MASK) | (rw_rel << SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_SET_INDEX_GPR(sq_cf_alloc_export_word0_reg, index_gpr) \
     sq_cf_alloc_export_word0_reg = (sq_cf_alloc_export_word0_reg & ~SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_MASK) | (index_gpr << SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD0_SET_ELEM_SIZE(sq_cf_alloc_export_word0_reg, elem_size) \
     sq_cf_alloc_export_word0_reg = (sq_cf_alloc_export_word0_reg & ~SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_MASK) | (elem_size << SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word0_t {
          unsigned int array_base                     : SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_SIZE;
          unsigned int type                           : SQ_CF_ALLOC_EXPORT_WORD0_TYPE_SIZE;
          unsigned int rw_gpr                         : SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_SIZE;
          unsigned int rw_rel                         : SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_SIZE;
          unsigned int index_gpr                      : SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_SIZE;
          unsigned int elem_size                      : SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_SIZE;
     } sq_cf_alloc_export_word0_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word0_t {
          unsigned int elem_size                      : SQ_CF_ALLOC_EXPORT_WORD0_ELEM_SIZE_SIZE;
          unsigned int index_gpr                      : SQ_CF_ALLOC_EXPORT_WORD0_INDEX_GPR_SIZE;
          unsigned int rw_rel                         : SQ_CF_ALLOC_EXPORT_WORD0_RW_REL_SIZE;
          unsigned int rw_gpr                         : SQ_CF_ALLOC_EXPORT_WORD0_RW_GPR_SIZE;
          unsigned int type                           : SQ_CF_ALLOC_EXPORT_WORD0_TYPE_SIZE;
          unsigned int array_base                     : SQ_CF_ALLOC_EXPORT_WORD0_ARRAY_BASE_SIZE;
     } sq_cf_alloc_export_word0_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_alloc_export_word0_t f;
} sq_cf_alloc_export_word0_u;


/*
 * SQ_CF_ALLOC_EXPORT_WORD1 struct
 */

#define SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_SIZE 4
#define SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_SIZE 1
#define SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_SIZE 1
#define SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_SIZE 7
#define SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_SIZE 1
#define SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_SIZE 1

#define SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_SHIFT 17
#define SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_SHIFT 21
#define SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_SHIFT 22
#define SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_SHIFT 23
#define SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_SHIFT 30
#define SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_SHIFT 31

#define SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_MASK 0x001e0000
#define SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_MASK 0x00200000
#define SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_MASK 0x00400000
#define SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_MASK 0x3f800000
#define SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_MASK 0x40000000
#define SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_MASK 0x80000000

#define SQ_CF_ALLOC_EXPORT_WORD1_MASK \
     (SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_MASK)

#define SQ_CF_ALLOC_EXPORT_WORD1_DEFAULT 0xcdcc0000

#define SQ_CF_ALLOC_EXPORT_WORD1_GET_BURST_COUNT(sq_cf_alloc_export_word1) \
     ((sq_cf_alloc_export_word1 & SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_GET_END_OF_PROGRAM(sq_cf_alloc_export_word1) \
     ((sq_cf_alloc_export_word1 & SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_GET_VALID_PIXEL_MODE(sq_cf_alloc_export_word1) \
     ((sq_cf_alloc_export_word1 & SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_GET_CF_INST(sq_cf_alloc_export_word1) \
     ((sq_cf_alloc_export_word1 & SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_GET_WHOLE_QUAD_MODE(sq_cf_alloc_export_word1) \
     ((sq_cf_alloc_export_word1 & SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_GET_BARRIER(sq_cf_alloc_export_word1) \
     ((sq_cf_alloc_export_word1 & SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_SHIFT)

#define SQ_CF_ALLOC_EXPORT_WORD1_SET_BURST_COUNT(sq_cf_alloc_export_word1_reg, burst_count) \
     sq_cf_alloc_export_word1_reg = (sq_cf_alloc_export_word1_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_MASK) | (burst_count << SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SET_END_OF_PROGRAM(sq_cf_alloc_export_word1_reg, end_of_program) \
     sq_cf_alloc_export_word1_reg = (sq_cf_alloc_export_word1_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_MASK) | (end_of_program << SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SET_VALID_PIXEL_MODE(sq_cf_alloc_export_word1_reg, valid_pixel_mode) \
     sq_cf_alloc_export_word1_reg = (sq_cf_alloc_export_word1_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_MASK) | (valid_pixel_mode << SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SET_CF_INST(sq_cf_alloc_export_word1_reg, cf_inst) \
     sq_cf_alloc_export_word1_reg = (sq_cf_alloc_export_word1_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_MASK) | (cf_inst << SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SET_WHOLE_QUAD_MODE(sq_cf_alloc_export_word1_reg, whole_quad_mode) \
     sq_cf_alloc_export_word1_reg = (sq_cf_alloc_export_word1_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_MASK) | (whole_quad_mode << SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SET_BARRIER(sq_cf_alloc_export_word1_reg, barrier) \
     sq_cf_alloc_export_word1_reg = (sq_cf_alloc_export_word1_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_MASK) | (barrier << SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word1_t {
          unsigned int                                : 17;
          unsigned int burst_count                    : SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_SIZE;
          unsigned int end_of_program                 : SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_SIZE;
          unsigned int valid_pixel_mode               : SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_SIZE;
          unsigned int cf_inst                        : SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_SIZE;
          unsigned int whole_quad_mode                : SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_SIZE;
          unsigned int barrier                        : SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_SIZE;
     } sq_cf_alloc_export_word1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word1_t {
          unsigned int barrier                        : SQ_CF_ALLOC_EXPORT_WORD1_BARRIER_SIZE;
          unsigned int whole_quad_mode                : SQ_CF_ALLOC_EXPORT_WORD1_WHOLE_QUAD_MODE_SIZE;
          unsigned int cf_inst                        : SQ_CF_ALLOC_EXPORT_WORD1_CF_INST_SIZE;
          unsigned int valid_pixel_mode               : SQ_CF_ALLOC_EXPORT_WORD1_VALID_PIXEL_MODE_SIZE;
          unsigned int end_of_program                 : SQ_CF_ALLOC_EXPORT_WORD1_END_OF_PROGRAM_SIZE;
          unsigned int burst_count                    : SQ_CF_ALLOC_EXPORT_WORD1_BURST_COUNT_SIZE;
          unsigned int                                : 17;
     } sq_cf_alloc_export_word1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_alloc_export_word1_t f;
} sq_cf_alloc_export_word1_u;


/*
 * SQ_CF_ALLOC_EXPORT_WORD1_BUF struct
 */

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_SIZE 12
#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_SIZE 4

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_SHIFT 0
#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_SHIFT 12

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_MASK 0x00000fff
#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_MASK 0x0000f000

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_MASK \
     (SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_MASK)

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_DEFAULT 0x0000cdcd

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_GET_ARRAY_SIZE(sq_cf_alloc_export_word1_buf) \
     ((sq_cf_alloc_export_word1_buf & SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_GET_COMP_MASK(sq_cf_alloc_export_word1_buf) \
     ((sq_cf_alloc_export_word1_buf & SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_SHIFT)

#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_SET_ARRAY_SIZE(sq_cf_alloc_export_word1_buf_reg, array_size) \
     sq_cf_alloc_export_word1_buf_reg = (sq_cf_alloc_export_word1_buf_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_MASK) | (array_size << SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_BUF_SET_COMP_MASK(sq_cf_alloc_export_word1_buf_reg, comp_mask) \
     sq_cf_alloc_export_word1_buf_reg = (sq_cf_alloc_export_word1_buf_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_MASK) | (comp_mask << SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word1_buf_t {
          unsigned int array_size                     : SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_SIZE;
          unsigned int comp_mask                      : SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_SIZE;
          unsigned int                                : 16;
     } sq_cf_alloc_export_word1_buf_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word1_buf_t {
          unsigned int                                : 16;
          unsigned int comp_mask                      : SQ_CF_ALLOC_EXPORT_WORD1_BUF_COMP_MASK_SIZE;
          unsigned int array_size                     : SQ_CF_ALLOC_EXPORT_WORD1_BUF_ARRAY_SIZE_SIZE;
     } sq_cf_alloc_export_word1_buf_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_alloc_export_word1_buf_t f;
} sq_cf_alloc_export_word1_buf_u;


/*
 * SQ_CF_ALLOC_EXPORT_WORD1_SWIZ struct
 */

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_SIZE 3
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_SIZE 3
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_SIZE 3
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_SIZE 3

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_SHIFT 0
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_SHIFT 3
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_SHIFT 6
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_SHIFT 9

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_MASK 0x00000007
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_MASK 0x00000038
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_MASK 0x000001c0
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_MASK 0x00000e00

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_MASK \
     (SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_MASK | \
      SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_MASK)

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_DEFAULT 0x00000dcd

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_GET_SEL_X(sq_cf_alloc_export_word1_swiz) \
     ((sq_cf_alloc_export_word1_swiz & SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_GET_SEL_Y(sq_cf_alloc_export_word1_swiz) \
     ((sq_cf_alloc_export_word1_swiz & SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_GET_SEL_Z(sq_cf_alloc_export_word1_swiz) \
     ((sq_cf_alloc_export_word1_swiz & SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_GET_SEL_W(sq_cf_alloc_export_word1_swiz) \
     ((sq_cf_alloc_export_word1_swiz & SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_MASK) >> SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_SHIFT)

#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SET_SEL_X(sq_cf_alloc_export_word1_swiz_reg, sel_x) \
     sq_cf_alloc_export_word1_swiz_reg = (sq_cf_alloc_export_word1_swiz_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_MASK) | (sel_x << SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SET_SEL_Y(sq_cf_alloc_export_word1_swiz_reg, sel_y) \
     sq_cf_alloc_export_word1_swiz_reg = (sq_cf_alloc_export_word1_swiz_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_MASK) | (sel_y << SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SET_SEL_Z(sq_cf_alloc_export_word1_swiz_reg, sel_z) \
     sq_cf_alloc_export_word1_swiz_reg = (sq_cf_alloc_export_word1_swiz_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_MASK) | (sel_z << SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_SHIFT)
#define SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SET_SEL_W(sq_cf_alloc_export_word1_swiz_reg, sel_w) \
     sq_cf_alloc_export_word1_swiz_reg = (sq_cf_alloc_export_word1_swiz_reg & ~SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_MASK) | (sel_w << SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word1_swiz_t {
          unsigned int sel_x                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_SIZE;
          unsigned int sel_y                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_SIZE;
          unsigned int sel_z                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_SIZE;
          unsigned int sel_w                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_SIZE;
          unsigned int                                : 20;
     } sq_cf_alloc_export_word1_swiz_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_cf_alloc_export_word1_swiz_t {
          unsigned int                                : 20;
          unsigned int sel_w                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_W_SIZE;
          unsigned int sel_z                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Z_SIZE;
          unsigned int sel_y                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_Y_SIZE;
          unsigned int sel_x                          : SQ_CF_ALLOC_EXPORT_WORD1_SWIZ_SEL_X_SIZE;
     } sq_cf_alloc_export_word1_swiz_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_cf_alloc_export_word1_swiz_t f;
} sq_cf_alloc_export_word1_swiz_u;


/*
 * SQ_ALU_WORD0 struct
 */

#define SQ_ALU_WORD0_SRC0_SEL_SIZE     9
#define SQ_ALU_WORD0_SRC0_REL_SIZE     1
#define SQ_ALU_WORD0_SRC0_CHAN_SIZE    2
#define SQ_ALU_WORD0_SRC0_NEG_SIZE     1
#define SQ_ALU_WORD0_SRC1_SEL_SIZE     9
#define SQ_ALU_WORD0_SRC1_REL_SIZE     1
#define SQ_ALU_WORD0_SRC1_CHAN_SIZE    2
#define SQ_ALU_WORD0_SRC1_NEG_SIZE     1
#define SQ_ALU_WORD0_INDEX_MODE_SIZE   3
#define SQ_ALU_WORD0_PRED_SEL_SIZE     2
#define SQ_ALU_WORD0_LAST_SIZE         1

#define SQ_ALU_WORD0_SRC0_SEL_SHIFT    0
#define SQ_ALU_WORD0_SRC0_REL_SHIFT    9
#define SQ_ALU_WORD0_SRC0_CHAN_SHIFT   10
#define SQ_ALU_WORD0_SRC0_NEG_SHIFT    12
#define SQ_ALU_WORD0_SRC1_SEL_SHIFT    13
#define SQ_ALU_WORD0_SRC1_REL_SHIFT    22
#define SQ_ALU_WORD0_SRC1_CHAN_SHIFT   23
#define SQ_ALU_WORD0_SRC1_NEG_SHIFT    25
#define SQ_ALU_WORD0_INDEX_MODE_SHIFT  26
#define SQ_ALU_WORD0_PRED_SEL_SHIFT    29
#define SQ_ALU_WORD0_LAST_SHIFT        31

#define SQ_ALU_WORD0_SRC0_SEL_MASK     0x000001ff
#define SQ_ALU_WORD0_SRC0_REL_MASK     0x00000200
#define SQ_ALU_WORD0_SRC0_CHAN_MASK    0x00000c00
#define SQ_ALU_WORD0_SRC0_NEG_MASK     0x00001000
#define SQ_ALU_WORD0_SRC1_SEL_MASK     0x003fe000
#define SQ_ALU_WORD0_SRC1_REL_MASK     0x00400000
#define SQ_ALU_WORD0_SRC1_CHAN_MASK    0x01800000
#define SQ_ALU_WORD0_SRC1_NEG_MASK     0x02000000
#define SQ_ALU_WORD0_INDEX_MODE_MASK   0x1c000000
#define SQ_ALU_WORD0_PRED_SEL_MASK     0x60000000
#define SQ_ALU_WORD0_LAST_MASK         0x80000000

#define SQ_ALU_WORD0_MASK \
     (SQ_ALU_WORD0_SRC0_SEL_MASK | \
      SQ_ALU_WORD0_SRC0_REL_MASK | \
      SQ_ALU_WORD0_SRC0_CHAN_MASK | \
      SQ_ALU_WORD0_SRC0_NEG_MASK | \
      SQ_ALU_WORD0_SRC1_SEL_MASK | \
      SQ_ALU_WORD0_SRC1_REL_MASK | \
      SQ_ALU_WORD0_SRC1_CHAN_MASK | \
      SQ_ALU_WORD0_SRC1_NEG_MASK | \
      SQ_ALU_WORD0_INDEX_MODE_MASK | \
      SQ_ALU_WORD0_PRED_SEL_MASK | \
      SQ_ALU_WORD0_LAST_MASK)

#define SQ_ALU_WORD0_DEFAULT           0xcdcdcdcd

#define SQ_ALU_WORD0_GET_SRC0_SEL(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC0_SEL_MASK) >> SQ_ALU_WORD0_SRC0_SEL_SHIFT)
#define SQ_ALU_WORD0_GET_SRC0_REL(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC0_REL_MASK) >> SQ_ALU_WORD0_SRC0_REL_SHIFT)
#define SQ_ALU_WORD0_GET_SRC0_CHAN(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC0_CHAN_MASK) >> SQ_ALU_WORD0_SRC0_CHAN_SHIFT)
#define SQ_ALU_WORD0_GET_SRC0_NEG(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC0_NEG_MASK) >> SQ_ALU_WORD0_SRC0_NEG_SHIFT)
#define SQ_ALU_WORD0_GET_SRC1_SEL(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC1_SEL_MASK) >> SQ_ALU_WORD0_SRC1_SEL_SHIFT)
#define SQ_ALU_WORD0_GET_SRC1_REL(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC1_REL_MASK) >> SQ_ALU_WORD0_SRC1_REL_SHIFT)
#define SQ_ALU_WORD0_GET_SRC1_CHAN(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC1_CHAN_MASK) >> SQ_ALU_WORD0_SRC1_CHAN_SHIFT)
#define SQ_ALU_WORD0_GET_SRC1_NEG(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_SRC1_NEG_MASK) >> SQ_ALU_WORD0_SRC1_NEG_SHIFT)
#define SQ_ALU_WORD0_GET_INDEX_MODE(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_INDEX_MODE_MASK) >> SQ_ALU_WORD0_INDEX_MODE_SHIFT)
#define SQ_ALU_WORD0_GET_PRED_SEL(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_PRED_SEL_MASK) >> SQ_ALU_WORD0_PRED_SEL_SHIFT)
#define SQ_ALU_WORD0_GET_LAST(sq_alu_word0) \
     ((sq_alu_word0 & SQ_ALU_WORD0_LAST_MASK) >> SQ_ALU_WORD0_LAST_SHIFT)

#define SQ_ALU_WORD0_SET_SRC0_SEL(sq_alu_word0_reg, src0_sel) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC0_SEL_MASK) | (src0_sel << SQ_ALU_WORD0_SRC0_SEL_SHIFT)
#define SQ_ALU_WORD0_SET_SRC0_REL(sq_alu_word0_reg, src0_rel) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC0_REL_MASK) | (src0_rel << SQ_ALU_WORD0_SRC0_REL_SHIFT)
#define SQ_ALU_WORD0_SET_SRC0_CHAN(sq_alu_word0_reg, src0_chan) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC0_CHAN_MASK) | (src0_chan << SQ_ALU_WORD0_SRC0_CHAN_SHIFT)
#define SQ_ALU_WORD0_SET_SRC0_NEG(sq_alu_word0_reg, src0_neg) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC0_NEG_MASK) | (src0_neg << SQ_ALU_WORD0_SRC0_NEG_SHIFT)
#define SQ_ALU_WORD0_SET_SRC1_SEL(sq_alu_word0_reg, src1_sel) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC1_SEL_MASK) | (src1_sel << SQ_ALU_WORD0_SRC1_SEL_SHIFT)
#define SQ_ALU_WORD0_SET_SRC1_REL(sq_alu_word0_reg, src1_rel) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC1_REL_MASK) | (src1_rel << SQ_ALU_WORD0_SRC1_REL_SHIFT)
#define SQ_ALU_WORD0_SET_SRC1_CHAN(sq_alu_word0_reg, src1_chan) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC1_CHAN_MASK) | (src1_chan << SQ_ALU_WORD0_SRC1_CHAN_SHIFT)
#define SQ_ALU_WORD0_SET_SRC1_NEG(sq_alu_word0_reg, src1_neg) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_SRC1_NEG_MASK) | (src1_neg << SQ_ALU_WORD0_SRC1_NEG_SHIFT)
#define SQ_ALU_WORD0_SET_INDEX_MODE(sq_alu_word0_reg, index_mode) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_INDEX_MODE_MASK) | (index_mode << SQ_ALU_WORD0_INDEX_MODE_SHIFT)
#define SQ_ALU_WORD0_SET_PRED_SEL(sq_alu_word0_reg, pred_sel) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_PRED_SEL_MASK) | (pred_sel << SQ_ALU_WORD0_PRED_SEL_SHIFT)
#define SQ_ALU_WORD0_SET_LAST(sq_alu_word0_reg, last) \
     sq_alu_word0_reg = (sq_alu_word0_reg & ~SQ_ALU_WORD0_LAST_MASK) | (last << SQ_ALU_WORD0_LAST_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_alu_word0_t {
          unsigned int src0_sel                       : SQ_ALU_WORD0_SRC0_SEL_SIZE;
          unsigned int src0_rel                       : SQ_ALU_WORD0_SRC0_REL_SIZE;
          unsigned int src0_chan                      : SQ_ALU_WORD0_SRC0_CHAN_SIZE;
          unsigned int src0_neg                       : SQ_ALU_WORD0_SRC0_NEG_SIZE;
          unsigned int src1_sel                       : SQ_ALU_WORD0_SRC1_SEL_SIZE;
          unsigned int src1_rel                       : SQ_ALU_WORD0_SRC1_REL_SIZE;
          unsigned int src1_chan                      : SQ_ALU_WORD0_SRC1_CHAN_SIZE;
          unsigned int src1_neg                       : SQ_ALU_WORD0_SRC1_NEG_SIZE;
          unsigned int index_mode                     : SQ_ALU_WORD0_INDEX_MODE_SIZE;
          unsigned int pred_sel                       : SQ_ALU_WORD0_PRED_SEL_SIZE;
          unsigned int last                           : SQ_ALU_WORD0_LAST_SIZE;
     } sq_alu_word0_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_alu_word0_t {
          unsigned int last                           : SQ_ALU_WORD0_LAST_SIZE;
          unsigned int pred_sel                       : SQ_ALU_WORD0_PRED_SEL_SIZE;
          unsigned int index_mode                     : SQ_ALU_WORD0_INDEX_MODE_SIZE;
          unsigned int src1_neg                       : SQ_ALU_WORD0_SRC1_NEG_SIZE;
          unsigned int src1_chan                      : SQ_ALU_WORD0_SRC1_CHAN_SIZE;
          unsigned int src1_rel                       : SQ_ALU_WORD0_SRC1_REL_SIZE;
          unsigned int src1_sel                       : SQ_ALU_WORD0_SRC1_SEL_SIZE;
          unsigned int src0_neg                       : SQ_ALU_WORD0_SRC0_NEG_SIZE;
          unsigned int src0_chan                      : SQ_ALU_WORD0_SRC0_CHAN_SIZE;
          unsigned int src0_rel                       : SQ_ALU_WORD0_SRC0_REL_SIZE;
          unsigned int src0_sel                       : SQ_ALU_WORD0_SRC0_SEL_SIZE;
     } sq_alu_word0_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_alu_word0_t f;
} sq_alu_word0_u;


/*
 * SQ_ALU_WORD1 struct
 */

#define SQ_ALU_WORD1_ENCODING_SIZE     3
#define SQ_ALU_WORD1_BANK_SWIZZLE_SIZE 3
#define SQ_ALU_WORD1_DST_GPR_SIZE      7
#define SQ_ALU_WORD1_DST_REL_SIZE      1
#define SQ_ALU_WORD1_DST_CHAN_SIZE     2
#define SQ_ALU_WORD1_CLAMP_SIZE        1

#define SQ_ALU_WORD1_ENCODING_SHIFT    15
#define SQ_ALU_WORD1_BANK_SWIZZLE_SHIFT 18
#define SQ_ALU_WORD1_DST_GPR_SHIFT     21
#define SQ_ALU_WORD1_DST_REL_SHIFT     28
#define SQ_ALU_WORD1_DST_CHAN_SHIFT    29
#define SQ_ALU_WORD1_CLAMP_SHIFT       31

#define SQ_ALU_WORD1_ENCODING_MASK     0x00038000
#define SQ_ALU_WORD1_BANK_SWIZZLE_MASK 0x001c0000
#define SQ_ALU_WORD1_DST_GPR_MASK      0x0fe00000
#define SQ_ALU_WORD1_DST_REL_MASK      0x10000000
#define SQ_ALU_WORD1_DST_CHAN_MASK     0x60000000
#define SQ_ALU_WORD1_CLAMP_MASK        0x80000000

#define SQ_ALU_WORD1_MASK \
     (SQ_ALU_WORD1_ENCODING_MASK | \
      SQ_ALU_WORD1_BANK_SWIZZLE_MASK | \
      SQ_ALU_WORD1_DST_GPR_MASK | \
      SQ_ALU_WORD1_DST_REL_MASK | \
      SQ_ALU_WORD1_DST_CHAN_MASK | \
      SQ_ALU_WORD1_CLAMP_MASK)

#define SQ_ALU_WORD1_DEFAULT           0xcdcd8000

#define SQ_ALU_WORD1_GET_ENCODING(sq_alu_word1) \
     ((sq_alu_word1 & SQ_ALU_WORD1_ENCODING_MASK) >> SQ_ALU_WORD1_ENCODING_SHIFT)
#define SQ_ALU_WORD1_GET_BANK_SWIZZLE(sq_alu_word1) \
     ((sq_alu_word1 & SQ_ALU_WORD1_BANK_SWIZZLE_MASK) >> SQ_ALU_WORD1_BANK_SWIZZLE_SHIFT)
#define SQ_ALU_WORD1_GET_DST_GPR(sq_alu_word1) \
     ((sq_alu_word1 & SQ_ALU_WORD1_DST_GPR_MASK) >> SQ_ALU_WORD1_DST_GPR_SHIFT)
#define SQ_ALU_WORD1_GET_DST_REL(sq_alu_word1) \
     ((sq_alu_word1 & SQ_ALU_WORD1_DST_REL_MASK) >> SQ_ALU_WORD1_DST_REL_SHIFT)
#define SQ_ALU_WORD1_GET_DST_CHAN(sq_alu_word1) \
     ((sq_alu_word1 & SQ_ALU_WORD1_DST_CHAN_MASK) >> SQ_ALU_WORD1_DST_CHAN_SHIFT)
#define SQ_ALU_WORD1_GET_CLAMP(sq_alu_word1) \
     ((sq_alu_word1 & SQ_ALU_WORD1_CLAMP_MASK) >> SQ_ALU_WORD1_CLAMP_SHIFT)

#define SQ_ALU_WORD1_SET_ENCODING(sq_alu_word1_reg, encoding) \
     sq_alu_word1_reg = (sq_alu_word1_reg & ~SQ_ALU_WORD1_ENCODING_MASK) | (encoding << SQ_ALU_WORD1_ENCODING_SHIFT)
#define SQ_ALU_WORD1_SET_BANK_SWIZZLE(sq_alu_word1_reg, bank_swizzle) \
     sq_alu_word1_reg = (sq_alu_word1_reg & ~SQ_ALU_WORD1_BANK_SWIZZLE_MASK) | (bank_swizzle << SQ_ALU_WORD1_BANK_SWIZZLE_SHIFT)
#define SQ_ALU_WORD1_SET_DST_GPR(sq_alu_word1_reg, dst_gpr) \
     sq_alu_word1_reg = (sq_alu_word1_reg & ~SQ_ALU_WORD1_DST_GPR_MASK) | (dst_gpr << SQ_ALU_WORD1_DST_GPR_SHIFT)
#define SQ_ALU_WORD1_SET_DST_REL(sq_alu_word1_reg, dst_rel) \
     sq_alu_word1_reg = (sq_alu_word1_reg & ~SQ_ALU_WORD1_DST_REL_MASK) | (dst_rel << SQ_ALU_WORD1_DST_REL_SHIFT)
#define SQ_ALU_WORD1_SET_DST_CHAN(sq_alu_word1_reg, dst_chan) \
     sq_alu_word1_reg = (sq_alu_word1_reg & ~SQ_ALU_WORD1_DST_CHAN_MASK) | (dst_chan << SQ_ALU_WORD1_DST_CHAN_SHIFT)
#define SQ_ALU_WORD1_SET_CLAMP(sq_alu_word1_reg, clamp) \
     sq_alu_word1_reg = (sq_alu_word1_reg & ~SQ_ALU_WORD1_CLAMP_MASK) | (clamp << SQ_ALU_WORD1_CLAMP_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_alu_word1_t {
          unsigned int                                : 15;
          unsigned int encoding                       : SQ_ALU_WORD1_ENCODING_SIZE;
          unsigned int bank_swizzle                   : SQ_ALU_WORD1_BANK_SWIZZLE_SIZE;
          unsigned int dst_gpr                        : SQ_ALU_WORD1_DST_GPR_SIZE;
          unsigned int dst_rel                        : SQ_ALU_WORD1_DST_REL_SIZE;
          unsigned int dst_chan                       : SQ_ALU_WORD1_DST_CHAN_SIZE;
          unsigned int clamp                          : SQ_ALU_WORD1_CLAMP_SIZE;
     } sq_alu_word1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_alu_word1_t {
          unsigned int clamp                          : SQ_ALU_WORD1_CLAMP_SIZE;
          unsigned int dst_chan                       : SQ_ALU_WORD1_DST_CHAN_SIZE;
          unsigned int dst_rel                        : SQ_ALU_WORD1_DST_REL_SIZE;
          unsigned int dst_gpr                        : SQ_ALU_WORD1_DST_GPR_SIZE;
          unsigned int bank_swizzle                   : SQ_ALU_WORD1_BANK_SWIZZLE_SIZE;
          unsigned int encoding                       : SQ_ALU_WORD1_ENCODING_SIZE;
          unsigned int                                : 15;
     } sq_alu_word1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_alu_word1_t f;
} sq_alu_word1_u;


/*
 * SQ_ALU_WORD1_OP2_V2 struct
 */

#define SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SIZE 1
#define SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SIZE 1
#define SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SIZE 1
#define SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SIZE 1
#define SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SIZE 1
#define SQ_ALU_WORD1_OP2_V2_OMOD_SIZE  2
#define SQ_ALU_WORD1_OP2_V2_ALU_INST_SIZE 11

#define SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SHIFT 0
#define SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SHIFT 1
#define SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SHIFT 2
#define SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SHIFT 3
#define SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SHIFT 4
#define SQ_ALU_WORD1_OP2_V2_OMOD_SHIFT 5
#define SQ_ALU_WORD1_OP2_V2_ALU_INST_SHIFT 7

#define SQ_ALU_WORD1_OP2_V2_SRC0_ABS_MASK 0x00000001
#define SQ_ALU_WORD1_OP2_V2_SRC1_ABS_MASK 0x00000002
#define SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_MASK 0x00000004
#define SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_MASK 0x00000008
#define SQ_ALU_WORD1_OP2_V2_WRITE_MASK_MASK 0x00000010
#define SQ_ALU_WORD1_OP2_V2_OMOD_MASK  0x00000060
#define SQ_ALU_WORD1_OP2_V2_ALU_INST_MASK 0x0003ff80

#define SQ_ALU_WORD1_OP2_V2_MASK \
     (SQ_ALU_WORD1_OP2_V2_SRC0_ABS_MASK | \
      SQ_ALU_WORD1_OP2_V2_SRC1_ABS_MASK | \
      SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_MASK | \
      SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_MASK | \
      SQ_ALU_WORD1_OP2_V2_WRITE_MASK_MASK | \
      SQ_ALU_WORD1_OP2_V2_OMOD_MASK | \
      SQ_ALU_WORD1_OP2_V2_ALU_INST_MASK)

#define SQ_ALU_WORD1_OP2_V2_DEFAULT    0x0001cdcd

#define SQ_ALU_WORD1_OP2_V2_GET_SRC0_ABS(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_SRC0_ABS_MASK) >> SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_GET_SRC1_ABS(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_SRC1_ABS_MASK) >> SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_GET_UPDATE_EXECUTE_MASK(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_MASK) >> SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_GET_UPDATE_PRED(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_MASK) >> SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_GET_WRITE_MASK(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_WRITE_MASK_MASK) >> SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_GET_OMOD(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_OMOD_MASK) >> SQ_ALU_WORD1_OP2_V2_OMOD_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_GET_ALU_INST(sq_alu_word1_op2_v2) \
     ((sq_alu_word1_op2_v2 & SQ_ALU_WORD1_OP2_V2_ALU_INST_MASK) >> SQ_ALU_WORD1_OP2_V2_ALU_INST_SHIFT)

#define SQ_ALU_WORD1_OP2_V2_SET_SRC0_ABS(sq_alu_word1_op2_v2_reg, src0_abs) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_SRC0_ABS_MASK) | (src0_abs << SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_SET_SRC1_ABS(sq_alu_word1_op2_v2_reg, src1_abs) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_SRC1_ABS_MASK) | (src1_abs << SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_SET_UPDATE_EXECUTE_MASK(sq_alu_word1_op2_v2_reg, update_execute_mask) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_MASK) | (update_execute_mask << SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_SET_UPDATE_PRED(sq_alu_word1_op2_v2_reg, update_pred) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_MASK) | (update_pred << SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_SET_WRITE_MASK(sq_alu_word1_op2_v2_reg, write_mask) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_WRITE_MASK_MASK) | (write_mask << SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_SET_OMOD(sq_alu_word1_op2_v2_reg, omod) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_OMOD_MASK) | (omod << SQ_ALU_WORD1_OP2_V2_OMOD_SHIFT)
#define SQ_ALU_WORD1_OP2_V2_SET_ALU_INST(sq_alu_word1_op2_v2_reg, alu_inst) \
     sq_alu_word1_op2_v2_reg = (sq_alu_word1_op2_v2_reg & ~SQ_ALU_WORD1_OP2_V2_ALU_INST_MASK) | (alu_inst << SQ_ALU_WORD1_OP2_V2_ALU_INST_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_alu_word1_op2_v2_t {
          unsigned int src0_abs                       : SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SIZE;
          unsigned int src1_abs                       : SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SIZE;
          unsigned int update_execute_mask            : SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SIZE;
          unsigned int update_pred                    : SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SIZE;
          unsigned int write_mask                     : SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SIZE;
          unsigned int omod                           : SQ_ALU_WORD1_OP2_V2_OMOD_SIZE;
          unsigned int alu_inst                       : SQ_ALU_WORD1_OP2_V2_ALU_INST_SIZE;
          unsigned int                                : 14;
     } sq_alu_word1_op2_v2_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_alu_word1_op2_v2_t {
          unsigned int                                : 14;
          unsigned int alu_inst                       : SQ_ALU_WORD1_OP2_V2_ALU_INST_SIZE;
          unsigned int omod                           : SQ_ALU_WORD1_OP2_V2_OMOD_SIZE;
          unsigned int write_mask                     : SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SIZE;
          unsigned int update_pred                    : SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SIZE;
          unsigned int update_execute_mask            : SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SIZE;
          unsigned int src1_abs                       : SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SIZE;
          unsigned int src0_abs                       : SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SIZE;
     } sq_alu_word1_op2_v2_t;

#endif

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_alu_word1_op2_r6xx_t {
          unsigned int src0_abs                       : SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SIZE;
          unsigned int src1_abs                       : SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SIZE;
          unsigned int update_execute_mask            : SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SIZE;
          unsigned int update_pred                    : SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SIZE;
          unsigned int write_mask                     : SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SIZE;
          unsigned int fog_export                     : 1;
          unsigned int omod                           : SQ_ALU_WORD1_OP2_V2_OMOD_SIZE;
          unsigned int alu_inst                       : 10;
          unsigned int                                : 14;
     } sq_alu_word1_op2_v1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_alu_word1_op2_r6xx_t {
          unsigned int                                : 14;
          unsigned int alu_inst                       : 10;
          unsigned int omod                           : SQ_ALU_WORD1_OP2_V2_OMOD_SIZE;
          unsigned int fog_export                     : 1;
          unsigned int write_mask                     : SQ_ALU_WORD1_OP2_V2_WRITE_MASK_SIZE;
          unsigned int update_pred                    : SQ_ALU_WORD1_OP2_V2_UPDATE_PRED_SIZE;
          unsigned int update_execute_mask            : SQ_ALU_WORD1_OP2_V2_UPDATE_EXECUTE_MASK_SIZE;
          unsigned int src1_abs                       : SQ_ALU_WORD1_OP2_V2_SRC1_ABS_SIZE;
          unsigned int src0_abs                       : SQ_ALU_WORD1_OP2_V2_SRC0_ABS_SIZE;
     } sq_alu_word1_op2_v1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_alu_word1_op2_v2_t f;
     sq_alu_word1_op2_v1_t f6;
} sq_alu_word1_op2_v2_u;


/*
 * SQ_ALU_WORD1_OP3 struct
 */

#define SQ_ALU_WORD1_OP3_SRC2_SEL_SIZE 9
#define SQ_ALU_WORD1_OP3_SRC2_REL_SIZE 1
#define SQ_ALU_WORD1_OP3_SRC2_CHAN_SIZE 2
#define SQ_ALU_WORD1_OP3_SRC2_NEG_SIZE 1
#define SQ_ALU_WORD1_OP3_ALU_INST_SIZE 5

#define SQ_ALU_WORD1_OP3_SRC2_SEL_SHIFT 0
#define SQ_ALU_WORD1_OP3_SRC2_REL_SHIFT 9
#define SQ_ALU_WORD1_OP3_SRC2_CHAN_SHIFT 10
#define SQ_ALU_WORD1_OP3_SRC2_NEG_SHIFT 12
#define SQ_ALU_WORD1_OP3_ALU_INST_SHIFT 13

#define SQ_ALU_WORD1_OP3_SRC2_SEL_MASK 0x000001ff
#define SQ_ALU_WORD1_OP3_SRC2_REL_MASK 0x00000200
#define SQ_ALU_WORD1_OP3_SRC2_CHAN_MASK 0x00000c00
#define SQ_ALU_WORD1_OP3_SRC2_NEG_MASK 0x00001000
#define SQ_ALU_WORD1_OP3_ALU_INST_MASK 0x0003e000

#define SQ_ALU_WORD1_OP3_MASK \
     (SQ_ALU_WORD1_OP3_SRC2_SEL_MASK | \
      SQ_ALU_WORD1_OP3_SRC2_REL_MASK | \
      SQ_ALU_WORD1_OP3_SRC2_CHAN_MASK | \
      SQ_ALU_WORD1_OP3_SRC2_NEG_MASK | \
      SQ_ALU_WORD1_OP3_ALU_INST_MASK)

#define SQ_ALU_WORD1_OP3_DEFAULT       0x0001cdcd

#define SQ_ALU_WORD1_OP3_GET_SRC2_SEL(sq_alu_word1_op3) \
     ((sq_alu_word1_op3 & SQ_ALU_WORD1_OP3_SRC2_SEL_MASK) >> SQ_ALU_WORD1_OP3_SRC2_SEL_SHIFT)
#define SQ_ALU_WORD1_OP3_GET_SRC2_REL(sq_alu_word1_op3) \
     ((sq_alu_word1_op3 & SQ_ALU_WORD1_OP3_SRC2_REL_MASK) >> SQ_ALU_WORD1_OP3_SRC2_REL_SHIFT)
#define SQ_ALU_WORD1_OP3_GET_SRC2_CHAN(sq_alu_word1_op3) \
     ((sq_alu_word1_op3 & SQ_ALU_WORD1_OP3_SRC2_CHAN_MASK) >> SQ_ALU_WORD1_OP3_SRC2_CHAN_SHIFT)
#define SQ_ALU_WORD1_OP3_GET_SRC2_NEG(sq_alu_word1_op3) \
     ((sq_alu_word1_op3 & SQ_ALU_WORD1_OP3_SRC2_NEG_MASK) >> SQ_ALU_WORD1_OP3_SRC2_NEG_SHIFT)
#define SQ_ALU_WORD1_OP3_GET_ALU_INST(sq_alu_word1_op3) \
     ((sq_alu_word1_op3 & SQ_ALU_WORD1_OP3_ALU_INST_MASK) >> SQ_ALU_WORD1_OP3_ALU_INST_SHIFT)

#define SQ_ALU_WORD1_OP3_SET_SRC2_SEL(sq_alu_word1_op3_reg, src2_sel) \
     sq_alu_word1_op3_reg = (sq_alu_word1_op3_reg & ~SQ_ALU_WORD1_OP3_SRC2_SEL_MASK) | (src2_sel << SQ_ALU_WORD1_OP3_SRC2_SEL_SHIFT)
#define SQ_ALU_WORD1_OP3_SET_SRC2_REL(sq_alu_word1_op3_reg, src2_rel) \
     sq_alu_word1_op3_reg = (sq_alu_word1_op3_reg & ~SQ_ALU_WORD1_OP3_SRC2_REL_MASK) | (src2_rel << SQ_ALU_WORD1_OP3_SRC2_REL_SHIFT)
#define SQ_ALU_WORD1_OP3_SET_SRC2_CHAN(sq_alu_word1_op3_reg, src2_chan) \
     sq_alu_word1_op3_reg = (sq_alu_word1_op3_reg & ~SQ_ALU_WORD1_OP3_SRC2_CHAN_MASK) | (src2_chan << SQ_ALU_WORD1_OP3_SRC2_CHAN_SHIFT)
#define SQ_ALU_WORD1_OP3_SET_SRC2_NEG(sq_alu_word1_op3_reg, src2_neg) \
     sq_alu_word1_op3_reg = (sq_alu_word1_op3_reg & ~SQ_ALU_WORD1_OP3_SRC2_NEG_MASK) | (src2_neg << SQ_ALU_WORD1_OP3_SRC2_NEG_SHIFT)
#define SQ_ALU_WORD1_OP3_SET_ALU_INST(sq_alu_word1_op3_reg, alu_inst) \
     sq_alu_word1_op3_reg = (sq_alu_word1_op3_reg & ~SQ_ALU_WORD1_OP3_ALU_INST_MASK) | (alu_inst << SQ_ALU_WORD1_OP3_ALU_INST_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_alu_word1_op3_t {
          unsigned int src2_sel                       : SQ_ALU_WORD1_OP3_SRC2_SEL_SIZE;
          unsigned int src2_rel                       : SQ_ALU_WORD1_OP3_SRC2_REL_SIZE;
          unsigned int src2_chan                      : SQ_ALU_WORD1_OP3_SRC2_CHAN_SIZE;
          unsigned int src2_neg                       : SQ_ALU_WORD1_OP3_SRC2_NEG_SIZE;
          unsigned int alu_inst                       : SQ_ALU_WORD1_OP3_ALU_INST_SIZE;
          unsigned int                                : 14;
     } sq_alu_word1_op3_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_alu_word1_op3_t {
          unsigned int                                : 14;
          unsigned int alu_inst                       : SQ_ALU_WORD1_OP3_ALU_INST_SIZE;
          unsigned int src2_neg                       : SQ_ALU_WORD1_OP3_SRC2_NEG_SIZE;
          unsigned int src2_chan                      : SQ_ALU_WORD1_OP3_SRC2_CHAN_SIZE;
          unsigned int src2_rel                       : SQ_ALU_WORD1_OP3_SRC2_REL_SIZE;
          unsigned int src2_sel                       : SQ_ALU_WORD1_OP3_SRC2_SEL_SIZE;
     } sq_alu_word1_op3_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_alu_word1_op3_t f;
} sq_alu_word1_op3_u;


/*
 * SQ_TEX_WORD0 struct
 */

#define SQ_TEX_WORD0_TEX_INST_SIZE     5
#define SQ_TEX_WORD0_BC_FRAC_MODE_SIZE 1
#define SQ_TEX_WORD0_FETCH_WHOLE_QUAD_SIZE 1
#define SQ_TEX_WORD0_RESOURCE_ID_SIZE  8
#define SQ_TEX_WORD0_SRC_GPR_SIZE      7
#define SQ_TEX_WORD0_SRC_REL_SIZE      1
#define SQ_TEX_WORD0_ALT_CONST_SIZE    1

#define SQ_TEX_WORD0_TEX_INST_SHIFT    0
#define SQ_TEX_WORD0_BC_FRAC_MODE_SHIFT 5
#define SQ_TEX_WORD0_FETCH_WHOLE_QUAD_SHIFT 7
#define SQ_TEX_WORD0_RESOURCE_ID_SHIFT 8
#define SQ_TEX_WORD0_SRC_GPR_SHIFT     16
#define SQ_TEX_WORD0_SRC_REL_SHIFT     23
#define SQ_TEX_WORD0_ALT_CONST_SHIFT   24

#define SQ_TEX_WORD0_TEX_INST_MASK     0x0000001f
#define SQ_TEX_WORD0_BC_FRAC_MODE_MASK 0x00000020
#define SQ_TEX_WORD0_FETCH_WHOLE_QUAD_MASK 0x00000080
#define SQ_TEX_WORD0_RESOURCE_ID_MASK  0x0000ff00
#define SQ_TEX_WORD0_SRC_GPR_MASK      0x007f0000
#define SQ_TEX_WORD0_SRC_REL_MASK      0x00800000
#define SQ_TEX_WORD0_ALT_CONST_MASK    0x01000000

#define SQ_TEX_WORD0_MASK \
     (SQ_TEX_WORD0_TEX_INST_MASK | \
      SQ_TEX_WORD0_BC_FRAC_MODE_MASK | \
      SQ_TEX_WORD0_FETCH_WHOLE_QUAD_MASK | \
      SQ_TEX_WORD0_RESOURCE_ID_MASK | \
      SQ_TEX_WORD0_SRC_GPR_MASK | \
      SQ_TEX_WORD0_SRC_REL_MASK | \
      SQ_TEX_WORD0_ALT_CONST_MASK)

#define SQ_TEX_WORD0_DEFAULT           0x01cdcd8d

#define SQ_TEX_WORD0_GET_TEX_INST(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_TEX_INST_MASK) >> SQ_TEX_WORD0_TEX_INST_SHIFT)
#define SQ_TEX_WORD0_GET_BC_FRAC_MODE(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_BC_FRAC_MODE_MASK) >> SQ_TEX_WORD0_BC_FRAC_MODE_SHIFT)
#define SQ_TEX_WORD0_GET_FETCH_WHOLE_QUAD(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_FETCH_WHOLE_QUAD_MASK) >> SQ_TEX_WORD0_FETCH_WHOLE_QUAD_SHIFT)
#define SQ_TEX_WORD0_GET_RESOURCE_ID(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_RESOURCE_ID_MASK) >> SQ_TEX_WORD0_RESOURCE_ID_SHIFT)
#define SQ_TEX_WORD0_GET_SRC_GPR(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_SRC_GPR_MASK) >> SQ_TEX_WORD0_SRC_GPR_SHIFT)
#define SQ_TEX_WORD0_GET_SRC_REL(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_SRC_REL_MASK) >> SQ_TEX_WORD0_SRC_REL_SHIFT)
#define SQ_TEX_WORD0_GET_ALT_CONST(sq_tex_word0) \
     ((sq_tex_word0 & SQ_TEX_WORD0_ALT_CONST_MASK) >> SQ_TEX_WORD0_ALT_CONST_SHIFT)

#define SQ_TEX_WORD0_SET_TEX_INST(sq_tex_word0_reg, tex_inst) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_TEX_INST_MASK) | (tex_inst << SQ_TEX_WORD0_TEX_INST_SHIFT)
#define SQ_TEX_WORD0_SET_BC_FRAC_MODE(sq_tex_word0_reg, bc_frac_mode) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_BC_FRAC_MODE_MASK) | (bc_frac_mode << SQ_TEX_WORD0_BC_FRAC_MODE_SHIFT)
#define SQ_TEX_WORD0_SET_FETCH_WHOLE_QUAD(sq_tex_word0_reg, fetch_whole_quad) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_FETCH_WHOLE_QUAD_MASK) | (fetch_whole_quad << SQ_TEX_WORD0_FETCH_WHOLE_QUAD_SHIFT)
#define SQ_TEX_WORD0_SET_RESOURCE_ID(sq_tex_word0_reg, resource_id) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_RESOURCE_ID_MASK) | (resource_id << SQ_TEX_WORD0_RESOURCE_ID_SHIFT)
#define SQ_TEX_WORD0_SET_SRC_GPR(sq_tex_word0_reg, src_gpr) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_SRC_GPR_MASK) | (src_gpr << SQ_TEX_WORD0_SRC_GPR_SHIFT)
#define SQ_TEX_WORD0_SET_SRC_REL(sq_tex_word0_reg, src_rel) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_SRC_REL_MASK) | (src_rel << SQ_TEX_WORD0_SRC_REL_SHIFT)
#define SQ_TEX_WORD0_SET_ALT_CONST(sq_tex_word0_reg, alt_const) \
     sq_tex_word0_reg = (sq_tex_word0_reg & ~SQ_TEX_WORD0_ALT_CONST_MASK) | (alt_const << SQ_TEX_WORD0_ALT_CONST_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_tex_word0_t {
          unsigned int tex_inst                       : SQ_TEX_WORD0_TEX_INST_SIZE;
          unsigned int bc_frac_mode                   : SQ_TEX_WORD0_BC_FRAC_MODE_SIZE;
          unsigned int                                : 1;
          unsigned int fetch_whole_quad               : SQ_TEX_WORD0_FETCH_WHOLE_QUAD_SIZE;
          unsigned int resource_id                    : SQ_TEX_WORD0_RESOURCE_ID_SIZE;
          unsigned int src_gpr                        : SQ_TEX_WORD0_SRC_GPR_SIZE;
          unsigned int src_rel                        : SQ_TEX_WORD0_SRC_REL_SIZE;
          unsigned int alt_const                      : SQ_TEX_WORD0_ALT_CONST_SIZE;
          unsigned int                                : 7;
     } sq_tex_word0_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_tex_word0_t {
          unsigned int                                : 7;
          unsigned int alt_const                      : SQ_TEX_WORD0_ALT_CONST_SIZE;
          unsigned int src_rel                        : SQ_TEX_WORD0_SRC_REL_SIZE;
          unsigned int src_gpr                        : SQ_TEX_WORD0_SRC_GPR_SIZE;
          unsigned int resource_id                    : SQ_TEX_WORD0_RESOURCE_ID_SIZE;
          unsigned int fetch_whole_quad               : SQ_TEX_WORD0_FETCH_WHOLE_QUAD_SIZE;
          unsigned int                                : 1;
          unsigned int bc_frac_mode                   : SQ_TEX_WORD0_BC_FRAC_MODE_SIZE;
          unsigned int tex_inst                       : SQ_TEX_WORD0_TEX_INST_SIZE;
     } sq_tex_word0_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_tex_word0_t f;
} sq_tex_word0_u;


/*
 * SQ_TEX_WORD1 struct
 */

#define SQ_TEX_WORD1_DST_GPR_SIZE      7
#define SQ_TEX_WORD1_DST_REL_SIZE      1
#define SQ_TEX_WORD1_DST_SEL_X_SIZE    3
#define SQ_TEX_WORD1_DST_SEL_Y_SIZE    3
#define SQ_TEX_WORD1_DST_SEL_Z_SIZE    3
#define SQ_TEX_WORD1_DST_SEL_W_SIZE    3
#define SQ_TEX_WORD1_LOD_BIAS_SIZE     7
#define SQ_TEX_WORD1_COORD_TYPE_X_SIZE 1
#define SQ_TEX_WORD1_COORD_TYPE_Y_SIZE 1
#define SQ_TEX_WORD1_COORD_TYPE_Z_SIZE 1
#define SQ_TEX_WORD1_COORD_TYPE_W_SIZE 1

#define SQ_TEX_WORD1_DST_GPR_SHIFT     0
#define SQ_TEX_WORD1_DST_REL_SHIFT     7
#define SQ_TEX_WORD1_DST_SEL_X_SHIFT   9
#define SQ_TEX_WORD1_DST_SEL_Y_SHIFT   12
#define SQ_TEX_WORD1_DST_SEL_Z_SHIFT   15
#define SQ_TEX_WORD1_DST_SEL_W_SHIFT   18
#define SQ_TEX_WORD1_LOD_BIAS_SHIFT    21
#define SQ_TEX_WORD1_COORD_TYPE_X_SHIFT 28
#define SQ_TEX_WORD1_COORD_TYPE_Y_SHIFT 29
#define SQ_TEX_WORD1_COORD_TYPE_Z_SHIFT 30
#define SQ_TEX_WORD1_COORD_TYPE_W_SHIFT 31

#define SQ_TEX_WORD1_DST_GPR_MASK      0x0000007f
#define SQ_TEX_WORD1_DST_REL_MASK      0x00000080
#define SQ_TEX_WORD1_DST_SEL_X_MASK    0x00000e00
#define SQ_TEX_WORD1_DST_SEL_Y_MASK    0x00007000
#define SQ_TEX_WORD1_DST_SEL_Z_MASK    0x00038000
#define SQ_TEX_WORD1_DST_SEL_W_MASK    0x001c0000
#define SQ_TEX_WORD1_LOD_BIAS_MASK     0x0fe00000
#define SQ_TEX_WORD1_COORD_TYPE_X_MASK 0x10000000
#define SQ_TEX_WORD1_COORD_TYPE_Y_MASK 0x20000000
#define SQ_TEX_WORD1_COORD_TYPE_Z_MASK 0x40000000
#define SQ_TEX_WORD1_COORD_TYPE_W_MASK 0x80000000

#define SQ_TEX_WORD1_MASK \
     (SQ_TEX_WORD1_DST_GPR_MASK | \
      SQ_TEX_WORD1_DST_REL_MASK | \
      SQ_TEX_WORD1_DST_SEL_X_MASK | \
      SQ_TEX_WORD1_DST_SEL_Y_MASK | \
      SQ_TEX_WORD1_DST_SEL_Z_MASK | \
      SQ_TEX_WORD1_DST_SEL_W_MASK | \
      SQ_TEX_WORD1_LOD_BIAS_MASK | \
      SQ_TEX_WORD1_COORD_TYPE_X_MASK | \
      SQ_TEX_WORD1_COORD_TYPE_Y_MASK | \
      SQ_TEX_WORD1_COORD_TYPE_Z_MASK | \
      SQ_TEX_WORD1_COORD_TYPE_W_MASK)

#define SQ_TEX_WORD1_DEFAULT           0xcdcdcccd

#define SQ_TEX_WORD1_GET_DST_GPR(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_DST_GPR_MASK) >> SQ_TEX_WORD1_DST_GPR_SHIFT)
#define SQ_TEX_WORD1_GET_DST_REL(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_DST_REL_MASK) >> SQ_TEX_WORD1_DST_REL_SHIFT)
#define SQ_TEX_WORD1_GET_DST_SEL_X(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_DST_SEL_X_MASK) >> SQ_TEX_WORD1_DST_SEL_X_SHIFT)
#define SQ_TEX_WORD1_GET_DST_SEL_Y(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_DST_SEL_Y_MASK) >> SQ_TEX_WORD1_DST_SEL_Y_SHIFT)
#define SQ_TEX_WORD1_GET_DST_SEL_Z(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_DST_SEL_Z_MASK) >> SQ_TEX_WORD1_DST_SEL_Z_SHIFT)
#define SQ_TEX_WORD1_GET_DST_SEL_W(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_DST_SEL_W_MASK) >> SQ_TEX_WORD1_DST_SEL_W_SHIFT)
#define SQ_TEX_WORD1_GET_LOD_BIAS(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_LOD_BIAS_MASK) >> SQ_TEX_WORD1_LOD_BIAS_SHIFT)
#define SQ_TEX_WORD1_GET_COORD_TYPE_X(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_COORD_TYPE_X_MASK) >> SQ_TEX_WORD1_COORD_TYPE_X_SHIFT)
#define SQ_TEX_WORD1_GET_COORD_TYPE_Y(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_COORD_TYPE_Y_MASK) >> SQ_TEX_WORD1_COORD_TYPE_Y_SHIFT)
#define SQ_TEX_WORD1_GET_COORD_TYPE_Z(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_COORD_TYPE_Z_MASK) >> SQ_TEX_WORD1_COORD_TYPE_Z_SHIFT)
#define SQ_TEX_WORD1_GET_COORD_TYPE_W(sq_tex_word1) \
     ((sq_tex_word1 & SQ_TEX_WORD1_COORD_TYPE_W_MASK) >> SQ_TEX_WORD1_COORD_TYPE_W_SHIFT)

#define SQ_TEX_WORD1_SET_DST_GPR(sq_tex_word1_reg, dst_gpr) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_DST_GPR_MASK) | (dst_gpr << SQ_TEX_WORD1_DST_GPR_SHIFT)
#define SQ_TEX_WORD1_SET_DST_REL(sq_tex_word1_reg, dst_rel) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_DST_REL_MASK) | (dst_rel << SQ_TEX_WORD1_DST_REL_SHIFT)
#define SQ_TEX_WORD1_SET_DST_SEL_X(sq_tex_word1_reg, dst_sel_x) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_DST_SEL_X_MASK) | (dst_sel_x << SQ_TEX_WORD1_DST_SEL_X_SHIFT)
#define SQ_TEX_WORD1_SET_DST_SEL_Y(sq_tex_word1_reg, dst_sel_y) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_DST_SEL_Y_MASK) | (dst_sel_y << SQ_TEX_WORD1_DST_SEL_Y_SHIFT)
#define SQ_TEX_WORD1_SET_DST_SEL_Z(sq_tex_word1_reg, dst_sel_z) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_DST_SEL_Z_MASK) | (dst_sel_z << SQ_TEX_WORD1_DST_SEL_Z_SHIFT)
#define SQ_TEX_WORD1_SET_DST_SEL_W(sq_tex_word1_reg, dst_sel_w) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_DST_SEL_W_MASK) | (dst_sel_w << SQ_TEX_WORD1_DST_SEL_W_SHIFT)
#define SQ_TEX_WORD1_SET_LOD_BIAS(sq_tex_word1_reg, lod_bias) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_LOD_BIAS_MASK) | (lod_bias << SQ_TEX_WORD1_LOD_BIAS_SHIFT)
#define SQ_TEX_WORD1_SET_COORD_TYPE_X(sq_tex_word1_reg, coord_type_x) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_COORD_TYPE_X_MASK) | (coord_type_x << SQ_TEX_WORD1_COORD_TYPE_X_SHIFT)
#define SQ_TEX_WORD1_SET_COORD_TYPE_Y(sq_tex_word1_reg, coord_type_y) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_COORD_TYPE_Y_MASK) | (coord_type_y << SQ_TEX_WORD1_COORD_TYPE_Y_SHIFT)
#define SQ_TEX_WORD1_SET_COORD_TYPE_Z(sq_tex_word1_reg, coord_type_z) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_COORD_TYPE_Z_MASK) | (coord_type_z << SQ_TEX_WORD1_COORD_TYPE_Z_SHIFT)
#define SQ_TEX_WORD1_SET_COORD_TYPE_W(sq_tex_word1_reg, coord_type_w) \
     sq_tex_word1_reg = (sq_tex_word1_reg & ~SQ_TEX_WORD1_COORD_TYPE_W_MASK) | (coord_type_w << SQ_TEX_WORD1_COORD_TYPE_W_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_tex_word1_t {
          unsigned int dst_gpr                        : SQ_TEX_WORD1_DST_GPR_SIZE;
          unsigned int dst_rel                        : SQ_TEX_WORD1_DST_REL_SIZE;
          unsigned int                                : 1;
          unsigned int dst_sel_x                      : SQ_TEX_WORD1_DST_SEL_X_SIZE;
          unsigned int dst_sel_y                      : SQ_TEX_WORD1_DST_SEL_Y_SIZE;
          unsigned int dst_sel_z                      : SQ_TEX_WORD1_DST_SEL_Z_SIZE;
          unsigned int dst_sel_w                      : SQ_TEX_WORD1_DST_SEL_W_SIZE;
          unsigned int lod_bias                       : SQ_TEX_WORD1_LOD_BIAS_SIZE;
          unsigned int coord_type_x                   : SQ_TEX_WORD1_COORD_TYPE_X_SIZE;
          unsigned int coord_type_y                   : SQ_TEX_WORD1_COORD_TYPE_Y_SIZE;
          unsigned int coord_type_z                   : SQ_TEX_WORD1_COORD_TYPE_Z_SIZE;
          unsigned int coord_type_w                   : SQ_TEX_WORD1_COORD_TYPE_W_SIZE;
     } sq_tex_word1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_tex_word1_t {
          unsigned int coord_type_w                   : SQ_TEX_WORD1_COORD_TYPE_W_SIZE;
          unsigned int coord_type_z                   : SQ_TEX_WORD1_COORD_TYPE_Z_SIZE;
          unsigned int coord_type_y                   : SQ_TEX_WORD1_COORD_TYPE_Y_SIZE;
          unsigned int coord_type_x                   : SQ_TEX_WORD1_COORD_TYPE_X_SIZE;
          unsigned int lod_bias                       : SQ_TEX_WORD1_LOD_BIAS_SIZE;
          unsigned int dst_sel_w                      : SQ_TEX_WORD1_DST_SEL_W_SIZE;
          unsigned int dst_sel_z                      : SQ_TEX_WORD1_DST_SEL_Z_SIZE;
          unsigned int dst_sel_y                      : SQ_TEX_WORD1_DST_SEL_Y_SIZE;
          unsigned int dst_sel_x                      : SQ_TEX_WORD1_DST_SEL_X_SIZE;
          unsigned int                                : 1;
          unsigned int dst_rel                        : SQ_TEX_WORD1_DST_REL_SIZE;
          unsigned int dst_gpr                        : SQ_TEX_WORD1_DST_GPR_SIZE;
     } sq_tex_word1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_tex_word1_t f;
} sq_tex_word1_u;


/*
 * SQ_TEX_WORD2 struct
 */

#define SQ_TEX_WORD2_OFFSET_X_SIZE     5
#define SQ_TEX_WORD2_OFFSET_Y_SIZE     5
#define SQ_TEX_WORD2_OFFSET_Z_SIZE     5
#define SQ_TEX_WORD2_SAMPLER_ID_SIZE   5
#define SQ_TEX_WORD2_SRC_SEL_X_SIZE    3
#define SQ_TEX_WORD2_SRC_SEL_Y_SIZE    3
#define SQ_TEX_WORD2_SRC_SEL_Z_SIZE    3
#define SQ_TEX_WORD2_SRC_SEL_W_SIZE    3

#define SQ_TEX_WORD2_OFFSET_X_SHIFT    0
#define SQ_TEX_WORD2_OFFSET_Y_SHIFT    5
#define SQ_TEX_WORD2_OFFSET_Z_SHIFT    10
#define SQ_TEX_WORD2_SAMPLER_ID_SHIFT  15
#define SQ_TEX_WORD2_SRC_SEL_X_SHIFT   20
#define SQ_TEX_WORD2_SRC_SEL_Y_SHIFT   23
#define SQ_TEX_WORD2_SRC_SEL_Z_SHIFT   26
#define SQ_TEX_WORD2_SRC_SEL_W_SHIFT   29

#define SQ_TEX_WORD2_OFFSET_X_MASK     0x0000001f
#define SQ_TEX_WORD2_OFFSET_Y_MASK     0x000003e0
#define SQ_TEX_WORD2_OFFSET_Z_MASK     0x00007c00
#define SQ_TEX_WORD2_SAMPLER_ID_MASK   0x000f8000
#define SQ_TEX_WORD2_SRC_SEL_X_MASK    0x00700000
#define SQ_TEX_WORD2_SRC_SEL_Y_MASK    0x03800000
#define SQ_TEX_WORD2_SRC_SEL_Z_MASK    0x1c000000
#define SQ_TEX_WORD2_SRC_SEL_W_MASK    0xe0000000

#define SQ_TEX_WORD2_MASK \
     (SQ_TEX_WORD2_OFFSET_X_MASK | \
      SQ_TEX_WORD2_OFFSET_Y_MASK | \
      SQ_TEX_WORD2_OFFSET_Z_MASK | \
      SQ_TEX_WORD2_SAMPLER_ID_MASK | \
      SQ_TEX_WORD2_SRC_SEL_X_MASK | \
      SQ_TEX_WORD2_SRC_SEL_Y_MASK | \
      SQ_TEX_WORD2_SRC_SEL_Z_MASK | \
      SQ_TEX_WORD2_SRC_SEL_W_MASK)

#define SQ_TEX_WORD2_DEFAULT           0xcdcdcdcd

#define SQ_TEX_WORD2_GET_OFFSET_X(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_OFFSET_X_MASK) >> SQ_TEX_WORD2_OFFSET_X_SHIFT)
#define SQ_TEX_WORD2_GET_OFFSET_Y(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_OFFSET_Y_MASK) >> SQ_TEX_WORD2_OFFSET_Y_SHIFT)
#define SQ_TEX_WORD2_GET_OFFSET_Z(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_OFFSET_Z_MASK) >> SQ_TEX_WORD2_OFFSET_Z_SHIFT)
#define SQ_TEX_WORD2_GET_SAMPLER_ID(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_SAMPLER_ID_MASK) >> SQ_TEX_WORD2_SAMPLER_ID_SHIFT)
#define SQ_TEX_WORD2_GET_SRC_SEL_X(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_SRC_SEL_X_MASK) >> SQ_TEX_WORD2_SRC_SEL_X_SHIFT)
#define SQ_TEX_WORD2_GET_SRC_SEL_Y(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_SRC_SEL_Y_MASK) >> SQ_TEX_WORD2_SRC_SEL_Y_SHIFT)
#define SQ_TEX_WORD2_GET_SRC_SEL_Z(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_SRC_SEL_Z_MASK) >> SQ_TEX_WORD2_SRC_SEL_Z_SHIFT)
#define SQ_TEX_WORD2_GET_SRC_SEL_W(sq_tex_word2) \
     ((sq_tex_word2 & SQ_TEX_WORD2_SRC_SEL_W_MASK) >> SQ_TEX_WORD2_SRC_SEL_W_SHIFT)

#define SQ_TEX_WORD2_SET_OFFSET_X(sq_tex_word2_reg, offset_x) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_OFFSET_X_MASK) | (offset_x << SQ_TEX_WORD2_OFFSET_X_SHIFT)
#define SQ_TEX_WORD2_SET_OFFSET_Y(sq_tex_word2_reg, offset_y) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_OFFSET_Y_MASK) | (offset_y << SQ_TEX_WORD2_OFFSET_Y_SHIFT)
#define SQ_TEX_WORD2_SET_OFFSET_Z(sq_tex_word2_reg, offset_z) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_OFFSET_Z_MASK) | (offset_z << SQ_TEX_WORD2_OFFSET_Z_SHIFT)
#define SQ_TEX_WORD2_SET_SAMPLER_ID(sq_tex_word2_reg, sampler_id) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_SAMPLER_ID_MASK) | (sampler_id << SQ_TEX_WORD2_SAMPLER_ID_SHIFT)
#define SQ_TEX_WORD2_SET_SRC_SEL_X(sq_tex_word2_reg, src_sel_x) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_SRC_SEL_X_MASK) | (src_sel_x << SQ_TEX_WORD2_SRC_SEL_X_SHIFT)
#define SQ_TEX_WORD2_SET_SRC_SEL_Y(sq_tex_word2_reg, src_sel_y) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_SRC_SEL_Y_MASK) | (src_sel_y << SQ_TEX_WORD2_SRC_SEL_Y_SHIFT)
#define SQ_TEX_WORD2_SET_SRC_SEL_Z(sq_tex_word2_reg, src_sel_z) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_SRC_SEL_Z_MASK) | (src_sel_z << SQ_TEX_WORD2_SRC_SEL_Z_SHIFT)
#define SQ_TEX_WORD2_SET_SRC_SEL_W(sq_tex_word2_reg, src_sel_w) \
     sq_tex_word2_reg = (sq_tex_word2_reg & ~SQ_TEX_WORD2_SRC_SEL_W_MASK) | (src_sel_w << SQ_TEX_WORD2_SRC_SEL_W_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_tex_word2_t {
          unsigned int offset_x                       : SQ_TEX_WORD2_OFFSET_X_SIZE;
          unsigned int offset_y                       : SQ_TEX_WORD2_OFFSET_Y_SIZE;
          unsigned int offset_z                       : SQ_TEX_WORD2_OFFSET_Z_SIZE;
          unsigned int sampler_id                     : SQ_TEX_WORD2_SAMPLER_ID_SIZE;
          unsigned int src_sel_x                      : SQ_TEX_WORD2_SRC_SEL_X_SIZE;
          unsigned int src_sel_y                      : SQ_TEX_WORD2_SRC_SEL_Y_SIZE;
          unsigned int src_sel_z                      : SQ_TEX_WORD2_SRC_SEL_Z_SIZE;
          unsigned int src_sel_w                      : SQ_TEX_WORD2_SRC_SEL_W_SIZE;
     } sq_tex_word2_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_tex_word2_t {
          unsigned int src_sel_w                      : SQ_TEX_WORD2_SRC_SEL_W_SIZE;
          unsigned int src_sel_z                      : SQ_TEX_WORD2_SRC_SEL_Z_SIZE;
          unsigned int src_sel_y                      : SQ_TEX_WORD2_SRC_SEL_Y_SIZE;
          unsigned int src_sel_x                      : SQ_TEX_WORD2_SRC_SEL_X_SIZE;
          unsigned int sampler_id                     : SQ_TEX_WORD2_SAMPLER_ID_SIZE;
          unsigned int offset_z                       : SQ_TEX_WORD2_OFFSET_Z_SIZE;
          unsigned int offset_y                       : SQ_TEX_WORD2_OFFSET_Y_SIZE;
          unsigned int offset_x                       : SQ_TEX_WORD2_OFFSET_X_SIZE;
     } sq_tex_word2_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_tex_word2_t f;
} sq_tex_word2_u;


/*
 * SQ_VTX_WORD0 struct
 */

#define SQ_VTX_WORD0_VTX_INST_SIZE     5
#define SQ_VTX_WORD0_FETCH_TYPE_SIZE   2
#define SQ_VTX_WORD0_FETCH_WHOLE_QUAD_SIZE 1
#define SQ_VTX_WORD0_BUFFER_ID_SIZE    8
#define SQ_VTX_WORD0_SRC_GPR_SIZE      7
#define SQ_VTX_WORD0_SRC_REL_SIZE      1
#define SQ_VTX_WORD0_SRC_SEL_X_SIZE    2
#define SQ_VTX_WORD0_MEGA_FETCH_COUNT_SIZE 6

#define SQ_VTX_WORD0_VTX_INST_SHIFT    0
#define SQ_VTX_WORD0_FETCH_TYPE_SHIFT  5
#define SQ_VTX_WORD0_FETCH_WHOLE_QUAD_SHIFT 7
#define SQ_VTX_WORD0_BUFFER_ID_SHIFT   8
#define SQ_VTX_WORD0_SRC_GPR_SHIFT     16
#define SQ_VTX_WORD0_SRC_REL_SHIFT     23
#define SQ_VTX_WORD0_SRC_SEL_X_SHIFT   24
#define SQ_VTX_WORD0_MEGA_FETCH_COUNT_SHIFT 26

#define SQ_VTX_WORD0_VTX_INST_MASK     0x0000001f
#define SQ_VTX_WORD0_FETCH_TYPE_MASK   0x00000060
#define SQ_VTX_WORD0_FETCH_WHOLE_QUAD_MASK 0x00000080
#define SQ_VTX_WORD0_BUFFER_ID_MASK    0x0000ff00
#define SQ_VTX_WORD0_SRC_GPR_MASK      0x007f0000
#define SQ_VTX_WORD0_SRC_REL_MASK      0x00800000
#define SQ_VTX_WORD0_SRC_SEL_X_MASK    0x03000000
#define SQ_VTX_WORD0_MEGA_FETCH_COUNT_MASK 0xfc000000

#define SQ_VTX_WORD0_MASK \
     (SQ_VTX_WORD0_VTX_INST_MASK | \
      SQ_VTX_WORD0_FETCH_TYPE_MASK | \
      SQ_VTX_WORD0_FETCH_WHOLE_QUAD_MASK | \
      SQ_VTX_WORD0_BUFFER_ID_MASK | \
      SQ_VTX_WORD0_SRC_GPR_MASK | \
      SQ_VTX_WORD0_SRC_REL_MASK | \
      SQ_VTX_WORD0_SRC_SEL_X_MASK | \
      SQ_VTX_WORD0_MEGA_FETCH_COUNT_MASK)

#define SQ_VTX_WORD0_DEFAULT           0xcdcdcdcd

#define SQ_VTX_WORD0_GET_VTX_INST(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_VTX_INST_MASK) >> SQ_VTX_WORD0_VTX_INST_SHIFT)
#define SQ_VTX_WORD0_GET_FETCH_TYPE(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_FETCH_TYPE_MASK) >> SQ_VTX_WORD0_FETCH_TYPE_SHIFT)
#define SQ_VTX_WORD0_GET_FETCH_WHOLE_QUAD(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_FETCH_WHOLE_QUAD_MASK) >> SQ_VTX_WORD0_FETCH_WHOLE_QUAD_SHIFT)
#define SQ_VTX_WORD0_GET_BUFFER_ID(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_BUFFER_ID_MASK) >> SQ_VTX_WORD0_BUFFER_ID_SHIFT)
#define SQ_VTX_WORD0_GET_SRC_GPR(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_SRC_GPR_MASK) >> SQ_VTX_WORD0_SRC_GPR_SHIFT)
#define SQ_VTX_WORD0_GET_SRC_REL(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_SRC_REL_MASK) >> SQ_VTX_WORD0_SRC_REL_SHIFT)
#define SQ_VTX_WORD0_GET_SRC_SEL_X(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_SRC_SEL_X_MASK) >> SQ_VTX_WORD0_SRC_SEL_X_SHIFT)
#define SQ_VTX_WORD0_GET_MEGA_FETCH_COUNT(sq_vtx_word0) \
     ((sq_vtx_word0 & SQ_VTX_WORD0_MEGA_FETCH_COUNT_MASK) >> SQ_VTX_WORD0_MEGA_FETCH_COUNT_SHIFT)

#define SQ_VTX_WORD0_SET_VTX_INST(sq_vtx_word0_reg, vtx_inst) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_VTX_INST_MASK) | (vtx_inst << SQ_VTX_WORD0_VTX_INST_SHIFT)
#define SQ_VTX_WORD0_SET_FETCH_TYPE(sq_vtx_word0_reg, fetch_type) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_FETCH_TYPE_MASK) | (fetch_type << SQ_VTX_WORD0_FETCH_TYPE_SHIFT)
#define SQ_VTX_WORD0_SET_FETCH_WHOLE_QUAD(sq_vtx_word0_reg, fetch_whole_quad) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_FETCH_WHOLE_QUAD_MASK) | (fetch_whole_quad << SQ_VTX_WORD0_FETCH_WHOLE_QUAD_SHIFT)
#define SQ_VTX_WORD0_SET_BUFFER_ID(sq_vtx_word0_reg, buffer_id) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_BUFFER_ID_MASK) | (buffer_id << SQ_VTX_WORD0_BUFFER_ID_SHIFT)
#define SQ_VTX_WORD0_SET_SRC_GPR(sq_vtx_word0_reg, src_gpr) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_SRC_GPR_MASK) | (src_gpr << SQ_VTX_WORD0_SRC_GPR_SHIFT)
#define SQ_VTX_WORD0_SET_SRC_REL(sq_vtx_word0_reg, src_rel) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_SRC_REL_MASK) | (src_rel << SQ_VTX_WORD0_SRC_REL_SHIFT)
#define SQ_VTX_WORD0_SET_SRC_SEL_X(sq_vtx_word0_reg, src_sel_x) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_SRC_SEL_X_MASK) | (src_sel_x << SQ_VTX_WORD0_SRC_SEL_X_SHIFT)
#define SQ_VTX_WORD0_SET_MEGA_FETCH_COUNT(sq_vtx_word0_reg, mega_fetch_count) \
     sq_vtx_word0_reg = (sq_vtx_word0_reg & ~SQ_VTX_WORD0_MEGA_FETCH_COUNT_MASK) | (mega_fetch_count << SQ_VTX_WORD0_MEGA_FETCH_COUNT_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_vtx_word0_t {
          unsigned int vtx_inst                       : SQ_VTX_WORD0_VTX_INST_SIZE;
          unsigned int fetch_type                     : SQ_VTX_WORD0_FETCH_TYPE_SIZE;
          unsigned int fetch_whole_quad               : SQ_VTX_WORD0_FETCH_WHOLE_QUAD_SIZE;
          unsigned int buffer_id                      : SQ_VTX_WORD0_BUFFER_ID_SIZE;
          unsigned int src_gpr                        : SQ_VTX_WORD0_SRC_GPR_SIZE;
          unsigned int src_rel                        : SQ_VTX_WORD0_SRC_REL_SIZE;
          unsigned int src_sel_x                      : SQ_VTX_WORD0_SRC_SEL_X_SIZE;
          unsigned int mega_fetch_count               : SQ_VTX_WORD0_MEGA_FETCH_COUNT_SIZE;
     } sq_vtx_word0_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_vtx_word0_t {
          unsigned int mega_fetch_count               : SQ_VTX_WORD0_MEGA_FETCH_COUNT_SIZE;
          unsigned int src_sel_x                      : SQ_VTX_WORD0_SRC_SEL_X_SIZE;
          unsigned int src_rel                        : SQ_VTX_WORD0_SRC_REL_SIZE;
          unsigned int src_gpr                        : SQ_VTX_WORD0_SRC_GPR_SIZE;
          unsigned int buffer_id                      : SQ_VTX_WORD0_BUFFER_ID_SIZE;
          unsigned int fetch_whole_quad               : SQ_VTX_WORD0_FETCH_WHOLE_QUAD_SIZE;
          unsigned int fetch_type                     : SQ_VTX_WORD0_FETCH_TYPE_SIZE;
          unsigned int vtx_inst                       : SQ_VTX_WORD0_VTX_INST_SIZE;
     } sq_vtx_word0_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_vtx_word0_t f;
} sq_vtx_word0_u;


/*
 * SQ_VTX_WORD1 struct
 */

#define SQ_VTX_WORD1_DST_SEL_X_SIZE    3
#define SQ_VTX_WORD1_DST_SEL_Y_SIZE    3
#define SQ_VTX_WORD1_DST_SEL_Z_SIZE    3
#define SQ_VTX_WORD1_DST_SEL_W_SIZE    3
#define SQ_VTX_WORD1_USE_CONST_FIELDS_SIZE 1
#define SQ_VTX_WORD1_DATA_FORMAT_SIZE  6
#define SQ_VTX_WORD1_NUM_FORMAT_ALL_SIZE 2
#define SQ_VTX_WORD1_FORMAT_COMP_ALL_SIZE 1
#define SQ_VTX_WORD1_SRF_MODE_ALL_SIZE 1

#define SQ_VTX_WORD1_DST_SEL_X_SHIFT   9
#define SQ_VTX_WORD1_DST_SEL_Y_SHIFT   12
#define SQ_VTX_WORD1_DST_SEL_Z_SHIFT   15
#define SQ_VTX_WORD1_DST_SEL_W_SHIFT   18
#define SQ_VTX_WORD1_USE_CONST_FIELDS_SHIFT 21
#define SQ_VTX_WORD1_DATA_FORMAT_SHIFT 22
#define SQ_VTX_WORD1_NUM_FORMAT_ALL_SHIFT 28
#define SQ_VTX_WORD1_FORMAT_COMP_ALL_SHIFT 30
#define SQ_VTX_WORD1_SRF_MODE_ALL_SHIFT 31

#define SQ_VTX_WORD1_DST_SEL_X_MASK    0x00000e00
#define SQ_VTX_WORD1_DST_SEL_Y_MASK    0x00007000
#define SQ_VTX_WORD1_DST_SEL_Z_MASK    0x00038000
#define SQ_VTX_WORD1_DST_SEL_W_MASK    0x001c0000
#define SQ_VTX_WORD1_USE_CONST_FIELDS_MASK 0x00200000
#define SQ_VTX_WORD1_DATA_FORMAT_MASK  0x0fc00000
#define SQ_VTX_WORD1_NUM_FORMAT_ALL_MASK 0x30000000
#define SQ_VTX_WORD1_FORMAT_COMP_ALL_MASK 0x40000000
#define SQ_VTX_WORD1_SRF_MODE_ALL_MASK 0x80000000

#define SQ_VTX_WORD1_MASK \
     (SQ_VTX_WORD1_DST_SEL_X_MASK | \
      SQ_VTX_WORD1_DST_SEL_Y_MASK | \
      SQ_VTX_WORD1_DST_SEL_Z_MASK | \
      SQ_VTX_WORD1_DST_SEL_W_MASK | \
      SQ_VTX_WORD1_USE_CONST_FIELDS_MASK | \
      SQ_VTX_WORD1_DATA_FORMAT_MASK | \
      SQ_VTX_WORD1_NUM_FORMAT_ALL_MASK | \
      SQ_VTX_WORD1_FORMAT_COMP_ALL_MASK | \
      SQ_VTX_WORD1_SRF_MODE_ALL_MASK)

#define SQ_VTX_WORD1_DEFAULT           0xcdcdcc00

#define SQ_VTX_WORD1_GET_DST_SEL_X(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_DST_SEL_X_MASK) >> SQ_VTX_WORD1_DST_SEL_X_SHIFT)
#define SQ_VTX_WORD1_GET_DST_SEL_Y(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_DST_SEL_Y_MASK) >> SQ_VTX_WORD1_DST_SEL_Y_SHIFT)
#define SQ_VTX_WORD1_GET_DST_SEL_Z(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_DST_SEL_Z_MASK) >> SQ_VTX_WORD1_DST_SEL_Z_SHIFT)
#define SQ_VTX_WORD1_GET_DST_SEL_W(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_DST_SEL_W_MASK) >> SQ_VTX_WORD1_DST_SEL_W_SHIFT)
#define SQ_VTX_WORD1_GET_USE_CONST_FIELDS(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_USE_CONST_FIELDS_MASK) >> SQ_VTX_WORD1_USE_CONST_FIELDS_SHIFT)
#define SQ_VTX_WORD1_GET_DATA_FORMAT(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_DATA_FORMAT_MASK) >> SQ_VTX_WORD1_DATA_FORMAT_SHIFT)
#define SQ_VTX_WORD1_GET_NUM_FORMAT_ALL(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_NUM_FORMAT_ALL_MASK) >> SQ_VTX_WORD1_NUM_FORMAT_ALL_SHIFT)
#define SQ_VTX_WORD1_GET_FORMAT_COMP_ALL(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_FORMAT_COMP_ALL_MASK) >> SQ_VTX_WORD1_FORMAT_COMP_ALL_SHIFT)
#define SQ_VTX_WORD1_GET_SRF_MODE_ALL(sq_vtx_word1) \
     ((sq_vtx_word1 & SQ_VTX_WORD1_SRF_MODE_ALL_MASK) >> SQ_VTX_WORD1_SRF_MODE_ALL_SHIFT)

#define SQ_VTX_WORD1_SET_DST_SEL_X(sq_vtx_word1_reg, dst_sel_x) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_DST_SEL_X_MASK) | (dst_sel_x << SQ_VTX_WORD1_DST_SEL_X_SHIFT)
#define SQ_VTX_WORD1_SET_DST_SEL_Y(sq_vtx_word1_reg, dst_sel_y) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_DST_SEL_Y_MASK) | (dst_sel_y << SQ_VTX_WORD1_DST_SEL_Y_SHIFT)
#define SQ_VTX_WORD1_SET_DST_SEL_Z(sq_vtx_word1_reg, dst_sel_z) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_DST_SEL_Z_MASK) | (dst_sel_z << SQ_VTX_WORD1_DST_SEL_Z_SHIFT)
#define SQ_VTX_WORD1_SET_DST_SEL_W(sq_vtx_word1_reg, dst_sel_w) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_DST_SEL_W_MASK) | (dst_sel_w << SQ_VTX_WORD1_DST_SEL_W_SHIFT)
#define SQ_VTX_WORD1_SET_USE_CONST_FIELDS(sq_vtx_word1_reg, use_const_fields) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_USE_CONST_FIELDS_MASK) | (use_const_fields << SQ_VTX_WORD1_USE_CONST_FIELDS_SHIFT)
#define SQ_VTX_WORD1_SET_DATA_FORMAT(sq_vtx_word1_reg, data_format) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_DATA_FORMAT_MASK) | (data_format << SQ_VTX_WORD1_DATA_FORMAT_SHIFT)
#define SQ_VTX_WORD1_SET_NUM_FORMAT_ALL(sq_vtx_word1_reg, num_format_all) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_NUM_FORMAT_ALL_MASK) | (num_format_all << SQ_VTX_WORD1_NUM_FORMAT_ALL_SHIFT)
#define SQ_VTX_WORD1_SET_FORMAT_COMP_ALL(sq_vtx_word1_reg, format_comp_all) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_FORMAT_COMP_ALL_MASK) | (format_comp_all << SQ_VTX_WORD1_FORMAT_COMP_ALL_SHIFT)
#define SQ_VTX_WORD1_SET_SRF_MODE_ALL(sq_vtx_word1_reg, srf_mode_all) \
     sq_vtx_word1_reg = (sq_vtx_word1_reg & ~SQ_VTX_WORD1_SRF_MODE_ALL_MASK) | (srf_mode_all << SQ_VTX_WORD1_SRF_MODE_ALL_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_vtx_word1_t {
          unsigned int                                : 9;
          unsigned int dst_sel_x                      : SQ_VTX_WORD1_DST_SEL_X_SIZE;
          unsigned int dst_sel_y                      : SQ_VTX_WORD1_DST_SEL_Y_SIZE;
          unsigned int dst_sel_z                      : SQ_VTX_WORD1_DST_SEL_Z_SIZE;
          unsigned int dst_sel_w                      : SQ_VTX_WORD1_DST_SEL_W_SIZE;
          unsigned int use_const_fields               : SQ_VTX_WORD1_USE_CONST_FIELDS_SIZE;
          unsigned int data_format                    : SQ_VTX_WORD1_DATA_FORMAT_SIZE;
          unsigned int num_format_all                 : SQ_VTX_WORD1_NUM_FORMAT_ALL_SIZE;
          unsigned int format_comp_all                : SQ_VTX_WORD1_FORMAT_COMP_ALL_SIZE;
          unsigned int srf_mode_all                   : SQ_VTX_WORD1_SRF_MODE_ALL_SIZE;
     } sq_vtx_word1_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_vtx_word1_t {
          unsigned int srf_mode_all                   : SQ_VTX_WORD1_SRF_MODE_ALL_SIZE;
          unsigned int format_comp_all                : SQ_VTX_WORD1_FORMAT_COMP_ALL_SIZE;
          unsigned int num_format_all                 : SQ_VTX_WORD1_NUM_FORMAT_ALL_SIZE;
          unsigned int data_format                    : SQ_VTX_WORD1_DATA_FORMAT_SIZE;
          unsigned int use_const_fields               : SQ_VTX_WORD1_USE_CONST_FIELDS_SIZE;
          unsigned int dst_sel_w                      : SQ_VTX_WORD1_DST_SEL_W_SIZE;
          unsigned int dst_sel_z                      : SQ_VTX_WORD1_DST_SEL_Z_SIZE;
          unsigned int dst_sel_y                      : SQ_VTX_WORD1_DST_SEL_Y_SIZE;
          unsigned int dst_sel_x                      : SQ_VTX_WORD1_DST_SEL_X_SIZE;
          unsigned int                                : 9;
     } sq_vtx_word1_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_vtx_word1_t f;
} sq_vtx_word1_u;


/*
 * SQ_VTX_WORD1_GPR struct
 */

#define SQ_VTX_WORD1_GPR_DST_GPR_SIZE  7
#define SQ_VTX_WORD1_GPR_DST_REL_SIZE  1

#define SQ_VTX_WORD1_GPR_DST_GPR_SHIFT 0
#define SQ_VTX_WORD1_GPR_DST_REL_SHIFT 7

#define SQ_VTX_WORD1_GPR_DST_GPR_MASK  0x0000007f
#define SQ_VTX_WORD1_GPR_DST_REL_MASK  0x00000080

#define SQ_VTX_WORD1_GPR_MASK \
     (SQ_VTX_WORD1_GPR_DST_GPR_MASK | \
      SQ_VTX_WORD1_GPR_DST_REL_MASK)

#define SQ_VTX_WORD1_GPR_DEFAULT       0x000000cd

#define SQ_VTX_WORD1_GPR_GET_DST_GPR(sq_vtx_word1_gpr) \
     ((sq_vtx_word1_gpr & SQ_VTX_WORD1_GPR_DST_GPR_MASK) >> SQ_VTX_WORD1_GPR_DST_GPR_SHIFT)
#define SQ_VTX_WORD1_GPR_GET_DST_REL(sq_vtx_word1_gpr) \
     ((sq_vtx_word1_gpr & SQ_VTX_WORD1_GPR_DST_REL_MASK) >> SQ_VTX_WORD1_GPR_DST_REL_SHIFT)

#define SQ_VTX_WORD1_GPR_SET_DST_GPR(sq_vtx_word1_gpr_reg, dst_gpr) \
     sq_vtx_word1_gpr_reg = (sq_vtx_word1_gpr_reg & ~SQ_VTX_WORD1_GPR_DST_GPR_MASK) | (dst_gpr << SQ_VTX_WORD1_GPR_DST_GPR_SHIFT)
#define SQ_VTX_WORD1_GPR_SET_DST_REL(sq_vtx_word1_gpr_reg, dst_rel) \
     sq_vtx_word1_gpr_reg = (sq_vtx_word1_gpr_reg & ~SQ_VTX_WORD1_GPR_DST_REL_MASK) | (dst_rel << SQ_VTX_WORD1_GPR_DST_REL_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_vtx_word1_gpr_t {
          unsigned int dst_gpr                        : SQ_VTX_WORD1_GPR_DST_GPR_SIZE;
          unsigned int dst_rel                        : SQ_VTX_WORD1_GPR_DST_REL_SIZE;
          unsigned int                                : 24;
     } sq_vtx_word1_gpr_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_vtx_word1_gpr_t {
          unsigned int                                : 24;
          unsigned int dst_rel                        : SQ_VTX_WORD1_GPR_DST_REL_SIZE;
          unsigned int dst_gpr                        : SQ_VTX_WORD1_GPR_DST_GPR_SIZE;
     } sq_vtx_word1_gpr_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_vtx_word1_gpr_t f;
} sq_vtx_word1_gpr_u;


/*
 * SQ_VTX_WORD1_SEM struct
 */

#define SQ_VTX_WORD1_SEM_SEMANTIC_ID_SIZE 8

#define SQ_VTX_WORD1_SEM_SEMANTIC_ID_SHIFT 0

#define SQ_VTX_WORD1_SEM_SEMANTIC_ID_MASK 0x000000ff

#define SQ_VTX_WORD1_SEM_MASK \
     (SQ_VTX_WORD1_SEM_SEMANTIC_ID_MASK)

#define SQ_VTX_WORD1_SEM_DEFAULT       0x000000cd

#define SQ_VTX_WORD1_SEM_GET_SEMANTIC_ID(sq_vtx_word1_sem) \
     ((sq_vtx_word1_sem & SQ_VTX_WORD1_SEM_SEMANTIC_ID_MASK) >> SQ_VTX_WORD1_SEM_SEMANTIC_ID_SHIFT)

#define SQ_VTX_WORD1_SEM_SET_SEMANTIC_ID(sq_vtx_word1_sem_reg, semantic_id) \
     sq_vtx_word1_sem_reg = (sq_vtx_word1_sem_reg & ~SQ_VTX_WORD1_SEM_SEMANTIC_ID_MASK) | (semantic_id << SQ_VTX_WORD1_SEM_SEMANTIC_ID_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_vtx_word1_sem_t {
          unsigned int semantic_id                    : SQ_VTX_WORD1_SEM_SEMANTIC_ID_SIZE;
          unsigned int                                : 24;
     } sq_vtx_word1_sem_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_vtx_word1_sem_t {
          unsigned int                                : 24;
          unsigned int semantic_id                    : SQ_VTX_WORD1_SEM_SEMANTIC_ID_SIZE;
     } sq_vtx_word1_sem_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_vtx_word1_sem_t f;
} sq_vtx_word1_sem_u;


/*
 * SQ_VTX_WORD2 struct
 */

#define SQ_VTX_WORD2_OFFSET_SIZE       16
#define SQ_VTX_WORD2_ENDIAN_SWAP_SIZE  2
#define SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_SIZE 1
#define SQ_VTX_WORD2_MEGA_FETCH_SIZE   1
#define SQ_VTX_WORD2_ALT_CONST_SIZE    1

#define SQ_VTX_WORD2_OFFSET_SHIFT      0
#define SQ_VTX_WORD2_ENDIAN_SWAP_SHIFT 16
#define SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_SHIFT 18
#define SQ_VTX_WORD2_MEGA_FETCH_SHIFT  19
#define SQ_VTX_WORD2_ALT_CONST_SHIFT   20

#define SQ_VTX_WORD2_OFFSET_MASK       0x0000ffff
#define SQ_VTX_WORD2_ENDIAN_SWAP_MASK  0x00030000
#define SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_MASK 0x00040000
#define SQ_VTX_WORD2_MEGA_FETCH_MASK   0x00080000
#define SQ_VTX_WORD2_ALT_CONST_MASK    0x00100000

#define SQ_VTX_WORD2_MASK \
     (SQ_VTX_WORD2_OFFSET_MASK | \
      SQ_VTX_WORD2_ENDIAN_SWAP_MASK | \
      SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_MASK | \
      SQ_VTX_WORD2_MEGA_FETCH_MASK | \
      SQ_VTX_WORD2_ALT_CONST_MASK)

#define SQ_VTX_WORD2_DEFAULT           0x000dcdcd

#define SQ_VTX_WORD2_GET_OFFSET(sq_vtx_word2) \
     ((sq_vtx_word2 & SQ_VTX_WORD2_OFFSET_MASK) >> SQ_VTX_WORD2_OFFSET_SHIFT)
#define SQ_VTX_WORD2_GET_ENDIAN_SWAP(sq_vtx_word2) \
     ((sq_vtx_word2 & SQ_VTX_WORD2_ENDIAN_SWAP_MASK) >> SQ_VTX_WORD2_ENDIAN_SWAP_SHIFT)
#define SQ_VTX_WORD2_GET_CONST_BUF_NO_STRIDE(sq_vtx_word2) \
     ((sq_vtx_word2 & SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_MASK) >> SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_SHIFT)
#define SQ_VTX_WORD2_GET_MEGA_FETCH(sq_vtx_word2) \
     ((sq_vtx_word2 & SQ_VTX_WORD2_MEGA_FETCH_MASK) >> SQ_VTX_WORD2_MEGA_FETCH_SHIFT)
#define SQ_VTX_WORD2_GET_ALT_CONST(sq_vtx_word2) \
     ((sq_vtx_word2 & SQ_VTX_WORD2_ALT_CONST_MASK) >> SQ_VTX_WORD2_ALT_CONST_SHIFT)

#define SQ_VTX_WORD2_SET_OFFSET(sq_vtx_word2_reg, offset) \
     sq_vtx_word2_reg = (sq_vtx_word2_reg & ~SQ_VTX_WORD2_OFFSET_MASK) | (offset << SQ_VTX_WORD2_OFFSET_SHIFT)
#define SQ_VTX_WORD2_SET_ENDIAN_SWAP(sq_vtx_word2_reg, endian_swap) \
     sq_vtx_word2_reg = (sq_vtx_word2_reg & ~SQ_VTX_WORD2_ENDIAN_SWAP_MASK) | (endian_swap << SQ_VTX_WORD2_ENDIAN_SWAP_SHIFT)
#define SQ_VTX_WORD2_SET_CONST_BUF_NO_STRIDE(sq_vtx_word2_reg, const_buf_no_stride) \
     sq_vtx_word2_reg = (sq_vtx_word2_reg & ~SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_MASK) | (const_buf_no_stride << SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_SHIFT)
#define SQ_VTX_WORD2_SET_MEGA_FETCH(sq_vtx_word2_reg, mega_fetch) \
     sq_vtx_word2_reg = (sq_vtx_word2_reg & ~SQ_VTX_WORD2_MEGA_FETCH_MASK) | (mega_fetch << SQ_VTX_WORD2_MEGA_FETCH_SHIFT)
#define SQ_VTX_WORD2_SET_ALT_CONST(sq_vtx_word2_reg, alt_const) \
     sq_vtx_word2_reg = (sq_vtx_word2_reg & ~SQ_VTX_WORD2_ALT_CONST_MASK) | (alt_const << SQ_VTX_WORD2_ALT_CONST_SHIFT)

#if		defined(LITTLEENDIAN_CPU)

     typedef struct _sq_vtx_word2_t {
          unsigned int offset                         : SQ_VTX_WORD2_OFFSET_SIZE;
          unsigned int endian_swap                    : SQ_VTX_WORD2_ENDIAN_SWAP_SIZE;
          unsigned int const_buf_no_stride            : SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_SIZE;
          unsigned int mega_fetch                     : SQ_VTX_WORD2_MEGA_FETCH_SIZE;
          unsigned int alt_const                      : SQ_VTX_WORD2_ALT_CONST_SIZE;
          unsigned int                                : 11;
     } sq_vtx_word2_t;

#elif		defined(BIGENDIAN_CPU)

     typedef struct _sq_vtx_word2_t {
          unsigned int                                : 11;
          unsigned int alt_const                      : SQ_VTX_WORD2_ALT_CONST_SIZE;
          unsigned int mega_fetch                     : SQ_VTX_WORD2_MEGA_FETCH_SIZE;
          unsigned int const_buf_no_stride            : SQ_VTX_WORD2_CONST_BUF_NO_STRIDE_SIZE;
          unsigned int endian_swap                    : SQ_VTX_WORD2_ENDIAN_SWAP_SIZE;
          unsigned int offset                         : SQ_VTX_WORD2_OFFSET_SIZE;
     } sq_vtx_word2_t;

#endif

typedef union {
     unsigned int val : 32;
     sq_vtx_word2_t f;
} sq_vtx_word2_u;

#endif /* _SQ_MICRO_REG_H */


