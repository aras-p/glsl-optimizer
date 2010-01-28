
#include "util/u_memory.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"

#include "brw_context.h"
#include "brw_defines.h"
#include "brw_debug.h"

static int translate_logicop(unsigned logicop)
{
   switch (logicop) {
   case PIPE_LOGICOP_CLEAR:
      return BRW_LOGICOPFUNCTION_CLEAR;
   case PIPE_LOGICOP_AND:
      return BRW_LOGICOPFUNCTION_AND;
   case PIPE_LOGICOP_AND_REVERSE:
      return BRW_LOGICOPFUNCTION_AND_REVERSE;
   case PIPE_LOGICOP_COPY:
      return BRW_LOGICOPFUNCTION_COPY;
   case PIPE_LOGICOP_COPY_INVERTED:
      return BRW_LOGICOPFUNCTION_COPY_INVERTED;
   case PIPE_LOGICOP_AND_INVERTED:
      return BRW_LOGICOPFUNCTION_AND_INVERTED;
   case PIPE_LOGICOP_NOOP:
      return BRW_LOGICOPFUNCTION_NOOP;
   case PIPE_LOGICOP_XOR:
      return BRW_LOGICOPFUNCTION_XOR;
   case PIPE_LOGICOP_OR:
      return BRW_LOGICOPFUNCTION_OR;
   case PIPE_LOGICOP_OR_INVERTED:
      return BRW_LOGICOPFUNCTION_OR_INVERTED;
   case PIPE_LOGICOP_NOR:
      return BRW_LOGICOPFUNCTION_NOR;
   case PIPE_LOGICOP_EQUIV:
      return BRW_LOGICOPFUNCTION_EQUIV;
   case PIPE_LOGICOP_INVERT:
      return BRW_LOGICOPFUNCTION_INVERT;
   case PIPE_LOGICOP_OR_REVERSE:
      return BRW_LOGICOPFUNCTION_OR_REVERSE;
   case PIPE_LOGICOP_NAND:
      return BRW_LOGICOPFUNCTION_NAND;
   case PIPE_LOGICOP_SET:
      return BRW_LOGICOPFUNCTION_SET;
   default:
      assert(0);
      return BRW_LOGICOPFUNCTION_SET;
   }
}


static unsigned translate_blend_equation( unsigned mode )
{
   switch (mode) {
   case PIPE_BLEND_ADD: 
      return BRW_BLENDFUNCTION_ADD; 
   case PIPE_BLEND_MIN: 
      return BRW_BLENDFUNCTION_MIN; 
   case PIPE_BLEND_MAX: 
      return BRW_BLENDFUNCTION_MAX; 
   case PIPE_BLEND_SUBTRACT: 
      return BRW_BLENDFUNCTION_SUBTRACT; 
   case PIPE_BLEND_REVERSE_SUBTRACT: 
      return BRW_BLENDFUNCTION_REVERSE_SUBTRACT; 
   default: 
      assert(0);
      return BRW_BLENDFUNCTION_ADD;
   }
}

static unsigned translate_blend_factor( unsigned factor )
{
   switch(factor) {
   case PIPE_BLENDFACTOR_ZERO: 
      return BRW_BLENDFACTOR_ZERO; 
   case PIPE_BLENDFACTOR_SRC_ALPHA: 
      return BRW_BLENDFACTOR_SRC_ALPHA; 
   case PIPE_BLENDFACTOR_ONE: 
      return BRW_BLENDFACTOR_ONE; 
   case PIPE_BLENDFACTOR_SRC_COLOR: 
      return BRW_BLENDFACTOR_SRC_COLOR; 
   case PIPE_BLENDFACTOR_INV_SRC_COLOR: 
      return BRW_BLENDFACTOR_INV_SRC_COLOR; 
   case PIPE_BLENDFACTOR_DST_COLOR: 
      return BRW_BLENDFACTOR_DST_COLOR; 
   case PIPE_BLENDFACTOR_INV_DST_COLOR: 
      return BRW_BLENDFACTOR_INV_DST_COLOR; 
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      return BRW_BLENDFACTOR_INV_SRC_ALPHA; 
   case PIPE_BLENDFACTOR_DST_ALPHA: 
      return BRW_BLENDFACTOR_DST_ALPHA; 
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      return BRW_BLENDFACTOR_INV_DST_ALPHA; 
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: 
      return BRW_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      return BRW_BLENDFACTOR_CONST_COLOR; 
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      return BRW_BLENDFACTOR_INV_CONST_COLOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      return BRW_BLENDFACTOR_CONST_ALPHA; 
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      return BRW_BLENDFACTOR_INV_CONST_ALPHA;
   default:
      assert(0);
      return BRW_BLENDFACTOR_ZERO;
   }   
}

