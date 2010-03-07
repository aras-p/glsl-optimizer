/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include "pipe/p_shader_tokens.h"
            
#include "util/u_memory.h"
#include "util/u_math.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_info.h"

#include "brw_context.h"
#include "brw_vs.h"
#include "brw_debug.h"
#include "brw_disasm.h"

/* Choose one of the 4 vec4's which can be packed into each 16-wide reg.
 */
static INLINE struct brw_reg brw_vec4_grf_repeat( GLuint reg, GLuint slot )
{
   int nr = reg + slot/2;
   int subnr = (slot%2) * 4;

   return stride(brw_vec4_grf(nr, subnr), 0, 4, 1);
}


static struct brw_reg get_tmp( struct brw_vs_compile *c )
{
   struct brw_reg tmp = brw_vec8_grf(c->last_tmp, 0);

   if (++c->last_tmp > c->prog_data.total_grf)
      c->prog_data.total_grf = c->last_tmp;

   return tmp;
}

static void release_tmp( struct brw_vs_compile *c, struct brw_reg tmp )
{
   if (tmp.nr == c->last_tmp-1)
      c->last_tmp--;
}
			       
static void release_tmps( struct brw_vs_compile *c )
{
   c->last_tmp = c->first_tmp;
}


static boolean is_position_output( struct brw_vs_compile *c,
                                   unsigned vs_output )
{
   const struct brw_vertex_shader *vs = c->vp;
   unsigned semantic = vs->info.output_semantic_name[vs_output];
   unsigned index = vs->info.output_semantic_index[vs_output];
      
   return (semantic == TGSI_SEMANTIC_POSITION &&
           index == 0);
}


static boolean find_output_slot( struct brw_vs_compile *c,
                                  unsigned vs_output,
                                  unsigned *fs_input_slot )
{
   const struct brw_vertex_shader *vs = c->vp;
   unsigned semantic = vs->info.output_semantic_name[vs_output];
   unsigned index = vs->info.output_semantic_index[vs_output];
   unsigned i;

   for (i = 0; i < c->key.fs_signature.nr_inputs; i++) {
      if (c->key.fs_signature.input[i].semantic == semantic &&
          c->key.fs_signature.input[i].semantic_index == index) {
         *fs_input_slot = i;
         return TRUE;
      }
   }

   return FALSE;
}


/**
 * Preallocate GRF register before code emit.
 * Do things as simply as possible.  Allocate and populate all regs
 * ahead of time.
 */
