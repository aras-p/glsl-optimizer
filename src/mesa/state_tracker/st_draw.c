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
 * This file implements the st_draw_vbo() function which is called from
 * Mesa's VBO module.  All point/line/triangle rendering is done through
 * this function whether the user called glBegin/End, glDrawArrays,
 * glDrawElements, glEvalMesh, or glCalList, etc.
 *
 * We basically convert the VBO's vertex attribute/array information into
 * Gallium vertex state, bind the vertex buffer objects and call
 * pipe->draw_vbo().
 *
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */


#include "main/imports.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/macros.h"
#include "main/mfeatures.h"
#include "program/prog_uniform.h"

#include "vbo/vbo.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bufferobjects.h"
#include "st_draw.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_prim.h"
#include "util/u_draw_quad.h"
#include "draw/draw_context.h"
#include "cso_cache/cso_context.h"


static GLuint double_types[4] = {
   PIPE_FORMAT_R64_FLOAT,
   PIPE_FORMAT_R64G64_FLOAT,
   PIPE_FORMAT_R64G64B64_FLOAT,
   PIPE_FORMAT_R64G64B64A64_FLOAT
};

static GLuint float_types[4] = {
   PIPE_FORMAT_R32_FLOAT,
   PIPE_FORMAT_R32G32_FLOAT,
   PIPE_FORMAT_R32G32B32_FLOAT,
   PIPE_FORMAT_R32G32B32A32_FLOAT
};

static GLuint half_float_types[4] = {
   PIPE_FORMAT_R16_FLOAT,
   PIPE_FORMAT_R16G16_FLOAT,
   PIPE_FORMAT_R16G16B16_FLOAT,
   PIPE_FORMAT_R16G16B16A16_FLOAT
};

static GLuint uint_types_norm[4] = {
   PIPE_FORMAT_R32_UNORM,
   PIPE_FORMAT_R32G32_UNORM,
   PIPE_FORMAT_R32G32B32_UNORM,
   PIPE_FORMAT_R32G32B32A32_UNORM
};

static GLuint uint_types_scale[4] = {
   PIPE_FORMAT_R32_USCALED,
   PIPE_FORMAT_R32G32_USCALED,
   PIPE_FORMAT_R32G32B32_USCALED,
   PIPE_FORMAT_R32G32B32A32_USCALED
};

static GLuint int_types_norm[4] = {
   PIPE_FORMAT_R32_SNORM,
   PIPE_FORMAT_R32G32_SNORM,
   PIPE_FORMAT_R32G32B32_SNORM,
   PIPE_FORMAT_R32G32B32A32_SNORM
};

static GLuint int_types_scale[4] = {
   PIPE_FORMAT_R32_SSCALED,
   PIPE_FORMAT_R32G32_SSCALED,
   PIPE_FORMAT_R32G32B32_SSCALED,
   PIPE_FORMAT_R32G32B32A32_SSCALED
};

static GLuint ushort_types_norm[4] = {
   PIPE_FORMAT_R16_UNORM,
   PIPE_FORMAT_R16G16_UNORM,
   PIPE_FORMAT_R16G16B16_UNORM,
   PIPE_FORMAT_R16G16B16A16_UNORM
};

static GLuint ushort_types_scale[4] = {
   PIPE_FORMAT_R16_USCALED,
   PIPE_FORMAT_R16G16_USCALED,
   PIPE_FORMAT_R16G16B16_USCALED,
   PIPE_FORMAT_R16G16B16A16_USCALED
};

static GLuint short_types_norm[4] = {
   PIPE_FORMAT_R16_SNORM,
   PIPE_FORMAT_R16G16_SNORM,
   PIPE_FORMAT_R16G16B16_SNORM,
   PIPE_FORMAT_R16G16B16A16_SNORM
};

static GLuint short_types_scale[4] = {
   PIPE_FORMAT_R16_SSCALED,
   PIPE_FORMAT_R16G16_SSCALED,
   PIPE_FORMAT_R16G16B16_SSCALED,
   PIPE_FORMAT_R16G16B16A16_SSCALED
};

static GLuint ubyte_types_norm[4] = {
   PIPE_FORMAT_R8_UNORM,
   PIPE_FORMAT_R8G8_UNORM,
   PIPE_FORMAT_R8G8B8_UNORM,
   PIPE_FORMAT_R8G8B8A8_UNORM
};

