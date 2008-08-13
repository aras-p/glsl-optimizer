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
 
#include "main/macros.h"
#include "st_context.h"
#include "st_atom.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "cso_cache/cso_context.h"


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


static void update_raster_state( struct st_context *st )
{
   GLcontext *ctx = st->ctx;
   struct pipe_rasterizer_state *raster = &st->state.rasterizer;
   const struct gl_vertex_program *vertProg = ctx->VertexProgram._Current;
   uint i;

   memset(raster, 0, sizeof(*raster));

   raster->origin_lower_left = 1; /* Always true for OpenGL */
   
   /* _NEW_POLYGON, _NEW_BUFFERS
    */
   {
      if (ctx->Polygon.FrontFace == GL_CCW)
         raster->front_winding = PIPE_WINDING_CCW;
      else
         raster->front_winding = PIPE_WINDING_CW;

      /* XXX
       * I think the intention here is that user-created framebuffer objects
       * use Y=0=TOP layout instead of OpenGL's normal Y=0=bottom layout.
       * Flipping Y changes CW to CCW and vice-versa.
       * But this is an implementation/driver-specific artifact - remove...
       */
      if (ctx->DrawBuffer && ctx->DrawBuffer->Name != 0)
         raster->front_winding ^= PIPE_WINDING_BOTH;
   }

   /* _NEW_LIGHT
    */
   if (ctx->Light.ShadeModel == GL_FLAT)
      raster->flatshade = 1;

   /* _NEW_LIGHT | _NEW_PROGRAM
    *
    * Back-face colors can come from traditional lighting (when
    * GL_LIGHT_MODEL_TWO_SIDE is set) or from vertex programs (when
    * GL_VERTEX_PROGRAM_TWO_SIDE is set).  Note the logic here.
    */
   if (ctx->VertexProgram._Current) {
      if (ctx->VertexProgram._Enabled) {
         /* user-defined program */
         raster->light_twoside = ctx->VertexProgram.TwoSideEnabled;
      }
      else {
         /* TNL-generated program */
         raster->light_twoside = ctx->Light.Enabled && ctx->Light.Model.TwoSide;
      }
   }
   else if (ctx->Light.Enabled && ctx->Light.Model.TwoSide) {
      raster->light_twoside = 1;
   }

   /* _NEW_POLYGON
    */
   if (ctx->Polygon.CullFlag) {
      if (ctx->Polygon.CullFaceMode == GL_FRONT_AND_BACK) {
	 raster->cull_mode = PIPE_WINDING_BOTH;
      }
      else if (ctx->Polygon.CullFaceMode == GL_FRONT) {
	 raster->cull_mode = raster->front_winding;
      }
      else {
	 raster->cull_mode = raster->front_winding ^ PIPE_WINDING_BOTH;
      }
   }

   /* _NEW_POLYGON
    */
   {
      GLuint fill_front = translate_fill( ctx->Polygon.FrontMode );
      GLuint fill_back = translate_fill( ctx->Polygon.BackMode );
      
      if (raster->front_winding == PIPE_WINDING_CW) {
	 raster->fill_cw = fill_front;
	 raster->fill_ccw = fill_back;
      }
      else {
	 raster->fill_cw = fill_back;
	 raster->fill_ccw = fill_front;
      }

      /* Simplify when culling is active:
       */
      if (raster->cull_mode & PIPE_WINDING_CW) {
	 raster->fill_cw = raster->fill_ccw;
      }
      
      if (raster->cull_mode & PIPE_WINDING_CCW) {
	 raster->fill_ccw = raster->fill_cw;
      }
   }

   /* _NEW_POLYGON 
    */
   if (ctx->Polygon.OffsetUnits != 0.0 ||
       ctx->Polygon.OffsetFactor != 0.0) {
      raster->offset_cw = get_offset_flag( raster->fill_cw, &ctx->Polygon );
      raster->offset_ccw = get_offset_flag( raster->fill_ccw, &ctx->Polygon );
      raster->offset_units = ctx->Polygon.OffsetUnits;
      raster->offset_scale = ctx->Polygon.OffsetFactor;
   }

   if (ctx->Polygon.SmoothFlag)
      raster->poly_smooth = 1;

   if (ctx->Polygon.StippleFlag)
      raster->poly_stipple_enable = 1;


   /* _NEW_BUFFERS, _NEW_POLYGON
    */
   if (raster->fill_cw != PIPE_POLYGON_MODE_FILL ||
       raster->fill_ccw != PIPE_POLYGON_MODE_FILL)
   {
      GLfloat mrd = (ctx->DrawBuffer ? 
		     ctx->DrawBuffer->_MRD : 
		     1.0f);

      raster->offset_units = ctx->Polygon.OffsetFactor * mrd;
      raster->offset_scale = (ctx->Polygon.OffsetUnits * mrd *
			    st->polygon_offset_scale);
   }
      
   /* _NEW_POINT
    */
   raster->point_size = ctx->Point.Size;

   raster->point_size_min = 0;    /* temporary, will go away */
   raster->point_size_max = 1000; /* temporary, will go away */

   raster->point_smooth = ctx->Point.SmoothFlag;
   raster->point_sprite = ctx->Point.PointSprite;
   for (i = 0; i < MAX_TEXTURE_COORD_UNITS; i++) {
      if (ctx->Point.CoordReplace[i]) {
         if (ctx->Point.SpriteOrigin == GL_UPPER_LEFT)
            raster->sprite_coord_mode[i] = PIPE_SPRITE_COORD_UPPER_LEFT;
         else 
            raster->sprite_coord_mode[i] = PIPE_SPRITE_COORD_LOWER_LEFT;
      }
      else {
         raster->sprite_coord_mode[i] = PIPE_SPRITE_COORD_NONE;
      }
   }
   if (vertProg) {
      if (vertProg->Base.Id == 0) {
         if (vertProg->Base.OutputsWritten & (1 << VERT_RESULT_PSIZ)) {
            /* generated program which emits point size */
            raster->point_size_per_vertex = TRUE;
         }
      }
      else if (ctx->VertexProgram.PointSizeEnabled) {
         /* user-defined program and GL_VERTEX_PROGRAM_POINT_SIZE set */
         raster->point_size_per_vertex = ctx->VertexProgram.PointSizeEnabled;
      }
   }
   if (!raster->point_size_per_vertex) {
      /* clamp size now */
      raster->point_size = CLAMP(ctx->Point.Size,
                                 ctx->Point.MinSize,
                                 ctx->Point.MaxSize);
   }

   /* _NEW_LINE
    */
   raster->line_smooth = ctx->Line.SmoothFlag;
   if (ctx->Line.SmoothFlag) {
      raster->line_width = CLAMP(ctx->Line.Width,
                                ctx->Const.MinLineWidthAA,
                                ctx->Const.MaxLineWidthAA);
   }
   else {
      raster->line_width = CLAMP(ctx->Line.Width,
                                ctx->Const.MinLineWidth,
                                ctx->Const.MaxLineWidth);
   }

   raster->line_stipple_enable = ctx->Line.StippleFlag;
   raster->line_stipple_pattern = ctx->Line.StipplePattern;
   /* GL stipple factor is in [1,256], remap to [0, 255] here */
   raster->line_stipple_factor = ctx->Line.StippleFactor - 1;

   /* _NEW_MULTISAMPLE */
   if (ctx->Multisample._Enabled)
      raster->multisample = 1;

   /* _NEW_SCISSOR */
   if (ctx->Scissor.Enabled)
      raster->scissor = 1;

   raster->gl_rasterization_rules = 1;

   cso_set_rasterizer(st->cso_context, raster);
}

const struct st_tracked_state st_update_rasterizer = {
   "st_update_rasterizer",    /* name */
   {
      (_NEW_BUFFERS |
       _NEW_LIGHT |
       _NEW_LINE |
       _NEW_MULTISAMPLE |
       _NEW_POINT |
       _NEW_POLYGON |
       _NEW_PROGRAM |
       _NEW_SCISSOR),      /* mesa state dependencies*/
      0,                   /* state tracker dependencies */
   },
   update_raster_state     /* update function */
};
