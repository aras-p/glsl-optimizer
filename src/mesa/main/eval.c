/* $Id: eval.c,v 1.21 2001/09/18 16:16:21 kschultz Exp $ */

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
 */


/*
 * eval.c was written by
 * Bernd Barsuhn (bdbarsuh@cip.informatik.uni-erlangen.de) and
 * Volker Weiss (vrweiss@cip.informatik.uni-erlangen.de).
 *
 * My original implementation of evaluators was simplistic and didn't
 * compute surface normal vectors properly.  Bernd and Volker applied
 * used more sophisticated methods to get better results.
 *
 * Thanks guys!
 */


#ifdef PC_HEADER
#include "all.h"
#else
#include "glheader.h"
#include "colormac.h"
#include "context.h"
#include "eval.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "mtypes.h"
#endif


/*
 * Return the number of components per control point for any type of
 * evaluator.  Return 0 if bad target.
 * See table 5.1 in the OpenGL 1.2 spec.
 */
GLuint _mesa_evaluator_components( GLenum target )
{
   switch (target) {
      case GL_MAP1_VERTEX_3:		return 3;
      case GL_MAP1_VERTEX_4:		return 4;
      case GL_MAP1_INDEX:		return 1;
      case GL_MAP1_COLOR_4:		return 4;
      case GL_MAP1_NORMAL:		return 3;
      case GL_MAP1_TEXTURE_COORD_1:	return 1;
      case GL_MAP1_TEXTURE_COORD_2:	return 2;
      case GL_MAP1_TEXTURE_COORD_3:	return 3;
      case GL_MAP1_TEXTURE_COORD_4:	return 4;
      case GL_MAP2_VERTEX_3:		return 3;
      case GL_MAP2_VERTEX_4:		return 4;
      case GL_MAP2_INDEX:		return 1;
      case GL_MAP2_COLOR_4:		return 4;
      case GL_MAP2_NORMAL:		return 3;
      case GL_MAP2_TEXTURE_COORD_1:	return 1;
      case GL_MAP2_TEXTURE_COORD_2:	return 2;
      case GL_MAP2_TEXTURE_COORD_3:	return 3;
      case GL_MAP2_TEXTURE_COORD_4:	return 4;
      default:				return 0;
   }
}


/**********************************************************************/
/***            Copy and deallocate control points                  ***/
/**********************************************************************/


/*
 * Copy 1-parametric evaluator control points from user-specified
 * memory space to a buffer of contiguous control points.
 * Input:  see glMap1f for details
 * Return:  pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
GLfloat *_mesa_copy_map_points1f( GLenum target, GLint ustride, GLint uorder,
                               const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLint i, k, size = _mesa_evaluator_components(target);

   if (!points || size==0) {
      return NULL;
   }

   buffer = (GLfloat *) MALLOC(uorder * size * sizeof(GLfloat));

   if(buffer)
      for(i=0, p=buffer; i<uorder; i++, points+=ustride)
	for(k=0; k<size; k++)
	  *p++ = points[k];

   return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
GLfloat *_mesa_copy_map_points1d( GLenum target, GLint ustride, GLint uorder,
                               const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLint i, k, size = _mesa_evaluator_components(target);

   if (!points || size==0) {
      return NULL;
   }

   buffer = (GLfloat *) MALLOC(uorder * size * sizeof(GLfloat));

   if(buffer)
      for(i=0, p=buffer; i<uorder; i++, points+=ustride)
	for(k=0; k<size; k++)
	  *p++ = (GLfloat) points[k];

   return buffer;
}



/*
 * Copy 2-parametric evaluator control points from user-specified
 * memory space to a buffer of contiguous control points.
 * Additional memory is allocated to be used by the horner and
 * de Casteljau evaluation schemes.
 *
 * Input:  see glMap2f for details
 * Return:  pointer to buffer of contiguous control points or NULL if out
 *          of memory.
 */
GLfloat *_mesa_copy_map_points2f( GLenum target,
                               GLint ustride, GLint uorder,
                               GLint vstride, GLint vorder,
                               const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLint i, j, k, size, dsize, hsize;
   GLint uinc;

   size = _mesa_evaluator_components(target);

   if (!points || size==0) {
      return NULL;
   }

   /* max(uorder, vorder) additional points are used in      */
   /* horner evaluation and uorder*vorder additional */
   /* values are needed for de Casteljau                     */
   dsize = (uorder == 2 && vorder == 2)? 0 : uorder*vorder;
   hsize = (uorder > vorder ? uorder : vorder)*size;

   if(hsize>dsize)
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+hsize)*sizeof(GLfloat));
   else
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+dsize)*sizeof(GLfloat));

   /* compute the increment value for the u-loop */
   uinc = ustride - vorder*vstride;

   if (buffer)
      for (i=0, p=buffer; i<uorder; i++, points += uinc)
	 for (j=0; j<vorder; j++, points += vstride)
	    for (k=0; k<size; k++)
	       *p++ = points[k];

   return buffer;
}



