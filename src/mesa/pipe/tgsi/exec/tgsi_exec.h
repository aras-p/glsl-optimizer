#if !defined TGSI_EXEC_H
#define TGSI_EXEC_H

#include "pipe/p_compiler.h"

#if defined __cplusplus
extern "C" {
#endif // defined __cplusplus

#define NUM_CHANNELS 4  /* R,G,B,A */
#define QUAD_SIZE    4  /* 4 pixel/quad */

union tgsi_exec_channel
{
   float    f[QUAD_SIZE];
   int      i[QUAD_SIZE];
   unsigned u[QUAD_SIZE];
};

struct tgsi_exec_vector
{
   union tgsi_exec_channel xyzw[NUM_CHANNELS];
};

struct tgsi_interp_coef
{
   float a0[NUM_CHANNELS];	/* in an xyzw layout */
   float dadx[NUM_CHANNELS];
   float dady[NUM_CHANNELS];
};

#define TEX_CACHE_TILE_SIZE 8
#define TEX_CACHE_NUM_ENTRIES 8

struct tgsi_texture_cache_entry
{
   int x, y, face, level, zslice;
   float data[TEX_CACHE_TILE_SIZE][TEX_CACHE_TILE_SIZE][4];
};

struct tgsi_sampler
{
   const struct pipe_sampler_state *state;
   struct pipe_mipmap_tree *texture;
   /** Get samples for four fragments in a quad */
   void (*get_samples)(struct tgsi_sampler *sampler,
                       const float s[QUAD_SIZE],
                       const float t[QUAD_SIZE],
                       const float p[QUAD_SIZE],
                       float lodbias,
                       float rgba[NUM_CHANNELS][QUAD_SIZE]);
   void *pipe; /*XXX temporary*/
   struct tgsi_texture_cache_entry cache[TEX_CACHE_NUM_ENTRIES];
};

struct tgsi_exec_labels
{
   unsigned labels[128][2];
   unsigned count;
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

#define TGSI_EXEC_MAX_COND_NESTING  10
#define TGSI_EXEC_MAX_LOOP_NESTING  10


/**
 * Run-time virtual machine state for executing TGSI shader.
 */
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

   struct tgsi_sampler           *Samplers;

   float                         Imms[256][4];
   unsigned                      ImmLimit;
   float                         (*Consts)[4];
   struct tgsi_exec_vector       *Inputs;
   struct tgsi_exec_vector       *Outputs;
   const struct tgsi_token       *Tokens;
   unsigned                      Processor;

   /* GEOMETRY processor only. */
   unsigned                      *Primitives;

   /* FRAGMENT processor only. */
   const struct tgsi_interp_coef *InterpCoefs;

   /* Conditional execution masks */
   uint CondMask;
   uint LoopMask;
   uint ExecMask;  /**< = CondMask & LoopMask */

   /** Condition mask stack (for nested conditionals) */
   uint CondStack[TGSI_EXEC_MAX_COND_NESTING];
   int CondStackTop;

   /** Loop mask stack (for nested loops) */
   uint LoopStack[TGSI_EXEC_MAX_LOOP_NESTING];
   int LoopStackTop;
};


void
tgsi_exec_machine_init(
   struct tgsi_exec_machine *mach,
   const struct tgsi_token *tokens,
   unsigned numSamplers,
   struct tgsi_sampler *samplers);

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

