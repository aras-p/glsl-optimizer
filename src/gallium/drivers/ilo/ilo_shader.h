/*
 * Mesa 3-D graphics library
 *
 * Copyright (C) 2012-2013 LunarG, Inc.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Chia-I Wu <olv@lunarg.com>
 */

#ifndef ILO_SHADER_H
#define ILO_SHADER_H

#include "ilo_common.h"

struct intel_bo;
struct ilo_context;
struct ilo_shader_cache;
struct ilo_shader_state;

struct ilo_shader_cache *
ilo_shader_cache_create(void);

void
ilo_shader_cache_destroy(struct ilo_shader_cache *shc);

void
ilo_shader_cache_add(struct ilo_shader_cache *shc,
                     struct ilo_shader_state *shader);

void
ilo_shader_cache_remove(struct ilo_shader_cache *shc,
                        struct ilo_shader_state *shader);

int
ilo_shader_cache_upload(struct ilo_shader_cache *shc,
                        struct intel_bo *bo, unsigned offset,
                        bool incremental);

struct ilo_shader_state *
ilo_shader_create_vs(const struct ilo_dev_info *dev,
                     const struct pipe_shader_state *state,
                     const struct ilo_context *precompile);

struct ilo_shader_state *
ilo_shader_create_gs(const struct ilo_dev_info *dev,
                     const struct pipe_shader_state *state,
                     const struct ilo_context *precompile);

struct ilo_shader_state *
ilo_shader_create_fs(const struct ilo_dev_info *dev,
                     const struct pipe_shader_state *state,
                     const struct ilo_context *precompile);

struct ilo_shader_state *
ilo_shader_create_cs(const struct ilo_dev_info *dev,
                     const struct pipe_compute_state *state,
                     const struct ilo_context *precompile);

void
ilo_shader_destroy(struct ilo_shader_state *shader);

bool
ilo_shader_select_kernel(struct ilo_shader_state *shader,
                         const struct ilo_context *ilo,
                         uint32_t dirty);

#endif /* ILO_SHADER_H */
