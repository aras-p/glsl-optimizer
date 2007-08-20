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
#include "tnl/t_vp_build.h"  /* USE_NEW_DRAW */

#include "st_context.h"
#include "st_atom.h"
#include "st_draw.h"
#include "st_cb_bufferobjects.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"

#include "vbo/vbo_context.h"

/*
 * Enabling this causes the VBO module to call draw_vbo() below,
 * bypassing the T&L module.  This only works with VBO-based demos,
 * such as progs/test/bufferobj.c
 */
#define USE_NEW_DRAW 01


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
#if USE_NEW_DRAW
   &_tnl_vertex_program_stage,
#else
   &_tnl_vertex_transform_stage,
   &_tnl_vertex_cull_stage,
   &_tnl_normal_transform_stage,
   &_tnl_lighting_stage,
   &_tnl_fog_coordinate_stage,
   &_tnl_texgen_stage,
   &_tnl_texture_transform_stage,
   &_tnl_point_attenuation_stage,
   &_tnl_vertex_program_stage,
#endif
   &st_draw,     /* ADD: escape to pipe */
   0,
};



static GLuint
pipe_vertex_format(GLenum format, GLuint size)
{
   static const GLuint float_fmts[4] = {
      PIPE_FORMAT_R32_FLOAT,
      PIPE_FORMAT_R32G32_FLOAT,
      PIPE_FORMAT_R32G32B32_FLOAT,
      PIPE_FORMAT_R32G32B32A32_FLOAT,
   };

   assert(format >= GL_BYTE);
   assert(format <= GL_DOUBLE);
   assert(size >= 1);
   assert(size <= 4);

   switch (format) {
   case GL_FLOAT:
      return float_fmts[size - 1];
   default:
      assert(0);
   }
}



/**
 * The default attribute buffer is basically a copy of the
 * ctx->Current.Attrib[] array.  It's used when the vertex program
 * references an attribute for which we don't have a VBO/array.
 */
static void
create_default_attribs_buffer(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   st->default_attrib_buffer = pipe->winsys->buffer_create( pipe->winsys, 32 );
}


static void
destroy_default_attribs_buffer(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   pipe->winsys->buffer_unreference(pipe->winsys, &st->default_attrib_buffer);
}


static void
update_default_attribs_buffer(GLcontext *ctx)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_buffer_handle *buf = ctx->st->default_attrib_buffer;
   const unsigned size = sizeof(ctx->Current.Attrib);
   const void *data = ctx->Current.Attrib;
   pipe->winsys->buffer_data(pipe->winsys, buf, size, data);
}


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
   GLuint attr, i;
   GLbitfield attrsNeeded;
   const unsigned attr0_offset = (unsigned) arrays[0]->Ptr;

   st_validate_state(ctx->st);
   update_default_attribs_buffer(ctx);

   /* this must be after state validation */
   attrsNeeded = ctx->st->state.vs.inputs_read;

   /* tell pipe about the vertex array element/attributes */
   for (attr = 0; attr < 16; attr++) {
      struct pipe_vertex_buffer vbuffer;
      struct pipe_vertex_element velement;

      vbuffer.buffer = NULL;
      vbuffer.pitch = 0;
      velement.src_offset = 0;
      velement.vertex_buffer_index = 0;
      velement.src_format = 0;

      if (attrsNeeded & (1 << attr)) {
         struct gl_buffer_object *bufobj = arrays[attr]->BufferObj;

         if (bufobj && bufobj->Name) {
            struct st_buffer_object *stobj = st_buffer_object(bufobj);
            /* Recall that for VBOs, the gl_client_array->Ptr field is
             * really an offset from the start of the VBO, not a pointer.
             */
            unsigned offset = (unsigned) arrays[attr]->Ptr;

            assert(stobj->buffer);

            vbuffer.buffer = stobj->buffer;
            vbuffer.buffer_offset = attr0_offset;  /* in bytes */
            vbuffer.pitch = arrays[attr]->StrideB; /* in bytes */
            vbuffer.max_index = 0;  /* need this? */

            velement.src_offset = offset - attr0_offset; /* bytes */
            velement.vertex_buffer_index = attr;
            velement.dst_offset = 0; /* need this? */
            velement.src_format = pipe_vertex_format(arrays[attr]->Type,
                                                     arrays[attr]->Size);
            assert(velement.src_format);
         }
         else {
            /* use the default attribute buffer */
            vbuffer.buffer = ctx->st->default_attrib_buffer;
            vbuffer.buffer_offset = 0;
            vbuffer.pitch = 0; /* must be zero! */
            vbuffer.max_index = 1;

            velement.src_offset = attr * 4 * sizeof(GLfloat);
            velement.vertex_buffer_index = attr;
            velement.dst_offset = 0;
            velement.src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
         }
      }

      if (attr == 0)
         assert(vbuffer.buffer);

      pipe->set_vertex_buffer(pipe, attr, &vbuffer);
      pipe->set_vertex_element(pipe, attr, &velement);
   }

   /* do actual drawing */
   if (ib) {
      /* indexed primitive */
      struct gl_buffer_object *bufobj = ib->obj;
      struct pipe_buffer_handle *bh = NULL;
      unsigned indexSize;

      if (bufobj && bufobj->Name) {
         /* elements/indexes are in a real VBO */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         bh = stobj->buffer;
         switch (ib->type) {
         case GL_UNSIGNED_INT:
            indexSize = 4;
            break;
         case GL_UNSIGNED_SHORT:
            indexSize = 2;
            break;
         default:
            assert(0);
         }
      }
      else {
         assert(0);
      }

      for (i = 0; i < nr_prims; i++) {
         pipe->draw_elements(pipe, bh, indexSize,
                              prims[i].mode, prims[i].start, prims[i].count);
      }
   }
   else {
      /* non-indexed */
      for (i = 0; i < nr_prims; i++) {
         pipe->draw_arrays(pipe, prims[i].mode, prims[i].start, prims[i].count);
      }
   }
}




/* This is all a hack to keep using tnl until we have vertex programs
 * up and running.
 */
void st_init_draw( struct st_context *st )
{
   GLcontext *ctx = st->ctx;

#if USE_NEW_DRAW
   struct vbo_context *vbo = (struct vbo_context *) ctx->swtnl_im;

   create_default_attribs_buffer(st);

   assert(vbo);
   assert(vbo->draw_prims);
   vbo->draw_prims = draw_vbo;

#endif
   _tnl_destroy_pipeline( ctx );
   _tnl_install_pipeline( ctx, st_pipeline );

   /* USE_NEW_DRAW */
   _tnl_ProgramCacheInit( ctx );
}


void st_destroy_draw( struct st_context *st )
{
   destroy_default_attribs_buffer(st);
}


