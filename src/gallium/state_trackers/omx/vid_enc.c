/**************************************************************************
 *
 * Copyright 2013 Advanced Micro Devices, Inc.
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

/*
 * Authors:
 *      Christian König <christian.koenig@amd.com>
 *
 */


#include <assert.h>

#include <OMX_Video.h>

/* bellagio defines a DEBUG macro that we don't want */
#ifndef DEBUG
#include <bellagio/omxcore.h>
#undef DEBUG
#else
#include <bellagio/omxcore.h>
#endif

#include <bellagio/omx_base_video_port.h>

#include "pipe/p_screen.h"
#include "pipe/p_video_codec.h"
#include "state_tracker/drm_driver.h"
#include "util/u_memory.h"
#include "vl/vl_video_buffer.h"

#include "entrypoint.h"
#include "vid_enc.h"

struct encode_task {
   struct list_head list;

   struct pipe_video_buffer *buf;
   unsigned pic_order_cnt;
   struct pipe_resource *bitstream;
   void *feedback;
};

struct input_buf_private {
   struct list_head tasks;

   struct pipe_resource *resource;
   struct pipe_transfer *transfer;
};

struct output_buf_private {
   struct pipe_resource *bitstream;
   struct pipe_transfer *transfer;
};

static OMX_ERRORTYPE vid_enc_Constructor(OMX_COMPONENTTYPE *comp, OMX_STRING name);
static OMX_ERRORTYPE vid_enc_Destructor(OMX_COMPONENTTYPE *comp);
static OMX_ERRORTYPE vid_enc_SetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param);
static OMX_ERRORTYPE vid_enc_GetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param);
static OMX_ERRORTYPE vid_enc_SetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config);
static OMX_ERRORTYPE vid_enc_GetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config);
static OMX_ERRORTYPE vid_enc_MessageHandler(OMX_COMPONENTTYPE *comp, internalRequestMessageType *msg);
static OMX_ERRORTYPE vid_enc_AllocateInBuffer(omx_base_PortType *port, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
                                              OMX_IN OMX_U32 idx, OMX_IN OMX_PTR private, OMX_IN OMX_U32 size);
static OMX_ERRORTYPE vid_enc_UseInBuffer(omx_base_PortType *port, OMX_BUFFERHEADERTYPE **buf, OMX_U32 idx,
                                         OMX_PTR private, OMX_U32 size, OMX_U8 *mem);
static OMX_ERRORTYPE vid_enc_FreeInBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf);
static OMX_ERRORTYPE vid_enc_EncodeFrame(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf);
static OMX_ERRORTYPE vid_enc_AllocateOutBuffer(omx_base_PortType *comp, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
                                               OMX_IN OMX_U32 idx, OMX_IN OMX_PTR private, OMX_IN OMX_U32 size);
static OMX_ERRORTYPE vid_enc_FreeOutBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf);
static void vid_enc_BufferEncoded(OMX_COMPONENTTYPE *comp, OMX_BUFFERHEADERTYPE* input, OMX_BUFFERHEADERTYPE* output);

static void enc_ReleaseTasks(struct list_head *head);

OMX_ERRORTYPE vid_enc_LoaderComponent(stLoaderComponentType *comp)
{
   comp->componentVersion.s.nVersionMajor = 0;
   comp->componentVersion.s.nVersionMinor = 0;
   comp->componentVersion.s.nRevision = 0;
   comp->componentVersion.s.nStep = 1;
   comp->name_specific_length = 1;
   comp->constructor = vid_enc_Constructor;

   comp->name = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (!comp->name)
      return OMX_ErrorInsufficientResources;

   comp->name_specific = CALLOC(1, sizeof(char *));
   if (!comp->name_specific)
      goto error_arrays;

   comp->role_specific = CALLOC(1, sizeof(char *));
   if (!comp->role_specific)
      goto error_arrays;

   comp->name_specific[0] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->name_specific[0] == NULL)
      goto error_specific;

   comp->role_specific[0] = CALLOC(1, OMX_MAX_STRINGNAME_SIZE);
   if (comp->role_specific[0] == NULL)
      goto error_specific;

   strcpy(comp->name, OMX_VID_ENC_BASE_NAME);
   strcpy(comp->name_specific[0], OMX_VID_ENC_AVC_NAME);
   strcpy(comp->role_specific[0], OMX_VID_ENC_AVC_ROLE);

   return OMX_ErrorNone;

error_specific:
   FREE(comp->role_specific[0]);
   FREE(comp->name_specific[0]);

error_arrays:
   FREE(comp->role_specific);
   FREE(comp->name_specific);

   FREE(comp->name);

   return OMX_ErrorInsufficientResources;
}

