#if !defined TGSI_TOKEN_H
#define TGSI_TOKEN_H

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

struct tgsi_version
{
   GLuint MajorVersion  : 8;
   GLuint MinorVersion  : 8;
   GLuint Padding       : 16;
};

struct tgsi_header
{
   GLuint HeaderSize : 8;
   GLuint BodySize   : 24;
};

#define TGSI_PROCESSOR_FRAGMENT  0
#define TGSI_PROCESSOR_VERTEX    1
#define TGSI_PROCESSOR_GEOMETRY  2

struct tgsi_processor
{
   GLuint Processor  : 4;  /* TGSI_PROCESSOR_ */
   GLuint Padding    : 28;
};

#define TGSI_TOKEN_TYPE_DECLARATION    0
#define TGSI_TOKEN_TYPE_IMMEDIATE      1
#define TGSI_TOKEN_TYPE_INSTRUCTION    2

struct tgsi_token
{
   GLuint Type       : 4;  /* TGSI_TOKEN_TYPE_ */
   GLuint Size       : 8;  /* UINT */
   GLuint Padding    : 19;
   GLuint Extended   : 1;  /* BOOL */
};

#define TGSI_FILE_NULL        0
#define TGSI_FILE_CONSTANT    1
#define TGSI_FILE_INPUT       2
#define TGSI_FILE_OUTPUT      3
#define TGSI_FILE_TEMPORARY   4
#define TGSI_FILE_SAMPLER     5
#define TGSI_FILE_ADDRESS     6
#define TGSI_FILE_IMMEDIATE   7

#define TGSI_DECLARE_RANGE    0
#define TGSI_DECLARE_MASK     1

struct tgsi_declaration
{
   GLuint Type          : 4;  /* TGSI_TOKEN_TYPE_DECLARATION */
   GLuint Size          : 8;  /* UINT */
   GLuint File          : 4;  /* TGSI_FILE_ */
   GLuint Declare       : 4;  /* TGSI_DECLARE_ */
   GLuint Interpolate   : 1;  /* BOOL */
   GLuint Padding       : 10;
   GLuint Extended      : 1;  /* BOOL */
};

struct tgsi_declaration_range
{
   GLuint First   : 16; /* UINT */
   GLuint Last    : 16; /* UINT */
};

struct tgsi_declaration_mask
{
   GLuint Mask : 32; /* UINT */
};

#define TGSI_INTERPOLATE_CONSTANT      0
#define TGSI_INTERPOLATE_LINEAR        1
#define TGSI_INTERPOLATE_PERSPECTIVE   2

struct tgsi_declaration_interpolation
{
   GLuint Interpolate   : 4;  /* TGSI_INTERPOLATE_ */
   GLuint Padding       : 28;
};

#define TGSI_IMM_FLOAT32   0

struct tgsi_immediate
{
   GLuint Type       : 4;  /* TGSI_TOKEN_TYPE_IMMEDIATE */
   GLuint Size       : 8;  /* UINT */
   GLuint DataType   : 4;  /* TGSI_IMM_ */
   GLuint Padding    : 15;
   GLuint Extended   : 1;  /* BOOL */
};

struct tgsi_immediate_float32
{
   GLfloat Float;
};

/*
 * GL_NV_vertex_program
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

/*
 * GL_ATI_fragment_shader
 */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_MUL */
#define TGSI_OPCODE_SUB                 17
#define TGSI_OPCODE_DOT3                TGSI_OPCODE_DP3
#define TGSI_OPCODE_DOT4                TGSI_OPCODE_DP4
/* TGSI_OPCODE_MAD */
#define TGSI_OPCODE_LERP                18
#define TGSI_OPCODE_CND                 19
#define TGSI_OPCODE_CND0                20
#define TGSI_OPCODE_DOT2ADD             21

/*
 * GL_EXT_vertex_shader
 */
#define TGSI_OPCODE_INDEX               22
#define TGSI_OPCODE_NEGATE              23
/* TGSI_OPCODE_DOT3 */
/* TGSI_OPCODE_DOT4 */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_ADD */
#define TGSI_OPCODE_MADD                TGSI_OPCODE_MAD
#define TGSI_OPCODE_FRAC                24
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
#define TGSI_OPCODE_SETGE               TGSI_OPCODE_SGE
#define TGSI_OPCODE_SETLT               TGSI_OPCODE_SLT
#define TGSI_OPCODE_CLAMP               25
#define TGSI_OPCODE_FLOOR               26
#define TGSI_OPCODE_ROUND               27
#define TGSI_OPCODE_EXPBASE2            28
#define TGSI_OPCODE_LOGBASE2            29
#define TGSI_OPCODE_POWER               30
#define TGSI_OPCODE_RECIP               TGSI_OPCODE_RCP
#define TGSI_OPCODE_RECIPSQRT           TGSI_OPCODE_RSQ
/* TGSI_OPCODE_SUB */
#define TGSI_OPCODE_CROSSPRODUCT        31
#define TGSI_OPCODE_MULTIPLYMATRIX      32
/* TGSI_OPCODE_MOV */

