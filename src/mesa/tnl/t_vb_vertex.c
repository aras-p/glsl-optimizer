/* $Id: t_vb_vertex.c,v 1.9 2001/05/30 10:01:41 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keithw@valinux.com>
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"

#include "math/m_xform.h"

#include "t_context.h"
#include "t_pipeline.h"



struct vertex_stage_data {
   GLvector4f eye;
   GLvector4f clip;
   GLvector4f proj;
   GLubyte *clipmask;
   GLubyte ormask;
   GLubyte andmask;


   /* Need these because it's difficult to replay the sideeffects
    * analytically.
    */
   GLvector4f *save_eyeptr;
   GLvector4f *save_clipptr;
   GLvector4f *save_projptr;
};

#define VERTEX_STAGE_DATA(stage) ((struct vertex_stage_data *)stage->privatePtr)




/* This function implements cliptesting for user-defined clip planes.
 * The clipping of primitives to these planes is implemented in
 * t_render_clip.h.
 */
#define USER_CLIPTEST(NAME, SZ)					\
static void NAME( GLcontext *ctx,				\
		  GLvector4f *clip,				\
		  GLubyte *clipmask,				\
		  GLubyte *clipormask,				\
		  GLubyte *clipandmask )			\
{								\
   GLuint p;							\
								\
   for (p = 0; p < ctx->Const.MaxClipPlanes; p++)		\
      if (ctx->Transform.ClipEnabled[p]) {			\
	 GLuint nr, i;						\
	 const GLfloat a = ctx->Transform._ClipUserPlane[p][0];	\
	 const GLfloat b = ctx->Transform._ClipUserPlane[p][1];	\
	 const GLfloat c = ctx->Transform._ClipUserPlane[p][2];	\
	 const GLfloat d = ctx->Transform._ClipUserPlane[p][3];	\
         GLfloat *coord = (GLfloat *)clip->data;		\
         GLuint stride = clip->stride;				\
         GLuint count = clip->count;				\
								\
	 for (nr = 0, i = 0 ; i < count ; i++) {		\
	    GLfloat dp = coord[0] * a + coord[1] * b;		\
	    if (SZ > 2) dp += coord[2] * c;			\
	    if (SZ > 3) dp += coord[3] * d; else dp += d;	\
								\
	    if (dp < 0) {					\
	       nr++;						\
	       clipmask[i] |= CLIP_USER_BIT;			\
	    }							\
								\
	    STRIDE_F(coord, stride);				\
	 }							\
								\
	 if (nr > 0) {						\
	    *clipormask |= CLIP_USER_BIT;			\
	    if (nr == count) {					\
	       *clipandmask |= CLIP_USER_BIT;			\
	       return;						\
	    }							\
	 }							\
      }								\
}


USER_CLIPTEST(userclip2, 2)
USER_CLIPTEST(userclip3, 3)
USER_CLIPTEST(userclip4, 4)

static void (*(usercliptab[5]))( GLcontext *,
				 GLvector4f *, GLubyte *,
				 GLubyte *, GLubyte * ) =
{
   0,
   0,
   userclip2,
   userclip3,
   userclip4
};



static GLboolean run_vertex_stage( GLcontext *ctx,
				   struct gl_pipeline_stage *stage )
{
   struct vertex_stage_data *store = (struct vertex_stage_data *)stage->privatePtr;
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   if (stage->changed_inputs) {

      if (ctx->_NeedEyeCoords) {
	 /* Separate modelview and project transformations:
	  */
	 if (ctx->ModelView.type == MATRIX_IDENTITY)
	    VB->EyePtr = VB->ObjPtr;
	 else
	    VB->EyePtr = TransformRaw( &store->eye, &ctx->ModelView,
				       VB->ObjPtr);

	 if (ctx->ProjectionMatrix.type == MATRIX_IDENTITY)
	    VB->ClipPtr = VB->EyePtr;
	 else
	    VB->ClipPtr = TransformRaw( &store->clip, &ctx->ProjectionMatrix,
					VB->EyePtr );
      }
      else {
	 /* Combined modelviewproject transform:
	  */
	 if (ctx->_ModelProjectMatrix.type == MATRIX_IDENTITY)
	    VB->ClipPtr = VB->ObjPtr;
	 else
	    VB->ClipPtr = TransformRaw( &store->clip,
                                        &ctx->_ModelProjectMatrix,
					VB->ObjPtr );
      }

      /* Drivers expect this to be clean to element 4...
       */
      if (VB->ClipPtr->size < 4) {
	 if (VB->ClipPtr->flags & VEC_NOT_WRITEABLE) {
	    ASSERT(VB->ClipPtr == VB->ObjPtr);
	    VB->import_data( ctx, VERT_OBJ, VEC_NOT_WRITEABLE );
	    VB->ClipPtr = VB->ObjPtr;
	 }
	 if (VB->ClipPtr->size == 2)
	    _mesa_vector4f_clean_elem( VB->ClipPtr, VB->Count, 2 );
	 _mesa_vector4f_clean_elem( VB->ClipPtr, VB->Count, 3 );
      }

      /* Cliptest and perspective divide.  Clip functions must clear
       * the clipmask.
       */
      store->ormask = 0;
      store->andmask = CLIP_ALL_BITS;

      if (tnl->NeedProjCoords) {
	 VB->ProjectedClipPtr =
	    _mesa_clip_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                               &store->proj,
                                               store->clipmask,
                                               &store->ormask,
                                               &store->andmask );

      } else {
	 VB->ProjectedClipPtr = 0;
	 _mesa_clip_np_tab[VB->ClipPtr->size]( VB->ClipPtr,
                                               0,
                                               store->clipmask,
                                               &store->ormask,
                                               &store->andmask );
      }

