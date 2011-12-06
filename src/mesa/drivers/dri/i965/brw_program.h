/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef BRW_PROGRAM_H
#define BRW_PROGRAM_H

/**
 * Sampler information needed by VS, WM, and GS program cache keys.
 */
struct brw_sampler_prog_key_data {
   /**
    * Per-sampler comparison functions:
    *
    * If comparison mode is GL_COMPARE_R_TO_TEXTURE, then this is set to one
    * of GL_NEVER, GL_LESS, GL_EQUAL, GL_LEQUAL, GL_GREATER, GL_NOTEQUAL,
    * GL_GEQUAL, or GL_ALWAYS.  Otherwise (comparison mode is GL_NONE), this
    * field is irrelevant so it's left as GL_NONE (0).
    *
    * While this is a GLenum, all possible values fit in 16-bits.
    */
   uint16_t compare_funcs[BRW_MAX_TEX_UNIT];

   /**
    * EXT_texture_swizzle and DEPTH_TEXTURE_MODE swizzles.
    */
   uint16_t swizzles[BRW_MAX_TEX_UNIT];

   uint16_t gl_clamp_mask[3];

   /**
    * YUV conversions, needed for the GL_MESA_ycbcr extension.
    */
   uint16_t yuvtex_mask;
   uint16_t yuvtex_swap_mask; /**< UV swaped */
};

void brw_populate_sampler_prog_key_data(struct gl_context *ctx,
				        struct brw_sampler_prog_key_data *key, int i);

#endif
