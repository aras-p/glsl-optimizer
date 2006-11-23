/**************************************************************************
 * 
 * Copyright 2005 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "swrast_setup/swrast_setup.h"
#include "swrast/swrast.h"
#include "tnl/tnl.h"
#include "context.h"
#include "brw_context.h"
#include "brw_exec.h"
#include "brw_save.h"
#include "brw_fallback.h"

#include "glheader.h"
#include "enums.h"
#include "glapi.h"
#include "imports.h"
#include "macros.h"
#include "mtypes.h"
#include "dispatch.h"


typedef void (*attr_func)( GLcontext *ctx, GLint target, const GLfloat * );


/* Wrapper functions in case glVertexAttrib*fvNV doesn't exist */
static void VertexAttrib1fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib1fvNV(ctx->Exec, (target, v));
}

static void VertexAttrib2fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib2fvNV(ctx->Exec, (target, v));
}

static void VertexAttrib3fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib3fvNV(ctx->Exec, (target, v));
}

static void VertexAttrib4fvNV(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib4fvNV(ctx->Exec, (target, v));
}

static attr_func vert_attrfunc[4] = {
   VertexAttrib1fvNV,
   VertexAttrib2fvNV,
   VertexAttrib3fvNV,
   VertexAttrib4fvNV
};

#if 0
static void VertexAttrib1fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib1fvARB(ctx->Exec, (target, v));
}

static void VertexAttrib2fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib2fvARB(ctx->Exec, (target, v));
}

static void VertexAttrib3fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib3fvARB(ctx->Exec, (target, v));
}

static void VertexAttrib4fvARB(GLcontext *ctx, GLint target, const GLfloat *v)
{
   CALL_VertexAttrib4fvARB(ctx->Exec, (target, v));
}


static attr_func vert_attrfunc_arb[4] = {
   VertexAttrib1fvARB,
   VertexAttrib2fvARB,
   VertexAttrib3fvARB,
   VertexAttrib4fvARB
};
#endif






static void mat_attr1fv( GLcontext *ctx, GLint target, const GLfloat *v )
{
   switch (target) {
   case BRW_ATTRIB_MAT_FRONT_SHININESS:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_SHININESS, v ));
      break;
   case BRW_ATTRIB_MAT_BACK_SHININESS:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_SHININESS, v ));
      break;
   }
}


static void mat_attr3fv( GLcontext *ctx, GLint target, const GLfloat *v )
{
   switch (target) {
   case BRW_ATTRIB_MAT_FRONT_INDEXES:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_COLOR_INDEXES, v ));
      break;
   case BRW_ATTRIB_MAT_BACK_INDEXES:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_COLOR_INDEXES, v ));
      break;
   }
}


static void mat_attr4fv( GLcontext *ctx, GLint target, const GLfloat *v )
{
   switch (target) {
   case BRW_ATTRIB_MAT_FRONT_EMISSION:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_EMISSION, v ));
      break;
   case BRW_ATTRIB_MAT_BACK_EMISSION:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_EMISSION, v ));
      break;
   case BRW_ATTRIB_MAT_FRONT_AMBIENT:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_AMBIENT, v ));
      break;
   case BRW_ATTRIB_MAT_BACK_AMBIENT:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_AMBIENT, v ));
      break;
   case BRW_ATTRIB_MAT_FRONT_DIFFUSE:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_DIFFUSE, v ));
      break;
   case BRW_ATTRIB_MAT_BACK_DIFFUSE:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_DIFFUSE, v ));
      break;
   case BRW_ATTRIB_MAT_FRONT_SPECULAR:
      CALL_Materialfv(ctx->Exec, ( GL_FRONT, GL_SPECULAR, v ));
      break;
   case BRW_ATTRIB_MAT_BACK_SPECULAR:
      CALL_Materialfv(ctx->Exec, ( GL_BACK, GL_SPECULAR, v ));
      break;
   }
}


static attr_func mat_attrfunc[4] = {
   mat_attr1fv,
   NULL,
   mat_attr3fv,
   mat_attr4fv
};


static void index_attr1fv(GLcontext *ctx, GLint target, const GLfloat *v)
{
   (void) target;
   CALL_Indexf(ctx->Exec, (v[0]));
}

