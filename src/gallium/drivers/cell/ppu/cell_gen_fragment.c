/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
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
 */
static void
gen_depth_test(const struct pipe_depth_stencil_alpha_state *dsa,
               struct spe_function *f,
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
   }
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
      spe_load_float(f, ref_reg, dsa->alpha.ref);
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

/* This pair of functions is used inline to allocate and deallocate
 * optional constant registers.  Once a constant is discovered to be 
 * needed, we will likely need it again, so we don't want to deallocate
 * it and have to allocate and load it again unnecessarily.
 */
static inline void
setup_const_register(struct spe_function *f, boolean *is_already_set, unsigned int *r, float value)
{
   if (*is_already_set) return;
   *r = spe_allocate_available_register(f);
   spe_load_float(f, *r, value);
   *is_already_set = true;
}

static inline void
release_const_register(struct spe_function *f, boolean *is_already_set, unsigned int r)
{
    if (!*is_already_set) return;
    spe_release_register(f, r);
    *is_already_set = false;
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
   boolean one_reg_set = false;
   unsigned int one_reg;
   boolean constR_reg_set = false, constG_reg_set = false, 
      constB_reg_set = false, constA_reg_set = false;
   unsigned int constR_reg, constG_reg, constB_reg, constA_reg;

   ASSERT(blend->blend_enable);

   /* Unpack/convert framebuffer colors from four 32-bit packed colors
    * (fbRGBA) to four float RGBA vectors (fbR, fbG, fbB, fbA).
    * Each 8-bit color component is expanded into a float in [0.0, 1.0].
    */
   {
      int mask_reg = spe_allocate_available_register(f);

      /* mask = {0x000000ff, 0x000000ff, 0x000000ff, 0x000000ff} */
      spe_load_int(f, mask_reg, 0xff);

      /* XXX there may be more clever ways to implement the following code */
      switch (color_format) {
      case PIPE_FORMAT_A8R8G8B8_UNORM:
         /* fbB = fbB & mask */
         spe_and(f, fbB_reg, fbRGBA_reg, mask_reg);
         /* mask = mask << 8 */
         spe_roti(f, mask_reg, mask_reg, 8);

         /* fbG = fbRGBA & mask */
         spe_and(f, fbG_reg, fbRGBA_reg, mask_reg);
         /* fbG = fbG >> 8 */
         spe_roti(f, fbG_reg, fbG_reg, -8);
         /* mask = mask << 8 */
         spe_roti(f, mask_reg, mask_reg, 8);

         /* fbR = fbRGBA & mask */
         spe_and(f, fbR_reg, fbRGBA_reg, mask_reg);
         /* fbR = fbR >> 16 */
         spe_roti(f, fbR_reg, fbR_reg, -16);
         /* mask = mask << 8 */
         spe_roti(f, mask_reg, mask_reg, 8);

         /* fbA = fbRGBA & mask */
         spe_and(f, fbA_reg, fbRGBA_reg, mask_reg);
         /* fbA = fbA >> 24 */
         spe_roti(f, fbA_reg, fbA_reg, -24);
         break;

      case PIPE_FORMAT_B8G8R8A8_UNORM:
         /* fbA = fbA & mask */
         spe_and(f, fbA_reg, fbRGBA_reg, mask_reg);
         /* mask = mask << 8 */
         spe_roti(f, mask_reg, mask_reg, 8);

         /* fbR = fbRGBA & mask */
         spe_and(f, fbR_reg, fbRGBA_reg, mask_reg);
         /* fbR = fbR >> 8 */
         spe_roti(f, fbR_reg, fbR_reg, -8);
         /* mask = mask << 8 */
         spe_roti(f, mask_reg, mask_reg, 8);

         /* fbG = fbRGBA & mask */
         spe_and(f, fbG_reg, fbRGBA_reg, mask_reg);
         /* fbG = fbG >> 16 */
         spe_roti(f, fbG_reg, fbG_reg, -16);
         /* mask = mask << 8 */
         spe_roti(f, mask_reg, mask_reg, 8);

         /* fbB = fbRGBA & mask */
         spe_and(f, fbB_reg, fbRGBA_reg, mask_reg);
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

      spe_release_register(f, mask_reg);
   }

   /*
    * Compute Src RGB terms.  We're actually looking for the value
    * of (the appropriate RGB factors) * (the incoming source RGB color),
    * because in some cases (like PIPE_BLENDFACTOR_ONE and 
    * PIPE_BLENDFACTOR_ZERO) we can avoid doing unnecessary math.
    */
   switch (blend->rgb_src_factor) {
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
      setup_const_register(f, &constR_reg_set, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg_set, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg_set, &constB_reg, blend_color->color[2]);
      /* now, factor = (Rc,Gc,Bc), so term = (R*Rc,G*Gc,B*Bc) */
      spe_fm(f, term1R_reg, fragR_reg, constR_reg);
      spe_fm(f, term1G_reg, fragG_reg, constG_reg);
      spe_fm(f, term1B_reg, fragB_reg, constB_reg);
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      /* we'll need the optional constant alpha register */
      setup_const_register(f, &constA_reg_set, &constA_reg, blend_color->color[3]);
      /* factor = (Ac,Ac,Ac), so term = (R*Ac,G*Ac,B*Ac) */
      spe_fm(f, term1R_reg, fragR_reg, constA_reg);
      spe_fm(f, term1G_reg, fragG_reg, constA_reg);
      spe_fm(f, term1B_reg, fragB_reg, constA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg_set, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg_set, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg_set, &constB_reg, blend_color->color[2]);
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
      setup_const_register(f, &constR_reg_set, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg_set, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg_set, &constB_reg, blend_color->color[2]);
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
      setup_const_register(f, &one_reg_set, &one_reg, 1.0f);
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
   switch (blend->alpha_src_factor) {
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
      setup_const_register(f, &constA_reg_set, &constA_reg, blend_color->color[3]);
      /* factor = Ac, so term = A*Ac */
      spe_fm(f, term1A_reg, fragA_reg, constA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constA_reg register */
      setup_const_register(f, &constA_reg_set, &constA_reg, blend_color->color[3]);
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
   switch (blend->rgb_dst_factor) {
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
      setup_const_register(f, &constR_reg_set, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg_set, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg_set, &constB_reg, blend_color->color[2]);
      /* now, factor = (Rc,Gc,Bc), so term = (Rfb*Rc,Gfb*Gc,Bfb*Bc) */
      spe_fm(f, term2R_reg, fbR_reg, constR_reg);
      spe_fm(f, term2G_reg, fbG_reg, constG_reg);
      spe_fm(f, term2B_reg, fbB_reg, constB_reg);
      break;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      /* we'll need the optional constant alpha register */
      setup_const_register(f, &constA_reg_set, &constA_reg, blend_color->color[3]);
      /* factor = (Ac,Ac,Ac), so term = (Rfb*Ac,Gfb*Ac,Bfb*Ac) */
      spe_fm(f, term2R_reg, fbR_reg, constA_reg);
      spe_fm(f, term2G_reg, fbG_reg, constA_reg);
      spe_fm(f, term2B_reg, fbB_reg, constA_reg);
      break;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constant color registers */
      setup_const_register(f, &constR_reg_set, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg_set, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg_set, &constB_reg, blend_color->color[2]);
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
      setup_const_register(f, &constR_reg_set, &constR_reg, blend_color->color[0]);
      setup_const_register(f, &constG_reg_set, &constG_reg, blend_color->color[1]);
      setup_const_register(f, &constB_reg_set, &constB_reg, blend_color->color[2]);
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
   switch (blend->alpha_dst_factor) {
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
      setup_const_register(f, &constA_reg_set, &constA_reg, blend_color->color[3]);
      /* factor = Ac, so term = Afb*Ac */
      spe_fm(f, term2A_reg, fbA_reg, constA_reg);
      break;

   case PIPE_BLENDFACTOR_INV_CONST_ALPHA: /* fall through */
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      /* We need the optional constA_reg register */
      setup_const_register(f, &constA_reg_set, &constA_reg, blend_color->color[3]);
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
   switch (blend->rgb_func) {
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
   switch (blend->alpha_func) {
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
   release_const_register(f, &one_reg_set, one_reg);
   release_const_register(f, &constR_reg_set, constR_reg);
   release_const_register(f, &constG_reg_set, constG_reg);
   release_const_register(f, &constB_reg_set, constB_reg);
   release_const_register(f, &constA_reg_set, constA_reg);
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


static void
gen_colormask(uint colormask,
              struct spe_function *f,
              int fragRGBA_reg, int fbRGBA_reg)
{
   /* We've got four 32-bit RGBA packed pixels in each of
    * fragRGBA_reg and fbRGBA_reg, not sets of floating-point
    * reds, greens, blues, and alphas.
    * */

   /* The color mask operation can prevent any set of color
    * components in the incoming fragment from being written to the frame 
    * buffer; we do this by replacing the masked components of the 
    * fragment with the frame buffer values.
    *
    * There are only 16 possibilities, with a unique mask for
    * each of the possibilities.  (Technically, there are only 15
    * possibilities, since we shouldn't be called for the one mask
    * that does nothing, but the complete implementation is here
    * anyway to avoid confusion.)
    *
    * We implement this via a constant static array which we'll index 
    * into to get the correct mask.
    * 
    * We're dependent on the mask values being low-order bits,
    * with particular values for each bit; so we start with a
    * few assertions, which will fail if any of the values were
    * to change.
    */
   ASSERT(PIPE_MASK_R == 0x1);
   ASSERT(PIPE_MASK_G == 0x2);
   ASSERT(PIPE_MASK_B == 0x4);
   ASSERT(PIPE_MASK_A == 0x8);

   /* Here's the list of all possible colormasks, indexed by the
    * value of the combined mask specifier.
    */
   static const unsigned int colormasks[16] = {
      0x00000000, /* 0: all colors masked */
      0xff000000, /* 1: PIPE_MASK_R */
      0x00ff0000, /* 2: PIPE_MASK_G */
      0xffff0000, /* 3: PIPE_MASK_R | PIPE_MASK_G */
      0x0000ff00, /* 4: PIPE_MASK_B */
      0xff00ff00, /* 5: PIPE_MASK_R | PIPE_MASK_B */
      0x00ffff00, /* 6: PIPE_MASK_G | PIPE_MASK_B */
      0xffffff00, /* 7: PIPE_MASK_R | PIPE_MASK_G | PIPE_MASK_B */
      0x000000ff, /* 8: PIPE_MASK_A */
      0xff0000ff, /* 9: PIPE_MASK_R | PIPE_MASK_A */
      0x00ff00ff, /* 10: PIPE_MASK_G | PIPE_MASK_A */
      0xffff00ff, /* 11: PIPE_MASK_R | PIPE_MASK_G | PIPE_MASK_A */
      0x0000ffff, /* 12: PIPE_MASK_B | PIPE_MASK_A */
      0xff00ffff, /* 13: PIPE_MASK_R | PIPE_MASK_B | PIPE_MASK_A */
      0x00ffffff, /* 14: PIPE_MASK_G | PIPE_MASK_B | PIPE_MASK_A */
      0xffffffff  /* 15: PIPE_MASK_R | PIPE_MASK_G | PIPE_MASK_B | PIPE_MASK_A */
   };

   /* Get a temporary register to hold the mask */
   int colormask_reg = spe_allocate_available_register(f);

   /* Look up the desired mask directly and load it into the mask register.
    * This will load the same mask into each of the four words in the
    * mask register.
    */
   spe_load_uint(f, colormask_reg, colormasks[colormask]);

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
   if (color_format == PIPE_FORMAT_A8R8G8B8_UNORM) {
      spe_roti(f, g_reg, g_reg, 8);   /* green <<= 8 */
      spe_roti(f, r_reg, r_reg, 16);  /* red <<= 16 */
      spe_roti(f, a_reg, a_reg, 24);  /* alpha <<= 24 */
   }
   else if (color_format == PIPE_FORMAT_B8G8R8A8_UNORM) {
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
 * XXX we only support PIPE_FORMAT_Z24S8_UNORM z/stencil buffer right now.
 *
 * See the spu_default_fragment_ops() function to see how the per-fragment
 * operations would be done with ordinary C code.
 * The code we generate here though has no branches, is SIMD, etc and
 * should be much faster.
 *
 * \param cell  the rendering context (in)
 * \param f     the generated function (out)
 */
void
cell_gen_fragment_function(struct cell_context *cell, struct spe_function *f)
{
   const struct pipe_depth_stencil_alpha_state *dsa =
      &cell->depth_stencil->base;
   const struct pipe_blend_state *blend = &cell->blend->base;
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

   /* offset of quad from start of tile
    * XXX assuming 4-byte pixels for color AND Z/stencil!!!!
    */
   int quad_offset_reg;

   int fbRGBA_reg;  /**< framebuffer's RGBA colors for quad */
   int fbZS_reg;    /**< framebuffer's combined z/stencil values for quad */

   spe_init_func(f, SPU_MAX_FRAGMENT_OPS_INSTS * SPE_INST_SIZE);

   if (cell->debug_flags & CELL_DEBUG_ASM) {
      spe_print_code(f, true);
      spe_indent(f, 8);
      spe_comment(f, -4, "Begin per-fragment ops");
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
   fbZS_reg = spe_allocate_available_register(f);

   /* compute offset of quad from start of tile, in bytes */
   {
      int x2_reg = spe_allocate_available_register(f);
      int y2_reg = spe_allocate_available_register(f);

      ASSERT(TILE_SIZE == 32);

      spe_rotmi(f, x2_reg, x_reg, -1);  /* x2 = x / 2 */
      spe_rotmi(f, y2_reg, y_reg, -1);  /* y2 = y / 2 */
      spe_shli(f, y2_reg, y2_reg, 4);   /* y2 *= 16 */
      spe_a(f, quad_offset_reg, y2_reg, x2_reg);  /* offset = y2 + x2 */
      spe_shli(f, quad_offset_reg, quad_offset_reg, 4);   /* offset *= 16 */

      spe_release_register(f, x2_reg);
      spe_release_register(f, y2_reg);
   }


   if (dsa->alpha.enabled) {
      gen_alpha_test(dsa, f, mask_reg, fragA_reg);
   }

   if (dsa->depth.enabled || dsa->stencil[0].enabled) {
      const enum pipe_format zs_format = cell->framebuffer.zsbuf->format;
      boolean write_depth_stencil;

      int fbZ_reg = spe_allocate_available_register(f); /* Z values */
      int fbS_reg = spe_allocate_available_register(f); /* Stencil values */

      /* fetch quad of depth/stencil values from tile at (x,y) */
      /* Load: fbZS_reg = memory[depth_tile_reg + offset_reg] */
      spe_lqx(f, fbZS_reg, depth_tile_reg, quad_offset_reg);

      if (dsa->depth.enabled) {
         /* Extract Z bits from fbZS_reg into fbZ_reg */
         if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
             zs_format == PIPE_FORMAT_X8Z24_UNORM) {
            int mask_reg = spe_allocate_available_register(f);
            spe_fsmbi(f, mask_reg, 0x7777);  /* mask[0,1,2,3] = 0x00ffffff */
            spe_and(f, fbZ_reg, fbZS_reg, mask_reg);  /* fbZ = fbZS & mask */
            spe_release_register(f, mask_reg);
            /* OK, fbZ_reg has four 24-bit Z values now */
         }
         else if (zs_format == PIPE_FORMAT_Z24S8_UNORM ||
                  zs_format == PIPE_FORMAT_Z24X8_UNORM) {
            spe_rotmi(f, fbZ_reg, fbZS_reg, -8);  /* fbZ = fbZS >> 8 */
            /* OK, fbZ_reg has four 24-bit Z values now */
         }
         else if (zs_format == PIPE_FORMAT_Z32_UNORM) {
            spe_move(f, fbZ_reg, fbZS_reg);
            /* OK, fbZ_reg has four 32-bit Z values now */
         }
         else if (zs_format == PIPE_FORMAT_Z16_UNORM) {
            spe_move(f, fbZ_reg, fbZS_reg);
            /* OK, fbZ_reg has four 16-bit Z values now */
         }
         else {
            ASSERT(0);  /* invalid format */
         }

         /* Convert fragZ values from float[4] to 16, 24 or 32-bit uint[4] */
         if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
             zs_format == PIPE_FORMAT_X8Z24_UNORM ||
             zs_format == PIPE_FORMAT_Z24S8_UNORM ||
             zs_format == PIPE_FORMAT_Z24X8_UNORM) {
            /* scale/convert fragZ from float in [0,1] to uint in [0, ~0] */
            spe_cfltu(f, fragZ_reg, fragZ_reg, 32);
            /* fragZ = fragZ >> 8 */
            spe_rotmi(f, fragZ_reg, fragZ_reg, -8);
         }
         else if (zs_format == PIPE_FORMAT_Z32_UNORM) {
            /* scale/convert fragZ from float in [0,1] to uint in [0, ~0] */
            spe_cfltu(f, fragZ_reg, fragZ_reg, 32);
         }
         else if (zs_format == PIPE_FORMAT_Z16_UNORM) {
            /* scale/convert fragZ from float in [0,1] to uint in [0, ~0] */
            spe_cfltu(f, fragZ_reg, fragZ_reg, 32);
            /* fragZ = fragZ >> 16 */
            spe_rotmi(f, fragZ_reg, fragZ_reg, -16);
         }
      }
      else {
         /* no Z test, but set Z to zero so we don't OR-in garbage below */
         spe_load_uint(f, fbZ_reg, 0); /* XXX set to zero for now */
      }


      if (dsa->stencil[0].enabled) {
         /* Extract Stencil bit sfrom fbZS_reg into fbS_reg */
         if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
             zs_format == PIPE_FORMAT_X8Z24_UNORM) {
            /* XXX extract with a shift */
            ASSERT(0);
         }
         else if (zs_format == PIPE_FORMAT_Z24S8_UNORM ||
                  zs_format == PIPE_FORMAT_Z24X8_UNORM) {
            /* XXX extract with a mask */
            ASSERT(0);
         }
      }
      else {
         /* no stencil test, but set to zero so we don't OR-in garbage below */
         spe_load_uint(f, fbS_reg, 0); /* XXX set to zero for now */
      }

      if (dsa->stencil[0].enabled) {
         /* XXX this may involve depth testing too */
         // gen_stencil_test(dsa, f, ... );
         ASSERT(0);
      }
      else if (dsa->depth.enabled) {
         int zmask_reg = spe_allocate_available_register(f);
         gen_depth_test(dsa, f, mask_reg, fragZ_reg, fbZ_reg, zmask_reg);
         spe_release_register(f, zmask_reg);
      }

      /* do we need to write Z and/or Stencil back into framebuffer? */
      write_depth_stencil = (dsa->depth.writemask |
                             dsa->stencil[0].write_mask |
                             dsa->stencil[1].write_mask);

      if (write_depth_stencil) {
         /* Merge latest Z and Stencil values into fbZS_reg.
          * fbZ_reg has four Z vals in bits [23..0] or bits [15..0].
          * fbS_reg has four 8-bit Z values in bits [7..0].
          */
         if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
             zs_format == PIPE_FORMAT_X8Z24_UNORM) {
            spe_shli(f, fbS_reg, fbS_reg, 24); /* fbS = fbS << 24 */
            spe_or(f, fbZS_reg, fbS_reg, fbZ_reg); /* fbZS = fbS | fbZ */
         }
         else if (zs_format == PIPE_FORMAT_Z24S8_UNORM ||
                  zs_format == PIPE_FORMAT_Z24X8_UNORM) {
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

      spe_release_register(f, fbZ_reg);
      spe_release_register(f, fbS_reg);
   }


   /* Get framebuffer quad/colors.  We'll need these for blending,
    * color masking, and to obey the quad/pixel mask.
    * Load: fbRGBA_reg = memory[color_tile + quad_offset]
    * Note: if mask={~0,~0,~0,~0} and we're not blending or colormasking
    * we could skip this load.
    */
   spe_lqx(f, fbRGBA_reg, color_tile_reg, quad_offset_reg);


   if (blend->blend_enable) {
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
      gen_pack_colors(f, color_format,
                      fragR_reg, fragG_reg, fragB_reg, fragA_reg,
                      rgba_reg);

      if (blend->logicop_enable) {
         gen_logicop(blend, f, rgba_reg, fbRGBA_reg);
      }

      if (blend->colormask != PIPE_MASK_RGBA) {
         gen_colormask(blend->colormask, f, rgba_reg, fbRGBA_reg);
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
      spe_stqx(f, rgba_reg, color_tile_reg, quad_offset_reg);

      spe_release_register(f, rgba_reg);
   }

   //printf("gen_fragment_ops nr instructions: %u\n", f->num_inst);

   spe_bi(f, SPE_REG_RA, 0, 0);  /* return from function call */


   spe_release_register(f, fbRGBA_reg);
   spe_release_register(f, fbZS_reg);
   spe_release_register(f, quad_offset_reg);

   if (cell->debug_flags & CELL_DEBUG_ASM) {
      spe_comment(f, -4, "End per-fragment ops");
   }
}
