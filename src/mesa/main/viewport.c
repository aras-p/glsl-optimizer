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
set_viewport_no_notify(struct gl_context *ctx, unsigned idx,
                       GLfloat x, GLfloat y,
                       GLfloat width, GLfloat height)
{
   /* clamp width and height to the implementation dependent range */
   width  = MIN2(width, (GLfloat) ctx->Const.MaxViewportWidth);
   height = MIN2(height, (GLfloat) ctx->Const.MaxViewportHeight);

   /* The GL_ARB_viewport_array spec says:
    *
    *     "The location of the viewport's bottom-left corner, given by (x,y),
    *     are clamped to be within the implementation-dependent viewport
    *     bounds range.  The viewport bounds range [min, max] tuple may be
    *     determined by calling GetFloatv with the symbolic constant
    *     VIEWPORT_BOUNDS_RANGE (see section 6.1)."
    */
   if (ctx->Extensions.ARB_viewport_array) {
      x = CLAMP(x,
                ctx->Const.ViewportBounds.Min, ctx->Const.ViewportBounds.Max);
      y = CLAMP(y,
                ctx->Const.ViewportBounds.Min, ctx->Const.ViewportBounds.Max);
   }

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

struct gl_viewport_inputs {
   GLfloat X, Y;                /**< position */
   GLfloat Width, Height;       /**< size */
};

struct gl_depthrange_inputs {
   GLdouble Near, Far;          /**< Depth buffer range */
};

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
   unsigned i;
   GET_CURRENT_CONTEXT(ctx);
   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glViewport %d %d %d %d\n", x, y, width, height);

   if (width < 0 || height < 0) {
      _mesa_error(ctx,  GL_INVALID_VALUE,
                   "glViewport(%d, %d, %d, %d)", x, y, width, height);
      return;
   }

   /* The GL_ARB_viewport_array spec says:
    *
    *     "Viewport sets the parameters for all viewports to the same values
    *     and is equivalent (assuming no errors are generated) to:
    *
    *     for (uint i = 0; i < MAX_VIEWPORTS; i++)
    *         ViewportIndexedf(i, 1, (float)x, (float)y, (float)w, (float)h);"
    *
    * Set all of the viewports supported by the implementation, but only
    * signal the driver once at the end.
    */
   for (i = 0; i < ctx->Const.MaxViewports; i++)
      set_viewport_no_notify(ctx, i, x, y, width, height);

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
_mesa_set_viewport(struct gl_context *ctx, unsigned idx, GLfloat x, GLfloat y,
                    GLfloat width, GLfloat height)
{
   set_viewport_no_notify(ctx, idx, x, y, width, height);

   if (ctx->Driver.Viewport) {
      /* Many drivers will use this call to check for window size changes
       * and reallocate the z/stencil/accum/etc buffers if needed.
       */
      ctx->Driver.Viewport(ctx);
   }
}

void GLAPIENTRY
_mesa_ViewportArrayv(GLuint first, GLsizei count, const GLfloat *v)
{
   int i;
   const struct gl_viewport_inputs *const p = (struct gl_viewport_inputs *) v;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glViewportArrayv %d %d\n", first, count);

   if ((first + count) > ctx->Const.MaxViewports) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glViewportArrayv: first (%d) + count (%d) > MaxViewports "
                  "(%d)",
                  first, count, ctx->Const.MaxViewports);
      return;
   }

   /* Verify width & height */
   for (i = 0; i < count; i++) {
      if (p[i].Width < 0 || p[i].Height < 0) {
         _mesa_error(ctx, GL_INVALID_VALUE,
                     "glViewportArrayv: index (%d) width or height < 0 "
                     "(%f, %f)",
                     i + first, p[i].Width, p[i].Height);
         return;
      }
   }

   for (i = 0; i < count; i++)
      set_viewport_no_notify(ctx, i + first,
                             p[i].X, p[i].Y,
                             p[i].Width, p[i].Height);

   if (ctx->Driver.Viewport)
      ctx->Driver.Viewport(ctx);
}

