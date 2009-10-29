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

#include "brw_wm.h"
#include "brw_util.h"


#define X    0
#define Y    1
#define Z    2
#define W    3


static const char *wm_opcode_strings[] = {
   "PIXELXY",
   "DELTAXY",
   "PIXELW",
   "LINTERP",
   "PINTERP",
   "CINTERP",
   "WPOSXY",
   "FB_WRITE",
   "FRONTFACING",
};





/* Many opcodes produce the same value across all the result channels.
 * We'd rather not have to support that splatting in the opcode implementations,
 * and brw_wm_pass*.c wants to optimize them out by shuffling references around
 * anyway.  We can easily get both by emitting the opcode to one channel, and
 * then MOVing it to the others, which brw_wm_pass*.c already understands.
 */
static void emit_scalar_insn(struct brw_wm_compile *c,
			     unsigned opcode,
			     struct brw_dst dst,
			     struct brw_src src0,
			     struct brw_src src1,
			     struct brw_src src2 )
{
   unsigned first_chan = ffs(dst.writemask) - 1;
   unsigned first_mask = 1 << first_chan;

   if (dst.writemask == 0)
      return;

   emit_op( c, opcode,
	    brw_writemask(dst, first_mask),
	    src0, src1, src2 );

   if (dst.writemask != first_mask) {
      emit_op1(c, TGSI_OPCODE_MOV,
	       brw_writemask(dst, ~first_mask),
	       src_swizzle1(brw_src(dst), first_chan));
   }
}


/***********************************************************************
 * Special instructions for interpolation and other tasks
 */

static struct ureg_src get_pixel_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->pixel_xy)) {
      struct ureg_dst pixel_xy = get_temp(c);
      struct ureg_src payload_r0_depth = src_reg(TGSI_FILE_PAYLOAD, PAYLOAD_DEPTH);
      
      
      /* Emit the out calculations, and hold onto the results.  Use
       * two instructions as a temporary is required.
       */   
      /* pixel_xy.xy = PIXELXY payload[0];
       */
      emit_op(c,
	      WM_PIXELXY,
	      dst_mask(pixel_xy, BRW_WRITEMASK_XY),
	      payload_r0_depth,
	      src_undef(),
	      src_undef());

      c->pixel_xy = src_reg_from_dst(pixel_xy);
   }

   return c->pixel_xy;
}

static struct ureg_src get_delta_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->delta_xy)) {
      struct ureg_dst delta_xy = get_temp(c);
      struct ureg_src pixel_xy = get_pixel_xy(c);
      struct ureg_src payload_r0_depth = src_reg(TGSI_FILE_PAYLOAD, PAYLOAD_DEPTH);
      
      /* deltas.xy = DELTAXY pixel_xy, payload[0]
       */
      emit_op(c,
	      WM_DELTAXY,
	      dst_mask(delta_xy, BRW_WRITEMASK_XY),
	      pixel_xy, 
	      payload_r0_depth,
	      src_undef());
      
      c->delta_xy = src_reg_from_dst(delta_xy);
   }

   return c->delta_xy;
}

static struct ureg_src get_pixel_w( struct brw_wm_compile *c )
{
   if (src_is_undef(c->pixel_w)) {
      struct ureg_dst pixel_w = get_temp(c);
      struct ureg_src deltas = get_delta_xy(c);
      struct ureg_src interp_wpos = src_reg(TGSI_FILE_PAYLOAD, FRAG_ATTRIB_WPOS);

      /* deltas.xyw = DELTAS2 deltas.xy, payload.interp_wpos.x
       */
      emit_op(c,
	      WM_PIXELW,
	      dst_mask(pixel_w, BRW_WRITEMASK_W),
	      interp_wpos,
	      deltas, 
	      src_undef());
      

      c->pixel_w = src_reg_from_dst(pixel_w);
   }

   return c->pixel_w;
}

