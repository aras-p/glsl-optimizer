/**************************************************************************
 *
 * Copyright 2009 Ben Skeggs
 * Copyright 2009 Younes Manton
 * Copyright 2010 Luca Barbieri
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NON-INFRINGEMENT. IN NO EVENT SHALL THE COPYRIGHT HOLDERS, AUTHORS
 * AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/

/* this code has no Mesa or Gallium dependency and can be reused in the classic Mesa driver or DDX */

#ifndef __NV04_2D_H__
#define __NV04_2D_H__

struct nv04_2d_context;
struct nouveau_channel;
struct nouveau_bo;

// NOTE: all functions taking this as a parameter will CLOBBER it (except for ->bo)
struct nv04_region {
	struct nouveau_bo* bo;
	int offset;
	unsigned pitch; // 0 -> swizzled
	unsigned bpps; // bpp shift (0, 1, 2; 3, 4 for fp/compressed)
	unsigned one_bits; // number of high bits read and written as ones (for "no-alpha" optimization)
	unsigned x, y, z;
	unsigned w, h, d;
};

static inline void
nv04_region_try_to_linearize(struct nv04_region* rgn)
{
	assert(!rgn->pitch);

	if(rgn->d <= 1)
	{
		if(rgn->h <= 1 || rgn->w <= 2)
			rgn->pitch = rgn->w << rgn->bpps;
	}
	else
	{
		if(rgn->h <= 2 && rgn->w <= 2)
		{
			rgn->pitch = rgn->w << rgn->bpps;
			rgn->offset += rgn->z * rgn->h * rgn->pitch;
		}
	}
}

void
nv04_memcpy(struct nv04_2d_context *ctx,
		struct nouveau_bo* dstbo, int dstoff,
		struct nouveau_bo* srcbo, int srcoff,
		unsigned size);

unsigned
nv04_region_begin(struct nv04_region* rgn, unsigned w, unsigned h);

unsigned
nv04_region_end(struct nv04_region* rgn, unsigned w, unsigned h);

void
nv04_2d_context_takedown(struct nv04_2d_context *pctx);

struct nv04_2d_context *
nv04_2d_context_init(struct nouveau_channel* chan);

void
nv04_region_copy_cpu(struct nv04_region* dst, struct nv04_region* src, int w, int h);

void
nv04_region_fill_cpu(struct nv04_region* dst, int w, int h, unsigned value);

int
nv04_region_copy_2d(struct nv04_2d_context *ctx,
		struct nv04_region* dst, struct nv04_region* src,
		int w, int h,
		int dst_to_gpu, int src_on_gpu);

int
nv04_region_fill_2d(struct nv04_2d_context *ctx,
		struct nv04_region *dst,
                int w, int h,
                unsigned value);

#endif
