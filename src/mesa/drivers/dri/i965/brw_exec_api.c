/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

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
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "vtxfmt.h"
#include "dlist.h"
#include "state.h"
#include "light.h"
#include "api_arrayelt.h"
#include "api_noop.h"
#include "dispatch.h"

#include "brw_exec.h"

static void reset_attrfv( struct brw_exec_context *exec );


/* Close off the last primitive, execute the buffer, restart the
 * primitive.  
 */
static void brw_exec_wrap_buffers( struct brw_exec_context *exec )
{
   if (exec->vtx.prim_count == 0) {
      exec->vtx.copied.nr = 0;
      exec->vtx.vert_count = 0;
      exec->vtx.vbptr = (GLfloat *)exec->vtx.buffer_map;
   }
   else {
      GLuint last_begin = exec->vtx.prim[exec->vtx.prim_count-1].begin;
      GLuint last_count;

      if (exec->ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
	 GLint i = exec->vtx.prim_count - 1;
	 assert(i >= 0);
	 exec->vtx.prim[i].count = (exec->vtx.vert_count - 
				    exec->vtx.prim[i].start);
      }

      last_count = exec->vtx.prim[exec->vtx.prim_count-1].count;

      /* Execute the buffer and save copied vertices.
       */
      if (exec->vtx.vert_count)
	 brw_exec_vtx_flush( exec );
      else {
	 exec->vtx.prim_count = 0;
	 exec->vtx.copied.nr = 0;
      }

      /* Emit a glBegin to start the new list.
       */
      assert(exec->vtx.prim_count == 0);

      if (exec->ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
	 exec->vtx.prim[0].mode = exec->ctx->Driver.CurrentExecPrimitive;
	 exec->vtx.prim[0].start = 0;
	 exec->vtx.prim[0].count = 0;
	 exec->vtx.prim_count++;
      
	 if (exec->vtx.copied.nr == last_count)
	    exec->vtx.prim[0].begin = last_begin;
      }
   }
}


/* Deal with buffer wrapping where provoked by the vertex buffer
 * filling up, as opposed to upgrade_vertex().
 */
void brw_exec_vtx_wrap( struct brw_exec_context *exec )
{
   GLfloat *data = exec->vtx.copied.buffer;
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   brw_exec_wrap_buffers( exec );
   
   /* Copy stored stored vertices to start of new list. 
    */
   assert(exec->vtx.max_vert - exec->vtx.vert_count > exec->vtx.copied.nr);

   for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
      _mesa_memcpy( exec->vtx.vbptr, data, 
		    exec->vtx.vertex_size * sizeof(GLfloat));
      exec->vtx.vbptr += exec->vtx.vertex_size;
      data += exec->vtx.vertex_size;
      exec->vtx.vert_count++;
   }

   exec->vtx.copied.nr = 0;
}


/*
 * Copy the active vertex's values to the ctx->Current fields.
 */
static void brw_exec_copy_to_current( struct brw_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   GLuint i;

   for (i = BRW_ATTRIB_POS+1 ; i < BRW_ATTRIB_MAX ; i++) {
      if (exec->vtx.attrsz[i]) {
         /* Note: the exec->vtx.current[i] pointers point into the
          * ctx->Current.Attrib and ctx->Light.Material.Attrib arrays.
          */
	 COPY_CLEAN_4V(exec->vtx.current[i], 
		       exec->vtx.attrsz[i], 
		       exec->vtx.attrptr[i]);

	 /* This triggers rather too much recalculation of Mesa state
	  * that doesn't get used (eg light positions).
	  */
	 if (i >= BRW_ATTRIB_MAT_FRONT_AMBIENT &&
	     i <= BRW_ATTRIB_MAT_BACK_INDEXES)
	    ctx->NewState |= _NEW_LIGHT;
      }
   }

   /* color index is special (it's not a float[4] so COPY_CLEAN_4V above
    * will trash adjacent memory!)
    */
   if (exec->vtx.attrsz[BRW_ATTRIB_INDEX]) {
      ctx->Current.Index = exec->vtx.attrptr[BRW_ATTRIB_INDEX][0];
   }

   /* Edgeflag requires additional treatment:
    */
   if (exec->vtx.attrsz[BRW_ATTRIB_EDGEFLAG]) {
      ctx->Current.EdgeFlag = (exec->vtx.CurrentFloatEdgeFlag == 1.0);
   }

   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled &&
       exec->vtx.attrsz[BRW_ATTRIB_COLOR0]) {
      _mesa_update_color_material(ctx, 
				  ctx->Current.Attrib[BRW_ATTRIB_COLOR0]);
   }

   ctx->Driver.NeedFlush &= ~FLUSH_UPDATE_CURRENT;
}


