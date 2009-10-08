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

#ifndef LP_RAST_H
#define LP_RAST_H

#include "pipe/p_compiler.h"
#include "lp_jit.h"

/* Initially create and program a single rasterizer directly.  Later
 * will want multiple of these, one or two per core.  At that stage
 * will probably pass command buffers into the rasterizers rather than
 * individual function calls like this.
 */
struct lp_rasterizer;

#define TILESIZE 64


struct lp_rast_state {
   /* State for the shader:
    */
   struct lp_jit_context jc;
   
   /* The shader itself.  Probably we also need to pass a pointer to
    * the tile color/z/stencil data somehow:
    */
   lp_jit_frag_func shader;

};

/* Coefficients necessary to run the shader at a given location:
 */
struct lp_rast_shader_inputs {

   /* Current rasterizer state:
    */
   const struct lp_rast_state *state;

   /* Attribute interpolation:  FIXME: reduce memory waste!
    */
   float a0[PIPE_MAX_ATTRIBS][4];
   float dadx[PIPE_MAX_ATTRIBS][4];
   float dady[PIPE_MAX_ATTRIBS][4];
};


/* Rasterization information for a triangle known to be in this bin,
 * plus inputs to run the shader:
 */
struct lp_rast_triangle {
   /* one-pixel sized trivial accept offsets for each plane */
   float ei1;                   
   float ei2;
   float ei3;

   /* one-pixel sized trivial reject offsets for each plane */
   float eo1;                   
   float eo2;
   float eo3;

   /* y deltas for vertex pairs */
   float dy12;
   float dy23;
   float dy31;

   /* x deltas for vertex pairs */
   float dx12;
   float dx23;
   float dx31;

   /* inputs for the shader */
   struct lp_rast_shader_inputs *inputs;
};

struct clear_tile {
   boolean do_color;
   boolean do_depth_stencil;
   unsigned rgba;
   unsigned depth_stencil;
};

struct load_tile {
   boolean do_color;
   boolean do_depth_stencil;
};


struct lp_rasterizer *lp_rast_create( void );

void lp_rast_bind_surfaces( struct lp_rasterizer *,
			    struct pipe_surface *cbuf,
			    struct pipe_surface *zsbuf,
			    const float *clear_color,
			    double clear_depth,
			    unsigned clear_stencil);

/* Begining of each tile:
 */
void lp_rast_start_tile( struct lp_rasterizer *,
			 unsigned x,
			 unsigned y );



union lp_rast_cmd_arg {
   const struct lp_rast_shader_inputs *shade_tile;
   const struct lp_rast_triangle *triangle;
   const struct lp_rast_state *set_state;
   const uint8_t clear_color[4];
   unsigned clear_zstencil;
};


/* Binnable Commands:
 */
void lp_rast_clear_color( struct lp_rasterizer *, 
                          const union lp_rast_cmd_arg *);

void lp_rast_clear_zstencil( struct lp_rasterizer *, 
                             const union lp_rast_cmd_arg *);

void lp_rast_load_color( struct lp_rasterizer *, 
                         const union lp_rast_cmd_arg *);

void lp_rast_load_zstencil( struct lp_rasterizer *, 
                            const union lp_rast_cmd_arg *);

void lp_rast_set_state( struct lp_rasterizer *, 
                        const union lp_rast_cmd_arg * );

void lp_rast_triangle( struct lp_rasterizer *, 
                       const union lp_rast_cmd_arg * );

void lp_rast_shade_tile( struct lp_rasterizer *,
                         const union lp_rast_cmd_arg *,
                         const struct lp_rast_shader_inputs *);

void lp_rast_store_color( struct lp_rasterizer *,
                          const union lp_rast_cmd_arg *);

void lp_rast_store_zstencil( struct lp_rasterizer *,
                             const union lp_rast_cmd_arg *);


/* End of tile:
 */

void lp_rast_end_tile( struct lp_rasterizer *rast,
                       boolean write_depth );

/* Shutdown:
 */
void lp_rast_destroy( struct lp_rasterizer * );


#endif
