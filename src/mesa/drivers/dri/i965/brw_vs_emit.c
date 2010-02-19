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
            

#include "main/macros.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "brw_context.h"
#include "brw_vs.h"


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


/**
 * Preallocate GRF register before code emit.
 * Do things as simply as possible.  Allocate and populate all regs
 * ahead of time.
 */
static void brw_vs_alloc_regs( struct brw_vs_compile *c )
{
   struct intel_context *intel = &c->func.brw->intel;
   GLuint i, reg = 0, mrf;
   int attributes_in_vue;

   /* Determine whether to use a real constant buffer or use a block
    * of GRF registers for constants.  The later is faster but only
    * works if everything fits in the GRF.
    * XXX this heuristic/check may need some fine tuning...
    */
   if (c->vp->program.Base.Parameters->NumParameters +
       c->vp->program.Base.NumTemporaries + 20 > BRW_MAX_GRF)
      c->vp->use_const_buffer = GL_TRUE;
   else
      c->vp->use_const_buffer = GL_FALSE;

   /*printf("use_const_buffer = %d\n", c->vp->use_const_buffer);*/

   /* r0 -- reserved as usual
    */
   c->r0 = brw_vec8_grf(reg, 0);
   reg++;

   /* User clip planes from curbe: 
    */
   if (c->key.nr_userclip) {
      for (i = 0; i < c->key.nr_userclip; i++) {
	 c->userplane[i] = stride( brw_vec4_grf(reg+3+i/2, (i%2) * 4), 0, 4, 1);
      }     

      /* Deal with curbe alignment:
       */
      reg += ((6 + c->key.nr_userclip + 3) / 4) * 2;
   }

   /* Vertex program parameters from curbe:
    */
   if (c->vp->use_const_buffer) {
      int max_constant = BRW_MAX_GRF - 20 - c->vp->program.Base.NumTemporaries;
      int constant = 0;

      /* We've got more constants than we can load with the push
       * mechanism.  This is often correlated with reladdr loads where
       * we should probably be using a pull mechanism anyway to avoid
       * excessive reading.  However, the pull mechanism is slow in
       * general.  So, we try to allocate as many non-reladdr-loaded
       * constants through the push buffer as we can before giving up.
       */
      memset(c->constant_map, -1, c->vp->program.Base.Parameters->NumParameters);
      for (i = 0;
	   i < c->vp->program.Base.NumInstructions && constant < max_constant;
	   i++) {
	 struct prog_instruction *inst = &c->vp->program.Base.Instructions[i];
	 int arg;

	 for (arg = 0; arg < 3 && constant < max_constant; arg++) {
	    if ((inst->SrcReg[arg].File != PROGRAM_STATE_VAR &&
		 inst->SrcReg[arg].File != PROGRAM_CONSTANT &&
		 inst->SrcReg[arg].File != PROGRAM_UNIFORM &&
		 inst->SrcReg[arg].File != PROGRAM_ENV_PARAM &&
		 inst->SrcReg[arg].File != PROGRAM_LOCAL_PARAM) ||
		inst->SrcReg[arg].RelAddr)
	       continue;

	    if (c->constant_map[inst->SrcReg[arg].Index] == -1) {
	       c->constant_map[inst->SrcReg[arg].Index] = constant++;
	    }
	 }
      }

      for (i = 0; i < constant; i++) {
         c->regs[PROGRAM_STATE_VAR][i] = stride( brw_vec4_grf(reg+i/2,
							      (i%2) * 4),
						 0, 4, 1);
      }
      reg += (constant + 1) / 2;
      c->prog_data.curb_read_length = reg - 1;
      /* XXX 0 causes a bug elsewhere... */
      c->prog_data.nr_params = MAX2(constant * 4, 4);
   }
   else {
      /* use a section of the GRF for constants */
      GLuint nr_params = c->vp->program.Base.Parameters->NumParameters;
      for (i = 0; i < nr_params; i++) {
         c->regs[PROGRAM_STATE_VAR][i] = stride( brw_vec4_grf(reg+i/2, (i%2) * 4), 0, 4, 1);
      }
      reg += (nr_params + 1) / 2;
      c->prog_data.curb_read_length = reg - 1;

      c->prog_data.nr_params = nr_params * 4;
   }

   /* Allocate input regs:  
    */
   c->nr_inputs = 0;
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      if (c->prog_data.inputs_read & (1 << i)) {
	 c->nr_inputs++;
	 c->regs[PROGRAM_INPUT][i] = brw_vec8_grf(reg, 0);
	 reg++;
      }
   }
   /* If there are no inputs, we'll still be reading one attribute's worth
    * because it's required -- see urb_read_length setting.
    */
   if (c->nr_inputs == 0)
      reg++;

   /* Allocate outputs.  The non-position outputs go straight into message regs.
    */
   c->nr_outputs = 0;
   c->first_output = reg;
   c->first_overflow_output = 0;

   if (intel->is_ironlake)
       mrf = 8;
   else
       mrf = 4;

   for (i = 0; i < VERT_RESULT_MAX; i++) {
      if (c->prog_data.outputs_written & BITFIELD64_BIT(i)) {
	 c->nr_outputs++;
         assert(i < Elements(c->regs[PROGRAM_OUTPUT]));
	 if (i == VERT_RESULT_HPOS) {
	    c->regs[PROGRAM_OUTPUT][i] = brw_vec8_grf(reg, 0);
	    reg++;
	 }
	 else if (i == VERT_RESULT_PSIZ) {
	    c->regs[PROGRAM_OUTPUT][i] = brw_vec8_grf(reg, 0);
	    reg++;
	    mrf++;		/* just a placeholder?  XXX fix later stages & remove this */
	 }
	 else {
            if (mrf < 16) {
               c->regs[PROGRAM_OUTPUT][i] = brw_message_reg(mrf);
               mrf++;
            }
            else {
               /* too many vertex results to fit in MRF, use GRF for overflow */
               if (!c->first_overflow_output)
                  c->first_overflow_output = i;
               c->regs[PROGRAM_OUTPUT][i] = brw_vec8_grf(reg, 0);
               reg++;
            }
	 }
      }
   }     

   /* Allocate program temporaries:
    */
   for (i = 0; i < c->vp->program.Base.NumTemporaries; i++) {
      c->regs[PROGRAM_TEMPORARY][i] = brw_vec8_grf(reg, 0);
      reg++;
   }

   /* Address reg(s).  Don't try to use the internal address reg until
    * deref time.
    */
   for (i = 0; i < c->vp->program.Base.NumAddressRegs; i++) {
      c->regs[PROGRAM_ADDRESS][i] =  brw_reg(BRW_GENERAL_REGISTER_FILE,
					     reg,
					     0,
					     BRW_REGISTER_TYPE_D,
					     BRW_VERTICAL_STRIDE_8,
					     BRW_WIDTH_8,
					     BRW_HORIZONTAL_STRIDE_1,
					     BRW_SWIZZLE_XXXX,
					     WRITEMASK_X);
      reg++;
   }

   if (c->vp->use_const_buffer) {
      for (i = 0; i < 3; i++) {
         c->current_const[i].index = -1;
         c->current_const[i].reg = brw_vec8_grf(reg, 0);
         reg++;
      }
   }

   for (i = 0; i < 128; i++) {
      if (c->output_regs[i].used_in_src) {
         c->output_regs[i].reg = brw_vec8_grf(reg, 0);
         reg++;
      }
   }

   if (c->needs_stack) {
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

   if (intel->is_ironlake)
       c->prog_data.urb_entry_size = (attributes_in_vue + 6 + 3) / 4;
   else
       c->prog_data.urb_entry_size = (attributes_in_vue + 2 + 3) / 4;

   c->prog_data.total_grf = reg;

   if (INTEL_DEBUG & DEBUG_VS) {
      printf("%s NumAddrRegs %d\n", __FUNCTION__, c->vp->program.Base.NumAddressRegs);
      printf("%s NumTemps %d\n", __FUNCTION__, c->vp->program.Base.NumTemporaries);
      printf("%s reg = %d\n", __FUNCTION__, reg);
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

static void emit_sop( struct brw_vs_compile *c,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1, 
		      GLuint cond)
{
   struct brw_compile *p = &c->func;

   brw_MOV(p, dst, brw_imm_f(0.0f));
   brw_CMP(p, brw_null_reg(), cond, arg0, arg1);
   brw_MOV(p, dst, brw_imm_f(1.0f));
   brw_set_predicate_control_flag_value(p, 0xff);
}

static void emit_seq( struct brw_vs_compile *c,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_EQ);
}

static void emit_sne( struct brw_vs_compile *c,
                      struct brw_reg dst,
                      struct brw_reg arg0,
                      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_NEQ);
}
static void emit_slt( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_L);
}

