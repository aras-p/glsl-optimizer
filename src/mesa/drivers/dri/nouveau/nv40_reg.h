#ifndef __NV40_REG_H__
#define __NV40_REG_H__

#define NV40_TX                                            0x00001A00
#define NV40_TX_UNIT(n)                                    (0x1A00 + (n * 32))
/* DWORD 0 - texture address */
/* DWORD 1 */
#  define NV40_TX_MIPMAP_COUNT_SHIFT                    20
#  define NV40_TX_MIPMAP_COUNT_MASK                     (0xF << 20) /* guess */
#  define NV40_TX_NPOT                                  (1 << 13)    /* also set on RECT, even if POT */
#  define NV40_TX_RECTANGLE                             (1 << 14)
#  define NV40_TX_FORMAT_SHIFT                          8
#  define NV40_TX_FORMAT_MASK                           (0x1F << 8) /* *bad* guess */
#    define NV40_TX_FORMAT_L8       0x01
#    define NV40_TX_FORMAT_A1R5G5B5 0x02
#    define NV40_TX_FORMAT_A4R4G4B4 0x03
#    define NV40_TX_FORMAT_R5G6B5   0x04
#    define NV40_TX_FORMAT_A8R8G8B8 0x05
#    define NV40_TX_FORMAT_DXT1     0x06
#    define NV40_TX_FORMAT_DXT3     0x07
#    define NV40_TX_FORMAT_DXT5     0x08
#    define NV40_TX_FORMAT_L16      0x14
#    define NV40_TX_FORMAT_G16R16   0x15    /* possibly wrong */
#    define NV40_TX_FORMAT_A8L8     0x18    /* possibly wrong */
#  define NV40_TX_NCOMP_SHIFT                           4            /* 2=2D, 3=3D*/
#  define NV40_TX_NCOMP_MASK                            (0x3 << 4)   /* possibly wrong */
#  define NV40_TX_CUBIC                                 (1 << 2)
/* DWORD 2
    Need to confirm whether or not "3" is CLAMP or CLAMP_TO_EDGE.  Posts around the
    internet seem to indicate that GL_CLAMP isn't supported on nvidia hardware, and
    GL_CLAMP_TO_EDGE is used instead.
*/
#  define NV40_TX_WRAP_S_SHIFT                          0
#  define NV40_TX_WRAP_S_MASK                           (0xF << 0)
#  define NV40_TX_WRAP_T_SHIFT                          8
#  define NV40_TX_WRAP_T_MASK                           (0xF << 8)
#  define NV40_TX_WRAP_R_SHIFT                          16
#  define NV40_TX_WRAP_R_MASK                           (0xF << 16)
#    define NV40_TX_REPEAT           1
#    define NV40_TX_MIRRORED_REPEAT  2
#    define NV40_TX_CLAMP_TO_EDGE    3
#    define NV40_TX_CLAMP_TO_BORDER  4
#    define NV40_TX_CLAMP            NV40_TX_CLAMP_TO_EDGE
/* DWORD 3 */
/* DWORD 4
    Appears to be related to swizzling of the texture image data into a RGBA value.
    A lot of uncertainty here...
*/
#  define NV40_TX_S0_X_SHIFT                            14
#  define NV40_TX_S0_Y_SHIFT                            12
#  define NV40_TX_S0_Z_SHIFT                            10
#  define NV40_TX_S0_W_SHIFT                            8
#    define NV40_TX_S0_ZERO 0
#    define NV40_TX_S0_ONE  1
#    define NV40_TX_S0_S1   2 /* take value from NV40_TX_S1_* */
#  define NV40_TX_S1_X_SHIFT                            6
#  define NV40_TX_S1_Y_SHIFT                            4
#  define NV40_TX_S1_Z_SHIFT                            2
#  define NV40_TX_S1_W_SHIFT                            0
#    define NV40_TX_S1_X    3
#    define NV40_TX_S1_Y    2
#    define NV40_TX_S1_Z    1
#    define NV40_TX_S1_W    0
/* DWORD 5 */
#  define NV40_TX_MIN_FILTER_SHIFT                      16
#  define NV40_TX_MIN_FILTER_MASK                       (0xF << 16)
#  define NV40_TX_MAG_FILTER_SHIFT                      24
#  define NV40_TX_MAG_FILTER_MASK                       (0xF << 24)
#    define NV40_TX_FILTER_NEAREST                1
#    define NV40_TX_FILTER_LINEAR                 2
#    define NV40_TX_FILTER_NEAREST_MIPMAP_NEAREST 3
#    define NV40_TX_FILTER_LINEAR_MIPMAP_NEAREST  4
#    define NV40_TX_FILTER_NEAREST_MIPMAP_LINEAR  5
#    define NV40_TX_FILTER_LINEAR_MIPMAP_LINEAR   6
/* DWORD 6 */
#  define NV40_TX_WIDTH_SHIFT                           16
#  define NV40_TX_WIDTH_MASK                            (0xFFFF << 16)
#  define NV40_TX_HEIGHT_SHIFT                          0
#  define NV40_TX_HEIGHT_MASK                           (0xFFFF << 0)
/* DWORD 7 */


