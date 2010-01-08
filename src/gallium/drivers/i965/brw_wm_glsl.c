#include "util/u_math.h"


#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"


static struct brw_reg get_dst_reg(struct brw_wm_compile *c,
                                  const struct brw_fp_instruction *inst,
                                  GLuint component);


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
      debug_printf("%s: ran out of registers for fragment program", __FUNCTION__);
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

static struct brw_reg lookup_tmp( struct brw_wm_compile *c, int index )
{
    return brw_vec8_grf( c->tmp_regs[ index ], 0 );
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
	case TGSI_FILE_NULL:
	    return brw_null_reg();	

	case TGSI_FILE_CONSTANT:
	case TGSI_FILE_TEMPORARY:
	case TGSI_FILE_INPUT:
	case TGSI_FILE_OUTPUT:
	case BRW_FILE_PAYLOAD:
	    break;

	default:
	   debug_printf("%s: Unexpected file type\n", __FUNCTION__);
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
 * Find first/last instruction that references each temporary register.
 */
GLboolean
_mesa_find_temp_intervals(const struct prog_instruction *instructions,
                          GLuint numInstructions,
                          GLint intBegin[MAX_PROGRAM_TEMPS],
                          GLint intEnd[MAX_PROGRAM_TEMPS])
{
   struct loop_info
   {
      GLuint Start, End;  /**< Start, end instructions of loop */
   };
   struct loop_info loopStack[MAX_LOOP_NESTING];
   GLuint loopStackDepth = 0;
   GLuint i;

   for (i = 0; i < MAX_PROGRAM_TEMPS; i++){
      intBegin[i] = intEnd[i] = -1;
   }

   /* Scan instructions looking for temporary registers */
   for (i = 0; i < numInstructions; i++) {
      const struct prog_instruction *inst = instructions + i;
      if (inst->Opcode == OPCODE_BGNLOOP) {
         loopStack[loopStackDepth].Start = i;
         loopStack[loopStackDepth].End = inst->BranchTarget;
         loopStackDepth++;
      }
      else if (inst->Opcode == OPCODE_ENDLOOP) {
         loopStackDepth--;
      }
      else if (inst->Opcode == OPCODE_CAL) {
         return GL_FALSE;
      }
      else {
         const GLuint numSrc = 3;
         GLuint j;
         for (j = 0; j < numSrc; j++) {
            if (inst->SrcReg[j].File == PROGRAM_TEMPORARY) {
               const GLuint index = inst->SrcReg[j].Index;
               if (inst->SrcReg[j].RelAddr)
                  return GL_FALSE;
               update_interval(intBegin, intEnd, index, i);
               if (loopStackDepth > 0) {
                  /* extend temp register's interval to end of loop */
                  GLuint loopEnd = loopStack[loopStackDepth - 1].End;
                  update_interval(intBegin, intEnd, index, loopEnd);
               }
            }
         }
         if (inst->DstReg.File == PROGRAM_TEMPORARY) {
            const GLuint index = inst->DstReg.Index;
            if (inst->DstReg.RelAddr)
               return GL_FALSE;
            update_interval(intBegin, intEnd, index, i);
            if (loopStackDepth > 0) {
               /* extend temp register's interval to end of loop */
               GLuint loopEnd = loopStack[loopStackDepth - 1].End;
               update_interval(intBegin, intEnd, index, loopEnd);
            }
         }
      }
   }

   return GL_TRUE;
}


/**
 * This is called if we run out of GRF registers.  Examine the live intervals
 * of temp regs in the program and free those which won't be used again.
 */
static void
reclaim_temps(struct brw_wm_compile *c)
{
   GLint intBegin[BRW_WM_MAX_TEMPS];
   GLint intEnd[BRW_WM_MAX_TEMPS];
   int index;

   /*printf("Reclaim temps:\n");*/

   _mesa_find_temp_intervals(c->fp_instructions, c->nr_fp_insns,
                             intBegin, intEnd);

   for (index = 0; index < BRW_WM_MAX_TEMPS; index++) {
      if (intEnd[index] != -1 && intEnd[index] < c->cur_inst) {
         /* program temp[i] can be freed */
         int component;
         /*printf("  temp[%d] is dead\n", index);*/
         for (component = 0; component < 4; component++) {
            if (c->wm_regs[TGSI_FILE_TEMPORARY][index][component].inited) {
               int r = c->wm_regs[TGSI_FILE_TEMPORARY][index][component].reg.nr;
               release_grf(c, r);
               /*
               printf("  Reclaim temp %d, reg %d at inst %d\n",
                      index, r, c->cur_inst);
               */
               c->wm_regs[TGSI_FILE_TEMPORARY][index][component].inited = GL_FALSE;
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
    int i, j;
    struct brw_reg reg;
    int urb_read_length = 0;
    GLuint inputs = FRAG_BIT_WPOS | c->fp_interp_emitted;
    GLuint reg_index = 0;

    memset(c->used_grf, GL_FALSE, sizeof(c->used_grf));
    c->first_free_grf = 0;

    for (i = 0; i < 4; i++) {
        if (i < c->key.nr_depth_regs) 
            reg = brw_vec8_grf(i * 2, 0);
        else
            reg = brw_vec8_grf(0, 0);
	set_reg(c, TGSI_FILE_PAYLOAD, PAYLOAD_DEPTH, i, reg);
    }
    reg_index += 2 * c->key.nr_depth_regs;

    /* constants */
    {
        const GLuint nr_params = c->fp->program.Base.Parameters->NumParameters;
        const GLuint nr_temps = c->fp->program.Base.NumTemporaries;

        /* use a real constant buffer, or just use a section of the GRF? */
        /* XXX this heuristic may need adjustment... */
        if ((nr_params + nr_temps) * 4 + reg_index > 80)
           c->fp->use_const_buffer = GL_TRUE;
        else
           c->fp->use_const_buffer = GL_FALSE;
        /*printf("WM use_const_buffer = %d\n", c->fp->use_const_buffer);*/

        if (c->fp->use_const_buffer) {
           /* We'll use a real constant buffer and fetch constants from
            * it with a dataport read message.
            */

           /* number of float constants in CURBE */
           c->prog_data.nr_params = 0;
        }
        else {
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
                 set_reg(c, TGSI_FILE_STATE_VAR, i, j, reg);
              }
           }
           /* number of constant regs used (each reg is float[8]) */
           c->nr_creg = 2 * ((4 * nr_params + 15) / 16);
           reg_index += c->nr_creg;
        }
    }

    /* fragment shader inputs */
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
	     set_reg(c, TGSI_FILE_PAYLOAD, fp_input, j, reg);
       }
       if (c->key.nr_vp_outputs > i) {
	  reg_index += 2;
       }
    }

    c->prog_data.first_curbe_grf = c->key.nr_depth_regs * 2;
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
	const struct brw_fp_instruction *inst = &c->fp_instructions[i];
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

    /* An instruction may reference up to three constants.
     * They'll be found in these registers.
     * XXX alloc these on demand!
     */
    if (c->fp->use_const_buffer) {
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
                            const struct brw_fp_instruction *inst)
{
   struct brw_compile *p = &c->func;
   GLuint i;

   /* loop over instruction src regs */
   for (i = 0; i < 3; i++) {
      const struct prog_src_register *src = &inst->SrcReg[i];
      if (src->File == TGSI_FILE_IMMEDIATE ||
          src->File == TGSI_FILE_CONSTANT) {
	 c->current_const[i].index = src->Index;

#if 0
	 printf("  fetch const[%d] for arg %d into reg %d\n",
		src->Index, i, c->current_const[i].reg.nr);
#endif

	 /* need to fetch the constant now */
	 brw_dp_READ_4(p,
		       c->current_const[i].reg,  /* writeback dest */
		       src->RelAddr,             /* relative indexing? */
		       16 * src->Index,          /* byte offset */
		       SURF_INDEX_FRAG_CONST_BUFFER/* binding table index */
		       );
      }
   }
}


/**
 * Convert Mesa dst register to brw register.
 */
static struct brw_reg get_dst_reg(struct brw_wm_compile *c, 
                                  const struct brw_fp_instruction *inst,
                                  GLuint component)
{
    const int nr = 1;
    return get_reg(c, inst->DstReg.File, inst->DstReg.Index, component, nr,
	    0, 0);
}


static struct brw_reg
get_src_reg_const(struct brw_wm_compile *c,
                  const struct brw_fp_instruction *inst,
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

   if (src->Negate)
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
                                  const struct brw_fp_instruction *inst,
                                  GLuint srcRegIndex, GLuint channel)
{
    const struct prog_src_register *src = &inst->SrcReg[srcRegIndex];
    const GLuint nr = 1;
    const GLuint component = BRW_GET_SWZ(src->Swizzle, channel);

    /* Extended swizzle terms */
    if (component == SWIZZLE_ZERO) {
       return brw_imm_f(0.0F);
    }
    else if (component == SWIZZLE_ONE) {
       return brw_imm_f(1.0F);
    }

    if (c->fp->use_const_buffer &&
        (src->File == TGSI_FILE_STATE_VAR ||
         src->File == TGSI_FILE_CONSTANT ||
         src->File == TGSI_FILE_UNIFORM)) {
       return get_src_reg_const(c, inst, srcRegIndex, component);
    }
    else {
       /* other type of source register */
       return get_reg(c, src->File, src->Index, component, nr, 
                      src->Negate, src->Abs);
    }
}


/**
 * Same as \sa get_src_reg() but if the register is a immediate, emit
 * a brw_reg encoding the immediate.
 * Note that a brw instruction only allows one src operand to be a immediate.
 * For instructions with more than one operand, only the second can be a
 * immediate.  This means that we treat some immediates as constants
 * (which why TGSI_FILE_IMMEDIATE is checked in fetch_constants()).
 * 
 */
static struct brw_reg get_src_reg_imm(struct brw_wm_compile *c, 
                                      const struct brw_fp_instruction *inst,
                                      GLuint srcRegIndex, GLuint channel)
{
    const struct prog_src_register *src = &inst->SrcReg[srcRegIndex];
    if (src->File == TGSI_FILE_IMMEDIATE) {
       /* an immediate */
       const int component = BRW_GET_SWZ(src->Swizzle, channel);
       const GLfloat *param =
          c->fp->program.Base.Parameters->ParameterValues[src->Index];
       GLfloat value = param[component];
       if (src->Negate)
          value = -value;
       if (src->Abs)
          value = FABSF(value);
#if 0
       printf("  form immed value %f for chan %d\n", value, channel);
#endif
       return brw_imm_f(value);
    }
    else {
       return get_src_reg(c, inst, srcRegIndex, channel);
    }
}


/**
 * Subroutines are minimal support for resusable instruction sequences.
 * They are implemented as simply as possible to minimise overhead: there
 * is no explicit support for communication between the caller and callee
 * other than saving the return address in a temporary register, nor is
 * there any automatic local storage.  This implies that great care is
 * required before attempting reentrancy or any kind of nested
 * subroutine invocations.
 */
static void invoke_subroutine( struct brw_wm_compile *c,
			       enum _subroutine subroutine,
			       void (*emit)( struct brw_wm_compile * ) )
{
    struct brw_compile *p = &c->func;

    assert( subroutine < BRW_WM_MAX_SUBROUTINE );
    
    if( c->subroutines[ subroutine ] ) {
	/* subroutine previously emitted: reuse existing instructions */

	int mark = mark_tmps( c );
	struct brw_reg return_address = retype( alloc_tmp( c ),
						BRW_REGISTER_TYPE_UD );
	int here = p->nr_insn;
	
	brw_push_insn_state(p);
	brw_set_mask_control(p, BRW_MASK_DISABLE);
	brw_ADD( p, return_address, brw_ip_reg(), brw_imm_ud( 2 << 4 ) );

	brw_ADD( p, brw_ip_reg(), brw_ip_reg(),
		 brw_imm_d( ( c->subroutines[ subroutine ] -
			      here - 1 ) << 4 ) );
	brw_pop_insn_state(p);

	release_tmps( c, mark );
    } else {
	/* previously unused subroutine: emit, and mark for later reuse */
	
	int mark = mark_tmps( c );
	struct brw_reg return_address = retype( alloc_tmp( c ),
						BRW_REGISTER_TYPE_UD );
	struct brw_instruction *calc;
	int base = p->nr_insn;
	
	brw_push_insn_state(p);
	brw_set_mask_control(p, BRW_MASK_DISABLE);
	calc = brw_ADD( p, return_address, brw_ip_reg(), brw_imm_ud( 0 ) );
	brw_pop_insn_state(p);
	
	c->subroutines[ subroutine ] = p->nr_insn;

	emit( c );
	
	brw_push_insn_state(p);
	brw_set_mask_control(p, BRW_MASK_DISABLE);
	brw_MOV( p, brw_ip_reg(), return_address );
	brw_pop_insn_state(p);

	brw_set_src1( calc, brw_imm_ud( ( p->nr_insn - base ) << 4 ) );
	
	release_tmps( c, mark );
    }
}

static void emit_trunc( struct brw_wm_compile *c,
                        const struct brw_fp_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    struct brw_reg src, dst;
	    dst = get_dst_reg(c, inst, i);
	    src = get_src_reg(c, inst, 0, i);
	    brw_RNDZ(p, dst, src);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_mov( struct brw_wm_compile *c,
                      const struct brw_fp_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    struct brw_reg src, dst;
	    dst = get_dst_reg(c, inst, i);
            /* XXX some moves from immediate value don't work reliably!!! */
            /*src = get_src_reg_imm(c, inst, 0, i);*/
            src = get_src_reg(c, inst, 0, i);
	    brw_MOV(p, dst, src);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_pixel_xy(struct brw_wm_compile *c,
                          const struct brw_fp_instruction *inst)
{
    struct brw_reg r1 = brw_vec1_grf(1, 0);
    struct brw_reg r1_uw = retype(r1, BRW_REGISTER_TYPE_UW);

    struct brw_reg dst0, dst1;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    dst0 = get_dst_reg(c, inst, 0);
    dst1 = get_dst_reg(c, inst, 1);
    /* Calculate pixel centers by adding 1 or 0 to each of the
     * micro-tile coordinates passed in r1.
     */
    if (mask & WRITEMASK_X) {
	brw_ADD(p,
		vec8(retype(dst0, BRW_REGISTER_TYPE_UW)),
		stride(suboffset(r1_uw, 4), 2, 4, 0),
		brw_imm_v(0x10101010));
    }

    if (mask & WRITEMASK_Y) {
	brw_ADD(p,
		vec8(retype(dst1, BRW_REGISTER_TYPE_UW)),
		stride(suboffset(r1_uw, 5), 2, 4, 0),
		brw_imm_v(0x11001100));
    }
}

static void emit_delta_xy(struct brw_wm_compile *c,
                          const struct brw_fp_instruction *inst)
{
    struct brw_reg r1 = brw_vec1_grf(1, 0);
    struct brw_reg dst0, dst1, src0, src1;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    dst0 = get_dst_reg(c, inst, 0);
    dst1 = get_dst_reg(c, inst, 1);
    src0 = get_src_reg(c, inst, 0, 0);
    src1 = get_src_reg(c, inst, 0, 1);
    /* Calc delta X,Y by subtracting origin in r1 from the pixel
     * centers.
     */
    if (mask & WRITEMASK_X) {
	brw_ADD(p,
		dst0,
		retype(src0, BRW_REGISTER_TYPE_UW),
		negate(r1));
    }

    if (mask & WRITEMASK_Y) {
	brw_ADD(p,
		dst1,
		retype(src1, BRW_REGISTER_TYPE_UW),
		negate(suboffset(r1,1)));

    }
}

static void fire_fb_write( struct brw_wm_compile *c,
                           GLuint base_reg,
                           GLuint nr,
                           GLuint target,
                           GLuint eot)
{
    struct brw_compile *p = &c->func;
    /* Pass through control information:
     */
    /*  mov (8) m1.0<1>:ud   r1.0<8;8,1>:ud   { Align1 NoMask } */
    {
	brw_push_insn_state(p);
	brw_set_mask_control(p, BRW_MASK_DISABLE); /* ? */
	brw_MOV(p,
		brw_message_reg(base_reg + 1),
		brw_vec8_grf(1, 0));
	brw_pop_insn_state(p);
    }
    /* Send framebuffer write message: */
    brw_fb_WRITE(p,
	    retype(vec8(brw_null_reg()), BRW_REGISTER_TYPE_UW),
	    base_reg,
	    retype(brw_vec8_grf(0, 0), BRW_REGISTER_TYPE_UW),
	    target,              
	    nr,
	    0,
	    eot);
}

static void emit_fb_write(struct brw_wm_compile *c,
                          const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    int nr = 2;
    int channel;
    GLuint target, eot;
    struct brw_reg src0;

    /* Reserve a space for AA - may not be needed:
     */
    if (c->key.aa_dest_stencil_reg)
	nr += 1;

    brw_push_insn_state(p);
    for (channel = 0; channel < 4; channel++) {
        src0 = get_src_reg(c,  inst, 0, channel);
        /*  mov (8) m2.0<1>:ud   r28.0<8;8,1>:ud  { Align1 } */
        /*  mov (8) m6.0<1>:ud   r29.0<8;8,1>:ud  { Align1 SecHalf } */
        brw_MOV(p, brw_message_reg(nr + channel), src0);
    }
    /* skip over the regs populated above: */
    nr += 8;
    brw_pop_insn_state(p);

    if (c->key.source_depth_to_render_target) {
       if (c->key.computes_depth) {
          src0 = get_src_reg(c, inst, 2, 2);
          brw_MOV(p, brw_message_reg(nr), src0);
       }
       else {
          src0 = get_src_reg(c, inst, 1, 1);
          brw_MOV(p, brw_message_reg(nr), src0);
       }

       nr += 2;
    }

    if (c->key.dest_depth_reg) {
        const GLuint comp = c->key.dest_depth_reg / 2;
        const GLuint off = c->key.dest_depth_reg % 2;

        if (off != 0) {
            /* XXX this code needs review/testing */
            struct brw_reg arg1_0 = get_src_reg(c, inst, 1, comp);
            struct brw_reg arg1_1 = get_src_reg(c, inst, 1, comp+1);

            brw_push_insn_state(p);
            brw_set_compression_control(p, BRW_COMPRESSION_NONE);

            brw_MOV(p, brw_message_reg(nr), offset(arg1_0, 1));
            /* 2nd half? */
            brw_MOV(p, brw_message_reg(nr+1), arg1_1);
            brw_pop_insn_state(p);
        }
        else
        {
            struct brw_reg src =  get_src_reg(c, inst, 1, 1);
            brw_MOV(p, brw_message_reg(nr), src);
        }
        nr += 2;
   }

    target = inst->Aux >> 1;
    eot = inst->Aux & 1;
    fire_fb_write(c, 0, nr, target, eot);
}

static void emit_pixel_w( struct brw_wm_compile *c,
                          const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    if (mask & WRITEMASK_W) {
	struct brw_reg dst, src0, delta0, delta1;
	struct brw_reg interp3;

	dst = get_dst_reg(c, inst, 3);
	src0 = get_src_reg(c, inst, 0, 0);
	delta0 = get_src_reg(c, inst, 1, 0);
	delta1 = get_src_reg(c, inst, 1, 1);

	interp3 = brw_vec1_grf(src0.nr+1, 4);
	/* Calc 1/w - just linterp wpos[3] optimized by putting the
	 * result straight into a message reg.
	 */
	brw_LINE(p, brw_null_reg(), interp3, delta0);
	brw_MAC(p, brw_message_reg(2), suboffset(interp3, 1), delta1);

	/* Calc w */
	brw_math_16( p, dst,
		BRW_MATH_FUNCTION_INV,
		BRW_MATH_SATURATE_NONE,
		2, brw_null_reg(),
		BRW_MATH_PRECISION_FULL);
    }
}

static void emit_linterp(struct brw_wm_compile *c,
                         const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg interp[4];
    struct brw_reg dst, delta0, delta1;
    struct brw_reg src0;
    GLuint nr, i;

    src0 = get_src_reg(c, inst, 0, 0);
    delta0 = get_src_reg(c, inst, 1, 0);
    delta1 = get_src_reg(c, inst, 1, 1);
    nr = src0.nr;

    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);

    for(i = 0; i < 4; i++ ) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_LINE(p, brw_null_reg(), interp[i], delta0);
	    brw_MAC(p, dst, suboffset(interp[i],1), delta1);
	}
    }
}

static void emit_cinterp(struct brw_wm_compile *c,
                         const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    struct brw_reg interp[4];
    struct brw_reg dst, src0;
    GLuint nr, i;

    src0 = get_src_reg(c, inst, 0, 0);
    nr = src0.nr;

    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);

