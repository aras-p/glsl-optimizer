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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <nouveau/nouveau_device.h>
#include <nouveau/nouveau_channel.h>
#include <nouveau/nouveau_bo.h>
#include <nouveau/nouveau_notifier.h>
#include <nouveau/nouveau_grobj.h>
#include <nouveau/nv04_pushbuf.h>
#include "nv04_2d.h"

#include "nouveau/nv_object.xml.h"
#include "nouveau/nv_m2mf.xml.h"
#include "nv01_2d.xml.h"

/* avoid depending on Mesa/Gallium */
#ifdef __GNUC__
#define likely(x) __builtin_expect(!!(x), 1)
#define unlikely(x) __builtin_expect(!!(x), 0)
#else
#define likely(x) !!(x)
#define unlikely(x) !!(x)
#endif

#define MIN2( A, B )   ( (A)<(B) ? (A) : (B) )
#define MAX2( A, B )   ( (A)>(B) ? (A) : (B) )

struct nv04_2d_context
{
	struct nouveau_notifier *ntfy;
	struct nouveau_grobj *surf2d;
	struct nouveau_grobj *swzsurf;
	struct nouveau_grobj *m2mf;
	struct nouveau_grobj *rect;
	struct nouveau_grobj *sifm;
	struct nouveau_grobj *blit;
};

static inline int
align(int value, int alignment)
{
   return (value + alignment - 1) & ~(alignment - 1);
}

static inline int
util_is_pot(unsigned x)
{
   return (x & (x - 1)) == 0;
}

/* Integer base-2 logarithm, rounded towards zero. */
static inline unsigned log2i(unsigned i)
{
	unsigned r = 0;

	if (i & 0xffff0000) {
		i >>= 16;
		r += 16;
	}
	if (i & 0x0000ff00) {
		i >>= 8;
		r += 8;
	}
	if (i & 0x000000f0) {
		i >>= 4;
		r += 4;
	}
	if (i & 0x0000000c) {
		i >>= 2;
		r += 2;
	}
	if (i & 0x00000002) {
		r += 1;
	}
	return r;
}

//#define NV04_REGION_DEBUG

// Yes, we really want to inline everything, since all the functions are used only once
#if defined(__GNUC__) && !defined(DEBUG)
#define inline __attribute__((always_inline)) inline
#endif

static inline unsigned
nv04_swizzle_bits_square(unsigned x, unsigned y)
{
	unsigned u = (x & 0x001) << 0 |
		     (x & 0x002) << 1 |
		     (x & 0x004) << 2 |
		     (x & 0x008) << 3 |
		     (x & 0x010) << 4 |
		     (x & 0x020) << 5 |
		     (x & 0x040) << 6 |
		     (x & 0x080) << 7 |
		     (x & 0x100) << 8 |
		     (x & 0x200) << 9 |
		     (x & 0x400) << 10 |
		     (x & 0x800) << 11;

	unsigned v = (y & 0x001) << 1 |
		     (y & 0x002) << 2 |
		     (y & 0x004) << 3 |
		     (y & 0x008) << 4 |
		     (y & 0x010) << 5 |
		     (y & 0x020) << 6 |
		     (y & 0x040) << 7 |
		     (y & 0x080) << 8 |
		     (y & 0x100) << 9 |
		     (y & 0x200) << 10 |
		     (y & 0x400) << 11 |
		     (y & 0x800) << 12;
	return v | u;
}

/* rectangular swizzled textures are linear concatenations of swizzled square tiles */
static inline unsigned
nv04_swizzle_bits_2d(unsigned x, unsigned y, unsigned w, unsigned h)
{
	if(h <= 1)
		return x;
	else
	{
		unsigned s = MIN2(w, h);
		unsigned m = s - 1;
		return (((x | y) & ~m) * s) | nv04_swizzle_bits_square(x & m, y & m);
	}
}

// general 3D texture case
static inline unsigned
nv04_swizzle_bits(unsigned x, unsigned y, unsigned z, unsigned w, unsigned h, unsigned d)
{
	if(d <= 1)
		return nv04_swizzle_bits_2d(x, y, w, h);
	else
	{
		// TODO: autogenerate code for all possible texture sizes (13 * 13 * 13 with dims <= 4096) and do a single indirect call
		unsigned v = 0;
		w >>= 1;
		h >>= 1;
		d >>= 1;
		for(int i = 0;;)
		{
			int oldi = i;
			if(likely(w))
			{
				v |= (x & 1) << i;
				x >>= 1;
				w >>= 1;
				++i;
			}

			if(likely(h))
			{
				v |= (y & 1) << i;
				y >>= 1;
				h >>= 1;
				++i;
			}

			if(likely(d))
			{
				v |= (z & 1) << i;
				z >>= 1;
				d >>= 1;
				++i;
			}

			if(i == oldi)
				break;
		}
		return v;
	}
}

unsigned
nv04_region_begin(struct nv04_region* rgn, unsigned w, unsigned h)
{
	if(rgn->pitch)
		return rgn->pitch * rgn->y + (rgn->x << rgn->bpps);
	else
		return nv04_swizzle_bits(rgn->x, rgn->y, rgn->z, rgn->w, rgn->h, rgn->d) << rgn->bpps;
}

unsigned
nv04_region_end(struct nv04_region* rgn, unsigned w, unsigned h)
{
	if(rgn->pitch)
		return rgn->pitch * (rgn->y + h - 1) + ((rgn->x + w) << rgn->bpps);
	else
		return (nv04_swizzle_bits(rgn->x + w - 1, rgn->y + h - 1, rgn->z, rgn->w, rgn->h, rgn->d) + 1) << rgn->bpps;
}

// *pitch = -1 -> use 3D swizzling for (x, y), *pitch = 0 -> use 2D swizzling, other *pitch -> use linear calculations
// returns 2 if pixel order is 3D-swizzled and 1 if subrect is 2D-swizzled
/* *pitch == -1 ret = 0 -> 3D swizzled subrect
 * *pitch == 0 ret = 0 -> 2D swizzled subrect
 * *pitch > 0 ret = 0 -> linear subrect
 * *pitch > 0 ret = 1 -> linear subrect, but with swizzled 3D data inside
 */