#define NV40_TX_DEPTH                                   0x1840
#define NV40_TX_DEPTH_UNIT(n)                           (0x1840 + n*4)
#  define NV40_TX_DEPTH_SHIFT                           20
#  define NV40_TX_DEPTH_MASK                            (0xFFF << 20)
#  define NV40_TX_DEPTH_NPOT                            (1 << 7) /* also set for RECT, even if POT */

/* Vertex Program upload / control */
#define NV40_VP_UPLOAD_FROM_ID         0x1E9C /* The next VP_UPLOAD_INST is uploading instruction <n> (guess..) */
#define NV40_VP_PROGRAM_START_ID       0x1EA0 /* Start executing program from instruction <n> */

/* Vertex programs instruction set
 *
 * 128bit opcodes, split into 4 32-bit ones for ease of use.
 *
 * Non-native instructions
 *     ABS - MOV + NV40_VP_INST0_DEST_ABS
 *     POW - EX2 + MUL + LG2
 *     SUB - ADD, second source negated
 *     SWZ - MOV
 *     XPD -  
 *
 * Register access
 *     - Only one INPUT can be accessed per-instruction (move extras into TEMPs)
 *     - Only one CONST can be accessed per-instruction (move extras into TEMPs)
 *
 * Relative Addressing
 *     According to the value returned for MAX_PROGRAM_NATIVE_ADDRESS_REGISTERS_ARB
 *     there are only two address registers available.  The destination in the ARL
 *     instruction is set to TEMP <n> (The temp isn't actually written).
 *
 *     When using vanilla ARB_v_p, the proprietary driver will squish both the available
 *     ADDRESS regs into the first hardware reg in the X and Y components.
 *
 *     To use an address reg as an index into consts, the CONST_SRC is set to
 *     (const_base + offset) and INDEX_CONST is set.
 *
 *     It is similar for inputs, INPUT_SRC is set to the offset value and INDEX_INPUT
 *     is set.
 *
 *     To access the second address reg use ADDR_REG_SELECT_1. A particular component
 *     of the address regs is selected with ADDR_SWZ.
 *
 *     Only one address register can be accessed per instruction, but you may use
 *     the address reg as an index into both consts and inputs in the same instruction
 *     as long as the swizzles also match.
 *
 * Conditional execution (see NV_vertex_program{2,3} for details)
 *     All instructions appear to be able to modify one of two condition code registers.
 *     This is enabled by setting COND_UPDATE_ENABLE.  The second condition registers is
 *     updated by setting COND_REG_SELECT_1.
 *
 *     Conditional execution of an instruction is enabled by setting COND_TEST_ENABLE, and
 *     selecting the condition which will allow the test to pass with COND_{FL,LT,...}.
 *     It is possible to swizzle the values in the condition register, which allows for
 *     testing against an individual component.
 *
 * Branching
 *     The BRA/CAL instructions seem to follow a slightly different opcode layout.  The
 *     destination instruction ID (IADDR) overlaps SRC2.  Instruction ID's seem to be
 *     numbered based on the UPLOAD_FROM_ID FIFO command, and is incremented automatically
 *     on each UPLOAD_INST FIFO command.
 *
 *     Conditional branching is achieved by using the condition tests described above.
 *     There doesn't appear to be dedicated looping instructions, but this can be done
 *     using a temp reg + conditional branching.
 *
 *     Subroutines may be uploaded before the main program itself, but the first executed
 *     instruction is determined by the PROGRAM_START_ID FIFO command.
 *
 * Texture lookup
 *     TODO
 */

