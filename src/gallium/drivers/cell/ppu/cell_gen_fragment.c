/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2009 VMware, Inc.  All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/

/**
 * Generate SPU per-fragment code (actually per-quad code).
 * \author Brian Paul
 * \author Bob Ellison
 */


#include "pipe/p_defines.h"
#include "pipe/p_state.h"
#include "rtasm/rtasm_ppc_spe.h"
#include "cell_context.h"
#include "cell_gen_fragment.h"



/** Do extra optimizations? */
#define OPTIMIZATIONS 1


/**
 * Generate SPE code to perform Z/depth testing.
 *
 * \param dsa         Gallium depth/stencil/alpha state to gen code for
 * \param f           SPE function to append instruction onto.
 * \param mask_reg    register containing quad/pixel "alive" mask (in/out)
 * \param ifragZ_reg  register containing integer fragment Z values (in)
 * \param ifbZ_reg    register containing integer frame buffer Z values (in/out)
 * \param zmask_reg   register containing result of Z test/comparison (out)
 *
 * Returns TRUE if the Z-buffer needs to be updated.
 */
static boolean
gen_depth_test(struct spe_function *f,
               const struct pipe_depth_stencil_alpha_state *dsa,
               int mask_reg, int ifragZ_reg, int ifbZ_reg, int zmask_reg)
{
   /* NOTE: we use clgt below, not cgt, because we want to compare _unsigned_
    * quantities.  This only makes a difference for 32-bit Z values though.
    */
   ASSERT(dsa->depth.enabled);

   switch (dsa->depth.func) {
   case PIPE_FUNC_EQUAL:
      /* zmask = (ifragZ == ref) */
      spe_ceq(f, zmask_reg, ifragZ_reg, ifbZ_reg);
      /* mask = (mask & zmask) */
      spe_and(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_NOTEQUAL:
      /* zmask = (ifragZ == ref) */
      spe_ceq(f, zmask_reg, ifragZ_reg, ifbZ_reg);
      /* mask = (mask & ~zmask) */
      spe_andc(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_GREATER:
      /* zmask = (ifragZ > ref) */
      spe_clgt(f, zmask_reg, ifragZ_reg, ifbZ_reg);
      /* mask = (mask & zmask) */
      spe_and(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_LESS:
      /* zmask = (ref > ifragZ) */
      spe_clgt(f, zmask_reg, ifbZ_reg, ifragZ_reg);
      /* mask = (mask & zmask) */
      spe_and(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_LEQUAL:
      /* zmask = (ifragZ > ref) */
      spe_clgt(f, zmask_reg, ifragZ_reg, ifbZ_reg);
      /* mask = (mask & ~zmask) */
      spe_andc(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_GEQUAL:
      /* zmask = (ref > ifragZ) */
      spe_clgt(f, zmask_reg, ifbZ_reg, ifragZ_reg);
      /* mask = (mask & ~zmask) */
      spe_andc(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_NEVER:
      spe_il(f, mask_reg, 0);  /* mask = {0,0,0,0} */
      spe_move(f, zmask_reg, mask_reg);  /* zmask = mask */
      break;

   case PIPE_FUNC_ALWAYS:
      /* mask unchanged */
      spe_il(f, zmask_reg, ~0);  /* zmask = {~0,~0,~0,~0} */
      break;

   default:
      ASSERT(0);
      break;
   }

   if (dsa->depth.writemask) {
      /*
       * If (ztest passed) {
       *    framebufferZ = fragmentZ;
       * }
       * OR,
       * framebufferZ = (ztest_passed ? fragmentZ : framebufferZ;
       */
      spe_selb(f, ifbZ_reg, ifbZ_reg, ifragZ_reg, mask_reg);
      return TRUE;
   }

   return FALSE;
}


/**
 * Generate SPE code to perform alpha testing.
 *
 * \param dsa        Gallium depth/stencil/alpha state to gen code for
 * \param f          SPE function to append instruction onto.
 * \param mask_reg   register containing quad/pixel "alive" mask (in/out)
 * \param fragA_reg  register containing four fragment alpha values (in)
 */
static void
gen_alpha_test(const struct pipe_depth_stencil_alpha_state *dsa,
               struct spe_function *f, int mask_reg, int fragA_reg)
{
   int ref_reg = spe_allocate_available_register(f);
   int amask_reg = spe_allocate_available_register(f);

   ASSERT(dsa->alpha.enabled);

   if ((dsa->alpha.func != PIPE_FUNC_NEVER) &&
       (dsa->alpha.func != PIPE_FUNC_ALWAYS)) {
      /* load/splat the alpha reference float value */
      spe_load_float(f, ref_reg, dsa->alpha.ref_value);
   }

   /* emit code to do the alpha comparison, updating 'mask' */
   switch (dsa->alpha.func) {
   case PIPE_FUNC_EQUAL:
      /* amask = (fragA == ref) */
      spe_fceq(f, amask_reg, fragA_reg, ref_reg);
      /* mask = (mask & amask) */
      spe_and(f, mask_reg, mask_reg, amask_reg);
      break;

   case PIPE_FUNC_NOTEQUAL:
      /* amask = (fragA == ref) */
      spe_fceq(f, amask_reg, fragA_reg, ref_reg);
      /* mask = (mask & ~amask) */
      spe_andc(f, mask_reg, mask_reg, amask_reg);
      break;

   case PIPE_FUNC_GREATER:
      /* amask = (fragA > ref) */
      spe_fcgt(f, amask_reg, fragA_reg, ref_reg);
      /* mask = (mask & amask) */
      spe_and(f, mask_reg, mask_reg, amask_reg);
      break;

   case PIPE_FUNC_LESS:
      /* amask = (ref > fragA) */
      spe_fcgt(f, amask_reg, ref_reg, fragA_reg);
      /* mask = (mask & amask) */
      spe_and(f, mask_reg, mask_reg, amask_reg);
      break;

   case PIPE_FUNC_LEQUAL:
      /* amask = (fragA > ref) */
      spe_fcgt(f, amask_reg, fragA_reg, ref_reg);
      /* mask = (mask & ~amask) */
      spe_andc(f, mask_reg, mask_reg, amask_reg);
      break;

   case PIPE_FUNC_GEQUAL:
      /* amask = (ref > fragA) */
      spe_fcgt(f, amask_reg, ref_reg, fragA_reg);
      /* mask = (mask & ~amask) */
      spe_andc(f, mask_reg, mask_reg, amask_reg);
      break;

   case PIPE_FUNC_NEVER:
      spe_il(f, mask_reg, 0);  /* mask = [0,0,0,0] */
      break;

   case PIPE_FUNC_ALWAYS:
      /* no-op, mask unchanged */
      break;

   default:
      ASSERT(0);
      break;
   }

#if OPTIMIZATIONS
   /* if mask == {0,0,0,0} we're all done, return */
   {
      /* re-use amask reg here */
      int tmp_reg = amask_reg;
      /* tmp[0] = (mask[0] | mask[1] | mask[2] | mask[3]) */
      spe_orx(f, tmp_reg, mask_reg);
      /* if tmp[0] == 0 then return from function call */
      spe_biz(f, tmp_reg, SPE_REG_RA, 0, 0);
   }
#endif

   spe_release_register(f, ref_reg);
   spe_release_register(f, amask_reg);
}


/**
 * This pair of functions is used inline to allocate and deallocate
 * optional constant registers.  Once a constant is discovered to be 
 * needed, we will likely need it again, so we don't want to deallocate
 * it and have to allocate and load it again unnecessarily.
 */
static INLINE void
setup_optional_register(struct spe_function *f,
                        int *r)
{
   if (*r < 0)
      *r = spe_allocate_available_register(f);
}

static INLINE void
release_optional_register(struct spe_function *f,
                          int r)
{
   if (r >= 0)
      spe_release_register(f, r);
}

static INLINE void
setup_const_register(struct spe_function *f,
                     int *r,
                     float value)
{
   if (*r >= 0)
      return;
   setup_optional_register(f, r);
   spe_load_float(f, *r, value);
}

static INLINE void
release_const_register(struct spe_function *f,
                       int r)
{
   release_optional_register(f, r);
}



/**
 * Unpack/convert framebuffer colors from four 32-bit packed colors
 * (fbRGBA) to four float RGBA vectors (fbR, fbG, fbB, fbA).
 * Each 8-bit color component is expanded into a float in [0.0, 1.0].
 */
static void
unpack_colors(struct spe_function *f,
              enum pipe_format color_format,
              int fbRGBA_reg,
              int fbR_reg, int fbG_reg, int fbB_reg, int fbA_reg)
{
   int mask0_reg = spe_allocate_available_register(f);
   int mask1_reg = spe_allocate_available_register(f);
   int mask2_reg = spe_allocate_available_register(f);
   int mask3_reg = spe_allocate_available_register(f);

   spe_load_int(f, mask0_reg, 0xff);
   spe_load_int(f, mask1_reg, 0xff00);
   spe_load_int(f, mask2_reg, 0xff0000);
   spe_load_int(f, mask3_reg, 0xff000000);

   spe_comment(f, 0, "Unpack framebuffer colors, convert to floats");

   switch (color_format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      /* fbB = fbRGBA & mask */
      spe_and(f, fbB_reg, fbRGBA_reg, mask0_reg);

      /* fbG = fbRGBA & mask */
      spe_and(f, fbG_reg, fbRGBA_reg, mask1_reg);

      /* fbR = fbRGBA & mask */
      spe_and(f, fbR_reg, fbRGBA_reg, mask2_reg);

      /* fbA = fbRGBA & mask */
      spe_and(f, fbA_reg, fbRGBA_reg, mask3_reg);

      /* fbG = fbG >> 8 */
      spe_roti(f, fbG_reg, fbG_reg, -8);

      /* fbR = fbR >> 16 */
      spe_roti(f, fbR_reg, fbR_reg, -16);

      /* fbA = fbA >> 24 */
      spe_roti(f, fbA_reg, fbA_reg, -24);
      break;

   case PIPE_FORMAT_A8R8G8B8_UNORM:
      /* fbA = fbRGBA & mask */
      spe_and(f, fbA_reg, fbRGBA_reg, mask0_reg);

      /* fbR = fbRGBA & mask */
      spe_and(f, fbR_reg, fbRGBA_reg, mask1_reg);

      /* fbG = fbRGBA & mask */
      spe_and(f, fbG_reg, fbRGBA_reg, mask2_reg);

      /* fbB = fbRGBA & mask */
      spe_and(f, fbB_reg, fbRGBA_reg, mask3_reg);

      /* fbR = fbR >> 8 */
      spe_roti(f, fbR_reg, fbR_reg, -8);

      /* fbG = fbG >> 16 */
      spe_roti(f, fbG_reg, fbG_reg, -16);

      /* fbB = fbB >> 24 */
      spe_roti(f, fbB_reg, fbB_reg, -24);
      break;

   default:
      ASSERT(0);
   }

   /* convert int[4] in [0,255] to float[4] in [0.0, 1.0] */
   spe_cuflt(f, fbR_reg, fbR_reg, 8);
   spe_cuflt(f, fbG_reg, fbG_reg, 8);
   spe_cuflt(f, fbB_reg, fbB_reg, 8);
   spe_cuflt(f, fbA_reg, fbA_reg, 8);

   spe_release_register(f, mask0_reg);
   spe_release_register(f, mask1_reg);
   spe_release_register(f, mask2_reg);
   spe_release_register(f, mask3_reg);
}


/**
 * Generate SPE code to implement the given blend mode for a quad of pixels.
 * \param f          SPE function to append instruction onto.
 * \param fragR_reg  register with fragment red values (float) (in/out)
 * \param fragG_reg  register with fragment green values (float) (in/out)
 * \param fragB_reg  register with fragment blue values (float) (in/out)
 * \param fragA_reg  register with fragment alpha values (float) (in/out)
 * \param fbRGBA_reg register with packed framebuffer colors (integer) (in)
 */
static void
gen_blend(const struct pipe_blend_state *blend,
          const struct pipe_blend_color *blend_color,
          struct spe_function *f,
          enum pipe_format color_format,
          int fragR_reg, int fragG_reg, int fragB_reg, int fragA_reg,
          int fbRGBA_reg)
{
   int term1R_reg = spe_allocate_available_register(f);
   int term1G_reg = spe_allocate_available_register(f);
   int term1B_reg = spe_allocate_available_register(f);
   int term1A_reg = spe_allocate_available_register(f);

   int term2R_reg = spe_allocate_available_register(f);
   int term2G_reg = spe_allocate_available_register(f);
   int term2B_reg = spe_allocate_available_register(f);
   int term2A_reg = spe_allocate_available_register(f);

   int fbR_reg = spe_allocate_available_register(f);
   int fbG_reg = spe_allocate_available_register(f);
   int fbB_reg = spe_allocate_available_register(f);
   int fbA_reg = spe_allocate_available_register(f);

   int tmp_reg = spe_allocate_available_register(f);

   /* Optional constant registers we might or might not end up using;
    * if we do use them, make sure we only allocate them once by
    * keeping a flag on each one.
    */
   int one_reg = -1;
   int constR_reg = -1, constG_reg = -1, constB_reg = -1, constA_reg = -1;

   ASSERT(blend->rt[0].blend_enable);

   /* packed RGBA -> float colors */
   unpack_colors(f, color_format, fbRGBA_reg,
                 fbR_reg, fbG_reg, fbB_reg, fbA_reg);

   /*
    * Compute Src RGB terms.  We're actually looking for the value
    * of (the appropriate RGB factors) * (the incoming source RGB color),
    * because in some cases (like PIPE_BLENDFACTOR_ONE and 
    * PIPE_BLENDFACTOR_ZERO) we can avoid doing unnecessary math.
    */
   switch (blend->rt[0].rgb_src_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* factors = (1,1,1), so term = (R,G,B) */
      spe_move(f, term1R_reg, fragR_reg);
      spe_move(f, term1G_reg, fragG_reg);
      spe_move(f, term1B_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_ZERO:
      /* factors = (0,0,0), so term = (0,0,0) */
      spe_load_float(f, term1R_reg, 0.0f);
      spe_load_float(f, term1G_reg, 0.0f);
      spe_load_float(f, term1B_reg, 0.0f);
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      /* factors = (R,G,B), so term = (R*R, G*G, B*B) */
      spe_fm(f, term1R_reg, fragR_reg, fragR_reg);
      spe_fm(f, term1G_reg, fragG_reg, fragG_reg);
      spe_fm(f, term1B_reg, fragB_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      /* factors = (A,A,A), so term = (R*A, G*A, B*A) */
      spe_fm(f, term1R_reg, fragR_reg, fragA_reg);
      spe_fm(f, term1G_reg, fragG_reg, fragA_reg);
      spe_fm(f, term1B_reg, fragB_reg, fragA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      /* factors = (1-R,1-G,1-B), so term = (R*(1-R), G*(1-G), B*(1-B)) 
       * or in other words term = (R-R*R, G-G*G, B-B*B)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term1R_reg, fragR_reg, fragR_reg, fragR_reg);
      spe_fnms(f, term1G_reg, fragG_reg, fragG_reg, fragG_reg);
      spe_fnms(f, term1B_reg, fragB_reg, fragB_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      /* factors = (Rfb,Gfb,Bfb), so term = (R*Rfb, G*Gfb, B*Bfb) */
      spe_fm(f, term1R_reg, fragR_reg, fbR_reg);
      spe_fm(f, term1G_reg, fragG_reg, fbG_reg);
      spe_fm(f, term1B_reg, fragB_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      /* factors = (1-Rfb,1-Gfb,1-Bfb), so term = (R*(1-Rfb),G*(1-Gfb),B*(1-Bfb))
       * or term = (R-R*Rfb, G-G*Gfb, B-B*Bfb)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term1R_reg, fragR_reg, fbR_reg, fragR_reg);
      spe_fnms(f, term1G_reg, fragG_reg, fbG_reg, fragG_reg);
      spe_fnms(f, term1B_reg, fragB_reg, fbB_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      /* factors = (1-A,1-A,1-A), so term = (R*(1-A),G*(1-A),B*(1-A))
       * or term = (R-R*A,G-G*A,B-B*A)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term1R_reg, fragR_reg, fragA_reg, fragR_reg);
      spe_fnms(f, term1G_reg, fragG_reg, fragA_reg, fragG_reg);
      spe_fnms(f, term1B_reg, fragB_reg, fragA_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      /* factors = (Afb, Afb, Afb), so term = (R*Afb, G*Afb, B*Afb) */
      spe_fm(f, term1R_reg, fragR_reg, fbA_reg);
      spe_fm(f, term1G_reg, fragG_reg, fbA_reg);
      spe_fm(f, term1B_reg, fragB_reg, fbA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      /* factors = (1-Afb, 1-Afb, 1-Afb), so term = (R*(1-Afb),G*(1-Afb),B*(1-Afb)) 
       * or term = (R-R*Afb,G-G*Afb,b-B*Afb)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term1R_reg, fragR_reg, fbA_reg, fragR_reg);
      spe_fnms(f, term1G_reg, fragG_reg, fbA_reg, fragG_reg);
      spe_fnms(f, term1B_reg, fragB_reg, fbA_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg, blend_color->color[2]);
      /* now, factor = (Rc,Gc,Bc), so term = (R*Rc,G*Gc,B*Bc) */
      spe_fm(f, term1R_reg, fragR_reg, constR_reg);
      spe_fm(f, term1G_reg, fragG_reg, constG_reg);
      spe_fm(f, term1B_reg, fragB_reg, constB_reg);
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      /* we'll need the optional constant alpha register */
      setup_const_register(f, &constA_reg, blend_color->color[3]);
      /* factor = (Ac,Ac,Ac), so term = (R*Ac,G*Ac,B*Ac) */
      spe_fm(f, term1R_reg, fragR_reg, constA_reg);
      spe_fm(f, term1G_reg, fragG_reg, constA_reg);
      spe_fm(f, term1B_reg, fragB_reg, constA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg, blend_color->color[2]);
      /* factor = (1-Rc,1-Gc,1-Bc), so term = (R*(1-Rc),G*(1-Gc),B*(1-Bc)) 
       * or term = (R-R*Rc, G-G*Gc, B-B*Bc)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term1R_reg, fragR_reg, constR_reg, fragR_reg);
      spe_fnms(f, term1G_reg, fragG_reg, constG_reg, fragG_reg);
      spe_fnms(f, term1B_reg, fragB_reg, constB_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg, blend_color->color[2]);
      /* factor = (1-Ac,1-Ac,1-Ac), so term = (R*(1-Ac),G*(1-Ac),B*(1-Ac))
       * or term = (R-R*Ac,G-G*Ac,B-B*Ac)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term1R_reg, fragR_reg, constA_reg, fragR_reg);
      spe_fnms(f, term1G_reg, fragG_reg, constA_reg, fragG_reg);
      spe_fnms(f, term1B_reg, fragB_reg, constA_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      /* We'll need the optional {1,1,1,1} register */
      setup_const_register(f, &one_reg, 1.0f);
      /* factor = (min(A,1-Afb),min(A,1-Afb),min(A,1-Afb)), so 
       * term = (R*min(A,1-Afb), G*min(A,1-Afb), B*min(A,1-Afb))
       * We could expand the term (as a*min(b,c) == min(a*b,a*c)
       * as long as a is positive), but then we'd have to do three
       * spe_float_min() functions instead of one, so this is simpler.
       */
      /* tmp = 1 - Afb */
      spe_fs(f, tmp_reg, one_reg, fbA_reg);
      /* tmp = min(A,tmp) */
      spe_float_min(f, tmp_reg, fragA_reg, tmp_reg);
      /* term = R*tmp */
      spe_fm(f, term1R_reg, fragR_reg, tmp_reg);
      spe_fm(f, term1G_reg, fragG_reg, tmp_reg);
      spe_fm(f, term1B_reg, fragB_reg, tmp_reg);
      break;

      /* These are special D3D cases involving a second color output
       * from the fragment shader.  I'm not sure we can support them
       * yet... XXX
       */
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:

   default:
      ASSERT(0);
   }

   /*
    * Compute Src Alpha term.  Like the above, we're looking for
    * the full term A*factor, not just the factor itself, because
    * in many cases we can avoid doing unnecessary multiplies.
    */
   switch (blend->rt[0].alpha_src_factor) {
   case PIPE_BLENDFACTOR_ZERO:
      /* factor = 0, so term = 0 */
      spe_load_float(f, term1A_reg, 0.0f);
      break;

   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: /* fall through */
   case PIPE_BLENDFACTOR_ONE:
      /* factor = 1, so term = A */
      spe_move(f, term1A_reg, fragA_reg);
      break;

   case PIPE_BLENDFACTOR_SRC_COLOR:
      /* factor = A, so term = A*A */
      spe_fm(f, term1A_reg, fragA_reg, fragA_reg);
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      spe_fm(f, term1A_reg, fragA_reg, fragA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_SRC_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      /* factor = 1-A, so term = A*(1-A) = A-A*A */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term1A_reg, fragA_reg, fragA_reg, fragA_reg);
      break;

   case PIPE_BLENDFACTOR_DST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_DST_COLOR:
      /* factor = Afb, so term = A*Afb */
      spe_fm(f, term1A_reg, fragA_reg, fbA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_DST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      /* factor = 1-Afb, so term = A*(1-Afb) = A - A*Afb */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term1A_reg, fragA_reg, fbA_reg, fragA_reg);
      break;

   case PIPE_BLENDFACTOR_CONST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_CONST_COLOR:
      /* We need the optional constA_reg register */
      setup_const_register(f, &constA_reg, blend_color->color[3]);
      /* factor = Ac, so term = A*Ac */
      spe_fm(f, term1A_reg, fragA_reg, constA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constA_reg register */
      setup_const_register(f, &constA_reg, blend_color->color[3]);
      /* factor = 1-Ac, so term = A*(1-Ac) = A-A*Ac */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term1A_reg, fragA_reg, constA_reg, fragA_reg);
      break;

      /* These are special D3D cases involving a second color output
       * from the fragment shader.  I'm not sure we can support them
       * yet... XXX
       */
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
   default:
      ASSERT(0);
   }

   /*
    * Compute Dest RGB term.  Like the above, we're looking for
    * the full term (Rfb,Gfb,Bfb)*(factor), not just the factor itself, because
    * in many cases we can avoid doing unnecessary multiplies.
    */
   switch (blend->rt[0].rgb_dst_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* factors = (1,1,1), so term = (Rfb,Gfb,Bfb) */
      spe_move(f, term2R_reg, fbR_reg);
      spe_move(f, term2G_reg, fbG_reg);
      spe_move(f, term2B_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_ZERO:
      /* factor s= (0,0,0), so term = (0,0,0) */
      spe_load_float(f, term2R_reg, 0.0f);
      spe_load_float(f, term2G_reg, 0.0f);
      spe_load_float(f, term2B_reg, 0.0f);
      break;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      /* factors = (R,G,B), so term = (R*Rfb, G*Gfb, B*Bfb) */
      spe_fm(f, term2R_reg, fbR_reg, fragR_reg);
      spe_fm(f, term2G_reg, fbG_reg, fragG_reg);
      spe_fm(f, term2B_reg, fbB_reg, fragB_reg);
      break;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      /* factors = (1-R,1-G,1-B), so term = (Rfb*(1-R), Gfb*(1-G), Bfb*(1-B)) 
       * or in other words term = (Rfb-Rfb*R, Gfb-Gfb*G, Bfb-Bfb*B)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term2R_reg, fragR_reg, fbR_reg, fbR_reg);
      spe_fnms(f, term2G_reg, fragG_reg, fbG_reg, fbG_reg);
      spe_fnms(f, term2B_reg, fragB_reg, fbB_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      /* factors = (A,A,A), so term = (Rfb*A, Gfb*A, Bfb*A) */
      spe_fm(f, term2R_reg, fbR_reg, fragA_reg);
      spe_fm(f, term2G_reg, fbG_reg, fragA_reg);
      spe_fm(f, term2B_reg, fbB_reg, fragA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      /* factors = (1-A,1-A,1-A) so term = (Rfb-Rfb*A,Gfb-Gfb*A,Bfb-Bfb*A) */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term2R_reg, fbR_reg, fragA_reg, fbR_reg);
      spe_fnms(f, term2G_reg, fbG_reg, fragA_reg, fbG_reg);
      spe_fnms(f, term2B_reg, fbB_reg, fragA_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_DST_COLOR:
      /* factors = (Rfb,Gfb,Bfb), so term = (Rfb*Rfb, Gfb*Gfb, Bfb*Bfb) */
      spe_fm(f, term2R_reg, fbR_reg, fbR_reg);
      spe_fm(f, term2G_reg, fbG_reg, fbG_reg);
      spe_fm(f, term2B_reg, fbB_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      /* factors = (1-Rfb,1-Gfb,1-Bfb), so term = (Rfb*(1-Rfb),Gfb*(1-Gfb),Bfb*(1-Bfb))
       * or term = (Rfb-Rfb*Rfb, Gfb-Gfb*Gfb, Bfb-Bfb*Bfb)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term2R_reg, fbR_reg, fbR_reg, fbR_reg);
      spe_fnms(f, term2G_reg, fbG_reg, fbG_reg, fbG_reg);
      spe_fnms(f, term2B_reg, fbB_reg, fbB_reg, fbB_reg);
      break;

   case PIPE_BLENDFACTOR_DST_ALPHA:
      /* factors = (Afb, Afb, Afb), so term = (Rfb*Afb, Gfb*Afb, Bfb*Afb) */
      spe_fm(f, term2R_reg, fbR_reg, fbA_reg);
      spe_fm(f, term2G_reg, fbG_reg, fbA_reg);
      spe_fm(f, term2B_reg, fbB_reg, fbA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      /* factors = (1-Afb, 1-Afb, 1-Afb), so term = (Rfb*(1-Afb),Gfb*(1-Afb),Bfb*(1-Afb)) 
       * or term = (Rfb-Rfb*Afb,Gfb-Gfb*Afb,Bfb-Bfb*Afb)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term2R_reg, fbR_reg, fbA_reg, fbR_reg);
      spe_fnms(f, term2G_reg, fbG_reg, fbA_reg, fbG_reg);
      spe_fnms(f, term2B_reg, fbB_reg, fbA_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg, blend_color->color[2]);
      /* now, factor = (Rc,Gc,Bc), so term = (Rfb*Rc,Gfb*Gc,Bfb*Bc) */
      spe_fm(f, term2R_reg, fbR_reg, constR_reg);
      spe_fm(f, term2G_reg, fbG_reg, constG_reg);
      spe_fm(f, term2B_reg, fbB_reg, constB_reg);
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      /* we'll need the optional constant alpha register */
      setup_const_register(f, &constA_reg, blend_color->color[3]);
      /* factor = (Ac,Ac,Ac), so term = (Rfb*Ac,Gfb*Ac,Bfb*Ac) */
      spe_fm(f, term2R_reg, fbR_reg, constA_reg);
      spe_fm(f, term2G_reg, fbG_reg, constA_reg);
      spe_fm(f, term2B_reg, fbB_reg, constA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg, blend_color->color[2]);
      /* factor = (1-Rc,1-Gc,1-Bc), so term = (Rfb*(1-Rc),Gfb*(1-Gc),Bfb*(1-Bc)) 
       * or term = (Rfb-Rfb*Rc, Gfb-Gfb*Gc, Bfb-Bfb*Bc)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term2R_reg, fbR_reg, constR_reg, fbR_reg);
      spe_fnms(f, term2G_reg, fbG_reg, constG_reg, fbG_reg);
      spe_fnms(f, term2B_reg, fbB_reg, constB_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg, blend_color->color[2]);
      /* factor = (1-Ac,1-Ac,1-Ac), so term = (Rfb*(1-Ac),Gfb*(1-Ac),Bfb*(1-Ac))
       * or term = (Rfb-Rfb*Ac,Gfb-Gfb*Ac,Bfb-Bfb*Ac)
       * fnms(a,b,c,d) computes a = d - b*c
       */
      spe_fnms(f, term2R_reg, fbR_reg, constA_reg, fbR_reg);
      spe_fnms(f, term2G_reg, fbG_reg, constA_reg, fbG_reg);
      spe_fnms(f, term2B_reg, fbB_reg, constA_reg, fbB_reg);
      break;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: /* not supported for dest RGB */
      ASSERT(0);
      break;

      /* These are special D3D cases involving a second color output
       * from the fragment shader.  I'm not sure we can support them
       * yet... XXX
       */
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:

   default:
      ASSERT(0);
   }

   /*
    * Compute Dest Alpha term.  Like the above, we're looking for
    * the full term Afb*factor, not just the factor itself, because
    * in many cases we can avoid doing unnecessary multiplies.
    */
   switch (blend->rt[0].alpha_dst_factor) {
   case PIPE_BLENDFACTOR_ONE:
      /* factor = 1, so term = Afb */
      spe_move(f, term2A_reg, fbA_reg);
      break;
   case PIPE_BLENDFACTOR_ZERO:
      /* factor = 0, so term = 0 */
      spe_load_float(f, term2A_reg, 0.0f);
      break;

   case PIPE_BLENDFACTOR_SRC_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_SRC_COLOR:
      /* factor = A, so term = Afb*A */
      spe_fm(f, term2A_reg, fbA_reg, fragA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_SRC_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      /* factor = 1-A, so term = Afb*(1-A) = Afb-Afb*A */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term2A_reg, fbA_reg, fragA_reg, fbA_reg);
      break;

   case PIPE_BLENDFACTOR_DST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_DST_COLOR:
      /* factor = Afb, so term = Afb*Afb */
      spe_fm(f, term2A_reg, fbA_reg, fbA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_DST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      /* factor = 1-Afb, so term = Afb*(1-Afb) = Afb - Afb*Afb */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term2A_reg, fbA_reg, fbA_reg, fbA_reg);
      break;

   case PIPE_BLENDFACTOR_CONST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_CONST_COLOR:
      /* We need the optional constA_reg register */
      setup_const_register(f, &constA_reg, blend_color->color[3]);
      /* factor = Ac, so term = Afb*Ac */
      spe_fm(f, term2A_reg, fbA_reg, constA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constA_reg register */
      setup_const_register(f, &constA_reg, blend_color->color[3]);
      /* factor = 1-Ac, so term = Afb*(1-Ac) = Afb-Afb*Ac */
      /* fnms(a,b,c,d) computes a = d - b*c */
      spe_fnms(f, term2A_reg, fbA_reg, constA_reg, fbA_reg);
      break;

   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE: /* not supported for dest alpha */
      ASSERT(0);
      break;

      /* These are special D3D cases involving a second color output
       * from the fragment shader.  I'm not sure we can support them
       * yet... XXX
       */
   case PIPE_BLENDFACTOR_SRC1_COLOR:
   case PIPE_BLENDFACTOR_SRC1_ALPHA:
   case PIPE_BLENDFACTOR_INV_SRC1_COLOR:
   case PIPE_BLENDFACTOR_INV_SRC1_ALPHA:
   default:
      ASSERT(0);
   }

   /*
    * Combine Src/Dest RGB terms as per the blend equation.
    */
   switch (blend->rt[0].rgb_func) {
   case PIPE_BLEND_ADD:
      spe_fa(f, fragR_reg, term1R_reg, term2R_reg);
      spe_fa(f, fragG_reg, term1G_reg, term2G_reg);
      spe_fa(f, fragB_reg, term1B_reg, term2B_reg);
      break;
   case PIPE_BLEND_SUBTRACT:
      spe_fs(f, fragR_reg, term1R_reg, term2R_reg);
      spe_fs(f, fragG_reg, term1G_reg, term2G_reg);
      spe_fs(f, fragB_reg, term1B_reg, term2B_reg);
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      spe_fs(f, fragR_reg, term2R_reg, term1R_reg);
      spe_fs(f, fragG_reg, term2G_reg, term1G_reg);
      spe_fs(f, fragB_reg, term2B_reg, term1B_reg);
      break;
   case PIPE_BLEND_MIN:
      spe_float_min(f, fragR_reg, term1R_reg, term2R_reg);
      spe_float_min(f, fragG_reg, term1G_reg, term2G_reg);
      spe_float_min(f, fragB_reg, term1B_reg, term2B_reg);
      break;
   case PIPE_BLEND_MAX:
      spe_float_max(f, fragR_reg, term1R_reg, term2R_reg);
      spe_float_max(f, fragG_reg, term1G_reg, term2G_reg);
      spe_float_max(f, fragB_reg, term1B_reg, term2B_reg);
      break;
   default:
      ASSERT(0);
   }

   /*
    * Combine Src/Dest A term
    */
   switch (blend->rt[0].alpha_func) {
   case PIPE_BLEND_ADD:
      spe_fa(f, fragA_reg, term1A_reg, term2A_reg);
      break;
   case PIPE_BLEND_SUBTRACT:
      spe_fs(f, fragA_reg, term1A_reg, term2A_reg);
      break;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      spe_fs(f, fragA_reg, term2A_reg, term1A_reg);
      break;
   case PIPE_BLEND_MIN:
      spe_float_min(f, fragA_reg, term1A_reg, term2A_reg);
      break;
   case PIPE_BLEND_MAX:
      spe_float_max(f, fragA_reg, term1A_reg, term2A_reg);
      break;
   default:
      ASSERT(0);
   }

   spe_release_register(f, term1R_reg);
   spe_release_register(f, term1G_reg);
   spe_release_register(f, term1B_reg);
   spe_release_register(f, term1A_reg);

   spe_release_register(f, term2R_reg);
   spe_release_register(f, term2G_reg);
   spe_release_register(f, term2B_reg);
   spe_release_register(f, term2A_reg);

   spe_release_register(f, fbR_reg);
   spe_release_register(f, fbG_reg);
   spe_release_register(f, fbB_reg);
   spe_release_register(f, fbA_reg);

   spe_release_register(f, tmp_reg);

   /* Free any optional registers that actually got used */
   release_const_register(f, one_reg);
   release_const_register(f, constR_reg);
   release_const_register(f, constG_reg);
   release_const_register(f, constB_reg);
   release_const_register(f, constA_reg);
}


static void
gen_logicop(const struct pipe_blend_state *blend,
            struct spe_function *f,
            int fragRGBA_reg, int fbRGBA_reg)
{
   /* We've got four 32-bit RGBA packed pixels in each of
    * fragRGBA_reg and fbRGBA_reg, not sets of floating-point
    * reds, greens, blues, and alphas.
    * */
   ASSERT(blend->logicop_enable);

   switch(blend->logicop_func) {
      case PIPE_LOGICOP_CLEAR: /* 0 */
         spe_zero(f, fragRGBA_reg);
         break;
      case PIPE_LOGICOP_NOR: /* ~(s | d) */
         spe_nor(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_AND_INVERTED: /* ~s & d */
         /* andc R, A, B computes R = A & ~B */
         spe_andc(f, fragRGBA_reg, fbRGBA_reg, fragRGBA_reg);
         break;
      case PIPE_LOGICOP_COPY_INVERTED: /* ~s */
         spe_complement(f, fragRGBA_reg, fragRGBA_reg);
         break;
      case PIPE_LOGICOP_AND_REVERSE: /* s & ~d */
         /* andc R, A, B computes R = A & ~B */
         spe_andc(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_INVERT: /* ~d */
         /* Note that (A nor A) == ~(A|A) == ~A */
         spe_nor(f, fragRGBA_reg, fbRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_XOR: /* s ^ d */
         spe_xor(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_NAND: /* ~(s & d) */
         spe_nand(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_AND: /* s & d */
         spe_and(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_EQUIV: /* ~(s ^ d) */
         spe_xor(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         spe_complement(f, fragRGBA_reg, fragRGBA_reg);
         break;
      case PIPE_LOGICOP_NOOP: /* d */
         spe_move(f, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_OR_INVERTED: /* ~s | d */
         /* orc R, A, B computes R = A | ~B */
         spe_orc(f, fragRGBA_reg, fbRGBA_reg, fragRGBA_reg);
         break;
      case PIPE_LOGICOP_COPY: /* s */
         break;
      case PIPE_LOGICOP_OR_REVERSE: /* s | ~d */
         /* orc R, A, B computes R = A | ~B */
         spe_orc(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_OR: /* s | d */
         spe_or(f, fragRGBA_reg, fragRGBA_reg, fbRGBA_reg);
         break;
      case PIPE_LOGICOP_SET: /* 1 */
         spe_load_int(f, fragRGBA_reg, 0xffffffff);
         break;
      default:
         ASSERT(0);
   }
}


/**
 * Generate code to pack a quad of float colors into four 32-bit integers.
 *
 * \param f             SPE function to append instruction onto.
 * \param color_format  the dest color packing format
 * \param r_reg         register containing four red values (in/clobbered)
 * \param g_reg         register containing four green values (in/clobbered)
 * \param b_reg         register containing four blue values (in/clobbered)
 * \param a_reg         register containing four alpha values (in/clobbered)
 * \param rgba_reg      register to store the packed RGBA colors (out)
 */
static void
gen_pack_colors(struct spe_function *f,
                enum pipe_format color_format,
                int r_reg, int g_reg, int b_reg, int a_reg,
                int rgba_reg)
{
   int rg_reg = spe_allocate_available_register(f);
   int ba_reg = spe_allocate_available_register(f);

   /* Convert float[4] in [0.0,1.0] to int[4] in [0,~0], with clamping */
   spe_cfltu(f, r_reg, r_reg, 32);
   spe_cfltu(f, g_reg, g_reg, 32);
   spe_cfltu(f, b_reg, b_reg, 32);
   spe_cfltu(f, a_reg, a_reg, 32);

   /* Shift the most significant bytes to the least significant positions.
    * I.e.: reg = reg >> 24
    */
   spe_rotmi(f, r_reg, r_reg, -24);
   spe_rotmi(f, g_reg, g_reg, -24);
   spe_rotmi(f, b_reg, b_reg, -24);
   spe_rotmi(f, a_reg, a_reg, -24);

   /* Shift the color bytes according to the surface format */
   if (color_format == PIPE_FORMAT_B8G8R8A8_UNORM) {
      spe_roti(f, g_reg, g_reg, 8);   /* green <<= 8 */
      spe_roti(f, r_reg, r_reg, 16);  /* red <<= 16 */
      spe_roti(f, a_reg, a_reg, 24);  /* alpha <<= 24 */
   }
   else if (color_format == PIPE_FORMAT_A8R8G8B8_UNORM) {
      spe_roti(f, r_reg, r_reg, 8);   /* red <<= 8 */
      spe_roti(f, g_reg, g_reg, 16);  /* green <<= 16 */
      spe_roti(f, b_reg, b_reg, 24);  /* blue <<= 24 */
   }
   else {
      ASSERT(0);
   }

   /* Merge red, green, blue, alpha registers to make packed RGBA colors.
    * Eg: after shifting according to color_format we might have:
    *     R = {0x00ff0000, 0x00110000, 0x00220000, 0x00330000}
    *     G = {0x0000ff00, 0x00004400, 0x00005500, 0x00006600}
    *     B = {0x000000ff, 0x00000077, 0x00000088, 0x00000099}
    *     A = {0xff000000, 0xaa000000, 0xbb000000, 0xcc000000}
    * OR-ing all those together gives us four packed colors:
    *  RGBA = {0xffffffff, 0xaa114477, 0xbb225588, 0xcc336699}
    */
   spe_or(f, rg_reg, r_reg, g_reg);
   spe_or(f, ba_reg, a_reg, b_reg);
   spe_or(f, rgba_reg, rg_reg, ba_reg);

   spe_release_register(f, rg_reg);
   spe_release_register(f, ba_reg);
}


static void
gen_colormask(struct spe_function *f,
              uint colormask,
              enum pipe_format color_format,
              int fragRGBA_reg, int fbRGBA_reg)
{
   /* We've got four 32-bit RGBA packed pixels in each of
    * fragRGBA_reg and fbRGBA_reg, not sets of floating-point
    * reds, greens, blues, and alphas.  Further, the pixels
    * are packed according to the given color format, not
    * necessarily RGBA...
    */
   uint r_mask;
   uint g_mask;
   uint b_mask;
   uint a_mask;

   /* Calculate exactly where the bits for any particular color
    * end up, so we can mask them correctly.
    */
   switch(color_format) {
      case PIPE_FORMAT_B8G8R8A8_UNORM:
         /* ARGB */
         a_mask = 0xff000000;
         r_mask = 0x00ff0000;
         g_mask = 0x0000ff00;
         b_mask = 0x000000ff;
         break;
      case PIPE_FORMAT_A8R8G8B8_UNORM:
         /* BGRA */
         b_mask = 0xff000000;
         g_mask = 0x00ff0000;
         r_mask = 0x0000ff00;
         a_mask = 0x000000ff;
         break;
      default:
         ASSERT(0);
   }

   /* For each R, G, B, and A component we're supposed to mask out, 
    * clear its bits.   Then our mask operation later will work 
    * as expected.
    */
   if (!(colormask & PIPE_MASK_R)) {
      r_mask = 0;
   }
   if (!(colormask & PIPE_MASK_G)) {
      g_mask = 0;
   }
   if (!(colormask & PIPE_MASK_B)) {
      b_mask = 0;
   }
   if (!(colormask & PIPE_MASK_A)) {
      a_mask = 0;
   }

   /* Get a temporary register to hold the mask that will be applied
    * to the fragment
    */
   int colormask_reg = spe_allocate_available_register(f);

   /* The actual mask we're going to use is an OR of the remaining R, G, B,
    * and A masks.  Load the result value into our temporary register.
    */
   spe_load_uint(f, colormask_reg, r_mask | g_mask | b_mask | a_mask);

   /* Use the mask register to select between the fragment color
    * values and the frame buffer color values.  Wherever the
    * mask has a 0 bit, the current frame buffer color should override
    * the fragment color.  Wherever the mask has a 1 bit, the 
    * fragment color should persevere.  The Select Bits (selb rt, rA, rB, rM)
    * instruction will select bits from its first operand rA wherever the
    * the mask bits rM are 0, and from its second operand rB wherever the
    * mask bits rM are 1.  That means that the frame buffer color is the
    * first operand, and the fragment color the second.
    */
    spe_selb(f, fragRGBA_reg, fbRGBA_reg, fragRGBA_reg, colormask_reg);

    /* Release the temporary register and we're done */
    spe_release_register(f, colormask_reg);
}


/**
 * This function is annoyingly similar to gen_depth_test(), above, except
 * that instead of comparing two varying values (i.e. fragment and buffer),
 * we're comparing a varying value with a static value.  As such, we have
 * access to the Compare Immediate instructions where we don't in 
 * gen_depth_test(), which is what makes us very different.
 *
 * There's some added complexity if there's a non-trivial state->mask
 * value; then stencil and reference both must be masked
 *
 * The return value in the stencil_pass_reg is a bitmask of valid
 * fragments that also passed the stencil test.  The bitmask of valid
 * fragments that failed would be found in
 * (fragment_mask_reg & ~stencil_pass_reg).
 */
static void
gen_stencil_test(struct spe_function *f,
                 const struct pipe_stencil_state *state,
                 const unsigned ref_value,
                 uint stencil_max_value,
                 int fragment_mask_reg,
                 int fbS_reg, 
                 int stencil_pass_reg)
{
   /* Generate code that puts the set of passing fragments into the
    * stencil_pass_reg register, taking into account whether each fragment
    * was active to begin with.
    */
   switch (state->func) {
   case PIPE_FUNC_EQUAL:
      if (state->valuemask == stencil_max_value) {
         /* stencil_pass = fragment_mask & (s == reference) */
         spe_compare_equal_uint(f, stencil_pass_reg, fbS_reg, ref_value);
         spe_and(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
      }
      else {
         /* stencil_pass = fragment_mask & ((s&mask) == (reference&mask)) */
         uint tmp_masked_stencil = spe_allocate_available_register(f);
         spe_and_uint(f, tmp_masked_stencil, fbS_reg, state->valuemask);
         spe_compare_equal_uint(f, stencil_pass_reg, tmp_masked_stencil,
                                state->valuemask & ref_value);
         spe_and(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_masked_stencil);
      }
      break;

   case PIPE_FUNC_NOTEQUAL:
      if (state->valuemask == stencil_max_value) {
         /* stencil_pass = fragment_mask & ~(s == reference) */
         spe_compare_equal_uint(f, stencil_pass_reg, fbS_reg, ref_value);
         spe_andc(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
      }
      else {
         /* stencil_pass = fragment_mask & ~((s&mask) == (reference&mask)) */
         int tmp_masked_stencil = spe_allocate_available_register(f);
         spe_and_uint(f, tmp_masked_stencil, fbS_reg, state->valuemask);
         spe_compare_equal_uint(f, stencil_pass_reg, tmp_masked_stencil,
                                state->valuemask & ref_value);
         spe_andc(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_masked_stencil);
      }
      break;

   case PIPE_FUNC_LESS:
      if (state->valuemask == stencil_max_value) {
         /* stencil_pass = fragment_mask & (reference < s)  */
         spe_compare_greater_uint(f, stencil_pass_reg, fbS_reg, ref_value);
         spe_and(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
      }
      else {
         /* stencil_pass = fragment_mask & ((reference&mask) < (s & mask)) */
         int tmp_masked_stencil = spe_allocate_available_register(f);
         spe_and_uint(f, tmp_masked_stencil, fbS_reg, state->valuemask);
         spe_compare_greater_uint(f, stencil_pass_reg, tmp_masked_stencil,
                                  state->valuemask & ref_value);
         spe_and(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_masked_stencil);
      }
      break;

   case PIPE_FUNC_GREATER:
      if (state->valuemask == stencil_max_value) {
         /* stencil_pass = fragment_mask & (reference > s) */
         /* There's no convenient Compare Less Than Immediate instruction, so
          * we'll have to do this one the harder way, by loading a register and 
          * comparing directly.  Compare Logical Greater Than Word (clgt) 
          * treats its operands as unsigned - no sign extension.
          */
         int tmp_reg = spe_allocate_available_register(f);
         spe_load_uint(f, tmp_reg, ref_value);
         spe_clgt(f, stencil_pass_reg, tmp_reg, fbS_reg);
         spe_and(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_reg);
      }
      else {
         /* stencil_pass = fragment_mask & ((reference&mask) > (s&mask)) */
         int tmp_reg = spe_allocate_available_register(f);
         int tmp_masked_stencil = spe_allocate_available_register(f);
         spe_load_uint(f, tmp_reg, state->valuemask & ref_value);
         spe_and_uint(f, tmp_masked_stencil, fbS_reg, state->valuemask);
         spe_clgt(f, stencil_pass_reg, tmp_reg, tmp_masked_stencil);
         spe_and(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_reg);
         spe_release_register(f, tmp_masked_stencil);
      }
      break;

   case PIPE_FUNC_GEQUAL:
      if (state->valuemask == stencil_max_value) {
         /* stencil_pass = fragment_mask & (reference >= s) 
          *              = fragment_mask & ~(s > reference) */
         spe_compare_greater_uint(f, stencil_pass_reg, fbS_reg,
                                  ref_value);
         spe_andc(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
      }
      else {
         /* stencil_pass = fragment_mask & ~((s&mask) > (reference&mask)) */
         int tmp_masked_stencil = spe_allocate_available_register(f);
         spe_and_uint(f, tmp_masked_stencil, fbS_reg, state->valuemask);
         spe_compare_greater_uint(f, stencil_pass_reg, tmp_masked_stencil,
                                  state->valuemask & ref_value);
         spe_andc(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_masked_stencil);
      }
      break;

   case PIPE_FUNC_LEQUAL:
      if (state->valuemask == stencil_max_value) {
         /* stencil_pass = fragment_mask & (reference <= s) ]
          *               = fragment_mask & ~(reference > s) */
         /* As above, we have to do this by loading a register */
         int tmp_reg = spe_allocate_available_register(f);
         spe_load_uint(f, tmp_reg, ref_value);
         spe_clgt(f, stencil_pass_reg, tmp_reg, fbS_reg);
         spe_andc(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_reg);
      }
      else {
         /* stencil_pass = fragment_mask & ~((reference&mask) > (s&mask)) */
         int tmp_reg = spe_allocate_available_register(f);
         int tmp_masked_stencil = spe_allocate_available_register(f);
         spe_load_uint(f, tmp_reg, ref_value & state->valuemask);
         spe_and_uint(f, tmp_masked_stencil, fbS_reg, state->valuemask);
         spe_clgt(f, stencil_pass_reg, tmp_reg, tmp_masked_stencil);
         spe_andc(f, stencil_pass_reg, fragment_mask_reg, stencil_pass_reg);
         spe_release_register(f, tmp_reg);
         spe_release_register(f, tmp_masked_stencil);
      }
      break;

   case PIPE_FUNC_NEVER:
      /* stencil_pass = fragment_mask & 0 = 0 */
      spe_load_uint(f, stencil_pass_reg, 0);
      break;

   case PIPE_FUNC_ALWAYS:
      /* stencil_pass = fragment_mask & 1 = fragment_mask */
      spe_move(f, stencil_pass_reg, fragment_mask_reg);
      break;
   }

   /* The fragments that passed the stencil test are now in stencil_pass_reg.
    * The fragments that failed would be (fragment_mask_reg & ~stencil_pass_reg).
    */
}


/**
 * This function generates code that calculates a set of new stencil values
 * given the earlier values and the operation to apply.  It does not
 * apply any tests.  It is intended to be called up to 3 times
 * (for the stencil fail operation, for the stencil pass-z fail operation,
 * and for the stencil pass-z pass operation) to collect up to three
 * possible sets of values, and for the caller to combine them based
 * on the result of the tests.
 *
 * stencil_max_value should be (2^n - 1) where n is the number of bits
 * in the stencil buffer - in other words, it should be usable as a mask.
 */
static void
gen_stencil_values(struct spe_function *f,
                   uint stencil_op,
                   uint stencil_ref_value,
                   uint stencil_max_value,
                   int fbS_reg,
                   int newS_reg)
{
   /* The code below assumes that newS_reg and fbS_reg are not the same
    * register; if they can be, the calculations below will have to use
    * an additional temporary register.  For now, mark the assumption
    * with an assertion that will fail if they are the same.
    */
   ASSERT(fbS_reg != newS_reg);

   /* The code also assumes that the stencil_max_value is of the form
    * 2^n-1 and can therefore be used as a mask for the valid bits in 
    * addition to a maximum.  Make sure this is the case as well.
    * The clever math below exploits the fact that incrementing a 
    * binary number serves to flip all the bits of a number starting at
    * the LSB and continuing to (and including) the first zero bit
    * found.  That means that a number and its increment will always
    * have at least one bit in common (the high order bit, if nothing
    * else) *unless* the number is zero, *or* the number is of a form
    * consisting of some number of 1s in the low-order bits followed
    * by nothing but 0s in the high-order bits.  The latter case
    * implies it's of the form 2^n-1.
    */
   ASSERT(stencil_max_value > 0 && ((stencil_max_value + 1) & stencil_max_value) == 0);

   switch(stencil_op) {
   case PIPE_STENCIL_OP_KEEP:
      /* newS = S */
      spe_move(f, newS_reg, fbS_reg);
      break;

   case PIPE_STENCIL_OP_ZERO:
      /* newS = 0 */
      spe_zero(f, newS_reg);
      break;

   case PIPE_STENCIL_OP_REPLACE:
      /* newS = stencil reference value */
      spe_load_uint(f, newS_reg, stencil_ref_value);
      break;

   case PIPE_STENCIL_OP_INCR: {
      /* newS = (s == max ? max : s + 1) */
      int equals_reg = spe_allocate_available_register(f);

      spe_compare_equal_uint(f, equals_reg, fbS_reg, stencil_max_value);
      /* Add Word Immediate computes rT = rA + 10-bit signed immediate */
      spe_ai(f, newS_reg, fbS_reg, 1);
      /* Select from the current value or the new value based on the equality test */
      spe_selb(f, newS_reg, newS_reg, fbS_reg, equals_reg);

      spe_release_register(f, equals_reg);
      break;
   }
   case PIPE_STENCIL_OP_DECR: {
      /* newS = (s == 0 ? 0 : s - 1) */
      int equals_reg = spe_allocate_available_register(f);

      spe_compare_equal_uint(f, equals_reg, fbS_reg, 0);
      /* Add Word Immediate with a (-1) value works */
      spe_ai(f, newS_reg, fbS_reg, -1);
      /* Select from the current value or the new value based on the equality test */
      spe_selb(f, newS_reg, newS_reg, fbS_reg, equals_reg);

      spe_release_register(f, equals_reg);
      break;
   }
   case PIPE_STENCIL_OP_INCR_WRAP:
      /* newS = (s == max ? 0 : s + 1), but since max is 2^n-1, we can
       * do a normal add and mask off the correct bits 
       */
      spe_ai(f, newS_reg, fbS_reg, 1);
      spe_and_uint(f, newS_reg, newS_reg, stencil_max_value);
      break;

   case PIPE_STENCIL_OP_DECR_WRAP:
      /* newS = (s == 0 ? max : s - 1), but we'll pull the same mask trick as above */
      spe_ai(f, newS_reg, fbS_reg, -1);
      spe_and_uint(f, newS_reg, newS_reg, stencil_max_value);
      break;

   case PIPE_STENCIL_OP_INVERT:
      /* newS = ~s.  We take advantage of the mask/max value to invert only
       * the valid bits for the field so we don't have to do an extra "and".
       */
      spe_xor_uint(f, newS_reg, fbS_reg, stencil_max_value);
      break;

   default:
      ASSERT(0);
   }
}


/**
 * This function generates code to get all the necessary possible
 * stencil values.  For each of the output registers (fail_reg,
 * zfail_reg, and zpass_reg), it either allocates a new register
 * and calculates a new set of values based on the stencil operation,
 * or it reuses a register allocation and calculation done for an
 * earlier (matching) operation, or it reuses the fbS_reg register
 * (if the stencil operation is KEEP, which doesn't change the 
 * stencil buffer).
 *
 * Since this function allocates a variable number of registers,
 * to avoid incurring complex logic to free them, they should
 * be allocated after a spe_allocate_register_set() call
 * and released by the corresponding spe_release_register_set() call.
 */
static void
gen_get_stencil_values(struct spe_function *f,
                       const struct pipe_stencil_state *stencil,
                       const unsigned ref_value,
                       const uint depth_enabled,
                       int fbS_reg, 
                       int *fail_reg,
                       int *zfail_reg, 
                       int *zpass_reg)
{
   uint zfail_op;

   /* Stenciling had better be enabled here */
   ASSERT(stencil->enabled);

   /* If the depth test is not enabled, it is treated as though it always
    * passes, which means that the zfail_op is not considered - a
    * failing stencil test triggers the fail_op, and a passing one
    * triggers the zpass_op
    *
    * As an optimization, override calculation of the zfail_op values
    * if they aren't going to be used.  By setting the value of
    * the operation to PIPE_STENCIL_OP_KEEP, its value will be assumed
    * to match the incoming stencil values, and no calculation will
    * be done.
    */
   if (depth_enabled) {
      zfail_op = stencil->zfail_op;
   }
   else {
      zfail_op = PIPE_STENCIL_OP_KEEP;
   }

   /* One-sided or front-facing stencil */
   if (stencil->fail_op == PIPE_STENCIL_OP_KEEP) {
      *fail_reg = fbS_reg;
   }
   else {
      *fail_reg = spe_allocate_available_register(f);
      gen_stencil_values(f, stencil->fail_op, ref_value, 
         0xff, fbS_reg, *fail_reg);
   }

   /* Check the possibly overridden value, not the structure value */
   if (zfail_op == PIPE_STENCIL_OP_KEEP) {
      *zfail_reg = fbS_reg;
   }
   else if (zfail_op == stencil->fail_op) {
      *zfail_reg = *fail_reg;
   }
   else {
      *zfail_reg = spe_allocate_available_register(f);
      gen_stencil_values(f, stencil->zfail_op, ref_value, 
         0xff, fbS_reg, *zfail_reg);
   }

   if (stencil->zpass_op == PIPE_STENCIL_OP_KEEP) {
      *zpass_reg = fbS_reg;
   }
   else if (stencil->zpass_op == stencil->fail_op) {
      *zpass_reg = *fail_reg;
   }
   else if (stencil->zpass_op == zfail_op) {
      *zpass_reg = *zfail_reg;
   }
   else {
      *zpass_reg = spe_allocate_available_register(f);
      gen_stencil_values(f, stencil->zpass_op, ref_value, 
         0xff, fbS_reg, *zpass_reg);
   }
}

/**
 * Note that fbZ_reg may *not* be set on entry, if in fact
 * the depth test is not enabled.  This function must not use
 * the register if depth is not enabled.
 */
static boolean
gen_stencil_depth_test(struct spe_function *f, 
                       const struct pipe_depth_stencil_alpha_state *dsa,
                       const struct pipe_stencil_ref *stencil_ref,
                       const uint facing,
                       const int mask_reg, const int fragZ_reg, 
                       const int fbZ_reg, const int fbS_reg)
{
   /* True if we've generated code that could require writeback to the
    * depth and/or stencil buffers
    */
   boolean modified_buffers = FALSE;

   boolean need_to_calculate_stencil_values;
   boolean need_to_writemask_stencil_values;

   struct pipe_stencil_state *stencil;

   /* Registers.  We may or may not actually allocate these, depending
    * on whether the state values indicate that we need them.
    */
   int stencil_pass_reg, stencil_fail_reg;
   int stencil_fail_values, stencil_pass_depth_fail_values, stencil_pass_depth_pass_values;
   int stencil_writemask_reg;
   int zmask_reg;
   int newS_reg;
   unsigned ref_value;

   /* Stenciling is quite complex: up to six different configurable stencil 
    * operations/calculations can be required (three each for front-facing
    * and back-facing fragments).  Many of those operations will likely 
    * be identical, so there's good reason to try to avoid calculating 
    * the same values more than once (which unfortunately makes the code less 
    * straightforward).
    *
    * To make register management easier, we start a new 
    * register set; we can release all the registers in the set at
    * once, and avoid having to keep track of exactly which registers
    * we allocate.  We can still allocate and free registers as 
    * desired (if we know we no longer need a register), but we don't
    * have to spend the complexity to track the more difficult variant
    * register usage scenarios.
    */
   spe_comment(f, 0, "Allocating stencil register set");
   spe_allocate_register_set(f);

   /* The facing we're given is the fragment facing; it doesn't
    * exactly match the stencil facing.  If stencil is enabled,
    * but two-sided stencil is *not* enabled, we use the same
    * stencil settings for both front- and back-facing fragments.
    * We only use the "back-facing" stencil for backfacing fragments
    * if two-sided stenciling is enabled.
    */
   if (facing == CELL_FACING_BACK && dsa->stencil[1].enabled) {
      stencil = &dsa->stencil[1];
      ref_value = stencil_ref->ref_value[1];
   }
   else {
      stencil = &dsa->stencil[0];
      ref_value = stencil_ref->ref_value[0];
   }

   /* Calculate the writemask.  If the writemask is trivial (either
    * all 0s, meaning that we don't need to calculate any stencil values
    * because they're not going to change the stencil anyway, or all 1s,
    * meaning that we have to calculate the stencil values but do not
    * need to mask them), we can avoid generating code.  Don't forget
    * that we need to consider backfacing stencil, if enabled.
    *
    * Note that if the backface stencil is *not* enabled, the backface
    * stencil will have the same values as the frontface stencil.
    */
   if (stencil->fail_op == PIPE_STENCIL_OP_KEEP &&
       stencil->zfail_op == PIPE_STENCIL_OP_KEEP &&
       stencil->zpass_op == PIPE_STENCIL_OP_KEEP) {
       need_to_calculate_stencil_values = FALSE;
       need_to_writemask_stencil_values = FALSE;
    }
    else if (stencil->writemask == 0x0) {
      /* All changes are writemasked out, so no need to calculate
       * what those changes might be, and no need to write anything back.
       */
      need_to_calculate_stencil_values = FALSE;
      need_to_writemask_stencil_values = FALSE;
   }
   else if (stencil->writemask == 0xff) {
      /* Still trivial, but a little less so.  We need to write the stencil
       * values, but we don't need to mask them.
       */
      need_to_calculate_stencil_values = TRUE;
      need_to_writemask_stencil_values = FALSE;
   }
   else {
      /* The general case: calculate, mask, and write */
      need_to_calculate_stencil_values = TRUE;
      need_to_writemask_stencil_values = TRUE;

      /* While we're here, generate code that calculates what the
       * writemask should be.  If backface stenciling is enabled,
       * and the backface writemask is not the same as the frontface
       * writemask, we'll have to generate code that merges the
       * two masks into a single effective mask based on fragment facing.
       */
      spe_comment(f, 0, "Computing stencil writemask");
      stencil_writemask_reg = spe_allocate_available_register(f);
      spe_load_uint(f, stencil_writemask_reg, dsa->stencil[facing].writemask);
   }

   /* At least one-sided stenciling must be on.  Generate code that
    * runs the stencil test on the basic/front-facing stencil, leaving
    * the mask of passing stencil bits in stencil_pass_reg.  This mask will
    * be used both to mask the set of active pixels, and also to
    * determine how the stencil buffer changes.
    *
    * This test will *not* change the value in mask_reg (because we don't
    * yet know whether to apply the two-sided stencil or one-sided stencil).
    */
   spe_comment(f, 0, "Running basic stencil test");
   stencil_pass_reg = spe_allocate_available_register(f);
   gen_stencil_test(f, stencil, ref_value, 0xff, mask_reg, fbS_reg, stencil_pass_reg);

   /* Generate code that, given the mask of valid fragments and the
    * mask of valid fragments that passed the stencil test, computes
    * the mask of valid fragments that failed the stencil test.  We
    * have to do this before we run a depth test (because the
    * depth test should not be performed on fragments that failed the
    * stencil test, and because the depth test will update the 
    * mask of valid fragments based on the results of the depth test).
    */
   spe_comment(f, 0, "Computing stencil fail mask and updating fragment mask");
   stencil_fail_reg = spe_allocate_available_register(f);
   spe_andc(f, stencil_fail_reg, mask_reg, stencil_pass_reg);
   /* Now remove the stenciled-out pixels from the valid fragment mask,
    * so we can later use the valid fragment mask in the depth test.
    */
   spe_and(f, mask_reg, mask_reg, stencil_pass_reg);

   /* We may not need to calculate stencil values, if the writemask is off */
   if (need_to_calculate_stencil_values) {
      /* Generate code that calculates exactly which stencil values we need,
       * without calculating the same value twice (say, if two different
       * stencil ops have the same value).  This code will work for one-sided
       * and two-sided stenciling (so that we take into account that operations
       * may match between front and back stencils), and will also take into
       * account whether the depth test is enabled (if the depth test is off,
       * we don't need any of the zfail results, because the depth test always
       * is considered to pass if it is disabled).  Any register value that
       * does not need to be calculated will come back with the same value
       * that's in fbS_reg.
       *
       * This function will allocate a variant number of registers that
       * will be released as part of the register set.
       */
      spe_comment(f, 0, facing == CELL_FACING_FRONT
                  ? "Computing front-facing stencil values"
                  : "Computing back-facing stencil values");
      gen_get_stencil_values(f, stencil, ref_value, dsa->depth.enabled, fbS_reg, 
         &stencil_fail_values, &stencil_pass_depth_fail_values, 
         &stencil_pass_depth_pass_values);
   }  

   /* We now have all the stencil values we need.  We also need 
    * the results of the depth test to figure out which
    * stencil values will become the new stencil values.  (Even if
    * we aren't actually calculating stencil values, we need to apply
    * the depth test if it's enabled.)
    *
    * The code generated by gen_depth_test() returns the results of the
    * test in the given register, but also alters the mask_reg based
    * on the results of the test.
    */
   if (dsa->depth.enabled) {
      spe_comment(f, 0, "Running stencil depth test");
      zmask_reg = spe_allocate_available_register(f);
      modified_buffers |= gen_depth_test(f, dsa, mask_reg, fragZ_reg,
                                         fbZ_reg, zmask_reg);
   }

   if (need_to_calculate_stencil_values) {

      /* If we need to writemask the stencil values before going into
       * the stencil buffer, we'll have to use a new register to
       * hold the new values.  If not, we can just keep using the
       * current register.
       */
      if (need_to_writemask_stencil_values) {
         newS_reg = spe_allocate_available_register(f);
         spe_comment(f, 0, "Saving current stencil values for writemasking");
         spe_move(f, newS_reg, fbS_reg);
      }
      else {
         newS_reg = fbS_reg;
      }

      /* Merge in the selected stencil fail values */
      if (stencil_fail_values != fbS_reg) {
         spe_comment(f, 0, "Loading stencil fail values");
         spe_selb(f, newS_reg, newS_reg, stencil_fail_values, stencil_fail_reg);
         modified_buffers = TRUE;
      }

      /* Same for the stencil pass/depth fail values.  If this calculation
       * is not needed (say, if depth test is off), then the
       * stencil_pass_depth_fail_values register will be equal to fbS_reg
       * and we'll skip the calculation.
       */
      if (stencil_pass_depth_fail_values != fbS_reg) {
         /* We don't actually have a stencil pass/depth fail mask yet.
          * Calculate it here from the stencil passing mask and the
          * depth passing mask.  Note that zmask_reg *must* have been
          * set above if we're here.
          */
         uint stencil_pass_depth_fail_mask =
            spe_allocate_available_register(f);

         spe_comment(f, 0, "Loading stencil pass/depth fail values");
         spe_andc(f, stencil_pass_depth_fail_mask, stencil_pass_reg, zmask_reg);

         spe_selb(f, newS_reg, newS_reg, stencil_pass_depth_fail_values,
                  stencil_pass_depth_fail_mask);

         spe_release_register(f, stencil_pass_depth_fail_mask);
         modified_buffers = TRUE;
      }

      /* Same for the stencil pass/depth pass mask.  Note that we
       * *can* get here with zmask_reg being unset (if the depth
       * test is off but the stencil test is on).  In this case,
       * we assume the depth test passes, and don't need to mask
       * the stencil pass mask with the Z mask.
       */
      if (stencil_pass_depth_pass_values != fbS_reg) {
         if (dsa->depth.enabled) {
            uint stencil_pass_depth_pass_mask = spe_allocate_available_register(f);
            /* We'll need a separate register */
            spe_comment(f, 0, "Loading stencil pass/depth pass values");
            spe_and(f, stencil_pass_depth_pass_mask, stencil_pass_reg, zmask_reg);
            spe_selb(f, newS_reg, newS_reg, stencil_pass_depth_pass_values, stencil_pass_depth_pass_mask);
            spe_release_register(f, stencil_pass_depth_pass_mask);
         }
         else {
            /* We can use the same stencil-pass register */
            spe_comment(f, 0, "Loading stencil pass values");
            spe_selb(f, newS_reg, newS_reg, stencil_pass_depth_pass_values, stencil_pass_reg);
         }
         modified_buffers = TRUE;
      }

      /* Almost done.  If we need to writemask, do it now, leaving the
       * results in the fbS_reg register passed in.  If we don't need
       * to writemask, then the results are *already* in the fbS_reg,
       * so there's nothing more to do.
       */

      if (need_to_writemask_stencil_values && modified_buffers) {
         /* The Select Bytes command makes a fine writemask.  Where
          * the mask is 0, the first (original) values are retained,
          * effectively masking out changes.  Where the mask is 1, the
          * second (new) values are retained, incorporating changes.
          */
         spe_comment(f, 0, "Writemasking new stencil values");
         spe_selb(f, fbS_reg, fbS_reg, newS_reg, stencil_writemask_reg);
      }

   } /* done calculating stencil values */

   /* The stencil and/or depth values have been applied, and the
    * mask_reg, fbS_reg, and fbZ_reg values have been updated.
    * We're all done, except that we've allocated a fair number
    * of registers that we didn't bother tracking.  Release all
    * those registers as part of the register set, and go home.
    */
   spe_comment(f, 0, "Releasing stencil register set");
   spe_release_register_set(f);

   /* Return TRUE if we could have modified the stencil and/or
    * depth buffers.
    */
   return modified_buffers;
}


/**
 * Generate depth and/or stencil test code.
 * \param cell  context
 * \param dsa  depth/stencil/alpha state
 * \param f  spe function to emit
 * \param facing  either CELL_FACING_FRONT or CELL_FACING_BACK
 * \param mask_reg  register containing the pixel alive/dead mask
 * \param depth_tile_reg  register containing address of z/stencil tile
 * \param quad_offset_reg  offset to quad from start of tile
 * \param fragZ_reg  register containg fragment Z values
 */
static void
gen_depth_stencil(struct cell_context *cell,
                  const struct pipe_depth_stencil_alpha_state *dsa,
                  const struct pipe_stencil_ref *stencil_ref,
                  struct spe_function *f,
                  uint facing,
                  int mask_reg,
                  int depth_tile_reg,
                  int quad_offset_reg,
                  int fragZ_reg)

{
   const enum pipe_format zs_format = cell->framebuffer.zsbuf->format;
   boolean write_depth_stencil;

   /* framebuffer's combined z/stencil values register */
   int fbZS_reg = spe_allocate_available_register(f);

   /* Framebufer Z values register */
   int fbZ_reg = spe_allocate_available_register(f);

   /* Framebuffer stencil values register (may not be used) */
   int fbS_reg = spe_allocate_available_register(f);

   /* 24-bit mask register (may not be used) */
   int zmask_reg = spe_allocate_available_register(f);

   /**
    * The following code:
    * 1. fetch quad of packed Z/S values from the framebuffer tile.
    * 2. extract the separate the Z and S values from packed values
    * 3. convert fragment Z values from float in [0,1] to 32/24/16-bit ints
    *
    * The instructions for doing this are interleaved for better performance.
    */
   spe_comment(f, 0, "Fetch Z/stencil quad from tile");

   switch(zs_format) {
   case PIPE_FORMAT_Z24S8_UNORM: /* fall through */
   case PIPE_FORMAT_Z24X8_UNORM:
      /* prepare mask to extract Z vals from ZS vals */
      spe_load_uint(f, zmask_reg, 0x00ffffff);

      /* convert fragment Z from [0,1] to 32-bit ints */
      spe_cfltu(f, fragZ_reg, fragZ_reg, 32);

      /* Load: fbZS_reg = memory[depth_tile_reg + offset_reg] */
      spe_lqx(f, fbZS_reg, depth_tile_reg, quad_offset_reg);

      /* right shift 32-bit fragment Z to 24 bits */
      spe_rotmi(f, fragZ_reg, fragZ_reg, -8);

      /* extract 24-bit Z values from ZS values by masking */
      spe_and(f, fbZ_reg, fbZS_reg, zmask_reg);

      /* extract 8-bit stencil values by shifting */
      spe_rotmi(f, fbS_reg, fbZS_reg, -24);
      break;

   case PIPE_FORMAT_S8Z24_UNORM: /* fall through */
   case PIPE_FORMAT_X8Z24_UNORM:
      /* convert fragment Z from [0,1] to 32-bit ints */
      spe_cfltu(f, fragZ_reg, fragZ_reg, 32);

      /* Load: fbZS_reg = memory[depth_tile_reg + offset_reg] */
      spe_lqx(f, fbZS_reg, depth_tile_reg, quad_offset_reg);

      /* right shift 32-bit fragment Z to 24 bits */
      spe_rotmi(f, fragZ_reg, fragZ_reg, -8);

      /* extract 24-bit Z values from ZS values by shifting */
      spe_rotmi(f, fbZ_reg, fbZS_reg, -8);

      /* extract 8-bit stencil values by masking */
      spe_and_uint(f, fbS_reg, fbZS_reg, 0x000000ff);
      break;

   case PIPE_FORMAT_Z32_UNORM:
      /* Load: fbZ_reg = memory[depth_tile_reg + offset_reg] */
      spe_lqx(f, fbZ_reg, depth_tile_reg, quad_offset_reg);

      /* convert fragment Z from [0,1] to 32-bit ints */
      spe_cfltu(f, fragZ_reg, fragZ_reg, 32);

      /* No stencil, so can't do anything there */
      break;

   case PIPE_FORMAT_Z16_UNORM:
      /* XXX This code for 16bpp Z is broken! */

      /* Load: fbZS_reg = memory[depth_tile_reg + offset_reg] */
      spe_lqx(f, fbZS_reg, depth_tile_reg, quad_offset_reg);

      /* Copy over 4 32-bit values */
      spe_move(f, fbZ_reg, fbZS_reg);

      /* convert Z from [0,1] to 16-bit ints */
      spe_cfltu(f, fragZ_reg, fragZ_reg, 32);
      spe_rotmi(f, fragZ_reg, fragZ_reg, -16);
      /* No stencil */
      break;

   default:
      ASSERT(0); /* invalid format */
   }

   /* If stencil is enabled, use the stencil-specific code
    * generator to generate both the stencil and depth (if needed)
    * tests.  Otherwise, if only depth is enabled, generate
    * a quick depth test.  The test generators themselves will
    * report back whether the depth/stencil buffer has to be
    * written back.
    */
   if (dsa->stencil[0].enabled) {
      /* This will perform the stencil and depth tests, and update
       * the mask_reg, fbZ_reg, and fbS_reg as required by the
       * tests.
       */
      ASSERT(fbS_reg >= 0);
      spe_comment(f, 0, "Perform stencil test");

      /* Note that fbZ_reg may not be set on entry, if stenciling
       * is enabled but there's no Z-buffer.  The 
       * gen_stencil_depth_test() function must ignore the
       * fbZ_reg register if depth is not enabled.
       */
      write_depth_stencil = gen_stencil_depth_test(f, dsa, stencil_ref, facing,
                                                   mask_reg, fragZ_reg,
                                                   fbZ_reg, fbS_reg);
   }
   else if (dsa->depth.enabled) {
      int zmask_reg = spe_allocate_available_register(f);
      ASSERT(fbZ_reg >= 0);
      spe_comment(f, 0, "Perform depth test");
      write_depth_stencil = gen_depth_test(f, dsa, mask_reg, fragZ_reg,
                                           fbZ_reg, zmask_reg);
      spe_release_register(f, zmask_reg);
   }
   else {
      write_depth_stencil = FALSE;
   }

   if (write_depth_stencil) {
      /* Merge latest Z and Stencil values into fbZS_reg.
       * fbZ_reg has four Z vals in bits [23..0] or bits [15..0].
       * fbS_reg has four 8-bit Z values in bits [7..0].
       */
      spe_comment(f, 0, "Store quad's depth/stencil values in tile");
      if (zs_format == PIPE_FORMAT_Z24S8_UNORM ||
          zs_format == PIPE_FORMAT_Z24X8_UNORM) {
         spe_shli(f, fbS_reg, fbS_reg, 24); /* fbS = fbS << 24 */
         spe_or(f, fbZS_reg, fbS_reg, fbZ_reg); /* fbZS = fbS | fbZ */
      }
      else if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
               zs_format == PIPE_FORMAT_X8Z24_UNORM) {
         spe_shli(f, fbZ_reg, fbZ_reg, 8); /* fbZ = fbZ << 8 */
         spe_or(f, fbZS_reg, fbS_reg, fbZ_reg); /* fbZS = fbS | fbZ */
      }
      else if (zs_format == PIPE_FORMAT_Z32_UNORM) {
         spe_move(f, fbZS_reg, fbZ_reg); /* fbZS = fbZ */
      }
      else if (zs_format == PIPE_FORMAT_Z16_UNORM) {
         spe_move(f, fbZS_reg, fbZ_reg); /* fbZS = fbZ */
      }
      else if (zs_format == PIPE_FORMAT_S8_UNORM) {
         ASSERT(0);   /* XXX to do */
      }
      else {
         ASSERT(0); /* bad zs_format */
      }

      /* Store: memory[depth_tile_reg + quad_offset_reg] = fbZS */
      spe_stqx(f, fbZS_reg, depth_tile_reg, quad_offset_reg);
   }

   /* Don't need these any more */
   spe_release_register(f, fbZS_reg);
   spe_release_register(f, fbZ_reg);
   spe_release_register(f, fbS_reg);
   spe_release_register(f, zmask_reg);
}



/**
 * Generate SPE code to implement the fragment operations (alpha test,
 * depth test, stencil test, blending, colormask, and final
 * framebuffer write) as specified by the current context state.
 *
 * Logically, this code will be called after running the fragment
 * shader.  But under some circumstances we could run some of this
 * code before the fragment shader to cull fragments/quads that are
 * totally occluded/discarded.
 *
 * XXX we only support PIPE_FORMAT_S8Z24_UNORM z/stencil buffer right now.
 *
 * See the spu_default_fragment_ops() function to see how the per-fragment
 * operations would be done with ordinary C code.
 * The code we generate here though has no branches, is SIMD, etc and
 * should be much faster.
 *
 * \param cell  the rendering context (in)
 * \param facing whether the generated code is for front-facing or 
 *              back-facing fragments
 * \param f     the generated function (in/out); on input, the function
 *              must already have been initialized.  On exit, whatever
 *              instructions within the generated function have had
 *              the fragment ops appended.
 */
void
cell_gen_fragment_function(struct cell_context *cell,
                           const uint facing,
                           struct spe_function *f)
{
   const struct pipe_depth_stencil_alpha_state *dsa = cell->depth_stencil;
   const struct pipe_stencil_ref *stencil_ref = &cell->stencil_ref;
   const struct pipe_blend_state *blend = cell->blend;
   const struct pipe_blend_color *blend_color = &cell->blend_color;
   const enum pipe_format color_format = cell->framebuffer.cbufs[0]->format;

   /* For SPE function calls: reg $3 = first param, $4 = second param, etc. */
   const int x_reg = 3;  /* uint */
   const int y_reg = 4;  /* uint */
   const int color_tile_reg = 5;  /* tile_t * */
   const int depth_tile_reg = 6;  /* tile_t * */
   const int fragZ_reg = 7;   /* vector float */
   const int fragR_reg = 8;   /* vector float */
   const int fragG_reg = 9;   /* vector float */
   const int fragB_reg = 10;  /* vector float */
   const int fragA_reg = 11;  /* vector float */
   const int mask_reg = 12;   /* vector uint */

   ASSERT(facing == CELL_FACING_FRONT || facing == CELL_FACING_BACK);

   /* offset of quad from start of tile
    * XXX assuming 4-byte pixels for color AND Z/stencil!!!!
    */
   int quad_offset_reg;

   int fbRGBA_reg;  /**< framebuffer's RGBA colors for quad */

   if (cell->debug_flags & CELL_DEBUG_ASM) {
      spe_print_code(f, TRUE);
      spe_indent(f, 8);
      spe_comment(f, -4, facing == CELL_FACING_FRONT
                  ? "Begin front-facing per-fragment ops"
                  : "Begin back-facing per-fragment ops");
   }

   spe_allocate_register(f, x_reg);
   spe_allocate_register(f, y_reg);
   spe_allocate_register(f, color_tile_reg);
   spe_allocate_register(f, depth_tile_reg);
   spe_allocate_register(f, fragZ_reg);
   spe_allocate_register(f, fragR_reg);
   spe_allocate_register(f, fragG_reg);
   spe_allocate_register(f, fragB_reg);
   spe_allocate_register(f, fragA_reg);
   spe_allocate_register(f, mask_reg);

   quad_offset_reg = spe_allocate_available_register(f);
   fbRGBA_reg = spe_allocate_available_register(f);

   /* compute offset of quad from start of tile, in bytes */
   {
      int x2_reg = spe_allocate_available_register(f);
      int y2_reg = spe_allocate_available_register(f);

      ASSERT(TILE_SIZE == 32);

      spe_comment(f, 0, "Compute quad offset within tile");
      spe_rotmi(f, y2_reg, y_reg, -1);  /* y2 = y / 2 */
      spe_rotmi(f, x2_reg, x_reg, -1);  /* x2 = x / 2 */
      spe_shli(f, y2_reg, y2_reg, 4);   /* y2 *= 16 */
      spe_a(f, quad_offset_reg, y2_reg, x2_reg);  /* offset = y2 + x2 */
      spe_shli(f, quad_offset_reg, quad_offset_reg, 4);   /* offset *= 16 */

      spe_release_register(f, x2_reg);
      spe_release_register(f, y2_reg);
   }

   /* Generate the alpha test, if needed. */
   if (dsa->alpha.enabled) {
      gen_alpha_test(dsa, f, mask_reg, fragA_reg);
   }

   /* generate depth and/or stencil test code */
   if (dsa->depth.enabled || dsa->stencil[0].enabled) {
      gen_depth_stencil(cell, dsa, stencil_ref, f,
                        facing,
                        mask_reg,
                        depth_tile_reg,
                        quad_offset_reg,
                        fragZ_reg);
   }

   /* Get framebuffer quad/colors.  We'll need these for blending,
    * color masking, and to obey the quad/pixel mask.
    * Load: fbRGBA_reg = memory[color_tile + quad_offset]
    * Note: if mask={~0,~0,~0,~0} and we're not blending or colormasking
    * we could skip this load.
    */
   spe_comment(f, 0, "Fetch quad colors from tile");
   spe_lqx(f, fbRGBA_reg, color_tile_reg, quad_offset_reg);

   if (blend->rt[0].blend_enable) {
      spe_comment(f, 0, "Perform blending");
      gen_blend(blend, blend_color, f, color_format,
                fragR_reg, fragG_reg, fragB_reg, fragA_reg, fbRGBA_reg);
   }

   /*
    * Write fragment colors to framebuffer/tile.
    * This involves converting the fragment colors from float[4] to the
    * tile's specific format and obeying the quad/pixel mask.
    */
   {
      int rgba_reg = spe_allocate_available_register(f);

      /* Pack four float colors as four 32-bit int colors */
      spe_comment(f, 0, "Convert float quad colors to packed int framebuffer colors");
      gen_pack_colors(f, color_format,
                      fragR_reg, fragG_reg, fragB_reg, fragA_reg,
                      rgba_reg);

      if (blend->logicop_enable) {
         spe_comment(f, 0, "Compute logic op");
         gen_logicop(blend, f, rgba_reg, fbRGBA_reg);
      }

      if (blend->rt[0].colormask != PIPE_MASK_RGBA) {
         spe_comment(f, 0, "Compute color mask");
         gen_colormask(f, blend->rt[0].colormask, color_format, rgba_reg, fbRGBA_reg);
      }

      /* Mix fragment colors with framebuffer colors using the quad/pixel mask:
       * if (mask[i])
       *    rgba[i] = rgba[i];
       * else
       *    rgba[i] = framebuffer[i];
       */
      spe_selb(f, rgba_reg, fbRGBA_reg, rgba_reg, mask_reg);

      /* Store updated quad in tile:
       * memory[color_tile + quad_offset] = rgba_reg;
       */
      spe_comment(f, 0, "Store quad colors into color tile");
      spe_stqx(f, rgba_reg, color_tile_reg, quad_offset_reg);

      spe_release_register(f, rgba_reg);
   }

   //printf("gen_fragment_ops nr instructions: %u\n", f->num_inst);

   spe_bi(f, SPE_REG_RA, 0, 0);  /* return from function call */

   spe_release_register(f, fbRGBA_reg);
   spe_release_register(f, quad_offset_reg);

   if (cell->debug_flags & CELL_DEBUG_ASM) {
      char buffer[1024];
      sprintf(buffer, "End %s-facing per-fragment ops: %d instructions", 
         facing == CELL_FACING_FRONT ? "front" : "back", f->num_inst);
      spe_comment(f, -4, buffer);
   }
}