static void brw_exec_copy_from_current( struct brw_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   GLint i;

   /* Edgeflag requires additional treatment:
    */
   exec->vtx.CurrentFloatEdgeFlag = 
      (GLfloat)ctx->Current.EdgeFlag;
   
   for (i = BRW_ATTRIB_POS+1 ; i < BRW_ATTRIB_MAX ; i++) 
      switch (exec->vtx.attrsz[i]) {
      case 4: exec->vtx.attrptr[i][3] = exec->vtx.current[i][3];
      case 3: exec->vtx.attrptr[i][2] = exec->vtx.current[i][2];
      case 2: exec->vtx.attrptr[i][1] = exec->vtx.current[i][1];
      case 1: exec->vtx.attrptr[i][0] = exec->vtx.current[i][0];
	 break;
      }

   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}


/* Flush existing data, set new attrib size, replay copied vertices.
 */ 
static void brw_exec_wrap_upgrade_vertex( struct brw_exec_context *exec,
					  GLuint attr,
					  GLuint newsz )
{
   GLcontext *ctx = exec->ctx;
   GLint lastcount = exec->vtx.vert_count;
   GLfloat *tmp;
   GLuint oldsz;
   GLuint i;

   /* Run pipeline on current vertices, copy wrapped vertices
    * to exec->vtx.copied.
    */
   brw_exec_wrap_buffers( exec );


   /* Do a COPY_TO_CURRENT to ensure back-copying works for the case
    * when the attribute already exists in the vertex and is having
    * its size increased.  
    */
   brw_exec_copy_to_current( exec );


   /* Heuristic: Attempt to isolate attributes received outside
    * begin/end so that they don't bloat the vertices.
    */
   if (ctx->Driver.CurrentExecPrimitive == PRIM_OUTSIDE_BEGIN_END &&
       exec->vtx.attrsz[attr] == 0 && 
       lastcount > 8 &&
       exec->vtx.vertex_size) {
      reset_attrfv( exec );
   }

   /* Fix up sizes:
    */
   oldsz = exec->vtx.attrsz[attr];
   exec->vtx.attrsz[attr] = newsz;

   exec->vtx.vertex_size += newsz - oldsz;
   exec->vtx.max_vert = BRW_VERT_BUFFER_SIZE / exec->vtx.vertex_size;
   exec->vtx.vert_count = 0;
   exec->vtx.vbptr = (GLfloat *)exec->vtx.buffer_map;
   

   /* Recalculate all the attrptr[] values
    */
   for (i = 0, tmp = exec->vtx.vertex ; i < BRW_ATTRIB_MAX ; i++) {
      if (exec->vtx.attrsz[i]) {
	 exec->vtx.attrptr[i] = tmp;
	 tmp += exec->vtx.attrsz[i];
      }
      else 
	 exec->vtx.attrptr[i] = NULL; /* will not be dereferenced */
   }

   /* Copy from current to repopulate the vertex with correct values.
    */
   brw_exec_copy_from_current( exec );

   /* Replay stored vertices to translate them
    * to new format here.
    *
    * -- No need to replay - just copy piecewise
    */
   if (exec->vtx.copied.nr)
   {
      GLfloat *data = exec->vtx.copied.buffer;
      GLfloat *dest = exec->vtx.vbptr;
      GLuint j;

      assert(exec->vtx.vbptr == (GLfloat *)exec->vtx.buffer_map);
      
      for (i = 0 ; i < exec->vtx.copied.nr ; i++) {
	 for (j = 0 ; j < BRW_ATTRIB_MAX ; j++) {
	    if (exec->vtx.attrsz[j]) {
	       if (j == attr) {
		  if (oldsz) {
		     COPY_CLEAN_4V( dest, oldsz, data );
		     data += oldsz;
		     dest += newsz;
		  } else {
		     COPY_SZ_4V( dest, newsz, exec->vtx.current[j] );
		     dest += newsz;
		  }
	       }
	       else {
		  GLuint sz = exec->vtx.attrsz[j];
		  COPY_SZ_4V( dest, sz, data );
		  dest += sz;
		  data += sz;
	       }
	    }
	 }
      }

      exec->vtx.vbptr = dest;
      exec->vtx.vert_count += exec->vtx.copied.nr;
      exec->vtx.copied.nr = 0;
   }
}