/* ---- OPCODE BITS 127:96 / data DWORD 0 --- */
#define NV40_VP_INST0_UNK0                (1 << 30) /* set when writing result regs */
#define NV40_VP_INST_COND_UPDATE_ENABLE   ((1 << 14)|1<<29) /* unsure about this */
#define NV40_VP_INST_INDEX_INPUT          (1 << 27) /* Use an address reg as in index into attribs */
#define NV40_VP_INST_COND_REG_SELECT_1    (1 << 25)
#define NV40_VP_INST_ADDR_REG_SELECT_1    (1 << 24)
#define NV40_VP_INST_DEST_TEMP_ABS        (1 << 21)
#define NV40_VP_INST_DEST_TEMP_SHIFT      15
#define NV40_VP_INST_DEST_TEMP_MASK       (0x3F << 15)
#define NV40_VP_INST_COND_TEST_ENABLE     (1 << 13) /* write masking based on condition test */
#define NV40_VP_INST_COND_SHIFT           10
#define NV40_VP_INST_COND_MASK            (0x7 << 10)
#    define NV40_VP_INST_COND_FL     0
#    define NV40_VP_INST_COND_LT     1
#    define NV40_VP_INST_COND_EQ     2
#    define NV40_VP_INST_COND_LE     3
#    define NV40_VP_INST_COND_GT     4
#    define NV40_VP_INST_COND_NE     5
#    define NV40_VP_INST_COND_GE     6
#    define NV40_VP_INST_COND_TR     7
#define NV40_VP_INST_COND_SWZ_X_SHIFT     8
#define NV40_VP_INST_COND_SWZ_X_MASK      (3 << 8)
#define NV40_VP_INST_COND_SWZ_Y_SHIFT     6
#define NV40_VP_INST_COND_SWZ_Y_MASK      (3 << 6)
#define NV40_VP_INST_COND_SWZ_Z_SHIFT     4
#define NV40_VP_INST_COND_SWZ_Z_MASK      (3 << 4)
#define NV40_VP_INST_COND_SWZ_W_SHIFT     2
#define NV40_VP_INST_COND_SWZ_W_MASK      (3 << 2)
#define NV40_VP_INST_COND_SWZ_ALL_SHIFT   2
#define NV40_VP_INST_COND_SWZ_ALL_MASK    (0xFF << 2)
#define NV40_VP_INST_ADDR_SWZ_SHIFT       0
#define NV40_VP_INST_ADDR_SWZ_MASK        (0x03 << 0)

/* ---- OPCODE BITS 95:64 / data DWORD 1 --- */
#define NV40_VP_INST_OPCODE_SHIFT         22
#define NV40_VP_INST_OPCODE_MASK          (0x3FF << 22)
/*TODO: confirm which source slots correspond to the GL sources,
 *      renouveau should be correct in most places though.. Also,
 *      document them here.
 */
