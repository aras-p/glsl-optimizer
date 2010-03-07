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

#include "util/u_math.h"
#include "util/u_memory.h"

#include "tgsi/tgsi_parse.h"
#include "tgsi/tgsi_dump.h"
#include "tgsi/tgsi_info.h"
#include "tgsi/tgsi_util.h"

#include "brw_wm.h"
#include "brw_debug.h"


/***********************************************************************
 * Source regs
 */

static struct brw_fp_src src_reg(GLuint file, GLuint idx)
{
   struct brw_fp_src reg;
   reg.file = file;
   reg.index = idx;
   reg.swizzle = BRW_SWIZZLE_XYZW;
   reg.indirect = 0;
   reg.negate = 0;
   reg.abs = 0;
   return reg;
}

static struct brw_fp_src src_reg_from_dst(struct brw_fp_dst dst)
{
   return src_reg(dst.file, dst.index);
}

static struct brw_fp_src src_undef( void )
{
   return src_reg(TGSI_FILE_NULL, 0);
}

static GLboolean src_is_undef(struct brw_fp_src src)
{
   return src.file == TGSI_FILE_NULL;
}

static struct brw_fp_src src_swizzle( struct brw_fp_src reg, int x, int y, int z, int w )
{
   unsigned swz = reg.swizzle;

   reg.swizzle = ( BRW_GET_SWZ(swz, x) << 0 |
		   BRW_GET_SWZ(swz, y) << 2 |
		   BRW_GET_SWZ(swz, z) << 4 |
		   BRW_GET_SWZ(swz, w) << 6 );

   return reg;
}

static struct brw_fp_src src_scalar( struct brw_fp_src reg, int x )
{
   return src_swizzle(reg, x, x, x, x);
}

static struct brw_fp_src src_abs( struct brw_fp_src src )
{
   src.negate = 0;
   src.abs = 1;
   return src;
}

static struct brw_fp_src src_negate( struct brw_fp_src src )
{
   src.negate = 1;
   src.abs = 0;
   return src;
}


static int match_or_expand_immediate( const float *v,
                                      unsigned nr,
                                      float *v2,
                                      unsigned *nr2,
                                      unsigned *swizzle )
{
   unsigned i, j;
   
   *swizzle = 0;

   for (i = 0; i < nr; i++) {
      boolean found = FALSE;

      for (j = 0; j < *nr2 && !found; j++) {
         if (v[i] == v2[j]) {
            *swizzle |= j << (i * 2);
            found = TRUE;
         }
      }

      if (!found) {
         if (*nr2 >= 4) 
            return FALSE;

         v2[*nr2] = v[i];
         *swizzle |= *nr2 << (i * 2);
         (*nr2)++;
      }
   }

   return TRUE;
}



/* Internally generated immediates: overkill...
 */
static struct brw_fp_src src_imm( struct brw_wm_compile *c, 
				  const GLfloat *v, 
				  unsigned nr)
{
   unsigned i, j;
   unsigned swizzle;

   /* Could do a first pass where we examine all existing immediates
    * without expanding.
    */

   for (i = 0; i < c->nr_immediates; i++) {
      if (match_or_expand_immediate( v, 
                                     nr,
                                     c->immediate[i].v,
                                     &c->immediate[i].nr, 
                                     &swizzle ))
         goto out;
   }

   if (c->nr_immediates < Elements(c->immediate)) {
      i = c->nr_immediates++;
      if (match_or_expand_immediate( v,
                                     nr,
                                     c->immediate[i].v,
                                     &c->immediate[i].nr, 
                                     &swizzle ))
         goto out;
   }

   c->error = 1;
   return src_undef();

out:
   /* Make sure that all referenced elements are from this immediate.
    * Has the effect of making size-one immediates into scalars.
    */
   for (j = nr; j < 4; j++)
      swizzle |= (swizzle & 0x3) << (j * 2);

   return src_swizzle( src_reg( TGSI_FILE_IMMEDIATE, i ),
		       BRW_GET_SWZ(swizzle, X),
		       BRW_GET_SWZ(swizzle, Y),
		       BRW_GET_SWZ(swizzle, Z),
		       BRW_GET_SWZ(swizzle, W) );
}



