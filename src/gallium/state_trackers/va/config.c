/**************************************************************************
 *
 * Copyright 2010 Thomas Balling Sørensen & Orasanu Lucian.
 * Copyright 2014 Advanced Micro Devices, Inc.
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
 * IN NO EVENT SHALL THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include "pipe/p_screen.h"

#include "vl/vl_winsys.h"

#include "va_private.h"

VAStatus
vlVaQueryConfigProfiles(VADriverContextP ctx, VAProfile *profile_list, int *num_profiles)
{
   struct pipe_screen *pscreen;
   enum pipe_video_profile p;
   VAProfile vap;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   *num_profiles = 0;

   pscreen = VL_VA_PSCREEN(ctx);
   for (p = PIPE_VIDEO_PROFILE_MPEG2_SIMPLE; p <= PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH; ++p)
      if (pscreen->get_video_param(pscreen, p, PIPE_VIDEO_ENTRYPOINT_BITSTREAM, PIPE_VIDEO_CAP_SUPPORTED)) {
         vap = PipeToProfile(p);
         if (vap != VAProfileNone)
            profile_list[(*num_profiles)++] = vap;
      }

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaQueryConfigEntrypoints(VADriverContextP ctx, VAProfile profile,
                           VAEntrypoint *entrypoint_list, int *num_entrypoints)
{
   struct pipe_screen *pscreen;
   enum pipe_video_profile p;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   *num_entrypoints = 0;

   p = ProfileToPipe(profile);
   if (p == PIPE_VIDEO_PROFILE_UNKNOWN)
      return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

   pscreen = VL_VA_PSCREEN(ctx);
   if (!pscreen->get_video_param(pscreen, p, PIPE_VIDEO_ENTRYPOINT_BITSTREAM, PIPE_VIDEO_CAP_SUPPORTED))
      return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

   entrypoint_list[(*num_entrypoints)++] = VAEntrypointVLD;

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaGetConfigAttributes(VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,
                        VAConfigAttrib *attrib_list, int num_attribs)
{
   int i;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   for (i = 0; i < num_attribs; ++i) {
      unsigned int value;
      switch (attrib_list[i].type) {
      case VAConfigAttribRTFormat:
         value = VA_RT_FORMAT_YUV420;
         break;
      case VAConfigAttribRateControl:
	 value = VA_RC_NONE;
         break;
      default:
         value = VA_ATTRIB_NOT_SUPPORTED;
         break;
      }
      attrib_list[i].value = value;
   }

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaCreateConfig(VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,
                 VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id)
{
   struct pipe_screen *pscreen;
   enum pipe_video_profile p;

   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   p = ProfileToPipe(profile);
   if (p == PIPE_VIDEO_PROFILE_UNKNOWN)
      return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

   pscreen = VL_VA_PSCREEN(ctx);
   if (!pscreen->get_video_param(pscreen, p, PIPE_VIDEO_ENTRYPOINT_BITSTREAM, PIPE_VIDEO_CAP_SUPPORTED))
      return VA_STATUS_ERROR_UNSUPPORTED_PROFILE;

   if (entrypoint != VAEntrypointVLD)
      return VA_STATUS_ERROR_UNSUPPORTED_ENTRYPOINT;

   *config_id = p;

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaDestroyConfig(VADriverContextP ctx, VAConfigID config_id)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   return VA_STATUS_SUCCESS;
}

VAStatus
vlVaQueryConfigAttributes(VADriverContextP ctx, VAConfigID config_id, VAProfile *profile,
                          VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list, int *num_attribs)
{
   if (!ctx)
      return VA_STATUS_ERROR_INVALID_CONTEXT;

   *profile = PipeToProfile(config_id);
   *entrypoint = VAEntrypointVLD;

   *num_attribs = 1;
   attrib_list[0].type = VAConfigAttribRTFormat;
   attrib_list[0].value = VA_RT_FORMAT_YUV420;

   return VA_STATUS_SUCCESS;
}
