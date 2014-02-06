/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
 * Copyright 2009 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/glheader.h"
#include "main/context.h"
#include "main/state.h"
#include "main/api_validate.h"
#include "main/dispatch.h"
#include "main/varray.h"
#include "main/bufferobj.h"
#include "main/enums.h"
#include "main/macros.h"
#include "main/transformfeedback.h"

#include "vbo_context.h"


/**
 * All vertex buffers should be in an unmapped state when we're about
 * to draw.  This debug function checks that.
 */
static void
check_buffers_are_unmapped(const struct gl_client_array **inputs)
{
#ifdef DEBUG
   GLuint i;

   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (inputs[i]) {
         struct gl_buffer_object *obj = inputs[i]->BufferObj;
         assert(!_mesa_check_disallowed_mapping(obj));
         (void) obj;
      }
   }
#endif
}


/**
 * A debug function that may be called from other parts of Mesa as
 * needed during debugging.
 */
void
vbo_check_buffers_are_unmapped(struct gl_context *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   /* check the current vertex arrays */
   check_buffers_are_unmapped(exec->array.inputs);
   /* check the current glBegin/glVertex/glEnd-style VBO */
   assert(!_mesa_check_disallowed_mapping(exec->vtx.bufferobj));
}



/**
 * Compute min and max elements by scanning the index buffer for
 * glDraw[Range]Elements() calls.
 * If primitive restart is enabled, we need to ignore restart
 * indexes when computing min/max.
 */