static void emit_sle( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_LE);
}

static void emit_sgt( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
   emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_G);
}

static void emit_sge( struct brw_vs_compile *c,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1 )
{
  emit_sop(c, dst, arg0, arg1, BRW_CONDITIONAL_GE);
}

static void emit_cmp( struct brw_compile *p,
		      struct brw_reg dst,
		      struct brw_reg arg0,
		      struct brw_reg arg1,
		      struct brw_reg arg2 )
{
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, arg0, brw_imm_f(0));
   brw_SEL(p, dst, arg1, arg2);
   brw_set_predicate_control(p, BRW_PREDICATE_NONE);
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
   

   if (dst.dw1.bits.writemask & WRITEMASK_X) {
      struct brw_reg tmp = get_tmp(c);
      struct brw_reg tmp_d = retype(tmp, BRW_REGISTER_TYPE_D);

      /* tmp_d = floor(arg0.x) */
      brw_RNDD(p, tmp_d, brw_swizzle1(arg0, 0));

      /* result[0] = 2.0 ^ tmp */

      /* Adjust exponent for floating point: 
       * exp += 127 
       */
      brw_ADD(p, brw_writemask(tmp_d, WRITEMASK_X), tmp_d, brw_imm_d(127));

      /* Install exponent and sign.  
       * Excess drops off the edge: 
       */
      brw_SHL(p, brw_writemask(retype(dst, BRW_REGISTER_TYPE_D), WRITEMASK_X), 
	      tmp_d, brw_imm_d(23));

      release_tmp(c, tmp);
   }

   if (dst.dw1.bits.writemask & WRITEMASK_Y) {
      /* result[1] = arg0.x - floor(arg0.x) */
      brw_FRC(p, brw_writemask(dst, WRITEMASK_Y), brw_swizzle1(arg0, 0));
   }
   
   if (dst.dw1.bits.writemask & WRITEMASK_Z) {
      /* As with the LOG instruction, we might be better off just
       * doing a taylor expansion here, seeing as we have to do all
       * the prep work.
       *
       * If mathbox partial precision is too low, consider also:
       * result[3] = result[0] * EXP(result[1])
       */
      emit_math1(c, 
		 BRW_MATH_FUNCTION_EXP, 
		 brw_writemask(dst, WRITEMASK_Z),
		 brw_swizzle1(arg0, 0), 
		 BRW_MATH_PRECISION_FULL);
   }  

   if (dst.dw1.bits.writemask & WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(dst, WRITEMASK_W), brw_imm_f(1));
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
   if (dst.dw1.bits.writemask & WRITEMASK_XZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, WRITEMASK_X),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1U<<31)-1));

      brw_SHR(p, 
	      brw_writemask(tmp_ud, WRITEMASK_X), 
	      tmp_ud,
	      brw_imm_ud(23));

      brw_ADD(p, 
	      brw_writemask(tmp, WRITEMASK_X), 
	      retype(tmp_ud, BRW_REGISTER_TYPE_D),	/* does it matter? */
	      brw_imm_d(-127));
   }

   if (dst.dw1.bits.writemask & WRITEMASK_YZ) {
      brw_AND(p, 
	      brw_writemask(tmp_ud, WRITEMASK_Y),
	      brw_swizzle1(arg0_ud, 0),
	      brw_imm_ud((1<<23)-1));

      brw_OR(p, 
	     brw_writemask(tmp_ud, WRITEMASK_Y), 
	     tmp_ud,
	     brw_imm_ud(127<<23));
   }
   
   if (dst.dw1.bits.writemask & WRITEMASK_Z) {
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
		 brw_writemask(tmp, WRITEMASK_Z), 
		 brw_swizzle1(tmp, 1), 
		 BRW_MATH_PRECISION_FULL);
      
      brw_ADD(p, 
	      brw_writemask(tmp, WRITEMASK_Z), 
	      brw_swizzle1(tmp, 2), 
	      brw_swizzle1(tmp, 0));
   }  

   if (dst.dw1.bits.writemask & WRITEMASK_W) {
      /* result[3] = 1.0; */
      brw_MOV(p, brw_writemask(tmp, WRITEMASK_W), brw_imm_f(1));
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
   if (dst.dw1.bits.writemask & WRITEMASK_X)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_X), brw_imm_f(1.0));
   if (dst.dw1.bits.writemask & WRITEMASK_Y)
      brw_MUL(p, brw_writemask(dst, WRITEMASK_Y), arg0, arg1);
   if (dst.dw1.bits.writemask & WRITEMASK_Z)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_Z), arg0);
   if (dst.dw1.bits.writemask & WRITEMASK_W)
      brw_MOV(p, brw_writemask(dst, WRITEMASK_W), arg1);
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
   
   brw_MOV(p, brw_writemask(dst, WRITEMASK_YZ), brw_imm_f(0)); 
   brw_MOV(p, brw_writemask(dst, WRITEMASK_XW), brw_imm_f(1)); 

   /* Need to use BRW_EXECUTE_8 and also do an 8-wide compare in order
    * to get all channels active inside the IF.  In the clipping code
    * we run with NoMask, so it's not an option and we can use
    * BRW_EXECUTE_1 for all comparisions.
    */
   brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,0), brw_imm_f(0));
   if_insn = brw_IF(p, BRW_EXECUTE_8);
   {
      brw_MOV(p, brw_writemask(dst, WRITEMASK_Y), brw_swizzle1(arg0,0));

      brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, brw_swizzle1(arg0,1), brw_imm_f(0));
      brw_MOV(p, brw_writemask(tmp, WRITEMASK_Z),  brw_swizzle1(arg0,1));
      brw_set_predicate_control(p, BRW_PREDICATE_NONE);

      emit_math2(c, 
		 BRW_MATH_FUNCTION_POW, 
		 brw_writemask(dst, WRITEMASK_Z),
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
             const struct prog_instruction *inst,
             GLuint argIndex)
{
   const struct prog_src_register *src = &inst->SrcReg[argIndex];
   struct brw_compile *p = &c->func;
   struct brw_reg const_reg = c->current_const[argIndex].reg;

   assert(argIndex < 3);

   if (c->current_const[argIndex].index != src->Index) {
      struct brw_reg addrReg = c->regs[PROGRAM_ADDRESS][0];

      /* Keep track of the last constant loaded in this slot, for reuse. */
      c->current_const[argIndex].index = src->Index;

#if 0
      printf("  fetch const[%d] for arg %d into reg %d\n",
             src->Index, argIndex, c->current_const[argIndex].reg.nr);
#endif
      /* need to fetch the constant now */
      brw_dp_READ_4_vs(p,
                       const_reg,                     /* writeback dest */
                       0,                             /* oword */
                       0,                             /* relative indexing? */
                       addrReg,                       /* address register */
                       16 * src->Index,               /* byte offset */
                       SURF_INDEX_VERT_CONST_BUFFER   /* binding table index */
                       );
   }

   /* replicate lower four floats into upper half (to get XYZWXYZW) */
   const_reg = stride(const_reg, 0, 4, 0);
   const_reg.subnr = 0;

   return const_reg;
}