static void emit_interp( struct brw_wm_compile *c,
			 GLuint semantic,
			 GLuint semantic_index,
			 GLuint interp_mode )
{
   struct ureg_dst dst = dst_reg(TGSI_FILE_INPUT, idx);
   struct ureg_src interp = src_reg(TGSI_FILE_PAYLOAD, idx);
   struct ureg_src deltas = get_delta_xy(c);

   /* Need to use PINTERP on attributes which have been
    * multiplied by 1/W in the SF program, and LINTERP on those
    * which have not:
    */
   switch (semantic) {
   case FRAG_ATTRIB_WPOS:
      /* Have to treat wpos.xy specially:
       */
      emit_op1(c,
	      WM_WPOSXY,
	      dst_mask(dst, BRW_WRITEMASK_XY),
	      get_pixel_xy(c));
      
      /* TGSI_FILE_INPUT.attr.xyzw = INTERP payload.interp[attr].x, deltas.xyw
       */
      emit_op2(c,
	       WM_LINTERP,
	       dst_mask(dst, BRW_WRITEMASK_ZW),
	       interp,
	       deltas);
      break;

   case TGSI_SEMANTIC_COLOR:
      if (c->key.flat_shade) {
	 emit_op1(c,
		 WM_CINTERP,
		 dst,
		 interp);
      }
      else if (interp_mode == TGSI_INTERPOLATE_LINEAR) {
	 emit_op2(c,
		  WM_LINTERP,
		  dst,
		  interp,
		  deltas);
      }
      else {
	 emit_op3(c,
		  WM_PINTERP,
		  dst,
		  interp,
		  deltas,
		  get_pixel_w(c));
      }

      break;
   case FRAG_ATTRIB_FOGC:
      /* Interpolate the fog coordinate */
      emit_op3(c,
	      WM_PINTERP,
	      dst_mask(dst, BRW_WRITEMASK_X),
	      interp,
	      deltas,
	      get_pixel_w(c));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_YZ),
	      brw_imm1f(0.0));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_W),
	      brw_imm1f(1.0));
      break;

   case FRAG_ATTRIB_FACE:
      /* XXX review/test this case */
      emit_op0(c,
	       WM_FRONTFACING,
	       dst_mask(dst, BRW_WRITEMASK_X));
      
      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_YZ),
	      brw_imm1f(0.0));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_W),
	      brw_imm1f(1.0));
      break;

   case FRAG_ATTRIB_PNTC:
      /* XXX review/test this case */
      emit_op3(c,
	       WM_PINTERP,
	       dst_mask(dst, BRW_WRITEMASK_XY),
	       interp,
	       deltas,
	       get_pixel_w(c));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_Z),
	      brw_imm1f(c->pass_fp, 0.0f));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_W),
	      brw_imm1f(c->pass_fp, 1.0f));
      break;

   default: 
      switch (interp_mode) {
      case TGSI_INTERPOLATE_CONSTANT:
	 emit_op1(c,
		  WM_CINTERP,
		  dst,
		  interp);
	 break;

      case TGSI_INTERPOLATE_LINEAR:
	 emit_op2(c,
		  WM_LINTERP,
		  dst,
		  interp,
		  deltas);
	 break;

      case TGSI_INTERPOLATE_PERSPECTIVE:
	 emit_op3(c,
		  WM_PINTERP,
		  dst,
		  interp,
		  deltas,
		  get_pixel_w(c));
	 break;
      }
      break;
   }
}


/***********************************************************************
 * Expand various instructions here to simpler forms.  
 */
static void precalc_dst( struct brw_wm_compile *c,
			 struct brw_dst dst,
			 struct brw_src src0,
			 struct brw_src src1 )
{
   if (dst.WriteMask & BRW_WRITEMASK_Y) {      
      /* dst.y = mul src0.y, src1.y
       */
      emit_op2(c,
	       TGSI_OPCODE_MUL,
	       dst_mask(dst, BRW_WRITEMASK_Y),
	       src0,
	       src1);
   }

