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
#include "brw_program.h"
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

struct brw_vec4_prog_key {
   GLuint program_string_id;

   /**
    * True if at least one clip flag is enabled, regardless of whether the
    * shader uses clip planes or gl_ClipDistance.
    */
   GLuint userclip_active:1;

   /**
    * How many user clipping planes are being uploaded to the vertex shader as
    * push constants.
    */
   GLuint nr_userclip_plane_consts:4;

   /**
    * True if the shader uses gl_ClipDistance, regardless of whether any clip
    * flags are enabled.
    */
   GLuint uses_clip_distance:1;

   /**
    * For pre-Gen6 hardware, a bitfield indicating which clipping planes are
    * enabled.  This is used to compact clip planes.
    *
    * For Gen6 and later hardware, clip planes are not compacted, so this
    * value is zero to avoid provoking unnecessary shader recompiles.
    */
   GLuint userclip_planes_enabled_gen_4_5:MAX_CLIP_PLANES;

   GLuint clamp_vertex_color:1;

   struct brw_sampler_prog_key_data tex;
};


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


struct brw_vec4_compile {
   GLuint last_scratch; /**< measured in 32-byte (register size) units */
};


struct brw_vs_compile {
   struct brw_vec4_compile base;
   struct brw_vs_prog_key key;

   struct brw_vertex_program *vp;
};

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
bool brw_vec4_prog_data_compare(const struct brw_vec4_prog_data *a,
                                const struct brw_vec4_prog_data *b);
bool brw_vs_prog_data_compare(const void *a, const void *b,
                              int aux_size, const void *key);
void brw_vec4_prog_data_free(const struct brw_vec4_prog_data *prog_data);
void brw_vs_prog_data_free(const void *in_prog_data);

#endif
