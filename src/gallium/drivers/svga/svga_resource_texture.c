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

#include "svga3d_reg.h"
#include "svga3d_surfacedefs.h"

#include "pipe/p_state.h"
#include "pipe/p_defines.h"
#include "os/os_thread.h"
#include "util/u_format.h"
#include "util/u_inlines.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_resource.h"

#include "svga_cmd.h"
#include "svga_format.h"
#include "svga_screen.h"
#include "svga_context.h"
#include "svga_resource_texture.h"
#include "svga_resource_buffer.h"
#include "svga_sampler_view.h"
#include "svga_winsys.h"
#include "svga_debug.h"


/* XXX: This isn't a real hardware flag, but just a hack for kernel to
 * know about primary surfaces. Find a better way to accomplish this.
 */
#define SVGA3D_SURFACE_HINT_SCANOUT (1 << 9)


static INLINE void
svga_transfer_dma_band(struct svga_context *svga,
                       struct svga_transfer *st,
                       SVGA3dTransferType transfer,
                       unsigned y, unsigned h, unsigned srcy,
                       SVGA3dSurfaceDMAFlags flags)
{
   struct svga_texture *texture = svga_texture(st->base.resource); 
   SVGA3dCopyBox box;
   enum pipe_error ret;
 
   assert(!st->use_direct_map);

   box.x = st->base.box.x;
   box.y = y;
   box.z = st->base.box.z;
   box.w = st->base.box.width;
   box.h = h;
   box.d = 1;
   box.srcx = 0;
   box.srcy = srcy;
   box.srcz = 0;

   if (st->base.resource->target == PIPE_TEXTURE_CUBE) {
      st->face = st->base.box.z;
      box.z = 0;
   }
   else
      st->face = 0;

   SVGA_DBG(DEBUG_DMA, "dma %s sid %p, face %u, (%u, %u, %u) - (%u, %u, %u), %ubpp\n",
                transfer == SVGA3D_WRITE_HOST_VRAM ? "to" : "from", 
                texture->handle,
                st->face,
                st->base.box.x,
                y,
                box.z,
                st->base.box.x + st->base.box.width,
                y + h,
                box.z + 1,
                util_format_get_blocksize(texture->b.b.format) * 8 /
                (util_format_get_blockwidth(texture->b.b.format)*util_format_get_blockheight(texture->b.b.format)));

   ret = SVGA3D_SurfaceDMA(svga->swc, st, transfer, &box, 1, flags);
   if(ret != PIPE_OK) {
      svga_context_flush(svga, NULL);
      ret = SVGA3D_SurfaceDMA(svga->swc, st, transfer, &box, 1, flags);
      assert(ret == PIPE_OK);
   }
}


static INLINE void
svga_transfer_dma(struct svga_context *svga,
                  struct svga_transfer *st,
                  SVGA3dTransferType transfer,
                  SVGA3dSurfaceDMAFlags flags)
{
   struct svga_texture *texture = svga_texture(st->base.resource); 
   struct svga_screen *screen = svga_screen(texture->b.b.screen);
   struct svga_winsys_screen *sws = screen->sws;
   struct pipe_fence_handle *fence = NULL;

   assert(!st->use_direct_map);

   if (transfer == SVGA3D_READ_HOST_VRAM) {
      SVGA_DBG(DEBUG_PERF, "%s: readback transfer\n", __FUNCTION__);
   }

   /* Ensure any pending operations on host surfaces are queued on the command
    * buffer first.
    */
   svga_surfaces_flush( svga );

   if(!st->swbuf) {
      /* Do the DMA transfer in a single go */

      svga_transfer_dma_band(svga, st, transfer,
                             st->base.box.y, st->base.box.height, 0,
                             flags);

      if(transfer == SVGA3D_READ_HOST_VRAM) {
         svga_context_flush(svga, &fence);
         sws->fence_finish(sws, fence, 0);
         sws->fence_reference(sws, &fence, NULL);
      }
   }
   else {
      int y, h, srcy;
      unsigned blockheight = util_format_get_blockheight(st->base.resource->format);
      h = st->hw_nblocksy * blockheight;
      srcy = 0;
      for(y = 0; y < st->base.box.height; y += h) {
         unsigned offset, length;
         void *hw, *sw;

         if (y + h > st->base.box.height)
            h = st->base.box.height - y;

         /* Transfer band must be aligned to pixel block boundaries */
         assert(y % blockheight == 0);
         assert(h % blockheight == 0);

         offset = y * st->base.stride / blockheight;
         length = h * st->base.stride / blockheight;

         sw = (uint8_t *)st->swbuf + offset;

         if (transfer == SVGA3D_WRITE_HOST_VRAM) {
            unsigned usage = PIPE_TRANSFER_WRITE;

            /* Wait for the previous DMAs to complete */
            /* TODO: keep one DMA (at half the size) in the background */
            if (y) {
               svga_context_flush(svga, NULL);
               usage |= PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE;
            }

            hw = sws->buffer_map(sws, st->hwbuf, usage);
            assert(hw);
            if (hw) {
               memcpy(hw, sw, length);
               sws->buffer_unmap(sws, st->hwbuf);
            }
         }

         svga_transfer_dma_band(svga, st, transfer, y, h, srcy, flags);

         /*
          * Prevent the texture contents to be discarded on the next band
          * upload.
          */

         flags.discard = FALSE;

         if(transfer == SVGA3D_READ_HOST_VRAM) {
            svga_context_flush(svga, &fence);
            sws->fence_finish(sws, fence, 0);

            hw = sws->buffer_map(sws, st->hwbuf, PIPE_TRANSFER_READ);
            assert(hw);
            if(hw) {
               memcpy(sw, hw, length);
               sws->buffer_unmap(sws, st->hwbuf);
            }
         }
      }
   }
}


