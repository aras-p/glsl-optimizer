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

#include "brw_blit.h"
#include "brw_context.h"
#include "brw_state.h"
#include "pipe/p_defines.h"
#include "pipe/p_util.h"
#include "pipe/p_inlines.h"
#include "pipe/p_winsys.h"


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
brw_get_tile_rgba(struct pipe_context *pipe,
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
   case PIPE_FORMAT_S8Z24_UNORM:
      {
         const float scale = 1.0f / (float) 0xffffff;
         for (i = 0; i < h; i++) {
            float *pRow = p;
            for (j = 0; j < w; j++) {
               const unsigned pixel = src[j];
               pRow[0] =
               pRow[1] =
               pRow[2] =
               pRow[3] = (pixel & 0xffffff) * scale;
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
brw_put_tile_rgba(struct pipe_context *pipe,
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
brw_get_tile(struct pipe_context *pipe,
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
brw_put_tile(struct pipe_context *pipe,
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


/*
 * XXX note: same as code in sp_surface.c
 */
static struct pipe_surface *
brw_get_tex_surface(struct pipe_context *pipe,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice)
{
   struct brw_texture *tex = (struct brw_texture *)pt;
   struct pipe_surface *ps;
   unsigned offset;  /* in bytes */

   offset = tex->level_offset[level];

   if (pt->target == PIPE_TEXTURE_CUBE) {
      offset += tex->image_offset[level][face] * pt->cpp;
   }
   else if (pt->target == PIPE_TEXTURE_3D) {
      offset += tex->image_offset[level][zslice] * pt->cpp;
   }
   else {
      assert(face == 0);
      assert(zslice == 0);
   }

   ps = pipe->winsys->surface_alloc(pipe->winsys);
   if (ps) {
      assert(ps->format);
      assert(ps->refcount);
      pipe->winsys->buffer_reference(pipe->winsys, &ps->buffer, tex->buffer);
      ps->format = pt->format;
      ps->cpp = pt->cpp;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->pitch = tex->pitch;
      ps->offset = offset;
   }
   return ps;
}

/*
 * XXX Move this into core Mesa?
 */
static void
_mesa_copy_rect(ubyte * dst,
                unsigned cpp,
                unsigned dst_pitch,
                unsigned dst_x,
                unsigned dst_y,
                unsigned width,
                unsigned height,
                const ubyte * src,
                unsigned src_pitch,
		unsigned src_x,
		unsigned src_y)
{
   unsigned i;

   dst_pitch *= cpp;
   src_pitch *= cpp;
   dst += dst_x * cpp;
   src += src_x * cpp;
   dst += dst_y * dst_pitch;
   src += src_y * dst_pitch;
   width *= cpp;

   if (width == dst_pitch && width == src_pitch)
      memcpy(dst, src, height * width);
   else {
      for (i = 0; i < height; i++) {
         memcpy(dst, src, width);
         dst += dst_pitch;
         src += src_pitch;
      }
   }
}

/* Upload data to a rectangular sub-region.  Lots of choices how to do this:
 *
 * - memcpy by span to current destination
 * - upload data as new buffer and blit
 *
 * Currently always memcpy.
 */
static void
brw_surface_data(struct pipe_context *pipe,
                 struct pipe_surface *dst,
                 unsigned dstx, unsigned dsty,
                 const void *src, unsigned src_pitch,
                 unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   _mesa_copy_rect(pipe_surface_map(dst) + dst->offset,
                   dst->cpp,
                   dst->pitch,
                   dstx, dsty, width, height, src, src_pitch, srcx, srcy);

   pipe_surface_unmap(dst);
}


/* Assumes all values are within bounds -- no checking at this level -
 * do it higher up if required.
 */
static void
brw_surface_copy(struct pipe_context *pipe,
                 struct pipe_surface *dst,
                 unsigned dstx, unsigned dsty,
                 struct pipe_surface *src,
                 unsigned srcx, unsigned srcy, unsigned width, unsigned height)
{
   assert(dst != src);
   assert(dst->cpp == src->cpp);

   if (0) {
      _mesa_copy_rect(pipe_surface_map(dst) + dst->offset,
		      dst->cpp,
		      dst->pitch,
		      dstx, dsty,
		      width, height,
		      pipe_surface_map(src) + src->offset,
		      src->pitch,
		      srcx, srcy);

      pipe_surface_unmap(src);
      pipe_surface_unmap(dst);
   }
   else {
      brw_copy_blit(brw_context(pipe),
                    dst->cpp,
                    (short) src->pitch, src->buffer, src->offset, FALSE,
                    (short) dst->pitch, dst->buffer, dst->offset, FALSE,
                    (short) srcx, (short) srcy, (short) dstx, (short) dsty,
                    (short) width, (short) height, PIPE_LOGICOP_COPY);
   }
}

/* Fill a rectangular sub-region.  Need better logic about when to
 * push buffers into AGP - will currently do so whenever possible.
 */
static ubyte *
get_pointer(struct pipe_surface *dst, unsigned x, unsigned y)
{
   return dst->map + (y * dst->pitch + x) * dst->cpp;
}


static void
brw_surface_fill(struct pipe_context *pipe,
                 struct pipe_surface *dst,
                 unsigned dstx, unsigned dsty,
                 unsigned width, unsigned height, unsigned value)
{
   if (0) {
      unsigned i, j;

      (void)pipe_surface_map(dst);

      switch (dst->cpp) {
      case 1: {
	 ubyte *row = get_pointer(dst, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    memset(row, value, width);
	    row += dst->pitch;
	 }
      }
	 break;
      case 2: {
	 ushort *row = (ushort *) get_pointer(dst, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++)
	       row[j] = (ushort) value;
	    row += dst->pitch;
	 }
      }
	 break;
      case 4: {
	 unsigned *row = (unsigned *) get_pointer(dst, dstx, dsty);
	 for (i = 0; i < height; i++) {
	    for (j = 0; j < width; j++)
	       row[j] = value;
	    row += dst->pitch;
	 }
      }
	 break;
      default:
	 assert(0);
	 break;
      }
   }
   else {
      brw_fill_blit(brw_context(pipe),
                    dst->cpp,
                    (short) dst->pitch,
                    dst->buffer, dst->offset, FALSE,
                    (short) dstx, (short) dsty,
                    (short) width, (short) height,
                    value);
   }
}

void
brw_init_surface_functions(struct brw_context *brw)
{
   brw->pipe.get_tex_surface = brw_get_tex_surface;
   brw->pipe.get_tile = brw_get_tile;
   brw->pipe.put_tile = brw_put_tile;
   brw->pipe.get_tile_rgba = brw_get_tile_rgba;
   brw->pipe.put_tile_rgba = brw_put_tile_rgba;

   brw->pipe.surface_data  = brw_surface_data;
   brw->pipe.surface_copy  = brw_surface_copy;
   brw->pipe.surface_fill  = brw_surface_fill;
}
