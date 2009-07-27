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

/* Authors:  Keith Whitwell <keith@tungstengraphics.com>
 */

#ifndef LP_QUAD_PIPE_H
#define LP_QUAD_PIPE_H


struct llvmpipe_context;
struct quad_header;


/**
 * Fragment processing is performed on 2x2 blocks of pixels called "quads".
 * Quad processing is performed with a pipeline of stages represented by
 * this type.
 */
struct quad_stage {
   struct llvmpipe_context *llvmpipe;

   struct quad_stage *next;

   void (*begin)(struct quad_stage *qs);

   /** the stage action */
   void (*run)(struct quad_stage *qs, struct quad_header *quad[], unsigned nr);

   void (*destroy)(struct quad_stage *qs);
};


struct quad_stage *lp_quad_polygon_stipple_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_earlyz_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_shade_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_alpha_test_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_stencil_test_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_depth_test_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_occlusion_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_coverage_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_blend_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_colormask_stage( struct llvmpipe_context *llvmpipe );
struct quad_stage *lp_quad_output_stage( struct llvmpipe_context *llvmpipe );

void lp_build_quad_pipeline(struct llvmpipe_context *lp);

#endif /* LP_QUAD_PIPE_H */