static OMX_ERRORTYPE vid_enc_Constructor(OMX_COMPONENTTYPE *comp, OMX_STRING name)
{
   vid_enc_PrivateType *priv;
   omx_base_video_PortType *port;
   struct pipe_screen *screen;
   OMX_ERRORTYPE r;
   int i;

   assert(!comp->pComponentPrivate);

   priv = comp->pComponentPrivate = CALLOC(1, sizeof(vid_enc_PrivateType));
   if (!priv)
      return OMX_ErrorInsufficientResources;

   r = omx_base_filter_Constructor(comp, name);
   if (r)
	return r;

   priv->BufferMgmtCallback = vid_enc_BufferEncoded;
   priv->messageHandler = vid_enc_MessageHandler;
   priv->destructor = vid_enc_Destructor;

   comp->SetParameter = vid_enc_SetParameter;
   comp->GetParameter = vid_enc_GetParameter;
   comp->GetConfig = vid_enc_GetConfig;
   comp->SetConfig = vid_enc_SetConfig;

   priv->screen = omx_get_screen();
   if (!priv->screen)
      return OMX_ErrorInsufficientResources;

   screen = priv->screen->pscreen;
   if (!screen->get_video_param(screen, PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH,
                                PIPE_VIDEO_ENTRYPOINT_ENCODE, PIPE_VIDEO_CAP_SUPPORTED))
      return OMX_ErrorBadParameter;
 
   priv->s_pipe = screen->context_create(screen, priv->screen);
   if (!priv->s_pipe)
      return OMX_ErrorInsufficientResources;

   if (!vl_compositor_init(&priv->compositor, priv->s_pipe)) {
      priv->s_pipe->destroy(priv->s_pipe);
      priv->s_pipe = NULL;
      return OMX_ErrorInsufficientResources;
   }

   if (!vl_compositor_init_state(&priv->cstate, priv->s_pipe)) {
      vl_compositor_cleanup(&priv->compositor);
      priv->s_pipe->destroy(priv->s_pipe);
      priv->s_pipe = NULL;
      return OMX_ErrorInsufficientResources;
   }

   priv->t_pipe = screen->context_create(screen, priv->screen);
   if (!priv->t_pipe)
      return OMX_ErrorInsufficientResources;

   priv->sPortTypesParam[OMX_PortDomainVideo].nStartPortNumber = 0;
   priv->sPortTypesParam[OMX_PortDomainVideo].nPorts = 2;
   priv->ports = CALLOC(2, sizeof(omx_base_PortType *));
   if (!priv->ports)
      return OMX_ErrorInsufficientResources;

   for (i = 0; i < 2; ++i) {
      priv->ports[i] = CALLOC(1, sizeof(omx_base_video_PortType));
      if (!priv->ports[i])
         return OMX_ErrorInsufficientResources;

      base_video_port_Constructor(comp, &priv->ports[i], i, i == 0);
   }

   port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
   port->sPortParam.format.video.nFrameWidth = 176;
   port->sPortParam.format.video.nFrameHeight = 144;
   port->sPortParam.format.video.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
   port->sVideoParam.eColorFormat = OMX_COLOR_FormatYUV420SemiPlanar;
   port->sPortParam.nBufferCountActual = 8;
   port->sPortParam.nBufferCountMin = 4;

   port->Port_SendBufferFunction = vid_enc_EncodeFrame;
   port->Port_AllocateBuffer = vid_enc_AllocateInBuffer;
   port->Port_UseBuffer = vid_enc_UseInBuffer;
   port->Port_FreeBuffer = vid_enc_FreeInBuffer;

   port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
   strcpy(port->sPortParam.format.video.cMIMEType,"video/H264");
   port->sPortParam.format.video.nFrameWidth = 176;
   port->sPortParam.format.video.nFrameHeight = 144;
   port->sPortParam.format.video.eCompressionFormat = OMX_VIDEO_CodingAVC;
   port->sVideoParam.eCompressionFormat = OMX_VIDEO_CodingAVC;

   port->Port_AllocateBuffer = vid_enc_AllocateOutBuffer;
   port->Port_FreeBuffer = vid_enc_FreeOutBuffer;
 
   priv->bitrate.eControlRate = OMX_Video_ControlRateDisable;
   priv->bitrate.nTargetBitrate = 0;

   priv->quant.nQpI = OMX_VID_ENC_QUANT_I_FRAMES_DEFAULT;
   priv->quant.nQpP = OMX_VID_ENC_QUANT_P_FRAMES_DEFAULT;
   priv->quant.nQpB = OMX_VID_ENC_QUANT_B_FRAMES_DEFAULT;

   priv->profile_level.eProfile = OMX_VIDEO_AVCProfileBaseline;
   priv->profile_level.eLevel = OMX_VIDEO_AVCLevel42;

   priv->force_pic_type.IntraRefreshVOP = OMX_FALSE; 
   priv->frame_num = 0;
   priv->pic_order_cnt = 0;
   priv->restricted_b_frames = debug_get_bool_option("OMX_USE_RESTRICTED_B_FRAMES", FALSE);

   priv->scale.xWidth = OMX_VID_ENC_SCALING_WIDTH_DEFAULT;
   priv->scale.xHeight = OMX_VID_ENC_SCALING_WIDTH_DEFAULT;

   LIST_INITHEAD(&priv->free_tasks);
   LIST_INITHEAD(&priv->used_tasks);
   LIST_INITHEAD(&priv->b_frames);

   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_Destructor(OMX_COMPONENTTYPE *comp)
{
   vid_enc_PrivateType* priv = comp->pComponentPrivate;
   int i;

   enc_ReleaseTasks(&priv->free_tasks);
   enc_ReleaseTasks(&priv->used_tasks);
   enc_ReleaseTasks(&priv->b_frames);

   if (priv->ports) {
      for (i = 0; i < priv->sPortTypesParam[OMX_PortDomainVideo].nPorts; ++i) {
         if(priv->ports[i])
            priv->ports[i]->PortDestructor(priv->ports[i]);
      }
      FREE(priv->ports);
      priv->ports=NULL;
   }

   for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i)
      if (priv->scale_buffer[i])
         priv->scale_buffer[i]->destroy(priv->scale_buffer[i]);

   if (priv->s_pipe) {
      vl_compositor_cleanup_state(&priv->cstate);
      vl_compositor_cleanup(&priv->compositor);
      priv->s_pipe->destroy(priv->s_pipe);
   }

   if (priv->t_pipe)
      priv->t_pipe->destroy(priv->t_pipe);

   if (priv->screen)
      omx_put_screen();

   return omx_workaround_Destructor(comp);
}

