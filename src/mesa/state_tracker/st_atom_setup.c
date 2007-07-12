/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */
 

#include "st_context.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "st_atom.h"

static GLuint translate_fill( GLenum mode )
{
   switch (mode) {
   case GL_POINT:
      return PIPE_POLYGON_MODE_POINT;
   case GL_LINE:
      return PIPE_POLYGON_MODE_LINE;
   case GL_FILL:
      return PIPE_POLYGON_MODE_FILL;
   default:
      assert(0);
      return 0;
   }
}

static GLboolean get_offset_flag( GLuint fill_mode, 
				  const struct gl_polygon_attrib *p )
{
   switch (fill_mode) {
   case PIPE_POLYGON_MODE_POINT:
      return p->OffsetPoint;
   case PIPE_POLYGON_MODE_LINE:
      return p->OffsetLine;
   case PIPE_POLYGON_MODE_FILL:
      return p->OffsetFill;
   default:
      assert(0);
      return 0;
   }
}


static void update_setup_state( struct st_context *st )
{
   GLcontext *ctx = st->ctx;
   struct pipe_setup_state setup;

   memset(&setup, 0, sizeof(setup));
   
   /* _NEW_POLYGON, _NEW_BUFFERS
    */
   {
      setup.front_winding = PIPE_WINDING_CW;
	
      if (ctx->DrawBuffer && ctx->DrawBuffer->Name != 0)
	 setup.front_winding ^= PIPE_WINDING_BOTH;

      if (ctx->Polygon.FrontFace != GL_CCW)
	 setup.front_winding ^= PIPE_WINDING_BOTH;
   }

   /* _NEW_LIGHT
    */
   if (ctx->Light.ShadeModel == GL_FLAT)
      setup.flatshade = 1;

   /* _NEW_LIGHT 
    *
    * Not sure about the light->enabled requirement - does this still
    * apply??
    */
   if (ctx->Light.Enabled && 
       ctx->Light.Model.TwoSide)
      setup.light_twoside = 1;

   /* _NEW_POLYGON
    */
   if (ctx->Polygon.CullFlag) {
      if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) {
	 setup.cull_mode = PIPE_WINDING_BOTH;
      }
      else if (ctx->Polygon.CullFaceMode == GL_FRONT) {
	 setup.cull_mode = setup.front_winding;
      }
      else {
	 setup.cull_mode = setup.front_winding ^ PIPE_WINDING_BOTH;
      }
   }

   /* _NEW_POLYGON
    */
   {
      GLuint fill_front = translate_fill( ctx->Polygon.FrontMode );
      GLuint fill_back = translate_fill( ctx->Polygon.BackMode );
      
      if (setup.front_winding == PIPE_WINDING_CW) {
	 setup.fill_cw = fill_front;
	 setup.fill_ccw = fill_back;
      }
      else {
	 setup.fill_cw = fill_back;
	 setup.fill_ccw = fill_front;
      }

      /* Simplify when culling is active:
       */
      if (setup.cull_mode & PIPE_WINDING_CW) {
	 setup.fill_cw = setup.fill_ccw;
      }
      
      if (setup.cull_mode & PIPE_WINDING_CCW) {
	 setup.fill_ccw = setup.fill_cw;
      }
   }

   /* _NEW_POLYGON 
    */
   if (ctx->Polygon.OffsetUnits != 0.0 ||
       ctx->Polygon.OffsetFactor != 0.0) {
      setup.offset_cw = get_offset_flag( setup.fill_cw, &ctx->Polygon );
      setup.offset_ccw = get_offset_flag( setup.fill_ccw, &ctx->Polygon );
      setup.offset_units = ctx->Polygon.OffsetUnits;
      setup.offset_scale = ctx->Polygon.OffsetFactor;
   }

   if (ctx->Polygon.SmoothFlag)
      setup.poly_smooth = 1;

   if (ctx->Polygon.StippleFlag)
      setup.poly_stipple_enable = 1;


   /* _NEW_BUFFERS, _NEW_POLYGON
    */
   if (setup.fill_cw != PIPE_POLYGON_MODE_FILL ||
       setup.fill_ccw != PIPE_POLYGON_MODE_FILL)
   {
      GLfloat mrd = (ctx->DrawBuffer ? 
		     ctx->DrawBuffer->_MRD : 
		     1.0);

      setup.offset_units = ctx->Polygon.OffsetFactor * mrd;
      setup.offset_scale = (ctx->Polygon.OffsetUnits * mrd *
			    st->polygon_offset_scale);
   }
      
   /* _NEW_POINT
    */
   setup.point_size = ctx->Point.Size;
   setup.point_smooth = ctx->Point.SmoothFlag;

   /* _NEW_LINE
    */
   setup.line_width = ctx->Line.Width;
   setup.line_smooth = ctx->Line.SmoothFlag;
   setup.line_stipple_enable = ctx->Line.StippleFlag;
   setup.line_stipple_pattern = ctx->Line.StipplePattern;
   /* GL stipple factor is in [1,256], remap to [0, 255] here */
   setup.line_stipple_factor = ctx->Line.StippleFactor - 1;

   /* _NEW_MULTISAMPLE */
   if (ctx->Multisample.Enabled)
      setup.multisample = 1;

   if (memcmp(&setup, &st->state.setup, sizeof(setup)) != 0) {
      st->state.setup = setup;
      st->pipe->set_setup_state( st->pipe, &setup );
   }
}

const struct st_tracked_state st_update_setup = {
   .dirty = {
      .mesa = (_NEW_LIGHT | _NEW_POLYGON | _NEW_LINE |
               _NEW_POINT | _NEW_BUFFERS | _NEW_MULTISAMPLE),
      .st  = 0,
   },
   .update = update_setup_state
};
