/* $Id: eval.c,v 1.6 1999/11/08 15:30:05 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.1
 * 
 * Copyright (C) 1999  Brian Paul   All Rights Reserved.
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
#ifndef XFree86Server
#include <math.h>
#include <stdlib.h>
#include <string.h>
#else
#include "GL/xf86glx.h"
#endif
#include "context.h"
#include "eval.h"
#include "macros.h"
#include "mmath.h"
#include "types.h"
#include "vbcull.h"
#include "vbfill.h"
#include "vbxform.h"
#endif


static GLfloat inv_tab[MAX_EVAL_ORDER];

/*
 * Do one-time initialization for evaluators.
 */
void gl_init_eval( void )
{
  static int init_flag = 0;
  GLuint i;

  /* Compute a table of nCr (combination) values used by the
   * Bernstein polynomial generator.
   */

  /* KW: precompute 1/x for useful x.
   */
  if (init_flag==0) 
  { 
     for (i = 1 ; i < MAX_EVAL_ORDER ; i++)
	inv_tab[i] = 1.0 / i;
  }

  init_flag = 1;
}



/*
 * Horner scheme for Bezier curves
 * 
 * Bezier curves can be computed via a Horner scheme.
 * Horner is numerically less stable than the de Casteljau
 * algorithm, but it is faster. For curves of degree n 
 * the complexity of Horner is O(n) and de Casteljau is O(n^2).
 * Since stability is not important for displaying curve 
 * points I decided to use the Horner scheme.
 *
 * A cubic Bezier curve with control points b0, b1, b2, b3 can be 
 * written as
 *
 *        (([3]        [3]     )     [3]       )     [3]
 * c(t) = (([0]*s*b0 + [1]*t*b1)*s + [2]*t^2*b2)*s + [3]*t^2*b3
 *
 *                                           [n]
 * where s=1-t and the binomial coefficients [i]. These can 
 * be computed iteratively using the identity:
 *
 * [n]               [n  ]             [n]
 * [i] = (n-i+1)/i * [i-1]     and     [0] = 1
 */


static void
horner_bezier_curve(const GLfloat *cp, GLfloat *out, GLfloat t,
                    GLuint dim, GLuint order)
{
  GLfloat s, powert;
  GLuint i, k, bincoeff;

  if(order >= 2)
  { 
    bincoeff = order-1;
    s = 1.0-t;

    for(k=0; k<dim; k++)
      out[k] = s*cp[k] + bincoeff*t*cp[dim+k];

    for(i=2, cp+=2*dim, powert=t*t; i<order; i++, powert*=t, cp +=dim)
    {
      bincoeff *= order-i;
      bincoeff *= inv_tab[i];

      for(k=0; k<dim; k++)
        out[k] = s*out[k] + bincoeff*powert*cp[k];
    }
  }
  else /* order=1 -> constant curve */
  { 
    for(k=0; k<dim; k++)
      out[k] = cp[k];
  } 
}

/*
 * Tensor product Bezier surfaces
 *
 * Again the Horner scheme is used to compute a point on a 
 * TP Bezier surface. First a control polygon for a curve
 * on the surface in one parameter direction is computed,
 * then the point on the curve for the other parameter 
 * direction is evaluated.
 *
 * To store the curve control polygon additional storage
 * for max(uorder,vorder) points is needed in the 
 * control net cn.
 */

static void
horner_bezier_surf(GLfloat *cn, GLfloat *out, GLfloat u, GLfloat v,
                   GLuint dim, GLuint uorder, GLuint vorder)
{
  GLfloat *cp = cn + uorder*vorder*dim;
  GLuint i, uinc = vorder*dim;

  if(vorder > uorder)
  {
    if(uorder >= 2)
    { 
      GLfloat s, poweru;
      GLuint j, k, bincoeff;

      /* Compute the control polygon for the surface-curve in u-direction */
      for(j=0; j<vorder; j++)
      {
        GLfloat *ucp = &cn[j*dim];

        /* Each control point is the point for parameter u on a */ 
        /* curve defined by the control polygons in u-direction */
	bincoeff = uorder-1;
	s = 1.0-u;

	for(k=0; k<dim; k++)
	  cp[j*dim+k] = s*ucp[k] + bincoeff*u*ucp[uinc+k];

	for(i=2, ucp+=2*uinc, poweru=u*u; i<uorder; 
            i++, poweru*=u, ucp +=uinc)
	{
	  bincoeff *= uorder-i;
          bincoeff *= inv_tab[i];

	  for(k=0; k<dim; k++)
	    cp[j*dim+k] = s*cp[j*dim+k] + bincoeff*poweru*ucp[k];
	}
      }
        
      /* Evaluate curve point in v */
      horner_bezier_curve(cp, out, v, dim, vorder);
    }
    else /* uorder=1 -> cn defines a curve in v */
      horner_bezier_curve(cn, out, v, dim, vorder);
  }
  else /* vorder <= uorder */
  {
    if(vorder > 1)
    {
      GLuint i;

      /* Compute the control polygon for the surface-curve in u-direction */
      for(i=0; i<uorder; i++, cn += uinc)
      {
	/* For constant i all cn[i][j] (j=0..vorder) are located */
	/* on consecutive memory locations, so we can use        */
	/* horner_bezier_curve to compute the control points     */

	horner_bezier_curve(cn, &cp[i*dim], v, dim, vorder);
      }

      /* Evaluate curve point in u */
      horner_bezier_curve(cp, out, u, dim, uorder);
    }
    else  /* vorder=1 -> cn defines a curve in u */
      horner_bezier_curve(cn, out, u, dim, uorder);
  }
}

/*
 * The direct de Casteljau algorithm is used when a point on the
 * surface and the tangent directions spanning the tangent plane
 * should be computed (this is needed to compute normals to the
 * surface). In this case the de Casteljau algorithm approach is
 * nicer because a point and the partial derivatives can be computed 
 * at the same time. To get the correct tangent length du and dv
 * must be multiplied with the (u2-u1)/uorder-1 and (v2-v1)/vorder-1. 
 * Since only the directions are needed, this scaling step is omitted.
 *
 * De Casteljau needs additional storage for uorder*vorder
 * values in the control net cn.
 */