static struct brw_fp_src src_imm1f( struct brw_wm_compile *c,
				    GLfloat f )
{
   return src_imm(c, &f, 1);
}

static struct brw_fp_src src_imm4f( struct brw_wm_compile *c,
				    GLfloat x,
				    GLfloat y,
				    GLfloat z,
				    GLfloat w)
{
   GLfloat f[4] = {x,y,z,w};
   return src_imm(c, f, 4);
}



/***********************************************************************
 * Dest regs
 */

static struct brw_fp_dst dst_reg(GLuint file, GLuint idx)
{
   struct brw_fp_dst reg;
   reg.file = file;
   reg.index = idx;
   reg.writemask = BRW_WRITEMASK_XYZW;
   reg.indirect = 0;
   reg.saturate = 0;
   return reg;
}

static struct brw_fp_dst dst_mask( struct brw_fp_dst reg, int mask )
{
   reg.writemask &= mask;
   return reg;
}

static struct brw_fp_dst dst_undef( void )
{
   return dst_reg(TGSI_FILE_NULL, 0);
}

static boolean dst_is_undef( struct brw_fp_dst dst )
{
   return dst.file == TGSI_FILE_NULL;
}

static struct brw_fp_dst dst_saturate( struct brw_fp_dst reg, boolean flag )
{
   reg.saturate = flag;
   return reg;
}

static struct brw_fp_dst get_temp( struct brw_wm_compile *c )
{
   int bit = ffs( ~c->fp_temp );

   if (!bit) {
      debug_printf("%s: out of temporaries\n", __FILE__);
   }

   c->fp_temp |= 1<<(bit-1);
   return dst_reg(TGSI_FILE_TEMPORARY, c->fp_first_internal_temp+(bit-1));
}


static void release_temp( struct brw_wm_compile *c, struct brw_fp_dst temp )
{
   c->fp_temp &= ~(1 << (temp.index - c->fp_first_internal_temp));
}


/***********************************************************************
 * Instructions 
 */

static struct brw_fp_instruction *get_fp_inst(struct brw_wm_compile *c)
{
   return &c->fp_instructions[c->nr_fp_insns++];
}

static struct brw_fp_instruction * emit_tex_op(struct brw_wm_compile *c,
					     GLuint op,
					     struct brw_fp_dst dest,
					     GLuint tex_unit,
					     GLuint target,
					     GLuint sampler,
					     struct brw_fp_src src0,
					     struct brw_fp_src src1,
					     struct brw_fp_src src2 )
{
   struct brw_fp_instruction *inst = get_fp_inst(c);

   if (tex_unit || target)
      assert(op == TGSI_OPCODE_TXP ||
             op == TGSI_OPCODE_TXB ||
             op == TGSI_OPCODE_TEX ||
             op == WM_FB_WRITE);

   inst->opcode = op;
   inst->dst = dest;
   inst->tex_unit = tex_unit;
   inst->target = target;
   inst->sampler = sampler;
   inst->src[0] = src0;
   inst->src[1] = src1;
   inst->src[2] = src2;

   return inst;
}
   

static INLINE void emit_op3(struct brw_wm_compile *c,
			    GLuint op,
			    struct brw_fp_dst dest,
			    struct brw_fp_src src0,
			    struct brw_fp_src src1,
			    struct brw_fp_src src2 )
{
   emit_tex_op(c, op, dest, 0, 0, 0, src0, src1, src2);
}


static INLINE void emit_op2(struct brw_wm_compile *c,
			    GLuint op,
			    struct brw_fp_dst dest,
			    struct brw_fp_src src0,
			    struct brw_fp_src src1)
{
   emit_tex_op(c, op, dest, 0, 0, 0, src0, src1, src_undef());
}

