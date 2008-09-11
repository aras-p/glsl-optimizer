#include "macros.h"
#include "shader/prog_parameter.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"

/* Only guess, need a flag in gl_fragment_program later */
GLboolean brw_wm_is_glsl(const struct gl_fragment_program *fp)
{
    int i;
    for (i = 0; i < fp->Base.NumInstructions; i++) {
	struct prog_instruction *inst = &fp->Base.Instructions[i];
	switch (inst->Opcode) {
	    case OPCODE_IF:
	    case OPCODE_INT:
	    case OPCODE_ENDIF:
	    case OPCODE_CAL:
	    case OPCODE_BRK:
	    case OPCODE_RET:
	    case OPCODE_DDX:
	    case OPCODE_DDY:
	    case OPCODE_BGNLOOP:
		return GL_TRUE; 
	    default:
		break;
	}
    }
    return GL_FALSE; 
}

static void set_reg(struct brw_wm_compile *c, int file, int index, 
	int component, struct brw_reg reg)
{
    c->wm_regs[file][index][component].reg = reg;
    c->wm_regs[file][index][component].inited = GL_TRUE;
}

static int get_scalar_dst_index(struct prog_instruction *inst)
{
    int i;
    for (i = 0; i < 4; i++)
	if (inst->DstReg.WriteMask & (1<<i))
	    break;
    return i;
}

static struct brw_reg alloc_tmp(struct brw_wm_compile *c)
{
    struct brw_reg reg;
    reg = brw_vec8_grf(c->tmp_index--, 0);
    return reg;
}

static void release_tmps(struct brw_wm_compile *c)
{
    c->tmp_index = 127;
}

static struct brw_reg 
get_reg(struct brw_wm_compile *c, int file, int index, int component, int nr, GLuint neg, GLuint abs)
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
	default:
	    break;
    }

    if(c->wm_regs[file][index][component].inited)
	reg = c->wm_regs[file][index][component].reg;
    else 
	reg = brw_vec8_grf(c->reg_index, 0);

    if(!c->wm_regs[file][index][component].inited) {
	set_reg(c, file, index, component, reg);
	c->reg_index++;
    }

    if (neg & (1<< component)) {
	reg = negate(reg);
    }
    if (abs)
	reg = brw_abs(reg);
    return reg;
}

static void prealloc_reg(struct brw_wm_compile *c)
{
    int i, j;
    struct brw_reg reg;
    int nr_interp_regs = 0;
    GLuint inputs = FRAG_BIT_WPOS | c->fp_interp_emitted | c->fp_deriv_emitted;

    for (i = 0; i < 4; i++) {
	reg = (i < c->key.nr_depth_regs) 
	    ? brw_vec8_grf(i*2, 0) : brw_vec8_grf(0, 0);
	set_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH, i, reg);
    }
    c->reg_index += 2*c->key.nr_depth_regs;
    {
	int nr_params = c->fp->program.Base.Parameters->NumParameters;
	struct gl_program_parameter_list *plist = 
	    c->fp->program.Base.Parameters;
	int index = 0;
	c->prog_data.nr_params = 4*nr_params;
	for (i = 0; i < nr_params; i++) {
	    for (j = 0; j < 4; j++, index++) {
		reg = brw_vec1_grf(c->reg_index + index/8, 
			index%8);
		c->prog_data.param[index] = 
		    &plist->ParameterValues[i][j];
		set_reg(c, PROGRAM_STATE_VAR, i, j, reg);
	    }
	}
	c->nr_creg = 2*((4*nr_params+15)/16);
	c->reg_index += c->nr_creg;
    }
    for (i = 0; i < FRAG_ATTRIB_MAX; i++) {
	if (inputs & (1<<i)) {
	    nr_interp_regs++;
	    reg = brw_vec8_grf(c->reg_index, 0);
	    for (j = 0; j < 4; j++)
		set_reg(c, PROGRAM_PAYLOAD, i, j, reg);
	    c->reg_index += 2;

	}
    }
    c->prog_data.first_curbe_grf = c->key.nr_depth_regs * 2;
    c->prog_data.urb_read_length = nr_interp_regs * 2;
    c->prog_data.curb_read_length = c->nr_creg;
    c->emit_mask_reg = brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, c->reg_index, 0);
    c->reg_index++;
    c->stack =  brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, c->reg_index, 0);
    c->reg_index += 2;
}

