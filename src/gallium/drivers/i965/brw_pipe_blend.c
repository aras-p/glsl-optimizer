

   /* _NEW_COLOR */
   if (key->logic_op != GL_COPY) {
      cc.cc2.logicop_enable = 1;
      cc.cc5.logicop_func = intel_translate_logic_op(key->logic_op);
   } else if (key->color_blend) {
      GLenum eqRGB = key->blend_eq_rgb;
      GLenum eqA = key->blend_eq_a;
      GLenum srcRGB = key->blend_src_rgb;
      GLenum dstRGB = key->blend_dst_rgb;
      GLenum srcA = key->blend_src_a;
      GLenum dstA = key->blend_dst_a;

      if (eqRGB == GL_MIN || eqRGB == GL_MAX) {
	 srcRGB = dstRGB = GL_ONE;
      }

      if (eqA == GL_MIN || eqA == GL_MAX) {
	 srcA = dstA = GL_ONE;
      }

      cc.cc6.dest_blend_factor = brw_translate_blend_factor(dstRGB);
      cc.cc6.src_blend_factor = brw_translate_blend_factor(srcRGB);
      cc.cc6.blend_function = brw_translate_blend_equation(eqRGB);

      cc.cc5.ia_dest_blend_factor = brw_translate_blend_factor(dstA);
      cc.cc5.ia_src_blend_factor = brw_translate_blend_factor(srcA);
      cc.cc5.ia_blend_function = brw_translate_blend_equation(eqA);

      cc.cc3.blend_enable = 1;
      cc.cc3.ia_blend_enable = (srcA != srcRGB ||
				dstA != dstRGB ||
				eqA != eqRGB);
   }

   if (key->dither) {
      cc.cc5.dither_enable = 1;
      cc.cc6.y_dither_offset = 0;
      cc.cc6.x_dither_offset = 0;
   }

   if (INTEL_DEBUG & DEBUG_STATS)
      cc.cc5.statistics_enable = 1;
}



static void brw_set_blend_color(struct pipe_context *pipe,
				const float *blend_color)
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_blend_constant_color *bcc = &brw->curr.blend_color.bcc;

   memset(bcc, 0, sizeof(*bcc));      
   bcc->header.opcode = CMD_BLEND_CONSTANT_COLOR;
   bcc->header.length = sizeof(*bcc)/4-2;
   bcc->blend_constant_color[0] = blend_color[0];
   bcc->blend_constant_color[1] = blend_color[1];
   bcc->blend_constant_color[2] = blend_color[2];
   bcc->blend_constant_color[3] = blend_color[3];

   brw->state.dirty.pipe |= PIPE_NEW_BLEND_COLOR;
}
