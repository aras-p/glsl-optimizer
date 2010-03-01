/**********************************************************
 * Copyright 2008-2009 VMware, Inc.  All rights reserved.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use, copy,
 * modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **********************************************************/

#include "svga_cmd.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
#include "os/os_thread.h"
#include "util/u_format.h"
#include "util/u_math.h"
#include "util/u_memory.h"

#include "svga_screen.h"
#include "svga_context.h"
#include "svga_screen_texture.h"
#include "svga_screen_buffer.h"
#include "svga_winsys.h"
#include "svga_debug.h"
#include "svga_screen_buffer.h"

#include <util/u_string.h>


/* XXX: This isn't a real hardware flag, but just a hack for kernel to
 * know about primary surfaces. Find a better way to accomplish this.
 */
#define SVGA3D_SURFACE_HINT_SCANOUT (1 << 9)


/*
 * Helper function and arrays
 */

SVGA3dSurfaceFormat
svga_translate_format(enum pipe_format format)
{
   switch(format) {
   
   case PIPE_FORMAT_B8G8R8A8_UNORM:
      return SVGA3D_A8R8G8B8;
   case PIPE_FORMAT_B8G8R8X8_UNORM:
      return SVGA3D_X8R8G8B8;

      /* Required for GL2.1:
       */
   case PIPE_FORMAT_B8G8R8A8_SRGB:
      return SVGA3D_A8R8G8B8;

   case PIPE_FORMAT_B5G6R5_UNORM:
      return SVGA3D_R5G6B5;
   case PIPE_FORMAT_B5G5R5A1_UNORM:
      return SVGA3D_A1R5G5B5;
   case PIPE_FORMAT_B4G4R4A4_UNORM:
      return SVGA3D_A4R4G4B4;

      
   /* XXX: Doesn't seem to work properly.
   case PIPE_FORMAT_Z32_UNORM:
      return SVGA3D_Z_D32;
    */
   case PIPE_FORMAT_Z16_UNORM:
      return SVGA3D_Z_D16;
   case PIPE_FORMAT_S8Z24_UNORM:
      return SVGA3D_Z_D24S8;
   case PIPE_FORMAT_X8Z24_UNORM:
      return SVGA3D_Z_D24X8;

   case PIPE_FORMAT_A8_UNORM:
      return SVGA3D_ALPHA8;
   case PIPE_FORMAT_L8_UNORM:
      return SVGA3D_LUMINANCE8;

   case PIPE_FORMAT_DXT1_RGB:
   case PIPE_FORMAT_DXT1_RGBA:
      return SVGA3D_DXT1;
   case PIPE_FORMAT_DXT3_RGBA:
      return SVGA3D_DXT3;
   case PIPE_FORMAT_DXT5_RGBA:
      return SVGA3D_DXT5;

   default:
      return SVGA3D_FORMAT_INVALID;
   }
}


SVGA3dSurfaceFormat
svga_translate_format_render(enum pipe_format format)
{
   switch(format) { 
   case PIPE_FORMAT_B8G8R8A8_UNORM:
   case PIPE_FORMAT_B8G8R8X8_UNORM:
   case PIPE_FORMAT_B5G5R5A1_UNORM:
   case PIPE_FORMAT_B4G4R4A4_UNORM:
   case PIPE_FORMAT_B5G6R5_UNORM:
   case PIPE_FORMAT_S8Z24_UNORM:
   case PIPE_FORMAT_X8Z24_UNORM:
   case PIPE_FORMAT_Z32_UNORM:
   case PIPE_FORMAT_Z16_UNORM:
   case PIPE_FORMAT_L8_UNORM:
      return svga_translate_format(format);

#if 1
   /* For on host conversion */
   case PIPE_FORMAT_DXT1_RGB:
      return SVGA3D_X8R8G8B8;
   case PIPE_FORMAT_DXT1_RGBA:
   case PIPE_FORMAT_DXT3_RGBA:
   case PIPE_FORMAT_DXT5_RGBA:
      return SVGA3D_A8R8G8B8;
#endif

   default:
      return SVGA3D_FORMAT_INVALID;
   }
}


