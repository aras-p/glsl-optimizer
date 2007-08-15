/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "main/imports.h"

#include "vbo/vbo.h"

#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_cb_bufferobjects.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"


/*
 * TNL stage which feeds into the above.
 *
 * XXX: this needs to go into each driver using this code, because we
 * cannot make the leap from ctx->draw_context in this file.  The
 * driver needs to customize tnl anyway, so this isn't a big deal.
 */
static GLboolean draw( GLcontext * ctx, struct tnl_pipeline_stage *stage )
{
   struct st_context *st = st_context(ctx);
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;

   /* Validate driver and pipe state:
    */
   st_validate_state( st );

   /* Call into the new draw code to handle the VB:
    */
   st->pipe->draw_vb( st->pipe, VB );
   
   /* Finished 
    */
   return GL_FALSE;
}

const struct tnl_pipeline_stage st_draw = {
   "check state and draw",
   NULL,
   NULL,
   NULL,
   NULL,
   draw
};

static const struct tnl_pipeline_stage *st_pipeline[] = {
   &_tnl_vertex_transform_stage,
   &_tnl_vertex_cull_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_vertex_program_stage,
   &st_draw,     /* ADD: escape to pipe */
   0,
};



/**
 * This function gets plugged into the VBO module and is called when
 * we have something to render.
 * Basically, translate the information into the format expected by pipe.
 */
static void
draw_vbo(GLcontext *ctx,
         const struct gl_client_array **arrays,
         const struct _mesa_prim *prims,
         GLuint nr_prims,
         const struct _mesa_index_buffer *ib,
         GLuint min_index,
         GLuint max_index)
{
   struct pipe_context *pipe = ctx->st->pipe;
   GLuint attr;

   /* tell pipe about the vertex array element/attributes */
   for (attr = 0; attr < 16; attr++) {
      struct gl_buffer_object *bufobj = arrays[attr]->BufferObj;
      if (bufobj && bufobj->Name) {
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         struct pipe_buffer_handle *buffer = stobj->buffer;
         GLenum type = arrays[attr]->Type;
         GLint  size = arrays[attr]->Size;
         struct pipe_vertex_buffer vbuffer;
         struct pipe_vertex_element velement;

         vbuffer.pitch = 0;
         vbuffer.max_index = 0;
         vbuffer.buffer = NULL;
         vbuffer.buffer_offset = 0;

         velement.src_offset = 0;
         velement.vertex_buffer_index = attr;
         velement.dst_offset = 0;
         velement.src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;

         pipe->set_vertex_buffer(pipe, attr, &vbuffer);
         pipe->set_vertex_element(pipe, attr, &velement);
      }
   }

   /* do actual drawing */
}




/* This is all a hack to keep using tnl until we have vertex programs
 * up and running.
 */
void st_init_draw( struct st_context *st )
{
   GLcontext *ctx = st->ctx;

   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, st_pipeline );
}


void st_destroy_draw( struct st_context *st )
{
   /* Nothing to do. 
    */
}


