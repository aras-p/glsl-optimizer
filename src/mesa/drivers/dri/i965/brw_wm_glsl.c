#include "main/macros.h"
#include "program/prog_parameter.h"
#include "program/prog_print.h"
#include "program/prog_optimize.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"

static struct brw_reg get_dst_reg(struct brw_wm_compile *c,
                                  const struct prog_instruction *inst,
                                  GLuint component);

/**
 * Determine if the given fragment program uses GLSL features such
 * as flow conditionals, loops, subroutines.
 * Some GLSL shaders may use these features, others might not.
 */
GLboolean brw_wm_is_glsl(const struct gl_fragment_program *fp)
{
    int i;

    if (unlikely(INTEL_DEBUG & DEBUG_GLSL_FORCE))
       return GL_TRUE;

    for (i = 0; i < fp->Base.NumInstructions; i++) {
	const struct prog_instruction *inst = &fp->Base.Instructions[i];
	switch (inst->Opcode) {
	    case OPCODE_ARL:
	    case OPCODE_IF:
	    case OPCODE_ENDIF:
	    case OPCODE_CAL:
	    case OPCODE_BRK:
	    case OPCODE_RET:
	    case OPCODE_BGNLOOP:
		return GL_TRUE; 
	    default:
		break;
	}
    }
    return GL_FALSE; 
}



static void
reclaim_temps(struct brw_wm_compile *c);


/** Mark GRF register as used. */
static void
prealloc_grf(struct brw_wm_compile *c, int r)
{
   c->used_grf[r] = GL_TRUE;
}


/** Mark given GRF register as not in use. */
static void
release_grf(struct brw_wm_compile *c, int r)
{
   /*assert(c->used_grf[r]);*/
   c->used_grf[r] = GL_FALSE;
   c->first_free_grf = MIN2(c->first_free_grf, r);
}


/** Return index of a free GRF, mark it as used. */
static int
alloc_grf(struct brw_wm_compile *c)
{
   GLuint r;
   for (r = c->first_free_grf; r < BRW_WM_MAX_GRF; r++) {
      if (!c->used_grf[r]) {
         c->used_grf[r] = GL_TRUE;
         c->first_free_grf = r + 1;  /* a guess */
         return r;
      }
   }

   /* no free temps, try to reclaim some */
   reclaim_temps(c);
   c->first_free_grf = 0;

   /* try alloc again */
   for (r = c->first_free_grf; r < BRW_WM_MAX_GRF; r++) {
      if (!c->used_grf[r]) {
         c->used_grf[r] = GL_TRUE;
         c->first_free_grf = r + 1;  /* a guess */
         return r;
      }
   }

   for (r = 0; r < BRW_WM_MAX_GRF; r++) {
      assert(c->used_grf[r]);
   }

   /* really, no free GRF regs found */
   if (!c->out_of_regs) {
      /* print warning once per compilation */
      _mesa_warning(NULL, "i965: ran out of registers for fragment program");
      c->out_of_regs = GL_TRUE;
   }

   return -1;
}


/** Return number of GRF registers used */
static int
num_grf_used(const struct brw_wm_compile *c)
{
   int r;
   for (r = BRW_WM_MAX_GRF - 1; r >= 0; r--)
      if (c->used_grf[r])
         return r + 1;
   return 0;
}



/**
 * Record the mapping of a Mesa register to a hardware register.
 */
static void set_reg(struct brw_wm_compile *c, int file, int index, 
	int component, struct brw_reg reg)
{
    c->wm_regs[file][index][component].reg = reg;
    c->wm_regs[file][index][component].inited = GL_TRUE;
}

static struct brw_reg alloc_tmp(struct brw_wm_compile *c)
{
    struct brw_reg reg;

    /* if we need to allocate another temp, grow the tmp_regs[] array */
    if (c->tmp_index == c->tmp_max) {
       int r = alloc_grf(c);
       if (r < 0) {
          /*printf("Out of temps in %s\n", __FUNCTION__);*/
          r = 50; /* XXX random register! */
       }
       c->tmp_regs[ c->tmp_max++ ] = r;
    }

    /* form the GRF register */
    reg = brw_vec8_grf(c->tmp_regs[ c->tmp_index++ ], 0);
    /*printf("alloc_temp %d\n", reg.nr);*/
    assert(reg.nr < BRW_WM_MAX_GRF);
    return reg;

}

/**
 * Save current temp register info.
 * There must be a matching call to release_tmps().
 */