static OMX_ERRORTYPE enc_AllocateBackTexture(omx_base_PortType *port,
                                             struct pipe_resource **resource,
                                             struct pipe_transfer **transfer,
                                             OMX_U8 **map)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct pipe_resource buf_templ;
   struct pipe_box box = {};
   OMX_U8 *ptr;

   memset(&buf_templ, 0, sizeof buf_templ);
   buf_templ.target = PIPE_TEXTURE_2D;
   buf_templ.format = PIPE_FORMAT_I8_UNORM;
   buf_templ.bind = PIPE_BIND_LINEAR;
   buf_templ.usage = PIPE_USAGE_STAGING;
   buf_templ.flags = 0;
   buf_templ.width0 = port->sPortParam.format.video.nFrameWidth;
   buf_templ.height0 = port->sPortParam.format.video.nFrameHeight * 3 / 2;
   buf_templ.depth0 = 1;
   buf_templ.array_size = 1;

   *resource = priv->s_pipe->screen->resource_create(priv->s_pipe->screen, &buf_templ);
   if (!*resource)
      return OMX_ErrorInsufficientResources;

   box.width = (*resource)->width0;
   box.height = (*resource)->height0;
   box.depth = (*resource)->depth0;
   ptr = priv->s_pipe->transfer_map(priv->s_pipe, *resource, 0, PIPE_TRANSFER_WRITE, &box, transfer);
   if (map)
      *map = ptr;

   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_SetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param)
{
   OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_ERRORTYPE r;

   if (!param)
      return OMX_ErrorBadParameter;

   switch(idx) {
   case OMX_IndexParamPortDefinition: {
      OMX_PARAM_PORTDEFINITIONTYPE *def = param;

      r = omx_base_component_SetParameter(handle, idx, param);
      if (r)
         return r;

      if (def->nPortIndex == OMX_BASE_FILTER_INPUTPORT_INDEX) {
         omx_base_video_PortType *port;
         unsigned framesize;
         struct pipe_resource *resource;
         struct pipe_transfer *transfer;

         port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];
         enc_AllocateBackTexture(priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX],
                                 &resource, &transfer, NULL);
         port->sPortParam.format.video.nStride = transfer->stride;
         pipe_transfer_unmap(priv->s_pipe, transfer);
         pipe_resource_reference(&resource, NULL);

         framesize = port->sPortParam.format.video.nStride *
                     port->sPortParam.format.video.nFrameHeight;
         port->sPortParam.format.video.nSliceHeight = port->sPortParam.format.video.nFrameHeight;
         port->sPortParam.nBufferSize = framesize * 3 / 2;

         port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX];
         port->sPortParam.nBufferSize = framesize * 512 / (16*16);
      
         priv->frame_rate = def->format.video.xFramerate;

         priv->callbacks->EventHandler(comp, priv->callbackData, OMX_EventPortSettingsChanged,
                                       OMX_BASE_FILTER_OUTPUTPORT_INDEX, 0, NULL);
      }
      break;
   }
   case OMX_IndexParamStandardComponentRole: {
      OMX_PARAM_COMPONENTROLETYPE *role = param;

      r = checkHeader(param, sizeof(OMX_PARAM_COMPONENTROLETYPE));
      if (r)
         return r;

      if (strcmp((char *)role->cRole, OMX_VID_ENC_AVC_ROLE)) {
         return OMX_ErrorBadParameter;
      }

      break;
   }
   case OMX_IndexParamVideoBitrate: {
      OMX_VIDEO_PARAM_BITRATETYPE *bitrate = param;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
      if (r)
         return r;

      priv->bitrate = *bitrate;

      break;
   }
   case OMX_IndexParamVideoQuantization: {
      OMX_VIDEO_PARAM_QUANTIZATIONTYPE *quant = param;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
      if (r)
         return r;

      priv->quant = *quant;

      break;
   }
   case OMX_IndexParamVideoProfileLevelCurrent: {
      OMX_VIDEO_PARAM_PROFILELEVELTYPE *profile_level = param;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
      if (r)
         return r;

      priv->profile_level = *profile_level;

      break;
   }
   default:
      return omx_base_component_SetParameter(handle, idx, param);
   }
   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_GetParameter(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR param)
{
   OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_ERRORTYPE r;

   if (!param)
      return OMX_ErrorBadParameter;

   switch(idx) {
   case OMX_IndexParamStandardComponentRole: {
      OMX_PARAM_COMPONENTROLETYPE *role = param;

      r = checkHeader(param, sizeof(OMX_PARAM_COMPONENTROLETYPE));
      if (r)
         return r;

      strcpy((char *)role->cRole, OMX_VID_ENC_AVC_ROLE);
      break;
   }
   case OMX_IndexParamVideoInit:
      r = checkHeader(param, sizeof(OMX_PORT_PARAM_TYPE));
      if (r)
         return r;

      memcpy(param, &priv->sPortTypesParam[OMX_PortDomainVideo], sizeof(OMX_PORT_PARAM_TYPE));
      break;

   case OMX_IndexParamVideoPortFormat: {
      OMX_VIDEO_PARAM_PORTFORMATTYPE *format = param;
      omx_base_video_PortType *port;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      if (r)
         return r;

      if (format->nPortIndex > 1)
         return OMX_ErrorBadPortIndex;

      port = (omx_base_video_PortType *)priv->ports[format->nPortIndex];
      memcpy(format, &port->sVideoParam, sizeof(OMX_VIDEO_PARAM_PORTFORMATTYPE));
      break;
   }
   case OMX_IndexParamVideoBitrate: {
      OMX_VIDEO_PARAM_BITRATETYPE *bitrate = param;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_BITRATETYPE));
      if (r)
         return r;

      bitrate->eControlRate = priv->bitrate.eControlRate;
      bitrate->nTargetBitrate = priv->bitrate.nTargetBitrate;

      break;
   }
   case OMX_IndexParamVideoQuantization: {
      OMX_VIDEO_PARAM_QUANTIZATIONTYPE *quant = param;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_QUANTIZATIONTYPE));
      if (r)
         return r;

      quant->nQpI = priv->quant.nQpI;
      quant->nQpP = priv->quant.nQpP;
      quant->nQpB = priv->quant.nQpB;

      break;
   }
   case OMX_IndexParamVideoProfileLevelCurrent: {
      OMX_VIDEO_PARAM_PROFILELEVELTYPE *profile_level = param;

      r = checkHeader(param, sizeof(OMX_VIDEO_PARAM_PROFILELEVELTYPE));
      if (r)
         return r;

      profile_level->eProfile = priv->profile_level.eProfile;
      profile_level->eLevel = priv->profile_level.eLevel;

      break;
   }
   default:
      return omx_base_component_GetParameter(handle, idx, param);
   }
   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_SetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config)
{
   OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_ERRORTYPE r;
   int i;
 
   if (!config)
      return OMX_ErrorBadParameter;
                         
   switch(idx) {
   case OMX_IndexConfigVideoIntraVOPRefresh: {
      OMX_CONFIG_INTRAREFRESHVOPTYPE *type = config;

      r = checkHeader(config, sizeof(OMX_CONFIG_INTRAREFRESHVOPTYPE));
      if (r)
         return r;
      
      priv->force_pic_type = *type;
      
      break;
   }
   case OMX_IndexConfigCommonScale: {
      OMX_CONFIG_SCALEFACTORTYPE *scale = config;

      r = checkHeader(config, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
      if (r)
         return r;

      if (scale->xWidth < 176 || scale->xHeight < 144)
         return OMX_ErrorBadParameter;

      for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i) {
         if (priv->scale_buffer[i]) {
            priv->scale_buffer[i]->destroy(priv->scale_buffer[i]);
            priv->scale_buffer[i] = NULL;
         }
      }

      priv->scale = *scale;
      if (priv->scale.xWidth != 0xffffffff && priv->scale.xHeight != 0xffffffff) {
         struct pipe_video_buffer templat = {};
 
         templat.buffer_format = PIPE_FORMAT_NV12;
         templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
         templat.width = priv->scale.xWidth; 
         templat.height = priv->scale.xHeight; 
         templat.interlaced = false;
         for (i = 0; i < OMX_VID_ENC_NUM_SCALING_BUFFERS; ++i) {
            priv->scale_buffer[i] = priv->s_pipe->create_video_buffer(priv->s_pipe, &templat);
            if (!priv->scale_buffer[i])
               return OMX_ErrorInsufficientResources;
         }
      }

      break;
   }
   default:
      return omx_base_component_SetConfig(handle, idx, config);
   }

   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_GetConfig(OMX_HANDLETYPE handle, OMX_INDEXTYPE idx, OMX_PTR config)
{
   OMX_COMPONENTTYPE *comp = handle;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_ERRORTYPE r;

   if (!config)
      return OMX_ErrorBadParameter;

   switch(idx) {
   case OMX_IndexConfigCommonScale: {
      OMX_CONFIG_SCALEFACTORTYPE *scale = config;

      r = checkHeader(config, sizeof(OMX_CONFIG_SCALEFACTORTYPE));
      if (r)
         return r;

      scale->xWidth = priv->scale.xWidth;
      scale->xHeight = priv->scale.xHeight;

      break;
   }
   default:
      return omx_base_component_GetConfig(handle, idx, config);
   }
   
   return OMX_ErrorNone;
}

static enum pipe_video_profile enc_TranslateOMXProfileToPipe(unsigned omx_profile)
{
   switch (omx_profile) {
   case OMX_VIDEO_AVCProfileBaseline:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE;
   case OMX_VIDEO_AVCProfileMain:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_MAIN;
   case OMX_VIDEO_AVCProfileExtended:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_EXTENDED;
   case OMX_VIDEO_AVCProfileHigh:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH;
   case OMX_VIDEO_AVCProfileHigh10:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH10;
   case OMX_VIDEO_AVCProfileHigh422:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH422;
   case OMX_VIDEO_AVCProfileHigh444:
      return PIPE_VIDEO_PROFILE_MPEG4_AVC_HIGH444;
   default:
      return PIPE_VIDEO_PROFILE_UNKNOWN;
   }
}

static unsigned enc_TranslateOMXLevelToPipe(unsigned omx_level)
{
   switch (omx_level) {
   case OMX_VIDEO_AVCLevel1:
   case OMX_VIDEO_AVCLevel1b:
      return 10;
   case OMX_VIDEO_AVCLevel11:
      return 11;
   case OMX_VIDEO_AVCLevel12:
      return 12;
   case OMX_VIDEO_AVCLevel13:
      return 13;
   case OMX_VIDEO_AVCLevel2:
      return 20;
   case OMX_VIDEO_AVCLevel21:
      return 21;
   case OMX_VIDEO_AVCLevel22:
      return 22;
   case OMX_VIDEO_AVCLevel3:
      return 30;
   case OMX_VIDEO_AVCLevel31:
      return 31;
   case OMX_VIDEO_AVCLevel32:
      return 32;
   case OMX_VIDEO_AVCLevel4:
      return 40;
   case OMX_VIDEO_AVCLevel41:
      return 41;
   default:
   case OMX_VIDEO_AVCLevel42:
      return 42;
   case OMX_VIDEO_AVCLevel5:
      return 50;
   case OMX_VIDEO_AVCLevel51:
      return 51;
   }
}

static OMX_ERRORTYPE vid_enc_MessageHandler(OMX_COMPONENTTYPE* comp, internalRequestMessageType *msg)
{
   vid_enc_PrivateType* priv = comp->pComponentPrivate;

   if (msg->messageType == OMX_CommandStateSet) {
      if ((msg->messageParam == OMX_StateIdle ) && (priv->state == OMX_StateLoaded)) {

         struct pipe_video_codec templat = {};
         omx_base_video_PortType *port;

         port = (omx_base_video_PortType *)priv->ports[OMX_BASE_FILTER_INPUTPORT_INDEX];

         templat.profile = enc_TranslateOMXProfileToPipe(priv->profile_level.eProfile);
         templat.level = enc_TranslateOMXLevelToPipe(priv->profile_level.eLevel);
         templat.entrypoint = PIPE_VIDEO_ENTRYPOINT_ENCODE;
         templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
         templat.width = priv->scale_buffer[priv->current_scale_buffer] ?
                            priv->scale.xWidth : port->sPortParam.format.video.nFrameWidth;
         templat.height = priv->scale_buffer[priv->current_scale_buffer] ?
                            priv->scale.xHeight : port->sPortParam.format.video.nFrameHeight;
         templat.max_references = (templat.profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE) ?
                            1 : OMX_VID_ENC_P_PERIOD_DEFAULT;

         priv->codec = priv->s_pipe->create_video_codec(priv->s_pipe, &templat);

      } else if ((msg->messageParam == OMX_StateLoaded) && (priv->state == OMX_StateIdle)) {
         if (priv->codec) {
            priv->codec->destroy(priv->codec);
            priv->codec = NULL;
         }
      }
   }

   return omx_base_component_MessageHandler(comp, msg);
}

static OMX_ERRORTYPE vid_enc_AllocateInBuffer(omx_base_PortType *port, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
                                              OMX_IN OMX_U32 idx, OMX_IN OMX_PTR private, OMX_IN OMX_U32 size)
{
   struct input_buf_private *inp;
   OMX_ERRORTYPE r;

   r = base_port_AllocateBuffer(port, buf, idx, private, size);
   if (r)
      return r;

   inp = (*buf)->pInputPortPrivate = CALLOC_STRUCT(input_buf_private);
   if (!inp) {
      base_port_FreeBuffer(port, idx, *buf);
      return OMX_ErrorInsufficientResources;
   }

   LIST_INITHEAD(&inp->tasks);

   FREE((*buf)->pBuffer);
   r = enc_AllocateBackTexture(port, &inp->resource, &inp->transfer, &(*buf)->pBuffer);
   if (r) {
      FREE(inp);
      base_port_FreeBuffer(port, idx, *buf);
      return r;
   }

   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_UseInBuffer(omx_base_PortType *port, OMX_BUFFERHEADERTYPE **buf, OMX_U32 idx,
                                         OMX_PTR private, OMX_U32 size, OMX_U8 *mem)
{
   struct input_buf_private *inp;
   OMX_ERRORTYPE r;

   r = base_port_UseBuffer(port, buf, idx, private, size, mem);
   if (r)
      return r;

   inp = (*buf)->pInputPortPrivate = CALLOC_STRUCT(input_buf_private);
   if (!inp) {
      base_port_FreeBuffer(port, idx, *buf);
      return OMX_ErrorInsufficientResources;
   }

   LIST_INITHEAD(&inp->tasks);

   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_FreeInBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct input_buf_private *inp = buf->pInputPortPrivate;

   if (inp) {
      enc_ReleaseTasks(&inp->tasks);
      if (inp->transfer)
         pipe_transfer_unmap(priv->s_pipe, inp->transfer);
      pipe_resource_reference(&inp->resource, NULL);
      FREE(inp);
   }
   buf->pBuffer = NULL;

   return base_port_FreeBuffer(port, idx, buf);
}

static OMX_ERRORTYPE vid_enc_AllocateOutBuffer(omx_base_PortType *port, OMX_INOUT OMX_BUFFERHEADERTYPE **buf,
                                               OMX_IN OMX_U32 idx, OMX_IN OMX_PTR private, OMX_IN OMX_U32 size)
{
   OMX_ERRORTYPE r;

   r = base_port_AllocateBuffer(port, buf, idx, private, size);
   if (r)
      return r;

   FREE((*buf)->pBuffer);
   (*buf)->pBuffer = NULL;
   (*buf)->pOutputPortPrivate = CALLOC(1, sizeof(struct output_buf_private));
   if (!(*buf)->pOutputPortPrivate) {
      base_port_FreeBuffer(port, idx, *buf);
      return OMX_ErrorInsufficientResources;
   }

   return OMX_ErrorNone;
}

static OMX_ERRORTYPE vid_enc_FreeOutBuffer(omx_base_PortType *port, OMX_U32 idx, OMX_BUFFERHEADERTYPE *buf)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;

   if (buf->pOutputPortPrivate) {
      struct output_buf_private *outp = buf->pOutputPortPrivate;
      if (outp->transfer)
         pipe_transfer_unmap(priv->t_pipe, outp->transfer);
      pipe_resource_reference(&outp->bitstream, NULL);
      FREE(outp);
      buf->pOutputPortPrivate = NULL;
   }
   buf->pBuffer = NULL;

   return base_port_FreeBuffer(port, idx, buf);
}

static struct encode_task *enc_NeedTask(omx_base_PortType *port)
{
   OMX_VIDEO_PORTDEFINITIONTYPE *def = &port->sPortParam.format.video;
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;

   struct pipe_video_buffer templat = {};
   struct encode_task *task;

   if (!LIST_IS_EMPTY(&priv->free_tasks)) {
      task = LIST_ENTRY(struct encode_task, priv->free_tasks.next, list);
      LIST_DEL(&task->list);
      return task;
   }

   /* allocate a new one */
   task = CALLOC_STRUCT(encode_task);
   if (!task)
      return NULL;

   templat.buffer_format = PIPE_FORMAT_NV12;
   templat.chroma_format = PIPE_VIDEO_CHROMA_FORMAT_420;
   templat.width = def->nFrameWidth;
   templat.height = def->nFrameHeight;
   templat.interlaced = false;

   task->buf = priv->s_pipe->create_video_buffer(priv->s_pipe, &templat);
   if (!task->buf) {
      FREE(task);
      return NULL;
   }

   return task;
}

static void enc_MoveTasks(struct list_head *from, struct list_head *to)
{
   to->prev->next = from->next;
   from->next->prev = to->prev;
   from->prev->next = to;
   to->prev = from->prev;
   LIST_INITHEAD(from);
}

static void enc_ReleaseTasks(struct list_head *head)
{
   struct encode_task *i, *next;

   LIST_FOR_EACH_ENTRY_SAFE(i, next, head, list) {
      pipe_resource_reference(&i->bitstream, NULL);
      i->buf->destroy(i->buf);
      FREE(i);
   }
}

static OMX_ERRORTYPE enc_LoadImage(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf,
                                   struct pipe_video_buffer *vbuf)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_VIDEO_PORTDEFINITIONTYPE *def = &port->sPortParam.format.video;
   struct pipe_box box = {};
   struct input_buf_private *inp = buf->pInputPortPrivate;

   if (!inp->resource) {
      struct pipe_sampler_view **views;
      void *ptr;

      views = vbuf->get_sampler_view_planes(vbuf);
      if (!views)
         return OMX_ErrorInsufficientResources;

      ptr = buf->pBuffer;
      box.width = def->nFrameWidth;
      box.height = def->nFrameHeight;
      box.depth = 1;
      priv->s_pipe->transfer_inline_write(priv->s_pipe, views[0]->texture, 0,
                                          PIPE_TRANSFER_WRITE, &box,
                                          ptr, def->nStride, 0);
      ptr = ((uint8_t*)buf->pBuffer) + (def->nStride * box.height);
      box.width = def->nFrameWidth / 2;
      box.height = def->nFrameHeight / 2;
      box.depth = 1;
      priv->s_pipe->transfer_inline_write(priv->s_pipe, views[1]->texture, 0,
                                          PIPE_TRANSFER_WRITE, &box,
                                          ptr, def->nStride, 0);
   } else {
      struct pipe_blit_info blit;
      struct vl_video_buffer *dst_buf = (struct vl_video_buffer *)vbuf;

      pipe_transfer_unmap(priv->s_pipe, inp->transfer);

      box.width = def->nFrameWidth;
      box.height = def->nFrameHeight;
      box.depth = 1;

      priv->s_pipe->resource_copy_region(priv->s_pipe,
                                         dst_buf->resources[0],
                                         0, 0, 0, 0, inp->resource, 0, &box);

      memset(&blit, 0, sizeof(blit));
      blit.src.resource = inp->resource;
      blit.src.format = inp->resource->format;

      blit.src.box.x = 0;
      blit.src.box.y = def->nFrameHeight;
      blit.src.box.width = def->nFrameWidth;
      blit.src.box.height = def->nFrameHeight / 2 ;
      blit.src.box.depth = 1;

      blit.dst.resource = dst_buf->resources[1];
      blit.dst.format = blit.dst.resource->format;

      blit.dst.box.width = def->nFrameWidth / 2;
      blit.dst.box.height = def->nFrameHeight / 2;
      blit.dst.box.depth = 1;
      blit.filter = PIPE_TEX_FILTER_NEAREST;

      blit.mask = PIPE_MASK_G;
      priv->s_pipe->blit(priv->s_pipe, &blit);

      blit.src.box.x = 1;
      blit.mask = PIPE_MASK_R;
      priv->s_pipe->blit(priv->s_pipe, &blit);
      priv->s_pipe->flush(priv->s_pipe, NULL, 0);

      box.width = inp->resource->width0;
      box.height = inp->resource->height0;
      box.depth = inp->resource->depth0;
      buf->pBuffer = priv->s_pipe->transfer_map(priv->s_pipe, inp->resource, 0,
                                                PIPE_TRANSFER_WRITE, &box,
                                                &inp->transfer);
   }

   return OMX_ErrorNone;
}