static inline void
nv04_region_print(struct nv04_region* rgn)
{
	fprintf(stderr, "<%i[%i]> ", rgn->bo->handle, rgn->offset);
	if(rgn->pitch)
		fprintf(stderr, "lin %i", rgn->pitch);
	else
		fprintf(stderr, "swz %ix%ix%i", rgn->w, rgn->h, rgn->d);
	fprintf(stderr, " (%i, %i, %i)", rgn->x, rgn->y, rgn->z);
}

static inline void
nv04_region_assert(struct nv04_region* rgn, unsigned w, unsigned h)
{
	unsigned end = rgn->offset + nv04_region_end(rgn, w, h);

	assert(rgn->offset <= (int)rgn->bo->size);
	assert(end <= rgn->bo->size);
	(void) end;
	if(!rgn->pitch) {
		assert(util_is_pot(rgn->w));
		assert(util_is_pot(rgn->h));
	}
}

/* determine if region can be linearized or fake-linearized */
static inline int
nv04_region_is_contiguous(struct nv04_region* rgn, int w, int h)
{
	int surf_min;
	int rect_min;

	if(rgn->pitch)
		return rgn->pitch == w << rgn->bpps;

	// redundant, but this is the fast path for the common case
	if(w == rgn->w && h == rgn->h && rgn->d <= 1)
		return 1;

	// must be POT
	if((w & (w - 1)) || (h & (h - 1)))
		return 0;

	// must be aligned
	if((rgn->x & (w - 1)) || (rgn->y & (h - 1)))
		return 0;

	if(rgn->d > 1)
		return 0;

	surf_min = MIN2(rgn->w, rgn->h);
	rect_min = MIN2(w, h);

	if((rect_min == surf_min) || (w == h) || (w == 2 * h))
		return 1;

	return 0;
}

// double the pitch until it is larger than the alignment, or the height becomes odd or 1
static inline void
nv04_region_contiguous_shape(struct nv04_region* rgn, int* w, int* h, int align)
{
	while(!(*h & 1) && (*w << rgn->bpps) < (1 << align))
	{
		*w <<= 1;
		*h >>= 1;
	}

	while((*w << rgn->bpps) > 16384 && !(*w & 1))
	{
		*w >>= 1;
		*h <<= 1;
	}

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tCONTIGUOUS %ix%i\n", *w, *h);
#endif
}

static inline void
nv04_region_linearize_contiguous(struct nv04_region* rgn, unsigned w, unsigned h)
{
	int pos;
	if(rgn->pitch)
	{
		rgn->offset += rgn->y * rgn->pitch + (rgn->x << rgn->bpps);
		rgn->x = 0;
		rgn->y = 0;
	}
	else
	{
		rgn->offset += (rgn->w * rgn->h * rgn->z) << rgn->bpps;
		pos = nv04_swizzle_bits(rgn->x, rgn->y, rgn->z, rgn->w, rgn->h, rgn->d);
		rgn->x = pos & (w - 1);
		rgn->y = pos / w;
	}
	rgn->pitch = w << rgn->bpps;

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tLINEARIZE ");
	nv04_region_print(rgn);
	fprintf(stderr, "\n");
#endif
}

	/* preserve the offset! */
	/*
	rgn->pitch = util_format_get_stride(rgn->format, w);
	int pos = nv04_swizzle_bits(rgn->x, rgn->y, rgn->z, rgn->w, rgn->h, rgn->d);
	rgn->x = pos & (w - 1);
	rgn->y = pos & ~(w - 1);
	*/

	/*
	rgn->offset +=
	rgn->pitch = util_format_get_stride(rgn->format, w);
	rgn->x = 0;
	rgn->y = 0;
	*/

/* This code will get used for, and always succeed on:
 * - 4x2 1bpp swizzled texture mipmap levels
 * - linear regions created by linearization
 *
 * This code will get used for, and MAY work for:
 * - misaligned texture blanket
 * - linear surfaces created without wide_pitch (in this case, it will only work if we are lucky)
 *
 * The general case requires splitting the region in 2.
 */
static inline int
nv04_region_do_align_offset(struct nv04_region* rgn, unsigned w, unsigned h, int shift)
{
	if(rgn->pitch > 0)
	{
		assert(!(rgn->offset & ((1 << rgn->bpps) - 1))); // fatal!

		if(h <= 1)
		{
			int delta;
			rgn->offset += rgn->y * rgn->pitch + (rgn->x << rgn->bpps);
			delta = rgn->offset & ((1 << shift) - 1);
			rgn->y = 0;
			rgn->x = delta >> rgn->bpps;
			rgn->offset -= delta;
			rgn->pitch = align((rgn->x + w) << rgn->bpps, 1 << shift);
		}
		else
		{
			int delta = rgn->offset & ((1 << shift) - 1);
			int newxo = (rgn->x << rgn->bpps) + delta;
			int dy = newxo / rgn->pitch;
			newxo -= dy * rgn->pitch;
			if((newxo + (w << rgn->bpps)) > rgn->pitch)
			{
				// TODO: split the region into two rectangles (!) if *really* necessary, unless the hardware actually supports "wrapping" rectangles
				// this does not happen if the surface is pitch-aligned, which it should always be
				assert(0);
				return -1;
			}
			rgn->x = newxo >> rgn->bpps;
			rgn->y += dy;
		}
	}
	else
	{
		int size;
		int min;
		int v;

		// we don't care about the alignment of 3D surfaces since the 2D engine can't use them
		if(rgn->d < 0)
			return -1;

		min = MIN2(rgn->w, rgn->h);
		size = min * min << rgn->bpps;

		// this is unfixable, and should not be happening
		if(rgn->offset & (size - 1))
			return -1;

		v = (rgn->offset & ((1 << shift) - 1)) / size;
		rgn->offset -= v * size;

		if(rgn->h == min)
		{
			unsigned w;
			rgn->x += rgn->h * v;
			w = rgn->w + rgn->h * v;

			while(rgn->w < w)
				rgn->w += rgn->w;
		}
		else
		{
			unsigned h;
			rgn->y += rgn->w * v;
			h = rgn->h + rgn->w * v;

			while(rgn->h < h)
				rgn->h += rgn->h;
		}
	}

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tALIGNED ");
	nv04_region_print(rgn);
	fprintf(stderr, "\n");
#endif
	return 0;
}

