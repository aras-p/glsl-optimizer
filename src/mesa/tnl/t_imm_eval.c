/* $Id: t_imm_eval.c,v 1.13 2001/05/14 09:00:51 keithw Exp $ */

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
 *
 */


#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#include "math/m_eval.h"

#include "t_context.h"
#include "t_imm_debug.h"
#include "t_imm_eval.h"
#include "t_imm_exec.h"
#include "t_imm_fixup.h"
#include "t_imm_alloc.h"


static void eval_points1( GLfloat outcoord[][4],
			  GLfloat coord[][4],
			  const GLuint *flags,
			  GLfloat du, GLfloat u1 )
{
   GLuint i;
   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & VERT_EVAL_ANY) {
	 outcoord[i][0] = coord[i][0];
	 outcoord[i][1] = coord[i][1];
	 if (flags[i] & VERT_EVAL_P1)
	    outcoord[i][0] = coord[i][0] * du + u1;
      }
}

static void eval_points2( GLfloat outcoord[][4],
			  GLfloat coord[][4],
			  const GLuint *flags,
			  GLfloat du, GLfloat u1,
			  GLfloat dv, GLfloat v1 )
{
   GLuint i;
   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++) {
      if (flags[i] & VERT_EVAL_ANY) {
	 outcoord[i][0] = coord[i][0];
	 outcoord[i][1] = coord[i][1];
	 if (flags[i] & VERT_EVAL_P2) {
	    outcoord[i][0] = coord[i][0] * du + u1;
	    outcoord[i][1] = coord[i][1] * dv + v1;
	 }
      }
   }
}

static const GLubyte dirty_flags[5] = {
   0,				/* not possible */
   VEC_DIRTY_0,
   VEC_DIRTY_1,
   VEC_DIRTY_2,
   VEC_DIRTY_3
};


static void eval1_4f( GLvector4f *dest,
		      GLfloat coord[][4],
		      const GLuint *flags,
		      GLuint dimension,
		      struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLfloat (*to)[4] = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 ASSIGN_4V(to[i], 0,0,0,1);
	 _math_horner_bezier_curve(map->Points, to[i], u,
				   dimension, map->Order);
      }

   dest->size = MAX2(dest->size, dimension);
   dest->flags |= dirty_flags[dimension];
}

static void eval1_4f_ca( struct gl_client_array *dest,
			 GLfloat coord[][4],
			 const GLuint *flags,
			 GLuint dimension,
			 struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLfloat (*to)[4] = (GLfloat (*)[4])dest->Ptr;
   GLuint i;

   ASSERT(dest->Type == GL_FLOAT);
   ASSERT(dest->StrideB == 4 * sizeof(GLfloat));

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 ASSIGN_4V(to[i], 0,0,0,1);
	 _math_horner_bezier_curve(map->Points, to[i], u,
				   dimension, map->Order);
      }

   dest->Size = MAX2(dest->Size, (GLint) dimension);
}


static void eval1_1ui( GLvector1ui *dest,
		       GLfloat coord[][4],
		       const GLuint *flags,
		       struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLuint *to = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat tmp;
	 _math_horner_bezier_curve(map->Points, &tmp, u, 1, map->Order);
	 to[i] = (GLuint) (GLint) tmp;
      }

}

static void eval1_norm( GLvector3f *dest,
			GLfloat coord[][4],
			const GLuint *flags,
			struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLfloat (*to)[3] = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 _math_horner_bezier_curve(map->Points, to[i], u, 3, map->Order);
      }
}





static void eval2_obj_norm( GLvector4f *obj_ptr,
			    GLvector3f *norm_ptr,
			    GLfloat coord[][4],
			    GLuint *flags,
			    GLuint dimension,
			    struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLfloat (*obj)[4] = obj_ptr->data;
   GLfloat (*normal)[3] = norm_ptr->data;
   GLuint i;

/*     fprintf(stderr, "%s\n", __FUNCTION__); */

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 GLfloat du[4], dv[4];

	 ASSIGN_4V(obj[i], 0,0,0,1);
	 _math_de_casteljau_surf(map->Points, obj[i], du, dv, u, v, dimension,
				 map->Uorder, map->Vorder);

	 CROSS3(normal[i], du, dv);
	 NORMALIZE_3FV(normal[i]);
      }

   obj_ptr->size = MAX2(obj_ptr->size, dimension);
   obj_ptr->flags |= dirty_flags[dimension];
}