static struct brw_reg get_dst_reg(struct brw_wm_compile *c, 
	struct prog_instruction *inst, int component, int nr)
{
    return get_reg(c, inst->DstReg.File, inst->DstReg.Index, component, nr,
	    0, 0);
}

static struct brw_reg get_src_reg(struct brw_wm_compile *c, 
	struct prog_src_register *src, int index, int nr)
{
    int component = GET_SWZ(src->Swizzle, index);
    return get_reg(c, src->File, src->Index, component, nr, 
	    src->NegateBase, src->Abs);
}

static void emit_abs( struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for (i = 0; i < 4; i++) {
	if (inst->DstReg.WriteMask & (1<<i)) {
	    struct brw_reg src, dst;
	    dst = get_dst_reg(c, inst, i, 1);
	    src = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    brw_MOV(p, dst, brw_abs(src));
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_int( struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    struct brw_reg src, dst;
	    dst = get_dst_reg(c, inst, i, 1) ;
	    src = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    brw_RNDD(p, dst, src);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_mov( struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    struct brw_reg src, dst;
	    dst = get_dst_reg(c, inst, i, 1);
	    src = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    brw_MOV(p, dst, src);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_pixel_xy(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_reg r1 = brw_vec1_grf(1, 0);
    struct brw_reg r1_uw = retype(r1, BRW_REGISTER_TYPE_UW);

    struct brw_reg dst0, dst1;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    dst0 = get_dst_reg(c, inst, 0, 1);
    dst1 = get_dst_reg(c, inst, 1, 1);
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
		struct prog_instruction *inst)
{
    struct brw_reg r1 = brw_vec1_grf(1, 0);
    struct brw_reg dst0, dst1, src0, src1;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    dst0 = get_dst_reg(c, inst, 0, 1);
    dst1 = get_dst_reg(c, inst, 1, 1);
    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    src1 = get_src_reg(c, &inst->SrcReg[0], 1, 1);
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
		struct prog_instruction *inst)
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
    {
	brw_push_insn_state(p);
	for (channel = 0; channel < 4; channel++) {
	    src0 = get_src_reg(c,  &inst->SrcReg[0], channel, 1);
	    /*  mov (8) m2.0<1>:ud   r28.0<8;8,1>:ud  { Align1 } */
	    /*  mov (8) m6.0<1>:ud   r29.0<8;8,1>:ud  { Align1 SecHalf } */
	    brw_MOV(p, brw_message_reg(nr + channel), src0);
	}
	/* skip over the regs populated above: */
	nr += 8;
	brw_pop_insn_state(p);
    }

   if (c->key.source_depth_to_render_target)
   {
      if (c->key.computes_depth) {
         src0 = get_src_reg(c, &inst->SrcReg[2], 2, 1);
         brw_MOV(p, brw_message_reg(nr), src0);
      } else {
         src0 = get_src_reg(c, &inst->SrcReg[1], 1, 1);
         brw_MOV(p, brw_message_reg(nr), src0);
      }

      nr += 2;
   }
    target = inst->Sampler >> 1;
    eot = inst->Sampler & 1;
    fire_fb_write(c, 0, nr, target, eot);
}

static void emit_pixel_w( struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    if (mask & WRITEMASK_W) {
	struct brw_reg dst, src0, delta0, delta1;
	struct brw_reg interp3;

	dst = get_dst_reg(c, inst, 3, 1);
	src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
	delta0 = get_src_reg(c, &inst->SrcReg[1], 0, 1);
	delta1 = get_src_reg(c, &inst->SrcReg[1], 1, 1);

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
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg interp[4];
    struct brw_reg dst, delta0, delta1;
    struct brw_reg src0;

    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    delta0 = get_src_reg(c, &inst->SrcReg[1], 0, 1);
    delta1 = get_src_reg(c, &inst->SrcReg[1], 1, 1);
    GLuint nr = src0.nr;
    int i;

    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);

    for(i = 0; i < 4; i++ ) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    brw_LINE(p, brw_null_reg(), interp[i], delta0);
	    brw_MAC(p, dst, suboffset(interp[i],1), delta1);
	}
    }
}

static void emit_cinterp(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    struct brw_reg interp[4];
    struct brw_reg dst, src0;

    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    GLuint nr = src0.nr;
    int i;

    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);

    for(i = 0; i < 4; i++ ) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    brw_MOV(p, dst, suboffset(interp[i],3));
	}
    }
}

