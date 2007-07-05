#if !defined TGSI_EXEC_H
#define TGSI_EXEC_H

#if 0
#include "x86/rtasm/x86sse.h"
#endif

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

union tgsi_exec_channel
{
   GLfloat  f[4];
   GLint    i[4];
   GLuint   u[4];
};

struct tgsi_exec_vector
{
   union tgsi_exec_channel xyzw[4];
};

struct tgsi_sampler_state
{
    GLboolean   NeedLambda;
    GLboolean   NeedLodBias;        /* if NeedLambda */
    GLboolean   NeedLambdaClamp;    /* if NeedLambda */
    GLfloat     LodBias;            /* if NeedLodBias */
    GLfloat     MinLod;             /* if NeedLambdaClamp */
    GLfloat     MaxLod;             /* if NeedLambdaClamp */
    GLfloat     ImageWidth;
    GLfloat     ImageHeight;
    GLfloat     ImageDepth;
};

struct tgsi_exec_labels
{
   GLuint   labels[128][2];
   GLuint   count;
};

#define TGSI_EXEC_TEMP_00000000_I   32
#define TGSI_EXEC_TEMP_00000000_C   0

#define TGSI_EXEC_TEMP_7FFFFFFF_I   32
#define TGSI_EXEC_TEMP_7FFFFFFF_C   1

#define TGSI_EXEC_TEMP_80000000_I   32
#define TGSI_EXEC_TEMP_80000000_C   2

#define TGSI_EXEC_TEMP_FFFFFFFF_I   32
#define TGSI_EXEC_TEMP_FFFFFFFF_C   3

#define TGSI_EXEC_TEMP_ONE_I        33
#define TGSI_EXEC_TEMP_ONE_C        0

#define TGSI_EXEC_TEMP_TWO_I        33
#define TGSI_EXEC_TEMP_TWO_C        1

#define TGSI_EXEC_TEMP_128_I        33
#define TGSI_EXEC_TEMP_128_C        2

#define TGSI_EXEC_TEMP_MINUS_128_I  33
#define TGSI_EXEC_TEMP_MINUS_128_C  3

#define TGSI_EXEC_TEMP_KILMASK_I    34
#define TGSI_EXEC_TEMP_KILMASK_C    0

#define TGSI_EXEC_TEMP_OUTPUT_I     34
#define TGSI_EXEC_TEMP_OUTPUT_C     1

#define TGSI_EXEC_TEMP_PRIMITIVE_I  34
#define TGSI_EXEC_TEMP_PRIMITIVE_C  2

#define TGSI_EXEC_TEMP_R0           35

#define TGSI_EXEC_NUM_TEMPS   (32 + 4)
#define TGSI_EXEC_NUM_ADDRS   1

struct tgsi_exec_machine
{
   /*
    * 32 program temporaries
    * 4  internal temporaries
    * 1  address
    * 1  temporary of padding to align to 16 bytes
    */
   struct tgsi_exec_vector       _Temps[TGSI_EXEC_NUM_TEMPS + TGSI_EXEC_NUM_ADDRS + 1];

   /*
    * This will point to _Temps after aligning to 16B boundary.
    */
   struct tgsi_exec_vector       *Temps;
   struct tgsi_exec_vector       *Addrs;

   struct tgsi_sampler_state     Samplers[16];

   GLfloat                       Imms[256][4];
   GLuint                        ImmLimit;
   GLfloat                       (*Consts)[4];
   const struct tgsi_exec_vector *Inputs;
   struct tgsi_exec_vector       *Outputs;
   struct tgsi_token             *Tokens;
   GLuint                        Processor;

   GLuint                        *Primitives;
#if XXX_SSE
   struct x86_function           Function;
#endif
};

void
tgsi_exec_machine_init(
   struct tgsi_exec_machine *mach,
   struct tgsi_token *tokens );

void
tgsi_exec_prepare(
   struct tgsi_exec_machine *mach,
   struct tgsi_exec_labels *labels );

void
tgsi_exec_machine_run(
   struct tgsi_exec_machine *mach );

void
tgsi_exec_machine_run2(
   struct tgsi_exec_machine *mach,
   struct tgsi_exec_labels *labels );

#if defined __cplusplus
} // extern "C"
#endif // defined __cplusplus

#endif // !defined TGSI_EXEC_H