static void
vbo_get_minmax_index(struct gl_context *ctx,
		     const struct _mesa_prim *prim,
		     const struct _mesa_index_buffer *ib,
		     GLuint *min_index, GLuint *max_index,
		     const GLuint count)
{
   const GLboolean restart = ctx->Array._PrimitiveRestart;
   const GLuint restartIndex = _mesa_primitive_restart_index(ctx, ib->type);
   const int index_size = vbo_sizeof_ib_type(ib->type);
   const char *indices;
   GLuint i;

   indices = (char *) ib->ptr + prim->start * index_size;
   if (_mesa_is_bufferobj(ib->obj)) {
      GLsizeiptr size = MIN2(count * index_size, ib->obj->Size);
      indices = ctx->Driver.MapBufferRange(ctx, (GLintptr) indices, size,
                                           GL_MAP_READ_BIT, ib->obj,
                                           MAP_INTERNAL);
   }

   switch (ib->type) {
   case GL_UNSIGNED_INT: {
      const GLuint *ui_indices = (const GLuint *)indices;
      GLuint max_ui = 0;
      GLuint min_ui = ~0U;
      if (restart) {
         for (i = 0; i < count; i++) {
            if (ui_indices[i] != restartIndex) {
               if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
               if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < count; i++) {
            if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
            if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
         }
      }
      *min_index = min_ui;
      *max_index = max_ui;
      break;
   }
   case GL_UNSIGNED_SHORT: {
      const GLushort *us_indices = (const GLushort *)indices;
      GLuint max_us = 0;
      GLuint min_us = ~0U;
      if (restart) {
         for (i = 0; i < count; i++) {
            if (us_indices[i] != restartIndex) {
               if (us_indices[i] > max_us) max_us = us_indices[i];
               if (us_indices[i] < min_us) min_us = us_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < count; i++) {
            if (us_indices[i] > max_us) max_us = us_indices[i];
            if (us_indices[i] < min_us) min_us = us_indices[i];
         }
      }
      *min_index = min_us;
      *max_index = max_us;
      break;
   }
   case GL_UNSIGNED_BYTE: {
      const GLubyte *ub_indices = (const GLubyte *)indices;
      GLuint max_ub = 0;
      GLuint min_ub = ~0U;
      if (restart) {
         for (i = 0; i < count; i++) {
            if (ub_indices[i] != restartIndex) {
               if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
               if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
            }
         }
      }
      else {
         for (i = 0; i < count; i++) {
            if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
            if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
         }
      }
      *min_index = min_ub;
      *max_index = max_ub;
      break;
   }
   default:
      assert(0);
      break;
   }

   if (_mesa_is_bufferobj(ib->obj)) {
      ctx->Driver.UnmapBuffer(ctx, ib->obj, MAP_INTERNAL);
   }
}

/**
 * Compute min and max elements for nr_prims
 */
void
vbo_get_minmax_indices(struct gl_context *ctx,
                       const struct _mesa_prim *prims,
                       const struct _mesa_index_buffer *ib,
                       GLuint *min_index,
                       GLuint *max_index,
                       GLuint nr_prims)
{
   GLuint tmp_min, tmp_max;
   GLuint i;
   GLuint count;

   *min_index = ~0;
   *max_index = 0;

   for (i = 0; i < nr_prims; i++) {
      const struct _mesa_prim *start_prim;

      start_prim = &prims[i];
      count = start_prim->count;
      /* Do combination if possible to reduce map/unmap count */
      while ((i + 1 < nr_prims) &&
             (prims[i].start + prims[i].count == prims[i+1].start)) {
         count += prims[i+1].count;
         i++;
      }
      vbo_get_minmax_index(ctx, start_prim, ib, &tmp_min, &tmp_max, count);
      *min_index = MIN2(*min_index, tmp_min);
      *max_index = MAX2(*max_index, tmp_max);
   }
}


/**
 * Check that element 'j' of the array has reasonable data.
 * Map VBO if needed.
 * For debugging purposes; not normally used.
 */
static void
check_array_data(struct gl_context *ctx, struct gl_client_array *array,
                 GLuint attrib, GLuint j)
{
   if (array->Enabled) {
      const void *data = array->Ptr;
      if (_mesa_is_bufferobj(array->BufferObj)) {
         if (!array->BufferObj->Mappings[MAP_INTERNAL].Pointer) {
            /* need to map now */
            array->BufferObj->Mappings[MAP_INTERNAL].Pointer =
               ctx->Driver.MapBufferRange(ctx, 0, array->BufferObj->Size,
					  GL_MAP_READ_BIT, array->BufferObj,
                                          MAP_INTERNAL);
         }
         data = ADD_POINTERS(data,
                             array->BufferObj->Mappings[MAP_INTERNAL].Pointer);
      }
      switch (array->Type) {
      case GL_FLOAT:
         {
            GLfloat *f = (GLfloat *) ((GLubyte *) data + array->StrideB * j);
            GLint k;
            for (k = 0; k < array->Size; k++) {
               if (IS_INF_OR_NAN(f[k]) ||
                   f[k] >= 1.0e20 || f[k] <= -1.0e10) {
                  printf("Bad array data:\n");
                  printf("  Element[%u].%u = %f\n", j, k, f[k]);
                  printf("  Array %u at %p\n", attrib, (void* ) array);
                  printf("  Type 0x%x, Size %d, Stride %d\n",
			 array->Type, array->Size, array->Stride);
                  printf("  Address/offset %p in Buffer Object %u\n",
			 array->Ptr, array->BufferObj->Name);
                  f[k] = 1.0; /* XXX replace the bad value! */
               }
               /*assert(!IS_INF_OR_NAN(f[k]));*/
            }
         }
         break;
      default:
         ;
      }
   }
}


/**
 * Unmap the buffer object referenced by given array, if mapped.
 */
static void
unmap_array_buffer(struct gl_context *ctx, struct gl_client_array *array)
{
   if (array->Enabled &&
       _mesa_is_bufferobj(array->BufferObj) &&
       _mesa_bufferobj_mapped(array->BufferObj, MAP_INTERNAL)) {
      ctx->Driver.UnmapBuffer(ctx, array->BufferObj, MAP_INTERNAL);
   }
}


/**
 * Examine the array's data for NaNs, etc.
 * For debug purposes; not normally used.
 */
static void
check_draw_elements_data(struct gl_context *ctx, GLsizei count, GLenum elemType,
                         const void *elements, GLint basevertex)
{
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   const void *elemMap;
   GLint i, k;

   if (_mesa_is_bufferobj(ctx->Array.VAO->IndexBufferObj)) {
      elemMap = ctx->Driver.MapBufferRange(ctx, 0,
					   ctx->Array.VAO->IndexBufferObj->Size,
					   GL_MAP_READ_BIT,
					   ctx->Array.VAO->IndexBufferObj,
                                           MAP_INTERNAL);
      elements = ADD_POINTERS(elements, elemMap);
   }

   for (i = 0; i < count; i++) {
      GLuint j;

      /* j = element[i] */
      switch (elemType) {
      case GL_UNSIGNED_BYTE:
         j = ((const GLubyte *) elements)[i];
         break;
      case GL_UNSIGNED_SHORT:
         j = ((const GLushort *) elements)[i];
         break;
      case GL_UNSIGNED_INT:
         j = ((const GLuint *) elements)[i];
         break;
      default:
         assert(0);
      }

      /* check element j of each enabled array */
      for (k = 0; k < Elements(vao->_VertexAttrib); k++) {
         check_array_data(ctx, &vao->_VertexAttrib[k], k, j);
      }
   }

   if (_mesa_is_bufferobj(vao->IndexBufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, ctx->Array.VAO->IndexBufferObj,
                              MAP_INTERNAL);
   }

   for (k = 0; k < Elements(vao->_VertexAttrib); k++) {
      unmap_array_buffer(ctx, &vao->_VertexAttrib[k]);
   }
}


/**
 * Check array data, looking for NaNs, etc.
 */
static void
check_draw_arrays_data(struct gl_context *ctx, GLint start, GLsizei count)
{
   /* TO DO */
}


/**
 * Print info/data for glDrawArrays(), for debugging.
 */
static void
print_draw_arrays(struct gl_context *ctx,
                  GLenum mode, GLint start, GLsizei count)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_vertex_array_object *vao = ctx->Array.VAO;
   int i;

   printf("vbo_exec_DrawArrays(mode 0x%x, start %d, count %d):\n",
	  mode, start, count);

   for (i = 0; i < 32; i++) {
      struct gl_buffer_object *bufObj = exec->array.inputs[i]->BufferObj;
      GLuint bufName = bufObj->Name;
      GLint stride = exec->array.inputs[i]->Stride;
      printf("attr %2d: size %d stride %d  enabled %d  "
	     "ptr %p  Bufobj %u\n",
	     i,
	     exec->array.inputs[i]->Size,
	     stride,
	     /*exec->array.inputs[i]->Enabled,*/
	     vao->_VertexAttrib[VERT_ATTRIB_FF(i)].Enabled,
	     exec->array.inputs[i]->Ptr,
	     bufName);

      if (bufName) {
         GLubyte *p = ctx->Driver.MapBufferRange(ctx, 0, bufObj->Size,
						 GL_MAP_READ_BIT, bufObj,
                                                 MAP_INTERNAL);
         int offset = (int) (GLintptr) exec->array.inputs[i]->Ptr;
         float *f = (float *) (p + offset);
         int *k = (int *) f;
         int i;
         int n = (count * stride) / 4;
         if (n > 32)
            n = 32;
         printf("  Data at offset %d:\n", offset);
         for (i = 0; i < n; i++) {
            printf("    float[%d] = 0x%08x %f\n", i, k[i], f[i]);
         }
         ctx->Driver.UnmapBuffer(ctx, bufObj, MAP_INTERNAL);
      }
   }
}


/**
 * Set the vbo->exec->inputs[] pointers to point to the enabled
 * vertex arrays.  This depends on the current vertex program/shader
 * being executed because of whether or not generic vertex arrays
 * alias the conventional vertex arrays.
 * For arrays that aren't enabled, we set the input[attrib] pointer
 * to point at a zero-stride current value "array".
 */
static void
recalculate_input_bindings(struct gl_context *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_client_array *vertexAttrib = ctx->Array.VAO->_VertexAttrib;
   const struct gl_client_array **inputs = &exec->array.inputs[0];
   GLbitfield64 const_inputs = 0x0;
   GLuint i;

   switch (get_program_mode(ctx)) {
   case VP_NONE:
      /* When no vertex program is active (or the vertex program is generated
       * from fixed-function state).  We put the material values into the
       * generic slots.  This is the only situation where material values
       * are available as per-vertex attributes.
       */
      for (i = 0; i < VERT_ATTRIB_FF_MAX; i++) {
	 if (vertexAttrib[VERT_ATTRIB_FF(i)].Enabled)
	    inputs[i] = &vertexAttrib[VERT_ATTRIB_FF(i)];
	 else {
	    inputs[i] = &vbo->currval[VBO_ATTRIB_POS+i];
            const_inputs |= VERT_BIT(i);
         }
      }

      for (i = 0; i < MAT_ATTRIB_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC(i)] =
	    &vbo->currval[VBO_ATTRIB_MAT_FRONT_AMBIENT+i];
         const_inputs |= VERT_BIT_GENERIC(i);
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = MAT_ATTRIB_MAX; i < VERT_ATTRIB_GENERIC_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC(i)] = &vbo->currval[VBO_ATTRIB_GENERIC0+i];
         const_inputs |= VERT_BIT_GENERIC(i);
      }
      break;

   case VP_ARB:
      /* There are no shaders in OpenGL ES 1.x, so this code path should be
       * impossible to reach.  The meta code is careful to not use shaders in
       * ES1.
       */
      assert(ctx->API != API_OPENGLES);

      /* In the compatibility profile of desktop OpenGL, the generic[0]
       * attribute array aliases and overrides the legacy position array.  
       * Otherwise, legacy attributes available in the legacy slots,
       * generic attributes in the generic slots and materials are not
       * available as per-vertex attributes.
       *
       * In all other APIs, only the generic attributes exist, and none of the
       * slots are considered "magic."
       */
      if (ctx->API == API_OPENGL_COMPAT) {
         if (vertexAttrib[VERT_ATTRIB_GENERIC0].Enabled)
            inputs[0] = &vertexAttrib[VERT_ATTRIB_GENERIC0];
         else if (vertexAttrib[VERT_ATTRIB_POS].Enabled)
            inputs[0] = &vertexAttrib[VERT_ATTRIB_POS];
         else {
            inputs[0] = &vbo->currval[VBO_ATTRIB_POS];
            const_inputs |= VERT_BIT_POS;
         }

         for (i = 1; i < VERT_ATTRIB_FF_MAX; i++) {
            if (vertexAttrib[VERT_ATTRIB_FF(i)].Enabled)
               inputs[i] = &vertexAttrib[VERT_ATTRIB_FF(i)];
            else {
               inputs[i] = &vbo->currval[VBO_ATTRIB_POS+i];
               const_inputs |= VERT_BIT_FF(i);
            }
         }

         for (i = 1; i < VERT_ATTRIB_GENERIC_MAX; i++) {
            if (vertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled)
               inputs[VERT_ATTRIB_GENERIC(i)] =
                  &vertexAttrib[VERT_ATTRIB_GENERIC(i)];
            else {
               inputs[VERT_ATTRIB_GENERIC(i)] =
                  &vbo->currval[VBO_ATTRIB_GENERIC0+i];
               const_inputs |= VERT_BIT_GENERIC(i);
            }
         }

         inputs[VERT_ATTRIB_GENERIC0] = inputs[0];
      } else {
         /* Other parts of the code assume that inputs[0] through
          * inputs[VERT_ATTRIB_FF_MAX] will be non-NULL.  However, in OpenGL
          * ES 2.0+ or OpenGL core profile, none of these arrays should ever
          * be enabled.
          */
         for (i = 0; i < VERT_ATTRIB_FF_MAX; i++) {
            assert(!vertexAttrib[VERT_ATTRIB_FF(i)].Enabled);

            inputs[i] = &vbo->currval[VBO_ATTRIB_POS+i];
            const_inputs |= VERT_BIT_FF(i);
         }

         for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; i++) {
            if (vertexAttrib[VERT_ATTRIB_GENERIC(i)].Enabled)
               inputs[VERT_ATTRIB_GENERIC(i)] =
                  &vertexAttrib[VERT_ATTRIB_GENERIC(i)];
            else {
               inputs[VERT_ATTRIB_GENERIC(i)] =
                  &vbo->currval[VBO_ATTRIB_GENERIC0+i];
               const_inputs |= VERT_BIT_GENERIC(i);
            }
         }
      }

      break;
   }

   _mesa_set_varying_vp_inputs( ctx, VERT_BIT_ALL & (~const_inputs) );
   ctx->NewDriverState |= ctx->DriverFlags.NewArray;
}


