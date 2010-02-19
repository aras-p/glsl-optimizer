/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

#include "main/glheader.h"
#include "main/context.h"
#include "main/state.h"
#include "main/api_validate.h"
#include "main/api_noop.h"
#include "main/varray.h"
#include "main/bufferobj.h"
#include "main/enums.h"
#include "main/macros.h"

#include "vbo_context.h"


/**
 * Compute min and max elements for glDraw[Range]Elements() calls.
 */
void
vbo_get_minmax_index(GLcontext *ctx,
		     const struct _mesa_prim *prim,
		     const struct _mesa_index_buffer *ib,
		     GLuint *min_index, GLuint *max_index)
{
   GLuint i;
   GLuint count = prim->count;
   const void *indices;

   if (_mesa_is_bufferobj(ib->obj)) {
      const GLvoid *map =
         ctx->Driver.MapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB,
                               GL_READ_ONLY, ib->obj);
      indices = ADD_POINTERS(map, ib->ptr);
   } else {
      indices = ib->ptr;
   }

   switch (ib->type) {
   case GL_UNSIGNED_INT: {
      const GLuint *ui_indices = (const GLuint *)indices;
      GLuint max_ui = ui_indices[count-1];
      GLuint min_ui = ui_indices[0];
      for (i = 0; i < count; i++) {
	 if (ui_indices[i] > max_ui) max_ui = ui_indices[i];
	 if (ui_indices[i] < min_ui) min_ui = ui_indices[i];
      }
      *min_index = min_ui;
      *max_index = max_ui;
      break;
   }
   case GL_UNSIGNED_SHORT: {
      const GLushort *us_indices = (const GLushort *)indices;
      GLuint max_us = us_indices[count-1];
      GLuint min_us = us_indices[0];
      for (i = 0; i < count; i++) {
	 if (us_indices[i] > max_us) max_us = us_indices[i];
	 if (us_indices[i] < min_us) min_us = us_indices[i];
      }
      *min_index = min_us;
      *max_index = max_us;
      break;
   }
   case GL_UNSIGNED_BYTE: {
      const GLubyte *ub_indices = (const GLubyte *)indices;
      GLuint max_ub = ub_indices[count-1];
      GLuint min_ub = ub_indices[0];
      for (i = 0; i < count; i++) {
	 if (ub_indices[i] > max_ub) max_ub = ub_indices[i];
	 if (ub_indices[i] < min_ub) min_ub = ub_indices[i];
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
      ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB, ib->obj);
   }
}


/**
 * Check that element 'j' of the array has reasonable data.
 * Map VBO if needed.
 */