/*
 * GL_NV_vertex_program1_1
 */
/* TGSI_OPCODE_ARL */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_LIT */
#define TGSI_OPCODE_ABS                 33
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_EXP */
/* TGSI_OPCODE_LOG */
#define TGSI_OPCODE_RCC                 34
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SGE */
#define TGSI_OPCODE_DPH                 35
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */

/*
 * GL_NV_fragment_program
 */
/* TGSI_OPCODE_ADD */
#define TGSI_OPCODE_COS                 36
#define TGSI_OPCODE_DDX                 37
#define TGSI_OPCODE_DDY                 38
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DST */
#define TGSI_OPCODE_EX2                 TGSI_OPCODE_EXPBASE2
#define TGSI_OPCODE_FLR                 TGSI_OPCODE_FLOOR
#define TGSI_OPCODE_FRC                 TGSI_OPCODE_FRAC
#define TGSI_OPCODE_KIL                 39
#define TGSI_OPCODE_LG2                 TGSI_OPCODE_LOGBASE2
/* TGSI_OPCODE_LIT */
#define TGSI_OPCODE_LRP                 TGSI_OPCODE_LERP
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_MUL */
#define TGSI_OPCODE_PK2H                40
#define TGSI_OPCODE_PK2US               41
#define TGSI_OPCODE_PK4B                42
#define TGSI_OPCODE_PK4UB               43
#define TGSI_OPCODE_POW                 TGSI_OPCODE_POWER
/* TGSI_OPCODE_RCP */
#define TGSI_OPCODE_RFL                 44
/* TGSI_OPCODE_RSQ */
#define TGSI_OPCODE_SEQ                 45
#define TGSI_OPCODE_SFL                 46
/* TGSI_OPCODE_SGE */
#define TGSI_OPCODE_SGT                 47
#define TGSI_OPCODE_SIN                 48
#define TGSI_OPCODE_SLE                 49
/* TGSI_OPCODE_SLT */
#define TGSI_OPCODE_SNE                 50
#define TGSI_OPCODE_STR                 51
/* TGSI_OPCODE_SUB */
#define TGSI_OPCODE_TEX                 52
#define TGSI_OPCODE_TXD                 53
#define TGSI_OPCODE_TXP                 132
#define TGSI_OPCODE_UP2H                54
#define TGSI_OPCODE_UP2US               55
#define TGSI_OPCODE_UP4B                56
#define TGSI_OPCODE_UP4UB               57
#define TGSI_OPCODE_X2D                 58

/*
 * GL_NV_vertex_program2
 */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_ADD */
#define TGSI_OPCODE_ARA                 59
/* TGSI_OPCODE_ARL */
#define TGSI_OPCODE_ARR                 60
#define TGSI_OPCODE_BRA                 61
#define TGSI_OPCODE_CAL                 62
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_EXP */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_LOG */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_RCC */
/* TGSI_OPCODE_RCP */
#define TGSI_OPCODE_RET                 63
/* TGSI_OPCODE_RSQNV - use TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SFL */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SNE */
#define TGSI_OPCODE_SSG                 64
/* TGSI_OPCODE_STR */
/* TGSI_OPCODE_SUB */

/*
 * GL_ARB_vertex_program
 */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_ARL */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_EXP */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_LOG */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SUB */
#define TGSI_OPCODE_SWZ                 TGSI_OPCODE_MOV
#define TGSI_OPCODE_XPD                 TGSI_OPCODE_CROSSPRODUCT

/*
 * GL_ARB_fragment_program
 */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_ADD */
#define TGSI_OPCODE_CMP                 65
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
#define TGSI_OPCODE_SCS                 66
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_SWZ */
/* TGSI_OPCODE_XPD */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TXP */
#define TGSI_OPCODE_TXB                 67
/* TGSI_OPCODE_KIL */

/*
 * GL_NV_fragment_program_option
 */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_DDX */
/* TGSI_OPCODE_DDY */
/* TGSI_OPCODE_PK2H */
/* TGSI_OPCODE_PK2US */
/* TGSI_OPCODE_PK4B */
/* TGSI_OPCODE_PK4UB */
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_SCS */
/* TGSI_OPCODE_UP2H */
/* TGSI_OPCODE_UP2US */
/* TGSI_OPCODE_UP4B */
/* TGSI_OPCODE_UP4UB */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_XPD */
/* TGSI_OPCODE_RFL */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SFL */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SNE */
/* TGSI_OPCODE_STR */
/* TGSI_OPCODE_CMP */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_X2D */
/* TGSI_OPCODE_SWZ */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TXP */
/* TGSI_OPCODE_TXB */
/* TGSI_OPCODE_KIL */
/* TGSI_OPCODE_TXD */

/*
 * GL_NV_fragment_program2
 */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_DDX */