/*
 * Same as above but convert doubles to floats.
 */
GLfloat *_mesa_copy_map_points2d(GLenum target,
                              GLint ustride, GLint uorder,
                              GLint vstride, GLint vorder,
                              const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLint i, j, k, size, hsize, dsize;
   GLint uinc;

   size = _mesa_evaluator_components(target);

   if (!points || size==0) {
      return NULL;
   }

   /* max(uorder, vorder) additional points are used in      */
   /* horner evaluation and uorder*vorder additional */
   /* values are needed for de Casteljau                     */
   dsize = (uorder == 2 && vorder == 2)? 0 : uorder*vorder;
   hsize = (uorder > vorder ? uorder : vorder)*size;

   if(hsize>dsize)
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+hsize)*sizeof(GLfloat));
   else
     buffer = (GLfloat *) MALLOC((uorder*vorder*size+dsize)*sizeof(GLfloat));

   /* compute the increment value for the u-loop */
   uinc = ustride - vorder*vstride;

   if (buffer)
      for (i=0, p=buffer; i<uorder; i++, points += uinc)
	 for (j=0; j<vorder; j++, points += vstride)
	    for (k=0; k<size; k++)
	       *p++ = (GLfloat) points[k];

   return buffer;
}




/**********************************************************************/
/***                      API entry points                          ***/
/**********************************************************************/


/*
 * This does the work of glMap1[fd].
 */
static void
map1(GLenum target, GLfloat u1, GLfloat u2, GLint ustride,
     GLint uorder, const GLvoid *points, GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint k;
   GLfloat *pnts;
   struct gl_1d_map *map = 0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   assert(type == GL_FLOAT || type == GL_DOUBLE);

   if (u1 == u2) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(u1,u2)" );
      return;
   }
   if (uorder < 1 || uorder > MAX_EVAL_ORDER) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(order)" );
      return;
   }
   if (!points) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(points)" );
      return;
   }

   k = _mesa_evaluator_components( target );
   if (k == 0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
   }

   if (ustride < k) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap1(stride)" );
      return;
   }

   if (ctx->Texture.CurrentUnit != 0) {
      /* See OpenGL 1.2.1 spec, section F.2.13 */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glMap2(ACTIVE_TEXTURE != 0)" );
      return;
   }

   switch (target) {
      case GL_MAP1_VERTEX_3:
         map = &ctx->EvalMap.Map1Vertex3;
	 break;
      case GL_MAP1_VERTEX_4:
         map = &ctx->EvalMap.Map1Vertex4;
	 break;
      case GL_MAP1_INDEX:
         map = &ctx->EvalMap.Map1Index;
	 break;
      case GL_MAP1_COLOR_4:
         map = &ctx->EvalMap.Map1Color4;
	 break;
      case GL_MAP1_NORMAL:
         map = &ctx->EvalMap.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
         map = &ctx->EvalMap.Map1Texture1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
         map = &ctx->EvalMap.Map1Texture2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
         map = &ctx->EvalMap.Map1Texture3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
         map = &ctx->EvalMap.Map1Texture4;
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
	 return;
   }

   /* make copy of the control points */
   if (type == GL_FLOAT)
      pnts = _mesa_copy_map_points1f(target, ustride, uorder, (GLfloat*) points);
   else
      pnts = _mesa_copy_map_points1d(target, ustride, uorder, (GLdouble*) points);


   FLUSH_VERTICES(ctx, _NEW_EVAL);
   map->Order = uorder;
   map->u1 = u1;
   map->u2 = u2;
   map->du = 1.0F / (u2 - u1);
   if (map->Points)
      FREE( map->Points );
   map->Points = pnts;
}



void
_mesa_Map1f( GLenum target, GLfloat u1, GLfloat u2, GLint stride,
             GLint order, const GLfloat *points )
{
   map1(target, u1, u2, stride, order, points, GL_FLOAT);
}


void
_mesa_Map1d( GLenum target, GLdouble u1, GLdouble u2, GLint stride,
             GLint order, const GLdouble *points )
{
   map1(target, (GLfloat) u1, (GLfloat) u2, stride, order, points, GL_DOUBLE);
}