static void enc_ScaleInput(omx_base_PortType *port, struct pipe_video_buffer **vbuf, unsigned *size)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   OMX_VIDEO_PORTDEFINITIONTYPE *def = &port->sPortParam.format.video;
   struct pipe_video_buffer *src_buf = *vbuf;
   struct vl_compositor *compositor = &priv->compositor;
   struct vl_compositor_state *s = &priv->cstate;
   struct pipe_sampler_view **views;
   struct pipe_surface **dst_surface;
   unsigned i;

   if (!priv->scale_buffer[priv->current_scale_buffer])
      return;

   views = src_buf->get_sampler_view_planes(src_buf);
   dst_surface = priv->scale_buffer[priv->current_scale_buffer]->get_surfaces
                 (priv->scale_buffer[priv->current_scale_buffer]);
   vl_compositor_clear_layers(s);

   for (i = 0; i < VL_MAX_SURFACES; ++i) {
      struct u_rect src_rect;
      if (!views[i] || !dst_surface[i])
         continue;
      src_rect.x0 = 0;
      src_rect.y0 = 0;
      src_rect.x1 = def->nFrameWidth;
      src_rect.y1 = def->nFrameHeight;
      if (i > 0) {
         src_rect.x1 /= 2;
         src_rect.y1 /= 2;
      }
      vl_compositor_set_rgba_layer(s, compositor, 0, views[i], &src_rect, NULL, NULL);
      vl_compositor_render(s, compositor, dst_surface[i], NULL, false);
   }
   *size  = priv->scale.xWidth * priv->scale.xHeight * 2;
   *vbuf = priv->scale_buffer[priv->current_scale_buffer++];
   priv->current_scale_buffer %= OMX_VID_ENC_NUM_SCALING_BUFFERS;
}

