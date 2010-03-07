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
            

#ifndef BRW_VS_H
#define BRW_VS_H


#include "brw_context.h"
#include "brw_eu.h"


struct brw_vs_prog_key {
   GLuint program_string_id;
   GLuint nr_userclip:4;
   GLuint pad:26;
   struct brw_fs_signature fs_signature;
};

#define brw_vs_prog_key_size(s) (offsetof(struct brw_vs_prog_key, fs_signature) + \
                                 brw_fs_signature_size(&(s)->fs_signature))


#define MAX_IF_DEPTH 32
#define MAX_LOOP_DEPTH 32

struct brw_vs_compile {
   struct brw_compile func;
   struct brw_vs_prog_key key;
   struct brw_vs_prog_data prog_data;
   struct brw_chipset chipset;

   struct brw_vertex_shader *vp;

   GLuint nr_inputs;
   GLuint nr_outputs;
   GLuint nr_immediates;
   GLfloat immediate[128][4];

   GLuint overflow_grf_start;
   GLuint overflow_count;

   GLuint first_tmp;
   GLuint last_tmp;

   struct brw_reg r0;
   struct brw_reg r1;
   struct brw_reg regs[TGSI_FILE_COUNT][128];
   struct brw_reg tmp;
   struct brw_reg stack;

   struct {	
       GLboolean used_in_src;
       struct brw_reg reg;
   } output_regs[128];

   struct brw_reg userplane[6];

   /** we may need up to 3 constants per instruction (if use_const_buffer) */
   struct {
      GLint index;
      struct brw_reg reg;
   } current_const[3];

   struct brw_instruction *if_inst[MAX_IF_DEPTH];
   struct brw_instruction *loop_inst[MAX_LOOP_DEPTH];
   GLuint insn;
   GLuint if_depth;
   GLuint loop_depth;
   GLuint end_offset;

   struct brw_indirect stack_index;
};


void brw_vs_emit( struct brw_vs_compile *c );

#endif
