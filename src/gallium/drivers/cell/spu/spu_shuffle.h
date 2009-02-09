#ifndef SPU_SHUFFLE_H
#define SPU_SHUFFLE_H

/*
 * Generate shuffle patterns with minimal fuss.
 *
 * Based on ideas from 
 * http://www.insomniacgames.com/tech/articles/0408/files/shuffles.pdf
 *
 * A-P indicates 0-15th position in first vector
 * a-p indicates 0-15th position in second vector
 *
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |00|01|02|03|04|05|06|07|08|09|0a|0b|0c|0d|0e|0f|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * |          A|          B|          C|          D|
 * +-----+-----+-----+-----+-----+-----+-----+-----+
 * |    A|    B|    C|    D|    E|    F|    G|    H|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 * | A| B| C| D| E| F| G| H| I| J| K| L| M| N| O| P|
 * +--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+--+
 *
 * x or X indicates 0xff
 * 8 indicates 0x80
 * 0 indicates 0x00
 *
 * The macros SHUFFLE4() SHUFFLE8() and SHUFFLE16() provide a const vector 
 * unsigned char literal suitable for use with spu_shuffle().
 *
 * The macros SHUFB4() SHUFB8() and SHUFB16() provide a const qword vector 
 * literal suitable for use with si_shufb().
 *
 *
 * For example :
 * SHUFB4(A,A,A,A)
 * expands to :
 * ((const qword){0,1,2,3,0,1,2,3,0,1,2,3,0,1,2,3})
 * 
 * SHUFFLE8(A,B,a,b,C,c,8,8)
 * expands to :
 * ((const vector unsigned char){0x00,0x01,0x02,0x03,0x10,0x11,0x12,0x13,
 *				 0x04,0x05,0x14,0x15,0xe0,0xe0,0xe0,0xe0})
 *
 */

#include <spu_intrinsics.h>

#define SHUFFLE_PATTERN_4_A__  0x00, 0x01, 0x02, 0x03
#define SHUFFLE_PATTERN_4_B__  0x04, 0x05, 0x06, 0x07
#define SHUFFLE_PATTERN_4_C__  0x08, 0x09, 0x0a, 0x0b
#define SHUFFLE_PATTERN_4_D__  0x0c, 0x0d, 0x0e, 0x0f
#define SHUFFLE_PATTERN_4_a__  0x10, 0x11, 0x12, 0x13
#define SHUFFLE_PATTERN_4_b__  0x14, 0x15, 0x16, 0x17
#define SHUFFLE_PATTERN_4_c__  0x18, 0x19, 0x1a, 0x1b
#define SHUFFLE_PATTERN_4_d__  0x1c, 0x1d, 0x1e, 0x1f
#define SHUFFLE_PATTERN_4_X__  0xc0, 0xc0, 0xc0, 0xc0
#define SHUFFLE_PATTERN_4_x__  0xc0, 0xc0, 0xc0, 0xc0
#define SHUFFLE_PATTERN_4_0__  0x80, 0x80, 0x80, 0x80
#define SHUFFLE_PATTERN_4_8__  0xe0, 0xe0, 0xe0, 0xe0

#define SHUFFLE_VECTOR_4__(A, B, C, D) \
   SHUFFLE_PATTERN_4_##A##__, \
   SHUFFLE_PATTERN_4_##B##__, \
   SHUFFLE_PATTERN_4_##C##__, \
   SHUFFLE_PATTERN_4_##D##__

#define SHUFFLE4(A, B, C, D) \
   ((const vector unsigned char){ \
      SHUFFLE_VECTOR_4__(A, B, C, D) \
   })

#define SHUFB4(A, B, C, D) \
   ((const qword){ \
      SHUFFLE_VECTOR_4__(A, B, C, D) \
   })


#define SHUFFLE_PATTERN_8_A__  0x00, 0x01
#define SHUFFLE_PATTERN_8_B__  0x02, 0x03
#define SHUFFLE_PATTERN_8_C__  0x04, 0x05
#define SHUFFLE_PATTERN_8_D__  0x06, 0x07
#define SHUFFLE_PATTERN_8_E__  0x08, 0x09
#define SHUFFLE_PATTERN_8_F__  0x0a, 0x0b
#define SHUFFLE_PATTERN_8_G__  0x0c, 0x0d
#define SHUFFLE_PATTERN_8_H__  0x0e, 0x0f
#define SHUFFLE_PATTERN_8_a__  0x10, 0x11
#define SHUFFLE_PATTERN_8_b__  0x12, 0x13
#define SHUFFLE_PATTERN_8_c__  0x14, 0x15
#define SHUFFLE_PATTERN_8_d__  0x16, 0x17
#define SHUFFLE_PATTERN_8_e__  0x18, 0x19
#define SHUFFLE_PATTERN_8_f__  0x1a, 0x1b
#define SHUFFLE_PATTERN_8_g__  0x1c, 0x1d
#define SHUFFLE_PATTERN_8_h__  0x1e, 0x1f
#define SHUFFLE_PATTERN_8_X__  0xc0, 0xc0
#define SHUFFLE_PATTERN_8_x__  0xc0, 0xc0
#define SHUFFLE_PATTERN_8_0__  0x80, 0x80
#define SHUFFLE_PATTERN_8_8__  0xe0, 0xe0