static void brw_vs_alloc_regs( struct brw_vs_compile *c )
{
   GLuint i, reg = 0, subreg = 0, mrf;
   int attributes_in_vue;

   /* Determine whether to use a real constant buffer or use a block
    * of GRF registers for constants.  The later is faster but only
    * works if everything fits in the GRF.
    * XXX this heuristic/check may need some fine tuning...
    */
   if (c->vp->info.file_max[TGSI_FILE_CONSTANT] + 1 +
       c->vp->info.file_max[TGSI_FILE_IMMEDIATE] + 1 +
       c->vp->info.file_max[TGSI_FILE_TEMPORARY] + 1 + 21 > BRW_MAX_GRF)
      c->vp->use_const_buffer = GL_TRUE;
   else {
      /* XXX: immediates can go elsewhere if necessary:
       */
      assert(c->vp->info.file_max[TGSI_FILE_IMMEDIATE] + 1 +
	     c->vp->info.file_max[TGSI_FILE_TEMPORARY] + 1 + 21 <= BRW_MAX_GRF);

      c->vp->use_const_buffer = GL_FALSE;
   }

   /*printf("use_const_buffer = %d\n", c->vp->use_const_buffer);*/

   /* r0 -- reserved as usual
    */
   c->r0 = brw_vec8_grf(reg, 0);
   reg++;

   /* User clip planes from curbe: 
    */
   if (c->key.nr_userclip) {
      /* Skip over fixed planes:  Or never read them into vs unit?
       */
      subreg += 6;

      for (i = 0; i < c->key.nr_userclip; i++, subreg++) {
	 c->userplane[i] = 
            stride( brw_vec4_grf(reg+subreg/2, (subreg%2) * 4), 0, 4, 1);
      }     

      /* Deal with curbe alignment:
       */
      subreg = align(subreg, 2);
      /*reg += ((6 + c->key.nr_userclip + 3) / 4) * 2;*/
   }


   /* Immediates: always in the curbe.
    *
    * XXX: Can try to encode some immediates as brw immediates
    * XXX: Make sure ureg sets minimal immediate size and respect it
    * here.
    */
   for (i = 0; i < c->vp->info.immediate_count; i++, subreg++) {
      c->regs[TGSI_FILE_IMMEDIATE][i] = 
         stride( brw_vec4_grf(reg+subreg/2, (subreg%2) * 4), 0, 4, 1);
   }
   c->prog_data.nr_params = c->vp->info.immediate_count * 4;


   /* Vertex constant buffer.
    *
    * Constants from the buffer can be either cached in the curbe or
    * loaded as needed from the actual constant buffer.
    */
   if (!c->vp->use_const_buffer) {
      GLuint nr_params = c->vp->info.file_max[TGSI_FILE_CONSTANT] + 1;

      for (i = 0; i < nr_params; i++, subreg++) {
         c->regs[TGSI_FILE_CONSTANT][i] =
            stride( brw_vec4_grf(reg+subreg/2, (subreg%2) * 4), 0, 4, 1);
      }

      c->prog_data.nr_params += nr_params * 4;
   }

   /* All regs allocated
    */
   reg += (subreg + 1) / 2;
   c->prog_data.curb_read_length = reg - 1;


   /* Allocate input regs:  
    */
   c->nr_inputs = c->vp->info.num_inputs;
   for (i = 0; i < c->nr_inputs; i++) {
      c->regs[TGSI_FILE_INPUT][i] = brw_vec8_grf(reg, 0);
      reg++;
   }

   /* If there are no inputs, we'll still be reading one attribute's worth
    * because it's required -- see urb_read_length setting.
    */
   if (c->nr_inputs == 0)
      reg++;



   /* Allocate outputs.  The non-position outputs go straight into message regs.
    */
   c->nr_outputs = c->prog_data.nr_outputs;

   if (c->chipset.is_igdng)
      mrf = 8;
   else
      mrf = 4;

   
   if (c->key.fs_signature.nr_inputs > BRW_MAX_MRF) {
      c->overflow_grf_start = reg;
      c->overflow_count = c->key.fs_signature.nr_inputs - BRW_MAX_MRF;
      reg += c->overflow_count;
   }

   /* XXX: need to access vertex output semantics here:
    */
   for (i = 0; i < c->nr_outputs; i++) {
      unsigned slot;

      /* XXX: Put output position in slot zero always.  Clipper, etc,
       * need access to this reg.
       */
      if (is_position_output(c, i)) {
	 c->regs[TGSI_FILE_OUTPUT][i] = brw_vec8_grf(reg, 0); /* copy to mrf 0 */
	 reg++;
      }
      else if (find_output_slot(c, i, &slot)) {
         
         if (0 /* is_psize_output(c, i) */ ) {
            /* c->psize_out.grf = reg; */
            /* c->psize_out.mrf = i; */
         }
         
         /* The first (16-4) outputs can go straight into the message regs.
          */
         if (slot + mrf < BRW_MAX_MRF) {
            c->regs[TGSI_FILE_OUTPUT][i] = brw_message_reg(slot + mrf);
         }
         else {
            int grf = c->overflow_grf_start + slot - BRW_MAX_MRF;
            c->regs[TGSI_FILE_OUTPUT][i] = brw_vec8_grf(grf, 0);
         }
      }
      else {
         c->regs[TGSI_FILE_OUTPUT][i] = brw_null_reg();
      }
   }     

   /* Allocate program temporaries:
    */
   
   for (i = 0; i < c->vp->info.file_max[TGSI_FILE_TEMPORARY]+1; i++) {
      c->regs[TGSI_FILE_TEMPORARY][i] = brw_vec8_grf(reg, 0);
      reg++;
   }

   /* Address reg(s).  Don't try to use the internal address reg until
    * deref time.
    */
   for (i = 0; i < c->vp->info.file_max[TGSI_FILE_ADDRESS]+1; i++) {
      c->regs[TGSI_FILE_ADDRESS][i] =  brw_reg(BRW_GENERAL_REGISTER_FILE,
					     reg,
					     0,
					     BRW_REGISTER_TYPE_D,
					     BRW_VERTICAL_STRIDE_8,
					     BRW_WIDTH_8,
					     BRW_HORIZONTAL_STRIDE_1,
					     BRW_SWIZZLE_XXXX,
					     BRW_WRITEMASK_X);
      reg++;
   }

   if (c->vp->use_const_buffer) {
      for (i = 0; i < 3; i++) {
         c->current_const[i].index = -1;
         c->current_const[i].reg = brw_vec8_grf(reg, 0);
         reg++;
      }
   }

#if 0
   for (i = 0; i < 128; i++) {
      if (c->output_regs[i].used_in_src) {
         c->output_regs[i].reg = brw_vec8_grf(reg, 0);
         reg++;
      }
   }
#endif

   if (c->vp->has_flow_control) {
      c->stack =  brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, reg, 0);
      reg += 2;
   }

   /* Some opcodes need an internal temporary:
    */
   c->first_tmp = reg;
   c->last_tmp = reg;		/* for allocation purposes */

   /* Each input reg holds data from two vertices.  The
    * urb_read_length is the number of registers read from *each*
    * vertex urb, so is half the amount:
    */
   c->prog_data.urb_read_length = (c->nr_inputs + 1) / 2;

   /* Setting this field to 0 leads to undefined behavior according to the
    * the VS_STATE docs.  Our VUEs will always have at least one attribute
    * sitting in them, even if it's padding.
    */
   if (c->prog_data.urb_read_length == 0)
      c->prog_data.urb_read_length = 1;

   /* The VS VUEs are shared by VF (outputting our inputs) and VS, so size
    * them to fit the biggest thing they need to.
    */
   attributes_in_vue = MAX2(c->nr_outputs, c->nr_inputs);

   if (c->chipset.is_igdng)
      c->prog_data.urb_entry_size = (attributes_in_vue + 6 + 3) / 4;
   else
      c->prog_data.urb_entry_size = (attributes_in_vue + 2 + 3) / 4;

   c->prog_data.total_grf = reg;

   if (BRW_DEBUG & DEBUG_VS) {
      debug_printf("%s NumAddrRegs %d\n", __FUNCTION__, 
		   c->vp->info.file_max[TGSI_FILE_ADDRESS]+1);
      debug_printf("%s NumTemps %d\n", __FUNCTION__,
		   c->vp->info.file_max[TGSI_FILE_TEMPORARY]+1);
      debug_printf("%s reg = %d\n", __FUNCTION__, reg);
   }
}


/**
 * If an instruction uses a temp reg both as a src and the dest, we
 * sometimes need to allocate an intermediate temporary.
 */
static void unalias1( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg ))
{
   if (dst.file == arg0.file && dst.nr == arg0.nr) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0);
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
   else {
      func(c, dst, arg0);
   }
}

/**
 * \sa unalias2
 * Checkes if 2-operand instruction needs an intermediate temporary.
 */
static void unalias2( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg ))
{
   if ((dst.file == arg0.file && dst.nr == arg0.nr) ||
       (dst.file == arg1.file && dst.nr == arg1.nr)) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0, arg1);
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
   else {
      func(c, dst, arg0, arg1);
   }
}

/**
 * \sa unalias2
 * Checkes if 3-operand instruction needs an intermediate temporary.
 */
