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
   unsigned projtex_mask:8;
   unsigned shadowtex_mask:8;
   unsigned computes_depth:1;	/* could be derived from program string */
   unsigned source_depth_to_render_target:1;
   unsigned flat_shade:1;
   unsigned runtime_check_aads_emit:1;
   
   unsigned yuvtex_mask:8;
   unsigned pad1:24;

   unsigned program_string_id:32;
};


/* A bit of a glossary:
 *
 * brw_wm_value: A computed value or program input.  Values are
 * constant, they are created once and are never modified.  When a
 * fragment program register is written or overwritten, new values are
 * created fresh, preserving the rule that values are constant.
 *
 * brw_wm_ref: A reference to a value.  Wherever a value used is by an
 * instruction or as a program output, that is tracked with an
 * instance of this struct.  All references to a value occur after it
 * is created.  After the last reference, a value is dead and can be
 * discarded.
 *
 * brw_wm_grf: Represents a physical hardware register.  May be either
 * empty or hold a value.  Register allocation is the process of
 * assigning values to grf registers.  This occurs in pass2 and the
 * brw_wm_grf struct is not used before that.
 *
 * Fragment program registers: These are time-varying constructs that
 * are hard to reason about and which we translate away in pass0.  A
 * single fragment program register element (eg. temp[0].x) will be
 * translated to one or more brw_wm_value structs, one for each time
 * that temp[0].x is written to during the program. 
 */



/* Used in pass2 to track register allocation.
 */
struct brw_wm_grf {
   struct brw_wm_value *value;
   unsigned nextuse;
};

struct brw_wm_value {
   struct brw_reg hw_reg;	/* emitted to this reg, may not always be there */
   struct brw_wm_ref *lastuse;
   struct brw_wm_grf *resident; 
   unsigned contributes_to_output:1;
   unsigned spill_slot:16;	/* if non-zero, spill immediately after calculation */
};

struct brw_wm_ref {
   struct brw_reg hw_reg;	/* nr filled in in pass2, everything else, pass0 */
   struct brw_wm_value *value;
   struct brw_wm_ref *prevuse;
   unsigned unspill_reg:7;	/* unspill to reg */
   unsigned emitted:1;
   unsigned insn:24;
};

struct brw_wm_constref {
   const struct brw_wm_ref *ref;
   float constval;
};


struct brw_wm_instruction {
   struct brw_wm_value *dst[4];
   struct brw_wm_ref *src[3][4];
   unsigned opcode:8;
   unsigned saturate:1;
   unsigned writemask:4;
   unsigned tex_unit:4;   /* texture unit for TEX, TXD, TXP instructions */
   unsigned tex_idx:3;    /* TEXTURE_1D,2D,3D,CUBE,RECT_INDEX source target */
};

#define PROGRAM_INTERNAL_PARAM
#define MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS 1024 /* 72 for GL_ARB_f_p */
#define BRW_WM_MAX_INSN  (MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS*3 + PIPE_ATTRIB_MAX + 3)
#define BRW_WM_MAX_GRF   128		/* hardware limit */
#define BRW_WM_MAX_VREG  (BRW_WM_MAX_INSN * 4)
#define BRW_WM_MAX_REF   (BRW_WM_MAX_INSN * 12)
#define BRW_WM_MAX_PARAM 256
#define BRW_WM_MAX_CONST 256
#define BRW_WM_MAX_KILLS MAX_NV_FRAGMENT_PROGRAM_INSTRUCTIONS



/* New opcodes to track internal operations required for WM unit.
 * These are added early so that the registers used can be tracked,
 * freed and reused like those of other instructions.
 */
#define WM_PIXELXY        (TGSI_OPCODE_LAST)
#define WM_DELTAXY        (TGSI_OPCODE_LAST + 1)
#define WM_PIXELW         (TGSI_OPCODE_LAST + 2)
#define WM_LINTERP        (TGSI_OPCODE_LAST + 3)
#define WM_PINTERP        (TGSI_OPCODE_LAST + 4)
#define WM_CINTERP        (TGSI_OPCODE_LAST + 5)
#define WM_WPOSXY         (TGSI_OPCODE_LAST + 6)
#define WM_FB_WRITE       (TGSI_OPCODE_LAST + 7)
#define MAX_WM_OPCODE     (TGSI_OPCODE_LAST + 8)

#define PAYLOAD_DEPTH     (PIPE_ATTRIB_MAX)

struct brw_wm_compile {
   struct brw_compile func;
   struct brw_wm_prog_key key;
   struct brw_wm_prog_data prog_data;

   struct brw_fragment_program *fp;

   float (*env_param)[4];

   enum {
      START,
      PASS2_DONE
   } state;

   /* Initial pass - translate fp instructions to fp instructions,
    * simplifying and adding instructions for interpolation and
    * framebuffer writes.
    */
   const struct pipe_shader_state *prog_instructions;
   unsigned nr_fp_insns;
   unsigned fp_temp;
   unsigned fp_interp_emitted;
   unsigned fp_deriv_emitted;

   struct tgsi_src_register pixel_xy;
   struct tgsi_src_register delta_xy;
   struct tgsi_src_register pixel_w;


   struct brw_wm_value vreg[BRW_WM_MAX_VREG];
   unsigned nr_vreg;

   struct brw_wm_value creg[BRW_WM_MAX_PARAM];
   unsigned nr_creg;

   struct {
      struct brw_wm_value depth[4]; /* includes r0/r1 */
      struct brw_wm_value input_interp[PIPE_ATTRIB_MAX];
   } payload;


   const struct brw_wm_ref *pass0_fp_reg[16][256][4];

   struct brw_wm_ref undef_ref;
   struct brw_wm_value undef_value;

   struct brw_wm_ref refs[BRW_WM_MAX_REF];
   unsigned nr_refs;

   struct brw_wm_instruction instruction[BRW_WM_MAX_INSN];
   unsigned nr_insns;

   struct brw_wm_constref constref[BRW_WM_MAX_CONST];
   unsigned nr_constrefs;

   struct brw_wm_grf pass2_grf[BRW_WM_MAX_GRF/2];

   unsigned grf_limit;
   unsigned max_wm_grf;
   unsigned last_scratch;

   struct {
	boolean inited;
	struct brw_reg reg;
   } wm_regs[16][256][4];
   struct brw_reg stack;
   struct brw_reg emit_mask_reg;
   unsigned reg_index;
   unsigned tmp_index;
};


unsigned brw_wm_nr_args( unsigned opcode );
unsigned brw_wm_is_scalar_result( unsigned opcode );

void brw_wm_pass_fp( struct brw_wm_compile *c );
void brw_wm_pass0( struct brw_wm_compile *c );
void brw_wm_pass1( struct brw_wm_compile *c );
void brw_wm_pass2( struct brw_wm_compile *c );
void brw_wm_emit( struct brw_wm_compile *c );

void brw_wm_print_value( struct brw_wm_compile *c,
			 struct brw_wm_value *value );

void brw_wm_print_ref( struct brw_wm_compile *c,
		       struct brw_wm_ref *ref );

void brw_wm_print_insn( struct brw_wm_compile *c,
			struct brw_wm_instruction *inst );

void brw_wm_print_program( struct brw_wm_compile *c,
			   const char *stage );

void brw_wm_lookup_iz( unsigned line_aa,
		       unsigned lookup,
		       struct brw_wm_prog_key *key );

#if 0
boolean brw_wm_is_glsl(struct gl_fragment_program *fp);
void brw_wm_glsl_emit(struct brw_wm_compile *c);
#endif

#endif