static GLuint ubyte_types_scale[4] = {
   PIPE_FORMAT_R8_USCALED,
   PIPE_FORMAT_R8G8_USCALED,
   PIPE_FORMAT_R8G8B8_USCALED,
   PIPE_FORMAT_R8G8B8A8_USCALED
};

static GLuint byte_types_norm[4] = {
   PIPE_FORMAT_R8_SNORM,
   PIPE_FORMAT_R8G8_SNORM,
   PIPE_FORMAT_R8G8B8_SNORM,
   PIPE_FORMAT_R8G8B8A8_SNORM
};

static GLuint byte_types_scale[4] = {
   PIPE_FORMAT_R8_SSCALED,
   PIPE_FORMAT_R8G8_SSCALED,
   PIPE_FORMAT_R8G8B8_SSCALED,
   PIPE_FORMAT_R8G8B8A8_SSCALED
};

static GLuint fixed_types[4] = {
   PIPE_FORMAT_R32_FIXED,
   PIPE_FORMAT_R32G32_FIXED,
   PIPE_FORMAT_R32G32B32_FIXED,
   PIPE_FORMAT_R32G32B32A32_FIXED
};



/**
 * Return a PIPE_FORMAT_x for the given GL datatype and size.
 */
enum pipe_format
st_pipe_vertex_format(GLenum type, GLuint size, GLenum format,
                      GLboolean normalized)
{
   assert((type >= GL_BYTE && type <= GL_DOUBLE) ||
          type == GL_FIXED || type == GL_HALF_FLOAT);
   assert(size >= 1);
   assert(size <= 4);
   assert(format == GL_RGBA || format == GL_BGRA);

   if (format == GL_BGRA) {
      /* this is an odd-ball case */
      assert(type == GL_UNSIGNED_BYTE);
      assert(normalized);
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   }

   if (normalized) {
      switch (type) {
      case GL_DOUBLE: return double_types[size-1];
      case GL_FLOAT: return float_types[size-1];
      case GL_HALF_FLOAT: return half_float_types[size-1];
      case GL_INT: return int_types_norm[size-1];
      case GL_SHORT: return short_types_norm[size-1];
      case GL_BYTE: return byte_types_norm[size-1];
      case GL_UNSIGNED_INT: return uint_types_norm[size-1];
      case GL_UNSIGNED_SHORT: return ushort_types_norm[size-1];
      case GL_UNSIGNED_BYTE: return ubyte_types_norm[size-1];
      case GL_FIXED: return fixed_types[size-1];
      default: assert(0); return 0;
      }
   }
   else {
      switch (type) {
      case GL_DOUBLE: return double_types[size-1];
      case GL_FLOAT: return float_types[size-1];
      case GL_HALF_FLOAT: return half_float_types[size-1];
      case GL_INT: return int_types_scale[size-1];
      case GL_SHORT: return short_types_scale[size-1];
      case GL_BYTE: return byte_types_scale[size-1];
      case GL_UNSIGNED_INT: return uint_types_scale[size-1];
      case GL_UNSIGNED_SHORT: return ushort_types_scale[size-1];
      case GL_UNSIGNED_BYTE: return ubyte_types_scale[size-1];
      case GL_FIXED: return fixed_types[size-1];
      default: assert(0); return 0;
      }
   }
   return PIPE_FORMAT_NONE; /* silence compiler warning */
}


/**
 * This is very similar to vbo_all_varyings_in_vbos() but we test
 * the stride.  See bug 38626.
 */
static GLboolean
all_varyings_in_vbos(const struct gl_client_array *arrays[])
{
   GLuint i;
   
   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      if (arrays[i]->StrideB && !_mesa_is_bufferobj(arrays[i]->BufferObj))
	 return GL_FALSE;

   return GL_TRUE;
}


/**
 * Examine the active arrays to determine if we have interleaved
 * vertex arrays all living in one VBO, or all living in user space.
 */
