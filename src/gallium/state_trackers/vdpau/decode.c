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
#include <util/u_memory.h>
#include <pipe/p_video_context.h>

VdpStatus
vlVdpDecoderCreate ( 	VdpDevice device, 
						VdpDecoderProfile profile, 
						uint32_t width, uint32_t height, 
						uint32_t max_references, 
						VdpDecoder *decoder 
)
{
	enum pipe_video_profile p_profile;
	VdpStatus ret;
	vlVdpDecoder *vldecoder;
	
	if (!decoder)
		return VDP_STATUS_INVALID_POINTER;
	
	if (!(width && height))
		return VDP_STATUS_INVALID_VALUE;
		
   vlVdpDevice *dev = vlGetDataHTAB(device);
   if (!dev)  {
      ret = VDP_STATUS_INVALID_HANDLE;
      goto inv_device;
   }
   
   vldecoder = CALLOC(1,sizeof(vlVdpDecoder));
   if (!vldecoder)   {
	   ret = VDP_STATUS_RESOURCES;
	   goto no_decoder;
   }
	
   vldecoder->vlscreen = vl_screen_create(dev->display, dev->screen);
   if (!vldecoder->vlscreen)  
      ret = VDP_STATUS_RESOURCES;
	  goto no_screen;
   
   
   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)	{
	   ret = VDP_STATUS_INVALID_DECODER_PROFILE;
	   goto inv_profile;
   }

	// TODO: Define max_references. Used mainly for H264
	
	vldecoder->chroma_format = p_profile;
	vldecoder->device = dev;
		
	*decoder = vlAddDataHTAB(vldecoder);
	if (*decoder == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
	}
	
	return VDP_STATUS_OK;
	
	no_handle:
	FREE(vldecoder);
	inv_profile:
	no_screen:
	no_decoder:
	inv_device:
    return ret;
}

VdpStatus
vlVdpDecoderDestroy  (VdpDecoder decoder
)
{
	vlVdpDecoder *vldecoder;
	
	vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
	if (!vldecoder)  {
      return VDP_STATUS_INVALID_HANDLE;
	}
	
	if (vldecoder->vctx)
		vl_video_destroy(vldecoder->vctx);
		
	if (vldecoder->vlscreen)
		vl_screen_destroy(vldecoder->vlscreen);
		
	FREE(vldecoder);
	
	return VDP_STATUS_OK;
}

VdpStatus
vlVdpCreateSurface		   (vlVdpDecoder *vldecoder,
							vlVdpSurface *vlsurf
)
{
	
	return VDP_STATUS_OK;
}

VdpStatus
vlVdpDecoderRenderMpeg2    (vlVdpDecoder *vldecoder,
							vlVdpSurface *vlsurf,
							VdpPictureInfoMPEG1Or2 *picture_info,
							uint32_t bitstream_buffer_count,
							VdpBitstreamBuffer const *bitstream_buffers
							)
{
	struct pipe_video_context *vpipe;
	vlVdpSurface *t_vdp_surf;
	vlVdpSurface *p_vdp_surf;
	vlVdpSurface *f_vdp_surf;
	struct pipe_surface *t_surf;
	struct pipe_surface *p_surf;
	struct pipe_surface *f_surf;
	uint32_t num_macroblocks;

	vpipe = vldecoder->vctx->vpipe;
	t_vdp_surf = vlsurf;
    p_vdp_surf = (vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference);
	if (p_vdp_surf)
		return VDP_STATUS_INVALID_HANDLE;
		
	f_vdp_surf = (vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference);
	if (f_vdp_surf)
		return VDP_STATUS_INVALID_HANDLE;
		
	/* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
	if (p_vdp_surf ==  VDP_INVALID_HANDLE) p_vdp_surf = NULL;
	if (f_vdp_surf ==  VDP_INVALID_HANDLE) f_vdp_surf = NULL;
	
	vlVdpCreateSurface(vldecoder,t_vdp_surf);
		
	num_macroblocks = picture_info->slice_count;
	struct pipe_mpeg12_macroblock pipe_macroblocks[num_macroblocks];
	
	/*VdpMacroBlocksToPipe(vpipe->screen, macroblocks, blocks, first_macroblock,
                     num_macroblocks, pipe_macroblocks);*/
		
	vpipe->set_decode_target(vpipe,t_surf);
	/*vpipe->decode_macroblocks(vpipe, p_surf, f_surf, num_macroblocks,
		&pipe_macroblocks->base, &target_surface_priv->render_fence);*/
}

VdpStatus
vlVdpDecoderRender (VdpDecoder decoder, 
					VdpVideoSurface target, 
					VdpPictureInfo const *picture_info, 
					uint32_t bitstream_buffer_count, 
					VdpBitstreamBuffer const *bitstream_buffers
)
{
	vlVdpDecoder *vldecoder;
	vlVdpSurface *vlsurf;
	VdpStatus ret;
		
	if (!(picture_info && bitstream_buffers))
		return VDP_STATUS_INVALID_POINTER;
	
	
	vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
	if (!vldecoder)
		return VDP_STATUS_INVALID_HANDLE;

	vlsurf = (vlVdpSurface *)vlGetDataHTAB(target);
	if (!vlsurf)
		return VDP_STATUS_INVALID_HANDLE;
	
	if (vlsurf->device != vldecoder->device)
		return VDP_STATUS_HANDLE_DEVICE_MISMATCH;
		
	if (vlsurf->chroma_format != vldecoder->chroma_format)
		return VDP_STATUS_INVALID_CHROMA_TYPE;
		
    // TODO: Right now only mpeg2 is supported.
	switch (vldecoder->vctx->vpipe->profile)   {
		case PIPE_VIDEO_PROFILE_MPEG2_SIMPLE:
		case PIPE_VIDEO_PROFILE_MPEG2_MAIN:
			ret = vlVdpDecoderRenderMpeg2(vldecoder,vlsurf,(VdpPictureInfoMPEG1Or2 *)picture_info,
											bitstream_buffer_count,bitstream_buffers);
			break;
		default:
			return VDP_STATUS_INVALID_DECODER_PROFILE;
	}
	assert(0);

	return ret;
}