static void unalias3( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      struct brw_reg arg2,
		      void (*func)( struct brw_vs_compile *,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg,
				    struct brw_reg ))
{
   if ((dst.file == arg0.file && dst.nr == arg0.nr) ||
       (dst.file == arg1.file && dst.nr == arg1.nr) ||
       (dst.file == arg2.file && dst.nr == arg2.nr)) {
      struct brw_compile *p = &c->func;
      struct brw_reg tmp = brw_writemask(get_tmp(c), dst.dw1.bits.writemask);
      func(c, tmp, arg0, arg1, arg2);
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
   else {
      func(c, dst, arg0, arg1, arg2);
   }
}

static void emit_sop( struct brw_compile *p,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1, 
		      GLuint cond)
{
   brw_MOV(p, dst, brw_imm_f(0.0f));
   brw_CMP(p, brw_null_reg(), cond, arg0, arg1);
   brw_MOV(p, dst, brw_imm_f(1.0f));
   brw_set_predicate_control_flag_value(p, 0xff);
}

static void emit_seq( struct brw_compile *p,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1 )
{
   emit_sop(p, dst, arg0, arg1, BRW_CONDITIONAL_EQ);
}

static void emit_sne( struct brw_compile *p,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1 )
{
   emit_sop(p, dst, arg0, arg1, BRW_CONDITIONAL_NEQ);
}
static void emit_slt( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(p, dst, arg0, arg1, BRW_CONDITIONAL_L);
}

static void emit_sle( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(p, dst, arg0, arg1, BRW_CONDITIONAL_LE);
}

static void emit_sgt( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(p, dst, arg0, arg1, BRW_CONDITIONAL_G);
}

static void emit_sge( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
  emit_sop(p, dst, arg0, arg1, BRW_CONDITIONAL_GE);
}

static void emit_max( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, arg1);
   brw_SEL(p, dst, arg1, arg0);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
}

static void emit_min( struct brw_compile *p, 
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, arg1);
   brw_SEL(p, dst, arg0, arg1);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
}


static void emit_math1( struct brw_vs_compile *c,
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			GLuint precision)
{
   /* There are various odd behaviours with SEND on the simulator.  In
    * addition there are documented issues with the fact that the GEN4
    * processor doesn't do dependency control properly on SEND
    * results.  So, on balance, this kludge to get around failures
    * with writemasked math results looks like it might be necessary
    * whether that turns out to be a simulator bug or not:
    */
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);

   brw_math(p, 
	    tmp,
	    function,
	    BRW_MATH_SATURATE_NONE,
	    2,
	    arg0,
	    BRW_MATH_DATA_SCALAR,
	    precision);

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}


static void emit_math2( struct brw_vs_compile *c, 
			GLuint function,
			struct brw_reg dst,
			struct brw_reg arg0,
			struct brw_reg arg1,
			GLuint precision)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);

   brw_MOV(p, brw_message_reg(3), arg1);
   
   brw_math(p, 
	    tmp,
	    function,
	    BRW_MATH_SATURATE_NONE,
	    2,
 	    arg0,
	    BRW_MATH_DATA_SCALAR,
	    precision);

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}


static void emit_exp_noalias( struct brw_vs_compile *c,
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   

   if (dst.dw1.bits.writemask & BRW_WRITEMASK_X) {
      struct brw_reg tmp = get_tmp(c);
      struct brw_reg tmp_d = retype(tmp, BRW_REGISTER_TYPE_D);

      /* tmp_d = floor(arg0.x) */
      brw_RNDD(p, tmp_d, brw_swizzle1(arg0, 0));

      /* result[0] = 2.0 ^ tmp */

      /* Adjust exponent for floating point: 
       * exp += 127 
       */
      brw_ADD(p, brw_writemask(tmp_d, BRW_WRITEMASK_X), tmp_d, brw_imm_d(127));

      /* Install exponent and sign.  
       * Excess drops off the edge: 
       */
      brw_SHL(p, brw_writemask(retype(dst, BRW_REGISTER_TYPE_D), BRW_WRITEMASK_X), 
	      tmp_d, brw_imm_d(23));

      release_tmp(c, tmp);
   }

   if (dst.dw1.bits.writemask & BRW_WRITEMASK_Y) {
      /* result[1] = arg0.x - floor(arg0.x) */
      brw_FRC(p, brw_writemask(dst, BRW_WRITEMASK_Y), brw_swizzle1(arg0, 0));
   }
   
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_Z) {
      /* As with the LOG instruction, we might be better off just
       * doing a taylor expansion here, seeing as we have to do all
       * the prep work.
       *
       * If mathbox partial precision is too low, consider also:
       * result[3] = result[0] * EXP(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_EXP, 
		 brw_writemask(dst, BRW_WRITEMASK_Z),
		 brw_swizzle1(arg0, 0), 
		 BRW_MATH_PRECISION_FULL);
   }  

   if (dst.dw1.bits.writemask & BRW_WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_W), brw_imm_f(1));
   }
}


static void emit_log_noalias( struct brw_vs_compile *c,
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   struct brw_reg tmp_ud = retype(tmp, BRW_REGISTER_TYPE_UD);
   struct brw_reg arg0_ud = retype(arg0, BRW_REGISTER_TYPE_UD);
   GLboolean need_tmp = (dst.dw1.bits.writemask != 0xf ||
			 dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) {
      tmp = get_tmp(c);
      tmp_ud = retype(tmp, BRW_REGISTER_TYPE_UD);
   }
   
   /* Perform mant = frexpf(fabsf(x), &exp), adjust exp and mnt
    * according to spec:
    *
    * These almost look likey they could be joined up, but not really
    * practical:
    *
    * result[0].f = (x.i & ((1<<31)-1) >> 23) - 127
    * result[1].i = (x.i & ((1<<23)-1)        + (127<<23)
    */
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_XZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, BRW_WRITEMASK_X),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1U<<31)-1));

      brw_SHR(p, 
	      brw_writemask(tmp_ud, BRW_WRITEMASK_X), 
	      tmp_ud,
	      brw_imm_ud(23));

      brw_ADD(p, 
	      brw_writemask(tmp, BRW_WRITEMASK_X), 
	      retype(tmp_ud, BRW_REGISTER_TYPE_D),	/* does it matter? */
	      brw_imm_d(-127));
   }

   if (dst.dw1.bits.writemask & BRW_WRITEMASK_YZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, BRW_WRITEMASK_Y),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1<<23)-1));

      brw_OR(p, 
	     brw_writemask(tmp_ud, BRW_WRITEMASK_Y), 
	     tmp_ud,
	     brw_imm_ud(127<<23));
   }
   
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_Z) {
      /* result[2] = result[0] + LOG2(result[1]); */

      /* Why bother?  The above is just a hint how to do this with a
       * taylor series.  Maybe we *should* use a taylor series as by
       * the time all the above has been done it's almost certainly
       * quicker than calling the mathbox, even with low precision.
       * 
       * Options are:
       *    - result[0] + mathbox.LOG2(result[1])
       *    - mathbox.LOG2(arg0.x)
       *    - result[0] + inline_taylor_approx(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_LOG, 
		 brw_writemask(tmp, BRW_WRITEMASK_Z), 
		 brw_swizzle1(tmp, 1), 
		 BRW_MATH_PRECISION_FULL);
      
      brw_ADD(p, 
	      brw_writemask(tmp, BRW_WRITEMASK_Z), 
	      brw_swizzle1(tmp, 2), 
	      brw_swizzle1(tmp, 0));
   }  

   if (dst.dw1.bits.writemask & BRW_WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(tmp, BRW_WRITEMASK_W), brw_imm_f(1));
   }

   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}


/* Need to unalias - consider swizzles:   r0 = DST r0.xxxx r1
 */
static void emit_dst_noalias( struct brw_vs_compile *c, 
			      struct brw_reg dst,
			      struct brw_reg arg0,
			      struct brw_reg arg1)
{
   struct brw_compile *p = &c->func;

   /* There must be a better way to do this: 
    */
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_X)
      brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_X), brw_imm_f(1.0));
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_Y)
      brw_MUL(p, brw_writemask(dst, BRW_WRITEMASK_Y), arg0, arg1);
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_Z)
      brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_Z), arg0);
   if (dst.dw1.bits.writemask & BRW_WRITEMASK_W)
      brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_W), arg1);
}