static GLboolean
is_interleaved_arrays(const struct st_vertex_program *vp,
                      const struct st_vp_variant *vpv,
                      const struct gl_client_array **arrays)
{
   GLuint attr;
   const struct gl_buffer_object *firstBufObj = NULL;
   GLint firstStride = -1;
   const GLubyte *firstPtr = NULL;
   GLboolean userSpaceBuffer = GL_FALSE;

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      const struct gl_buffer_object *bufObj = array->BufferObj;
      const GLsizei stride = array->StrideB; /* in bytes */

      if (attr == 0) {
         /* save info about the first array */
         firstStride = stride;
         firstPtr = array->Ptr;         
         firstBufObj = bufObj;
         userSpaceBuffer = !bufObj || !bufObj->Name;
      }
      else {
         /* check if other arrays interleave with the first, in same buffer */
         if (stride != firstStride)
            return GL_FALSE; /* strides don't match */

         if (bufObj != firstBufObj)
            return GL_FALSE; /* arrays in different VBOs */

         if (abs(array->Ptr - firstPtr) > firstStride)
            return GL_FALSE; /* arrays start too far apart */

         if ((!bufObj || !_mesa_is_bufferobj(bufObj)) != userSpaceBuffer)
            return GL_FALSE; /* mix of VBO and user-space arrays */
      }
   }

   return GL_TRUE;
}


/**
 * Set up for drawing interleaved arrays that all live in one VBO
 * or all live in user space.
 * \param vbuffer  returns vertex buffer info
 * \param velements  returns vertex element info
 */
static void
setup_interleaved_attribs(struct gl_context *ctx,
                          const struct st_vertex_program *vp,
                          const struct st_vp_variant *vpv,
                          const struct gl_client_array **arrays,
                          struct pipe_vertex_buffer *vbuffer,
                          struct pipe_vertex_element velements[],
                          unsigned max_index,
                          unsigned num_instances)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   GLuint attr;
   const GLubyte *low_addr = NULL;

   /* Find the lowest address of the arrays we're drawing */
   if (vpv->num_inputs) {
      low_addr = arrays[vp->index_to_input[0]]->Ptr;

      for (attr = 1; attr < vpv->num_inputs; attr++) {
         const GLubyte *start = arrays[vp->index_to_input[attr]]->Ptr;
         low_addr = MIN2(low_addr, start);
      }
   }

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      struct gl_buffer_object *bufobj = array->BufferObj;
      struct st_buffer_object *stobj = st_buffer_object(bufobj);
      unsigned src_offset = (unsigned) (array->Ptr - low_addr);
      GLuint element_size = array->_ElementSize;
      GLsizei stride = array->StrideB;

      assert(element_size == array->Size * _mesa_sizeof_type(array->Type));

      if (attr == 0) {
         if (bufobj && _mesa_is_bufferobj(bufobj)) {
            vbuffer->buffer = NULL;
            pipe_resource_reference(&vbuffer->buffer, stobj->buffer);
            vbuffer->buffer_offset = pointer_to_offset(low_addr);
         }
         else {
            uint divisor = array->InstanceDivisor;
            uint last_index = divisor ? num_instances / divisor : max_index;
            uint bytes = src_offset + stride * last_index + element_size;

            vbuffer->buffer = pipe_user_buffer_create(pipe->screen,
                                                      (void*) low_addr,
                                                      bytes,
                                                      PIPE_BIND_VERTEX_BUFFER);
            vbuffer->buffer_offset = 0;

            /* Track user vertex buffers. */
            pipe_resource_reference(&st->user_attrib[0].buffer, vbuffer->buffer);
            st->user_attrib[0].element_size = element_size;
            st->user_attrib[0].stride = stride;
            st->num_user_attribs = 1;
         }
         vbuffer->stride = stride; /* in bytes */
      }

      velements[attr].src_offset = src_offset;
      velements[attr].instance_divisor = array->InstanceDivisor;
      velements[attr].vertex_buffer_index = 0;
      velements[attr].src_format = st_pipe_vertex_format(array->Type,
                                                         array->Size,
                                                         array->Format,
                                                         array->Normalized);
      assert(velements[attr].src_format);
   }
}


/**
 * Set up a separate pipe_vertex_buffer and pipe_vertex_element for each
 * vertex attribute.
 * \param vbuffer  returns vertex buffer info
 * \param velements  returns vertex element info
 */