static void
check_array_data(GLcontext *ctx, struct gl_client_array *array,
                 GLuint attrib, GLuint j)
{
   if (array->Enabled) {
      const void *data = array->Ptr;
      if (_mesa_is_bufferobj(array->BufferObj)) {
         if (!array->BufferObj->Pointer) {
            /* need to map now */
            array->BufferObj->Pointer =
               ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER_ARB,
                                     GL_READ_ONLY, array->BufferObj);
         }
         data = ADD_POINTERS(data, array->BufferObj->Pointer);
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
unmap_array_buffer(GLcontext *ctx, struct gl_client_array *array)
{
   if (array->Enabled &&
       _mesa_is_bufferobj(array->BufferObj) &&
       _mesa_bufferobj_mapped(array->BufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER_ARB, array->BufferObj);
   }
}


/**
 * Examine the array's data for NaNs, etc.
 */
static void
check_draw_elements_data(GLcontext *ctx, GLsizei count, GLenum elemType,
                         const void *elements, GLint basevertex)
{
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
   const void *elemMap;
   GLint i, k;

   if (_mesa_is_bufferobj(ctx->Array.ElementArrayBufferObj)) {
      elemMap = ctx->Driver.MapBuffer(ctx,
                                      GL_ELEMENT_ARRAY_BUFFER_ARB,
                                      GL_READ_ONLY,
                                      ctx->Array.ElementArrayBufferObj);
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
      check_array_data(ctx, &arrayObj->Vertex, VERT_ATTRIB_POS, j);
      check_array_data(ctx, &arrayObj->Normal, VERT_ATTRIB_NORMAL, j);
      check_array_data(ctx, &arrayObj->Color, VERT_ATTRIB_COLOR0, j);
      check_array_data(ctx, &arrayObj->SecondaryColor, VERT_ATTRIB_COLOR1, j);
      for (k = 0; k < Elements(arrayObj->TexCoord); k++) {
         check_array_data(ctx, &arrayObj->TexCoord[k], VERT_ATTRIB_TEX0 + k, j);
      }
      for (k = 0; k < Elements(arrayObj->VertexAttrib); k++) {
         check_array_data(ctx, &arrayObj->VertexAttrib[k],
                          VERT_ATTRIB_GENERIC0 + k, j);
      }
   }

   if (_mesa_is_bufferobj(ctx->Array.ElementArrayBufferObj)) {
      ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB,
			      ctx->Array.ElementArrayBufferObj);
   }

   unmap_array_buffer(ctx, &arrayObj->Vertex);
   unmap_array_buffer(ctx, &arrayObj->Normal);
   unmap_array_buffer(ctx, &arrayObj->Color);
   for (k = 0; k < Elements(arrayObj->TexCoord); k++) {
      unmap_array_buffer(ctx, &arrayObj->TexCoord[k]);
   }
   for (k = 0; k < Elements(arrayObj->VertexAttrib); k++) {
      unmap_array_buffer(ctx, &arrayObj->VertexAttrib[k]);
   }
}


/**
 * Check array data, looking for NaNs, etc.
 */
static void
check_draw_arrays_data(GLcontext *ctx, GLint start, GLsizei count)
{
   /* TO DO */
}


/**
 * Print info/data for glDrawArrays().
 */
static void
print_draw_arrays(GLcontext *ctx, struct vbo_exec_context *exec,
                  GLenum mode, GLint start, GLsizei count)
{
   int i;

   printf("vbo_exec_DrawArrays(mode 0x%x, start %d, count %d):\n",
	  mode, start, count);

   for (i = 0; i < 32; i++) {
      GLuint bufName = exec->array.inputs[i]->BufferObj->Name;
      GLint stride = exec->array.inputs[i]->Stride;
      printf("attr %2d: size %d stride %d  enabled %d  "
	     "ptr %p  Bufobj %u\n",
	     i,
	     exec->array.inputs[i]->Size,
	     stride,
	     /*exec->array.inputs[i]->Enabled,*/
	     exec->array.legacy_array[i]->Enabled,
	     exec->array.inputs[i]->Ptr,
	     bufName);

      if (bufName) {
         struct gl_buffer_object *buf = _mesa_lookup_bufferobj(ctx, bufName);
         GLubyte *p = ctx->Driver.MapBuffer(ctx, GL_ARRAY_BUFFER_ARB,
                                            GL_READ_ONLY_ARB, buf);
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
         ctx->Driver.UnmapBuffer(ctx, GL_ARRAY_BUFFER_ARB, buf);
      }
   }
}


/**
 * Just translate the arrayobj into a sane layout.
 */
static void
bind_array_obj(GLcontext *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_array_object *arrayObj = ctx->Array.ArrayObj;
   GLuint i;

   /* TODO: Fix the ArrayObj struct to keep legacy arrays in an array
    * rather than as individual named arrays.  Then this function can
    * go away.
    */
   exec->array.legacy_array[VERT_ATTRIB_POS] = &arrayObj->Vertex;
   exec->array.legacy_array[VERT_ATTRIB_WEIGHT] = &arrayObj->Weight;
   exec->array.legacy_array[VERT_ATTRIB_NORMAL] = &arrayObj->Normal;
   exec->array.legacy_array[VERT_ATTRIB_COLOR0] = &arrayObj->Color;
   exec->array.legacy_array[VERT_ATTRIB_COLOR1] = &arrayObj->SecondaryColor;
   exec->array.legacy_array[VERT_ATTRIB_FOG] = &arrayObj->FogCoord;
   exec->array.legacy_array[VERT_ATTRIB_COLOR_INDEX] = &arrayObj->Index;
   if (arrayObj->PointSize.Enabled) {
      /* this aliases COLOR_INDEX */
      exec->array.legacy_array[VERT_ATTRIB_POINT_SIZE] = &arrayObj->PointSize;
   }
   exec->array.legacy_array[VERT_ATTRIB_EDGEFLAG] = &arrayObj->EdgeFlag;

   for (i = 0; i < Elements(arrayObj->TexCoord); i++)
      exec->array.legacy_array[VERT_ATTRIB_TEX0 + i] = &arrayObj->TexCoord[i];

   for (i = 0; i < Elements(arrayObj->VertexAttrib); i++) {
      assert(i < Elements(exec->array.generic_array));
      exec->array.generic_array[i] = &arrayObj->VertexAttrib[i];
   }
   
   exec->array.array_obj = arrayObj->Name;
}


static void
recalculate_input_bindings(GLcontext *ctx)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   const struct gl_client_array **inputs = &exec->array.inputs[0];
   GLbitfield const_inputs = 0x0;
   GLuint i;

   exec->array.program_mode = get_program_mode(ctx);
   exec->array.enabled_flags = ctx->Array.ArrayObj->_Enabled;

   switch (exec->array.program_mode) {
   case VP_NONE:
      /* When no vertex program is active (or the vertex program is generated
       * from fixed-function state).  We put the material values into the
       * generic slots.  This is the only situation where material values
       * are available as per-vertex attributes.
       */
      for (i = 0; i <= VERT_ATTRIB_TEX7; i++) {
	 if (exec->array.legacy_array[i]->Enabled)
	    inputs[i] = exec->array.legacy_array[i];
	 else {
	    inputs[i] = &vbo->legacy_currval[i];
            const_inputs |= 1 << i;
         }
      }

      for (i = 0; i < MAT_ATTRIB_MAX; i++) {
	 inputs[VERT_ATTRIB_GENERIC0 + i] = &vbo->mat_currval[i];
         const_inputs |= 1 << (VERT_ATTRIB_GENERIC0 + i);
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = MAT_ATTRIB_MAX; i < VERT_ATTRIB_MAX - VERT_ATTRIB_GENERIC0; i++) {
	 inputs[VERT_ATTRIB_GENERIC0 + i] = &vbo->generic_currval[i];
         const_inputs |= 1 << (VERT_ATTRIB_GENERIC0 + i);
      }
      break;

   case VP_NV:
      /* NV_vertex_program - attribute arrays alias and override
       * conventional, legacy arrays.  No materials, and the generic
       * slots are vacant.
       */
      for (i = 0; i <= VERT_ATTRIB_TEX7; i++) {
	 if (exec->array.generic_array[i]->Enabled)
	    inputs[i] = exec->array.generic_array[i];
	 else if (exec->array.legacy_array[i]->Enabled)
	    inputs[i] = exec->array.legacy_array[i];
	 else {
	    inputs[i] = &vbo->legacy_currval[i];
            const_inputs |= 1 << i;
         }
      }

      /* Could use just about anything, just to fill in the empty
       * slots:
       */
      for (i = VERT_ATTRIB_GENERIC0; i < VERT_ATTRIB_MAX; i++) {
	 inputs[i] = &vbo->generic_currval[i - VERT_ATTRIB_GENERIC0];
         const_inputs |= 1 << i;
      }
      break;

   case VP_ARB:
      /* GL_ARB_vertex_program or GLSL vertex shader - Only the generic[0]
       * attribute array aliases and overrides the legacy position array.  
       *
       * Otherwise, legacy attributes available in the legacy slots,
       * generic attributes in the generic slots and materials are not
       * available as per-vertex attributes.
       */
      if (exec->array.generic_array[0]->Enabled)
	 inputs[0] = exec->array.generic_array[0];
      else if (exec->array.legacy_array[0]->Enabled)
	 inputs[0] = exec->array.legacy_array[0];
      else {
	 inputs[0] = &vbo->legacy_currval[0];
         const_inputs |= 1 << 0;
      }

      for (i = 1; i <= VERT_ATTRIB_TEX7; i++) {
	 if (exec->array.legacy_array[i]->Enabled)
	    inputs[i] = exec->array.legacy_array[i];
	 else {
	    inputs[i] = &vbo->legacy_currval[i];
            const_inputs |= 1 << i;
         }
      }

      for (i = 0; i < MAX_VERTEX_GENERIC_ATTRIBS; i++) {
	 if (exec->array.generic_array[i]->Enabled)
	    inputs[VERT_ATTRIB_GENERIC0 + i] = exec->array.generic_array[i];
	 else {
	    inputs[VERT_ATTRIB_GENERIC0 + i] = &vbo->generic_currval[i];
            const_inputs |= 1 << (VERT_ATTRIB_GENERIC0 + i);
         }

      }
      break;
   }

   _mesa_set_varying_vp_inputs( ctx, ~const_inputs );
}