static void brw_exec_fixup_vertex( GLcontext *ctx,
				   GLuint attr, GLuint sz )
{
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
   int i;

   if (sz > exec->vtx.attrsz[attr]) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      brw_exec_wrap_upgrade_vertex( exec, attr, sz );
   }
   else if (sz < exec->vtx.active_sz[attr]) {
      static const GLfloat id[4] = { 0, 0, 0, 1 };

      /* New size is smaller - just need to fill in some
       * zeros.  Don't need to flush or wrap.
       */
      for (i = sz ; i <= exec->vtx.attrsz[attr] ; i++)
	 exec->vtx.attrptr[attr][i-1] = id[i-1];
   }

   exec->vtx.active_sz[attr] = sz;

   /* Does setting NeedFlush belong here?  Necessitates resetting
    * vtxfmt on each flush (otherwise flags won't get reset
    * afterwards).
    */
   if (attr == 0) 
      exec->ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   else 
      exec->ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}




/* 
 */
#define ATTR( A, N, V0, V1, V2, V3 )				\
do {								\
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;	\
								\
   if (exec->vtx.active_sz[A] != N)				\
      brw_exec_fixup_vertex(ctx, A, N);			\
								\
   {								\
      GLfloat *dest = exec->vtx.attrptr[A];			\
      if (N>0) dest[0] = V0;					\
      if (N>1) dest[1] = V1;					\
      if (N>2) dest[2] = V2;					\
      if (N>3) dest[3] = V3;					\
   }								\
								\
   if ((A) == 0) {						\
      GLuint i;							\
								\
      for (i = 0; i < exec->vtx.vertex_size; i++)		\
	 exec->vtx.vbptr[i] = exec->vtx.vertex[i];		\
								\
      exec->vtx.vbptr += exec->vtx.vertex_size;			\
      exec->ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;	\
								\
      if (++exec->vtx.vert_count >= exec->vtx.max_vert)		\
	 brw_exec_vtx_wrap( exec );				\
   }								\
} while (0)


#define ERROR() _mesa_error( ctx, GL_INVALID_ENUM, __FUNCTION__ )
#define TAG(x) brw_##x

#include "brw_attrib_tmp.h"





/* Eval
 */