/* TGSI_OPCODE_DDY */
/* TGSI_OPCODE_PK2H */
/* TGSI_OPCODE_PK2US */
/* TGSI_OPCODE_PK4B */
/* TGSI_OPCODE_PK4UB */
#define TGSI_OPCODE_NRM                 68
#define TGSI_OPCODE_DIV                 69
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_SCS */
/* TGSI_OPCODE_UP2H */
/* TGSI_OPCODE_UP2US */
/* TGSI_OPCODE_UP4B */
/* TGSI_OPCODE_UP4UB */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_XPD */
/* TGSI_OPCODE_RFL */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SFL */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SNE */
/* TGSI_OPCODE_STR */
#define TGSI_OPCODE_DP2                 70
/* TGSI_OPCODE_CMP */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_X2D */
#define TGSI_OPCODE_DP2A                TGSI_OPCODE_DOT2ADD
/* TGSI_OPCODE_SWZ */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TXP */
/* TGSI_OPCODE_TXB */
#define TGSI_OPCODE_TXL                 71
/* TGSI_OPCODE_KIL */
/* TGSI_OPCODE_TXD */
/* TGSI_OPCODE_CAL */
/* TGSI_OPCODE_RET */
#define TGSI_OPCODE_BRK                 72
#define TGSI_OPCODE_IF                  73
#define TGSI_OPCODE_LOOP                74
#define TGSI_OPCODE_REP                 75
#define TGSI_OPCODE_ELSE                76
#define TGSI_OPCODE_ENDIF               77
#define TGSI_OPCODE_ENDLOOP             78
#define TGSI_OPCODE_ENDREP              79

/*
 * GL_NV_vertex_program2_option
 */
/* TGSI_OPCODE_ARL */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_SSG */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_EXP */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LOG */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_RCC */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_XPD */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SFL */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SNE */
/* TGSI_OPCODE_STR */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_SWZ */
/* TGSI_OPCODE_ARR */
/* TGSI_OPCODE_ARA */
/* TGSI_OPCODE_BRA */
/* TGSI_OPCODE_CAL */
/* TGSI_OPCODE_RET */

/*
 * GL_NV_vertex_program3
 */
/* TGSI_OPCODE_ARL */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_SSG */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_EXP */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LOG */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_RCC */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_XPD */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SFL */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SNE */
/* TGSI_OPCODE_STR */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_SWZ */
/* TGSI_OPCODE_ARR */
/* TGSI_OPCODE_ARA */
/* TGSI_OPCODE_BRA */
/* TGSI_OPCODE_CAL */
/* TGSI_OPCODE_RET */
#define TGSI_OPCODE_PUSHA               80
#define TGSI_OPCODE_POPA                81
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TXP */
/* TGSI_OPCODE_TXB */
/* TGSI_OPCODE_TXL */

/*
 * GL_NV_gpu_program4
 */
/* TGSI_OPCODE_ABS */
#define TGSI_OPCODE_CEIL                82
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
#define TGSI_OPCODE_I2F                 83
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_MOV */
#define TGSI_OPCODE_NOT                 84
/* TGSI_OPCODE_NRM */
/* TGSI_OPCODE_PK2H */
/* TGSI_OPCODE_PK2US */
/* TGSI_OPCODE_PK4B */
/* TGSI_OPCODE_PK4UB */
/* TGSI_OPCODE_ROUND */
/* TGSI_OPCODE_SSG */
#define TGSI_OPCODE_TRUNC               85
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_RCC */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_SCS */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_UP2H */
/* TGSI_OPCODE_UP2US */
/* TGSI_OPCODE_UP4B */
/* TGSI_OPCODE_UP4UB */
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_DIV */
#define TGSI_OPCODE_SHL                 86
#define TGSI_OPCODE_SHR                 87
/* TGSI_OPCODE_ADD */
#define TGSI_OPCODE_AND                 88
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_DPH */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MUL */
#define TGSI_OPCODE_OR                  89
/* TGSI_OPCODE_RFL */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SFL */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SNE */
/* TGSI_OPCODE_STR */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_XPD */
/* TGSI_OPCODE_DP2 */
#define TGSI_OPCODE_MOD                 90
#define TGSI_OPCODE_XOR                 91
/* TGSI_OPCODE_CMP */
/* TGSI_OPCODE_DP2A */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_MAD */
#define TGSI_OPCODE_SAD                 92
/* TGSI_OPCODE_X2D */
/* TGSI_OPCODE_SWZ */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TXB */
#define TGSI_OPCODE_TXF                 93
/* TGSI_OPCODE_TXL */
/* TGSI_OPCODE_TXP */
#define TGSI_OPCODE_TXQ                 94
/* TGSI_OPCODE_TXD */
/* TGSI_OPCODE_CAL */
/* TGSI_OPCODE_RET */
/* TGSI_OPCODE_BRK */
#define TGSI_OPCODE_CONT                95
/* TGSI_OPCODE_IF */
/* TGSI_OPCODE_REP */
/* TGSI_OPCODE_ELSE */
/* TGSI_OPCODE_ENDIF */
/* TGSI_OPCODE_ENDREP */

/*
 * GL_NV_vertex_program4
 */
/* Same as GL_NV_gpu_program4 */

/*
 * GL_NV_fragment_program4
 */
/* Same as GL_NV_gpu_program4 */
/* TGSI_OPCODE_KIL */
/* TGSI_OPCODE_DDX */
/* TGSI_OPCODE_DDY */