static void eval2_4f( GLvector4f *dest,
		      GLfloat coord[][4],
		      const GLuint *flags,
		      GLuint dimension,
		      struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLfloat (*to)[4] = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
/*  	 fprintf(stderr, "coord %d: %f %f\n", i, coord[i][0], coord[i][1]); */

	 _math_horner_bezier_surf(map->Points, to[i], u, v, dimension,
				  map->Uorder, map->Vorder);
      }

   dest->size = MAX2(dest->size, dimension);
   dest->flags |= dirty_flags[dimension];
}

static void eval2_4f_ca( struct gl_client_array *dest,
			 GLfloat coord[][4],
			 const GLuint *flags,
			 GLuint dimension,
			 struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLfloat (*to)[4] = (GLfloat (*)[4])dest->Ptr;
   GLuint i;

   ASSERT(dest->Type == GL_FLOAT);
   ASSERT(dest->StrideB == 4 * sizeof(GLfloat));

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 _math_horner_bezier_surf(map->Points, to[i], u, v, dimension,
				  map->Uorder, map->Vorder);
      }

   dest->Size = MAX2(dest->Size, (GLint) dimension);
}


static void eval2_norm( GLvector3f *dest,
			GLfloat coord[][4],
			GLuint *flags,
			struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLfloat (*to)[3] = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 _math_horner_bezier_surf(map->Points, to[i], u, v, 3,
				  map->Uorder, map->Vorder);
     }

}


static void eval2_1ui( GLvector1ui *dest,
		       GLfloat coord[][4],
		       const GLuint *flags,
		       struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLuint *to = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 GLfloat tmp;
	 _math_horner_bezier_surf(map->Points, &tmp, u, v, 1,
				  map->Uorder, map->Vorder);

	 to[i] = (GLuint) (GLint) tmp;
      }
}






static void copy_4f( GLfloat to[][4], GLfloat from[][4], GLuint count )
{
   MEMCPY( to, from, count * sizeof(to[0]));
}

static void copy_4f_stride( GLfloat to[][4], GLfloat *from, 
			    GLuint stride, GLuint count )
{
   if (stride == 4 * sizeof(GLfloat))
      MEMCPY( to, from, count * sizeof(to[0]));
   else {
      GLuint i;
/*        fprintf(stderr, "%s stride %d count %d\n", __FUNCTION__, */
/*  	      stride, count); */
      for (i = 0 ; i < count ; i++, STRIDE_F(from, stride))
	 COPY_4FV( to[i], from );
   }
}

static void copy_3f( GLfloat to[][3], GLfloat from[][3], GLuint count )
{
   MEMCPY( to, from, (count) * sizeof(to[0]));
}


static void copy_1ui( GLuint to[], GLuint from[], GLuint count )
{
   MEMCPY( to, from, (count) * sizeof(to[0]));
}



/* Translate eval enabled flags to VERT_* flags.
 */
static void update_eval( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint eval1 = 0, eval2 = 0;

   if (ctx->Eval.Map1Index)
      eval1 |= VERT_INDEX;

   if (ctx->Eval.Map2Index)
      eval2 |= VERT_INDEX;

   if (ctx->Eval.Map1Color4)
      eval1 |= VERT_RGBA;

   if (ctx->Eval.Map2Color4)
      eval2 |= VERT_RGBA;

   if (ctx->Eval.Map1Normal)
      eval1 |= VERT_NORM;

   if (ctx->Eval.Map2Normal)
      eval2 |= VERT_NORM;

   if (ctx->Eval.Map1TextureCoord4 ||
       ctx->Eval.Map1TextureCoord3 ||
       ctx->Eval.Map1TextureCoord2 ||
       ctx->Eval.Map1TextureCoord1)
      eval1 |= VERT_TEX0;

   if (ctx->Eval.Map2TextureCoord4 ||
       ctx->Eval.Map2TextureCoord3 ||
       ctx->Eval.Map2TextureCoord2 ||
       ctx->Eval.Map2TextureCoord1)
      eval2 |= VERT_TEX0;

   if (ctx->Eval.Map1Vertex4)
      eval1 |= VERT_OBJ_234;

   if (ctx->Eval.Map1Vertex3)
      eval1 |= VERT_OBJ_23;

   if (ctx->Eval.Map2Vertex4) {
      if (ctx->Eval.AutoNormal)
	 eval2 |= VERT_OBJ_234 | VERT_NORM;
      else
	 eval2 |= VERT_OBJ_234;
   }
   else if (ctx->Eval.Map2Vertex3) {
      if (ctx->Eval.AutoNormal)
	 eval2 |= VERT_OBJ_23 | VERT_NORM;
      else
	 eval2 |= VERT_OBJ_23;
   }

   tnl->eval.EvalMap1Flags = eval1;
   tnl->eval.EvalMap2Flags = eval2;
   tnl->eval.EvalNewState = 0;
}