/**
 * Examine the enabled vertex arrays to set the exec->array.inputs[] values.
 * These will point to the arrays to actually use for drawing.  Some will
 * be user-provided arrays, other will be zero-stride const-valued arrays.
 * Note that this might set the _NEW_ARRAY dirty flag so state validation
 * must be done after this call.
 */
static void
bind_arrays(GLcontext *ctx)
{
#if 0
   if (ctx->Array.ArrayObj.Name != exec->array.array_obj) {
      bind_array_obj(ctx);
      recalculate_input_bindings(ctx);
   }
   else if (exec->array.program_mode != get_program_mode(ctx) ||
	    exec->array.enabled_flags != ctx->Array.ArrayObj->_Enabled) {
      recalculate_input_bindings(ctx);
   }
#else
   bind_array_obj(ctx);
   recalculate_input_bindings(ctx);
#endif
}



/***********************************************************************
 * API functions.
 */

static void GLAPIENTRY
vbo_exec_DrawArrays(GLenum mode, GLint start, GLsizei count)
{
   GET_CURRENT_CONTEXT(ctx);
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_prim prim[1];

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx, "glDrawArrays(%s, %d, %d)\n",
                  _mesa_lookup_enum_by_nr(mode), start, count);

   if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glDrawArrays")) {
      return;
   }