/*
 * GL_NV_geometry_program4
 */
/* Same as GL_NV_gpu_program4 */
#define TGSI_OPCODE_EMIT                96
#define TGSI_OPCODE_ENDPRIM             97

/*
 * GLSL
 */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_ADD */
#define TGSI_OPCODE_BGNLOOP2            98
#define TGSI_OPCODE_BGNSUB              99
/* TGSI_OPCODE_BRA */
/* TGSI_OPCODE_BRK */
/* TGSI_OPCODE_CONT */
/* TGSI_OPCODE_COS */
/* TGSI_OPCODE_DDX */
/* TGSI_OPCODE_DDY */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_ELSE */
/* TGSI_OPCODE_ENDIF */
#define TGSI_OPCODE_ENDLOOP2            100
#define TGSI_OPCODE_ENDSUB              101
/* TGSI_OPCODE_EX2 */
/* TGSI_OPCODE_EXP */
/* TGSI_OPCODE_FLR */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_IF */
#define TGSI_OPCODE_INT                 TGSI_OPCODE_TRUNC
/* TGSI_OPCODE_KIL */
/* TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LOG */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_MUL */
#define TGSI_OPCODE_NOISE1              102
#define TGSI_OPCODE_NOISE2              103
#define TGSI_OPCODE_NOISE3              104
#define TGSI_OPCODE_NOISE4              105
#define TGSI_OPCODE_NOP                 106
/* TGSI_OPCODE_POW */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SIN */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SNE */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TXB */
/* TGSI_OPCODE_TXD */
/* TGSI_OPCODE_TXL */
/* TGSI_OPCODE_TXP */
/* TGSI_OPCODE_XPD */

/*
 * ps_1_1
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_LRP */
#define TGSI_OPCODE_TEXCOORD            TGSI_OPCODE_NOP
#define TGSI_OPCODE_TEXKILL             TGSI_OPCODE_KIL
/* TGSI_OPCODE_TEX */
#define TGSI_OPCODE_TEXBEM              107
#define TGSI_OPCODE_TEXBEML             108
#define TGSI_OPCODE_TEXREG2AR           109
#define TGSI_OPCODE_TEXM3X2PAD          110
#define TGSI_OPCODE_TEXM3X2TEX          111
#define TGSI_OPCODE_TEXM3X3PAD          112
#define TGSI_OPCODE_TEXM3X3TEX          113
#define TGSI_OPCODE_TEXM3X3SPEC         114
#define TGSI_OPCODE_TEXM3X3VSPEC        115
/* TGSI_OPCODE_CND */

/*
 * ps_1_2
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_TEXCOORD */
/* TGSI_OPCODE_TEXKILL */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TEXBEM */
/* TGSI_OPCODE_TEXBEML */
/* TGSI_OPCODE_TEXREG2AR */
#define TGSI_OPCODE_TEXREG2GB           116
/* TGSI_OPCODE_TEXM3X2PAD */
/* TGSI_OPCODE_TEXM3X2TEX */
/* TGSI_OPCODE_TEXM3X3PAD */
/* TGSI_OPCODE_TEXM3X3TEX */
/* TGSI_OPCODE_TEXM3X3SPEC */
/* TGSI_OPCODE_TEXM3X3VSPEC */
/* TGSI_OPCODE_CND */
#define TGSI_OPCODE_TEXREG2RGB          117
#define TGSI_OPCODE_TEXDP3TEX           118
#define TGSI_OPCODE_TEXDP3              119
#define TGSI_OPCODE_TEXM3X3             120
/* CMP - use TGSI_OPCODE_CND0 */

/*
 * ps_1_3
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_TEXCOORD */
/* TGSI_OPCODE_TEXKILL */
/* TGSI_OPCODE_TEX */
/* TGSI_OPCODE_TEXBEM */
/* TGSI_OPCODE_TEXBEML */
/* TGSI_OPCODE_TEXREG2AR */
/* TGSI_OPCODE_TEXREG2GB */
/* TGSI_OPCODE_TEXM3X2PAD */
/* TGSI_OPCODE_TEXM3X2TEX */
/* TGSI_OPCODE_TEXM3X3PAD */
/* TGSI_OPCODE_TEXM3X3TEX */
/* TGSI_OPCODE_TEXM3X3SPEC */
/* TGSI_OPCODE_TEXM3X3VSPEC */
/* TGSI_OPCODE_CND */
/* TGSI_OPCODE_TEXREG2RGB */
/* TGSI_OPCODE_TEXDP3TEX */
#define TGSI_OPCODE_TEXM3X2DEPTH        121
/* TGSI_OPCODE_TEXDP3 */
/* TGSI_OPCODE_TEXM3X3 */
/* CMP - use TGSI_OPCODE_CND0 */

/*
 * ps_1_4
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_LRP */
#define TGSI_OPCODE_TEXCRD              TGSI_OPCODE_TEXCOORD
/* TGSI_OPCODE_TEXKILL */
#define TGSI_OPCODE_TEXLD               TGSI_OPCODE_TEX
/* TGSI_OPCODE_CND */
#define TGSI_OPCODE_TEXDEPTH            122
/* CMP - use TGSI_OPCODE_CND0 */
#define TGSI_OPCODE_BEM                 123