#    define NV40_VP_INST_OP_NOP           0x000
#    define NV40_VP_INST_OP_MOV           0x001
#    define NV40_VP_INST_OP_MUL           0x002
#    define NV40_VP_INST_OP_ADD           0x003
#    define NV40_VP_INST_OP_MAD           0x004
#    define NV40_VP_INST_OP_DP3           0x005
#    define NV40_VP_INST_OP_DP4           0x007
#    define NV40_VP_INST_OP_DPH           0x006
#    define NV40_VP_INST_OP_DST           0x008
#    define NV40_VP_INST_OP_MIN           0x009
#    define NV40_VP_INST_OP_MAX           0x00A
#    define NV40_VP_INST_OP_SLT           0x00B
#    define NV40_VP_INST_OP_SGE           0x00C
#    define NV40_VP_INST_OP_ARL           0x00D
#    define NV40_VP_INST_OP_FRC           0x00E
#    define NV40_VP_INST_OP_FLR           0x00F
#    define NV40_VP_INST_OP_SEQ           0x010
#    define NV40_VP_INST_OP_SFL           0x011
#    define NV40_VP_INST_OP_SGT           0x012
#    define NV40_VP_INST_OP_SLE           0x013
#    define NV40_VP_INST_OP_SNE           0x014
#    define NV40_VP_INST_OP_STR           0x015
#    define NV40_VP_INST_OP_SSG           0x016
#    define NV40_VP_INST_OP_ARR           0x017
#    define NV40_VP_INST_OP_ARA           0x018
#    define NV40_VP_INST_OP_RCP           0x040
#    define NV40_VP_INST_OP_RCC           0x060
#    define NV40_VP_INST_OP_RSQ           0x080
#    define NV40_VP_INST_OP_EXP           0x0A0
#    define NV40_VP_INST_OP_LOG           0x0C0
#    define NV40_VP_INST_OP_LIT           0x0E0
#    define NV40_VP_INST_OP_BRA           0x120
#    define NV40_VP_INST_OP_CAL           0x160
#    define NV40_VP_INST_OP_RET           0x180
#    define NV40_VP_INST_OP_LG2           0x1A0
#    define NV40_VP_INST_OP_EX2           0x1C0
#    define NV40_VP_INST_OP_COS           0x200
#    define NV40_VP_INST_OP_PUSHA         0x260
#    define NV40_VP_INST_OP_POPA          0x280
#define NV40_VP_INST_CONST_SRC_SHIFT      12
#define NV40_VP_INST_CONST_SRC_MASK       (0xFF << 12)
#define NV40_VP_INST_INPUT_SRC_SHIFT      8 
#define NV40_VP_INST_INPUT_SRC_MASK       (0x0F << 8)
#    define NV40_VP_INST_IN_POS           0      /* These seem to match the bindings specified in   */
#    define NV40_VP_INST_IN_WEIGHT        1      /* the ARB_v_p spec (2.14.3.1)                     */
#    define NV40_VP_INST_IN_NORMAL        2      
#    define NV40_VP_INST_IN_COL0          3      /* Should probably confirm them all thougth        */
#    define NV40_VP_INST_IN_COL1          4
#    define NV40_VP_INST_IN_FOGC          5
#    define NV40_VP_INST_IN_TC0           8
#    define NV40_VP_INST_IN_TC(n)         (8+n)
#define NV40_VP_INST_SRC0H_SHIFT          0
#define NV40_VP_INST_SRC0H_MASK           (0xFF << 0)

/* ---- OPCODE BITS 63:32 / data DWORD 2 --- */
#define NV40_VP_INST_SRC0L_SHIFT          23
#define NV40_VP_INST_SRC0L_MASK           (0x1FF << 23)
#define NV40_VP_INST_SRC1_SHIFT           6
#define NV40_VP_INST_SRC1_MASK            (0x1FFFF << 6)
#define NV40_VP_INST_SRC2H_SHIFT          0
#define NV40_VP_INST_SRC2H_MASK           (0x3F << 0)
#define NV40_VP_INST_IADDRH_SHIFT         0
#define NV40_VP_INST_IADDRH_MASK          (0x1F << 0) /* guess, need to test this */
#
/* ---- OPCODE BITS 31:0 / data DWORD 3 --- */
#define NV40_VP_INST_IADDRL_SHIFT         29        
#define NV40_VP_INST_IADDRL_MASK          (7 << 29)
#define NV40_VP_INST_SRC2L_SHIFT          21
#define NV40_VP_INST_SRC2L_MASK           (0x7FF << 21)
/* bits 7-12 seem to always be set to 1 */
#define NV40_VP_INST_WRITEMASK_SHIFT      13
#define NV40_VP_INST_WRITEMASK_MASK       (0xF << 13)
#    define NV40_VP_INST_WRITEMASK_X      (1 << 16)
#    define NV40_VP_INST_WRITEMASK_Y      (1 << 15)
#    define NV40_VP_INST_WRITEMASK_Z      (1 << 14)
#    define NV40_VP_INST_WRITEMASK_W      (1 << 13)
#define NV40_VP_INST_DEST_SHIFT           2
#define NV40_VP_INST_DEST_MASK            (31 << 2)
#    define NV40_VP_INST_DEST_POS         0
#    define NV40_VP_INST_DEST_COL0        1
#    define NV40_VP_INST_DEST_COL1        2
#    define NV40_VP_INST_DEST_BFC0        3
#    define NV40_VP_INST_DEST_BFC1        4
#    define NV40_VP_INST_DEST_FOGC        5
#    define NV40_VP_INST_DEST_PSZ         6
#    define NV40_VP_INST_DEST_TC0         7
#    define NV40_VP_INST_DEST_TC(n)       (7+n)
#    define NV40_VP_INST_DEST_TEMP        0x1F     /* see NV40_VP_INST0_* for actual register */
#define NV40_VP_INST_INDEX_CONST          (1 << 1)
#define NV40_VP_INST_UNK_00               (1 << 0) /* appears to be set on the last inst only */

