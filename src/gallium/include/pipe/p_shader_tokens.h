/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * Copyright 2009 VMware, Inc.
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

#ifndef P_SHADER_TOKENS_H
#define P_SHADER_TOKENS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "p_compiler.h"

struct tgsi_version
{
   unsigned MajorVersion  : 8;
   unsigned MinorVersion  : 8;
   unsigned Padding       : 16;
};

struct tgsi_header
{
   unsigned HeaderSize : 8;
   unsigned BodySize   : 24;
};

#define TGSI_PROCESSOR_FRAGMENT  0
#define TGSI_PROCESSOR_VERTEX    1
#define TGSI_PROCESSOR_GEOMETRY  2

struct tgsi_processor
{
   unsigned Processor  : 4;  /* TGSI_PROCESSOR_ */
   unsigned Padding    : 28;
};

#define TGSI_TOKEN_TYPE_DECLARATION    0
#define TGSI_TOKEN_TYPE_IMMEDIATE      1
#define TGSI_TOKEN_TYPE_INSTRUCTION    2

struct tgsi_token
{
   unsigned Type       : 4;  /**< TGSI_TOKEN_TYPE_x */
   unsigned NrTokens   : 8;  /**< UINT */
   unsigned Padding    : 19;
   unsigned Extended   : 1;  /**< BOOL */
};

enum tgsi_file_type {
   TGSI_FILE_NULL        =0,
   TGSI_FILE_CONSTANT    =1,
   TGSI_FILE_INPUT       =2,
   TGSI_FILE_OUTPUT      =3,
   TGSI_FILE_TEMPORARY   =4,
   TGSI_FILE_SAMPLER     =5,
   TGSI_FILE_ADDRESS     =6,
   TGSI_FILE_IMMEDIATE   =7,
   TGSI_FILE_LOOP        =8,
   TGSI_FILE_PREDICATE   =9,
   TGSI_FILE_COUNT      /**< how many TGSI_FILE_ types */
};


#define TGSI_WRITEMASK_NONE     0x00
#define TGSI_WRITEMASK_X        0x01
#define TGSI_WRITEMASK_Y        0x02
#define TGSI_WRITEMASK_XY       0x03
#define TGSI_WRITEMASK_Z        0x04
#define TGSI_WRITEMASK_XZ       0x05
#define TGSI_WRITEMASK_YZ       0x06
#define TGSI_WRITEMASK_XYZ      0x07
#define TGSI_WRITEMASK_W        0x08
#define TGSI_WRITEMASK_XW       0x09
#define TGSI_WRITEMASK_YW       0x0A
#define TGSI_WRITEMASK_XYW      0x0B
#define TGSI_WRITEMASK_ZW       0x0C
#define TGSI_WRITEMASK_XZW      0x0D
#define TGSI_WRITEMASK_YZW      0x0E
#define TGSI_WRITEMASK_XYZW     0x0F

#define TGSI_INTERPOLATE_CONSTANT      0
#define TGSI_INTERPOLATE_LINEAR        1
#define TGSI_INTERPOLATE_PERSPECTIVE   2
#define TGSI_INTERPOLATE_COUNT         3

struct tgsi_declaration
{
   unsigned Type        : 4;  /**< TGSI_TOKEN_TYPE_DECLARATION */
   unsigned NrTokens    : 8;  /**< UINT */
   unsigned File        : 4;  /**< one of TGSI_FILE_x */
   unsigned UsageMask   : 4;  /**< bitmask of TGSI_WRITEMASK_x flags */
   unsigned Interpolate : 4;  /**< one of TGSI_INTERPOLATE_x */
   unsigned Semantic    : 1;  /**< BOOL, any semantic info? */
   unsigned Centroid    : 1;  /**< centroid sampling? */
   unsigned Invariant   : 1;  /**< invariant optimization? */
   unsigned Padding     : 4;
   unsigned Extended    : 1;  /**< BOOL */
};

struct tgsi_declaration_range
{
   unsigned First   : 16; /**< UINT */
   unsigned Last    : 16; /**< UINT */
};