/*
 * ps_2_0
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */   /* XXX: takes ABS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MAX */
/* EXP - use TGSI_OPCODE_EX2 */
/* LOG - use TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_FRC */
#define TGSI_OPCODE_M4X4                TGSI_OPCODE_MULTIPLYMATRIX
#define TGSI_OPCODE_M4X3                124
#define TGSI_OPCODE_M3X4                125
#define TGSI_OPCODE_M3X3                126
#define TGSI_OPCODE_M3X2                127
/* TGSI_OPCODE_POW */   /* XXX: takes ABS */
#define TGSI_OPCODE_CRS                 TGSI_OPCODE_XPD
/* TGSI_OPCODE_ABS */
#define TGSI_OPCODE_NRM4                128
#define TGSI_OPCODE_SINCOS              TGSI_OPCODE_SCS
/* TGSI_OPCODE_TEXKILL */
/* TGSI_OPCODE_TEXLD */
#define TGSI_OPCODE_TEXLDB              TGSI_OPCODE_TXB
#define TGSI_OPCODE_TEXLDP              TGSI_OPCODE_TXP
/* CMP - use TGSI_OPCODE_CND0 */
#define TGSI_OPCODE_DP2ADD              TGSI_OPCODE_DP2A

/*
 * ps_2_x
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */   /* XXX: takes ABS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SNE */
/* EXP - use TGSI_OPCODE_EX2 */
/* LOG - use TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_M4X4 */
/* TGSI_OPCODE_M4X3 */
/* TGSI_OPCODE_M3X4 */
/* TGSI_OPCODE_M3X3 */
/* TGSI_OPCODE_M3X2 */
#define TGSI_OPCODE_CALL                TGSI_OPCODE_CAL
#define TGSI_OPCODE_CALLNZ              129
/* TGSI_OPCODE_RET */
/* TGSI_OPCODE_POW */   /* XXX: takes ABS */
/* TGSI_OPCODE_CRS */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_NRM4 */
/* TGSI_OPCODE_SINCOS */
/* TGSI_OPCODE_REP */
/* TGSI_OPCODE_ENDREP */
/* TGSI_OPCODE_IF */
#define TGSI_OPCODE_IFC                 130
/* TGSI_OPCODE_ELSE */
/* TGSI_OPCODE_ENDIF */
#define TGSI_OPCODE_BREAK               TGSI_OPCODE_BRK
#define TGSI_OPCODE_BREAKC              131
/* TGSI_OPCODE_TEXKILL */
/* TGSI_OPCODE_TEXLD */
/* TGSI_OPCODE_TEXLDB */
/* CMP - use TGSI_OPCODE_CND0 */
/* TGSI_OPCODE_DP2ADD */
#define TGSI_OPCODE_DSX                 TGSI_OPCODE_DDX
#define TGSI_OPCODE_DSY                 TGSI_OPCODE_DDY
#define TGSI_OPCODE_TEXLDD              TGSI_OPCODE_TXD
/* TGSI_OPCODE_TEXLDP */

/*
 * vs_1_1
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */   /* XXX: takes ABS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SGE */
/* EXP - use TGSI_OPCODE_EX2 */
/* LOG - use TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_M4X4 */
/* TGSI_OPCODE_M4X3 */
/* TGSI_OPCODE_M3X4 */
/* TGSI_OPCODE_M3X3 */
/* TGSI_OPCODE_M3X2 */
#define TGSI_OPCODE_EXPP                TGSI_OPCODE_EXP
#define TGSI_OPCODE_LOGP                TGSI_OPCODE_LG2

/*
 * vs_2_0
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */   /* XXX: takes ABS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SGE */
/* EXP - use TGSI_OPCODE_EX2 */
/* LOG - use TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_M4X4 */
/* TGSI_OPCODE_M4X3 */
/* TGSI_OPCODE_M3X4 */
/* TGSI_OPCODE_M3X3 */
/* TGSI_OPCODE_M3X2 */
/* TGSI_OPCODE_CALL */
/* TGSI_OPCODE_CALLNZ */
/* TGSI_OPCODE_LOOP */
/* TGSI_OPCODE_RET */
/* TGSI_OPCODE_ENDLOOP */
/* TGSI_OPCODE_POW */   /* XXX: takes ABS */
/* TGSI_OPCODE_CRS */
#define TGSI_OPCODE_SGN                 TGSI_OPCODE_SSG
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_NRM4 */
/* TGSI_OPCODE_SINCOS */
/* TGSI_OPCODE_REP */
/* TGSI_OPCODE_ENDREP */
/* TGSI_OPCODE_IF */
/* TGSI_OPCODE_ELSE */
/* TGSI_OPCODE_ENDIF */
#define TGSI_OPCODE_MOVA                TGSI_OPCODE_ARR
/* TGSI_OPCODE_LOGP */

/*
 * vs_2_x
 */