   if (dst.WriteMask & BRW_WRITEMASK_XZ) {
      struct prog_instruction *swz;
      GLuint z = GET_SWZ(src0.Swizzle, Z);

      /* dst.z = mov src0.zzzz
       */
      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_Z),
	      src_swizzle1(src0, Z));

      /* dst.x = immf(1.0)
       */
      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      brw_saturate(dst_mask(dst, BRW_WRITEMASK_X), 0),
	      src_immf(c, 1.0));
   }
   if (dst.WriteMask & BRW_WRITEMASK_W) {
      /* dst.w = mov src1.w
       */
      emit_op1(c,
	       TGSI_OPCODE_MOV,
	       dst_mask(dst, BRW_WRITEMASK_W),
	       src1);
   }
}


static void precalc_lit( struct brw_wm_compile *c,
			 struct ureg_dst dst,
			 struct ureg_src src0 )
{
   if (dst.WriteMask & BRW_WRITEMASK_XW) {
      /* dst.xw = imm(1.0f)
       */
      emit_op1(c,
	       TGSI_OPCODE_MOV,
	       brw_saturate(brw_writemask(dst, BRW_WRITEMASK_XW), 0),
	       brw_imm1f(1.0f));
   }

   if (dst.WriteMask & BRW_WRITEMASK_YZ) {
      emit_op1(c,
	       TGSI_OPCODE_LIT,
	       brw_writemask(dst, BRW_WRITEMASK_YZ),
	       src0);
   }
}


/**
 * Some TEX instructions require extra code, cube map coordinate
 * normalization, or coordinate scaling for RECT textures, etc.
 * This function emits those extra instructions and the TEX
 * instruction itself.
 */
static void precalc_tex( struct brw_wm_compile *c,
			 struct brw_dst dst,
			 unsigned unit,
			 struct brw_src src0 )
{
   struct ureg_src coord = src_undef();
   struct ureg_dst tmp = dst_undef();

   assert(unit < BRW_MAX_TEX_UNIT);

   /* Cubemap: find longest component of coord vector and normalize
    * it.
    */
   if (inst->TexSrcTarget == TEXTURE_CUBE_INDEX) {
      struct ureg_src tmpsrc;

      tmp = get_temp(c);
      tmpsrc = brw_src(tmpcoord)

      /* tmp = abs(src0) */
      emit_op1(c, 
	       TGSI_OPCODE_MOV,
	       tmp,
	       brw_abs(src0));

      /* tmp.X = MAX(tmp.X, tmp.Y) */
      emit_op2(c, TGSI_OPCODE_MAX,
	       brw_writemask(tmp, BRW_WRITEMASK_X),
	       src_swizzle1(tmpsrc, X),
	       src_swizzle1(tmpsrc, Y));

      /* tmp.X = MAX(tmp.X, tmp.Z) */
      emit_op2(c, TGSI_OPCODE_MAX,
	       brw_writemask(tmp, BRW_WRITEMASK_X),
	       tmpsrc,
	       src_swizzle1(tmpsrc, Z));

      /* tmp.X = 1 / tmp.X */
      emit_op1(c, TGSI_OPCODE_RCP,
	      dst_mask(tmp, BRW_WRITEMASK_X),
	      tmpsrc);

      /* tmp = src0 * tmp.xxxx */
      emit_op2(c, TGSI_OPCODE_MUL,
	       tmp,
	       src0,
	       src_swizzle1(tmpsrc, SWIZZLE_X));

      coord = tmpsrc;
   }
   else if (inst->TexSrcTarget == TEXTURE_RECT_INDEX) {
      /* XXX: need a mechanism for internally generated constants.
       */
      coord = src0;
   }
   else {
      coord = src0;
   }

