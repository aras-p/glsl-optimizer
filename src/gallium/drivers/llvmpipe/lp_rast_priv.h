/**************************************************************************
 *
 * Copyright 2009 VMware, Inc.
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

#ifndef LP_RAST_PRIV_H
#define LP_RAST_PRIV_H

#include "lp_rast.h"


/* We can choose whatever layout for the internal tile storage we
 * prefer:
 */
struct lp_rast_tile
{
   uint8_t *color;

   uint32_t *depth;
};


struct lp_rasterizer {

   /* We can choose whatever layout for the internal tile storage we
    * prefer:
    */
   struct lp_rast_tile tile;

      
   unsigned x;
   unsigned y;

   
   struct {
      struct pipe_surface *cbuf;
      struct pipe_surface *zsbuf;
      unsigned clear_color;
      unsigned clear_depth;
      char clear_stencil;
   } state;

   const struct lp_rast_state *shader_state;
};


void lp_rast_shade_quads( struct lp_rasterizer *rast,
                          const struct lp_rast_shader_inputs *inputs,
                          unsigned x, unsigned y,
                          const unsigned *masks);

#endif
