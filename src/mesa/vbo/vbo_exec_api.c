/**************************************************************************

Copyright 2002-2008 VMware, Inc.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
VMWARE AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keithw@vmware.com>
 */

#include "main/glheader.h"
#include "main/bufferobj.h"
#include "main/context.h"
#include "main/macros.h"
#include "main/vtxfmt.h"
#include "main/dlist.h"
#include "main/eval.h"
#include "main/state.h"
#include "main/light.h"
#include "main/api_arrayelt.h"
#include "main/api_validate.h"
#include "main/dispatch.h"

#include "vbo_context.h"
#include "vbo_noop.h"


#ifdef ERROR
#undef ERROR
#endif


/** ID/name for immediate-mode VBO */
#define IMM_BUFFER_NAME 0xaabbccdd


static void reset_attrfv( struct vbo_exec_context *exec );


/**
 * Close off the last primitive, execute the buffer, restart the
 * primitive.  
 */
static void vbo_exec_wrap_buffers( struct vbo_exec_context *exec )
{
   if (exec->vtx.prim_count == 0) {
      exec->vtx.copied.nr = 0;
      exec->vtx.vert_count = 0;
      exec->vtx.buffer_ptr = exec->vtx.buffer_map;
   }
   else {
      GLuint last_begin = exec->vtx.prim[exec->vtx.prim_count-1].begin;
      GLuint last_count;

      if (_mesa_inside_begin_end(exec->ctx)) {
	 GLint i = exec->vtx.prim_count - 1;
	 assert(i >= 0);
	 exec->vtx.prim[i].count = (exec->vtx.vert_count - 
				    exec->vtx.prim[i].start);
      }

      last_count = exec->vtx.prim[exec->vtx.prim_count-1].count;

      /* Execute the buffer and save copied vertices.
       */
      if (exec->vtx.vert_count)
	 vbo_exec_vtx_flush( exec, GL_FALSE );
      else {
	 exec->vtx.prim_count = 0;
	 exec->vtx.copied.nr = 0;
      }

      /* Emit a glBegin to start the new list.
       */
      assert(exec->vtx.prim_count == 0);

      if (_mesa_inside_begin_end(exec->ctx)) {
	 exec->vtx.prim[0].mode = exec->ctx->Driver.CurrentExecPrimitive;
	 exec->vtx.prim[0].start = 0;
	 exec->vtx.prim[0].count = 0;
	 exec->vtx.prim_count++;
      
	 if (exec->vtx.copied.nr == last_count)
	    exec->vtx.prim[0].begin = last_begin;
      }
   }
}


/**
 * Deal with buffer wrapping where provoked by the vertex buffer
 * filling up, as opposed to upgrade_vertex().
 */
void vbo_exec_vtx_wrap( struct vbo_exec_context *exec )
{
   GLfloat *data = exec->vtx.copied.buffer;
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   vbo_exec_wrap_buffers( exec );
   
   if (!exec->vtx.buffer_ptr) {
      /* probably ran out of memory earlier when allocating the VBO */
      return;
   }

   /* Copy stored stored vertices to start of new list. 
    */
   assert(exec->vtx.max_vert - exec->vtx.vert_count > exec->vtx.copied.nr);

   for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
      memcpy( exec->vtx.buffer_ptr, data, 
	      exec->vtx.vertex_size * sizeof(GLfloat));
      exec->vtx.buffer_ptr += exec->vtx.vertex_size;
      data += exec->vtx.vertex_size;
      exec->vtx.vert_count++;
   }

   exec->vtx.copied.nr = 0;
}


/**
 * Copy the active vertex's values to the ctx->Current fields.
 */
static void vbo_exec_copy_to_current( struct vbo_exec_context *exec )
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLuint i;

   for (i = VBO_ATTRIB_POS+1 ; i < VBO_ATTRIB_MAX ; i++) {
      if (exec->vtx.attrsz[i]) {
         /* Note: the exec->vtx.current[i] pointers point into the
          * ctx->Current.Attrib and ctx->Light.Material.Attrib arrays.
          */
	 GLfloat *current = (GLfloat *)vbo->currval[i].Ptr;
         GLfloat tmp[4];

         COPY_CLEAN_4V_TYPE_AS_FLOAT(tmp,
                                     exec->vtx.attrsz[i],
                                     exec->vtx.attrptr[i],
                                     exec->vtx.attrtype[i]);
         
         if (exec->vtx.attrtype[i] != vbo->currval[i].Type ||
             memcmp(current, tmp, sizeof(tmp)) != 0) {
            memcpy(current, tmp, sizeof(tmp));
	 
            /* Given that we explicitly state size here, there is no need
             * for the COPY_CLEAN above, could just copy 16 bytes and be
             * done.  The only problem is when Mesa accesses ctx->Current
             * directly.
             */
            vbo->currval[i].Size = exec->vtx.attrsz[i];
            vbo->currval[i]._ElementSize = vbo->currval[i].Size * sizeof(GLfloat);
            vbo->currval[i].Type = exec->vtx.attrtype[i];
            vbo->currval[i].Integer =
                  vbo_attrtype_to_integer_flag(exec->vtx.attrtype[i]);

            /* This triggers rather too much recalculation of Mesa state
             * that doesn't get used (eg light positions).
             */
            if (i >= VBO_ATTRIB_MAT_FRONT_AMBIENT &&
                i <= VBO_ATTRIB_MAT_BACK_INDEXES)
               ctx->NewState |= _NEW_LIGHT;
            
            ctx->NewState |= _NEW_CURRENT_ATTRIB;
         }
      }
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled &&
       exec->vtx.attrsz[VBO_ATTRIB_COLOR0]) {
      _mesa_update_color_material(ctx, 
				  ctx->Current.Attrib[VBO_ATTRIB_COLOR0]);
   }
}


