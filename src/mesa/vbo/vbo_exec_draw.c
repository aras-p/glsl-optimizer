/*
 * Mesa 3-D graphics library
 * Version:  7.2
 *
 * Copyright (C) 1999-2008  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/enums.h"
#include "main/state.h"

#include "vbo_context.h"


#if FEATURE_beginend


static void
vbo_exec_debug_verts( struct vbo_exec_context *exec )
{
   GLuint count = exec->vtx.vert_count;
   GLuint i;

   printf("%s: %u vertices %d primitives, %d vertsize\n",
	  __FUNCTION__,
	  count,
	  exec->vtx.prim_count,
	  exec->vtx.vertex_size);

   for (i = 0 ; i < exec->vtx.prim_count ; i++) {
      struct _mesa_prim *prim = &exec->vtx.prim[i];
      printf("   prim %d: %s%s %d..%d %s %s\n",
	     i, 
	     _mesa_lookup_prim_by_nr(prim->mode),
	     prim->weak ? " (weak)" : "",
	     prim->start, 
	     prim->start + prim->count,
	     prim->begin ? "BEGIN" : "(wrap)",
	     prim->end ? "END" : "(wrap)");
   }
}


/*
 * NOTE: Need to have calculated primitives by this point -- do it on the fly.
 * NOTE: Old 'parity' issue is gone.
 */
static GLuint
vbo_copy_vertices( struct vbo_exec_context *exec )
{
   GLuint nr = exec->vtx.prim[exec->vtx.prim_count-1].count;
   GLuint ovf, i;
   GLuint sz = exec->vtx.vertex_size;
   GLfloat *dst = exec->vtx.copied.buffer;
   const GLfloat *src = (exec->vtx.buffer_map + 
                         exec->vtx.prim[exec->vtx.prim_count-1].start * 
                         exec->vtx.vertex_size);


   switch (exec->ctx->Driver.CurrentExecPrimitive) {
   case GL_POINTS:
      return 0;
   case GL_LINES:
      ovf = nr&1;
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_TRIANGLES:
      ovf = nr%3;
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_QUADS:
      ovf = nr&3;
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case GL_LINE_STRIP:
      if (nr == 0) {
	 return 0;
      }
      else {
	 memcpy( dst, src+(nr-1)*sz, sz * sizeof(GLfloat) );
	 return 1;
      }
   case GL_LINE_LOOP:
   case GL_TRIANGLE_FAN:
   case GL_POLYGON:
      if (nr == 0) {
	 return 0;
      }
      else if (nr == 1) {
	 memcpy( dst, src+0, sz * sizeof(GLfloat) );
	 return 1;
      }
      else {
	 memcpy( dst, src+0, sz * sizeof(GLfloat) );
	 memcpy( dst+sz, src+(nr-1)*sz, sz * sizeof(GLfloat) );
	 return 2;
      }
   case GL_TRIANGLE_STRIP:
      /* no parity issue, but need to make sure the tri is not drawn twice */
      if (nr & 1) {
	 exec->vtx.prim[exec->vtx.prim_count-1].count--;
      }
      /* fallthrough */
   case GL_QUAD_STRIP:
      switch (nr) {
      case 0:
         ovf = 0;
         break;
      case 1:
         ovf = 1;
         break;
      default:
         ovf = 2 + (nr & 1);
         break;
      }
      for (i = 0 ; i < ovf ; i++)
	 memcpy( dst+i*sz, src+(nr-ovf+i)*sz, sz * sizeof(GLfloat) );
      return i;
   case PRIM_OUTSIDE_BEGIN_END:
      return 0;
   default:
      assert(0);
      return 0;
   }
}



/* TODO: populate these as the vertex is defined:
 */
