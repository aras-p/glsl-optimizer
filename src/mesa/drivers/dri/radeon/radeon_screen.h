/**************************************************************************

Copyright 2000, 2001 ATI Technologies Inc., Ontario, Canada, and
                     VA Linux Systems Inc., Fremont, California.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining
a copy of this software and associated documentation files (the
"Software"), to deal in the Software without restriction, including
without limitation the rights to use, copy, modify, merge, publish,
distribute, sublicense, and/or sell copies of the Software, and to
permit persons to whom the Software is furnished to do so, subject to
the following conditions:

The above copyright notice and this permission notice (including the
next paragraph) shall be included in all copies or substantial
portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Kevin E. Martin <martin@valinux.com>
 *   Gareth Hughes <gareth@valinux.com>
 */

#ifndef __RADEON_SCREEN_H__
#define __RADEON_SCREEN_H__

/*
 * IMPORTS: these headers contain all the DRI, X and kernel-related
 * definitions that we need.
 */
#include "dri_util.h"
#include "radeon_dri.h"
#include "radeon_chipset.h"
#include "radeon_reg.h"
#include "drm_sarea.h"
#include "xmlconfig.h"


typedef struct {
   drm_handle_t handle;			/* Handle to the DRM region */
   drmSize size;			/* Size of the DRM region */
   drmAddress map;			/* Mapping of the DRM region */
} radeonRegionRec, *radeonRegionPtr;

typedef struct radeon_screen {
   int chip_family;
   int chip_flags;
   int cpp;
   int card_type;
   int device_id; /* PCI ID */
   int AGPMode;
   unsigned int irq;			/* IRQ number (0 means none) */

   unsigned int fbLocation;
   unsigned int frontOffset;
   unsigned int frontPitch;
   unsigned int backOffset;
   unsigned int backPitch;

   unsigned int depthOffset;
   unsigned int depthPitch;

    /* Shared texture data */
   int numTexHeaps;
   int texOffset[RADEON_NR_TEX_HEAPS];
   int texSize[RADEON_NR_TEX_HEAPS];
   int logTexGranularity[RADEON_NR_TEX_HEAPS];

   radeonRegionRec mmio;
   radeonRegionRec status;
   radeonRegionRec gartTextures;

   drmBufMapPtr buffers;

   __volatile__ uint32_t *scratch;

   __DRIscreen *driScreen;
   unsigned int sarea_priv_offset;
   unsigned int gart_buffer_offset;	/* offset in card memory space */
   unsigned int gart_texture_offset;	/* offset in card memory space */
   unsigned int gart_base;

   GLboolean depthHasSurface;

   /* Configuration cache with default values for all contexts */
   driOptionCache optionCache;

   int num_gb_pipes;
   int num_z_pipes;
   drm_radeon_sarea_t *sarea;	/* Private SAREA data */
   struct radeon_bo_manager *bom;

} radeonScreenRec, *radeonScreenPtr;

struct __DRIimageRec {
   struct radeon_bo *bo;
   GLenum internal_format;
   uint32_t dri_format;
   GLuint format;
   GLenum data_type;
   int width, height;  /* in pixels */
   int pitch;          /* in pixels */
   int cpp;
   void *data;
};

#ifdef RADEON_R200
/* These defines are to ensure that r200_dri's symbols don't conflict with
 * radeon's when linked together.
 */