static void emit_xpd( struct brw_compile *p,
		      struct brw_reg dst,
		      struct brw_reg t,
		      struct brw_reg u)
{
   brw_MUL(p, brw_null_reg(), brw_swizzle(t, 1,2,0,3),  brw_swizzle(u,2,0,1,3));
   brw_MAC(p, dst,     negate(brw_swizzle(t, 2,0,1,3)), brw_swizzle(u,1,2,0,3));
}


static void emit_lit_noalias( struct brw_vs_compile *c, 
			      struct brw_reg dst,
			      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *if_insn;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.file != BRW_GENERAL_REGISTER_FILE);

   if (need_tmp) 
      tmp = get_tmp(c);
   
   brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_YZ), brw_imm_f(0)); 
   brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_XW), brw_imm_f(1)); 

   /* Need to use BRW_EXECUTE_8 and also do an 8-wide compare in order
    * to get all channels active inside the IF.  In the clipping code
    * we run with NoMask, so it's not an option and we can use
    * BRW_EXECUTE_1 for all comparisions.
    */
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,0), brw_imm_f(0));
   if_insn = brw_IF(p, BRW_EXECUTE_8);
   {
      brw_MOV(p, brw_writemask(dst, BRW_WRITEMASK_Y), brw_swizzle1(arg0,0));

      brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,1), brw_imm_f(0));
      brw_MOV(p, brw_writemask(tmp, BRW_WRITEMASK_Z),  brw_swizzle1(arg0,1));
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);

      emit_math2(c, 
		 BRW_MATH_FUNCTION_POW, 
		 brw_writemask(dst, BRW_WRITEMASK_Z),
		 brw_swizzle1(tmp, 2),
		 brw_swizzle1(arg0, 3),
		 BRW_MATH_PRECISION_PARTIAL);      
   }

   brw_ENDIF(p, if_insn);

   release_tmp(c, tmp);
}

static void emit_lrp_noalias(struct brw_vs_compile *c,
			     struct brw_reg dst,
			     struct brw_reg arg0,
			     struct brw_reg arg1,
			     struct brw_reg arg2)
{
   struct brw_compile *p = &c->func;

   brw_ADD(p, dst, negate(arg0), brw_imm_f(1.0));
   brw_MUL(p, brw_null_reg(), dst, arg2);
   brw_MAC(p, dst, arg0, arg1);
}

/** 3 or 4-component vector normalization */
static void emit_nrm( struct brw_vs_compile *c, 
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      int num_comps)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = get_tmp(c);

   /* tmp = dot(arg0, arg0) */
   if (num_comps == 3)
      brw_DP3(p, tmp, arg0, arg0);
   else
      brw_DP4(p, tmp, arg0, arg0);

   /* tmp = 1 / sqrt(tmp) */
   emit_math1(c, BRW_MATH_FUNCTION_RSQ, tmp, tmp, BRW_MATH_PRECISION_FULL);

   /* dst = arg0 * tmp */
   brw_MUL(p, dst, arg0, tmp);

   release_tmp(c, tmp);
}


static struct brw_reg
get_constant(struct brw_vs_compile *c,
	     GLuint argIndex,
	     GLuint index,
	     GLboolean relAddr)
{
   struct brw_compile *p = &c->func;
   struct brw_reg const_reg;
   struct brw_reg const2_reg;

   assert(argIndex < 3);

   if (c->current_const[argIndex].index != index || relAddr) {
      struct brw_reg addrReg = c->regs[TGSI_FILE_ADDRESS][0];

      c->current_const[argIndex].index = index;

#if 0
      printf("  fetch const[%d] for arg %d into reg %d\n",
             src.Index, argIndex, c->current_const[argIndex].reg.nr);
#endif
      /* need to fetch the constant now */
      brw_dp_READ_4_vs(p,
                       c->current_const[argIndex].reg,/* writeback dest */
                       0,                             /* oword */
                       relAddr,                       /* relative indexing? */
                       addrReg,                       /* address register */
                       16 * index,               /* byte offset */
                       SURF_INDEX_VERT_CONST_BUFFER   /* binding table index */
                       );

      if (relAddr) {
         /* second read */
         const2_reg = get_tmp(c);

         /* use upper half of address reg for second read */
         addrReg = stride(addrReg, 0, 4, 0);
         addrReg.subnr = 16;

         brw_dp_READ_4_vs(p,
                          const2_reg,              /* writeback dest */
                          1,                       /* oword */
                          relAddr,                 /* relative indexing? */
                          addrReg,                 /* address register */
                          16 * index,         /* byte offset */
                          SURF_INDEX_VERT_CONST_BUFFER
                          );
      }
   }

   const_reg = c->current_const[argIndex].reg;

   if (relAddr) {
      /* merge the two Owords into the constant register */
      /* const_reg[7..4] = const2_reg[7..4] */
      brw_MOV(p,
              suboffset(stride(const_reg, 0, 4, 1), 4),
              suboffset(stride(const2_reg, 0, 4, 1), 4));
      release_tmp(c, const2_reg);
   }
   else {
      /* replicate lower four floats into upper half (to get XYZWXYZW) */
      const_reg = stride(const_reg, 0, 4, 0);
      const_reg.subnr = 0;
   }

   return const_reg;
}