static void
setup_non_interleaved_attribs(struct gl_context *ctx,
                              const struct st_vertex_program *vp,
                              const struct st_vp_variant *vpv,
                              const struct gl_client_array **arrays,
                              struct pipe_vertex_buffer vbuffer[],
                              struct pipe_vertex_element velements[],
                              unsigned max_index,
                              unsigned num_instances)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   GLuint attr;

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      struct gl_buffer_object *bufobj = array->BufferObj;
      GLuint element_size = array->_ElementSize;
      GLsizei stride = array->StrideB;

      assert(element_size == array->Size * _mesa_sizeof_type(array->Type));

      if (bufobj && _mesa_is_bufferobj(bufobj)) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         assert(stobj->buffer);

         vbuffer[attr].buffer = NULL;
         pipe_resource_reference(&vbuffer[attr].buffer, stobj->buffer);
         vbuffer[attr].buffer_offset = pointer_to_offset(array->Ptr);
      }
      else {
         /* wrap user data */
         uint bytes;
         void *ptr;

         if (array->Ptr) {
            uint divisor = array->InstanceDivisor;
            uint last_index = divisor ? num_instances / divisor : max_index;

            bytes = stride * last_index + element_size;

            ptr = (void *) array->Ptr;
         }
         else {
            /* no array, use ctx->Current.Attrib[] value */
            bytes = element_size = sizeof(ctx->Current.Attrib[0]);
            ptr = (void *) ctx->Current.Attrib[mesaAttr];
            stride = 0;
         }

         assert(ptr);
         assert(bytes);

         vbuffer[attr].buffer =
            pipe_user_buffer_create(pipe->screen, ptr, bytes,
                                    PIPE_BIND_VERTEX_BUFFER);

         vbuffer[attr].buffer_offset = 0;

         /* Track user vertex buffers. */
         pipe_resource_reference(&st->user_attrib[attr].buffer, vbuffer[attr].buffer);
         st->user_attrib[attr].element_size = element_size;
         st->user_attrib[attr].stride = stride;
         st->num_user_attribs = MAX2(st->num_user_attribs, attr + 1);
      }

      /* common-case setup */
      vbuffer[attr].stride = stride; /* in bytes */

      velements[attr].src_offset = 0;
      velements[attr].instance_divisor = array->InstanceDivisor;
      velements[attr].vertex_buffer_index = attr;
      velements[attr].src_format = st_pipe_vertex_format(array->Type,
                                                         array->Size,
                                                         array->Format,
                                                         array->Normalized);
      assert(velements[attr].src_format);
   }
}


static void
setup_index_buffer(struct gl_context *ctx,
                   const struct _mesa_index_buffer *ib,
                   struct pipe_index_buffer *ibuffer)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;

   memset(ibuffer, 0, sizeof(*ibuffer));
   if (ib) {
      struct gl_buffer_object *bufobj = ib->obj;

      switch (ib->type) {
      case GL_UNSIGNED_INT:
         ibuffer->index_size = 4;
         break;
      case GL_UNSIGNED_SHORT:
         ibuffer->index_size = 2;
         break;
      case GL_UNSIGNED_BYTE:
         ibuffer->index_size = 1;
         break;
      default:
         assert(0);
	 return;
      }

      /* get/create the index buffer object */
      if (bufobj && _mesa_is_bufferobj(bufobj)) {
         /* elements/indexes are in a real VBO */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         pipe_resource_reference(&ibuffer->buffer, stobj->buffer);
         ibuffer->offset = pointer_to_offset(ib->ptr);
      }
      else {
         /* element/indicies are in user space memory */
         ibuffer->buffer =
            pipe_user_buffer_create(pipe->screen, (void *) ib->ptr,
                                    ib->count * ibuffer->index_size,
                                    PIPE_BIND_INDEX_BUFFER);
      }
   }
}


/**
 * Prior to drawing, check that any uniforms referenced by the
 * current shader have been set.  If a uniform has not been set,
 * issue a warning.
 */
static void
check_uniforms(struct gl_context *ctx)
{
   struct gl_shader_program *shProg[3] = {
      ctx->Shader.CurrentVertexProgram,
      ctx->Shader.CurrentGeometryProgram,
      ctx->Shader.CurrentFragmentProgram,
   };
   unsigned j;

   for (j = 0; j < 3; j++) {
      unsigned i;

      if (shProg[j] == NULL || !shProg[j]->LinkStatus)
	 continue;

      for (i = 0; i < shProg[j]->Uniforms->NumUniforms; i++) {
         const struct gl_uniform *u = &shProg[j]->Uniforms->Uniforms[i];
         if (!u->Initialized) {
            _mesa_warning(ctx,
                          "Using shader with uninitialized uniform: %s",
                          u->Name);
         }
      }
   }
}


