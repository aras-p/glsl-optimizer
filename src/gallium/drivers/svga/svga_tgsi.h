/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#ifndef SVGA_TGSI_H
#define SVGA_TGSI_H

#include "pipe/p_state.h"

#include "svga_hw_reg.h"

struct svga_fragment_shader;
struct svga_vertex_shader;
struct svga_shader;
struct tgsi_shader_info;
struct tgsi_token;


struct svga_vs_compile_key
{
   unsigned zero_stride_vertex_elements;
   unsigned need_prescale:1;
   unsigned allow_psiz:1;
   unsigned num_zero_stride_vertex_elements:6;
};

struct svga_fs_compile_key
{
   unsigned light_twoside:1;
   unsigned front_cw:1;
   unsigned white_fragments:1;
   unsigned num_textures:8;
   unsigned num_unnormalized_coords:8;
   struct {
      unsigned compare_mode:1;
      unsigned compare_func:3;
      unsigned unnormalized:1;
      unsigned width_height_idx:7;
      unsigned texture_target:8;
   } tex[PIPE_MAX_SAMPLERS];
};

union svga_compile_key {
   struct svga_vs_compile_key vkey;
   struct svga_fs_compile_key fkey;
};

struct svga_shader_result
{
   const struct svga_shader *shader;

   /* Parameters used to generate this compilation result:
    */
   union svga_compile_key key;

   /* Compiled shader tokens:
    */
   const unsigned *tokens;
   unsigned nr_tokens;

   /* SVGA Shader ID:
    */
   unsigned id;
   
   /* Next compilation result:
    */
   struct svga_shader_result *next;
};


/* TGSI doesn't provide use with VS input semantics (they're actually
 * pretty meaningless), so we just generate some plausible ones here.
 * This is called both from within the TGSI translator and when
 * building vdecls to ensure they match up.
 *
 * The real use of this information is matching vertex elements to
 * fragment shader inputs in the case where vertex shader is disabled.
 */
static INLINE void svga_generate_vdecl_semantics( unsigned idx,
                                                  unsigned *usage,
                                                  unsigned *usage_index )
{
   if (idx == 0) {
      *usage = SVGA3D_DECLUSAGE_POSITION;
      *usage_index = 0;
   }
   else {
      *usage = SVGA3D_DECLUSAGE_TEXCOORD;
      *usage_index = idx - 1;
   }
}



static INLINE unsigned svga_vs_key_size( const struct svga_vs_compile_key *key )
{
   return sizeof *key;
}

static INLINE unsigned svga_fs_key_size( const struct svga_fs_compile_key *key )
{
   return (const char *)&key->tex[key->num_textures] - (const char *)key;
}

struct svga_shader_result *
svga_translate_fragment_program( const struct svga_fragment_shader *fs,
                                 const struct svga_fs_compile_key *fkey );

struct svga_shader_result *
svga_translate_vertex_program( const struct svga_vertex_shader *fs,
                               const struct svga_vs_compile_key *vkey );


void svga_destroy_shader_result( struct svga_shader_result *result );

#endif