#if 0
   check_draw_arrays_data(ctx, start, count);
#else
   (void) check_draw_arrays_data;
#endif

   bind_arrays( ctx );

   /* Again... because we may have changed the bitmask of per-vertex varying
    * attributes.  If we regenerate the fixed-function vertex program now
    * we may be able to prune down the number of vertex attributes which we
    * need in the shader.
    */
   if (ctx->NewState)
      _mesa_update_state( ctx );

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = start;
   prim[0].count = count;
   prim[0].indexed = 0;
   prim[0].basevertex = 0;

   vbo->draw_prims( ctx, exec->array.inputs, prim, 1, NULL,
                    GL_TRUE, start, start + count - 1 );

#if 0
   print_draw_arrays(ctx, exec, mode, start, count);
#else
   (void) print_draw_arrays;
#endif
}


/**
 * Map GL_ELEMENT_ARRAY_BUFFER and print contents.
 */
static void
dump_element_buffer(GLcontext *ctx, GLenum type)
{
   const GLvoid *map = ctx->Driver.MapBuffer(ctx,
                                             GL_ELEMENT_ARRAY_BUFFER_ARB,
                                             GL_READ_ONLY,
                                             ctx->Array.ElementArrayBufferObj);
   switch (type) {
   case GL_UNSIGNED_BYTE:
      {
         const GLubyte *us = (const GLubyte *) map;
         GLint i;
         for (i = 0; i < ctx->Array.ElementArrayBufferObj->Size; i++) {
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
         for (i = 0; i < ctx->Array.ElementArrayBufferObj->Size / 2; i++) {
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
         for (i = 0; i < ctx->Array.ElementArrayBufferObj->Size / 4; i++) {
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

   ctx->Driver.UnmapBuffer(ctx, GL_ELEMENT_ARRAY_BUFFER_ARB,
                           ctx->Array.ElementArrayBufferObj);
}


/* Inner support for both _mesa_DrawElements and _mesa_DrawRangeElements */
static void
vbo_validated_drawrangeelements(GLcontext *ctx, GLenum mode,
				GLboolean index_bounds_valid,
				GLuint start, GLuint end,
				GLsizei count, GLenum type,
				const GLvoid *indices,
				GLint basevertex)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim prim[1];

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glDraw[Range]Elements")) {
      return;
   }

   bind_arrays( ctx );

   /* check for dirty state again */
   if (ctx->NewState)
      _mesa_update_state( ctx );

   ib.count = count;
   ib.type = type;
   ib.obj = ctx->Array.ElementArrayBufferObj;
   ib.ptr = indices;

   prim[0].begin = 1;
   prim[0].end = 1;
   prim[0].weak = 0;
   prim[0].pad = 0;
   prim[0].mode = mode;
   prim[0].start = 0;
   prim[0].count = count;
   prim[0].indexed = 1;
   prim[0].basevertex = basevertex;

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

   vbo->draw_prims( ctx, exec->array.inputs, prim, 1, &ib,
		    index_bounds_valid, start, end );
}

static void GLAPIENTRY
vbo_exec_DrawRangeElementsBaseVertex(GLenum mode,
				     GLuint start, GLuint end,
				     GLsizei count, GLenum type,
				     const GLvoid *indices,
				     GLint basevertex)
{
   static GLuint warnCount = 0;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx,
                "glDrawRangeElementsBaseVertex(%s, %u, %u, %d, %s, %p, %d)\n",
                _mesa_lookup_enum_by_nr(mode), start, end, count,
                _mesa_lookup_enum_by_nr(type), indices, basevertex);

   if (!_mesa_validate_DrawRangeElements( ctx, mode, start, end, count,
                                          type, indices, basevertex ))
      return;

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

   if (end >= ctx->Array.ArrayObj->_MaxElement) {
      /* the max element is out of bounds of one or more enabled arrays */
      warnCount++;

      if (warnCount < 10) {
         _mesa_warning(ctx, "glDraw[Range]Elements(start %u, end %u, count %d, "
                       "type 0x%x, indices=%p)\n"
                       "\tend is out of bounds (max=%u)  "
                       "Element Buffer %u (size %d)\n"
                       "\tThis should probably be fixed in the application.",
                       start, end, count, type, indices,
                       ctx->Array.ArrayObj->_MaxElement - 1,
                       ctx->Array.ElementArrayBufferObj->Name,
                       ctx->Array.ElementArrayBufferObj->Size);
      }

      if (0)
         dump_element_buffer(ctx, type);

      if (0)
         _mesa_print_arrays(ctx);

#ifdef DEBUG
      /* 'end' was out of bounds, but now let's check the actual array
       * indexes to see if any of them are out of bounds.
       */
      {
         GLuint max = _mesa_max_buffer_index(ctx, count, type, indices,
                                             ctx->Array.ElementArrayBufferObj);
         if (max >= ctx->Array.ArrayObj->_MaxElement) {
            if (warnCount < 10) {
               _mesa_warning(ctx, "glDraw[Range]Elements(start %u, end %u, "
                             "count %d, type 0x%x, indices=%p)\n"
                             "\tindex=%u is out of bounds (max=%u)  "
                             "Element Buffer %u (size %d)\n"
                             "\tSkipping the glDrawRangeElements() call",
                             start, end, count, type, indices, max,
                             ctx->Array.ArrayObj->_MaxElement - 1,
                             ctx->Array.ElementArrayBufferObj->Name,
                             ctx->Array.ElementArrayBufferObj->Size);
            }
         }
         /* XXX we could also find the min index and compare to 'start'
          * to see if start is correct.  But it's more likely to get the
          * upper bound wrong.
          */
      }
#endif

      /* Set 'end' to the max possible legal value */
      assert(ctx->Array.ArrayObj->_MaxElement >= 1);
      end = ctx->Array.ArrayObj->_MaxElement - 1;
   }
   else if (0) {
      printf("glDraw[Range]Elements{,BaseVertex}"
	     "(start %u, end %u, type 0x%x, count %d) ElemBuf %u, "
	     "base %d\n",
	     start, end, type, count,
	     ctx->Array.ElementArrayBufferObj->Name,
	     basevertex);
   }

#if 0
   check_draw_elements_data(ctx, count, type, indices);
#else
   (void) check_draw_elements_data;
#endif

   vbo_validated_drawrangeelements(ctx, mode, GL_TRUE, start, end,
				   count, type, indices, basevertex);
}


static void GLAPIENTRY
vbo_exec_DrawRangeElements(GLenum mode, GLuint start, GLuint end,
                           GLsizei count, GLenum type, const GLvoid *indices)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_DRAW)
      _mesa_debug(ctx,
                  "glDrawRangeElements(%s, %u, %u, %d, %s, %p)\n",
                  _mesa_lookup_enum_by_nr(mode), start, end, count,
                  _mesa_lookup_enum_by_nr(type), indices);

   vbo_exec_DrawRangeElementsBaseVertex(mode, start, end, count, type,
					indices, 0);
}


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
				   count, type, indices, 0);
}


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
				   count, type, indices, basevertex);
}