static INLINE void emit_op1(struct brw_wm_compile *c,
			    GLuint op,
			    struct brw_fp_dst dest,
			    struct brw_fp_src src0)
{
   emit_tex_op(c, op, dest, 0, 0, 0, src0, src_undef(), src_undef());
}

static INLINE void emit_op0(struct brw_wm_compile *c,
			   GLuint op,
			   struct brw_fp_dst dest)
{
   emit_tex_op(c, op, dest, 0, 0, 0, src_undef(), src_undef(), src_undef());
}



/* Many opcodes produce the same value across all the result channels.
 * We'd rather not have to support that splatting in the opcode implementations,
 * and brw_wm_pass*.c wants to optimize them out by shuffling references around
 * anyway.  We can easily get both by emitting the opcode to one channel, and
 * then MOVing it to the others, which brw_wm_pass*.c already understands.
 */
static void emit_scalar_insn(struct brw_wm_compile *c,
			     unsigned opcode,
			     struct brw_fp_dst dst,
			     struct brw_fp_src src0,
			     struct brw_fp_src src1,
			     struct brw_fp_src src2 )
{
   unsigned first_chan = ffs(dst.writemask) - 1;
   unsigned first_mask = 1 << first_chan;

   if (dst.writemask == 0)
      return;

   emit_op3( c, opcode,
	     dst_mask(dst, first_mask),
	     src0, src1, src2 );

   if (dst.writemask != first_mask) {
      emit_op1(c, TGSI_OPCODE_MOV,
	       dst_mask(dst, ~first_mask),
	       src_scalar(src_reg_from_dst(dst), first_chan));
   }
}


/***********************************************************************
 * Special instructions for interpolation and other tasks
 */

static struct brw_fp_src get_pixel_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->fp_pixel_xy)) {
      struct brw_fp_dst pixel_xy = get_temp(c);
      struct brw_fp_src payload_r0_depth = src_reg(BRW_FILE_PAYLOAD, PAYLOAD_DEPTH);
      
      
      /* Emit the out calculations, and hold onto the results.  Use
       * two instructions as a temporary is required.
       */   
      /* pixel_xy.xy = PIXELXY payload[0];
       */
      emit_op1(c,
	       WM_PIXELXY,
	       dst_mask(pixel_xy, BRW_WRITEMASK_XY),
	       payload_r0_depth);

      c->fp_pixel_xy = src_reg_from_dst(pixel_xy);
   }

   return c->fp_pixel_xy;
}

static struct brw_fp_src get_delta_xy( struct brw_wm_compile *c )
{
   if (src_is_undef(c->fp_delta_xy)) {
      struct brw_fp_dst delta_xy = get_temp(c);
      struct brw_fp_src pixel_xy = get_pixel_xy(c);
      struct brw_fp_src payload_r0_depth = src_reg(BRW_FILE_PAYLOAD, PAYLOAD_DEPTH);
      
      /* deltas.xy = DELTAXY pixel_xy, payload[0]
       */
      emit_op3(c,
	      WM_DELTAXY,
	      dst_mask(delta_xy, BRW_WRITEMASK_XY),
	      pixel_xy, 
	      payload_r0_depth,
	      src_undef());
      
      c->fp_delta_xy = src_reg_from_dst(delta_xy);
   }

   return c->fp_delta_xy;
}

static struct brw_fp_src get_pixel_w( struct brw_wm_compile *c )
{
   if (src_is_undef(c->fp_pixel_w)) {
      struct brw_fp_dst pixel_w = get_temp(c);
      struct brw_fp_src deltas = get_delta_xy(c);

      /* XXX: assuming position is always first -- valid? 
       */
      struct brw_fp_src interp_wpos = src_reg(BRW_FILE_PAYLOAD, 0);

      /* deltas.xyw = DELTAS2 deltas.xy, payload.interp_wpos.x
       */
      emit_op3(c,
	       WM_PIXELW,
	       dst_mask(pixel_w, BRW_WRITEMASK_W),
	       interp_wpos,
	       deltas, 
	       src_undef());
      

      c->fp_pixel_w = src_reg_from_dst(pixel_w);
   }