/**
 * Copy current vertex attribute values into the current vertex.
 */
static void
vbo_exec_copy_from_current(struct vbo_exec_context *exec)
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLint i;

   for (i = VBO_ATTRIB_POS + 1; i < VBO_ATTRIB_MAX; i++) {
      const GLfloat *current = (GLfloat *) vbo->currval[i].Ptr;
      switch (exec->vtx.attrsz[i]) {
      case 4: exec->vtx.attrptr[i][3] = current[3];
      case 3: exec->vtx.attrptr[i][2] = current[2];
      case 2: exec->vtx.attrptr[i][1] = current[1];
      case 1: exec->vtx.attrptr[i][0] = current[0];
	 break;
      }
   }
}


/**
 * Flush existing data, set new attrib size, replay copied vertices.
 * This is called when we transition from a small vertex attribute size
 * to a larger one.  Ex: glTexCoord2f -> glTexCoord4f.
 * We need to go back over the previous 2-component texcoords and insert
 * zero and one values.
 */ 
static void
vbo_exec_wrap_upgrade_vertex(struct vbo_exec_context *exec,
                             GLuint attr, GLuint newSize )
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   const GLint lastcount = exec->vtx.vert_count;
   GLfloat *old_attrptr[VBO_ATTRIB_MAX];
   const GLuint old_vtx_size = exec->vtx.vertex_size; /* floats per vertex */
   const GLuint oldSize = exec->vtx.attrsz[attr];
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   vbo_exec_wrap_buffers( exec );

   if (unlikely(exec->vtx.copied.nr)) {
      /* We're in the middle of a primitive, keep the old vertex
       * format around to be able to translate the copied vertices to
       * the new format.
       */
      memcpy(old_attrptr, exec->vtx.attrptr, sizeof(old_attrptr));
   }

   if (unlikely(oldSize)) {
      /* Do a COPY_TO_CURRENT to ensure back-copying works for the
       * case when the attribute already exists in the vertex and is
       * having its size increased.
       */
      vbo_exec_copy_to_current( exec );
   }

   /* Heuristic: Attempt to isolate attributes received outside
    * begin/end so that they don't bloat the vertices.
    */
   if (!_mesa_inside_begin_end(ctx) &&
       !oldSize && lastcount > 8 && exec->vtx.vertex_size) {
      vbo_exec_copy_to_current( exec );
      reset_attrfv( exec );
   }

   /* Fix up sizes:
    */
   exec->vtx.attrsz[attr] = newSize;
   exec->vtx.vertex_size += newSize - oldSize;
   exec->vtx.max_vert = ((VBO_VERT_BUFFER_SIZE - exec->vtx.buffer_used) / 
                         (exec->vtx.vertex_size * sizeof(GLfloat)));
   exec->vtx.vert_count = 0;
   exec->vtx.buffer_ptr = exec->vtx.buffer_map;

   if (unlikely(oldSize)) {
      /* Size changed, recalculate all the attrptr[] values
       */
      GLfloat *tmp = exec->vtx.vertex;

      for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
	 if (exec->vtx.attrsz[i]) {
	    exec->vtx.attrptr[i] = tmp;
	    tmp += exec->vtx.attrsz[i];
	 }
	 else
	    exec->vtx.attrptr[i] = NULL; /* will not be dereferenced */
      }

      /* Copy from current to repopulate the vertex with correct
       * values.
       */
      vbo_exec_copy_from_current( exec );
   }
   else {
      /* Just have to append the new attribute at the end */
      exec->vtx.attrptr[attr] = exec->vtx.vertex +
	 exec->vtx.vertex_size - newSize;
   }

   /* Replay stored vertices to translate them
    * to new format here.
    *
    * -- No need to replay - just copy piecewise
    */
   if (unlikely(exec->vtx.copied.nr)) {
      GLfloat *data = exec->vtx.copied.buffer;
      GLfloat *dest = exec->vtx.buffer_ptr;
      GLuint j;

      assert(exec->vtx.buffer_ptr == exec->vtx.buffer_map);

      for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
	 for (j = 0 ; j < VBO_ATTRIB_MAX ; j++) {
	    GLuint sz = exec->vtx.attrsz[j];

	    if (sz) {
	       GLint old_offset = old_attrptr[j] - exec->vtx.vertex;
	       GLint new_offset = exec->vtx.attrptr[j] - exec->vtx.vertex;

	       if (j == attr) {
		  if (oldSize) {
		     GLfloat tmp[4];
                     COPY_CLEAN_4V_TYPE_AS_FLOAT(tmp, oldSize,
                                                 data + old_offset,
                                                 exec->vtx.attrtype[j]);
		     COPY_SZ_4V(dest + new_offset, newSize, tmp);
		  } else {
		     GLfloat *current = (GLfloat *)vbo->currval[j].Ptr;
		     COPY_SZ_4V(dest + new_offset, sz, current);
		  }
	       }
	       else {
		  COPY_SZ_4V(dest + new_offset, sz, data + old_offset);
	       }
	    }
	 }

	 data += old_vtx_size;
	 dest += exec->vtx.vertex_size;
      }

      exec->vtx.buffer_ptr = dest;
      exec->vtx.vert_count += exec->vtx.copied.nr;
      exec->vtx.copied.nr = 0;
   }
}


/**
 * This is when a vertex attribute transitions to a different size.
 * For example, we saw a bunch of glTexCoord2f() calls and now we got a
 * glTexCoord4f() call.  We promote the array from size=2 to size=4.
 */
