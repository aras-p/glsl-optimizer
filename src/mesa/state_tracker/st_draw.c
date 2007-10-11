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
#include "main/image.h"

#include "vbo/vbo.h"
#include "vbo/vbo_context.h"

#include "tnl/t_vp_build.h"

#include "st_atom.h"
#include "st_cache.h"
#include "st_context.h"
#include "st_cb_bufferobjects.h"
#include "st_draw.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_winsys.h"

#include "pipe/draw/draw_private.h"
#include "pipe/draw/draw_context.h"


static GLuint
pipe_vertex_format(GLenum format, GLuint size)
{
   static const GLuint float_fmts[4] = {
      PIPE_FORMAT_R32_FLOAT,
      PIPE_FORMAT_R32G32_FLOAT,
      PIPE_FORMAT_R32G32B32_FLOAT,
      PIPE_FORMAT_R32G32B32A32_FLOAT,
   };
   static const GLuint int_fmts[4] = {
      PIPE_FORMAT_R32_SSCALED,
      PIPE_FORMAT_R32G32_SSCALED,
      PIPE_FORMAT_R32G32B32_SSCALED,
      PIPE_FORMAT_R32G32B32A32_SSCALED,
   };

   assert(format >= GL_BYTE);
   assert(format <= GL_DOUBLE);
   assert(size >= 1);
   assert(size <= 4);

   switch (format) {
   case GL_FLOAT:
      return float_fmts[size - 1];
   case GL_INT:
      return int_fmts[size - 1];
   default:
      assert(0);
   }
}


#if 0
/**
 * Convert a mesa vertex attribute to a TGSI attribute
 */
static GLuint
tgsi_attrib_to_mesa_attrib(GLuint attr)
{
   switch (attr) {
   case TGSI_ATTRIB_POS:
      return VERT_ATTRIB_POS;
   case TGSI_ATTRIB_WEIGHT:
      return VERT_ATTRIB_WEIGHT;
   case TGSI_ATTRIB_NORMAL:
      return VERT_ATTRIB_NORMAL;
   case TGSI_ATTRIB_COLOR0:
      return VERT_ATTRIB_COLOR0;
   case TGSI_ATTRIB_COLOR1:
      return VERT_ATTRIB_COLOR1;
   case TGSI_ATTRIB_FOG:
      return VERT_ATTRIB_FOG;
   case TGSI_ATTRIB_COLOR_INDEX:
      return VERT_ATTRIB_COLOR_INDEX;
   case TGSI_ATTRIB_EDGEFLAG:
      return VERT_ATTRIB_EDGEFLAG;
   case TGSI_ATTRIB_TEX0:
      return VERT_ATTRIB_TEX0;
   case TGSI_ATTRIB_TEX1:
      return VERT_ATTRIB_TEX1;
   case TGSI_ATTRIB_TEX2:
      return VERT_ATTRIB_TEX2;
   case TGSI_ATTRIB_TEX3:
      return VERT_ATTRIB_TEX3;
   case TGSI_ATTRIB_TEX4:
      return VERT_ATTRIB_TEX4;
   case TGSI_ATTRIB_TEX5:
      return VERT_ATTRIB_TEX5;
   case TGSI_ATTRIB_TEX6:
      return VERT_ATTRIB_TEX6;
   case TGSI_ATTRIB_TEX7:
      return VERT_ATTRIB_TEX7;
   case TGSI_ATTRIB_VAR0:
      return VERT_ATTRIB_GENERIC0;
   case TGSI_ATTRIB_VAR1:
      return VERT_ATTRIB_GENERIC1;
   case TGSI_ATTRIB_VAR2:
      return VERT_ATTRIB_GENERIC2;
   case TGSI_ATTRIB_VAR3:
      return VERT_ATTRIB_GENERIC3;
   case TGSI_ATTRIB_VAR4:
      return VERT_ATTRIB_GENERIC4;
   case TGSI_ATTRIB_VAR5:
      return VERT_ATTRIB_GENERIC5;
   case TGSI_ATTRIB_VAR6:
      return VERT_ATTRIB_GENERIC6;
   case TGSI_ATTRIB_VAR7:
      return VERT_ATTRIB_GENERIC7;
   default:
      assert(0);
      return 0;
   }
}
#endif


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
   pipe->winsys->buffer_reference(pipe->winsys,
                                  &st->default_attrib_buffer, NULL);
}


