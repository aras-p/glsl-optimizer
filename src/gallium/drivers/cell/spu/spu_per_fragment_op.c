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


#define LINEAR_QUAD_LAYOUT 1


static INLINE vector float
spu_min(vector float a, vector float b)
{
   vector unsigned int m;
   m = spu_cmpgt(a, b);    /* m = a > b ? ~0 : 0 */
   return spu_sel(a, b, m);
}


static INLINE vector float
spu_max(vector float a, vector float b)
{
   vector unsigned int m;
   m = spu_cmpgt(a, b);    /* m = a > b ? ~0 : 0 */
   return spu_sel(b, a, m);
}


/**
 * Called by rasterizer for each quad after the shader has run.  Do
 * all the per-fragment operations including alpha test, z test,
 * stencil test, blend, colormask and logicops.  This is a
 * fallback/debug function.  In reality we'll use a generated function
 * produced by the PPU.  But this function is useful for
 * debug/validation.
 */
void
spu_fallback_fragment_ops(uint x, uint y,
                          tile_t *colorTile,
                          tile_t *depthStencilTile,
                          vector float fragZ,
                          vector float fragR,
                          vector float fragG,
                          vector float fragB,
                          vector float fragA,
                          vector unsigned int mask)
{
   vector float frag_aos[4];
   unsigned int fbc0, fbc1, fbc2, fbc3 ; /* framebuffer/tile colors */
   unsigned int fragc0, fragc1, fragc2, fragc3;  /* fragment colors */

   /*
    * Do alpha test
    */
   if (spu.depth_stencil_alpha.alpha.enabled) {
      vector float ref = spu_splats(spu.depth_stencil_alpha.alpha.ref_value);
      vector unsigned int amask;

      switch (spu.depth_stencil_alpha.alpha.func) {
      case PIPE_FUNC_LESS:
         amask = spu_cmpgt(ref, fragA);  /* mask = (fragA < ref) */
         break;
      case PIPE_FUNC_GREATER:
         amask = spu_cmpgt(fragA, ref);  /* mask = (fragA > ref) */
         break;
      case PIPE_FUNC_GEQUAL:
         amask = spu_cmpgt(ref, fragA);
         amask = spu_nor(amask, amask);
         break;
      case PIPE_FUNC_LEQUAL:
         amask = spu_cmpgt(fragA, ref);
         amask = spu_nor(amask, amask);
         break;
      case PIPE_FUNC_EQUAL:
         amask = spu_cmpeq(ref, fragA);
         break;
      case PIPE_FUNC_NOTEQUAL:
         amask = spu_cmpeq(ref, fragA);
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


   /*
    * Z and/or stencil testing...
    */
   if (spu.depth_stencil_alpha.depth.enabled ||
       spu.depth_stencil_alpha.stencil[0].enabled) {

      /* get four Z/Stencil values from tile */
      vector unsigned int mask24 = spu_splats((unsigned int)0x00ffffffU);
      vector unsigned int ifbZS = depthStencilTile->ui4[y/2][x/2];
      vector unsigned int ifbZ = spu_and(ifbZS, mask24);
      vector unsigned int ifbS = spu_andc(ifbZS, mask24);

      if (spu.depth_stencil_alpha.stencil[0].enabled) {
         /* do stencil test */
         ASSERT(spu.fb.depth_format == PIPE_FORMAT_Z24S8_UNORM);

      }
      else if (spu.depth_stencil_alpha.depth.enabled) {
         /* do depth test */

         ASSERT(spu.fb.depth_format == PIPE_FORMAT_Z24S8_UNORM ||
                spu.fb.depth_format == PIPE_FORMAT_Z24X8_UNORM);

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


   /*
    * If we'll need the current framebuffer/tile colors for blending
    * or logicop or colormask, fetch them now.
    */
   if (spu.blend.rt[0].blend_enable ||
       spu.blend.logicop_enable ||
       spu.blend.rt[0].colormask != 0xf) {

#if LINEAR_QUAD_LAYOUT /* See comments/diagram below */
      fbc0 = colorTile->ui[y][x*2+0];
      fbc1 = colorTile->ui[y][x*2+1];
      fbc2 = colorTile->ui[y][x*2+2];
      fbc3 = colorTile->ui[y][x*2+3];
#else
      fbc0 = colorTile->ui[y+0][x+0];
      fbc1 = colorTile->ui[y+0][x+1];
      fbc2 = colorTile->ui[y+1][x+0];
      fbc3 = colorTile->ui[y+1][x+1];
#endif
   }


   /*
    * Do blending
    */
   if (spu.blend.rt[0].blend_enable) {
      /* blending terms, misc regs */
      vector float term1r, term1g, term1b, term1a;
      vector float term2r, term2g, term2b, term2a;
      vector float one, tmp;

      vector float fbRGBA[4];  /* current framebuffer colors */

      /* convert framebuffer colors from packed int to vector float */
      {
         vector float temp[4]; /* float colors in AOS form */
         switch (spu.fb.color_format) {
         case PIPE_FORMAT_A8R8G8B8_UNORM:
            temp[0] = spu_unpack_B8G8R8A8(fbc0);
            temp[1] = spu_unpack_B8G8R8A8(fbc1);
            temp[2] = spu_unpack_B8G8R8A8(fbc2);
            temp[3] = spu_unpack_B8G8R8A8(fbc3);
            break;
         case PIPE_FORMAT_B8G8R8A8_UNORM:
            temp[0] = spu_unpack_A8R8G8B8(fbc0);
            temp[1] = spu_unpack_A8R8G8B8(fbc1);
            temp[2] = spu_unpack_A8R8G8B8(fbc2);
            temp[3] = spu_unpack_A8R8G8B8(fbc3);
            break;
         default:
            ASSERT(0);
         }
         _transpose_matrix4x4(fbRGBA, temp); /* fbRGBA = transpose(temp) */
      }

      /*
       * Compute Src RGB terms (fragment color * factor)
       */
      switch (spu.blend.rt[0].rgb_src_factor) {
      case PIPE_BLENDFACTOR_ONE:
         term1r = fragR;
         term1g = fragG;
         term1b = fragB;
         break;
      case PIPE_BLENDFACTOR_ZERO:
         term1r =
         term1g =
         term1b = spu_splats(0.0f);
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         term1r = spu_mul(fragR, fragR);
         term1g = spu_mul(fragG, fragG);
         term1b = spu_mul(fragB, fragB);
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         term1r = spu_mul(fragR, fragA);
         term1g = spu_mul(fragG, fragA);
         term1b = spu_mul(fragB, fragA);
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         term1r = spu_mul(fragR, fbRGBA[0]);
         term1g = spu_mul(fragG, fbRGBA[1]);
         term1b = spu_mul(fragB, fbRGBA[1]);
         break;
      case PIPE_BLENDFACTOR_DST_ALPHA:
         term1r = spu_mul(fragR, fbRGBA[3]);
         term1g = spu_mul(fragG, fbRGBA[3]);
         term1b = spu_mul(fragB, fbRGBA[3]);
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         term1r = spu_mul(fragR, spu_splats(spu.blend_color.color[0]));
         term1g = spu_mul(fragG, spu_splats(spu.blend_color.color[1]));
         term1b = spu_mul(fragB, spu_splats(spu.blend_color.color[2]));
         break;
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         term1r = spu_mul(fragR, spu_splats(spu.blend_color.color[3]));
         term1g = spu_mul(fragG, spu_splats(spu.blend_color.color[3]));
         term1b = spu_mul(fragB, spu_splats(spu.blend_color.color[3]));
         break;
      /* XXX more cases */
      default:
         ASSERT(0);
      }

      /*
       * Compute Src Alpha term (fragment alpha * factor)
       */
      switch (spu.blend.rt[0].alpha_src_factor) {
      case PIPE_BLENDFACTOR_ONE:
         term1a = fragA;
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         term1a = spu_splats(0.0f);
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         term1a = spu_mul(fragA, fragA);
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_DST_ALPHA:
         term1a = spu_mul(fragA, fbRGBA[3]);
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         term1a = spu_mul(fragR, spu_splats(spu.blend_color.color[3]));
         break;
      /* XXX more cases */
      default:
         ASSERT(0);
      }

      /*
       * Compute Dest RGB terms (framebuffer color * factor)
       */
      switch (spu.blend.rt[0].rgb_dst_factor) {
      case PIPE_BLENDFACTOR_ONE:
         term2r = fbRGBA[0];
         term2g = fbRGBA[1];
         term2b = fbRGBA[2];
         break;
      case PIPE_BLENDFACTOR_ZERO:
         term2r =
         term2g =
         term2b = spu_splats(0.0f);
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         term2r = spu_mul(fbRGBA[0], fragR);
         term2g = spu_mul(fbRGBA[1], fragG);
         term2b = spu_mul(fbRGBA[2], fragB);
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         term2r = spu_mul(fbRGBA[0], fragA);
         term2g = spu_mul(fbRGBA[1], fragA);
         term2b = spu_mul(fbRGBA[2], fragA);
         break;
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         one = spu_splats(1.0f);
         tmp = spu_sub(one, fragA);
         term2r = spu_mul(fbRGBA[0], tmp);
         term2g = spu_mul(fbRGBA[1], tmp);
         term2b = spu_mul(fbRGBA[2], tmp);
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         term2r = spu_mul(fbRGBA[0], fbRGBA[0]);
         term2g = spu_mul(fbRGBA[1], fbRGBA[1]);
         term2b = spu_mul(fbRGBA[2], fbRGBA[2]);
         break;
      case PIPE_BLENDFACTOR_DST_ALPHA:
         term2r = spu_mul(fbRGBA[0], fbRGBA[3]);
         term2g = spu_mul(fbRGBA[1], fbRGBA[3]);
         term2b = spu_mul(fbRGBA[2], fbRGBA[3]);
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         term2r = spu_mul(fbRGBA[0], spu_splats(spu.blend_color.color[0]));
         term2g = spu_mul(fbRGBA[1], spu_splats(spu.blend_color.color[1]));
         term2b = spu_mul(fbRGBA[2], spu_splats(spu.blend_color.color[2]));
         break;
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         term2r = spu_mul(fbRGBA[0], spu_splats(spu.blend_color.color[3]));
         term2g = spu_mul(fbRGBA[1], spu_splats(spu.blend_color.color[3]));
         term2b = spu_mul(fbRGBA[2], spu_splats(spu.blend_color.color[3]));
         break;
       /* XXX more cases */
      default:
         ASSERT(0);
      }

      /*
       * Compute Dest Alpha term (framebuffer alpha * factor)
       */
      switch (spu.blend.rt[0].alpha_dst_factor) {
      case PIPE_BLENDFACTOR_ONE:
         term2a = fbRGBA[3];
         break;
      case PIPE_BLENDFACTOR_SRC_COLOR:
         term2a = spu_splats(0.0f);
         break;
      case PIPE_BLENDFACTOR_SRC_ALPHA:
         term2a = spu_mul(fbRGBA[3], fragA);
         break;
      case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
         one = spu_splats(1.0f);
         tmp = spu_sub(one, fragA);
         term2a = spu_mul(fbRGBA[3], tmp);
         break;
      case PIPE_BLENDFACTOR_DST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_DST_ALPHA:
         term2a = spu_mul(fbRGBA[3], fbRGBA[3]);
         break;
      case PIPE_BLENDFACTOR_CONST_COLOR:
         /* fall-through */
      case PIPE_BLENDFACTOR_CONST_ALPHA:
         term2a = spu_mul(fbRGBA[3], spu_splats(spu.blend_color.color[3]));
         break;
      /* XXX more cases */
      default:
         ASSERT(0);
      }

      /*
       * Combine Src/Dest RGB terms
       */
      switch (spu.blend.rt[0].rgb_func) {
      case PIPE_BLEND_ADD:
         fragR = spu_add(term1r, term2r);
         fragG = spu_add(term1g, term2g);
         fragB = spu_add(term1b, term2b);
         break;
      case PIPE_BLEND_SUBTRACT:
         fragR = spu_sub(term1r, term2r);
         fragG = spu_sub(term1g, term2g);
         fragB = spu_sub(term1b, term2b);
         break;
      case PIPE_BLEND_REVERSE_SUBTRACT:
         fragR = spu_sub(term2r, term1r);
         fragG = spu_sub(term2g, term1g);
         fragB = spu_sub(term2b, term1b);
         break;
      case PIPE_BLEND_MIN:
         fragR = spu_min(term1r, term2r);
         fragG = spu_min(term1g, term2g);
         fragB = spu_min(term1b, term2b);
         break;
      case PIPE_BLEND_MAX:
         fragR = spu_max(term1r, term2r);
         fragG = spu_max(term1g, term2g);
         fragB = spu_max(term1b, term2b);
         break;
      default:
         ASSERT(0);
      }

      /*
       * Combine Src/Dest A term
       */
      switch (spu.blend.rt[0].alpha_func) {
      case PIPE_BLEND_ADD:
         fragA = spu_add(term1a, term2a);
         break;
      case PIPE_BLEND_SUBTRACT:
         fragA = spu_sub(term1a, term2a);
         break;
      case PIPE_BLEND_REVERSE_SUBTRACT:
         fragA = spu_sub(term2a, term1a);
         break;
      case PIPE_BLEND_MIN:
         fragA = spu_min(term1a, term2a);
         break;
      case PIPE_BLEND_MAX:
         fragA = spu_max(term1a, term2a);
         break;
      default:
         ASSERT(0);
      }
   }


   /*
    * Convert RRRR,GGGG,BBBB,AAAA to RGBA,RGBA,RGBA,RGBA.
    */
#if 0
   /* original code */
   {
      vector float frag_soa[4];
      frag_soa[0] = fragR;
      frag_soa[1] = fragG;
      frag_soa[2] = fragB;
      frag_soa[3] = fragA;
      _transpose_matrix4x4(frag_aos, frag_soa);
   }
#else
   /* short-cut relying on function parameter layout: */
   _transpose_matrix4x4(frag_aos, &fragR);
   (void) fragG;
   (void) fragB;
#endif

   /*
    * Pack fragment float colors into 32-bit RGBA words.
    */
   switch (spu.fb.color_format) {
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      fragc0 = spu_pack_A8R8G8B8(frag_aos[0]);
      fragc1 = spu_pack_A8R8G8B8(frag_aos[1]);
      fragc2 = spu_pack_A8R8G8B8(frag_aos[2]);
      fragc3 = spu_pack_A8R8G8B8(frag_aos[3]);
      break;
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      fragc0 = spu_pack_B8G8R8A8(frag_aos[0]);
      fragc1 = spu_pack_B8G8R8A8(frag_aos[1]);
      fragc2 = spu_pack_B8G8R8A8(frag_aos[2]);
      fragc3 = spu_pack_B8G8R8A8(frag_aos[3]);
      break;
   default:
      fprintf(stderr, "SPU: Bad pixel format in spu_default_fragment_ops\n");
      ASSERT(0);
   }


   /*
    * Do color masking
    */
   if (spu.blend.rt[0].colormask != 0xf) {
      uint cmask = 0x0; /* each byte corresponds to a color channel */

      /* Form bitmask depending on color buffer format and colormask bits */
      switch (spu.fb.color_format) {
      case PIPE_FORMAT_B8G8R8A8_UNORM:
         if (spu.blend.rt[0].colormask & PIPE_MASK_R)
            cmask |= 0x00ff0000; /* red */
         if (spu.blend.rt[0].colormask & PIPE_MASK_G)
            cmask |= 0x0000ff00; /* green */
         if (spu.blend.rt[0].colormask & PIPE_MASK_B)
            cmask |= 0x000000ff; /* blue */
         if (spu.blend.rt[0].colormask & PIPE_MASK_A)
            cmask |= 0xff000000; /* alpha */
         break;
      case PIPE_FORMAT_A8R8G8B8_UNORM:
         if (spu.blend.rt[0].colormask & PIPE_MASK_R)
            cmask |= 0x0000ff00; /* red */
         if (spu.blend.rt[0].colormask & PIPE_MASK_G)
            cmask |= 0x00ff0000; /* green */
         if (spu.blend.rt[0].colormask & PIPE_MASK_B)
            cmask |= 0xff000000; /* blue */
         if (spu.blend.rt[0].colormask & PIPE_MASK_A)
            cmask |= 0x000000ff; /* alpha */
         break;
      default:
         ASSERT(0);
      }

      /*
       * Apply color mask to the 32-bit packed colors.
       * if (cmask[i])
       *    frag color[i] = frag color[i];
       * else
       *    frag color[i] = framebuffer color[i];
       */
      fragc0 = (fragc0 & cmask) | (fbc0 & ~cmask);
      fragc1 = (fragc1 & cmask) | (fbc1 & ~cmask);
      fragc2 = (fragc2 & cmask) | (fbc2 & ~cmask);
      fragc3 = (fragc3 & cmask) | (fbc3 & ~cmask);
   }


   /*
    * Do logic ops
    */
   if (spu.blend.logicop_enable) {
      /* XXX to do */
      /* apply logicop to 32-bit packed colors (fragcx and fbcx) */
   }


   /*
    * If mask is non-zero, mark tile as dirty.
    */
   if (spu_extract(spu_orx(mask), 0)) {
      spu.cur_ctile_status = TILE_STATUS_DIRTY;
   }
   else {
      /* write no fragments */
      return;
   }


   /*
    * Write new fragment/quad colors to the framebuffer/tile.
    * Only write pixels where the corresponding mask word is set.
    */
#if LINEAR_QUAD_LAYOUT
   /*
    * Quad layout:
    *  +--+--+--+--+
    *  |p0|p1|p2|p3|...
    *  +--+--+--+--+
    */
   if (spu_extract(mask, 0))
      colorTile->ui[y][x*2] = fragc0;
   if (spu_extract(mask, 1))
      colorTile->ui[y][x*2+1] = fragc1;
   if (spu_extract(mask, 2))
      colorTile->ui[y][x*2+2] = fragc2;
   if (spu_extract(mask, 3))
      colorTile->ui[y][x*2+3] = fragc3;
#else
   /*
    * Quad layout:
    *  +--+--+
    *  |p0|p1|...
    *  +--+--+
    *  |p2|p3|...
    *  +--+--+
    */
   if (spu_extract(mask, 0))
      colorTile->ui[y+0][x+0] = fragc0;
   if (spu_extract(mask, 1))
      colorTile->ui[y+0][x+1] = fragc1;
   if (spu_extract(mask, 2))
      colorTile->ui[y+1][x+0] = fragc2;
   if (spu_extract(mask, 3))
      colorTile->ui[y+1][x+1] = fragc3;
#endif
}