/* TGSI_OPCODE_NOP */
/* TGSI_OPCODE_MOV */
/* TGSI_OPCODE_ADD */
/* TGSI_OPCODE_SUB */
/* TGSI_OPCODE_MAD */
/* TGSI_OPCODE_MUL */
/* TGSI_OPCODE_RCP */
/* TGSI_OPCODE_RSQ */   /* XXX: takes ABS */
/* TGSI_OPCODE_DP3 */
/* TGSI_OPCODE_DP4 */
/* TGSI_OPCODE_MIN */
/* TGSI_OPCODE_MAX */
/* TGSI_OPCODE_SLT */
/* TGSI_OPCODE_SGE */
/* TGSI_OPCODE_SGT */
/* TGSI_OPCODE_SLE */
/* TGSI_OPCODE_SEQ */
/* TGSI_OPCODE_SNE */
/* EXP - use TGSI_OPCODE_EX2 */
/* LOG - use TGSI_OPCODE_LG2 */
/* TGSI_OPCODE_LIT */
/* TGSI_OPCODE_DST */
/* TGSI_OPCODE_LRP */
/* TGSI_OPCODE_FRC */
/* TGSI_OPCODE_M4X4 */
/* TGSI_OPCODE_M4X3 */
/* TGSI_OPCODE_M3X4 */
/* TGSI_OPCODE_M3X3 */
/* TGSI_OPCODE_M3X2 */
/* TGSI_OPCODE_CALL */
/* TGSI_OPCODE_CALLNZ */
/* TGSI_OPCODE_LOOP */
/* TGSI_OPCODE_RET */
/* TGSI_OPCODE_ENDLOOP */
/* TGSI_OPCODE_POW */   /* XXX: takes ABS */
/* TGSI_OPCODE_CRS */
/* TGSI_OPCODE_SGN */
/* TGSI_OPCODE_ABS */
/* TGSI_OPCODE_NRM4 */
/* TGSI_OPCODE_SINCOS */
/* TGSI_OPCODE_REP */
/* TGSI_OPCODE_ENDREP */
/* TGSI_OPCODE_IF */
/* TGSI_OPCODE_IFC */
/* TGSI_OPCODE_ELSE */
/* TGSI_OPCODE_ENDIF */
/* TGSI_OPCODE_BREAK */
/* TGSI_OPCODE_BREAKC */
/* TGSI_OPCODE_MOVA */
/* TGSI_OPCODE_LOGP */

#define TGSI_OPCODE_LAST                133

#define TGSI_SAT_NONE            0  /* do not saturate */
#define TGSI_SAT_ZERO_ONE        1  /* clamp to [0,1] */
#define TGSI_SAT_MINUS_PLUS_ONE  2  /* clamp to [-1,1] */

/*
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
   GLuint Type       : 4;  /* TGSI_TOKEN_TYPE_INSTRUCTION */
   GLuint Size       : 8;  /* UINT */
   GLuint Opcode     : 8;  /* TGSI_OPCODE_ */
   GLuint Saturate   : 2;  /* TGSI_SAT_ */
   GLuint NumDstRegs : 2;  /* UINT */
   GLuint NumSrcRegs : 4;  /* UINT */
   GLuint Padding    : 3;
   GLuint Extended   : 1;  /* BOOL */
};

/*
 * If tgsi_instruction::Extended is TRUE, tgsi_instruction_ext follows.
 * 
 * Then, tgsi_instruction::NumDstRegs of tgsi_dst_register follow.
 * 
 * Then, tgsi_instruction::NumSrcRegs of tgsi_src_register follow.
 *
 * tgsi_instruction::Size contains the total number of words that make the
 * instruction, including the instruction word.
 */

#define TGSI_INSTRUCTION_EXT_TYPE_NV        0
#define TGSI_INSTRUCTION_EXT_TYPE_LABEL     1
#define TGSI_INSTRUCTION_EXT_TYPE_TEXTURE   2
#define TGSI_INSTRUCTION_EXT_TYPE_PREDICATE 3

struct tgsi_instruction_ext
{
   GLuint Type       : 4;  /* TGSI_INSTRUCTION_EXT_TYPE_ */
   GLuint Padding    : 27;
   GLuint Extended   : 1;  /* BOOL */
};

/*
 * If tgsi_instruction_ext::Type is TGSI_INSTRUCTION_EXT_TYPE_NV, it should
 * be cast to tgsi_instruction_ext_nv.
 * 
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

#define TGSI_PRECISION_DEFAULT      0
#define TGSI_PRECISION_FLOAT32      1
#define TGSI_PRECISION_FLOAT16      2
#define TGSI_PRECISION_FIXED12      3

#define TGSI_CC_GT      0
#define TGSI_CC_EQ      1
#define TGSI_CC_LT      2
#define TGSI_CC_UN      3
#define TGSI_CC_GE      4
#define TGSI_CC_LE      5
#define TGSI_CC_NE      6
#define TGSI_CC_TR      7
#define TGSI_CC_FL      8

#define TGSI_SWIZZLE_X      0
#define TGSI_SWIZZLE_Y      1
#define TGSI_SWIZZLE_Z      2
#define TGSI_SWIZZLE_W      3

/*
 * Precision controls the precision at which the operation should be executed.
 *
 * CondDstUpdate enables condition code register writes. When this field is
 * TRUE, CondDstIndex specifies the index of the condition code register to
 * update.
 *
 * CondFlowEnable enables conditional execution of the operation. When this
 * field is TRUE, CondFlowIndex specifies the index of the condition code
 * register to test against CondMask with component swizzle controled by
 * CondSwizzleX, CondSwizzleY, CondSwizzleZ and CondSwizzleW. If the test fails,
 * the operation is not executed.
 */