static int mark_tmps(struct brw_wm_compile *c)
{
    return c->tmp_index;
}

static void release_tmps(struct brw_wm_compile *c, int mark)
{
    c->tmp_index = mark;
}

/**
 * Convert Mesa src register to brw register.
 *
 * Since we're running in SOA mode each Mesa register corresponds to four
 * hardware registers.  We allocate the hardware registers as needed here.
 *
 * \param file  register file, one of PROGRAM_x
 * \param index  register number
 * \param component  src component (X=0, Y=1, Z=2, W=3)
 * \param nr  not used?!?
 * \param neg  negate value?
 * \param abs  take absolute value?
 */
static struct brw_reg 
get_reg(struct brw_wm_compile *c, int file, int index, int component,
        int nr, GLuint neg, GLuint abs)
{
    struct brw_reg reg;
    switch (file) {
	case PROGRAM_STATE_VAR:
	case PROGRAM_CONSTANT:
	case PROGRAM_UNIFORM:
	    file = PROGRAM_STATE_VAR;
	    break;
	case PROGRAM_UNDEFINED:
	    return brw_null_reg();	
	case PROGRAM_TEMPORARY:
	case PROGRAM_INPUT:
	case PROGRAM_OUTPUT:
	case PROGRAM_PAYLOAD:
	    break;
	default:
	    _mesa_problem(NULL, "Unexpected file in get_reg()");
	    return brw_null_reg();
    }

    assert(index < 256);
    assert(component < 4);

    /* see if we've already allocated a HW register for this Mesa register */
    if (c->wm_regs[file][index][component].inited) {
       /* yes, re-use */
       reg = c->wm_regs[file][index][component].reg;
    }
    else {
	/* no, allocate new register */
       int grf = alloc_grf(c);
       /*printf("alloc grf %d for reg %d:%d.%d\n", grf, file, index, component);*/
       if (grf < 0) {
          /* totally out of temps */
          grf = 51; /* XXX random register! */
       }

       reg = brw_vec8_grf(grf, 0);
       /*printf("Alloc new grf %d for %d.%d\n", reg.nr, index, component);*/

       set_reg(c, file, index, component, reg);
    }

    if (neg & (1 << component)) {
	reg = negate(reg);
    }
    if (abs)
	reg = brw_abs(reg);
    return reg;
}



/**
 * This is called if we run out of GRF registers.  Examine the live intervals
 * of temp regs in the program and free those which won't be used again.
 */
static void
reclaim_temps(struct brw_wm_compile *c)
{
   GLint intBegin[MAX_PROGRAM_TEMPS];
   GLint intEnd[MAX_PROGRAM_TEMPS];
   int index;

   /*printf("Reclaim temps:\n");*/

   _mesa_find_temp_intervals(c->prog_instructions, c->nr_fp_insns,
                             intBegin, intEnd);

   for (index = 0; index < MAX_PROGRAM_TEMPS; index++) {
      if (intEnd[index] != -1 && intEnd[index] < c->cur_inst) {
         /* program temp[i] can be freed */
         int component;
         /*printf("  temp[%d] is dead\n", index);*/
         for (component = 0; component < 4; component++) {
            if (c->wm_regs[PROGRAM_TEMPORARY][index][component].inited) {
               int r = c->wm_regs[PROGRAM_TEMPORARY][index][component].reg.nr;
               release_grf(c, r);
               /*
               printf("  Reclaim temp %d, reg %d at inst %d\n",
                      index, r, c->cur_inst);
               */
               c->wm_regs[PROGRAM_TEMPORARY][index][component].inited = GL_FALSE;
            }
         }
      }
   }
}




/**
 * Preallocate registers.  This sets up the Mesa to hardware register
 * mapping for certain registers, such as constants (uniforms/state vars)
 * and shader inputs.
 */