static void enc_ControlPicture(omx_base_PortType *port, struct pipe_h264_enc_picture_desc *picture)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct pipe_h264_enc_rate_control *rate_ctrl = &picture->rate_ctrl;

   switch (priv->bitrate.eControlRate) {
   case OMX_Video_ControlRateVariable:
      rate_ctrl->rate_ctrl_method = PIPE_H264_ENC_RATE_CONTROL_METHOD_VARIABLE;
      break; 
   case OMX_Video_ControlRateConstant:
      rate_ctrl->rate_ctrl_method = PIPE_H264_ENC_RATE_CONTROL_METHOD_CONSTANT;
      break; 
   case OMX_Video_ControlRateVariableSkipFrames:
      rate_ctrl->rate_ctrl_method = PIPE_H264_ENC_RATE_CONTROL_METHOD_VARIABLE_SKIP;
      break;
   case OMX_Video_ControlRateConstantSkipFrames:
      rate_ctrl->rate_ctrl_method = PIPE_H264_ENC_RATE_CONTROL_METHOD_CONSTANT_SKIP;
      break;
   default:
      rate_ctrl->rate_ctrl_method = PIPE_H264_ENC_RATE_CONTROL_METHOD_DISABLE;
      break;
   } 
      
   if (rate_ctrl->rate_ctrl_method != PIPE_H264_ENC_RATE_CONTROL_METHOD_DISABLE) {
      if (priv->bitrate.nTargetBitrate < OMX_VID_ENC_BITRATE_MIN)
         rate_ctrl->target_bitrate = OMX_VID_ENC_BITRATE_MIN;
      else if (priv->bitrate.nTargetBitrate < OMX_VID_ENC_BITRATE_MAX)
         rate_ctrl->target_bitrate = priv->bitrate.nTargetBitrate;
      else
         rate_ctrl->target_bitrate = OMX_VID_ENC_BITRATE_MAX;
      rate_ctrl->peak_bitrate = rate_ctrl->target_bitrate;    
      rate_ctrl->frame_rate_den = OMX_VID_ENC_CONTROL_FRAME_RATE_DEN_DEFAULT;
      rate_ctrl->frame_rate_num = ((priv->frame_rate) >> 16) * rate_ctrl->frame_rate_den;
      if (rate_ctrl->target_bitrate < OMX_VID_ENC_BITRATE_MEDIAN)
         rate_ctrl->vbv_buffer_size = MIN2((rate_ctrl->target_bitrate * 2.75), OMX_VID_ENC_BITRATE_MEDIAN);
      else
         rate_ctrl->vbv_buffer_size = rate_ctrl->target_bitrate;

      if (rate_ctrl->frame_rate_num) {
         unsigned long long t = rate_ctrl->target_bitrate;
         t *= rate_ctrl->frame_rate_den;
         rate_ctrl->target_bits_picture = t / rate_ctrl->frame_rate_num;
      } else {
         rate_ctrl->target_bits_picture = rate_ctrl->target_bitrate;
      }
      rate_ctrl->peak_bits_picture_integer = rate_ctrl->target_bits_picture;
      rate_ctrl->peak_bits_picture_fraction = 0;
   } else
      memset(rate_ctrl, 0, sizeof(struct pipe_h264_enc_rate_control));
   
   picture->quant_i_frames = priv->quant.nQpI;
   picture->quant_p_frames = priv->quant.nQpP;
   picture->quant_b_frames = priv->quant.nQpB;

   picture->frame_num = priv->frame_num;
   picture->ref_idx_l0 = priv->ref_idx_l0;
   picture->ref_idx_l1 = priv->ref_idx_l1;
}

