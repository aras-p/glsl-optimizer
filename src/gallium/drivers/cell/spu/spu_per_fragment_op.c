/*
 * (C) Copyright IBM Corporation 2008
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * on the rights to use, copy, modify, merge, publish, distribute, sub
 * license, and/or sell copies of the Software, and to permit persons to whom
 * the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * AUTHORS, COPYRIGHT HOLDERS, AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/**
 * \file spu_per_fragment_op.c
 * SPU implementation various per-fragment operations.
 *
 * \author Ian Romanick <idr@us.ibm.com>
 */


#include <transpose_matrix4x4.h>
#include "pipe/p_format.h"
#include "spu_main.h"
#include "spu_colorpack.h"
#include "spu_per_fragment_op.h"

#define ZERO 0x80


/**
 * Get a "quad" of four fragment Z/stencil values from the given tile.
 * \param tile  the tile of Z/stencil values
 * \param x, y  location of the quad in the tile, in pixels
 * \param depth_format  format of the tile's data
 * \param detph  returns four depth values
 * \param stencil  returns four stencil values
 */
static void
read_ds_quad(tile_t *tile, unsigned x, unsigned y,
             enum pipe_format depth_format, qword *depth,
             qword *stencil)
{
   const int ix = x / 2;
   const int iy = y / 2;

   switch (depth_format) {
   case PIPE_FORMAT_Z16_UNORM: {
      qword *ptr = (qword *) &tile->us8[iy][ix / 2];

      const qword shuf_vec = (qword) {
         ZERO, ZERO, 0, 1, ZERO, ZERO, 2, 3,
         ZERO, ZERO, 4, 5, ZERO, ZERO, 6, 7
      };

      /* At even X values we want the first 4 shorts, and at odd X values we
       * want the second 4 shorts.
       */
      qword bias = (qword) spu_splats((unsigned char) ((ix & 0x01) << 3));
      qword bias_mask = si_fsmbi(0x3333);
      qword sv = si_a(shuf_vec, si_and(bias_mask, bias));

      *depth = si_shufb(*ptr, *ptr, sv);
      *stencil = si_il(0);
      break;
   }

   case PIPE_FORMAT_Z32_UNORM: {
      qword *ptr = (qword *) &tile->ui4[iy][ix];

      *depth = *ptr;
      *stencil = si_il(0);
      break;
   }

   case PIPE_FORMAT_Z24S8_UNORM: {
      qword *ptr = (qword *) &tile->ui4[iy][ix];
      qword mask = si_fsmbi(0xEEEE);

      *depth = si_rotmai(si_and(*ptr, mask), -8);
      *stencil = si_andc(*ptr, mask);
      break;
   }

   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM: {
      qword *ptr = (qword *) &tile->ui4[iy][ix];

      *depth = si_and(*ptr, si_fsmbi(0x7777));
      *stencil = si_andi(si_roti(*ptr, 8), 0x0ff);
      break;
   }

   default:
      ASSERT(0);
      break;
   }
}


/**
 * Put a quad of Z/stencil values into a tile.
 * \param tile  the tile of Z/stencil values to write into
 * \param x, y  location of the quad in the tile, in pixels
 * \param depth_format  format of the tile's data
 * \param detph  depth values to store
 * \param stencil  stencil values to store
 */
static void
write_ds_quad(tile_t *buffer, unsigned x, unsigned y,
              enum pipe_format depth_format,
              qword depth, qword stencil)
{
   const int ix = x / 2;
   const int iy = y / 2;

   (void) stencil;

   switch (depth_format) {
   case PIPE_FORMAT_Z16_UNORM: {
      qword *ptr = (qword *) &buffer->us8[iy][ix / 2];

      qword sv = ((ix & 0x01) == 0) 
          ? (qword) { 2, 3, 6, 7, 10, 11, 14, 15,
                      24, 25, 26, 27, 28, 29, 30, 31 }
          : (qword) { 16, 17, 18, 19, 20 , 21, 22, 23,
                      2, 3, 6, 7, 10, 11, 14, 15 };
      *ptr = si_shufb(depth, *ptr, sv);
      break;
   }

   case PIPE_FORMAT_Z32_UNORM: {
      qword *ptr = (qword *) &buffer->ui4[iy][ix];
      *ptr = depth;
      break;
   }

   case PIPE_FORMAT_Z24S8_UNORM: {
      qword *ptr = (qword *) &buffer->ui4[iy][ix];
      /* form select mask = 1110,1110,1110,1110 */
      qword mask = si_fsmbi(0xEEEE);
      /* depth[i] = depth[i] << 8 */
      depth = si_shli(depth, 8);
      /* *ptr[i] = depth[i][31:8] | stencil[i][7:0] */
      *ptr = si_selb(stencil, depth, mask);
      break;
   }

   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM: {
      qword *ptr = (qword *) &buffer->ui4[iy][ix];
      /* form select mask = 0111,0111,0111,0111 */
      qword mask = si_fsmbi(0x7777);
      /* stencil[i] = stencil[i] << 24 */
      stencil = si_shli(stencil, 24);
      /* *ptr[i] = stencil[i][31:24] | depth[i][23:0] */
      *ptr = si_selb(stencil, depth, mask);
      break;
   }

   default:
      ASSERT(0);
      break;
   }
}