static void prealloc_reg(struct brw_wm_compile *c)
{
    struct intel_context *intel = &c->func.brw->intel;
    int i, j;
    struct brw_reg reg;
    int urb_read_length = 0;
    GLuint inputs = FRAG_BIT_WPOS | c->fp_interp_emitted;
    GLuint reg_index = 0;

    memset(c->used_grf, GL_FALSE, sizeof(c->used_grf));
    c->first_free_grf = 0;

    for (i = 0; i < 4; i++) {
	if (i < (c->nr_payload_regs + 1) / 2)
            reg = brw_vec8_grf(i * 2, 0);
        else
            reg = brw_vec8_grf(0, 0);
	set_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH, i, reg);
    }
    set_reg(c, PROGRAM_PAYLOAD, PAYLOAD_W, 0,
	    brw_vec8_grf(c->source_w_reg, 0));
    reg_index += c->nr_payload_regs;

    /* constants */
    {
        const GLuint nr_params = c->fp->program.Base.Parameters->NumParameters;
        const GLuint nr_temps = c->fp->program.Base.NumTemporaries;

        /* use a real constant buffer, or just use a section of the GRF? */
        /* XXX this heuristic may need adjustment... */
        if ((nr_params + nr_temps) * 4 + reg_index > 80) {
	   for (i = 0; i < nr_params; i++) {
	      float *pv = c->fp->program.Base.Parameters->ParameterValues[i];
	      for (j = 0; j < 4; j++) {
		 c->prog_data.pull_param[c->prog_data.nr_pull_params] = &pv[j];
		 c->prog_data.nr_pull_params++;
	      }
	   }

	   c->prog_data.nr_params = 0;
	}
        /*printf("WM use_const_buffer = %d\n", c->fp->use_const_buffer);*/

        if (!c->prog_data.nr_pull_params) {
           const struct gl_program_parameter_list *plist = 
              c->fp->program.Base.Parameters;
           int index = 0;

           /* number of float constants in CURBE */
           c->prog_data.nr_params = 4 * nr_params;

           /* loop over program constants (float[4]) */
           for (i = 0; i < nr_params; i++) {
              /* loop over XYZW channels */
              for (j = 0; j < 4; j++, index++) {
                 reg = brw_vec1_grf(reg_index + index / 8, index % 8);
                 /* Save pointer to parameter/constant value.
                  * Constants will be copied in prepare_constant_buffer()
                  */
                 c->prog_data.param[index] = &plist->ParameterValues[i][j];
                 set_reg(c, PROGRAM_STATE_VAR, i, j, reg);
              }
           }
           /* number of constant regs used (each reg is float[8]) */
	   c->nr_creg = ALIGN(nr_params, 2) / 2;
	   reg_index += c->nr_creg;
        }
    }

    /* fragment shader inputs: One 2-reg pair of interpolation
     * coefficients for each vec4 to be set up.
     */
    if (intel->gen >= 6) {
       for (i = 0; i < FRAG_ATTRIB_MAX; i++) {
	  if (!(c->fp->program.Base.InputsRead & BITFIELD64_BIT(i)))
	     continue;

	  reg = brw_vec8_grf(reg_index, 0);
	  for (j = 0; j < 4; j++) {
	     set_reg(c, PROGRAM_PAYLOAD, i, j, reg);
	  }
	  reg_index += 2;
       }
       urb_read_length = reg_index;
    } else {
       for (i = 0; i < VERT_RESULT_MAX; i++) {
	  int fp_input;

	  if (i >= VERT_RESULT_VAR0)
	     fp_input = i - VERT_RESULT_VAR0 + FRAG_ATTRIB_VAR0;
	  else if (i <= VERT_RESULT_TEX7)
	     fp_input = i;
	  else
	     fp_input = -1;

	  if (fp_input >= 0 && inputs & (1 << fp_input)) {
	     urb_read_length = reg_index;
	     reg = brw_vec8_grf(reg_index, 0);
	     for (j = 0; j < 4; j++)
		set_reg(c, PROGRAM_PAYLOAD, fp_input, j, reg);
	  }
	  if (c->key.vp_outputs_written & BITFIELD64_BIT(i)) {
	     reg_index += 2;
	  }
       }
    }

    c->prog_data.first_curbe_grf = c->nr_payload_regs;
    c->prog_data.urb_read_length = urb_read_length;
    c->prog_data.curb_read_length = c->nr_creg;
    c->emit_mask_reg = brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, reg_index, 0);
    reg_index++;
    c->stack =  brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, reg_index, 0);
    reg_index += 2;

    /* mark GRF regs [0..reg_index-1] as in-use */
    for (i = 0; i < reg_index; i++)
       prealloc_grf(c, i);

    /* Don't use GRF 126, 127.  Using them seems to lead to GPU lock-ups */
    prealloc_grf(c, 126);
    prealloc_grf(c, 127);

    for (i = 0; i < c->nr_fp_insns; i++) {
	const struct prog_instruction *inst = &c->prog_instructions[i];
	struct brw_reg dst[4];

	switch (inst->Opcode) {
	case OPCODE_TEX:
	case OPCODE_TXB:
	    /* Allocate the channels of texture results contiguously,
	     * since they are written out that way by the sampler unit.
	     */
	    for (j = 0; j < 4; j++) {
		dst[j] = get_dst_reg(c, inst, j);
		if (j != 0)
		    assert(dst[j].nr == dst[j - 1].nr + 1);
	    }
	    break;
	default:
	    break;
	}
    }

    for (i = 0; i < c->nr_fp_insns; i++) {
	const struct prog_instruction *inst = &c->prog_instructions[i];

	switch (inst->Opcode) {
	case WM_DELTAXY:
	    /* Allocate WM_DELTAXY destination on G45/GM45 to an
	     * even-numbered GRF if possible so that we can use the PLN
	     * instruction.
	     */
	    if (inst->DstReg.WriteMask == WRITEMASK_XY &&
		!c->wm_regs[inst->DstReg.File][inst->DstReg.Index][0].inited &&
		!c->wm_regs[inst->DstReg.File][inst->DstReg.Index][1].inited &&
		(IS_G4X(intel->intelScreen->deviceID) || intel->gen == 5)) {
		int grf;

		for (grf = c->first_free_grf & ~1;
		     grf < BRW_WM_MAX_GRF;
		     grf += 2)
		{
		    if (!c->used_grf[grf] && !c->used_grf[grf + 1]) {
			c->used_grf[grf] = GL_TRUE;
			c->used_grf[grf + 1] = GL_TRUE;
			c->first_free_grf = grf + 2;  /* a guess */

			set_reg(c, inst->DstReg.File, inst->DstReg.Index, 0,
				brw_vec8_grf(grf, 0));
			set_reg(c, inst->DstReg.File, inst->DstReg.Index, 1,
				brw_vec8_grf(grf + 1, 0));
			break;
		    }
		}
	    }
	default:
	    break;
	}
    }

    /* An instruction may reference up to three constants.
     * They'll be found in these registers.
     * XXX alloc these on demand!
     */
    if (c->prog_data.nr_pull_params) {
       for (i = 0; i < 3; i++) {
          c->current_const[i].index = -1;
          c->current_const[i].reg = brw_vec8_grf(alloc_grf(c), 0);
       }
    }