static boolean 
svga_texture_get_handle(struct pipe_screen *screen,
                               struct pipe_resource *texture,
                               struct winsys_handle *whandle)
{
   struct svga_winsys_screen *sws = svga_winsys_screen(texture->screen);
   unsigned stride;

   assert(svga_texture(texture)->key.cachable == 0);
   svga_texture(texture)->key.cachable = 0;
   stride = util_format_get_nblocksx(texture->format, texture->width0) *
            util_format_get_blocksize(texture->format);
   return sws->surface_get_handle(sws, svga_texture(texture)->handle, stride, whandle);
}


static void
svga_texture_destroy(struct pipe_screen *screen,
		     struct pipe_resource *pt)
{
   struct svga_screen *ss = svga_screen(screen);
   struct svga_texture *tex = svga_texture(pt);

   ss->texture_timestamp++;

   svga_sampler_view_reference(&tex->cached_view, NULL);

   /*
     DBG("%s deleting %p\n", __FUNCTION__, (void *) tex);
   */
   SVGA_DBG(DEBUG_DMA, "unref sid %p (texture)\n", tex->handle);
   svga_screen_surface_destroy(ss, &tex->key, &tex->handle);

   ss->total_resource_bytes -= tex->size;

   FREE(tex->rendered_to);
   FREE(tex);
}


/**
 * Determine if we need to read back a texture image before mapping it.
 */
static boolean
need_tex_readback(struct pipe_transfer *transfer)
{
   struct svga_texture *t = svga_texture(transfer->resource);

   if (transfer->usage & PIPE_TRANSFER_READ)
      return TRUE;

   if ((transfer->usage & PIPE_TRANSFER_WRITE) &&
       ((transfer->usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) == 0)) {
      unsigned face;

      if (transfer->resource->target == PIPE_TEXTURE_CUBE) {
         assert(transfer->box.depth == 1);
         face = transfer->box.z;
      }
      else {
         face = 0;
      }
      if (svga_was_texture_rendered_to(t, face, transfer->level)) {
         return TRUE;
      }
   }

   return FALSE;
}



/* XXX: Still implementing this as if it was a screen function, but
 * can now modify it to queue transfers on the context.
 */
