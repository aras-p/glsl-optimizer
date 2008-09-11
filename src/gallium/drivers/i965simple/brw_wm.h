/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
              

#ifndef BRW_WM_H
#define BRW_WM_H


#include "brw_context.h"
#include "brw_eu.h"

/* A big lookup table is used to figure out which and how many
 * additional regs will inserted before the main payload in the WM
 * program execution.  These mainly relate to depth and stencil
 * processing and the early-depth-test optimization.
 */
#define IZ_PS_KILL_ALPHATEST_BIT    0x1
#define IZ_PS_COMPUTES_DEPTH_BIT    0x2
#define IZ_DEPTH_WRITE_ENABLE_BIT   0x4
#define IZ_DEPTH_TEST_ENABLE_BIT    0x8
#define IZ_STENCIL_WRITE_ENABLE_BIT 0x10
#define IZ_STENCIL_TEST_ENABLE_BIT  0x20
#define IZ_EARLY_DEPTH_TEST_BIT     0x40
#define IZ_BIT_MAX                  0x80

#define AA_NEVER     0
#define AA_SOMETIMES 1
#define AA_ALWAYS    2

struct brw_wm_prog_key {
   unsigned source_depth_reg:3;
   unsigned aa_dest_stencil_reg:3;
   unsigned dest_depth_reg:3;
   unsigned nr_depth_regs:3;
   unsigned shadowtex_mask:8;
   unsigned computes_depth:1;	/* could be derived from program string */
   unsigned source_depth_to_render_target:1;
   unsigned runtime_check_aads_emit:1;

   unsigned yuvtex_mask:8;

   unsigned program_string_id;
};





#define PROGRAM_INTERNAL_PARAM
#define MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS 1024 /* 72 for GL_ARB_f_p */
#define BRW_WM_MAX_INSN  (MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS*3 + PIPE_MAX_ATTRIBS + 3)
#define BRW_WM_MAX_GRF   128		/* hardware limit */
#define BRW_WM_MAX_VREG  (BRW_WM_MAX_INSN * 4)
#define BRW_WM_MAX_REF   (BRW_WM_MAX_INSN * 12)
#define BRW_WM_MAX_PARAM 256
#define BRW_WM_MAX_CONST 256
#define BRW_WM_MAX_KILLS MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS

#define PAYLOAD_DEPTH     (PIPE_MAX_ATTRIBS)

#define MAX_IFSN 32
#define MAX_LOOP_DEPTH 32

struct brw_wm_compile {
   struct brw_compile func;
   struct brw_wm_prog_key key;
   struct brw_wm_prog_data prog_data; /* result */

   struct brw_fragment_program *fp;

   unsigned grf_limit;
   unsigned max_wm_grf;


   struct brw_reg pixel_xy[2];
   struct brw_reg delta_xy[2];
   struct brw_reg pixel_w;


   struct brw_reg wm_regs[8][32][4];

   struct brw_reg payload_depth[4];
   struct brw_reg payload_coef[16];

   struct brw_reg emit_mask_reg;

   struct brw_instruction *if_inst[MAX_IFSN];
   int if_insn;

   struct brw_instruction *loop_inst[MAX_LOOP_DEPTH];
   int loop_insn;

   struct brw_instruction *inst0;
   struct brw_instruction *inst1;

   struct brw_reg stack;
   struct brw_indirect stack_index;

   unsigned reg_index;

   unsigned tmp_start;
   unsigned tmp_index;
};



void brw_wm_lookup_iz( unsigned line_aa,
		       unsigned lookup,
		       struct brw_wm_prog_key *key );

void brw_wm_glsl_emit(struct brw_wm_compile *c);
void brw_wm_emit_decls(struct brw_wm_compile *c);

#endif