static void emit_pinterp(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;

    struct brw_reg interp[4];
    struct brw_reg dst, delta0, delta1;
    struct brw_reg src0, w;

    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    delta0 = get_src_reg(c, &inst->SrcReg[1], 0, 1);
    delta1 = get_src_reg(c, &inst->SrcReg[1], 1, 1);
    w = get_src_reg(c, &inst->SrcReg[2], 3, 1);
    GLuint nr = src0.nr;
    int i;

    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);

    for(i = 0; i < 4; i++ ) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    brw_LINE(p, brw_null_reg(), interp[i], delta0);
	    brw_MAC(p, dst, suboffset(interp[i],1), 
		    delta1);
	    brw_MUL(p, dst, dst, w);
	}
    }
}

static void emit_xpd(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    int i;
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    for (i = 0; i < 4; i++) {
	GLuint i2 = (i+2)%3;
	GLuint i1 = (i+1)%3;
	if (mask & (1<<i)) {
	    struct brw_reg src0, src1, dst;
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = negate(get_src_reg(c, &inst->SrcReg[0], i2, 1));
	    src1 = get_src_reg(c, &inst->SrcReg[1], i1, 1);
	    brw_MUL(p, brw_null_reg(), src0, src1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i1, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i2, 1);
	    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
	    brw_MAC(p, dst, src0, src1);
	    brw_set_saturate(p, 0);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_dp3(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_reg src0[3], src1[3], dst;
    int i;
    struct brw_compile *p = &c->func;
    for (i = 0; i < 3; i++) {
	src0[i] = get_src_reg(c, &inst->SrcReg[0], i, 1);
	src1[i] = get_src_reg(c, &inst->SrcReg[1], i, 1);
    }

    dst = get_dst_reg(c, inst, get_scalar_dst_index(inst), 1);
    brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
    brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    brw_MAC(p, dst, src0[2], src1[2]);
    brw_set_saturate(p, 0);
}

static void emit_dp4(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_reg src0[4], src1[4], dst;
    int i;
    struct brw_compile *p = &c->func;
    for (i = 0; i < 4; i++) {
	src0[i] = get_src_reg(c, &inst->SrcReg[0], i, 1);
	src1[i] = get_src_reg(c, &inst->SrcReg[1], i, 1);
    }
    dst = get_dst_reg(c, inst, get_scalar_dst_index(inst), 1);
    brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
    brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
    brw_MAC(p, brw_null_reg(), src0[2], src1[2]);
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    brw_MAC(p, dst, src0[3], src1[3]);
    brw_set_saturate(p, 0);
}

static void emit_dph(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_reg src0[4], src1[4], dst;
    int i;
    struct brw_compile *p = &c->func;
    for (i = 0; i < 4; i++) {
	src0[i] = get_src_reg(c, &inst->SrcReg[0], i, 1);
	src1[i] = get_src_reg(c, &inst->SrcReg[1], i, 1);
    }
    dst = get_dst_reg(c, inst, get_scalar_dst_index(inst), 1);
    brw_MUL(p, brw_null_reg(), src0[0], src1[0]);
    brw_MAC(p, brw_null_reg(), src0[1], src1[1]);
    brw_MAC(p, dst, src0[2], src1[2]);
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    brw_ADD(p, dst, src0[3], src1[3]);
    brw_set_saturate(p, 0);
}

static void emit_math1(struct brw_wm_compile *c,
		struct prog_instruction *inst, GLuint func)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, dst;

    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    dst = get_dst_reg(c, inst, get_scalar_dst_index(inst), 1);
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
		struct prog_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_INV);
}

static void emit_rsq(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_RSQ);
}