#define TGSI_SEMANTIC_POSITION 0
#define TGSI_SEMANTIC_COLOR    1
#define TGSI_SEMANTIC_BCOLOR   2 /**< back-face color */
#define TGSI_SEMANTIC_FOG      3
#define TGSI_SEMANTIC_PSIZE    4
#define TGSI_SEMANTIC_GENERIC  5
#define TGSI_SEMANTIC_NORMAL   6
#define TGSI_SEMANTIC_FACE     7
#define TGSI_SEMANTIC_COUNT    8 /**< number of semantic values */

struct tgsi_declaration_semantic
{
   unsigned SemanticName   : 8;  /**< one of TGSI_SEMANTIC_x */
   unsigned SemanticIndex  : 16; /**< UINT */
   unsigned Padding        : 8;
};

#define TGSI_IMM_FLOAT32   0

struct tgsi_immediate
{
   unsigned Type       : 4;  /**< TGSI_TOKEN_TYPE_IMMEDIATE */
   unsigned NrTokens   : 8;  /**< UINT */
   unsigned DataType   : 4;  /**< one of TGSI_IMM_x */
   unsigned Padding    : 15;
   unsigned Extended   : 1;  /**< BOOL */
};

union tgsi_immediate_data
{
   float Float;
};

/* TGSI opcodes.  
 * 
 * For more information on semantics of opcodes and
 * which APIs are known to use which opcodes, see
 * auxiliary/tgsi/tgsi-instruction-set.txt
 */
#define TGSI_OPCODE_ARL                 0
#define TGSI_OPCODE_MOV                 1
#define TGSI_OPCODE_LIT                 2
#define TGSI_OPCODE_RCP                 3
#define TGSI_OPCODE_RSQ                 4
#define TGSI_OPCODE_EXP                 5
#define TGSI_OPCODE_LOG                 6
#define TGSI_OPCODE_MUL                 7
#define TGSI_OPCODE_ADD                 8
#define TGSI_OPCODE_DP3                 9
#define TGSI_OPCODE_DP4                 10
#define TGSI_OPCODE_DST                 11
#define TGSI_OPCODE_MIN                 12
#define TGSI_OPCODE_MAX                 13
#define TGSI_OPCODE_SLT                 14
#define TGSI_OPCODE_SGE                 15
#define TGSI_OPCODE_MAD                 16
#define TGSI_OPCODE_SUB                 17
#define TGSI_OPCODE_LRP                 18
#define TGSI_OPCODE_CND                 19
                                /* gap */
#define TGSI_OPCODE_DP2A                21
                                /* gap */
#define TGSI_OPCODE_FRC                 24
#define TGSI_OPCODE_CLAMP               25
#define TGSI_OPCODE_FLR                 26
#define TGSI_OPCODE_ROUND               27
#define TGSI_OPCODE_EX2                 28
#define TGSI_OPCODE_LG2                 29
#define TGSI_OPCODE_POW                 30
#define TGSI_OPCODE_XPD                 31
                                /* gap */