static void
vbo_exec_fixup_vertex(struct gl_context *ctx, GLuint attr, GLuint newSize)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (newSize > exec->vtx.attrsz[attr]) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      vbo_exec_wrap_upgrade_vertex( exec, attr, newSize );
   }
   else if (newSize < exec->vtx.active_sz[attr]) {
      GLuint i;
      const GLfloat *id =
            vbo_get_default_vals_as_float(exec->vtx.attrtype[attr]);

      /* New size is smaller - just need to fill in some
       * zeros.  Don't need to flush or wrap.
       */
      for (i = newSize; i <= exec->vtx.attrsz[attr]; i++)
	 exec->vtx.attrptr[attr][i-1] = id[i-1];
   }

   exec->vtx.active_sz[attr] = newSize;

   /* Does setting NeedFlush belong here?  Necessitates resetting
    * vtxfmt on each flush (otherwise flags won't get reset
    * afterwards).
    */
   if (attr == 0) 
      ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
}


/**
 * This macro is used to implement all the glVertex, glColor, glTexCoord,
 * glVertexAttrib, etc functions.
 */
#define ATTR( A, N, T, V0, V1, V2, V3 )					\
do {									\
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;		\
									\
   if (unlikely(!(ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT)))	\
      ctx->Driver.BeginVertices( ctx );					\
   									\
   if (unlikely(exec->vtx.active_sz[A] != N))				\
      vbo_exec_fixup_vertex(ctx, A, N);					\
   									\
   {									\
      GLfloat *dest = exec->vtx.attrptr[A];				\
      if (N>0) dest[0] = V0;						\
      if (N>1) dest[1] = V1;						\
      if (N>2) dest[2] = V2;						\
      if (N>3) dest[3] = V3;						\
      exec->vtx.attrtype[A] = T;                                        \
   }									\
									\
   if ((A) == 0) {							\
      /* This is a glVertex call */					\
      GLuint i;								\
									\
      for (i = 0; i < exec->vtx.vertex_size; i++)			\
	 exec->vtx.buffer_ptr[i] = exec->vtx.vertex[i];			\
									\
      exec->vtx.buffer_ptr += exec->vtx.vertex_size;			\
									\
      /* Set FLUSH_STORED_VERTICES to indicate that there's now */	\
      /* something to draw (not just updating a color or texcoord).*/	\
      ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;			\
									\
      if (++exec->vtx.vert_count >= exec->vtx.max_vert)			\
	 vbo_exec_vtx_wrap( exec );					\
   }									\
} while (0)


#define ERROR(err) _mesa_error( ctx, err, __FUNCTION__ )
#define TAG(x) vbo_##x

#include "vbo_attrib_tmp.h"



/**
 * Execute a glMaterial call.  Note that if GL_COLOR_MATERIAL is enabled,
 * this may be a (partial) no-op.
 */
static void GLAPIENTRY
vbo_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
   GLbitfield updateMats;
   GET_CURRENT_CONTEXT(ctx);

   /* This function should be a no-op when it tries to update material
    * attributes which are currently tracking glColor via glColorMaterial.
    * The updateMats var will be a mask of the MAT_BIT_FRONT/BACK_x bits
    * indicating which material attributes can actually be updated below.
    */
   if (ctx->Light.ColorMaterialEnabled) {
      updateMats = ~ctx->Light._ColorMaterialBitmask;
   }
   else {
      /* GL_COLOR_MATERIAL is disabled so don't skip any material updates */
      updateMats = ALL_MATERIAL_BITS;
   }

   if (ctx->API == API_OPENGL_COMPAT && face == GL_FRONT) {
      updateMats &= FRONT_MATERIAL_BITS;
   }
   else if (ctx->API == API_OPENGL_COMPAT && face == GL_BACK) {
      updateMats &= BACK_MATERIAL_BITS;
   }
   else if (face != GL_FRONT_AND_BACK) {
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterial(invalid face)");
      return;
   }

   switch (pname) {
   case GL_EMISSION:
      if (updateMats & MAT_BIT_FRONT_EMISSION)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_EMISSION, 4, params);
      if (updateMats & MAT_BIT_BACK_EMISSION)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_EMISSION, 4, params);
      break;
   case GL_AMBIENT:
      if (updateMats & MAT_BIT_FRONT_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, params);
      if (updateMats & MAT_BIT_BACK_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_AMBIENT, 4, params);
      break;
   case GL_DIFFUSE:
      if (updateMats & MAT_BIT_FRONT_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, params);
      if (updateMats & MAT_BIT_BACK_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_DIFFUSE, 4, params);
      break;
   case GL_SPECULAR:
      if (updateMats & MAT_BIT_FRONT_SPECULAR)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_SPECULAR, 4, params);
      if (updateMats & MAT_BIT_BACK_SPECULAR)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_SPECULAR, 4, params);
      break;
   case GL_SHININESS:
      if (*params < 0 || *params > ctx->Const.MaxShininess) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glMaterial(invalid shininess: %f out range [0, %f])",
		     *params, ctx->Const.MaxShininess);
         return;
      }
      if (updateMats & MAT_BIT_FRONT_SHININESS)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_SHININESS, 1, params);
      if (updateMats & MAT_BIT_BACK_SHININESS)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_SHININESS, 1, params);
      break;
   case GL_COLOR_INDEXES:
      if (ctx->API != API_OPENGL_COMPAT) {
         _mesa_error(ctx, GL_INVALID_ENUM, "glMaterialfv(pname)");
         return;
      }
      if (updateMats & MAT_BIT_FRONT_INDEXES)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_INDEXES, 3, params);
      if (updateMats & MAT_BIT_BACK_INDEXES)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_INDEXES, 3, params);
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      if (updateMats & MAT_BIT_FRONT_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_AMBIENT, 4, params);
      if (updateMats & MAT_BIT_FRONT_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_FRONT_DIFFUSE, 4, params);
      if (updateMats & MAT_BIT_BACK_AMBIENT)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_AMBIENT, 4, params);
      if (updateMats & MAT_BIT_BACK_DIFFUSE)
         MAT_ATTR(VBO_ATTRIB_MAT_BACK_DIFFUSE, 4, params);
      break;
   default:
      _mesa_error(ctx, GL_INVALID_ENUM, "glMaterialfv(pname)");
      return;
   }
}