    for(i = 0; i < 4; i++ ) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV(p, dst, suboffset(interp[i],3));
	}
    }
}

static void emit_pinterp(struct brw_wm_compile *c,
                         const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    struct brw_reg interp[4];
    struct brw_reg dst, delta0, delta1;
    struct brw_reg src0, w;
    GLuint nr, i;

    src0 = get_src_reg(c, inst, 0, 0);
    delta0 = get_src_reg(c, inst, 1, 0);
    delta1 = get_src_reg(c, inst, 1, 1);
    w = get_src_reg(c, inst, 2, 3);
    nr = src0.nr;

    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);

    for(i = 0; i < 4; i++ ) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_LINE(p, brw_null_reg(), interp[i], delta0);
	    brw_MAC(p, dst, suboffset(interp[i],1), 
		    delta1);
	    brw_MUL(p, dst, dst, w);
	}
    }
}

/* Sets the destination channels to 1.0 or 0.0 according to glFrontFacing. */
static void emit_frontfacing(struct brw_wm_compile *c,
			     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg r1_6ud = retype(brw_vec1_grf(1, 6), BRW_REGISTER_TYPE_UD);
    struct brw_reg dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;

    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV(p, dst, brw_imm_f(0.0));
	}
    }

    /* bit 31 is "primitive is back face", so checking < (1 << 31) gives
     * us front face
     */
    brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, r1_6ud, brw_imm_ud(1 << 31));
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV(p, dst, brw_imm_f(1.0));
	}
    }
    brw_set_predicate_control_flag_value(p, 0xff);
}

