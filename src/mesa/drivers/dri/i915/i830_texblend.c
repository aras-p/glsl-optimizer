/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "glheader.h"
#include "macros.h"
#include "mtypes.h"
#include "simple_list.h"
#include "enums.h"
#include "texformat.h"
#include "texstore.h"

#include "mm.h"

#include "intel_screen.h"
#include "intel_ioctl.h"
#include "intel_tex.h"

#include "i830_context.h"
#include "i830_reg.h"


/* ================================================================
 * Texture combine functions
 */
static GLuint pass_through( GLuint *state, GLuint blendUnit )
{
   state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
	       TEXPIPE_COLOR |
	       ENABLE_TEXOUTPUT_WRT_SEL |
	       TEXOP_OUTPUT_CURRENT |
	       DISABLE_TEX_CNTRL_STAGE |
	       TEXOP_SCALE_1X |
	       TEXOP_MODIFY_PARMS |
	       TEXBLENDOP_ARG1);
   state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
	       TEXPIPE_ALPHA |
	       ENABLE_TEXOUTPUT_WRT_SEL |
	       TEXOP_OUTPUT_CURRENT |
	       TEXOP_SCALE_1X |
	       TEXOP_MODIFY_PARMS |
	       TEXBLENDOP_ARG1);
   state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_COLOR |
	       TEXBLEND_ARG1 |
	       TEXBLENDARG_MODIFY_PARMS |
	       TEXBLENDARG_CURRENT);
   state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_ALPHA |
	       TEXBLEND_ARG1 |
	       TEXBLENDARG_MODIFY_PARMS |
	       TEXBLENDARG_CURRENT);

   return 4;
}

static GLuint emit_factor( GLuint blendUnit, GLuint *state, GLuint count, 
			   const GLfloat *factor )
{
   GLubyte r, g, b, a;
   GLuint col;
      
   if (0)
      fprintf(stderr, "emit constant %d: %.2f %.2f %.2f %.2f\n",
	  blendUnit, factor[0], factor[1], factor[2], factor[3]);

   UNCLAMPED_FLOAT_TO_UBYTE(r, factor[0]);
   UNCLAMPED_FLOAT_TO_UBYTE(g, factor[1]);
   UNCLAMPED_FLOAT_TO_UBYTE(b, factor[2]);
   UNCLAMPED_FLOAT_TO_UBYTE(a, factor[3]);

   col = ((a << 24) | (r << 16) | (g << 8) | b);

   state[count++] = _3DSTATE_COLOR_FACTOR_N_CMD(blendUnit); 
   state[count++] = col;

   return count;
}


static __inline__ GLuint GetTexelOp(GLint unit)
{
   switch(unit) {
   case 0: return TEXBLENDARG_TEXEL0;
   case 1: return TEXBLENDARG_TEXEL1;
   case 2: return TEXBLENDARG_TEXEL2;
   case 3: return TEXBLENDARG_TEXEL3;
   default: return TEXBLENDARG_TEXEL0;
   }
}


GLuint i830SetBlend_GL1_2(i830ContextPtr i830, int blendUnit, 
			  GLenum envMode, GLenum format, GLuint texel_op,
			  GLuint *state, const GLfloat *factor)
{
   if(INTEL_DEBUG&DEBUG_TEXTURE)
      fprintf(stderr, "%s %s %s texel_op(0x%x)\n",
	      __FUNCTION__,
	      _mesa_lookup_enum_by_nr(format),
	      _mesa_lookup_enum_by_nr(envMode),
	      texel_op);

   switch(envMode) {
   case GL_REPLACE:
      switch(format) {
      case GL_ALPHA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 return 4;

      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 4;

      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 return 4;

      default:
	 /* Always set to passthru if something is funny */
	 return pass_through( state, blendUnit );
      }

   case GL_MODULATE:
      switch(format) {
      case GL_ALPHA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 5;

      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 5;

      case GL_INTENSITY:
      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 6;

      default:
	 /* Always set to passthru if something is funny */
	 return pass_through( state, blendUnit );
      }

   case GL_DECAL:
      switch(format) {
      case GL_RGB:
      case GL_YCBCR_MESA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 4;

      case GL_RGBA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_BLEND);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG0 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_REPLICATE_ALPHA |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 6;
      default:
	 /* Always set to passthru if something is funny */
	 return pass_through( state, blendUnit );
      }

   case GL_BLEND:
      switch(format) {
      case GL_ALPHA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 5;

      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_BLEND);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG0 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_FACTOR_N);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return emit_factor( blendUnit, state, 6, factor );

      case GL_INTENSITY:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_BLEND);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_BLEND);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG0 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_FACTOR_N);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG0 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[6] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_FACTOR_N);
	 state[7] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return emit_factor( blendUnit, state, 8, factor );


      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_BLEND);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG0 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_FACTOR_N);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[6] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return emit_factor( blendUnit, state, 7, factor );

      default:
	 /* Always set to passthru if something is funny */
	 return pass_through( state, blendUnit );
      }

   case GL_ADD:
      switch(format) {
      case GL_ALPHA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 5;

      case GL_LUMINANCE:
      case GL_RGB:
      case GL_YCBCR_MESA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ADD);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ARG1);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 5;

      case GL_INTENSITY:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ADD);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ADD);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 6;

      case GL_LUMINANCE_ALPHA:
      case GL_RGBA:
	 state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     DISABLE_TEX_CNTRL_STAGE |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_ADD);
	 state[1] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     ENABLE_TEXOUTPUT_WRT_SEL |
		     TEXOP_OUTPUT_CURRENT |
		     TEXOP_SCALE_1X |
		     TEXOP_MODIFY_PARMS |
		     TEXBLENDOP_MODULATE);
	 state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_COLOR |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 state[4] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG1 |
		     TEXBLENDARG_MODIFY_PARMS |
		     texel_op);
	 state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
		     TEXPIPE_ALPHA |
		     TEXBLEND_ARG2 |
		     TEXBLENDARG_MODIFY_PARMS |
		     TEXBLENDARG_CURRENT);
	 return 6;

      default:
	 /* Always set to passthru if something is funny */
	 return pass_through( state, blendUnit );
      }

   default:
      /* Always set to passthru if something is funny */
      return pass_through( state, blendUnit );
   }
}