/** Inner support for both _mesa_DrawElements and _mesa_DrawRangeElements */
static void
vbo_validated_multidrawelements(GLcontext *ctx, GLenum mode,
				const GLsizei *count, GLenum type,
				const GLvoid **indices, GLsizei primcount,
				const GLint *basevertex)
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct _mesa_index_buffer ib;
   struct _mesa_prim *prim;
   unsigned int index_type_size = 0;
   uintptr_t min_index_ptr, max_index_ptr;
   GLboolean fallback = GL_FALSE;
   int i;

   if (primcount == 0)
      return;

   FLUSH_CURRENT( ctx, 0 );

   if (!_mesa_valid_to_render(ctx, "glMultiDrawElements")) {
      return;
   }

   prim = calloc(1, primcount * sizeof(*prim));
   if (prim == NULL) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "glMultiDrawElements");
      return;
   }

   /* Decide if we can do this all as one set of primitives sharing the
    * same index buffer, or if we have to reset the index pointer per primitive.
    */
   bind_arrays( ctx );

   /* check for dirty state again */
   if (ctx->NewState)
      _mesa_update_state( ctx );

   switch (type) {
   case GL_UNSIGNED_INT:
      index_type_size = 4;
      break;
   case GL_UNSIGNED_SHORT:
      index_type_size = 2;
      break;
   case GL_UNSIGNED_BYTE:
      index_type_size = 1;
      break;
   default:
      assert(0);
   }

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
    */
   if (index_type_size != 1) {
      for (i = 0; i < primcount; i++) {
	 if ((((uintptr_t)indices[i] - min_index_ptr) % index_type_size) != 0) {
	    fallback = GL_TRUE;
	    break;
	 }
      }
   }

   /* If the index buffer isn't in a VBO, then treating the application's
    * subranges of the index buffer as one large index buffer may lead to
    * us reading unmapped memory.
    */
   if (!_mesa_is_bufferobj(ctx->Array.ElementArrayBufferObj))
      fallback = GL_TRUE;

   if (!fallback) {
      ib.count = (max_index_ptr - min_index_ptr) / index_type_size;
      ib.type = type;
      ib.obj = ctx->Array.ElementArrayBufferObj;
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
	 if (basevertex != NULL)
	    prim[i].basevertex = basevertex[i];
	 else
	    prim[i].basevertex = 0;
      }

      vbo->draw_prims(ctx, exec->array.inputs, prim, primcount, &ib,
		      GL_FALSE, ~0, ~0);
   } else {
      /* render one prim at a time */
      for (i = 0; i < primcount; i++) {
	 ib.count = count[i];
	 ib.type = type;
	 ib.obj = ctx->Array.ElementArrayBufferObj;
	 ib.ptr = indices[i];

	 prim[0].begin = 1;
	 prim[0].end = 1;
	 prim[0].weak = 0;
	 prim[0].pad = 0;
	 prim[0].mode = mode;
	 prim[0].start = 0;
	 prim[0].count = count[i];
	 prim[0].indexed = 1;
	 if (basevertex != NULL)
	    prim[0].basevertex = basevertex[i];
	 else
	    prim[0].basevertex = 0;

         vbo->draw_prims(ctx, exec->array.inputs, prim, 1, &ib,
                         GL_FALSE, ~0, ~0);
      }
   }

   free(prim);
}