static void
de_casteljau_surf(GLfloat *cn, GLfloat *out, GLfloat *du, GLfloat *dv,
                  GLfloat u, GLfloat v, GLuint dim, 
                  GLuint uorder, GLuint vorder)
{
  GLfloat *dcn = cn + uorder*vorder*dim;
  GLfloat us = 1.0-u, vs = 1.0-v;
  GLuint h, i, j, k;
  GLuint minorder = uorder < vorder ? uorder : vorder;
  GLuint uinc = vorder*dim;
  GLuint dcuinc = vorder;
 
  /* Each component is evaluated separately to save buffer space  */
  /* This does not drasticaly decrease the performance of the     */
  /* algorithm. If additional storage for (uorder-1)*(vorder-1)   */
  /* points would be available, the components could be accessed  */
  /* in the innermost loop which could lead to less cache misses. */

#define CN(I,J,K) cn[(I)*uinc+(J)*dim+(K)] 
#define DCN(I, J) dcn[(I)*dcuinc+(J)]
  if(minorder < 3)
  {
    if(uorder==vorder)
    {
      for(k=0; k<dim; k++)
      {
	/* Derivative direction in u */
	du[k] = vs*(CN(1,0,k) - CN(0,0,k)) +
	         v*(CN(1,1,k) - CN(0,1,k));

	/* Derivative direction in v */
	dv[k] = us*(CN(0,1,k) - CN(0,0,k)) + 
	         u*(CN(1,1,k) - CN(1,0,k));

	/* bilinear de Casteljau step */
        out[k] =  us*(vs*CN(0,0,k) + v*CN(0,1,k)) +
	           u*(vs*CN(1,0,k) + v*CN(1,1,k));
      }
    }
    else if(minorder == uorder)
    {
      for(k=0; k<dim; k++)
      {
	/* bilinear de Casteljau step */
	DCN(1,0) =    CN(1,0,k) -   CN(0,0,k);
	DCN(0,0) = us*CN(0,0,k) + u*CN(1,0,k);

	for(j=0; j<vorder-1; j++)
	{
	  /* for the derivative in u */
	  DCN(1,j+1) =    CN(1,j+1,k) -   CN(0,j+1,k);
	  DCN(1,j)   = vs*DCN(1,j)    + v*DCN(1,j+1);

	  /* for the `point' */
	  DCN(0,j+1) = us*CN(0,j+1,k) + u*CN(1,j+1,k);
	  DCN(0,j)   = vs*DCN(0,j)    + v*DCN(0,j+1);
	}
        
	/* remaining linear de Casteljau steps until the second last step */
	for(h=minorder; h<vorder-1; h++)
	  for(j=0; j<vorder-h; j++)
	  {
	    /* for the derivative in u */
	    DCN(1,j) = vs*DCN(1,j) + v*DCN(1,j+1);

	    /* for the `point' */
	    DCN(0,j) = vs*DCN(0,j) + v*DCN(0,j+1);
	  }

	/* derivative direction in v */
	dv[k] = DCN(0,1) - DCN(0,0);

	/* derivative direction in u */
	du[k] =   vs*DCN(1,0) + v*DCN(1,1);

	/* last linear de Casteljau step */
	out[k] =  vs*DCN(0,0) + v*DCN(0,1);
      }
    }
    else /* minorder == vorder */
    {
      for(k=0; k<dim; k++)
      {
	/* bilinear de Casteljau step */
	DCN(0,1) =    CN(0,1,k) -   CN(0,0,k);
	DCN(0,0) = vs*CN(0,0,k) + v*CN(0,1,k);
	for(i=0; i<uorder-1; i++)
	{
	  /* for the derivative in v */
	  DCN(i+1,1) =    CN(i+1,1,k) -   CN(i+1,0,k);
	  DCN(i,1)   = us*DCN(i,1)    + u*DCN(i+1,1);

	  /* for the `point' */
	  DCN(i+1,0) = vs*CN(i+1,0,k) + v*CN(i+1,1,k);
	  DCN(i,0)   = us*DCN(i,0)    + u*DCN(i+1,0);
	}
        
	/* remaining linear de Casteljau steps until the second last step */
	for(h=minorder; h<uorder-1; h++)
	  for(i=0; i<uorder-h; i++)
	  {
	    /* for the derivative in v */
	    DCN(i,1) = us*DCN(i,1) + u*DCN(i+1,1);

	    /* for the `point' */
	    DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  }

	/* derivative direction in u */
	du[k] = DCN(1,0) - DCN(0,0);

	/* derivative direction in v */
	dv[k] =   us*DCN(0,1) + u*DCN(1,1);

	/* last linear de Casteljau step */
	out[k] =  us*DCN(0,0) + u*DCN(1,0);
      }
    }
  }
  else if(uorder == vorder)
  {
    for(k=0; k<dim; k++)
    {
      /* first bilinear de Casteljau step */
      for(i=0; i<uorder-1; i++)
      {
	DCN(i,0) = us*CN(i,0,k) + u*CN(i+1,0,k);
	for(j=0; j<vorder-1; j++)
	{
	  DCN(i,j+1) = us*CN(i,j+1,k) + u*CN(i+1,j+1,k);
	  DCN(i,j)   = vs*DCN(i,j)    + v*DCN(i,j+1);
	}
      }

      /* remaining bilinear de Casteljau steps until the second last step */
      for(h=2; h<minorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  for(j=0; j<vorder-h; j++)
	  {
	    DCN(i,j+1) = us*DCN(i,j+1) + u*DCN(i+1,j+1);
	    DCN(i,j)   = vs*DCN(i,j)   + v*DCN(i,j+1);
	  }
	}

      /* derivative direction in u */
      du[k] = vs*(DCN(1,0) - DCN(0,0)) +
	       v*(DCN(1,1) - DCN(0,1));

      /* derivative direction in v */
      dv[k] = us*(DCN(0,1) - DCN(0,0)) + 
	       u*(DCN(1,1) - DCN(1,0));

      /* last bilinear de Casteljau step */
      out[k] =  us*(vs*DCN(0,0) + v*DCN(0,1)) +
	         u*(vs*DCN(1,0) + v*DCN(1,1));
    }
  }
  else if(minorder == uorder)
  {
    for(k=0; k<dim; k++)
    {
      /* first bilinear de Casteljau step */
      for(i=0; i<uorder-1; i++)
      {
	DCN(i,0) = us*CN(i,0,k) + u*CN(i+1,0,k);
	for(j=0; j<vorder-1; j++)
	{
	  DCN(i,j+1) = us*CN(i,j+1,k) + u*CN(i+1,j+1,k);
	  DCN(i,j)   = vs*DCN(i,j)    + v*DCN(i,j+1);
	}
      }

      /* remaining bilinear de Casteljau steps until the second last step */
      for(h=2; h<minorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  for(j=0; j<vorder-h; j++)
	  {
	    DCN(i,j+1) = us*DCN(i,j+1) + u*DCN(i+1,j+1);
	    DCN(i,j)   = vs*DCN(i,j)   + v*DCN(i,j+1);
	  }
	}

      /* last bilinear de Casteljau step */
      DCN(2,0) =    DCN(1,0) -   DCN(0,0);
      DCN(0,0) = us*DCN(0,0) + u*DCN(1,0);
      for(j=0; j<vorder-1; j++)
      {
	/* for the derivative in u */
	DCN(2,j+1) =    DCN(1,j+1) -    DCN(0,j+1);
	DCN(2,j)   = vs*DCN(2,j)    + v*DCN(2,j+1);
	
	/* for the `point' */
	DCN(0,j+1) = us*DCN(0,j+1 ) + u*DCN(1,j+1);
	DCN(0,j)   = vs*DCN(0,j)    + v*DCN(0,j+1);
      }
        
      /* remaining linear de Casteljau steps until the second last step */
      for(h=minorder; h<vorder-1; h++)
	for(j=0; j<vorder-h; j++)
	{
	  /* for the derivative in u */
	  DCN(2,j) = vs*DCN(2,j) + v*DCN(2,j+1);
	  
	  /* for the `point' */
	  DCN(0,j) = vs*DCN(0,j) + v*DCN(0,j+1);
	}
      
      /* derivative direction in v */
      dv[k] = DCN(0,1) - DCN(0,0);
      
      /* derivative direction in u */
      du[k] =   vs*DCN(2,0) + v*DCN(2,1);
      
      /* last linear de Casteljau step */
      out[k] =  vs*DCN(0,0) + v*DCN(0,1);
    }
  }
  else /* minorder == vorder */
  {
    for(k=0; k<dim; k++)
    {
      /* first bilinear de Casteljau step */
      for(i=0; i<uorder-1; i++)
      {
	DCN(i,0) = us*CN(i,0,k) + u*CN(i+1,0,k);
	for(j=0; j<vorder-1; j++)
	{
	  DCN(i,j+1) = us*CN(i,j+1,k) + u*CN(i+1,j+1,k);
	  DCN(i,j)   = vs*DCN(i,j)    + v*DCN(i,j+1);
	}
      }

      /* remaining bilinear de Casteljau steps until the second last step */
      for(h=2; h<minorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	  for(j=0; j<vorder-h; j++)
	  {
	    DCN(i,j+1) = us*DCN(i,j+1) + u*DCN(i+1,j+1);
	    DCN(i,j)   = vs*DCN(i,j)   + v*DCN(i,j+1);
	  }
	}

      /* last bilinear de Casteljau step */
      DCN(0,2) =    DCN(0,1) -   DCN(0,0);
      DCN(0,0) = vs*DCN(0,0) + v*DCN(0,1);
      for(i=0; i<uorder-1; i++)
      {
	/* for the derivative in v */
	DCN(i+1,2) =    DCN(i+1,1)  -   DCN(i+1,0);
	DCN(i,2)   = us*DCN(i,2)    + u*DCN(i+1,2);
	
	/* for the `point' */
	DCN(i+1,0) = vs*DCN(i+1,0)  + v*DCN(i+1,1);
	DCN(i,0)   = us*DCN(i,0)    + u*DCN(i+1,0);
      }
      
      /* remaining linear de Casteljau steps until the second last step */
      for(h=minorder; h<uorder-1; h++)
	for(i=0; i<uorder-h; i++)
	{
	  /* for the derivative in v */
	  DCN(i,2) = us*DCN(i,2) + u*DCN(i+1,2);
	  
	  /* for the `point' */
	  DCN(i,0) = us*DCN(i,0) + u*DCN(i+1,0);
	}
      
      /* derivative direction in u */
      du[k] = DCN(1,0) - DCN(0,0);
      
      /* derivative direction in v */
      dv[k] =   us*DCN(0,2) + u*DCN(1,2);
      
      /* last linear de Casteljau step */
      out[k] =  us*DCN(0,0) + u*DCN(1,0);
    }
  }
#undef DCN
#undef CN
}

