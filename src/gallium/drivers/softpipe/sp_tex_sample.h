/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * Copyright 2010 VMware, Inc.  All rights reserved.
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

#ifndef SP_TEX_SAMPLE_H
#define SP_TEX_SAMPLE_H


#include "tgsi/tgsi_exec.h"

struct sp_sampler_varient;

typedef void (*wrap_nearest_func)(const float s[4],
                                  unsigned size,
                                  int icoord[4]);

typedef void (*wrap_linear_func)(const float s[4], 
                                 unsigned size,
                                 int icoord0[4],
                                 int icoord1[4],
                                 float w[4]);

typedef float (*compute_lambda_func)(const struct sp_sampler_varient *sampler,
                                     const float s[QUAD_SIZE],
                                     const float t[QUAD_SIZE],
                                     const float p[QUAD_SIZE]);

typedef void (*filter_func)(struct tgsi_sampler *tgsi_sampler,
                            const float s[QUAD_SIZE],
                            const float t[QUAD_SIZE],
                            const float p[QUAD_SIZE],
                            const float c0[QUAD_SIZE],
                            enum tgsi_sampler_control control,
                            float rgba[NUM_CHANNELS][QUAD_SIZE]);


union sp_sampler_key {
   struct {
      unsigned target:3;
      unsigned is_pot:1;
      unsigned processor:2;
      unsigned unit:4;
      unsigned pad:22;
   } bits;
   unsigned value;
};

/**
 * Subclass of tgsi_sampler
 */
struct sp_sampler_varient
{
   struct tgsi_sampler base;  /**< base class */

   union sp_sampler_key key;

   /* The owner of this struct:
    */
   const struct pipe_sampler_state *sampler;


   /* Currently bound texture:
    */
   const struct pipe_texture *texture;
   struct softpipe_tex_tile_cache *cache;

   unsigned processor;

   /* For sp_get_samples_2d_linear_POT:
    */
   unsigned xpot;
   unsigned ypot;
   unsigned level;

   unsigned faces[4];
   
   wrap_nearest_func nearest_texcoord_s;
   wrap_nearest_func nearest_texcoord_t;
   wrap_nearest_func nearest_texcoord_p;

   wrap_linear_func linear_texcoord_s;
   wrap_linear_func linear_texcoord_t;
   wrap_linear_func linear_texcoord_p;

   filter_func min_img_filter;
   filter_func mag_img_filter;

   compute_lambda_func compute_lambda;

   filter_func mip_filter;
   filter_func compare;
   
   /* Linked list:
    */
   struct sp_sampler_varient *next;
};

struct sp_sampler;

/* Create a sampler varient for a given set of non-orthogonal state.  Currently the 
 */
struct sp_sampler_varient *
sp_create_sampler_varient( const struct pipe_sampler_state *sampler,
                           const union sp_sampler_key key );

void sp_sampler_varient_bind_texture( struct sp_sampler_varient *varient,
                                      struct softpipe_tex_tile_cache *tex_cache,
                                      const struct pipe_texture *tex );

void sp_sampler_varient_destroy( struct sp_sampler_varient * );



static INLINE struct sp_sampler_varient *
sp_sampler_varient(const struct tgsi_sampler *sampler)
{
   return (struct sp_sampler_varient *) sampler;
}

extern void
sp_get_samples(struct tgsi_sampler *tgsi_sampler,
               const float s[QUAD_SIZE],
               const float t[QUAD_SIZE],
               const float p[QUAD_SIZE],
               float lodbias,
               float rgba[NUM_CHANNELS][QUAD_SIZE]);


#endif /* SP_TEX_SAMPLE_H */