#define get_radeon_buffer_object            r200_get_radeon_buffer_object
#define radeonInitBufferObjectFuncs         r200_radeonInitBufferObjectFuncs
#define radeonDestroyContext                r200_radeonDestroyContext
#define radeonInitContext                   r200_radeonInitContext
#define radeonMakeCurrent                   r200_radeonMakeCurrent
#define radeon_prepare_render               r200_radeon_prepare_render
#define radeonUnbindContext                 r200_radeonUnbindContext
#define radeon_update_renderbuffers         r200_radeon_update_renderbuffers
#define radeon_check_front_buffer_rendering r200_radeon_check_front_buffer_rendering
#define radeonCountStateEmitSize            r200_radeonCountStateEmitSize
#define radeon_draw_buffer                  r200_radeon_draw_buffer
#define radeonDrawBuffer                    r200_radeonDrawBuffer
#define radeonEmitState                     r200_radeonEmitState
#define radeonFinish                        r200_radeonFinish
#define radeonFlush                         r200_radeonFlush
#define radeonGetAge                        r200_radeonGetAge
#define radeonReadBuffer                    r200_radeonReadBuffer
#define radeonScissor                       r200_radeonScissor
#define radeonSetCliprects                  r200_radeonSetCliprects
#define radeonUpdateScissor                 r200_radeonUpdateScissor
#define radeonUserClear                     r200_radeonUserClear
#define radeon_viewport                     r200_radeon_viewport
#define radeon_window_moved                 r200_radeon_window_moved
#define rcommonBeginBatch                   r200_rcommonBeginBatch
#define rcommonDestroyCmdBuf                r200_rcommonDestroyCmdBuf
#define rcommonEnsureCmdBufSpace            r200_rcommonEnsureCmdBufSpace
#define rcommonFlushCmdBuf                  r200_rcommonFlushCmdBuf
#define rcommonFlushCmdBufLocked            r200_rcommonFlushCmdBufLocked
#define rcommonInitCmdBuf                   r200_rcommonInitCmdBuf
#define radeonAllocDmaRegion                r200_radeonAllocDmaRegion
#define radeonEmitVec12                     r200_radeonEmitVec12
#define radeonEmitVec16                     r200_radeonEmitVec16
#define radeonEmitVec4                      r200_radeonEmitVec4
#define radeonEmitVec8                      r200_radeonEmitVec8
#define radeonFreeDmaRegions                r200_radeonFreeDmaRegions
#define radeon_init_dma                     r200_radeon_init_dma
#define radeonRefillCurrentDmaRegion        r200_radeonRefillCurrentDmaRegion
#define radeonReleaseArrays                 r200_radeonReleaseArrays
#define radeonReleaseDmaRegions             r200_radeonReleaseDmaRegions
#define radeonReturnDmaRegion               r200_radeonReturnDmaRegion
#define rcommonAllocDmaLowVerts             r200_rcommonAllocDmaLowVerts
#define rcommon_emit_vecfog                 r200_rcommon_emit_vecfog
#define rcommon_emit_vector                 r200_rcommon_emit_vector
#define rcommon_flush_last_swtcl_prim       r200_rcommon_flush_last_swtcl_prim
#define _radeon_debug_add_indent            r200__radeon_debug_add_indent
#define _radeon_debug_remove_indent         r200__radeon_debug_remove_indent
#define radeon_init_debug                   r200_radeon_init_debug
#define _radeon_print                       r200__radeon_print
#define radeon_create_renderbuffer          r200_radeon_create_renderbuffer
#define radeon_fbo_init                     r200_radeon_fbo_init
#define radeon_renderbuffer_set_bo          r200_radeon_renderbuffer_set_bo
#define radeonComputeFogBlendFactor         r200_radeonComputeFogBlendFactor
#define radeonInitStaticFogData             r200_radeonInitStaticFogData
#define get_base_teximage_offset            r200_get_base_teximage_offset
#define get_texture_image_row_stride        r200_get_texture_image_row_stride
#define get_texture_image_size              r200_get_texture_image_size
#define radeon_miptree_create               r200_radeon_miptree_create
#define radeon_miptree_image_offset         r200_radeon_miptree_image_offset
#define radeon_miptree_matches_image        r200_radeon_miptree_matches_image
#define radeon_miptree_reference            r200_radeon_miptree_reference
#define radeon_miptree_unreference          r200_radeon_miptree_unreference
#define radeon_try_alloc_miptree            r200_radeon_try_alloc_miptree
#define radeon_validate_texture_miptree     r200_radeon_validate_texture_miptree
#define radeonReadPixels                    r200_radeonReadPixels
#define radeon_check_query_active           r200_radeon_check_query_active
#define radeonEmitQueryEnd                  r200_radeonEmitQueryEnd
#define radeon_emit_queryobj                r200_radeon_emit_queryobj
#define radeonInitQueryObjFunctions         r200_radeonInitQueryObjFunctions
#define radeonInitSpanFuncs                 r200_radeonInitSpanFuncs
#define copy_rows                           r200_copy_rows
#define radeonChooseTextureFormat           r200_radeonChooseTextureFormat
#define radeonChooseTextureFormat_mesa      r200_radeonChooseTextureFormat_mesa
#define radeonFreeTextureImageBuffer        r200_radeonFreeTextureImageBuffer
#define radeon_image_target_texture_2d      r200_radeon_image_target_texture_2d
#define radeon_init_common_texture_funcs    r200_radeon_init_common_texture_funcs
#define radeonIsFormatRenderable            r200_radeonIsFormatRenderable
#define radeonNewTextureImage               r200_radeonNewTextureImage
#define _radeon_texformat_al88              r200__radeon_texformat_al88
#define _radeon_texformat_argb1555          r200__radeon_texformat_argb1555
#define _radeon_texformat_argb4444          r200__radeon_texformat_argb4444
#define _radeon_texformat_argb8888          r200__radeon_texformat_argb8888
#define _radeon_texformat_rgb565            r200__radeon_texformat_rgb565
#define _radeon_texformat_rgba8888          r200__radeon_texformat_rgba8888
#define radeonCopyTexSubImage               r200_radeonCopyTexSubImage
#define get_tile_size                       r200_get_tile_size
#define tile_image                          r200_tile_image
#define untile_image                        r200_untile_image
#define set_re_cntl_d3d                     r200_set_re_cntl_d3d
#define radeonDestroyBuffer                 r200_radeonDestroyBuffer
#define radeonVendorString                  r200_radeonVendorString
#define radeonGetRendererString             r200_radeonGetRendererString
#endif

extern void radeonDestroyBuffer(__DRIdrawable *driDrawPriv);
const __DRIextension **__driDriverGetExtensions_radeon(void);
const __DRIextension **__driDriverGetExtensions_r200(void);

#endif /* __RADEON_SCREEN_H__ */