static void GLAPIENTRY
vbo_exec_MultiDrawElements(GLenum mode,
			   const GLsizei *count, GLenum type,
			   const GLvoid **indices,
			   GLsizei primcount)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   for (i = 0; i < primcount; i++) {
      if (!_mesa_validate_DrawElements(ctx, mode, count[i], type, indices[i],
				       0))
	 return;
   }

   vbo_validated_multidrawelements(ctx, mode, count, type, indices, primcount,
				   NULL);
}


static void GLAPIENTRY
vbo_exec_MultiDrawElementsBaseVertex(GLenum mode,
				     const GLsizei *count, GLenum type,
				     const GLvoid **indices,
				     GLsizei primcount,
				     const GLsizei *basevertex)
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx);

   for (i = 0; i < primcount; i++) {
      if (!_mesa_validate_DrawElements(ctx, mode, count[i], type, indices[i],
				       basevertex[i]))
	 return;
   }

   vbo_validated_multidrawelements(ctx, mode, count, type, indices, primcount,
				   basevertex);
}


/***********************************************************************
 * Initialization
 */

void
vbo_exec_array_init( struct vbo_exec_context *exec )
{
#if 1
   exec->vtxfmt.DrawArrays = vbo_exec_DrawArrays;
   exec->vtxfmt.DrawElements = vbo_exec_DrawElements;
   exec->vtxfmt.DrawRangeElements = vbo_exec_DrawRangeElements;
   exec->vtxfmt.MultiDrawElementsEXT = vbo_exec_MultiDrawElements;
   exec->vtxfmt.DrawElementsBaseVertex = vbo_exec_DrawElementsBaseVertex;
   exec->vtxfmt.DrawRangeElementsBaseVertex = vbo_exec_DrawRangeElementsBaseVertex;
   exec->vtxfmt.MultiDrawElementsBaseVertex = vbo_exec_MultiDrawElementsBaseVertex;
#else
   exec->vtxfmt.DrawArrays = _mesa_noop_DrawArrays;
   exec->vtxfmt.DrawElements = _mesa_noop_DrawElements;
   exec->vtxfmt.DrawRangeElements = _mesa_noop_DrawRangeElements;
   exec->vtxfmt.MultiDrawElementsEXT = _mesa_noop_MultiDrawElements;
   exec->vtxfmt.DrawElementsBaseVertex = _mesa_noop_DrawElementsBaseVertex;
   exec->vtxfmt.DrawRangeElementsBaseVertex = _mesa_noop_DrawRangeElementsBaseVertex;
   exec->vtxfmt.MultiDrawElementsBaseVertex = _mesa_noop_MultiDrawElementsBaseVertex;
#endif
}


void
vbo_exec_array_destroy( struct vbo_exec_context *exec )
{
   /* nothing to do */
}


/* This API entrypoint is not ordinarily used */
void GLAPIENTRY
_mesa_DrawArrays(GLenum mode, GLint first, GLsizei count)
{
   vbo_exec_DrawArrays(mode, first, count);
}


/* This API entrypoint is not ordinarily used */
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


/* This API entrypoint is not ordinarily used */
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


/* GL_EXT_multi_draw_arrays */
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
