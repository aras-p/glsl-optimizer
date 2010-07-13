/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen.
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

#include "vdpau_private.h"
#include <pipe/p_screen.h>
#include <pipe/p_state.h>
#include <util/u_memory.h>

VdpStatus
vlVdpVideoSurfaceCreate(VdpDevice device,
			VdpChromaType chroma_type, 
			uint32_t width, 
			uint32_t height, 
			VdpVideoSurface *surface)
{
    vlVdpSurface *p_surf;
    VdpStatus ret;

    if (!(width && height))
      {
         ret = VDP_STATUS_INVALID_SIZE;
         goto inv_size;
      }
      

    if (!vlCreateHTAB()) {
       ret = VDP_STATUS_RESOURCES;
       goto no_htab;
    }

   p_surf = CALLOC(0, sizeof(p_surf));
   if (!p_surf) {
      ret = VDP_STATUS_RESOURCES;
      goto no_res;
   }

   p_surf->psurface = CALLOC(0,sizeof(struct pipe_surface));
   if (!p_surf->psurface)  {
	   ret = VDP_STATUS_RESOURCES;
	   goto no_surf;
   }

   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)  {
      ret = VDP_STATUS_INVALID_HANDLE;
      goto inv_device;
   }

   if (!dev->vlscreen)
   dev->vlscreen = vl_screen_create(dev->display, dev->screen);
   if (!dev->vlscreen)   {
      ret = VDP_STATUS_RESOURCES;
      goto inv_device;
   }

   p_surf->psurface->height = height;
   p_surf->psurface->width = width;
   p_surf->psurface->level = 0;
   p_surf->psurface->usage = PIPE_USAGE_DEFAULT;
   p_surf->chroma_format = FormatToPipe(chroma_type);
   p_surf->vlscreen = dev->vlscreen;
    
   *surface = vlAddDataHTAB(p_surf);
   if (*surface == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
   }
   
   return VDP_STATUS_OK;

no_handle:
   FREE(p_surf->psurface);
inv_device:
no_surf:
   FREE(p_surf);
no_res:
   // vlDestroyHTAB(); XXX: Do not destroy this tab, I think.
no_htab:
inv_size:
   return ret;
}

VdpStatus
vlVdpVideoSurfaceDestroy  ( VdpVideoSurface surface )
{
   vlVdpSurface *p_surf;

   p_surf = (vlVdpSurface *)vlGetDataHTAB((vlHandle)surface);
   if (!p_surf)
       return VDP_STATUS_INVALID_HANDLE;

   if (p_surf->psurface)
	   p_surf->vlscreen->pscreen->tex_surface_destroy(p_surf->psurface);
	   
   FREE(p_surf);
   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfaceGetParameters ( VdpVideoSurface surface, 
				 VdpChromaType *chroma_type, 
				 uint32_t *width, 
				 uint32_t *height
)
{
   if (!(width && height && chroma_type))  
      return VDP_STATUS_INVALID_POINTER; 
   

   if (!vlCreateHTAB()) 
      return VDP_STATUS_RESOURCES;


   vlVdpSurface *p_surf = vlGetDataHTAB(surface);
   if (!p_surf) 
      return VDP_STATUS_INVALID_HANDLE;


   if (!(p_surf->psurface && p_surf->chroma_format))  
      return VDP_STATUS_INVALID_HANDLE;

   *width = p_surf->psurface->width;
   *height = p_surf->psurface->height;
   *chroma_type = PipeToType(p_surf->chroma_format);

   return VDP_STATUS_OK;
}

VdpStatus
vlVdpVideoSurfaceGetBitsYCbCr ( VdpVideoSurface surface, 
				VdpYCbCrFormat destination_ycbcr_format, 
				void *const *destination_data, 
				uint32_t const *destination_pitches
)
{
   

}
