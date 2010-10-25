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
#include <va/va.h>
#include <va/va_backend.h>
#include "va_private.h"

static struct VADriverVTable vtable =
{
   0x1, /* VAStatus (*vaTerminate) ( VADriverContextP ctx ); */
   0x2, /* VAStatus (*vaQueryConfigProfiles) ( VADriverContextP ctx, VAProfile *profile_list,int *num_profiles); */
   0x3, /* VAStatus (*vaQueryConfigEntrypoints) ( VADriverContextP ctx,	VAProfile profile, VAEntrypoint  *entrypoint_list, int *num_entrypoints	); */
   0x4, /* VAStatus (*vaGetConfigAttributes) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint, VAConfigAttrib *attrib_list, int num_attribs ); */
   0x5, /* VAStatus (*vaCreateConfig) ( VADriverContextP ctx, VAProfile profile, VAEntrypoint entrypoint,	VAConfigAttrib *attrib_list, int num_attribs, VAConfigID *config_id); */
   0x6, /* VAStatus (*vaDestroyConfig) ( VADriverContextP ctx, VAConfigID config_id); */
   0x7, /* VAStatus (*vaQueryConfigAttributes) ( VADriverContextP ctx, VAConfigID config_id, VAProfile *profile, VAEntrypoint *entrypoint, VAConfigAttrib *attrib_list, int *num_attribs); */
   0x8, /* VAStatus (*vaCreateSurfaces) ( VADriverContextP ctx,int width,int height,int format,int num_surfaces,VASurfaceID *surfaces); */
   0x9, /* VAStatus (*vaDestroySurfaces) ( VADriverContextP ctx, VASurfaceID *surface_list, int num_surfaces ); */
   0x10, /* VAStatus (*vaCreateContext) (VADriverContextP ctx,VAConfigID config_id,int picture_width,int picture_height,int flag,VASurfaceID *render_targets,int num_render_targets,VAContextID *context); */
   0x11, /* VAStatus (*vaDestroyContext) (VADriverContextP ctx,VAContextID context); */
   0x12, /* VAStatus (*vaCreateBuffer) (VADriverContextP ctx,VAContextID context,VABufferType type,unsigned int size,unsigned int num_elements,void *data,VABufferID *buf_id); */
   0x13, /* VAStatus (*vaBufferSetNumElements) (VADriverContextP ctx,VABufferID buf_id,unsigned int num_elements); */
   0x14, /* VAStatus (*vaMapBuffer) (VADriverContextP ctx,VABufferID buf_id,void **pbuf); */
   0x15, /* VAStatus (*vaUnmapBuffer) (VADriverContextP ctx,VABufferID buf_id); */
   0x16, /* VAStatus (*vaDestroyBuffer) (VADriverContextP ctx,VABufferID buffer_id); */
   0x17, /* VAStatus (*vaBeginPicture) (VADriverContextP ctx,VAContextID context,VASurfaceID render_target); */
   0x18, /* VAStatus (*vaRenderPicture) (VADriverContextP ctx,VAContextID context,VABufferID *buffers,int num_buffers); */
   0x19, /* VAStatus (*vaEndPicture) (VADriverContextP ctx,VAContextID context); */
   0x20, /* VAStatus (*vaSyncSurface) (VADriverContextP ctx,VASurfaceID render_target); */
   0x21, /* VAStatus (*vaQuerySurfaceStatus) (VADriverContextP ctx,VASurfaceID render_target,VASurfaceStatus *status); */
   0x22, /* VAStatus (*vaPutSurface) (
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
   &vlVaQueryImageFormats, /* VAStatus (*vaQueryImageFormats) ( VADriverContextP ctx, VAImageFormat *format_list,int *num_formats); */
   &vlVaCreateImage, /* VAStatus (*vaCreateImage) (VADriverContextP ctx,VAImageFormat *format,int width,int height,VAImage *image); */
   0x25, /* VAStatus (*vaDeriveImage) (VADriverContextP ctx,VASurfaceID surface,VAImage *image); */
   0x26, /* VAStatus (*vaDestroyImage) (VADriverContextP ctx,VAImageID image); */
   0x27, /* VAStatus (*vaSetImagePalette) (VADriverContextP ctx,VAImageID image, unsigned char *palette); */
   0x28, /* VAStatus (*vaGetImage) (VADriverContextP ctx,VASurfaceID surface,int x,int y,unsigned int width,unsigned int height,VAImageID image); */
   0x29, /* VAStatus (*vaPutImage) (
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
   &vlVaQuerySubpictureFormats,	/* VAStatus (*vaQuerySubpictureFormats) (VADriverContextP ctx,VAImageFormat *format_list,unsigned int *flags,unsigned int *num_formats); */
   0x31, /* VAStatus (*vaCreateSubpicture) (VADriverContextP ctx,VAImageID image,VASubpictureID *subpicture); */
   0x32, /* VAStatus (*vaDestroySubpicture) (VADriverContextP ctx,VASubpictureID subpicture); */
   0x33, /* VAStatus (*vaSetSubpictureImage) (VADriverContextP ctx,VASubpictureID subpicture,VAImageID image); */
   0x34, /* VAStatus (*vaSetSubpictureChromakey) (VADriverContextP ctx,VASubpictureID subpicture,unsigned int chromakey_min,unsigned int chromakey_max,unsigned int chromakey_mask); */
   0x35, /* VAStatus (*vaSetSubpictureGlobalAlpha) (VADriverContextP ctx,VASubpictureID subpicture,float global_alpha); */
   0x36, /* VAStatus (*vaAssociateSubpicture) (
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
   0x37, /* VAStatus (*vaDeassociateSubpicture) (VADriverContextP ctx,VASubpictureID subpicture,VASurfaceID *target_surfaces,int num_surfaces); */
   0x38, /* VAStatus (*vaQueryDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int *num_attributes); */
   0x39, /* VAStatus (*vaGetDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes); */
   0x40, /* VAStatus (*vaSetDisplayAttributes) (VADriverContextP ctx,VADisplayAttribute *attr_list,int num_attributes); */
   0x41, /* VAStatus (*vaBufferInfo) (VADriverContextP ctx,VAContextID context,VABufferID buf_id,VABufferType *type,unsigned int *size,unsigned int *num_elements); */
   0x42, /* VAStatus (*vaLockSurface) (
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
   0x43, /* VAStatus (*vaUnlockSurface) (VADriverContextP ctx,VASurfaceID surface); */
   0x44 /* struct VADriverVTableGLX *glx; "Optional" */
};

struct VADriverVTable vlVaGetVtable()
{
	return vtable;
}