static void emit_sin(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_SIN);
}

static void emit_cos(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_COS);
}

static void emit_ex2(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_EXP);
}

static void emit_lg2(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_math1(c, inst, BRW_MATH_FUNCTION_LOG);
}

static void emit_add(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
	    brw_ADD(p, dst, src0, src1);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_sub(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
	    brw_ADD(p, dst, src0, negate(src1));
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_mul(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, src1, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
	    brw_MUL(p, dst, src0, src1);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_frc(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    brw_FRC(p, dst, src0);
	}
    }
    if (inst->SaturateMode != SATURATE_OFF)
	brw_set_saturate(p, 0);
}

static void emit_flr(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg src0, dst;
    GLuint mask = inst->DstReg.WriteMask;
    int i;
    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
    for (i = 0 ; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    brw_RNDD(p, dst, src0);
	}
    }
    brw_set_saturate(p, 0);
}

static void emit_max(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg src0, src1, dst;
    int i;
    brw_push_insn_state(p);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_MOV(p, dst, src0);
	    brw_set_saturate(p, 0);

	    brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, src0, src1);
	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	    brw_MOV(p, dst, src1);
	    brw_set_saturate(p, 0);
	    brw_set_predicate_control_flag_value(p, 0xff);
	}
    }
    brw_pop_insn_state(p);
}

static void emit_min(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg src0, src1, dst;
    int i;
    brw_push_insn_state(p);
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_MOV(p, dst, src0);
	    brw_set_saturate(p, 0);

	    brw_CMP(p, brw_null_reg(), BRW_CONDITIONAL_L, src1, src0);
	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	    brw_MOV(p, dst, src1);
	    brw_set_saturate(p, 0);
	    brw_set_predicate_control_flag_value(p, 0xff);
	}
    }
    brw_pop_insn_state(p);
}

static void emit_pow(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg dst, src0, src1;
    dst = get_dst_reg(c, inst, get_scalar_dst_index(inst), 1);
    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    src1 = get_src_reg(c, &inst->SrcReg[1], 0, 1);

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
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg dst, tmp1, tmp2, src0, src1, src2;
    int i;
    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);

	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);

	    if (src1.nr == dst.nr) {
		tmp1 = alloc_tmp(c);
		brw_MOV(p, tmp1, src1);
	    } else
		tmp1 = src1;

	    src2 = get_src_reg(c, &inst->SrcReg[2], i, 1);
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
	release_tmps(c);
    }
}

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
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg dst, src0, src1, src2;
    int i;

    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
	    src2 = get_src_reg(c, &inst->SrcReg[2], i, 1);
	    brw_MUL(p, dst, src0, src1);

	    brw_set_saturate(p, (inst->SaturateMode != SATURATE_OFF) ? 1 : 0);
	    brw_ADD(p, dst, dst, src2);
	    brw_set_saturate(p, 0);
	}
    }
}

static void emit_sop(struct brw_wm_compile *c,
		struct prog_instruction *inst, GLuint cond)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg dst, src0, src1;
    int i;

    for (i = 0; i < 4; i++) {
	if (mask & (1<<i)) {
	    dst = get_dst_reg(c, inst, i, 1);
	    src0 = get_src_reg(c, &inst->SrcReg[0], i, 1);
	    src1 = get_src_reg(c, &inst->SrcReg[1], i, 1);
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
		struct prog_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_L);
}

static void emit_sle(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_LE);
}

static void emit_sgt(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_G);
}

static void emit_sge(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_GE);
}

static void emit_seq(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_EQ);
}

static void emit_sne(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    emit_sop(c, inst, BRW_CONDITIONAL_NEQ);
}

static void emit_ddx(struct brw_wm_compile *c,
                struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg interp[4];
    struct brw_reg dst;
    struct brw_reg src0, w;
    GLuint nr, i;
    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    w = get_src_reg(c, &inst->SrcReg[1], 3, 1);
    nr = src0.nr;
    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for(i = 0; i < 4; i++ ) {
        if (mask & (1<<i)) {
            dst = get_dst_reg(c, inst, i, 1);
            brw_MOV(p, dst, interp[i]);
            brw_MUL(p, dst, dst, w);
        }
    }
    brw_set_saturate(p, 0);
}