   return c->fp_pixel_w;
}


/***********************************************************************
 * Emit INTERP instructions ahead of first use of each attrib.
 */

static void emit_interp( struct brw_wm_compile *c,
			 GLuint idx,
			 GLuint semantic,
			 GLuint interp_mode )
{
   struct brw_fp_dst dst = dst_reg(TGSI_FILE_INPUT, idx);
   struct brw_fp_src interp = src_reg(BRW_FILE_PAYLOAD, idx);
   struct brw_fp_src deltas = get_delta_xy(c);

   /* Need to use PINTERP on attributes which have been
    * multiplied by 1/W in the SF program, and LINTERP on those
    * which have not:
    */
   switch (semantic) {
   case TGSI_SEMANTIC_POSITION:
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

   case TGSI_SEMANTIC_FOG:
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
	       src_imm1f(c, 0.0));

      emit_op1(c,
	       TGSI_OPCODE_MOV,
	       dst_mask(dst, BRW_WRITEMASK_W),
	       src_imm1f(c, 1.0));
      break;

   case TGSI_SEMANTIC_FACE:
      /* XXX review/test this case */
      emit_op0(c,
	       WM_FRONTFACING,
	       dst_mask(dst, BRW_WRITEMASK_X));
      
      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_YZ),
	       src_imm1f(c, 0.0));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_W),
	       src_imm1f(c, 1.0));
      break;

   case TGSI_SEMANTIC_PSIZE:
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
	      src_imm1f(c, 0.0f));

      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_W),
	      src_imm1f(c, 1.0f));
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
			 struct brw_fp_dst dst,
			 struct brw_fp_src src0,
			 struct brw_fp_src src1 )
{
   if (dst.writemask & BRW_WRITEMASK_Y) {      
      /* dst.y = mul src0.y, src1.y
       */
      emit_op2(c,
	       TGSI_OPCODE_MUL,
	       dst_mask(dst, BRW_WRITEMASK_Y),
	       src0,
	       src1);
   }

   if (dst.writemask & BRW_WRITEMASK_XZ) {
      /* dst.z = mov src0.zzzz
       */
      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_mask(dst, BRW_WRITEMASK_Z),
	      src_scalar(src0, Z));

      /* dst.x = imm1f(1.0)
       */
      emit_op1(c,
	      TGSI_OPCODE_MOV,
	      dst_saturate(dst_mask(dst, BRW_WRITEMASK_X), 0),
	      src_imm1f(c, 1.0));
   }
   if (dst.writemask & BRW_WRITEMASK_W) {
      /* dst.w = mov src1.w
       */
      emit_op1(c,
	       TGSI_OPCODE_MOV,
	       dst_mask(dst, BRW_WRITEMASK_W),
	       src1);
   }
}


static void precalc_lit( struct brw_wm_compile *c,
			 struct brw_fp_dst dst,
			 struct brw_fp_src src0 )
{
   if (dst.writemask & BRW_WRITEMASK_XW) {
      /* dst.xw = imm(1.0f)
       */
      emit_op1(c,
	       TGSI_OPCODE_MOV,
	       dst_saturate(dst_mask(dst, BRW_WRITEMASK_XW), 0),
	       src_imm1f(c, 1.0f));
   }