static void enc_HandleTask(omx_base_PortType *port, struct encode_task *task,
                           enum pipe_h264_enc_picture_type picture_type)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   unsigned size = priv->ports[OMX_BASE_FILTER_OUTPUTPORT_INDEX]->sPortParam.nBufferSize;
   struct pipe_video_buffer *vbuf = task->buf;
   struct pipe_h264_enc_picture_desc picture = {};
 
   /* -------------- scale input image --------- */
   enc_ScaleInput(port, &vbuf, &size);
   priv->s_pipe->flush(priv->s_pipe, NULL, 0);

   /* -------------- allocate output buffer --------- */
   task->bitstream = pipe_buffer_create(priv->s_pipe->screen, PIPE_BIND_VERTEX_BUFFER,
                                        PIPE_USAGE_STREAM, size);

   picture.picture_type = picture_type;
   picture.pic_order_cnt = task->pic_order_cnt;
   if (priv->restricted_b_frames && picture_type == PIPE_H264_ENC_PICTURE_TYPE_B)
      picture.not_referenced = true;
   enc_ControlPicture(port, &picture);

   /* -------------- encode frame --------- */
   priv->codec->begin_frame(priv->codec, vbuf, &picture.base);
   priv->codec->encode_bitstream(priv->codec, vbuf, task->bitstream, &task->feedback);
   priv->codec->end_frame(priv->codec, vbuf, &picture.base);
}

