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

#ifndef INTEL_BLIT_H
#define INTEL_BLIT_H

#include "brw_context.h"

bool
intelEmitCopyBlit(struct brw_context *brw,
                              GLuint cpp,
                              GLshort src_pitch,
                              drm_intel_bo *src_buffer,
                              GLuint src_offset,
			      uint32_t src_tiling,
                              GLshort dst_pitch,
                              drm_intel_bo *dst_buffer,
                              GLuint dst_offset,
			      uint32_t dst_tiling,
                              GLshort srcx, GLshort srcy,
                              GLshort dstx, GLshort dsty,
                              GLshort w, GLshort h,
			      GLenum logicop );

bool intel_miptree_blit(struct brw_context *brw,
                        struct intel_mipmap_tree *src_mt,
                        int src_level, int src_slice,
                        uint32_t src_x, uint32_t src_y, bool src_flip,
                        struct intel_mipmap_tree *dst_mt,
                        int dst_level, int dst_slice,
                        uint32_t dst_x, uint32_t dst_y, bool dst_flip,
                        uint32_t width, uint32_t height,
                        GLenum logicop);

bool
intelEmitImmediateColorExpandBlit(struct brw_context *brw,
				  GLuint cpp,
				  GLubyte *src_bits, GLuint src_size,
				  GLuint fg_color,
				  GLshort dst_pitch,
				  drm_intel_bo *dst_buffer,
				  GLuint dst_offset,
				  uint32_t dst_tiling,
				  GLshort x, GLshort y,
				  GLshort w, GLshort h,
				  GLenum logic_op);
void intel_emit_linear_blit(struct brw_context *brw,
			    drm_intel_bo *dst_bo,
			    unsigned int dst_offset,
			    drm_intel_bo *src_bo,
			    unsigned int src_offset,
			    unsigned int size);

#endif