static void
map2( GLenum target, GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
      const GLvoid *points, GLenum type )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint k;
   GLfloat *pnts;
   struct gl_2d_map *map = 0;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (u1==u2) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(u1,u2)" );
      return;
   }

   if (v1==v2) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(v1,v2)" );
      return;
   }

   if (uorder<1 || uorder>MAX_EVAL_ORDER) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(uorder)" );
      return;
   }

   if (vorder<1 || vorder>MAX_EVAL_ORDER) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(vorder)" );
      return;
   }

   k = _mesa_evaluator_components( target );
   if (k==0) {
      _mesa_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
   }

   if (ustride < k) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(ustride)" );
      return;
   }
   if (vstride < k) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMap2(vstride)" );
      return;
   }

   if (ctx->Texture.CurrentUnit != 0) {
      /* See OpenGL 1.2.1 spec, section F.2.13 */
      _mesa_error( ctx, GL_INVALID_OPERATION, "glMap2(ACTIVE_TEXTURE != 0)" );
      return;
   }

   switch (target) {
      case GL_MAP2_VERTEX_3:
         map = &ctx->EvalMap.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
         map = &ctx->EvalMap.Map2Vertex4;
	 break;
      case GL_MAP2_INDEX:
         map = &ctx->EvalMap.Map2Index;
	 break;
      case GL_MAP2_COLOR_4:
         map = &ctx->EvalMap.Map2Color4;
	 break;
      case GL_MAP2_NORMAL:
         map = &ctx->EvalMap.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
         map = &ctx->EvalMap.Map2Texture1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
         map = &ctx->EvalMap.Map2Texture2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
         map = &ctx->EvalMap.Map2Texture3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
         map = &ctx->EvalMap.Map2Texture4;
	 break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
	 return;
   }

   /* make copy of the control points */
   if (type == GL_FLOAT)
      pnts = _mesa_copy_map_points2f(target, ustride, uorder,
                                  vstride, vorder, (GLfloat*) points);
   else
      pnts = _mesa_copy_map_points2d(target, ustride, uorder,
                                  vstride, vorder, (GLdouble*) points);


   FLUSH_VERTICES(ctx, _NEW_EVAL);
   map->Uorder = uorder;
   map->u1 = u1;
   map->u2 = u2;
   map->du = 1.0F / (u2 - u1);
   map->Vorder = vorder;
   map->v1 = v1;
   map->v2 = v2;
   map->dv = 1.0F / (v2 - v1);
   if (map->Points)
      FREE( map->Points );
   map->Points = pnts;
}


void
_mesa_Map2f( GLenum target,
             GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
             GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
             const GLfloat *points)
{
   map2(target, u1, u2, ustride, uorder, v1, v2, vstride, vorder,
        points, GL_FLOAT);
}


void
_mesa_Map2d( GLenum target,
             GLdouble u1, GLdouble u2, GLint ustride, GLint uorder,
             GLdouble v1, GLdouble v2, GLint vstride, GLint vorder,
             const GLdouble *points )
{
   map2(target, (GLfloat) u1, (GLfloat) u2, ustride, uorder, 
	(GLfloat) v1, (GLfloat) v2, vstride, vorder, points, GL_DOUBLE);
}