static void
vbo_exec_bind_arrays( GLcontext *ctx )
{
   struct vbo_context *vbo = vbo_context(ctx);
   struct vbo_exec_context *exec = &vbo->exec;
   struct gl_client_array *arrays = exec->vtx.arrays;
   const GLuint count = exec->vtx.vert_count;
   const GLubyte *data = (GLubyte *) exec->vtx.buffer_map;
   const GLuint *map;
   GLuint attr;
   GLbitfield varying_inputs = 0x0;

   /* Install the default (ie Current) attributes first, then overlay
    * all active ones.
    */
   switch (get_program_mode(exec->ctx)) {
   case VP_NONE:
      for (attr = 0; attr < 16; attr++) {
         exec->vtx.inputs[attr] = &vbo->legacy_currval[attr];
      }
      for (attr = 0; attr < MAT_ATTRIB_MAX; attr++) {
         ASSERT(attr + 16 < Elements(exec->vtx.inputs));
         exec->vtx.inputs[attr + 16] = &vbo->mat_currval[attr];
      }
      map = vbo->map_vp_none;
      break;
   case VP_NV:
   case VP_ARB:
      /* The aliasing of attributes for NV vertex programs has already
       * occurred.  NV vertex programs cannot access material values,
       * nor attributes greater than VERT_ATTRIB_TEX7.  
       */
      for (attr = 0; attr < 16; attr++) {
         exec->vtx.inputs[attr] = &vbo->legacy_currval[attr];
         ASSERT(attr + 16 < Elements(exec->vtx.inputs));
         exec->vtx.inputs[attr + 16] = &vbo->generic_currval[attr];
      }
      map = vbo->map_vp_arb;

      /* check if VERT_ATTRIB_POS is not read but VERT_BIT_GENERIC0 is read.
       * In that case we effectively need to route the data from
       * glVertexAttrib(0, val) calls to feed into the GENERIC0 input.
       */
      if ((ctx->VertexProgram._Current->Base.InputsRead & VERT_BIT_POS) == 0 &&
          (ctx->VertexProgram._Current->Base.InputsRead & VERT_BIT_GENERIC0)) {
         exec->vtx.inputs[16] = exec->vtx.inputs[0];
         exec->vtx.attrsz[16] = exec->vtx.attrsz[0];
         exec->vtx.attrptr[16] = exec->vtx.attrptr[0];
         exec->vtx.attrsz[0] = 0;
      }
      break;
   default:
      assert(0);
   }

   /* Make all active attributes (including edgeflag) available as
    * arrays of floats.
    */
   for (attr = 0; attr < VERT_ATTRIB_MAX ; attr++) {
      const GLuint src = map[attr];

      if (exec->vtx.attrsz[src]) {
         /* override the default array set above */
         ASSERT(attr < Elements(exec->vtx.inputs));
         ASSERT(attr < Elements(exec->vtx.arrays)); /* arrays[] */
         exec->vtx.inputs[attr] = &arrays[attr];

         if (_mesa_is_bufferobj(exec->vtx.bufferobj)) {
            /* a real buffer obj: Ptr is an offset, not a pointer*/
            GLsizeiptr offset;
            assert(exec->vtx.bufferobj->Pointer);  /* buf should be mapped */
            offset = (GLbyte *) data -
	       (GLbyte *) exec->vtx.bufferobj->Pointer +
	       exec->vtx.bufferobj->Offset;
            assert(offset >= 0);
            arrays[attr].Ptr = (void *) offset;
         }
         else {
            /* Ptr into ordinary app memory */
            arrays[attr].Ptr = (void *) data;
         }
	 arrays[attr].Size = exec->vtx.attrsz[src];
	 arrays[attr].StrideB = exec->vtx.vertex_size * sizeof(GLfloat);
	 arrays[attr].Stride = exec->vtx.vertex_size * sizeof(GLfloat);
	 arrays[attr].Type = GL_FLOAT;
         arrays[attr].Format = GL_RGBA;
	 arrays[attr].Enabled = 1;
         _mesa_reference_buffer_object(ctx,
                                       &arrays[attr].BufferObj,
                                       exec->vtx.bufferobj);
	 arrays[attr]._MaxElement = count; /* ??? */

	 data += exec->vtx.attrsz[src] * sizeof(GLfloat);
         varying_inputs |= 1 << attr;
      }
   }

   _mesa_set_varying_vp_inputs( ctx, varying_inputs );
}


static void
vbo_exec_vtx_unmap( struct vbo_exec_context *exec )
{
   GLenum target = GL_ARRAY_BUFFER_ARB;

   if (_mesa_is_bufferobj(exec->vtx.bufferobj)) {
      GLcontext *ctx = exec->ctx;
      
      if (ctx->Driver.FlushMappedBufferRange) {
         GLintptr offset = exec->vtx.buffer_used - exec->vtx.bufferobj->Offset;
         GLsizeiptr length = (exec->vtx.buffer_ptr - exec->vtx.buffer_map) * sizeof(float);

         if (length)
            ctx->Driver.FlushMappedBufferRange(ctx, target,
                                               offset, length,
                                               exec->vtx.bufferobj);
      }

      exec->vtx.buffer_used += (exec->vtx.buffer_ptr -
                                exec->vtx.buffer_map) * sizeof(float);

      assert(exec->vtx.buffer_used <= VBO_VERT_BUFFER_SIZE);
      assert(exec->vtx.buffer_ptr != NULL);
      
      ctx->Driver.UnmapBuffer(ctx, target, exec->vtx.bufferobj);
      exec->vtx.buffer_map = NULL;
      exec->vtx.buffer_ptr = NULL;
      exec->vtx.max_vert = 0;
   }
}