// both pitch and shift
// will leave the region unchanged if it fails
static inline int
nv04_region_align(struct nv04_region* rgn, unsigned w, unsigned h, int shift)
{
	if(rgn->pitch & ((1 << shift) - 1))
	{
		if(h == 1)
			goto do_align; /* this will fix pitch too in this case */
		else
			return -1;
	}

	if(rgn->offset & ((1 << shift) - 1))
	{
		do_align:
		if(nv04_region_do_align_offset(rgn, w, h, shift))
			return -1;
	}
	return 0;
}

/* this contains 22 different copy loops after preprocessing. unfortunately, it's necessary */
void
nv04_region_copy_cpu(struct nv04_region* dst, struct nv04_region* src, int w, int h)
{
	uint8_t* mdst;
	uint8_t* msrc;
	int size;

	if(dst->bo != src->bo)
	{
		nouveau_bo_map(dst->bo, NOUVEAU_BO_WR);
		nouveau_bo_map(src->bo, NOUVEAU_BO_RD);
	}
	else
		nouveau_bo_map(dst->bo, NOUVEAU_BO_WR | NOUVEAU_BO_RD);

	mdst = (uint8_t*)dst->bo->map + dst->offset;
	msrc = (uint8_t*)src->bo->map + src->offset;

	size = w << dst->bpps;

	nv04_region_assert(dst, w, h);
	nv04_region_assert(src, w, h);

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tRGN_COPY_CPU [%i, %i: %i] ", w, h, dst->bpps);
	for(int i = 0; i < 2; ++i)
	{
		nv04_region_print(i ? src : dst);
		fprintf(stderr, i ? "\n" : " <- ");
	}

//	for(int i = 0; i < 16; ++i)
//		fprintf(stderr, "%02x ", msrc[i]);
//	fprintf(stderr, "\n");
#endif

	// TODO: support overlapping copies!
	if(src->pitch && dst->pitch)
	{
		mdst += dst->y * dst->pitch + (dst->x << dst->bpps);
		msrc += src->y * src->pitch + (src->x << src->bpps);
		if(dst->bo != src->bo)
			goto simple;
		else if(mdst < msrc)
		{
			if(mdst + size <= msrc)
			{
simple:
				for(int iy = 0; iy < h; ++iy)
				{
					assert(mdst + size <= (uint8_t*)dst->bo->map + dst->bo->size);
					assert(msrc + size <= (uint8_t*)src->bo->map + src->bo->size);
					memcpy(mdst, msrc, size);
					msrc += src->pitch; mdst += dst->pitch;
				}
			}
			else
			{
				for(int iy = 0; iy < h; ++iy)
				{
					assert(mdst + size <= (uint8_t*)dst->bo->map + dst->bo->size);
					assert(msrc + size <= (uint8_t*)src->bo->map + src->bo->size);
					memmove(mdst, msrc, size);
					msrc += src->pitch; mdst += dst->pitch;
				}
			}
		}
		else
		{
			/* copy backwards so we don't destroy data we have to read yet */
			if(msrc + size <= mdst)
			{
				for(int iy = h - 1; iy >= 0; --iy)
				{
					assert(mdst + size <= (uint8_t*)dst->bo->map + dst->bo->size);
					assert(msrc + size <= (uint8_t*)src->bo->map + src->bo->size);
					memcpy(mdst, msrc, size);
					msrc += src->pitch; mdst += dst->pitch;
				}
			}
			else
			{
				for(int iy = h - 1; iy >= 0; --iy)
				{
					assert(mdst + size <= (uint8_t*)dst->bo->map + dst->bo->size);
					assert(msrc + size <= (uint8_t*)src->bo->map + src->bo->size);
					memmove(mdst, msrc, size);
					msrc += src->pitch; mdst += dst->pitch;
				}
			}
		}
	}
	else
	{
		int* dswx = NULL;
		int* dswy = NULL;
		int* sswx = NULL;
		int* sswy = NULL;
		int dir;

		if(!dst->pitch)
		{
			dswx = alloca(w * sizeof(int));
			for(int ix = 0; ix < w; ++ix) // we are adding, so z cannot be contributed by both
				dswx[ix] = nv04_swizzle_bits(dst->x + ix, 0, 0, dst->w, dst->h, dst->d);
			dswy = alloca(h * sizeof(int));
			for(int iy = 0; iy < h; ++iy)
				dswy[iy] = nv04_swizzle_bits(0, dst->y + iy, dst->z, dst->w, dst->h, dst->d);
		}

		if(!src->pitch)
		{
			sswx = alloca(w * sizeof(int));
			for(int ix = 0; ix < w; ++ix)
				sswx[ix] = nv04_swizzle_bits(src->x + ix, 0, 0, src->w, src->h, src->d);
			sswy = alloca(h * sizeof(int));
			for(int iy = 0; iy < h; ++iy)
				sswy[iy] = nv04_swizzle_bits(0, src->y + iy, src->z, src->w, src->h, src->d);
		}

		dir = 1;
		/* do backwards copies for overlapping swizzled surfaces */
		if(dst->pitch == src->pitch && dst->offset == src->offset)
		{
			if(dst->y > src->y || (dst->y == src->y && dst->x > src->x))
				dir = -1;
		}

#define SWIZZLED_COPY_LOOPS
		if(dir == 1)
		{
			int dir = 1;
#define LOOP_Y for(int iy = 0; iy < h; ++iy)
#define LOOP_X for(int ix = 0; ix < w; ++ix)
#include "nv04_2d_loops.h"
#undef LOOP_X
#undef LOOP_Y
		}
		else
		{
			int dir = -1;
#define LOOP_Y for(int iy = h - 1; iy >= 0; --iy)
#define LOOP_X for(int ix = w - 1; ix >= 0; --ix)
#include "nv04_2d_loops.h"
#undef LOOP_X
#undef LOOP_Y
		}
#undef SWIZZLED_COPY_LOOP
	}

	if(src->bo != dst->bo)
		nouveau_bo_unmap(src->bo);
	nouveau_bo_unmap(dst->bo);
}