static void *
svga_texture_transfer_map(struct pipe_context *pipe,
                          struct pipe_resource *texture,
                          unsigned level,
                          unsigned usage,
                          const struct pipe_box *box,
                          struct pipe_transfer **ptransfer)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st;
   unsigned nblocksx, nblocksy;
   boolean use_direct_map = svga_have_gb_objects(svga) &&
      !svga_have_gb_dma(svga);
   unsigned d;

   /* We can't map texture storage directly unless we have GB objects */
   if (usage & PIPE_TRANSFER_MAP_DIRECTLY) {
      if (svga_have_gb_objects(svga))
         use_direct_map = TRUE;
      else
         return NULL;
   }

   st = CALLOC_STRUCT(svga_transfer);
   if (!st)
      return NULL;

   {
      unsigned w, h;
      if (use_direct_map) {
         /* we'll directly access the guest-backed surface */
         w = u_minify(texture->width0, level);
         h = u_minify(texture->height0, level);
         d = u_minify(texture->depth0, level);
      }
      else {
         /* we'll put the data into a tightly packed buffer */
         w = box->width;
         h = box->height;
         d = box->depth;
      }
      nblocksx = util_format_get_nblocksx(texture->format, w);
      nblocksy = util_format_get_nblocksy(texture->format, h);
   }

   pipe_resource_reference(&st->base.resource, texture);
   st->base.level = level;
   st->base.usage = usage;
   st->base.box = *box;
   st->base.stride = nblocksx*util_format_get_blocksize(texture->format);
   st->base.layer_stride = st->base.stride * nblocksy;

   if (!use_direct_map) {
      /* Use a DMA buffer */
      st->hw_nblocksy = nblocksy;

      st->hwbuf = svga_winsys_buffer_create(svga,
                                            1, 
                                            0,
                                            st->hw_nblocksy * st->base.stride * d);
      while(!st->hwbuf && (st->hw_nblocksy /= 2)) {
         st->hwbuf = svga_winsys_buffer_create(svga,
                                               1, 
                                               0,
                                               st->hw_nblocksy * st->base.stride * d);
      }

      if (!st->hwbuf) {
         FREE(st);
         return NULL;
      }

      if(st->hw_nblocksy < nblocksy) {
         /* We couldn't allocate a hardware buffer big enough for the transfer, 
          * so allocate regular malloc memory instead */
         if (0) {
            debug_printf("%s: failed to allocate %u KB of DMA, "
                         "splitting into %u x %u KB DMA transfers\n",
                         __FUNCTION__,
                         (nblocksy*st->base.stride + 1023)/1024,
                         (nblocksy + st->hw_nblocksy - 1)/st->hw_nblocksy,
                         (st->hw_nblocksy*st->base.stride + 1023)/1024);
         }

         st->swbuf = MALLOC(nblocksy * st->base.stride * d);
         if (!st->swbuf) {
            sws->buffer_destroy(sws, st->hwbuf);
            FREE(st);
            return NULL;
         }
      }

      if (usage & PIPE_TRANSFER_READ) {
         SVGA3dSurfaceDMAFlags flags;
         memset(&flags, 0, sizeof flags);
         svga_transfer_dma(svga, st, SVGA3D_READ_HOST_VRAM, flags);
      }
   } else {
      struct pipe_transfer *transfer = &st->base;
      struct svga_texture *tex = svga_texture(transfer->resource);
      struct svga_winsys_surface *surf = tex->handle;
      unsigned face;

      assert(surf);

      if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
	 face = transfer->box.z;
      } else {
	 face = 0;
      }

      if (need_tex_readback(transfer)) {
	 SVGA3dBox box;
	 enum pipe_error ret;

	 box.x = transfer->box.x;
	 box.y = transfer->box.y;
	 box.w = transfer->box.width;
	 box.h = transfer->box.height;
	 box.d = transfer->box.depth;
	 if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
	    box.z = 0;
	 }
	 else {
	    box.z = transfer->box.z;
	 }

         (void) box;  /* not used at this time */

         svga_surfaces_flush(svga);

	 ret = SVGA3D_ReadbackGBImage(svga->swc, surf, face, transfer->level);

	 if (ret != PIPE_OK) {
	    svga_context_flush(svga, NULL);
	    ret = SVGA3D_ReadbackGBImage(svga->swc, surf, face, transfer->level);
	    assert(ret == PIPE_OK);
	 }

	 svga_context_flush(svga, NULL);

         /*
          * Note: if PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE were specified
          * we could potentially clear the flag for all faces/layers/mips.
          */
         svga_clear_texture_rendered_to(tex, face, transfer->level);
      }
      else {
	 assert(transfer->usage & PIPE_TRANSFER_WRITE);
	 if ((transfer->usage & PIPE_TRANSFER_UNSYNCHRONIZED) == 0) {
            svga_surfaces_flush(svga);
            if (!sws->surface_is_flushed(sws, surf))
               svga_context_flush(svga, NULL);
	 }
      }
   }

   st->use_direct_map = use_direct_map;

   *ptransfer = &st->base;

   /*
    * Begin mapping code
    */
   if (st->swbuf) {
      return st->swbuf;
   }
   else if (!st->use_direct_map) {
      return sws->buffer_map(sws, st->hwbuf, usage);
   }
   else {
      struct svga_screen *screen = svga_screen(svga->pipe.screen);
      SVGA3dSurfaceFormat format;
      SVGA3dSize baseLevelSize;
      struct svga_texture *tex = svga_texture(texture);
      struct svga_winsys_surface *surf = tex->handle;
      uint8_t *map;
      boolean retry;
      unsigned face, offset, mip_width, mip_height;
      unsigned xoffset = box->x;
      unsigned yoffset = box->y;
      unsigned zoffset = box->z;

      map = svga->swc->surface_map(svga->swc, surf, usage, &retry);
      if (map == NULL && retry) {
         /*
          * At this point, the svga_surfaces_flush() should already have
          * called in svga_texture_get_transfer().
          */
         svga_context_flush(svga, NULL);
         map = svga->swc->surface_map(svga->swc, surf, usage, &retry);
      }

      /*
       * Make sure whe return NULL if the map fails
       */
      if (map == NULL) {
         FREE(st);
         return map;
      }

      /**
       * Compute the offset to the specific texture slice in the buffer.
       */
      if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
         face = zoffset;
         zoffset = 0;
      } else {
         face = 0;
      }

      format = svga_translate_format(screen, tex->b.b.format, 0);
      baseLevelSize.width = tex->b.b.width0;
      baseLevelSize.height = tex->b.b.height0;
      baseLevelSize.depth = tex->b.b.depth0;

      offset = svga3dsurface_get_image_offset(format, baseLevelSize,
                                              tex->b.b.last_level + 1, /* numMips */
                                              face, level);
      if (level > 0) {
         assert(offset > 0);
      }

      mip_width = u_minify(tex->b.b.width0, level);
      mip_height = u_minify(tex->b.b.height0, level);

      offset += svga3dsurface_get_pixel_offset(format, mip_width, mip_height,
                                               xoffset, yoffset, zoffset);

      return (void *) (map + offset);
   }
}