/* This looks a lot like a pipeline stage, but for various reasons is
 * better handled outside the pipeline, and considered the final stage
 * of fixing up an immediate struct for execution.
 *
 * Really want to cache the results of this function in display lists,
 * at least for EvalMesh commands.
 */
void _tnl_eval_immediate( GLcontext *ctx, struct immediate *IM )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_arrays *tmp = &tnl->imm_inputs;
   struct immediate *store = tnl->eval.im;
   GLuint *flags = IM->Flag + IM->CopyStart;
   GLuint copycount;
   GLuint orflag = IM->OrFlag;
   GLuint any_eval1 = orflag & (VERT_EVAL_C1|VERT_EVAL_P1);
   GLuint any_eval2 = orflag & (VERT_EVAL_C2|VERT_EVAL_P2);
   GLuint req = 0;
   GLuint purge_flags = 0;
   GLfloat (*coord)[4] = IM->Obj + IM->CopyStart;

   if (IM->AndFlag & VERT_EVAL_ANY)
      copycount = IM->Start - IM->CopyStart; /* just copy copied vertices */
   else
      copycount = IM->Count - IM->CopyStart; /* copy all vertices */

/*     fprintf(stderr, "%s copystart %d start %d count %d copycount %d\n", */
/*  	   __FUNCTION__, IM->CopyStart, IM->Start, IM->Count, copycount); */

   if (!store)
      store = tnl->eval.im = _tnl_alloc_immediate( ctx );

   if (tnl->eval.EvalNewState & _NEW_EVAL)
      update_eval( ctx );

   if (any_eval1) {
      req |= tnl->pipeline.inputs & tnl->eval.EvalMap1Flags;

      if (!ctx->Eval.Map1Vertex4 && !ctx->Eval.Map1Vertex3)
	 purge_flags = (VERT_EVAL_P1|VERT_EVAL_C1);

      if (orflag & VERT_EVAL_P1) {
	 eval_points1( store->Obj + IM->CopyStart, 
		       coord, flags,
		       ctx->Eval.MapGrid1du,
		       ctx->Eval.MapGrid1u1);
	 
	 coord = store->Obj + IM->CopyStart;
      }
   }

   if (any_eval2) {
      req |= tnl->pipeline.inputs & tnl->eval.EvalMap2Flags;

      if (!ctx->Eval.Map2Vertex4 && !ctx->Eval.Map2Vertex3)
	 purge_flags |= (VERT_EVAL_P2|VERT_EVAL_C2);

      if (orflag & VERT_EVAL_P2) {
	 eval_points2( store->Obj + IM->CopyStart, 
		       coord, flags,
		       ctx->Eval.MapGrid2du,
		       ctx->Eval.MapGrid2u1,
		       ctx->Eval.MapGrid2dv,
		       ctx->Eval.MapGrid2v1 );

	 coord = store->Obj + IM->CopyStart;
      }
   }