static void emit_ddy(struct brw_wm_compile *c,
                struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg interp[4];
    struct brw_reg dst;
    struct brw_reg src0, w;
    GLuint nr, i;

    src0 = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    nr = src0.nr;
    w = get_src_reg(c, &inst->SrcReg[1], 3, 1);
    interp[0] = brw_vec1_grf(nr, 0);
    interp[1] = brw_vec1_grf(nr, 4);
    interp[2] = brw_vec1_grf(nr+1, 0);
    interp[3] = brw_vec1_grf(nr+1, 4);
    brw_set_saturate(p, inst->SaturateMode != SATURATE_OFF);
    for(i = 0; i < 4; i++ ) {
        if (mask & (1<<i)) {
            dst = get_dst_reg(c, inst, i, 1);
            brw_MOV(p, dst, suboffset(interp[i], 1));
            brw_MUL(p, dst, dst, w);
        }
    }
    brw_set_saturate(p, 0);
}

static void emit_wpos_xy(struct brw_wm_compile *c,
                struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    GLuint mask = inst->DstReg.WriteMask;
    struct brw_reg src0[2], dst[2];

    dst[0] = get_dst_reg(c, inst, 0, 1);
    dst[1] = get_dst_reg(c, inst, 1, 1);

    src0[0] = get_src_reg(c, &inst->SrcReg[0], 0, 1);
    src0[1] = get_src_reg(c, &inst->SrcReg[0], 1, 1);

    /* Calculate the pixel offset from window bottom left into destination
     * X and Y channels.
     */
    if (mask & WRITEMASK_X) {
	/* X' = X - origin_x */
	brw_ADD(p,
		dst[0],
		retype(src0[0], BRW_REGISTER_TYPE_W),
		brw_imm_d(0 - c->key.origin_x));
    }

    if (mask & WRITEMASK_Y) {
	/* Y' = height - (Y - origin_y) = height + origin_y - Y */
	brw_ADD(p,
		dst[1],
		negate(retype(src0[1], BRW_REGISTER_TYPE_W)),
		brw_imm_d(c->key.origin_y + c->key.drawable_height - 1));
    }
}

/* TODO
   BIAS on SIMD8 not workind yet...
 */	
static void emit_txb(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg dst[4], src[4], payload_reg;
    GLuint unit = c->fp->program.Base.SamplerUnits[inst->TexSrcUnit];

    GLuint i;
    payload_reg = get_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH, 0, 1, 0, 0);
    for (i = 0; i < 4; i++) 
	dst[i] = get_dst_reg(c, inst, i, 1);
    for (i = 0; i < 4; i++)
	src[i] = get_src_reg(c, &inst->SrcReg[0], i, 1);

    switch (inst->TexSrcTarget) {
	case TEXTURE_1D_INDEX:
	    brw_MOV(p, brw_message_reg(2), src[0]);
	    brw_MOV(p, brw_message_reg(3), brw_imm_f(0));
	    brw_MOV(p, brw_message_reg(4), brw_imm_f(0));
	    break;
	case TEXTURE_2D_INDEX:
	case TEXTURE_RECT_INDEX:
	    brw_MOV(p, brw_message_reg(2), src[0]);
	    brw_MOV(p, brw_message_reg(3), src[1]);
	    brw_MOV(p, brw_message_reg(4), brw_imm_f(0));
	    break;
	default:
	    brw_MOV(p, brw_message_reg(2), src[0]);
	    brw_MOV(p, brw_message_reg(3), src[1]);
	    brw_MOV(p, brw_message_reg(4), src[2]);
	    break;
    }
    brw_MOV(p, brw_message_reg(5), src[3]);
    brw_MOV(p, brw_message_reg(6), brw_imm_f(0));
    brw_SAMPLE(p,
	    retype(vec8(dst[0]), BRW_REGISTER_TYPE_UW),
	    1,
	    retype(payload_reg, BRW_REGISTER_TYPE_UW),
	    unit + MAX_DRAW_BUFFERS, /* surface */
	    unit,     /* sampler */
	    inst->DstReg.WriteMask,
	    BRW_SAMPLER_MESSAGE_SIMD16_SAMPLE_BIAS,
	    4,
	    4,
	    0);
}