static void emit_xpd(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    for (i = 0; i < 4; i++) {
	GLuint i2 = (i+2)%3;
	GLuint i1 = (i+1)%3;
	if (mask & (1<<i)) {
	    struct brw_reg src0, src1, dst;
	    dst = get_dst_reg(c, inst, i);
	    src0 = negate(get_src_reg(c, inst, 0, i2));
	    src1 = get_src_reg_imm(c, inst, 1, i1);
	    brw_MUL(p, brw_null_reg(), src0, src1);
	    src0 = get_src_reg(c, inst, 0, i1);
	    src1 = get_src_reg_imm(c, inst, 1, i2);
	    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
	    brw_MAC(p, dst, src0, src1);
	    brw_set_saturate(p, 0);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_dp3(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_reg src0[3], src1[3], dst;
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    int dst_chan = ffs(mask & WRITEMASK_XYZW) - 1;

    if (!(mask & WRITEMASK_XYZW))
	return;

    assert(is_power_of_two(mask & WRITEMASK_XYZW));

    for (i = 0; i < 3; i++) {
	src0[i] = get_src_reg(c, inst, 0, i);
	src1[i] = get_src_reg_imm(c, inst, 1, i);
    }

    dst = get_dst_reg(c, inst, dst_chan);
    brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
    brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    brw_MAC(p, dst, src0[2], src1[2]);
    brw_set_saturate(p, 0);
}

static void emit_dp4(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_reg src0[4], src1[4], dst;
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    int dst_chan = ffs(mask & WRITEMASK_XYZW) - 1;

    if (!(mask & WRITEMASK_XYZW))
	return;

    assert(is_power_of_two(mask & WRITEMASK_XYZW));

    for (i = 0; i < 4; i++) {
	src0[i] = get_src_reg(c, inst, 0, i);
	src1[i] = get_src_reg_imm(c, inst, 1, i);
    }
    dst = get_dst_reg(c, inst, dst_chan);
    brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
    brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
    brw_MAC(p, brw_null_reg(), src0[2], src1[2]);
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    brw_MAC(p, dst, src0[3], src1[3]);
    brw_set_saturate(p, 0);
}

static void emit_dph(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_reg src0[4], src1[4], dst;
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    int dst_chan = ffs(mask & WRITEMASK_XYZW) - 1;

    if (!(mask & WRITEMASK_XYZW))
	return;

    assert(is_power_of_two(mask & WRITEMASK_XYZW));

    for (i = 0; i < 4; i++) {
	src0[i] = get_src_reg(c, inst, 0, i);
	src1[i] = get_src_reg_imm(c, inst, 1, i);
    }
    dst = get_dst_reg(c, inst, dst_chan);
    brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
    brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
    brw_MAC(p, dst, src0[2], src1[2]);
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    brw_ADD(p, dst, dst, src1[3]);
    brw_set_saturate(p, 0);
}

/**
 * Emit a scalar instruction, like RCP, RSQ, LOG, EXP.
 * Note that the result of the function is smeared across the dest
 * register's X, Y, Z and W channels (subject to writemasking of course).
 */
static void emit_math1(struct brw_wm_compile *c,
                       const struct brw_fp_instruction *inst, GLuint func)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int dst_chan = ffs(mask & WRITEMASK_XYZW) - 1;

    if (!(mask & WRITEMASK_XYZW))
	return;

    assert(is_power_of_two(mask & WRITEMASK_XYZW));

    /* Get first component of source register */
    dst = get_dst_reg(c, inst, dst_chan);
    src0 = get_src_reg(c, inst, 0, 0);

    brw_MOV(p, brw_message_reg(2), src0);
    brw_math(p,
             dst,
             func,
             (inst->SaturateMode != SATURATE_OFF) ? BRW_MATH_SATURATE_SATURATE : BRW_MATH_SATURATE_NONE,
             2,
             brw_null_reg(),
             BRW_MATH_DATA_VECTOR,
             BRW_MATH_PRECISION_FULL);
}

static void emit_rcp(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_INV);
}

static void emit_rsq(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_RSQ);
}