/**
 * Translate OpenGL primtive type (GL_POINTS, GL_TRIANGLE_STRIP, etc) to
 * the corresponding Gallium type.
 */
static unsigned
translate_prim(const struct gl_context *ctx, unsigned prim)
{
   /* GL prims should match Gallium prims, spot-check a few */
   assert(GL_POINTS == PIPE_PRIM_POINTS);
   assert(GL_QUADS == PIPE_PRIM_QUADS);
   assert(GL_TRIANGLE_STRIP_ADJACENCY == PIPE_PRIM_TRIANGLE_STRIP_ADJACENCY);

   /* Avoid quadstrips if it's easy to do so:
    * Note: it's important to do the correct trimming if we change the
    * prim type!  We do that wherever this function is called.
    */
   if (prim == GL_QUAD_STRIP &&
       ctx->Light.ShadeModel != GL_FLAT &&
       ctx->Polygon.FrontMode == GL_FILL &&
       ctx->Polygon.BackMode == GL_FILL)
      prim = GL_TRIANGLE_STRIP;

   return prim;
}


static void
st_validate_varrays(struct gl_context *ctx,
                    const struct gl_client_array **arrays,
                    unsigned max_index,
                    unsigned num_instances)
{
   struct st_context *st = st_context(ctx);
   const struct st_vertex_program *vp;
   const struct st_vp_variant *vpv;
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
   unsigned num_vbuffers, num_velements;
   GLuint attr;
   unsigned i;

   /* must get these after state validation! */
   vp = st->vp;
   vpv = st->vp_variant;

   memset(velements, 0, sizeof(struct pipe_vertex_element) * vpv->num_inputs);

   /* Unreference any user vertex buffers. */
   for (i = 0; i < st->num_user_attribs; i++) {
      pipe_resource_reference(&st->user_attrib[i].buffer, NULL);
   }
   st->num_user_attribs = 0;

   /*
    * Setup the vbuffer[] and velements[] arrays.
    */
   if (is_interleaved_arrays(vp, vpv, arrays)) {
      setup_interleaved_attribs(ctx, vp, vpv, arrays, vbuffer, velements,
                                max_index, num_instances);

      num_vbuffers = 1;
      num_velements = vpv->num_inputs;
      if (num_velements == 0)
         num_vbuffers = 0;
   }
   else {
      setup_non_interleaved_attribs(ctx, vp, vpv, arrays,
                                    vbuffer, velements, max_index,
                                    num_instances);
      num_vbuffers = vpv->num_inputs;
      num_velements = vpv->num_inputs;
   }

   cso_set_vertex_buffers(st->cso_context, num_vbuffers, vbuffer);
   cso_set_vertex_elements(st->cso_context, num_velements, velements);

   /* unreference buffers (frees wrapped user-space buffer objects)
    * This is OK, because the pipe driver should reference buffers by itself
    * in set_vertex_buffers. */
   for (attr = 0; attr < num_vbuffers; attr++) {
      pipe_resource_reference(&vbuffer[attr].buffer, NULL);
      assert(!vbuffer[attr].buffer);
   }
}


/**
 * This function gets plugged into the VBO module and is called when
 * we have something to render.
 * Basically, translate the information into the format expected by gallium.
 */
