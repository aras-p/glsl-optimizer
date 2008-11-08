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
#include "main/macros.h"
#include "shader/prog_uniform.h"

#include "vbo/vbo.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_cb_bufferobjects.h"
#include "st_draw.h"
#include "st_program.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"

#include "draw/draw_private.h"
#include "draw/draw_context.h"


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
static GLuint
pipe_vertex_format(GLenum type, GLuint size, GLboolean normalized)
{
   assert((type >= GL_BYTE && type <= GL_DOUBLE) ||
          type == GL_FIXED);
   assert(size >= 1);
   assert(size <= 4);

   if (normalized) {
      switch (type) {
      case GL_DOUBLE: return double_types[size-1];
      case GL_FLOAT: return float_types[size-1];
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
   return 0; /* silence compiler warning */
}


/*
 * If edge flags are needed, setup an bitvector of flags and call
 * pipe->set_edgeflags().
 * XXX memleak: need to free the returned pointer at some point
 */
static void *
setup_edgeflags(GLcontext *ctx, GLenum primMode, GLint start, GLint count,
                const struct gl_client_array *array)
{
   struct pipe_context *pipe = ctx->st->pipe;

   if ((primMode == GL_TRIANGLES ||
        primMode == GL_QUADS ||
        primMode == GL_POLYGON) &&
       (ctx->Polygon.FrontMode != GL_FILL ||
        ctx->Polygon.BackMode != GL_FILL)) {
      /* need edge flags */
      GLint i;
      unsigned *vec;
      struct st_buffer_object *stobj = st_buffer_object(array->BufferObj);
      ubyte *map;

      if (!stobj)
         return NULL;

      vec = (unsigned *) calloc(sizeof(unsigned), (count + 31) / 32);
      if (!vec)
         return NULL;

      map = pipe_buffer_map(pipe->screen, stobj->buffer, PIPE_BUFFER_USAGE_CPU_READ);
      map = ADD_POINTERS(map, array->Ptr);

      for (i = 0; i < count; i++) {
         if (*((float *) map))
            vec[i/32] |= 1 << (i % 32);

         map += array->StrideB;
      }

      pipe_buffer_unmap(pipe->screen, stobj->buffer);

      pipe->set_edgeflags(pipe, vec);

      return vec;
   }
   else {
      /* edge flags not needed */
      pipe->set_edgeflags(pipe, NULL);
      return NULL;
   }
}


/**
 * Examine the active arrays to determine if we have interleaved
 * vertex arrays all living in one VBO, or all living in user space.
 * \param userSpace  returns whether the arrays are in user space.
 */
static GLboolean
is_interleaved_arrays(const struct st_vertex_program *vp,
                      const struct gl_client_array **arrays,
                      GLboolean *userSpace)
{
   GLuint attr;
   const struct gl_buffer_object *firstBufObj = NULL;
   GLint firstStride = -1;
   GLuint num_client_arrays = 0;
   const GLubyte *client_addr = NULL;

   for (attr = 0; attr < vp->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const struct gl_buffer_object *bufObj = arrays[mesaAttr]->BufferObj;
      const GLsizei stride = arrays[mesaAttr]->StrideB; /* in bytes */

      if (firstStride < 0) {
         firstStride = stride;
      }
      else if (firstStride != stride) {
         return GL_FALSE;
      }
         
      if (!bufObj || !bufObj->Name) {
         num_client_arrays++;
         /* Try to detect if the client-space arrays are
          * "close" to each other.
          */
         if (!client_addr) {
            client_addr = arrays[mesaAttr]->Ptr;
         }
         else if (abs(arrays[mesaAttr]->Ptr - client_addr) > firstStride) {
            /* arrays start too far apart */
            return GL_FALSE;
         }
      }
      else if (!firstBufObj) {
         firstBufObj = bufObj;
      }
      else if (bufObj != firstBufObj) {
         return GL_FALSE;
      }
   }

   *userSpace = (num_client_arrays == vp->num_inputs);
   /*printf("user space: %d\n", (int) *userSpace);*/

   return GL_TRUE;
}


/**
 * Once we know all the arrays are in user space, this function
 * computes the memory range occupied by the arrays.
 */
static void
get_user_arrays_bounds(const struct st_vertex_program *vp,
                       const struct gl_client_array **arrays,
                       GLuint max_index,
                       const GLubyte **low, const GLubyte **high)
{
   const GLubyte *low_addr = NULL;
   GLuint attr;
   GLint stride;

   for (attr = 0; attr < vp->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      const GLubyte *start = arrays[mesaAttr]->Ptr;
      stride = arrays[mesaAttr]->StrideB;
      if (attr == 0) {
         low_addr = start;
      }
      else {
         low_addr = MIN2(low_addr, start);
      }
   }

   *low = low_addr;
   *high = low_addr + (max_index + 1) * stride;
}


/**
 * Set up for drawing interleaved arrays that all live in one VBO
 * or all live in user space.
 * \param vbuffer  returns vertex buffer info
 * \param velements  returns vertex element info
 */
static void
setup_interleaved_attribs(GLcontext *ctx,
                          const struct st_vertex_program *vp,
                          const struct gl_client_array **arrays,
                          GLuint max_index,
                          GLboolean userSpace,
                          struct pipe_vertex_buffer *vbuffer,
                          struct pipe_vertex_element velements[])
{
   struct pipe_context *pipe = ctx->st->pipe;
   GLuint attr;
   const GLubyte *offset0;

   for (attr = 0; attr < vp->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      struct gl_buffer_object *bufobj = arrays[mesaAttr]->BufferObj;
      struct st_buffer_object *stobj = st_buffer_object(bufobj);
      GLsizei stride = arrays[mesaAttr]->StrideB;

      /*printf("stobj %u = %p\n", attr, (void*)stobj);*/

      if (attr == 0) {
         if (userSpace) {
            const GLubyte *low, *high;
            get_user_arrays_bounds(vp, arrays, max_index, &low, &high);
            /*printf("user buffer range: %p %p  %d\n", low, high, high-low);*/
            vbuffer->buffer =
               pipe_user_buffer_create(pipe->screen, (void *) low, high - low);
            vbuffer->buffer_offset = 0;
            offset0 = low;
         }
         else {
            vbuffer->buffer = NULL;
            pipe_buffer_reference(pipe->screen, &vbuffer->buffer, stobj->buffer);
            vbuffer->buffer_offset = (unsigned) arrays[mesaAttr]->Ptr;
            offset0 = arrays[mesaAttr]->Ptr;
         }
         vbuffer->pitch = stride; /* in bytes */
         vbuffer->max_index = max_index;
      }

      velements[attr].src_offset =
         (unsigned) (arrays[mesaAttr]->Ptr - offset0);
      velements[attr].vertex_buffer_index = 0;
      velements[attr].nr_components = arrays[mesaAttr]->Size;
      velements[attr].src_format =
         pipe_vertex_format(arrays[mesaAttr]->Type,
                            arrays[mesaAttr]->Size,
                            arrays[mesaAttr]->Normalized);
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
setup_non_interleaved_attribs(GLcontext *ctx,
                              const struct st_vertex_program *vp,
                              const struct gl_client_array **arrays,
                              GLuint max_index,
                              struct pipe_vertex_buffer vbuffer[],
                              struct pipe_vertex_element velements[])
{
   struct pipe_context *pipe = ctx->st->pipe;
   GLuint attr;

   for (attr = 0; attr < vp->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      struct gl_buffer_object *bufobj = arrays[mesaAttr]->BufferObj;
      GLsizei stride = arrays[mesaAttr]->StrideB;

      if (bufobj && bufobj->Name) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         assert(stobj->buffer);
         /*printf("stobj %u = %p\n", attr, (void*) stobj);*/

         vbuffer[attr].buffer = NULL;
         pipe_buffer_reference(pipe->screen, &vbuffer[attr].buffer, stobj->buffer);
         vbuffer[attr].buffer_offset = (unsigned) arrays[mesaAttr]->Ptr;
         velements[attr].src_offset = 0;
      }
      else {
         /* attribute data is in user-space memory, not a VBO */
         uint bytes;
         /*printf("user-space array %d stride %d\n", attr, stride);*/
	
         /* wrap user data */
         if (arrays[mesaAttr]->Ptr) {
            /* user's vertex array */
            if (arrays[mesaAttr]->StrideB) {
               bytes = arrays[mesaAttr]->StrideB * (max_index + 1);
            }
            else {
               bytes = arrays[mesaAttr]->Size
                  * _mesa_sizeof_type(arrays[mesaAttr]->Type);
            }
            vbuffer[attr].buffer = pipe_user_buffer_create(pipe->screen,
                           (void *) arrays[mesaAttr]->Ptr, bytes);
         }
         else {
            /* no array, use ctx->Current.Attrib[] value */
            bytes = sizeof(ctx->Current.Attrib[0]);
            vbuffer[attr].buffer = pipe_user_buffer_create(pipe->screen,
                           (void *) ctx->Current.Attrib[mesaAttr], bytes);
            stride = 0;
         }

         vbuffer[attr].buffer_offset = 0;
         velements[attr].src_offset = 0;
      }

      assert(velements[attr].src_offset <= 2048); /* 11-bit field */

      /* common-case setup */
      vbuffer[attr].pitch = stride; /* in bytes */
      vbuffer[attr].max_index = max_index;
      velements[attr].vertex_buffer_index = attr;
      velements[attr].nr_components = arrays[mesaAttr]->Size;
      velements[attr].src_format
         = pipe_vertex_format(arrays[mesaAttr]->Type,
                              arrays[mesaAttr]->Size,
                              arrays[mesaAttr]->Normalized);
      assert(velements[attr].src_format);
   }
}



/**
 * Prior to drawing, check that any uniforms referenced by the
 * current shader have been set.  If a uniform has not been set,
 * issue a warning.
 */
static void
check_uniforms(GLcontext *ctx)
{
   const struct gl_shader_program *shProg = ctx->Shader.CurrentProgram;
   if (shProg && shProg->LinkStatus) {
      GLuint i;
      for (i = 0; i < shProg->Uniforms->NumUniforms; i++) {
         const struct gl_uniform *u = &shProg->Uniforms->Uniforms[i];
         if (!u->Initialized) {
            _mesa_warning(ctx,
                          "Using shader with uninitialized uniform: %s",
                          u->Name);
         }
      }
   }
}


/**
 * This function gets plugged into the VBO module and is called when
 * we have something to render.
 * Basically, translate the information into the format expected by gallium.
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
   const struct st_vertex_program *vp;
   const struct pipe_shader_state *vs;
   struct pipe_vertex_buffer vbuffer[PIPE_MAX_SHADER_INPUTS];
   GLuint attr;
   struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
   unsigned num_vbuffers, num_velements;
   GLboolean userSpace;

   /* sanity check for pointer arithmetic below */
   assert(sizeof(arrays[0]->Ptr[0]) == 1);

   st_validate_state(ctx->st);

   /* must get these after state validation! */
   vp = ctx->st->vp;
   vs = &ctx->st->vp->state;

   if (MESA_VERBOSE & VERBOSE_GLSL) {
      check_uniforms(ctx);
   }

   /*
    * Setup the vbuffer[] and velements[] arrays.
    */
   if (is_interleaved_arrays(vp, arrays, &userSpace)) {
      /*printf("Draw interleaved\n");*/
      setup_interleaved_attribs(ctx, vp, arrays, max_index, userSpace,
                                vbuffer, velements);
      num_vbuffers = 1;
      num_velements = vp->num_inputs;
      if (num_velements == 0)
         num_vbuffers = 0;
   }
   else {
      /*printf("Draw non-interleaved\n");*/
      setup_non_interleaved_attribs(ctx, vp, arrays, max_index,
                                    vbuffer, velements);
      num_vbuffers = vp->num_inputs;
      num_velements = vp->num_inputs;
   }

#if 0
   {
      GLuint i;
      for (i = 0; i < num_vbuffers; i++) {
         printf("buffers[%d].pitch = %u\n", i, vbuffer[i].pitch);
         printf("buffers[%d].max_index = %u\n", i, vbuffer[i].max_index);
         printf("buffers[%d].buffer_offset = %u\n", i, vbuffer[i].buffer_offset);
         printf("buffers[%d].buffer = %p\n", i, (void*) vbuffer[i].buffer);
      }
      for (i = 0; i < num_velements; i++) {
         printf("vlements[%d].vbuffer_index = %u\n", i, velements[i].vertex_buffer_index);
         printf("vlements[%d].src_offset = %u\n", i, velements[i].src_offset);
         printf("vlements[%d].nr_comps = %u\n", i, velements[i].nr_components);
         printf("vlements[%d].format = %s\n", i, pf_name(velements[i].src_format));
      }
   }
#endif

   pipe->set_vertex_buffers(pipe, num_vbuffers, vbuffer);
   pipe->set_vertex_elements(pipe, num_velements, velements);

   if (num_vbuffers == 0 || num_velements == 0)
      return;

   /* do actual drawing */
   if (ib) {
      /* indexed primitive */
      struct gl_buffer_object *bufobj = ib->obj;
      struct pipe_buffer *indexBuf = NULL;
      unsigned indexSize, indexOffset, i;

      switch (ib->type) {
      case GL_UNSIGNED_INT:
         indexSize = 4;
         break;
      case GL_UNSIGNED_SHORT:
         indexSize = 2;
         break;
      case GL_UNSIGNED_BYTE:
         indexSize = 1;
         break;
      default:
         assert(0);
	 return;
      }

      /* get/create the index buffer object */
      if (bufobj && bufobj->Name) {
         /* elements/indexes are in a real VBO */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         pipe_buffer_reference(pipe->screen, &indexBuf, stobj->buffer);
         indexOffset = (unsigned) ib->ptr / indexSize;
      }
      else {
         /* element/indicies are in user space memory */
         indexBuf = pipe_user_buffer_create(pipe->screen, (void *) ib->ptr,
                                            ib->count * indexSize);
         indexOffset = 0;
      }

      /* draw */
      if (nr_prims == 1 && pipe->draw_range_elements != NULL) {
         i = 0;

         /* XXX: exercise temporary path to pass min/max directly
          * through to driver & draw module.  These interfaces still
          * need a bit of work...
          */
         setup_edgeflags(ctx, prims[i].mode,
                         prims[i].start + indexOffset, prims[i].count,
                         arrays[VERT_ATTRIB_EDGEFLAG]);

         pipe->draw_range_elements(pipe, indexBuf, indexSize,
                                   min_index,
                                   max_index,
                                   prims[i].mode,
                                   prims[i].start + indexOffset, prims[i].count);
      }
      else {
         for (i = 0; i < nr_prims; i++) {
            setup_edgeflags(ctx, prims[i].mode,
                            prims[i].start + indexOffset, prims[i].count,
                            arrays[VERT_ATTRIB_EDGEFLAG]);
            
            pipe->draw_elements(pipe, indexBuf, indexSize,
                                prims[i].mode,
                                prims[i].start + indexOffset, prims[i].count);
         }
      }

      pipe_buffer_reference(pipe->screen, &indexBuf, NULL);
   }
   else {
      /* non-indexed */
      GLuint i;
      for (i = 0; i < nr_prims; i++) {
         setup_edgeflags(ctx, prims[i].mode,
                         prims[i].start, prims[i].count,
                         arrays[VERT_ATTRIB_EDGEFLAG]);

         pipe->draw_arrays(pipe, prims[i].mode, prims[i].start, prims[i].count);
      }
   }

   /* unreference buffers (frees wrapped user-space buffer objects) */
   for (attr = 0; attr < num_vbuffers; attr++) {
      pipe_buffer_reference(pipe->screen, &vbuffer[attr].buffer, NULL);
      assert(!vbuffer[attr].buffer);
   }
   pipe->set_vertex_buffers(pipe, num_vbuffers, vbuffer);
}


#if FEATURE_feedback || FEATURE_drawpix

/**
 * Set the (private) draw module's post-transformed vertex format when in
 * GL_SELECT or GL_FEEDBACK mode or for glRasterPos.
 */
static void
set_feedback_vertex_format(GLcontext *ctx)
{
#if 0
   struct st_context *st = ctx->st;
   struct vertex_info vinfo;
   GLuint i;

   memset(&vinfo, 0, sizeof(vinfo));

   if (ctx->RenderMode == GL_SELECT) {
      assert(ctx->RenderMode == GL_SELECT);
      vinfo.num_attribs = 1;
      vinfo.format[0] = FORMAT_4F;
      vinfo.interp_mode[0] = INTERP_LINEAR;
   }
   else {
      /* GL_FEEDBACK, or glRasterPos */
      /* emit all attribs (pos, color, texcoord) as GLfloat[4] */
      vinfo.num_attribs = st->state.vs->cso->state.num_outputs;
      for (i = 0; i < vinfo.num_attribs; i++) {
         vinfo.format[i] = FORMAT_4F;
         vinfo.interp_mode[i] = INTERP_LINEAR;
      }
   }

   draw_set_vertex_info(st->draw, &vinfo);
#endif
}


/**
 * Called by VBO to draw arrays when in selection or feedback mode and
 * to implement glRasterPos.
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
   const struct st_vertex_program *vp;
   const struct pipe_shader_state *vs;
   struct pipe_buffer *index_buffer_handle = 0;
   struct pipe_vertex_buffer vbuffers[PIPE_MAX_SHADER_INPUTS];
   struct pipe_vertex_element velements[PIPE_MAX_ATTRIBS];
   GLuint attr, i;
   ubyte *mapped_constants;

   assert(draw);

   st_validate_state(ctx->st);

   /* must get these after state validation! */
   vp = ctx->st->vp;
   vs = &st->vp->state;

   if (!st->vp->draw_shader) {
      st->vp->draw_shader = draw_create_vertex_shader(draw, vs);
   }

   /*
    * Set up the draw module's state.
    *
    * We'd like to do this less frequently, but the normal state-update
    * code sends state updates to the pipe, not to our private draw module.
    */
   assert(draw);
   draw_set_viewport_state(draw, &st->state.viewport);
   draw_set_clip_state(draw, &st->state.clip);
   draw_set_rasterizer_state(draw, &st->state.rasterizer);
   draw_bind_vertex_shader(draw, st->vp->draw_shader);
   set_feedback_vertex_format(ctx);

   /* loop over TGSI shader inputs to determine vertex buffer
    * and attribute info
    */
   for (attr = 0; attr < vp->num_inputs; attr++) {
      const GLuint mesaAttr = vp->index_to_input[attr];
      struct gl_buffer_object *bufobj = arrays[mesaAttr]->BufferObj;
      void *map;

      if (bufobj && bufobj->Name) {
         /* Attribute data is in a VBO.
          * Recall that for VBOs, the gl_client_array->Ptr field is
          * really an offset from the start of the VBO, not a pointer.
          */
         struct st_buffer_object *stobj = st_buffer_object(bufobj);
         assert(stobj->buffer);

         vbuffers[attr].buffer = NULL;
         pipe_buffer_reference(pipe->screen, &vbuffers[attr].buffer, stobj->buffer);
         vbuffers[attr].buffer_offset = (unsigned) arrays[0]->Ptr;/* in bytes */
         velements[attr].src_offset = arrays[mesaAttr]->Ptr - arrays[0]->Ptr;
      }
      else {
         /* attribute data is in user-space memory, not a VBO */
         uint bytes = (arrays[mesaAttr]->Size
                       * _mesa_sizeof_type(arrays[mesaAttr]->Type)
                       * (max_index + 1));

         /* wrap user data */
         vbuffers[attr].buffer
            = pipe_user_buffer_create(pipe->screen, (void *) arrays[mesaAttr]->Ptr,
                                      bytes);
         vbuffers[attr].buffer_offset = 0;
         velements[attr].src_offset = 0;
      }

      /* common-case setup */
      vbuffers[attr].pitch = arrays[mesaAttr]->StrideB; /* in bytes */
      vbuffers[attr].max_index = max_index;
      velements[attr].vertex_buffer_index = attr;
      velements[attr].nr_components = arrays[mesaAttr]->Size;
      velements[attr].src_format = pipe_vertex_format(arrays[mesaAttr]->Type,
                                               arrays[mesaAttr]->Size,
                                               arrays[mesaAttr]->Normalized);
      assert(velements[attr].src_format);

      /* tell draw about this attribute */
#if 0
      draw_set_vertex_buffer(draw, attr, &vbuffer[attr]);
#endif

      /* map the attrib buffer */
      map = pipe_buffer_map(pipe->screen, vbuffers[attr].buffer,
                            PIPE_BUFFER_USAGE_CPU_READ);
      draw_set_mapped_vertex_buffer(draw, attr, map);
   }

   draw_set_vertex_buffers(draw, vp->num_inputs, vbuffers);
   draw_set_vertex_elements(draw, vp->num_inputs, velements);

   if (ib) {
      unsigned indexSize;
      struct gl_buffer_object *bufobj = ib->obj;
      struct st_buffer_object *stobj = st_buffer_object(bufobj);
      void *map;

      index_buffer_handle = stobj->buffer;

      switch (ib->type) {
      case GL_UNSIGNED_INT:
         indexSize = 4;
         break;
      case GL_UNSIGNED_SHORT:
         indexSize = 2;
         break;
      default:
         assert(0);
	 return;
      }

      map = pipe_buffer_map(pipe->screen, index_buffer_handle,
                            PIPE_BUFFER_USAGE_CPU_READ);
      draw_set_mapped_element_buffer(draw, indexSize, map);
   }
   else {
      /* no index/element buffer */
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }


   /* map constant buffers */
   mapped_constants = pipe_buffer_map(pipe->screen,
                                      st->state.constants[PIPE_SHADER_VERTEX].buffer,
                                      PIPE_BUFFER_USAGE_CPU_READ);
   draw_set_mapped_constant_buffer(st->draw, mapped_constants,
                                   st->state.constants[PIPE_SHADER_VERTEX].buffer->size);


   /* draw here */
   for (i = 0; i < nr_prims; i++) {
      draw_arrays(draw, prims[i].mode, prims[i].start, prims[i].count);
   }


   /* unmap constant buffers */
   pipe_buffer_unmap(pipe->screen, st->state.constants[PIPE_SHADER_VERTEX].buffer);

   /*
    * unmap vertex/index buffers
    */
   for (i = 0; i < PIPE_MAX_ATTRIBS; i++) {
      if (draw->pt.vertex_buffer[i].buffer) {
         pipe_buffer_unmap(pipe->screen, draw->pt.vertex_buffer[i].buffer);
         pipe_buffer_reference(pipe->screen, &draw->pt.vertex_buffer[i].buffer, NULL);
         draw_set_mapped_vertex_buffer(draw, i, NULL);
      }
   }
   if (ib) {
      pipe_buffer_unmap(pipe->screen, index_buffer_handle);
      draw_set_mapped_element_buffer(draw, 0, NULL);
   }
}

#endif /* FEATURE_feedback || FEATURE_drawpix */


void st_init_draw( struct st_context *st )
{
   GLcontext *ctx = st->ctx;

   vbo_set_draw_func(ctx, st_draw_vbo);
}


void st_destroy_draw( struct st_context *st )
{
}