static void emit_sin(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_SIN);
}

static void emit_cos(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_COS);
}

static void emit_ex2(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_EXP);
}

static void emit_lg2(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_LOG);
}

static void emit_add(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg(c, inst, 0, i);
	    src1 = get_src_reg_imm(c, inst, 1, i);
	    brw_ADD(p, dst, src0, src1);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_arl(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
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


static void emit_mul(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg(c, inst, 0, i);
	    src1 = get_src_reg_imm(c, inst, 1, i);
	    brw_MUL(p, dst, src0, src1);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_frc(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg_imm(c, inst, 0, i);
	    brw_FRC(p, dst, src0);
	}
    }
    if (inst->SaturateMode != SATURATE_OFF)
	brw_set_saturate(p, 0);
}

static void emit_flr(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg_imm(c, inst, 0, i);
	    brw_RNDD(p, dst, src0);
	}
    }
    brw_set_saturate(p, 0);
}


static void emit_min_max(struct brw_wm_compile *c,
                         const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    const GLuint mask = inst->DstReg.WriteMask;
    const int mark = mark_tmps(c);
    int i;
    brw_push_insn_state(p);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
            struct brw_reg real_dst = get_dst_reg(c, inst, i);
	    struct brw_reg src0 = get_src_reg(c, inst, 0, i);
	    struct brw_reg src1 = get_src_reg(c, inst, 1, i);
            struct brw_reg dst;
            /* if dst==src0 or dst==src1 we need to use a temp reg */
            GLboolean use_temp = brw_same_reg(dst, src0) ||
                                 brw_same_reg(dst, src1);
            if (use_temp)
               dst = alloc_tmp(c);
            else
               dst = real_dst;

            /*
            printf("  Min/max: dst %d  src0 %d  src1 %d\n",
                   dst.nr, src0.nr, src1.nr);
            */
	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_MOV(p, dst, src0);
	    brw_set_saturate(p, 0);

            if (inst->Opcode == OPCODE_MIN)
               brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, src1, src0);
            else
               brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_G, src1, src0);

	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	    brw_MOV(p, dst, src1);
	    brw_set_saturate(p, 0);
	    brw_set_predicate_control_flag_value(p, 0xff);
            if (use_temp)
               brw_MOV(p, real_dst, dst);
	}
    }
    brw_pop_insn_state(p);
    release_tmps(c, mark);
}