void
_mesa_GetMapdv( GLenum target, GLenum query, GLdouble *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i, n;
   GLfloat *data;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (query) {
      case GL_COEFF:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       data = ctx->EvalMap.Map1Color4.Points;
	       n = ctx->EvalMap.Map1Color4.Order * 4;
	       break;
	    case GL_MAP1_INDEX:
	       data = ctx->EvalMap.Map1Index.Points;
	       n = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       data = ctx->EvalMap.Map1Normal.Points;
	       n = ctx->EvalMap.Map1Normal.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map1Texture1.Points;
	       n = ctx->EvalMap.Map1Texture1.Order * 1;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map1Texture2.Points;
	       n = ctx->EvalMap.Map1Texture2.Order * 2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map1Texture3.Points;
	       n = ctx->EvalMap.Map1Texture3.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map1Texture4.Points;
	       n = ctx->EvalMap.Map1Texture4.Order * 4;
	       break;
	    case GL_MAP1_VERTEX_3:
	       data = ctx->EvalMap.Map1Vertex3.Points;
	       n = ctx->EvalMap.Map1Vertex3.Order * 3;
	       break;
	    case GL_MAP1_VERTEX_4:
	       data = ctx->EvalMap.Map1Vertex4.Points;
	       n = ctx->EvalMap.Map1Vertex4.Order * 4;
	       break;
	    case GL_MAP2_COLOR_4:
	       data = ctx->EvalMap.Map2Color4.Points;
	       n = ctx->EvalMap.Map2Color4.Uorder
                 * ctx->EvalMap.Map2Color4.Vorder * 4;
	       break;
	    case GL_MAP2_INDEX:
	       data = ctx->EvalMap.Map2Index.Points;
	       n = ctx->EvalMap.Map2Index.Uorder
                 * ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       data = ctx->EvalMap.Map2Normal.Points;
	       n = ctx->EvalMap.Map2Normal.Uorder
                 * ctx->EvalMap.Map2Normal.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map2Texture1.Points;
	       n = ctx->EvalMap.Map2Texture1.Uorder
                 * ctx->EvalMap.Map2Texture1.Vorder * 1;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map2Texture2.Points;
	       n = ctx->EvalMap.Map2Texture2.Uorder
                 * ctx->EvalMap.Map2Texture2.Vorder * 2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map2Texture3.Points;
	       n = ctx->EvalMap.Map2Texture3.Uorder
                 * ctx->EvalMap.Map2Texture3.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map2Texture4.Points;
	       n = ctx->EvalMap.Map2Texture4.Uorder
                 * ctx->EvalMap.Map2Texture4.Vorder * 4;
	       break;
	    case GL_MAP2_VERTEX_3:
	       data = ctx->EvalMap.Map2Vertex3.Points;
	       n = ctx->EvalMap.Map2Vertex3.Uorder
                 * ctx->EvalMap.Map2Vertex3.Vorder * 3;
	       break;
	    case GL_MAP2_VERTEX_4:
	       data = ctx->EvalMap.Map2Vertex4.Points;
	       n = ctx->EvalMap.Map2Vertex4.Uorder
                 * ctx->EvalMap.Map2Vertex4.Vorder * 4;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	       return;
	 }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = data[i];
	    }
	 }
         break;
      case GL_ORDER:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       *v = ctx->EvalMap.Map1Color4.Order;
	       break;
	    case GL_MAP1_INDEX:
	       *v = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       *v = ctx->EvalMap.Map1Normal.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       *v = ctx->EvalMap.Map1Texture1.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       *v = ctx->EvalMap.Map1Texture2.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       *v = ctx->EvalMap.Map1Texture3.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       *v = ctx->EvalMap.Map1Texture4.Order;
	       break;
	    case GL_MAP1_VERTEX_3:
	       *v = ctx->EvalMap.Map1Vertex3.Order;
	       break;
	    case GL_MAP1_VERTEX_4:
	       *v = ctx->EvalMap.Map1Vertex4.Order;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.Uorder;
	       v[1] = ctx->EvalMap.Map2Color4.Vorder;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.Uorder;
	       v[1] = ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.Uorder;
	       v[1] = ctx->EvalMap.Map2Normal.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture1.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture2.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture3.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture4.Vorder;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex3.Vorder;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex4.Vorder;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	       return;
	 }
         break;
      case GL_DOMAIN:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       v[0] = ctx->EvalMap.Map1Color4.u1;
	       v[1] = ctx->EvalMap.Map1Color4.u2;
	       break;
	    case GL_MAP1_INDEX:
	       v[0] = ctx->EvalMap.Map1Index.u1;
	       v[1] = ctx->EvalMap.Map1Index.u2;
	       break;
	    case GL_MAP1_NORMAL:
	       v[0] = ctx->EvalMap.Map1Normal.u1;
	       v[1] = ctx->EvalMap.Map1Normal.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map1Texture1.u1;
	       v[1] = ctx->EvalMap.Map1Texture1.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map1Texture2.u1;
	       v[1] = ctx->EvalMap.Map1Texture2.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map1Texture3.u1;
	       v[1] = ctx->EvalMap.Map1Texture3.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map1Texture4.u1;
	       v[1] = ctx->EvalMap.Map1Texture4.u2;
	       break;
	    case GL_MAP1_VERTEX_3:
	       v[0] = ctx->EvalMap.Map1Vertex3.u1;
	       v[1] = ctx->EvalMap.Map1Vertex3.u2;
	       break;
	    case GL_MAP1_VERTEX_4:
	       v[0] = ctx->EvalMap.Map1Vertex4.u1;
	       v[1] = ctx->EvalMap.Map1Vertex4.u2;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.u1;
	       v[1] = ctx->EvalMap.Map2Color4.u2;
	       v[2] = ctx->EvalMap.Map2Color4.v1;
	       v[3] = ctx->EvalMap.Map2Color4.v2;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.u1;
	       v[1] = ctx->EvalMap.Map2Index.u2;
	       v[2] = ctx->EvalMap.Map2Index.v1;
	       v[3] = ctx->EvalMap.Map2Index.v2;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.u1;
	       v[1] = ctx->EvalMap.Map2Normal.u2;
	       v[2] = ctx->EvalMap.Map2Normal.v1;
	       v[3] = ctx->EvalMap.Map2Normal.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.u1;
	       v[1] = ctx->EvalMap.Map2Texture1.u2;
	       v[2] = ctx->EvalMap.Map2Texture1.v1;
	       v[3] = ctx->EvalMap.Map2Texture1.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.u1;
	       v[1] = ctx->EvalMap.Map2Texture2.u2;
	       v[2] = ctx->EvalMap.Map2Texture2.v1;
	       v[3] = ctx->EvalMap.Map2Texture2.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.u1;
	       v[1] = ctx->EvalMap.Map2Texture3.u2;
	       v[2] = ctx->EvalMap.Map2Texture3.v1;
	       v[3] = ctx->EvalMap.Map2Texture3.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.u1;
	       v[1] = ctx->EvalMap.Map2Texture4.u2;
	       v[2] = ctx->EvalMap.Map2Texture4.v1;
	       v[3] = ctx->EvalMap.Map2Texture4.v2;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.u1;
	       v[1] = ctx->EvalMap.Map2Vertex3.u2;
	       v[2] = ctx->EvalMap.Map2Vertex3.v1;
	       v[3] = ctx->EvalMap.Map2Vertex3.v2;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.u1;
	       v[1] = ctx->EvalMap.Map2Vertex4.u2;
	       v[2] = ctx->EvalMap.Map2Vertex4.v1;
	       v[3] = ctx->EvalMap.Map2Vertex4.v2;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	 }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapdv(query)" );
   }
}