/*
 * Return the number of components per control point for any type of
 * evaluator.  Return 0 if bad target.
 */

static GLint components( GLenum target )
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
GLfloat *gl_copy_map_points1f( GLenum target,
                               GLint ustride, GLint uorder,
                               const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLint i, k, size = components(target);

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
GLfloat *gl_copy_map_points1d( GLenum target,
			        GLint ustride, GLint uorder,
			        const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLint i, k, size = components(target);

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
GLfloat *gl_copy_map_points2f( GLenum target,
			        GLint ustride, GLint uorder,
			        GLint vstride, GLint vorder,
			        const GLfloat *points )
{
   GLfloat *buffer, *p;
   GLint i, j, k, size, dsize, hsize;
   GLint uinc;

   size = components(target);

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
GLfloat *gl_copy_map_points2d(GLenum target,
                              GLint ustride, GLint uorder,
                              GLint vstride, GLint vorder,
                              const GLdouble *points )
{
   GLfloat *buffer, *p;
   GLint i, j, k, size, hsize, dsize;
   GLint uinc;

   size = components(target);

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


/*
 * This function is called by the display list deallocator function to
 * specify that a given set of control points are no longer needed.
 */
void gl_free_control_points( GLcontext* ctx, GLenum target, GLfloat *data )
{
   struct gl_1d_map *map1 = NULL;
   struct gl_2d_map *map2 = NULL;

   switch (target) {
      case GL_MAP1_VERTEX_3:
         map1 = &ctx->EvalMap.Map1Vertex3;
         break;
      case GL_MAP1_VERTEX_4:
         map1 = &ctx->EvalMap.Map1Vertex4;
	 break;
      case GL_MAP1_INDEX:
         map1 = &ctx->EvalMap.Map1Index;
         break;
      case GL_MAP1_COLOR_4:
         map1 = &ctx->EvalMap.Map1Color4;
         break;
      case GL_MAP1_NORMAL:
         map1 = &ctx->EvalMap.Map1Normal;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
         map1 = &ctx->EvalMap.Map1Texture1;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
         map1 = &ctx->EvalMap.Map1Texture2;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
         map1 = &ctx->EvalMap.Map1Texture3;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
         map1 = &ctx->EvalMap.Map1Texture4;
	 break;
      case GL_MAP2_VERTEX_3:
         map2 = &ctx->EvalMap.Map2Vertex3;
	 break;
      case GL_MAP2_VERTEX_4:
         map2 = &ctx->EvalMap.Map2Vertex4;
	 break;
      case GL_MAP2_INDEX:
         map2 = &ctx->EvalMap.Map2Index;
	 break;
      case GL_MAP2_COLOR_4:
         map2 = &ctx->EvalMap.Map2Color4;
         break;
      case GL_MAP2_NORMAL:
         map2 = &ctx->EvalMap.Map2Normal;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
         map2 = &ctx->EvalMap.Map2Texture1;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
         map2 = &ctx->EvalMap.Map2Texture2;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
         map2 = &ctx->EvalMap.Map2Texture3;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
         map2 = &ctx->EvalMap.Map2Texture4;
	 break;
      default:
	 gl_error( ctx, GL_INVALID_ENUM, "gl_free_control_points" );
         return;
   }

   if (map1) {
      if (data==map1->Points) {
         /* The control points in the display list are currently */
         /* being used so we can mark them as discard-able. */
         map1->Retain = GL_FALSE;
      }
      else {
         /* The control points in the display list are not currently */
         /* being used. */
         FREE( data );
      }
   }
   if (map2) {
      if (data==map2->Points) {
         /* The control points in the display list are currently */
         /* being used so we can mark them as discard-able. */
         map2->Retain = GL_FALSE;
      }
      else {
         /* The control points in the display list are not currently */
         /* being used. */
         FREE( data );
      }
   }

}



/**********************************************************************/
/***                      API entry points                          ***/
/**********************************************************************/


/*
 * Note that the array of control points must be 'unpacked' at this time.
 * Input:  retain - if TRUE, this control point data is also in a display
 *                  list and can't be freed until the list is freed.
 */
void gl_Map1f( GLcontext* ctx, GLenum target,
               GLfloat u1, GLfloat u2, GLint stride,
               GLint order, const GLfloat *points, GLboolean retain )
{
   GLint k;

   if (!points) {
      gl_error( ctx, GL_OUT_OF_MEMORY, "glMap1f" );
      return;
   }

   /* may be a new stride after copying control points */
   stride = components( target );

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glMap1");

   if (u1==u2) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap1(u1,u2)" );
      return;
   }

   if (order<1 || order>MAX_EVAL_ORDER) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap1(order)" );
      return;
   }

   k = components( target );
   if (k==0) {
      gl_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
   }

   if (stride < k) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap1(stride)" );
      return;
   }

   switch (target) {
      case GL_MAP1_VERTEX_3:
         ctx->EvalMap.Map1Vertex3.Order = order;
	 ctx->EvalMap.Map1Vertex3.u1 = u1;
	 ctx->EvalMap.Map1Vertex3.u2 = u2;
	 ctx->EvalMap.Map1Vertex3.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Vertex3.Points
             && !ctx->EvalMap.Map1Vertex3.Retain) {
	    FREE( ctx->EvalMap.Map1Vertex3.Points );
	 }
	 ctx->EvalMap.Map1Vertex3.Points = (GLfloat *) points;
         ctx->EvalMap.Map1Vertex3.Retain = retain;
	 break;
      case GL_MAP1_VERTEX_4:
         ctx->EvalMap.Map1Vertex4.Order = order;
	 ctx->EvalMap.Map1Vertex4.u1 = u1;
	 ctx->EvalMap.Map1Vertex4.u2 = u2;
	 ctx->EvalMap.Map1Vertex4.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Vertex4.Points
             && !ctx->EvalMap.Map1Vertex4.Retain) {
	    FREE( ctx->EvalMap.Map1Vertex4.Points );
	 }
	 ctx->EvalMap.Map1Vertex4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Vertex4.Retain = retain;
	 break;
      case GL_MAP1_INDEX:
         ctx->EvalMap.Map1Index.Order = order;
	 ctx->EvalMap.Map1Index.u1 = u1;
	 ctx->EvalMap.Map1Index.u2 = u2;
	 ctx->EvalMap.Map1Index.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Index.Points
             && !ctx->EvalMap.Map1Index.Retain) {
	    FREE( ctx->EvalMap.Map1Index.Points );
	 }
	 ctx->EvalMap.Map1Index.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Index.Retain = retain;
	 break;
      case GL_MAP1_COLOR_4:
         ctx->EvalMap.Map1Color4.Order = order;
	 ctx->EvalMap.Map1Color4.u1 = u1;
	 ctx->EvalMap.Map1Color4.u2 = u2;
	 ctx->EvalMap.Map1Color4.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Color4.Points
             && !ctx->EvalMap.Map1Color4.Retain) {
	    FREE( ctx->EvalMap.Map1Color4.Points );
	 }
	 ctx->EvalMap.Map1Color4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Color4.Retain = retain;
	 break;
      case GL_MAP1_NORMAL:
         ctx->EvalMap.Map1Normal.Order = order;
	 ctx->EvalMap.Map1Normal.u1 = u1;
	 ctx->EvalMap.Map1Normal.u2 = u2;
	 ctx->EvalMap.Map1Normal.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Normal.Points
             && !ctx->EvalMap.Map1Normal.Retain) {
	    FREE( ctx->EvalMap.Map1Normal.Points );
	 }
	 ctx->EvalMap.Map1Normal.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Normal.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_1:
         ctx->EvalMap.Map1Texture1.Order = order;
	 ctx->EvalMap.Map1Texture1.u1 = u1;
	 ctx->EvalMap.Map1Texture1.u2 = u2;
	 ctx->EvalMap.Map1Texture1.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Texture1.Points
             && !ctx->EvalMap.Map1Texture1.Retain) {
	    FREE( ctx->EvalMap.Map1Texture1.Points );
	 }
	 ctx->EvalMap.Map1Texture1.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture1.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_2:
         ctx->EvalMap.Map1Texture2.Order = order;
	 ctx->EvalMap.Map1Texture2.u1 = u1;
	 ctx->EvalMap.Map1Texture2.u2 = u2;
	 ctx->EvalMap.Map1Texture2.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Texture2.Points
             && !ctx->EvalMap.Map1Texture2.Retain) {
	    FREE( ctx->EvalMap.Map1Texture2.Points );
	 }
	 ctx->EvalMap.Map1Texture2.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture2.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_3:
         ctx->EvalMap.Map1Texture3.Order = order;
	 ctx->EvalMap.Map1Texture3.u1 = u1;
	 ctx->EvalMap.Map1Texture3.u2 = u2;
	 ctx->EvalMap.Map1Texture3.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Texture3.Points
             && !ctx->EvalMap.Map1Texture3.Retain) {
	    FREE( ctx->EvalMap.Map1Texture3.Points );
	 }
	 ctx->EvalMap.Map1Texture3.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture3.Retain = retain;
	 break;
      case GL_MAP1_TEXTURE_COORD_4:
         ctx->EvalMap.Map1Texture4.Order = order;
	 ctx->EvalMap.Map1Texture4.u1 = u1;
	 ctx->EvalMap.Map1Texture4.u2 = u2;
	 ctx->EvalMap.Map1Texture4.du = 1.0 / (u2 - u1);
	 if (ctx->EvalMap.Map1Texture4.Points
             && !ctx->EvalMap.Map1Texture4.Retain) {
	    FREE( ctx->EvalMap.Map1Texture4.Points );
	 }
	 ctx->EvalMap.Map1Texture4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map1Texture4.Retain = retain;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMap1(target)" );
   }
}




