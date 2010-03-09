#include "main/macros.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"
#include "shader/prog_optimize.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"

enum _subroutine {
    SUB_NOISE1, SUB_NOISE2, SUB_NOISE3, SUB_NOISE4
};

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

    for (i = 0; i < fp->Base.NumInstructions; i++) {
	const struct prog_instruction *inst = &fp->Base.Instructions[i];
	switch (inst->Opcode) {
	    case OPCODE_ARL:
	    case OPCODE_IF:
	    case OPCODE_ENDIF:
	    case OPCODE_CAL:
	    case OPCODE_BRK:
	    case OPCODE_RET:
	    case OPCODE_NOISE1:
	    case OPCODE_NOISE2:
	    case OPCODE_NOISE3:
	    case OPCODE_NOISE4:
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
	set_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH, i, reg);
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
                 set_reg(c, PROGRAM_STATE_VAR, i, j, reg);
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
	     set_reg(c, PROGRAM_PAYLOAD, fp_input, j, reg);
       }
       if (c->key.vp_outputs_written & BITFIELD64_BIT(i)) {
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

    /* Extended swizzle terms */
    if (component == SWIZZLE_ZERO) {
       return brw_imm_f(0.0F);
    }
    else if (component == SWIZZLE_ONE) {
       return brw_imm_f(1.0F);
    }

    if (c->fp->use_const_buffer &&
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
    brw_NOT(p, c->emit_mask_reg, brw_mask_reg(1)); /* IMASK */
    brw_AND(p, depth, c->emit_mask_reg, depth);
    brw_pop_insn_state(p);
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

/* One-, two- and three-dimensional Perlin noise, similar to the description
   in _Improving Noise_, Ken Perlin, Computer Graphics vol. 35 no. 3. */
static void noise1_sub( struct brw_wm_compile *c ) {

    struct brw_compile *p = &c->func;
    struct brw_reg param,
	x0, x1, /* gradients at each end */       
	t, tmp[ 2 ], /* float temporaries */
	itmp[ 5 ]; /* unsigned integer temporaries (aliases of floats above) */
    int i;
    int mark = mark_tmps( c );

    x0 = alloc_tmp( c );
    x1 = alloc_tmp( c );
    t = alloc_tmp( c );
    tmp[ 0 ] = alloc_tmp( c );
    tmp[ 1 ] = alloc_tmp( c );
    itmp[ 0 ] = retype( tmp[ 0 ], BRW_REGISTER_TYPE_UD );
    itmp[ 1 ] = retype( tmp[ 1 ], BRW_REGISTER_TYPE_UD );
    itmp[ 2 ] = retype( x0, BRW_REGISTER_TYPE_UD );
    itmp[ 3 ] = retype( x1, BRW_REGISTER_TYPE_UD );
    itmp[ 4 ] = retype( t, BRW_REGISTER_TYPE_UD );
    
    param = lookup_tmp( c, mark - 2 );

    brw_set_access_mode( p, BRW_ALIGN_1 );

    brw_MOV( p, itmp[ 2 ], brw_imm_ud( 0xBA97 ) ); /* constant used later */

    /* Arrange the two end coordinates into scalars (itmp0/itmp1) to
       be hashed.  Also compute the remainder (offset within the unit
       length), interleaved to reduce register dependency penalties. */
    brw_RNDD( p, retype( itmp[ 0 ], BRW_REGISTER_TYPE_D ), param );
    brw_FRC( p, param, param );
    brw_ADD( p, itmp[ 1 ], itmp[ 0 ], brw_imm_ud( 1 ) );
    brw_MOV( p, itmp[ 3 ], brw_imm_ud( 0x79D9 ) ); /* constant used later */
    brw_MOV( p, itmp[ 4 ], brw_imm_ud( 0xD5B1 ) ); /* constant used later */

    /* We're now ready to perform the hashing.  The two hashes are
       interleaved for performance.  The hash function used is
       designed to rapidly achieve avalanche and require only 32x16
       bit multiplication, and 16-bit swizzles (which we get for
       free).  We can't use immediate operands in the multiplies,
       because immediates are permitted only in src1 and the 16-bit
       factor is permitted only in src0. */
    for( i = 0; i < 2; i++ )
	brw_MUL( p, itmp[ i ], itmp[ 2 ], itmp[ i ] );
    for( i = 0; i < 2; i++ )
       brw_XOR( p, low_words( itmp[ i ] ), low_words( itmp[ i ] ),
		high_words( itmp[ i ] ) );
    for( i = 0; i < 2; i++ )
	brw_MUL( p, itmp[ i ], itmp[ 3 ], itmp[ i ] );
    for( i = 0; i < 2; i++ )
       brw_XOR( p, low_words( itmp[ i ] ), low_words( itmp[ i ] ),
		high_words( itmp[ i ] ) );
    for( i = 0; i < 2; i++ )
	brw_MUL( p, itmp[ i ], itmp[ 4 ], itmp[ i ] );
    for( i = 0; i < 2; i++ )
       brw_XOR( p, low_words( itmp[ i ] ), low_words( itmp[ i ] ),
		high_words( itmp[ i ] ) );

    /* Now we want to initialise the two gradients based on the
       hashes.  Format conversion from signed integer to float leaves
       everything scaled too high by a factor of pow( 2, 31 ), but
       we correct for that right at the end. */
    brw_ADD( p, t, param, brw_imm_f( -1.0 ) );
    brw_MOV( p, x0, retype( tmp[ 0 ], BRW_REGISTER_TYPE_D ) );
    brw_MOV( p, x1, retype( tmp[ 1 ], BRW_REGISTER_TYPE_D ) );

    brw_MUL( p, x0, x0, param );
    brw_MUL( p, x1, x1, t );
    
    /* We interpolate between the gradients using the polynomial
       6t^5 - 15t^4 + 10t^3 (Perlin). */
    brw_MUL( p, tmp[ 0 ], param, brw_imm_f( 6.0 ) );
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], brw_imm_f( -15.0 ) );
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param );
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], brw_imm_f( 10.0 ) );
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param );
    brw_ADD( p, x1, x1, negate( x0 ) ); /* unrelated work to fill the
					   pipeline */
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param );
    brw_MUL( p, param, tmp[ 0 ], param );
    brw_MUL( p, x1, x1, param );
    brw_ADD( p, x0, x0, x1 );    
    /* scale by pow( 2, -30 ), to compensate for the format conversion
       above and an extra factor of 2 so that a single gradient covers
       the [-1,1] range */
    brw_MUL( p, param, x0, brw_imm_f( 0.000000000931322574615478515625 ) );

    release_tmps( c, mark );
}