#define TGSI_OPCODE_ABS                 33
#define TGSI_OPCODE_RCC                 34
#define TGSI_OPCODE_DPH                 35
#define TGSI_OPCODE_COS                 36
#define TGSI_OPCODE_DDX                 37
#define TGSI_OPCODE_DDY                 38
#define TGSI_OPCODE_KILP                39  /* predicated kill */
#define TGSI_OPCODE_PK2H                40
#define TGSI_OPCODE_PK2US               41
#define TGSI_OPCODE_PK4B                42
#define TGSI_OPCODE_PK4UB               43
#define TGSI_OPCODE_RFL                 44
#define TGSI_OPCODE_SEQ                 45
#define TGSI_OPCODE_SFL                 46
#define TGSI_OPCODE_SGT                 47
#define TGSI_OPCODE_SIN                 48
#define TGSI_OPCODE_SLE                 49
#define TGSI_OPCODE_SNE                 50
#define TGSI_OPCODE_STR                 51
#define TGSI_OPCODE_TEX                 52
#define TGSI_OPCODE_TXD                 53
#define TGSI_OPCODE_TXP                 54
#define TGSI_OPCODE_UP2H                55
#define TGSI_OPCODE_UP2US               56
#define TGSI_OPCODE_UP4B                57
#define TGSI_OPCODE_UP4UB               58
#define TGSI_OPCODE_X2D                 59
#define TGSI_OPCODE_ARA                 60
#define TGSI_OPCODE_ARR                 61
#define TGSI_OPCODE_BRA                 62
#define TGSI_OPCODE_CAL                 63
#define TGSI_OPCODE_RET                 64
#define TGSI_OPCODE_SSG                 65 /* SGN */
#define TGSI_OPCODE_CMP                 66
#define TGSI_OPCODE_SCS                 67
#define TGSI_OPCODE_TXB                 68
#define TGSI_OPCODE_NRM                 69
#define TGSI_OPCODE_DIV                 70
#define TGSI_OPCODE_DP2                 71
#define TGSI_OPCODE_TXL                 72
#define TGSI_OPCODE_BRK                 73
#define TGSI_OPCODE_IF                  74
#define TGSI_OPCODE_BGNFOR              75
#define TGSI_OPCODE_REP                 76
#define TGSI_OPCODE_ELSE                77
#define TGSI_OPCODE_ENDIF               78
#define TGSI_OPCODE_ENDFOR              79
#define TGSI_OPCODE_ENDREP              80
#define TGSI_OPCODE_PUSHA               81
#define TGSI_OPCODE_POPA                82
#define TGSI_OPCODE_CEIL                83
#define TGSI_OPCODE_I2F                 84
#define TGSI_OPCODE_NOT                 85
#define TGSI_OPCODE_TRUNC               86
#define TGSI_OPCODE_SHL                 87
#define TGSI_OPCODE_SHR                 88
#define TGSI_OPCODE_AND                 89
#define TGSI_OPCODE_OR                  90
#define TGSI_OPCODE_MOD                 91
#define TGSI_OPCODE_XOR                 92
#define TGSI_OPCODE_SAD                 93
#define TGSI_OPCODE_TXF                 94
#define TGSI_OPCODE_TXQ                 95
#define TGSI_OPCODE_CONT                96
#define TGSI_OPCODE_EMIT                97
#define TGSI_OPCODE_ENDPRIM             98
#define TGSI_OPCODE_BGNLOOP             99
#define TGSI_OPCODE_BGNSUB              100
#define TGSI_OPCODE_ENDLOOP             101
#define TGSI_OPCODE_ENDSUB              102
                                /* gap */
#define TGSI_OPCODE_NOP                 107
                                /* gap */
#define TGSI_OPCODE_NRM4                112
#define TGSI_OPCODE_CALLNZ              113
#define TGSI_OPCODE_IFC                 114
#define TGSI_OPCODE_BREAKC              115
#define TGSI_OPCODE_KIL                 116  /* conditional kill */
#define TGSI_OPCODE_END                 117  /* aka HALT */
                                /* gap */
#define TGSI_OPCODE_LAST                119

#define TGSI_SAT_NONE            0  /* do not saturate */
#define TGSI_SAT_ZERO_ONE        1  /* clamp to [0,1] */
#define TGSI_SAT_MINUS_PLUS_ONE  2  /* clamp to [-1,1] */

/**
 * Opcode is the operation code to execute. A given operation defines the
 * semantics how the source registers (if any) are interpreted and what is
 * written to the destination registers (if any) as a result of execution.
 *
 * NumDstRegs and NumSrcRegs is the number of destination and source registers,
 * respectively. For a given operation code, those numbers are fixed and are
 * present here only for convenience.
 *
 * If Extended is TRUE, it is now executed.
 *
 * Saturate controls how are final results in destination registers modified.
 */

struct tgsi_instruction
{
   unsigned Type       : 4;  /* TGSI_TOKEN_TYPE_INSTRUCTION */
   unsigned NrTokens   : 8;  /* UINT */
   unsigned Opcode     : 8;  /* TGSI_OPCODE_ */
   unsigned Saturate   : 2;  /* TGSI_SAT_ */
   unsigned NumDstRegs : 2;  /* UINT */
   unsigned NumSrcRegs : 4;  /* UINT */
   unsigned Padding    : 3;
   unsigned Extended   : 1;  /* BOOL */
};