static INLINE void
svga_transfer_dma_band(struct svga_transfer *st,
                       SVGA3dTransferType transfer,
                       unsigned y, unsigned h, unsigned srcy)
{
   struct svga_texture *texture = svga_texture(st->base.texture); 
   struct svga_screen *screen = svga_screen(texture->base.screen);
   SVGA3dCopyBox box;
   enum pipe_error ret;
   
   SVGA_DBG(DEBUG_DMA, "dma %s sid %p, face %u, (%u, %u, %u) - (%u, %u, %u), %ubpp\n",
                transfer == SVGA3D_WRITE_HOST_VRAM ? "to" : "from", 
                texture->handle,
                st->base.face,
                st->base.x,
                y,
                st->base.zslice,
                st->base.x + st->base.width,
                y + h,
                st->base.zslice + 1,
                util_format_get_blocksize(texture->base.format)*8/
                (util_format_get_blockwidth(texture->base.format)*util_format_get_blockheight(texture->base.format)));
   
   box.x = st->base.x;
   box.y = y;
   box.z = st->base.zslice;
   box.w = st->base.width;
   box.h = h;
   box.d = 1;
   box.srcx = 0;
   box.srcy = srcy;
   box.srcz = 0;

   pipe_mutex_lock(screen->swc_mutex);
   ret = SVGA3D_SurfaceDMA(screen->swc, st, transfer, &box, 1);
   if(ret != PIPE_OK) {
      screen->swc->flush(screen->swc, NULL);
      ret = SVGA3D_SurfaceDMA(screen->swc, st, transfer, &box, 1);
      assert(ret == PIPE_OK);
   }
   pipe_mutex_unlock(screen->swc_mutex);
}


static INLINE void
svga_transfer_dma(struct svga_transfer *st,
                 SVGA3dTransferType transfer)
{
   struct svga_texture *texture = svga_texture(st->base.texture); 
   struct svga_screen *screen = svga_screen(texture->base.screen);
   struct svga_winsys_screen *sws = screen->sws;
   struct pipe_fence_handle *fence = NULL;
   
   if (transfer == SVGA3D_READ_HOST_VRAM) {
      SVGA_DBG(DEBUG_PERF, "%s: readback transfer\n", __FUNCTION__);
   }


   if(!st->swbuf) {
      /* Do the DMA transfer in a single go */
      
      svga_transfer_dma_band(st, transfer, st->base.y, st->base.height, 0);

      if(transfer == SVGA3D_READ_HOST_VRAM) {
         svga_screen_flush(screen, &fence);
         sws->fence_finish(sws, fence, 0);
         sws->fence_reference(sws, &fence, NULL);
      }
   }
   else {
      unsigned y, h, srcy;
      unsigned blockheight = util_format_get_blockheight(st->base.texture->format);
      h = st->hw_nblocksy * blockheight;
      srcy = 0;
      for(y = 0; y < st->base.height; y += h) {
         unsigned offset, length;
         void *hw, *sw;

         if (y + h > st->base.height)
            h = st->base.height - y;

         /* Transfer band must be aligned to pixel block boundaries */
         assert(y % blockheight == 0);
         assert(h % blockheight == 0);
         
         offset = y * st->base.stride / blockheight;
         length = h * st->base.stride / blockheight;

         sw = (uint8_t *)st->swbuf + offset;
         
         if(transfer == SVGA3D_WRITE_HOST_VRAM) {
            /* Wait for the previous DMAs to complete */
            /* TODO: keep one DMA (at half the size) in the background */
            if(y) {
               svga_screen_flush(screen, &fence);
               sws->fence_finish(sws, fence, 0);
               sws->fence_reference(sws, &fence, NULL);
            }

            hw = sws->buffer_map(sws, st->hwbuf, PIPE_BUFFER_USAGE_CPU_WRITE);
            assert(hw);
            if(hw) {
               memcpy(hw, sw, length);
               sws->buffer_unmap(sws, st->hwbuf);
            }
         }
         
         svga_transfer_dma_band(st, transfer, y, h, srcy);
         
         if(transfer == SVGA3D_READ_HOST_VRAM) {
            svga_screen_flush(screen, &fence);
            sws->fence_finish(sws, fence, 0);

            hw = sws->buffer_map(sws, st->hwbuf, PIPE_BUFFER_USAGE_CPU_READ);
            assert(hw);
            if(hw) {
               memcpy(sw, hw, length);
               sws->buffer_unmap(sws, st->hwbuf);
            }
         }
      }
   }
}