#if 0

/* TODO: relative addressing!
 */
static struct brw_reg get_reg( struct brw_vs_compile *c,
			       enum tgsi_file_type file,
			       GLuint index )
{
   switch (file) {
   case TGSI_FILE_TEMPORARY:
   case TGSI_FILE_INPUT:
   case TGSI_FILE_OUTPUT:
   case TGSI_FILE_CONSTANT:
      assert(c->regs[file][index].nr != 0);
      return c->regs[file][index];

   case TGSI_FILE_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case TGSI_FILE_NULL:			/* undef values */
      return brw_null_reg();

   default:
      assert(0);
      return brw_null_reg();
   }
}

#endif


/**
 * Indirect addressing:  get reg[[arg] + offset].
 */
static struct brw_reg deref( struct brw_vs_compile *c,
			     struct brw_reg arg,
			     GLint offset)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = vec4(get_tmp(c));
   struct brw_reg addr_reg = c->regs[TGSI_FILE_ADDRESS][0];
   struct brw_reg vp_address = retype(vec1(addr_reg), BRW_REGISTER_TYPE_UW);
   GLuint byte_offset = arg.nr * 32 + arg.subnr + offset * 16;
   struct brw_reg indirect = brw_vec4_indirect(0,0);

   {
      brw_push_insn_state(p);
      brw_set_access_mode(p, BRW_ALIGN_1);

      /* This is pretty clunky - load the address register twice and
       * fetch each 4-dword value in turn.  There must be a way to do
       * this in a single pass, but I couldn't get it to work.
       */
      brw_ADD(p, brw_address_reg(0), vp_address, brw_imm_d(byte_offset));
      brw_MOV(p, tmp, indirect);

      brw_ADD(p, brw_address_reg(0), suboffset(vp_address, 8), brw_imm_d(byte_offset));
      brw_MOV(p, suboffset(tmp, 4), indirect);

      brw_pop_insn_state(p);
   }
   
   /* NOTE: tmp not released */
   return vec8(tmp);
}


/**
 * Get brw reg corresponding to the instruction's [argIndex] src reg.
 * TODO: relative addressing!
 */
static struct brw_reg
get_src_reg( struct brw_vs_compile *c,
	     GLuint argIndex,
	     GLuint file,
	     GLint index,
	     GLboolean relAddr )
{

   switch (file) {
   case TGSI_FILE_TEMPORARY:
   case TGSI_FILE_INPUT:
   case TGSI_FILE_OUTPUT:
      if (relAddr) {
         return deref(c, c->regs[file][0], index);
      }
      else {
         assert(c->regs[file][index].nr != 0);
         return c->regs[file][index];
      }

   case TGSI_FILE_IMMEDIATE:
      return c->regs[file][index];

   case TGSI_FILE_CONSTANT:
      if (c->vp->use_const_buffer) {
         return get_constant(c, argIndex, index, relAddr);
      }
      else if (relAddr) {
         return deref(c, c->regs[TGSI_FILE_CONSTANT][0], index);
      }
      else {
         assert(c->regs[TGSI_FILE_CONSTANT][index].nr != 0);
         return c->regs[TGSI_FILE_CONSTANT][index];
      }
   case TGSI_FILE_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case TGSI_FILE_NULL:
      /* this is a normal case since we loop over all three src args */
      return brw_null_reg();

   default:
      assert(0);
      return brw_null_reg();
   }
}


static void emit_arl( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0 )
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = dst;
   GLboolean need_tmp = (dst.file != BRW_GENERAL_REGISTER_FILE);
   
   if (need_tmp) 
      tmp = get_tmp(c);

   brw_RNDD(p, tmp, arg0);               /* tmp = round(arg0) */
   brw_MUL(p, dst, tmp, brw_imm_d(16));  /* dst = tmp * 16 */

   if (need_tmp)
      release_tmp(c, tmp);
}


/**
 * Return the brw reg for the given instruction's src argument.
 */
static struct brw_reg get_arg( struct brw_vs_compile *c,
                               const struct tgsi_full_src_register *src,
                               GLuint argIndex )
{
   struct brw_reg reg;

   if (src->Register.File == TGSI_FILE_NULL)
      return brw_null_reg();

   reg = get_src_reg(c, argIndex,
		     src->Register.File,
		     src->Register.Index,
		     src->Register.Indirect);

   /* Convert 3-bit swizzle to 2-bit.  
    */
   reg.dw1.bits.swizzle = BRW_SWIZZLE4(src->Register.SwizzleX,
				       src->Register.SwizzleY,
				       src->Register.SwizzleZ,
				       src->Register.SwizzleW);

   reg.negate = src->Register.Negate ? 1 : 0;   

   /* XXX: abs, absneg
    */

   return reg;
}


/**
 * Get brw register for the given program dest register.
 */
static struct brw_reg get_dst( struct brw_vs_compile *c,
			       unsigned file,
			       unsigned index,
			       unsigned writemask )
{
   struct brw_reg reg;

   switch (file) {
   case TGSI_FILE_TEMPORARY:
   case TGSI_FILE_OUTPUT:
      assert(c->regs[file][index].nr != 0);
      reg = c->regs[file][index];
      break;
   case TGSI_FILE_ADDRESS:
      assert(index == 0);
      reg = c->regs[file][index];
      break;
   case TGSI_FILE_NULL:
      /* we may hit this for OPCODE_END, OPCODE_KIL, etc */
      reg = brw_null_reg();
      break;
   default:
      assert(0);
      reg = brw_null_reg();
   }

   reg.dw1.bits.writemask = writemask;

   return reg;
}




/**
 * Post-vertex-program processing.  Send the results to the URB.
 */
