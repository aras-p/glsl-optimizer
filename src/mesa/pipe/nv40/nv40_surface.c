
/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "nv40_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"
#include "pipe/p_inlines.h"


#define CLIP_TILE \
   do { \
      if (x >= ps->width) \
         return; \
      if (y >= ps->height) \
         return; \
      if (x + w > ps->width) \
         w = ps->width - x; \
      if (y + h > ps->height) \
         h = ps->height -y; \
   } while(0)


/**
 * Note: this is exactly like a8r8g8b8_get_tile() in sp_surface.c
 * Share it someday.
 */
static void
nv40_get_tile_rgba(struct pipe_context *pipe,
                   struct pipe_surface *ps,
                   uint x, uint y, uint w, uint h, float *p)
{
   const unsigned *src
      = ((const unsigned *) (ps->map + ps->offset))
      + y * ps->pitch + x;
   unsigned i, j;
   unsigned w0 = w;

   CLIP_TILE;

   switch (ps->format) {
   case PIPE_FORMAT_A8R8G8B8_UNORM:
      for (i = 0; i < h; i++) {
         float *pRow = p;
         for (j = 0; j < w; j++) {
            const unsigned pixel = src[j];
            pRow[0] = UBYTE_TO_FLOAT((pixel >> 16) & 0xff);
            pRow[1] = UBYTE_TO_FLOAT((pixel >>  8) & 0xff);
            pRow[2] = UBYTE_TO_FLOAT((pixel >>  0) & 0xff);
            pRow[3] = UBYTE_TO_FLOAT((pixel >> 24) & 0xff);
            pRow += 4;
         }
         src += ps->pitch;
         p += w0 * 4;
      }
      break;
   case PIPE_FORMAT_Z24S8_UNORM:
      {
         const float scale = 1.0 / (float) 0xffffff;
         for (i = 0; i < h; i++) {
            float *pRow = p;
            for (j = 0; j < w; j++) {
               const unsigned pixel = src[j];
               pRow[0] =
               pRow[1] =
               pRow[2] =
               pRow[3] = ((pixel & 0xffffff) >> 8) * scale;
               pRow += 4;
            }
            src += ps->pitch;
            p += w0 * 4;
         }
      }
      break;
   default:
      assert(0);
   }
}


static void
nv40_put_tile_rgba(struct pipe_context *pipe,
                   struct pipe_surface *ps,
                   uint x, uint y, uint w, uint h, const float *p)
{
   /* TODO */
   assert(0);
}


/*
 * XXX note: same as code in sp_surface.c
 */
static void
nv40_get_tile(struct pipe_context *pipe,
              struct pipe_surface *ps,
              uint x, uint y, uint w, uint h,
              void *p, int dst_stride)
{
   const uint cpp = ps->cpp;
   const uint w0 = w;
   const ubyte *pSrc;
   ubyte *pDest;
   uint i;

   assert(ps->map);

   CLIP_TILE;

   if (dst_stride == 0) {
      dst_stride = w0 * cpp;
   }

   pSrc = ps->map + ps->offset + (y * ps->pitch + x) * cpp;
   pDest = (ubyte *) p;

   for (i = 0; i < h; i++) {
      memcpy(pDest, pSrc, w0 * cpp);
      pDest += dst_stride;
      pSrc += ps->pitch * cpp;
   }
}


/*
 * XXX note: same as code in sp_surface.c
 */
static void
nv40_put_tile(struct pipe_context *pipe,
              struct pipe_surface *ps,
              uint x, uint y, uint w, uint h,
              const void *p, int src_stride)
{
   const uint cpp = ps->cpp;
   const uint w0 = w;
   const ubyte *pSrc;
   ubyte *pDest;
   uint i;

   assert(ps->map);

   CLIP_TILE;

   if (src_stride == 0) {
      src_stride = w0 * cpp;
   }

   pSrc = (const ubyte *) p;
   pDest = ps->map + ps->offset + (y * ps->pitch + x) * cpp;

   for (i = 0; i < h; i++) {
      memcpy(pDest, pSrc, w0 * cpp);
      pDest += ps->pitch * cpp;
      pSrc += src_stride;
   }
}


static struct pipe_surface *
nv40_get_tex_surface(struct pipe_context *pipe,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice)
{
	struct pipe_winsys *ws = pipe->winsys;
	struct nv40_miptree *nv40mt = (struct nv40_miptree *)pt;
	struct pipe_surface *ps;

	ps = ws->surface_alloc(ws);
	if (!ps)
		return NULL;
	ws->buffer_reference(ws, &ps->buffer, nv40mt->buffer);
	ps->format = pt->format;
	ps->cpp = pt->cpp;
	ps->width = pt->width[level];
	ps->height = pt->height[level];
	ps->pitch = nv40mt->level[level].pitch / ps->cpp;

	if (pt->target == PIPE_TEXTURE_CUBE) {
		ps->offset = nv40mt->level[level].image_offset[face];
	} else
	if (pt->target == PIPE_TEXTURE_3D) {
		ps->offset = nv40mt->level[level].image_offset[zslice];
	} else {
		ps->offset = nv40mt->level[level].image_offset[0];
	}

	return ps;
}

static void
nv40_surface_data(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, const void *src,
		  unsigned src_stride, unsigned srcx, unsigned srcy,
		  unsigned width, unsigned height)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->surface_data(nvws, dest, destx, desty, src, src_stride,
			   srcx, srcy, width, height);
}

static void
nv40_surface_copy(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, struct pipe_surface *src,
		  unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->surface_copy(nvws, dest, destx, desty, src, srcx, srcy,
			   width, height);
}

static void
nv40_surface_fill(struct pipe_context *pipe, struct pipe_surface *dest,
		  unsigned destx, unsigned desty, unsigned width,
		  unsigned height, unsigned value)
{
	struct nv40_context *nv40 = (struct nv40_context *)pipe;
	struct nouveau_winsys *nvws = nv40->nvws;

	nvws->surface_fill(nvws, dest, destx, desty, width, height, value);
}

void
nv40_init_surface_functions(struct nv40_context *nv40)
{
   nv40->pipe.get_tex_surface = nv40_get_tex_surface;
   nv40->pipe.get_tile = nv40_get_tile;
   nv40->pipe.put_tile = nv40_put_tile;
   nv40->pipe.get_tile_rgba = nv40_get_tile_rgba;
   nv40->pipe.put_tile_rgba = nv40_put_tile_rgba;
   nv40->pipe.surface_data = nv40_surface_data;
   nv40->pipe.surface_copy = nv40_surface_copy;
   nv40->pipe.surface_fill = nv40_surface_fill;
}
