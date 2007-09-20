/**************************************************************************
 *
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
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

 /*
  * Authors:
  *   Zack Rusin <zack@tungstengraphics.com>
  */

#ifndef CSO_CACHE_H
#define CSO_CACHE_H

#include "pipe/p_context.h"
#include "pipe/p_state.h"


struct cso_hash;

struct cso_cache {
   struct cso_hash *blend_hash;
   struct cso_hash *sampler_hash;
   struct cso_hash *depth_stencil_hash;
   struct cso_hash *rasterizer_hash;
   struct cso_hash *fs_hash;
   struct cso_hash *vs_hash;
};

struct cso_blend {
   struct pipe_blend_state state;
   void   *data;
};

struct cso_depth_stencil {
   struct pipe_depth_stencil_state state;
   void *data;
};

struct cso_rasterizer {
   struct pipe_rasterizer_state state;
   void *data;
};

struct cso_fragment_shader {
   struct pipe_shader_state state;
   void *data;
};

struct cso_vertex_shader {
   struct pipe_shader_state state;
   void *data;
};

struct cso_sampler {
   struct pipe_sampler_state state;
   void *data;
};

enum cso_cache_type {
   CSO_BLEND,
   CSO_SAMPLER,
   CSO_DEPTH_STENCIL,
   CSO_RASTERIZER,
   CSO_FRAGMENT_SHADER,
   CSO_VERTEX_SHADER
};

unsigned cso_construct_key(void *item, int item_size);

struct cso_cache *cso_cache_create(void);
void cso_cache_delete(struct cso_cache *sc);

struct cso_hash_iter cso_insert_state(struct cso_cache *sc,
                                      unsigned hash_key, enum cso_cache_type type,
                                      void *state);
struct cso_hash_iter cso_find_state(struct cso_cache *sc,
                                    unsigned hash_key, enum cso_cache_type type);
struct cso_hash_iter cso_find_state_template(struct cso_cache *sc,
                                             unsigned hash_key, enum cso_cache_type type,
                                             void *templ);
void * cso_take_state(struct cso_cache *sc, unsigned hash_key,
                      enum cso_cache_type type);

#endif