/*     _tnl_print_vert_flags(__FUNCTION__, req); */

   /* Perform the evaluations on active data elements.
    */
   if (req & VERT_INDEX)
   {
      GLuint generated = 0;

      if (copycount)
	 copy_1ui( store->Index + IM->CopyStart, tmp->Index.data, copycount );

      tmp->Index.data = store->Index + IM->CopyStart;
      tmp->Index.start = store->Index + IM->CopyStart;

      if (ctx->Eval.Map1Index && any_eval1) {
	 eval1_1ui( &tmp->Index, coord, flags, &ctx->EvalMap.Map1Index );
	 generated |= VERT_EVAL_C1|VERT_EVAL_P1;
      }

      if (ctx->Eval.Map2Index && any_eval2) {
	 eval2_1ui( &tmp->Index, coord, flags, &ctx->EvalMap.Map2Index );
	 generated |= VERT_EVAL_C2|VERT_EVAL_P2;
      }

      /* Propogate values to generate correct vertices when vertex
       * maps are disabled.
       */
      if (purge_flags & generated)
	 _tnl_fixup_1ui( tmp->Index.data, flags, 0, 
			 VERT_INDEX|
			 VERT_OBJ|
			 generated|
			 (VERT_EVAL_ANY&~purge_flags) );
   }

   if (req & VERT_RGBA)
   {
      GLuint generated = 0;

      if (copycount) 
	 copy_4f_stride( store->Color + IM->CopyStart, 
			 (GLfloat *)tmp->Color.Ptr, 
			 tmp->Color.StrideB,
			 copycount );

      tmp->Color.Ptr = store->Color + IM->CopyStart;
      tmp->Color.StrideB = 4 * sizeof(GLfloat);
      tmp->Color.Flags = 0;
      tnl->vb.importable_data &= ~VERT_RGBA;

      if (ctx->Eval.Map1Color4 && any_eval1) {
	 eval1_4f_ca( &tmp->Color, coord, flags, 4, &ctx->EvalMap.Map1Color4 );
	 generated |= VERT_EVAL_C1|VERT_EVAL_P1;
      }

      if (ctx->Eval.Map2Color4 && any_eval2) {
	 eval2_4f_ca( &tmp->Color, coord, flags, 4, &ctx->EvalMap.Map2Color4 );
	 generated |= VERT_EVAL_C2|VERT_EVAL_P2;
      }

      /* Propogate values to generate correct vertices when vertex
       * maps are disabled.
       */
      if (purge_flags & generated)
	 _tnl_fixup_4f( store->Color + IM->CopyStart, 
			flags, 0, 
			VERT_RGBA|
			VERT_OBJ|
			generated|
			(VERT_EVAL_ANY&~purge_flags) );
   }


   if (req & VERT_TEX(0))
   {
      GLuint generated = 0;

      if (copycount)
	 copy_4f( store->TexCoord[0] + IM->CopyStart, 
		  tmp->TexCoord[0].data, copycount );
      else
	 tmp->TexCoord[0].size = 0;

      tmp->TexCoord[0].data = store->TexCoord[0] + IM->CopyStart;
      tmp->TexCoord[0].start = (GLfloat *)tmp->TexCoord[0].data;

      if (any_eval1) {
	 if (ctx->Eval.Map1TextureCoord4) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 4,
		      &ctx->EvalMap.Map1Texture4 );
	    generated |= VERT_EVAL_C1|VERT_EVAL_P1;
	 }
	 else if (ctx->Eval.Map1TextureCoord3) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 3,
		      &ctx->EvalMap.Map1Texture3 );
	    generated |= VERT_EVAL_C1|VERT_EVAL_P1;
	 }
	 else if (ctx->Eval.Map1TextureCoord2) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 2,
		      &ctx->EvalMap.Map1Texture2 );
	    generated |= VERT_EVAL_C1|VERT_EVAL_P1;
	 }
	 else if (ctx->Eval.Map1TextureCoord1) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 1,
		      &ctx->EvalMap.Map1Texture1 );
	    generated |= VERT_EVAL_C1|VERT_EVAL_P1;
	 }
      }

      if (any_eval2) {
	 if (ctx->Eval.Map2TextureCoord4) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 4,
		      &ctx->EvalMap.Map2Texture4 );
	    generated |= VERT_EVAL_C2|VERT_EVAL_P2;
	 }
	 else if (ctx->Eval.Map2TextureCoord3) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 3,
		      &ctx->EvalMap.Map2Texture3 );
	    generated |= VERT_EVAL_C2|VERT_EVAL_P2;
	 }
	 else if (ctx->Eval.Map2TextureCoord2) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 2,
		      &ctx->EvalMap.Map2Texture2 );
	    generated |= VERT_EVAL_C2|VERT_EVAL_P2;
	 }
	 else if (ctx->Eval.Map2TextureCoord1) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 1,
		      &ctx->EvalMap.Map2Texture1 );
	    generated |= VERT_EVAL_C2|VERT_EVAL_P2;
	 }
      }

      /* Propogate values to generate correct vertices when vertex
       * maps are disabled.
       */
      if (purge_flags & generated)
	 _tnl_fixup_4f( tmp->TexCoord[0].data, flags, 0, 
			VERT_TEX0|
			VERT_OBJ|
			generated|
			(VERT_EVAL_ANY&~purge_flags) );
   }


   if (req & VERT_NORM)
   {
      GLuint generated = 0;

      if (copycount) {
/*  	 fprintf(stderr, "%s: Copy normals\n", __FUNCTION__); */
	 copy_3f( store->Normal + IM->CopyStart, tmp->Normal.data, 
		  copycount );
      }

      tmp->Normal.data = store->Normal + IM->CopyStart;
      tmp->Normal.start = (GLfloat *)tmp->Normal.data;

      if (ctx->Eval.Map1Normal && any_eval1) {
	 eval1_norm( &tmp->Normal, coord, flags,
		     &ctx->EvalMap.Map1Normal );
	 generated |= VERT_EVAL_C1|VERT_EVAL_P1;
      }

      if (ctx->Eval.Map2Normal && any_eval2) {
	 eval2_norm( &tmp->Normal, coord, flags,
		     &ctx->EvalMap.Map2Normal );
	 generated |= VERT_EVAL_C2|VERT_EVAL_P2;
      }

      /* Propogate values to generate correct vertices when vertex
       * maps are disabled.
       */
      if (purge_flags & generated)
	 _tnl_fixup_3f( tmp->Normal.data, flags, 0, 
			VERT_NORM|
			VERT_OBJ|
			generated|
			(VERT_EVAL_ANY&~purge_flags) );
   }



   /* In the AutoNormal case, the copy and assignment of tmp->NormalPtr
    * are done above.
    */
   if (req & VERT_OBJ)
   {
      if (copycount) {
	 /* This copy may already have occurred when eliminating
	  * glEvalPoint calls:
	  */
	 if  (coord != store->Obj + IM->CopyStart)
	    copy_4f( store->Obj + IM->CopyStart, tmp->Obj.data, copycount );
      } else
	 tmp->Obj.size = 0;

      tmp->Obj.data = store->Obj + IM->CopyStart;
      tmp->Obj.start = (GLfloat *)tmp->Obj.data;

      /* Note: Normal data is already prepared above.
       */

      if (any_eval1) {
	 if (ctx->Eval.Map1Vertex4) {
	    eval1_4f( &tmp->Obj, coord, flags, 4,
		      &ctx->EvalMap.Map1Vertex4 );
	 }
	 else if (ctx->Eval.Map1Vertex3) {
	    eval1_4f( &tmp->Obj, coord, flags, 3,
		      &ctx->EvalMap.Map1Vertex3 );
	 }
      }

      if (any_eval2) {
	 if (ctx->Eval.Map2Vertex4)
	 {
	    if (ctx->Eval.AutoNormal && (req & VERT_NORM))
	       eval2_obj_norm( &tmp->Obj, &tmp->Normal, coord, flags, 4,
			       &ctx->EvalMap.Map2Vertex4 );
	    else
	       eval2_4f( &tmp->Obj, coord, flags, 4,
			 &ctx->EvalMap.Map2Vertex4 );
	 }
	 else if (ctx->Eval.Map2Vertex3)
	 {
	    if (ctx->Eval.AutoNormal && (req & VERT_NORM))
	       eval2_obj_norm( &tmp->Obj, &tmp->Normal, coord, flags, 3,
			       &ctx->EvalMap.Map2Vertex3 );
	    else
	       eval2_4f( &tmp->Obj, coord, flags, 3,
			 &ctx->EvalMap.Map2Vertex3 );
	 }
      }
   }


   /* Calculate new IM->Elts, IM->Primitive, IM->PrimitiveLength for
    * the case where vertex maps are not enabled for some received eval
    * coordinates.
    */
   if (purge_flags) {
      GLuint vertex = VERT_OBJ|(VERT_EVAL_ANY & ~purge_flags);
      GLuint last_new_prim = 0;
      GLuint new_prim_length = 0;
      GLuint next_old_prim = 0;
      struct vertex_buffer *VB = &tnl->vb;
      GLuint i,j,count = VB->Count;

/*        fprintf(stderr, "PURGING\n"); */

      for (i = 0, j = 0 ; i < count ; i++) {
	 if (flags[i] & vertex) {
	    store->Elt[j++] = i;
	    new_prim_length++;
	 }
	 if (i == next_old_prim) {
	    next_old_prim += VB->PrimitiveLength[i];
	    VB->PrimitiveLength[last_new_prim] = new_prim_length;
	    VB->Primitive[j] = VB->Primitive[i];
	    last_new_prim = j;
	 }
      }
      
      VB->Elts = store->Elt;
      _tnl_get_purged_copy_verts( ctx, store );
   }

   /* Produce new flags array:
    */
   {
      GLuint i;
      GLuint count = tnl->vb.Count + 1;

      copy_1ui( store->Flag, flags, count );
      tnl->vb.Flag = store->Flag;
      for (i = 0 ; i < count ; i++)
	 store->Flag[i] |= req;
      IM->CopyOrFlag |= req;	/* hack for copying. */
   }
}