static void emit_pow(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg dst, src0, src1;
    GLuint mask = inst->DstReg.WriteMask;
    int dst_chan = ffs(mask & WRITEMASK_XYZW) - 1;

    if (!(mask & WRITEMASK_XYZW))
	return;

    assert(is_power_of_two(mask & WRITEMASK_XYZW));

    dst = get_dst_reg(c, inst, dst_chan);
    src0 = get_src_reg_imm(c, inst, 0, 0);
    src1 = get_src_reg_imm(c, inst, 1, 0);

    brw_MOV(p, brw_message_reg(2), src0);
    brw_MOV(p, brw_message_reg(3), src1);

    brw_math(p,
	    dst,
	    BRW_MATH_FUNCTION_POW,
	    (inst->SaturateMode != SATURATE_OFF) ? BRW_MATH_SATURATE_SATURATE : BRW_MATH_SATURATE_NONE,
	    2,
	    brw_null_reg(),
	    BRW_MATH_DATA_VECTOR,
	    BRW_MATH_PRECISION_FULL);
}

static void emit_lrp(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg dst, tmp1, tmp2, src0, src1, src2;
    int i;
    int mark = mark_tmps(c);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg(c, inst, 0, i);

	    src1 = get_src_reg_imm(c, inst, 1, i);

	    if (src1.nr == dst.nr) {
		tmp1 = alloc_tmp(c);
		brw_MOV(p, tmp1, src1);
	    } else
		tmp1 = src1;

	    src2 = get_src_reg(c, inst, 2, i);
	    if (src2.nr == dst.nr) {
		tmp2 = alloc_tmp(c);
		brw_MOV(p, tmp2, src2);
	    } else
		tmp2 = src2;

	    brw_ADD(p, dst, negate(src0), brw_imm_f(1.0));
	    brw_MUL(p, brw_null_reg(), dst, tmp2);
	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_MAC(p, dst, src0, tmp1);
	    brw_set_saturate(p, 0);
	}
	release_tmps(c, mark);
    }
}