/**
 * Do depth/stencil/alpha test for a "quad" of 4 fragments.
 * \param x,y  location of quad within tile
 * \param frag_mask  indicates which fragments are "alive"
 * \param frag_depth  four fragment depth values
 * \param frag_alpha  four fragment alpha values
 * \param facing  front/back facing for four fragments (1=front, 0=back)
 */
qword
spu_do_depth_stencil(int x, int y,
                     qword frag_mask, qword frag_depth, qword frag_alpha,
                     qword facing)
{
   struct spu_frag_test_results  result;
   qword pixel_depth;
   qword pixel_stencil;

   /* All of this preable code (everthing before the call to frag_test) should
    * be generated on the PPU and upload to the SPU.
    */
   if (spu.read_depth || spu.read_stencil) {
      read_ds_quad(&spu.ztile, x, y, spu.fb.depth_format,
                   &pixel_depth, &pixel_stencil);
   }

   /* convert floating point Z values to 32-bit uint */

   /* frag_depth *= spu.fb.zscale */
   frag_depth = si_fm(frag_depth, (qword)spu_splats(spu.fb.zscale));
   /* frag_depth = uint(frag_depth) */
   frag_depth = si_cfltu(frag_depth, 0);

   result = (*spu.frag_test)(frag_mask, pixel_depth, pixel_stencil,
                             frag_depth, frag_alpha, facing);


   /* This code (everthing after the call to frag_test) should
    * be generated on the PPU and upload to the SPU.
    */
   if (spu.read_depth || spu.read_stencil) {
      write_ds_quad(&spu.ztile, x, y, spu.fb.depth_format,
                    result.depth, result.stencil);
   }

   return result.mask;
}




/**
 * Called by rasterizer for each quad after the shader has run.  This
 * is a fallback/debug function.  In reality we'll use a generated
 * function produced by the PPU.  But this function is useful for
 * debug/validation.
 */