static void enc_ClearBframes(omx_base_PortType *port, struct input_buf_private *inp)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct encode_task *task;

   if (LIST_IS_EMPTY(&priv->b_frames))
      return;

   task = LIST_ENTRY(struct encode_task, priv->b_frames.prev, list);
   LIST_DEL(&task->list);

   /* promote last from to P frame */
   priv->ref_idx_l0 = priv->ref_idx_l1;
   enc_HandleTask(port, task, PIPE_H264_ENC_PICTURE_TYPE_P);
   LIST_ADDTAIL(&task->list, &inp->tasks);
   priv->ref_idx_l1 = priv->frame_num++;

   /* handle B frames */
   LIST_FOR_EACH_ENTRY(task, &priv->b_frames, list) {
      enc_HandleTask(port, task, PIPE_H264_ENC_PICTURE_TYPE_B);
      if (!priv->restricted_b_frames)
         priv->ref_idx_l0 = priv->frame_num;
      priv->frame_num++;
   }

   enc_MoveTasks(&priv->b_frames, &inp->tasks);
}

static OMX_ERRORTYPE vid_enc_EncodeFrame(omx_base_PortType *port, OMX_BUFFERHEADERTYPE *buf)
{
   OMX_COMPONENTTYPE* comp = port->standCompContainer;
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct input_buf_private *inp = buf->pInputPortPrivate;
   enum pipe_h264_enc_picture_type picture_type;
   struct encode_task *task;
   OMX_ERRORTYPE err;

   enc_MoveTasks(&inp->tasks, &priv->free_tasks);
   task = enc_NeedTask(port);
   if (!task)
      return OMX_ErrorInsufficientResources;

   if (buf->nFilledLen == 0) {
      if (buf->nFlags & OMX_BUFFERFLAG_EOS) {
         buf->nFilledLen = buf->nAllocLen;
         enc_ClearBframes(port, inp);
      }
      return base_port_SendBufferFunction(port, buf);
   }

   if (buf->pOutputPortPrivate) {
      struct pipe_video_buffer *vbuf = buf->pOutputPortPrivate;
      buf->pOutputPortPrivate = task->buf;
      task->buf = vbuf;
   } else {
      /* ------- load input image into video buffer ---- */
      err = enc_LoadImage(port, buf, task->buf);
      if (err != OMX_ErrorNone)
         return err;
   }

   /* -------------- determine picture type --------- */
   if (!(priv->pic_order_cnt % OMX_VID_ENC_IDR_PERIOD_DEFAULT) ||
       priv->force_pic_type.IntraRefreshVOP) {
      enc_ClearBframes(port, inp);
      picture_type = PIPE_H264_ENC_PICTURE_TYPE_IDR;
      priv->force_pic_type.IntraRefreshVOP = OMX_FALSE; 
      priv->frame_num = 0;
   } else if (priv->codec->profile == PIPE_VIDEO_PROFILE_MPEG4_AVC_BASELINE ||
              !(priv->pic_order_cnt % OMX_VID_ENC_P_PERIOD_DEFAULT) ||
              (buf->nFlags & OMX_BUFFERFLAG_EOS)) {
      picture_type = PIPE_H264_ENC_PICTURE_TYPE_P;
   } else {
      picture_type = PIPE_H264_ENC_PICTURE_TYPE_B;
   }
   
   task->pic_order_cnt = priv->pic_order_cnt++;

   if (picture_type == PIPE_H264_ENC_PICTURE_TYPE_B) {
      /* put frame at the tail of the queue */
      LIST_ADDTAIL(&task->list, &priv->b_frames);
   } else {
      /* handle I or P frame */
      priv->ref_idx_l0 = priv->ref_idx_l1;
      enc_HandleTask(port, task, picture_type);
      LIST_ADDTAIL(&task->list, &inp->tasks);
      priv->ref_idx_l1 = priv->frame_num++;

      /* handle B frames */
      LIST_FOR_EACH_ENTRY(task, &priv->b_frames, list) {
         enc_HandleTask(port, task, PIPE_H264_ENC_PICTURE_TYPE_B);
         if (!priv->restricted_b_frames)
            priv->ref_idx_l0 = priv->frame_num;
         priv->frame_num++;
      }

      enc_MoveTasks(&priv->b_frames, &inp->tasks);
   }

   if (LIST_IS_EMPTY(&inp->tasks))
      return port->ReturnBufferFunction(port, buf);
   else
      return base_port_SendBufferFunction(port, buf);
}