static void emit_vertex_write( struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_reg m0 = brw_message_reg(0);
   struct brw_reg pos = c->regs[TGSI_FILE_OUTPUT][VERT_RESULT_HPOS];
   struct brw_reg ndc;
   int eot;
   int i;
   GLuint len_vertext_header = 2;

   /* Build ndc coords */
   ndc = get_tmp(c);
   /* ndc = 1.0 / pos.w */
   emit_math1(c, BRW_MATH_FUNCTION_INV, ndc, brw_swizzle1(pos, 3), BRW_MATH_PRECISION_FULL);
   /* ndc.xyz = pos * ndc */
   brw_MUL(p, brw_writemask(ndc, BRW_WRITEMASK_XYZ), pos, ndc);

   /* Update the header for point size, user clipping flags, and -ve rhw
    * workaround.
    */
   if (c->prog_data.writes_psiz ||
       c->key.nr_userclip || 
       c->chipset.is_965)
   {
      struct brw_reg header1 = retype(get_tmp(c), BRW_REGISTER_TYPE_UD);
      GLuint i;

      brw_MOV(p, header1, brw_imm_ud(0));

      brw_set_access_mode(p, BRW_ALIGN_16);	

      if (c->prog_data.writes_psiz) {
	 struct brw_reg psiz = c->regs[TGSI_FILE_OUTPUT][VERT_RESULT_PSIZ];
	 brw_MUL(p, brw_writemask(header1, BRW_WRITEMASK_W), brw_swizzle1(psiz, 0), brw_imm_f(1<<11));
	 brw_AND(p, brw_writemask(header1, BRW_WRITEMASK_W), header1, brw_imm_ud(0x7ff<<8));
      }

      for (i = 0; i < c->key.nr_userclip; i++) {
	 brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	 brw_DP4(p, brw_null_reg(), pos, c->userplane[i]);
	 brw_OR(p, brw_writemask(header1, BRW_WRITEMASK_W), header1, brw_imm_ud(1<<i));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      }

      /* i965 clipping workaround: 
       * 1) Test for -ve rhw
       * 2) If set, 
       *      set ndc = (0,0,0,0)
       *      set ucp[6] = 1
       *
       * Later, clipping will detect ucp[6] and ensure the primitive is
       * clipped against all fixed planes.
       */
      if (c->chipset.is_965) {
	 brw_CMP(p,
		 vec8(brw_null_reg()),
		 BRW_CONDITIONAL_L,
		 brw_swizzle1(ndc, 3),
		 brw_imm_f(0));
   
	 brw_OR(p, brw_writemask(header1, BRW_WRITEMASK_W), header1, brw_imm_ud(1<<6));
	 brw_MOV(p, ndc, brw_imm_f(0));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      }

      brw_set_access_mode(p, BRW_ALIGN_1);	/* why? */
      brw_MOV(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_UD), header1);
      brw_set_access_mode(p, BRW_ALIGN_16);

      release_tmp(c, header1);
   }
   else {
      brw_MOV(p, retype(brw_message_reg(1), BRW_REGISTER_TYPE_UD), brw_imm_ud(0));
   }

   /* Emit the (interleaved) headers for the two vertices - an 8-reg
    * of zeros followed by two sets of NDC coordinates:
    */
   brw_set_access_mode(p, BRW_ALIGN_1);
   brw_MOV(p, offset(m0, 2), ndc);

   if (c->chipset.is_igdng) {
       /* There are 20 DWs (D0-D19) in VUE vertex header on IGDNG */
       brw_MOV(p, offset(m0, 3), pos); /* a portion of vertex header */
       /* m4, m5 contain the distances from vertex to the user clip planeXXX. 
        * Seems it is useless for us.
        * m6 is used for aligning, so that the remainder of vertex element is 
        * reg-aligned.
        */
       brw_MOV(p, offset(m0, 7), pos); /* the remainder of vertex element */
       len_vertext_header = 6;
   } else {
       brw_MOV(p, offset(m0, 3), pos);
       len_vertext_header = 2;
   }

   eot = (c->overflow_count == 0);

   brw_urb_WRITE(p, 
		 brw_null_reg(), /* dest */
		 0,		/* starting mrf reg nr */
		 c->r0,		/* src */
		 0,		/* allocate */
		 1,		/* used */
		 MIN2(c->nr_outputs + 1 + len_vertext_header, (BRW_MAX_MRF-1)), /* msg len */
		 0,		/* response len */
		 eot, 		/* eot */
		 eot, 		/* writes complete */
		 0, 		/* urb destination offset */
		 BRW_URB_SWIZZLE_INTERLEAVE);

   /* Not all of the vertex outputs/results fit into the MRF.
    * Move the overflowed attributes from the GRF to the MRF and
    * issue another brw_urb_WRITE().
    */
   for (i = 0; i < c->overflow_count; i += BRW_MAX_MRF) {
      unsigned nr = MIN2(c->overflow_count - i, BRW_MAX_MRF);
      GLuint j;

      eot = (i + nr >= c->overflow_count);

      /* XXX I'm not 100% sure about which MRF regs to use here.  Starting
       * at mrf[4] atm...
       */
      for (j = 0; j < nr; j++) {
	 brw_MOV(p, brw_message_reg(4+j), 
                 brw_vec8_grf(c->overflow_grf_start + i + j, 0));
      }

      brw_urb_WRITE(p,
                    brw_null_reg(), /* dest */
                    4,              /* starting mrf reg nr */
                    c->r0,          /* src */
                    0,              /* allocate */
                    1,              /* used */
                    nr+1,          /* msg len */
                    0,              /* response len */
                    eot,            /* eot */
                    eot,            /* writes complete */
                    i-1,            /* urb destination offset */
                    BRW_URB_SWIZZLE_INTERLEAVE);
   }
}


/**
 * Called after code generation to resolve subroutine calls and the
 * END instruction.
 * \param end_inst  points to brw code for END instruction
 * \param last_inst  points to last instruction emitted before vertex write
 */
static void 
post_vs_emit( struct brw_vs_compile *c,
              struct brw_instruction *end_inst,
              struct brw_instruction *last_inst )
{
   GLint offset;

   brw_resolve_cals(&c->func);

