/**************************************************************************
 * 
 * Copyright 2003 VMware, Inc.
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

#ifndef _INTEL_INIT_H_
#define _INTEL_INIT_H_

#include <stdbool.h>
#include <sys/time.h>
#include "dri_util.h"
#include "intel_bufmgr.h"
#include "i915_drm.h"
#include "xmlconfig.h"

struct intel_screen
{
   int deviceID;
   int gen;

   __DRIscreen *driScrnPriv;

   bool no_hw;

   bool hw_has_swizzling;

   bool no_vbo;
   dri_bufmgr *bufmgr;

   /**
   * Configuration cache with default values for all contexts
   */
   driOptionCache optionCache;
};

/* These defines are to ensure that i915_dri's symbols don't conflict with
 * i965's when linked together.
 */
#define intel_region_alloc                  old_intel_region_alloc
#define intel_region_alloc_for_fd           old_intel_region_alloc_for_fd
#define intel_region_alloc_for_handle       old_intel_region_alloc_for_handle
#define intel_region_flink                  old_intel_region_flink
#define intel_region_get_aligned_offset     old_intel_region_get_aligned_offset
#define intel_region_get_tile_masks         old_intel_region_get_tile_masks
#define intel_region_reference              old_intel_region_reference
#define intel_region_release                old_intel_region_release
#define intel_bufferobj_buffer              old_intel_bufferobj_buffer
#define intel_bufferobj_source              old_intel_bufferobj_source
#define intelInitBufferObjectFuncs          old_intelInitBufferObjectFuncs
#define intel_upload_data                   old_intel_upload_data
#define intel_upload_finish                 old_intel_upload_finish
#define intel_batchbuffer_data              old_intel_batchbuffer_data
#define intel_batchbuffer_emit_mi_flush     old_intel_batchbuffer_emit_mi_flush
#define intel_batchbuffer_emit_reloc        old_intel_batchbuffer_emit_reloc
#define intel_batchbuffer_emit_reloc_fenced old_intel_batchbuffer_emit_reloc_fenced
#define _intel_batchbuffer_flush            old__intel_batchbuffer_flush
#define intel_batchbuffer_free              old_intel_batchbuffer_free
#define intel_batchbuffer_init              old_intel_batchbuffer_init
#define intelInitClearFuncs                 old_intelInitClearFuncs
#define intelInitExtensions                 old_intelInitExtensions
#define intel_miptree_copy_teximage         old_intel_miptree_copy_teximage
#define intel_miptree_create                old_intel_miptree_create
#define intel_miptree_create_for_bo         old_intel_miptree_create_for_bo
#define intel_miptree_create_for_dri2_buffer old_intel_miptree_create_for_dri2_buffer
#define intel_miptree_create_for_renderbuffer old_intel_miptree_create_for_renderbuffer
#define intel_miptree_create_layout         old_intel_miptree_create_layout
#define intel_miptree_get_dimensions_for_image old_intel_miptree_get_dimensions_for_image
#define intel_miptree_get_image_offset      old_intel_miptree_get_image_offset
#define intel_miptree_get_tile_offsets      old_intel_miptree_get_tile_offsets
#define intel_miptree_map                   old_intel_miptree_map
#define intel_miptree_map_raw               old_intel_miptree_map_raw
#define intel_miptree_match_image           old_intel_miptree_match_image
#define intel_miptree_reference             old_intel_miptree_reference
#define intel_miptree_release               old_intel_miptree_release
#define intel_miptree_set_image_offset      old_intel_miptree_set_image_offset
#define intel_miptree_set_level_info        old_intel_miptree_set_level_info
#define intel_miptree_unmap                 old_intel_miptree_unmap
#define intel_miptree_unmap_raw             old_intel_miptree_unmap_raw
#define i945_miptree_layout_2d              old_i945_miptree_layout_2d
#define intel_get_texture_alignment_unit    old_intel_get_texture_alignment_unit
#define intelInitTextureImageFuncs          old_intelInitTextureImageFuncs
#define intel_miptree_create_for_teximage   old_intel_miptree_create_for_teximage
#define intelSetTexBuffer                   old_intelSetTexBuffer
#define intelSetTexBuffer2                  old_intelSetTexBuffer2
#define intelInitTextureSubImageFuncs       old_intelInitTextureSubImageFuncs
#define intelInitTextureCopyImageFuncs      old_intelInitTextureCopyImageFuncs
#define intel_finalize_mipmap_tree          old_intel_finalize_mipmap_tree
#define intelInitTextureFuncs               old_intelInitTextureFuncs
#define intel_check_blit_fragment_ops       old_intel_check_blit_fragment_ops
#define intelInitPixelFuncs                 old_intelInitPixelFuncs
#define intelBitmap                         old_intelBitmap
#define intelCopyPixels                     old_intelCopyPixels
#define intelDrawPixels                     old_intelDrawPixels
#define intelReadPixels                     old_intelReadPixels
#define intel_check_front_buffer_rendering  old_intel_check_front_buffer_rendering
#define intelInitBufferFuncs                old_intelInitBufferFuncs
#define intelClearWithBlit                  old_intelClearWithBlit
#define intelEmitCopyBlit                   old_intelEmitCopyBlit
#define intelEmitImmediateColorExpandBlit   old_intelEmitImmediateColorExpandBlit
#define intel_emit_linear_blit              old_intel_emit_linear_blit
#define intel_miptree_blit                  old_intel_miptree_blit
#define i945_miptree_layout                 old_i945_miptree_layout
#define intel_init_texture_formats          old_intel_init_texture_formats
#define intelCalcViewport                   old_intelCalcViewport
#define INTEL_DEBUG                         old_INTEL_DEBUG
#define intelDestroyContext                 old_intelDestroyContext
#define intelFinish                         old_intelFinish
#define _intel_flush                        old__intel_flush
#define intel_flush_rendering_to_batch      old_intel_flush_rendering_to_batch
#define intelInitContext                    old_intelInitContext
#define intelInitDriverFunctions            old_intelInitDriverFunctions
#define intelMakeCurrent                    old_intelMakeCurrent
#define intel_prepare_render                old_intel_prepare_render
#define intelUnbindContext                  old_intelUnbindContext
#define intel_update_renderbuffers          old_intel_update_renderbuffers
#define aub_dump_bmp                        old_aub_dump_bmp
#define get_time                            old_get_time
#define intel_translate_blend_factor        old_intel_translate_blend_factor
#define intel_translate_compare_func        old_intel_translate_compare_func
#define intel_translate_logic_op            old_intel_translate_logic_op
#define intel_translate_shadow_compare_func old_intel_translate_shadow_compare_func
#define intel_translate_stencil_op          old_intel_translate_stencil_op
#define intel_init_syncobj_functions        old_intel_init_syncobj_functions
#define intelChooseRenderState              old_intelChooseRenderState
#define intelFallback                       old_intelFallback
#define intel_finish_vb                     old_intel_finish_vb
#define intel_flush_prim                    old_intel_flush_prim
#define intel_get_prim_space                old_intel_get_prim_space
#define intelInitTriFuncs                   old_intelInitTriFuncs
#define intel_set_prim                      old_intel_set_prim
#define intel_create_private_renderbuffer   old_intel_create_private_renderbuffer
#define intel_create_renderbuffer           old_intel_create_renderbuffer
#define intel_fbo_init                      old_intel_fbo_init
#define intel_get_rb_region                 old_intel_get_rb_region
#define intel_renderbuffer_set_draw_offset  old_intel_renderbuffer_set_draw_offset
#define intel_miptree_create_for_image_buffer old_intel_miptree_create_for_image_buffer

extern void intelDestroyContext(__DRIcontext * driContextPriv);

extern GLboolean intelUnbindContext(__DRIcontext * driContextPriv);

const __DRIextension **__driDriverGetExtensions_i915(void);

extern GLboolean
intelMakeCurrent(__DRIcontext * driContextPriv,
                 __DRIdrawable * driDrawPriv,
                 __DRIdrawable * driReadPriv);

double get_time(void);
void aub_dump_bmp(struct gl_context *ctx);

#endif