/**
 * Unmap a GB texture surface.
 */
static void
svga_texture_surface_unmap(struct svga_context *svga,
                           struct pipe_transfer *transfer)
{
   struct svga_winsys_surface *surf = svga_texture(transfer->resource)->handle;
   struct svga_winsys_context *swc = svga->swc;
   boolean rebind;

   assert(surf);

   swc->surface_unmap(swc, surf, &rebind);
   if (rebind) {
      enum pipe_error ret;
      ret = SVGA3D_BindGBSurface(swc, surf);
      if (ret != PIPE_OK) {
         /* flush and retry */
         svga_context_flush(svga, NULL);
         ret = SVGA3D_BindGBSurface(swc, surf);
         assert(ret == PIPE_OK);
      }
   }
}


/* XXX: Still implementing this as if it was a screen function, but
 * can now modify it to queue transfers on the context.
 */
static void
svga_texture_transfer_unmap(struct pipe_context *pipe,
			    struct pipe_transfer *transfer)
{
   struct svga_context *svga = svga_context(pipe);
   struct svga_screen *ss = svga_screen(pipe->screen);
   struct svga_winsys_screen *sws = ss->sws;
   struct svga_transfer *st = svga_transfer(transfer);
   struct svga_texture *tex = svga_texture(transfer->resource);

   if (!st->swbuf) {
      if (st->use_direct_map) {
         svga_texture_surface_unmap(svga, transfer);
      }
      else {
         sws->buffer_unmap(sws, st->hwbuf);
      }
   }