/**
 * Examine the enabled vertex arrays to set the exec->array.inputs[] values.
 * These will point to the arrays to actually use for drawing.  Some will
 * be user-provided arrays, other will be zero-stride const-valued arrays.
 * Note that this might set the _NEW_VARYING_VP_INPUTS dirty flag so state
 * validation must be done after this call.
 */
void
vbo_bind_arrays(struct gl_context *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;

   vbo_draw_method(vbo, DRAW_ARRAYS);

   if (exec->array.recalculate_inputs) {
      recalculate_input_bindings(ctx);
      exec->array.recalculate_inputs = GL_FALSE;

      /* Again... because we may have changed the bitmask of per-vertex varying
       * attributes.  If we regenerate the fixed-function vertex program now
       * we may be able to prune down the number of vertex attributes which we
       * need in the shader.
       */
      if (ctx->NewState) {
         /* Setting "validating" to TRUE prevents _mesa_update_state from
          * invalidating what we just did.
          */
         exec->validating = GL_TRUE;
         _mesa_update_state(ctx);
         exec->validating = GL_FALSE;
      }
   }
}


/**
 * Handle a draw case that potentially has primitive restart enabled.
 *
 * If primitive restart is enabled, and PrimitiveRestartInSoftware is
 * set, then vbo_sw_primitive_restart is used to handle the primitive
 * restart case in software.
 */
static void
vbo_handle_primitive_restart(struct gl_context *ctx,
                             const struct _mesa_prim *prim,
                             GLuint nr_prims,
                             const struct _mesa_index_buffer *ib,
                             GLboolean index_bounds_valid,
                             GLuint min_index,
                             GLuint max_index)
{
   struct vbo_context *vbo = vbo_context(ctx);

   if ((ib != NULL) &&
       ctx->Const.PrimitiveRestartInSoftware &&
       ctx->Array._PrimitiveRestart) {
      /* Handle primitive restart in software */
      vbo_sw_primitive_restart(ctx, prim, nr_prims, ib, NULL);
   } else {
      /* Call driver directly for draw_prims */
      vbo->draw_prims(ctx, prim, nr_prims, ib,
                      index_bounds_valid, min_index, max_index, NULL, NULL);
   }
}


/**
 * Helper function called by the other DrawArrays() functions below.
 * This is where we handle primitive restart for drawing non-indexed
 * arrays.  If primitive restart is enabled, it typically means
 * splitting one DrawArrays() into two.
 */
static void
vbo_draw_arrays(struct gl_context *ctx, GLenum mode, GLint start,
                GLsizei count, GLuint numInstances, GLuint baseInstance)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[2];

   vbo_bind_arrays(ctx);

   /* init most fields to zero */
   memset(prim, 0, sizeof(prim));
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].mode = mode;
   prim[0].num_instances = numInstances;
   prim[0].base_instance = baseInstance;
   prim[0].is_indirect = 0;

   /* Implement the primitive restart index */
   if (ctx->Array.PrimitiveRestart && ctx->Array.RestartIndex < count) {
      GLuint primCount = 0;

      if (ctx->Array.RestartIndex == start) {
         /* special case: RestartIndex at beginning */
         if (count > 1) {
            prim[0].start = start + 1;
            prim[0].count = count - 1;
            primCount = 1;
         }
      }
      else if (ctx->Array.RestartIndex == start + count - 1) {
         /* special case: RestartIndex at end */
         if (count > 1) {
            prim[0].start = start;
            prim[0].count = count - 1;
            primCount = 1;
         }
      }
      else {
         /* general case: RestartIndex in middle, split into two prims */
         prim[0].start = start;
         prim[0].count = ctx->Array.RestartIndex - start;

         prim[1] = prim[0];
         prim[1].start = ctx->Array.RestartIndex + 1;
         prim[1].count = count - prim[1].start;

         primCount = 2;
      }

      if (primCount > 0) {
         /* draw one or two prims */
         check_buffers_are_unmapped(exec->array.inputs);
         vbo->draw_prims(ctx, prim, primCount, NULL,
                         GL_TRUE, start, start + count - 1, NULL, NULL);
      }
   }
   else {
      /* no prim restart */
      prim[0].start = start;
      prim[0].count = count;

      check_buffers_are_unmapped(exec->array.inputs);
      vbo->draw_prims(ctx, prim, 1, NULL,
                      GL_TRUE, start, start + count - 1,
                      NULL, NULL);
   }

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
      _mesa_flush(ctx);
   }
}


/**
 * Execute a glRectf() function.
 */
static void GLAPIENTRY
vbo_exec_Rectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   CALL_Begin(GET_DISPATCH(), (GL_QUADS));
   CALL_Vertex2f(GET_DISPATCH(), (x1, y1));
   CALL_Vertex2f(GET_DISPATCH(), (x2, y1));
   CALL_Vertex2f(GET_DISPATCH(), (x2, y2));
   CALL_Vertex2f(GET_DISPATCH(), (x1, y2));
   CALL_End(GET_DISPATCH(), ());
}