static GLuint i830SetTexEnvCombine(i830ContextPtr i830,
				   const struct gl_texture_unit *texUnit, 
				   GLint blendUnit,
				   GLuint texel_op,
				   GLuint *state,
				   const GLfloat *factor )
{
   GLuint blendop;
   GLuint ablendop;
   GLuint args_RGB[3];
   GLuint args_A[3];
   GLuint rgb_shift;
   GLuint alpha_shift;
   GLboolean need_factor = 0;
   int i;

   if(INTEL_DEBUG&DEBUG_TEXTURE)
      fprintf(stderr, "%s\n", __FUNCTION__);


   /* The EXT version of the DOT3 extension does not support the
    * scale factor, but the ARB version (and the version in OpenGL
    * 1.3) does.
    */
   switch (texUnit->Combine.ModeRGB) {
   case GL_DOT3_RGB_EXT:
      alpha_shift = texUnit->Combine.ScaleShiftA;
      rgb_shift = 0;
      break;

   case GL_DOT3_RGBA_EXT:
      alpha_shift = 0;
      rgb_shift = 0;
      break;

   default:
      rgb_shift = texUnit->Combine.ScaleShiftRGB;
      alpha_shift = texUnit->Combine.ScaleShiftA;
      break;
   }


   switch(texUnit->Combine.ModeRGB) {
   case GL_REPLACE: 
      blendop = TEXBLENDOP_ARG1;
      break;
   case GL_MODULATE: 
      blendop = TEXBLENDOP_MODULATE;
      break;
   case GL_ADD: 
      blendop = TEXBLENDOP_ADD;
      break;
   case GL_ADD_SIGNED:
      blendop = TEXBLENDOP_ADDSIGNED; 
      break;
   case GL_INTERPOLATE:
      blendop = TEXBLENDOP_BLEND; 
      break;
   case GL_SUBTRACT: 
      blendop = TEXBLENDOP_SUBTRACT;
      break;
   case GL_DOT3_RGB_EXT:
   case GL_DOT3_RGB:
      blendop = TEXBLENDOP_DOT3;
      break;
   case GL_DOT3_RGBA_EXT:
   case GL_DOT3_RGBA:
      blendop = TEXBLENDOP_DOT3;
      break;
   default: 
      return pass_through( state, blendUnit );
   }

   blendop |= (rgb_shift << TEXOP_SCALE_SHIFT);


   /* Handle RGB args */
   for(i = 0; i < 3; i++) {
      switch(texUnit->Combine.SourceRGB[i]) {
      case GL_TEXTURE: 
	 args_RGB[i] = texel_op;
	 break;
      case GL_CONSTANT:
	 args_RGB[i] = TEXBLENDARG_FACTOR_N; 
	 need_factor = 1;
	 break;
      case GL_PRIMARY_COLOR:
	 args_RGB[i] = TEXBLENDARG_DIFFUSE;
	 break;
      case GL_PREVIOUS:
	 args_RGB[i] = TEXBLENDARG_CURRENT; 
	 break;
      default: 
	 return pass_through( state, blendUnit );
      }

      switch(texUnit->Combine.OperandRGB[i]) {
      case GL_SRC_COLOR: 
	 args_RGB[i] |= 0;
	 break;
      case GL_ONE_MINUS_SRC_COLOR: 
	 args_RGB[i] |= TEXBLENDARG_INV_ARG;
	 break;
      case GL_SRC_ALPHA: 
	 args_RGB[i] |= TEXBLENDARG_REPLICATE_ALPHA;
	 break;
      case GL_ONE_MINUS_SRC_ALPHA: 
	 args_RGB[i] |= (TEXBLENDARG_REPLICATE_ALPHA | 
			 TEXBLENDARG_INV_ARG);
	 break;
      default: 
	 return pass_through( state, blendUnit );
      }
   }


   /* Need to knobble the alpha calculations of TEXBLENDOP_DOT4 to
    * match the spec.  Can't use DOT3 as it won't propogate values
    * into alpha as required:
    *
    * Note - the global factor is set up with alpha == .5, so 
    * the alpha part of the DOT4 calculation should be zero.
    */
   if ( texUnit->Combine.ModeRGB == GL_DOT3_RGBA_EXT || 
	texUnit->Combine.ModeRGB == GL_DOT3_RGBA ) {
      ablendop = TEXBLENDOP_DOT4;
      args_A[0] = TEXBLENDARG_FACTOR; /* the global factor */
      args_A[1] = TEXBLENDARG_FACTOR;
      args_A[2] = TEXBLENDARG_FACTOR;
   }
   else {
      switch(texUnit->Combine.ModeA) {
      case GL_REPLACE: 
	 ablendop = TEXBLENDOP_ARG1;
	 break;
      case GL_MODULATE: 
	 ablendop = TEXBLENDOP_MODULATE;
	 break;
      case GL_ADD: 
	 ablendop = TEXBLENDOP_ADD;
	 break;
      case GL_ADD_SIGNED:
	 ablendop = TEXBLENDOP_ADDSIGNED; 
	 break;
      case GL_INTERPOLATE:
	 ablendop = TEXBLENDOP_BLEND; 
	 break;
      case GL_SUBTRACT: 
	 ablendop = TEXBLENDOP_SUBTRACT;
	 break;
      default:
	 return pass_through( state, blendUnit );
      }


      ablendop |= (alpha_shift << TEXOP_SCALE_SHIFT);

      /* Handle A args */
      for(i = 0; i < 3; i++) {
	 switch(texUnit->Combine.SourceA[i]) {
	 case GL_TEXTURE: 
	    args_A[i] = texel_op;
	    break;
	 case GL_CONSTANT:
	    args_A[i] = TEXBLENDARG_FACTOR_N; 
	    need_factor = 1;
	    break;
	 case GL_PRIMARY_COLOR:
	    args_A[i] = TEXBLENDARG_DIFFUSE; 
	    break;
	 case GL_PREVIOUS:
	    args_A[i] = TEXBLENDARG_CURRENT; 
	    break;
	 default: 
	    return pass_through( state, blendUnit );
	 }

	 switch(texUnit->Combine.OperandA[i]) {
	 case GL_SRC_ALPHA: 
	    args_A[i] |= 0;
	    break;
	 case GL_ONE_MINUS_SRC_ALPHA: 
	    args_A[i] |= TEXBLENDARG_INV_ARG;
	    break;
	 default: 
	    return pass_through( state, blendUnit );
	 }
      }
   }



   /* Native Arg1 == Arg0 in GL_EXT_texture_env_combine spec */
   /* Native Arg2 == Arg1 in GL_EXT_texture_env_combine spec */
   /* Native Arg0 == Arg2 in GL_EXT_texture_env_combine spec */

   /* When we render we need to figure out which is the last really enabled
    * tex unit, and put last stage on it
    */


   /* Build color pipeline */

   state[0] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
	       TEXPIPE_COLOR |
	       ENABLE_TEXOUTPUT_WRT_SEL |
	       TEXOP_OUTPUT_CURRENT |
	       DISABLE_TEX_CNTRL_STAGE |
	       TEXOP_MODIFY_PARMS |
	       blendop);
   state[1] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_COLOR |
	       TEXBLEND_ARG1 |
	       TEXBLENDARG_MODIFY_PARMS |
	       args_RGB[0]);
   state[2] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_COLOR |
	       TEXBLEND_ARG2 |
	       TEXBLENDARG_MODIFY_PARMS |
	       args_RGB[1]);
   state[3] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_COLOR |
	       TEXBLEND_ARG0 |
	       TEXBLENDARG_MODIFY_PARMS |
	       args_RGB[2]);

   /* Build Alpha pipeline */
   state[4] = (_3DSTATE_MAP_BLEND_OP_CMD(blendUnit) |
	       TEXPIPE_ALPHA |
	       ENABLE_TEXOUTPUT_WRT_SEL |
	       TEXOP_OUTPUT_CURRENT |
	       TEXOP_MODIFY_PARMS |
	       ablendop);
   state[5] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_ALPHA |
	       TEXBLEND_ARG1 |
	       TEXBLENDARG_MODIFY_PARMS |
	       args_A[0]);
   state[6] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_ALPHA |
	       TEXBLEND_ARG2 |
	       TEXBLENDARG_MODIFY_PARMS |
	       args_A[1]);
   state[7] = (_3DSTATE_MAP_BLEND_ARG_CMD(blendUnit) |
	       TEXPIPE_ALPHA |
	       TEXBLEND_ARG0 |
	       TEXBLENDARG_MODIFY_PARMS |
	       args_A[2]);


   if (need_factor) 
      return emit_factor( blendUnit, state, 8, factor );
   else 
      return 8;
}