static void *brw_create_blend_state( struct pipe_context *pipe,
				     const struct pipe_blend_state *templ )
{
   struct brw_blend_state *blend = CALLOC_STRUCT(brw_blend_state);
   if (blend == NULL)
      return NULL;

   if (templ->logicop_enable) {
      blend->cc2.logicop_enable = 1;
      blend->cc5.logicop_func = translate_logicop(templ->logicop_func);
   } 
   else if (templ->rt[0].blend_enable) {
      blend->cc6.dest_blend_factor = translate_blend_factor(templ->rt[0].rgb_dst_factor);
      blend->cc6.src_blend_factor = translate_blend_factor(templ->rt[0].rgb_src_factor);
      blend->cc6.blend_function = translate_blend_equation(templ->rt[0].rgb_func);

      blend->cc5.ia_dest_blend_factor = translate_blend_factor(templ->rt[0].alpha_dst_factor);
      blend->cc5.ia_src_blend_factor = translate_blend_factor(templ->rt[0].alpha_src_factor);
      blend->cc5.ia_blend_function = translate_blend_equation(templ->rt[0].alpha_func);

      blend->cc3.blend_enable = 1;
      blend->cc3.ia_blend_enable = 
	 (blend->cc6.dest_blend_factor != blend->cc5.ia_dest_blend_factor ||
	  blend->cc6.src_blend_factor != blend->cc5.ia_src_blend_factor ||
	  blend->cc6.blend_function != blend->cc5.ia_blend_function);

      /* Per-surface blend enables, currently just follow global
       * state:
       */
      blend->ss0.color_blend = 1;
   }

   blend->cc5.dither_enable = templ->dither;

   if (BRW_DEBUG & DEBUG_STATS)
      blend->cc5.statistics_enable = 1;

   /* Per-surface color mask -- just follow global state:
    */
   blend->ss0.writedisable_red   = (templ->rt[0].colormask & PIPE_MASK_R) ? 0 : 1;
   blend->ss0.writedisable_green = (templ->rt[0].colormask & PIPE_MASK_G) ? 0 : 1;
   blend->ss0.writedisable_blue  = (templ->rt[0].colormask & PIPE_MASK_B) ? 0 : 1;
   blend->ss0.writedisable_alpha = (templ->rt[0].colormask & PIPE_MASK_A) ? 0 : 1;

   return (void *)blend;
}

static void brw_bind_blend_state(struct pipe_context *pipe,
				 void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   brw->curr.blend = (const struct brw_blend_state *)cso;
   brw->state.dirty.mesa |= PIPE_NEW_BLEND;
}

static void brw_delete_blend_state(struct pipe_context *pipe,
				  void *cso)
{
   struct brw_context *brw = brw_context(pipe);
   assert((const void *)cso != (const void *)brw->curr.blend);
   FREE(cso);
}


static void brw_set_blend_color(struct pipe_context *pipe,
				const struct pipe_blend_color *blend_color)
{
   struct brw_context *brw = brw_context(pipe);
   struct brw_blend_constant_color *bcc = &brw->curr.bcc;

   bcc->blend_constant_color[0] = blend_color->color[0];
   bcc->blend_constant_color[1] = blend_color->color[1];
   bcc->blend_constant_color[2] = blend_color->color[2];
   bcc->blend_constant_color[3] = blend_color->color[3];

   brw->state.dirty.mesa |= PIPE_NEW_BLEND_COLOR;
}


void brw_pipe_blend_init( struct brw_context *brw )
{
   brw->base.set_blend_color = brw_set_blend_color;
   brw->base.create_blend_state = brw_create_blend_state;
   brw->base.bind_blend_state = brw_bind_blend_state;
   brw->base.delete_blend_state = brw_delete_blend_state;

   {
      struct brw_blend_constant_color *bcc = &brw->curr.bcc;

      memset(bcc, 0, sizeof(*bcc));      
      bcc->header.opcode = CMD_BLEND_CONSTANT_COLOR;
      bcc->header.length = sizeof(*bcc)/4-2;
   }

}

void brw_pipe_blend_cleanup( struct brw_context *brw )
{
}
