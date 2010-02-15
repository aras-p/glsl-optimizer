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

   struct pipe_shader_state tgsi;
   void *driver_shader;

   /** Program prefixed with glBitmap prologue */
   struct st_fragment_program *bitmap_program;
   uint bitmap_sampler;
};



struct st_vp_varient_key
{
   boolean passthrough_edgeflags;
};


/**
 * This represents a vertex program, especially translated to match
 * the inputs of a particular fragment shader.
 */
struct st_vp_varient
{
   /* Parameters which generated this translated version of a vertex
    * shader:
    */
   struct st_vp_varient_key key;

   /**
    * TGSI tokens (to later generate a 'draw' module shader for
    * selection/feedback/rasterpos)
    */
   struct pipe_shader_state tgsi;

   /** Driver's compiled shader */
   void *driver_shader;

   /** For using our private draw module (glRasterPos) */
   struct draw_vertex_shader *draw_shader;

   /** Next in linked list */
   struct st_vp_varient *next;  

   /** similar to that in st_vertex_program, but with information about edgeflags too */
   GLuint num_inputs;
};




/**
 * Derived from Mesa gl_fragment_program:
 */
struct st_vertex_program
{
   struct gl_vertex_program Base;  /**< The Mesa vertex program */
   GLuint serialNo, lastSerialNo;

   /** maps a Mesa VERT_ATTRIB_x to a packed TGSI input index */
   GLuint input_to_index[VERT_ATTRIB_MAX];
   /** maps a TGSI input index back to a Mesa VERT_ATTRIB_x */
   GLuint index_to_input[PIPE_MAX_SHADER_INPUTS];
   GLuint num_inputs;

   /** Maps VERT_RESULT_x to slot */
   GLuint result_to_output[VERT_RESULT_MAX];
   ubyte output_semantic_name[VERT_RESULT_MAX];
   ubyte output_semantic_index[VERT_RESULT_MAX];
   GLuint num_outputs;

   /** List of translated varients of this vertex program.
    */
   struct st_vp_varient *varients;
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
                              struct st_fragment_program *fp);


/* Called after program string change, discard all previous
 * compilation results.
 */
extern void
st_prepare_vertex_program(struct st_context *st,
                          struct st_vertex_program *stvp);

extern struct st_vp_varient *
st_translate_vertex_program(struct st_context *st,
                            struct st_vertex_program *stvp,
                            const struct st_vp_varient_key *key);

void
st_vp_release_varients( struct st_context *st,
                        struct st_vertex_program *stvp );

extern void
st_print_shaders(GLcontext *ctx);


#endif