static void GLAPIENTRY brw_exec_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 brw_exec_eval_update( exec );

      for (i = 0 ; i <= BRW_ATTRIB_INDEX ; i++) {
	 if (exec->eval.map1[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map1[i].sz)
	       brw_exec_fixup_vertex( ctx, i, exec->eval.map1[i].sz );
      }
   }


   _mesa_memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
                 exec->vtx.vertex_size * sizeof(GLfloat));

   brw_exec_do_EvalCoord1f( exec, u );

   _mesa_memcpy( exec->vtx.vertex, exec->vtx.copied.buffer,
                 exec->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY brw_exec_EvalCoord2f( GLfloat u, GLfloat v )
{
   GET_CURRENT_CONTEXT( ctx );
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 brw_exec_eval_update( exec );

      for (i = 0 ; i <= BRW_ATTRIB_INDEX ; i++) {
	 if (exec->eval.map2[i].map) 
	    if (exec->vtx.active_sz[i] != exec->eval.map2[i].sz)
	       brw_exec_fixup_vertex( ctx, i, exec->eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal) 
	 if (exec->vtx.active_sz[BRW_ATTRIB_NORMAL] != 3)
	    brw_exec_fixup_vertex( ctx, BRW_ATTRIB_NORMAL, 3 );
   }

   _mesa_memcpy( exec->vtx.copied.buffer, exec->vtx.vertex, 
                 exec->vtx.vertex_size * sizeof(GLfloat));

   brw_exec_do_EvalCoord2f( exec, u, v );

   _mesa_memcpy( exec->vtx.vertex, exec->vtx.copied.buffer, 
                 exec->vtx.vertex_size * sizeof(GLfloat));
}

static void GLAPIENTRY brw_exec_EvalCoord1fv( const GLfloat *u )
{
   brw_exec_EvalCoord1f( u[0] );
}

static void GLAPIENTRY brw_exec_EvalCoord2fv( const GLfloat *u )
{
   brw_exec_EvalCoord2f( u[0], u[1] );
}

static void GLAPIENTRY brw_exec_EvalPoint1( GLint i )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid1u2 - ctx->Eval.MapGrid1u1) /
		 (GLfloat) ctx->Eval.MapGrid1un);
   GLfloat u = i * du + ctx->Eval.MapGrid1u1;

   brw_exec_EvalCoord1f( u );
}


static void GLAPIENTRY brw_exec_EvalPoint2( GLint i, GLint j )
{
   GET_CURRENT_CONTEXT( ctx );
   GLfloat du = ((ctx->Eval.MapGrid2u2 - ctx->Eval.MapGrid2u1) / 
		 (GLfloat) ctx->Eval.MapGrid2un);
   GLfloat dv = ((ctx->Eval.MapGrid2v2 - ctx->Eval.MapGrid2v1) / 
		 (GLfloat) ctx->Eval.MapGrid2vn);
   GLfloat u = i * du + ctx->Eval.MapGrid2u1;
   GLfloat v = j * dv + ctx->Eval.MapGrid2v1;

   brw_exec_EvalCoord2f( u, v );
}


/* Build a list of primitives on the fly.  Keep
 * ctx->Driver.CurrentExecPrimitive uptodate as well.
 */
static void GLAPIENTRY brw_exec_Begin( GLenum mode )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive == GL_POLYGON+1) {
      struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
      int i;

      if (ctx->NewState) {
	 _mesa_update_state( ctx );

         if ((ctx->VertexProgram.Enabled && !ctx->VertexProgram._Enabled) ||
            (ctx->FragmentProgram.Enabled && !ctx->FragmentProgram._Enabled)) {
            _mesa_error(ctx, GL_INVALID_OPERATION,
                        "glBegin (invalid vertex/fragment program)");
            return;
         }

	 CALL_Begin(ctx->Exec, (mode));
	 return;
      }

      /* Heuristic: attempt to isolate attributes occuring outside
       * begin/end pairs.
       */
      if (exec->vtx.vertex_size && !exec->vtx.attrsz[0]) 
	 brw_exec_FlushVertices( ctx, ~0 );

      i = exec->vtx.prim_count++;
      exec->vtx.prim[i].mode = mode;
      exec->vtx.prim[i].begin = 1;
      exec->vtx.prim[i].end = 0;
      exec->vtx.prim[i].indexed = 0;
      exec->vtx.prim[i].weak = 0;
      exec->vtx.prim[i].pad = 0;
      exec->vtx.prim[i].start = exec->vtx.vert_count;
      exec->vtx.prim[i].count = 0;

      ctx->Driver.CurrentExecPrimitive = mode;
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glBegin" );
      
}

static void GLAPIENTRY brw_exec_End( void )
{
   GET_CURRENT_CONTEXT( ctx ); 

   if (ctx->Driver.CurrentExecPrimitive != GL_POLYGON+1) {
      struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
      int idx = exec->vtx.vert_count;
      int i = exec->vtx.prim_count - 1;

      exec->vtx.prim[i].end = 1; 
      exec->vtx.prim[i].count = idx - exec->vtx.prim[i].start;

      ctx->Driver.CurrentExecPrimitive = GL_POLYGON+1;

      if (exec->vtx.prim_count == BRW_MAX_PRIM)
	 brw_exec_vtx_flush( exec );	
   }
   else 
      _mesa_error( ctx, GL_INVALID_OPERATION, "glEnd" );
}