/*
 * Note that the array of control points must be 'unpacked' at this time.
 * Input:  retain - if TRUE, this control point data is also in a display
 *                  list and can't be freed until the list is freed.
 */
void gl_Map2f( GLcontext* ctx, GLenum target,
	      GLfloat u1, GLfloat u2, GLint ustride, GLint uorder,
	      GLfloat v1, GLfloat v2, GLint vstride, GLint vorder,
	      const GLfloat *points, GLboolean retain )
{
   GLint k;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glMap2");

   if (u1==u2) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(u1,u2)" );
      return;
   }

   if (v1==v2) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(v1,v2)" );
      return;
   }

   if (uorder<1 || uorder>MAX_EVAL_ORDER) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(uorder)" );
      return;
   }

   if (vorder<1 || vorder>MAX_EVAL_ORDER) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(vorder)" );
      return;
   }

   k = components( target );
   if (k==0) {
      gl_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
   }

   if (ustride < k) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(ustride)" );
      return;
   }
   if (vstride < k) {
      gl_error( ctx, GL_INVALID_VALUE, "glMap2(vstride)" );
      return;
   }

   switch (target) {
      case GL_MAP2_VERTEX_3:
         ctx->EvalMap.Map2Vertex3.Uorder = uorder;
	 ctx->EvalMap.Map2Vertex3.u1 = u1;
	 ctx->EvalMap.Map2Vertex3.u2 = u2;
	 ctx->EvalMap.Map2Vertex3.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Vertex3.Vorder = vorder;
	 ctx->EvalMap.Map2Vertex3.v1 = v1;
	 ctx->EvalMap.Map2Vertex3.v2 = v2;
	 ctx->EvalMap.Map2Vertex3.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Vertex3.Points
             && !ctx->EvalMap.Map2Vertex3.Retain) {
	    FREE( ctx->EvalMap.Map2Vertex3.Points );
	 }
	 ctx->EvalMap.Map2Vertex3.Retain = retain;
	 ctx->EvalMap.Map2Vertex3.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_VERTEX_4:
         ctx->EvalMap.Map2Vertex4.Uorder = uorder;
	 ctx->EvalMap.Map2Vertex4.u1 = u1;
	 ctx->EvalMap.Map2Vertex4.u2 = u2;
	 ctx->EvalMap.Map2Vertex4.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Vertex4.Vorder = vorder;
	 ctx->EvalMap.Map2Vertex4.v1 = v1;
	 ctx->EvalMap.Map2Vertex4.v2 = v2;
	 ctx->EvalMap.Map2Vertex4.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Vertex4.Points
             && !ctx->EvalMap.Map2Vertex4.Retain) {
	    FREE( ctx->EvalMap.Map2Vertex4.Points );
	 }
	 ctx->EvalMap.Map2Vertex4.Points = (GLfloat *) points;
	 ctx->EvalMap.Map2Vertex4.Retain = retain;
	 break;
      case GL_MAP2_INDEX:
         ctx->EvalMap.Map2Index.Uorder = uorder;
	 ctx->EvalMap.Map2Index.u1 = u1;
	 ctx->EvalMap.Map2Index.u2 = u2;
	 ctx->EvalMap.Map2Index.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Index.Vorder = vorder;
	 ctx->EvalMap.Map2Index.v1 = v1;
	 ctx->EvalMap.Map2Index.v2 = v2;
	 ctx->EvalMap.Map2Index.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Index.Points
             && !ctx->EvalMap.Map2Index.Retain) {
	    FREE( ctx->EvalMap.Map2Index.Points );
	 }
	 ctx->EvalMap.Map2Index.Retain = retain;
	 ctx->EvalMap.Map2Index.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_COLOR_4:
         ctx->EvalMap.Map2Color4.Uorder = uorder;
	 ctx->EvalMap.Map2Color4.u1 = u1;
	 ctx->EvalMap.Map2Color4.u2 = u2;
	 ctx->EvalMap.Map2Color4.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Color4.Vorder = vorder;
	 ctx->EvalMap.Map2Color4.v1 = v1;
	 ctx->EvalMap.Map2Color4.v2 = v2;
	 ctx->EvalMap.Map2Color4.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Color4.Points
             && !ctx->EvalMap.Map2Color4.Retain) {
	    FREE( ctx->EvalMap.Map2Color4.Points );
	 }
	 ctx->EvalMap.Map2Color4.Retain = retain;
	 ctx->EvalMap.Map2Color4.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_NORMAL:
         ctx->EvalMap.Map2Normal.Uorder = uorder;
	 ctx->EvalMap.Map2Normal.u1 = u1;
	 ctx->EvalMap.Map2Normal.u2 = u2;
	 ctx->EvalMap.Map2Normal.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Normal.Vorder = vorder;
	 ctx->EvalMap.Map2Normal.v1 = v1;
	 ctx->EvalMap.Map2Normal.v2 = v2;
	 ctx->EvalMap.Map2Normal.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Normal.Points
             && !ctx->EvalMap.Map2Normal.Retain) {
	    FREE( ctx->EvalMap.Map2Normal.Points );
	 }
	 ctx->EvalMap.Map2Normal.Retain = retain;
	 ctx->EvalMap.Map2Normal.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_1:
         ctx->EvalMap.Map2Texture1.Uorder = uorder;
	 ctx->EvalMap.Map2Texture1.u1 = u1;
	 ctx->EvalMap.Map2Texture1.u2 = u2;
	 ctx->EvalMap.Map2Texture1.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Texture1.Vorder = vorder;
	 ctx->EvalMap.Map2Texture1.v1 = v1;
	 ctx->EvalMap.Map2Texture1.v2 = v2;
	 ctx->EvalMap.Map2Texture1.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Texture1.Points
             && !ctx->EvalMap.Map2Texture1.Retain) {
	    FREE( ctx->EvalMap.Map2Texture1.Points );
	 }
	 ctx->EvalMap.Map2Texture1.Retain = retain;
	 ctx->EvalMap.Map2Texture1.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_2:
         ctx->EvalMap.Map2Texture2.Uorder = uorder;
	 ctx->EvalMap.Map2Texture2.u1 = u1;
	 ctx->EvalMap.Map2Texture2.u2 = u2;
	 ctx->EvalMap.Map2Texture2.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Texture2.Vorder = vorder;
	 ctx->EvalMap.Map2Texture2.v1 = v1;
	 ctx->EvalMap.Map2Texture2.v2 = v2;
	 ctx->EvalMap.Map2Texture2.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Texture2.Points
             && !ctx->EvalMap.Map2Texture2.Retain) {
	    FREE( ctx->EvalMap.Map2Texture2.Points );
	 }
	 ctx->EvalMap.Map2Texture2.Retain = retain;
	 ctx->EvalMap.Map2Texture2.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_3:
         ctx->EvalMap.Map2Texture3.Uorder = uorder;
	 ctx->EvalMap.Map2Texture3.u1 = u1;
	 ctx->EvalMap.Map2Texture3.u2 = u2;
	 ctx->EvalMap.Map2Texture3.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Texture3.Vorder = vorder;
	 ctx->EvalMap.Map2Texture3.v1 = v1;
	 ctx->EvalMap.Map2Texture3.v2 = v2;
	 ctx->EvalMap.Map2Texture3.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Texture3.Points
             && !ctx->EvalMap.Map2Texture3.Retain) {
	    FREE( ctx->EvalMap.Map2Texture3.Points );
	 }
	 ctx->EvalMap.Map2Texture3.Retain = retain;
	 ctx->EvalMap.Map2Texture3.Points = (GLfloat *) points;
	 break;
      case GL_MAP2_TEXTURE_COORD_4:
         ctx->EvalMap.Map2Texture4.Uorder = uorder;
	 ctx->EvalMap.Map2Texture4.u1 = u1;
	 ctx->EvalMap.Map2Texture4.u2 = u2;
	 ctx->EvalMap.Map2Texture4.du = 1.0 / (u2 - u1);
         ctx->EvalMap.Map2Texture4.Vorder = vorder;
	 ctx->EvalMap.Map2Texture4.v1 = v1;
	 ctx->EvalMap.Map2Texture4.v2 = v2;
	 ctx->EvalMap.Map2Texture4.dv = 1.0 / (v2 - v1);
	 if (ctx->EvalMap.Map2Texture4.Points
             && !ctx->EvalMap.Map2Texture4.Retain) {
	    FREE( ctx->EvalMap.Map2Texture4.Points );
	 }
	 ctx->EvalMap.Map2Texture4.Retain = retain;
	 ctx->EvalMap.Map2Texture4.Points = (GLfloat *) points;
	 break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glMap2(target)" );
   }
}


   