/**
 * This function gets plugged into the VBO module and is called when
 * we have something to render.
 * Basically, translate the information into the format expected by pipe.
 */
void
st_draw_vbo(GLcontext *ctx,
            const struct gl_client_array **arrays,
            const struct _mesa_prim *prims,
            GLuint nr_prims,
            const struct _mesa_index_buffer *ib,
            GLuint min_index,
            GLuint max_index)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_winsys *winsys = pipe->winsys;
   const struct st_vertex_program *vp;
   const struct pipe_shader_state *vs;
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_SHADER_INPUTS];
   GLuint attr;

   /* sanity check for pointer arithmetic below */
   assert(sizeof(arrays[0]->Ptr[0]) == 1);

   st_validate_state(ctx->st);

   /* must get these after state validation! */
   vp = ctx->st->vp;
   vs = &ctx->st->state.vs->state;

   /* loop over TGSI shader inputs to determine vertex buffer
    * and attribute info
    */
   for (attr = 0; attr < vs->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      struct gl_buffer_object *bufobj = arrays[mesaAttr]->BufferObj;
      struct pipe_vertex_element velement;

      if (bufobj && bufobj->Name) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         assert(stobj->buffer);

         vbuffer[attr].buffer = NULL;
         winsys->buffer_reference(winsys, &vbuffer[attr].buffer, stobj->buffer);
         vbuffer[attr].buffer_offset = (unsigned) arrays[0]->Ptr;/* in bytes */
         velement.src_offset = arrays[mesaAttr]->Ptr - arrays[0]->Ptr;
      }
      else {
         /* attribute data is in user-space memory, not a VBO */
         uint bytes = (arrays[mesaAttr]->Size
                       * _mesa_sizeof_type(arrays[mesaAttr]->Type)
                       * (max_index + 1));

         /* wrap user data */
         vbuffer[attr].buffer
            = winsys->user_buffer_create(winsys,
                                         (void *) arrays[mesaAttr]->Ptr,
                                         bytes);
         vbuffer[attr].buffer_offset = 0;
         velement.src_offset = 0;
      }

      /* common-case setup */
      vbuffer[attr].pitch = arrays[mesaAttr]->StrideB; /* in bytes */
      vbuffer[attr].max_index = 0;  /* need this? */
      velement.vertex_buffer_index = attr;
      velement.dst_offset = 0; /* need this? */
      velement.src_format = pipe_vertex_format(arrays[mesaAttr]->Type,
                                               arrays[mesaAttr]->Size);
      assert(velement.src_format);

      /* tell pipe about this attribute */
      pipe->set_vertex_buffer(pipe, attr, &vbuffer[attr]);
      pipe->set_vertex_element(pipe, attr, &velement);
   }


   /* do actual drawing */
   if (ib) {
      /* indexed primitive */
      struct gl_buffer_object *bufobj = ib->obj;
      struct pipe_buffer_handle *indexBuf = NULL;
      unsigned indexSize, i;

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

      /* get/create the index buffer object */
      if (bufobj && bufobj->Name) {
         /* elements/indexes are in a real VBO */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         winsys->buffer_reference(winsys, &indexBuf, stobj->buffer);
      }
      else {
         /* element/indicies are in user space memory */
         indexBuf = winsys->user_buffer_create(winsys,
                                               (void *) ib->ptr,
                                               ib->count * indexSize);
      }

      /* draw */
      for (i = 0; i < nr_prims; i++) {
         pipe->draw_elements(pipe, indexBuf, indexSize,
                             prims[i].mode, prims[i].start, prims[i].count);
      }

      winsys->buffer_reference(winsys, &indexBuf, NULL);
   }
   else {
      /* non-indexed */
      GLuint i;
      for (i = 0; i < nr_prims; i++) {
         pipe->draw_arrays(pipe, prims[i].mode, prims[i].start, prims[i].count);
      }
   }

   /* unreference buffers (frees wrapped user-space buffer objects) */
   for (attr = 0; attr < vs->num_inputs; attr++) {
      winsys->buffer_reference(winsys, &vbuffer[attr].buffer, NULL);
   }
}