static void brw_exec_vtxfmt_init( struct brw_exec_context *exec )
{
   GLvertexformat *vfmt = &exec->vtxfmt;

   vfmt->ArrayElement = _ae_loopback_array_elt;	        /* generic helper */
   vfmt->Begin = brw_exec_Begin;
   vfmt->CallList = _mesa_CallList;
   vfmt->CallLists = _mesa_CallLists;
   vfmt->End = brw_exec_End;
   vfmt->EvalCoord1f = brw_exec_EvalCoord1f;
   vfmt->EvalCoord1fv = brw_exec_EvalCoord1fv;
   vfmt->EvalCoord2f = brw_exec_EvalCoord2f;
   vfmt->EvalCoord2fv = brw_exec_EvalCoord2fv;
   vfmt->EvalPoint1 = brw_exec_EvalPoint1;
   vfmt->EvalPoint2 = brw_exec_EvalPoint2;

   vfmt->Rectf = _mesa_noop_Rectf;
   vfmt->EvalMesh1 = _mesa_noop_EvalMesh1;
   vfmt->EvalMesh2 = _mesa_noop_EvalMesh2;


   /* from attrib_tmp.h:
    */
   vfmt->Color3f = brw_Color3f;
   vfmt->Color3fv = brw_Color3fv;
   vfmt->Color4f = brw_Color4f;
   vfmt->Color4fv = brw_Color4fv;
   vfmt->FogCoordfEXT = brw_FogCoordfEXT;
   vfmt->FogCoordfvEXT = brw_FogCoordfvEXT;
   vfmt->MultiTexCoord1fARB = brw_MultiTexCoord1f;
   vfmt->MultiTexCoord1fvARB = brw_MultiTexCoord1fv;
   vfmt->MultiTexCoord2fARB = brw_MultiTexCoord2f;
   vfmt->MultiTexCoord2fvARB = brw_MultiTexCoord2fv;
   vfmt->MultiTexCoord3fARB = brw_MultiTexCoord3f;
   vfmt->MultiTexCoord3fvARB = brw_MultiTexCoord3fv;
   vfmt->MultiTexCoord4fARB = brw_MultiTexCoord4f;
   vfmt->MultiTexCoord4fvARB = brw_MultiTexCoord4fv;
   vfmt->Normal3f = brw_Normal3f;
   vfmt->Normal3fv = brw_Normal3fv;
   vfmt->SecondaryColor3fEXT = brw_SecondaryColor3fEXT;
   vfmt->SecondaryColor3fvEXT = brw_SecondaryColor3fvEXT;
   vfmt->TexCoord1f = brw_TexCoord1f;
   vfmt->TexCoord1fv = brw_TexCoord1fv;
   vfmt->TexCoord2f = brw_TexCoord2f;
   vfmt->TexCoord2fv = brw_TexCoord2fv;
   vfmt->TexCoord3f = brw_TexCoord3f;
   vfmt->TexCoord3fv = brw_TexCoord3fv;
   vfmt->TexCoord4f = brw_TexCoord4f;
   vfmt->TexCoord4fv = brw_TexCoord4fv;
   vfmt->Vertex2f = brw_Vertex2f;
   vfmt->Vertex2fv = brw_Vertex2fv;
   vfmt->Vertex3f = brw_Vertex3f;
   vfmt->Vertex3fv = brw_Vertex3fv;
   vfmt->Vertex4f = brw_Vertex4f;
   vfmt->Vertex4fv = brw_Vertex4fv;
   
   vfmt->VertexAttrib1fARB = brw_VertexAttrib1fARB;
   vfmt->VertexAttrib1fvARB = brw_VertexAttrib1fvARB;
   vfmt->VertexAttrib2fARB = brw_VertexAttrib2fARB;
   vfmt->VertexAttrib2fvARB = brw_VertexAttrib2fvARB;
   vfmt->VertexAttrib3fARB = brw_VertexAttrib3fARB;
   vfmt->VertexAttrib3fvARB = brw_VertexAttrib3fvARB;
   vfmt->VertexAttrib4fARB = brw_VertexAttrib4fARB;
   vfmt->VertexAttrib4fvARB = brw_VertexAttrib4fvARB;

   vfmt->VertexAttrib1fNV = brw_VertexAttrib1fNV;
   vfmt->VertexAttrib1fvNV = brw_VertexAttrib1fvNV;
   vfmt->VertexAttrib2fNV = brw_VertexAttrib2fNV;
   vfmt->VertexAttrib2fvNV = brw_VertexAttrib2fvNV;
   vfmt->VertexAttrib3fNV = brw_VertexAttrib3fNV;
   vfmt->VertexAttrib3fvNV = brw_VertexAttrib3fvNV;
   vfmt->VertexAttrib4fNV = brw_VertexAttrib4fNV;
   vfmt->VertexAttrib4fvNV = brw_VertexAttrib4fvNV;

   vfmt->Materialfv = brw_Materialfv;

   vfmt->EdgeFlag = brw_EdgeFlag;
   vfmt->Indexf = brw_Indexf;
   vfmt->Indexfv = brw_Indexfv;

}