void
_mesa_GetMapfv( GLenum target, GLenum query, GLfloat *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLint i, n;
   GLfloat *data;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (query) {
      case GL_COEFF:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       data = ctx->EvalMap.Map1Color4.Points;
	       n = ctx->EvalMap.Map1Color4.Order * 4;
	       break;
	    case GL_MAP1_INDEX:
	       data = ctx->EvalMap.Map1Index.Points;
	       n = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       data = ctx->EvalMap.Map1Normal.Points;
	       n = ctx->EvalMap.Map1Normal.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map1Texture1.Points;
	       n = ctx->EvalMap.Map1Texture1.Order * 1;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map1Texture2.Points;
	       n = ctx->EvalMap.Map1Texture2.Order * 2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map1Texture3.Points;
	       n = ctx->EvalMap.Map1Texture3.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map1Texture4.Points;
	       n = ctx->EvalMap.Map1Texture4.Order * 4;
	       break;
	    case GL_MAP1_VERTEX_3:
	       data = ctx->EvalMap.Map1Vertex3.Points;
	       n = ctx->EvalMap.Map1Vertex3.Order * 3;
	       break;
	    case GL_MAP1_VERTEX_4:
	       data = ctx->EvalMap.Map1Vertex4.Points;
	       n = ctx->EvalMap.Map1Vertex4.Order * 4;
	       break;
	    case GL_MAP2_COLOR_4:
	       data = ctx->EvalMap.Map2Color4.Points;
	       n = ctx->EvalMap.Map2Color4.Uorder
                 * ctx->EvalMap.Map2Color4.Vorder * 4;
	       break;
	    case GL_MAP2_INDEX:
	       data = ctx->EvalMap.Map2Index.Points;
	       n = ctx->EvalMap.Map2Index.Uorder
                 * ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       data = ctx->EvalMap.Map2Normal.Points;
	       n = ctx->EvalMap.Map2Normal.Uorder
                 * ctx->EvalMap.Map2Normal.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map2Texture1.Points;
	       n = ctx->EvalMap.Map2Texture1.Uorder
                 * ctx->EvalMap.Map2Texture1.Vorder * 1;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map2Texture2.Points;
	       n = ctx->EvalMap.Map2Texture2.Uorder
                 * ctx->EvalMap.Map2Texture2.Vorder * 2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map2Texture3.Points;
	       n = ctx->EvalMap.Map2Texture3.Uorder
                 * ctx->EvalMap.Map2Texture3.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map2Texture4.Points;
	       n = ctx->EvalMap.Map2Texture4.Uorder
                 * ctx->EvalMap.Map2Texture4.Vorder * 4;
	       break;
	    case GL_MAP2_VERTEX_3:
	       data = ctx->EvalMap.Map2Vertex3.Points;
	       n = ctx->EvalMap.Map2Vertex3.Uorder
                 * ctx->EvalMap.Map2Vertex3.Vorder * 3;
	       break;
	    case GL_MAP2_VERTEX_4:
	       data = ctx->EvalMap.Map2Vertex4.Points;
	       n = ctx->EvalMap.Map2Vertex4.Uorder
                 * ctx->EvalMap.Map2Vertex4.Vorder * 4;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	       return;
	 }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = data[i];
	    }
	 }
         break;
      case GL_ORDER:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       *v = (GLfloat) ctx->EvalMap.Map1Color4.Order;
	       break;
	    case GL_MAP1_INDEX:
	       *v = (GLfloat) ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       *v = (GLfloat) ctx->EvalMap.Map1Normal.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       *v = (GLfloat) ctx->EvalMap.Map1Texture1.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       *v = (GLfloat) ctx->EvalMap.Map1Texture2.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       *v = (GLfloat) ctx->EvalMap.Map1Texture3.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       *v = (GLfloat) ctx->EvalMap.Map1Texture4.Order;
	       break;
	    case GL_MAP1_VERTEX_3:
	       *v = (GLfloat) ctx->EvalMap.Map1Vertex3.Order;
	       break;
	    case GL_MAP1_VERTEX_4:
	       *v = (GLfloat) ctx->EvalMap.Map1Vertex4.Order;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Color4.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Color4.Vorder;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Index.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Normal.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Normal.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Texture1.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Texture1.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Texture2.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Texture2.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Texture3.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Texture3.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Texture4.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Texture4.Vorder;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Vertex3.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Vertex3.Vorder;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = (GLfloat) ctx->EvalMap.Map2Vertex4.Uorder;
	       v[1] = (GLfloat) ctx->EvalMap.Map2Vertex4.Vorder;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	       return;
	 }
         break;
      case GL_DOMAIN:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       v[0] = ctx->EvalMap.Map1Color4.u1;
	       v[1] = ctx->EvalMap.Map1Color4.u2;
	       break;
	    case GL_MAP1_INDEX:
	       v[0] = ctx->EvalMap.Map1Index.u1;
	       v[1] = ctx->EvalMap.Map1Index.u2;
	       break;
	    case GL_MAP1_NORMAL:
	       v[0] = ctx->EvalMap.Map1Normal.u1;
	       v[1] = ctx->EvalMap.Map1Normal.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map1Texture1.u1;
	       v[1] = ctx->EvalMap.Map1Texture1.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map1Texture2.u1;
	       v[1] = ctx->EvalMap.Map1Texture2.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map1Texture3.u1;
	       v[1] = ctx->EvalMap.Map1Texture3.u2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map1Texture4.u1;
	       v[1] = ctx->EvalMap.Map1Texture4.u2;
	       break;
	    case GL_MAP1_VERTEX_3:
	       v[0] = ctx->EvalMap.Map1Vertex3.u1;
	       v[1] = ctx->EvalMap.Map1Vertex3.u2;
	       break;
	    case GL_MAP1_VERTEX_4:
	       v[0] = ctx->EvalMap.Map1Vertex4.u1;
	       v[1] = ctx->EvalMap.Map1Vertex4.u2;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.u1;
	       v[1] = ctx->EvalMap.Map2Color4.u2;
	       v[2] = ctx->EvalMap.Map2Color4.v1;
	       v[3] = ctx->EvalMap.Map2Color4.v2;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.u1;
	       v[1] = ctx->EvalMap.Map2Index.u2;
	       v[2] = ctx->EvalMap.Map2Index.v1;
	       v[3] = ctx->EvalMap.Map2Index.v2;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.u1;
	       v[1] = ctx->EvalMap.Map2Normal.u2;
	       v[2] = ctx->EvalMap.Map2Normal.v1;
	       v[3] = ctx->EvalMap.Map2Normal.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.u1;
	       v[1] = ctx->EvalMap.Map2Texture1.u2;
	       v[2] = ctx->EvalMap.Map2Texture1.v1;
	       v[3] = ctx->EvalMap.Map2Texture1.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.u1;
	       v[1] = ctx->EvalMap.Map2Texture2.u2;
	       v[2] = ctx->EvalMap.Map2Texture2.v1;
	       v[3] = ctx->EvalMap.Map2Texture2.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.u1;
	       v[1] = ctx->EvalMap.Map2Texture3.u2;
	       v[2] = ctx->EvalMap.Map2Texture3.v1;
	       v[3] = ctx->EvalMap.Map2Texture3.v2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.u1;
	       v[1] = ctx->EvalMap.Map2Texture4.u2;
	       v[2] = ctx->EvalMap.Map2Texture4.v1;
	       v[3] = ctx->EvalMap.Map2Texture4.v2;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.u1;
	       v[1] = ctx->EvalMap.Map2Vertex3.u2;
	       v[2] = ctx->EvalMap.Map2Vertex3.v1;
	       v[3] = ctx->EvalMap.Map2Vertex3.v2;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.u1;
	       v[1] = ctx->EvalMap.Map2Vertex4.u2;
	       v[2] = ctx->EvalMap.Map2Vertex4.v1;
	       v[3] = ctx->EvalMap.Map2Vertex4.v2;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	 }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapfv(query)" );
   }
}