/* Useful to split the source selection regs into their pieces */
#define NV40_VP_SRC0_HIGH_SHIFT 9
#define NV40_VP_SRC0_HIGH_MASK  0x0001FE00
#define NV40_VP_SRC0_LOW_MASK   0x000001FF
#define NV40_VP_SRC2_HIGH_SHIFT 11
#define NV40_VP_SRC2_HIGH_MASK  0x0001F800
#define NV40_VP_SRC2_LOW_MASK   0x000007FF

/* Source selection - these are the bits you fill NV40_VP_INST_SRCn with */
#define NV40_VP_SRC_NEGATE               16
#define NV40_VP_SRC_SWZ_X_SHIFT          14
#define NV40_VP_SRC_SWZ_X_MASK           (3 << 14)
#define NV40_VP_SRC_SWZ_Y_SHIFT          12
#define NV40_VP_SRC_SWZ_Y_MASK           (3 << 12)
#define NV40_VP_SRC_SWZ_Z_SHIFT          10
#define NV40_VP_SRC_SWZ_Z_MASK           (3 << 10)
#define NV40_VP_SRC_SWZ_W_SHIFT          8
#define NV40_VP_SRC_SWZ_W_MASK           (3 << 8)
#define NV40_VP_SRC_SWZ_ALL_SHIFT        8
#define NV40_VP_SRC_SWZ_ALL_MASK         (0xFF << 8)
#define NV40_VP_SRC_TEMP_SRC_SHIFT       2
#define NV40_VP_SRC_TEMP_SRC_MASK        (0x3F << 2)
#define NV40_VP_SRC_REG_TYPE_SHIFT       0
#define NV40_VP_SRC_REG_TYPE_MASK        (3 << 0)
#    define NV40_VP_SRC_REG_TYPE_UNK0    0
#    define NV40_VP_SRC_REG_TYPE_TEMP    1
#    define NV40_VP_SRC_REG_TYPE_INPUT   2
#    define NV40_VP_SRC_REG_TYPE_CONST   3

/* 
-- GF6800GT - PCIID 10de:0045 (rev a1) --

== Fragment program instruction set
    Not FIFO commands, uploaded into a memory buffer.  The fragment program has
    always appeared in the same map as the texture image data has.  Usually it's
    the first thing in the map, followed immediately by the textures.
*/


