/* $Id: t_imm_eval.c,v 1.5 2001/02/20 18:28:52 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
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
#include "t_imm_eval.h"
#include "t_imm_exec.h"
#include "t_imm_fixup.h"


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

static void eval1_color( GLvector4chan *dest,
			 GLfloat coord[][4],
			 const GLuint *flags,
			 struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLchan (*to)[4] = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++) {
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat fcolor[4];
	 _math_horner_bezier_curve(map->Points, fcolor, u, 4, map->Order);
         UNCLAMPED_FLOAT_TO_RGBA_CHAN(to[i], fcolor);
      }
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
	 _math_horner_bezier_surf(map->Points, to[i], u, v, dimension,
				  map->Uorder, map->Vorder);
      }

   dest->size = MAX2(dest->size, dimension);
   dest->flags |= dirty_flags[dimension];
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



static void eval2_color( GLvector4chan *dest,
			 GLfloat coord[][4],
			 GLuint *flags,
			 struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLchan (*to)[4] = dest->data;
   GLuint i;

   for (i = 0 ; !(flags[i] & VERT_END_VB) ; i++) {
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 GLfloat fcolor[4];
	 _math_horner_bezier_surf(map->Points, fcolor, u, v, 4,
				  map->Uorder, map->Vorder);
         UNCLAMPED_FLOAT_TO_RGBA_CHAN(to[i], fcolor);
      }
   }
}



static void copy_4f( GLfloat to[][4], GLfloat from[][4], GLuint count )
{
   MEMCPY( to, from, count * sizeof(to[0])); 
}

static void copy_3f( GLfloat to[][3], GLfloat from[][3], GLuint count )
{
   MEMCPY( to, from, (count) * sizeof(to[0])); 
}

static void copy_4chan( GLchan to[][4], GLchan from[][4], GLuint count )
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
void _tnl_eval_vb( GLcontext *ctx, 
		   GLfloat (*coord)[4],
		   GLuint orflag, 
		   GLuint andflag )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_arrays *tmp = &tnl->imm_inputs;
   struct tnl_eval_store *store = &tnl->eval;
   GLuint *flags = tnl->vb.Flag;
   GLuint count = tnl->vb.Count;
   GLuint any_eval1 = orflag & (VERT_EVAL_C1|VERT_EVAL_P1);
   GLuint any_eval2 = orflag & (VERT_EVAL_C2|VERT_EVAL_P2);
   GLuint all_eval = andflag & VERT_EVAL_ANY; /* may have false negatives */
   GLuint req = 0;
   GLuint purge_flags = 0;

   if (tnl->eval.EvalNewState & _NEW_EVAL)
      update_eval( ctx );

   /* Handle the degenerate cases.
    */
   if (any_eval1 && !ctx->Eval.Map1Vertex4 && !ctx->Eval.Map1Vertex3)
      purge_flags = (VERT_EVAL_P1|VERT_EVAL_C1);

   if (any_eval2 && !ctx->Eval.Map2Vertex4 && !ctx->Eval.Map2Vertex3) 
      purge_flags |= (VERT_EVAL_P1|VERT_EVAL_C1);

   if (any_eval1) 
      req |= tnl->pipeline.inputs & tnl->eval.EvalMap1Flags;

   if (any_eval2) 
      req |= tnl->pipeline.inputs & tnl->eval.EvalMap2Flags;

   
   /* Translate points into coords.  Use store->Coord to hold the
    * new data.  
    */
   if (any_eval1 && (orflag & VERT_EVAL_P1))
   {
      eval_points1( store->Coord, coord, flags,
		    ctx->Eval.MapGrid1du,
		    ctx->Eval.MapGrid1u1);

      coord = store->Coord;
   }

   if (any_eval2 && (orflag & VERT_EVAL_P2))
   {
      eval_points2( store->Coord, coord, flags,
		    ctx->Eval.MapGrid2du,
		    ctx->Eval.MapGrid2u1,
		    ctx->Eval.MapGrid2dv,
		    ctx->Eval.MapGrid2v1 );

      coord = store->Coord;
   }


   /* Perform the evaluations on active data elements.
    */
   if (req & VERT_INDEX)
   {
      if (!all_eval) 
	 copy_1ui( store->Index, tmp->Index.data, count );

      tmp->Index.data = store->Index;
      tmp->Index.start = store->Index;

      if (ctx->Eval.Map1Index && any_eval1) 
	 eval1_1ui( &tmp->Index, coord, flags, &ctx->EvalMap.Map1Index );

      if (ctx->Eval.Map2Index && any_eval2)
	 eval2_1ui( &tmp->Index, coord, flags, &ctx->EvalMap.Map2Index );

   }

   if (req & VERT_RGBA)
   {
      if (!all_eval)
	 copy_4chan( store->Color, tmp->Color.data, count );

      tmp->Color.data = store->Color;
      tmp->Color.start = (GLchan *) store->Color;

      if (ctx->Eval.Map1Color4 && any_eval1)
	 eval1_color( &tmp->Color, coord, flags, &ctx->EvalMap.Map1Color4 );

      if (ctx->Eval.Map2Color4 && any_eval2)
	 eval2_color( &tmp->Color, coord, flags, &ctx->EvalMap.Map2Color4 );
   }


   if (req & VERT_TEX(0))
   {
      if (!all_eval) 
	 copy_4f( store->TexCoord, tmp->TexCoord[0].data, count );
      else 
	 tmp->TexCoord[0].size = 0;
	 
      tmp->TexCoord[0].data = store->TexCoord;
      tmp->TexCoord[0].start = (GLfloat *)store->TexCoord;

      if (any_eval1) {
	 if (ctx->Eval.Map1TextureCoord4) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 4,
		      &ctx->EvalMap.Map1Texture4 );
	 }
	 else if (ctx->Eval.Map1TextureCoord3) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 3,
		      &ctx->EvalMap.Map1Texture3 );
	 }
	 else if (ctx->Eval.Map1TextureCoord2) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 2,
		      &ctx->EvalMap.Map1Texture2 );
	 } 
	 else if (ctx->Eval.Map1TextureCoord1) {
	    eval1_4f( &tmp->TexCoord[0], coord, flags, 1,
		      &ctx->EvalMap.Map1Texture1 );
	 }
      }

      if (any_eval2) {
	 if (ctx->Eval.Map2TextureCoord4) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 4,
		      &ctx->EvalMap.Map2Texture4 );
	 }
	 else if (ctx->Eval.Map2TextureCoord3) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 3, 
		      &ctx->EvalMap.Map2Texture3 );
	 }
	 else if (ctx->Eval.Map2TextureCoord2) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 2,
		      &ctx->EvalMap.Map2Texture2 );
	 }
	 else if (ctx->Eval.Map2TextureCoord1) {
	    eval2_4f( &tmp->TexCoord[0], coord, flags, 1,
		      &ctx->EvalMap.Map2Texture1 );
	 }
      }
   }


   if (req & VERT_NORM)
   {
      if (!all_eval)
	 copy_3f( store->Normal, tmp->Normal.data, count );

      tmp->Normal.data = store->Normal;
      tmp->Normal.start = (GLfloat *)store->Normal;

      if (ctx->Eval.Map1Normal && any_eval1)
	 eval1_norm( &tmp->Normal, coord, flags,
		     &ctx->EvalMap.Map1Normal );

      if (ctx->Eval.Map2Normal && any_eval2)
	 eval2_norm( &tmp->Normal, coord, flags,
		     &ctx->EvalMap.Map2Normal );
   }



   /* In the AutoNormal case, the copy and assignment of tmp->NormalPtr
    * are done above.
    */
   if (req & VERT_OBJ)
   {
      if (!all_eval) {
	 copy_4f( store->Obj, tmp->Obj.data, count );
      } else
	 tmp->Obj.size = 0;

      tmp->Obj.data = store->Obj;
      tmp->Obj.start = (GLfloat *)store->Obj;

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


   {
      GLuint i;
      copy_1ui( store->Flag, flags, count );
      tnl->vb.Flag = store->Flag;
      
      /* This is overkill, but correct as fixup will have copied the
       * values to all vertices in the VB - we may be falsely stating
       * that some repeated values are new, but doing so is fairly
       * harmless.
       */
      for (i = 0 ; i < count ; i++)
	 store->Flag[i] |= req;
   }
}