static void vid_enc_BufferEncoded(OMX_COMPONENTTYPE *comp, OMX_BUFFERHEADERTYPE* input, OMX_BUFFERHEADERTYPE* output)
{
   vid_enc_PrivateType *priv = comp->pComponentPrivate;
   struct output_buf_private *outp = output->pOutputPortPrivate;
   struct input_buf_private *inp = input->pInputPortPrivate;
   struct encode_task *task;
   struct pipe_box box = {};
   unsigned size;

   if (!inp || LIST_IS_EMPTY(&inp->tasks)) {
      input->nFilledLen = 0; /* mark buffer as empty */
      enc_MoveTasks(&priv->used_tasks, &inp->tasks);
      return;
   }

   task = LIST_ENTRY(struct encode_task, inp->tasks.next, list);
   LIST_DEL(&task->list);
   LIST_ADDTAIL(&task->list, &priv->used_tasks);

   if (!task->bitstream)
      return;

   /* ------------- map result buffer ----------------- */

   if (outp->transfer)
      pipe_transfer_unmap(priv->t_pipe, outp->transfer);

   pipe_resource_reference(&outp->bitstream, task->bitstream);
   pipe_resource_reference(&task->bitstream, NULL);

   box.width = outp->bitstream->width0;
   box.height = outp->bitstream->height0;
   box.depth = outp->bitstream->depth0;

   output->pBuffer = priv->t_pipe->transfer_map(priv->t_pipe, outp->bitstream, 0,
                                                PIPE_TRANSFER_READ_WRITE,
                                                &box, &outp->transfer);
 
   /* ------------- get size of result ----------------- */

   priv->codec->get_feedback(priv->codec, task->feedback, &size);

   output->nOffset = 0;
   output->nFilledLen = size; /* mark buffer as full */
}