/**
 * For GLSL shaders, this KIL will be unconditional.
 * It may be contained inside an IF/ENDIF structure of course.
 */
static void emit_kil(struct brw_wm_compile *c)
{
    struct brw_compile *p = &c->func;
    struct brw_reg depth = retype(brw_vec1_grf(0, 0), BRW_REGISTER_TYPE_UW);
    brw_push_insn_state(p);
    brw_set_mask_control(p, BRW_MASK_DISABLE);
    brw_NOT(p, c->emit_mask_reg, brw_mask_reg(1)); //IMASK
    brw_AND(p, depth, c->emit_mask_reg, depth);
    brw_pop_insn_state(p);
}

static void emit_mad(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg dst, src0, src1, src2;
    int i;

    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg(c, inst, 0, i);
	    src1 = get_src_reg_imm(c, inst, 1, i);
	    src2 = get_src_reg_imm(c, inst, 2, i);
	    brw_MUL(p, dst, src0, src1);

	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_ADD(p, dst, dst, src2);
	    brw_set_saturate(p, 0);
	}
    }
}

static void emit_sop(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst, GLuint cond)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg dst, src0, src1;
    int i;

    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    src0 = get_src_reg(c, inst, 0, i);
	    src1 = get_src_reg_imm(c, inst, 1, i);
	    brw_push_insn_state(p);
	    brw_CMP(p, brw_null_reg(), cond, src0, src1);
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
	    brw_MOV(p, dst, brw_imm_f(0.0));
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	    brw_MOV(p, dst, brw_imm_f(1.0));
	    brw_pop_insn_state(p);
	}
    }
}

static void emit_slt(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_L);
}

static void emit_sle(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_LE);
}

static void emit_sgt(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_G);
}

static void emit_sge(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_GE);
}

static void emit_seq(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_EQ);
}

static void emit_sne(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_NEQ);
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


    
static void emit_wpos_xy(struct brw_wm_compile *c,
                         const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg src0[2], dst[2];

    dst[0] = get_dst_reg(c, inst, 0);
    dst[1] = get_dst_reg(c, inst, 1);

    src0[0] = get_src_reg(c, inst, 0, 0);
    src0[1] = get_src_reg(c, inst, 0, 1);

    /* Calculate the pixel offset from window bottom left into destination
     * X and Y channels.
     */
    if (mask & WRITEMASK_X) {
	/* X' = X */
	brw_MOV(p,
		dst[0],
		retype(src0[0], BRW_REGISTER_TYPE_W));
    }

    if (mask & WRITEMASK_Y) {
	/* Y' = height - 1 - Y */
	brw_ADD(p,
		dst[1],
		negate(retype(src0[1], BRW_REGISTER_TYPE_W)),
		brw_imm_d(c->key.drawable_height - 1));
    }
}

/* TODO
   BIAS on SIMD8 not working yet...
 */	
static void emit_txb(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg dst[4], src[4], payload_reg;
    /* Note: tex_unit was already looked up through SamplerTextures[] */
    const GLuint unit = inst->tex_unit;
    GLuint i;
    GLuint msg_type;

    assert(unit < BRW_MAX_TEX_UNIT);

    payload_reg = get_reg(c, TGSI_FILE_PAYLOAD, PAYLOAD_DEPTH, 0, 1, 0, 0);

    for (i = 0; i < 4; i++) 
	dst[i] = get_dst_reg(c, inst, i);
    for (i = 0; i < 4; i++)
	src[i] = get_src_reg(c, inst, 0, i);

    switch (inst->tex_target) {
	case TEXTURE_1D_INDEX:
	    brw_MOV(p, brw_message_reg(2), src[0]);         /* s coord */
	    brw_MOV(p, brw_message_reg(3), brw_imm_f(0));   /* t coord */
	    brw_MOV(p, brw_message_reg(4), brw_imm_f(0));   /* r coord */
	    break;
	case TEXTURE_2D_INDEX:
	case TEXTURE_RECT_INDEX:
	    brw_MOV(p, brw_message_reg(2), src[0]);
	    brw_MOV(p, brw_message_reg(3), src[1]);
	    brw_MOV(p, brw_message_reg(4), brw_imm_f(0));
	    break;
	case TEXTURE_3D_INDEX:
	case TEXTURE_CUBE_INDEX:
	    brw_MOV(p, brw_message_reg(2), src[0]);
	    brw_MOV(p, brw_message_reg(3), src[1]);
	    brw_MOV(p, brw_message_reg(4), src[2]);
	    break;
	default:
            /* invalid target */
            abort();
    }
    brw_MOV(p, brw_message_reg(5), src[3]);          /* bias */
    brw_MOV(p, brw_message_reg(6), brw_imm_f(0));    /* ref (unused?) */

    if (BRW_IS_IGDNG(p->brw)) {
        msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_BIAS_IGDNG;
    } else {
        /* Does it work well on SIMD8? */
        msg_type = BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS;
    }

    brw_SAMPLE(p,
               retype(vec8(dst[0]), BRW_REGISTER_TYPE_UW),  /* dest */
               1,                                           /* msg_reg_nr */
               retype(payload_reg, BRW_REGISTER_TYPE_UW),   /* src0 */
               SURF_INDEX_TEXTURE(unit),
               unit,                                        /* sampler */
               inst->DstReg.WriteMask,                      /* writemask */
               msg_type,                                    /* msg_type */
               4,                                           /* response_length */
               4,                                           /* msg_length */
               0,                                           /* eot */
               1,
               BRW_SAMPLER_SIMD_MODE_SIMD8);	
}