   /* Need to emit YUV texture conversions by hand.  Probably need to
    * do this here - the alternative is in brw_wm_emit.c, but the
    * conversion requires allocating a temporary variable which we
    * don't have the facility to do that late in the compilation.
    */
   if (c->key.yuvtex_mask & (1 << unit)) {
      /* convert ycbcr to RGBA */
      GLboolean  swap_uv = c->key.yuvtex_swap_mask & (1<<unit);
      struct ureg_dst dst = inst->DstReg;
      struct ureg_dst tmp = get_temp(c);
      struct ureg_src tmpsrc = src_reg_from_dst(tmp);
      struct ureg_src C0 = ureg_imm4f( c->ureg,  -.5, -.0625, -.5, 1.164 );
      struct ureg_src C1 = ureg_imm4f( c->ureg, 1.596, -0.813, 2.018, -.391 );
     
      /* tmp     = TEX ...
       */
      emit_tex_op(c, 
                  TGSI_OPCODE_TEX,
                  brw_saturate(tmp, dst.Saturate),
                  unit,
                  inst->TexSrcTarget,
                  coord,
                  src_undef(),
                  src_undef());

      /* tmp.xyz =  ADD TMP, C0
       */
      emit_op2(c, TGSI_OPCODE_ADD,
	       dst_mask(tmp, BRW_WRITEMASK_XYZ),
	       tmpsrc,
	       C0);

      /* YUV.y   = MUL YUV.y, C0.w
       */
      emit_op2(c, TGSI_OPCODE_MUL,
	       dst_mask(tmp, BRW_WRITEMASK_Y),
	       tmpsrc,
	       src_swizzle1(C0, W));

      /* 
       * if (UV swaped)
       *     RGB.xyz = MAD YUV.zzx, C1, YUV.y
       * else
       *     RGB.xyz = MAD YUV.xxz, C1, YUV.y
       */

      emit_op3(c, TGSI_OPCODE_MAD,
	       dst_mask(dst, BRW_WRITEMASK_XYZ),
	       ( swap_uv ? 
		 src_swizzle(tmpsrc, Z,Z,X,X) : 
		 src_swizzle(tmpsrc, X,X,Z,Z)),
	       C1,
	       src_swizzle1(tmpsrc, Y));

      /*  RGB.y   = MAD YUV.z, C1.w, RGB.y
       */
      emit_op3(c,
	       TGSI_OPCODE_MAD,
	       dst_mask(dst, BRW_WRITEMASK_Y),
	       src_swizzle1(tmpsrc, Z),
	       src_swizzle1(C1, W),
	       src_swizzle1(src_reg_from_dst(dst), Y));

      release_temp(c, tmp);
   }
   else {
      /* ordinary RGBA tex instruction */
      emit_tex_op(c, 
                  TGSI_OPCODE_TEX,
                  inst->DstReg,
                  unit,
                  inst->TexSrcTarget,
                  coord,
                  src_undef(),
                  src_undef());
   }

   /* XXX: add GL_EXT_texture_swizzle support to gallium -- by
    * generating shader varients in mesa state tracker.
    */

   /* Release this temp if we ended up allocating it:
    */
   if (!brw_dst_is_undef(tmpcoord))
      release_temp(c, tmpcoord);
}


/**
 * Check if the given TXP instruction really needs the divide-by-W step.
 */
static GLboolean projtex( struct brw_wm_compile *c,
			  const struct prog_instruction *inst )
{
   const struct ureg_src src = inst->SrcReg[0];
   GLboolean retVal;

   assert(inst->Opcode == TGSI_OPCODE_TXP);

   /* Only try to detect the simplest cases.  Could detect (later)
    * cases where we are trying to emit code like RCP {1.0}, MUL x,
    * {1.0}, and so on.
    *
    * More complex cases than this typically only arise from
    * user-provided fragment programs anyway:
    */
   if (inst->TexSrcTarget == TEXTURE_CUBE_INDEX)
      retVal = GL_FALSE;  /* ut2004 gun rendering !?! */
   else if (src.File == TGSI_FILE_INPUT && 
	    GET_SWZ(src.Swizzle, W) == W &&
            (c->key.proj_attrib_mask & (1 << src.Index)) == 0)
      retVal = GL_FALSE;
   else
      retVal = GL_TRUE;

   return retVal;
}


