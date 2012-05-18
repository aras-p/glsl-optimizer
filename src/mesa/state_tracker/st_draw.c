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

#include "vbo/vbo.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bufferobjects.h"
#include "st_cb_xformfb.h"
#include "st_draw.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "util/u_format.h"
#include "util/u_prim.h"
#include "util/u_draw_quad.h"
#include "util/u_upload_mgr.h"
#include "draw/draw_context.h"
#include "cso_cache/cso_context.h"

#include "../glsl/ir_uniform.h"


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

static GLuint uint_types_int[4] = {
   PIPE_FORMAT_R32_UINT,
   PIPE_FORMAT_R32G32_UINT,
   PIPE_FORMAT_R32G32B32_UINT,
   PIPE_FORMAT_R32G32B32A32_UINT
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

static GLuint int_types_int[4] = {
   PIPE_FORMAT_R32_SINT,
   PIPE_FORMAT_R32G32_SINT,
   PIPE_FORMAT_R32G32B32_SINT,
   PIPE_FORMAT_R32G32B32A32_SINT
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

static GLuint ushort_types_int[4] = {
   PIPE_FORMAT_R16_UINT,
   PIPE_FORMAT_R16G16_UINT,
   PIPE_FORMAT_R16G16B16_UINT,
   PIPE_FORMAT_R16G16B16A16_UINT
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

static GLuint short_types_int[4] = {
   PIPE_FORMAT_R16_SINT,
   PIPE_FORMAT_R16G16_SINT,
   PIPE_FORMAT_R16G16B16_SINT,
   PIPE_FORMAT_R16G16B16A16_SINT
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

static GLuint ubyte_types_int[4] = {
   PIPE_FORMAT_R8_UINT,
   PIPE_FORMAT_R8G8_UINT,
   PIPE_FORMAT_R8G8B8_UINT,
   PIPE_FORMAT_R8G8B8A8_UINT
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

static GLuint byte_types_int[4] = {
   PIPE_FORMAT_R8_SINT,
   PIPE_FORMAT_R8G8_SINT,
   PIPE_FORMAT_R8G8B8_SINT,
   PIPE_FORMAT_R8G8B8A8_SINT
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
                      GLboolean normalized, GLboolean integer)
{
   assert((type >= GL_BYTE && type <= GL_DOUBLE) ||
          type == GL_FIXED || type == GL_HALF_FLOAT ||
          type == GL_INT_2_10_10_10_REV ||
          type == GL_UNSIGNED_INT_2_10_10_10_REV);
   assert(size >= 1);
   assert(size <= 4);
   assert(format == GL_RGBA || format == GL_BGRA);

   if (type == GL_INT_2_10_10_10_REV ||
       type == GL_UNSIGNED_INT_2_10_10_10_REV) {
      assert(size == 4);
      assert(!integer);

      if (format == GL_BGRA) {
         if (type == GL_INT_2_10_10_10_REV) {
            if (normalized)
               return PIPE_FORMAT_B10G10R10A2_SNORM;
            else
               return PIPE_FORMAT_B10G10R10A2_SSCALED;
         } else {
            if (normalized)
               return PIPE_FORMAT_B10G10R10A2_UNORM;
            else
               return PIPE_FORMAT_B10G10R10A2_USCALED;
         }
      } else {
         if (type == GL_INT_2_10_10_10_REV) {
            if (normalized)
               return PIPE_FORMAT_R10G10B10A2_SNORM;
            else
               return PIPE_FORMAT_R10G10B10A2_SSCALED;
         } else {
            if (normalized)
               return PIPE_FORMAT_R10G10B10A2_UNORM;
            else
               return PIPE_FORMAT_R10G10B10A2_USCALED;
         }
      }
   }

   if (format == GL_BGRA) {
      /* this is an odd-ball case */
      assert(type == GL_UNSIGNED_BYTE);
      assert(normalized);
      return PIPE_FORMAT_B8G8R8A8_UNORM;
   }

   if (integer) {
      switch (type) {
      case GL_INT: return int_types_int[size-1];
      case GL_SHORT: return short_types_int[size-1];
      case GL_BYTE: return byte_types_int[size-1];
      case GL_UNSIGNED_INT: return uint_types_int[size-1];
      case GL_UNSIGNED_SHORT: return ushort_types_int[size-1];
      case GL_UNSIGNED_BYTE: return ubyte_types_int[size-1];
      default: assert(0); return 0;
      }
   }
   else if (normalized) {
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
 * This is very similar to vbo_all_varyings_in_vbos() but we are
 * only interested in per-vertex data.  See bug 38626.
 */
static GLboolean
all_varyings_in_vbos(const struct gl_client_array *arrays[])
{
   GLuint i;
   
   for (i = 0; i < VERT_ATTRIB_MAX; i++)
      if (arrays[i]->StrideB &&
          !arrays[i]->InstanceDivisor &&
          !_mesa_is_bufferobj(arrays[i]->BufferObj))
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

         if ((!_mesa_is_bufferobj(bufObj)) != userSpaceBuffer)
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
 * \return GL_TRUE for success, GL_FALSE otherwise (probably out of memory)
 */
static GLboolean
setup_interleaved_attribs(struct gl_context *ctx,
                          const struct st_vertex_program *vp,
                          const struct st_vp_variant *vpv,
                          const struct gl_client_array **arrays,
                          struct pipe_vertex_buffer *vbuffer,
                          struct pipe_vertex_element velements[])
{
   GLuint attr;
   const GLubyte *low_addr = NULL;
   GLboolean usingVBO;      /* all arrays in a VBO? */
   struct gl_buffer_object *bufobj;
   GLsizei stride;

   /* Find the lowest address of the arrays we're drawing,
    * Init bufobj and stride.
    */
   if (vpv->num_inputs) {
      const GLuint mesaAttr0 = vp->index_to_input[0];
      const struct gl_client_array *array = arrays[mesaAttr0];

      /* Since we're doing interleaved arrays, we know there'll be at most
       * one buffer object and the stride will be the same for all arrays.
       * Grab them now.
       */
      bufobj = array->BufferObj;
      stride = array->StrideB;

      low_addr = arrays[vp->index_to_input[0]]->Ptr;

      for (attr = 1; attr < vpv->num_inputs; attr++) {
         const GLubyte *start = arrays[vp->index_to_input[attr]]->Ptr;
         low_addr = MIN2(low_addr, start);
      }
   }
   else {
      /* not sure we'll ever have zero inputs, but play it safe */
      bufobj = NULL;
      stride = 0;
      low_addr = 0;
   }

   /* are the arrays in user space? */
   usingVBO = _mesa_is_bufferobj(bufobj);

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      unsigned src_offset = (unsigned) (array->Ptr - low_addr);
      GLuint element_size = array->_ElementSize;

      assert(element_size == array->Size * _mesa_sizeof_type(array->Type));

      velements[attr].src_offset = src_offset;
      velements[attr].instance_divisor = array->InstanceDivisor;
      velements[attr].vertex_buffer_index = 0;
      velements[attr].src_format = st_pipe_vertex_format(array->Type,
                                                         array->Size,
                                                         array->Format,
                                                         array->Normalized,
                                                         array->Integer);
      assert(velements[attr].src_format);
   }

   /*
    * Return the vbuffer info and setup user-space attrib info, if needed.
    */
   if (vpv->num_inputs == 0) {
      /* just defensive coding here */
      vbuffer->buffer = NULL;
      vbuffer->user_buffer = NULL;
      vbuffer->buffer_offset = 0;
      vbuffer->stride = 0;
   }
   else if (usingVBO) {
      /* all interleaved arrays in a VBO */
      struct st_buffer_object *stobj = st_buffer_object(bufobj);

      if (!stobj || !stobj->buffer) {
         /* probably out of memory (or zero-sized buffer) */
         return GL_FALSE;
      }

      vbuffer->buffer = stobj->buffer;
      vbuffer->user_buffer = NULL;
      vbuffer->buffer_offset = pointer_to_offset(low_addr);
      vbuffer->stride = stride;
   }
   else {
      /* all interleaved arrays in user memory */
      vbuffer->buffer = NULL;
      vbuffer->user_buffer = low_addr;
      vbuffer->buffer_offset = 0;
      vbuffer->stride = stride;
   }

   return GL_TRUE;
}


/**
 * Set up a separate pipe_vertex_buffer and pipe_vertex_element for each
 * vertex attribute.
 * \param vbuffer  returns vertex buffer info
 * \param velements  returns vertex element info
 * \return GL_TRUE for success, GL_FALSE otherwise (probably out of memory)
 */
static GLboolean
setup_non_interleaved_attribs(struct gl_context *ctx,
                              const struct st_vertex_program *vp,
                              const struct st_vp_variant *vpv,
                              const struct gl_client_array **arrays,
                              struct pipe_vertex_buffer vbuffer[],
                              struct pipe_vertex_element velements[])
{
   GLuint attr;

   for (attr = 0; attr < vpv->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_client_array *array = arrays[mesaAttr];
      struct gl_buffer_object *bufobj = array->BufferObj;
      GLsizei stride = array->StrideB;

      assert(array->_ElementSize == array->Size * _mesa_sizeof_type(array->Type));

      if (_mesa_is_bufferobj(bufobj)) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);

         if (!stobj || !stobj->buffer) {
            /* probably out of memory (or zero-sized buffer) */
            return GL_FALSE;
         }

         vbuffer[attr].buffer = stobj->buffer;
         vbuffer[attr].user_buffer = NULL;
         vbuffer[attr].buffer_offset = pointer_to_offset(array->Ptr);
      }
      else {
         /* wrap user data */
         void *ptr;

         if (array->Ptr) {
            ptr = (void *) array->Ptr;
         }
         else {
            /* no array, use ctx->Current.Attrib[] value */
            ptr = (void *) ctx->Current.Attrib[mesaAttr];
            stride = 0;
         }

         assert(ptr);

         vbuffer[attr].buffer = NULL;
         vbuffer[attr].user_buffer = ptr;
         vbuffer[attr].buffer_offset = 0;
      }

      /* common-case setup */
      vbuffer[attr].stride = stride; /* in bytes */

      velements[attr].src_offset = 0;
      velements[attr].instance_divisor = array->InstanceDivisor;
      velements[attr].vertex_buffer_index = attr;
      velements[attr].src_format = st_pipe_vertex_format(array->Type,
                                                         array->Size,
                                                         array->Format,
                                                         array->Normalized,
                                                         array->Integer);
      assert(velements[attr].src_format);
   }

   return GL_TRUE;
}


static void
setup_index_buffer(struct st_context *st,
                   const struct _mesa_index_buffer *ib,
                   struct pipe_index_buffer *ibuffer)
{
   struct gl_buffer_object *bufobj = ib->obj;

   ibuffer->index_size = vbo_sizeof_ib_type(ib->type);

   /* get/create the index buffer object */
   if (_mesa_is_bufferobj(bufobj)) {
      /* indices are in a real VBO */
      ibuffer->buffer = st_buffer_object(bufobj)->buffer;
      ibuffer->offset = pointer_to_offset(ib->ptr);
   }
   else if (st->indexbuf_uploader) {
      u_upload_data(st->indexbuf_uploader, 0, ib->count * ibuffer->index_size,
                    ib->ptr, &ibuffer->offset, &ibuffer->buffer);
      u_upload_unmap(st->indexbuf_uploader);
   }
   else {
      /* indices are in user space memory */
      ibuffer->user_buffer = ib->ptr;
   }

   cso_set_index_buffer(st->cso_context, ibuffer);
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

      for (i = 0; i < shProg[j]->NumUserUniformStorage; i++) {
         const struct gl_uniform_storage *u = &shProg[j]->UniformStorage[i];
         if (!u->initialized) {
            _mesa_warning(ctx,
                          "Using shader with uninitialized uniform: %s",
                          u->name);
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


/**
 * Setup vertex arrays and buffers prior to drawing.
 * \return GL_TRUE for success, GL_FALSE otherwise (probably out of memory)
 */
static GLboolean
st_validate_varrays(struct gl_context *ctx,
                    const struct gl_client_array **arrays)
{
   struct st_context *st = st_context(ctx);
   const struct st_vertex_program *vp;
   const struct st_vp_variant *vpv;
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
   unsigned num_vbuffers, num_velements;

   /* must get these after state validation! */
   vp = st->vp;
   vpv = st->vp_variant;

   memset(velements, 0, sizeof(struct pipe_vertex_element) * vpv->num_inputs);

   /*
    * Setup the vbuffer[] and velements[] arrays.
    */
   if (is_interleaved_arrays(vp, vpv, arrays)) {
      if (!setup_interleaved_attribs(ctx, vp, vpv, arrays, vbuffer,
                                     velements)) {
         return GL_FALSE;
      }

      num_vbuffers = 1;
      num_velements = vpv->num_inputs;
      if (num_velements == 0)
         num_vbuffers = 0;
   }
   else {
      if (!setup_non_interleaved_attribs(ctx, vp, vpv, arrays,
                                         vbuffer, velements)) {
         return GL_FALSE;
      }

      num_vbuffers = vpv->num_inputs;
      num_velements = vpv->num_inputs;
   }

   cso_set_vertex_buffers(st->cso_context, num_vbuffers, vbuffer);
   cso_set_vertex_elements(st->cso_context, num_velements, velements);

   return GL_TRUE;
}


/**
 * This function gets plugged into the VBO module and is called when
 * we have something to render.
 * Basically, translate the information into the format expected by gallium.
 */
void
st_draw_vbo(struct gl_context *ctx,
            const struct _mesa_prim *prims,
            GLuint nr_prims,
            const struct _mesa_index_buffer *ib,
	    GLboolean index_bounds_valid,
            GLuint min_index,
            GLuint max_index,
            struct gl_transform_feedback_object *tfb_vertcount)
{
   struct st_context *st = st_context(ctx);
   struct pipe_index_buffer ibuffer = {0};
   struct pipe_draw_info info;
   const struct gl_client_array **arrays = ctx->Array._DrawArrays;
   unsigned i;
   GLboolean new_array;

   /* Mesa core state should have been validated already */
   assert(ctx->NewState == 0x0);

   /* Get Mesa driver state. */
   st->dirty.st |= ctx->NewDriverState;
   ctx->NewDriverState = 0;

   new_array =
      (st->dirty.st & (ST_NEW_VERTEX_ARRAYS | ST_NEW_VERTEX_PROGRAM)) ||
      (st->dirty.mesa & (_NEW_PROGRAM | _NEW_BUFFER_OBJECT)) != 0;

   /* Validate state. */
   if (st->dirty.st) {
      GLboolean vertDataEdgeFlags;

      vertDataEdgeFlags = arrays[VERT_ATTRIB_EDGEFLAG]->BufferObj &&
                          arrays[VERT_ATTRIB_EDGEFLAG]->BufferObj->Name;
      if (vertDataEdgeFlags != st->vertdata_edgeflags) {
         st->vertdata_edgeflags = vertDataEdgeFlags;
         st->dirty.st |= ST_NEW_EDGEFLAGS_DATA;
      }

      st_validate_state(st);

      if (new_array) {
         if (!st_validate_varrays(ctx, arrays)) {
            /* probably out of memory, no-op the draw call */
            return;
         }
      }

#if 0
      if (MESA_VERBOSE & VERBOSE_GLSL) {
         check_uniforms(ctx);
      }
#else
      (void) check_uniforms;
#endif
   }

   util_draw_init_info(&info);
   if (ib) {
      /* Get index bounds for user buffers. */
      if (!index_bounds_valid)
         if (!all_varyings_in_vbos(arrays))
            vbo_get_minmax_indices(ctx, prims, ib, &min_index, &max_index,
                                   nr_prims);

      setup_index_buffer(st, ib, &ibuffer);

      info.indexed = TRUE;
      if (min_index != ~0 && max_index != ~0) {
         info.min_index = min_index;
         info.max_index = max_index;
      }

      /* The VBO module handles restart for the non-indexed GLDrawArrays
       * so we only set these fields for indexed drawing:
       */
      info.primitive_restart = ctx->Array.PrimitiveRestart;
      info.restart_index = ctx->Array.RestartIndex;
   }
   else {
      /* Transform feedback drawing is always non-indexed. */
      /* Set info.count_from_stream_output. */
      if (tfb_vertcount) {
         st_transform_feedback_draw_init(tfb_vertcount, &info);
      }
   }

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

      if (info.count_from_stream_output) {
         cso_draw_vbo(st->cso_context, &info);
      }
      else if (info.primitive_restart) {
         /* don't trim, restarts might be inside index list */
         cso_draw_vbo(st->cso_context, &info);
      }
      else if (u_trim_pipe_prim(info.mode, &info.count))
         cso_draw_vbo(st->cso_context, &info);
   }

   if (ib && st->indexbuf_uploader && !_mesa_is_bufferobj(ib->obj)) {
      pipe_resource_reference(&ibuffer.buffer, NULL);
   }
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