static void emit_tex(struct brw_wm_compile *c,
                     const struct brw_fp_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg dst[4], src[4], payload_reg;
    /* Note: tex_unit was already looked up through SamplerTextures[] */
    const GLuint unit = inst->tex_unit;
    GLuint msg_len;
    GLuint i, nr;
    GLuint emit;
    GLboolean shadow = (c->key.shadowtex_mask & (1<<unit)) ? 1 : 0;
    GLuint msg_type;

    assert(unit < BRW_MAX_TEX_UNIT);

    payload_reg = get_reg(c, TGSI_FILE_PAYLOAD, PAYLOAD_DEPTH, 0, 1, 0, 0);

    for (i = 0; i < 4; i++) 
	dst[i] = get_dst_reg(c, inst, i);
    for (i = 0; i < 4; i++)
	src[i] = get_src_reg(c, inst, 0, i);

    switch (inst->tex_target) {
	case TEXTURE_1D_INDEX:
	    emit = WRITEMASK_X;
	    nr = 1;
	    break;
	case TEXTURE_2D_INDEX:
	case TEXTURE_RECT_INDEX:
	    emit = WRITEMASK_XY;
	    nr = 2;
	    break;
	case TEXTURE_3D_INDEX:
	case TEXTURE_CUBE_INDEX:
	    emit = WRITEMASK_XYZ;
	    nr = 3;
	    break;
	default:
           /* invalid target */
           abort();
    }
    msg_len = 1;

    /* move/load S, T, R coords */
    for (i = 0; i < nr; i++) {
	static const GLuint swz[4] = {0,1,2,2};
	if (emit & (1<<i))
	    brw_MOV(p, brw_message_reg(msg_len+1), src[swz[i]]);
	else
	    brw_MOV(p, brw_message_reg(msg_len+1), brw_imm_f(0));
	msg_len += 1;
    }

    if (shadow) {
       brw_MOV(p, brw_message_reg(5), brw_imm_f(0));  /* lod / bias */
       brw_MOV(p, brw_message_reg(6), src[2]);        /* ref value / R coord */
    }

    if (BRW_IS_IGDNG(p->brw)) {
        if (shadow)
            msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_COMPARE_IGDNG;
        else
            msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE_IGDNG;
    } else {
        /* Does it work for shadow on SIMD8 ? */
        msg_type = BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE;
    }
    
    brw_SAMPLE(p,
               retype(vec8(dst[0]), BRW_REGISTER_TYPE_UW), /* dest */
               1,                                          /* msg_reg_nr */
               retype(payload_reg, BRW_REGISTER_TYPE_UW),  /* src0 */
               SURF_INDEX_TEXTURE(unit),
               unit,                                       /* sampler */
               inst->DstReg.WriteMask,                     /* writemask */
               msg_type,                                   /* msg_type */
               4,                                          /* response_length */
               shadow ? 6 : 4,                             /* msg_length */
               0,                                          /* eot */
               1,
               BRW_SAMPLER_SIMD_MODE_SIMD8);	

    if (shadow)
	brw_MOV(p, dst[3], brw_imm_f(1.0));
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
		  const struct brw_fp_instruction *inst,
		  int index,
		  struct brw_reg *regs,
		  int mask)
{
    int i;

    for (i = 0; i < 4; i++) {
	if (mask & (1 << i))
	    regs[i] = get_src_reg(c, inst, index, i);
    }
}