/**
 * Emit code for TXP.
 */
static void precalc_txp( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   struct ureg_src src0 = inst->SrcReg[0];

   if (projtex(c, inst)) {
      struct ureg_dst tmp = get_temp(c);
      struct prog_instruction tmp_inst;

      /* tmp0.w = RCP inst.arg[0][3]
       */
      emit_op(c,
	      TGSI_OPCODE_RCP,
	      dst_mask(tmp, BRW_WRITEMASK_W),
	      src_swizzle1(src0, GET_SWZ(src0.Swizzle, W)),
	      src_undef(),
	      src_undef());

      /* tmp0.xyz =  MUL inst.arg[0], tmp0.wwww
       */
      emit_op(c,
	      TGSI_OPCODE_MUL,
	      dst_mask(tmp, BRW_WRITEMASK_XYZ),
	      src0,
	      src_swizzle1(src_reg_from_dst(tmp), W),
	      src_undef());

      /* dst = precalc(TEX tmp0)
       */
      tmp_inst = *inst;
      tmp_inst.SrcReg[0] = src_reg_from_dst(tmp);
      precalc_tex(c, &tmp_inst);

      release_temp(c, tmp);
   }
   else
   {
      /* dst = precalc(TEX src0)
       */
      precalc_tex(c, inst);
   }
}



static void emit_fb_write( struct brw_wm_compile *c )
{
   struct ureg_src payload_r0_depth = src_reg(TGSI_FILE_PAYLOAD, PAYLOAD_DEPTH);
   struct ureg_src outdepth = src_reg(TGSI_FILE_OUTPUT, FRAG_RESULT_DEPTH);
   struct ureg_src outcolor;
   struct prog_instruction *inst;
   GLuint i;


   /* The inst->Aux field is used for FB write target and the EOT marker */

   for (i = 0 ; i < c->key.nr_cbufs; i++) {
      outcolor = find_output_by_semantic(c, TGSI_SEMANTIC_COLOR, i);

      inst = emit_op(c, WM_FB_WRITE,
		     dst_mask(dst_undef(), 0),
		     outcolor,
		     payload_r0_depth,
		     outdepth);

      inst->Aux = (i<<1);
   }
 
   /* Set EOT flag on last inst:
    */
   inst->Aux |= 1; //eot
}




/***********************************************************************
 * Emit INTERP instructions ahead of first use of each attrib.
 */

static void validate_src_regs( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   GLuint nr_args = brw_wm_nr_args( inst->Opcode );
   GLuint i;

   for (i = 0; i < nr_args; i++) {
      if (inst->SrcReg[i].File == TGSI_FILE_INPUT) {
	 GLuint idx = inst->SrcReg[i].Index;
	 if (!(c->fp_interp_emitted & (1<<idx))) {
	    emit_interp(c, idx);
	    c->fp_interp_emitted |= 1<<idx;
	 }
      }
   }
}
	 
static void validate_dst_regs( struct brw_wm_compile *c,
			       const struct prog_instruction *inst )
{
   if (inst->DstReg.File == TGSI_FILE_OUTPUT) {
      GLuint idx = inst->DstReg.Index;
      if (idx == FRAG_RESULT_COLOR)
         c->fp_fragcolor_emitted |= inst->DstReg.WriteMask;
   }
}



