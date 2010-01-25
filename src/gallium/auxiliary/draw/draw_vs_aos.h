/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef DRAW_VS_AOS_H
#define DRAW_VS_AOS_H

#include "pipe/p_config.h"

#ifdef PIPE_ARCH_X86

struct tgsi_token;
struct x86_function;

#include "pipe/p_state.h"
#include "rtasm/rtasm_x86sse.h"





#define X    0
#define Y    1
#define Z    2
#define W    3

#define MAX_INPUTS     PIPE_MAX_ATTRIBS
#define MAX_OUTPUTS    PIPE_MAX_SHADER_OUTPUTS
#define MAX_TEMPS      TGSI_EXEC_NUM_TEMPS
#define MAX_CONSTANTS  1024  /** only used for sanity checking */
#define MAX_IMMEDIATES 1024  /** only used for sanity checking */
#define MAX_INTERNALS  8     /** see IMM_x values below */

#define AOS_FILE_INTERNAL TGSI_FILE_COUNT

#define FPU_RND_NEG    1
#define FPU_RND_NEAREST 2

struct aos_machine;
typedef void (PIPE_CDECL *lit_func)( struct aos_machine *,
                                    float *result,
                                    const float *in,
                                    unsigned count );

void PIPE_CDECL aos_do_lit( struct aos_machine *machine,
                            float *result,
                            const float *in,
                            unsigned count );

struct shine_tab {
   float exponent;
   float values[258];
   unsigned last_used;
};

struct lit_info {
   lit_func func;
   struct shine_tab *shine_tab;
};

#define MAX_SHINE_TAB    4
#define MAX_LIT_INFO     16

struct aos_buffer {
   const void *base_ptr;
   unsigned stride;
   void *ptr;                   /* updated per vertex */
};




/* This is the temporary storage used by all the aos_sse vs varients.
 * Create one per context and reuse by passing a pointer in at
 * vs_varient creation??
 */
struct aos_machine {
   float input    [MAX_INPUTS    ][4];
   float output   [MAX_OUTPUTS   ][4];
   float temp     [MAX_TEMPS     ][4];
   float internal [MAX_INTERNALS ][4];

   float scale[4];              /* viewport */
   float translate[4];          /* viewport */

   float tmp[2][4];             /* scratch space for LIT */

   struct shine_tab shine_tab[MAX_SHINE_TAB];
   struct lit_info  lit_info[MAX_LIT_INFO];
   unsigned now;
   

   ushort fpu_rnd_nearest;
   ushort fpu_rnd_neg_inf;
   ushort fpu_restore;
   ushort fpucntl;              /* one of FPU_* above */

   const float (*immediates)[4];     /* points to shader data */
   const void *constants[PIPE_MAX_CONSTANT_BUFFERS]; /* points to draw data */

   const struct aos_buffer *buffer; /* points to ? */
};




struct aos_compilation {
   struct x86_function *func;
   struct draw_vs_varient_aos_sse *vaos;

   unsigned insn_counter;
   unsigned num_immediates;
   unsigned count;
   unsigned lit_count;

   struct {
      unsigned idx:16;
      unsigned file:8;
      unsigned dirty:8;
      unsigned last_used;
   } xmm[8];

   unsigned x86_reg[2];                /* one of X86_* */

   boolean input_fetched[PIPE_MAX_ATTRIBS];
   unsigned output_last_write[PIPE_MAX_ATTRIBS];

   boolean have_sse2;
   boolean error;
   short fpucntl;

   /* these are actually known values, but putting them in a struct
    * like this is helpful to keep them in sync across the file.
    */
   struct x86_reg tmp_EAX;
   struct x86_reg idx_EBX;     /* either start+i or &elt[i] */
   struct x86_reg outbuf_ECX;
   struct x86_reg machine_EDX;
   struct x86_reg count_ESI;    /* decrements to zero */
   struct x86_reg temp_EBP;
   struct x86_reg stack_ESP;
};

struct x86_reg aos_get_xmm_reg( struct aos_compilation *cp );
void aos_release_xmm_reg( struct aos_compilation *cp, unsigned idx );

void aos_adopt_xmm_reg( struct aos_compilation *cp,
                        struct x86_reg reg,
                        unsigned file,
                        unsigned idx,
                        unsigned dirty );

void aos_spill_all( struct aos_compilation *cp );

struct x86_reg aos_get_shader_reg( struct aos_compilation *cp, 
                                   unsigned file,
                                   unsigned idx );

boolean aos_init_inputs( struct aos_compilation *cp, boolean linear );
boolean aos_fetch_inputs( struct aos_compilation *cp, boolean linear );
boolean aos_incr_inputs( struct aos_compilation *cp, boolean linear );

boolean aos_emit_outputs( struct aos_compilation *cp );


#define IMM_ONES     0              /* 1, 1,1,1 */
#define IMM_SWZ      1              /* 1,-1,0, 0xffffffff */
#define IMM_IDENTITY 2              /* 0, 0,0,1 */
#define IMM_INV_255  3              /* 1/255, 1/255, 1/255, 1/255 */
#define IMM_255      4              /* 255, 255, 255, 255 */
#define IMM_NEGS     5              /* -1,-1,-1,-1 */
#define IMM_RSQ      6              /* -.5,1.5,_,_ */
#define IMM_PSIZE    7              /* not really an immediate - updated each run */

struct x86_reg aos_get_internal( struct aos_compilation *cp,
                                 unsigned imm );
struct x86_reg aos_get_internal_xmm( struct aos_compilation *cp,
                                     unsigned imm );


#define AOS_ERROR(cp, msg)                                                  \
do {                                                                    \
   if (0) debug_printf("%s: x86 translation failed: %s\n", __FUNCTION__, msg); \
   cp->error = 1;                                                       \
} while (0)


#define X86_NULL       0
#define X86_IMMEDIATES 1
#define X86_CONSTANTS  2
#define X86_BUFFERS    3

struct x86_reg aos_get_x86( struct aos_compilation *cp,
                            unsigned which_reg,
                            unsigned value );


typedef void (PIPE_CDECL *vaos_run_elts_func)( struct aos_machine *,
                                               const unsigned *elts,
                                               unsigned count,
                                               void *output_buffer);

typedef void (PIPE_CDECL *vaos_run_linear_func)( struct aos_machine *,
                                                unsigned start,
                                                unsigned count,
                                                void *output_buffer);


struct draw_vs_varient_aos_sse {
   struct draw_vs_varient base;
   struct draw_context *draw;

   struct aos_buffer *buffer;
   unsigned nr_vb;

   vaos_run_linear_func gen_run_linear;
   vaos_run_elts_func gen_run_elts;


   struct x86_function func[2];
};


#endif

#endif 