void gl_GetMapdv( GLcontext* ctx, GLenum target, GLenum query, GLdouble *v )
{
   GLint i, n;
   GLfloat *data;

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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(target)" );
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMapdv(query)" );
   }
}


void gl_GetMapfv( GLcontext* ctx, GLenum target, GLenum query, GLfloat *v )
{
   GLint i, n;
   GLfloat *data;

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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(target)" );
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMapfv(query)" );
   }
}


void gl_GetMapiv( GLcontext* ctx, GLenum target, GLenum query, GLint *v )
{
   GLuint i, n;
   GLfloat *data;

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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
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
	       gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(target)" );
	 }
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glGetMapiv(query)" );
   }
}



static void eval_points1( GLfloat outcoord[][4], 
			  GLfloat coord[][4],
			  const GLuint *flags,
			  GLuint start,
			  GLfloat du, GLfloat u1 )
{
   GLuint i;
   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & VERT_EVAL_P1) 
	 outcoord[i][0] = coord[i][0] * du + u1;
      else if (flags[i] & VERT_EVAL_ANY) {
	 outcoord[i][0] = coord[i][0];
	 outcoord[i][1] = coord[i][1];
      }
}

static void eval_points2( GLfloat outcoord[][4], 
			  GLfloat coord[][4],
			  const GLuint *flags,
			  GLuint start,
			  GLfloat du, GLfloat u1,
			  GLfloat dv, GLfloat v1 )
{
   GLuint i;
   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & VERT_EVAL_P2) {
	 outcoord[i][0] = coord[i][0] * du + u1;
	 outcoord[i][1] = coord[i][1] * dv + v1;
      } else if (flags[i] & VERT_EVAL_ANY) {
	 outcoord[i][0] = coord[i][0];
	 outcoord[i][1] = coord[i][1];
      }
}


static const GLubyte dirty_flags[5] = {
   0,				/* not possible */
   VEC_DIRTY_0,
   VEC_DIRTY_1, 
   VEC_DIRTY_2, 
   VEC_DIRTY_3
};


static GLvector4f *eval1_4f( GLvector4f *dest, 
			     GLfloat coord[][4], 
			     const GLuint *flags,
			     GLuint start,
			     GLuint dimension,
			     struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLfloat (*to)[4] = dest->data;
   GLuint i;
   
   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 ASSIGN_4V(to[i], 0,0,0,1);
	 horner_bezier_curve(map->Points, to[i], u, dimension, map->Order);
      }

   dest->count = i;
   dest->start = VEC_ELT(dest, GLfloat, start);
   dest->size = MAX2(dest->size, dimension);
   dest->flags |= dirty_flags[dimension];
   return dest;
}


