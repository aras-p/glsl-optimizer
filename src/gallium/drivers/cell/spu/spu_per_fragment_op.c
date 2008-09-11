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
 * \author Brian Paul
 */


#include <transpose_matrix4x4.h>
#include "pipe/p_format.h"
#include "spu_main.h"
#include "spu_colorpack.h"
#include "spu_per_fragment_op.h"



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