static void emit_noise1( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
    struct brw_compile *p = &c->func;
    struct brw_reg src, param, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    int mark = mark_tmps( c );

    assert( mark == 0 );
    
    src = get_src_reg( c, inst, 0, 0 );

    param = alloc_tmp( c );

    brw_MOV( p, param, src );

    invoke_subroutine( c, SUB_NOISE1, noise1_sub );
    
    /* Fill in the result: */
    brw_set_saturate( p, inst->SaturateMode == SATURATE_ZERO_ONE );
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV( p, dst, param );
	}
    }
    if( inst->SaturateMode == SATURATE_ZERO_ONE )
	brw_set_saturate( p, 0 );
    
    release_tmps( c, mark );
}
    
static void noise2_sub( struct brw_wm_compile *c ) {

    struct brw_compile *p = &c->func;
    struct brw_reg param0, param1,
	x0y0, x0y1, x1y0, x1y1, /* gradients at each corner */       
	t, tmp[ 4 ], /* float temporaries */
	itmp[ 7 ]; /* unsigned integer temporaries (aliases of floats above) */
    int i;
    int mark = mark_tmps( c );

    x0y0 = alloc_tmp( c );
    x0y1 = alloc_tmp( c );
    x1y0 = alloc_tmp( c );
    x1y1 = alloc_tmp( c );
    t = alloc_tmp( c );
    for( i = 0; i < 4; i++ ) {
	tmp[ i ] = alloc_tmp( c );
	itmp[ i ] = retype( tmp[ i ], BRW_REGISTER_TYPE_UD );
    }
    itmp[ 4 ] = retype( x0y0, BRW_REGISTER_TYPE_UD );
    itmp[ 5 ] = retype( x0y1, BRW_REGISTER_TYPE_UD );
    itmp[ 6 ] = retype( x1y0, BRW_REGISTER_TYPE_UD );
    
    param0 = lookup_tmp( c, mark - 3 );
    param1 = lookup_tmp( c, mark - 2 );

    brw_set_access_mode( p, BRW_ALIGN_1 );
    
    /* Arrange the four corner coordinates into scalars (itmp0..itmp3) to
       be hashed.  Also compute the remainders (offsets within the unit
       square), interleaved to reduce register dependency penalties. */
    brw_RNDD( p, retype( itmp[ 0 ], BRW_REGISTER_TYPE_D ), param0 );
    brw_RNDD( p, retype( itmp[ 1 ], BRW_REGISTER_TYPE_D ), param1 );
    brw_FRC( p, param0, param0 );
    brw_FRC( p, param1, param1 );
    brw_MOV( p, itmp[ 4 ], brw_imm_ud( 0xBA97 ) ); /* constant used later */
    brw_ADD( p, high_words( itmp[ 0 ] ), high_words( itmp[ 0 ] ),
	     low_words( itmp[ 1 ] ) );
    brw_MOV( p, itmp[ 5 ], brw_imm_ud( 0x79D9 ) ); /* constant used later */
    brw_MOV( p, itmp[ 6 ], brw_imm_ud( 0xD5B1 ) ); /* constant used later */
    brw_ADD( p, itmp[ 1 ], itmp[ 0 ], brw_imm_ud( 0x10000 ) );
    brw_ADD( p, itmp[ 2 ], itmp[ 0 ], brw_imm_ud( 0x1 ) );
    brw_ADD( p, itmp[ 3 ], itmp[ 0 ], brw_imm_ud( 0x10001 ) );

    /* We're now ready to perform the hashing.  The four hashes are
       interleaved for performance.  The hash function used is
       designed to rapidly achieve avalanche and require only 32x16
       bit multiplication, and 16-bit swizzles (which we get for
       free).  We can't use immediate operands in the multiplies,
       because immediates are permitted only in src1 and the 16-bit
       factor is permitted only in src0. */
    for( i = 0; i < 4; i++ )
	brw_MUL( p, itmp[ i ], itmp[ 4 ], itmp[ i ] );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, low_words( itmp[ i ] ), low_words( itmp[ i ] ),
		 high_words( itmp[ i ] ) );
    for( i = 0; i < 4; i++ )
	brw_MUL( p, itmp[ i ], itmp[ 5 ], itmp[ i ] );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, low_words( itmp[ i ] ), low_words( itmp[ i ] ),
		 high_words( itmp[ i ] ) );
    for( i = 0; i < 4; i++ )
	brw_MUL( p, itmp[ i ], itmp[ 6 ], itmp[ i ] );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, low_words( itmp[ i ] ), low_words( itmp[ i ] ),
		 high_words( itmp[ i ] ) );

    /* Now we want to initialise the four gradients based on the
       hashes.  Format conversion from signed integer to float leaves
       everything scaled too high by a factor of pow( 2, 15 ), but
       we correct for that right at the end. */
    brw_ADD( p, t, param0, brw_imm_f( -1.0 ) );
    brw_MOV( p, x0y0, low_words( tmp[ 0 ] ) );
    brw_MOV( p, x0y1, low_words( tmp[ 1 ] ) );
    brw_MOV( p, x1y0, low_words( tmp[ 2 ] ) );
    brw_MOV( p, x1y1, low_words( tmp[ 3 ] ) );
    
    brw_MOV( p, tmp[ 0 ], high_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 1 ], high_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 2 ], high_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 3 ], high_words( tmp[ 3 ] ) );
    
    brw_MUL( p, x1y0, x1y0, t );
    brw_MUL( p, x1y1, x1y1, t );
    brw_ADD( p, t, param1, brw_imm_f( -1.0 ) );
    brw_MUL( p, x0y0, x0y0, param0 );
    brw_MUL( p, x0y1, x0y1, param0 );

    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param1 );
    brw_MUL( p, tmp[ 2 ], tmp[ 2 ], param1 );
    brw_MUL( p, tmp[ 1 ], tmp[ 1 ], t );
    brw_MUL( p, tmp[ 3 ], tmp[ 3 ], t );

    brw_ADD( p, x0y0, x0y0, tmp[ 0 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 2 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 1 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 3 ] );
    
    /* We interpolate between the gradients using the polynomial
       6t^5 - 15t^4 + 10t^3 (Perlin). */
    brw_MUL( p, tmp[ 0 ], param0, brw_imm_f( 6.0 ) );
    brw_MUL( p, tmp[ 1 ], param1, brw_imm_f( 6.0 ) );
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], brw_imm_f( -15.0 ) );
    brw_ADD( p, tmp[ 1 ], tmp[ 1 ], brw_imm_f( -15.0 ) );
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param0 );
    brw_MUL( p, tmp[ 1 ], tmp[ 1 ], param1 );
    brw_ADD( p, x0y1, x0y1, negate( x0y0 ) ); /* unrelated work to fill the
						 pipeline */
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], brw_imm_f( 10.0 ) );
    brw_ADD( p, tmp[ 1 ], tmp[ 1 ], brw_imm_f( 10.0 ) );
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param0 );
    brw_MUL( p, tmp[ 1 ], tmp[ 1 ], param1 );
    brw_ADD( p, x1y1, x1y1, negate( x1y0 ) ); /* unrelated work to fill the
						 pipeline */
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], param0 );
    brw_MUL( p, tmp[ 1 ], tmp[ 1 ], param1 );
    brw_MUL( p, param0, tmp[ 0 ], param0 );
    brw_MUL( p, param1, tmp[ 1 ], param1 );
    
    /* Here we interpolate in the y dimension... */
    brw_MUL( p, x0y1, x0y1, param1 );
    brw_MUL( p, x1y1, x1y1, param1 );
    brw_ADD( p, x0y0, x0y0, x0y1 );
    brw_ADD( p, x1y0, x1y0, x1y1 );

    /* And now in x.  There are horrible register dependencies here,
       but we have nothing else to do. */
    brw_ADD( p, x1y0, x1y0, negate( x0y0 ) );
    brw_MUL( p, x1y0, x1y0, param0 );
    brw_ADD( p, x0y0, x0y0, x1y0 );
    
    /* scale by pow( 2, -15 ), as described above */
    brw_MUL( p, param0, x0y0, brw_imm_f( 0.000030517578125 ) );

    release_tmps( c, mark );
}