/* TODO: if the destination is swizzled, we are doing random writes, which causes write combining to fail
 * the alternative is to read, modify and copy back, which may or may not be faster
 * loading 3D textures is a common case that hits this and could probably benefit from the temporary
 */
void
nv04_region_fill_cpu(struct nv04_region* dst, int w, int h, unsigned value)
{
	uint8_t* mdst = (nouveau_bo_map(dst->bo, NOUVEAU_BO_WR), (uint8_t*)dst->bo->map + dst->offset);

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tRGN_FILL_CPU ");
	nv04_region_print(dst);
	fprintf(stderr, "\n");
#endif

	nv04_region_assert(dst, w, h);

	if(dst->pitch)
	{
		unsigned size = w << dst->bpps;

#define FILL(T) do { \
			for(int iy = 0; iy < h; ++iy) \
			{ \
				assert((char*)((T*)mdst + w) <= (char*)dst->bo->map + dst->bo->size); \
				for(int ix = 0; ix < w; ++ix) \
					((T*)mdst)[ix] = (T)value; \
				mdst += dst->pitch; \
			} \
		} while(0)

		mdst += dst->y * dst->pitch + (dst->x << dst->bpps);

		if(dst->bpps == 0)
		{
ms:
			assert(mdst + size * h <= (uint8_t*)dst->bo->map + dst->bo->size);
			if(size == dst->pitch)
				memset(mdst, (uint8_t)value, size * h);
			else
			{
				for(int iy = 0; iy < h; ++iy)
				{
					assert(mdst + size <= (uint8_t*)dst->bo->map + dst->bo->size);
					memset(mdst, (uint8_t)value, size);
					mdst += dst->pitch;
				}
			}
		}
		else if(dst->bpps == 1)
		{
			if(!((uint8_t)value ^ (uint8_t)(value >> 8)))
				goto ms;

			FILL(uint16_t);
		}
		else if(dst->bpps == 2)
		{
			if(value == (uint8_t)value * 0x1010101)
				goto ms;
			FILL(uint32_t);
		}
		else
			assert(0);
#undef FILL
	}
	else
	{
		int* dswx;
		int* dswy;

		dswx = alloca(w * sizeof(int));
		for(int ix = 0; ix < w; ++ix)
			dswx[ix] = nv04_swizzle_bits(dst->x + ix, 0, dst->z, dst->w, dst->h, dst->d);
		dswy = alloca(h * sizeof(int));
		for(int iy = 0; iy < h; ++iy)
			dswy[iy] = nv04_swizzle_bits(0, dst->y + iy, dst->z, dst->w, dst->h, dst->d);

#define FILL(T) do { \
			T tvalue = (T)value; \
			for(int iy = 0; iy < h; ++iy) \
			{ \
				T* pdst = (T*)mdst + dswy[iy]; \
				for(int ix = 0; ix < w; ++ix) \
				{ \
					assert((uint8_t*)&pdst[dswx[ix] + 1] <= (uint8_t*)dst->bo->map + dst->bo->size); \
					pdst[dswx[ix]] = tvalue; \
				} \
			} \
		} while(0)

		if(dst->bpps == 0)
			FILL(uint8_t);
		else if(dst->bpps == 1)
			FILL(uint16_t);
		else if(dst->bpps == 2)
			FILL(uint32_t);
		else
			assert(0 && "unhandled bpp");
#undef FILL
	}

	nouveau_bo_unmap(dst->bo);
}

static inline int
nv04_region_cs2d_format(struct nv04_region* rgn)
{
	switch(rgn->bpps) {
	case 0:
		return NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
	case 1:
		if(rgn->one_bits >= 1)
			return NV04_CONTEXT_SURFACES_2D_FORMAT_X1R5G5B5_X1R5G5B5;
		else
			return NV04_CONTEXT_SURFACES_2D_FORMAT_R5G6B5;
	case 2:
		if(rgn->one_bits >= 8)
			return NV04_CONTEXT_SURFACES_2D_FORMAT_X8R8G8B8_X8R8G8B8;
		else
			return NV04_CONTEXT_SURFACES_2D_FORMAT_A8R8G8B8;
	default:
		return -1;
	}
}