void
spu_fallback_fragment_ops(uint x, uint y,
                          tile_t *colorTile,
                          tile_t *depthStencilTile,
                          vector float fragZ,
                          vector float fragRed,
                          vector float fragGreen,
                          vector float fragBlue,
                          vector float fragAlpha,
                          vector unsigned int mask)
{
   vector float frag_soa[4], frag_aos[4];
   unsigned int c0, c1, c2, c3;

   /* do alpha test */
   if (spu.depth_stencil_alpha.alpha.enabled) {
      vector float ref = spu_splats(spu.depth_stencil_alpha.alpha.ref);
      vector unsigned int amask;

      switch (spu.depth_stencil_alpha.alpha.func) {
      case PIPE_FUNC_LESS:
         amask = spu_cmpgt(ref, fragAlpha);  /* mask = (fragAlpha < ref) */
         break;
      case PIPE_FUNC_GREATER:
         amask = spu_cmpgt(fragAlpha, ref);  /* mask = (fragAlpha > ref) */
         break;
      case PIPE_FUNC_GEQUAL:
         amask = spu_cmpgt(ref, fragAlpha);
         amask = spu_nor(amask, amask);
         break;
      case PIPE_FUNC_LEQUAL:
         amask = spu_cmpgt(fragAlpha, ref);
         amask = spu_nor(amask, amask);
         break;
      case PIPE_FUNC_EQUAL:
         amask = spu_cmpeq(ref, fragAlpha);
         break;
      case PIPE_FUNC_NOTEQUAL:
         amask = spu_cmpeq(ref, fragAlpha);
         amask = spu_nor(amask, amask);
         break;
      case PIPE_FUNC_ALWAYS:
         amask = spu_splats(0xffffffffU);
         break;
      case PIPE_FUNC_NEVER:
         amask = spu_splats( 0x0U);
         break;
      default:
         ;
      }

      mask = spu_and(mask, amask);
   }

   /* Z and/or stencil testing... */
   if (spu.depth_stencil_alpha.depth.enabled ||
       spu.depth_stencil_alpha.stencil[0].enabled) {

      /* get four Z/Stencil values from tile */
      vector unsigned int mask24 = spu_splats((unsigned int)0x00ffffffU);
      vector unsigned int ifbZS = depthStencilTile->ui4[y/2][x/2];
      vector unsigned int ifbZ = spu_and(ifbZS, mask24);
      vector unsigned int ifbS = spu_andc(ifbZS, mask24);

      if (spu.depth_stencil_alpha.stencil[0].enabled) {
         /* do stencil test */
         ASSERT(spu.fb.depth_format == PIPE_FORMAT_S8Z24_UNORM);

      }
      else if (spu.depth_stencil_alpha.depth.enabled) {
         /* do depth test */

         ASSERT(spu.fb.depth_format == PIPE_FORMAT_S8Z24_UNORM ||
                spu.fb.depth_format == PIPE_FORMAT_X8Z24_UNORM);

         vector unsigned int ifragZ;
         vector unsigned int zmask;

         /* convert four fragZ from float to uint */
         fragZ = spu_mul(fragZ, spu_splats((float) 0xffffff));
         ifragZ = spu_convtu(fragZ, 0);

         /* do depth comparison, setting zmask with results */
         switch (spu.depth_stencil_alpha.depth.func) {
         case PIPE_FUNC_LESS:
            zmask = spu_cmpgt(ifbZ, ifragZ);  /* mask = (ifragZ < ifbZ) */
            break;
         case PIPE_FUNC_GREATER:
            zmask = spu_cmpgt(ifragZ, ifbZ);  /* mask = (ifbZ > ifragZ) */
            break;
         case PIPE_FUNC_GEQUAL:
            zmask = spu_cmpgt(ifbZ, ifragZ);
            zmask = spu_nor(zmask, zmask);
            break;
         case PIPE_FUNC_LEQUAL:
            zmask = spu_cmpgt(ifragZ, ifbZ);
            zmask = spu_nor(zmask, zmask);
            break;
         case PIPE_FUNC_EQUAL:
            zmask = spu_cmpeq(ifbZ, ifragZ);
            break;
         case PIPE_FUNC_NOTEQUAL:
            zmask = spu_cmpeq(ifbZ, ifragZ);
            zmask = spu_nor(zmask, zmask);
            break;
         case PIPE_FUNC_ALWAYS:
            zmask = spu_splats(0xffffffffU);
            break;
         case PIPE_FUNC_NEVER:
            zmask = spu_splats( 0x0U);
            break;
         default:
            ;
         }

         mask = spu_and(mask, zmask);

         /* merge framebuffer Z and fragment Z according to the mask */
         ifbZ = spu_or(spu_and(ifragZ, mask),
                       spu_andc(ifbZ, mask));
      }

      if (spu_extract(spu_orx(mask), 0)) {
         /* put new fragment Z/Stencil values back into Z/Stencil tile */
         depthStencilTile->ui4[y/2][x/2] = spu_or(ifbZ, ifbS);

         spu.cur_ztile_status = TILE_STATUS_DIRTY;
      }
   }

   /* XXX do blending here */

   /* XXX do colormask test here */


   if (spu_extract(spu_orx(mask), 0)) {
      spu.cur_ctile_status = TILE_STATUS_DIRTY;
   }
   else {
      return;
   }

   /* convert RRRR,GGGG,BBBB,AAAA to RGBA,RGBA,RGBA,RGBA */
#if 0
   {
      vector float frag_soa[4];
      frag_soa[0] = fragRed;
      frag_soa[1] = fragGreen;
      frag_soa[2] = fragBlue;
      frag_soa[3] = fragAlpha;
      _transpose_matrix4x4(frag_aos, frag_soa);
   }
#else
   /* short-cut relying on function parameter layout: */
   _transpose_matrix4x4(frag_aos, &fragRed);
   (void) fragGreen;
   (void) fragBlue;
#endif

   switch (spu.fb.color_format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      c0 = spu_pack_A8R8G8B8(frag_aos[0]);
      c1 = spu_pack_A8R8G8B8(frag_aos[1]);
      c2 = spu_pack_A8R8G8B8(frag_aos[2]);
      c3 = spu_pack_A8R8G8B8(frag_aos[3]);
      break;

   case PIPE_FORMAT_B8G8R8A8_UNORM:
      c0 = spu_pack_B8G8R8A8(frag_aos[0]);
      c1 = spu_pack_B8G8R8A8(frag_aos[1]);
      c2 = spu_pack_B8G8R8A8(frag_aos[2]);
      c3 = spu_pack_B8G8R8A8(frag_aos[3]);
      break;
   default:
      fprintf(stderr, "SPU: Bad pixel format in spu_default_fragment_ops\n");
      ASSERT(0);
   }

#if 0
   /*
    * Quad layout:
    *  +--+--+
    *  |p0|p1|
    *  +--+--+
    *  |p2|p3|
    *  +--+--+
    */
   if (spu_extract(mask, 0))
      colorTile->ui[y+0][x+0] = c0;
   if (spu_extract(mask, 1))
      colorTile->ui[y+0][x+1] = c1;
   if (spu_extract(mask, 2))
      colorTile->ui[y+1][x+0] = c2;
   if (spu_extract(mask, 3))
      colorTile->ui[y+1][x+1] = c3;   
#else
   /*
    * Quad layout:
    *  +--+--+--+--+
    *  |p0|p1|p2|p3|
    *  +--+--+--+--+
    */
   if (spu_extract(mask, 0))
      colorTile->ui[y][x*2] = c0;
   if (spu_extract(mask, 1))
      colorTile->ui[y][x*2+1] = c1;
   if (spu_extract(mask, 2))
      colorTile->ui[y][x*2+2] = c2;
   if (spu_extract(mask, 3))
      colorTile->ui[y][x*2+3] = c3;   
#endif
}