static GLvector1ui *eval1_1ui( GLvector1ui *dest, 
			       GLfloat coord[][4], 
			       const GLuint *flags,
			       GLuint start,
			       struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLuint *to = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat tmp;
	 horner_bezier_curve(map->Points, &tmp, u, 1, map->Order);
	 to[i] = (GLuint) (GLint) tmp;
      }

   dest->start = VEC_ELT(dest, GLuint, start);
   dest->count = i;
   return dest;
}

static GLvector3f *eval1_norm( GLvector3f *dest, 
			       GLfloat coord[][4],
			       GLuint *flags, /* not const */
			       GLuint start,
			       struct gl_1d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLfloat (*to)[3] = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 horner_bezier_curve(map->Points, to[i], u, 3, map->Order);
	 flags[i+1] |= VERT_NORM; /* reset */
      }

   dest->start = VEC_ELT(dest, GLfloat, start);
   dest->count = i;
   return dest;
}

static GLvector4ub *eval1_color( GLvector4ub *dest, 
				 GLfloat coord[][4],
				 GLuint *flags, /* not const */
				 GLuint start,
				 struct gl_1d_map *map )
{   
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   GLubyte (*to)[4] = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C1|VERT_EVAL_P1)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat fcolor[4];
	 horner_bezier_curve(map->Points, fcolor, u, 4, map->Order);
	 FLOAT_RGBA_TO_UBYTE_RGBA(to[i], fcolor);
	 flags[i+1] |= VERT_RGBA; /* reset */
      }

   dest->start = VEC_ELT(dest, GLubyte, start);
   dest->count = i;
   return dest;
}




static GLvector4f *eval2_obj_norm( GLvector4f *obj_ptr, 
				   GLvector3f *norm_ptr,
				   GLfloat coord[][4], 
				   GLuint *flags, 
				   GLuint start,
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
   
   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 GLfloat du[4], dv[4];

	 ASSIGN_4V(obj[i], 0,0,0,1);
	 de_casteljau_surf(map->Points, obj[i], du, dv, u, v, dimension,
			   map->Uorder, map->Vorder);
	       
	 CROSS3(normal[i], du, dv);
	 NORMALIZE_3FV(normal[i]);
	 flags[i+1] |= VERT_NORM;
      }
 
   obj_ptr->start = VEC_ELT(obj_ptr, GLfloat, start);
   obj_ptr->count = i;
   obj_ptr->size = MAX2(obj_ptr->size, dimension);
   obj_ptr->flags |= dirty_flags[dimension];
   return obj_ptr;
}


static GLvector4f *eval2_4f( GLvector4f *dest, 
			     GLfloat coord[][4], 
			     const GLuint *flags,
			     GLuint start,
			     GLuint dimension,
			     struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLfloat (*to)[4] = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 horner_bezier_surf(map->Points, to[i], u, v, dimension,
			    map->Uorder, map->Vorder);
      }

   dest->start = VEC_ELT(dest, GLfloat, start);
   dest->count = i;
   dest->size = MAX2(dest->size, dimension);
   dest->flags |= dirty_flags[dimension];
   return dest;
}


static GLvector3f *eval2_norm( GLvector3f *dest, 
			       GLfloat coord[][4], 
			       GLuint *flags, 
			       GLuint start,
			       struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLfloat (*to)[3] = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 horner_bezier_surf(map->Points, to[i], u, v, 3,
			    map->Uorder, map->Vorder);
 	 flags[i+1] |= VERT_NORM; /* reset */
     }

   dest->start = VEC_ELT(dest, GLfloat, start);
   dest->count = i;
   return dest;
}


static GLvector1ui *eval2_1ui( GLvector1ui *dest, 
			       GLfloat coord[][4], 
			       const GLuint *flags,
			       GLuint start,
			       struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLuint *to = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 GLfloat tmp;
	 horner_bezier_surf(map->Points, &tmp, u, v, 1,
			    map->Uorder, map->Vorder);

	 to[i] = (GLuint) (GLint) tmp;
      }

   dest->start = VEC_ELT(dest, GLuint, start);
   dest->count = i;
   return dest;
}



static GLvector4ub *eval2_color( GLvector4ub *dest,
				 GLfloat coord[][4], 
				 GLuint *flags,
				 GLuint start,
				 struct gl_2d_map *map )
{
   const GLfloat u1 = map->u1;
   const GLfloat du = map->du;
   const GLfloat v1 = map->v1;
   const GLfloat dv = map->dv;
   GLubyte (*to)[4] = dest->data;
   GLuint i;

   for (i = start ; !(flags[i] & VERT_END_VB) ; i++)
      if (flags[i] & (VERT_EVAL_C2|VERT_EVAL_P2)) {
	 GLfloat u = (coord[i][0] - u1) * du;
	 GLfloat v = (coord[i][1] - v1) * dv;
	 GLfloat fcolor[4];
	 horner_bezier_surf(map->Points, fcolor, u, v, 4,
			    map->Uorder, map->Vorder);
	 FLOAT_RGBA_TO_UBYTE_RGBA(to[i], fcolor);
	 flags[i+1] |= VERT_RGBA; /* reset */
      }

   dest->start = VEC_ELT(dest, GLubyte, start);
   dest->count = i;
   return dest;
}


static GLvector4f *copy_4f( GLvector4f *out, CONST GLvector4f *in, 
			    const GLuint *flags,
			    GLuint start )
{
   GLfloat (*to)[4] = out->data;
   GLfloat (*from)[4] = in->data;
   GLuint i;
   
   for ( i = start ; !(flags[i] & VERT_END_VB) ; i++) 
      if (!(flags[i] & VERT_EVAL_ANY)) 
	 COPY_4FV( to[i], from[i] );
   
   out->start = VEC_ELT(out, GLfloat, start);
   return out;
}

static GLvector3f *copy_3f( GLvector3f *out, CONST GLvector3f *in, 
			    const GLuint *flags,
			    GLuint start )
{
   GLfloat (*to)[3] = out->data;
   GLfloat (*from)[3] = in->data;
   GLuint i;
   
   for ( i = start ; !(flags[i] & VERT_END_VB) ; i++) 
      if (!(flags[i] & VERT_EVAL_ANY)) 
	 COPY_3V( to[i], from[i] );
   
   out->start = VEC_ELT(out, GLfloat, start);
   return out;
}

static GLvector4ub *copy_4ub( GLvector4ub *out, 
			      CONST GLvector4ub *in, 
			      const GLuint *flags,
			      GLuint start )
{
   GLubyte (*to)[4] = out->data;
   GLubyte (*from)[4] = in->data;
   GLuint i;
   
   for ( i = start ; !(flags[i] & VERT_END_VB) ; i++) 
      if (!(flags[i] & VERT_EVAL_ANY)) 
	 COPY_4UBV( to[i], from[i] );

   out->start = VEC_ELT(out, GLubyte, start);
   return out;
}

static GLvector1ui *copy_1ui( GLvector1ui *out, 
			      CONST GLvector1ui *in, 
			      const GLuint *flags,
			      GLuint start )
{
   GLuint *to = out->data;
   CONST GLuint *from = in->data;
   GLuint i;
   
   for ( i = start ; !(flags[i] & VERT_END_VB) ; i++) 
      if (!(flags[i] & VERT_EVAL_ANY)) 
	 to[i] = from[i];

   out->start = VEC_ELT(out, GLuint, start);
   return out;
}