static void emit_noise2( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, param0, param1, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    int mark = mark_tmps( c );

    assert( mark == 0 );
    
    src0 = get_src_reg( c, inst, 0, 0 );
    src1 = get_src_reg( c, inst, 0, 1 );

    param0 = alloc_tmp( c );
    param1 = alloc_tmp( c );

    brw_MOV( p, param0, src0 );
    brw_MOV( p, param1, src1 );

    invoke_subroutine( c, SUB_NOISE2, noise2_sub );
    
    /* Fill in the result: */
    brw_set_saturate( p, inst->SaturateMode == SATURATE_ZERO_ONE );
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV( p, dst, param0 );
	}
    }
    if( inst->SaturateMode == SATURATE_ZERO_ONE )
	brw_set_saturate( p, 0 );
    
    release_tmps( c, mark );
}

/**
 * The three-dimensional case is much like the one- and two- versions above,
 * but since the number of corners is rapidly growing we now pack 16 16-bit
 * hashes into each register to extract more parallelism from the EUs.
 */
static void noise3_sub( struct brw_wm_compile *c ) {

    struct brw_compile *p = &c->func;
    struct brw_reg param0, param1, param2,
	x0y0, x0y1, x1y0, x1y1, /* gradients at four of the corners */
	xi, yi, zi, /* interpolation coefficients */
	t, tmp[ 8 ], /* float temporaries */
	itmp[ 8 ], /* unsigned integer temporaries (aliases of floats above) */
	wtmp[ 8 ]; /* 16-way unsigned word temporaries (aliases of above) */
    int i;
    int mark = mark_tmps( c );

    x0y0 = alloc_tmp( c );
    x0y1 = alloc_tmp( c );
    x1y0 = alloc_tmp( c );
    x1y1 = alloc_tmp( c );
    xi = alloc_tmp( c );
    yi = alloc_tmp( c );
    zi = alloc_tmp( c );
    t = alloc_tmp( c );
    for( i = 0; i < 8; i++ ) {
	tmp[ i ] = alloc_tmp( c );
	itmp[ i ] = retype( tmp[ i ], BRW_REGISTER_TYPE_UD );
	wtmp[ i ] = brw_uw16_grf( tmp[ i ].nr, 0 );
    }
    
    param0 = lookup_tmp( c, mark - 4 );
    param1 = lookup_tmp( c, mark - 3 );
    param2 = lookup_tmp( c, mark - 2 );

    brw_set_access_mode( p, BRW_ALIGN_1 );
    
    /* Arrange the eight corner coordinates into scalars (itmp0..itmp3) to
       be hashed.  Also compute the remainders (offsets within the unit
       cube), interleaved to reduce register dependency penalties. */
    brw_RNDD( p, retype( itmp[ 0 ], BRW_REGISTER_TYPE_D ), param0 );
    brw_RNDD( p, retype( itmp[ 1 ], BRW_REGISTER_TYPE_D ), param1 );
    brw_RNDD( p, retype( itmp[ 2 ], BRW_REGISTER_TYPE_D ), param2 );
    brw_FRC( p, param0, param0 );
    brw_FRC( p, param1, param1 );
    brw_FRC( p, param2, param2 );
    /* Since we now have only 16 bits of precision in the hash, we must
       be more careful about thorough mixing to maintain entropy as we
       squash the input vector into a small scalar. */
    brw_MUL( p, brw_null_reg(), low_words( itmp[ 0 ] ), brw_imm_uw( 0xBC8F ) );
    brw_MAC( p, brw_null_reg(), low_words( itmp[ 1 ] ), brw_imm_uw( 0xD0BD ) );
    brw_MAC( p, low_words( itmp[ 0 ] ), low_words( itmp[ 2 ] ),
	     brw_imm_uw( 0x9B93 ) );
    brw_ADD( p, high_words( itmp[ 0 ] ), low_words( itmp[ 0 ] ),
	     brw_imm_uw( 0xBC8F ) );

    /* Temporarily disable the execution mask while we work with ExecSize=16
       channels (the mask is set for ExecSize=8 and is probably incorrect).
       Although this might cause execution of unwanted channels, the code
       writes only to temporary registers and has no side effects, so
       disabling the mask is harmless. */
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_ADD( p, wtmp[ 1 ], wtmp[ 0 ], brw_imm_uw( 0xD0BD ) );
    brw_ADD( p, wtmp[ 2 ], wtmp[ 0 ], brw_imm_uw( 0x9B93 ) );
    brw_ADD( p, wtmp[ 3 ], wtmp[ 1 ], brw_imm_uw( 0x9B93 ) );

    /* We're now ready to perform the hashing.  The eight hashes are
       interleaved for performance.  The hash function used is
       designed to rapidly achieve avalanche and require only 16x16
       bit multiplication, and 8-bit swizzles (which we get for
       free). */
    for( i = 0; i < 4; i++ )
	brw_MUL( p, wtmp[ i ], wtmp[ i ], brw_imm_uw( 0x28D9 ) );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, even_bytes( wtmp[ i ] ), even_bytes( wtmp[ i ] ),
		 odd_bytes( wtmp[ i ] ) );
    for( i = 0; i < 4; i++ )
	brw_MUL( p, wtmp[ i ], wtmp[ i ], brw_imm_uw( 0xC6D5 ) );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, even_bytes( wtmp[ i ] ), even_bytes( wtmp[ i ] ),
		 odd_bytes( wtmp[ i ] ) );
    brw_pop_insn_state( p );

    /* Now we want to initialise the four rear gradients based on the
       hashes.  Format conversion from signed integer to float leaves
       everything scaled too high by a factor of pow( 2, 15 ), but
       we correct for that right at the end. */
    /* x component */
    brw_ADD( p, t, param0, brw_imm_f( -1.0 ) );
    brw_MOV( p, x0y0, low_words( tmp[ 0 ] ) );
    brw_MOV( p, x0y1, low_words( tmp[ 1 ] ) );
    brw_MOV( p, x1y0, high_words( tmp[ 0 ] ) );
    brw_MOV( p, x1y1, high_words( tmp[ 1 ] ) );

    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 0 ], wtmp[ 0 ], brw_imm_uw( 5 ) );
    brw_SHL( p, wtmp[ 1 ], wtmp[ 1 ], brw_imm_uw( 5 ) );
    brw_pop_insn_state( p );
    
    brw_MUL( p, x1y0, x1y0, t );
    brw_MUL( p, x1y1, x1y1, t );
    brw_ADD( p, t, param1, brw_imm_f( -1.0 ) );
    brw_MUL( p, x0y0, x0y0, param0 );
    brw_MUL( p, x0y1, x0y1, param0 );

    /* y component */
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 0 ] ) );
    
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 0 ], wtmp[ 0 ], brw_imm_uw( 5 ) );
    brw_SHL( p, wtmp[ 1 ], wtmp[ 1 ], brw_imm_uw( 5 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    brw_ADD( p, t, param0, brw_imm_f( -1.0 ) );
    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], param1 );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], param1 );
    
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    
    /* z component */
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 1 ] ) );

    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], param2 );
    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], param2 );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], param2 );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], param2 );
    
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );
    
    /* We interpolate between the gradients using the polynomial
       6t^5 - 15t^4 + 10t^3 (Perlin). */
    brw_MUL( p, xi, param0, brw_imm_f( 6.0 ) );
    brw_MUL( p, yi, param1, brw_imm_f( 6.0 ) );
    brw_MUL( p, zi, param2, brw_imm_f( 6.0 ) );
    brw_ADD( p, xi, xi, brw_imm_f( -15.0 ) );
    brw_ADD( p, yi, yi, brw_imm_f( -15.0 ) );
    brw_ADD( p, zi, zi, brw_imm_f( -15.0 ) );
    brw_MUL( p, xi, xi, param0 );
    brw_MUL( p, yi, yi, param1 );
    brw_MUL( p, zi, zi, param2 );
    brw_ADD( p, xi, xi, brw_imm_f( 10.0 ) );
    brw_ADD( p, yi, yi, brw_imm_f( 10.0 ) );
    brw_ADD( p, zi, zi, brw_imm_f( 10.0 ) );
    brw_ADD( p, x0y1, x0y1, negate( x0y0 ) ); /* unrelated work */
    brw_ADD( p, x1y1, x1y1, negate( x1y0 ) ); /* unrelated work */
    brw_MUL( p, xi, xi, param0 );
    brw_MUL( p, yi, yi, param1 );
    brw_MUL( p, zi, zi, param2 );
    brw_MUL( p, xi, xi, param0 );
    brw_MUL( p, yi, yi, param1 );
    brw_MUL( p, zi, zi, param2 );
    brw_MUL( p, xi, xi, param0 );
    brw_MUL( p, yi, yi, param1 );
    brw_MUL( p, zi, zi, param2 );
    
    /* Here we interpolate in the y dimension... */
    brw_MUL( p, x0y1, x0y1, yi );
    brw_MUL( p, x1y1, x1y1, yi );
    brw_ADD( p, x0y0, x0y0, x0y1 );
    brw_ADD( p, x1y0, x1y0, x1y1 );

    /* And now in x.  Leave the result in tmp[ 0 ] (see below)... */
    brw_ADD( p, x1y0, x1y0, negate( x0y0 ) );
    brw_MUL( p, x1y0, x1y0, xi );
    brw_ADD( p, tmp[ 0 ], x0y0, x1y0 );

    /* Now do the same thing for the front four gradients... */
    /* x component */
    brw_MOV( p, x0y0, low_words( tmp[ 2 ] ) );
    brw_MOV( p, x0y1, low_words( tmp[ 3 ] ) );
    brw_MOV( p, x1y0, high_words( tmp[ 2 ] ) );
    brw_MOV( p, x1y1, high_words( tmp[ 3 ] ) );

    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 2 ], wtmp[ 2 ], brw_imm_uw( 5 ) );
    brw_SHL( p, wtmp[ 3 ], wtmp[ 3 ], brw_imm_uw( 5 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, x1y0, x1y0, t );
    brw_MUL( p, x1y1, x1y1, t );
    brw_ADD( p, t, param1, brw_imm_f( -1.0 ) );
    brw_MUL( p, x0y0, x0y0, param0 );
    brw_MUL( p, x0y1, x0y1, param0 );

    /* y component */
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 2 ] ) );
    
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 2 ], wtmp[ 2 ], brw_imm_uw( 5 ) );
    brw_SHL( p, wtmp[ 3 ], wtmp[ 3 ], brw_imm_uw( 5 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    brw_ADD( p, t, param2, brw_imm_f( -1.0 ) );
    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], param1 );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], param1 );
    
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    
    /* z component */
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 3 ] ) );

    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], t );
    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );
    
    /* The interpolation coefficients are still around from last time, so
       again interpolate in the y dimension... */
    brw_ADD( p, x0y1, x0y1, negate( x0y0 ) );
    brw_ADD( p, x1y1, x1y1, negate( x1y0 ) );
    brw_MUL( p, x0y1, x0y1, yi );
    brw_MUL( p, x1y1, x1y1, yi );
    brw_ADD( p, x0y0, x0y0, x0y1 );
    brw_ADD( p, x1y0, x1y0, x1y1 );

    /* And now in x.  The rear face is in tmp[ 0 ] (see above), so this
       time put the front face in tmp[ 1 ] and we're nearly there... */
    brw_ADD( p, x1y0, x1y0, negate( x0y0 ) );
    brw_MUL( p, x1y0, x1y0, xi );
    brw_ADD( p, tmp[ 1 ], x0y0, x1y0 );

    /* The final interpolation, in the z dimension: */
    brw_ADD( p, tmp[ 1 ], tmp[ 1 ], negate( tmp[ 0 ] ) );    
    brw_MUL( p, tmp[ 1 ], tmp[ 1 ], zi );
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], tmp[ 1 ] );
    
    /* scale by pow( 2, -15 ), as described above */
    brw_MUL( p, param0, tmp[ 0 ], brw_imm_f( 0.000030517578125 ) );

    release_tmps( c, mark );
}