static void GLAPIENTRY
vbo_exec_EvalMesh1(GLenum mode, GLint i1, GLint i2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;
   GLfloat u, du;
   GLenum prim;

   switch (mode) {
   case GL_POINT:
      prim = GL_POINTS;
      break;
   case GL_LINE:
      prim = GL_LINE_STRIP;
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glEvalMesh1(mode)" );
      return;
   }

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map1Vertex4 && 
       !ctx->Eval.Map1Vertex3)
      return;

   du = ctx->Eval.MapGrid1du;
   u = ctx->Eval.MapGrid1u1 + i1 * du;

   CALL_Begin(GET_DISPATCH(), (prim));
   for (i=i1;i<=i2;i++,u+=du) {
      CALL_EvalCoord1f(GET_DISPATCH(), (u));
   }
   CALL_End(GET_DISPATCH(), ());
}


static void GLAPIENTRY
vbo_exec_EvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
   GET_CURRENT_CONTEXT(ctx);
   GLfloat u, du, v, dv, v1, u1;
   GLint i, j;

   switch (mode) {
   case GL_POINT:
   case GL_LINE:
   case GL_FILL:
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glEvalMesh2(mode)" );
      return;
   }

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map2Vertex4 && 
       !ctx->Eval.Map2Vertex3)
      return;

   du = ctx->Eval.MapGrid2du;
   dv = ctx->Eval.MapGrid2dv;
   v1 = ctx->Eval.MapGrid2v1 + j1 * dv;
   u1 = ctx->Eval.MapGrid2u1 + i1 * du;

   switch (mode) {
   case GL_POINT:
      CALL_Begin(GET_DISPATCH(), (GL_POINTS));
      for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	 }
      }
      CALL_End(GET_DISPATCH(), ());
      break;
   case GL_LINE:
      for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	 CALL_Begin(GET_DISPATCH(), (GL_LINE_STRIP));
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	 }
	 CALL_End(GET_DISPATCH(), ());
      }
      for (u=u1,i=i1;i<=i2;i++,u+=du) {
	 CALL_Begin(GET_DISPATCH(), (GL_LINE_STRIP));
	 for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	 }
	 CALL_End(GET_DISPATCH(), ());
      }
      break;
   case GL_FILL:
      for (v=v1,j=j1;j<j2;j++,v+=dv) {
	 CALL_Begin(GET_DISPATCH(), (GL_TRIANGLE_STRIP));
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v));
	    CALL_EvalCoord2f(GET_DISPATCH(), (u, v+dv));
	 }
	 CALL_End(GET_DISPATCH(), ());
      }
      break;
   }
}


/**
 * Called from glDrawArrays when in immediate mode (not display list mode).
 */
static void GLAPIENTRY
vbo_exec_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArrays(%s, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), start, count);

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   if (0)
      check_draw_arrays_data(ctx, start, count);

   vbo_draw_arrays(ctx, mode, start, count, 1, 0);

   if (0)
      print_draw_arrays(ctx, mode, start, count);
}


/**
 * Called from glDrawArraysInstanced when in immediate mode (not
 * display list mode).
 */
static void GLAPIENTRY
vbo_exec_DrawArraysInstanced(GLenum mode, GLint start, GLsizei count,
                             GLsizei numInstances)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArraysInstanced(%s, %d, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), start, count, numInstances);

   if (!_mesa_validate_DrawArraysInstanced(ctx, mode, start, count, numInstances))
      return;

   if (0)
      check_draw_arrays_data(ctx, start, count);

   vbo_draw_arrays(ctx, mode, start, count, numInstances, 0);

   if (0)
      print_draw_arrays(ctx, mode, start, count);
}


/**
 * Called from glDrawArraysInstancedBaseInstance when in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawArraysInstancedBaseInstance(GLenum mode, GLint first, GLsizei count,
                                         GLsizei numInstances, GLuint baseInstance)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArraysInstancedBaseInstance(%s, %d, %d, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), first, count,
                  numInstances, baseInstance);

   if (!_mesa_validate_DrawArraysInstanced(ctx, mode, first, count,
                                           numInstances))
      return;

   if (0)
      check_draw_arrays_data(ctx, first, count);

   vbo_draw_arrays(ctx, mode, first, count, numInstances, baseInstance);

   if (0)
      print_draw_arrays(ctx, mode, first, count);
}



/**
 * Map GL_ELEMENT_ARRAY_BUFFER and print contents.
 * For debugging.
 */
#if 0
static void
dump_element_buffer(struct gl_context *ctx, GLenum type)
{
   const GLvoid *map =
      ctx->Driver.MapBufferRange(ctx, 0,
				 ctx->Array.VAO->IndexBufferObj->Size,
				 GL_MAP_READ_BIT,
                                 ctx->Array.VAO->IndexBufferObj,
                                 MAP_INTERNAL);
   switch (type) {
   case GL_UNSIGNED_BYTE:
      {
         const GLubyte *us = (const GLubyte *) map;
         GLint i;
         for (i = 0; i < ctx->Array.VAO->IndexBufferObj->Size; i++) {
            printf("%02x ", us[i]);
            if (i % 32 == 31)
               printf("\n");
         }
         printf("\n");
      }
      break;
   case GL_UNSIGNED_SHORT:
      {
         const GLushort *us = (const GLushort *) map;
         GLint i;
         for (i = 0; i < ctx->Array.VAO->IndexBufferObj->Size / 2; i++) {
            printf("%04x ", us[i]);
            if (i % 16 == 15)
               printf("\n");
         }
         printf("\n");
      }
      break;
   case GL_UNSIGNED_INT:
      {
         const GLuint *us = (const GLuint *) map;
         GLint i;
         for (i = 0; i < ctx->Array.VAO->IndexBufferObj->Size / 4; i++) {
            printf("%08x ", us[i]);
            if (i % 8 == 7)
               printf("\n");
         }
         printf("\n");
      }
      break;
   default:
      ;
   }

   ctx->Driver.UnmapBuffer(ctx, ctx->Array.VAO->IndexBufferObj,
                           MAP_INTERNAL);
}
#endif


/**
 * Inner support for both _mesa_DrawElements and _mesa_DrawRangeElements.
 * Do the rendering for a glDrawElements or glDrawRangeElements call after
 * we've validated buffer bounds, etc.
 */