struct tgsi_instruction_ext_nv
{
   GLuint Type             : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_NV */
   GLuint Precision        : 4;    /* TGSI_PRECISION_ */
   GLuint CondDstIndex     : 4;    /* UINT */
   GLuint CondFlowIndex    : 4;    /* UINT */
   GLuint CondMask         : 4;    /* TGSI_CC_ */
   GLuint CondSwizzleX     : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSwizzleY     : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSwizzleZ     : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSwizzleW     : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondDstUpdate    : 1;    /* BOOL */
   GLuint CondFlowEnable   : 1;    /* BOOL */
   GLuint Padding          : 1;
   GLuint Extended         : 1;    /* BOOL */
};

struct tgsi_instruction_ext_label
{
   GLuint Type     : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_LABEL */
   GLuint Label    : 24;   /* UINT */
   GLuint Padding  : 3;
   GLuint Extended : 1;    /* BOOL */
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

struct tgsi_instruction_ext_texture
{
   GLuint Type     : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_TEXTURE */
   GLuint Texture  : 8;    /* TGSI_TEXTURE_ */
   GLuint Padding  : 19;
   GLuint Extended : 1;    /* BOOL */
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

struct tgsi_instruction_ext_predicate
{
   GLuint Type             : 4;    /* TGSI_INSTRUCTION_EXT_TYPE_PREDICATE */
   GLuint PredDstIndex     : 4;    /* UINT */
   GLuint PredWriteMask    : 4;    /* TGSI_WRITEMASK_ */
   GLuint Padding          : 19;
   GLuint Extended         : 1;    /* BOOL */
};

/*
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
   GLuint File         : 4;    /* TGSI_FILE_ */
   GLuint SwizzleX     : 2;    /* TGSI_SWIZZLE_ */
   GLuint SwizzleY     : 2;    /* TGSI_SWIZZLE_ */
   GLuint SwizzleZ     : 2;    /* TGSI_SWIZZLE_ */
   GLuint SwizzleW     : 2;    /* TGSI_SWIZZLE_ */
   GLuint Negate       : 1;    /* BOOL */
   GLuint Indirect     : 1;    /* BOOL */
   GLuint Dimension    : 1;    /* BOOL */
   GLint  Index        : 16;   /* SINT */
   GLuint Extended     : 1;    /* BOOL */
};

/*
 * If tgsi_src_register::Extended is TRUE, tgsi_src_register_ext follows.
 * 
 * Then, if tgsi_src_register::Indirect is TRUE, another tgsi_src_register
 * follows.
 *
 * Then, if tgsi_src_register::Dimension is TRUE, tgsi_dimension follows.
 */

#define TGSI_SRC_REGISTER_EXT_TYPE_SWZ      0
#define TGSI_SRC_REGISTER_EXT_TYPE_MOD      1

struct tgsi_src_register_ext
{
   GLuint Type     : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_ */
   GLuint Padding  : 27;
   GLuint Extended : 1;    /* BOOL */
};

/*
 * If tgsi_src_register_ext::Type is TGSI_SRC_REGISTER_EXT_TYPE_SWZ,
 * it should be cast to tgsi_src_register_ext_extswz.
 * 
 * If tgsi_src_register_ext::Type is TGSI_SRC_REGISTER_EXT_TYPE_MOD,
 * it should be cast to tgsi_src_register_ext_mod.
 * 
 * If tgsi_dst_register_ext::Extended is TRUE, another tgsi_dst_register_ext
 * follows.
 */

#define TGSI_EXTSWIZZLE_X       TGSI_SWIZZLE_X
#define TGSI_EXTSWIZZLE_Y       TGSI_SWIZZLE_Y
#define TGSI_EXTSWIZZLE_Z       TGSI_SWIZZLE_Z
#define TGSI_EXTSWIZZLE_W       TGSI_SWIZZLE_W
#define TGSI_EXTSWIZZLE_ZERO    4
#define TGSI_EXTSWIZZLE_ONE     5

/*
 * ExtSwizzleX, ExtSwizzleY, ExtSwizzleZ and ExtSwizzleW swizzle the source
 * register in an extended manner.
 *
 * NegateX, NegateY, NegateZ and NegateW negate individual components of the
 * source register.
 *
 * ExtDivide specifies which component is used to divide all components of the
 * source register.
 */

struct tgsi_src_register_ext_swz
{
   GLuint Type         : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_SWZ */
   GLuint ExtSwizzleX  : 4;    /* TGSI_EXTSWIZZLE_ */
   GLuint ExtSwizzleY  : 4;    /* TGSI_EXTSWIZZLE_ */
   GLuint ExtSwizzleZ  : 4;    /* TGSI_EXTSWIZZLE_ */
   GLuint ExtSwizzleW  : 4;    /* TGSI_EXTSWIZZLE_ */
   GLuint NegateX      : 1;    /* BOOL */
   GLuint NegateY      : 1;    /* BOOL */
   GLuint NegateZ      : 1;    /* BOOL */
   GLuint NegateW      : 1;    /* BOOL */
   GLuint ExtDivide    : 4;    /* TGSI_EXTSWIZZLE_ */
   GLuint Padding      : 3;
   GLuint Extended     : 1;    /* BOOL */
};

/*
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
   GLuint Type         : 4;    /* TGSI_SRC_REGISTER_EXT_TYPE_MOD */
   GLuint Complement   : 1;    /* BOOL */
   GLuint Bias         : 1;    /* BOOL */
   GLuint Scale2X      : 1;    /* BOOL */
   GLuint Absolute     : 1;    /* BOOL */
   GLuint Negate       : 1;    /* BOOL */
   GLuint Padding      : 22;
   GLuint Extended     : 1;    /* BOOL */
};

