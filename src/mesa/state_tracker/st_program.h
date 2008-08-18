/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
    

#ifndef ST_PROGRAM_H
#define ST_PROGRAM_H

#include "main/mtypes.h"
#include "shader/program.h"
#include "pipe/p_shader_tokens.h"


struct cso_fragment_shader;
struct cso_vertex_shader;
struct translated_vertex_program;


/**
 * Derived from Mesa gl_fragment_program:
 */
struct st_fragment_program
{
   struct gl_fragment_program Base;
   GLuint serialNo;

   GLuint input_to_slot[FRAG_ATTRIB_MAX];  /**< Maps FRAG_ATTRIB_x to slot */
   GLuint num_input_slots;

   /** map FP input back to VP output */
   GLuint input_map[PIPE_MAX_SHADER_INPUTS];

   ubyte input_semantic_name[PIPE_MAX_SHADER_INPUTS];
   ubyte input_semantic_index[PIPE_MAX_SHADER_INPUTS];

   struct pipe_shader_state state;
   void *driver_shader;

   GLuint param_state;

   /** List of vertex programs which have been translated such that their
    * outputs match this fragment program's inputs.
    */
   struct translated_vertex_program *vertex_programs;

   /** Program prefixed with glBitmap prologue */
   struct st_fragment_program *bitmap_program;
   uint bitmap_sampler;
};


/**
 * Derived from Mesa gl_fragment_program:
 */
struct st_vertex_program
{
   struct gl_vertex_program Base;  /**< The Mesa vertex program */
   GLuint serialNo;

   /** maps a Mesa VERT_ATTRIB_x to a packed TGSI input index */
   GLuint input_to_index[VERT_ATTRIB_MAX];
   /** maps a TGSI input index back to a Mesa VERT_ATTRIB_x */
   GLuint index_to_input[PIPE_MAX_SHADER_INPUTS];

   GLuint num_inputs;

   struct pipe_shader_state state;
   void *driver_shader;

   /** For using our private draw module (glRasterPos) */
   struct draw_vertex_shader *draw_shader;

   GLuint param_state;
};


static INLINE struct st_fragment_program *
st_fragment_program( struct gl_fragment_program *fp )
{
   return (struct st_fragment_program *)fp;
}


static INLINE struct st_vertex_program *
st_vertex_program( struct gl_vertex_program *vp )
{
   return (struct st_vertex_program *)vp;
}


static INLINE void
st_reference_vertprog(struct st_context *st,
                      struct st_vertex_program **ptr,
                      struct st_vertex_program *prog)
{
   _mesa_reference_program(st->ctx,
                           (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}

static INLINE void
st_reference_fragprog(struct st_context *st,
                      struct st_fragment_program **ptr,
                      struct st_fragment_program *prog)
{
   _mesa_reference_program(st->ctx,
                           (struct gl_program **) ptr,
                           (struct gl_program *) prog);
}


extern void
st_translate_fragment_program(struct st_context *st,
                              struct st_fragment_program *fp,
                              const GLuint inputMapping[]);


extern void
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *vp,
                            const GLuint vert_output_to_slot[],
                            const ubyte *fs_input_semantic_name,
                            const ubyte *fs_input_semantic_index);


#endif