static void emit_noise3( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, src2, param0, param1, param2, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    int mark = mark_tmps( c );

    assert( mark == 0 );
    
    src0 = get_src_reg( c, inst, 0, 0 );
    src1 = get_src_reg( c, inst, 0, 1 );
    src2 = get_src_reg( c, inst, 0, 2 );

    param0 = alloc_tmp( c );
    param1 = alloc_tmp( c );
    param2 = alloc_tmp( c );

    brw_MOV( p, param0, src0 );
    brw_MOV( p, param1, src1 );
    brw_MOV( p, param2, src2 );

    invoke_subroutine( c, SUB_NOISE3, noise3_sub );
    
    /* Fill in the result: */
    brw_set_saturate( p, inst->SaturateMode == SATURATE_ZERO_ONE );
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV( p, dst, param0 );
	}
    }
    if( inst->SaturateMode == SATURATE_ZERO_ONE )
	brw_set_saturate( p, 0 );
    
    release_tmps( c, mark );
}
    
/**
 * For the four-dimensional case, the little micro-optimisation benefits
 * we obtain by unrolling all the loops aren't worth the massive bloat it
 * now causes.  Instead, we loop twice around performing a similar operation
 * to noise3, once for the w=0 cube and once for the w=1, with a bit more
 * code to glue it all together.
 */