void
_mesa_GetMapiv( GLenum target, GLenum query, GLint *v )
{
   GET_CURRENT_CONTEXT(ctx);
   GLuint i, n;
   GLfloat *data;
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   switch (query) {
      case GL_COEFF:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       data = ctx->EvalMap.Map1Color4.Points;
	       n = ctx->EvalMap.Map1Color4.Order * 4;
	       break;
	    case GL_MAP1_INDEX:
	       data = ctx->EvalMap.Map1Index.Points;
	       n = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       data = ctx->EvalMap.Map1Normal.Points;
	       n = ctx->EvalMap.Map1Normal.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map1Texture1.Points;
	       n = ctx->EvalMap.Map1Texture1.Order * 1;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map1Texture2.Points;
	       n = ctx->EvalMap.Map1Texture2.Order * 2;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map1Texture3.Points;
	       n = ctx->EvalMap.Map1Texture3.Order * 3;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map1Texture4.Points;
	       n = ctx->EvalMap.Map1Texture4.Order * 4;
	       break;
	    case GL_MAP1_VERTEX_3:
	       data = ctx->EvalMap.Map1Vertex3.Points;
	       n = ctx->EvalMap.Map1Vertex3.Order * 3;
	       break;
	    case GL_MAP1_VERTEX_4:
	       data = ctx->EvalMap.Map1Vertex4.Points;
	       n = ctx->EvalMap.Map1Vertex4.Order * 4;
	       break;
	    case GL_MAP2_COLOR_4:
	       data = ctx->EvalMap.Map2Color4.Points;
	       n = ctx->EvalMap.Map2Color4.Uorder
                 * ctx->EvalMap.Map2Color4.Vorder * 4;
	       break;
	    case GL_MAP2_INDEX:
	       data = ctx->EvalMap.Map2Index.Points;
	       n = ctx->EvalMap.Map2Index.Uorder
                 * ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       data = ctx->EvalMap.Map2Normal.Points;
	       n = ctx->EvalMap.Map2Normal.Uorder
                 * ctx->EvalMap.Map2Normal.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       data = ctx->EvalMap.Map2Texture1.Points;
	       n = ctx->EvalMap.Map2Texture1.Uorder
                 * ctx->EvalMap.Map2Texture1.Vorder * 1;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       data = ctx->EvalMap.Map2Texture2.Points;
	       n = ctx->EvalMap.Map2Texture2.Uorder
                 * ctx->EvalMap.Map2Texture2.Vorder * 2;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       data = ctx->EvalMap.Map2Texture3.Points;
	       n = ctx->EvalMap.Map2Texture3.Uorder
                 * ctx->EvalMap.Map2Texture3.Vorder * 3;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       data = ctx->EvalMap.Map2Texture4.Points;
	       n = ctx->EvalMap.Map2Texture4.Uorder
                 * ctx->EvalMap.Map2Texture4.Vorder * 4;
	       break;
	    case GL_MAP2_VERTEX_3:
	       data = ctx->EvalMap.Map2Vertex3.Points;
	       n = ctx->EvalMap.Map2Vertex3.Uorder
                 * ctx->EvalMap.Map2Vertex3.Vorder * 3;
	       break;
	    case GL_MAP2_VERTEX_4:
	       data = ctx->EvalMap.Map2Vertex4.Points;
	       n = ctx->EvalMap.Map2Vertex4.Uorder
                 * ctx->EvalMap.Map2Vertex4.Vorder * 4;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	       return;
	 }
	 if (data) {
	    for (i=0;i<n;i++) {
	       v[i] = ROUNDF(data[i]);
	    }
	 }
         break;
      case GL_ORDER:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       *v = ctx->EvalMap.Map1Color4.Order;
	       break;
	    case GL_MAP1_INDEX:
	       *v = ctx->EvalMap.Map1Index.Order;
	       break;
	    case GL_MAP1_NORMAL:
	       *v = ctx->EvalMap.Map1Normal.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       *v = ctx->EvalMap.Map1Texture1.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       *v = ctx->EvalMap.Map1Texture2.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       *v = ctx->EvalMap.Map1Texture3.Order;
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       *v = ctx->EvalMap.Map1Texture4.Order;
	       break;
	    case GL_MAP1_VERTEX_3:
	       *v = ctx->EvalMap.Map1Vertex3.Order;
	       break;
	    case GL_MAP1_VERTEX_4:
	       *v = ctx->EvalMap.Map1Vertex4.Order;
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ctx->EvalMap.Map2Color4.Uorder;
	       v[1] = ctx->EvalMap.Map2Color4.Vorder;
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ctx->EvalMap.Map2Index.Uorder;
	       v[1] = ctx->EvalMap.Map2Index.Vorder;
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ctx->EvalMap.Map2Normal.Uorder;
	       v[1] = ctx->EvalMap.Map2Normal.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ctx->EvalMap.Map2Texture1.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture1.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ctx->EvalMap.Map2Texture2.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture2.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ctx->EvalMap.Map2Texture3.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture3.Vorder;
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ctx->EvalMap.Map2Texture4.Uorder;
	       v[1] = ctx->EvalMap.Map2Texture4.Vorder;
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ctx->EvalMap.Map2Vertex3.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex3.Vorder;
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ctx->EvalMap.Map2Vertex4.Uorder;
	       v[1] = ctx->EvalMap.Map2Vertex4.Vorder;
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	       return;
	 }
         break;
      case GL_DOMAIN:
	 switch (target) {
	    case GL_MAP1_COLOR_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Color4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Color4.u2);
	       break;
	    case GL_MAP1_INDEX:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Index.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Index.u2);
	       break;
	    case GL_MAP1_NORMAL:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Normal.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Normal.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_1:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture1.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture1.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_2:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture2.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture2.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture3.u2);
	       break;
	    case GL_MAP1_TEXTURE_COORD_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Texture4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Texture4.u2);
	       break;
	    case GL_MAP1_VERTEX_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Vertex3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Vertex3.u2);
	       break;
	    case GL_MAP1_VERTEX_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map1Vertex4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map1Vertex4.u2);
	       break;
	    case GL_MAP2_COLOR_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Color4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Color4.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Color4.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Color4.v2);
	       break;
	    case GL_MAP2_INDEX:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Index.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Index.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Index.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Index.v2);
	       break;
	    case GL_MAP2_NORMAL:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Normal.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Normal.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Normal.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Normal.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_1:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture1.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture1.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture1.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture1.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_2:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture2.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture2.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture2.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture2.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture3.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture3.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture3.v2);
	       break;
	    case GL_MAP2_TEXTURE_COORD_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Texture4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Texture4.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Texture4.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Texture4.v2);
	       break;
	    case GL_MAP2_VERTEX_3:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Vertex3.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Vertex3.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Vertex3.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Vertex3.v2);
	       break;
	    case GL_MAP2_VERTEX_4:
	       v[0] = ROUNDF(ctx->EvalMap.Map2Vertex4.u1);
	       v[1] = ROUNDF(ctx->EvalMap.Map2Vertex4.u2);
	       v[2] = ROUNDF(ctx->EvalMap.Map2Vertex4.v1);
	       v[3] = ROUNDF(ctx->EvalMap.Map2Vertex4.v2);
	       break;
	    default:
	       _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	 }
         break;
      default:
         _mesa_error( ctx, GL_INVALID_ENUM, "glGetMapiv(query)" );
   }
}