static void emit_tex(struct brw_wm_compile *c,
		struct prog_instruction *inst)
{
    struct brw_compile *p = &c->func;
    struct brw_reg dst[4], src[4], payload_reg;
    GLuint unit = c->fp->program.Base.SamplerUnits[inst->TexSrcUnit];

    GLuint msg_len;
    GLuint i, nr;
    GLuint emit;
    GLboolean shadow = (c->key.shadowtex_mask & (1<<unit)) ? 1 : 0;

    payload_reg = get_reg(c, PROGRAM_PAYLOAD, PAYLOAD_DEPTH, 0, 1, 0, 0);

    for (i = 0; i < 4; i++) 
	dst[i] = get_dst_reg(c, inst, i, 1);
    for (i = 0; i < 4; i++)
	src[i] = get_src_reg(c, &inst->SrcReg[0], i, 1);


    switch (inst->TexSrcTarget) {
	case TEXTURE_1D_INDEX:
	    emit = WRITEMASK_X;
	    nr = 1;
	    break;
	case TEXTURE_2D_INDEX:
	case TEXTURE_RECT_INDEX:
	    emit = WRITEMASK_XY;
	    nr = 2;
	    break;
	default:
	    emit = WRITEMASK_XYZ;
	    nr = 3;
	    break;
    }
    msg_len = 1;

    for (i = 0; i < nr; i++) {
	static const GLuint swz[4] = {0,1,2,2};
	if (emit & (1<<i))
	    brw_MOV(p, brw_message_reg(msg_len+1), src[swz[i]]);
	else
	    brw_MOV(p, brw_message_reg(msg_len+1), brw_imm_f(0));
	msg_len += 1;
    }

    if (shadow) {
	brw_MOV(p, brw_message_reg(5), brw_imm_f(0));
	brw_MOV(p, brw_message_reg(6), src[2]);
    }

    brw_SAMPLE(p,
	    retype(vec8(dst[0]), BRW_REGISTER_TYPE_UW),
	    1,
	    retype(payload_reg, BRW_REGISTER_TYPE_UW),
	    unit + MAX_DRAW_BUFFERS, /* surface */
	    unit,     /* sampler */
	    inst->DstReg.WriteMask,
	    BRW_SAMPLER_MESSAGE_SIMD8_SAMPLE,
	    4,
	    shadow ? 6 : 4,
	    0);

    if (shadow)
	brw_MOV(p, dst[3], brw_imm_f(1.0));
}

static void post_wm_emit( struct brw_wm_compile *c )
{
    GLuint nr_insns = c->fp->program.Base.NumInstructions;
    GLuint insn, target_insn;
    struct prog_instruction *inst1, *inst2;
    struct brw_instruction *brw_inst1, *brw_inst2;
    int offset;
    for (insn = 0; insn < nr_insns; insn++) {
	inst1 = &c->fp->program.Base.Instructions[insn];
	brw_inst1 = inst1->Data;
	switch (inst1->Opcode) {
	    case OPCODE_CAL:
		target_insn = inst1->BranchTarget;
		inst2 = &c->fp->program.Base.Instructions[target_insn];
		brw_inst2 = inst2->Data;
		offset = brw_inst2 - brw_inst1;
		brw_set_src1(brw_inst1, brw_imm_d(offset*16));
		break;
	    default:
		break;
	}
    }
}

