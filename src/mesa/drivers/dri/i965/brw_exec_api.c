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

static brw_attrfv_func choose[BRW_MAX_ATTR_CODEGEN+1][4]; /* +1 for ERROR_ATTRIB */
static brw_attrfv_func generic_attr_func[BRW_MAX_ATTR_CODEGEN][4];


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

   for (i = BRW_ATTRIB_POS+1 ; i < BRW_ATTRIB_INDEX ; i++) {
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

#if 1
   /* Colormaterial -- this kindof sucks.
    */
   if (ctx->Light.ColorMaterialEnabled &&
       exec->vtx.attrsz[VERT_ATTRIB_COLOR0]) {
      _mesa_update_color_material(ctx, 
				  ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
   }
#endif

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

   /* For codegen - attrptr's may have changed, so need to redo
    * codegen.  Might be a reasonable place to try & detect attributes
    * in the vertex which aren't being submitted any more.
    */
   for (i = 0 ; i < BRW_ATTRIB_MAX ; i++) 
      if (exec->vtx.attrsz[i]) {
	 GLuint j = exec->vtx.attrsz[i] - 1;

	 if (i < BRW_MAX_ATTR_CODEGEN)
	    exec->vtx.tabfv[i][j] = choose[i][j];
      }

}


static void brw_exec_fixup_vertex( struct brw_exec_context *exec,
				   GLuint attr, GLuint sz )
{
   static const GLfloat id[4] = { 0, 0, 0, 1 };
   int i;

   if (exec->vtx.attrsz[attr] < sz) {
      /* New size is larger.  Need to flush existing vertices and get
       * an enlarged vertex format.
       */
      brw_exec_wrap_upgrade_vertex( exec, attr, sz );
   }
   else if (exec->vtx.attrsz[attr] > sz) {
      /* New size is smaller - just need to fill in some
       * zeros.  Don't need to flush or wrap.
       */
      for (i = sz ; i <= exec->vtx.attrsz[attr] ; i++)
	 exec->vtx.attrptr[attr][i-1] = id[i-1];
   }

   /* Does setting NeedFlush belong here?  Necessitates resetting
    * vtxfmt on each flush (otherwise flags won't get reset
    * afterwards).
    */
   if (attr == 0) 
      exec->ctx->Driver.NeedFlush |= FLUSH_STORED_VERTICES;
   else 
      exec->ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;
}


/* Helper function for 'CHOOSE' macro.  Do what's necessary when an
 * entrypoint is called for the first time.
 */

static brw_attrfv_func do_choose( GLuint attr, GLuint sz )
{ 
   GET_CURRENT_CONTEXT( ctx ); 
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
   GLuint oldsz = exec->vtx.attrsz[attr];

   assert(attr < BRW_MAX_ATTR_CODEGEN);

   if (oldsz != sz) {
      /* Reset any active pointers for this attribute 
       */
      if (oldsz)
	 exec->vtx.tabfv[attr][oldsz-1] = choose[attr][oldsz-1];
   
      brw_exec_fixup_vertex( exec, attr, sz );
 
   }

   /* Codegen?
    */

   /* Else use generic version:
    */
   exec->vtx.tabfv[attr][sz-1] = generic_attr_func[attr][sz-1];

   return exec->vtx.tabfv[attr][sz-1];
}



#define CHOOSE( ATTR, N )				\
static void choose_##ATTR##_##N( const GLfloat *v )	\
{							\
   brw_attrfv_func f = do_choose(ATTR, N);			\
   f( v );						\
}

#define CHOOSERS( ATTRIB ) \
   CHOOSE( ATTRIB, 1 )				\
   CHOOSE( ATTRIB, 2 )				\
   CHOOSE( ATTRIB, 3 )				\
   CHOOSE( ATTRIB, 4 )				\


#define INIT_CHOOSERS(ATTR)				\
   choose[ATTR][0] = choose_##ATTR##_1;				\
   choose[ATTR][1] = choose_##ATTR##_2;				\
   choose[ATTR][2] = choose_##ATTR##_3;				\
   choose[ATTR][3] = choose_##ATTR##_4;

CHOOSERS( 0 )
CHOOSERS( 1 )
CHOOSERS( 2 )
CHOOSERS( 3 )
CHOOSERS( 4 )
CHOOSERS( 5 )
CHOOSERS( 6 )
CHOOSERS( 7 )
CHOOSERS( 8 )
CHOOSERS( 9 )
CHOOSERS( 10 )
CHOOSERS( 11 )
CHOOSERS( 12 )
CHOOSERS( 13 )
CHOOSERS( 14 )
CHOOSERS( 15 )

static void error_attrib( const GLfloat *unused )
{
   GET_CURRENT_CONTEXT( ctx );
   (void) unused;
   _mesa_error( ctx, GL_INVALID_ENUM, "glVertexAttrib" );
}   



static void reset_attrfv( struct brw_exec_context *exec )
{   
   GLuint i;

   for (i = 0 ; i < BRW_ATTRIB_MAX ; i++) 
      if (exec->vtx.attrsz[i]) {
	 GLint j = exec->vtx.attrsz[i] - 1;
	 exec->vtx.attrsz[i] = 0;

	 if (i < BRW_MAX_ATTR_CODEGEN) {
            while (j >= 0) {
	       exec->vtx.tabfv[i][j] = choose[i][j];
               j--;
            }
         }
      }

   exec->vtx.vertex_size = 0;
}
      


/* Materials:  
 * 
 * These are treated as per-vertex attributes, at indices above where
 * the NV_vertex_program leaves off.  There are a lot of good things
 * about treating materials this way.  
 *
 * However: I don't want to double the number of generated functions
 * just to cope with this, so I unroll the 'C' varients of CHOOSE and
 * ATTRF into this function, and dispense with codegen and
 * second-level dispatch.
 *
 * There is no aliasing of material attributes with other entrypoints.
 */
#define OTHER_ATTR( exec, A, N, params )		\
do {							\
   if (exec->vtx.attrsz[A] != N) {			\
      brw_exec_fixup_vertex( exec, A, N );			\
   }							\
							\
   {							\
      GLfloat *dest = exec->vtx.attrptr[A];		\
      if (N>0) dest[0] = (params)[0];			\
      if (N>1) dest[1] = (params)[1];			\
      if (N>2) dest[2] = (params)[2];			\
      if (N>3) dest[3] = (params)[3];			\
   }							\
} while (0)


#define MAT( exec, ATTR, N, face, params )			\
do {								\
   if (face != GL_BACK)						\
      OTHER_ATTR( exec, ATTR, N, params ); /* front */	\
   if (face != GL_FRONT)					\
      OTHER_ATTR( exec, ATTR + 1, N, params ); /* back */	\
} while (0)


/* Colormaterial is dealt with later on.
 */
static void GLAPIENTRY brw_exec_Materialfv( GLenum face, GLenum pname, 
			       const GLfloat *params )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   switch (face) {
   case GL_FRONT:
   case GL_BACK:
   case GL_FRONT_AND_BACK:
      break;
      
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glMaterialfv" );
      return;
   }

   switch (pname) {
   case GL_EMISSION:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_EMISSION, 4, face, params );
      break;
   case GL_AMBIENT:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      break;
   case GL_DIFFUSE:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   case GL_SPECULAR:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_SPECULAR, 4, face, params );
      break;
   case GL_SHININESS:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_SHININESS, 1, face, params );
      break;
   case GL_COLOR_INDEXES:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_INDEXES, 3, face, params );
      break;
   case GL_AMBIENT_AND_DIFFUSE:
      MAT( exec, BRW_ATTRIB_MAT_FRONT_AMBIENT, 4, face, params );
      MAT( exec, BRW_ATTRIB_MAT_FRONT_DIFFUSE, 4, face, params );
      break;
   default:
      _mesa_error( ctx, GL_INVALID_ENUM, "glMaterialfv" );
      return;
   }
}