static void
vbo_validated_drawrangeelements(struct gl_context *ctx, GLenum mode,
				GLboolean index_bounds_valid,
				GLuint start, GLuint end,
				GLsizei count, GLenum type,
				const GLvoid *indices,
				GLint basevertex, GLuint numInstances,
				GLuint baseInstance)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim prim[1];

   vbo_bind_arrays(ctx);

   ib.count = count;
   ib.type = type;
   ib.obj = ctx->Array.VAO->IndexBufferObj;
   ib.ptr = indices;

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = 0;
   prim[0].count = count;
   prim[0].indexed = 1;
   prim[0].is_indirect = 0;
   prim[0].basevertex = basevertex;
   prim[0].num_instances = numInstances;
   prim[0].base_instance = baseInstance;

   /* Need to give special consideration to rendering a range of
    * indices starting somewhere above zero.  Typically the
    * application is issuing multiple DrawRangeElements() to draw
    * successive primitives layed out linearly in the vertex arrays.
    * Unless the vertex arrays are all in a VBO (or locked as with
    * CVA), the OpenGL semantics imply that we need to re-read or
    * re-upload the vertex data on each draw call.  
    *
    * In the case of hardware tnl, we want to avoid starting the
    * upload at zero, as it will mean every draw call uploads an
    * increasing amount of not-used vertex data.  Worse - in the
    * software tnl module, all those vertices might be transformed and
    * lit but never rendered.
    *
    * If we just upload or transform the vertices in start..end,
    * however, the indices will be incorrect.
    *
    * At this level, we don't know exactly what the requirements of
    * the backend are going to be, though it will likely boil down to
    * either:
    *
    * 1) Do nothing, everything is in a VBO and is processed once
    *       only.
    *
    * 2) Adjust the indices and vertex arrays so that start becomes
    *    zero.
    *
    * Rather than doing anything here, I'll provide a helper function
    * for the latter case elsewhere.
    */

   check_buffers_are_unmapped(exec->array.inputs);
   vbo_handle_primitive_restart(ctx, prim, 1, &ib,
                                index_bounds_valid, start, end);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
      _mesa_flush(ctx);
   }
}


/**
 * Called by glDrawRangeElementsBaseVertex() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawRangeElementsBaseVertex(GLenum mode,
				     GLuint start, GLuint end,
				     GLsizei count, GLenum type,
				     const GLvoid *indices,
				     GLint basevertex)
{
   static GLuint warnCount = 0;
   GLboolean index_bounds_valid = GL_TRUE;
   GLuint max_element;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx,
                "glDrawRangeElementsBaseVertex(%s, %u, %u, %d, %s, %p, %d)\n",
                _mesa_lookup_enum_by_nr(mode), start, end, count,
                _mesa_lookup_enum_by_nr(type), indices, basevertex);

   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count,
                                          type, indices, basevertex ))
      return;

   if (ctx->Const.CheckArrayBounds) {
      /* _MaxElement was computed, so we can use it.
       * This path is used for drivers which need strict bounds checking.
       */
      max_element = ctx->Array.VAO->_MaxElement;
   }
   else {
      /* Generally, hardware drivers don't need to know the buffer bounds
       * if all vertex attributes are in VBOs.
       * However, if none of vertex attributes are in VBOs, _MaxElement
       * is always set to some random big number anyway, so bounds checking
       * is mostly useless.
       *
       * This is only useful to catch invalid values in the "end" parameter
       * like ~0.
       */
      max_element = 2 * 1000 * 1000 * 1000; /* just a big number */
   }

   if ((int) end + basevertex < 0 ||
       start + basevertex >= max_element) {
      /* The application requested we draw using a range of indices that's
       * outside the bounds of the current VBO.  This is invalid and appears
       * to give undefined results.  The safest thing to do is to simply
       * ignore the range, in case the application botched their range tracking
       * but did provide valid indices.  Also issue a warning indicating that
       * the application is broken.
       */
      if (warnCount++ < 10) {
         _mesa_warning(ctx, "glDrawRangeElements(start %u, end %u, "
                       "basevertex %d, count %d, type 0x%x, indices=%p):\n"
                       "\trange is outside VBO bounds (max=%u); ignoring.\n"
                       "\tThis should be fixed in the application.",
                       start, end, basevertex, count, type, indices,
                       max_element - 1);
      }
      index_bounds_valid = GL_FALSE;
   }

   /* NOTE: It's important that 'end' is a reasonable value.
    * in _tnl_draw_prims(), we use end to determine how many vertices
    * to transform.  If it's too large, we can unnecessarily split prims
    * or we can read/write out of memory in several different places!
    */

   /* Catch/fix some potential user errors */
   if (type == GL_UNSIGNED_BYTE) {
      start = MIN2(start, 0xff);
      end = MIN2(end, 0xff);
   }
   else if (type == GL_UNSIGNED_SHORT) {
      start = MIN2(start, 0xffff);
      end = MIN2(end, 0xffff);
   }

   if (0) {
      printf("glDraw[Range]Elements{,BaseVertex}"
	     "(start %u, end %u, type 0x%x, count %d) ElemBuf %u, "
	     "base %d\n",
	     start, end, type, count,
	     ctx->Array.VAO->IndexBufferObj->Name,
	     basevertex);
   }

   if ((int) start + basevertex < 0 ||
       end + basevertex >= max_element)
      index_bounds_valid = GL_FALSE;

#if 0
   check_draw_elements_data(ctx, count, type, indices);
#else
   (void) check_draw_elements_data;
#endif

   vbo_validated_drawrangeelements(ctx, mode, index_bounds_valid, start, end,
				   count, type, indices, basevertex, 1, 0);
}


/**
 * Called by glDrawRangeElements() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
                           GLsizei count, GLenum type, const GLvoid *indices)
{
   if (MESA_VERBOSE & VERBOSE_DRAW) {
      GET_CURRENT_CONTEXT(ctx);
      _mesa_debug(ctx,
                  "glDrawRangeElements(%s, %u, %u, %d, %s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode), start, end, count,
                  _mesa_lookup_enum_by_nr(type), indices);
   }

   vbo_exec_DrawRangeElementsBaseVertex(mode, start, end, count, type,
					indices, 0);
}


/**
 * Called by glDrawElements() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElements(GLenum mode, GLsizei count, GLenum type,
                      const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElements(%s, %u, %s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices);

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices, 0 ))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, 0, 1, 0);
}


/**
 * Called by glDrawElementsBaseVertex() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
				const GLvoid *indices, GLint basevertex)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsBaseVertex(%s, %d, %s, %p, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices, basevertex);

   if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices,
				     basevertex ))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, basevertex, 1, 0);
}


/**
 * Called by glDrawElementsInstanced() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsInstanced(GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices, GLsizei numInstances)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsInstanced(%s, %d, %s, %p, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices, numInstances);

   if (!_mesa_validate_DrawElementsInstanced(ctx, mode, count, type, indices,
                                             numInstances, 0))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, 0, numInstances, 0);
}


/**
 * Called by glDrawElementsInstancedBaseVertex() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsInstancedBaseVertex(GLenum mode, GLsizei count, GLenum type,
                               const GLvoid *indices, GLsizei numInstances,
                               GLint basevertex)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsInstancedBaseVertex(%s, %d, %s, %p, %d; %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices,
                  numInstances, basevertex);

   if (!_mesa_validate_DrawElementsInstanced(ctx, mode, count, type, indices,
                                             numInstances, basevertex))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
				   count, type, indices, basevertex, numInstances, 0);
}


/**
 * Called by glDrawElementsInstancedBaseInstance() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsInstancedBaseInstance(GLenum mode, GLsizei count, GLenum type,
                                           const GLvoid *indices, GLsizei numInstances,
                                           GLuint baseInstance)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsInstancedBaseInstance(%s, %d, %s, %p, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices,
                  numInstances, baseInstance);

   if (!_mesa_validate_DrawElementsInstanced(ctx, mode, count, type, indices,
                                             numInstances, 0))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
                                   count, type, indices, 0, numInstances,
                                   baseInstance);
}


/**
 * Called by glDrawElementsInstancedBaseVertexBaseInstance() in immediate mode.
 */