/**
 * Flush (draw) vertices.
 * \param  unmap - leave VBO unmapped after flushing?
 */
static void
vbo_exec_FlushVertices_internal(struct vbo_exec_context *exec, GLboolean unmap)
{
   if (exec->vtx.vert_count || unmap) {
      vbo_exec_vtx_flush( exec, unmap );
   }

   if (exec->vtx.vertex_size) {
      vbo_exec_copy_to_current( exec );
      reset_attrfv( exec );
   }
}


static void GLAPIENTRY vbo_exec_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 vbo_exec_eval_update( exec );

      for (i = 0; i <= VBO_ATTRIB_TEX7; i++) {
	 if (exec->eval.map1[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map1[i].sz)
	       vbo_exec_fixup_vertex( ctx, i, exec->eval.map1[i].sz );
      }
   }


   memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
           exec->vtx.vertex_size * sizeof(GLfloat));

   vbo_exec_do_EvalCoord1f( exec, u );

   memcpy( exec->vtx.vertex, exec->vtx.copied.buffer,
           exec->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY vbo_exec_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CURRENT_CONTEXT( ctx );
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 vbo_exec_eval_update( exec );

      for (i = 0; i <= VBO_ATTRIB_TEX7; i++) {
	 if (exec->eval.map2[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map2[i].sz)
	       vbo_exec_fixup_vertex( ctx, i, exec->eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal) 
	 if (exec->vtx.active_sz[VBO_ATTRIB_NORMAL] != 3)
	    vbo_exec_fixup_vertex( ctx, VBO_ATTRIB_NORMAL, 3 );
   }

   memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
           exec->vtx.vertex_size * sizeof(GLfloat));

   vbo_exec_do_EvalCoord2f( exec, u, v );

   memcpy( exec->vtx.vertex, exec->vtx.copied.buffer, 
           exec->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY vbo_exec_EvalCoord1fv( const GLfloat *u )
{
   vbo_exec_EvalCoord1f( u[0] );
}

static void GLAPIENTRY vbo_exec_EvalCoord2fv( const GLfloat *u )
{
   vbo_exec_EvalCoord2f( u[0], u[1] );
}

static void GLAPIENTRY vbo_exec_EvalPoint1( GLint i )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1) /
		 (GLfloat) ctx->Eval.MapGrid1un);
   GLfloat u = i * du + ctx->Eval.MapGrid1u1;

   vbo_exec_EvalCoord1f( u );
}


static void GLAPIENTRY vbo_exec_EvalPoint2( GLint i, GLint j )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1) / 
		 (GLfloat) ctx->Eval.MapGrid2un);
   GLfloat dv = ((ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1) / 
		 (GLfloat) ctx->Eval.MapGrid2vn);
   GLfloat u = i * du + ctx->Eval.MapGrid2u1;
   GLfloat v = j * dv + ctx->Eval.MapGrid2v1;

   vbo_exec_EvalCoord2f( u, v );
}


/**
 * Called via glBegin.
 */
static void GLAPIENTRY vbo_exec_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   int i;

   if (_mesa_inside_begin_end(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glBegin");
      return;
   }

   if (!_mesa_valid_prim_mode(ctx, mode, "glBegin")) {
      return;
   }

   vbo_draw_method(vbo_context(ctx), DRAW_BEGIN_END);

   if (ctx->NewState) {
      _mesa_update_state( ctx );

      CALL_Begin(ctx->Exec, (mode));
      return;
   }

   if (!_mesa_valid_to_render(ctx, "glBegin")) {
      return;
   }

   /* Heuristic: attempt to isolate attributes occuring outside
    * begin/end pairs.
    */
   if (exec->vtx.vertex_size && !exec->vtx.attrsz[0])
      vbo_exec_FlushVertices_internal(exec, GL_FALSE);

   i = exec->vtx.prim_count++;
   exec->vtx.prim[i].mode = mode;
   exec->vtx.prim[i].begin = 1;
   exec->vtx.prim[i].end = 0;
   exec->vtx.prim[i].indexed = 0;
   exec->vtx.prim[i].weak = 0;
   exec->vtx.prim[i].pad = 0;
   exec->vtx.prim[i].start = exec->vtx.vert_count;
   exec->vtx.prim[i].count = 0;
   exec->vtx.prim[i].num_instances = 1;
   exec->vtx.prim[i].base_instance = 0;
   exec->vtx.prim[i].is_indirect = 0;

   ctx->Driver.CurrentExecPrimitive = mode;

   ctx->Exec = ctx->BeginEnd;
   /* We may have been called from a display list, in which case we should
    * leave dlist.c's dispatch table in place.
    */
   if (ctx->CurrentDispatch == ctx->OutsideBeginEnd) {
      ctx->CurrentDispatch = ctx->BeginEnd;
      _glapi_set_dispatch(ctx->CurrentDispatch);
   } else {
      assert(ctx->CurrentDispatch == ctx->Save);
   }
}


/**
 * Try to merge / concatenate the two most recent VBO primitives.
 */