static void emit_texblend( i830ContextPtr i830, GLuint unit, GLuint blendUnit,
			   GLboolean last_stage )
{
   struct gl_texture_unit *texUnit = &i830->intel.ctx.Texture.Unit[unit];
   struct gl_texture_object *tObj = texUnit->_Current;
   i830TextureObjectPtr t = (i830TextureObjectPtr)tObj->DriverData;
   GLuint tmp[I830_TEXBLEND_SIZE], tmp_sz;


   if (0) fprintf(stderr, "%s unit %d\n", __FUNCTION__, unit);

   /* Update i830->state.TexBlend
    */ 
   if (texUnit->EnvMode == GL_COMBINE) {
      tmp_sz = i830SetTexEnvCombine(i830, texUnit, blendUnit, 
				    GetTexelOp(unit), tmp,
				    texUnit->EnvColor );
   } 
   else {
      tmp_sz = i830SetBlend_GL1_2(i830, blendUnit, texUnit->EnvMode,
				  t->intel.image[0][0].internalFormat, 
				  GetTexelOp(unit), tmp,
				  texUnit->EnvColor );
   }

   if (last_stage) 
      tmp[0] |= TEXOP_LAST_STAGE;

   if (tmp_sz != i830->state.TexBlendWordsUsed[blendUnit] ||
       memcmp( tmp, i830->state.TexBlend[blendUnit], tmp_sz * sizeof(GLuint))) {
      
      I830_STATECHANGE( i830, I830_UPLOAD_TEXBLEND(blendUnit) );
      memcpy( i830->state.TexBlend[blendUnit], tmp, tmp_sz * sizeof(GLuint));
      i830->state.TexBlendWordsUsed[blendUnit] = tmp_sz;
   }

   I830_ACTIVESTATE(i830, I830_UPLOAD_TEXBLEND(blendUnit), GL_TRUE);
}