static void edgeflag_attr1fv(GLcontext *ctx, GLint target, const GLfloat *v)
{
   (void) target;
   CALL_EdgeFlag(ctx->Exec, ((GLboolean)(v[0] == 1.0)));
}

struct loopback_attr {
   GLint target;
   GLint sz;
   attr_func func;
};

/* Don't emit ends and begins on wrapped primitives.  Don't replay
 * wrapped vertices.  If we get here, it's probably because the the
 * precalculated wrapping is wrong.
 */
static void loopback_prim( GLcontext *ctx,
			   const GLfloat *buffer,
			   const struct brw_draw_prim *prim,
			   GLuint wrap_count,
			   GLuint vertex_size,
			   const struct loopback_attr *la, GLuint nr )
{
   GLint start = prim->start;
   GLint end = start + prim->count;
   const GLfloat *data;
   GLint j;
   GLuint k;

   if (0)
      _mesa_printf("loopback prim %s(%s,%s) verts %d..%d\n",
		   _mesa_lookup_enum_by_nr(prim->mode),
		   prim->begin ? "begin" : "..",
		   prim->end ? "end" : "..",
		   start, 
		   end);

   if (prim->begin) {
      CALL_Begin(GET_DISPATCH(), ( prim->mode ));
   }
   else {
      assert(start == 0);
      start += wrap_count;
   }

   data = buffer + start * vertex_size;

   for (j = start ; j < end ; j++) {
      const GLfloat *tmp = data + la[0].sz;

      for (k = 1 ; k < nr ; k++) {
	 la[k].func( ctx, la[k].target, tmp );
	 tmp += la[k].sz;
      }
	 
      /* Fire the vertex
       */
      la[0].func( ctx, BRW_ATTRIB_POS, data );
      data = tmp;
   }

   if (prim->end) {
      CALL_End(GET_DISPATCH(), ());
   }
}

/* Primitives generated by DrawArrays/DrawElements/Rectf may be
 * caught here.  If there is no primitive in progress, execute them
 * normally, otherwise need to track and discard the generated
 * primitives.
 */
static void loopback_weak_prim( GLcontext *ctx,
				const struct brw_draw_prim *prim )
{
   /* Use the prim_weak flag to ensure that if this primitive
    * wraps, we don't mistake future vertex_lists for part of the
    * surrounding primitive.
    *
    * While this flag is set, we are simply disposing of data
    * generated by an operation now known to be a noop.
    */
   if (prim->begin)
      ctx->Driver.CurrentExecPrimitive |= BRW_SAVE_PRIM_WEAK;
   if (prim->end)
      ctx->Driver.CurrentExecPrimitive &= ~BRW_SAVE_PRIM_WEAK;
}


void brw_loopback_vertex_list( GLcontext *ctx,
			       const GLfloat *buffer,
			       const GLubyte *attrsz,
			       const struct brw_draw_prim *prim,
			       GLuint prim_count,
			       GLuint wrap_count,
			       GLuint vertex_size)
{
   struct loopback_attr la[BRW_ATTRIB_MAX];
   GLuint i, nr = 0;

   for (i = 0 ; i <= BRW_ATTRIB_TEX7 ; i++) {
      if (i == BRW_ATTRIB_INDEX || i == BRW_ATTRIB_EDGEFLAG)
                   continue;

      if (attrsz[i]) {
	 la[nr].target = i;
	 la[nr].sz = attrsz[i];
	 la[nr].func = vert_attrfunc[attrsz[i]-1];
	 nr++;
      }
   }

   for (i = BRW_ATTRIB_MAT_FRONT_AMBIENT ; 
	i <= BRW_ATTRIB_MAT_BACK_INDEXES ; 
	i++) {
      if (attrsz[i]) {
	 la[nr].target = i;
	 la[nr].sz = attrsz[i];
	 la[nr].func = mat_attrfunc[attrsz[i]-1];
	 nr++;
      }
   }

   if (attrsz[BRW_ATTRIB_EDGEFLAG]) {
      la[nr].target = BRW_ATTRIB_EDGEFLAG;
      la[nr].sz = attrsz[BRW_ATTRIB_EDGEFLAG];
      la[nr].func = edgeflag_attr1fv;
      nr++;
   }

   if (attrsz[BRW_ATTRIB_INDEX]) {
      la[nr].target = BRW_ATTRIB_INDEX;
      la[nr].sz = attrsz[BRW_ATTRIB_INDEX];
      la[nr].func = index_attr1fv;
      nr++;
   }

   /* XXX ARB vertex attribs */

   for (i = 0 ; i < prim_count ; i++) {
      if ((prim[i].mode & BRW_SAVE_PRIM_WEAK) &&
	  (ctx->Driver.CurrentExecPrimitive != PRIM_OUTSIDE_BEGIN_END))
      {
	 loopback_weak_prim( ctx, &prim[i] );
      }
      else
      {
	 loopback_prim( ctx, buffer, &prim[i], wrap_count, vertex_size, la, nr );
      }
   }
}








