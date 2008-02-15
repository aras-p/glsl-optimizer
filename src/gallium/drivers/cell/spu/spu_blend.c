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


#include "spu_main.h"
#include "spu_blend.h"
#include "spu_colorpack.h"


void
blend_quad(uint itx, uint ity, vector float colors[4])
{
   /* simple SRC_ALPHA, ONE_MINUS_SRC_ALPHA blending */
   vector float fbc00 = spu_unpack_color(spu.ctile.ui[ity][itx]);
   vector float fbc01 = spu_unpack_color(spu.ctile.ui[ity][itx+1]);
   vector float fbc10 = spu_unpack_color(spu.ctile.ui[ity+1][itx]);
   vector float fbc11 = spu_unpack_color(spu.ctile.ui[ity+1][itx+1]);

   vector float alpha00 = spu_splats(spu_extract(colors[0], 3));
   vector float alpha01 = spu_splats(spu_extract(colors[1], 3));
   vector float alpha10 = spu_splats(spu_extract(colors[2], 3));
   vector float alpha11 = spu_splats(spu_extract(colors[3], 3));

   vector float one_minus_alpha00 = spu_sub(spu_splats(1.0f), alpha00);
   vector float one_minus_alpha01 = spu_sub(spu_splats(1.0f), alpha01);
   vector float one_minus_alpha10 = spu_sub(spu_splats(1.0f), alpha10);
   vector float one_minus_alpha11 = spu_sub(spu_splats(1.0f), alpha11);

   colors[0] = spu_add(spu_mul(colors[0], alpha00),
                       spu_mul(fbc00, one_minus_alpha00));
   colors[1] = spu_add(spu_mul(colors[1], alpha01),
                       spu_mul(fbc01, one_minus_alpha01));
   colors[2] = spu_add(spu_mul(colors[2], alpha10),
                       spu_mul(fbc10, one_minus_alpha10));
   colors[3] = spu_add(spu_mul(colors[3], alpha11),
                       spu_mul(fbc11, one_minus_alpha11));
}