/*
 * If tgsi_instruction::Extended is TRUE, tgsi_instruction_ext follows.
 * 
 * Then, tgsi_instruction::NumDstRegs of tgsi_dst_register follow.
 * 
 * Then, tgsi_instruction::NumSrcRegs of tgsi_src_register follow.
 *
 * tgsi_instruction::NrTokens contains the total number of words that make the
 * instruction, including the instruction word.
 */

#define TGSI_INSTRUCTION_EXT_TYPE_LABEL     1
#define TGSI_INSTRUCTION_EXT_TYPE_TEXTURE   2
#define TGSI_INSTRUCTION_EXT_TYPE_PREDICATE 3

struct tgsi_instruction_ext
{
   unsigned Type       : 4;  /* TGSI_INSTRUCTION_EXT_TYPE_ */
   unsigned Padding    : 27;
   unsigned Extended   : 1;  /* BOOL */
};

/*
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_LABEL, it
 * should be cast to tgsi_instruction_ext_label.
 * 
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_TEXTURE, it
 * should be cast to tgsi_instruction_ext_texture.
 * 
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_PREDICATE, it
 * should be cast to tgsi_instruction_ext_predicate.
 * 
 * If tgsi_instruction_ext::Extended is TRUE, another tgsi_instruction_ext
 * follows.
 */

#define TGSI_SWIZZLE_X      0
#define TGSI_SWIZZLE_Y      1
#define TGSI_SWIZZLE_Z      2
#define TGSI_SWIZZLE_W      3

struct tgsi_instruction_ext_label
{
   unsigned Type     : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_LABEL */
   unsigned Label    : 24;   /* UINT */
   unsigned Padding  : 3;
   unsigned Extended : 1;    /* BOOL */
};

#define TGSI_TEXTURE_UNKNOWN        0
#define TGSI_TEXTURE_1D             1
#define TGSI_TEXTURE_2D             2
#define TGSI_TEXTURE_3D             3
#define TGSI_TEXTURE_CUBE           4
#define TGSI_TEXTURE_RECT           5
#define TGSI_TEXTURE_SHADOW1D       6
#define TGSI_TEXTURE_SHADOW2D       7
#define TGSI_TEXTURE_SHADOWRECT     8
#define TGSI_TEXTURE_COUNT          9

struct tgsi_instruction_ext_texture
{
   unsigned Type     : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_TEXTURE */
   unsigned Texture  : 8;    /* TGSI_TEXTURE_ */
   unsigned Padding  : 19;
   unsigned Extended : 1;    /* BOOL */
};

/*
 * For SM3, the following constraint applies.
 *   - Swizzle is either set to identity or replicate.
 */
struct tgsi_instruction_ext_predicate
{
   unsigned Type     : 4;  /* TGSI_INSTRUCTION_EXT_TYPE_PREDICATE */
   unsigned SwizzleX : 2;  /* TGSI_SWIZZLE_x */
   unsigned SwizzleY : 2;  /* TGSI_SWIZZLE_x */
   unsigned SwizzleZ : 2;  /* TGSI_SWIZZLE_x */
   unsigned SwizzleW : 2;  /* TGSI_SWIZZLE_x */
   unsigned Negate   : 1;  /* BOOL */
   unsigned SrcIndex : 8;  /* UINT */
   unsigned Padding  : 10;
   unsigned Extended : 1;  /* BOOL */
};

/**
 * File specifies the register array to access.
 *
 * Index specifies the element number of a register in the register file.
 *
 * If Indirect is TRUE, Index should be offset by the X component of a source
 * register that follows. The register can be now fetched into local storage
 * for further processing.
 *
 * If Negate is TRUE, all components of the fetched register are negated.
 *
 * The fetched register components are swizzled according to SwizzleX, SwizzleY,
 * SwizzleZ and SwizzleW.
 *
 * If Extended is TRUE, any further modifications to the source register are
 * made to this temporary storage.
 */

struct tgsi_src_register
{
   unsigned File        : 4;  /* TGSI_FILE_ */
   unsigned SwizzleX    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleY    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleZ    : 2;  /* TGSI_SWIZZLE_ */
   unsigned SwizzleW    : 2;  /* TGSI_SWIZZLE_ */
   unsigned Negate      : 1;  /* BOOL */
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Dimension   : 1;  /* BOOL */
   int      Index       : 16; /* SINT */
   unsigned Extended    : 1;  /* BOOL */
};