#if 0
    printf("USE CONST BUFFER? %d\n", c->fp->use_const_buffer);
    printf("AFTER PRE_ALLOC, reg_index = %d\n", reg_index);
#endif
}


/**
 * Check if any of the instruction's src registers are constants, uniforms,
 * or statevars.  If so, fetch any constants that we don't already have in
 * the three GRF slots.
 */
static void fetch_constants(struct brw_wm_compile *c,
                            const struct prog_instruction *inst)
{
   struct brw_compile *p = &c->func;
   GLuint i;

   /* loop over instruction src regs */
   for (i = 0; i < 3; i++) {
      const struct prog_src_register *src = &inst->SrcReg[i];
      if (src->File == PROGRAM_STATE_VAR ||
          src->File == PROGRAM_CONSTANT ||
          src->File == PROGRAM_UNIFORM) {
	 c->current_const[i].index = src->Index;

#if 0
	 printf("  fetch const[%d] for arg %d into reg %d\n",
		src->Index, i, c->current_const[i].reg.nr);
#endif

	 /* need to fetch the constant now */
	 brw_oword_block_read(p,
			      c->current_const[i].reg,
			      brw_message_reg(1),
			      16 * src->Index,
			      SURF_INDEX_FRAG_CONST_BUFFER);
      }
   }
}


/**
 * Convert Mesa dst register to brw register.
 */
static struct brw_reg get_dst_reg(struct brw_wm_compile *c, 
                                  const struct prog_instruction *inst,
                                  GLuint component)
{
    const int nr = 1;
    return get_reg(c, inst->DstReg.File, inst->DstReg.Index, component, nr,
	    0, 0);
}