static void GLAPIENTRY
vbo_exec_DrawElementsInstancedBaseVertexBaseInstance(GLenum mode, GLsizei count, GLenum type,
                                                     const GLvoid *indices, GLsizei numInstances,
                                                     GLint basevertex, GLuint baseInstance)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsInstancedBaseVertexBaseInstance(%s, %d, %s, %p, %d, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), count,
                  _mesa_lookup_enum_by_nr(type), indices,
                  numInstances, basevertex, baseInstance);

   if (!_mesa_validate_DrawElementsInstanced(ctx, mode, count, type, indices,
                                             numInstances, basevertex))
      return;

   vbo_validated_drawrangeelements(ctx, mode, GL_FALSE, ~0, ~0,
                                   count, type, indices, basevertex, numInstances,
                                   baseInstance);
}


/**
 * Inner support for both _mesa_MultiDrawElements() and
 * _mesa_MultiDrawRangeElements().
 * This does the actual rendering after we've checked array indexes, etc.
 */
static void
vbo_validated_multidrawelements(struct gl_context *ctx, GLenum mode,
				const GLsizei *count, GLenum type,
				const GLvoid * const *indices,
				GLsizei primcount,
				const GLint *basevertex)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim *prim;
   unsigned int index_type_size = vbo_sizeof_ib_type(type);
   uintptr_t min_index_ptr, max_index_ptr;
   GLboolean fallback = GL_FALSE;
   int i;

   if (primcount == 0)
      return;

   prim = calloc(1, primcount * sizeof(*prim));
   if (prim == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMultiDrawElements");
      return;
   }

   vbo_bind_arrays(ctx);

   min_index_ptr = (uintptr_t)indices[0];
   max_index_ptr = 0;
   for (i = 0; i < primcount; i++) {
      min_index_ptr = MIN2(min_index_ptr, (uintptr_t)indices[i]);
      max_index_ptr = MAX2(max_index_ptr, (uintptr_t)indices[i] +
			   index_type_size * count[i]);
   }

   /* Check if we can handle this thing as a bunch of index offsets from the
    * same index pointer.  If we can't, then we have to fall back to doing
    * a draw_prims per primitive.
    * Check that the difference between each prim's indexes is a multiple of
    * the index/element size.
    */
   if (index_type_size != 1) {
      for (i = 0; i < primcount; i++) {
	 if ((((uintptr_t)indices[i] - min_index_ptr) % index_type_size) != 0) {
	    fallback = GL_TRUE;
	    break;
	 }
      }
   }

   /* Draw primitives individually if one count is zero, so we can easily skip
    * that primitive.
    */
   for (i = 0; i < primcount; i++) {
      if (count[i] == 0) {
         fallback = GL_TRUE;
         break;
      }
   }

   /* If the index buffer isn't in a VBO, then treating the application's
    * subranges of the index buffer as one large index buffer may lead to
    * us reading unmapped memory.
    */
   if (!_mesa_is_bufferobj(ctx->Array.VAO->IndexBufferObj))
      fallback = GL_TRUE;

   if (!fallback) {
      ib.count = (max_index_ptr - min_index_ptr) / index_type_size;
      ib.type = type;
      ib.obj = ctx->Array.VAO->IndexBufferObj;
      ib.ptr = (void *)min_index_ptr;

      for (i = 0; i < primcount; i++) {
	 prim[i].begin = (i == 0);
	 prim[i].end = (i == primcount - 1);
	 prim[i].weak = 0;
	 prim[i].pad = 0;
	 prim[i].mode = mode;
	 prim[i].start = ((uintptr_t)indices[i] - min_index_ptr) / index_type_size;
	 prim[i].count = count[i];
	 prim[i].indexed = 1;
         prim[i].num_instances = 1;
         prim[i].base_instance = 0;
         prim[i].is_indirect = 0;
	 if (basevertex != NULL)
	    prim[i].basevertex = basevertex[i];
	 else
	    prim[i].basevertex = 0;
      }

      check_buffers_are_unmapped(exec->array.inputs);
      vbo_handle_primitive_restart(ctx, prim, primcount, &ib,
                                   GL_FALSE, ~0, ~0);
   } else {
      /* render one prim at a time */
      for (i = 0; i < primcount; i++) {
	 if (count[i] == 0)
	    continue;
	 ib.count = count[i];
	 ib.type = type;
	 ib.obj = ctx->Array.VAO->IndexBufferObj;
	 ib.ptr = indices[i];

	 prim[0].begin = 1;
	 prim[0].end = 1;
	 prim[0].weak = 0;
	 prim[0].pad = 0;
	 prim[0].mode = mode;
	 prim[0].start = 0;
	 prim[0].count = count[i];
	 prim[0].indexed = 1;
         prim[0].num_instances = 1;
         prim[0].base_instance = 0;
         prim[0].is_indirect = 0;
	 if (basevertex != NULL)
	    prim[0].basevertex = basevertex[i];
	 else
	    prim[0].basevertex = 0;

         check_buffers_are_unmapped(exec->array.inputs);
         vbo_handle_primitive_restart(ctx, prim, 1, &ib,
                                      GL_FALSE, ~0, ~0);
      }
   }

   free(prim);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
      _mesa_flush(ctx);
   }
}


static void GLAPIENTRY
vbo_exec_MultiDrawElements(GLenum mode,
			   const GLsizei *count, GLenum type,
			   const GLvoid * const *indices,
			   GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!_mesa_validate_MultiDrawElements(ctx, mode, count, type, indices,
                                         primcount, NULL))
      return;

   vbo_validated_multidrawelements(ctx, mode, count, type, indices, primcount,
				   NULL);
}


static void GLAPIENTRY
vbo_exec_MultiDrawElementsBaseVertex(GLenum mode,
				     const GLsizei *count, GLenum type,
				     const GLvoid * const *indices,
				     GLsizei primcount,
				     const GLsizei *basevertex)
{
   GET_CURRENT_CONTEXT(ctx);

   if (!_mesa_validate_MultiDrawElements(ctx, mode, count, type, indices,
                                         primcount, basevertex))
      return;

   vbo_validated_multidrawelements(ctx, mode, count, type, indices, primcount,
				   basevertex);
}

