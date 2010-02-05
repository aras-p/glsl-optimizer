/**************************************************************************
 *
 * Copyright 2007-2009 VMware, Inc.
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


/**
 * The setup code is concerned with point/line/triangle setup and
 * putting commands/data into the bins.
 */


#ifndef LP_SETUP_CONTEXT_H
#define LP_SETUP_CONTEXT_H

#include "lp_setup.h"
#include "lp_rast.h"
#include "lp_tile_soa.h"        /* for TILE_SIZE */
#include "lp_scene.h"

#include "draw/draw_vbuf.h"

#define LP_SETUP_NEW_FS          0x01
#define LP_SETUP_NEW_CONSTANTS   0x02
#define LP_SETUP_NEW_BLEND_COLOR 0x04
#define LP_SETUP_NEW_SCISSOR     0x08


struct lp_scene_queue;


/** Max number of scenes */
#define MAX_SCENES 2



/**
 * Point/line/triangle setup context.
 * Note: "stored" below indicates data which is stored in the bins,
 * not arbitrary malloc'd memory.
 *
 *
 * Subclass of vbuf_render, plugged directly into the draw module as
 * the rendering backend.
 */
struct setup_context
{
   struct vbuf_render base;

   struct vertex_info *vertex_info;
   uint prim;
   uint vertex_size;
   uint nr_vertices;
   uint vertex_buffer_size;
   void *vertex_buffer;

   /* Final pipeline stage for draw module.  Draw module should
    * create/install this itself now.
    */
   struct draw_stage *vbuf;
   struct lp_rasterizer *rast;
   struct lp_scene *scenes[MAX_SCENES];  /**< all the scenes */
   struct lp_scene *scene;               /**< current scene being built */
   struct lp_scene_queue *empty_scenes;  /**< queue of empty scenes */

   boolean flatshade_first;
   boolean ccw_is_frontface;
   boolean scissor_test;
   unsigned cullmode;

   struct pipe_framebuffer_state fb;

   struct {
      unsigned flags;
      union lp_rast_cmd_arg color;    /**< lp_rast_clear_color() cmd */
      union lp_rast_cmd_arg zstencil; /**< lp_rast_clear_zstencil() cmd */
   } clear;

   enum {
      SETUP_FLUSHED,
      SETUP_CLEARED,
      SETUP_ACTIVE
   } state;
   
   struct {
      struct lp_shader_input input[PIPE_MAX_ATTRIBS];
      unsigned nr_inputs;

      const struct lp_rast_state *stored; /**< what's in the scene */
      struct lp_rast_state current;  /**< currently set state */
   } fs;

   /** fragment shader constants */
   struct {
      struct pipe_buffer *current;
      unsigned stored_size;
      const void *stored_data;
   } constants;

   struct {
      struct pipe_blend_color current;
      uint8_t *stored;
   } blend_color;

   struct {
      struct pipe_scissor_state current;
      const void *stored;
   } scissor;

   unsigned dirty;   /**< bitmask of LP_SETUP_NEW_x bits */

   void (*point)( struct setup_context *,
                  const float (*v0)[4]);

   void (*line)( struct setup_context *,
                 const float (*v0)[4],
                 const float (*v1)[4]);

   void (*triangle)( struct setup_context *,
                     const float (*v0)[4],
                     const float (*v1)[4],
                     const float (*v2)[4]);
};

void lp_setup_choose_triangle( struct setup_context *setup );
void lp_setup_choose_line( struct setup_context *setup );
void lp_setup_choose_point( struct setup_context *setup );

struct lp_scene *lp_setup_get_current_scene(struct setup_context *setup);

void lp_setup_init_vbuf(struct setup_context *setup);

void lp_setup_update_state( struct setup_context *setup );

void lp_setup_destroy( struct setup_context *setup );

#endif
