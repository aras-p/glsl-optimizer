/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#ifndef ST_CONTEXT_H
#define ST_CONTEXT_H

#include "main/mtypes.h"
#include "shader/prog_cache.h"
#include "pipe/p_state.h"


struct st_context;
struct st_texture_object;
struct st_fragment_program;
struct draw_context;
struct draw_stage;
struct cso_cache;
struct cso_blend;
struct gen_mipmap_state;
struct blit_state;
struct bitmap_cache;


/** XXX we'd like to get rid of these */
#define FRONT_STATUS_UNDEFINED    0
#define FRONT_STATUS_DIRTY        1
#define FRONT_STATUS_COPY_OF_BACK 2


#define ST_NEW_MESA                    0x1 /* Mesa state has changed */
#define ST_NEW_FRAGMENT_PROGRAM        0x2
#define ST_NEW_VERTEX_PROGRAM          0x4
#define ST_NEW_FRAMEBUFFER             0x8
#define ST_NEW_EDGEFLAGS_DATA          0x10


struct st_state_flags {
   GLuint mesa;
   GLuint st;
};

struct st_tracked_state {
   const char *name;
   struct st_state_flags dirty;
   void (*update)( struct st_context *st );
};



struct st_context
{
   GLcontext *ctx;

   struct pipe_context *pipe;

   struct draw_context *draw;  /**< For selection/feedback/rastpos only */
   struct draw_stage *feedback_stage;  /**< For GL_FEEDBACK rendermode */
   struct draw_stage *selection_stage;  /**< For GL_SELECT rendermode */
   struct draw_stage *rastpos_stage;  /**< For glRasterPos */

   /* Some state is contained in constant objects.
    * Other state is just parameter values.
    */
   struct {
      struct pipe_blend_state               blend;
      struct pipe_depth_stencil_alpha_state depth_stencil;
      struct pipe_rasterizer_state          rasterizer;
      struct pipe_sampler_state             samplers[PIPE_MAX_SAMPLERS];
      struct pipe_sampler_state             *sampler_list[PIPE_MAX_SAMPLERS];
      struct pipe_clip_state clip;
      struct pipe_buffer *constants[2];
      struct pipe_framebuffer_state framebuffer;
      struct pipe_texture *sampler_texture[PIPE_MAX_SAMPLERS];
      struct pipe_scissor_state scissor;
      struct pipe_viewport_state viewport;

      GLuint num_samplers;
      GLuint num_textures;

      GLuint poly_stipple[32];  /**< In OpenGL's bottom-to-top order */
   } state;

   struct {
      struct st_tracked_state tracked_state[PIPE_SHADER_TYPES];
   } constants;

   /* XXX unused: */
   struct {
      struct gl_fragment_program *fragment_program;
   } cb;

   GLuint frontbuffer_status;  /**< one of FRONT_STATUS_ (XXX to be removed) */

   char vendor[100];
   char renderer[100];

   struct st_state_flags dirty;

   GLboolean missing_textures;
   GLboolean vertdata_edgeflags;

   /** Mapping from VERT_RESULT_x to post-transformed vertex slot */
   const GLuint *vertex_result_to_slot;

   struct st_vertex_program *vp;    /**< Currently bound vertex program */
   struct st_fragment_program *fp;  /**< Currently bound fragment program */

   struct st_vp_varient *vp_varient;

   struct gl_texture_object *default_texture;

   struct {
      struct gl_program_cache *cache;
      struct st_fragment_program *program;  /**< cur pixel transfer prog */
      GLuint xfer_prog_sn;  /**< pixel xfer program serial no. */
      GLuint user_prog_sn;  /**< user fragment program serial no. */
      struct st_fragment_program *combined_prog;
      GLuint combined_prog_sn;
      struct pipe_texture *pixelmap_texture;
      boolean pixelmap_enabled;  /**< use the pixelmap texture? */
   } pixel_xfer;

   /** for glBitmap */
   struct {
      struct pipe_rasterizer_state rasterizer;
      struct pipe_sampler_state sampler;
      enum pipe_format tex_format;
      void *vs;
      float vertices[4][3][4];  /**< vertex pos + color + texcoord */
      struct pipe_buffer *vbuf;
      unsigned vbuf_slot;       /* next free slot in vbuf */
      struct bitmap_cache *cache;
   } bitmap;

   /** for glDraw/CopyPixels */
   struct {
      struct st_fragment_program *z_shader;
      void *vert_shaders[2];   /**< ureg shaders */
   } drawpix;

   /** for glClear */
   struct {
      struct pipe_rasterizer_state raster;
      struct pipe_viewport_state viewport;
      struct pipe_clip_state clip;
      void *vs;
      void *fs;
      float vertices[4][2][4];  /**< vertex pos + color */
      struct pipe_buffer *vbuf;
      unsigned vbuf_slot;
   } clear;

   void *passthrough_fs;  /**< simple pass-through frag shader */

   struct gen_mipmap_state *gen_mipmap;
   struct blit_state *blit;

   struct cso_context *cso_context;

   int force_msaa;
};


/* Need this so that we can implement Mesa callbacks in this module.
 */
static INLINE struct st_context *st_context(GLcontext *ctx)
{
   return ctx->st;
}


/**
 * Wrapper for GLframebuffer.
 * This is an opaque type to the outside world.
 */
struct st_framebuffer
{
   GLframebuffer Base;
   void *Private;
   GLuint InitWidth, InitHeight;
};


extern void st_init_driver_functions(struct dd_function_table *functions);

void st_invalidate_state(GLcontext * ctx, GLuint new_state);



#define Y_0_TOP 1
#define Y_0_BOTTOM 2

static INLINE GLuint
st_fb_orientation(const struct gl_framebuffer *fb)
{
   if (fb && fb->Name == 0) {
      /* Drawing into a window (on-screen buffer).
       *
       * Negate Y scale to flip image vertically.
       * The NDC Y coords prior to viewport transformation are in the range
       * [y=-1=bottom, y=1=top]
       * Hardware window coords are in the range [y=0=top, y=H-1=bottom] where
       * H is the window height.
       * Use the viewport transformation to invert Y.
       */
      return Y_0_TOP;
   }
   else {
      /* Drawing into user-created FBO (very likely a texture).
       *
       * For textures, T=0=Bottom, so by extension Y=0=Bottom for rendering.
       */
      return Y_0_BOTTOM;
   }
}


/** clear-alloc a struct-sized object, with casting */
#define ST_CALLOC_STRUCT(T)   (struct T *) calloc(1, sizeof(struct T))


extern int
st_get_msaa(void);


#endif