static struct brw_reg
get_src_reg_const(struct brw_wm_compile *c,
                  const struct prog_instruction *inst,
                  GLuint srcRegIndex, GLuint component)
{
   /* We should have already fetched the constant from the constant
    * buffer in fetch_constants().  Now we just have to return a
    * register description that extracts the needed component and
    * smears it across all eight vector components.
    */
   const struct prog_src_register *src = &inst->SrcReg[srcRegIndex];
   struct brw_reg const_reg;

   assert(component < 4);
   assert(srcRegIndex < 3);
   assert(c->current_const[srcRegIndex].index != -1);
   const_reg = c->current_const[srcRegIndex].reg;

   /* extract desired float from the const_reg, and smear */
   const_reg = stride(const_reg, 0, 1, 0);
   const_reg.subnr = component * 4;

   if (src->Negate & (1 << component))
      const_reg = negate(const_reg);
   if (src->Abs)
      const_reg = brw_abs(const_reg);

#if 0
   printf("  form const[%d].%d for arg %d, reg %d\n",
          c->current_const[srcRegIndex].index,
          component,
          srcRegIndex,
          const_reg.nr);
#endif

   return const_reg;
}


/**
 * Convert Mesa src register to brw register.
 */
static struct brw_reg get_src_reg(struct brw_wm_compile *c, 
                                  const struct prog_instruction *inst,
                                  GLuint srcRegIndex, GLuint channel)
{
    const struct prog_src_register *src = &inst->SrcReg[srcRegIndex];
    const GLuint nr = 1;
    const GLuint component = GET_SWZ(src->Swizzle, channel);

    /* Only one immediate value can be used per native opcode, and it
     * has be in the src1 slot, so not all Mesa instructions will get
     * to take advantage of immediate constants.
     */
    if (brw_wm_arg_can_be_immediate(inst->Opcode, srcRegIndex)) {
       const struct gl_program_parameter_list *params;

       params = c->fp->program.Base.Parameters;

       /* Extended swizzle terms */
       if (component == SWIZZLE_ZERO) {
	  return brw_imm_f(0.0F);
       } else if (component == SWIZZLE_ONE) {
	  if (src->Negate)
	     return brw_imm_f(-1.0F);
	  else
	     return brw_imm_f(1.0F);
       }

       if (src->File == PROGRAM_CONSTANT) {
	  float f = params->ParameterValues[src->Index][component];

	  if (src->Abs)
	     f = fabs(f);
	  if (src->Negate)
	     f = -f;

	  return brw_imm_f(f);
       }
    }

    if (c->prog_data.nr_pull_params &&
        (src->File == PROGRAM_STATE_VAR ||
         src->File == PROGRAM_CONSTANT ||
         src->File == PROGRAM_UNIFORM)) {
       return get_src_reg_const(c, inst, srcRegIndex, component);
    }
    else {
       /* other type of source register */
       return get_reg(c, src->File, src->Index, component, nr, 
                      src->Negate, src->Abs);
    }
}

static void emit_arl(struct brw_wm_compile *c,
                     const struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, addr_reg;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    addr_reg = brw_uw8_reg(BRW_ARCHITECTURE_REGISTER_FILE, 
                           BRW_ARF_ADDRESS, 0);
    src0 = get_src_reg(c, inst, 0, 0); /* channel 0 */
    brw_MOV(p, addr_reg, src0);
    brw_set_saturate(p, 0);
}

static INLINE struct brw_reg high_words( struct brw_reg reg )
{
    return stride( suboffset( retype( reg, BRW_REGISTER_TYPE_W ), 1 ),
		   0, 8, 2 );
}

static INLINE struct brw_reg low_words( struct brw_reg reg )
{
    return stride( retype( reg, BRW_REGISTER_TYPE_W ), 0, 8, 2 );
}

static INLINE struct brw_reg even_bytes( struct brw_reg reg )
{
    return stride( retype( reg, BRW_REGISTER_TYPE_B ), 0, 16, 2 );
}

static INLINE struct brw_reg odd_bytes( struct brw_reg reg )
{
    return stride( suboffset( retype( reg, BRW_REGISTER_TYPE_B ), 1 ),
		   0, 16, 2 );
}

/**
 * Resolve subroutine calls after code emit is done.
 */
static void post_wm_emit( struct brw_wm_compile *c )
{
    brw_resolve_cals(&c->func);
}

static void
get_argument_regs(struct brw_wm_compile *c,
		  const struct prog_instruction *inst,
		  int index,
		  struct brw_reg *dst,
		  struct brw_reg *regs,
		  int mask)
{
    struct brw_compile *p = &c->func;
    int i, j;

    for (i = 0; i < 4; i++) {
	if (mask & (1 << i)) {
	    regs[i] = get_src_reg(c, inst, index, i);

	    /* Unalias destination registers from our sources. */
	    if (regs[i].file == BRW_GENERAL_REGISTER_FILE) {
	       for (j = 0; j < 4; j++) {
		   if (memcmp(&regs[i], &dst[j], sizeof(regs[0])) == 0) {
		       struct brw_reg tmp = alloc_tmp(c);
		       brw_MOV(p, tmp, regs[i]);
		       regs[i] = tmp;
		       break;
		   }
	       }
	    }
	}
    }
}