   if (dst.writemask & BRW_WRITEMASK_YZ) {
      emit_op1(c,
	       TGSI_OPCODE_LIT,
	       dst_mask(dst, BRW_WRITEMASK_YZ),
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
			 struct brw_fp_dst dst,
			 unsigned target,
			 unsigned unit,
			 struct brw_fp_src src0,
			 struct brw_fp_src sampler )
{
   struct brw_fp_src coord = src_undef();
   struct brw_fp_dst tmp = dst_undef();

   assert(unit < BRW_MAX_TEX_UNIT);

   /* Cubemap: find longest component of coord vector and normalize
    * it.
    */
   if (target == TGSI_TEXTURE_CUBE) {
      struct brw_fp_src tmpsrc;

      tmp = get_temp(c);
      tmpsrc = src_reg_from_dst(tmp);

      /* tmp = abs(src0) */
      emit_op1(c, 
	       TGSI_OPCODE_MOV,
	       tmp,
	       src_abs(src0));

      /* tmp.X = MAX(tmp.X, tmp.Y) */
      emit_op2(c, TGSI_OPCODE_MAX,
	       dst_mask(tmp, BRW_WRITEMASK_X),
	       src_scalar(tmpsrc, X),
	       src_scalar(tmpsrc, Y));

      /* tmp.X = MAX(tmp.X, tmp.Z) */
      emit_op2(c, TGSI_OPCODE_MAX,
	       dst_mask(tmp, BRW_WRITEMASK_X),
	       tmpsrc,
	       src_scalar(tmpsrc, Z));

      /* tmp.X = 1 / tmp.X */
      emit_op1(c, TGSI_OPCODE_RCP,
	      dst_mask(tmp, BRW_WRITEMASK_X),
	      tmpsrc);

      /* tmp = src0 * tmp.xxxx */
      emit_op2(c, TGSI_OPCODE_MUL,
	       tmp,
	       src0,
	       src_scalar(tmpsrc, X));

      coord = tmpsrc;
   }
   else if (target == TGSI_TEXTURE_RECT ||
	    target == TGSI_TEXTURE_SHADOWRECT) {
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
      struct brw_fp_dst tmp = get_temp(c);
      struct brw_fp_src tmpsrc = src_reg_from_dst(tmp);
      struct brw_fp_src C0 = src_imm4f( c,  -.5, -.0625, -.5, 1.164 );
      struct brw_fp_src C1 = src_imm4f( c, 1.596, -0.813, 2.018, -.391 );
     
      /* tmp     = TEX ...
       */
      emit_tex_op(c, 
                  TGSI_OPCODE_TEX,
                  dst_saturate(tmp, dst.saturate),
                  unit,
                  target,
                  sampler.index,
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
	       src_scalar(C0, W));

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
	       src_scalar(tmpsrc, Y));

      /*  RGB.y   = MAD YUV.z, C1.w, RGB.y
       */
      emit_op3(c,
	       TGSI_OPCODE_MAD,
	       dst_mask(dst, BRW_WRITEMASK_Y),
	       src_scalar(tmpsrc, Z),
	       src_scalar(C1, W),
	       src_scalar(src_reg_from_dst(dst), Y));

      release_temp(c, tmp);
   }
   else {
      /* ordinary RGBA tex instruction */
      emit_tex_op(c, 
                  TGSI_OPCODE_TEX,
                  dst,
                  unit,
                  target,
                  sampler.index,
                  coord,
                  src_undef(),
                  src_undef());
   }

   /* XXX: add GL_EXT_texture_swizzle support to gallium -- by
    * generating shader varients in mesa state tracker.
    */

   /* Release this temp if we ended up allocating it:
    */
   if (!dst_is_undef(tmp))
      release_temp(c, tmp);
}


/**
 * Check if the given TXP instruction really needs the divide-by-W step.
 */
static GLboolean projtex( struct brw_wm_compile *c,
			  unsigned target, 
			  struct brw_fp_src src )
{
   /* Only try to detect the simplest cases.  Could detect (later)
    * cases where we are trying to emit code like RCP {1.0}, MUL x,
    * {1.0}, and so on.
    *
    * More complex cases than this typically only arise from
    * user-provided fragment programs anyway:
    */
   if (target == TGSI_TEXTURE_CUBE)
      return GL_FALSE;  /* ut2004 gun rendering !?! */
   