static void noise4_sub( struct brw_wm_compile *c )
{
    struct brw_compile *p = &c->func;
    struct brw_reg param[ 4 ],
	x0y0, x0y1, x1y0, x1y1, /* gradients at four of the corners */
	w0, /* noise for the w=0 cube */
	floors[ 2 ], /* integer coordinates of base corner of hypercube */
	interp[ 4 ], /* interpolation coefficients */
	t, tmp[ 8 ], /* float temporaries */
	itmp[ 8 ], /* unsigned integer temporaries (aliases of floats above) */
	wtmp[ 8 ]; /* 16-way unsigned word temporaries (aliases of above) */
    int i, j;
    int mark = mark_tmps( c );
    GLuint loop, origin;
    
    x0y0 = alloc_tmp( c );
    x0y1 = alloc_tmp( c );
    x1y0 = alloc_tmp( c );
    x1y1 = alloc_tmp( c );
    t = alloc_tmp( c );
    w0 = alloc_tmp( c );    
    floors[ 0 ] = retype( alloc_tmp( c ), BRW_REGISTER_TYPE_UD );
    floors[ 1 ] = retype( alloc_tmp( c ), BRW_REGISTER_TYPE_UD );

    for( i = 0; i < 4; i++ ) {
	param[ i ] = lookup_tmp( c, mark - 5 + i );
	interp[ i ] = alloc_tmp( c );
    }
    
    for( i = 0; i < 8; i++ ) {
	tmp[ i ] = alloc_tmp( c );
	itmp[ i ] = retype( tmp[ i ], BRW_REGISTER_TYPE_UD );
	wtmp[ i ] = brw_uw16_grf( tmp[ i ].nr, 0 );
    }

    brw_set_access_mode( p, BRW_ALIGN_1 );

    /* We only want 16 bits of precision from the integral part of each
       co-ordinate, but unfortunately the RNDD semantics would saturate
       at 16 bits if we performed the operation directly to a 16-bit
       destination.  Therefore, we round to 32-bit temporaries where
       appropriate, and then store only the lower 16 bits. */
    brw_RNDD( p, retype( floors[ 0 ], BRW_REGISTER_TYPE_D ), param[ 0 ] );
    brw_RNDD( p, retype( itmp[ 0 ], BRW_REGISTER_TYPE_D ), param[ 1 ] );
    brw_RNDD( p, retype( floors[ 1 ], BRW_REGISTER_TYPE_D ), param[ 2 ] );
    brw_RNDD( p, retype( itmp[ 1 ], BRW_REGISTER_TYPE_D ), param[ 3 ] );
    brw_MOV( p, high_words( floors[ 0 ] ), low_words( itmp[ 0 ] ) );
    brw_MOV( p, high_words( floors[ 1 ] ), low_words( itmp[ 1 ] ) );

    /* Modify the flag register here, because the side effect is useful
       later (see below).  We know for certain that all flags will be
       cleared, since the FRC instruction cannot possibly generate
       negative results.  Even for exceptional inputs (infinities, denormals,
       NaNs), the architecture guarantees that the L conditional is false. */
    brw_set_conditionalmod( p, BRW_CONDITIONAL_L );
    brw_FRC( p, param[ 0 ], param[ 0 ] );
    brw_set_predicate_control( p, BRW_PREDICATE_NONE );
    for( i = 1; i < 4; i++ )	
	brw_FRC( p, param[ i ], param[ i ] );
    
    /* Calculate the interpolation coefficients (6t^5 - 15t^4 + 10t^3) first
       of all. */
    for( i = 0; i < 4; i++ )
	brw_MUL( p, interp[ i ], param[ i ], brw_imm_f( 6.0 ) );
    for( i = 0; i < 4; i++ )
	brw_ADD( p, interp[ i ], interp[ i ], brw_imm_f( -15.0 ) );
    for( i = 0; i < 4; i++ )
	brw_MUL( p, interp[ i ], interp[ i ], param[ i ] );
    for( i = 0; i < 4; i++ )
	brw_ADD( p, interp[ i ], interp[ i ], brw_imm_f( 10.0 ) );
    for( j = 0; j < 3; j++ )
	for( i = 0; i < 4; i++ )
	    brw_MUL( p, interp[ i ], interp[ i ], param[ i ] );

    /* Mark the current address, as it will be a jump destination.  The
       following code will be executed twice: first, with the flag
       register clear indicating the w=0 case, and second with flags
       set for w=1. */
    loop = p->nr_insn;
    
    /* Arrange the eight corner coordinates into scalars (itmp0..itmp3) to
       be hashed.  Since we have only 16 bits of precision in the hash, we
       must be careful about thorough mixing to maintain entropy as we
       squash the input vector into a small scalar. */
    brw_MUL( p, brw_null_reg(), low_words( floors[ 0 ] ),
	     brw_imm_uw( 0xBC8F ) );
    brw_MAC( p, brw_null_reg(), high_words( floors[ 0 ] ),
	     brw_imm_uw( 0xD0BD ) );
    brw_MAC( p, brw_null_reg(), low_words( floors[ 1 ] ),
	     brw_imm_uw( 0x9B93 ) );
    brw_MAC( p, low_words( itmp[ 0 ] ), high_words( floors[ 1 ] ),
	     brw_imm_uw( 0xA359 ) );
    brw_ADD( p, high_words( itmp[ 0 ] ), low_words( itmp[ 0 ] ),
	     brw_imm_uw( 0xBC8F ) );

    /* Temporarily disable the execution mask while we work with ExecSize=16
       channels (the mask is set for ExecSize=8 and is probably incorrect).
       Although this might cause execution of unwanted channels, the code
       writes only to temporary registers and has no side effects, so
       disabling the mask is harmless. */
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_ADD( p, wtmp[ 1 ], wtmp[ 0 ], brw_imm_uw( 0xD0BD ) );
    brw_ADD( p, wtmp[ 2 ], wtmp[ 0 ], brw_imm_uw( 0x9B93 ) );
    brw_ADD( p, wtmp[ 3 ], wtmp[ 1 ], brw_imm_uw( 0x9B93 ) );

    /* We're now ready to perform the hashing.  The eight hashes are
       interleaved for performance.  The hash function used is
       designed to rapidly achieve avalanche and require only 16x16
       bit multiplication, and 8-bit swizzles (which we get for
       free). */
    for( i = 0; i < 4; i++ )
	brw_MUL( p, wtmp[ i ], wtmp[ i ], brw_imm_uw( 0x28D9 ) );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, even_bytes( wtmp[ i ] ), even_bytes( wtmp[ i ] ),
		 odd_bytes( wtmp[ i ] ) );
    for( i = 0; i < 4; i++ )
	brw_MUL( p, wtmp[ i ], wtmp[ i ], brw_imm_uw( 0xC6D5 ) );
    for( i = 0; i < 4; i++ )
	brw_XOR( p, even_bytes( wtmp[ i ] ), even_bytes( wtmp[ i ] ),
		 odd_bytes( wtmp[ i ] ) );
    brw_pop_insn_state( p );

    /* Now we want to initialise the four rear gradients based on the
       hashes.  Format conversion from signed integer to float leaves
       everything scaled too high by a factor of pow( 2, 15 ), but
       we correct for that right at the end. */
    /* x component */
    brw_ADD( p, t, param[ 0 ], brw_imm_f( -1.0 ) );
    brw_MOV( p, x0y0, low_words( tmp[ 0 ] ) );
    brw_MOV( p, x0y1, low_words( tmp[ 1 ] ) );
    brw_MOV( p, x1y0, high_words( tmp[ 0 ] ) );
    brw_MOV( p, x1y1, high_words( tmp[ 1 ] ) );

    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 0 ], wtmp[ 0 ], brw_imm_uw( 4 ) );
    brw_SHL( p, wtmp[ 1 ], wtmp[ 1 ], brw_imm_uw( 4 ) );
    brw_pop_insn_state( p );
    
    brw_MUL( p, x1y0, x1y0, t );
    brw_MUL( p, x1y1, x1y1, t );
    brw_ADD( p, t, param[ 1 ], brw_imm_f( -1.0 ) );
    brw_MUL( p, x0y0, x0y0, param[ 0 ] );
    brw_MUL( p, x0y1, x0y1, param[ 0 ] );

    /* y component */
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 0 ] ) );
    
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 0 ], wtmp[ 0 ], brw_imm_uw( 4 ) );
    brw_SHL( p, wtmp[ 1 ], wtmp[ 1 ], brw_imm_uw( 4 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );    
    /* prepare t for the w component (used below): w the first time through
       the loop; w - 1 the second time) */
    brw_set_predicate_control( p, BRW_PREDICATE_NORMAL );
    brw_ADD( p, t, param[ 3 ], brw_imm_f( -1.0 ) );
    p->current->header.predicate_inverse = 1;
    brw_MOV( p, t, param[ 3 ] );
    p->current->header.predicate_inverse = 0;
    brw_set_predicate_control( p, BRW_PREDICATE_NONE );
    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], param[ 1 ] );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], param[ 1 ] );
    
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    
    /* z component */
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 1 ] ) );

    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 0 ], wtmp[ 0 ], brw_imm_uw( 4 ) );
    brw_SHL( p, wtmp[ 1 ], wtmp[ 1 ], brw_imm_uw( 4 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], param[ 2 ] );
    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], param[ 2 ] );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], param[ 2 ] );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], param[ 2 ] );
    
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );

    /* w component */
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 1 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 0 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 1 ] ) );

    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], t );
    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    brw_ADD( p, t, param[ 0 ], brw_imm_f( -1.0 ) );
    
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );

    /* Here we interpolate in the y dimension... */
    brw_ADD( p, x0y1, x0y1, negate( x0y0 ) );
    brw_ADD( p, x1y1, x1y1, negate( x1y0 ) );
    brw_MUL( p, x0y1, x0y1, interp[ 1 ] );
    brw_MUL( p, x1y1, x1y1, interp[ 1 ] );
    brw_ADD( p, x0y0, x0y0, x0y1 );
    brw_ADD( p, x1y0, x1y0, x1y1 );

    /* And now in x.  Leave the result in tmp[ 0 ] (see below)... */
    brw_ADD( p, x1y0, x1y0, negate( x0y0 ) );
    brw_MUL( p, x1y0, x1y0, interp[ 0 ] );
    brw_ADD( p, tmp[ 0 ], x0y0, x1y0 );

    /* Now do the same thing for the front four gradients... */
    /* x component */
    brw_MOV( p, x0y0, low_words( tmp[ 2 ] ) );
    brw_MOV( p, x0y1, low_words( tmp[ 3 ] ) );
    brw_MOV( p, x1y0, high_words( tmp[ 2 ] ) );
    brw_MOV( p, x1y1, high_words( tmp[ 3 ] ) );

    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 2 ], wtmp[ 2 ], brw_imm_uw( 4 ) );
    brw_SHL( p, wtmp[ 3 ], wtmp[ 3 ], brw_imm_uw( 4 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, x1y0, x1y0, t );
    brw_MUL( p, x1y1, x1y1, t );
    brw_ADD( p, t, param[ 1 ], brw_imm_f( -1.0 ) );
    brw_MUL( p, x0y0, x0y0, param[ 0 ] );
    brw_MUL( p, x0y1, x0y1, param[ 0 ] );

    /* y component */
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 2 ] ) );
    
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 2 ], wtmp[ 2 ], brw_imm_uw( 4 ) );
    brw_SHL( p, wtmp[ 3 ], wtmp[ 3 ], brw_imm_uw( 4 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    brw_ADD( p, t, param[ 2 ], brw_imm_f( -1.0 ) );
    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], param[ 1 ] );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], param[ 1 ] );
    
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    
    /* z component */
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 3 ] ) );

    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_SHL( p, wtmp[ 2 ], wtmp[ 2 ], brw_imm_uw( 4 ) );
    brw_SHL( p, wtmp[ 3 ], wtmp[ 3 ], brw_imm_uw( 4 ) );
    brw_pop_insn_state( p );

    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], t );
    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    /* prepare t for the w component (used below): w the first time through
       the loop; w - 1 the second time) */
    brw_set_predicate_control( p, BRW_PREDICATE_NORMAL );
    brw_ADD( p, t, param[ 3 ], brw_imm_f( -1.0 ) );
    p->current->header.predicate_inverse = 1;
    brw_MOV( p, t, param[ 3 ] );
    p->current->header.predicate_inverse = 0;
    brw_set_predicate_control( p, BRW_PREDICATE_NONE );
    
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );

    /* w component */
    brw_MOV( p, tmp[ 4 ], low_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 5 ], low_words( tmp[ 3 ] ) );
    brw_MOV( p, tmp[ 6 ], high_words( tmp[ 2 ] ) );
    brw_MOV( p, tmp[ 7 ], high_words( tmp[ 3 ] ) );

    brw_MUL( p, tmp[ 4 ], tmp[ 4 ], t );
    brw_MUL( p, tmp[ 5 ], tmp[ 5 ], t );
    brw_MUL( p, tmp[ 6 ], tmp[ 6 ], t );
    brw_MUL( p, tmp[ 7 ], tmp[ 7 ], t );
    
    brw_ADD( p, x0y0, x0y0, tmp[ 4 ] );
    brw_ADD( p, x0y1, x0y1, tmp[ 5 ] );
    brw_ADD( p, x1y0, x1y0, tmp[ 6 ] );
    brw_ADD( p, x1y1, x1y1, tmp[ 7 ] );

    /* Interpolate in the y dimension: */
    brw_ADD( p, x0y1, x0y1, negate( x0y0 ) );
    brw_ADD( p, x1y1, x1y1, negate( x1y0 ) );
    brw_MUL( p, x0y1, x0y1, interp[ 1 ] );
    brw_MUL( p, x1y1, x1y1, interp[ 1 ] );
    brw_ADD( p, x0y0, x0y0, x0y1 );
    brw_ADD( p, x1y0, x1y0, x1y1 );

    /* And now in x.  The rear face is in tmp[ 0 ] (see above), so this
       time put the front face in tmp[ 1 ] and we're nearly there... */
    brw_ADD( p, x1y0, x1y0, negate( x0y0 ) );
    brw_MUL( p, x1y0, x1y0, interp[ 0 ] );
    brw_ADD( p, tmp[ 1 ], x0y0, x1y0 );

    /* Another interpolation, in the z dimension: */
    brw_ADD( p, tmp[ 1 ], tmp[ 1 ], negate( tmp[ 0 ] ) );    
    brw_MUL( p, tmp[ 1 ], tmp[ 1 ], interp[ 2 ] );
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], tmp[ 1 ] );

    /* Exit the loop if we've computed both cubes... */
    origin = p->nr_insn;
    brw_push_insn_state( p );
    brw_set_predicate_control( p, BRW_PREDICATE_NORMAL );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_ADD( p, brw_ip_reg(), brw_ip_reg(), brw_imm_d( 0 ) );
    brw_pop_insn_state( p );

    /* Save the result for the w=0 case, and increment the w coordinate: */
    brw_MOV( p, w0, tmp[ 0 ] );
    brw_ADD( p, high_words( floors[ 1 ] ), high_words( floors[ 1 ] ),
	     brw_imm_uw( 1 ) );

    /* Loop around for the other cube.  Explicitly set the flag register
       (unfortunately we must spend an extra instruction to do this: we
       can't rely on a side effect of the previous MOV or ADD because
       conditional modifiers which are normally true might be false in
       exceptional circumstances, e.g. given a NaN input; the add to
       brw_ip_reg() is not suitable because the IP is not an 8-vector). */
    brw_push_insn_state( p );
    brw_set_mask_control( p, BRW_MASK_DISABLE );
    brw_MOV( p, brw_flag_reg(), brw_imm_uw( 0xFF ) );
    brw_ADD( p, brw_ip_reg(), brw_ip_reg(),
	     brw_imm_d( ( loop - p->nr_insn ) << 4 ) );
    brw_pop_insn_state( p );

    /* Patch the previous conditional branch now that we know the
       destination address. */
    brw_set_src1( p->store + origin,
		  brw_imm_d( ( p->nr_insn - origin ) << 4 ) );

    /* The very last interpolation. */
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], negate( w0 ) );    
    brw_MUL( p, tmp[ 0 ], tmp[ 0 ], interp[ 3 ] );
    brw_ADD( p, tmp[ 0 ], tmp[ 0 ], w0 );

    /* scale by pow( 2, -15 ), as described above */
    brw_MUL( p, param[ 0 ], tmp[ 0 ], brw_imm_f( 0.000030517578125 ) );

    release_tmps( c, mark );
}