static inline int
nv04_region_sifm_format(struct nv04_region* rgn)
{
	switch(rgn->bpps) {
	case 0:
		return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_Y8;
	case 1:
		if(rgn->one_bits >= 1)
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_X1R5G5B5;
		else
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_R5G6B5;
	case 2:
		if(rgn->one_bits >= 8)
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_X8R8G8B8;
		else
			return NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_FORMAT_A8R8G8B8;
	default:
		return -1;
	}
}
static void
nv04_region_copy_swizzle(struct nv04_2d_context *ctx,
			  struct nv04_region* dst,
			  struct nv04_region* src,
			  int w, int h)
{
	struct nouveau_channel *chan = ctx->swzsurf->channel;
	struct nouveau_grobj *swzsurf = ctx->swzsurf;
	struct nouveau_grobj *sifm = ctx->sifm;
	int cs2d_format = nv04_region_cs2d_format(dst);
	int sifm_format = nv04_region_sifm_format(src);
	/* Max width & height may not be the same on all HW, but must be POT */
	unsigned max_shift = 10;
	unsigned cw = 1 << max_shift;
	unsigned ch = 1 << max_shift;
	unsigned sx = dst->x >> max_shift;
	unsigned sy = dst->y >> max_shift;
	unsigned ex = (dst->x + w - 1) >> max_shift;
	unsigned ey = (dst->y + h - 1) >> max_shift;
	unsigned chunks = (ex - sx + 1) * (ey - sy + 1);
	unsigned chunk_size;
	if(dst->w < cw)
		cw = dst->w;
	if(dst->h < ch)
		ch = dst->h;
	chunk_size = cw * ch << dst->bpps;

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tRGN_COPY_SWIZZLE [%i, %i: %i] ", w, h, dst->bpps);
	for(int i = 0; i < 2; ++i)
	{
		nv04_region_print(i ? src : dst);
		fprintf(stderr, i ? "\n" : " <- ");
	}
#endif

	nv04_region_assert(dst, w, h);
	nv04_region_assert(src, w, h);

	MARK_RING (chan, 8 + chunks * 17, 2 + chunks * 2);

	BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_DMA_IMAGE, 1);
	OUT_RELOCo(chan, dst->bo,
			NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_FORMAT, 1);
	OUT_RING  (chan, cs2d_format |
			 log2i(cw) << NV04_SWIZZLED_SURFACE_FORMAT_BASE_SIZE_U__SHIFT |
			 log2i(ch) << NV04_SWIZZLED_SURFACE_FORMAT_BASE_SIZE_V__SHIFT);

	BEGIN_RING(chan, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_DMA_IMAGE, 1);
	OUT_RELOCo(chan, src->bo,
			 NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	BEGIN_RING(chan, sifm, NV04_SCALED_IMAGE_FROM_MEMORY_SURFACE, 1);
	OUT_RING  (chan, swzsurf->handle);

	assert(!(dst->offset & 63));

	for (int cy = sy; cy <= ey; ++cy) {
	  int ry = MAX2(0, (int)(dst->y - ch * cy));
	  int rh = MIN2((int)ch, (int)(dst->y - ch * cy + h)) - ry;
	  for (int cx = sx; cx <= ex; ++cx) {
	    int rx = MAX2(0, (int)(dst->x - cw * cx));
	    int rw = MIN2((int)cw, (int)(dst->x - cw * cx + w)) - rx;
	    unsigned dst_offset;
	    unsigned src_offset;

	    BEGIN_RING(chan, swzsurf, NV04_SWIZZLED_SURFACE_OFFSET, 1);

	    dst_offset = dst->offset + (nv04_swizzle_bits_2d(cx * cw, cy * ch, dst->w, dst->h) << dst->bpps);
	    assert(dst_offset <= dst->bo->size);
	    assert(dst_offset + chunk_size <= dst->bo->size);
	    OUT_RELOCl(chan, dst->bo, dst_offset,
			    NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	    BEGIN_RING(chan, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION, 9);
	    OUT_RING  (chan, NV03_SCALED_IMAGE_FROM_MEMORY_COLOR_CONVERSION_TRUNCATE);
	    OUT_RING  (chan, sifm_format);
	    OUT_RING  (chan, NV03_SCALED_IMAGE_FROM_MEMORY_OPERATION_SRCCOPY);
	    OUT_RING  (chan, rx | (ry << NV03_SCALED_IMAGE_FROM_MEMORY_CLIP_POINT_Y__SHIFT));
	    OUT_RING  (chan, rh << NV03_SCALED_IMAGE_FROM_MEMORY_CLIP_SIZE_H__SHIFT | rw);
	    OUT_RING  (chan, rx | (ry << NV03_SCALED_IMAGE_FROM_MEMORY_OUT_POINT_Y__SHIFT));
	    OUT_RING  (chan, rh << NV03_SCALED_IMAGE_FROM_MEMORY_OUT_SIZE_H__SHIFT | rw);
	    OUT_RING  (chan, 1 << 20);
	    OUT_RING  (chan, 1 << 20);

	    BEGIN_RING(chan, sifm, NV03_SCALED_IMAGE_FROM_MEMORY_SIZE, 4);
	    OUT_RING  (chan, rh << NV03_SCALED_IMAGE_FROM_MEMORY_SIZE_H__SHIFT | align(rw, 8));
	    OUT_RING  (chan, src->pitch |
			     NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_ORIGIN_CENTER |
			     NV03_SCALED_IMAGE_FROM_MEMORY_FORMAT_FILTER_POINT_SAMPLE);
	    src_offset = src->offset + (cy * ch + ry + src->y - dst->y) * src->pitch + ((cx * cw + rx + src->x - dst->x) << src->bpps);
	    assert(src_offset <= src->bo->size);
	    assert(src_offset + (src->pitch * (rh - 1)) + (rw << src->bpps) <= src->bo->size);
	    OUT_RELOCl(chan, src->bo, src_offset,
			     NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	    OUT_RING  (chan, 0);
	  }
	}
}

static inline void
nv04_copy_m2mf_begin(struct nv04_2d_context *ctx, struct nouveau_bo* dstbo, struct nouveau_bo* srcbo, unsigned commands)
{
	struct nouveau_channel *chan = ctx->m2mf->channel;
	struct nouveau_grobj *m2mf = ctx->m2mf;
	MARK_RING (chan, 3 + commands * 9, 2 + commands * 2);
	BEGIN_RING(chan, m2mf, NV04_M2MF_DMA_BUFFER_IN, 2);
	OUT_RELOCo(chan, srcbo,
		   NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(chan, dstbo,
		   NOUVEAU_BO_GART | NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
}

static inline void
nv04_copy_m2mf_body(struct nv04_2d_context *ctx, struct nouveau_bo* dstbo, int* pdstoff, unsigned dstpitch, struct nouveau_bo* srcbo, int* psrcoff, unsigned srcpitch, unsigned size, unsigned lines)
{
	struct nouveau_channel *chan = ctx->m2mf->channel;
	struct nouveau_grobj *m2mf = ctx->m2mf;

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\t\t\tCOPY_M2MF_BODY [%i, %i] <%i[%u]> lin %u <- <%i[%u]> lin %u\n", size, lines, dstbo->handle, *pdstoff, dstpitch, srcbo->handle, *psrcoff, srcpitch);
#endif

	BEGIN_RING(chan, m2mf, NV04_M2MF_OFFSET_IN, 8);
	OUT_RELOCl(chan, srcbo, *psrcoff,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, dstbo, *pdstoff,
		   NOUVEAU_BO_VRAM | NOUVEAU_BO_GART | NOUVEAU_BO_WR);
	OUT_RING  (chan, srcpitch);
	OUT_RING  (chan, dstpitch);
	OUT_RING  (chan, size);
	OUT_RING  (chan, lines);
	OUT_RING  (chan, 0x0101);
	OUT_RING  (chan, 0);

	*psrcoff += srcpitch * lines;
	*pdstoff += dstpitch * lines;
}

static void
nv04_copy_m2mf(struct nv04_2d_context *ctx,
		struct nouveau_bo* dstbo, int dstoff, unsigned dstpitch,
		struct nouveau_bo* srcbo, int srcoff, unsigned srcpitch,
		unsigned size, unsigned h)
{
	unsigned max_pitch = 32767;
	unsigned max_lines = 2047;

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\t\tCOPY_M2MF [%i, %i] <%i[%i]> lin %u <- <%i[%i]> lin %u\n", size, h, dstbo->handle, dstoff, dstpitch, srcbo->handle, srcoff, srcpitch);
#endif

	if(srcpitch <= max_pitch && dstpitch <= max_pitch)
	{
		unsigned full_pages = h / max_lines;
		unsigned leftover_lines = h - full_pages * max_lines;

		nv04_copy_m2mf_begin(ctx, dstbo, srcbo, full_pages + !!leftover_lines);

		for(unsigned i = 0; i < full_pages; ++i)
			nv04_copy_m2mf_body(ctx, dstbo, &dstoff, dstpitch, srcbo, &srcoff, srcpitch, size, max_lines);

		if(leftover_lines)
			nv04_copy_m2mf_body(ctx, dstbo, &dstoff, dstpitch, srcbo, &srcoff, srcpitch, size, leftover_lines);
	}
	else
	{
		unsigned lines = size / max_pitch;
		unsigned leftover = size - lines * max_pitch;
		unsigned full_pages = lines / max_lines;
		unsigned leftover_lines = lines - full_pages * max_lines;
		unsigned srcgap = srcpitch - size;
		unsigned dstgap = dstpitch - size;

		nv04_copy_m2mf_begin(ctx, dstbo, srcbo, h * (full_pages + !!leftover_lines + !!leftover));

		for(unsigned i = 0; i < h; ++i)
		{
			for(unsigned j = 0; j < full_pages; ++j)
				nv04_copy_m2mf_body(ctx, dstbo, &dstoff, max_pitch, srcbo, &srcoff, max_pitch, max_pitch, max_lines);

			if(leftover_lines)
				nv04_copy_m2mf_body(ctx, dstbo, &dstoff, max_pitch, srcbo, &srcoff, max_pitch, max_pitch, leftover_lines);

			if(leftover)
				nv04_copy_m2mf_body(ctx, dstbo, &dstoff, leftover, srcbo, &srcoff, leftover, leftover, 1);

			srcoff += srcgap;
			dstoff += dstgap;
		}
	}
}

void
nv04_memcpy(struct nv04_2d_context *ctx, struct nouveau_bo* dstbo, int dstoff, struct nouveau_bo* srcbo, int srcoff, unsigned size)
{
#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tMEMCPY [%i] <%i[%i]> <- <%i[%i]>\n", size, dstbo->handle, dstoff, srcbo->handle, srcoff);
#endif

	nv04_copy_m2mf(ctx, dstbo, dstoff, size, srcbo, srcoff, size, size, 1);
}

static void
nv04_region_copy_m2mf(struct nv04_2d_context *ctx, struct nv04_region *dst, struct nv04_region *src, int w, int h)
{
#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tRGN_COPY_M2MF [%i, %i: %i] ", w, h, dst->bpps);
	for(int i = 0; i < 2; ++i)
	{
		nv04_region_print(i ? src : dst);
		fprintf(stderr, i ? "\n" : " <- ");
	}
#endif

	nv04_region_assert(dst, w, h);
	nv04_region_assert(src, w, h);
	assert(src->pitch);
	assert(dst->pitch);

	nv04_copy_m2mf(ctx,
			dst->bo, dst->offset + dst->y * dst->pitch + (dst->x << dst->bpps), dst->pitch,
			src->bo, src->offset + src->y * src->pitch + (src->x << src->bpps), src->pitch,
			w << src->bpps, h);
}

static inline void
nv04_region_copy_blit(struct nv04_2d_context *ctx, struct nv04_region* dst, struct nv04_region* src, int w, int h)
{
	struct nouveau_channel *chan = ctx->surf2d->channel;
	struct nouveau_grobj *surf2d = ctx->surf2d;
	struct nouveau_grobj *blit = ctx->blit;
	int cs2d_format = nv04_region_cs2d_format(dst);

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tRGN_COPY_BLIT [%i, %i: %i] ", w, h, dst->bpps);
	for(int i = 0; i < 2; ++i)
	{
		nv04_region_print(i ? src : dst);
		fprintf(stderr, i ? "\n" : " <- ");
	}
#endif

	assert(!(src->pitch & 63) && src->pitch);
	assert(!(dst->pitch & 63) && dst->pitch);
	nv04_region_assert(dst, w, h);
	nv04_region_assert(src, w, h);

	MARK_RING (chan, 12, 4);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, src->bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCo(chan, dst->bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, cs2d_format);
	OUT_RING  (chan, (dst->pitch << 16) | src->pitch);
	OUT_RELOCl(chan, src->bo, src->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_RD);
	OUT_RELOCl(chan, dst->bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, blit, 0x0300, 3);
	OUT_RING  (chan, (src->y << 16) | src->x);
	OUT_RING  (chan, (dst->y << 16) | dst->x);
	OUT_RING  (chan, ( h << 16) |  w);
}

/* THEOREM: a non-linearizable swizzled destination is always 64 byte aligned, except for 4x2 mipmap levels of swizzled 1bpp surfaces
 * HYPOTESIS:
 * 1. The first mipmap level is 64-byte-aligned
 * PROOF:
 * 1. Thus, all mipmaps level with a parent which is 64-byte or more in size are.
 * 2. At 1bpp, the smallest levels with a <= 32-byte parent are either Nx1 or 1xN or size <=8, thus 4x2, 2x2 or 2x4
 * 3. Nx1, 1xN, 2x4, 2x2 have all subrects linearizable. 4x2 does not.
 * 4. At 2/4bpp or more, the smallest levels with a 32-byte parent are 1xN, Nx1 or 2x2
 *
 * However, nv04_region_align handles that.
 */

// 0 -> done, 1 -> do with 3D engine or CPU, -1 -> do with CPU
// dst and src may be modified, and the possibly modified version should be passed to nv04_region_cpu if necessary
int
nv04_region_copy_2d(struct nv04_2d_context *ctx, struct nv04_region* dst, struct nv04_region* src,
		int w, int h, int dst_to_gpu, int src_on_gpu)
{
	assert(src->bpps == dst->bpps);

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "RGN_COPY [%i, %i: %i] ", w, h, dst->bpps);
	for(int i = 0; i < 2; ++i)
	{
		int gpu = i ? src_on_gpu : dst_to_gpu;
		nv04_region_print(i ? src : dst);
		fprintf(stderr, " %s", gpu ? "gpu" : "cpu");
		fprintf(stderr, i ? "\n" : " <- ");
	}
#endif

	// if they are contiguous and either both swizzled or both linear, reshape
	if(!dst->pitch == !src->pitch
		&& nv04_region_is_contiguous(dst, w, h)
		&& nv04_region_is_contiguous(src, w, h))
	{
		nv04_region_contiguous_shape(dst, &w, &h, 6);
		nv04_region_linearize_contiguous(dst, w, h);
		nv04_region_linearize_contiguous(src, w, h);
	}

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tOPT ");
	for(int i = 0; i < 2; ++i)
	{
		nv04_region_print(i ? src : dst);
		fprintf(stderr, i ? "\n" : " <- ");
	}
#endif

	/* if the destination is not for GPU _and_ source is on CPU, use CPU */
	/* if the destination is not for GPU _or_ source is on CPU, use CPU only if we think it's faster than the GPU */
	/* TODO: benchmark to find out in which cases exactly we should prefer the CPU */
	 if((!dst_to_gpu && !src_on_gpu)
		|| (!dst->pitch && dst->d > 1)
		/* 3D swizzled destination are unwritable by the GPU, and 2D swizzled ones are readable only by the 3D engine */
	 )
		 return -1;
	/* there is no known way to read 2D/3D-swizzled surfaces with the 2D engine
	 * ask the caller to use the 3D engine
	 * If a format cannot be sampled from the 3D engine there is no point in making it swizzled, so we must not do so
	 */
	 else if(!src->pitch)
	 {
#ifdef NV04_REGION_DEBUG
		fprintf(stderr, "\tCOPY_ENG3D\n");
#endif
		 return 1;
	 }
	/* Setup transfer to swizzle the texture to vram if needed */
	else
	{
		if (!dst->pitch)
		{
			if(!dst_to_gpu)
			{
#ifdef NV04_REGION_DEBUG
				fprintf(stderr, "\tCOPY_ENG3D\n");
#endif
				return 1;
			}
			else
			{
				assert(!nv04_region_align(dst, w, h, 6));

				nv04_region_copy_swizzle(ctx, dst, src, w, h);
				return 0;
			}
		}
		else
		{
			/* NV_CONTEXT_SURFACES_2D has buffer alignment restrictions, fallback
			 * to NV_M2MF in this case.
			 * TODO: is this also true for the source? possibly not
			 * TODO: should we just always use m2mf?
			 * TODO: if not, add support for multiple operations to copy_blit
			 */

			if (!dst_to_gpu
				|| w > 2047
				|| h > 2047
				|| (w & 1)
				|| nv04_region_align(src, w, h, 6)
				|| nv04_region_align(dst, w, h, 6)
				)
				nv04_region_copy_m2mf(ctx, dst, src, w, h);
			else
				nv04_region_copy_blit(ctx, dst, src, w, h);

			return 0;
		}
	}
}

static inline void
nv04_region_fill_gdirect(struct nv04_2d_context *ctx, struct nv04_region* dst, int w, int h, unsigned value)
{
	struct nouveau_channel *chan = ctx->surf2d->channel;
	struct nouveau_grobj *surf2d = ctx->surf2d;
	struct nouveau_grobj *rect = ctx->rect;
	int cs2d_format, gdirect_format;

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "\tFILL_GDIRECT\n");
#endif

	assert(!(dst->pitch & 63) && dst->pitch);
	nv04_region_assert(dst, w, h);

	switch(dst->bpps)
	{
	case 0:
		gdirect_format = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
		cs2d_format = NV04_CONTEXT_SURFACES_2D_FORMAT_Y8;
		break;
	case 1:
		gdirect_format = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A16R5G6B5;
		cs2d_format = NV04_CONTEXT_SURFACES_2D_FORMAT_Y16;
		break;
	case 2:
		gdirect_format = NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT_A8R8G8B8;
		cs2d_format = NV04_CONTEXT_SURFACES_2D_FORMAT_Y32;
		break;
	default:
		assert(0);
		gdirect_format = 0;
		cs2d_format = 0;
		break;
	}

	MARK_RING (chan, 15, 4);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RELOCo(chan, dst->bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCo(chan, dst->bo, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	BEGIN_RING(chan, surf2d, NV04_CONTEXT_SURFACES_2D_FORMAT, 4);
	OUT_RING  (chan, cs2d_format);
	OUT_RING  (chan, (dst->pitch << 16) | dst->pitch);
	OUT_RELOCl(chan, dst->bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);
	OUT_RELOCl(chan, dst->bo, dst->offset, NOUVEAU_BO_VRAM | NOUVEAU_BO_WR);

	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR_FORMAT, 1);
	OUT_RING  (chan, gdirect_format);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_COLOR1_A, 1);
	OUT_RING  (chan, value);
	BEGIN_RING(chan, rect, NV04_GDI_RECTANGLE_TEXT_UNCLIPPED_RECTANGLE_POINT(0), 2);
	OUT_RING  (chan, (dst->x << 16) | dst->y);
	OUT_RING  (chan, ( w << 16) |  h);
}

int
nv04_region_fill_2d(struct nv04_2d_context *ctx, struct nv04_region *dst,
		  int w, int h, unsigned value)
{
	if(!w || !h)
		return 0;

#ifdef NV04_REGION_DEBUG
	fprintf(stderr, "FILL [%i, %i: %i] ", w, h, dst->bpps);
	nv04_region_print(dst);
	fprintf(stderr, " <- 0x%x\n", value);
#endif

	if(nv04_region_is_contiguous(dst, w, h))
	{
		nv04_region_contiguous_shape(dst, &w, &h, 6);
		nv04_region_linearize_contiguous(dst, w, h);
	}

	// TODO: maybe do intermediate copies for some cases instead of using the 3D engine/CPU
	/* GdiRect doesn't work together with swzsurf, so the 3D engine, or an intermediate copy, is the only option here */
	if(!dst->pitch)
	{
#ifdef NV04_REGION_DEBUG
		fprintf(stderr, "\tFILL_ENG3D\n");
#endif
		return 1;
	}
	else if(!nv04_region_align(dst, w, h, 6))
	{
		nv04_region_fill_gdirect(ctx, dst, w, h, value);
		return 0;
	}
	else
		return -1;
}


void
nv04_2d_context_takedown(struct nv04_2d_context *ctx)
{
	nouveau_notifier_free(&ctx->ntfy);
	nouveau_grobj_free(&ctx->m2mf);
	nouveau_grobj_free(&ctx->surf2d);
	nouveau_grobj_free(&ctx->swzsurf);
	nouveau_grobj_free(&ctx->rect);
	nouveau_grobj_free(&ctx->blit);
	nouveau_grobj_free(&ctx->sifm);

	free(ctx);
}

struct nv04_2d_context *
nv04_2d_context_init(struct nouveau_channel* chan)
{
	struct nv04_2d_context *ctx = calloc(1, sizeof(struct nv04_2d_context));
	unsigned handle = 0x88000000, class;
	int ret;

	if (!ctx)
		return NULL;

	ret = nouveau_notifier_alloc(chan, handle++, 1, &ctx->ntfy);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	ret = nouveau_grobj_alloc(chan, handle++, 0x0039, &ctx->m2mf);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->m2mf, NV04_M2MF_DMA_NOTIFY, 1);
	OUT_RING  (chan, ctx->ntfy->handle);

	if (chan->device->chipset < 0x10)
		class = NV04_CONTEXT_SURFACES_2D;
	else
		class = NV10_CONTEXT_SURFACES_2D;

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->surf2d);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->surf2d,
			 NV04_CONTEXT_SURFACES_2D_DMA_IMAGE_SOURCE, 2);
	OUT_RING  (chan, chan->vram->handle);
	OUT_RING  (chan, chan->vram->handle);

	if (chan->device->chipset < 0x10)
		class = NV04_IMAGE_BLIT;
	else
		class = NV11_IMAGE_BLIT;

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->blit);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->blit, NV01_IMAGE_BLIT_DMA_NOTIFY, 1);
	OUT_RING  (chan, ctx->ntfy->handle);
	BEGIN_RING(chan, ctx->blit, NV04_IMAGE_BLIT_SURFACES, 1);
	OUT_RING  (chan, ctx->surf2d->handle);
	BEGIN_RING(chan, ctx->blit, NV01_IMAGE_BLIT_OPERATION, 1);
	OUT_RING  (chan, NV01_IMAGE_BLIT_OPERATION_SRCCOPY);

	ret = nouveau_grobj_alloc(chan, handle++, NV04_GDI_RECTANGLE_TEXT,
				  &ctx->rect);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	BEGIN_RING(chan, ctx->rect, NV04_GDI_RECTANGLE_TEXT_DMA_NOTIFY, 1);
	OUT_RING  (chan, ctx->ntfy->handle);
	BEGIN_RING(chan, ctx->rect, NV04_GDI_RECTANGLE_TEXT_SURFACE, 1);
	OUT_RING  (chan, ctx->surf2d->handle);
	BEGIN_RING(chan, ctx->rect, NV04_GDI_RECTANGLE_TEXT_OPERATION, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_OPERATION_SRCCOPY);
	BEGIN_RING(chan, ctx->rect,
			 NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT, 1);
	OUT_RING  (chan, NV04_GDI_RECTANGLE_TEXT_MONOCHROME_FORMAT_LE);

	switch (chan->device->chipset & 0xf0) {
	case 0x00:
	case 0x10:
		class = NV04_SWIZZLED_SURFACE;
		break;
	case 0x20:
		class = NV11_SWIZZLED_SURFACE;
		break;
	case 0x30:
		class = NV30_SWIZZLED_SURFACE;
		break;
	case 0x40:
	case 0x60:
		class = NV40_SWIZZLED_SURFACE;
		break;
	default:
		/* Famous last words: this really can't happen.. */
		assert(0);
		break;
	}

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->swzsurf);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	/* all the Gallium MARK_RING calculations assume no autobinding, so do that now */
	if(ctx->swzsurf->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_grobj_autobind(ctx->swzsurf);

	switch (chan->device->chipset & 0xf0) {
	case 0x10:
	case 0x20:
		class = NV10_SCALED_IMAGE_FROM_MEMORY;
		break;
	case 0x30:
		class = NV30_SCALED_IMAGE_FROM_MEMORY;
		break;
	case 0x40:
	case 0x60:
		class = NV40_SCALED_IMAGE_FROM_MEMORY;
		break;
	default:
		class = NV04_SCALED_IMAGE_FROM_MEMORY;
		break;
	}

	ret = nouveau_grobj_alloc(chan, handle++, class, &ctx->sifm);
	if (ret) {
		nv04_2d_context_takedown(ctx);
		return NULL;
	}

	/* all the Gallium MARK_RING calculations assume no autobinding, so do that now */
	if(ctx->sifm->bound == NOUVEAU_GROBJ_UNBOUND)
		nouveau_grobj_autobind(ctx->sifm);

	return ctx;
}