/*
 * Each fragment program opcode appears to be comprised of 4 32-bit values.
 *
 *     0 - Opcode, output reg/mask, ATTRIB source
 *     1 - Source 0
 *     2 - Source 1
 *     3 - Source 2
 *
 * Constants are inserted directly after the instruction that uses them.
 * 
 * It appears that it's not possible to use two input registers in one
 * instruction as the input sourcing is done in the instruction dword
 * and not the source selection dwords.  As such instructions such as:
 * 
 *         ADD result.color, fragment.color, fragment.texcoord[0];
 *
 * must be split into two MOV's and then an ADD (nvidia does this) but
 * I'm not sure why it's not just one MOV and then source the second input
 * in the ADD instruction..
 *
 * Negation of the full source is done with NV40_FP_REG_NEGATE, arbitrary
 * negation requires multiplication with a const.
 *
 * Arbitrary swizzling is supported with the exception of SWIZZLE_ZERO/SWIZZLE_ONE
 * The temp/result regs appear to be initialised to (0.0, 0.0, 0.0, 0.0) as SWIZZLE_ZERO
 * is implemented simply by not writing to the relevant components of the destination.
 * 
 * Non-native instructions:
 *  LIT
 *  LRP - MAD+MAD
 *     SUB - ADD, negate second source
 *     RSQ - LG2 + EX2
 *     POW - LG2 + MUL + EX2
 *     SCS - COS + SIN
 *     XPD
 *     DP2 - MUL + ADD
 */
        
//== Opcode / Destination selection ==
#define NV40_FP_OP_PROGRAM_END        0x00000001
#define NV40_FP_OP_OUT_RESULT        (1 << 0)    /* uncertain? and what about depth? */
#define NV40_FP_OP_OUT_REG_SHIFT    1
#define NV40_FP_OP_OUT_REG_MASK    (31 << 1)    /* uncertain */
#define NV40_FP_OP_OUTMASK_SHIFT    9
#define NV40_FP_OP_OUTMASK_MASK    (0xF << 9)
#    define NV40_FP_OP_OUT_X        (1 << 9)
#    define NV40_FP_OP_OUT_Y        (1 << 10)
#    define NV40_FP_OP_OUT_Z        (1 << 11)
#    define NV40_FP_OP_OUT_W        (1 << 12)
/* Uncertain about these, especially the input_src values.. it's possible that
 * they can be dynamically changed.
 */
#define NV40_FP_OP_INPUT_SRC_SHIFT    13
#define NV40_FP_OP_INPUT_SRC_MASK    (15 << 13)
#    define NV40_FP_OP_INPUT_SRC_POSITION    0x0
#    define NV40_FP_OP_INPUT_SRC_COL0        0x1
#    define NV40_FP_OP_INPUT_SRC_COL1        0x2
#    define NV40_FP_OP_INPUT_SRC_TC0        0x4
#    define NV40_FP_OP_INPUT_SRC_TC(n)        (0x4 + n)
#define NV40_FP_OP_TEX_UNIT_SHIFT    17
#define NV40_FP_OP_TEX_UNIT_MASK    (0xF << 17) /* guess */
#define NV40_FP_OP_PRECISION_SHIFT 22
#define NV40_FP_OP_PRECISION_MASK  (3 << 22)
#   define NV40_FP_PRECISION_FP32  0
#   define NV40_FP_PRECISION_FP16  1
#   define NV40_FP_PRECISION_FX12  2
#define NV40_FP_OP_OPCODE_SHIFT    24
#define NV40_FP_OP_OPCODE_MASK        (0x7F << 24)
#    define NV40_FP_OP_OPCODE_MOV    0x01
#    define NV40_FP_OP_OPCODE_MUL    0x02
#    define NV40_FP_OP_OPCODE_ADD    0x03
#    define NV40_FP_OP_OPCODE_MAD    0x04
#    define NV40_FP_OP_OPCODE_DP3    0x05
#    define NV40_FP_OP_OPCODE_DP4    0x06
#    define NV40_FP_OP_OPCODE_DST    0x07
#    define NV40_FP_OP_OPCODE_MIN    0x08
#    define NV40_FP_OP_OPCODE_MAX    0x09
#    define NV40_FP_OP_OPCODE_SLT    0x0A
#    define NV40_FP_OP_OPCODE_SGE    0x0B
#    define NV40_FP_OP_OPCODE_SLE    0x0C
#    define NV40_FP_OP_OPCODE_SGT    0x0D
#    define NV40_FP_OP_OPCODE_SNE    0x0E
#    define NV40_FP_OP_OPCODE_SEQ    0x0F
#    define NV40_FP_OP_OPCODE_FRC    0x10
#    define NV40_FP_OP_OPCODE_FLR    0x11
#    define NV40_FP_OP_OPCODE_TEX    0x17
#    define NV40_FP_OP_OPCODE_TXP    0x18
#    define NV40_FP_OP_OPCODE_RCP    0x1A
#    define NV40_FP_OP_OPCODE_EX2    0x1C
#    define NV40_FP_OP_OPCODE_LG2    0x1D
#    define NV40_FP_OP_OPCODE_COS    0x22
#    define NV40_FP_OP_OPCODE_SIN    0x23
#    define NV40_FP_OP_OPCODE_DP2A  0x2E
#    define NV40_FP_OP_OPCODE_TXB    0x31
#    define NV40_FP_OP_OPCODE_DIV    0x3A
#define NV40_FP_OP_OUT_SAT            (1 << 31)