static struct pipe_texture *
svga_texture_create(struct pipe_screen *screen,
                    const struct pipe_texture *templat)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_texture *tex = CALLOC_STRUCT(svga_texture);
   unsigned width, height, depth;
   unsigned level;
   
   if (!tex)
      goto error1;

   tex->base = *templat;
   pipe_reference_init(&tex->base.reference, 1);
   tex->base.screen = screen;

   assert(templat->last_level < SVGA_MAX_TEXTURE_LEVELS);
   if(templat->last_level >= SVGA_MAX_TEXTURE_LEVELS)
      goto error2;
   
   width = templat->width0;
   height = templat->height0;
   depth = templat->depth0;
   for(level = 0; level <= templat->last_level; ++level) {
      width = u_minify(width, 1);
      height = u_minify(height, 1);
      depth = u_minify(depth, 1);
   }
   
   tex->key.flags = 0;
   tex->key.size.width = templat->width0;
   tex->key.size.height = templat->height0;
   tex->key.size.depth = templat->depth0;
   
   if(templat->target == PIPE_TEXTURE_CUBE) {
      tex->key.flags |= SVGA3D_SURFACE_CUBEMAP;
      tex->key.numFaces = 6;
   }
   else {
      tex->key.numFaces = 1;
   }

   tex->key.cachable = 1;

   if(templat->tex_usage & PIPE_TEXTURE_USAGE_SAMPLER)
      tex->key.flags |= SVGA3D_SURFACE_HINT_TEXTURE;

   if(templat->tex_usage & PIPE_TEXTURE_USAGE_DISPLAY_TARGET) {
      tex->key.cachable = 0;
   }

   if(templat->tex_usage & PIPE_TEXTURE_USAGE_PRIMARY) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_SCANOUT;
      tex->key.cachable = 0;
   }
   
   /* 
    * XXX: Never pass the SVGA3D_SURFACE_HINT_RENDERTARGET hint. Mesa cannot
    * know beforehand whether a texture will be used as a rendertarget or not
    * and it always requests PIPE_TEXTURE_USAGE_RENDER_TARGET, therefore
    * passing the SVGA3D_SURFACE_HINT_RENDERTARGET here defeats its purpose.
    */
#if 0
   if((templat->tex_usage & PIPE_TEXTURE_USAGE_RENDER_TARGET) &&
      !util_format_is_compressed(templat->format))
      tex->key.flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;
#endif
   
   if(templat->tex_usage & PIPE_TEXTURE_USAGE_DEPTH_STENCIL)
      tex->key.flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
   
   tex->key.numMipLevels = templat->last_level + 1;
   
   tex->key.format = svga_translate_format(templat->format);
   if(tex->key.format == SVGA3D_FORMAT_INVALID)
      goto error2;

   SVGA_DBG(DEBUG_DMA, "surface_create for texture\n", tex->handle);
   tex->handle = svga_screen_surface_create(svgascreen, &tex->key);
   if (tex->handle)
      SVGA_DBG(DEBUG_DMA, "  --> got sid %p (texture)\n", tex->handle);

   return &tex->base;

error2:
   FREE(tex);
error1:
   return NULL;
}


static struct pipe_texture *
svga_texture_blanket(struct pipe_screen * screen,
                     const struct pipe_texture *base,
                     const unsigned *stride,
                     struct pipe_buffer *buffer)
{
   struct svga_texture *tex;
   struct svga_buffer *sbuf = svga_buffer(buffer);
   struct svga_winsys_screen *sws = svga_winsys_screen(screen);
   assert(screen);

   /* Only supports one type */
   if (base->target != PIPE_TEXTURE_2D ||
       base->last_level != 0 ||
       base->depth0 != 1) {
      return NULL;
   }

   /**
    * We currently can't do texture blanket on
    * SVGA3D_BUFFER. Need to blit to a temporary surface?
    */

   assert(sbuf->handle);
   if (!sbuf->handle)
      return NULL;

   if (svga_translate_format(base->format) != sbuf->key.format) {
      unsigned f1 = svga_translate_format(base->format);
      unsigned f2 = sbuf->key.format;

      /* It's okay for XRGB and ARGB or depth with/out stencil to get mixed up */
      if ( !( (f1 == SVGA3D_X8R8G8B8 && f2 == SVGA3D_A8R8G8B8) ||
              (f1 == SVGA3D_A8R8G8B8 && f2 == SVGA3D_X8R8G8B8) ||
              (f1 == SVGA3D_Z_D24X8 && f2 == SVGA3D_Z_D24S8) ) ) {
         debug_printf("%s wrong format %u != %u\n", __FUNCTION__, f1, f2);
         return NULL;
      }
   }

   tex = CALLOC_STRUCT(svga_texture);
   if (!tex)
      return NULL;

   tex->base = *base;
   

   if (sbuf->key.format == 1)
      tex->base.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   else if (sbuf->key.format == 2)
      tex->base.format = PIPE_FORMAT_B8G8R8A8_UNORM;

   pipe_reference_init(&tex->base.reference, 1);
   tex->base.screen = screen;

   SVGA_DBG(DEBUG_DMA, "blanket sid %p\n", sbuf->handle);

   /* We don't own this storage, so don't try to cache it.
    */
   assert(sbuf->key.cachable == 0);
   tex->key.cachable = 0;
   sws->surface_reference(sws, &tex->handle, sbuf->handle);

   return &tex->base;
}