struct tgsi_dimension
{
   GLuint Indirect     : 1;    /* BOOL */
   GLuint Dimension    : 1;    /* BOOL */
   GLuint Padding      : 13;
   GLint  Index        : 16;   /* SINT */
   GLuint Extended     : 1;    /* BOOL */
};

struct tgsi_dst_register
{
   GLuint File         : 4;    /* TGSI_FILE_ */
   GLuint WriteMask    : 4;    /* TGSI_WRITEMASK_ */
   GLuint Indirect     : 1;    /* BOOL */
   GLuint Dimension    : 1;    /* BOOL */
   GLint  Index        : 16;   /* SINT */
   GLuint Padding      : 5;
   GLuint Extended     : 1;    /* BOOL */
};

/*
 * If tgsi_dst_register::Extended is TRUE, tgsi_dst_register_ext follows.
 * 
 * Then, if tgsi_dst_register::Indirect is TRUE, tgsi_src_register follows.
 */

#define TGSI_DST_REGISTER_EXT_TYPE_CONDCODE     0
#define TGSI_DST_REGISTER_EXT_TYPE_MODULATE     1
#define TGSI_DST_REGISTER_EXT_TYPE_PREDICATE    2

struct tgsi_dst_register_ext
{
   GLuint Type     : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_ */
   GLuint Padding  : 27;
   GLuint Extended : 1;    /* BOOL */
};

/*
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_CONDCODE,
 * it should be cast to tgsi_dst_register_ext_condcode.
 * 
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_MODULATE,
 * it should be cast to tgsi_dst_register_ext_modulate.
 * 
 * If tgsi_dst_register_ext::Type is TGSI_DST_REGISTER_EXT_TYPE_PREDICATE,
 * it should be cast to tgsi_dst_register_ext_predicate.
 * 
 * If tgsi_dst_register_ext::Extended is TRUE, another tgsi_dst_register_ext
 * follows.
 */

struct tgsi_dst_register_ext_concode
{
   GLuint Type         : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_CONDCODE */
   GLuint CondMask     : 4;    /* TGSI_CC_ */
   GLuint CondSwizzleX : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSwizzleY : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSwizzleZ : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSwizzleW : 2;    /* TGSI_SWIZZLE_ */
   GLuint CondSrcIndex : 4;    /* UINT */
   GLuint Padding      : 11;
   GLuint Extended     : 1;    /* BOOL */
};

#define TGSI_MODULATE_1X        0
#define TGSI_MODULATE_2X        1
#define TGSI_MODULATE_4X        2
#define TGSI_MODULATE_8X        3
#define TGSI_MODULATE_HALF      4
#define TGSI_MODULATE_QUARTER   5
#define TGSI_MODULATE_EIGHTH    6

struct tgsi_dst_register_ext_modulate
{
   GLuint Type     : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_MODULATE */
   GLuint Modulate : 4;    /* TGSI_MODULATE_ */
   GLuint Padding  : 23;
   GLuint Extended : 1;    /* BOOL */
};

/*
 * Currently, the following constraints apply.
 *
 * - PredSwizzleXYZW is either set to identity or replicate.
 * - PredSrcIndex is 0.
 */

struct tgsi_dst_register_ext_predicate
{
   GLuint Type         : 4;    /* TGSI_DST_REGISTER_EXT_TYPE_PREDICATE */
   GLuint PredSwizzleX : 2;    /* TGSI_SWIZZLE_ */
   GLuint PredSwizzleY : 2;    /* TGSI_SWIZZLE_ */
   GLuint PredSwizzleZ : 2;    /* TGSI_SWIZZLE_ */
   GLuint PredSwizzleW : 2;    /* TGSI_SWIZZLE_ */
   GLuint PredSrcIndex : 4;    /* UINT */
   GLuint Negate       : 1;    /* BOOL */
   GLuint Padding      : 14;
   GLuint Extended     : 1;    /* BOOL */
};

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_TOKEN_H

