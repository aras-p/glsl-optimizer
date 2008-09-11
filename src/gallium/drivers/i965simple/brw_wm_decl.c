
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_wm.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "pipe/p_shader_tokens.h"
#include "tgsi/tgsi_parse.h"

static struct brw_reg alloc_tmp(struct brw_wm_compile *c)
{
   c->tmp_index++;
   c->reg_index = MAX2(c->reg_index, c->tmp_start + c->tmp_index);
   return brw_vec8_grf(c->tmp_start + c->tmp_index, 0);
}

static void release_tmps(struct brw_wm_compile *c)
{
   c->tmp_index = 0;
}



static int is_null( struct brw_reg reg )
{
   return (reg.file == BRW_ARCHITECTURE_REGISTER_FILE &&
	   reg.nr == BRW_ARF_NULL);
}

static void emit_pixel_xy( struct brw_wm_compile *c )
{
   if (is_null(c->pixel_xy[0])) {

      struct brw_compile *p = &c->func;
      struct brw_reg r1_uw = retype(brw_vec1_grf(1, 0), BRW_REGISTER_TYPE_UW);

      c->pixel_xy[0] = vec8(retype(alloc_tmp(c), BRW_REGISTER_TYPE_UW));
      c->pixel_xy[1] = vec8(retype(alloc_tmp(c), BRW_REGISTER_TYPE_UW));

      /* Calculate pixel centers by adding 1 or 0 to each of the
       * micro-tile coordinates passed in r1.
       */
      brw_ADD(p,
	      c->pixel_xy[0],
	      stride(suboffset(r1_uw, 4), 2, 4, 0),
	      brw_imm_v(0x10101010));

      brw_ADD(p,
	      c->pixel_xy[1],
	      stride(suboffset(r1_uw, 5), 2, 4, 0),
	      brw_imm_v(0x11001100));
   }
}






static void emit_delta_xy( struct brw_wm_compile *c )
{
   if (is_null(c->delta_xy[0])) {
      struct brw_compile *p = &c->func;
      struct brw_reg r1 = brw_vec1_grf(1, 0);

      emit_pixel_xy(c);

      c->delta_xy[0] = alloc_tmp(c);
      c->delta_xy[1] = alloc_tmp(c);

      /* Calc delta X,Y by subtracting origin in r1 from the pixel
       * centers.
       */
      brw_ADD(p,
	      c->delta_xy[0],
	      retype(c->pixel_xy[0], BRW_REGISTER_TYPE_UW),
	      negate(r1));

      brw_ADD(p,
	      c->delta_xy[1],
	      retype(c->pixel_xy[1], BRW_REGISTER_TYPE_UW),
	      negate(suboffset(r1,1)));
   }
}



#if 0
static void emit_pixel_w( struct brw_wm_compile *c )
{
   if (is_null(c->pixel_w)) {
      struct brw_compile *p = &c->func;

      struct brw_reg interp_wpos = c->coef_wpos;
      
      c->pixel_w = alloc_tmp(c);

      emit_delta_xy(c);

      /* Calc 1/w - just linterp wpos[3] optimized by putting the
       * result straight into a message reg.
       */
      struct brw_reg interp3 = brw_vec1_grf(interp_wpos.nr+1, 4);
      brw_LINE(p, brw_null_reg(), interp3, c->delta_xy[0]);
      brw_MAC(p, brw_message_reg(2), suboffset(interp3, 1), c->delta_xy[1]);

      /* Calc w */
      brw_math_16( p, 
		   c->pixel_w,
		   BRW_MATH_FUNCTION_INV,
		   BRW_MATH_SATURATE_NONE,
		   2, 
		   brw_null_reg(),
		   BRW_MATH_PRECISION_FULL);
   }
}
#endif


static void emit_cinterp(struct brw_wm_compile *c,
			 int idx,
			 int mask )
{
   struct brw_compile *p = &c->func;
   struct brw_reg interp[4];
   struct brw_reg coef = c->payload_coef[idx];
   int i;

   interp[0] = brw_vec1_grf(coef.nr, 0);
   interp[1] = brw_vec1_grf(coef.nr, 4);
   interp[2] = brw_vec1_grf(coef.nr+1, 0);
   interp[3] = brw_vec1_grf(coef.nr+1, 4);

   for(i = 0; i < 4; i++ ) {
      if (mask & (1<<i)) {
	 struct brw_reg dst = c->wm_regs[TGSI_FILE_INPUT][idx][i];
	 brw_MOV(p, dst, suboffset(interp[i],3));
      }
   }
}

