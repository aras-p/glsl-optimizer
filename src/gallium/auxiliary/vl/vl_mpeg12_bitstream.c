/**************************************************************************
 *
 * Copyright 2011 Christian König.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

/**
 * This file is based uppon slice_xvmc.c and vlc.h from the xine project,
 * which in turn is based on mpeg2dec. The following is the original copyright:
 *
 * Copyright (C) 2000-2002 Michel Lespinasse <walken@zoy.org>
 * Copyright (C) 1999-2000 Aaron Holtzman <aholtzma@ess.engr.uvic.ca>
 *
 * This file is part of mpeg2dec, a free MPEG-2 video stream decoder.
 * See http://libmpeg2.sourceforge.net/ for updates.
 *
 * mpeg2dec is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * mpeg2dec is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <stdint.h>

#include <pipe/p_video_state.h>

#include "vl_vlc.h"
#include "vl_zscan.h"
#include "vl_mpeg12_bitstream.h"

/* take num bits from the high part of bit_buf and zero extend them */
#define UBITS(buf,num) (((uint32_t)(buf)) >> (32 - (num)))

/* take num bits from the high part of bit_buf and sign extend them */
#define SBITS(buf,num) (((int32_t)(buf)) >> (32 - (num)))

#define SATURATE(val)			\
do {					\
   if ((uint32_t)(val + 2048) > 4095)	\
      val = (val > 0) ? 2047 : -2048;	\
} while (0)

/* macroblock modes */
#define MACROBLOCK_INTRA 1
#define MACROBLOCK_PATTERN 2
#define MACROBLOCK_MOTION_BACKWARD 4
#define MACROBLOCK_MOTION_FORWARD 8
#define MACROBLOCK_QUANT 16

/* motion_type */
#define MOTION_TYPE_MASK (3*64)
#define MOTION_TYPE_BASE 64
#define MC_FIELD (1*64)
#define MC_FRAME (2*64)
#define MC_16X8 (2*64)
#define MC_DMV (3*64)

/* picture structure */
#define TOP_FIELD     1
#define BOTTOM_FIELD  2
#define FRAME_PICTURE 3

/* picture coding type (mpeg2 header) */
#define I_TYPE 1
#define P_TYPE 2
#define B_TYPE 3
#define D_TYPE 4

typedef struct {
   uint8_t modes;
   uint8_t len;
} MBtab;

typedef struct {
   uint8_t delta;
   uint8_t len;
} MVtab;

typedef struct {
   int8_t dmv;
   uint8_t len;
} DMVtab;

typedef struct {
   uint8_t cbp;
   uint8_t len;
} CBPtab;

typedef struct {
   uint8_t size;
   uint8_t len;
} DCtab;

typedef struct {
   uint8_t run;
   uint8_t level;
   uint8_t len;
} DCTtab;

typedef struct {
   uint8_t mba;
   uint8_t len;
} MBAtab;

#define INTRA MACROBLOCK_INTRA
#define QUANT MACROBLOCK_QUANT
#define MC MACROBLOCK_MOTION_FORWARD
#define CODED MACROBLOCK_PATTERN
#define FWD MACROBLOCK_MOTION_FORWARD
#define BWD MACROBLOCK_MOTION_BACKWARD
#define INTER MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD

static const MBtab MB_I [] = {
   {INTRA|QUANT, 2}, {INTRA, 1}
};