struct pipe_texture *
svga_screen_texture_wrap_surface(struct pipe_screen *screen,
				 struct pipe_texture *base,
				 enum SVGA3dSurfaceFormat format,
				 struct svga_winsys_surface *srf)
{
   struct svga_texture *tex;
   assert(screen);

   /* Only supports one type */
   if (base->target != PIPE_TEXTURE_2D ||
       base->last_level != 0 ||
       base->depth0 != 1) {
      return NULL;
   }

   if (!srf)
      return NULL;

   if (svga_translate_format(base->format) != format) {
      unsigned f1 = svga_translate_format(base->format);
      unsigned f2 = format;

      /* It's okay for XRGB and ARGB or depth with/out stencil to get mixed up */
      if ( !( (f1 == SVGA3D_X8R8G8B8 && f2 == SVGA3D_A8R8G8B8) ||
              (f1 == SVGA3D_A8R8G8B8 && f2 == SVGA3D_X8R8G8B8) ||
              (f1 == SVGA3D_Z_D24X8 && f2 == SVGA3D_Z_D24S8) ) ) {
         debug_printf("%s wrong format %u != %u\n", __FUNCTION__, f1, f2);
         return NULL;
      }
   }

   tex = CALLOC_STRUCT(svga_texture);
   if (!tex)
      return NULL;

   tex->base = *base;
   

   if (format == 1)
      tex->base.format = PIPE_FORMAT_B8G8R8X8_UNORM;
   else if (format == 2)
      tex->base.format = PIPE_FORMAT_B8G8R8A8_UNORM;

   pipe_reference_init(&tex->base.reference, 1);
   tex->base.screen = screen;

   SVGA_DBG(DEBUG_DMA, "wrap surface sid %p\n", srf);

   tex->key.cachable = 0;
   tex->handle = srf;

   return &tex->base;
}


static void
svga_texture_destroy(struct pipe_texture *pt)
{
   struct svga_screen *ss = svga_screen(pt->screen);
   struct svga_texture *tex = (struct svga_texture *)pt;

   ss->texture_timestamp++;

   svga_sampler_view_reference(&tex->cached_view, NULL);

   /*
     DBG("%s deleting %p\n", __FUNCTION__, (void *) tex);
   */
   SVGA_DBG(DEBUG_DMA, "unref sid %p (texture)\n", tex->handle);
   svga_screen_surface_destroy(ss, &tex->key, &tex->handle);

   FREE(tex);
}


static void
svga_texture_copy_handle(struct svga_context *svga,
                         struct svga_screen *ss,
                         struct svga_winsys_surface *src_handle,
                         unsigned src_x, unsigned src_y, unsigned src_z,
                         unsigned src_level, unsigned src_face,
                         struct svga_winsys_surface *dst_handle,
                         unsigned dst_x, unsigned dst_y, unsigned dst_z,
                         unsigned dst_level, unsigned dst_face,
                         unsigned width, unsigned height, unsigned depth)
{
   struct svga_surface dst, src;
   enum pipe_error ret;
   SVGA3dCopyBox box, *boxes;

   assert(svga || ss);

   src.handle = src_handle;
   src.real_level = src_level;
   src.real_face = src_face;
   src.real_zslice = 0;

   dst.handle = dst_handle;
   dst.real_level = dst_level;
   dst.real_face = dst_face;
   dst.real_zslice = 0;

   box.x = dst_x;
   box.y = dst_y;
   box.z = dst_z;
   box.w = width;
   box.h = height;
   box.d = depth;
   box.srcx = src_x;
   box.srcy = src_y;
   box.srcz = src_z;

/*
   SVGA_DBG(DEBUG_VIEWS, "mipcopy src: %p %u (%ux%ux%u), dst: %p %u (%ux%ux%u)\n",
            src_handle, src_level, src_x, src_y, src_z,
            dst_handle, dst_level, dst_x, dst_y, dst_z);
*/

   if (svga) {
      ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                    &src.base,
                                    &dst.base,
                                    &boxes, 1);
      if(ret != PIPE_OK) {
         svga_context_flush(svga, NULL);
         ret = SVGA3D_BeginSurfaceCopy(svga->swc,
                                       &src.base,
                                       &dst.base,
                                       &boxes, 1);
         assert(ret == PIPE_OK);
      }
      *boxes = box;
      SVGA_FIFOCommitAll(svga->swc);
   } else {
      pipe_mutex_lock(ss->swc_mutex);
      ret = SVGA3D_BeginSurfaceCopy(ss->swc,
                                    &src.base,
                                    &dst.base,
                                    &boxes, 1);
      if(ret != PIPE_OK) {
         ss->swc->flush(ss->swc, NULL);
         ret = SVGA3D_BeginSurfaceCopy(ss->swc,
                                       &src.base,
                                       &dst.base,
                                       &boxes, 1);
         assert(ret == PIPE_OK);
      }
      *boxes = box;
      SVGA_FIFOCommitAll(ss->swc);
      pipe_mutex_unlock(ss->swc_mutex);
   }
}

