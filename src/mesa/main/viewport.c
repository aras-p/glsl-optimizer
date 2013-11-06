/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2009  VMware, Inc.  All Rights Reserved.
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
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THE SOFTWARE.
 */


/**
 * \file viewport.c
 * glViewport and glDepthRange functions.
 */


#include "context.h"
#include "macros.h"
#include "mtypes.h"
#include "viewport.h"

static void
set_viewport_no_notify(struct gl_context *ctx, unsigned idx, GLint x, GLint y,
                       GLsizei width, GLsizei height)
{
   /* clamp width and height to the implementation dependent range */
   width  = MIN2(width, (GLsizei) ctx->Const.MaxViewportWidth);
   height = MIN2(height, (GLsizei) ctx->Const.MaxViewportHeight);

   ctx->ViewportArray[idx].X = x;
   ctx->ViewportArray[idx].Width = width;
   ctx->ViewportArray[idx].Y = y;
   ctx->ViewportArray[idx].Height = height;
   ctx->NewState |= _NEW_VIEWPORT;

#if 1
   /* XXX remove this someday.  Currently the DRI drivers rely on
    * the WindowMap matrix being up to date in the driver's Viewport
    * and DepthRange functions.
    */
   _math_matrix_viewport(&ctx->ViewportArray[idx]._WindowMap,
                         ctx->ViewportArray[idx].X,
                         ctx->ViewportArray[idx].Y,
                         ctx->ViewportArray[idx].Width,
                         ctx->ViewportArray[idx].Height,
                         ctx->ViewportArray[idx].Near,
                         ctx->ViewportArray[idx].Far,
                         ctx->DrawBuffer->_DepthMaxF);
#endif
}

/**
 * Set the viewport.
 * \sa Called via glViewport() or display list execution.
 *
 * Flushes the vertices and calls _mesa_set_viewport() with the given
 * parameters.
 */
void GLAPIENTRY
_mesa_Viewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glViewport %d %d %d %d\n", x, y, width, height);

   if (width < 0 || height < 0) {
      _mesa_error(ctx,  GL_INVALID_VALUE,
                   "glViewport(%d, %d, %d, %d)", x, y, width, height);
      return;
   }

   set_viewport_no_notify(ctx, 0, x, y, width, height);

   if (ctx->Driver.Viewport) {
      /* Many drivers will use this call to check for window size changes
       * and reallocate the z/stencil/accum/etc buffers if needed.
       */
      ctx->Driver.Viewport(ctx);
   }
}


/**
 * Set new viewport parameters and update derived state (the _WindowMap
 * matrix).  Usually called from _mesa_Viewport().
 * 
 * \param ctx GL context.
 * \param idx    Index of the viewport to be updated.
 * \param x, y coordinates of the lower left corner of the viewport rectangle.
 * \param width width of the viewport rectangle.
 * \param height height of the viewport rectangle.
 */
void
_mesa_set_viewport(struct gl_context *ctx, unsigned idx, GLint x, GLint y,
                    GLsizei width, GLsizei height)
{
   set_viewport_no_notify(ctx, idx, x, y, width, height);

   if (ctx->Driver.Viewport) {
      /* Many drivers will use this call to check for window size changes
       * and reallocate the z/stencil/accum/etc buffers if needed.
       */
      ctx->Driver.Viewport(ctx);
   }
}

static void
set_depth_range_no_notify(struct gl_context *ctx, unsigned idx,
                          GLclampd nearval, GLclampd farval)
{
   if (ctx->ViewportArray[idx].Near == nearval &&
       ctx->ViewportArray[idx].Far == farval)
      return;

   ctx->ViewportArray[idx].Near = CLAMP(nearval, 0.0, 1.0);
   ctx->ViewportArray[idx].Far = CLAMP(farval, 0.0, 1.0);
   ctx->NewState |= _NEW_VIEWPORT;

#if 1
   /* XXX remove this someday.  Currently the DRI drivers rely on
    * the WindowMap matrix being up to date in the driver's Viewport
    * and DepthRange functions.
    */
   _math_matrix_viewport(&ctx->ViewportArray[idx]._WindowMap,
                         ctx->ViewportArray[idx].X,
                         ctx->ViewportArray[idx].Y,
                         ctx->ViewportArray[idx].Width,
                         ctx->ViewportArray[idx].Height,
                         ctx->ViewportArray[idx].Near,
                         ctx->ViewportArray[idx].Far,
                         ctx->DrawBuffer->_DepthMaxF);
#endif
}

void
_mesa_set_depth_range(struct gl_context *ctx, unsigned idx,
                      GLclampd nearval, GLclampd farval)
{
   set_depth_range_no_notify(ctx, idx, nearval, farval);

   if (ctx->Driver.DepthRange)
      ctx->Driver.DepthRange(ctx);
}

/**
 * Called by glDepthRange
 *
 * \param nearval  specifies the Z buffer value which should correspond to
 *                 the near clip plane
 * \param farval  specifies the Z buffer value which should correspond to
 *                the far clip plane
 */
void GLAPIENTRY
_mesa_DepthRange(GLclampd nearval, GLclampd farval)
{
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glDepthRange %f %f\n", nearval, farval);

   set_depth_range_no_notify(ctx, 0, nearval, farval);

   if (ctx->Driver.DepthRange) {
      ctx->Driver.DepthRange(ctx);
   }
}

void GLAPIENTRY
_mesa_DepthRangef(GLclampf nearval, GLclampf farval)
{
   _mesa_DepthRange(nearval, farval);
}

/** 
 * Initialize the context viewport attribute group.
 * \param ctx  the GL context.
 */
void _mesa_init_viewport(struct gl_context *ctx)
{
   GLfloat depthMax = 65535.0F; /* sorf of arbitrary */
   unsigned i;

   /* Note: ctx->Const.MaxViewports may not have been set by the driver yet,
    * so just initialize all of them.
    */
   for (i = 0; i < MAX_VIEWPORTS; i++) {
      /* Viewport group */
      ctx->ViewportArray[i].X = 0;
      ctx->ViewportArray[i].Y = 0;
      ctx->ViewportArray[i].Width = 0;
      ctx->ViewportArray[i].Height = 0;
      ctx->ViewportArray[i].Near = 0.0;
      ctx->ViewportArray[i].Far = 1.0;
      _math_matrix_ctr(&ctx->ViewportArray[i]._WindowMap);

      _math_matrix_viewport(&ctx->ViewportArray[i]._WindowMap, 0, 0, 0, 0,
                            0.0F, 1.0F, depthMax);
   }
}


/** 
 * Free the context viewport attribute group data.
 * \param ctx  the GL context.
 */
void _mesa_free_viewport_data(struct gl_context *ctx)
{
   unsigned i;

   for (i = 0; i < MAX_VIEWPORTS; i++)
      _math_matrix_dtr(&ctx->ViewportArray[i]._WindowMap);
}