static void emit_linterp(struct brw_wm_compile *c,
			 int idx,
			 int mask )
{
   struct brw_compile *p = &c->func;
   struct brw_reg interp[4];
   struct brw_reg coef = c->payload_coef[idx];
   int i;

   emit_delta_xy(c);

   interp[0] = brw_vec1_grf(coef.nr, 0);
   interp[1] = brw_vec1_grf(coef.nr, 4);
   interp[2] = brw_vec1_grf(coef.nr+1, 0);
   interp[3] = brw_vec1_grf(coef.nr+1, 4);

   for(i = 0; i < 4; i++ ) {
      if (mask & (1<<i)) {
	 struct brw_reg dst = c->wm_regs[TGSI_FILE_INPUT][idx][i];
	 brw_LINE(p, brw_null_reg(), interp[i], c->delta_xy[0]);
	 brw_MAC(p, dst, suboffset(interp[i],1), c->delta_xy[1]);
      }
   }
}

#if 0
static void emit_pinterp(struct brw_wm_compile *c,
			 int idx,
			 int mask )
{
   struct brw_compile *p = &c->func;
   struct brw_reg interp[4];
   struct brw_reg coef = c->payload_coef[idx];
   int i;

   get_delta_xy(c);
   get_pixel_w(c);

   interp[0] = brw_vec1_grf(coef.nr, 0);
   interp[1] = brw_vec1_grf(coef.nr, 4);
   interp[2] = brw_vec1_grf(coef.nr+1, 0);
   interp[3] = brw_vec1_grf(coef.nr+1, 4);

   for(i = 0; i < 4; i++ ) {
      if (mask & (1<<i)) {
	 struct brw_reg dst = allocate_reg(c, TGSI_FILE_INPUT, idx, i);
	 brw_LINE(p, brw_null_reg(), interp[i], c->delta_xy[0]);
	 brw_MAC(p, dst, suboffset(interp[i],1), c->delta_xy[1]);
	 brw_MUL(p, dst, dst, c->pixel_w);
      }
   }
}
#endif



#if 0
static void emit_wpos( )
{ 
   struct prog_dst_register dst = dst_reg(PROGRAM_INPUT, idx);
   struct tgsi_full_src_register interp = src_reg(PROGRAM_PAYLOAD, idx);
   struct tgsi_full_src_register deltas = get_delta_xy(c);
   struct tgsi_full_src_register arg2;
   unsigned opcode;

   opcode = WM_LINTERP;
   arg2 = src_undef();

   /* Have to treat wpos.xy specially:
    */
   emit_op(c,
	   WM_WPOSXY,
	   dst_mask(dst, WRITEMASK_XY),
	   0, 0, 0,
	   get_pixel_xy(c),
	   src_undef(),
	   src_undef());
      
   dst = dst_mask(dst, WRITEMASK_ZW);

   /* PROGRAM_INPUT.attr.xyzw = INTERP payload.interp[attr].x, deltas.xyw
    */
   emit_op(c,
	   WM_LINTERP,
	   dst,
	   0, 0, 0,
	   interp,
	   deltas,
	   arg2);
}
#endif




/* Perform register allocation:
 * 
 *  -- r0???
 *  -- passthrough depth regs (and stencil/aa??)
 *  -- curbe ??
 *  -- inputs (coefficients)
 *
 * Use a totally static register allocation.  This will perform poorly
 * but is an easy way to get started (again).
 */