static void
try_vbo_merge(struct vbo_exec_context *exec)
{
   struct _mesa_prim *cur =  &exec->vtx.prim[exec->vtx.prim_count - 1];

   assert(exec->vtx.prim_count >= 1);

   vbo_try_prim_conversion(cur);

   if (exec->vtx.prim_count >= 2) {
      struct _mesa_prim *prev = &exec->vtx.prim[exec->vtx.prim_count - 2];
      assert(prev == cur - 1);

      if (vbo_can_merge_prims(prev, cur)) {
         assert(cur->begin);
         assert(cur->end);
         assert(prev->begin);
         assert(prev->end);
         vbo_merge_prims(prev, cur);
         exec->vtx.prim_count--;  /* drop the last primitive */
      }
   }
}


/**
 * Called via glEnd.
 */
static void GLAPIENTRY vbo_exec_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (!_mesa_inside_begin_end(ctx)) {
      _mesa_error(ctx, GL_INVALID_OPERATION, "glEnd");
      return;
   }

   ctx->Exec = ctx->OutsideBeginEnd;
   if (ctx->CurrentDispatch == ctx->BeginEnd) {
      ctx->CurrentDispatch = ctx->OutsideBeginEnd;
      _glapi_set_dispatch(ctx->CurrentDispatch);
   }

   if (exec->vtx.prim_count > 0) {
      /* close off current primitive */
      int idx = exec->vtx.vert_count;
      int i = exec->vtx.prim_count - 1;

      exec->vtx.prim[i].end = 1;
      exec->vtx.prim[i].count = idx - exec->vtx.prim[i].start;

      try_vbo_merge(exec);
   }

   ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;

   if (exec->vtx.prim_count == VBO_MAX_PRIM)
      vbo_exec_vtx_flush( exec, GL_FALSE );

   if (MESA_DEBUG_FLAGS & DEBUG_ALWAYS_FLUSH) {
      _mesa_flush(ctx);
   }
}


/**
 * Called via glPrimitiveRestartNV()
 */
static void GLAPIENTRY
vbo_exec_PrimitiveRestartNV(void)
{
   GLenum curPrim;
   GET_CURRENT_CONTEXT( ctx ); 

   curPrim = ctx->Driver.CurrentExecPrimitive;

   if (curPrim == PRIM_OUTSIDE_BEGIN_END) {
      _mesa_error( ctx, GL_INVALID_OPERATION, "glPrimitiveRestartNV" );
   }
   else {
      vbo_exec_End();
      vbo_exec_Begin(curPrim);
   }
}