/* KW: Rewrote this to perform eval on a whole buffer at once.
 *     Only evaluates active data items, and avoids scribbling
 *     the source buffer if we are running from a display list.
 *
 *     If the user (in this case looser) sends eval coordinates
 *     or runs a display list containing eval coords with no
 *     vertex maps enabled, we have to either copy all non-eval
 *     data to a new buffer, or find a way of working around
 *     the eval data.  I choose the second option.
 *
 * KW: This code not reached by cva - use IM to access storage.
 */
void gl_eval_vb( struct vertex_buffer *VB )
{
   struct immediate *IM = VB->IM;
   GLcontext *ctx = VB->ctx;
   GLuint req = ctx->CVA.elt.inputs;
   GLfloat (*coord)[4] = VB->ObjPtr->data;
   GLuint *flags = VB->Flag;
   GLuint new_flags = 0;
   

   GLuint any_eval1 = VB->OrFlag & (VERT_EVAL_C1|VERT_EVAL_P1);
   GLuint any_eval2 = VB->OrFlag & (VERT_EVAL_C2|VERT_EVAL_P2);
   GLuint all_eval = IM->AndFlag & VERT_EVAL_ANY;

   /* Handle the degenerate cases.
    */
   if (any_eval1 && !ctx->Eval.Map1Vertex4 && !ctx->Eval.Map1Vertex3) {
      VB->PurgeFlags |= (VERT_EVAL_C1|VERT_EVAL_P1);
      VB->EarlyCull = 0;
      any_eval1 = GL_FALSE;
   }
  
   if (any_eval2 && !ctx->Eval.Map2Vertex4 && !ctx->Eval.Map2Vertex3) {
      VB->PurgeFlags |= (VERT_EVAL_C2|VERT_EVAL_P2);
      VB->EarlyCull = 0;
      any_eval2 = GL_FALSE;
   }

   /* KW: This really is a degenerate case - doing this disables
    * culling, and causes dummy values for the missing vertices to be
    * transformed and clip tested.  It also forces the individual
    * cliptesting of each primitive in vb_render.  I wish there was a
    * nice alternative, but I can't say I want to put effort into
    * optimizing such a bad usage of the library - I'd much rather
    * work on useful changes.
    */
   if (VB->PurgeFlags) {
      if (!any_eval1 && !any_eval2 && all_eval) VB->Count = VB->Start;
      gl_purge_vertices( VB );
      if (!any_eval1 && !any_eval2) return;
   } else
      VB->IndirectCount = VB->Count;

   /* Translate points into coords.
    */
   if (any_eval1 && (VB->OrFlag & VERT_EVAL_P1)) 
   {
      eval_points1( IM->Obj, coord, flags, IM->Start,
		    ctx->Eval.MapGrid1du,
		    ctx->Eval.MapGrid1u1);

      coord = IM->Obj;
   }

   if (any_eval2 && (VB->OrFlag & VERT_EVAL_P2)) 
   {
      eval_points2( IM->Obj, coord, flags, IM->Start,
		    ctx->Eval.MapGrid2du,
		    ctx->Eval.MapGrid2u1,
		    ctx->Eval.MapGrid2dv,
		    ctx->Eval.MapGrid2v1 );

      coord = IM->Obj;
   }

   /* Perform the evaluations on active data elements.
    */
   if (req & VERT_INDEX) 
   {
      GLvector1ui  *in_index = VB->IndexPtr;
      GLvector1ui  *out_index = &IM->v.Index;

      if (ctx->Eval.Map1Index && any_eval1) 
	 VB->IndexPtr = eval1_1ui( out_index, coord, flags, IM->Start,
				   &ctx->EvalMap.Map1Index );
      
      if (ctx->Eval.Map2Index && any_eval2)
	 VB->IndexPtr = eval2_1ui( out_index, coord, flags, IM->Start,
				   &ctx->EvalMap.Map2Index );
	 
      if (VB->IndexPtr != in_index) {
	 new_flags |= VERT_INDEX;
	 if (!all_eval)
	    VB->IndexPtr = copy_1ui( out_index, in_index, flags, IM->Start );
      }
   }

   if (req & VERT_RGBA) 
   {   
      GLvector4ub  *in_color = VB->ColorPtr;
      GLvector4ub  *out_color = &IM->v.Color;

      if (ctx->Eval.Map1Color4 && any_eval1) 
	 VB->ColorPtr = eval1_color( out_color, coord, flags, IM->Start,
				   &ctx->EvalMap.Map1Color4 );
      
      if (ctx->Eval.Map2Color4 && any_eval2)
	 VB->ColorPtr = eval2_color( out_color, coord, flags, IM->Start,
				     &ctx->EvalMap.Map2Color4 );
	 
      if (VB->ColorPtr != in_color) {
	 new_flags |= VERT_RGBA;
	 if (!all_eval)
	    VB->ColorPtr = copy_4ub( out_color, in_color, flags, IM->Start );
      }

      VB->Color[0] = VB->Color[1] = VB->ColorPtr;
   }


   if (req & VERT_NORM) 
   {   
      GLvector3f  *in_normal = VB->NormalPtr;
      GLvector3f  *out_normal = &IM->v.Normal;

      if (ctx->Eval.Map1Normal && any_eval1) 
	 VB->NormalPtr = eval1_norm( out_normal, coord, flags, IM->Start,
				     &ctx->EvalMap.Map1Normal );
      
      if (ctx->Eval.Map2Normal && any_eval2)
	 VB->NormalPtr = eval2_norm( out_normal, coord, flags, IM->Start,
				     &ctx->EvalMap.Map2Normal );
	 
      if (VB->NormalPtr != in_normal) {
	 new_flags |= VERT_NORM;
	 if (!all_eval)
	    VB->NormalPtr = copy_3f( out_normal, in_normal, flags, IM->Start );
      }
   }

     
   if (req & VERT_TEX_ANY(0)) 
   {
      GLvector4f *tc = VB->TexCoordPtr[0];
      GLvector4f *in = tc;
      GLvector4f *out = &IM->v.TexCoord[0];

      if (any_eval1) {
	 if (ctx->Eval.Map1TextureCoord4) 
	    tc = eval1_4f( out, coord, flags, IM->Start, 
			   4, &ctx->EvalMap.Map1Texture4);
	 else if (ctx->Eval.Map1TextureCoord3) 
	    tc = eval1_4f( out, coord, flags, IM->Start, 3,
			   &ctx->EvalMap.Map1Texture3);
	 else if (ctx->Eval.Map1TextureCoord2) 
	    tc = eval1_4f( out, coord, flags, IM->Start, 2,
			   &ctx->EvalMap.Map1Texture2);
	 else if (ctx->Eval.Map1TextureCoord1) 
	    tc = eval1_4f( out, coord, flags, IM->Start, 1,
			   &ctx->EvalMap.Map1Texture1);
      }

      if (any_eval2) {
	 if (ctx->Eval.Map2TextureCoord4) 
	    tc = eval2_4f( out, coord, flags, IM->Start,
			   4, &ctx->EvalMap.Map2Texture4);
	 else if (ctx->Eval.Map2TextureCoord3) 
	    tc = eval2_4f( out, coord, flags, IM->Start,
			   3, &ctx->EvalMap.Map2Texture3);
	 else if (ctx->Eval.Map2TextureCoord2) 
	    tc = eval2_4f( out, coord, flags, IM->Start,
			   2, &ctx->EvalMap.Map2Texture2);
	 else if (ctx->Eval.Map2TextureCoord1) 
	    tc = eval2_4f( out, coord, flags, IM->Start,
			   1, &ctx->EvalMap.Map2Texture1);
      }

      if (tc != in) {
	 new_flags |= VERT_TEX_ANY(0); /* fix for sizes.. */
	 if (!all_eval)
	    tc = copy_4f( out, in, flags, IM->Start );
      }

      VB->TexCoordPtr[0] = tc;
   }


   {
      GLvector4f *in = VB->ObjPtr;
      GLvector4f *out = &IM->v.Obj;
      GLvector4f *obj = in;
   
      if (any_eval1) {
	 if (ctx->Eval.Map1Vertex4) 
	    obj = eval1_4f( out, coord, flags, IM->Start,
			    4, &ctx->EvalMap.Map1Vertex4);
	 else 
	    obj = eval1_4f( out, coord, flags, IM->Start,
			    3, &ctx->EvalMap.Map1Vertex3);
      }

      if (any_eval2) {
	 if (ctx->Eval.Map2Vertex4) 
	 {
	    if (ctx->Eval.AutoNormal && (req & VERT_NORM)) 
	       obj = eval2_obj_norm( out, VB->NormalPtr, coord, flags, IM->Start,
				     4, &ctx->EvalMap.Map2Vertex4 );
	    else
	       obj = eval2_4f( out, coord, flags, IM->Start,
			       4, &ctx->EvalMap.Map2Vertex4);
	 }
	 else if (ctx->Eval.Map2Vertex3) 
	 {
	    if (ctx->Eval.AutoNormal && (req & VERT_NORM)) 
	       obj = eval2_obj_norm( out, VB->NormalPtr, coord, flags, IM->Start,
				     3, &ctx->EvalMap.Map2Vertex3 );
	    else
	       obj = eval2_4f( out, coord, flags, IM->Start,
			       3, &ctx->EvalMap.Map2Vertex3 );
	 }
      }

      if (obj != in && !all_eval)
	 obj = copy_4f( out, in, flags, IM->Start );

      VB->ObjPtr = obj;
   }

   if (new_flags) {
      GLuint *oldflags = VB->Flag;
      GLuint *flags = VB->Flag = VB->EvaluatedFlags;
      GLuint i;
      GLuint count = VB->Count;

      if (!flags) {
	 VB->EvaluatedFlags = (GLuint *) MALLOC(VB->Size * sizeof(GLuint));
	 flags = VB->Flag = VB->EvaluatedFlags;
      }

      if (all_eval) {
	 for (i = 0 ; i < count ; i++) 
	    flags[i] = oldflags[i] | new_flags;
      } else {
	 GLuint andflag = ~0;
	 for (i = 0 ; i < count ; i++) {
	    if (oldflags[i] & VERT_EVAL_ANY) 
	       flags[i] = oldflags[i] | new_flags;
	    andflag &= flags[i];
	 }
      }
   }
}


