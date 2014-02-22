/*
 * Copyright © 2011 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#ifndef BRW_PROGRAM_H
#define BRW_PROGRAM_H

enum gen6_gather_sampler_wa {
   WA_SIGN = 1,      /* whether we need to sign extend */
   WA_8BIT = 2,      /* if we have an 8bit format needing wa */
   WA_16BIT = 4,     /* if we have a 16bit format needing wa */
};

/**
 * Sampler information needed by VS, WM, and GS program cache keys.
 */
struct brw_sampler_prog_key_data {
   /**
    * EXT_texture_swizzle and DEPTH_TEXTURE_MODE swizzles.
    */
   uint16_t swizzles[MAX_SAMPLERS];

   uint32_t gl_clamp_mask[3];

   /**
    * For RG32F, gather4's channel select is broken.
    */
   uint32_t gather_channel_quirk_mask;

   /**
    * Whether this sampler uses the compressed multisample surface layout.
    */
   uint32_t compressed_multisample_layout_mask;

   /**
    * For Sandybridge, which shader w/a we need for gather quirks.
    */
   uint8_t gen6_gather_wa[MAX_SAMPLERS];
};

#ifdef __cplusplus
extern "C" {
#endif

void brw_populate_sampler_prog_key_data(struct gl_context *ctx,
				        const struct gl_program *prog,
                                        unsigned sampler_count,
				        struct brw_sampler_prog_key_data *key);
bool brw_debug_recompile_sampler_key(struct brw_context *brw,
                                     const struct brw_sampler_prog_key_data *old_key,
                                     const struct brw_sampler_prog_key_data *key);
void brw_add_texrect_params(struct gl_program *prog);

void
brw_mark_surface_used(struct brw_stage_prog_data *prog_data,
                      unsigned surf_index);

bool
brw_stage_prog_data_compare(const struct brw_stage_prog_data *a,
                            const struct brw_stage_prog_data *b);

void
brw_stage_prog_data_free(const void *prog_data);

void
brw_dump_ir(struct brw_context *brw, const char *stage,
            struct gl_shader_program *shader_prog,
            struct gl_shader *shader, struct gl_program *prog);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