static struct svga_winsys_surface *
svga_texture_view_surface(struct pipe_context *pipe,
                          struct svga_texture *tex,
                          SVGA3dSurfaceFormat format,
                          unsigned start_mip,
                          unsigned num_mip,
                          int face_pick,
                          int zslice_pick,
                          struct svga_host_surface_cache_key *key) /* OUT */
{
   struct svga_screen *ss = svga_screen(tex->base.screen);
   struct svga_winsys_surface *handle;
   uint32_t i, j;
   unsigned z_offset = 0;

   SVGA_DBG(DEBUG_PERF, 
            "svga: Create surface view: face %d zslice %d mips %d..%d\n",
            face_pick, zslice_pick, start_mip, start_mip+num_mip-1);

   key->flags = 0;
   key->format = format;
   key->numMipLevels = num_mip;
   key->size.width = u_minify(tex->base.width0, start_mip);
   key->size.height = u_minify(tex->base.height0, start_mip);
   key->size.depth = zslice_pick < 0 ? u_minify(tex->base.depth0, start_mip) : 1;
   key->cachable = 1;
   assert(key->size.depth == 1);
   
   if(tex->base.target == PIPE_TEXTURE_CUBE && face_pick < 0) {
      key->flags |= SVGA3D_SURFACE_CUBEMAP;
      key->numFaces = 6;
   } else {
      key->numFaces = 1;
   }

   if(key->format == SVGA3D_FORMAT_INVALID) {
      key->cachable = 0;
      return NULL;
   }

   SVGA_DBG(DEBUG_DMA, "surface_create for texture view\n");
   handle = svga_screen_surface_create(ss, key);
   if (!handle) {
      key->cachable = 0;
      return NULL;
   }

   SVGA_DBG(DEBUG_DMA, " --> got sid %p (texture view)\n", handle);

   if (face_pick < 0)
      face_pick = 0;

   if (zslice_pick >= 0)
       z_offset = zslice_pick;

   for (i = 0; i < key->numMipLevels; i++) {
      for (j = 0; j < key->numFaces; j++) {
         if(tex->defined[j + face_pick][i + start_mip]) {
            unsigned depth = (zslice_pick < 0 ?
                              u_minify(tex->base.depth0, i + start_mip) :
                              1);

            svga_texture_copy_handle(svga_context(pipe),
                                     ss,
                                     tex->handle, 
                                     0, 0, z_offset, 
                                     i + start_mip, 
                                     j + face_pick,
                                     handle, 0, 0, 0, i, j,
                                     u_minify(tex->base.width0, i + start_mip),
                                     u_minify(tex->base.height0, i + start_mip),
                                     depth);
         }
      }
   }

   return handle;
}


static struct pipe_surface *
svga_get_tex_surface(struct pipe_screen *screen,
                     struct pipe_texture *pt,
                     unsigned face, unsigned level, unsigned zslice,
                     unsigned flags)
{
   struct svga_texture *tex = svga_texture(pt);
   struct svga_surface *s;
   boolean render = flags & PIPE_BUFFER_USAGE_GPU_WRITE ? TRUE : FALSE;
   boolean view = FALSE;
   SVGA3dSurfaceFormat format;

   s = CALLOC_STRUCT(svga_surface);
   if (!s)
      return NULL;

   pipe_reference_init(&s->base.reference, 1);
   pipe_texture_reference(&s->base.texture, pt);
   s->base.format = pt->format;
   s->base.width = u_minify(pt->width0, level);
   s->base.height = u_minify(pt->height0, level);
   s->base.usage = flags;
   s->base.level = level;
   s->base.face = face;
   s->base.zslice = zslice;

   if (!render)
      format = svga_translate_format(pt->format);
   else
      format = svga_translate_format_render(pt->format);

   assert(format != SVGA3D_FORMAT_INVALID);
   assert(!(flags & PIPE_BUFFER_USAGE_CPU_READ_WRITE));


   if (svga_screen(screen)->debug.force_surface_view)
      view = TRUE;

   /* Currently only used for compressed textures */
   if (render && 
       format != svga_translate_format(pt->format)) {
      view = TRUE;
   }

   if (level != 0 && 
       svga_screen(screen)->debug.force_level_surface_view)
      view = TRUE;

   if (pt->target == PIPE_TEXTURE_3D)
      view = TRUE;

   if (svga_screen(screen)->debug.no_surface_view)
      view = FALSE;

   if (view) {
      SVGA_DBG(DEBUG_VIEWS, "svga: Surface view: yes %p, level %u face %u z %u, %p\n",
               pt, level, face, zslice, s);

      s->handle = svga_texture_view_surface(NULL, tex, format, level, 1, face, zslice,
                                            &s->key);
      s->real_face = 0;
      s->real_level = 0;
      s->real_zslice = 0;
   } else {
      SVGA_DBG(DEBUG_VIEWS, "svga: Surface view: no %p, level %u, face %u, z %u, %p\n",
               pt, level, face, zslice, s);

      memset(&s->key, 0, sizeof s->key);
      s->handle = tex->handle;
      s->real_face = face;
      s->real_level = level;
      s->real_zslice = zslice;
   }

   return &s->base;
}