static void vbo_exec_vtxfmt_init( struct vbo_exec_context *exec )
{
   struct gl_context *ctx = exec->ctx;
   GLvertexformat *vfmt = &exec->vtxfmt;

   vfmt->ArrayElement = _ae_ArrayElement;

   vfmt->Begin = vbo_exec_Begin;
   vfmt->End = vbo_exec_End;
   vfmt->PrimitiveRestartNV = vbo_exec_PrimitiveRestartNV;

   vfmt->CallList = _mesa_CallList;
   vfmt->CallLists = _mesa_CallLists;

   vfmt->EvalCoord1f = vbo_exec_EvalCoord1f;
   vfmt->EvalCoord1fv = vbo_exec_EvalCoord1fv;
   vfmt->EvalCoord2f = vbo_exec_EvalCoord2f;
   vfmt->EvalCoord2fv = vbo_exec_EvalCoord2fv;
   vfmt->EvalPoint1 = vbo_exec_EvalPoint1;
   vfmt->EvalPoint2 = vbo_exec_EvalPoint2;

   /* from attrib_tmp.h:
    */
   vfmt->Color3f = vbo_Color3f;
   vfmt->Color3fv = vbo_Color3fv;
   vfmt->Color4f = vbo_Color4f;
   vfmt->Color4fv = vbo_Color4fv;
   vfmt->FogCoordfEXT = vbo_FogCoordfEXT;
   vfmt->FogCoordfvEXT = vbo_FogCoordfvEXT;
   vfmt->MultiTexCoord1fARB = vbo_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = vbo_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = vbo_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = vbo_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = vbo_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = vbo_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = vbo_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = vbo_MultiTexCoord4fv;
   vfmt->Normal3f = vbo_Normal3f;
   vfmt->Normal3fv = vbo_Normal3fv;
   vfmt->SecondaryColor3fEXT = vbo_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = vbo_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = vbo_TexCoord1f;
   vfmt->TexCoord1fv = vbo_TexCoord1fv;
   vfmt->TexCoord2f = vbo_TexCoord2f;
   vfmt->TexCoord2fv = vbo_TexCoord2fv;
   vfmt->TexCoord3f = vbo_TexCoord3f;
   vfmt->TexCoord3fv = vbo_TexCoord3fv;
   vfmt->TexCoord4f = vbo_TexCoord4f;
   vfmt->TexCoord4fv = vbo_TexCoord4fv;
   vfmt->Vertex2f = vbo_Vertex2f;
   vfmt->Vertex2fv = vbo_Vertex2fv;
   vfmt->Vertex3f = vbo_Vertex3f;
   vfmt->Vertex3fv = vbo_Vertex3fv;
   vfmt->Vertex4f = vbo_Vertex4f;
   vfmt->Vertex4fv = vbo_Vertex4fv;
   
   if (ctx->API == API_OPENGLES2) {
      vfmt->VertexAttrib1fARB = _es_VertexAttrib1f;
      vfmt->VertexAttrib1fvARB = _es_VertexAttrib1fv;
      vfmt->VertexAttrib2fARB = _es_VertexAttrib2f;
      vfmt->VertexAttrib2fvARB = _es_VertexAttrib2fv;
      vfmt->VertexAttrib3fARB = _es_VertexAttrib3f;
      vfmt->VertexAttrib3fvARB = _es_VertexAttrib3fv;
      vfmt->VertexAttrib4fARB = _es_VertexAttrib4f;
      vfmt->VertexAttrib4fvARB = _es_VertexAttrib4fv;
   } else {
      vfmt->VertexAttrib1fARB = vbo_VertexAttrib1fARB;
      vfmt->VertexAttrib1fvARB = vbo_VertexAttrib1fvARB;
      vfmt->VertexAttrib2fARB = vbo_VertexAttrib2fARB;
      vfmt->VertexAttrib2fvARB = vbo_VertexAttrib2fvARB;
      vfmt->VertexAttrib3fARB = vbo_VertexAttrib3fARB;
      vfmt->VertexAttrib3fvARB = vbo_VertexAttrib3fvARB;
      vfmt->VertexAttrib4fARB = vbo_VertexAttrib4fARB;
      vfmt->VertexAttrib4fvARB = vbo_VertexAttrib4fvARB;
   }

   /* Note that VertexAttrib4fNV is used from dlist.c and api_arrayelt.c so
    * they can have a single entrypoint for updating any of the legacy
    * attribs.
    */
   vfmt->VertexAttrib1fNV = vbo_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = vbo_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = vbo_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = vbo_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = vbo_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = vbo_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = vbo_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = vbo_VertexAttrib4fvNV;

   /* integer-valued */
   vfmt->VertexAttribI1i = vbo_VertexAttribI1i;
   vfmt->VertexAttribI2i = vbo_VertexAttribI2i;
   vfmt->VertexAttribI3i = vbo_VertexAttribI3i;
   vfmt->VertexAttribI4i = vbo_VertexAttribI4i;
   vfmt->VertexAttribI2iv = vbo_VertexAttribI2iv;
   vfmt->VertexAttribI3iv = vbo_VertexAttribI3iv;
   vfmt->VertexAttribI4iv = vbo_VertexAttribI4iv;

   /* unsigned integer-valued */
   vfmt->VertexAttribI1ui = vbo_VertexAttribI1ui;
   vfmt->VertexAttribI2ui = vbo_VertexAttribI2ui;
   vfmt->VertexAttribI3ui = vbo_VertexAttribI3ui;
   vfmt->VertexAttribI4ui = vbo_VertexAttribI4ui;
   vfmt->VertexAttribI2uiv = vbo_VertexAttribI2uiv;
   vfmt->VertexAttribI3uiv = vbo_VertexAttribI3uiv;
   vfmt->VertexAttribI4uiv = vbo_VertexAttribI4uiv;

   vfmt->Materialfv = vbo_Materialfv;

   vfmt->EdgeFlag = vbo_EdgeFlag;
   vfmt->Indexf = vbo_Indexf;
   vfmt->Indexfv = vbo_Indexfv;

   /* ARB_vertex_type_2_10_10_10_rev */
   vfmt->VertexP2ui = vbo_VertexP2ui;
   vfmt->VertexP2uiv = vbo_VertexP2uiv;
   vfmt->VertexP3ui = vbo_VertexP3ui;
   vfmt->VertexP3uiv = vbo_VertexP3uiv;
   vfmt->VertexP4ui = vbo_VertexP4ui;
   vfmt->VertexP4uiv = vbo_VertexP4uiv;

   vfmt->TexCoordP1ui = vbo_TexCoordP1ui;
   vfmt->TexCoordP1uiv = vbo_TexCoordP1uiv;
   vfmt->TexCoordP2ui = vbo_TexCoordP2ui;
   vfmt->TexCoordP2uiv = vbo_TexCoordP2uiv;
   vfmt->TexCoordP3ui = vbo_TexCoordP3ui;
   vfmt->TexCoordP3uiv = vbo_TexCoordP3uiv;
   vfmt->TexCoordP4ui = vbo_TexCoordP4ui;
   vfmt->TexCoordP4uiv = vbo_TexCoordP4uiv;

   vfmt->MultiTexCoordP1ui = vbo_MultiTexCoordP1ui;
   vfmt->MultiTexCoordP1uiv = vbo_MultiTexCoordP1uiv;
   vfmt->MultiTexCoordP2ui = vbo_MultiTexCoordP2ui;
   vfmt->MultiTexCoordP2uiv = vbo_MultiTexCoordP2uiv;
   vfmt->MultiTexCoordP3ui = vbo_MultiTexCoordP3ui;
   vfmt->MultiTexCoordP3uiv = vbo_MultiTexCoordP3uiv;
   vfmt->MultiTexCoordP4ui = vbo_MultiTexCoordP4ui;
   vfmt->MultiTexCoordP4uiv = vbo_MultiTexCoordP4uiv;
   
   vfmt->NormalP3ui = vbo_NormalP3ui;
   vfmt->NormalP3uiv = vbo_NormalP3uiv;

   vfmt->ColorP3ui = vbo_ColorP3ui;
   vfmt->ColorP3uiv = vbo_ColorP3uiv;
   vfmt->ColorP4ui = vbo_ColorP4ui;
   vfmt->ColorP4uiv = vbo_ColorP4uiv;

   vfmt->SecondaryColorP3ui = vbo_SecondaryColorP3ui;
   vfmt->SecondaryColorP3uiv = vbo_SecondaryColorP3uiv;

   vfmt->VertexAttribP1ui = vbo_VertexAttribP1ui;
   vfmt->VertexAttribP1uiv = vbo_VertexAttribP1uiv;
   vfmt->VertexAttribP2ui = vbo_VertexAttribP2ui;
   vfmt->VertexAttribP2uiv = vbo_VertexAttribP2uiv;
   vfmt->VertexAttribP3ui = vbo_VertexAttribP3ui;
   vfmt->VertexAttribP3uiv = vbo_VertexAttribP3uiv;
   vfmt->VertexAttribP4ui = vbo_VertexAttribP4ui;
   vfmt->VertexAttribP4uiv = vbo_VertexAttribP4uiv;
}