static void emit_insn( struct brw_wm_compile *c,
		       const struct tgsi_full_instruction *inst )
{

   switch (inst->Opcode) {
   case TGSI_OPCODE_ABS:
      emit_op1(c, TGSI_OPCODE_MOV,
	       dst, 
	       brw_abs(src[0]));
      break;

   case TGSI_OPCODE_SUB: 
      emit_op2(c, TGSI_OPCODE_ADD,
	       dst,
	       src[0],
	       brw_negate(src[1]));
      break;

   case TGSI_OPCODE_SCS: 
      emit_op1(c, TGSI_OPCODE_SCS,
	       brw_writemask(dst, BRW_WRITEMASK_XY),
	       src[0]);
      break;
	 
   case TGSI_OPCODE_DST:
      precalc_dst(c, inst);
      break;

   case TGSI_OPCODE_LIT:
      precalc_lit(c, inst);
      break;

   case TGSI_OPCODE_TEX:
      precalc_tex(c, inst);
      break;

   case TGSI_OPCODE_TXP:
      precalc_txp(c, inst);
      break;

   case TGSI_OPCODE_TXB:
      out = emit_insn(c, inst);
      out->TexSrcUnit = fp->program.Base.SamplerUnits[inst->TexSrcUnit];
      assert(out->TexSrcUnit < BRW_MAX_TEX_UNIT);
      break;

   case TGSI_OPCODE_XPD: 
      emit_op2(c, TGSI_OPCODE_XPD,
	       brw_writemask(dst, BRW_WRITEMASK_XYZ),
	       src[0], 
	       src[1]);
      break;

   case TGSI_OPCODE_KIL: 
      emit_op1(c, TGSI_OPCODE_KIL,
	       brw_writemask(dst_undef(), 0),
	       src[0]);
      break;

   case TGSI_OPCODE_END:
      emit_fb_write(c);
      break;
   default:
      if (brw_wm_is_scalar_result(inst->Opcode))
	 emit_scalar_insn(c, opcode, dst, src[0], src[1], src[2]);
      else
	 emit_op(c, opcode, dst, src[0], src[1], src[2]);
      break;
   }
}

/**
 * Initial pass for fragment program code generation.
 * This function is used by both the GLSL and non-GLSL paths.
 */
void brw_wm_pass_fp( struct brw_wm_compile *c )
{
   struct brw_fragment_program *fp = c->fp;
   GLuint insn;

   if (BRW_DEBUG & DEBUG_WM) {
      debug_printf("pre-fp:\n");
      tgsi_dump(fp->tokens, 0); 
   }

   c->pixel_xy = brw_src_undef();
   c->delta_xy = brw_src_undef();
   c->pixel_w = brw_src_undef();
   c->nr_fp_insns = 0;
   c->fp->tex_units_used = 0x0;


   /* Loop over all instructions doing assorted simplifications and
    * transformations.
    */
   tgsi_parse_init( &parse, tokens );
   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
	 /* If branching shader, emit preamble instructions at decl time, as
	  * instruction order in the shader does not correspond to the order
	  * instructions are executed in the wild.
	  *
	  * This is where special instructions such as WM_CINTERP,
	  * WM_LINTERP, WM_PINTERP and WM_WPOSXY are emitted to compute
	  * shader inputs from varying vars.
	  *
	  * XXX: For non-branching shaders, consider deferring variable
	  * initialization as late as possible to minimize register
	  * usage.  This is how the original BRW driver worked.
	  */
	 validate_src_regs(c, inst);
	 validate_dst_regs(c, inst);
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
	 /* Unlike VS programs we can probably manage fine encoding
	  * immediate values directly into the emitted EU
	  * instructions, as we probably only need to reference one
	  * float value per instruction.  Just save the data for now
	  * and use directly later.
	  */
	 break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         inst = &parse.FullToken.FullInstruction;
	 emit_insn( c, inst );
	 break;
      }
   }

   c->brw_program = brw_finalize( c->builder );

   if (BRW_DEBUG & DEBUG_WM) {
      debug_printf("pass_fp:\n");
      brw_print_program( c->brw_program );
      debug_printf("\n");
   }
}

