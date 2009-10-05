/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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

/**
 * @file
 * Simple vertex/fragment shader generators.
 *  
 * @author Brian Paul
 */


#include "pipe/p_context.h"
#include "pipe/p_shader_tokens.h"
#include "util/u_simple_shaders.h"
#include "tgsi/tgsi_ureg.h"



/**
 * Make simple vertex pass-through shader.
 */
void *
util_make_vertex_passthrough_shader(struct pipe_context *pipe,
                                    uint num_attribs,
                                    const uint *semantic_names,
                                    const uint *semantic_indexes)
                                    
{
   struct ureg_program *ureg;
   uint i;

   ureg = ureg_create( TGSI_PROCESSOR_VERTEX );
   if (ureg == NULL)
      return NULL;

   for (i = 0; i < num_attribs; i++) {
      struct ureg_src src;
      struct ureg_dst dst;

      src = ureg_DECL_vs_input( ureg, i );
      
      dst = ureg_DECL_output( ureg,
                              semantic_names[i],
                              semantic_indexes[i]);
      
      ureg_MOV( ureg, dst, src );
   }

   ureg_END( ureg );

   return ureg_create_shader_and_destroy( ureg, pipe );
}




/**
 * Make simple fragment texture shader:
 *  IMM {0,0,0,1}                         // (if writemask != 0xf)
 *  MOV OUT[0], IMM[0]                    // (if writemask != 0xf)
 *  TEX OUT[0].writemask, IN[0], SAMP[0], 2D;
 *  END;
 */
void *
util_make_fragment_tex_shader_writemask(struct pipe_context *pipe,
                                        unsigned writemask )
{
   struct ureg_program *ureg;
   struct ureg_src sampler;
   struct ureg_src tex;
   struct ureg_dst out;

   ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
   if (ureg == NULL)
      return NULL;
   
   sampler = ureg_DECL_sampler( ureg, 0 );

   tex = ureg_DECL_fs_input( ureg, 
                             TGSI_SEMANTIC_GENERIC, 0, 
                             TGSI_INTERPOLATE_PERSPECTIVE );

   out = ureg_DECL_output( ureg, 
                           TGSI_SEMANTIC_COLOR,
                           0 );

   if (writemask != TGSI_WRITEMASK_XYZW) {
      struct ureg_src imm = ureg_imm4f( ureg, 0, 0, 0, 1 );

      ureg_MOV( ureg, out, imm );
   }

   ureg_TEX( ureg, 
             ureg_writemask(out, writemask),
             TGSI_TEXTURE_2D, tex, sampler );
   ureg_END( ureg );

   return ureg_create_shader_and_destroy( ureg, pipe );
}

void *
util_make_fragment_tex_shader(struct pipe_context *pipe )
{
   return util_make_fragment_tex_shader_writemask( pipe,
                                                   TGSI_WRITEMASK_XYZW );
}



/**
 * Make simple fragment color pass-through shader.
 */
void *
util_make_fragment_passthrough_shader(struct pipe_context *pipe)
{
   struct ureg_program *ureg;
   struct ureg_src src;
   struct ureg_dst dst;

   ureg = ureg_create( TGSI_PROCESSOR_FRAGMENT );
   if (ureg == NULL)
      return NULL;

   src = ureg_DECL_fs_input( ureg, TGSI_SEMANTIC_COLOR, 0, 
                             TGSI_INTERPOLATE_PERSPECTIVE );

   dst = ureg_DECL_output( ureg, TGSI_SEMANTIC_COLOR, 0 );

   ureg_MOV( ureg, dst, src );
   ureg_END( ureg );

   return ureg_create_shader_and_destroy( ureg, pipe );
}