static struct brw_reg
get_reladdr_constant(struct brw_vs_compile *c,
		     const struct prog_instruction *inst,
		     GLuint argIndex)
{
   const struct prog_src_register *src = &inst->SrcReg[argIndex];
   struct brw_compile *p = &c->func;
   struct brw_reg const_reg = c->current_const[argIndex].reg;
   struct brw_reg const2_reg;
   struct brw_reg addrReg = c->regs[PROGRAM_ADDRESS][0];

   assert(argIndex < 3);

   /* Can't reuse a reladdr constant load. */
   c->current_const[argIndex].index = -1;

 #if 0
   printf("  fetch const[a0.x+%d] for arg %d into reg %d\n",
	  src->Index, argIndex, c->current_const[argIndex].reg.nr);
#endif

   /* fetch the first vec4 */
   brw_dp_READ_4_vs(p,
		    const_reg,                     /* writeback dest */
		    0,                             /* oword */
		    1,                             /* relative indexing? */
		    addrReg,                       /* address register */
		    16 * src->Index,               /* byte offset */
		    SURF_INDEX_VERT_CONST_BUFFER   /* binding table index */
		    );
   /* second vec4 */
   const2_reg = get_tmp(c);

   /* use upper half of address reg for second read */
   addrReg = stride(addrReg, 0, 4, 0);
   addrReg.subnr = 16;

   brw_dp_READ_4_vs(p,
		    const2_reg,              /* writeback dest */
		    1,                       /* oword */
		    1,                       /* relative indexing? */
		    addrReg,                 /* address register */
		    16 * src->Index,         /* byte offset */
		    SURF_INDEX_VERT_CONST_BUFFER
		    );

   /* merge the two Owords into the constant register */
   /* const_reg[7..4] = const2_reg[7..4] */
   brw_MOV(p,
	   suboffset(stride(const_reg, 0, 4, 1), 4),
	   suboffset(stride(const2_reg, 0, 4, 1), 4));
   release_tmp(c, const2_reg);

   return const_reg;
}



