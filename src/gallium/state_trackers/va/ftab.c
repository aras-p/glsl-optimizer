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

#include <assert.h>
#include <VA/va_backend.h>

const struct VADriverVTable vtable =
{
   0, /* VAStatus (*vaTerminate) ( VADriverContextP ctx ); */
   0, /* VAStatus (*vaQueryConfigProfiles) ( VADriverContextP ctx, VAProfile *profile_list,int *num_profiles); */
   0, /* VAStatus (*vaQueryConfigEntrypoints) ( VADriverContextP ctx,	VAProfile profile, VAEntrypoint  *entrypoint_list, int *num_entrypoints	); */
   0, /* VAStatus (*vaGetConfigAttributes) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs ); */
   0, /* VAStatus (*vaCreateConfig) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,	VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id); */
   0, /* VAStatus (*vaDestroyConfig) ( VADriverContextP ctx, VAConfigID config_id); */
   0, /* VAStatus (*vaQueryConfigAttributes) ( VADriverContextP ctx, VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list, int *num_attribs); */
   0, /* VAStatus (*vaCreateConfig) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,	VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id); */
   0, /* VAStatus (*vaDestroyConfig) ( VADriverContextP ctx, VAConfigID config_id ); */
   0, /* VAStatus (*vaQueryConfigAttributes) (VADriverContextP ctx,VAConfigID config_id,VAProfile *profile,VAEntrypoint *entrypoint,VAConfigAttrib *attrib_list,int *num_attribs); */
   0, /* VAStatus (*vaCreateSurfaces) ( VADriverContextP ctx,int width,int height,int format,int num_surfaces,VASurfaceID *surfaces); */
   0, /* VAStatus (*vaDestroySurfaces) ( VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces ); */
   0, /* VAStatus (*vaCreateContext) (VADriverContextP ctx,VAConfigID config_id,int picture_width,int picture_height,int flag,VASurfaceID *render_targets,int num_render_targets,VAContextID *context); */
   0, /* VAStatus (*vaDestroyContext) (VADriverContextP ctx,VAContextID context); */
   0, /* VAStatus (*vaCreateBuffer) (VADriverContextP ctx,VAContextID context,VABufferType type,unsigned int size,unsigned int num_elements,void *data,VABufferID *buf_id); */
   0, /* VAStatus (*vaBufferSetNumElements) (VADriverContextP ctx,VABufferID buf_id,unsigned int num_elements); */
   0, /* VAStatus (*vaMapBuffer) (VADriverContextP ctx,VABufferID buf_id,void **pbuf); */
   0, /* VAStatus (*vaUnmapBuffer) (VADriverContextP ctx,VABufferID buf_id); */
   0, /* VAStatus (*vaDestroyBuffer) (VADriverContextP ctx,VABufferID buffer_id); */
   0, /* VAStatus (*vaBeginPicture) (VADriverContextP ctx,VAContextID context,VASurfaceID render_target); */
   0, /* VAStatus (*vaRenderPicture) (VADriverContextP ctx,VAContextID context,VABufferID *buffers,int num_buffers); */
   0, /* VAStatus (*vaEndPicture) (VADriverContextP ctx,VAContextID context); */
   0, /* VAStatus (*vaSyncSurface) (VADriverContextP ctx,VASurfaceID render_target); */
   0, /* VAStatus (*vaQuerySurfaceStatus) (VADriverContextP ctx,VASurfaceID render_target,VASurfaceStatus *status); */
   0, /* VAStatus (*vaPutSurface) (
    		VADriverContextP ctx,
		VASurfaceID surface,
		void* draw,
		short srcx,
		short srcy,
		unsigned short srcw,
		unsigned short srch,
		short destx,
		short desty,
		unsigned short destw,
		unsigned short desth,
		VARectangle *cliprects, 
		unsigned int number_cliprects, 
		unsigned int flags); */
   0, /* VAStatus (*vaQueryImageFormats) ( VADriverContextP ctx, VAImageFormat *format_list,int *num_formats); */
   0, /* VAStatus (*vaCreateImage) (VADriverContextP ctx,VAImageFormat *format,int width,int height,VAImage *image); */
   0, /* VAStatus (*vaDeriveImage) (VADriverContextP ctx,VASurfaceID surface,VAImage *image); */
   0, /* VAStatus (*vaDestroyImage) (VADriverContextP ctx,VAImageID image); */
   0, /* VAStatus (*vaSetImagePalette) (VADriverContextP ctx,VAImageID image, unsigned char *palette); */
   0, /* VAStatus (*vaGetImage) (VADriverContextP ctx,VASurfaceID surface,int x,int y,unsigned int width,unsigned int height,VAImageID image); */
   0, /* VAStatus (*vaPutImage) (
		VADriverContextP ctx,
		VASurfaceID surface,
		VAImageID image,
		int src_x,
		int src_y,
		unsigned int src_width,
		unsigned int src_height,
		int dest_x,
		int dest_y,
		unsigned int dest_width,
		unsigned int dest_height
	); */
   0,	/* VAStatus (*vaQuerySubpictureFormats) (VADriverContextP ctx,VAImageFormat *format_list,unsigned int *flags,unsigned int *num_formats); */
   0, /* VAStatus (*vaCreateSubpicture) (VADriverContextP ctx,VAImageID image,VASubpictureID *subpicture); */
   0, /* VAStatus (*vaDestroySubpicture) (VADriverContextP ctx,VASubpictureID subpicture); */
   0, /* VAStatus (*vaSetSubpictureImage) (VADriverContextP ctx,VASubpictureID subpicture,VAImageID image); */
   0, /* VAStatus (*vaSetSubpictureChromakey) (VADriverContextP ctx,VASubpictureID subpicture,unsigned int chromakey_min,unsigned int chromakey_max,unsigned int chromakey_mask); */
   0, /* VAStatus (*vaSetSubpictureGlobalAlpha) (VADriverContextP ctx,VASubpictureID subpicture,float global_alpha); */
   0, /* VAStatus (*vaAssociateSubpicture) (
		VADriverContextP ctx,
		VASubpictureID subpicture,
		VASurfaceID *target_surfaces,
		int num_surfaces,
		short src_x,
		short src_y,
		unsigned short src_width,
		unsigned short src_height,
		short dest_x,
		short dest_y,
		unsigned short dest_width,
		unsigned short dest_height,
		unsigned int flags); */
   0, /* VAStatus (*vaDeassociateSubpicture) (VADriverContextP ctx,VASubpictureID subpicture,VASurfaceID *target_surfaces,int num_surfaces); */
   0, /* VAStatus (*vaQueryDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int *num_attributes); */
   0, /* VAStatus (*vaGetDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes); */
   0, /* VAStatus (*vaSetDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes); */
   0, /* VAStatus (*vaBufferInfo) (VADriverContextP ctx,VAContextID context,VABufferID buf_id,VABufferType *type,unsigned int *size,unsigned int *num_elements); */
   0, /* VAStatus (*vaLockSurface) (
		VADriverContextP ctx,
                VASurfaceID surface,
                unsigned int *fourcc,
                unsigned int *luma_stride,
                unsigned int *chroma_u_stride,
                unsigned int *chroma_v_stride,
                unsigned int *luma_offset,
                unsigned int *chroma_u_offset,
                unsigned int *chroma_v_offset,
                unsigned int *buffer_name,
                void **buffer); */
   0, /* VAStatus (*vaUnlockSurface) (VADriverContextP ctx,VASurfaceID surface); */
   0 /* struct VADriverVTableGLX *glx; "Optional" */
};

