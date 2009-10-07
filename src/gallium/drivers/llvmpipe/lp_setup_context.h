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
#ifndef LP_SETUP_CONTEXT_H
#define LP_SETUP_CONTEXT_H

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

/* Shade tile points directly at this:
 */
struct shader_inputs {
   /* Some way of updating rasterizer state:
    */
   /* ??? */

   /* Attribute interpolation:
    */
   float oneoverarea;
   float x1;
   float y1;

   struct tgsi_interp_coef position_coef;
   struct tgsi_interp_coef *coef;
};

/* Shade triangle points at this:
 */
struct shade_triangle {
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
   
   struct shader_inputs inputs;
};

struct bin_cmd {
   enum {
      CMD_END = 0,
      CMD_CLEAR,
      CMD_LOAD_TILE,
      CMD_SHADE_TILE,
      CMD_SHADE_TRIANGLE,
   } cmd;

   union {
      struct triangle *tri;
      struct clear *clear;
   } ptr;
};

struct cmd_block {
   struct bin_cmd cmds[128];
   unsigned count;
   struct cmd_block *next;
};

/* Triangles
 */
struct data_block {
   ubyte data[4096 - sizeof(unsigned) - sizeof(struct cmd_block *)];
   unsigned count;
   struct data_block *next;
};

/* Need to store the state at the time the triangle was drawn, at
 * least as it is needed during rasterization.  That would include at
 * minimum the constant values referred to by the fragment shader,
 * blend state, etc.  Much of this is code-generated into the shader
 * in llvmpipe -- may be easier to do this work there.
 */
struct state_block {
};


/**
 * Basically all the data from a binner scene:
 */
struct binned_scene {
   struct llvmpipe_context *llvmpipe;

   struct cmd_block *bin[MAX_HEIGHT / BIN_SIZE][MAX_WIDTH / BIN_SIZE];
   struct data_block *data;
};

static INLINE struct triangle *get_triangle( struct setup_context *setup )
{
   if (setup->triangles->count == TRIANGLE_BLOCK_COUNT)
      return setup_triangle_from_new_block( setup );

   return &setup->triangles[setup->triangles->count++];
}
