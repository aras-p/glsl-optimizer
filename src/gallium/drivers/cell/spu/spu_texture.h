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

#ifndef SPU_TEXTURE_H
#define SPU_TEXTURE_H


#include "pipe/p_compiler.h"


extern void
invalidate_tex_cache(void);


extern void
sample_texture_2d_nearest(vector float s, vector float t,
                          uint unit, uint level, uint face,
                          vector float colors[4]);


extern void
sample_texture_2d_bilinear(vector float s, vector float t,
                           uint unit, uint level, uint face,
                           vector float colors[4]);

extern void
sample_texture_2d_bilinear_int(vector float s, vector float t,
                               uint unit, uint level, uint face,
                               vector float colors[4]);


extern void
sample_texture_2d_lod(vector float s, vector float t,
                      uint unit, uint level, uint face,
                      vector float colors[4]);


extern void
sample_texture_cube(vector float s, vector float t, vector float r,
                    uint unit, vector float colors[4]);


#endif /* SPU_TEXTURE_H */