static void prealloc_reg(struct brw_wm_compile *c)
{
   int i, j;
   int nr_curbe_regs = 0;

   /* R0, then some depth related regs:
    */
   for (i = 0; i < c->key.nr_depth_regs; i++) {
      c->payload_depth[i] =  brw_vec8_grf(i*2, 0);
      c->reg_index += 2;
   }


   /* Then a copy of our part of the CURBE entry:
    */
   {
      int nr_constants = c->fp->info.file_max[TGSI_FILE_CONSTANT] + 1;
      int index = 0;

      /* XXX number of constants, or highest numbered constant? */
      assert(nr_constants == c->fp->info.file_count[TGSI_FILE_CONSTANT]);

      c->prog_data.max_const = 4*nr_constants;
      for (i = 0; i < nr_constants; i++) {
	 for (j = 0; j < 4; j++, index++) 
	    c->wm_regs[TGSI_FILE_CONSTANT][i][j] = brw_vec1_grf(c->reg_index + index/8,
								index%8);
      }

      nr_curbe_regs = 2*((4*nr_constants+15)/16);
      c->reg_index += nr_curbe_regs;
   }

   /* Adjust for parameter coefficients for position, which are
    * currently always provided.
    */
//   c->position_coef[i] = brw_vec8_grf(c->reg_index, 0);
   c->reg_index += 2;

   /* Next we receive the plane coefficients for parameter
    * interpolation:
    */
   assert(c->fp->info.file_max[TGSI_FILE_INPUT] == c->fp->info.num_inputs);
   for (i = 0; i < c->fp->info.file_max[TGSI_FILE_INPUT] + 1; i++) {
      c->payload_coef[i] = brw_vec8_grf(c->reg_index, 0);
      c->reg_index += 2;
   }

   c->prog_data.first_curbe_grf = c->key.nr_depth_regs * 2;
   c->prog_data.urb_read_length = (c->fp->info.num_inputs + 1) * 2;
   c->prog_data.curb_read_length = nr_curbe_regs;

   /* That's the end of the payload, now we can start allocating registers.
    */
   c->emit_mask_reg = brw_uw1_reg(BRW_GENERAL_REGISTER_FILE, c->reg_index, 0);
   c->reg_index++;

   c->stack = brw_uw16_reg(BRW_GENERAL_REGISTER_FILE, c->reg_index, 0);
   c->reg_index += 2;

   /* Now allocate room for the interpolated inputs and staging
    * registers for the outputs:
    */
   /* XXX do we want to loop over the _number_ of inputs/outputs or loop
    * to the highest input/output index that's used?
    *  Probably the same, actually.
    */
   assert(c->fp->info.file_max[TGSI_FILE_INPUT] + 1 == c->fp->info.num_inputs);
   assert(c->fp->info.file_max[TGSI_FILE_OUTPUT] + 1 == c->fp->info.num_outputs);
   for (i = 0; i < c->fp->info.file_max[TGSI_FILE_INPUT] + 1; i++) 
      for (j = 0; j < 4; j++)
	 c->wm_regs[TGSI_FILE_INPUT][i][j] = brw_vec8_grf( c->reg_index++, 0 );

   for (i = 0; i < c->fp->info.file_max[TGSI_FILE_OUTPUT] + 1; i++) 
      for (j = 0; j < 4; j++)
	 c->wm_regs[TGSI_FILE_OUTPUT][i][j] = brw_vec8_grf( c->reg_index++, 0 );

   /* Beyond this we should only need registers for internal temporaries:
    */
   c->tmp_start = c->reg_index;
}





/* Need to interpolate fragment program inputs in as a preamble to the
 * shader.  A more sophisticated compiler would do this on demand, but
 * we'll do it up front:
 */
void brw_wm_emit_decls(struct brw_wm_compile *c)
{
   struct tgsi_parse_context parse;
   int done = 0;

   prealloc_reg(c);

   tgsi_parse_init( &parse, c->fp->program.tokens );

   while( !done &&
	  !tgsi_parse_end_of_tokens( &parse ) ) 
   {
      tgsi_parse_token( &parse );

      switch( parse.FullToken.Token.Type ) {
      case TGSI_TOKEN_TYPE_DECLARATION:
      {
	 const struct tgsi_full_declaration *decl = &parse.FullToken.FullDeclaration;
	 unsigned first = decl->DeclarationRange.First;
	 unsigned last = decl->DeclarationRange.Last;
	 unsigned mask = decl->Declaration.UsageMask; /* ? */
	 unsigned i;

	 if (decl->Declaration.File != TGSI_FILE_INPUT)
	    break;

	 for( i = first; i <= last; i++ ) {
	    switch (decl->Declaration.Interpolate) {
	    case TGSI_INTERPOLATE_CONSTANT:
	       emit_cinterp(c, i, mask);
	       break;

	    case TGSI_INTERPOLATE_LINEAR:
	       emit_linterp(c, i, mask);
	       break;

	    case TGSI_INTERPOLATE_PERSPECTIVE:
	       //emit_pinterp(c, i, mask);
	       emit_linterp(c, i, mask);
	       break;
	    }
	 }
	 break;
      }
      case TGSI_TOKEN_TYPE_IMMEDIATE:
      case TGSI_TOKEN_TYPE_INSTRUCTION:
      default:
         done = 1;
	 break;
      }
   }

   tgsi_parse_free (&parse);
   
   release_tmps(c);
}