void
_mesa_MapGrid1f( GLint un, GLfloat u1, GLfloat u2 )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (un<1) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMapGrid1f" );
      return;
   }
   FLUSH_VERTICES(ctx, _NEW_EVAL);
   ctx->Eval.MapGrid1un = un;
   ctx->Eval.MapGrid1u1 = u1;
   ctx->Eval.MapGrid1u2 = u2;
   ctx->Eval.MapGrid1du = (u2 - u1) / (GLfloat) un;
}


void
_mesa_MapGrid1d( GLint un, GLdouble u1, GLdouble u2 )
{
   _mesa_MapGrid1f( un, (GLfloat) u1, (GLfloat) u2 );
}


void
_mesa_MapGrid2f( GLint un, GLfloat u1, GLfloat u2,
                 GLint vn, GLfloat v1, GLfloat v2 )
{
   GET_CURRENT_CONTEXT(ctx);
   ASSERT_OUTSIDE_BEGIN_END(ctx);

   if (un<1) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(un)" );
      return;
   }
   if (vn<1) {
      _mesa_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(vn)" );
      return;
   }

   FLUSH_VERTICES(ctx, _NEW_EVAL);
   ctx->Eval.MapGrid2un = un;
   ctx->Eval.MapGrid2u1 = u1;
   ctx->Eval.MapGrid2u2 = u2;
   ctx->Eval.MapGrid2du = (u2 - u1) / (GLfloat) un;
   ctx->Eval.MapGrid2vn = vn;
   ctx->Eval.MapGrid2v1 = v1;
   ctx->Eval.MapGrid2v2 = v2;
   ctx->Eval.MapGrid2dv = (v2 - v1) / (GLfloat) vn;
}


void
_mesa_MapGrid2d( GLint un, GLdouble u1, GLdouble u2,
                 GLint vn, GLdouble v1, GLdouble v2 )
{
   _mesa_MapGrid2f( un, (GLfloat) u1, (GLfloat) u2, 
		    vn, (GLfloat) v1, (GLfloat) v2 );
}