/**
 * If tgsi_src_register::Extended is TRUE, tgsi_src_register_ext follows.
 * 
 * Then, if tgsi_src_register::Indirect is TRUE, another tgsi_src_register
 * follows.
 *
 * Then, if tgsi_src_register::Dimension is TRUE, tgsi_dimension follows.
 */

#define TGSI_SRC_REGISTER_EXT_TYPE_MOD      1

struct tgsi_src_register_ext
{
   unsigned Type     : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_ */
   unsigned Padding  : 27;
   unsigned Extended : 1;    /* BOOL */
};

/**
 * If tgsi_src_register_ext::Type is TGSI_SRC_REGISTER_EXT_TYPE_MOD,
 * it should be cast to tgsi_src_register_ext_mod.
 * 
 * If tgsi_dst_register_ext::Extended is TRUE, another tgsi_dst_register_ext
 * follows.
 */


/**
 * Extra src register modifiers
 *
 * If Complement is TRUE, the source register is modified by subtracting it
 * from 1.0.
 *
 * If Bias is TRUE, the source register is modified by subtracting 0.5 from it.
 *
 * If Scale2X is TRUE, the source register is modified by multiplying it by 2.0.
 *
 * If Absolute is TRUE, the source register is modified by removing the sign.
 *
 * If Negate is TRUE, the source register is modified by negating it.
 */

struct tgsi_src_register_ext_mod
{
   unsigned Type         : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_MOD */
   unsigned Complement   : 1;    /* BOOL */
   unsigned Bias         : 1;    /* BOOL */
   unsigned Scale2X      : 1;    /* BOOL */
   unsigned Absolute     : 1;    /* BOOL */
   unsigned Negate       : 1;    /* BOOL */
   unsigned Padding      : 22;
   unsigned Extended     : 1;    /* BOOL */
};

struct tgsi_dimension
{
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Dimension   : 1;  /* BOOL */
   unsigned Padding     : 13;
   int      Index       : 16; /* SINT */
   unsigned Extended    : 1;  /* BOOL */
};

struct tgsi_dst_register
{
   unsigned File        : 4;  /* TGSI_FILE_ */
   unsigned WriteMask   : 4;  /* TGSI_WRITEMASK_ */
   unsigned Indirect    : 1;  /* BOOL */
   unsigned Dimension   : 1;  /* BOOL */
   int      Index       : 16; /* SINT */
   unsigned Padding     : 5;
   unsigned Extended    : 1;  /* BOOL */
};

/*
 * If tgsi_dst_register::Extended is TRUE, tgsi_dst_register_ext follows.
 * 
 * Then, if tgsi_dst_register::Indirect is TRUE, tgsi_src_register follows.
 */

#define TGSI_DST_REGISTER_EXT_TYPE_MODULATE     1

struct tgsi_dst_register_ext
{
   unsigned Type     : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_ */
   unsigned Padding  : 27;
   unsigned Extended : 1;    /* BOOL */
};

/**
 * Extra destination register modifiers
 *
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_MODULATE,
 * it should be cast to tgsi_dst_register_ext_modulate.
 * 
 * If tgsi_dst_register_ext::Extended is TRUE, another tgsi_dst_register_ext
 * follows.
 */

#define TGSI_MODULATE_1X        0
#define TGSI_MODULATE_2X        1
#define TGSI_MODULATE_4X        2
#define TGSI_MODULATE_8X        3
#define TGSI_MODULATE_HALF      4
#define TGSI_MODULATE_QUARTER   5
#define TGSI_MODULATE_EIGHTH    6
#define TGSI_MODULATE_COUNT     7

struct tgsi_dst_register_ext_modulate
{
   unsigned Type     : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_MODULATE */
   unsigned Modulate : 4;    /* TGSI_MODULATE_ */
   unsigned Padding  : 23;
   unsigned Extended : 1;    /* BOOL */
};

#ifdef __cplusplus
}
#endif

#endif /* P_SHADER_TOKENS_H */