   /* patch up the END code to jump past subroutines, etc */
   offset = last_inst - end_inst;
   if (offset > 1) {
      brw_set_src1(end_inst, brw_imm_d(offset * 16));
   } else {
      end_inst->header.opcode = BRW_OPCODE_NOP;
   }
}

static uint32_t
get_predicate(const struct tgsi_full_instruction *inst)
{
   /* XXX: disabling for now
    */
#if 0
   if (inst->dst.CondMask == COND_TR)
      return BRW_PREDICATE_NONE;

   /* All of GLSL only produces predicates for COND_NE and one channel per
    * vector.  Fail badly if someone starts doing something else, as it might
    * mean infinite looping or something.
    *
    * We'd like to support all the condition codes, but our hardware doesn't
    * quite match the Mesa IR, which is modeled after the NV extensions.  For
    * those, the instruction may update the condition codes or not, then any
    * later instruction may use one of those condition codes.  For gen4, the
    * instruction may update the flags register based on one of the condition
    * codes output by the instruction, and then further instructions may
    * predicate on that.  We can probably support this, but it won't
    * necessarily be easy.
    */
/*   assert(inst->dst.CondMask == COND_NE); */

   switch (inst->dst.CondSwizzle) {
   case SWIZZLE_XXXX:
      return BRW_PREDICATE_ALIGN16_REPLICATE_X;
   case SWIZZLE_YYYY:
      return BRW_PREDICATE_ALIGN16_REPLICATE_Y;
   case SWIZZLE_ZZZZ:
      return BRW_PREDICATE_ALIGN16_REPLICATE_Z;
   case SWIZZLE_WWWW:
      return BRW_PREDICATE_ALIGN16_REPLICATE_W;
   default:
      debug_printf("Unexpected predicate: 0x%08x\n",
		    inst->dst.CondMask);
      return BRW_PREDICATE_NORMAL;
   }
#else
   return BRW_PREDICATE_NORMAL;
#endif
}

