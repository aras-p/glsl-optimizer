/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics to
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
  *   Keith Whitwell <keithw@vmware.com>
  */


#ifndef BRW_VS_H
#define BRW_VS_H


#include "brw_context.h"
#include "brw_eu.h"
#include "brw_vec4.h"
#include "program/program.h"

/**
 * The VF can't natively handle certain types of attributes, such as GL_FIXED
 * or most 10_10_10_2 types.  These flags enable various VS workarounds to
 * "fix" attributes at the beginning of shaders.
 */
#define BRW_ATTRIB_WA_COMPONENT_MASK    7  /* mask for GL_FIXED scale channel count */
#define BRW_ATTRIB_WA_NORMALIZE     8   /* normalize in shader */
#define BRW_ATTRIB_WA_BGRA          16  /* swap r/b channels in shader */
#define BRW_ATTRIB_WA_SIGN          32  /* interpret as signed in shader */
#define BRW_ATTRIB_WA_SCALE         64  /* interpret as scaled in shader */

struct brw_vs_prog_key {
   struct brw_vec4_prog_key base;

   /*
    * Per-attribute workaround flags
    */
   uint8_t gl_attrib_wa_flags[VERT_ATTRIB_MAX];

   GLuint copy_edgeflag:1;

   /**
    * For pre-Gen6 hardware, a bitfield indicating which texture coordinates
    * are going to be replaced with point coordinates (as a consequence of a
    * call to glTexEnvi(GL_POINT_SPRITE, GL_COORD_REPLACE, GL_TRUE)).  Because
    * our SF thread requires exact matching between VS outputs and FS inputs,
    * these texture coordinates will need to be unconditionally included in
    * the VUE, even if they aren't written by the vertex shader.
    */
   GLuint point_coord_replace:8;
};


struct brw_vs_compile {
   struct brw_vec4_compile base;
   struct brw_vs_prog_key key;

   struct brw_vertex_program *vp;
};

#ifdef __cplusplus
extern "C" {
#endif

const unsigned *brw_vs_emit(struct brw_context *brw,
                            struct gl_shader_program *prog,
                            struct brw_vs_compile *c,
                            struct brw_vs_prog_data *prog_data,
                            void *mem_ctx,
                            unsigned *program_size);
bool brw_vs_precompile(struct gl_context *ctx, struct gl_shader_program *prog);
void brw_vs_debug_recompile(struct brw_context *brw,
                            struct gl_shader_program *prog,
                            const struct brw_vs_prog_key *key);
bool brw_vs_prog_data_compare(const void *a, const void *b);

#ifdef __cplusplus
} /* extern "C" */


namespace brw {

class vec4_vs_visitor : public vec4_visitor
{
public:
   vec4_vs_visitor(struct brw_context *brw,
                   struct brw_vs_compile *vs_compile,
                   struct brw_vs_prog_data *vs_prog_data,
                   struct gl_shader_program *prog,
                   void *mem_ctx);

protected:
   virtual dst_reg *make_reg_for_system_value(ir_variable *ir);
   virtual void setup_payload();
   virtual void emit_prolog();
   virtual void emit_program_code();
   virtual void emit_thread_end();
   virtual void emit_urb_write_header(int mrf);
   virtual vec4_instruction *emit_urb_write_opcode(bool complete);

private:
   int setup_attributes(int payload_reg);
   void setup_vp_regs();
   dst_reg get_vp_dst_reg(const prog_dst_register &dst);
   src_reg get_vp_src_reg(const prog_src_register &src);

   struct brw_vs_compile * const vs_compile;
   struct brw_vs_prog_data * const vs_prog_data;
   src_reg *vp_temp_regs;
   src_reg vp_addr_reg;
};

} /* namespace brw */


#endif

#endif