static void
svga_tex_surface_destroy(struct pipe_surface *surf)
{
   struct svga_surface *s = svga_surface(surf);
   struct svga_texture *t = svga_texture(surf->texture);
   struct svga_screen *ss = svga_screen(surf->texture->screen);

   if(s->handle != t->handle) {
      SVGA_DBG(DEBUG_DMA, "unref sid %p (tex surface)\n", s->handle);
      svga_screen_surface_destroy(ss, &s->key, &s->handle);
   }

   pipe_texture_reference(&surf->texture, NULL);
   FREE(surf);
}


static INLINE void 
svga_mark_surface_dirty(struct pipe_surface *surf)
{
   struct svga_surface *s = svga_surface(surf);

   if(!s->dirty) {
      struct svga_texture *tex = svga_texture(surf->texture);

      s->dirty = TRUE;

      if (s->handle == tex->handle)
         tex->defined[surf->face][surf->level] = TRUE;
      else {
         /* this will happen later in svga_propagate_surface */
      }
   }
}


void svga_mark_surfaces_dirty(struct svga_context *svga)
{
   unsigned i;

   for (i = 0; i < PIPE_MAX_COLOR_BUFS; i++) {
      if (svga->curr.framebuffer.cbufs[i])
         svga_mark_surface_dirty(svga->curr.framebuffer.cbufs[i]);
   }
   if (svga->curr.framebuffer.zsbuf)
      svga_mark_surface_dirty(svga->curr.framebuffer.zsbuf);
}

/**
 * Progagate any changes from surfaces to texture.
 * pipe is optional context to inline the blit command in.
 */
void
svga_propagate_surface(struct pipe_context *pipe, struct pipe_surface *surf)
{
   struct svga_surface *s = svga_surface(surf);
   struct svga_texture *tex = svga_texture(surf->texture);
   struct svga_screen *ss = svga_screen(surf->texture->screen);

   if (!s->dirty)
      return;

   s->dirty = FALSE;
   ss->texture_timestamp++;
   tex->view_age[surf->level] = ++(tex->age);

   if (s->handle != tex->handle) {
      SVGA_DBG(DEBUG_VIEWS, "svga: Surface propagate: tex %p, level %u, from %p\n", tex, surf->level, surf);
      svga_texture_copy_handle(svga_context(pipe), ss,
                               s->handle, 0, 0, 0, s->real_level, s->real_face,
                               tex->handle, 0, 0, surf->zslice, surf->level, surf->face,
                               u_minify(tex->base.width0, surf->level),
                               u_minify(tex->base.height0, surf->level), 1);
      tex->defined[surf->face][surf->level] = TRUE;
   }
}

/**
 * Check if we should call svga_propagate_surface on the surface.
 */
extern boolean
svga_surface_needs_propagation(struct pipe_surface *surf)
{
   struct svga_surface *s = svga_surface(surf);
   struct svga_texture *tex = svga_texture(surf->texture);

   return s->dirty && s->handle != tex->handle;
}