static void brw_wm_emit_glsl(struct brw_context *brw, struct brw_wm_compile *c)
{
   struct intel_context *intel = &brw->intel;
#define MAX_IF_DEPTH 32
#define MAX_LOOP_DEPTH 32
    struct brw_instruction *if_inst[MAX_IF_DEPTH], *loop_inst[MAX_LOOP_DEPTH];
    int if_depth_in_loop[MAX_LOOP_DEPTH];
    GLuint i, if_depth = 0, loop_depth = 0;
    struct brw_compile *p = &c->func;
    struct brw_indirect stack_index = brw_indirect(0, 0);

    c->out_of_regs = GL_FALSE;

    if_depth_in_loop[loop_depth] = 0;

    prealloc_reg(c);
    brw_set_compression_control(p, BRW_COMPRESSION_NONE);
    brw_MOV(p, get_addr_reg(stack_index), brw_address(c->stack));

    if (intel->gen >= 6)
	brw_set_acc_write_control(p, 1);

    for (i = 0; i < c->nr_fp_insns; i++) {
        const struct prog_instruction *inst = &c->prog_instructions[i];
	int dst_flags;
	struct brw_reg args[3][4], dst[4];
	int j;
	int mark = mark_tmps( c );

        c->cur_inst = i;

#if 0
        printf("Inst %d: ", i);
        _mesa_print_instruction(inst);
#endif

        /* fetch any constants that this instruction needs */
        if (c->prog_data.nr_pull_params)
           fetch_constants(c, inst);

	if (inst->Opcode != OPCODE_ARL) {
	   for (j = 0; j < 4; j++) {
	      if (inst->DstReg.WriteMask & (1 << j))
		 dst[j] = get_dst_reg(c, inst, j);
	      else
		 dst[j] = brw_null_reg();
	   }
	}
	for (j = 0; j < brw_wm_nr_args(inst->Opcode); j++)
	    get_argument_regs(c, inst, j, dst, args[j], WRITEMASK_XYZW);

	dst_flags = inst->DstReg.WriteMask;
	if (inst->SaturateMode == SATURATE_ZERO_ONE)
	    dst_flags |= SATURATE;

	if (inst->CondUpdate)
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
	else
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);

	switch (inst->Opcode) {
	    case WM_PIXELXY:
		emit_pixel_xy(c, dst, dst_flags);
		break;
	    case WM_DELTAXY: 
		emit_delta_xy(p, dst, dst_flags, args[0]);
		break;
	    case WM_PIXELW:
		emit_pixel_w(c, dst, dst_flags, args[0], args[1]);
		break;	
	    case WM_LINTERP:
		emit_linterp(p, dst, dst_flags, args[0], args[1]);
		break;
	    case WM_PINTERP:
		emit_pinterp(p, dst, dst_flags, args[0], args[1], args[2]);
		break;
	    case WM_CINTERP:
		emit_cinterp(p, dst, dst_flags, args[0]);
		break;
	    case WM_WPOSXY:
		emit_wpos_xy(c, dst, dst_flags, args[0]);
		break;
	    case WM_FB_WRITE:
		emit_fb_write(c, args[0], args[1], args[2],
			      INST_AUX_GET_TARGET(inst->Aux),
			      inst->Aux & INST_AUX_EOT);
		break;
	    case WM_FRONTFACING:
		emit_frontfacing(p, dst, dst_flags);
		break;
	    case OPCODE_ADD:
		emit_alu2(p, brw_ADD, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_ARL:
		emit_arl(c, inst);
		break;
	    case OPCODE_FRC:
		emit_alu1(p, brw_FRC, dst, dst_flags, args[0]);
		break;
	    case OPCODE_FLR:
		emit_alu1(p, brw_RNDD, dst, dst_flags, args[0]);
		break;
	    case OPCODE_LRP:
		emit_lrp(p, dst, dst_flags, args[0], args[1], args[2]);
		break;
	    case OPCODE_TRUNC:
		emit_alu1(p, brw_RNDZ, dst, dst_flags, args[0]);
		break;
	    case OPCODE_MOV:
	    case OPCODE_SWZ:
		emit_alu1(p, brw_MOV, dst, dst_flags, args[0]);
		break;
	    case OPCODE_DP2:
		emit_dp2(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_DP3:
		emit_dp3(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_DP4:
		emit_dp4(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_XPD:
		emit_xpd(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_DPH:
		emit_dph(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_RCP:
		emit_math1(c, BRW_MATH_FUNCTION_INV, dst, dst_flags, args[0]);
		break;
	    case OPCODE_RSQ:
		emit_math1(c, BRW_MATH_FUNCTION_RSQ, dst, dst_flags, args[0]);
		break;
	    case OPCODE_SIN:
		emit_math1(c, BRW_MATH_FUNCTION_SIN, dst, dst_flags, args[0]);
		break;
	    case OPCODE_COS:
		emit_math1(c, BRW_MATH_FUNCTION_COS, dst, dst_flags, args[0]);
		break;
	    case OPCODE_EX2:
		emit_math1(c, BRW_MATH_FUNCTION_EXP, dst, dst_flags, args[0]);
		break;
	    case OPCODE_LG2:
		emit_math1(c, BRW_MATH_FUNCTION_LOG, dst, dst_flags, args[0]);
		break;
	    case OPCODE_CMP:
		emit_cmp(p, dst, dst_flags, args[0], args[1], args[2]);
		break;
	    case OPCODE_MIN:	
		emit_min(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_MAX:	
		emit_max(p, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_DDX:
	    case OPCODE_DDY:
		emit_ddxy(p, dst, dst_flags, (inst->Opcode == OPCODE_DDX),
			  args[0]);
                break;
	    case OPCODE_SLT:
		emit_sop(p, dst, dst_flags,
			 BRW_CONDITIONAL_L, args[0], args[1]);
		break;
	    case OPCODE_SLE:
		emit_sop(p, dst, dst_flags,
			 BRW_CONDITIONAL_LE, args[0], args[1]);
		break;
	    case OPCODE_SGT:
		emit_sop(p, dst, dst_flags,
			 BRW_CONDITIONAL_G, args[0], args[1]);
		break;
	    case OPCODE_SGE:
		emit_sop(p, dst, dst_flags,
			 BRW_CONDITIONAL_GE, args[0], args[1]);
		break;
	    case OPCODE_SEQ:
		emit_sop(p, dst, dst_flags,
			 BRW_CONDITIONAL_EQ, args[0], args[1]);
		break;
	    case OPCODE_SNE:
		emit_sop(p, dst, dst_flags,
			 BRW_CONDITIONAL_NEQ, args[0], args[1]);
		break;
	    case OPCODE_SSG:
		emit_sign(p, dst, dst_flags, args[0]);
		break;
	    case OPCODE_MUL:
		emit_alu2(p, brw_MUL, dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_POW:
		emit_math2(c, BRW_MATH_FUNCTION_POW,
			   dst, dst_flags, args[0], args[1]);
		break;
	    case OPCODE_MAD:
		emit_mad(p, dst, dst_flags, args[0], args[1], args[2]);
		break;
	    case OPCODE_TEX:
		emit_tex(c, dst, dst_flags, args[0],
			 get_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH,
				 0, 1, 0, 0),
			 inst->TexSrcTarget,
			 inst->TexSrcUnit,
			 (c->key.shadowtex_mask & (1 << inst->TexSrcUnit)) != 0);
		break;
	    case OPCODE_TXB:
		emit_txb(c, dst, dst_flags, args[0],
			 get_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH,
				 0, 1, 0, 0),
			 inst->TexSrcTarget,
			 c->fp->program.Base.SamplerUnits[inst->TexSrcUnit]);
		break;
	    case OPCODE_KIL_NV:
		emit_kil_nv(c);
		break;
	    case OPCODE_IF:
		assert(if_depth < MAX_IF_DEPTH);
		if_inst[if_depth++] = brw_IF(p, BRW_EXECUTE_8);
		if_depth_in_loop[loop_depth]++;
		break;
	    case OPCODE_ELSE:
		assert(if_depth > 0);
		if_inst[if_depth-1]  = brw_ELSE(p, if_inst[if_depth-1]);
		break;
	    case OPCODE_ENDIF:
		assert(if_depth > 0);
		brw_ENDIF(p, if_inst[--if_depth]);
		if_depth_in_loop[loop_depth]--;
		break;
	    case OPCODE_BGNSUB:
		brw_save_label(p, inst->Comment, p->nr_insn);
		break;
	    case OPCODE_ENDSUB:
		/* no-op */
		break;
	    case OPCODE_CAL: 
		brw_push_insn_state(p);
		brw_set_mask_control(p, BRW_MASK_DISABLE);
                brw_set_access_mode(p, BRW_ALIGN_1);
                brw_ADD(p, deref_1ud(stack_index, 0), brw_ip_reg(), brw_imm_d(3*16));
                brw_set_access_mode(p, BRW_ALIGN_16);
                brw_ADD(p, get_addr_reg(stack_index),
                         get_addr_reg(stack_index), brw_imm_d(4));
		brw_save_call(&c->func, inst->Comment, p->nr_insn);
                brw_ADD(p, brw_ip_reg(), brw_ip_reg(), brw_imm_d(1*16));
                brw_pop_insn_state(p);
		break;

	    case OPCODE_RET:
		brw_push_insn_state(p);
		brw_set_mask_control(p, BRW_MASK_DISABLE);
                brw_ADD(p, get_addr_reg(stack_index),
                        get_addr_reg(stack_index), brw_imm_d(-4));
                brw_set_access_mode(p, BRW_ALIGN_1);
                brw_MOV(p, brw_ip_reg(), deref_1ud(stack_index, 0));
                brw_set_access_mode(p, BRW_ALIGN_16);
		brw_pop_insn_state(p);

		break;
	    case OPCODE_BGNLOOP:
                /* XXX may need to invalidate the current_constant regs */
		loop_inst[loop_depth++] = brw_DO(p, BRW_EXECUTE_8);
		if_depth_in_loop[loop_depth] = 0;
		break;
	    case OPCODE_BRK:
		brw_BREAK(p, if_depth_in_loop[loop_depth]);
		brw_set_predicate_control(p, BRW_PREDICATE_NONE);
		break;
	    case OPCODE_CONT:
		brw_CONT(p, if_depth_in_loop[loop_depth]);
		brw_set_predicate_control(p, BRW_PREDICATE_NONE);
		break;
	    case OPCODE_ENDLOOP: 
               {
                  struct brw_instruction *inst0, *inst1;
                  GLuint br = 1;

                  if (intel->gen == 5)
                     br = 2;

		  assert(loop_depth > 0);
                  loop_depth--;
                  inst0 = inst1 = brw_WHILE(p, loop_inst[loop_depth]);
                  /* patch all the BREAK/CONT instructions from last BGNLOOP */
                  while (inst0 > loop_inst[loop_depth]) {
                     inst0--;
                     if (inst0->header.opcode == BRW_OPCODE_BREAK &&
			 inst0->bits3.if_else.jump_count == 0) {
			inst0->bits3.if_else.jump_count = br * (inst1 - inst0 + 1);
                     }
                     else if (inst0->header.opcode == BRW_OPCODE_CONTINUE &&
			      inst0->bits3.if_else.jump_count == 0) {
                        inst0->bits3.if_else.jump_count = br * (inst1 - inst0);
                     }
                  }
               }
               break;
	    default:
		printf("unsupported opcode %d (%s) in fragment shader\n",
		       inst->Opcode, inst->Opcode < MAX_OPCODE ?
		       _mesa_opcode_string(inst->Opcode) : "unknown");
	}

	/* Release temporaries containing any unaliased source regs. */
	release_tmps( c, mark );

	if (inst->CondUpdate)
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	else
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
    }
    post_wm_emit(c);

    if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
      printf("wm-native:\n");
      for (i = 0; i < p->nr_insn; i++)
	 brw_disasm(stdout, &p->store[i], intel->gen);
      printf("\n");
    }
}

/**
 * Do GPU code generation for shaders that use GLSL features such as
 * flow control.  Other shaders will be compiled with the 
 */
void brw_wm_glsl_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
    if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
        printf("brw_wm_glsl_emit:\n");
    }

    /* initial instruction translation/simplification */
    brw_wm_pass_fp(c);

    /* actual code generation */
    brw_wm_emit_glsl(brw, c);

    if (unlikely(INTEL_DEBUG & DEBUG_WM)) {
        brw_wm_print_program(c, "brw_wm_glsl_emit done");
    }

    c->prog_data.total_grf = num_grf_used(c);
    c->prog_data.total_scratch = 0;
}
