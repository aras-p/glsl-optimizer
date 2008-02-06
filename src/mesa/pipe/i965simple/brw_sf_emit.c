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


#include "brw_defines.h"
#include "brw_context.h"
#include "brw_eu.h"
#include "brw_util.h"
#include "brw_sf.h"



/***********************************************************************
 * Triangle setup.
 */


static void alloc_regs( struct brw_sf_compile *c )
{
   unsigned reg, i;

   /* Values computed by fixed function unit:
    */
   c->pv  = retype(brw_vec1_grf(1, 1), BRW_REGISTER_TYPE_UD);
   c->det = brw_vec1_grf(1, 2);
   c->dx0 = brw_vec1_grf(1, 3);
   c->dx2 = brw_vec1_grf(1, 4);
   c->dy0 = brw_vec1_grf(1, 5);
   c->dy2 = brw_vec1_grf(1, 6);

   /* z and 1/w passed in seperately:
    */
   c->z[0]     = brw_vec1_grf(2, 0);
   c->inv_w[0] = brw_vec1_grf(2, 1);
   c->z[1]     = brw_vec1_grf(2, 2);
   c->inv_w[1] = brw_vec1_grf(2, 3);
   c->z[2]     = brw_vec1_grf(2, 4);
   c->inv_w[2] = brw_vec1_grf(2, 5);

   /* The vertices:
    */
   reg = 3;
   for (i = 0; i < c->nr_verts; i++) {
      c->vert[i] = brw_vec8_grf(reg, 0);
      reg += c->nr_attr_regs;
   }

   /* Temporaries, allocated after last vertex reg.
    */
   c->inv_det = brw_vec1_grf(reg, 0);  reg++;
   c->a1_sub_a0 = brw_vec8_grf(reg, 0);  reg++;
   c->a2_sub_a0 = brw_vec8_grf(reg, 0);  reg++;
   c->tmp = brw_vec8_grf(reg, 0);  reg++;

   /* Note grf allocation:
    */
   c->prog_data.total_grf = reg;


   /* Outputs of this program - interpolation coefficients for
    * rasterization:
    */
   c->m1Cx = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 1, 0);
   c->m2Cy = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 2, 0);
   c->m3C0 = brw_vec8_reg(BRW_MESSAGE_REGISTER_FILE, 3, 0);
}


static void copy_z_inv_w( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   unsigned i;

   brw_push_insn_state(p);

   /* Copy both scalars with a single MOV:
    */
   for (i = 0; i < c->nr_verts; i++)
      brw_MOV(p, vec2(suboffset(c->vert[i], 2)), vec2(c->z[i]));

   brw_pop_insn_state(p);
}


static void invert_det( struct brw_sf_compile *c)
{
   brw_math(&c->func,
	    c->inv_det,
	    BRW_MATH_FUNCTION_INV,
	    BRW_MATH_SATURATE_NONE,
	    0,
	    c->det,
	    BRW_MATH_DATA_SCALAR,
	    BRW_MATH_PRECISION_FULL);

}

#define NON_PERPECTIVE_ATTRS  (FRAG_BIT_WPOS | \
                               FRAG_BIT_COL0 | \
			       FRAG_BIT_COL1)

static boolean calculate_masks( struct brw_sf_compile *c,
				  unsigned reg,
				  ushort *pc,
				  ushort *pc_persp,
				  ushort *pc_linear)
{
   boolean is_last_attr = (reg == c->nr_setup_regs - 1);
   unsigned persp_mask = c->key.persp_mask;
   unsigned linear_mask = c->key.linear_mask;

   debug_printf("persp_mask: %x\n", persp_mask);
   debug_printf("linear_mask: %x\n", linear_mask);

   *pc_persp = 0;
   *pc_linear = 0;
   *pc = 0xf;

   if (persp_mask & (1 << (reg*2)))
      *pc_persp = 0xf;

   if (linear_mask & (1 << (reg*2)))
      *pc_linear = 0xf;

   /* Maybe only processs one attribute on the final round:
    */
   if (reg*2+1 < c->nr_setup_attrs) {
      *pc |= 0xf0;

      if (persp_mask & (1 << (reg*2+1)))
	 *pc_persp |= 0xf0;

      if (linear_mask & (1 << (reg*2+1)))
	 *pc_linear |= 0xf0;
   }

   debug_printf("pc: %x\n", *pc);
   debug_printf("pc_persp: %x\n", *pc_persp);
   debug_printf("pc_linear: %x\n", *pc_linear);
   

   return is_last_attr;
}