static void emit_noise4( struct brw_wm_compile *c,
			 const struct prog_instruction *inst )
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, src2, src3, param0, param1, param2, param3, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    int mark = mark_tmps( c );

    assert( mark == 0 );
    
    src0 = get_src_reg( c, inst, 0, 0 );
    src1 = get_src_reg( c, inst, 0, 1 );
    src2 = get_src_reg( c, inst, 0, 2 );
    src3 = get_src_reg( c, inst, 0, 3 );

    param0 = alloc_tmp( c );
    param1 = alloc_tmp( c );
    param2 = alloc_tmp( c );
    param3 = alloc_tmp( c );

    brw_MOV( p, param0, src0 );
    brw_MOV( p, param1, src1 );
    brw_MOV( p, param2, src2 );
    brw_MOV( p, param3, src3 );

    invoke_subroutine( c, SUB_NOISE4, noise4_sub );
    
    /* Fill in the result: */
    brw_set_saturate( p, inst->SaturateMode == SATURATE_ZERO_ONE );
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i);
	    brw_MOV( p, dst, param0 );
	}
    }
    if( inst->SaturateMode == SATURATE_ZERO_ONE )
	brw_set_saturate( p, 0 );
    
    release_tmps( c, mark );
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
    GLuint i, if_depth = 0, loop_depth = 0;
    struct brw_compile *p = &c->func;
    struct brw_indirect stack_index = brw_indirect(0, 0);

    c->out_of_regs = GL_FALSE;

    prealloc_reg(c);
    brw_set_compression_control(p, BRW_COMPRESSION_NONE);
    brw_MOV(p, get_addr_reg(stack_index), brw_address(c->stack));

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
        if (c->fp->use_const_buffer)
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
	    case OPCODE_NOISE1:
		emit_noise1(c, inst);
		break;
	    case OPCODE_NOISE2:
		emit_noise2(c, inst);
		break;
	    case OPCODE_NOISE3:
		emit_noise3(c, inst);
		break;
	    case OPCODE_NOISE4:
		emit_noise4(c, inst);
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
		emit_kil(c);
		break;
	    case OPCODE_IF:
		assert(if_depth < MAX_IF_DEPTH);
		if_inst[if_depth++] = brw_IF(p, BRW_EXECUTE_8);
		break;
	    case OPCODE_ELSE:
		assert(if_depth > 0);
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

                  if (intel->is_ironlake)
                     br = 2;

		  assert(loop_depth > 0);
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
		printf("unsupported IR in fragment shader %d\n",
			inst->Opcode);
	}

	/* Release temporaries containing any unaliased source regs. */
	release_tmps( c, mark );

	if (inst->CondUpdate)
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	else
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
    }
    post_wm_emit(c);

    if (INTEL_DEBUG & DEBUG_WM) {
      printf("wm-native:\n");
      for (i = 0; i < p->nr_insn; i++)
	 brw_disasm(stderr, &p->store[i]);
      printf("\n");
    }
}

/**
 * Do GPU code generation for shaders that use GLSL features such as
 * flow control.  Other shaders will be compiled with the 
 */
void brw_wm_glsl_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
    if (INTEL_DEBUG & DEBUG_WM) {
        printf("brw_wm_glsl_emit:\n");
    }

    /* initial instruction translation/simplification */
    brw_wm_pass_fp(c);

    /* actual code generation */
    brw_wm_emit_glsl(brw, c);

    if (INTEL_DEBUG & DEBUG_WM) {
        brw_wm_print_program(c, "brw_wm_glsl_emit done");
    }

    c->prog_data.total_grf = num_grf_used(c);
    c->prog_data.total_scratch = 0;
}