static void
vbo_draw_transform_feedback(struct gl_context *ctx, GLenum mode,
                            struct gl_transform_feedback_object *obj,
                            GLuint stream, GLuint numInstances)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[2];

   if (!_mesa_validate_DrawTransformFeedback(ctx, mode, obj, stream,
                                             numInstances)) {
      return;
   }

   if (ctx->Driver.GetTransformFeedbackVertexCount &&
       (ctx->Const.AlwaysUseGetTransformFeedbackVertexCount ||
        (ctx->Const.PrimitiveRestartInSoftware &&
         ctx->Array._PrimitiveRestart) ||
        !vbo_all_varyings_in_vbos(exec->array.inputs))) {
      GLsizei n = ctx->Driver.GetTransformFeedbackVertexCount(ctx, obj, stream);
      vbo_draw_arrays(ctx, mode, 0, n, numInstances, 0);
      return;
   }

   vbo_bind_arrays(ctx);

   /* init most fields to zero */
   memset(prim, 0, sizeof(prim));
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].mode = mode;
   prim[0].num_instances = numInstances;
   prim[0].base_instance = 0;
   prim[0].is_indirect = 0;

   /* Maybe we should do some primitive splitting for primitive restart
    * (like in DrawArrays), but we have no way to know how many vertices
    * will be rendered. */

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, prim, 1, NULL,
                   GL_TRUE, 0, 0, obj, NULL);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
      _mesa_flush(ctx);
   }
}

/**
 * Like DrawArrays, but take the count from a transform feedback object.
 * \param mode  GL_POINTS, GL_LINES, GL_TRIANGLE_STRIP, etc.
 * \param name  the transform feedback object
 * User still has to setup of the vertex attribute info with
 * glVertexPointer, glColorPointer, etc.
 * Part of GL_ARB_transform_feedback2.
 */
static void GLAPIENTRY
vbo_exec_DrawTransformFeedback(GLenum mode, GLuint name)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
      _mesa_lookup_transform_feedback_object(ctx, name);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawTransformFeedback(%s, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), name);

   vbo_draw_transform_feedback(ctx, mode, obj, 0, 1);
}

static void GLAPIENTRY
vbo_exec_DrawTransformFeedbackStream(GLenum mode, GLuint name, GLuint stream)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
      _mesa_lookup_transform_feedback_object(ctx, name);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawTransformFeedbackStream(%s, %u, %u)\n",
                  _mesa_lookup_enum_by_nr(mode), name, stream);

   vbo_draw_transform_feedback(ctx, mode, obj, stream, 1);
}

static void GLAPIENTRY
vbo_exec_DrawTransformFeedbackInstanced(GLenum mode, GLuint name,
                                        GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
      _mesa_lookup_transform_feedback_object(ctx, name);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawTransformFeedbackInstanced(%s, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), name);

   vbo_draw_transform_feedback(ctx, mode, obj, 0, primcount);
}

static void GLAPIENTRY
vbo_exec_DrawTransformFeedbackStreamInstanced(GLenum mode, GLuint name,
                                              GLuint stream, GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   struct gl_transform_feedback_object *obj =
      _mesa_lookup_transform_feedback_object(ctx, name);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawTransformFeedbackStreamInstanced"
                  "(%s, %u, %u, %i)\n",
                  _mesa_lookup_enum_by_nr(mode), name, stream, primcount);

   vbo_draw_transform_feedback(ctx, mode, obj, stream, primcount);
}

static void
vbo_validated_drawarraysindirect(struct gl_context *ctx,
                                 GLenum mode, const GLvoid *indirect)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[1];

   vbo_bind_arrays(ctx);

   memset(prim, 0, sizeof(prim));
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].mode = mode;
   prim[0].is_indirect = 1;
   prim[0].indirect_offset = (GLsizeiptr)indirect;

   /* NOTE: We do NOT want to handle primitive restart here, nor perform any
    * other checks that require knowledge of the values in the command buffer.
    * That would defeat the whole purpose of this function.
    */

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, prim, 1,
                   NULL, GL_TRUE, 0, ~0,
                   NULL,
                   ctx->DrawIndirectBuffer);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

static void
vbo_validated_multidrawarraysindirect(struct gl_context *ctx,
                                      GLenum mode,
                                      const GLvoid *indirect,
                                      GLsizei primcount, GLsizei stride)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim *prim;
   GLsizei i;
   GLsizeiptr offset = (GLsizeiptr)indirect;

   if (primcount == 0)
      return;
   prim = calloc(primcount, sizeof(*prim));
   if (prim == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMultiDrawArraysIndirect");
      return;
   }

   vbo_bind_arrays(ctx);

   prim[0].begin = 1;
   prim[primcount - 1].end = 1;
   for (i = 0; i < primcount; ++i, offset += stride) {
      prim[i].mode = mode;
      prim[i].indirect_offset = offset;
      prim[i].is_indirect = 1;
   }

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, prim, primcount,
                   NULL, GL_TRUE, 0, ~0,
                   NULL,
                   ctx->DrawIndirectBuffer);

   free(prim);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

static void
vbo_validated_drawelementsindirect(struct gl_context *ctx,
                                   GLenum mode, GLenum type,
                                   const GLvoid *indirect)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim prim[1];

   vbo_bind_arrays(ctx);

   ib.count = 0; /* unknown */
   ib.type = type;
   ib.obj = ctx->Array.VAO->IndexBufferObj;
   ib.ptr = NULL;

   memset(prim, 0, sizeof(prim));
   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].mode = mode;
   prim[0].indexed = 1;
   prim[0].indirect_offset = (GLsizeiptr)indirect;
   prim[0].is_indirect = 1;

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, prim, 1,
                   &ib, GL_TRUE, 0, ~0,
                   NULL,
                   ctx->DrawIndirectBuffer);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

static void
vbo_validated_multidrawelementsindirect(struct gl_context *ctx,
                                        GLenum mode, GLenum type,
                                        const GLvoid *indirect,
                                        GLsizei primcount, GLsizei stride)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim *prim;
   GLsizei i;
   GLsizeiptr offset = (GLsizeiptr)indirect;

   if (primcount == 0)
      return;
   prim = calloc(primcount, sizeof(*prim));
   if (prim == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMultiDrawElementsIndirect");
      return;
   }

   vbo_bind_arrays(ctx);

   /* NOTE: IndexBufferObj is guaranteed to be a VBO. */

   ib.count = 0; /* unknown */
   ib.type = type;
   ib.obj = ctx->Array.VAO->IndexBufferObj;
   ib.ptr = NULL;

   prim[0].begin = 1;
   prim[primcount - 1].end = 1;
   for (i = 0; i < primcount; ++i, offset += stride) {
      prim[i].mode = mode;
      prim[i].indexed = 1;
      prim[i].indirect_offset = offset;
      prim[i].is_indirect = 1;
   }

   check_buffers_are_unmapped(exec->array.inputs);
   vbo->draw_prims(ctx, prim, primcount,
                   &ib, GL_TRUE, 0, ~0,
                   NULL,
                   ctx->DrawIndirectBuffer);

   free(prim);

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH)
      _mesa_flush(ctx);
}

/**
 * Like [Multi]DrawArrays/Elements, but they take most arguments from
 * a buffer object.
 */
static void GLAPIENTRY
vbo_exec_DrawArraysIndirect(GLenum mode, const GLvoid *indirect)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArraysIndirect(%s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode), indirect);

   if (!_mesa_validate_DrawArraysIndirect(ctx, mode, indirect))
      return;

   vbo_validated_drawarraysindirect(ctx, mode, indirect);
}

static void GLAPIENTRY
vbo_exec_DrawElementsIndirect(GLenum mode, GLenum type,
                              const GLvoid *indirect)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawElementsIndirect(%s, %s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode),
                  _mesa_lookup_enum_by_nr(type), indirect);

   if (!_mesa_validate_DrawElementsIndirect(ctx, mode, type, indirect))
      return;

   vbo_validated_drawelementsindirect(ctx, mode, type, indirect);
}

