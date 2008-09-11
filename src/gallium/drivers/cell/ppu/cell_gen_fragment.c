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
      spe_cgt(f, zmask_reg, ifragZ_reg, ifbZ_reg);
      /* mask = (mask & zmask) */
      spe_and(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_LESS:
      /* zmask = (ref > ifragZ) */
      spe_cgt(f, zmask_reg, ifbZ_reg, ifragZ_reg);
      /* mask = (mask & zmask) */
      spe_and(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_LEQUAL:
      /* zmask = (ifragZ > ref) */
      spe_cgt(f, zmask_reg, ifragZ_reg, ifbZ_reg);
      /* mask = (mask & ~zmask) */
      spe_andc(f, mask_reg, mask_reg, zmask_reg);
      break;

   case PIPE_FUNC_GEQUAL:
      /* zmask = (ref > ifragZ) */
      spe_cgt(f, zmask_reg, ifbZ_reg, ifragZ_reg);
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
gen_fragment_function(struct cell_context *cell, struct spe_function *f)
{
   const struct pipe_depth_stencil_alpha_state *dsa =
      &cell->depth_stencil->base;
   const struct pipe_blend_state *blend = &cell->blend->base;

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
         else {
            /* XXX handle other z/stencil formats */
            ASSERT(0);
         }

         /* Convert fragZ values from float[4] to uint[4] */
         if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
             zs_format == PIPE_FORMAT_X8Z24_UNORM ||
             zs_format == PIPE_FORMAT_Z24S8_UNORM ||
             zs_format == PIPE_FORMAT_Z24X8_UNORM) {
            /* 24-bit Z values */
            int scale_reg = spe_allocate_available_register(f);

            /* scale_reg[0,1,2,3] = float(2^24-1) */
            spe_load_float(f, scale_reg, (float) 0xffffff);

            /* XXX these two instructions might be combined */
            spe_fm(f, fragZ_reg, fragZ_reg, scale_reg); /* fragZ *= scale */
            spe_cfltu(f, fragZ_reg, fragZ_reg, 0);  /* fragZ = (int) fragZ */

            spe_release_register(f, scale_reg);
         }
         else {
            /* XXX handle 16-bit Z format */
            ASSERT(0);
         }
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
         else if (zs_format == PIPE_FORMAT_S8Z24_UNORM ||
                  zs_format == PIPE_FORMAT_X8Z24_UNORM) {
            /* XXX to do */
            ASSERT(0);
         }
         else if (zs_format == PIPE_FORMAT_Z16_UNORM) {
            /* XXX to do */
            ASSERT(0);
         }
         else if (zs_format == PIPE_FORMAT_S8_UNORM) {
            /* XXX to do */
            ASSERT(0);
         }
         else {
            /* bad zs_format */
            ASSERT(0);
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
      /* convert packed tile colors in fbRGBA_reg to float[4] vectors */

      // gen_blend_code(blend, f, mask_reg, ... );

   }



   /*
    * Write fragment colors to framebuffer/tile.
    * This involves converting the fragment colors from float[4] to the
    * tile's specific format and obeying the quad/pixel mask.
    */
   {
      const enum pipe_format color_format = cell->framebuffer.cbufs[0]->format;
      int rgba_reg = spe_allocate_available_register(f);

      /* Convert float[4] in [0.0,1.0] to int[4] in [0,~0], with clamping */
      spe_cfltu(f, fragR_reg, fragR_reg, 32);
      spe_cfltu(f, fragG_reg, fragG_reg, 32);
      spe_cfltu(f, fragB_reg, fragB_reg, 32);
      spe_cfltu(f, fragA_reg, fragA_reg, 32);

      /* Shift most the significant bytes to least the significant positions.
       * I.e.: reg = reg >> 24
       */
      spe_rotmi(f, fragR_reg, fragR_reg, -24);
      spe_rotmi(f, fragG_reg, fragG_reg, -24);
      spe_rotmi(f, fragB_reg, fragB_reg, -24);
      spe_rotmi(f, fragA_reg, fragA_reg, -24);

      /* Shift the color bytes according to the surface format */
      if (color_format == PIPE_FORMAT_A8R8G8B8_UNORM) {
         spe_roti(f, fragG_reg, fragG_reg, 8);   /* green <<= 8 */
         spe_roti(f, fragR_reg, fragR_reg, 16);  /* red <<= 16 */
         spe_roti(f, fragA_reg, fragA_reg, 24);  /* alpha <<= 24 */
      }
      else if (color_format == PIPE_FORMAT_B8G8R8A8_UNORM) {
         spe_roti(f, fragR_reg, fragR_reg, 8);   /* red <<= 8 */
         spe_roti(f, fragG_reg, fragG_reg, 16);  /* green <<= 16 */
         spe_roti(f, fragB_reg, fragB_reg, 24);  /* blue <<= 24 */
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
      spe_or(f, rgba_reg, fragR_reg, fragG_reg);
      spe_or(f, rgba_reg, rgba_reg, fragB_reg);
      spe_or(f, rgba_reg, rgba_reg, fragA_reg);

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

   printf("gen_fragment_ops nr instructions: %u\n", f->num_inst);

   spe_bi(f, SPE_REG_RA, 0, 0);  /* return from function call */


   spe_release_register(f, fbRGBA_reg);
   spe_release_register(f, fbZS_reg);
   spe_release_register(f, quad_offset_reg);
}