   if (!st->use_direct_map && (st->base.usage & PIPE_TRANSFER_WRITE)) {
      /* Use DMA to transfer texture data */
      SVGA3dSurfaceDMAFlags flags;

      memset(&flags, 0, sizeof flags);
      if (transfer->usage & PIPE_TRANSFER_DISCARD_WHOLE_RESOURCE) {
         flags.discard = TRUE;
      }
      if (transfer->usage & PIPE_TRANSFER_UNSYNCHRONIZED) {
         flags.unsynchronized = TRUE;
      }

      svga_transfer_dma(svga, st, SVGA3D_WRITE_HOST_VRAM, flags);
   } else if (transfer->usage & PIPE_TRANSFER_WRITE) {
      struct svga_winsys_surface *surf =
	 svga_texture(transfer->resource)->handle;
      unsigned face;
      SVGA3dBox box;
      enum pipe_error ret;

      assert(svga_have_gb_objects(svga));

      /* update the effected region */
      if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
	 face = transfer->box.z;
      } else {
	 face = 0;
      }

      box.x = transfer->box.x;
      box.y = transfer->box.y;
      if (tex->b.b.target == PIPE_TEXTURE_CUBE) {
         box.z = 0;
      }
      else {
         box.z = transfer->box.z;
      }
      box.w = transfer->box.width;
      box.h = transfer->box.height;
      box.d = transfer->box.depth;

      if (0)
         debug_printf("%s %d, %d, %d  %d x %d x %d\n",
                      __FUNCTION__,
                      box.x, box.y, box.z,
                      box.w, box.h, box.d);

      ret = SVGA3D_UpdateGBImage(svga->swc, surf, &box, face, transfer->level);
      if (ret != PIPE_OK) {
         svga_context_flush(svga, NULL);
         ret = SVGA3D_UpdateGBImage(svga->swc, surf, &box, face, transfer->level);
         assert(ret == PIPE_OK);
      }
   }

   ss->texture_timestamp++;
   svga_age_texture_view(tex, transfer->level);
   if (transfer->resource->target == PIPE_TEXTURE_CUBE)
      svga_define_texture_level(tex, transfer->box.z, transfer->level);
   else
      svga_define_texture_level(tex, 0, transfer->level);

   pipe_resource_reference(&st->base.resource, NULL);

   FREE(st->swbuf);
   if (!st->use_direct_map) {
      sws->buffer_destroy(sws, st->hwbuf);
   }
   FREE(st);
}


struct u_resource_vtbl svga_texture_vtbl = 
{
   svga_texture_get_handle,	      /* get_handle */
   svga_texture_destroy,	      /* resource_destroy */
   svga_texture_transfer_map,	      /* transfer_map */
   u_default_transfer_flush_region,   /* transfer_flush_region */
   svga_texture_transfer_unmap,	      /* transfer_unmap */
   u_default_transfer_inline_write    /* transfer_inline_write */
};