static void brw_wm_emit_branching_shader(struct brw_context *brw, struct brw_wm_compile *c)
{
#define MAX_IF_DEPTH 32
#define MAX_LOOP_DEPTH 32
    struct brw_instruction *if_inst[MAX_IF_DEPTH], *loop_inst[MAX_LOOP_DEPTH];
    GLuint i, if_depth = 0, loop_depth = 0;
    struct brw_compile *p = &c->func;
    struct brw_indirect stack_index = brw_indirect(0, 0);

    c->out_of_regs = GL_FALSE;

    prealloc_reg(c);
    brw_set_compression_control(p, BRW_COMPRESSION_NONE);
    brw_MOV(p, get_addr_reg(stack_index), brw_address(c->stack));

    for (i = 0; i < c->nr_fp_insns; i++) {
        const struct brw_fp_instruction *inst = &c->fp_instructions[i];
	int dst_flags;
	struct brw_reg args[3][4], dst[4];
	int j;

        c->cur_inst = i;

#if 0
        debug_printf("Inst %d: ", i);
        _mesa_print_instruction(inst);
#endif

        /* fetch any constants that this instruction needs */
        if (c->fp->use_const_buffer)
           fetch_constants(c, inst);

	if (inst->CondUpdate)
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
	else
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);

	dst_flags = inst->DstReg.WriteMask;
	if (inst->SaturateMode == SATURATE_ZERO_ONE)
	    dst_flags |= SATURATE;

	switch (inst->Opcode) {
	    case WM_PIXELXY:
		emit_pixel_xy(c, inst);
		break;
	    case WM_DELTAXY: 
		emit_delta_xy(c, inst);
		break;
	    case WM_PIXELW:
		emit_pixel_w(c, inst);
		break;	
	    case WM_LINTERP:
		emit_linterp(c, inst);
		break;
	    case WM_PINTERP:
		emit_pinterp(c, inst);
		break;
	    case WM_CINTERP:
		emit_cinterp(c, inst);
		break;
	    case WM_WPOSXY:
		emit_wpos_xy(c, inst);
		break;
	    case WM_FB_WRITE:
		emit_fb_write(c, inst);
		break;
	    case WM_FRONTFACING:
		emit_frontfacing(c, inst);
		break;
	    case OPCODE_ADD:
		emit_add(c, inst);
		break;
	    case OPCODE_ARL:
		emit_arl(c, inst);
		break;
	    case OPCODE_FRC:
		emit_frc(c, inst);
		break;
	    case OPCODE_FLR:
		emit_flr(c, inst);
		break;
	    case OPCODE_LRP:
		emit_lrp(c, inst);
		break;
	    case OPCODE_TRUNC:
		emit_trunc(c, inst);
		break;
	    case OPCODE_MOV:
		emit_mov(c, inst);
		break;
	    case OPCODE_DP3:
		emit_dp3(c, inst);
		break;
	    case OPCODE_DP4:
		emit_dp4(c, inst);
		break;
	    case OPCODE_XPD:
		emit_xpd(c, inst);
		break;
	    case OPCODE_DPH:
		emit_dph(c, inst);
		break;
	    case OPCODE_RCP:
		emit_rcp(c, inst);
		break;
	    case OPCODE_RSQ:
		emit_rsq(c, inst);
		break;
	    case OPCODE_SIN:
		emit_sin(c, inst);
		break;
	    case OPCODE_COS:
		emit_cos(c, inst);
		break;
	    case OPCODE_EX2:
		emit_ex2(c, inst);
		break;
	    case OPCODE_LG2:
		emit_lg2(c, inst);
		break;
	    case OPCODE_MIN:	
	    case OPCODE_MAX:	
		emit_min_max(c, inst);
		break;
	    case OPCODE_DDX:
	    case OPCODE_DDY:
		for (j = 0; j < 4; j++) {
		    if (inst->DstReg.WriteMask & (1 << j))
			dst[j] = get_dst_reg(c, inst, j);
		    else
			dst[j] = brw_null_reg();
		}
		get_argument_regs(c, inst, 0, args[0], WRITEMASK_XYZW);
		emit_ddxy(p, dst, dst_flags, (inst->Opcode == OPCODE_DDX),
			  args[0]);
                break;
	    case OPCODE_SLT:
		emit_slt(c, inst);
		break;
	    case OPCODE_SLE:
		emit_sle(c, inst);
		break;
	    case OPCODE_SGT:
		emit_sgt(c, inst);
		break;
	    case OPCODE_SGE:
		emit_sge(c, inst);
		break;
	    case OPCODE_SEQ:
		emit_seq(c, inst);
		break;
	    case OPCODE_SNE:
		emit_sne(c, inst);
		break;
	    case OPCODE_MUL:
		emit_mul(c, inst);
		break;
	    case OPCODE_POW:
		emit_pow(c, inst);
		break;
	    case OPCODE_MAD:
		emit_mad(c, inst);
		break;
	    case OPCODE_TEX:
		emit_tex(c, inst);
		break;
	    case OPCODE_TXB:
		emit_txb(c, inst);
		break;
	    case OPCODE_KIL_NV:
		emit_kil(c);
		break;
	    case OPCODE_IF:
		assert(if_depth < MAX_IF_DEPTH);
		if_inst[if_depth++] = brw_IF(p, BRW_EXECUTE_8);
		break;
	    case OPCODE_ELSE:
		if_inst[if_depth-1]  = brw_ELSE(p, if_inst[if_depth-1]);
		break;
	    case OPCODE_ENDIF:
		assert(if_depth > 0);
		brw_ENDIF(p, if_inst[--if_depth]);
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
		brw_save_call(&c->func, inst->label, p->nr_insn);
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
		break;
	    case OPCODE_BRK:
		brw_BREAK(p);
		brw_set_predicate_control(p, BRW_PREDICATE_NONE);
		break;
	    case OPCODE_CONT:
		brw_CONT(p);
		brw_set_predicate_control(p, BRW_PREDICATE_NONE);
		break;
	    case OPCODE_ENDLOOP: 
               {
                  struct brw_instruction *inst0, *inst1;
                  GLuint br = 1;

                  if (BRW_IS_IGDNG(brw))
                     br = 2;
 
                  loop_depth--;
                  inst0 = inst1 = brw_WHILE(p, loop_inst[loop_depth]);
                  /* patch all the BREAK/CONT instructions from last BGNLOOP */
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
	    default:
		debug_printf("unsupported IR in fragment shader %d\n",
			inst->Opcode);
	}

	if (inst->CondUpdate)
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	else
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
    }
    post_wm_emit(c);

    if (BRW_DEBUG & DEBUG_WM) {
      debug_printf("wm-native:\n");
      brw_disasm(stderr, p->store, p->nr_insn);
    }
}

/**
 * Do GPU code generation for shaders that use GLSL features such as
 * flow control.  Other shaders will be compiled with the 
 */
void brw_wm_branching_shader_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
    if (BRW_DEBUG & DEBUG_WM) {
       debug_printf("%s:\n", __FUNCTION__);
    }

    /* initial instruction translation/simplification */
    brw_wm_pass_fp(c);

    /* actual code generation */
    brw_wm_emit_branching_shader(brw, c);

    if (BRW_DEBUG & DEBUG_WM) {
        brw_wm_print_program(c, "brw_wm_branching_shader_emit done");
    }

    c->prog_data.total_grf = num_grf_used(c);
    c->prog_data.total_scratch = 0;
}