static void brw_wm_emit_glsl(struct brw_context *brw, struct brw_wm_compile *c)
{
#define MAX_IFSN 32
#define MAX_LOOP_DEPTH 32
    struct brw_instruction *if_inst[MAX_IFSN], *loop_inst[MAX_LOOP_DEPTH];
    struct brw_instruction *inst0, *inst1;
    int i, if_insn = 0, loop_insn = 0;
    struct brw_compile *p = &c->func;
    struct brw_indirect stack_index = brw_indirect(0, 0);

    c->reg_index = 0;
    prealloc_reg(c);
    brw_set_compression_control(p, BRW_COMPRESSION_NONE);
    brw_MOV(p, get_addr_reg(stack_index), brw_address(c->stack));

    for (i = 0; i < c->nr_fp_insns; i++) {
	struct prog_instruction *inst = &c->prog_instructions[i];
	struct prog_instruction *orig_inst;

	if ((orig_inst = inst->Data) != 0)
	    orig_inst->Data = current_insn(p);

	if (inst->CondUpdate)
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NZ);
	else
	    brw_set_conditionalmod(p, BRW_CONDITIONAL_NONE);

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
	    case OPCODE_ABS:
		emit_abs(c, inst);
		break;
	    case OPCODE_ADD:
		emit_add(c, inst);
		break;
	    case OPCODE_SUB:
		emit_sub(c, inst);
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
	    case OPCODE_INT:
		emit_int(c, inst);
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
	    case OPCODE_MAX:	
		emit_max(c, inst);
		break;
	    case OPCODE_MIN:	
		emit_min(c, inst);
		break;
	    case OPCODE_DDX:
		emit_ddx(c, inst);
		break;
	    case OPCODE_DDY:
                emit_ddy(c, inst);
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
		assert(if_insn < MAX_IFSN);
		if_inst[if_insn++] = brw_IF(p, BRW_EXECUTE_8);
		break;
	    case OPCODE_ELSE:
		if_inst[if_insn-1]  = brw_ELSE(p, if_inst[if_insn-1]);
		break;
	    case OPCODE_ENDIF:
		assert(if_insn > 0);
		brw_ENDIF(p, if_inst[--if_insn]);
		break;
	    case OPCODE_BGNSUB:
	    case OPCODE_ENDSUB:
		break;
	    case OPCODE_CAL: 
		brw_push_insn_state(p);
		brw_set_mask_control(p, BRW_MASK_DISABLE);
                brw_set_access_mode(p, BRW_ALIGN_1);
                brw_ADD(p, deref_1ud(stack_index, 0), brw_ip_reg(), brw_imm_d(3*16));
                brw_set_access_mode(p, BRW_ALIGN_16);
                brw_ADD(p, get_addr_reg(stack_index),
                         get_addr_reg(stack_index), brw_imm_d(4));
                orig_inst = inst->Data;
                orig_inst->Data = &p->store[p->nr_insn];
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
		loop_inst[loop_insn++] = brw_DO(p, BRW_EXECUTE_8);
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
		loop_insn--;
		inst0 = inst1 = brw_WHILE(p, loop_inst[loop_insn]);
		/* patch all the BREAK instructions from
		   last BEGINLOOP */
		while (inst0 > loop_inst[loop_insn]) {
		    inst0--;
		    if (inst0->header.opcode == BRW_OPCODE_BREAK) {
			inst0->bits3.if_else.jump_count = inst1 - inst0 + 1;
			inst0->bits3.if_else.pop_count = 0;
		    } else if (inst0->header.opcode == BRW_OPCODE_CONTINUE) {
                        inst0->bits3.if_else.jump_count = inst1 - inst0;
                        inst0->bits3.if_else.pop_count = 0;
                    }
		}
		break;
	    default:
		_mesa_printf("unsupported IR in fragment shader %d\n",
			inst->Opcode);
	}
	if (inst->CondUpdate)
	    brw_set_predicate_control(p, BRW_PREDICATE_NORMAL);
	else
	    brw_set_predicate_control(p, BRW_PREDICATE_NONE);
    }
    post_wm_emit(c);
    for (i = 0; i < c->fp->program.Base.NumInstructions; i++)
	c->fp->program.Base.Instructions[i].Data = NULL;
}

void brw_wm_glsl_emit(struct brw_context *brw, struct brw_wm_compile *c)
{
    brw_wm_pass_fp(c);
    c->tmp_index = 127;
    brw_wm_emit_glsl(brw, c);
    c->prog_data.total_grf = c->reg_index;
    c->prog_data.total_scratch = 0;
}