void brw_emit_tri_setup( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   unsigned i;

   debug_printf("%s START ==============\n", __FUNCTION__);

   c->nr_verts = 3;
   alloc_regs(c);
   invert_det(c);
   copy_z_inv_w(c);


   for (i = 0; i < c->nr_setup_regs; i++)
   {
      /* Pair of incoming attributes:
       */
      struct brw_reg a0 = offset(c->vert[0], i);
      struct brw_reg a1 = offset(c->vert[1], i);
      struct brw_reg a2 = offset(c->vert[2], i);
      ushort pc = 0, pc_persp = 0, pc_linear = 0;
      boolean last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);

      if (pc_persp)
      {
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
	 brw_MUL(p, a1, a1, c->inv_w[1]);
	 brw_MUL(p, a2, a2, c->inv_w[2]);
      }


      /* Calculate coefficients for interpolated values:
       */
      if (pc_linear)
      {
	 brw_set_predicate_control_flag_value(p, pc_linear);

	 brw_ADD(p, c->a1_sub_a0, a1, negate(a0));
	 brw_ADD(p, c->a2_sub_a0, a2, negate(a0));

	 /* calculate dA/dx
	  */
	 brw_MUL(p, brw_null_reg(), c->a1_sub_a0, c->dy2);
	 brw_MAC(p, c->tmp, c->a2_sub_a0, negate(c->dy0));
	 brw_MUL(p, c->m1Cx, c->tmp, c->inv_det);

	 /* calculate dA/dy
	  */
	 brw_MUL(p, brw_null_reg(), c->a2_sub_a0, c->dx0);
	 brw_MAC(p, c->tmp, c->a1_sub_a0, negate(c->dx2));
	 brw_MUL(p, c->m2Cy, c->tmp, c->inv_det);
      }

      {
	 brw_set_predicate_control_flag_value(p, pc);
	 /* start point for interpolation
	  */
	 brw_MOV(p, c->m3C0, a0);

	 /* Copy m0..m3 to URB.  m0 is implicitly copied from r0 in
	  * the send instruction:
	  */
	 brw_urb_WRITE(p,
		       brw_null_reg(),
		       0,
		       brw_vec8_grf(0, 0), /* r0, will be copied to m0 */
		       0, 	/* allocate */
		       1,	/* used */
		       4, 	/* msg len */
		       0,	/* response len */
		       last,	/* eot */
		       last, 	/* writes complete */
		       i*4,	/* offset */
		       BRW_URB_SWIZZLE_TRANSPOSE); /* XXX: Swizzle control "SF to windower" */
      }
   }

   debug_printf("%s DONE ==============\n", __FUNCTION__);

}



void brw_emit_line_setup( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   unsigned i;


   c->nr_verts = 2;
   alloc_regs(c);
   invert_det(c);
   copy_z_inv_w(c);

   for (i = 0; i < c->nr_setup_regs; i++)
   {
      /* Pair of incoming attributes:
       */
      struct brw_reg a0 = offset(c->vert[0], i);
      struct brw_reg a1 = offset(c->vert[1], i);
      ushort pc, pc_persp, pc_linear;
      boolean last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);

      if (pc_persp)
      {
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
	 brw_MUL(p, a1, a1, c->inv_w[1]);
      }

      /* Calculate coefficients for position, color:
       */
      if (pc_linear) {
	 brw_set_predicate_control_flag_value(p, pc_linear);

	 brw_ADD(p, c->a1_sub_a0, a1, negate(a0));

 	 brw_MUL(p, c->tmp, c->a1_sub_a0, c->dx0);
	 brw_MUL(p, c->m1Cx, c->tmp, c->inv_det);

	 brw_MUL(p, c->tmp, c->a1_sub_a0, c->dy0);
	 brw_MUL(p, c->m2Cy, c->tmp, c->inv_det);
      }

      {
	 brw_set_predicate_control_flag_value(p, pc);

	 /* start point for interpolation
	  */
	 brw_MOV(p, c->m3C0, a0);

	 /* Copy m0..m3 to URB.
	  */
	 brw_urb_WRITE(p,
		       brw_null_reg(),
		       0,
		       brw_vec8_grf(0, 0),
		       0, 	/* allocate */
		       1, 	/* used */
		       4, 	/* msg len */
		       0,	/* response len */
		       last, 	/* eot */
		       last, 	/* writes complete */
		       i*4,	/* urb destination offset */
		       BRW_URB_SWIZZLE_TRANSPOSE);
      }
   }
}


/* Points setup - several simplifications as all attributes are
 * constant across the face of the point (point sprites excluded!)
 */
void brw_emit_point_setup( struct brw_sf_compile *c )
{
   struct brw_compile *p = &c->func;
   unsigned i;

   c->nr_verts = 1;
   alloc_regs(c);
   copy_z_inv_w(c);

   brw_MOV(p, c->m1Cx, brw_imm_ud(0)); /* zero - move out of loop */
   brw_MOV(p, c->m2Cy, brw_imm_ud(0)); /* zero - move out of loop */

   for (i = 0; i < c->nr_setup_regs; i++)
   {
      struct brw_reg a0 = offset(c->vert[0], i);
      ushort pc, pc_persp, pc_linear;
      boolean last = calculate_masks(c, i, &pc, &pc_persp, &pc_linear);

      if (pc_persp)
      {
	 /* This seems odd as the values are all constant, but the
	  * fragment shader will be expecting it:
	  */
	 brw_set_predicate_control_flag_value(p, pc_persp);
	 brw_MUL(p, a0, a0, c->inv_w[0]);
      }


      /* The delta values are always zero, just send the starting
       * coordinate.  Again, this is to fit in with the interpolation
       * code in the fragment shader.
       */
      {
	 brw_set_predicate_control_flag_value(p, pc);

	 brw_MOV(p, c->m3C0, a0); /* constant value */

	 /* Copy m0..m3 to URB.
	  */
	 brw_urb_WRITE(p,
		       brw_null_reg(),
		       0,
		       brw_vec8_grf(0, 0),
		       0, 	/* allocate */
		       1,	/* used */
		       4, 	/* msg len */
		       0,	/* response len */
		       last, 	/* eot */
		       last, 	/* writes complete */
		       i*4,	/* urb destination offset */
		       BRW_URB_SWIZZLE_TRANSPOSE);
      }
   }
}