void
vbo_exec_vtx_map( struct vbo_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   const GLenum target = GL_ARRAY_BUFFER_ARB;
   const GLenum access = GL_READ_WRITE_ARB; /* for MapBuffer */
   const GLenum accessRange = GL_MAP_WRITE_BIT |  /* for MapBufferRange */
                              GL_MAP_INVALIDATE_RANGE_BIT |
                              GL_MAP_UNSYNCHRONIZED_BIT |
                              GL_MAP_FLUSH_EXPLICIT_BIT |
                              MESA_MAP_NOWAIT_BIT;
   const GLenum usage = GL_STREAM_DRAW_ARB;
   
   if (!_mesa_is_bufferobj(exec->vtx.bufferobj))
      return;

   if (exec->vtx.buffer_map != NULL) {
      assert(0);
      exec->vtx.buffer_map = NULL;
      exec->vtx.buffer_ptr = NULL;
   }

   if (VBO_VERT_BUFFER_SIZE > exec->vtx.buffer_used + 1024 &&
       ctx->Driver.MapBufferRange) {
      exec->vtx.buffer_map = 
         (GLfloat *)ctx->Driver.MapBufferRange(ctx, 
                                               target, 
                                               exec->vtx.buffer_used,
                                               (VBO_VERT_BUFFER_SIZE - 
                                                exec->vtx.buffer_used),
                                               accessRange,
                                               exec->vtx.bufferobj);
      exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   }
   
   if (!exec->vtx.buffer_map) {
      exec->vtx.buffer_used = 0;

      ctx->Driver.BufferData(ctx, target, 
                             VBO_VERT_BUFFER_SIZE, 
                             NULL, usage, exec->vtx.bufferobj);


      if (ctx->Driver.MapBufferRange)
         exec->vtx.buffer_map = 
            (GLfloat *)ctx->Driver.MapBufferRange(ctx, target,
                                                  0, VBO_VERT_BUFFER_SIZE,
                                                  accessRange,
                                                  exec->vtx.bufferobj);
      if (!exec->vtx.buffer_map)
         exec->vtx.buffer_map =
            (GLfloat *)ctx->Driver.MapBuffer(ctx, target, access, exec->vtx.bufferobj);
      assert(exec->vtx.buffer_map);
      exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   }

   if (0)
      printf("map %d..\n", exec->vtx.buffer_used);
}



/**
 * Execute the buffer and save copied verts.
 */
void
vbo_exec_vtx_flush( struct vbo_exec_context *exec, GLboolean unmap )
{
   if (0)
      vbo_exec_debug_verts( exec );

   if (exec->vtx.prim_count && 
       exec->vtx.vert_count) {

      exec->vtx.copied.nr = vbo_copy_vertices( exec ); 

      if (exec->vtx.copied.nr != exec->vtx.vert_count) {
	 GLcontext *ctx = exec->ctx;
	 
	 /* Before the update_state() as this may raise _NEW_ARRAY
          * from _mesa_set_varying_vp_inputs().
	  */
	 vbo_exec_bind_arrays( ctx );

         if (ctx->NewState)
            _mesa_update_state( ctx );

         if (_mesa_is_bufferobj(exec->vtx.bufferobj)) {
            vbo_exec_vtx_unmap( exec );
         }

         if (0)
            printf("%s %d %d\n", __FUNCTION__, exec->vtx.prim_count,
		   exec->vtx.vert_count);

	 vbo_context(ctx)->draw_prims( ctx, 
				       exec->vtx.inputs, 
				       exec->vtx.prim, 
				       exec->vtx.prim_count,
				       NULL,
				       GL_TRUE,
				       0,
				       exec->vtx.vert_count - 1);

	 /* If using a real VBO, get new storage -- unless asked not to.
          */
         if (_mesa_is_bufferobj(exec->vtx.bufferobj) && !unmap) {
            vbo_exec_vtx_map( exec );
         }
      }
   }

   /* May have to unmap explicitly if we didn't draw:
    */
   if (unmap && 
       _mesa_is_bufferobj(exec->vtx.bufferobj) &&
       exec->vtx.buffer_map) {
      vbo_exec_vtx_unmap( exec );
   }


   if (unmap || exec->vtx.vertex_size == 0)
      exec->vtx.max_vert = 0;
   else
      exec->vtx.max_vert = ((VBO_VERT_BUFFER_SIZE - exec->vtx.buffer_used) / 
                            (exec->vtx.vertex_size * sizeof(GLfloat)));

   exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   exec->vtx.prim_count = 0;
   exec->vtx.vert_count = 0;
}


#endif /* FEATURE_beginend */
