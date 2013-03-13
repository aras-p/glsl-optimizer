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

#include <stdbool.h>

#include "program/prog_instruction.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_program.h"

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
#define IZ_BIT_MAX                  0x40

#define AA_NEVER     0
#define AA_SOMETIMES 1
#define AA_ALWAYS    2

struct brw_wm_prog_key {
   uint8_t iz_lookup;
   GLuint stats_wm:1;
   GLuint flat_shade:1;
   GLuint nr_color_regions:5;
   GLuint sample_alpha_to_coverage:1;
   GLuint render_to_fbo:1;
   GLuint clamp_fragment_color:1;
   GLuint line_aa:2;

   GLushort drawable_height;
   GLbitfield64 input_slots_valid;
   GLuint program_string_id:32;

   struct brw_sampler_prog_key_data tex;
};

struct brw_wm_compile {
   struct brw_wm_prog_key key;
   struct brw_wm_prog_data prog_data;

   uint8_t source_depth_reg;
   uint8_t source_w_reg;
   uint8_t aa_dest_stencil_reg;
   uint8_t dest_depth_reg;
   uint8_t barycentric_coord_reg[BRW_WM_BARYCENTRIC_INTERP_MODE_COUNT];
   uint8_t nr_payload_regs;
   GLuint source_depth_to_render_target:1;
   GLuint runtime_check_aads_emit:1;

   GLuint last_scratch;
};

/**
 * Compile a fragment shader.
 *
 * Returns the final assembly and the program's size.
 */
const unsigned *brw_wm_fs_emit(struct brw_context *brw,
                               struct brw_wm_compile *c,
                               struct gl_fragment_program *fp,
                               struct gl_shader_program *prog,
                               unsigned *final_assembly_size);

GLboolean brw_link_shader(struct gl_context *ctx, struct gl_shader_program *prog);
struct gl_shader *brw_new_shader(struct gl_context *ctx, GLuint name, GLuint type);
struct gl_shader_program *brw_new_shader_program(struct gl_context *ctx, GLuint name);

bool brw_color_buffer_write_enabled(struct brw_context *brw);
bool brw_render_target_supported(struct intel_context *intel,
				 struct gl_renderbuffer *rb);
bool do_wm_prog(struct brw_context *brw,
		struct gl_shader_program *prog,
		struct brw_fragment_program *fp,
		struct brw_wm_prog_key *key);
void brw_wm_debug_recompile(struct brw_context *brw,
                            struct gl_shader_program *prog,
                            const struct brw_wm_prog_key *key);
bool brw_wm_prog_data_compare(const void *a, const void *b,
                              int aux_size, const void *key);
void brw_wm_prog_data_free(const void *in_prog_data);

#endif