static struct pipe_transfer *
svga_get_tex_transfer(struct pipe_screen *screen,
                     struct pipe_texture *texture,
                     unsigned face, unsigned level, unsigned zslice,
                     enum pipe_transfer_usage usage, unsigned x, unsigned y,
                     unsigned w, unsigned h)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st;
   unsigned nblocksx = util_format_get_nblocksx(texture->format, w);
   unsigned nblocksy = util_format_get_nblocksy(texture->format, h);

   /* We can't map texture storage directly */
   if (usage & PIPE_TRANSFER_MAP_DIRECTLY)
      return NULL;

   st = CALLOC_STRUCT(svga_transfer);
   if (!st)
      return NULL;
   
   st->base.x = x;
   st->base.y = y;
   st->base.width = w;
   st->base.height = h;
   st->base.stride = nblocksx*util_format_get_blocksize(texture->format);
   st->base.usage = usage;
   st->base.face = face;
   st->base.level = level;
   st->base.zslice = zslice;

   st->hw_nblocksy = nblocksy;
   
   st->hwbuf = svga_winsys_buffer_create(ss, 
                                         1, 
                                         0,
                                         st->hw_nblocksy*st->base.stride);
   while(!st->hwbuf && (st->hw_nblocksy /= 2)) {
      st->hwbuf = svga_winsys_buffer_create(ss, 
                                            1, 
                                            0,
                                            st->hw_nblocksy*st->base.stride);
   }

   if(!st->hwbuf)
      goto no_hwbuf;

   if(st->hw_nblocksy < nblocksy) {
      /* We couldn't allocate a hardware buffer big enough for the transfer, 
       * so allocate regular malloc memory instead */
      debug_printf("%s: failed to allocate %u KB of DMA, splitting into %u x %u KB DMA transfers\n",
                   __FUNCTION__,
                   (nblocksy*st->base.stride + 1023)/1024,
                   (nblocksy + st->hw_nblocksy - 1)/st->hw_nblocksy,
                   (st->hw_nblocksy*st->base.stride + 1023)/1024);
      st->swbuf = MALLOC(nblocksy*st->base.stride);
      if(!st->swbuf)
         goto no_swbuf;
   }
   
   pipe_texture_reference(&st->base.texture, texture);

   if (usage & PIPE_TRANSFER_READ)
      svga_transfer_dma(st, SVGA3D_READ_HOST_VRAM);

   return &st->base;

no_swbuf:
   sws->buffer_destroy(sws, st->hwbuf);
no_hwbuf:
   FREE(st);
   return NULL;
}


static void *
svga_transfer_map( struct pipe_screen *screen,
                   struct pipe_transfer *transfer )
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);

   if(st->swbuf)
      return st->swbuf;
   else
      /* The wait for read transfers already happened when svga_transfer_dma
       * was called. */
      return sws->buffer_map(sws, st->hwbuf,
                             pipe_transfer_buffer_flags(transfer));
}


static void
svga_transfer_unmap(struct pipe_screen *screen,
                    struct pipe_transfer *transfer)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);
   
   if(!st->swbuf)
      sws->buffer_unmap(sws, st->hwbuf);
}


static void
svga_tex_transfer_destroy(struct pipe_transfer *transfer)
{
   struct svga_texture *tex = svga_texture(transfer->texture);
   struct svga_screen *ss = svga_screen(transfer->texture->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);

   if (st->base.usage & PIPE_TRANSFER_WRITE) {
      svga_transfer_dma(st, SVGA3D_WRITE_HOST_VRAM);
      ss->texture_timestamp++;
      tex->view_age[transfer->level] = ++(tex->age);
      tex->defined[transfer->face][transfer->level] = TRUE;
   }

   pipe_texture_reference(&st->base.texture, NULL);
   FREE(st->swbuf);
   sws->buffer_destroy(sws, st->hwbuf);
   FREE(st);
}

void
svga_screen_init_texture_functions(struct pipe_screen *screen)
{
   screen->texture_create = svga_texture_create;
   screen->texture_destroy = svga_texture_destroy;
   screen->get_tex_surface = svga_get_tex_surface;
   screen->tex_surface_destroy = svga_tex_surface_destroy;
   screen->texture_blanket = svga_texture_blanket;
   screen->get_tex_transfer = svga_get_tex_transfer;
   screen->transfer_map = svga_transfer_map;
   screen->transfer_unmap = svga_transfer_unmap;
   screen->tex_transfer_destroy = svga_tex_transfer_destroy;
}

/*********************************************************************** 
 */

