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


struct tgsi_token;
struct x86_function;

#include "pipe/p_state.h"
#include "rtasm/rtasm_x86sse.h"





#define X    0
#define Y    1
#define Z    2
#define W    3

#define MAX_INPUTS     PIPE_MAX_ATTRIBS
#define MAX_OUTPUTS    PIPE_MAX_ATTRIBS
#define MAX_TEMPS      PIPE_MAX_ATTRIBS /* say */
#define MAX_CONSTANTS  PIPE_MAX_ATTRIBS /* say */
#define MAX_IMMEDIATES PIPE_MAX_ATTRIBS /* say */
#define MAX_INTERNALS  4

#define AOS_FILE_INTERNAL TGSI_FILE_COUNT

/* This is the temporary storage used by all the aos_sse vs varients.
 * Create one per context and reuse by passing a pointer in at
 * vs_varient creation??
 */
struct aos_machine {
   float input    [MAX_INPUTS    ][4];
   float output   [MAX_OUTPUTS   ][4];
   float temp     [MAX_TEMPS     ][4];
   float constant [MAX_CONSTANTS ][4]; /* fixme -- should just be a pointer */
   float immediate[MAX_IMMEDIATES][4]; /* fixme -- should just be a pointer */
   float internal [MAX_INTERNALS ][4];

   float scale[4];              /* viewport */
   float translate[4];          /* viewport */

   ushort fpu_round_nearest;
   ushort fpu_round_neg_inf;
   ushort fpu_restore;
   ushort fpucntl;              /* one of FPU_* above */

   struct {
      const void *input_ptr;
      unsigned input_stride;

      unsigned output_offset;
   } attrib[PIPE_MAX_ATTRIBS];
};




struct aos_compilation {
   struct x86_function *func;
   struct draw_vs_varient_aos_sse *vaos;

   unsigned insn_counter;
   unsigned num_immediates;

   struct {
      unsigned idx:16;
      unsigned file:8;
      unsigned dirty:8;
      unsigned last_used;
   } xmm[8];


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
};

struct x86_reg aos_get_xmm_reg( struct aos_compilation *cp );
void aos_release_xmm_reg( struct aos_compilation *cp, unsigned idx );

void aos_adopt_xmm_reg( struct aos_compilation *cp,
                        struct x86_reg reg,
                        unsigned file,
                        unsigned idx,
                        unsigned dirty );

struct x86_reg aos_get_shader_reg( struct aos_compilation *cp, 
                                   unsigned file,
                                   unsigned idx );

boolean aos_fetch_inputs( struct aos_compilation *cp,
                          boolean linear );

boolean aos_emit_outputs( struct aos_compilation *cp );


#define IMM_ONES     0              /* 1, 1,1,1 */
#define IMM_NEGS     1              /* 1,-1,0,0 */
#define IMM_IDENTITY 2              /* 0, 0,0,1 */
#define IMM_INV_255  3              /* 1/255, 1/255, 1/255, 1/255 */
#define IMM_255      4              /* 255, 255, 255, 255 */

struct x86_reg aos_get_internal( struct aos_compilation *cp,
                                 unsigned imm );


#define ERROR(cp, msg)                                                  \
do {                                                                    \
   debug_printf("%s: x86 translation failed: %s\n", __FUNCTION__, msg); \
   cp->error = 1;                                                       \
   assert(0);                                                           \
} while (0)






struct draw_vs_varient_aos_sse {
   struct draw_vs_varient base;
   struct draw_context *draw;

#if 0
   struct {
      const void *ptr;
      unsigned stride;
   } attrib[PIPE_MAX_ATTRIBS];
#endif

   struct aos_machine *machine; /* XXX: temporarily unshared */

   vsv_run_linear_func gen_run_linear;
   vsv_run_elts_func gen_run_elts;


   struct x86_function func[2];
};



#endif 