/**
 * Tell the VBO module to use a real OpenGL vertex buffer object to
 * store accumulated immediate-mode vertex data.
 * This replaces the malloced buffer which was created in
 * vb_exec_vtx_init() below.
 */
void vbo_use_buffer_objects(struct gl_context *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   /* Any buffer name but 0 can be used here since this bufferobj won't
    * go into the bufferobj hashtable.
    */
   GLuint bufName = IMM_BUFFER_NAME;
   GLenum target = GL_ARRAY_BUFFER_ARB;
   GLenum usage = GL_STREAM_DRAW_ARB;
   GLsizei size = VBO_VERT_BUFFER_SIZE;

   /* Make sure this func is only used once */
   assert(exec->vtx.bufferobj == ctx->Shared->NullBufferObj);

   _mesa_align_free(exec->vtx.buffer_map);
   exec->vtx.buffer_map = NULL;
   exec->vtx.buffer_ptr = NULL;

   /* Allocate a real buffer object now */
   _mesa_reference_buffer_object(ctx, &exec->vtx.bufferobj, NULL);
   exec->vtx.bufferobj = ctx->Driver.NewBufferObject(ctx, bufName, target);
   if (!ctx->Driver.BufferData(ctx, target, size, NULL, usage,
                               GL_MAP_WRITE_BIT |
                               GL_DYNAMIC_STORAGE_BIT |
                               GL_CLIENT_STORAGE_BIT,
                               exec->vtx.bufferobj)) {
      _mesa_error(ctx, GL_OUT_OF_MEMORY, "VBO allocation");
   }
}


/**
 * If this function is called, all VBO buffers will be unmapped when
 * we flush.
 * Otherwise, if a simple command like glColor3f() is called and we flush,
 * the current VBO may be left mapped.
 */
void
vbo_always_unmap_buffers(struct gl_context *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   exec->begin_vertices_flags |= FLUSH_STORED_VERTICES;
}


void vbo_exec_vtx_init( struct vbo_exec_context *exec )
{
   struct gl_context *ctx = exec->ctx;
   struct vbo_context *vbo = vbo_context(ctx);
   GLuint i;

   /* Allocate a buffer object.  Will just reuse this object
    * continuously, unless vbo_use_buffer_objects() is called to enable
    * use of real VBOs.
    */
   _mesa_reference_buffer_object(ctx,
                                 &exec->vtx.bufferobj,
                                 ctx->Shared->NullBufferObj);

   ASSERT(!exec->vtx.buffer_map);
   exec->vtx.buffer_map = _mesa_align_malloc(VBO_VERT_BUFFER_SIZE, 64);
   exec->vtx.buffer_ptr = exec->vtx.buffer_map;

   vbo_exec_vtxfmt_init( exec );
   _mesa_noop_vtxfmt_init(&exec->vtxfmt_noop);

   for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
      ASSERT(i < Elements(exec->vtx.attrsz));
      exec->vtx.attrsz[i] = 0;
      ASSERT(i < Elements(exec->vtx.attrtype));
      exec->vtx.attrtype[i] = GL_FLOAT;
      ASSERT(i < Elements(exec->vtx.active_sz));
      exec->vtx.active_sz[i] = 0;
   }
   for (i = 0 ; i < VERT_ATTRIB_MAX; i++) {
      ASSERT(i < Elements(exec->vtx.inputs));
      ASSERT(i < Elements(exec->vtx.arrays));
      exec->vtx.inputs[i] = &exec->vtx.arrays[i];
   }
   
   {
      struct gl_client_array *arrays = exec->vtx.arrays;
      unsigned i;

      memcpy(arrays, &vbo->currval[VBO_ATTRIB_POS],
             VERT_ATTRIB_FF_MAX * sizeof(arrays[0]));
      for (i = 0; i < VERT_ATTRIB_FF_MAX; ++i) {
         struct gl_client_array *array;
         array = &arrays[VERT_ATTRIB_FF(i)];
         array->BufferObj = NULL;
         _mesa_reference_buffer_object(ctx, &arrays->BufferObj,
                                 vbo->currval[VBO_ATTRIB_POS+i].BufferObj);
      }

      memcpy(arrays + VERT_ATTRIB_GENERIC(0),
             &vbo->currval[VBO_ATTRIB_GENERIC0],
             VERT_ATTRIB_GENERIC_MAX * sizeof(arrays[0]));

      for (i = 0; i < VERT_ATTRIB_GENERIC_MAX; ++i) {
         struct gl_client_array *array;
         array = &arrays[VERT_ATTRIB_GENERIC(i)];
         array->BufferObj = NULL;
         _mesa_reference_buffer_object(ctx, &array->BufferObj,
                           vbo->currval[VBO_ATTRIB_GENERIC0+i].BufferObj);
      }
   }

   exec->vtx.vertex_size = 0;

   exec->begin_vertices_flags = FLUSH_UPDATE_CURRENT;
}