/* TODO: relative addressing!
 */
static struct brw_reg get_reg( struct brw_vs_compile *c,
			       gl_register_file file,
			       GLuint index )
{
   switch (file) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_INPUT:
   case PROGRAM_OUTPUT:
      assert(c->regs[file][index].nr != 0);
      return c->regs[file][index];
   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:
   case PROGRAM_UNIFORM:
      assert(c->regs[PROGRAM_STATE_VAR][index].nr != 0);
      return c->regs[PROGRAM_STATE_VAR][index];
   case PROGRAM_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case PROGRAM_UNDEFINED:			/* undef values */
      return brw_null_reg();

   case PROGRAM_LOCAL_PARAM: 
   case PROGRAM_ENV_PARAM: 
   case PROGRAM_WRITE_ONLY:
   default:
      assert(0);
      return brw_null_reg();
   }
}


/**
 * Indirect addressing:  get reg[[arg] + offset].
 */
static struct brw_reg deref( struct brw_vs_compile *c,
			     struct brw_reg arg,
			     GLint offset)
{
   struct brw_compile *p = &c->func;
   struct brw_reg tmp = vec4(get_tmp(c));
   struct brw_reg addr_reg = c->regs[PROGRAM_ADDRESS][0];
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
             const struct prog_instruction *inst,
             GLuint argIndex )
{
   const GLuint file = inst->SrcReg[argIndex].File;
   const GLint index = inst->SrcReg[argIndex].Index;
   const GLboolean relAddr = inst->SrcReg[argIndex].RelAddr;

   switch (file) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_INPUT:
   case PROGRAM_OUTPUT:
      if (relAddr) {
         return deref(c, c->regs[file][0], index);
      }
      else {
         assert(c->regs[file][index].nr != 0);
         return c->regs[file][index];
      }

   case PROGRAM_STATE_VAR:
   case PROGRAM_CONSTANT:
   case PROGRAM_UNIFORM:
   case PROGRAM_ENV_PARAM:
   case PROGRAM_LOCAL_PARAM:
      if (c->vp->use_const_buffer) {
	 if (!relAddr && c->constant_map[index] != -1) {
	    assert(c->regs[PROGRAM_STATE_VAR][c->constant_map[index]].nr != 0);
	    return c->regs[PROGRAM_STATE_VAR][c->constant_map[index]];
	 } else if (relAddr)
	    return get_reladdr_constant(c, inst, argIndex);
	 else
	    return get_constant(c, inst, argIndex);
      }
      else if (relAddr) {
         return deref(c, c->regs[PROGRAM_STATE_VAR][0], index);
      }
      else {
         assert(c->regs[PROGRAM_STATE_VAR][index].nr != 0);
         return c->regs[PROGRAM_STATE_VAR][index];
      }
   case PROGRAM_ADDRESS:
      assert(index == 0);
      return c->regs[file][index];

   case PROGRAM_UNDEFINED:
      /* this is a normal case since we loop over all three src args */
      return brw_null_reg();

   case PROGRAM_WRITE_ONLY:
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
 * Will return mangled results for SWZ op.  The emit_swz() function
 * ignores this result and recalculates taking extended swizzles into
 * account.
 */
static struct brw_reg get_arg( struct brw_vs_compile *c,
                               const struct prog_instruction *inst,
                               GLuint argIndex )
{
   const struct prog_src_register *src = &inst->SrcReg[argIndex];
   struct brw_reg reg;

   if (src->File == PROGRAM_UNDEFINED)
      return brw_null_reg();

   reg = get_src_reg(c, inst, argIndex);

   /* Convert 3-bit swizzle to 2-bit.  
    */
   reg.dw1.bits.swizzle = BRW_SWIZZLE4(GET_SWZ(src->Swizzle, 0),
				       GET_SWZ(src->Swizzle, 1),
				       GET_SWZ(src->Swizzle, 2),
				       GET_SWZ(src->Swizzle, 3));

   /* Note this is ok for non-swizzle instructions: 
    */
   reg.negate = src->Negate ? 1 : 0;   

   return reg;
}


/**
 * Get brw register for the given program dest register.
 */
static struct brw_reg get_dst( struct brw_vs_compile *c,
			       struct prog_dst_register dst )
{
   struct brw_reg reg;

   switch (dst.File) {
   case PROGRAM_TEMPORARY:
   case PROGRAM_OUTPUT:
      assert(c->regs[dst.File][dst.Index].nr != 0);
      reg = c->regs[dst.File][dst.Index];
      break;
   case PROGRAM_ADDRESS:
      assert(dst.Index == 0);
      reg = c->regs[dst.File][dst.Index];
      break;
   case PROGRAM_UNDEFINED:
      /* we may hit this for OPCODE_END, OPCODE_KIL, etc */
      reg = brw_null_reg();
      break;
   default:
      assert(0);
      reg = brw_null_reg();
   }

   reg.dw1.bits.writemask = dst.WriteMask;

   return reg;
}


static void emit_swz( struct brw_vs_compile *c, 
		      struct brw_reg dst,
                      const struct prog_instruction *inst)
{
   const GLuint argIndex = 0;
   const struct prog_src_register src = inst->SrcReg[argIndex];
   struct brw_compile *p = &c->func;
   GLuint zeros_mask = 0;
   GLuint ones_mask = 0;
   GLuint src_mask = 0;
   GLubyte src_swz[4];
   GLboolean need_tmp = (src.Negate &&
			 dst.file != BRW_GENERAL_REGISTER_FILE);
   struct brw_reg tmp = dst;
   GLuint i;

   if (need_tmp)
      tmp = get_tmp(c);

   for (i = 0; i < 4; i++) {
      if (dst.dw1.bits.writemask & (1<<i)) {
	 GLubyte s = GET_SWZ(src.Swizzle, i);
	 switch (s) {
	 case SWIZZLE_X:
	 case SWIZZLE_Y:
	 case SWIZZLE_Z:
	 case SWIZZLE_W:
	    src_mask |= 1<<i;
	    src_swz[i] = s;
	    break;
	 case SWIZZLE_ZERO:
	    zeros_mask |= 1<<i;
	    break;
	 case SWIZZLE_ONE:
	    ones_mask |= 1<<i;
	    break;
	 }
      }
   }
   
   /* Do src first, in case dst aliases src:
    */
   if (src_mask) {
      struct brw_reg arg0;

      arg0 = get_src_reg(c, inst, argIndex);

      arg0 = brw_swizzle(arg0, 
			 src_swz[0], src_swz[1], 
			 src_swz[2], src_swz[3]);

      brw_MOV(p, brw_writemask(tmp, src_mask), arg0);
   } 
   
   if (zeros_mask) 
      brw_MOV(p, brw_writemask(tmp, zeros_mask), brw_imm_f(0));

   if (ones_mask) 
      brw_MOV(p, brw_writemask(tmp, ones_mask), brw_imm_f(1));

   if (src.Negate)
      brw_MOV(p, brw_writemask(tmp, src.Negate), negate(tmp));
   
   if (need_tmp) {
      brw_MOV(p, dst, tmp);
      release_tmp(c, tmp);
   }
}


/**
 * Post-vertex-program processing.  Send the results to the URB.
 */
static void emit_vertex_write( struct brw_vs_compile *c)
{
   struct brw_compile *p = &c->func;
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   struct brw_reg m0 = brw_message_reg(0);
   struct brw_reg pos = c->regs[PROGRAM_OUTPUT][VERT_RESULT_HPOS];
   struct brw_reg ndc;
   int eot;
   GLuint len_vertext_header = 2;

   if (c->key.copy_edgeflag) {
      brw_MOV(p, 
	      get_reg(c, PROGRAM_OUTPUT, VERT_RESULT_EDGE),
	      get_reg(c, PROGRAM_INPUT, VERT_ATTRIB_EDGEFLAG));
   }

   /* Build ndc coords */
   ndc = get_tmp(c);
   /* ndc = 1.0 / pos.w */
   emit_math1(c, BRW_MATH_FUNCTION_INV, ndc, brw_swizzle1(pos, 3), BRW_MATH_PRECISION_FULL);
   /* ndc.xyz = pos * ndc */
   brw_MUL(p, brw_writemask(ndc, WRITEMASK_XYZ), pos, ndc);

   /* Update the header for point size, user clipping flags, and -ve rhw
    * workaround.
    */
   if ((c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) ||
       c->key.nr_userclip || brw->has_negative_rhw_bug)
   {
      struct brw_reg header1 = retype(get_tmp(c), BRW_REGISTER_TYPE_UD);
      GLuint i;

      brw_MOV(p, header1, brw_imm_ud(0));

      brw_set_access_mode(p, BRW_ALIGN_16);	

      if (c->prog_data.outputs_written & BITFIELD64_BIT(VERT_RESULT_PSIZ)) {
	 struct brw_reg psiz = c->regs[PROGRAM_OUTPUT][VERT_RESULT_PSIZ];
	 brw_MUL(p, brw_writemask(header1, WRITEMASK_W), brw_swizzle1(psiz, 0), brw_imm_f(1<<11));
	 brw_AND(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(0x7ff<<8));
      }

      for (i = 0; i < c->key.nr_userclip; i++) {
	 brw_set_conditionalmod(p, BRW_CONDITIONAL_L);
	 brw_DP4(p, brw_null_reg(), pos, c->userplane[i]);
	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<i));
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
      if (brw->has_negative_rhw_bug) {
	 brw_CMP(p,
		 vec8(brw_null_reg()),
		 BRW_CONDITIONAL_L,
		 brw_swizzle1(ndc, 3),
		 brw_imm_f(0));
   
	 brw_OR(p, brw_writemask(header1, WRITEMASK_W), header1, brw_imm_ud(1<<6));
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

   if (intel->is_ironlake) {
       /* There are 20 DWs (D0-D19) in VUE vertex header on Ironlake */
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

   eot = (c->first_overflow_output == 0);

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

   if (c->first_overflow_output > 0) {
      /* Not all of the vertex outputs/results fit into the MRF.
       * Move the overflowed attributes from the GRF to the MRF and
       * issue another brw_urb_WRITE().
       */
      /* XXX I'm not 100% sure about which MRF regs to use here.  Starting
       * at mrf[4] atm...
       */
      GLuint i, mrf = 0;
      for (i = c->first_overflow_output; i < VERT_RESULT_MAX; i++) {
         if (c->prog_data.outputs_written & BITFIELD64_BIT(i)) {
            /* move from GRF to MRF */
            brw_MOV(p, brw_message_reg(4+mrf), c->regs[PROGRAM_OUTPUT][i]);
            mrf++;
         }
      }

      brw_urb_WRITE(p,
                    brw_null_reg(), /* dest */
                    4,              /* starting mrf reg nr */
                    c->r0,          /* src */
                    0,              /* allocate */
                    1,              /* used */
                    mrf+1,          /* msg len */
                    0,              /* response len */
                    1,              /* eot */
                    1,              /* writes complete */
                    BRW_MAX_MRF-1,  /* urb destination offset */
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

static GLboolean
accumulator_contains(struct brw_vs_compile *c, struct brw_reg val)
{
   struct brw_compile *p = &c->func;
   struct brw_instruction *prev_insn = &p->store[p->nr_insn - 1];

   if (p->nr_insn == 0)
      return GL_FALSE;

   if (val.address_mode != BRW_ADDRESS_DIRECT)
      return GL_FALSE;

   switch (prev_insn->header.opcode) {
   case BRW_OPCODE_MOV:
   case BRW_OPCODE_MAC:
   case BRW_OPCODE_MUL:
      if (prev_insn->header.access_mode == BRW_ALIGN_16 &&
	  prev_insn->header.execution_size == val.width &&
	  prev_insn->bits1.da1.dest_reg_file == val.file &&
	  prev_insn->bits1.da1.dest_reg_type == val.type &&
	  prev_insn->bits1.da1.dest_address_mode == val.address_mode &&
	  prev_insn->bits1.da1.dest_reg_nr == val.nr &&
	  prev_insn->bits1.da16.dest_subreg_nr == val.subnr / 16 &&
	  prev_insn->bits1.da16.dest_writemask == 0xf)
	 return GL_TRUE;
      else
	 return GL_FALSE;
   default:
      return GL_FALSE;
   }
}

static uint32_t
get_predicate(const struct prog_instruction *inst)
{
   if (inst->DstReg.CondMask == COND_TR)
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
   assert(inst->DstReg.CondMask == COND_NE);

   switch (inst->DstReg.CondSwizzle) {
   case SWIZZLE_XXXX:
      return BRW_PREDICATE_ALIGN16_REPLICATE_X;
   case SWIZZLE_YYYY:
      return BRW_PREDICATE_ALIGN16_REPLICATE_Y;
   case SWIZZLE_ZZZZ:
      return BRW_PREDICATE_ALIGN16_REPLICATE_Z;
   case SWIZZLE_WWWW:
      return BRW_PREDICATE_ALIGN16_REPLICATE_W;
   default:
      _mesa_problem(NULL, "Unexpected predicate: 0x%08x\n",
		    inst->DstReg.CondMask);
      return BRW_PREDICATE_NORMAL;
   }
}

/* Emit the vertex program instructions here.
 */
void brw_vs_emit(struct brw_vs_compile *c )
{
#define MAX_IF_DEPTH 32
#define MAX_LOOP_DEPTH 32
   struct brw_compile *p = &c->func;
   struct brw_context *brw = p->brw;
   struct intel_context *intel = &brw->intel;
   const GLuint nr_insns = c->vp->program.Base.NumInstructions;
   GLuint insn, if_depth = 0, loop_depth = 0;
   GLuint end_offset = 0;
   struct brw_instruction *end_inst, *last_inst;
   struct brw_instruction *if_inst[MAX_IF_DEPTH], *loop_inst[MAX_LOOP_DEPTH] = { 0 };
   const struct brw_indirect stack_index = brw_indirect(0, 0);   
   GLuint index;
   GLuint file;

   if (INTEL_DEBUG & DEBUG_VS) {
      printf("vs-mesa:\n");
      _mesa_print_program(&c->vp->program.Base); 
      printf("\n");
   }

   brw_set_compression_control(p, BRW_COMPRESSION_NONE);
   brw_set_access_mode(p, BRW_ALIGN_16);

   for (insn = 0; insn < nr_insns; insn++) {
       GLuint i;
       struct prog_instruction *inst = &c->vp->program.Base.Instructions[insn];

       /* Message registers can't be read, so copy the output into GRF
	* register if they are used in source registers
	*/
       for (i = 0; i < 3; i++) {
	   struct prog_src_register *src = &inst->SrcReg[i];
	   GLuint index = src->Index;
	   GLuint file = src->File;	
	   if (file == PROGRAM_OUTPUT && index != VERT_RESULT_HPOS)
	       c->output_regs[index].used_in_src = GL_TRUE;
       }

       switch (inst->Opcode) {
       case OPCODE_CAL:
       case OPCODE_RET:
	  c->needs_stack = GL_TRUE;
	  break;
       default:
	  break;
       }
   }

   /* Static register allocation
    */
   brw_vs_alloc_regs(c);

   if (c->needs_stack)
      brw_MOV(p, get_addr_reg(stack_index), brw_address(c->stack));

   for (insn = 0; insn < nr_insns; insn++) {

      const struct prog_instruction *inst = &c->vp->program.Base.Instructions[insn];
      struct brw_reg args[3], dst;
      GLuint i;
      
#if 0
      printf("%d: ", insn);
      _mesa_print_instruction(inst);
#endif

      /* Get argument regs.  SWZ is special and does this itself.
       */
      if (inst->Opcode != OPCODE_SWZ)
	  for (i = 0; i < 3; i++) {
	      const struct prog_src_register *src = &inst->SrcReg[i];
	      index = src->Index;
	      file = src->File;	
	      if (file == PROGRAM_OUTPUT && c->output_regs[index].used_in_src)
		  args[i] = c->output_regs[index].reg;
	      else
                  args[i] = get_arg(c, inst, i);
	  }

      /* Get dest regs.  Note that it is possible for a reg to be both
       * dst and arg, given the static allocation of registers.  So
       * care needs to be taken emitting multi-operation instructions.
       */ 
      index = inst->DstReg.Index;
      file = inst->DstReg.File;
      if (file == PROGRAM_OUTPUT && c->output_regs[index].used_in_src)
	  dst = c->output_regs[index].reg;
      else
	  dst = get_dst(c, inst->DstReg);

      if (inst->SaturateMode != SATURATE_OFF) {
	 _mesa_problem(NULL, "Unsupported saturate %d in vertex shader",
                       inst->SaturateMode);
      }

      switch (inst->Opcode) {
      case OPCODE_ABS:
	 brw_MOV(p, dst, brw_abs(args[0]));
	 break;
      case OPCODE_ADD:
	 brw_ADD(p, dst, args[0], args[1]);
	 break;
      case OPCODE_COS:
	 emit_math1(c, BRW_MATH_FUNCTION_COS, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_DP3:
	 brw_DP3(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DP4:
	 brw_DP4(p, dst, args[0], args[1]);
	 break;
      case OPCODE_DPH:
	 brw_DPH(p, dst, args[0], args[1]);
	 break;
      case OPCODE_NRM3:
	 emit_nrm(c, dst, args[0], 3);
	 break;
      case OPCODE_NRM4:
	 emit_nrm(c, dst, args[0], 4);
	 break;
      case OPCODE_DST:
	 unalias2(c, dst, args[0], args[1], emit_dst_noalias); 
	 break;
      case OPCODE_EXP:
	 unalias1(c, dst, args[0], emit_exp_noalias);
	 break;
      case OPCODE_EX2:
	 emit_math1(c, BRW_MATH_FUNCTION_EXP, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_ARL:
	 emit_arl(c, dst, args[0]);
	 break;
      case OPCODE_FLR:
	 brw_RNDD(p, dst, args[0]);
	 break;
      case OPCODE_FRC:
	 brw_FRC(p, dst, args[0]);
	 break;
      case OPCODE_LOG:
	 unalias1(c, dst, args[0], emit_log_noalias);
	 break;
      case OPCODE_LG2:
	 emit_math1(c, BRW_MATH_FUNCTION_LOG, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_LIT:
	 unalias1(c, dst, args[0], emit_lit_noalias);
	 break;
      case OPCODE_LRP:
	 unalias3(c, dst, args[0], args[1], args[2], emit_lrp_noalias);
	 break;
      case OPCODE_MAD:
	 if (!accumulator_contains(c, args[2]))
	    brw_MOV(p, brw_acc_reg(), args[2]);
	 brw_MAC(p, dst, args[0], args[1]);
	 break;
      case OPCODE_CMP:
	 emit_cmp(p, dst, args[0], args[1], args[2]);
	 break;
      case OPCODE_MAX:
	 emit_max(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MIN:
	 emit_min(p, dst, args[0], args[1]);
	 break;
      case OPCODE_MOV:
	 brw_MOV(p, dst, args[0]);
	 break;
      case OPCODE_MUL:
	 brw_MUL(p, dst, args[0], args[1]);
	 break;
      case OPCODE_POW:
	 emit_math2(c, BRW_MATH_FUNCTION_POW, dst, args[0], args[1], BRW_MATH_PRECISION_FULL); 
	 break;
      case OPCODE_RCP:
	 emit_math1(c, BRW_MATH_FUNCTION_INV, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_RSQ:
	 emit_math1(c, BRW_MATH_FUNCTION_RSQ, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;

      case OPCODE_SEQ:
         unalias2(c, dst, args[0], args[1], emit_seq);
         break;
      case OPCODE_SIN:
	 emit_math1(c, BRW_MATH_FUNCTION_SIN, dst, args[0], BRW_MATH_PRECISION_FULL);
	 break;
      case OPCODE_SNE:
         unalias2(c, dst, args[0], args[1], emit_sne);
         break;
      case OPCODE_SGE:
         unalias2(c, dst, args[0], args[1], emit_sge);
	 break;
      case OPCODE_SGT:
         unalias2(c, dst, args[0], args[1], emit_sgt);
         break;
      case OPCODE_SLT:
         unalias2(c, dst, args[0], args[1], emit_slt);
	 break;
      case OPCODE_SLE:
         unalias2(c, dst, args[0], args[1], emit_sle);
         break;
      case OPCODE_SUB:
	 brw_ADD(p, dst, args[0], negate(args[1]));
	 break;
      case OPCODE_SWZ:
	 /* The args[0] value can't be used here as it won't have
	  * correctly encoded the full swizzle:
	  */
	 emit_swz(c, dst, inst);
	 break;
      case OPCODE_TRUNC:
         /* round toward zero */
	 brw_RNDZ(p, dst, args[0]);
	 break;
      case OPCODE_XPD:
	 emit_xpd(p, dst, args[0], args[1]);
	 break;
      case OPCODE_IF:
	 assert(if_depth < MAX_IF_DEPTH);
	 if_inst[if_depth] = brw_IF(p, BRW_EXECUTE_8);
	 /* Note that brw_IF smashes the predicate_control field. */
	 if_inst[if_depth]->header.predicate_control = get_predicate(inst);
	 if_depth++;
	 break;
      case OPCODE_ELSE:
	 if_inst[if_depth-1] = brw_ELSE(p, if_inst[if_depth-1]);
	 break;
      case OPCODE_ENDIF:
         assert(if_depth > 0);
	 brw_ENDIF(p, if_inst[--if_depth]);
	 break;			
      case OPCODE_BGNLOOP:
         loop_inst[loop_depth++] = brw_DO(p, BRW_EXECUTE_8);
         break;
      case OPCODE_BRK:
	 brw_set_predicate_control(p, get_predicate(inst));
         brw_BREAK(p);
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
         break;
      case OPCODE_CONT:
	 brw_set_predicate_control(p, get_predicate(inst));
         brw_CONT(p);
         brw_set_predicate_control(p, BRW_PREDICATE_NONE);
         break;
      case OPCODE_ENDLOOP: 
         {
            struct brw_instruction *inst0, *inst1;
	    GLuint br = 1;

            loop_depth--;

	    if (intel->is_ironlake)
	       br = 2;

            inst0 = inst1 = brw_WHILE(p, loop_inst[loop_depth]);
            /* patch all the BREAK/CONT instructions from last BEGINLOOP */
            while (inst0 > loop_inst[loop_depth]) {
               inst0--;
               if (inst0->header.opcode == BRW_OPCODE_BREAK) {
                  inst0->bits3.if_else.jump_count = br * (inst1 - inst0 + 1);
                  inst0->bits3.if_else.pop_count = 0;
               }
               else if (inst0->header.opcode == BRW_OPCODE_CONTINUE) {
                  inst0->bits3.if_else.jump_count = br * (inst1 - inst0);
                  inst0->bits3.if_else.pop_count = 0;
               }
            }
         }
         break;
      case OPCODE_BRA:
	 brw_set_predicate_control(p, get_predicate(inst));
         brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
	 brw_set_predicate_control(p, BRW_PREDICATE_NONE);
         break;
      case OPCODE_CAL:
	 brw_set_access_mode(p, BRW_ALIGN_1);
	 brw_ADD(p, deref_1d(stack_index, 0), brw_ip_reg(), brw_imm_d(3*16));
	 brw_set_access_mode(p, BRW_ALIGN_16);
	 brw_ADD(p, get_addr_reg(stack_index),
			 get_addr_reg(stack_index), brw_imm_d(4));
         brw_save_call(p, inst->Comment, p->nr_insn);
	 brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
         break;
      case OPCODE_RET:
	 brw_ADD(p, get_addr_reg(stack_index),
			 get_addr_reg(stack_index), brw_imm_d(-4));
	 brw_set_access_mode(p, BRW_ALIGN_1);
         brw_MOV(p, brw_ip_reg(), deref_1d(stack_index, 0));
	 brw_set_access_mode(p, BRW_ALIGN_16);
	 break;
      case OPCODE_END:	
         end_offset = p->nr_insn;
         /* this instruction will get patched later to jump past subroutine
          * code, etc.
          */
         brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
         break;
      case OPCODE_PRINT:
         /* no-op */
         break;
      case OPCODE_BGNSUB:
         brw_save_label(p, inst->Comment, p->nr_insn);
         break;
      case OPCODE_ENDSUB:
         /* no-op */
         break;
      default:
	 _mesa_problem(NULL, "Unsupported opcode %i (%s) in vertex shader",
                       inst->Opcode, inst->Opcode < MAX_OPCODE ?
				    _mesa_opcode_string(inst->Opcode) :
				    "unknown");
      }

      /* Set the predication update on the last instruction of the native
       * instruction sequence.
       *
       * This would be problematic if it was set on a math instruction,
       * but that shouldn't be the case with the current GLSL compiler.
       */
      if (inst->CondUpdate) {
	 struct brw_instruction *hw_insn = &p->store[p->nr_insn - 1];

	 assert(hw_insn->header.destreg__conditionalmod == 0);
	 hw_insn->header.destreg__conditionalmod = BRW_CONDITIONAL_NZ;
      }

      if ((inst->DstReg.File == PROGRAM_OUTPUT)
          && (inst->DstReg.Index != VERT_RESULT_HPOS)
          && c->output_regs[inst->DstReg.Index].used_in_src) {
         brw_MOV(p, get_dst(c, inst->DstReg), dst);
      }

      /* Result color clamping.
       *
       * When destination register is an output register and
       * it's primary/secondary front/back color, we have to clamp
       * the result to [0,1]. This is done by enabling the
       * saturation bit for the last instruction.
       *
       * We don't use brw_set_saturate() as it modifies
       * p->current->header.saturate, which affects all the subsequent
       * instructions. Instead, we directly modify the header
       * of the last (already stored) instruction.
       */
      if (inst->DstReg.File == PROGRAM_OUTPUT) {
         if ((inst->DstReg.Index == VERT_RESULT_COL0)
             || (inst->DstReg.Index == VERT_RESULT_COL1)
             || (inst->DstReg.Index == VERT_RESULT_BFC0)
             || (inst->DstReg.Index == VERT_RESULT_BFC1)) {
            p->store[p->nr_insn-1].header.saturate = 1;
         }
      }

      release_tmps(c);
   }

   end_inst = &p->store[end_offset];
   last_inst = &p->store[p->nr_insn];

   /* The END instruction will be patched to jump to this code */
   emit_vertex_write(c);

   post_vs_emit(c, end_inst, last_inst);

   if (INTEL_DEBUG & DEBUG_VS) {
      int i;

      printf("vs-native:\n");
      for (i = 0; i < p->nr_insn; i++)
	 brw_disasm(stderr, &p->store[i]);
      printf("\n");
   }
}