void
st_draw_vbo(struct gl_context *ctx,
            const struct gl_client_array **arrays,
            const struct _mesa_prim *prims,
            GLuint nr_prims,
            const struct _mesa_index_buffer *ib,
	    GLboolean index_bounds_valid,
            GLuint min_index,
            GLuint max_index)
{
   struct st_context *st = st_context(ctx);
   struct pipe_context *pipe = st->pipe;
   struct pipe_index_buffer ibuffer;
   struct pipe_draw_info info;
   unsigned i, num_instances = 1;
   GLboolean new_array =
      st->dirty.st &&
      (st->dirty.mesa & (_NEW_ARRAY | _NEW_PROGRAM | _NEW_BUFFER_OBJECT)) != 0;

   /* Mesa core state should have been validated already */
   assert(ctx->NewState == 0x0);

   if (ib) {
      /* Gallium probably doesn't want this in some cases. */
      if (!index_bounds_valid)
         if (!all_varyings_in_vbos(arrays))
            vbo_get_minmax_index(ctx, prims, ib, &min_index, &max_index);

      for (i = 0; i < nr_prims; i++) {
         num_instances = MAX2(num_instances, prims[i].num_instances);
      }
   }
   else {
      /* Get min/max index for non-indexed drawing. */
      min_index = ~0;
      max_index = 0;

      for (i = 0; i < nr_prims; i++) {
         min_index = MIN2(min_index, prims[i].start);
         max_index = MAX2(max_index, prims[i].start + prims[i].count - 1);
         num_instances = MAX2(num_instances, prims[i].num_instances);
      }
   }

   /* Validate state. */
   if (st->dirty.st) {
      GLboolean vertDataEdgeFlags;

      /* sanity check for pointer arithmetic below */
      assert(sizeof(arrays[0]->Ptr[0]) == 1);

      vertDataEdgeFlags = arrays[VERT_ATTRIB_EDGEFLAG]->BufferObj &&
                          arrays[VERT_ATTRIB_EDGEFLAG]->BufferObj->Name;
      if (vertDataEdgeFlags != st->vertdata_edgeflags) {
         st->vertdata_edgeflags = vertDataEdgeFlags;
         st->dirty.st |= ST_NEW_EDGEFLAGS_DATA;
      }

      st_validate_state(st);

      if (new_array) {
         st_validate_varrays(ctx, arrays, max_index, num_instances);
      }

#if 0
      if (MESA_VERBOSE & VERBOSE_GLSL) {
         check_uniforms(ctx);
      }
#else
      (void) check_uniforms;
#endif
   }

   /* Notify the driver that the content of user buffers may have been
    * changed. */
   assert(max_index >= min_index);
   if (!new_array && st->num_user_attribs) {
      for (i = 0; i < st->num_user_attribs; i++) {
         if (st->user_attrib[i].buffer) {
            unsigned element_size = st->user_attrib[i].element_size;
            unsigned stride = st->user_attrib[i].stride;
            unsigned min_offset = min_index * stride;
            unsigned max_offset = max_index * stride + element_size;

            assert(max_offset > min_offset);

            pipe->redefine_user_buffer(pipe, st->user_attrib[i].buffer,
                                       min_offset,
                                       max_offset - min_offset);
         }
      }
   }

   setup_index_buffer(ctx, ib, &ibuffer);
   pipe->set_index_buffer(pipe, &ibuffer);

   util_draw_init_info(&info);
   if (ib) {
      info.indexed = TRUE;
      if (min_index != ~0 && max_index != ~0) {
         info.min_index = min_index;
         info.max_index = max_index;
      }
   }

   info.primitive_restart = ctx->Array.PrimitiveRestart;
   info.restart_index = ctx->Array.RestartIndex;

   /* do actual drawing */
   for (i = 0; i < nr_prims; i++) {
      info.mode = translate_prim( ctx, prims[i].mode );
      info.start = prims[i].start;
      info.count = prims[i].count;
      info.instance_count = prims[i].num_instances;
      info.index_bias = prims[i].basevertex;
      if (!ib) {
         info.min_index = info.start;
         info.max_index = info.start + info.count - 1;
      }

      if (u_trim_pipe_prim(info.mode, &info.count))
         pipe->draw_vbo(pipe, &info);
   }

   pipe_resource_reference(&ibuffer.buffer, NULL);
}


void
st_init_draw(struct st_context *st)
{
   struct gl_context *ctx = st->ctx;

   vbo_set_draw_func(ctx, st_draw_vbo);

#if FEATURE_feedback || FEATURE_rastpos
   st->draw = draw_create(st->pipe); /* for selection/feedback */

   /* Disable draw options that might convert points/lines to tris, etc.
    * as that would foul-up feedback/selection mode.
    */
   draw_wide_line_threshold(st->draw, 1000.0f);
   draw_wide_point_threshold(st->draw, 1000.0f);
   draw_enable_line_stipple(st->draw, FALSE);
   draw_enable_point_sprites(st->draw, FALSE);
#endif
}


void
st_destroy_draw(struct st_context *st)
{
#if FEATURE_feedback || FEATURE_rastpos
   draw_destroy(st->draw);
#endif
}