static GLboolean do_check_fallback(struct brw_context *brw)
{
   GLcontext *ctx = &brw->intel.ctx;
   GLuint i;
   
   /* BRW_NEW_METAOPS
    */
   if (brw->metaops.active)
      return GL_FALSE;

   if (brw->intel.no_rast)
      return GL_TRUE;
   
   /* _NEW_BUFFERS
    */
   if (ctx->DrawBuffer->_ColorDrawBufferMask[0] != BUFFER_BIT_FRONT_LEFT &&
       ctx->DrawBuffer->_ColorDrawBufferMask[0] != BUFFER_BIT_BACK_LEFT)
      return GL_TRUE;

   /* _NEW_RENDERMODE
    *
    * XXX: need to save/restore RenderMode in metaops state, or
    * somehow move to a new attribs pointer:
    */
   if (ctx->RenderMode != GL_RENDER)
      return GL_TRUE;

   /* _NEW_TEXTURE:
    */
   for (i = 0; i < BRW_MAX_TEX_UNIT; i++) {
      struct gl_texture_unit *texUnit = &brw->attribs.Texture->Unit[i];
      if (texUnit->_ReallyEnabled) {
	 struct intel_texture_object *intelObj = intel_texture_object(texUnit->_Current);
	 struct gl_texture_image *texImage = intelObj->base.Image[0][intelObj->firstLevel];
	 if (texImage->Border)
	    return GL_TRUE;
      }
   }
   
   /* _NEW_STENCIL 
    */
   if (brw->attribs.Stencil->Enabled && 
       !brw->intel.hw_stencil) {
      return GL_TRUE;
   }


   return GL_FALSE;
}

static void check_fallback(struct brw_context *brw)
{
   brw->intel.Fallback = do_check_fallback(brw);
}

const struct brw_tracked_state brw_check_fallback = {
   .dirty = {
      .mesa = _NEW_BUFFERS | _NEW_RENDERMODE | _NEW_TEXTURE | _NEW_STENCIL,
      .brw  = BRW_NEW_METAOPS,
      .cache = 0
   },
   .update = check_fallback
};




/* If there is a fallback, fallback to software rasterization and
 * transformation together.  There is never a requirement to have
 * software t&l but hardware rasterization.
 * 
 * Further, all fallbacks are based on GL state, not on eg. primitive
 * or vertex data.
 */

static void do_fallback( struct brw_context *brw,
			 GLboolean fallback )
{
   GLcontext *ctx = &brw->intel.ctx;

   /* flush:
    */
   ctx->Driver.Flush( ctx );

   if (fallback) {
      _swsetup_Wakeup( ctx );
      _tnl_wakeup_exec( ctx );	

      /* Need this because tnl_wakeup_exec does too much: 
       */
      brw_save_wakeup(ctx);
      brw_save_fallback(ctx, GL_TRUE);
   }
   else {
      /* Flush vertices and copy-to-current:
       */
      FLUSH_CURRENT(ctx, 0); 

      _swrast_flush( ctx );

      brw_exec_wakeup(ctx);

      /* Need this because tnl_wakeup_exec does too much: 
       */
      brw_save_wakeup(ctx);
      brw_save_fallback(ctx, GL_FALSE);	 
   }
}


void brw_fallback( GLcontext *ctx )
{
   struct brw_context *brw = brw_context(ctx);
   do_fallback(brw, 1);
}


void brw_unfallback( GLcontext *ctx )
{
   struct brw_context *brw = brw_context(ctx);
   do_fallback(brw, 0);
}

/* Not used:
 */
void intelFallback( struct intel_context *intel, GLuint bit, GLboolean mode )
{
}