#define SHUFFLE_VECTOR_8__(A, B, C, D, E, F, G, H) \
   SHUFFLE_PATTERN_8_##A##__, \
   SHUFFLE_PATTERN_8_##B##__, \
   SHUFFLE_PATTERN_8_##C##__, \
   SHUFFLE_PATTERN_8_##D##__, \
   SHUFFLE_PATTERN_8_##E##__, \
   SHUFFLE_PATTERN_8_##F##__, \
   SHUFFLE_PATTERN_8_##G##__, \
   SHUFFLE_PATTERN_8_##H##__

#define SHUFFLE8(A, B, C, D, E, F, G, H) \
   ((const vector unsigned char){ \
      SHUFFLE_VECTOR_8__(A, B, C, D, E, F, G, H) \
   })

#define SHUFB8(A, B, C, D, E, F, G, H) \
   ((const qword){ \
      SHUFFLE_VECTOR_8__(A, B, C, D, E, F, G, H) \
   })


#define SHUFFLE_PATTERN_16_A__  0x00
#define SHUFFLE_PATTERN_16_B__  0x01
#define SHUFFLE_PATTERN_16_C__  0x02
#define SHUFFLE_PATTERN_16_D__  0x03
#define SHUFFLE_PATTERN_16_E__  0x04
#define SHUFFLE_PATTERN_16_F__  0x05
#define SHUFFLE_PATTERN_16_G__  0x06
#define SHUFFLE_PATTERN_16_H__  0x07
#define SHUFFLE_PATTERN_16_I__  0x08
#define SHUFFLE_PATTERN_16_J__  0x09
#define SHUFFLE_PATTERN_16_K__  0x0a
#define SHUFFLE_PATTERN_16_L__  0x0b
#define SHUFFLE_PATTERN_16_M__  0x0c
#define SHUFFLE_PATTERN_16_N__  0x0d
#define SHUFFLE_PATTERN_16_O__  0x0e
#define SHUFFLE_PATTERN_16_P__  0x0f
#define SHUFFLE_PATTERN_16_a__  0x10
#define SHUFFLE_PATTERN_16_b__  0x11
#define SHUFFLE_PATTERN_16_c__  0x12
#define SHUFFLE_PATTERN_16_d__  0x13
#define SHUFFLE_PATTERN_16_e__  0x14
#define SHUFFLE_PATTERN_16_f__  0x15
#define SHUFFLE_PATTERN_16_g__  0x16
#define SHUFFLE_PATTERN_16_h__  0x17
#define SHUFFLE_PATTERN_16_i__  0x18
#define SHUFFLE_PATTERN_16_j__  0x19
#define SHUFFLE_PATTERN_16_k__  0x1a
#define SHUFFLE_PATTERN_16_l__  0x1b
#define SHUFFLE_PATTERN_16_m__  0x1c
#define SHUFFLE_PATTERN_16_n__  0x1d
#define SHUFFLE_PATTERN_16_o__  0x1e
#define SHUFFLE_PATTERN_16_p__  0x1f
#define SHUFFLE_PATTERN_16_X__  0xc0
#define SHUFFLE_PATTERN_16_x__  0xc0
#define SHUFFLE_PATTERN_16_0__  0x80
#define SHUFFLE_PATTERN_16_8__  0xe0

#define SHUFFLE_VECTOR_16__(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) \
   SHUFFLE_PATTERN_16_##A##__, \
   SHUFFLE_PATTERN_16_##B##__, \
   SHUFFLE_PATTERN_16_##C##__, \
   SHUFFLE_PATTERN_16_##D##__, \
   SHUFFLE_PATTERN_16_##E##__, \
   SHUFFLE_PATTERN_16_##F##__, \
   SHUFFLE_PATTERN_16_##G##__, \
   SHUFFLE_PATTERN_16_##H##__, \
   SHUFFLE_PATTERN_16_##I##__, \
   SHUFFLE_PATTERN_16_##J##__, \
   SHUFFLE_PATTERN_16_##K##__, \
   SHUFFLE_PATTERN_16_##L##__, \
   SHUFFLE_PATTERN_16_##M##__, \
   SHUFFLE_PATTERN_16_##N##__, \
   SHUFFLE_PATTERN_16_##O##__, \
   SHUFFLE_PATTERN_16_##P##__

#define SHUFFLE16(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) \
   ((const vector unsigned char){ \
      SHUFFLE_VECTOR_16__(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) \
   })

#define SHUFB16(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) \
   ((const qword){ \
      SHUFFLE_VECTOR_16__(A, B, C, D, E, F, G, H, I, J, K, L, M, N, O, P) \
   })

#endif