struct pipe_resource *
svga_texture_create(struct pipe_screen *screen,
                    const struct pipe_resource *template)
{
   struct svga_screen *svgascreen = svga_screen(screen);
   struct svga_texture *tex = CALLOC_STRUCT(svga_texture);

   if (!tex)
      goto error1;

   tex->b.b = *template;
   tex->b.vtbl = &svga_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   assert(template->last_level < SVGA_MAX_TEXTURE_LEVELS);
   if(template->last_level >= SVGA_MAX_TEXTURE_LEVELS)
      goto error2;
   
   tex->key.flags = 0;
   tex->key.size.width = template->width0;
   tex->key.size.height = template->height0;
   tex->key.size.depth = template->depth0;

   if(template->target == PIPE_TEXTURE_CUBE) {
      tex->key.flags |= SVGA3D_SURFACE_CUBEMAP;
      tex->key.numFaces = 6;
   }
   else {
      tex->key.numFaces = 1;
   }

   if (template->target == PIPE_TEXTURE_3D) {
      tex->key.flags |= SVGA3D_SURFACE_VOLUME;
   }

   tex->key.cachable = 1;

   if (template->bind & PIPE_BIND_SAMPLER_VIEW)
      tex->key.flags |= SVGA3D_SURFACE_HINT_TEXTURE;

   if (template->bind & PIPE_BIND_DISPLAY_TARGET) {
      tex->key.cachable = 0;
   }

   if (template->bind & PIPE_BIND_SHARED) {
      tex->key.cachable = 0;
   }

   if (template->bind & (PIPE_BIND_SCANOUT |
                         PIPE_BIND_CURSOR)) {
      tex->key.flags |= SVGA3D_SURFACE_HINT_SCANOUT;
      tex->key.cachable = 0;
   }

   /* 
    * Note: Previously we never passed the
    * SVGA3D_SURFACE_HINT_RENDERTARGET hint. Mesa cannot
    * know beforehand whether a texture will be used as a rendertarget or not
    * and it always requests PIPE_BIND_RENDER_TARGET, therefore
    * passing the SVGA3D_SURFACE_HINT_RENDERTARGET here defeats its purpose.
    *
    * However, this was changed since other state trackers
    * (XA for example) uses it accurately and certain device versions
    * relies on it in certain situations to render correctly.
    */
   if((template->bind & PIPE_BIND_RENDER_TARGET) &&
      !util_format_is_s3tc(template->format))
      tex->key.flags |= SVGA3D_SURFACE_HINT_RENDERTARGET;
   
   if(template->bind & PIPE_BIND_DEPTH_STENCIL)
      tex->key.flags |= SVGA3D_SURFACE_HINT_DEPTHSTENCIL;
   
   tex->key.numMipLevels = template->last_level + 1;
   
   tex->key.format = svga_translate_format(svgascreen, template->format, template->bind);
   if(tex->key.format == SVGA3D_FORMAT_INVALID)
      goto error2;

   SVGA_DBG(DEBUG_DMA, "surface_create for texture\n", tex->handle);
   tex->handle = svga_screen_surface_create(svgascreen, &tex->key);
   if (tex->handle)
      SVGA_DBG(DEBUG_DMA, "  --> got sid %p (texture)\n", tex->handle);

   debug_reference(&tex->b.b.reference,
                   (debug_reference_descriptor)debug_describe_resource, 0);

   tex->size = util_resource_size(template);
   svgascreen->total_resource_bytes += tex->size;

   tex->rendered_to = CALLOC(template->depth0 * template->array_size,
                             sizeof(tex->rendered_to[0]));
   if (!tex->rendered_to)
      goto error2;

   return &tex->b.b;

error2:
   FREE(tex->rendered_to);
   FREE(tex);
error1:
   return NULL;
}


struct pipe_resource *
svga_texture_from_handle(struct pipe_screen *screen,
			 const struct pipe_resource *template,
			 struct winsys_handle *whandle)
{
   struct svga_winsys_screen *sws = svga_winsys_screen(screen);
   struct svga_winsys_surface *srf;
   struct svga_texture *tex;
   enum SVGA3dSurfaceFormat format = 0;
   assert(screen);

   /* Only supports one type */
   if ((template->target != PIPE_TEXTURE_2D &&
       template->target != PIPE_TEXTURE_RECT) ||
       template->last_level != 0 ||
       template->depth0 != 1) {
      return NULL;
   }

   srf = sws->surface_from_handle(sws, whandle, &format);

   if (!srf)
      return NULL;

   if (svga_translate_format(svga_screen(screen), template->format, template->bind) != format) {
      unsigned f1 = svga_translate_format(svga_screen(screen), template->format, template->bind);
      unsigned f2 = format;

      /* It's okay for XRGB and ARGB or depth with/out stencil to get mixed up */
      if ( !( (f1 == SVGA3D_X8R8G8B8 && f2 == SVGA3D_A8R8G8B8) ||
              (f1 == SVGA3D_A8R8G8B8 && f2 == SVGA3D_X8R8G8B8) ||
              (f1 == SVGA3D_Z_D24X8 && f2 == SVGA3D_Z_D24S8) ||
              (f1 == SVGA3D_Z_DF24 && f2 == SVGA3D_Z_D24S8_INT) ) ) {
         debug_printf("%s wrong format %u != %u\n", __FUNCTION__, f1, f2);
         return NULL;
      }
   }

   tex = CALLOC_STRUCT(svga_texture);
   if (!tex)
      return NULL;

   tex->b.b = *template;
   tex->b.vtbl = &svga_texture_vtbl;
   pipe_reference_init(&tex->b.b.reference, 1);
   tex->b.b.screen = screen;

   SVGA_DBG(DEBUG_DMA, "wrap surface sid %p\n", srf);

   tex->key.cachable = 0;
   tex->handle = srf;

   tex->rendered_to = CALLOC(1, sizeof(tex->rendered_to[0]));

   return &tex->b.b;
}