/**
 * Utility function for drawing simple primitives (such as quads for
 * glClear and glDrawPixels).  Coordinates are in screen space.
 * \param mode  one of PIPE_PRIM_x
 * \param numVertex  number of vertices
 * \param verts  vertex data (all attributes are float[4])
 * \param numAttribs  number of attributes per vertex
 * \param attribs  index of each attribute (0=pos, 3=color, etc)
 */
void 
st_draw_vertices(GLcontext *ctx, unsigned prim,
                 unsigned numVertex, float *verts,
                 unsigned numAttribs)
{
   const float width = ctx->DrawBuffer->Width;
   const float height = ctx->DrawBuffer->Height;
   const unsigned vertex_bytes = numVertex * numAttribs * 4 * sizeof(float);
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_buffer_handle *vbuf;
   struct pipe_vertex_buffer vbuffer;
   struct pipe_vertex_element velement;
   unsigned i;

   assert(numAttribs > 0);

   /* convert to clip coords */
   for (i = 0; i < numVertex; i++) {
      float x = verts[i * numAttribs * 4 + 0];
      float y = verts[i * numAttribs * 4 + 1];
      x = x / width * 2.0 - 1.0;
      y = y / height * 2.0 - 1.0;
      verts[i * numAttribs * 4 + 0] = x;
      verts[i * numAttribs * 4 + 1] = y;
   }

   /* XXX create one-time */
   vbuf = pipe->winsys->buffer_create(pipe->winsys, 32);
   pipe->winsys->buffer_data(pipe->winsys, vbuf, vertex_bytes, verts);

   /* tell pipe about the vertex buffer */
   vbuffer.buffer = vbuf;
   vbuffer.pitch = numAttribs * 4 * sizeof(float);  /* vertex size */
   vbuffer.buffer_offset = 0;
   pipe->set_vertex_buffer(pipe, 0, &vbuffer);

   /* tell pipe about the vertex attributes */
   for (i = 0; i < numAttribs; i++) {
      velement.src_offset = i * 4 * sizeof(GLfloat);
      velement.vertex_buffer_index = 0;
      velement.src_format = PIPE_FORMAT_R32G32B32A32_FLOAT;
      velement.dst_offset = 0;
      pipe->set_vertex_element(pipe, i, &velement);
   }

   /* draw */
   pipe->draw_arrays(pipe, prim, 0, numVertex);

   /* XXX: do one-time */
   pipe->winsys->buffer_reference(pipe->winsys, &vbuf, NULL);
}




/**
 * Called by VBO to draw arrays when in selection or feedback mode.
 * This is very much like the normal draw_vbo() function above.
 * Look at code refactoring some day.
 * Might move this into the failover module some day.
 */