void vbo_exec_vtx_destroy( struct vbo_exec_context *exec )
{
   /* using a real VBO for vertex data */
   struct gl_context *ctx = exec->ctx;
   unsigned i;

   /* True VBOs should already be unmapped
    */
   if (exec->vtx.buffer_map) {
      ASSERT(exec->vtx.bufferobj->Name == 0 ||
             exec->vtx.bufferobj->Name == IMM_BUFFER_NAME);
      if (exec->vtx.bufferobj->Name == 0) {
         _mesa_align_free(exec->vtx.buffer_map);
         exec->vtx.buffer_map = NULL;
         exec->vtx.buffer_ptr = NULL;
      }
   }

   /* Drop any outstanding reference to the vertex buffer
    */
   for (i = 0; i < Elements(exec->vtx.arrays); i++) {
      _mesa_reference_buffer_object(ctx,
                                    &exec->vtx.arrays[i].BufferObj,
                                    NULL);
   }

   /* Free the vertex buffer.  Unmap first if needed.
    */
   if (_mesa_bufferobj_mapped(exec->vtx.bufferobj, MAP_INTERNAL)) {
      ctx->Driver.UnmapBuffer(ctx, exec->vtx.bufferobj, MAP_INTERNAL);
   }
   _mesa_reference_buffer_object(ctx, &exec->vtx.bufferobj, NULL);
}


/**
 * Called upon first glVertex, glColor, glTexCoord, etc.
 */
void vbo_exec_BeginVertices( struct gl_context *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   vbo_exec_vtx_map( exec );

   assert((ctx->Driver.NeedFlush & FLUSH_UPDATE_CURRENT) == 0);
   assert(exec->begin_vertices_flags);

   ctx->Driver.NeedFlush |= exec->begin_vertices_flags;
}


/**
 * Called via ctx->Driver.FlushVertices()
 * \param flags  bitmask of FLUSH_STORED_VERTICES, FLUSH_UPDATE_CURRENT
 */
void vbo_exec_FlushVertices( struct gl_context *ctx, GLuint flags )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

#ifdef DEBUG
   /* debug check: make sure we don't get called recursively */
   exec->flush_call_depth++;
   assert(exec->flush_call_depth == 1);
#endif

   if (_mesa_inside_begin_end(ctx)) {
      /* We've had glBegin but not glEnd! */
#ifdef DEBUG
      exec->flush_call_depth--;
      assert(exec->flush_call_depth == 0);
#endif
      return;
   }

   /* Flush (draw), and make sure VBO is left unmapped when done */
   vbo_exec_FlushVertices_internal(exec, GL_TRUE);

   /* Need to do this to ensure BeginVertices gets called again:
    */
   ctx->Driver.NeedFlush &= ~(FLUSH_UPDATE_CURRENT | flags);

#ifdef DEBUG
   exec->flush_call_depth--;
   assert(exec->flush_call_depth == 0);
#endif
}


static void reset_attrfv( struct vbo_exec_context *exec )
{   
   GLuint i;

   for (i = 0 ; i < VBO_ATTRIB_MAX ; i++) {
      exec->vtx.attrsz[i] = 0;
      exec->vtx.attrtype[i] = GL_FLOAT;
      exec->vtx.active_sz[i] = 0;
   }

   exec->vtx.vertex_size = 0;
}
      

void GLAPIENTRY
_es_Color4f(GLfloat r, GLfloat g, GLfloat b, GLfloat a)
{
   vbo_Color4f(r, g, b, a);
}


void GLAPIENTRY
_es_Normal3f(GLfloat x, GLfloat y, GLfloat z)
{
   vbo_Normal3f(x, y, z);
}


void GLAPIENTRY
_es_MultiTexCoord4f(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
   vbo_MultiTexCoord4f(target, s, t, r, q);
}


void GLAPIENTRY
_es_Materialfv(GLenum face, GLenum pname, const GLfloat *params)
{
   vbo_Materialfv(face, pname, params);
}


void GLAPIENTRY
_es_Materialf(GLenum face, GLenum pname, GLfloat param)
{
   GLfloat p[4];
   p[0] = param;
   p[1] = p[2] = p[3] = 0.0F;
   vbo_Materialfv(face, pname, p);
}


/**
 * A special version of glVertexAttrib4f that does not treat index 0 as
 * VBO_ATTRIB_POS.
 */
static void
VertexAttrib4f_nopos(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   GET_CURRENT_CONTEXT(ctx);
   if (index < MAX_VERTEX_GENERIC_ATTRIBS)
      ATTR(VBO_ATTRIB_GENERIC0 + index, 4, GL_FLOAT, x, y, z, w);
   else
      ERROR(GL_INVALID_VALUE);
}

void GLAPIENTRY
_es_VertexAttrib4f(GLuint index, GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
   VertexAttrib4f_nopos(index, x, y, z, w);
}


void GLAPIENTRY
_es_VertexAttrib1f(GLuint indx, GLfloat x)
{
   VertexAttrib4f_nopos(indx, x, 0.0f, 0.0f, 1.0f);
}


void GLAPIENTRY
_es_VertexAttrib1fv(GLuint indx, const GLfloat* values)
{
   VertexAttrib4f_nopos(indx, values[0], 0.0f, 0.0f, 1.0f);
}


void GLAPIENTRY
_es_VertexAttrib2f(GLuint indx, GLfloat x, GLfloat y)
{
   VertexAttrib4f_nopos(indx, x, y, 0.0f, 1.0f);
}


void GLAPIENTRY
_es_VertexAttrib2fv(GLuint indx, const GLfloat* values)
{
   VertexAttrib4f_nopos(indx, values[0], values[1], 0.0f, 1.0f);
}


void GLAPIENTRY
_es_VertexAttrib3f(GLuint indx, GLfloat x, GLfloat y, GLfloat z)
{
   VertexAttrib4f_nopos(indx, x, y, z, 1.0f);
}


void GLAPIENTRY
_es_VertexAttrib3fv(GLuint indx, const GLfloat* values)
{
   VertexAttrib4f_nopos(indx, values[0], values[1], values[2], 1.0f);
}


void GLAPIENTRY
_es_VertexAttrib4fv(GLuint indx, const GLfloat* values)
{
   VertexAttrib4f_nopos(indx, values[0], values[1], values[2], values[3]);
}