static void GLAPIENTRY
vbo_exec_MultiDrawArraysIndirect(GLenum mode,
                                 const GLvoid *indirect,
                                 GLsizei primcount, GLsizei stride)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glMultiDrawArraysIndirect(%s, %p, %i, %i)\n",
                  _mesa_lookup_enum_by_nr(mode), indirect, primcount, stride);

   /* If <stride> is zero, the array elements are treated as tightly packed. */
   if (stride == 0)
      stride = 4 * sizeof(GLuint); /* sizeof(DrawArraysIndirectCommand) */

   if (!_mesa_validate_MultiDrawArraysIndirect(ctx, mode,
                                               indirect,
                                               primcount, stride))
      return;

   vbo_validated_multidrawarraysindirect(ctx, mode,
                                         indirect,
                                         primcount, stride);
}

static void GLAPIENTRY
vbo_exec_MultiDrawElementsIndirect(GLenum mode, GLenum type,
                                   const GLvoid *indirect,
                                   GLsizei primcount, GLsizei stride)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glMultiDrawElementsIndirect(%s, %s, %p, %i, %i)\n",
                  _mesa_lookup_enum_by_nr(mode),
                  _mesa_lookup_enum_by_nr(type), indirect, primcount, stride);

   /* If <stride> is zero, the array elements are treated as tightly packed. */
   if (stride == 0)
      stride = 5 * sizeof(GLuint); /* sizeof(DrawElementsIndirectCommand) */

   if (!_mesa_validate_MultiDrawElementsIndirect(ctx, mode, type,
                                                 indirect,
                                                 primcount, stride))
      return;

   vbo_validated_multidrawelementsindirect(ctx, mode, type,
                                           indirect,
                                           primcount, stride);
}

/**
 * Initialize the dispatch table with the VBO functions for drawing.
 */
void
vbo_initialize_exec_dispatch(const struct gl_context *ctx,
                             struct _glapi_table *exec)
{
   SET_DrawArrays(exec, vbo_exec_DrawArrays);
   SET_DrawElements(exec, vbo_exec_DrawElements);

   if (_mesa_is_desktop_gl(ctx) || _mesa_is_gles3(ctx)) {
      SET_DrawRangeElements(exec, vbo_exec_DrawRangeElements);
   }

   SET_MultiDrawElementsEXT(exec, vbo_exec_MultiDrawElements);

   if (ctx->API == API_OPENGL_COMPAT) {
      SET_Rectf(exec, vbo_exec_Rectf);
      SET_EvalMesh1(exec, vbo_exec_EvalMesh1);
      SET_EvalMesh2(exec, vbo_exec_EvalMesh2);
   }

   if (_mesa_is_desktop_gl(ctx)) {
      SET_DrawElementsBaseVertex(exec, vbo_exec_DrawElementsBaseVertex);
      SET_DrawRangeElementsBaseVertex(exec, vbo_exec_DrawRangeElementsBaseVertex);
      SET_MultiDrawElementsBaseVertex(exec, vbo_exec_MultiDrawElementsBaseVertex);
      SET_DrawArraysInstancedBaseInstance(exec, vbo_exec_DrawArraysInstancedBaseInstance);
      SET_DrawElementsInstancedBaseInstance(exec, vbo_exec_DrawElementsInstancedBaseInstance);
      SET_DrawElementsInstancedBaseVertex(exec, vbo_exec_DrawElementsInstancedBaseVertex);
      SET_DrawElementsInstancedBaseVertexBaseInstance(exec, vbo_exec_DrawElementsInstancedBaseVertexBaseInstance);
   }

   if (ctx->API == API_OPENGL_CORE) {
      SET_DrawArraysIndirect(exec, vbo_exec_DrawArraysIndirect);
      SET_DrawElementsIndirect(exec, vbo_exec_DrawElementsIndirect);
      SET_MultiDrawArraysIndirect(exec, vbo_exec_MultiDrawArraysIndirect);
      SET_MultiDrawElementsIndirect(exec, vbo_exec_MultiDrawElementsIndirect);
   }

   if (_mesa_is_desktop_gl(ctx) || _mesa_is_gles3(ctx)) {
      SET_DrawArraysInstancedARB(exec, vbo_exec_DrawArraysInstanced);
      SET_DrawElementsInstancedARB(exec, vbo_exec_DrawElementsInstanced);
   }

   if (_mesa_is_desktop_gl(ctx)) {
      SET_DrawTransformFeedback(exec, vbo_exec_DrawTransformFeedback);
      SET_DrawTransformFeedbackStream(exec, vbo_exec_DrawTransformFeedbackStream);
      SET_DrawTransformFeedbackInstanced(exec, vbo_exec_DrawTransformFeedbackInstanced);
      SET_DrawTransformFeedbackStreamInstanced(exec, vbo_exec_DrawTransformFeedbackStreamInstanced);
   }
}



/**
 * The following functions are only used for OpenGL ES 1/2 support.
 * And some aren't even supported (yet) in ES 1/2.
 */


void GLAPIENTRY
_mesa_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
   vbo_exec_DrawArrays(mode, first, count);
}


void GLAPIENTRY
_mesa_DrawElements(GLenum mode, GLsizei count, GLenum type,
                   const GLvoid *indices)
{
   vbo_exec_DrawElements(mode, count, type, indices);
}


void GLAPIENTRY
_mesa_DrawElementsBaseVertex(GLenum mode, GLsizei count, GLenum type,
			     const GLvoid *indices, GLint basevertex)
{
   vbo_exec_DrawElementsBaseVertex(mode, count, type, indices, basevertex);
}


void GLAPIENTRY
_mesa_DrawRangeElements(GLenum mode, GLuint start, GLuint end, GLsizei count,
                        GLenum type, const GLvoid *indices)
{
   vbo_exec_DrawRangeElements(mode, start, end, count, type, indices);
}


void GLAPIENTRY
_mesa_DrawRangeElementsBaseVertex(GLenum mode, GLuint start, GLuint end,
				  GLsizei count, GLenum type,
				  const GLvoid *indices, GLint basevertex)
{
   vbo_exec_DrawRangeElementsBaseVertex(mode, start, end, count, type,
					indices, basevertex);
}


void GLAPIENTRY
_mesa_MultiDrawElementsEXT(GLenum mode, const GLsizei *count, GLenum type,
			   const GLvoid **indices, GLsizei primcount)
{
   vbo_exec_MultiDrawElements(mode, count, type, indices, primcount);
}


void GLAPIENTRY
_mesa_MultiDrawElementsBaseVertex(GLenum mode,
				  const GLsizei *count, GLenum type,
				  const GLvoid **indices, GLsizei primcount,
				  const GLint *basevertex)
{
   vbo_exec_MultiDrawElementsBaseVertex(mode, count, type, indices,
					primcount, basevertex);
}

void GLAPIENTRY
_mesa_DrawTransformFeedback(GLenum mode, GLuint name)
{
   vbo_exec_DrawTransformFeedback(mode, name);
}