static void emit_insn(struct brw_vs_compile *c,
		      const struct tgsi_full_instruction *inst)
{
   unsigned opcode = inst->Instruction.Opcode;
   unsigned label = inst->Label.Label;
   struct brw_compile *p = &c->func;
   struct brw_reg args[3], dst;
   GLuint i;

#if 0
   printf("%d: ", insn);
   _mesa_print_instruction(inst);
#endif

   /* Get argument regs.
    */
   for (i = 0; i < 3; i++) {
      args[i] = get_arg(c, &inst->Src[i], i);
   }

   /* Get dest regs.  Note that it is possible for a reg to be both
    * dst and arg, given the static allocation of registers.  So
    * care needs to be taken emitting multi-operation instructions.
    */ 
   dst = get_dst(c, 
		 inst->Dst[0].Register.File,
		 inst->Dst[0].Register.Index,
		 inst->Dst[0].Register.WriteMask);

   /* XXX: saturate
    */
   if (inst->Instruction.Saturate != TGSI_SAT_NONE) {
      debug_printf("Unsupported saturate in vertex shader");
   }

   switch (opcode) {
   case TGSI_OPCODE_ABS:
      brw_MOV(p, dst, brw_abs(args[0]));
      break;
   case TGSI_OPCODE_ADD:
      brw_ADD(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_COS:
      emit_math1(c, BRW_MATH_FUNCTION_COS, dst, args[0], BRW_MATH_PRECISION_FULL);
      break;
   case TGSI_OPCODE_DP3:
      brw_DP3(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_DP4:
      brw_DP4(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_DPH:
      brw_DPH(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_NRM:
      emit_nrm(c, dst, args[0], 3);
      break;
   case TGSI_OPCODE_NRM4:
      emit_nrm(c, dst, args[0], 4);
      break;
   case TGSI_OPCODE_DST:
      unalias2(c, dst, args[0], args[1], emit_dst_noalias); 
      break;
   case TGSI_OPCODE_EXP:
      unalias1(c, dst, args[0], emit_exp_noalias);
      break;
   case TGSI_OPCODE_EX2:
      emit_math1(c, BRW_MATH_FUNCTION_EXP, dst, args[0], BRW_MATH_PRECISION_FULL);
      break;
   case TGSI_OPCODE_ARL:
      emit_arl(c, dst, args[0]);
      break;
   case TGSI_OPCODE_FLR:
      brw_RNDD(p, dst, args[0]);
      break;
   case TGSI_OPCODE_FRC:
      brw_FRC(p, dst, args[0]);
      break;
   case TGSI_OPCODE_LOG:
      unalias1(c, dst, args[0], emit_log_noalias);
      break;
   case TGSI_OPCODE_LG2:
      emit_math1(c, BRW_MATH_FUNCTION_LOG, dst, args[0], BRW_MATH_PRECISION_FULL);
      break;
   case TGSI_OPCODE_LIT:
      unalias1(c, dst, args[0], emit_lit_noalias);
      break;
   case TGSI_OPCODE_LRP:
      unalias3(c, dst, args[0], args[1], args[2], emit_lrp_noalias);
      break;
   case TGSI_OPCODE_MAD:
      brw_MOV(p, brw_acc_reg(), args[2]);
      brw_MAC(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_MAX:
      emit_max(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_MIN:
      emit_min(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_MOV:
      brw_MOV(p, dst, args[0]);
      break;
   case TGSI_OPCODE_MUL:
      brw_MUL(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_POW:
      emit_math2(c, BRW_MATH_FUNCTION_POW, dst, args[0], args[1], BRW_MATH_PRECISION_FULL); 
      break;
   case TGSI_OPCODE_RCP:
      emit_math1(c, BRW_MATH_FUNCTION_INV, dst, args[0], BRW_MATH_PRECISION_FULL);
      break;
   case TGSI_OPCODE_RSQ:
      emit_math1(c, BRW_MATH_FUNCTION_RSQ, dst, 
                 brw_swizzle(args[0], 0,0,0,0), BRW_MATH_PRECISION_FULL);
      break;
   case TGSI_OPCODE_SEQ:
      emit_seq(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_SIN:
      emit_math1(c, BRW_MATH_FUNCTION_SIN, dst, args[0], BRW_MATH_PRECISION_FULL);
      break;
   case TGSI_OPCODE_SNE:
      emit_sne(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_SGE:
      emit_sge(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_SGT:
      emit_sgt(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_SLT:
      emit_slt(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_SLE:
      emit_sle(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_SUB:
      brw_ADD(p, dst, args[0], negate(args[1]));
      break;
   case TGSI_OPCODE_TRUNC:
      /* round toward zero */
      brw_RNDZ(p, dst, args[0]);
      break;
   case TGSI_OPCODE_XPD:
      emit_xpd(p, dst, args[0], args[1]);
      break;
   case TGSI_OPCODE_IF:
      assert(c->if_depth < MAX_IF_DEPTH);
      c->if_inst[c->if_depth] = brw_IF(p, BRW_EXECUTE_8);
      /* Note that brw_IF smashes the predicate_control field. */
      c->if_inst[c->if_depth]->header.predicate_control = get_predicate(inst);
      c->if_depth++;
      break;
   case TGSI_OPCODE_ELSE:
      c->if_inst[c->if_depth-1] = brw_ELSE(p, c->if_inst[c->if_depth-1]);
      break;
   case TGSI_OPCODE_ENDIF:
      assert(c->if_depth > 0);
      brw_ENDIF(p, c->if_inst[--c->if_depth]);
      break;			
   case TGSI_OPCODE_BGNLOOP:
      c->loop_inst[c->loop_depth++] = brw_DO(p, BRW_EXECUTE_8);
      break;
   case TGSI_OPCODE_BRK:
      brw_set_predicate_control(p, get_predicate(inst));
      brw_BREAK(p);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;
   case TGSI_OPCODE_CONT:
      brw_set_predicate_control(p, get_predicate(inst));
      brw_CONT(p);
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;
   case TGSI_OPCODE_ENDLOOP: 
   {
      struct brw_instruction *inst0, *inst1;
      GLuint br = 1;

      c->loop_depth--;

      if (c->chipset.is_igdng)
	 br = 2;

      inst0 = inst1 = brw_WHILE(p, c->loop_inst[c->loop_depth]);
      /* patch all the BREAK/CONT instructions from last BEGINLOOP */
      while (inst0 > c->loop_inst[c->loop_depth]) {
	 inst0--;
	 if (inst0->header.opcode == TGSI_OPCODE_BRK) {
	    inst0->bits3.if_else.jump_count = br * (inst1 - inst0 + 1);
	    inst0->bits3.if_else.pop_count = 0;
	 }
	 else if (inst0->header.opcode == TGSI_OPCODE_CONT) {
	    inst0->bits3.if_else.jump_count = br * (inst1 - inst0);
	    inst0->bits3.if_else.pop_count = 0;
	 }
      }
   }
   break;
   case TGSI_OPCODE_BRA:
      brw_set_predicate_control(p, get_predicate(inst));
      brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);
      break;
   case TGSI_OPCODE_CAL:
      brw_set_access_mode(p, BRW_ALIGN_1);
      brw_ADD(p, deref_1d(c->stack_index, 0), brw_ip_reg(), brw_imm_d(3*16));
      brw_set_access_mode(p, BRW_ALIGN_16);
      brw_ADD(p, get_addr_reg(c->stack_index),
	      get_addr_reg(c->stack_index), brw_imm_d(4));
      brw_save_call(p, label, p->nr_insn);
      brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
      break;
   case TGSI_OPCODE_RET:
      brw_ADD(p, get_addr_reg(c->stack_index),
	      get_addr_reg(c->stack_index), brw_imm_d(-4));
      brw_set_access_mode(p, BRW_ALIGN_1);
      brw_MOV(p, brw_ip_reg(), deref_1d(c->stack_index, 0));
      brw_set_access_mode(p, BRW_ALIGN_16);
      break;
   case TGSI_OPCODE_END:	
      c->end_offset = p->nr_insn;
      /* this instruction will get patched later to jump past subroutine
       * code, etc.
       */
      brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
      break;
   case TGSI_OPCODE_BGNSUB:
      brw_save_label(p, p->nr_insn, p->nr_insn);
      break;
   case TGSI_OPCODE_ENDSUB:
      /* no-op */
      break;
   default:
      debug_printf("Unsupported opcode %i (%s) in vertex shader",
		   opcode, 
		   tgsi_get_opcode_name(opcode));
   }

   /* Set the predication update on the last instruction of the native
    * instruction sequence.
    *
    * This would be problematic if it was set on a math instruction,
    * but that shouldn't be the case with the current GLSL compiler.
    */
#if 0
   /* XXX: disabled
    */
   if (inst->CondUpdate) {
      struct brw_instruction *hw_insn = &p->store[p->nr_insn - 1];

      assert(hw_insn->header.destreg__conditionalmod == 0);
      hw_insn->header.destreg__conditionalmod = BRW_CONDITIONAL_NZ;
   }
#endif

   release_tmps(c);
}


/* Emit the vertex program instructions here.
 */
void brw_vs_emit(struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   const struct tgsi_token *tokens = c->vp->tokens;
   struct brw_instruction *end_inst, *last_inst;
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction *inst;

   if (BRW_DEBUG & DEBUG_VS)
      tgsi_dump(c->vp->tokens, 0); 

   c->stack_index = brw_indirect(0, 0);

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_access_mode(p, BRW_ALIGN_16);
   

   /* Static register allocation
    */
   brw_vs_alloc_regs(c);

   if (c->vp->has_flow_control) {
      brw_MOV(p, get_addr_reg(c->stack_index), brw_address(c->stack));
   }

   /* Instructions
    */
   tgsi_parse_init( &parse, tokens );
   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
      case TGSI_TOKEN_TYPE_IMMEDIATE:
	 break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         inst = &parse.FullToken.FullInstruction;
	 emit_insn( c, inst );
         break;

      default:
         assert( 0 );
      }
   }
   tgsi_parse_free( &parse );

   end_inst = &p->store[c->end_offset];
   last_inst = &p->store[p->nr_insn];

   /* The END instruction will be patched to jump to this code */
   emit_vertex_write(c);

   post_vs_emit(c, end_inst, last_inst);

   if (BRW_DEBUG & DEBUG_VS) {
      debug_printf("vs-native:\n");
      brw_disasm(stderr, p->store, p->nr_insn);
   }
}