static void GLAPIENTRY brw_exec_EdgeFlag( GLboolean b )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
   GLfloat f = (GLfloat)b;

   OTHER_ATTR( exec, BRW_ATTRIB_EDGEFLAG, 1, &f );
}

#if 0
static void GLAPIENTRY brw_exec_EdgeFlagv( const GLboolean *v )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;
   GLfloat f = (GLfloat)v[0];

   OTHER_ATTR( exec, BRW_ATTRIB_EDGEFLAG, 1, &f );
}
#endif

static void GLAPIENTRY brw_exec_Indexf( GLfloat f )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   OTHER_ATTR( exec, BRW_ATTRIB_INDEX, 1, &f );
}

static void GLAPIENTRY brw_exec_Indexfv( const GLfloat *v )
{
   GET_CURRENT_CONTEXT( ctx ); 
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   OTHER_ATTR( exec, BRW_ATTRIB_INDEX, 1, v );
}

/* Eval
 */
static void GLAPIENTRY brw_exec_EvalCoord1f( GLfloat u )
{
   GET_CURRENT_CONTEXT( ctx );
   struct brw_exec_context *exec = IMM_CONTEXT(ctx)->exec;

   /* TODO: use a CHOOSE() function for this: */
   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 brw_exec_eval_update( exec );

      for (i = 0 ; i <= BRW_ATTRIB_INDEX ; i++) {
	 if (exec->eval.map1[i].map) 
	    if (exec->vtx.attrsz[i] != exec->eval.map1[i].sz)
	       brw_exec_fixup_vertex( exec, i, exec->eval.map1[i].sz );
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

   /* TODO: use a CHOOSE() function for this: */
   {
      GLint i;
      if (exec->eval.recalculate_maps) 
	 brw_exec_eval_update( exec );

      for (i = 0 ; i <= BRW_ATTRIB_INDEX ; i++) {
	 if (exec->eval.map2[i].map) 
	    if (exec->vtx.attrsz[i] != exec->eval.map2[i].sz)
	       brw_exec_fixup_vertex( exec, i, exec->eval.map2[i].sz );
      }

      if (ctx->Eval.AutoNormal) 
	 if (exec->vtx.attrsz[BRW_ATTRIB_NORMAL] != 3)
	    brw_exec_fixup_vertex( exec, BRW_ATTRIB_NORMAL, 3 );
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
   vfmt->EdgeFlag = brw_exec_EdgeFlag;
/*    vfmt->EdgeFlagv = brw_exec_EdgeFlagv; */
   vfmt->End = brw_exec_End;
   vfmt->EvalCoord1f = brw_exec_EvalCoord1f;
   vfmt->EvalCoord1fv = brw_exec_EvalCoord1fv;
   vfmt->EvalCoord2f = brw_exec_EvalCoord2f;
   vfmt->EvalCoord2fv = brw_exec_EvalCoord2fv;
   vfmt->EvalPoint1 = brw_exec_EvalPoint1;
   vfmt->EvalPoint2 = brw_exec_EvalPoint2;
   vfmt->Indexf = brw_exec_Indexf;
   vfmt->Indexfv = brw_exec_Indexfv;
   vfmt->Materialfv = brw_exec_Materialfv;

   vfmt->Rectf = _mesa_noop_Rectf;
   vfmt->EvalMesh1 = _mesa_noop_EvalMesh1;
   vfmt->EvalMesh2 = _mesa_noop_EvalMesh2;
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
   static int firsttime = 1;
   
   if (firsttime) {
      firsttime = 0;

      INIT_CHOOSERS( 0 );
      INIT_CHOOSERS( 1 );
      INIT_CHOOSERS( 2 );
      INIT_CHOOSERS( 3 );
      INIT_CHOOSERS( 4 );
      INIT_CHOOSERS( 5 );
      INIT_CHOOSERS( 6 );
      INIT_CHOOSERS( 7 );
      INIT_CHOOSERS( 8 );
      INIT_CHOOSERS( 9 );
      INIT_CHOOSERS( 10 );
      INIT_CHOOSERS( 11 );
      INIT_CHOOSERS( 12 );
      INIT_CHOOSERS( 13 );
      INIT_CHOOSERS( 14 );
      INIT_CHOOSERS( 15 );

      choose[ERROR_ATTRIB][0] = error_attrib;
      choose[ERROR_ATTRIB][1] = error_attrib;
      choose[ERROR_ATTRIB][2] = error_attrib;
      choose[ERROR_ATTRIB][3] = error_attrib;

      brw_exec_generic_attr_table_init( generic_attr_func );
   }

   /* Allocate a buffer object.  Will just reuse this object
    * continuously.
    */
   exec->vtx.bufferobj = ctx->Array.NullBufferObj;
   exec->vtx.buffer_map = ALIGN_MALLOC(BRW_VERT_BUFFER_SIZE * sizeof(GLfloat), 64);

   brw_exec_current_init( exec );
   brw_exec_vtxfmt_init( exec );
   brw_exec_vtx_generic_init( exec );

   /* Hook our functions into the dispatch table.
    */
   _mesa_install_exec_vtxfmt( exec->ctx, &exec->vtxfmt );

   memcpy( exec->vtx.tabfv, choose, sizeof(choose) );

   for (i = 0 ; i < BRW_ATTRIB_MAX ; i++) {
      exec->vtx.attrsz[i] = 0;
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