      if (store->andmask)
	 return GL_FALSE;


      /* Test userclip planes.  This contributes to VB->ClipMask, so
       * is essentially required to be in this stage.
       */
      if (ctx->Transform._AnyClip) {
	 usercliptab[VB->ClipPtr->size]( ctx,
					 VB->ClipPtr,
					 store->clipmask,
					 &store->ormask,
					 &store->andmask );

	 if (store->andmask)
	    return GL_FALSE;
      }

      VB->ClipOrMask = store->ormask;
      VB->ClipMask = store->clipmask;

      if (VB->ClipPtr == VB->ObjPtr && (VB->importable_data & VERT_OBJ))
	 VB->importable_data |= VERT_CLIP;

      store->save_eyeptr = VB->EyePtr;
      store->save_clipptr = VB->ClipPtr;
      store->save_projptr = VB->ProjectedClipPtr;
   }
   else {
      /* Replay the sideeffects.
       */
      VB->EyePtr = store->save_eyeptr;
      VB->ClipPtr = store->save_clipptr;
      VB->ProjectedClipPtr = store->save_projptr;
      VB->ClipMask = store->clipmask;
      VB->ClipOrMask = store->ormask;
      if (VB->ClipPtr == VB->ObjPtr && (VB->importable_data & VERT_OBJ))
	 VB->importable_data |= VERT_CLIP;
      if (store->andmask)
	 return GL_FALSE;
   }

   return GL_TRUE;
}


static void check_vertex( GLcontext *ctx, struct gl_pipeline_stage *stage )
{
   (void) ctx;
   (void) stage;
}

static GLboolean init_vertex_stage( GLcontext *ctx,
				    struct gl_pipeline_stage *stage )
{
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   struct vertex_stage_data *store;
   GLuint size = VB->Size;

   stage->privatePtr = CALLOC(sizeof(*store));
   store = VERTEX_STAGE_DATA(stage);
   if (!store)
      return GL_FALSE;

   _mesa_vector4f_alloc( &store->eye, 0, size, 32 );
   _mesa_vector4f_alloc( &store->clip, 0, size, 32 );
   _mesa_vector4f_alloc( &store->proj, 0, size, 32 );

   store->clipmask = (GLubyte *) ALIGN_MALLOC(sizeof(GLubyte)*size, 32 );

   if (!store->clipmask ||
       !store->eye.data ||
       !store->clip.data ||
       !store->proj.data)
      return GL_FALSE;

   /* Now run the stage.
    */
   stage->run = run_vertex_stage;
   return stage->run( ctx, stage );
}

static void dtr( struct gl_pipeline_stage *stage )
{
   struct vertex_stage_data *store = VERTEX_STAGE_DATA(stage);

   if (store) {
      _mesa_vector4f_free( &store->eye );
      _mesa_vector4f_free( &store->clip );
      _mesa_vector4f_free( &store->proj );
      ALIGN_FREE( store->clipmask );
      FREE(store);
      stage->privatePtr = NULL;
      stage->run = init_vertex_stage;
   }
}


const struct gl_pipeline_stage _tnl_vertex_transform_stage =
{
   "modelview/project/cliptest/divide",
   0,				/* re-check -- always on */
   _MESA_NEW_NEED_EYE_COORDS |
   _NEW_MODELVIEW|
   _NEW_PROJECTION|
   _NEW_TRANSFORM,		/* re-run */
   GL_TRUE,			/* active */
   VERT_OBJ,		/* inputs */
   VERT_EYE|VERT_CLIP,		/* outputs */
   0, 0,			/* changed_inputs, private */
   dtr,				/* destructor */
   check_vertex,		/* check */
   init_vertex_stage		/* run -- initially set to init */
};