void gl_MapGrid1f( GLcontext* ctx, GLint un, GLfloat u1, GLfloat u2 )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glMapGrid1f");

   if (un<1) {
      gl_error( ctx, GL_INVALID_VALUE, "glMapGrid1f" );
      return;
   }
   ctx->Eval.MapGrid1un = un;
   ctx->Eval.MapGrid1u1 = u1;
   ctx->Eval.MapGrid1u2 = u2;
   ctx->Eval.MapGrid1du = (u2 - u1) / (GLfloat) un;
}


void gl_MapGrid2f( GLcontext* ctx, GLint un, GLfloat u1, GLfloat u2,
		  GLint vn, GLfloat v1, GLfloat v2 )
{
   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glMapGrid2f");
   if (un<1) {
      gl_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(un)" );
      return;
   }
   if (vn<1) {
      gl_error( ctx, GL_INVALID_VALUE, "glMapGrid2f(vn)" );
      return;
   }
   ctx->Eval.MapGrid2un = un;
   ctx->Eval.MapGrid2u1 = u1;
   ctx->Eval.MapGrid2u2 = u2;
   ctx->Eval.MapGrid2du = (u2 - u1) / (GLfloat) un;
   ctx->Eval.MapGrid2vn = vn;
   ctx->Eval.MapGrid2v1 = v1;
   ctx->Eval.MapGrid2v2 = v2;
   ctx->Eval.MapGrid2dv = (v2 - v1) / (GLfloat) vn;
}



void gl_EvalMesh1( GLcontext* ctx, GLenum mode, GLint i1, GLint i2 )
{
   GLint i;
   GLfloat u, du;
   GLenum prim;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glEvalMesh1");

   switch (mode) {
      case GL_POINT:
         prim = GL_POINTS;
         break;
      case GL_LINE:
         prim = GL_LINE_STRIP;
         break;
      default:
         gl_error( ctx, GL_INVALID_ENUM, "glEvalMesh1(mode)" );
         return;
   }

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map1Vertex4 && !ctx->Eval.Map1Vertex3) 
      return;

   du = ctx->Eval.MapGrid1du;
   u = ctx->Eval.MapGrid1u1 + i1 * du;

   /* KW: Could short-circuit this to avoid the immediate mechanism.
    */
   RESET_IMMEDIATE(ctx);

   gl_Begin( ctx, prim );
   for (i=i1;i<=i2;i++,u+=du) {
      gl_EvalCoord1f( ctx, u );
   }
   gl_End(ctx);
}



void gl_EvalMesh2( GLcontext* ctx, 
		   GLenum mode, 
		   GLint i1, GLint i2, 
		   GLint j1, GLint j2 )
{
   GLint i, j;
   GLfloat u, du, v, dv, v1, u1;

   ASSERT_OUTSIDE_BEGIN_END_AND_FLUSH(ctx, "glEvalMesh2");

   /* No effect if vertex maps disabled.
    */
   if (!ctx->Eval.Map2Vertex4 && !ctx->Eval.Map2Vertex3) 
      return;

   du = ctx->Eval.MapGrid2du;
   dv = ctx->Eval.MapGrid2dv;
   v1 = ctx->Eval.MapGrid2v1 + j1 * dv;
   u1 = ctx->Eval.MapGrid2u1 + i1 * du;

   RESET_IMMEDIATE(ctx);

   switch (mode) {
   case GL_POINT:
      gl_Begin( ctx, GL_POINTS );
      for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    gl_EvalCoord2f( ctx, u, v );
	 }
      }
      gl_End(ctx);
      break;
   case GL_LINE:
      for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	 gl_Begin( ctx, GL_LINE_STRIP );
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    gl_EvalCoord2f( ctx, u, v );
	 }
	 gl_End(ctx);
      }
      for (u=u1,i=i1;i<=i2;i++,u+=du) {
	 gl_Begin( ctx, GL_LINE_STRIP );
	 for (v=v1,j=j1;j<=j2;j++,v+=dv) {
	    gl_EvalCoord2f( ctx, u, v );
	 }
	 gl_End(ctx);
      }
      break;
   case GL_FILL:
      for (v=v1,j=j1;j<j2;j++,v+=dv) {
	 /* NOTE: a quad strip can't be used because the four */
	 /* can't be guaranteed to be coplanar! */
	 gl_Begin( ctx, GL_TRIANGLE_STRIP );
	 for (u=u1,i=i1;i<=i2;i++,u+=du) {
	    gl_EvalCoord2f( ctx, u, v );
	    gl_EvalCoord2f( ctx, u, v+dv );
	 }
	 gl_End(ctx);
      }
      break;
   default:
      gl_error( ctx, GL_INVALID_ENUM, "glEvalMesh2(mode)" );
      return;
   }
}