static void brw_exec_current_init( struct brw_exec_context *exec ) 
{
   GLcontext *ctx = exec->ctx;
   GLint i;

   /* setup the pointers for the typical 16 vertex attributes */
   for (i = 0; i < BRW_ATTRIB_FIRST_MATERIAL; i++) 
      exec->vtx.current[i] = ctx->Current.Attrib[i];

   /* setup pointers for the 12 material attributes */
   for (i = 0; i < MAT_ATTRIB_MAX; i++)
      exec->vtx.current[BRW_ATTRIB_FIRST_MATERIAL + i] = 
	 ctx->Light.Material.Attrib[i];

   exec->vtx.current[BRW_ATTRIB_INDEX] = &ctx->Current.Index;
   exec->vtx.current[BRW_ATTRIB_EDGEFLAG] = &exec->vtx.CurrentFloatEdgeFlag;
}

void brw_exec_vtx_init( struct brw_exec_context *exec )
{
   GLcontext *ctx = exec->ctx;
   GLuint i;

   /* Allocate a buffer object.  Will just reuse this object
    * continuously.
    */
   exec->vtx.bufferobj = ctx->Array.NullBufferObj;
   exec->vtx.buffer_map = ALIGN_MALLOC(BRW_VERT_BUFFER_SIZE * sizeof(GLfloat), 64);

   brw_exec_current_init( exec );
   brw_exec_vtxfmt_init( exec );

   /* Hook our functions into the dispatch table.
    */
   _mesa_install_exec_vtxfmt( exec->ctx, &exec->vtxfmt );

   for (i = 0 ; i < BRW_ATTRIB_MAX ; i++) {
      exec->vtx.attrsz[i] = 0;
      exec->vtx.active_sz[i] = 0;
      exec->vtx.inputs[i] = &exec->vtx.arrays[i];
   }
 
   exec->vtx.vertex_size = 0;
}


void brw_exec_vtx_destroy( struct brw_exec_context *exec )
{
   if (exec->vtx.buffer_map) {
      ALIGN_FREE(exec->vtx.buffer_map);
      exec->vtx.buffer_map = NULL;
   }
}


void brw_exec_FlushVertices( GLcontext *ctx, GLuint flags )
{
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   if (exec->ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END)
      return;

   if (exec->vtx.vert_count) {
      brw_exec_vtx_flush( exec );
   }

   if (exec->vtx.vertex_size) {
      brw_exec_copy_to_current( exec );
      reset_attrfv( exec );
   }

   exec->ctx->Driver.NeedFlush = 0;
}


static void reset_attrfv( struct brw_exec_context *exec )
{   
   GLuint i;

   for (i = 0 ; i < BRW_ATTRIB_MAX ; i++) {
      exec->vtx.attrsz[i] = 0;
      exec->vtx.active_sz[i] = 0;
   }

   exec->vtx.vertex_size = 0;
}
      