void
st_feedback_draw_vbo(GLcontext *ctx,
                     const struct gl_client_array **arrays,
                     const struct _mesa_prim *prims,
                     GLuint nr_prims,
                     const struct _mesa_index_buffer *ib,
                     GLuint min_index,
                     GLuint max_index)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = st->pipe;
   struct draw_context *draw = st->draw;
   struct pipe_winsys *winsys = pipe->winsys;
   const struct st_vertex_program *vp;
   const struct pipe_shader_state *vs;
   struct pipe_buffer_handle *index_buffer_handle = 0;
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_SHADER_INPUTS];
   GLuint attr, i;

   assert(ctx->RenderMode == GL_SELECT ||
          ctx->RenderMode == GL_FEEDBACK);
   assert(draw);

   st_validate_state(ctx->st);

   /* must get these after state validation! */
   vp = ctx->st->vp;
   vs = &ctx->st->state.vs->state;

   /*
    * Set up the draw module's state.
    *
    * We'd like to do this less frequently, but the normal state-update
    * code sends state updates to the pipe, not to our private draw module.
    */
   assert(draw);
   draw_set_viewport_state(draw, &st->state.viewport);
   draw_set_clip_state(draw, &st->state.clip);
   draw_set_rasterizer_state(draw, &st->state.rasterizer->state);
   draw_bind_vertex_shader(draw, st->state.vs->data);

   /* loop over TGSI shader inputs to determine vertex buffer
    * and attribute info
    */
   for (attr = 0; attr < vs->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      struct gl_buffer_object *bufobj = arrays[mesaAttr]->BufferObj;
      struct pipe_vertex_element velement;
      void *map;

      if (bufobj && bufobj->Name) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         assert(stobj->buffer);

         vbuffer[attr].buffer = NULL;
         winsys->buffer_reference(winsys, &vbuffer[attr].buffer, stobj->buffer);
         vbuffer[attr].buffer_offset = (unsigned) arrays[0]->Ptr;/* in bytes */
         velement.src_offset = arrays[mesaAttr]->Ptr - arrays[0]->Ptr;
      }
      else {
         /* attribute data is in user-space memory, not a VBO */
         uint bytes = (arrays[mesaAttr]->Size
                       * _mesa_sizeof_type(arrays[mesaAttr]->Type)
                       * (max_index + 1));

         /* wrap user data */
         vbuffer[attr].buffer
            = winsys->user_buffer_create(winsys,
                                         (void *) arrays[mesaAttr]->Ptr,
                                         bytes);
         vbuffer[attr].buffer_offset = 0;
         velement.src_offset = 0;
      }

      /* common-case setup */
      vbuffer[attr].pitch = arrays[mesaAttr]->StrideB; /* in bytes */
      vbuffer[attr].max_index = 0;  /* need this? */
      velement.vertex_buffer_index = attr;
      velement.dst_offset = 0; /* need this? */
      velement.src_format = pipe_vertex_format(arrays[mesaAttr]->Type,
                                               arrays[mesaAttr]->Size);
      assert(velement.src_format);

      /* tell draw about this attribute */
      draw_set_vertex_buffer(draw, attr, &vbuffer[attr]);
      draw_set_vertex_element(draw, attr, &velement);

      /* map the attrib buffer */
      map = pipe->winsys->buffer_map(pipe->winsys,
                                     vbuffer[attr].buffer,
                                     PIPE_BUFFER_FLAG_READ);
      draw_set_mapped_vertex_buffer(draw, attr, map);
   }

   if (ib) {
      unsigned indexSize;
      struct gl_buffer_object *bufobj = ib->obj;
      struct st_buffer_object *stobj = st_buffer_object(bufobj);
      index_buffer_handle = stobj->buffer;
      void *map;

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

      map = pipe->winsys->buffer_map(pipe->winsys,
                                     index_buffer_handle,
                                     PIPE_BUFFER_FLAG_READ);
      draw_set_mapped_element_buffer(draw, indexSize, map);
   }


   /* draw here */
   for (i = 0; i < nr_prims; i++) {
      draw_arrays(draw, prims[i].mode, prims[i].start, prims[i].count);
   }


   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < PIPE_ATTRIB_MAX; i++) {
      if (draw->vertex_buffer[i].buffer) {
         pipe->winsys->buffer_unmap(pipe->winsys,
                                    draw->vertex_buffer[i].buffer);
         draw_set_mapped_vertex_buffer(draw, i, NULL);
      }
   }
   if (ib) {
      pipe->winsys->buffer_unmap(pipe->winsys, index_buffer_handle);
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }
}



/* This is all a hack to keep using tnl until we have vertex programs
 * up and running.
 */
void st_init_draw( struct st_context *st )
{
   GLcontext *ctx = st->ctx;
   struct vbo_context *vbo = (struct vbo_context *) ctx->swtnl_im;

   create_default_attribs_buffer(st);

   assert(vbo);
   assert(vbo->draw_prims);
   vbo->draw_prims = st_draw_vbo;

   _tnl_ProgramCacheInit( ctx );
}


void st_destroy_draw( struct st_context *st )
{
   destroy_default_attribs_buffer(st);
}