/* high order bits of SRC0 */
#define NV40_FP_OP_OUT_ABS            (1 << 29)
#define NV40_FP_OP_COND_SWZ_W_SHIFT   27
#define NV40_FP_OP_COND_SWZ_W_MASK    (3 << 27)
#define NV40_FP_OP_COND_SWZ_Z_SHIFT   25
#define NV40_FP_OP_COND_SWZ_Z_MASK    (3 << 25)
#define NV40_FP_OP_COND_SWZ_Y_SHIFT   23
#define NV40_FP_OP_COND_SWZ_Y_MASK    (3 << 23)
#define NV40_FP_OP_COND_SWZ_X_SHIFT   21
#define NV40_FP_OP_COND_SWZ_X_MASK    (3 << 21)
#define NV40_FP_OP_COND_SWZ_ALL_SHIFT 21
#define NV40_FP_OP_COND_SWZ_ALL_MASK  (0xFF << 21)
#define NV40_FP_OP_COND_SHIFT         18
#define NV40_FP_OP_COND_MASK          (0x07 << 18)
#    define NV40_FP_OP_COND_FL     0
#    define NV40_FP_OP_COND_LT     1
#    define NV40_FP_OP_COND_EQ     2
#    define NV40_FP_OP_COND_LE     3
#    define NV40_FP_OP_COND_GT     4
#    define NV40_FP_OP_COND_NE     5
#    define NV40_FP_OP_COND_GE     6
#    define NV40_FP_OP_COND_TR     7

/* high order bits of SRC1 */
#define NV40_FP_OP_SRC_SCALE_SHIFT    28
#define NV40_FP_OP_SRC_SCALE_MASK     (3 << 28)

//== Register selection ==
#define NV40_FP_REG_SRC_INPUT        (1 << 0)    /* uncertain */
#define NV40_FP_REG_SRC_CONST        (1 << 1)
#define NV40_FP_REG_SRC_SHIFT        2            /* uncertain */
#define NV40_FP_REG_SRC_MASK        (31 << 2)
#define NV40_FP_REG_UNK_0            (1 << 8)
#define NV40_FP_REG_SWZ_ALL_SHIFT    9
#define NV40_FP_REG_SWZ_ALL_MASK    (255 << 9)
#define NV40_FP_REG_SWZ_X_SHIFT    9
#define NV40_FP_REG_SWZ_X_MASK        (3 << 9)
#define NV40_FP_REG_SWZ_Y_SHIFT    11
#define NV40_FP_REG_SWZ_Y_MASK        (3 << 11)
#define NV40_FP_REG_SWZ_Z_SHIFT    13
#define NV40_FP_REG_SWZ_Z_MASK        (3 << 13)
#define NV40_FP_REG_SWZ_W_SHIFT    15
#define NV40_FP_REG_SWZ_W_MASK        (3 << 15)
#    define NV40_FP_SWIZZLE_X        0
#    define NV40_FP_SWIZZLE_Y        1
#    define NV40_FP_SWIZZLE_Z        2
#    define NV40_FP_SWIZZLE_W        3
#define NV40_FP_REG_NEGATE            (1 << 17)

#endif