static const MBtab MB_P [] = {
   {INTRA|QUANT, 6}, {CODED|QUANT, 5}, {MC|CODED|QUANT, 5}, {INTRA,    5},
   {MC,          3}, {MC,          3}, {MC,             3}, {MC,       3},
   {CODED,       2}, {CODED,       2}, {CODED,          2}, {CODED,    2},
   {CODED,       2}, {CODED,       2}, {CODED,          2}, {CODED,    2},
   {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
   {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
   {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1},
   {MC|CODED,    1}, {MC|CODED,    1}, {MC|CODED,       1}, {MC|CODED, 1}
};

static const MBtab MB_B [] = {
   {0,                 0}, {INTRA|QUANT,       6},
   {BWD|CODED|QUANT,   6}, {FWD|CODED|QUANT,   6},
   {INTER|CODED|QUANT, 5}, {INTER|CODED|QUANT, 5},
                                     {INTRA,       5}, {INTRA,       5},
   {FWD,         4}, {FWD,         4}, {FWD,         4}, {FWD,         4},
   {FWD|CODED,   4}, {FWD|CODED,   4}, {FWD|CODED,   4}, {FWD|CODED,   4},
   {BWD,         3}, {BWD,         3}, {BWD,         3}, {BWD,         3},
   {BWD,         3}, {BWD,         3}, {BWD,         3}, {BWD,         3},
   {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3},
   {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3}, {BWD|CODED,   3},
   {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
   {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
   {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
   {INTER,       2}, {INTER,       2}, {INTER,       2}, {INTER,       2},
   {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
   {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
   {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2},
   {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}, {INTER|CODED, 2}
};

#undef INTRA
#undef QUANT
#undef MC
#undef CODED
#undef FWD
#undef BWD
#undef INTER

static const MVtab MV_4 [] = {
   { 3, 6}, { 2, 4}, { 1, 3}, { 1, 3}, { 0, 2}, { 0, 2}, { 0, 2}, { 0, 2}
};

static const MVtab MV_10 [] = {
   { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10}, { 0,10},
   { 0,10}, { 0,10}, { 0,10}, { 0,10}, {15,10}, {14,10}, {13,10}, {12,10},
   {11,10}, {10,10}, { 9, 9}, { 9, 9}, { 8, 9}, { 8, 9}, { 7, 9}, { 7, 9},
   { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7}, { 6, 7},
   { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7}, { 5, 7},
   { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}, { 4, 7}
};

static const DMVtab DMV_2 [] = {
   { 0, 1}, { 0, 1}, { 1, 2}, {-1, 2}
};

static const CBPtab CBP_7 [] = {
   {0x22, 7}, {0x12, 7}, {0x0a, 7}, {0x06, 7},
   {0x21, 7}, {0x11, 7}, {0x09, 7}, {0x05, 7},
   {0x3f, 6}, {0x3f, 6}, {0x03, 6}, {0x03, 6},
   {0x24, 6}, {0x24, 6}, {0x18, 6}, {0x18, 6},
   {0x3e, 5}, {0x3e, 5}, {0x3e, 5}, {0x3e, 5},
   {0x02, 5}, {0x02, 5}, {0x02, 5}, {0x02, 5},
   {0x3d, 5}, {0x3d, 5}, {0x3d, 5}, {0x3d, 5},
   {0x01, 5}, {0x01, 5}, {0x01, 5}, {0x01, 5},
   {0x38, 5}, {0x38, 5}, {0x38, 5}, {0x38, 5},
   {0x34, 5}, {0x34, 5}, {0x34, 5}, {0x34, 5},
   {0x2c, 5}, {0x2c, 5}, {0x2c, 5}, {0x2c, 5},
   {0x1c, 5}, {0x1c, 5}, {0x1c, 5}, {0x1c, 5},
   {0x28, 5}, {0x28, 5}, {0x28, 5}, {0x28, 5},
   {0x14, 5}, {0x14, 5}, {0x14, 5}, {0x14, 5},
   {0x30, 5}, {0x30, 5}, {0x30, 5}, {0x30, 5},
   {0x0c, 5}, {0x0c, 5}, {0x0c, 5}, {0x0c, 5},
   {0x20, 4}, {0x20, 4}, {0x20, 4}, {0x20, 4},
   {0x20, 4}, {0x20, 4}, {0x20, 4}, {0x20, 4},
   {0x10, 4}, {0x10, 4}, {0x10, 4}, {0x10, 4},
   {0x10, 4}, {0x10, 4}, {0x10, 4}, {0x10, 4},
   {0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
   {0x08, 4}, {0x08, 4}, {0x08, 4}, {0x08, 4},
   {0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
   {0x04, 4}, {0x04, 4}, {0x04, 4}, {0x04, 4},
   {0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3},
   {0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3},
   {0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3},
   {0x3c, 3}, {0x3c, 3}, {0x3c, 3}, {0x3c, 3}
};

static const CBPtab CBP_9 [] = {
   {0,    0}, {0x00, 9}, {0x27, 9}, {0x1b, 9},
   {0x3b, 9}, {0x37, 9}, {0x2f, 9}, {0x1f, 9},
   {0x3a, 8}, {0x3a, 8}, {0x36, 8}, {0x36, 8},
   {0x2e, 8}, {0x2e, 8}, {0x1e, 8}, {0x1e, 8},
   {0x39, 8}, {0x39, 8}, {0x35, 8}, {0x35, 8},
   {0x2d, 8}, {0x2d, 8}, {0x1d, 8}, {0x1d, 8},
   {0x26, 8}, {0x26, 8}, {0x1a, 8}, {0x1a, 8},
   {0x25, 8}, {0x25, 8}, {0x19, 8}, {0x19, 8},
   {0x2b, 8}, {0x2b, 8}, {0x17, 8}, {0x17, 8},
   {0x33, 8}, {0x33, 8}, {0x0f, 8}, {0x0f, 8},
   {0x2a, 8}, {0x2a, 8}, {0x16, 8}, {0x16, 8},
   {0x32, 8}, {0x32, 8}, {0x0e, 8}, {0x0e, 8},
   {0x29, 8}, {0x29, 8}, {0x15, 8}, {0x15, 8},
   {0x31, 8}, {0x31, 8}, {0x0d, 8}, {0x0d, 8},
   {0x23, 8}, {0x23, 8}, {0x13, 8}, {0x13, 8},
   {0x0b, 8}, {0x0b, 8}, {0x07, 8}, {0x07, 8}
};

static const DCtab DC_lum_5 [] = {
   {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
   {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
   {0, 3}, {0, 3}, {0, 3}, {0, 3}, {3, 3}, {3, 3}, {3, 3}, {3, 3},
   {4, 3}, {4, 3}, {4, 3}, {4, 3}, {5, 4}, {5, 4}, {6, 5}
};

static const DCtab DC_chrom_5 [] = {
   {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2}, {0, 2},
   {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2}, {1, 2},
   {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2}, {2, 2},
   {3, 3}, {3, 3}, {3, 3}, {3, 3}, {4, 4}, {4, 4}, {5, 5}
};

static const DCtab DC_long [] = {
   {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, { 6, 5}, { 6, 5},
   {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, {6, 5}, { 6, 5}, { 6, 5},
   {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, {7, 6}, { 7, 6}, { 7, 6},
   {8, 7}, {8, 7}, {8, 7}, {8, 7}, {9, 8}, {9, 8}, {10, 9}, {11, 9}
};

static const DCTtab DCT_16 [] = {
   {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
   {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
   {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
   {129, 0, 0}, {129, 0, 0}, {129, 0, 0}, {129, 0, 0},
   {  2,18, 0}, {  2,17, 0}, {  2,16, 0}, {  2,15, 0},
   {  7, 3, 0}, { 17, 2, 0}, { 16, 2, 0}, { 15, 2, 0},
   { 14, 2, 0}, { 13, 2, 0}, { 12, 2, 0}, { 32, 1, 0},
   { 31, 1, 0}, { 30, 1, 0}, { 29, 1, 0}, { 28, 1, 0}
};

static const DCTtab DCT_15 [] = {
   {  1,40,15}, {  1,39,15}, {  1,38,15}, {  1,37,15},
   {  1,36,15}, {  1,35,15}, {  1,34,15}, {  1,33,15},
   {  1,32,15}, {  2,14,15}, {  2,13,15}, {  2,12,15},
   {  2,11,15}, {  2,10,15}, {  2, 9,15}, {  2, 8,15},
   {  1,31,14}, {  1,31,14}, {  1,30,14}, {  1,30,14},
   {  1,29,14}, {  1,29,14}, {  1,28,14}, {  1,28,14},
   {  1,27,14}, {  1,27,14}, {  1,26,14}, {  1,26,14},
   {  1,25,14}, {  1,25,14}, {  1,24,14}, {  1,24,14},
   {  1,23,14}, {  1,23,14}, {  1,22,14}, {  1,22,14},
   {  1,21,14}, {  1,21,14}, {  1,20,14}, {  1,20,14},
   {  1,19,14}, {  1,19,14}, {  1,18,14}, {  1,18,14},
   {  1,17,14}, {  1,17,14}, {  1,16,14}, {  1,16,14}
};

static const DCTtab DCT_13 [] = {
   { 11, 2,13}, { 10, 2,13}, {  6, 3,13}, {  4, 4,13},
   {  3, 5,13}, {  2, 7,13}, {  2, 6,13}, {  1,15,13},
   {  1,14,13}, {  1,13,13}, {  1,12,13}, { 27, 1,13},
   { 26, 1,13}, { 25, 1,13}, { 24, 1,13}, { 23, 1,13},
   {  1,11,12}, {  1,11,12}, {  9, 2,12}, {  9, 2,12},
   {  5, 3,12}, {  5, 3,12}, {  1,10,12}, {  1,10,12},
   {  3, 4,12}, {  3, 4,12}, {  8, 2,12}, {  8, 2,12},
   { 22, 1,12}, { 22, 1,12}, { 21, 1,12}, { 21, 1,12},
   {  1, 9,12}, {  1, 9,12}, { 20, 1,12}, { 20, 1,12},
   { 19, 1,12}, { 19, 1,12}, {  2, 5,12}, {  2, 5,12},
   {  4, 3,12}, {  4, 3,12}, {  1, 8,12}, {  1, 8,12},
   {  7, 2,12}, {  7, 2,12}, { 18, 1,12}, { 18, 1,12}
};

static const DCTtab DCT_B14_10 [] = {
   { 17, 1,10}, {  6, 2,10}, {  1, 7,10}, {  3, 3,10},
   {  2, 4,10}, { 16, 1,10}, { 15, 1,10}, {  5, 2,10}
};

static const DCTtab DCT_B14_8 [] = {
   { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6},
   {  3, 2, 7}, {  3, 2, 7}, { 10, 1, 7}, { 10, 1, 7},
   {  1, 4, 7}, {  1, 4, 7}, {  9, 1, 7}, {  9, 1, 7},
   {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6}, {  8, 1, 6},
   {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6}, {  7, 1, 6},
   {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6}, {  2, 2, 6},
   {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
   { 14, 1, 8}, {  1, 6, 8}, { 13, 1, 8}, { 12, 1, 8},
   {  4, 2, 8}, {  2, 3, 8}, {  1, 5, 8}, { 11, 1, 8}
};

static const DCTtab DCT_B14AC_5 [] = {
                {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
   {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
   {129, 0, 2}, {129, 0, 2}, {129, 0, 2}, {129, 0, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}
};

static const DCTtab DCT_B14DC_5 [] = {
                {  1, 3, 5}, {  5, 1, 5}, {  4, 1, 5},
   {  1, 2, 4}, {  1, 2, 4}, {  3, 1, 4}, {  3, 1, 4},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
   {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
   {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1},
   {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}, {  1, 1, 1}
};

static const DCTtab DCT_B15_10 [] = {
   {  6, 2, 9}, {  6, 2, 9}, { 15, 1, 9}, { 15, 1, 9},
   {  3, 4,10}, { 17, 1,10}, { 16, 1, 9}, { 16, 1, 9}
};

static const DCTtab DCT_B15_8 [] = {
   { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6}, { 65, 0, 6},
   {  8, 1, 7}, {  8, 1, 7}, {  9, 1, 7}, {  9, 1, 7},
   {  7, 1, 7}, {  7, 1, 7}, {  3, 2, 7}, {  3, 2, 7},
   {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6}, {  1, 7, 6},
   {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6}, {  1, 6, 6},
   {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6}, {  5, 1, 6},
   {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6}, {  6, 1, 6},
   {  2, 5, 8}, { 12, 1, 8}, {  1,11, 8}, {  1,10, 8},
   { 14, 1, 8}, { 13, 1, 8}, {  4, 2, 8}, {  2, 4, 8},
   {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
   {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5}, {  3, 1, 5},
   {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
   {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5}, {  2, 2, 5},
   {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
   {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5}, {  4, 1, 5},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3}, {  2, 1, 3},
   {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
   {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
   {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
   {129, 0, 4}, {129, 0, 4}, {129, 0, 4}, {129, 0, 4},
   {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
   {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
   {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
   {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4}, {  1, 3, 4},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2}, {  1, 1, 2},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3}, {  1, 2, 3},
   {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
   {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5}, {  1, 4, 5},
   {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
   {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5}, {  1, 5, 5},
   { 10, 1, 7}, { 10, 1, 7}, {  2, 3, 7}, {  2, 3, 7},
   { 11, 1, 7}, { 11, 1, 7}, {  1, 8, 7}, {  1, 8, 7},
   {  1, 9, 7}, {  1, 9, 7}, {  1,12, 8}, {  1,13, 8},
   {  3, 3, 8}, {  5, 2, 8}, {  1,14, 8}, {  1,15, 8}
};

static const MBAtab MBA_5 [] = {
                   {6, 5}, {5, 5}, {4, 4}, {4, 4}, {3, 4}, {3, 4},
   {2, 3}, {2, 3}, {2, 3}, {2, 3}, {1, 3}, {1, 3}, {1, 3}, {1, 3},
   {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1},
   {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}, {0, 1}
};

static const MBAtab MBA_11 [] = {
   {32, 11}, {31, 11}, {30, 11}, {29, 11},
   {28, 11}, {27, 11}, {26, 11}, {25, 11},
   {24, 11}, {23, 11}, {22, 11}, {21, 11},
   {20, 10}, {20, 10}, {19, 10}, {19, 10},
   {18, 10}, {18, 10}, {17, 10}, {17, 10},
   {16, 10}, {16, 10}, {15, 10}, {15, 10},
   {14,  8}, {14,  8}, {14,  8}, {14,  8},
   {14,  8}, {14,  8}, {14,  8}, {14,  8},
   {13,  8}, {13,  8}, {13,  8}, {13,  8},
   {13,  8}, {13,  8}, {13,  8}, {13,  8},
   {12,  8}, {12,  8}, {12,  8}, {12,  8},
   {12,  8}, {12,  8}, {12,  8}, {12,  8},
   {11,  8}, {11,  8}, {11,  8}, {11,  8},
   {11,  8}, {11,  8}, {11,  8}, {11,  8},
   {10,  8}, {10,  8}, {10,  8}, {10,  8},
   {10,  8}, {10,  8}, {10,  8}, {10,  8},
   { 9,  8}, { 9,  8}, { 9,  8}, { 9,  8},
   { 9,  8}, { 9,  8}, { 9,  8}, { 9,  8},
   { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
   { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
   { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
   { 8,  7}, { 8,  7}, { 8,  7}, { 8,  7},
   { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
   { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
   { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7},
   { 7,  7}, { 7,  7}, { 7,  7}, { 7,  7}
};

static const int non_linear_quantizer_scale[] = {
   0,  1,  2,  3,  4,  5,   6,   7,
   8, 10, 12, 14, 16, 18,  20,  22,
   24, 28, 32, 36, 40, 44,  48,  52,
   56, 64, 72, 80, 88, 96, 104, 112
};

static inline int
get_macroblock_modes(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc * picture)
{
   int macroblock_modes;
   const MBtab * tab;

   switch (picture->picture_coding_type) {
   case I_TYPE:

      tab = MB_I + vl_vlc_ubits(&bs->vlc, 1);
      vl_vlc_dumpbits(&bs->vlc, tab->len);
      macroblock_modes = tab->modes;

      return macroblock_modes;

   case P_TYPE:

      tab = MB_P + vl_vlc_ubits(&bs->vlc, 5);
      vl_vlc_dumpbits(&bs->vlc, tab->len);
      macroblock_modes = tab->modes;

      if (picture->picture_structure != FRAME_PICTURE) {
         if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
            macroblock_modes |= vl_vlc_ubits(&bs->vlc, 2) * MOTION_TYPE_BASE;
            vl_vlc_dumpbits(&bs->vlc, 2);
          }
          return macroblock_modes;
      } else if (picture->frame_pred_frame_dct) {
          if (macroblock_modes & MACROBLOCK_MOTION_FORWARD)
            macroblock_modes |= MC_FRAME;
          return macroblock_modes;
      } else {
          if (macroblock_modes & MACROBLOCK_MOTION_FORWARD) {
            macroblock_modes |= vl_vlc_ubits(&bs->vlc, 2) * MOTION_TYPE_BASE;
            vl_vlc_dumpbits(&bs->vlc, 2);
          }
          return macroblock_modes;
      }

   case B_TYPE:

      tab = MB_B + vl_vlc_ubits(&bs->vlc, 6);
      vl_vlc_dumpbits(&bs->vlc, tab->len);
      macroblock_modes = tab->modes;

      if (picture->picture_structure != FRAME_PICTURE) {
          if (! (macroblock_modes & MACROBLOCK_INTRA)) {
            macroblock_modes |= vl_vlc_ubits(&bs->vlc, 2) * MOTION_TYPE_BASE;
            vl_vlc_dumpbits(&bs->vlc, 2);
          }
      } else if (picture->frame_pred_frame_dct) {
          macroblock_modes |= MC_FRAME;
      } else if (!(macroblock_modes & MACROBLOCK_INTRA)) {
          macroblock_modes |= vl_vlc_ubits(&bs->vlc, 2) * MOTION_TYPE_BASE;
          vl_vlc_dumpbits(&bs->vlc, 2);
      }
      return macroblock_modes;

   case D_TYPE:

      vl_vlc_dumpbits(&bs->vlc, 1);
      return MACROBLOCK_INTRA;

   default:
      return 0;
   }
}

static inline enum pipe_mpeg12_dct_type
get_dct_type(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc * picture, int macroblock_modes)
{
   enum pipe_mpeg12_dct_type dct_type = PIPE_MPEG12_DCT_TYPE_FRAME;

   if ((picture->picture_structure == FRAME_PICTURE) &&
       (!picture->frame_pred_frame_dct) &&
       (macroblock_modes & (MACROBLOCK_INTRA | MACROBLOCK_PATTERN))) {

      dct_type = vl_vlc_ubits(&bs->vlc, 1) ? PIPE_MPEG12_DCT_TYPE_FIELD : PIPE_MPEG12_DCT_TYPE_FRAME;
      vl_vlc_dumpbits(&bs->vlc, 1);
   }
   return dct_type;
}

static inline int
get_quantizer_scale(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc * picture)
{
   int quantizer_scale_code;

   quantizer_scale_code = vl_vlc_ubits(&bs->vlc, 5);
   vl_vlc_dumpbits(&bs->vlc, 5);

   if (picture->q_scale_type)
      return non_linear_quantizer_scale[quantizer_scale_code];
   else
      return quantizer_scale_code << 1;
}

static inline int
get_motion_delta(struct vl_mpg12_bs *bs, unsigned f_code)
{
   int delta;
   int sign;
   const MVtab * tab;

   if (bs->vlc.buf & 0x80000000) {
      vl_vlc_dumpbits(&bs->vlc, 1);
      return 0;
   } else if (bs->vlc.buf >= 0x0c000000) {

      tab = MV_4 + vl_vlc_ubits(&bs->vlc, 4);
      delta = (tab->delta << f_code) + 1;
      bs->vlc.bits += tab->len + f_code + 1;
      bs->vlc.buf <<= tab->len;

      sign = vl_vlc_sbits(&bs->vlc, 1);
      bs->vlc.buf <<= 1;

      if (f_code)
         delta += vl_vlc_ubits(&bs->vlc, f_code);
      bs->vlc.buf <<= f_code;

      return (delta ^ sign) - sign;

   } else {

      tab = MV_10 + vl_vlc_ubits(&bs->vlc, 10);
      delta = (tab->delta << f_code) + 1;
      bs->vlc.bits += tab->len + 1;
      bs->vlc.buf <<= tab->len;

      sign = vl_vlc_sbits(&bs->vlc, 1);
      bs->vlc.buf <<= 1;

      if (f_code) {
         vl_vlc_needbits(&bs->vlc);
         delta += vl_vlc_ubits(&bs->vlc, f_code);
         vl_vlc_dumpbits(&bs->vlc, f_code);
      }

      return (delta ^ sign) - sign;
   }
}

static inline int
bound_motion_vector(int vec, unsigned f_code)
{
#if 1
   unsigned int limit;
   int sign;

   limit = 16 << f_code;

   if ((unsigned int)(vec + limit) < 2 * limit)
      return vec;
   else {
      sign = ((int32_t)vec) >> 31;
      return vec - ((2 * limit) ^ sign) + sign;
   }
#else
   return ((int32_t)vec << (28 - f_code)) >> (28 - f_code);
#endif
}

static inline int
get_dmv(struct vl_mpg12_bs *bs)
{
   const DMVtab * tab;

   tab = DMV_2 + vl_vlc_ubits(&bs->vlc, 2);
   vl_vlc_dumpbits(&bs->vlc, tab->len);
   return tab->dmv;
}

static inline int
get_coded_block_pattern(struct vl_mpg12_bs *bs)
{
   const CBPtab * tab;

   vl_vlc_needbits(&bs->vlc);

   if (bs->vlc.buf >= 0x20000000) {

      tab = CBP_7 + (vl_vlc_ubits(&bs->vlc, 7) - 16);
      vl_vlc_dumpbits(&bs->vlc, tab->len);
      return tab->cbp;

   } else {

      tab = CBP_9 + vl_vlc_ubits(&bs->vlc, 9);
      vl_vlc_dumpbits(&bs->vlc, tab->len);
      return tab->cbp;
   }
}

static inline int
get_luma_dc_dct_diff(struct vl_mpg12_bs *bs)
{
   const DCtab * tab;
   int size;
   int dc_diff;

   if (bs->vlc.buf < 0xf8000000) {
      tab = DC_lum_5 + vl_vlc_ubits(&bs->vlc, 5);
      size = tab->size;
      if (size) {
         bs->vlc.bits += tab->len + size;
         bs->vlc.buf <<= tab->len;
         dc_diff = vl_vlc_ubits(&bs->vlc, size) - UBITS (SBITS (~bs->vlc.buf, 1), size);
         bs->vlc.buf <<= size;
         return dc_diff;
      } else {
         vl_vlc_dumpbits(&bs->vlc, 3);
         return 0;
      }
   } else {
      tab = DC_long + (vl_vlc_ubits(&bs->vlc, 9) - 0x1e0);
      size = tab->size;
      vl_vlc_dumpbits(&bs->vlc, tab->len);
      vl_vlc_needbits(&bs->vlc);
      dc_diff = vl_vlc_ubits(&bs->vlc, size) - UBITS (SBITS (~bs->vlc.buf, 1), size);
      vl_vlc_dumpbits(&bs->vlc, size);
      return dc_diff;
   }
}

static inline int
get_chroma_dc_dct_diff(struct vl_mpg12_bs *bs)
{
   const DCtab * tab;
   int size;
   int dc_diff;

   if (bs->vlc.buf < 0xf8000000) {
      tab = DC_chrom_5 + vl_vlc_ubits(&bs->vlc, 5);
      size = tab->size;
      if (size) {
         bs->vlc.bits += tab->len + size;
         bs->vlc.buf <<= tab->len;
         dc_diff = vl_vlc_ubits(&bs->vlc, size) - UBITS (SBITS (~bs->vlc.buf, 1), size);
         bs->vlc.buf <<= size;
         return dc_diff;
      } else {
         vl_vlc_dumpbits(&bs->vlc, 2);
         return 0;
      }
   } else {
      tab = DC_long + (vl_vlc_ubits(&bs->vlc, 10) - 0x3e0);
      size = tab->size;
      vl_vlc_dumpbits(&bs->vlc, tab->len + 1);
      vl_vlc_needbits(&bs->vlc);
      dc_diff = vl_vlc_ubits(&bs->vlc, size) - UBITS (SBITS (~bs->vlc.buf, 1), size);
      vl_vlc_dumpbits(&bs->vlc, size);
      return dc_diff;
   }
}

static inline void
get_intra_block_B14(struct vl_mpg12_bs *bs, const int quant_matrix[64], int quantizer_scale, short *dest)
{
   int i, val;
   int mismatch;
   const DCTtab *tab;

   i = 0;
   mismatch = ~dest[0];

   vl_vlc_needbits(&bs->vlc);

   while (1) {
      if (bs->vlc.buf >= 0x28000000) {

         tab = DCT_B14AC_5 + (vl_vlc_ubits(&bs->vlc, 5) - 5);

         i += tab->run;
         if (i >= 64)
            break;	/* end of block */

      normal_code:
         bs->vlc.buf <<= tab->len;
         bs->vlc.bits += tab->len + 1;
         val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;

         /* if (bitstream_get (1)) val = -val; */
         val = (val ^ vl_vlc_sbits(&bs->vlc, 1)) - vl_vlc_sbits(&bs->vlc, 1);

         SATURATE (val);
         dest[i] = val;
         mismatch ^= val;

         bs->vlc.buf <<= 1;
         vl_vlc_needbits(&bs->vlc);

         continue;

      } else if (bs->vlc.buf >= 0x04000000) {

         tab = DCT_B14_8 + (vl_vlc_ubits(&bs->vlc, 8) - 4);

         i += tab->run;
         if (i < 64)
            goto normal_code;

         /* escape code */

         i += UBITS(bs->vlc.buf << 6, 6) - 64;
         if (i >= 64)
            break;	/* illegal, check needed to avoid buffer overflow */

         vl_vlc_dumpbits(&bs->vlc, 12);
         vl_vlc_needbits(&bs->vlc);
         val = (vl_vlc_sbits(&bs->vlc, 12) * quantizer_scale * quant_matrix[i]) / 16;

         SATURATE (val);
         dest[i] = val;
         mismatch ^= val;

         vl_vlc_dumpbits(&bs->vlc, 12);
         vl_vlc_needbits(&bs->vlc);

         continue;

      } else if (bs->vlc.buf >= 0x02000000) {
         tab = DCT_B14_10 + (vl_vlc_ubits(&bs->vlc, 10) - 8);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00800000) {
         tab = DCT_13 + (vl_vlc_ubits(&bs->vlc, 13) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00200000) {
         tab = DCT_15 + (vl_vlc_ubits(&bs->vlc, 15) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else {
         tab = DCT_16 + vl_vlc_ubits(&bs->vlc, 16);
         bs->vlc.buf <<= 16;
         vl_vlc_getword(&bs->vlc, bs->vlc.bits + 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      }
      break;	/* illegal, check needed to avoid buffer overflow */
   }

   dest[63] ^= mismatch & 1;
   vl_vlc_dumpbits(&bs->vlc, 2);	/* dump end of block code */
}

static inline void
get_intra_block_B15(struct vl_mpg12_bs *bs, const int quant_matrix[64], int quantizer_scale, short *dest)
{
   int i, val;
   int mismatch;
   const DCTtab * tab;

   i = 0;
   mismatch = ~dest[0];

   vl_vlc_needbits(&bs->vlc);

   while (1) {
      if (bs->vlc.buf >= 0x04000000) {

         tab = DCT_B15_8 + (vl_vlc_ubits(&bs->vlc, 8) - 4);

         i += tab->run;
         if (i < 64) {

         normal_code:
            bs->vlc.buf <<= tab->len;
            bs->vlc.bits += tab->len + 1;
            val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;

            /* if (bitstream_get (1)) val = -val; */
            val = (val ^ vl_vlc_sbits(&bs->vlc, 1)) - vl_vlc_sbits(&bs->vlc, 1);

            SATURATE (val);
            dest[i] = val;
            mismatch ^= val;

            bs->vlc.buf <<= 1;
            vl_vlc_needbits(&bs->vlc);

            continue;

         } else {

            /* end of block. I commented out this code because if we */
            /* dont exit here we will still exit at the later test :) */

            /* if (i >= 128) break;	*/	/* end of block */

            /* escape code */

            i += UBITS(bs->vlc.buf << 6, 6) - 64;
            if (i >= 64)
                break;	/* illegal, check against buffer overflow */

            vl_vlc_dumpbits(&bs->vlc, 12);
            vl_vlc_needbits(&bs->vlc);
            val = (vl_vlc_sbits(&bs->vlc, 12) * quantizer_scale * quant_matrix[i]) / 16;

            SATURATE (val);
            dest[i] = val;
            mismatch ^= val;

            vl_vlc_dumpbits(&bs->vlc, 12);
            vl_vlc_needbits(&bs->vlc);

            continue;

          }
      } else if (bs->vlc.buf >= 0x02000000) {
         tab = DCT_B15_10 + (vl_vlc_ubits(&bs->vlc, 10) - 8);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00800000) {
         tab = DCT_13 + (vl_vlc_ubits(&bs->vlc, 13) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00200000) {
         tab = DCT_15 + (vl_vlc_ubits(&bs->vlc, 15) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else {
         tab = DCT_16 + vl_vlc_ubits(&bs->vlc, 16);
         bs->vlc.buf <<= 16;
         vl_vlc_getword(&bs->vlc, bs->vlc.bits + 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      }
      break;	/* illegal, check needed to avoid buffer overflow */
   }

   dest[63] ^= mismatch & 1;
   vl_vlc_dumpbits(&bs->vlc, 4);	/* dump end of block code */
}

static inline void
get_non_intra_block(struct vl_mpg12_bs *bs, const int quant_matrix[64], int quantizer_scale, short *dest)
{
   int i, val;
   int mismatch;
   const DCTtab *tab;

   i = -1;
   mismatch = 1;

   vl_vlc_needbits(&bs->vlc);
   if (bs->vlc.buf >= 0x28000000) {
      tab = DCT_B14DC_5 + (vl_vlc_ubits(&bs->vlc, 5) - 5);
      goto entry_1;
   } else
      goto entry_2;

   while (1) {
      if (bs->vlc.buf >= 0x28000000) {

         tab = DCT_B14AC_5 + (vl_vlc_ubits(&bs->vlc, 5) - 5);

      entry_1:
         i += tab->run;
         if (i >= 64)
            break;	/* end of block */

      normal_code:
         bs->vlc.buf <<= tab->len;
         bs->vlc.bits += tab->len + 1;
         val = ((2*tab->level+1) * quantizer_scale * quant_matrix[i]) >> 5;

         /* if (bitstream_get (1)) val = -val; */
         val = (val ^ vl_vlc_sbits(&bs->vlc, 1)) - vl_vlc_sbits(&bs->vlc, 1);

         SATURATE (val);
         dest[i] = val;
         mismatch ^= val;

         bs->vlc.buf <<= 1;
         vl_vlc_needbits(&bs->vlc);

         continue;

      }

   entry_2:
      if (bs->vlc.buf >= 0x04000000) {

         tab = DCT_B14_8 + (vl_vlc_ubits(&bs->vlc, 8) - 4);

         i += tab->run;
         if (i < 64)
            goto normal_code;

         /* escape code */

         i += UBITS(bs->vlc.buf << 6, 6) - 64;
         if (i >= 64)
            break;	/* illegal, check needed to avoid buffer overflow */

         vl_vlc_dumpbits(&bs->vlc, 12);
         vl_vlc_needbits(&bs->vlc);
         val = 2 * (vl_vlc_sbits(&bs->vlc, 12) + vl_vlc_sbits(&bs->vlc, 1)) + 1;
         val = (val * quantizer_scale * quant_matrix[i]) / 32;

         SATURATE (val);
         dest[i] = val;
         mismatch ^= val;

         vl_vlc_dumpbits(&bs->vlc, 12);
         vl_vlc_needbits(&bs->vlc);

         continue;

      } else if (bs->vlc.buf >= 0x02000000) {
         tab = DCT_B14_10 + (vl_vlc_ubits(&bs->vlc, 10) - 8);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00800000) {
         tab = DCT_13 + (vl_vlc_ubits(&bs->vlc, 13) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00200000) {
         tab = DCT_15 + (vl_vlc_ubits(&bs->vlc, 15) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else {
         tab = DCT_16 + vl_vlc_ubits(&bs->vlc, 16);
         bs->vlc.buf <<= 16;
         vl_vlc_getword(&bs->vlc, bs->vlc.bits + 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      }
      break;	/* illegal, check needed to avoid buffer overflow */
   }
   dest[63] ^= mismatch & 1;
   vl_vlc_dumpbits(&bs->vlc, 2);	/* dump end of block code */
}

static inline void
get_mpeg1_intra_block(struct vl_mpg12_bs *bs, const int quant_matrix[64], int quantizer_scale, short *dest)
{
   int i, val;
   const DCTtab * tab;

   i = 0;

   vl_vlc_needbits(&bs->vlc);

   while (1) {
      if (bs->vlc.buf >= 0x28000000) {

         tab = DCT_B14AC_5 + (vl_vlc_ubits(&bs->vlc, 5) - 5);

         i += tab->run;
         if (i >= 64)
            break;	/* end of block */

      normal_code:
         bs->vlc.buf <<= tab->len;
         bs->vlc.bits += tab->len + 1;
         val = (tab->level * quantizer_scale * quant_matrix[i]) >> 4;

         /* oddification */
         val = (val - 1) | 1;

         /* if (bitstream_get (1)) val = -val; */
         val = (val ^ vl_vlc_sbits(&bs->vlc, 1)) - vl_vlc_sbits(&bs->vlc, 1);

         SATURATE (val);
         dest[i] = val;

         bs->vlc.buf <<= 1;
         vl_vlc_needbits(&bs->vlc);

         continue;

      } else if (bs->vlc.buf >= 0x04000000) {

         tab = DCT_B14_8 + (vl_vlc_ubits(&bs->vlc, 8) - 4);

         i += tab->run;
         if (i < 64)
            goto normal_code;

         /* escape code */

         i += UBITS(bs->vlc.buf << 6, 6) - 64;
         if (i >= 64)
            break;	/* illegal, check needed to avoid buffer overflow */

         vl_vlc_dumpbits(&bs->vlc, 12);
         vl_vlc_needbits(&bs->vlc);
         val = vl_vlc_sbits(&bs->vlc, 8);
         if (! (val & 0x7f)) {
            vl_vlc_dumpbits(&bs->vlc, 8);
            val = vl_vlc_ubits(&bs->vlc, 8) + 2 * val;
         }
         val = (val * quantizer_scale * quant_matrix[i]) / 16;

         /* oddification */
         val = (val + ~SBITS (val, 1)) | 1;

         SATURATE (val);
         dest[i] = val;

         vl_vlc_dumpbits(&bs->vlc, 8);
         vl_vlc_needbits(&bs->vlc);

         continue;

      } else if (bs->vlc.buf >= 0x02000000) {
         tab = DCT_B14_10 + (vl_vlc_ubits(&bs->vlc, 10) - 8);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00800000) {
         tab = DCT_13 + (vl_vlc_ubits(&bs->vlc, 13) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00200000) {
         tab = DCT_15 + (vl_vlc_ubits(&bs->vlc, 15) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else {
         tab = DCT_16 + vl_vlc_ubits(&bs->vlc, 16);
         bs->vlc.buf <<= 16;
         vl_vlc_getword(&bs->vlc, bs->vlc.bits + 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      }
      break;	/* illegal, check needed to avoid buffer overflow */
   }
   vl_vlc_dumpbits(&bs->vlc, 2);	/* dump end of block code */
}

static inline void
get_mpeg1_non_intra_block(struct vl_mpg12_bs *bs, const int quant_matrix[64], int quantizer_scale, short *dest)
{
   int i, val;
   const DCTtab * tab;

   i = -1;

   vl_vlc_needbits(&bs->vlc);
   if (bs->vlc.buf >= 0x28000000) {
      tab = DCT_B14DC_5 + (vl_vlc_ubits(&bs->vlc, 5) - 5);
      goto entry_1;
   } else
      goto entry_2;

   while (1) {
      if (bs->vlc.buf >= 0x28000000) {

         tab = DCT_B14AC_5 + (vl_vlc_ubits(&bs->vlc, 5) - 5);

      entry_1:
         i += tab->run;
         if (i >= 64)
            break;	/* end of block */

      normal_code:
         bs->vlc.buf <<= tab->len;
         bs->vlc.bits += tab->len + 1;
         val = ((2*tab->level+1) * quantizer_scale * quant_matrix[i]) >> 5;

         /* oddification */
         val = (val - 1) | 1;

         /* if (bitstream_get (1)) val = -val; */
         val = (val ^ vl_vlc_sbits(&bs->vlc, 1)) - vl_vlc_sbits(&bs->vlc, 1);

         SATURATE (val);
         dest[i] = val;

         bs->vlc.buf <<= 1;
         vl_vlc_needbits(&bs->vlc);

         continue;

      }

   entry_2:
      if (bs->vlc.buf >= 0x04000000) {

         tab = DCT_B14_8 + (vl_vlc_ubits(&bs->vlc, 8) - 4);

         i += tab->run;
         if (i < 64)
            goto normal_code;

         /* escape code */

         i += UBITS(bs->vlc.buf << 6, 6) - 64;
         if (i >= 64)
            break;	/* illegal, check needed to avoid buffer overflow */

         vl_vlc_dumpbits(&bs->vlc, 12);
         vl_vlc_needbits(&bs->vlc);
         val = vl_vlc_sbits(&bs->vlc, 8);
         if (! (val & 0x7f)) {
            vl_vlc_dumpbits(&bs->vlc, 8);
            val = vl_vlc_ubits(&bs->vlc, 8) + 2 * val;
         }
         val = 2 * (val + SBITS (val, 1)) + 1;
         val = (val * quantizer_scale * quant_matrix[i]) / 32;

         /* oddification */
         val = (val + ~SBITS (val, 1)) | 1;

         SATURATE (val);
         dest[i] = val;

         vl_vlc_dumpbits(&bs->vlc, 8);
         vl_vlc_needbits(&bs->vlc);

         continue;

      } else if (bs->vlc.buf >= 0x02000000) {
         tab = DCT_B14_10 + (vl_vlc_ubits(&bs->vlc, 10) - 8);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00800000) {
         tab = DCT_13 + (vl_vlc_ubits(&bs->vlc, 13) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else if (bs->vlc.buf >= 0x00200000) {
         tab = DCT_15 + (vl_vlc_ubits(&bs->vlc, 15) - 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      } else {
         tab = DCT_16 + vl_vlc_ubits(&bs->vlc, 16);
         bs->vlc.buf <<= 16;
         vl_vlc_getword(&bs->vlc, bs->vlc.bits + 16);
         i += tab->run;
         if (i < 64)
            goto normal_code;
      }
      break;	/* illegal, check needed to avoid buffer overflow */
   }
   vl_vlc_dumpbits(&bs->vlc, 2);	/* dump end of block code */
}

static inline void
slice_intra_DCT(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc * picture, const int quant_matrix[64], int cc,
                 unsigned x, unsigned y, enum pipe_mpeg12_dct_type coding, int quantizer_scale, int dc_dct_pred[3])
{
   short dest[64];

   bs->ycbcr_stream[cc]->x = x;
   bs->ycbcr_stream[cc]->y = y;
   bs->ycbcr_stream[cc]->intra = PIPE_MPEG12_DCT_INTRA;
   bs->ycbcr_stream[cc]->coding = coding;

   vl_vlc_needbits(&bs->vlc);

   /* Get the intra DC coefficient and inverse quantize it */
   if (cc == 0)
      dc_dct_pred[0] += get_luma_dc_dct_diff(bs);
   else
      dc_dct_pred[cc] += get_chroma_dc_dct_diff(bs);

   memset(dest, 0, sizeof(int16_t) * 64);
   dest[0] = dc_dct_pred[cc] << (3 - picture->intra_dc_precision);
   if (picture->mpeg1) {
      if (picture->picture_coding_type != D_TYPE)
          get_mpeg1_intra_block(bs, quant_matrix, quantizer_scale, dest);
   } else if (picture->intra_vlc_format)
      get_intra_block_B15(bs, quant_matrix, quantizer_scale, dest);
   else
      get_intra_block_B14(bs, quant_matrix, quantizer_scale, dest);

   memcpy(bs->ycbcr_buffer[cc], dest, sizeof(int16_t) * 64);

   bs->num_ycbcr_blocks[cc]++;
   bs->ycbcr_stream[cc]++;
   bs->ycbcr_buffer[cc] += 64;
}

static inline void
slice_non_intra_DCT(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc * picture, const int quant_matrix[64], int cc,
                    unsigned x, unsigned y,  enum pipe_mpeg12_dct_type coding, int quantizer_scale)
{
   short dest[64];

   bs->ycbcr_stream[cc]->x = x;
   bs->ycbcr_stream[cc]->y = y;
   bs->ycbcr_stream[cc]->intra = PIPE_MPEG12_DCT_DELTA;
   bs->ycbcr_stream[cc]->coding = coding;

   memset(dest, 0, sizeof(int16_t) * 64);
   if (picture->mpeg1)
      get_mpeg1_non_intra_block(bs, quant_matrix, quantizer_scale, dest);
   else
      get_non_intra_block(bs, quant_matrix, quantizer_scale, dest);

   memcpy(bs->ycbcr_buffer[cc], dest, sizeof(int16_t) * 64);

   bs->num_ycbcr_blocks[cc]++;
   bs->ycbcr_stream[cc]++;
   bs->ycbcr_buffer[cc] += 64;
}

static inline void
motion_mp1(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   mv->top.field_select = mv->bottom.field_select = PIPE_VIDEO_FRAME;

   vl_vlc_needbits(&bs->vlc);
   motion_x = (mv->top.x + (get_motion_delta(bs, f_code[0]) << f_code[1]));
   motion_x = bound_motion_vector (motion_x, f_code[0] + f_code[1]);
   mv->top.x = mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = (mv->top.y + (get_motion_delta(bs, f_code[0]) << f_code[1]));
   motion_y = bound_motion_vector (motion_y, f_code[0] + f_code[1]);
   mv->top.y = mv->bottom.y = motion_y;
}

static inline void
motion_fr_frame(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   mv->top.field_select = mv->bottom.field_select = PIPE_VIDEO_FRAME;

   vl_vlc_needbits(&bs->vlc);
   motion_x = mv->top.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector(motion_x, f_code[0]);
   mv->top.x = mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = mv->top.y + get_motion_delta(bs, f_code[1]);
   motion_y = bound_motion_vector(motion_y, f_code[1]);
   mv->top.y = mv->bottom.y = motion_y;
}

static inline void
motion_fr_field(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   vl_vlc_needbits(&bs->vlc);
   mv->top.field_select = vl_vlc_ubits(&bs->vlc, 1) ?
      PIPE_VIDEO_BOTTOM_FIELD : PIPE_VIDEO_TOP_FIELD;
   vl_vlc_dumpbits(&bs->vlc, 1);

   motion_x = mv->top.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector (motion_x, f_code[0]);
   mv->top.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = (mv->top.y >> 1) + get_motion_delta(bs, f_code[1]);
   /* motion_y = bound_motion_vector (motion_y, f_code[1]); */
   mv->top.y = motion_y << 1;

   vl_vlc_needbits(&bs->vlc);
   mv->bottom.field_select = vl_vlc_ubits(&bs->vlc, 1) ?
      PIPE_VIDEO_BOTTOM_FIELD : PIPE_VIDEO_TOP_FIELD;
   vl_vlc_dumpbits(&bs->vlc, 1);

   motion_x = mv->bottom.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector (motion_x, f_code[0]);
   mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = (mv->bottom.y >> 1) + get_motion_delta(bs, f_code[1]);
   /* motion_y = bound_motion_vector (motion_y, f_code[1]); */
   mv->bottom.y = motion_y << 1;
}

static inline void
motion_fr_dmv(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   // TODO Implement dmv
   mv->top.field_select = mv->bottom.field_select = PIPE_VIDEO_FRAME;

   vl_vlc_needbits(&bs->vlc);
   motion_x = mv->top.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector(motion_x, f_code[0]);
   mv->top.x = mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = (mv->top.y >> 1) + get_motion_delta(bs, f_code[1]);
   /* motion_y = bound_motion_vector (motion_y, f_code[1]); */
   mv->top.y = mv->bottom.y = motion_y << 1;
}

/* like motion_frame, but parsing without actual motion compensation */
static inline void
motion_fr_conceal(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int tmp;

   mv->top.field_select = mv->bottom.field_select = PIPE_VIDEO_FRAME;

   vl_vlc_needbits(&bs->vlc);
   tmp = (mv->top.x + get_motion_delta(bs, f_code[0]));
   tmp = bound_motion_vector (tmp, f_code[0]);
   mv->top.x = mv->bottom.x = tmp;

   vl_vlc_needbits(&bs->vlc);
   tmp = (mv->top.y + get_motion_delta(bs, f_code[1]));
   tmp = bound_motion_vector (tmp, f_code[1]);
   mv->top.y = mv->bottom.y = tmp;

   vl_vlc_dumpbits(&bs->vlc, 1); /* remove marker_bit */
}

static inline void
motion_fi_field(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   vl_vlc_needbits(&bs->vlc);

   // ref_field
   //vl_vlc_ubits(&bs->vlc, 1);

   // TODO field select may need to do something here for bob (weave ok)
   mv->top.field_select = mv->bottom.field_select = PIPE_VIDEO_FRAME;
   vl_vlc_dumpbits(&bs->vlc, 1);

   motion_x = mv->top.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector (motion_x, f_code[0]);
   mv->top.x = mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = mv->top.y + get_motion_delta(bs, f_code[1]);
   motion_y = bound_motion_vector (motion_y, f_code[1]);
   mv->top.y = mv->bottom.y = motion_y;
}

static inline void
motion_fi_16x8(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   vl_vlc_needbits(&bs->vlc);

   // ref_field
   //vl_vlc_ubits(&bs->vlc, 1);

   // TODO field select may need to do something here bob  (weave ok)
   mv->top.field_select = PIPE_VIDEO_FRAME;
   vl_vlc_dumpbits(&bs->vlc, 1);

   motion_x = mv->top.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector (motion_x, f_code[0]);
   mv->top.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = mv->top.y + get_motion_delta(bs, f_code[1]);
   motion_y = bound_motion_vector (motion_y, f_code[1]);
   mv->top.y = motion_y;

   vl_vlc_needbits(&bs->vlc);
   // ref_field
   //vl_vlc_ubits(&bs->vlc, 1);

   // TODO field select may need to do something here for bob (weave ok)
   mv->bottom.field_select = PIPE_VIDEO_FRAME;
   vl_vlc_dumpbits(&bs->vlc, 1);

   motion_x = mv->bottom.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector (motion_x, f_code[0]);
   mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = mv->bottom.y + get_motion_delta(bs, f_code[1]);
   motion_y = bound_motion_vector (motion_y, f_code[1]);
   mv->bottom.y = motion_y;
}

static inline void
motion_fi_dmv(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int motion_x, motion_y;

   // TODO field select may need to do something here for bob  (weave ok)
   mv->top.field_select = mv->bottom.field_select = PIPE_VIDEO_FRAME;

   vl_vlc_needbits(&bs->vlc);
   motion_x = mv->top.x + get_motion_delta(bs, f_code[0]);
   motion_x = bound_motion_vector (motion_x, f_code[0]);
   mv->top.x = mv->bottom.x = motion_x;

   vl_vlc_needbits(&bs->vlc);
   motion_y = mv->top.y + get_motion_delta(bs, f_code[1]);
   motion_y = bound_motion_vector (motion_y, f_code[1]);
   mv->top.y = mv->bottom.y = motion_y;
}


static inline void
motion_fi_conceal(struct vl_mpg12_bs *bs, unsigned f_code[2], struct pipe_motionvector *mv)
{
   int tmp;

   vl_vlc_needbits(&bs->vlc);
   vl_vlc_dumpbits(&bs->vlc, 1); /* remove field_select */

   tmp = (mv->top.x + get_motion_delta(bs, f_code[0]));
   tmp = bound_motion_vector(tmp, f_code[0]);
   mv->top.x = mv->bottom.x = tmp;

   vl_vlc_needbits(&bs->vlc);
   tmp = (mv->top.y + get_motion_delta(bs, f_code[1]));
   tmp = bound_motion_vector(tmp, f_code[1]);
   mv->top.y = mv->bottom.y = tmp;

   vl_vlc_dumpbits(&bs->vlc, 1); /* remove marker_bit */
}

#define MOTION_CALL(routine, macroblock_modes)		\
do {							\
   if ((macroblock_modes) & MACROBLOCK_MOTION_FORWARD)  \
      routine(bs, picture->f_code[0], &mv_fwd);         \
   if ((macroblock_modes) & MACROBLOCK_MOTION_BACKWARD)	\
      routine(bs, picture->f_code[1], &mv_bwd);         \
} while (0)

static inline void
store_motionvectors(struct vl_mpg12_bs *bs, unsigned *mv_pos,
                    struct pipe_motionvector *mv_fwd,
                    struct pipe_motionvector *mv_bwd)
{
   bs->mv_stream[0][*mv_pos].top = mv_fwd->top;
   bs->mv_stream[0][*mv_pos].bottom =
      mv_fwd->top.field_select == PIPE_VIDEO_FRAME ?
      mv_fwd->top : mv_fwd->bottom;

   bs->mv_stream[1][*mv_pos].top = mv_bwd->top;
   bs->mv_stream[1][*mv_pos].bottom =
      mv_bwd->top.field_select == PIPE_VIDEO_FRAME ?
      mv_bwd->top : mv_bwd->bottom;

   (*mv_pos)++;
}

static inline bool
slice_init(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc * picture,
           int *quantizer_scale, unsigned *x, unsigned *y, unsigned *mv_pos)
{
   const MBAtab * mba;

   vl_vlc_need32bits(&bs->vlc);
   while(bs->vlc.buf < 0x101 || bs->vlc.buf > 0x1AF) {
      if(!vl_vlc_getbyte(&bs->vlc))
         return false;
   }
   *y = (bs->vlc.buf & 0xFF) - 1;
   vl_vlc_restart(&bs->vlc);

   *quantizer_scale = get_quantizer_scale(bs, picture);

   /* ignore intra_slice and all the extra data */
   while (bs->vlc.buf & 0x80000000) {
      vl_vlc_dumpbits(&bs->vlc, 9);
      vl_vlc_needbits(&bs->vlc);
   }

   /* decode initial macroblock address increment */
   *x = 0;
   while (1) {
      if (bs->vlc.buf >= 0x08000000) {
          mba = MBA_5 + (vl_vlc_ubits(&bs->vlc, 6) - 2);
          break;
      } else if (bs->vlc.buf >= 0x01800000) {
          mba = MBA_11 + (vl_vlc_ubits(&bs->vlc, 12) - 24);
          break;
      } else switch (vl_vlc_ubits(&bs->vlc, 12)) {
      case 8:		/* macroblock_escape */
          *x += 33;
          vl_vlc_dumpbits(&bs->vlc, 11);
          vl_vlc_needbits(&bs->vlc);
          continue;
      case 15:	/* macroblock_stuffing (MPEG1 only) */
          bs->vlc.buf &= 0xfffff;
          vl_vlc_dumpbits(&bs->vlc, 11);
          vl_vlc_needbits(&bs->vlc);
          continue;
      default:	/* error */
          return false;
      }
   }
   vl_vlc_dumpbits(&bs->vlc, mba->len + 1);
   *x += mba->mba;

   while (*x >= bs->width) {
      *x -= bs->width;
      (*y)++;
   }
   if (*y > bs->height)
      return false;

   *mv_pos = *x + *y * bs->width;

   return true;
}

static inline bool
decode_slice(struct vl_mpg12_bs *bs, struct pipe_mpeg12_picture_desc *picture,
             const int intra_quantizer_matrix[64], const int non_intra_quantizer_matrix[64])
{
   enum pipe_video_field_select default_field_select;
   struct pipe_motionvector mv_fwd, mv_bwd;
   enum pipe_mpeg12_dct_type dct_type;

   /* predictor for DC coefficients in intra blocks */
   int dc_dct_pred[3] = { 0, 0, 0 };
   int quantizer_scale;

   unsigned x, y, mv_pos;

   switch(picture->picture_structure) {
   case TOP_FIELD:
      default_field_select = PIPE_VIDEO_TOP_FIELD;
      break;

   case BOTTOM_FIELD:
      default_field_select = PIPE_VIDEO_BOTTOM_FIELD;
      break;

   default:
      default_field_select = PIPE_VIDEO_FRAME;
      break;
   }

   if (!slice_init(bs, picture, &quantizer_scale, &x, &y, &mv_pos))
      return false;

   mv_fwd.top.x = mv_fwd.top.y = mv_fwd.bottom.x = mv_fwd.bottom.y = 0;
   mv_fwd.top.field_select = mv_fwd.bottom.field_select = default_field_select;

   mv_bwd.top.x = mv_bwd.top.y = mv_bwd.bottom.x = mv_bwd.bottom.y = 0;
   mv_bwd.top.field_select = mv_bwd.bottom.field_select = default_field_select;

   while (1) {
      int macroblock_modes;
      int mba_inc;
      const MBAtab * mba;

      vl_vlc_needbits(&bs->vlc);

      macroblock_modes = get_macroblock_modes(bs, picture);
      dct_type = get_dct_type(bs, picture, macroblock_modes);

      switch(macroblock_modes & (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD)) {
      case (MACROBLOCK_MOTION_FORWARD|MACROBLOCK_MOTION_BACKWARD):
         mv_fwd.top.weight = mv_fwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_HALF;
         mv_bwd.top.weight = mv_bwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_HALF;
         break;

      default:
         mv_fwd.top.field_select = mv_fwd.bottom.field_select = default_field_select;
         mv_bwd.top.field_select = mv_bwd.bottom.field_select = default_field_select;

         /* fall through */
      case MACROBLOCK_MOTION_FORWARD:
         mv_fwd.top.weight = mv_fwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MAX;
         mv_bwd.top.weight = mv_bwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MIN;
         break;

      case MACROBLOCK_MOTION_BACKWARD:
         mv_fwd.top.weight = mv_fwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MIN;
         mv_bwd.top.weight = mv_bwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MAX;
         break;
      }

      /* maybe integrate MACROBLOCK_QUANT test into get_macroblock_modes ? */
      if (macroblock_modes & MACROBLOCK_QUANT)
         quantizer_scale = get_quantizer_scale(bs, picture);

      if (macroblock_modes & MACROBLOCK_INTRA) {

         if (picture->concealment_motion_vectors) {
            if (picture->picture_structure == FRAME_PICTURE)
               motion_fr_conceal(bs, picture->f_code[0], &mv_fwd);
            else
               motion_fi_conceal(bs, picture->f_code[0], &mv_fwd);

         } else {
            mv_fwd.top.x = mv_fwd.top.y = mv_fwd.bottom.x = mv_fwd.bottom.y = 0;
            mv_bwd.top.x = mv_bwd.top.y = mv_bwd.bottom.x = mv_bwd.bottom.y = 0;
         }
         mv_fwd.top.weight = mv_fwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MIN;
         mv_bwd.top.weight = mv_bwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MIN;

         // unravaled loop of 6 block(i) calls in macroblock()
         slice_intra_DCT(bs, picture, intra_quantizer_matrix, 0, x*2+0, y*2+0, dct_type, quantizer_scale, dc_dct_pred);
         slice_intra_DCT(bs, picture, intra_quantizer_matrix, 0, x*2+1, y*2+0, dct_type, quantizer_scale, dc_dct_pred);
         slice_intra_DCT(bs, picture, intra_quantizer_matrix, 0, x*2+0, y*2+1, dct_type, quantizer_scale, dc_dct_pred);
         slice_intra_DCT(bs, picture, intra_quantizer_matrix, 0, x*2+1, y*2+1, dct_type, quantizer_scale, dc_dct_pred);
         slice_intra_DCT(bs, picture, intra_quantizer_matrix, 1, x, y, PIPE_MPEG12_DCT_TYPE_FRAME, quantizer_scale, dc_dct_pred);
         slice_intra_DCT(bs, picture, intra_quantizer_matrix, 2, x, y, PIPE_MPEG12_DCT_TYPE_FRAME, quantizer_scale, dc_dct_pred);

         if (picture->picture_coding_type == D_TYPE) {
            vl_vlc_needbits(&bs->vlc);
            vl_vlc_dumpbits(&bs->vlc, 1);
         }

      } else {
         if (picture->picture_structure == FRAME_PICTURE)
            switch (macroblock_modes & MOTION_TYPE_MASK) {
            case MC_FRAME:
               if (picture->mpeg1) {
                  MOTION_CALL(motion_mp1, macroblock_modes);
               } else {
                  MOTION_CALL(motion_fr_frame, macroblock_modes);
               }
               break;

            case MC_FIELD:
               MOTION_CALL (motion_fr_field, macroblock_modes);
               break;

            case MC_DMV:
               MOTION_CALL (motion_fr_dmv, MACROBLOCK_MOTION_FORWARD);
               break;

            case 0:
               /* non-intra mb without forward mv in a P picture */
               mv_fwd.top.x = mv_fwd.top.y = mv_fwd.bottom.x = mv_fwd.bottom.y = 0;
               mv_bwd.top.x = mv_bwd.top.y = mv_bwd.bottom.x = mv_bwd.bottom.y = 0;
               break;
            }
         else
            switch (macroblock_modes & MOTION_TYPE_MASK) {
            case MC_FIELD:
               MOTION_CALL (motion_fi_field, macroblock_modes);
               break;

            case MC_16X8:
               MOTION_CALL (motion_fi_16x8, macroblock_modes);
               break;

            case MC_DMV:
               MOTION_CALL (motion_fi_dmv, MACROBLOCK_MOTION_FORWARD);
               break;

            case 0:
               /* non-intra mb without forward mv in a P picture */
               mv_fwd.top.x = mv_fwd.top.y = mv_fwd.bottom.x = mv_fwd.bottom.y = 0;
               mv_bwd.top.x = mv_bwd.top.y = mv_bwd.bottom.x = mv_bwd.bottom.y = 0;
               break;
            }

         if (macroblock_modes & MACROBLOCK_PATTERN) {
            int coded_block_pattern = get_coded_block_pattern(bs);

            // TODO  optimize not fully used for idct accel only mc.
            if (coded_block_pattern & 0x20)
               slice_non_intra_DCT(bs, picture, non_intra_quantizer_matrix, 0, x*2+0, y*2+0, dct_type, quantizer_scale); // cc0  luma 0
            if (coded_block_pattern & 0x10)
               slice_non_intra_DCT(bs, picture, non_intra_quantizer_matrix, 0, x*2+1, y*2+0, dct_type, quantizer_scale); // cc0 luma 1
            if (coded_block_pattern & 0x08)
               slice_non_intra_DCT(bs, picture, non_intra_quantizer_matrix, 0, x*2+0, y*2+1, dct_type, quantizer_scale); // cc0 luma 2
            if (coded_block_pattern & 0x04)
               slice_non_intra_DCT(bs, picture, non_intra_quantizer_matrix, 0, x*2+1, y*2+1, dct_type, quantizer_scale); // cc0 luma 3
            if (coded_block_pattern & 0x2)
               slice_non_intra_DCT(bs, picture, non_intra_quantizer_matrix, 1, x, y, PIPE_MPEG12_DCT_TYPE_FRAME, quantizer_scale); // cc1 croma
            if (coded_block_pattern & 0x1)
               slice_non_intra_DCT(bs, picture, non_intra_quantizer_matrix, 2, x, y, PIPE_MPEG12_DCT_TYPE_FRAME, quantizer_scale); // cc2 croma
         }

         dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;
      }

      store_motionvectors(bs, &mv_pos, &mv_fwd, &mv_bwd);
      if (++x >= bs->width) {
         ++y;
         if (y >= bs->height)
            return false;
         x -= bs->width;
      }

      vl_vlc_needbits(&bs->vlc);
      mba_inc = 0;
      while (1) {
         if (bs->vlc.buf >= 0x10000000) {
            mba = MBA_5 + (vl_vlc_ubits(&bs->vlc, 5) - 2);
            break;
         } else if (bs->vlc.buf >= 0x03000000) {
            mba = MBA_11 + (vl_vlc_ubits(&bs->vlc, 11) - 24);
            break;
         } else switch (vl_vlc_ubits(&bs->vlc, 11)) {
         case 8:		/* macroblock_escape */
            mba_inc += 33;
            /* pass through */
         case 15:	/* macroblock_stuffing (MPEG1 only) */
            vl_vlc_dumpbits(&bs->vlc, 11);
            vl_vlc_needbits(&bs->vlc);
            continue;
         default:	/* end of slice, or error */
            return true;
         }
      }
      vl_vlc_dumpbits(&bs->vlc, mba->len);
      mba_inc += mba->mba;
      if (mba_inc) {
         //TODO  conversion to signed format signed format
         dc_dct_pred[0] = dc_dct_pred[1] = dc_dct_pred[2] = 0;

         mv_fwd.top.field_select = mv_fwd.bottom.field_select = default_field_select;
         mv_bwd.top.field_select = mv_bwd.bottom.field_select = default_field_select;

         if (picture->picture_coding_type == P_TYPE) {
            mv_fwd.top.x = mv_fwd.top.y = mv_fwd.bottom.x = mv_fwd.bottom.y = 0;
            mv_fwd.top.weight = mv_fwd.bottom.weight = PIPE_VIDEO_MV_WEIGHT_MAX;
         }

         x += mba_inc;
         do {
            store_motionvectors(bs, &mv_pos, &mv_fwd, &mv_bwd);
         } while (--mba_inc);
      }
      while (x >= bs->width) {
         ++y;
         if (y >= bs->height)
            return false;
         x -= bs->width;
      }
   }
}

void
vl_mpg12_bs_init(struct vl_mpg12_bs *bs, unsigned width, unsigned height)
{
   assert(bs);

   memset(bs, 0, sizeof(struct vl_mpg12_bs));

   bs->width = width;
   bs->height = height;
}

void
vl_mpg12_bs_set_buffers(struct vl_mpg12_bs *bs, struct pipe_ycbcr_block *ycbcr_stream[VL_MAX_PLANES],
                        short *ycbcr_buffer[VL_MAX_PLANES], struct pipe_motionvector *mv_stream[VL_MAX_REF_FRAMES])
{
   unsigned i;

   assert(bs);
   assert(ycbcr_stream && ycbcr_buffer);
   assert(mv_stream);

   for (i = 0; i < VL_MAX_PLANES; ++i) {
      bs->ycbcr_stream[i] = ycbcr_stream[i];
      bs->ycbcr_buffer[i] = ycbcr_buffer[i];
   }
   for (i = 0; i < VL_MAX_REF_FRAMES; ++i)
      bs->mv_stream[i] = mv_stream[i];

   // TODO
   for (i = 0; i < bs->width*bs->height; ++i) {
      bs->mv_stream[0][i].top.x = bs->mv_stream[0][i].top.y = 0;
      bs->mv_stream[0][i].top.field_select = PIPE_VIDEO_FRAME;
      bs->mv_stream[0][i].top.weight = PIPE_VIDEO_MV_WEIGHT_MAX;
      bs->mv_stream[0][i].bottom.x = bs->mv_stream[0][i].bottom.y = 0;
      bs->mv_stream[0][i].bottom.field_select = PIPE_VIDEO_FRAME;
      bs->mv_stream[0][i].bottom.weight = PIPE_VIDEO_MV_WEIGHT_MAX;

      bs->mv_stream[1][i].top.x = bs->mv_stream[1][i].top.y = 0;
      bs->mv_stream[1][i].top.field_select = PIPE_VIDEO_FRAME;
      bs->mv_stream[1][i].top.weight = PIPE_VIDEO_MV_WEIGHT_MIN;
      bs->mv_stream[1][i].bottom.x = bs->mv_stream[1][i].bottom.y = 0;
      bs->mv_stream[1][i].bottom.field_select = PIPE_VIDEO_FRAME;
      bs->mv_stream[1][i].bottom.weight = PIPE_VIDEO_MV_WEIGHT_MIN;
   }
}

void
vl_mpg12_bs_decode(struct vl_mpg12_bs *bs, unsigned num_bytes, const void *buffer,
                   struct pipe_mpeg12_picture_desc *picture, unsigned num_ycbcr_blocks[3])
{
   int intra_quantizer_matrix[64];
   int non_intra_quantizer_matrix[64];

   const int *scan;
   unsigned i;

   assert(bs);
   assert(num_ycbcr_blocks);
   assert(buffer && num_bytes);

   bs->num_ycbcr_blocks = num_ycbcr_blocks;

   vl_vlc_init(&bs->vlc, buffer, num_bytes);

   scan = picture->alternate_scan ? vl_zscan_alternate : vl_zscan_normal;
   for (i = 0; i < 64; ++i) {
      intra_quantizer_matrix[i] = picture->intra_quantizer_matrix[scan[i]];
      non_intra_quantizer_matrix[i] = picture->non_intra_quantizer_matrix[scan[i]];
   }

   while(decode_slice(bs, picture, intra_quantizer_matrix, non_intra_quantizer_matrix));
}
