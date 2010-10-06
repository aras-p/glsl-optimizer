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
#include "mpeg2_bitstream_parser.h"
#include <util/u_memory.h>
#include <util/u_math.h>
#include <pipe/p_video_context.h>
#include <util/u_debug.h>

VdpStatus
vlVdpDecoderCreate ( 	VdpDevice device, 
						VdpDecoderProfile profile, 
						uint32_t width, uint32_t height, 
						uint32_t max_references, 
						VdpDecoder *decoder 
)
{
	struct vl_screen *vscreen;
	enum pipe_video_profile p_profile;
	VdpStatus ret;
	vlVdpDecoder *vldecoder;
	
	debug_printf("[VDPAU] Creating decoder\n");
	
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
   
   p_profile = ProfileToPipe(profile);
   if (p_profile == PIPE_VIDEO_PROFILE_UNKNOWN)	{
	   ret = VDP_STATUS_INVALID_DECODER_PROFILE;
	   goto inv_profile;
   }

	// TODO: Define max_references. Used mainly for H264
	
	vldecoder->profile = p_profile;
	vldecoder->device = dev;
		
	*decoder = vlAddDataHTAB(vldecoder);
	if (*decoder == 0) {
      ret = VDP_STATUS_ERROR;
      goto no_handle;
	}
	debug_printf("[VDPAU] Decoder created succesfully\n");
	
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
	debug_printf("[VDPAU] Destroying decoder\n");
	vlVdpDecoder *vldecoder;
	
	vldecoder = (vlVdpDecoder *)vlGetDataHTAB(decoder);
	if (!vldecoder)  {
      return VDP_STATUS_INVALID_HANDLE;
	}
	
	if (vldecoder->vctx)
	{
		if (vldecoder->vctx->vscreen)
			vl_screen_destroy(vldecoder->vctx->vscreen);
	}
	
	if (vldecoder->vctx)
		vl_video_destroy(vldecoder->vctx);
		
	FREE(vldecoder);
	
	return VDP_STATUS_OK;
}

VdpStatus
vlVdpCreateSurfaceTarget   (vlVdpDecoder *vldecoder,
							vlVdpSurface *vlsurf
)
{
	struct pipe_resource tmplt;
	struct pipe_resource *surf_tex;
	struct pipe_video_context *vpipe;
	
	debug_printf("[VDPAU] Creating surface\n");
		
	if(!(vldecoder && vlsurf))
		return VDP_STATUS_INVALID_POINTER;
		
	vpipe = vldecoder->vctx;
		
	memset(&tmplt, 0, sizeof(struct pipe_resource));
	tmplt.target = PIPE_TEXTURE_2D;
	tmplt.format = vlsurf->format;
	tmplt.last_level = 0;
	if (vpipe->is_format_supported(vpipe, tmplt.format,
                                  PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET,
                                  PIPE_TEXTURE_GEOM_NON_POWER_OF_TWO)) {
      tmplt.width0 = vlsurf->width;
      tmplt.height0 = vlsurf->height;
    }
    else {
      assert(vpipe->is_format_supported(vpipe, tmplt.format,
                                       PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET,
                                       PIPE_TEXTURE_GEOM_NON_SQUARE));
      tmplt.width0 = util_next_power_of_two(vlsurf->width);
      tmplt.height0 = util_next_power_of_two(vlsurf->height);
    }
	tmplt.depth0 = 1;
	tmplt.usage = PIPE_USAGE_DEFAULT;
	tmplt.bind = PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET;
	tmplt.flags = 0;
	
	surf_tex = vpipe->screen->resource_create(vpipe->screen, &tmplt);
	
	vlsurf->psurface = vpipe->screen->get_tex_surface(vpipe->screen, surf_tex, 0, 0, 0,
                                         PIPE_BIND_SAMPLER_VIEW | PIPE_BIND_RENDER_TARGET);
										 
	pipe_resource_reference(&surf_tex, NULL);
	
	if (!vlsurf->psurface)
		return VDP_STATUS_RESOURCES;
	
	
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
	struct pipe_mpeg12_macroblock *pipe_macroblocks;
	VdpStatus ret;
	
	debug_printf("[VDPAU] Decoding MPEG2\n");

	vpipe = vldecoder->vctx->vpipe;
	t_vdp_surf = vlsurf;
	
	/* if surfaces equals VDP_STATUS_INVALID_HANDLE, they are not used */
	if (picture_info->backward_reference ==  VDP_INVALID_HANDLE) 
		p_vdp_surf = NULL;
	else	{
		p_vdp_surf = (vlVdpSurface *)vlGetDataHTAB(picture_info->backward_reference);
		if (!p_vdp_surf)
			return VDP_STATUS_INVALID_HANDLE;
	}

	if (picture_info->forward_reference ==  VDP_INVALID_HANDLE) 
		f_vdp_surf = NULL;
	else	{
		f_vdp_surf = (vlVdpSurface *)vlGetDataHTAB(picture_info->forward_reference);
		if (!f_vdp_surf)
			return VDP_STATUS_INVALID_HANDLE;
	}
		
	
	if (f_vdp_surf ==  VDP_INVALID_HANDLE) f_vdp_surf = NULL;
	
	ret = vlVdpCreateSurfaceTarget(vldecoder,t_vdp_surf);

	if (vlVdpMPEG2BitstreamToMacroblock(vpipe->screen, bitstream_buffers, bitstream_buffer_count,
                     &num_macroblocks, &pipe_macroblocks))
					 {
						 debug_printf("[VDPAU] Error in frame-header. Skipping.\n");
						 
						 ret = VDP_STATUS_OK;
						 goto skip_frame;
					 }
		
	vpipe->set_decode_target(vpipe,t_surf);
	//vpipe->decode_macroblocks(vpipe, p_surf, f_surf, num_macroblocks, (struct pipe_macroblock *)pipe_macroblocks, NULL);
	
	skip_frame:
	return ret;
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
	struct vl_screen *vscreen;
	VdpStatus ret;
	debug_printf("[VDPAU] Decoding\n");
		
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
		
	vscreen = vl_screen_create(vldecoder->device->display, vldecoder->device->screen);
	if (!vscreen)
		return VDP_STATUS_RESOURCES;
	
	vldecoder->vctx = vl_video_create(vscreen, vldecoder->profile, vlsurf->format, vlsurf->width, vlsurf->height);
	if (!vldecoder->vctx)
		return VDP_STATUS_RESOURCES;
		
	vldecoder->vctx->vscreen = vscreen;
		
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

VdpStatus 
vlVdpGenerateCSCMatrix(
	VdpProcamp *procamp, 
	VdpColorStandard standard,
	VdpCSCMatrix *csc_matrix)
{
	debug_printf("[VDPAU] Generating CSCMatrix\n");
	if (!(csc_matrix && procamp))
		return VDP_STATUS_INVALID_POINTER;
		
	return VDP_STATUS_OK;
}