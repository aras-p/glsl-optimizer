/* $Id: s_context.h,v 1.10 2001/05/17 09:32:17 keithw Exp $ */

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

#ifndef S_CONTEXT_H
#define S_CONTEXT_H

#include "mtypes.h"
#include "swrast.h"

/*
 * For texture sampling:
 */
typedef void (*TextureSampleFunc)( GLcontext *ctx, GLuint texUnit,
				   const struct gl_texture_object *tObj,
                                   GLuint n,
                                   const GLfloat s[], const GLfloat t[],
                                   const GLfloat u[], const GLfloat lambda[],
                                   GLchan rgba[][4] );



/*
 * Blending function
 */
#ifdef USE_MMX_ASM
typedef void (_ASMAPIP blend_func)( GLcontext *ctx, GLuint n,
                                    const GLubyte mask[],
                                    GLchan src[][4], CONST GLchan dst[][4] );
#else
typedef void (*blend_func)( GLcontext *ctx, GLuint n, const GLubyte mask[],
                            GLchan src[][4], CONST GLchan dst[][4] );
#endif

typedef void (*swrast_point_func)( GLcontext *ctx, const SWvertex *);

typedef void (*swrast_line_func)( GLcontext *ctx,
                                  const SWvertex *, const SWvertex *);

typedef void (*swrast_tri_func)( GLcontext *ctx, const SWvertex *,
                                 const SWvertex *, const SWvertex *);



/*
 * Bitmasks to indicate which rasterization options are enabled (RasterMask)
 */
#define ALPHATEST_BIT		0x001	/* Alpha-test pixels */
#define BLEND_BIT		0x002	/* Blend pixels */
#define DEPTH_BIT		0x004	/* Depth-test pixels */
#define FOG_BIT			0x008	/* Fog pixels */
#define LOGIC_OP_BIT		0x010	/* Apply logic op in software */
#define SCISSOR_BIT		0x020	/* Scissor pixels */
#define STENCIL_BIT		0x040	/* Stencil pixels */
#define MASKING_BIT		0x080	/* Do glColorMask or glIndexMask */
#define ALPHABUF_BIT		0x100	/* Using software alpha buffer */
#define WINCLIP_BIT		0x200	/* Clip pixels/primitives to window */
#define MULTI_DRAW_BIT		0x400	/* Write to more than one color- */
                                        /* buffer or no buffers. */
#define OCCLUSION_BIT           0x800   /* GL_HP_occlusion_test enabled */
#define TEXTURE_BIT		0x1000	/* Texturing really enabled */


#define _SWRAST_NEW_RASTERMASK (_NEW_BUFFERS|	\
			        _NEW_SCISSOR|	\
			        _NEW_COLOR|	\
			        _NEW_DEPTH|	\
			        _NEW_FOG|	\
			        _NEW_STENCIL|	\
			        _NEW_TEXTURE|	\
			        _NEW_VIEWPORT|	\
			        _NEW_DEPTH)



typedef struct
{
   /* Driver interface:
    */
   struct swrast_device_driver Driver;

   /* Configuration mechanisms to make software rasterizer match
    * characteristics of the hardware rasterizer (if present):
    */
   GLboolean AllowVertexFog;
   GLboolean AllowPixelFog;

   /* Derived values, invalidated on statechanges, updated from
    * _swrast_validate_derived():
    */
   GLuint _RasterMask;
   GLfloat _MinMagThresh[MAX_TEXTURE_UNITS];
   GLfloat _backface_sign;
   GLboolean _PreferPixelFog;

   /* Accum buffer temporaries.
    */
   GLboolean _IntegerAccumMode;	/* Storing unscaled integers? */
   GLfloat _IntegerAccumScaler;	/* Implicit scale factor */


   /* Working values:
    */
   struct pixel_buffer* PB;
   GLuint StippleCounter;    /* Line stipple counter */
   GLuint NewState;
   GLuint StateChanges;

   /* Mechanism to allow driver (like X11) to register further
    * software rasterization routines.
    */
   void (*choose_point)( GLcontext * );
   void (*choose_line)( GLcontext * );
   void (*choose_triangle)( GLcontext * );

   GLuint invalidate_point;
   GLuint invalidate_line;
   GLuint invalidate_triangle;


   /* Function pointers for dispatch behind public entrypoints.
    */
   void (*InvalidateState)( GLcontext *ctx, GLuint new_state );

   swrast_point_func Point;
   swrast_line_func Line;
   swrast_tri_func Triangle;

   /* Placeholders for when separate specular (or secondary color) is
    * enabled but texturing is not.
    */
   swrast_point_func SpecPoint;
   swrast_line_func SpecLine;
   swrast_tri_func SpecTriangle;


   /* Internal hooks, kept uptodate by the same mechanism as above.
    */
   blend_func BlendFunc;
   TextureSampleFunc TextureSample[MAX_TEXTURE_UNITS];

} SWcontext;


void
_swrast_validate_derived( GLcontext *ctx );


#define SWRAST_CONTEXT(ctx) ((SWcontext *)ctx->swrast_context)

#define RENDER_START(SWctx, GLctx)			\
   do {						\
      if ((SWctx)->Driver.SpanRenderStart) {		\
         (*(SWctx)->Driver.SpanRenderStart)(GLctx);	\
      }						\
   } while (0)

#define RENDER_FINISH(SWctx, GLctx)			\
   do {						\
      if ((SWctx)->Driver.SpanRenderFinish) {		\
         (*(SWctx)->Driver.SpanRenderFinish)(GLctx);	\
      }						\
   } while (0)


#endif