   if (src.file == TGSI_FILE_INPUT && 
       BRW_GET_SWZ(src.swizzle, W) == W &&
       c->fp->info.input_interpolate[src.index] != TGSI_INTERPOLATE_PERSPECTIVE)
      return GL_FALSE;

   return GL_TRUE;
}


/**
 * Emit code for TXP.
 */
static void precalc_txp( struct brw_wm_compile *c,
			 struct brw_fp_dst dst,
			 unsigned target,
			 unsigned unit,
			 struct brw_fp_src src0,
                         struct brw_fp_src sampler )
{
   if (projtex(c, target, src0)) {
      struct brw_fp_dst tmp = get_temp(c);

      /* tmp0.w = RCP inst.arg[0][3]
       */
      emit_op1(c,
	      TGSI_OPCODE_RCP,
	      dst_mask(tmp, BRW_WRITEMASK_W),
	      src_scalar(src0, W));

      /* tmp0.xyz =  MUL inst.arg[0], tmp0.wwww
       */
      emit_op2(c,
	       TGSI_OPCODE_MUL,
	       dst_mask(tmp, BRW_WRITEMASK_XYZ),
	       src0,
	       src_scalar(src_reg_from_dst(tmp), W));

      /* dst = TEX tmp0
       */
      precalc_tex(c, 
		  dst,
		  target,
		  unit,
		  src_reg_from_dst(tmp),
                  sampler );

      release_temp(c, tmp);
   }
   else
   {
      /* dst = TEX src0
       */
      precalc_tex(c, dst, target, unit, src0, sampler);
   }
}


/* XXX: note this returns a src_reg.
 */
static struct brw_fp_src
find_output_by_semantic( struct brw_wm_compile *c,
			 unsigned semantic,
			 unsigned index )
{
   const struct tgsi_shader_info *info = &c->fp->info;
   unsigned i;

   for (i = 0; i < info->num_outputs; i++)
      if (info->output_semantic_name[i] == semantic &&
	  info->output_semantic_index[i] == index)
	 return src_reg( TGSI_FILE_OUTPUT, i );

   /* If not found, return some arbitrary immediate value:
    *
    * XXX: this is a good idea but immediates are up generating extra
    * curbe entries atm, as they would have in the original driver.
    */
   return src_reg( TGSI_FILE_OUTPUT, 0 ); /* src_imm1f(c, 1.0); */
}


static void emit_fb_write( struct brw_wm_compile *c )
{
   struct brw_fp_src payload_r0_depth = src_reg(BRW_FILE_PAYLOAD, PAYLOAD_DEPTH);
   struct brw_fp_src outdepth = find_output_by_semantic(c, TGSI_SEMANTIC_POSITION, 0);
   GLuint i;


   outdepth = src_scalar(outdepth, Z);

   for (i = 0 ; i < c->key.nr_cbufs; i++) {
      struct brw_fp_src outcolor;
      
      outcolor = find_output_by_semantic(c, TGSI_SEMANTIC_COLOR, i);

      /* Use emit_tex_op so that we can specify the inst->target
       * field, which is abused to contain the FB write target and the
       * EOT marker
       */
      emit_tex_op(c, WM_FB_WRITE,
		  dst_undef(),
		  (i == c->key.nr_cbufs - 1), /* EOT */
		  i,
                  0,            /* no sampler */
		  outcolor,
		  payload_r0_depth,
		  outdepth);
   }
}


static struct brw_fp_dst translate_dst( struct brw_wm_compile *c,
					const struct tgsi_full_dst_register *dst,
					unsigned saturate )
{
   struct brw_fp_dst out;

   out.file = dst->Register.File;
   out.index = dst->Register.Index;
   out.writemask = dst->Register.WriteMask;
   out.indirect = dst->Register.Indirect;
   out.saturate = (saturate == TGSI_SAT_ZERO_ONE);
   
   if (out.indirect) {
      assert(dst->Indirect.File == TGSI_FILE_ADDRESS);
      assert(dst->Indirect.Index == 0);
   }
   
   return out;
}


static struct brw_fp_src translate_src( struct brw_wm_compile *c,
					const struct tgsi_full_src_register *src )
{
   struct brw_fp_src out;

   out.file = src->Register.File;
   out.index = src->Register.Index;
   out.indirect = src->Register.Indirect;

   out.swizzle = ((src->Register.SwizzleX << 0) |
		  (src->Register.SwizzleY << 2) |
		  (src->Register.SwizzleZ << 4) |
		  (src->Register.SwizzleW << 6));
   
   switch (tgsi_util_get_full_src_register_sign_mode( src, 0 )) {
   case TGSI_UTIL_SIGN_CLEAR:
      out.abs = 1;
      out.negate = 0;
      break;

   case TGSI_UTIL_SIGN_SET:
      out.abs = 1;
      out.negate = 1;
      break;

   case TGSI_UTIL_SIGN_TOGGLE:
      out.abs = 0;
      out.negate = 1;
      break;

   case TGSI_UTIL_SIGN_KEEP:
   default:
      out.abs = 0;
      out.negate = 0;
      break;
   }

   if (out.indirect) {
      assert(src->Indirect.File == TGSI_FILE_ADDRESS);
      assert(src->Indirect.Index == 0);
   }
   
   return out;
}



static void emit_insn( struct brw_wm_compile *c,
		       const struct tgsi_full_instruction *inst )
{
   unsigned opcode = inst->Instruction.Opcode;
   struct brw_fp_dst dst;
   struct brw_fp_src src[3];
   int i;

   dst = translate_dst( c, &inst->Dst[0],
			inst->Instruction.Saturate );

   for (i = 0; i < inst->Instruction.NumSrcRegs; i++)
      src[i] = translate_src( c, &inst->Src[i] );
   
   switch (opcode) {
   case TGSI_OPCODE_ABS:
      emit_op1(c, TGSI_OPCODE_MOV,
	       dst, 
	       src_abs(src[0]));
      break;

   case TGSI_OPCODE_SUB: 
      emit_op2(c, TGSI_OPCODE_ADD,
	       dst,
	       src[0],
	       src_negate(src[1]));
      break;

   case TGSI_OPCODE_SCS: 
      emit_op1(c, TGSI_OPCODE_SCS,
	       dst_mask(dst, BRW_WRITEMASK_XY),
	       src[0]);
      break;
	 
   case TGSI_OPCODE_DST:
      precalc_dst(c, dst, src[0], src[1]);
      break;

   case TGSI_OPCODE_LIT:
      precalc_lit(c, dst, src[0]);
      break;

   case TGSI_OPCODE_TEX:
      precalc_tex(c, dst,
		  inst->Texture.Texture,
		  src[1].index,	/* use sampler unit for tex idx */
		  src[0],       /* coord */
                  src[1]);      /* sampler */
      break;

   case TGSI_OPCODE_TXP:
      precalc_txp(c, dst,
		  inst->Texture.Texture,
		  src[1].index,	/* use sampler unit for tex idx */
		  src[0],       /* coord */
                  src[1]);      /* sampler */
      break;

   case TGSI_OPCODE_TXB:
      /* XXX: TXB not done
       */
      precalc_tex(c, dst,
		  inst->Texture.Texture,
		  src[1].index,	/* use sampler unit for tex idx*/
		  src[0],
                  src[1]);
      break;

   case TGSI_OPCODE_XPD: 
      emit_op2(c, TGSI_OPCODE_XPD,
	       dst_mask(dst, BRW_WRITEMASK_XYZ),
	       src[0], 
	       src[1]);
      break;

   case TGSI_OPCODE_KIL: 
      emit_op1(c, TGSI_OPCODE_KIL,
	       dst_mask(dst_undef(), 0),
	       src[0]);
      break;

   case TGSI_OPCODE_END:
      emit_fb_write(c);
      break;
   default:
      if (!c->key.has_flow_control &&
	  brw_wm_is_scalar_result(opcode))
	 emit_scalar_insn(c, opcode, dst, src[0], src[1], src[2]);
      else
	 emit_op3(c, opcode, dst, src[0], src[1], src[2]);
      break;
   }
}

/**
 * Initial pass for fragment program code generation.
 * This function is used by both the GLSL and non-GLSL paths.
 */
int brw_wm_pass_fp( struct brw_wm_compile *c )
{
   struct brw_fragment_shader *fs = c->fp;
   struct tgsi_parse_context parse;
   struct tgsi_full_instruction *inst;
   struct tgsi_full_declaration *decl;
   const float *imm;
   GLuint size;
   GLuint i;

   if (BRW_DEBUG & DEBUG_WM) {
      debug_printf("pre-fp:\n");
      tgsi_dump(fs->tokens, 0); 
   }

   c->fp_pixel_xy = src_undef();
   c->fp_delta_xy = src_undef();
   c->fp_pixel_w = src_undef();
   c->nr_fp_insns = 0;
   c->nr_immediates = 0;


   /* Loop over all instructions doing assorted simplifications and
    * transformations.
    */
   tgsi_parse_init( &parse, fs->tokens );
   while( !tgsi_parse_end_of_tokens( &parse ) ) {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
	 /* Turn intput declarations into special WM_* instructions.
	  *
	  * XXX: For non-branching shaders, consider deferring variable
	  * initialization as late as possible to minimize register
	  * usage.  This is how the original BRW driver worked.
	  *
	  * In a branching shader, must preamble instructions at decl
	  * time, as instruction order in the shader does not
	  * correspond to the order instructions are executed in the
	  * wild.
	  *
	  * This is where special instructions such as WM_CINTERP,
	  * WM_LINTERP, WM_PINTERP and WM_WPOSXY are emitted to
	  * compute shader inputs from the payload registers and pixel
	  * position.
	  */
         decl = &parse.FullToken.FullDeclaration;
         if( decl->Declaration.File == TGSI_FILE_INPUT ) {
            unsigned first, last, mask;
            unsigned attrib;

            first = decl->Range.First;
            last = decl->Range.Last;
            mask = decl->Declaration.UsageMask;

            for (attrib = first; attrib <= last; attrib++) {
	       emit_interp(c, 
			   attrib, 
			   decl->Semantic.Name,
			   decl->Declaration.Interpolate );
            }
         }
	 
         break;

      case TGSI_TOKEN_TYPE_IMMEDIATE:
	 /* Unlike VS programs we can probably manage fine encoding
	  * immediate values directly into the emitted EU
	  * instructions, as we probably only need to reference one
	  * float value per instruction.  Just save the data for now
	  * and use directly later.
	  */
	 i = c->nr_immediates++;
	 imm = &parse.FullToken.FullImmediate.u[i].Float;
	 size = parse.FullToken.FullImmediate.Immediate.NrTokens - 1;

	 if (c->nr_immediates >= BRW_WM_MAX_CONST)
	    return PIPE_ERROR_OUT_OF_MEMORY;

	 for (i = 0; i < size; i++)
	    c->immediate[c->nr_immediates].v[i] = imm[i];

	 for (; i < 4; i++)
	    c->immediate[c->nr_immediates].v[i] = 0.0;

	 c->immediate[c->nr_immediates].nr = size;
	 c->nr_immediates++;
	 break;

      case TGSI_TOKEN_TYPE_INSTRUCTION:
         inst = &parse.FullToken.FullInstruction;
	 emit_insn(c, inst);
	 break;
      }
   }

   if (BRW_DEBUG & DEBUG_WM) {
      brw_wm_print_fp_program( c, "pass_fp" );
      debug_printf("\n");
   }

   return c->error;
}