struct svga_sampler_view *
svga_get_tex_sampler_view(struct pipe_context *pipe, struct pipe_texture *pt,
                          unsigned min_lod, unsigned max_lod)
{
   struct svga_screen *ss = svga_screen(pt->screen);
   struct svga_texture *tex = svga_texture(pt); 
   struct svga_sampler_view *sv = NULL;
   SVGA3dSurfaceFormat format = svga_translate_format(pt->format);
   boolean view = TRUE;

   assert(pt);
   assert(min_lod >= 0);
   assert(min_lod <= max_lod);
   assert(max_lod <= pt->last_level);


   /* Is a view needed */
   {
      /*
       * Can't control max lod. For first level views and when we only
       * look at one level we disable mip filtering to achive the same
       * results as a view.
       */
      if (min_lod == 0 && max_lod >= pt->last_level)
         view = FALSE;

      if (util_format_is_compressed(pt->format) && view) {
         format = svga_translate_format_render(pt->format);
      }

      if (ss->debug.no_sampler_view)
         view = FALSE;

      if (ss->debug.force_sampler_view)
         view = TRUE;
   }

   /* First try the cache */
   if (view) {
      pipe_mutex_lock(ss->tex_mutex);
      if (tex->cached_view &&
          tex->cached_view->min_lod == min_lod &&
          tex->cached_view->max_lod == max_lod) {
         svga_sampler_view_reference(&sv, tex->cached_view);
         pipe_mutex_unlock(ss->tex_mutex);
         SVGA_DBG(DEBUG_VIEWS, "svga: Sampler view: reuse %p, %u %u, last %u\n",
                              pt, min_lod, max_lod, pt->last_level);
         svga_validate_sampler_view(svga_context(pipe), sv);
         return sv;
      }
      pipe_mutex_unlock(ss->tex_mutex);
   }

   sv = CALLOC_STRUCT(svga_sampler_view);
   pipe_reference_init(&sv->reference, 1);
   pipe_texture_reference(&sv->texture, pt);
   sv->min_lod = min_lod;
   sv->max_lod = max_lod;

   /* No view needed just use the whole texture */
   if (!view) {
      SVGA_DBG(DEBUG_VIEWS,
               "svga: Sampler view: no %p, mips %u..%u, nr %u, size (%ux%ux%u), last %u\n",
               pt, min_lod, max_lod,
               max_lod - min_lod + 1,
               pt->width0,
               pt->height0,
               pt->depth0,
               pt->last_level);
      sv->key.cachable = 0;
      sv->handle = tex->handle;
      return sv;
   }

   SVGA_DBG(DEBUG_VIEWS,
            "svga: Sampler view: yes %p, mips %u..%u, nr %u, size (%ux%ux%u), last %u\n",
            pt, min_lod, max_lod,
            max_lod - min_lod + 1,
            pt->width0,
            pt->height0,
            pt->depth0,
            pt->last_level);

   sv->age = tex->age;
   sv->handle = svga_texture_view_surface(pipe, tex, format,
                                          min_lod,
                                          max_lod - min_lod + 1,
                                          -1, -1,
                                          &sv->key);

   if (!sv->handle) {
      assert(0);
      sv->key.cachable = 0;
      sv->handle = tex->handle;
      return sv;
   }

   pipe_mutex_lock(ss->tex_mutex);
   svga_sampler_view_reference(&tex->cached_view, sv);
   pipe_mutex_unlock(ss->tex_mutex);

   return sv;
}

void
svga_validate_sampler_view(struct svga_context *svga, struct svga_sampler_view *v)
{
   struct svga_texture *tex = svga_texture(v->texture);
   unsigned numFaces;
   unsigned age = 0;
   int i, k;

   assert(svga);

   if (v->handle == tex->handle)
      return;

   age = tex->age;

   if(tex->base.target == PIPE_TEXTURE_CUBE)
      numFaces = 6;
   else
      numFaces = 1;

   for (i = v->min_lod; i <= v->max_lod; i++) {
      for (k = 0; k < numFaces; k++) {
         if (v->age < tex->view_age[i])
            svga_texture_copy_handle(svga, NULL,
                                     tex->handle, 0, 0, 0, i, k,
                                     v->handle, 0, 0, 0, i - v->min_lod, k,
                                     u_minify(tex->base.width0, i),
                                     u_minify(tex->base.height0, i),
                                     u_minify(tex->base.depth0, i));
      }
   }

   v->age = age;
}

void
svga_destroy_sampler_view_priv(struct svga_sampler_view *v)
{
   struct svga_texture *tex = svga_texture(v->texture);

   if(v->handle != tex->handle) {
      struct svga_screen *ss = svga_screen(v->texture->screen);
      SVGA_DBG(DEBUG_DMA, "unref sid %p (sampler view)\n", v->handle);
      svga_screen_surface_destroy(ss, &v->key, &v->handle);
   }
   pipe_texture_reference(&v->texture, NULL);
   FREE(v);
}

boolean
svga_screen_buffer_from_texture(struct pipe_texture *texture,
				struct pipe_buffer **buffer,
				unsigned *stride)
{
   struct svga_texture *stex = svga_texture(texture);

   *buffer = svga_screen_buffer_wrap_surface
      (texture->screen,
       svga_translate_format(texture->format),
       stex->handle);

   *stride = util_format_get_stride(texture->format, texture->width0);

   return *buffer != NULL;
}


struct svga_winsys_surface *
svga_screen_texture_get_winsys_surface(struct pipe_texture *texture)
{
   struct svga_winsys_screen *sws = svga_winsys_screen(texture->screen);
   struct svga_winsys_surface *vsurf = NULL;

   assert(svga_texture(texture)->key.cachable == 0);
   svga_texture(texture)->key.cachable = 0;
   sws->surface_reference(sws, &vsurf, svga_texture(texture)->handle);
   return vsurf;
}
