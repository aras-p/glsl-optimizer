/**************************************************************************
 *
 * Copyright 2006 VMware, Inc.
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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#ifndef INTEL_IMAGE_H
#define INTEL_IMAGE_H

/** @file intel_image.h
 *
 * Structure definitions and prototypes for __DRIimage, the driver-private
 * structure backing EGLImage or a drawable in DRI3.
 *
 * The __DRIimage is passed around the loader code (src/glx and src/egl), but
 * it's opaque to that code and may only be accessed by loader extensions
 * (mostly located in intel_screen.c).
 */

#include <stdbool.h>
#include <xf86drm.h>

#include "main/mtypes.h"
#include "intel_bufmgr.h"
#include <GL/internal/dri_interface.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Used with images created with image_from_names
 * to help support planar images.
 */
struct intel_image_format {
   int fourcc;
   int components;
   int nplanes;
   struct {
      int buffer_index;
      int width_shift;
      int height_shift;
      uint32_t dri_format;
      int cpp;
   } planes[3];
};

struct __DRIimageRec {
   drm_intel_bo *bo;
   uint32_t pitch; /**< in bytes */
   GLenum internal_format;
   uint32_t dri_format;
   GLuint format;
   uint32_t offset;

   /*
    * Need to save these here between calls to
    * image_from_names and calls to image_from_planar.
    */
   uint32_t strides[3];
   uint32_t offsets[3];
   struct intel_image_format *planar_format;

   /* particular miptree level */
   GLuint width;
   GLuint height;
   GLuint tile_x;
   GLuint tile_y;
   bool has_depthstencil;

   /**
    * Provided by EGL_EXT_image_dma_buf_import.
    *
    * The flag is set in order to restrict the use of the image later on.
    *
    * See intel_image_target_texture_2d()
    */
   bool dma_buf_imported;
   enum __DRIYUVColorSpace yuv_color_space;
   enum __DRISampleRange sample_range;
   enum __DRIChromaSiting horizontal_siting;
   enum __DRIChromaSiting vertical_siting;

   void *data;
};

#ifdef __cplusplus
}
#endif

#endif