static void
ViewportIndexedf(GLuint index, GLfloat x, GLfloat y,
                 GLfloat w, GLfloat h, const char *function)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "%s(%d, %f, %f, %f, %f)\n",
                  function, index, x, y, w, h);

   if (index >= ctx->Const.MaxViewports) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s: index (%d) >= MaxViewports (%d)",
                  function, index, ctx->Const.MaxViewports);
      return;
   }

   /* Verify width & height */
   if (w < 0 || h < 0) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "%s: index (%d) width or height < 0 (%f, %f)",
                  function, index, w, h);
      return;
   }

   _mesa_set_viewport(ctx, index, x, y, w, h);
}

void GLAPIENTRY
_mesa_ViewportIndexedf(GLuint index, GLfloat x, GLfloat y,
                       GLfloat w, GLfloat h)
{
   ViewportIndexedf(index, x, y, w, h, "glViewportIndexedf");
}

void GLAPIENTRY
_mesa_ViewportIndexedfv(GLuint index, const GLfloat *v)
{
   ViewportIndexedf(index, v[0], v[1], v[2], v[3], "glViewportIndexedfv");
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
   unsigned i;
   GET_CURRENT_CONTEXT(ctx);

   FLUSH_VERTICES(ctx, 0);

   if (MESA_VERBOSE&VERBOSE_API)
      _mesa_debug(ctx, "glDepthRange %f %f\n", nearval, farval);

   /* The GL_ARB_viewport_array spec says:
    *
    *     "DepthRange sets the depth range for all viewports to the same
    *     values and is equivalent (assuming no errors are generated) to:
    *
    *     for (uint i = 0; i < MAX_VIEWPORTS; i++)
    *         DepthRangeIndexed(i, n, f);"
    *
    * Set the depth range for all of the viewports supported by the
    * implementation, but only signal the driver once at the end.
    */
   for (i = 0; i < ctx->Const.MaxViewports; i++)
      set_depth_range_no_notify(ctx, i, nearval, farval);

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
 * Update a range DepthRange values
 *
 * \param first   starting array index
 * \param count   count of DepthRange items to update
 * \param v       pointer to memory containing
 *                GLclampd near and far clip-plane values
 */
void GLAPIENTRY
_mesa_DepthRangeArrayv(GLuint first, GLsizei count, const GLclampd *v)
{
   int i;
   const struct gl_depthrange_inputs *const p =
      (struct gl_depthrange_inputs *) v;
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDepthRangeArrayv %d %d\n", first, count);

   if ((first + count) > ctx->Const.MaxViewports) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDepthRangev: first (%d) + count (%d) >= MaxViewports (%d)",
                  first, count, ctx->Const.MaxViewports);
      return;
   }

   for (i = 0; i < count; i++)
      set_depth_range_no_notify(ctx, i + first, p[i].Near, p[i].Far);

   if (ctx->Driver.DepthRange)
      ctx->Driver.DepthRange(ctx);
}

/**
 * Update a single DepthRange
 *
 * \param index    array index to update
 * \param nearval  specifies the Z buffer value which should correspond to
 *                 the near clip plane
 * \param farval   specifies the Z buffer value which should correspond to
 *                 the far clip plane
 */
void GLAPIENTRY
_mesa_DepthRangeIndexed(GLuint index, GLclampd nearval, GLclampd farval)
{
   GET_CURRENT_CONTEXT(ctx);

   if (MESA_VERBOSE & VERBOSE_API)
      _mesa_debug(ctx, "glDepthRangeIndexed(%d, %f, %f)\n",
                  index, nearval, farval);

   if (index >= ctx->Const.MaxViewports) {
      _mesa_error(ctx, GL_INVALID_VALUE,
                  "glDepthRangeIndexed: index (%d) >= MaxViewports (%d)",
                  index, ctx->Const.MaxViewports);
      return;
   }

   _mesa_set_depth_range(ctx, index, nearval, farval);
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