static void emit_passthrough( i830ContextPtr i830 )
{
   GLuint tmp[I830_TEXBLEND_SIZE], tmp_sz;
   GLuint unit = 0;

   tmp_sz = pass_through( tmp, unit );
   tmp[0] |= TEXOP_LAST_STAGE;

   if (tmp_sz != i830->state.TexBlendWordsUsed[unit] ||
       memcmp( tmp, i830->state.TexBlend[unit], tmp_sz * sizeof(GLuint))) {
      
      I830_STATECHANGE( i830, I830_UPLOAD_TEXBLEND(unit) );
      memcpy( i830->state.TexBlend[unit], tmp, tmp_sz * sizeof(GLuint));
      i830->state.TexBlendWordsUsed[unit] = tmp_sz;
   }

   I830_ACTIVESTATE(i830, I830_UPLOAD_TEXBLEND(unit), GL_TRUE);
}

void i830EmitTextureBlend( i830ContextPtr i830 )
{
   GLcontext *ctx = &i830->intel.ctx;
   GLuint unit, last_stage = 0, blendunit = 0;

   I830_ACTIVESTATE(i830, I830_UPLOAD_TEXBLEND_ALL, GL_FALSE);

   if (ctx->Texture._EnabledUnits) {
      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits ; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) 
	    last_stage = unit;

      for (unit = 0 ; unit < ctx->Const.MaxTextureUnits ; unit++)
	 if (ctx->Texture.Unit[unit]._ReallyEnabled) 
	    emit_texblend( i830, unit, blendunit++, last_stage == unit );
   }
   else {
      emit_passthrough( i830 );
   }
}

