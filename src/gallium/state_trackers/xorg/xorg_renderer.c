#include "xorg_exa.h"
#include "xorg_renderer.h"

#include "xorg_exa_tgsi.h"

#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_rect.h"

#include "pipe/p_inlines.h"

#include <math.h>

enum AxisOrientation {
   Y0_BOTTOM,
   Y0_TOP
};

#define floatsEqual(x, y) (fabs(x - y) <= 0.00001f * MIN2(fabs(x), fabs(y)))
#define floatIsZero(x) (floatsEqual((x) + 1, 1))

#define NUM_COMPONENTS 4

static INLINE boolean is_affine(float *matrix)
{
   return floatIsZero(matrix[2]) && floatIsZero(matrix[5])
      && floatsEqual(matrix[8], 1);
}
static INLINE void map_point(float *mat, float x, float y,
                             float *out_x, float *out_y)
{
   if (!mat) {
      *out_x = x;
      *out_y = y;
      return;
   }

   *out_x = mat[0]*x + mat[3]*y + mat[6];
   *out_y = mat[1]*x + mat[4]*y + mat[7];
   if (!is_affine(mat)) {
      float w = 1/(mat[2]*x + mat[5]*y + mat[8]);
      *out_x *= w;
      *out_y *= w;
   }
}

static INLINE struct pipe_buffer *
renderer_buffer_create(struct xorg_renderer *r)
{
   struct pipe_buffer *buf =
      pipe_user_buffer_create(r->pipe->screen,
                              r->vertices,
                              sizeof(float)*
                              r->num_vertices);
   r->num_vertices = 0;

   return buf;
}

static INLINE void
renderer_draw(struct xorg_renderer *r)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_buffer *buf = 0;
   int num_verts = r->num_vertices/(r->num_attributes * NUM_COMPONENTS);

   if (!r->num_vertices)
      return;

   buf = renderer_buffer_create(r);


   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_QUADS,
                              num_verts,  /* verts */
                              r->num_attributes); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
}

static INLINE void
renderer_draw_conditional(struct xorg_renderer *r,
                          int next_batch)
{
   if (r->num_vertices + next_batch >= BUF_SIZE ||
       (next_batch == 0 && r->num_vertices)) {
      renderer_draw(r);
   }
}

static void
renderer_init_state(struct xorg_renderer *r)
{
   struct pipe_depth_stencil_alpha_state dsa;

   /* set common initial clip state */
   memset(&dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));
   cso_set_depth_stencil_alpha(r->cso, &dsa);
}


static INLINE void
add_vertex_color(struct xorg_renderer *r,
                 float x, float y,
                 float color[4])
{
   float *vertex = r->vertices + r->num_vertices;

   vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f; /*z*/
   vertex[3] = 1.f; /*w*/

   vertex[4] = color[0]; /*r*/
   vertex[5] = color[1]; /*g*/
   vertex[6] = color[2]; /*b*/
   vertex[7] = color[3]; /*a*/

   r->num_vertices += 8;
}

static INLINE void
add_vertex_1tex(struct xorg_renderer *r,
                float x, float y, float s, float t)
{
   float *vertex = r->vertices + r->num_vertices;

   vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f; /*z*/
   vertex[3] = 1.f; /*w*/

   vertex[4] = s;   /*s*/
   vertex[5] = t;   /*t*/
   vertex[6] = 0.f; /*r*/
   vertex[7] = 1.f; /*q*/

   r->num_vertices += 8;
}

static void
add_vertex_data1(struct xorg_renderer *r,
                 float srcX, float srcY,  float dstX, float dstY,
                 float width, float height,
                 struct pipe_texture *src, float *src_matrix)
{
   float s0, t0, s1, t1;
   float pt0[2], pt1[2];

   pt0[0] = srcX;
   pt0[1] = srcY;
   pt1[0] = (srcX + width);
   pt1[1] = (srcY + height);

   if (src_matrix) {
      map_point(src_matrix, pt0[0], pt0[1], &pt0[0], &pt0[1]);
      map_point(src_matrix, pt1[0], pt1[1], &pt1[0], &pt1[1]);
   }

   s0 =  pt0[0] / src->width[0];
   s1 =  pt1[0] / src->width[0];
   t0 =  pt0[1] / src->height[0];
   t1 =  pt1[1] / src->height[0];

   /* 1st vertex */
   add_vertex_1tex(r, dstX, dstY, s0, t0);
   /* 2nd vertex */
   add_vertex_1tex(r, dstX + width, dstY, s1, t0);
   /* 3rd vertex */
   add_vertex_1tex(r, dstX + width, dstY + height, s1, t1);
   /* 4th vertex */
   add_vertex_1tex(r, dstX, dstY + height, s0, t1);
}

static struct pipe_buffer *
setup_vertex_data_tex(struct xorg_renderer *r,
                      float x0, float y0, float x1, float y1,
                      float s0, float t0, float s1, float t1,
                      float z)
{
   /* 1st vertex */
   add_vertex_1tex(r, x0, y0, s0, t0);
   /* 2nd vertex */
   add_vertex_1tex(r, x1, y0, s1, t0);
   /* 3rd vertex */
   add_vertex_1tex(r, x1, y1, s1, t1);
   /* 4th vertex */
   add_vertex_1tex(r, x0, y1, s0, t1);

   return renderer_buffer_create(r);
}

static INLINE void
add_vertex_2tex(struct xorg_renderer *r,
                float x, float y,
                float s0, float t0, float s1, float t1)
{
   float *vertex = r->vertices + r->num_vertices;

   vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f; /*z*/
   vertex[3] = 1.f; /*w*/

   vertex[4] = s0;  /*s*/
   vertex[5] = t0;  /*t*/
   vertex[6] = 0.f; /*r*/
   vertex[7] = 1.f; /*q*/

   vertex[8] = s1;  /*s*/
   vertex[9] = t1;  /*t*/
   vertex[10] = 0.f; /*r*/
   vertex[11] = 1.f; /*q*/

   r->num_vertices += 12;
}

static void
add_vertex_data2(struct xorg_renderer *r,
                 float srcX, float srcY, float maskX, float maskY,
                 float dstX, float dstY, float width, float height,
                 struct pipe_texture *src,
                 struct pipe_texture *mask,
                 float *src_matrix, float *mask_matrix)
{
   float src_s0, src_t0, src_s1, src_t1;
   float mask_s0, mask_t0, mask_s1, mask_t1;
   float spt0[2], spt1[2];
   float mpt0[2], mpt1[2];

   spt0[0] = srcX;
   spt0[1] = srcY;
   spt1[0] = srcX + width;
   spt1[1] = srcY + height;

   mpt0[0] = maskX;
   mpt0[1] = maskY;
   mpt1[0] = maskX + width;
   mpt1[1] = maskY + height;

   if (src_matrix) {
      map_point(src_matrix, spt0[0], spt0[1], &spt0[0], &spt0[1]);
      map_point(src_matrix, spt1[0], spt1[1], &spt1[0], &spt1[1]);
   }

   if (mask_matrix) {
      map_point(mask_matrix, mpt0[0], mpt0[1], &mpt0[0], &mpt0[1]);
      map_point(mask_matrix, mpt1[0], mpt1[1], &mpt1[0], &mpt1[1]);
   }

   src_s0 = spt0[0] / src->width[0];
   src_t0 = spt0[1] / src->height[0];
   src_s1 = spt1[0] / src->width[0];
   src_t1 = spt1[1] / src->height[0];

   mask_s0 = mpt0[0] / mask->width[0];
   mask_t0 = mpt0[1] / mask->height[0];
   mask_s1 = mpt1[0] / mask->width[0];
   mask_t1 = mpt1[1] / mask->height[0];

   /* 1st vertex */
   add_vertex_2tex(r, dstX, dstY,
                   src_s0, src_t0, mask_s0, mask_t0);
   /* 2nd vertex */
   add_vertex_2tex(r, dstX + width, dstY,
                   src_s1, src_t0, mask_s1, mask_t0);
   /* 3rd vertex */
   add_vertex_2tex(r, dstX + width, dstY + height,
                   src_s1, src_t1, mask_s1, mask_t1);
   /* 4th vertex */
   add_vertex_2tex(r, dstX, dstY + height,
                   src_s0, src_t1, mask_s0, mask_t1);
}

static struct pipe_buffer *
setup_vertex_data_yuv(struct xorg_renderer *r,
                      float srcX, float srcY, float srcW, float srcH,
                      float dstX, float dstY, float dstW, float dstH,
                      struct pipe_texture **tex)
{
   float s0, t0, s1, t1;
   float spt0[2], spt1[2];

   spt0[0] = srcX;
   spt0[1] = srcY;
   spt1[0] = srcX + srcW;
   spt1[1] = srcY + srcH;

   s0 = spt0[0] / tex[0]->width[0];
   t0 = spt0[1] / tex[0]->height[0];
   s1 = spt1[0] / tex[0]->width[0];
   t1 = spt1[1] / tex[0]->height[0];

   /* 1st vertex */
   add_vertex_1tex(r, dstX, dstY, s0, t0);
   /* 2nd vertex */
   add_vertex_1tex(r, dstX + dstW, dstY,
                   s1, t0);
   /* 3rd vertex */
   add_vertex_1tex(r, dstX + dstW, dstY + dstH,
                   s1, t1);
   /* 4th vertex */
   add_vertex_1tex(r, dstX, dstY + dstH,
                   s0, t1);

   return renderer_buffer_create(r);
}



static void
set_viewport(struct xorg_renderer *r, int width, int height,
             enum AxisOrientation orientation)
{
   struct pipe_viewport_state viewport;
   float y_scale = (orientation == Y0_BOTTOM) ? -2.f : 2.f;

   viewport.scale[0] =  width / 2.f;
   viewport.scale[1] =  height / y_scale;
   viewport.scale[2] =  1.0;
   viewport.scale[3] =  1.0;
   viewport.translate[0] = width / 2.f;
   viewport.translate[1] = height / 2.f;
   viewport.translate[2] = 0.0;
   viewport.translate[3] = 0.0;

   cso_set_viewport(r->cso, &viewport);
}



struct xorg_renderer * renderer_create(struct pipe_context *pipe)
{
   struct xorg_renderer *renderer = CALLOC_STRUCT(xorg_renderer);

   renderer->pipe = pipe;
   renderer->cso = cso_create_context(pipe);
   renderer->shaders = xorg_shaders_create(renderer);

   renderer_init_state(renderer);

   return renderer;
}

void renderer_destroy(struct xorg_renderer *r)
{
   struct pipe_constant_buffer *vsbuf = &r->vs_const_buffer;
   struct pipe_constant_buffer *fsbuf = &r->fs_const_buffer;

   if (vsbuf && vsbuf->buffer)
      pipe_buffer_reference(&vsbuf->buffer, NULL);

   if (fsbuf && fsbuf->buffer)
      pipe_buffer_reference(&fsbuf->buffer, NULL);

   if (r->shaders) {
      xorg_shaders_destroy(r->shaders);
      r->shaders = NULL;
   }

   if (r->cso) {
      cso_release_all(r->cso);
      cso_destroy_context(r->cso);
      r->cso = NULL;
   }
}

void renderer_bind_framebuffer(struct xorg_renderer *r,
                               struct exa_pixmap_priv *priv)
{
   unsigned i;
   struct pipe_framebuffer_state state;
   struct pipe_surface *surface = xorg_gpu_surface(r->pipe->screen, priv);
   memset(&state, 0, sizeof(struct pipe_framebuffer_state));

   state.width  = priv->tex->width[0];
   state.height = priv->tex->height[0];

   state.nr_cbufs = 1;
   state.cbufs[0] = surface;
   for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
      state.cbufs[i] = 0;

   /* currently we don't use depth/stencil */
   state.zsbuf = 0;

   cso_set_framebuffer(r->cso, &state);

   /* we do fire and forget for the framebuffer, this is the forget part */
   pipe_surface_reference(&surface, NULL);
}

void renderer_bind_viewport(struct xorg_renderer *r,
                            struct exa_pixmap_priv *dst)
{
   int width = dst->tex->width[0];
   int height = dst->tex->height[0];

   /*debug_printf("Bind viewport (%d, %d)\n", width, height);*/

   set_viewport(r, width, height, Y0_TOP);
}

void renderer_bind_rasterizer(struct xorg_renderer *r)
{
   struct pipe_rasterizer_state raster;

   /* XXX: move to renderer_init_state? */
   memset(&raster, 0, sizeof(struct pipe_rasterizer_state));
   raster.gl_rasterization_rules = 1;
   cso_set_rasterizer(r->cso, &raster);
}

void renderer_set_constants(struct xorg_renderer *r,
                            int shader_type,
                            const float *params,
                            int param_bytes)
{
   struct pipe_constant_buffer *cbuf =
      (shader_type == PIPE_SHADER_VERTEX) ? &r->vs_const_buffer :
      &r->fs_const_buffer;

   pipe_buffer_reference(&cbuf->buffer, NULL);
   cbuf->buffer = pipe_buffer_create(r->pipe->screen, 16,
                                     PIPE_BUFFER_USAGE_CONSTANT,
                                     param_bytes);

   if (cbuf->buffer) {
      pipe_buffer_write(r->pipe->screen, cbuf->buffer,
                        0, param_bytes, params);
   }
   r->pipe->set_constant_buffer(r->pipe, shader_type, 0, cbuf);
}

static void
setup_vs_constant_buffer(struct xorg_renderer *r,
                         int width, int height)
{
   const int param_bytes = 8 * sizeof(float);
   float vs_consts[8] = {
      2.f/width, 2.f/height, 1, 1,
      -1, -1, 0, 0
   };
   renderer_set_constants(r, PIPE_SHADER_VERTEX,
                          vs_consts, param_bytes);
}

static void
setup_fs_constant_buffer(struct xorg_renderer *r)
{
   const int param_bytes = 4 * sizeof(float);
   const float fs_consts[8] = {
      0, 0, 0, 1,
   };
   renderer_set_constants(r, PIPE_SHADER_FRAGMENT,
                          fs_consts, param_bytes);
}

static INLINE void shift_rectx(float coords[4],
                               const float *bounds,
                               const float shift)
{
   coords[0] += shift;
   coords[2] -= shift;
   if (bounds) {
      coords[2] = MIN2(coords[2], bounds[2]);
      /* bound x/y + width/height */
      if ((coords[0] + coords[2]) > (bounds[0] + bounds[2])) {
         coords[2] = (bounds[0] + bounds[2]) - coords[0];
      }
   }
}

static INLINE void shift_recty(float coords[4],
                               const float *bounds,
                               const float shift)
{
   coords[1] += shift;
   coords[3] -= shift;
   if (bounds) {
      coords[3] = MIN2(coords[3], bounds[3]);
      if ((coords[1] + coords[3]) > (bounds[1] + bounds[3])) {
         coords[3] = (bounds[1] + bounds[3]) - coords[1];
      }
   }
}

static INLINE void bound_rect(float coords[4],
                              const float bounds[4],
                              float shift[4])
{
   /* if outside the bounds */
   if (coords[0] > (bounds[0] + bounds[2]) ||
       coords[1] > (bounds[1] + bounds[3]) ||
       (coords[0] + coords[2]) < bounds[0] ||
       (coords[1] + coords[3]) < bounds[1]) {
      coords[0] = 0.f;
      coords[1] = 0.f;
      coords[2] = 0.f;
      coords[3] = 0.f;
      shift[0] = 0.f;
      shift[1] = 0.f;
      return;
   }

   /* bound x */
   if (coords[0] < bounds[0]) {
      shift[0] = bounds[0] - coords[0];
      coords[2] -= shift[0];
      coords[0] = bounds[0];
   } else
      shift[0] = 0.f;

   /* bound y */
   if (coords[1] < bounds[1]) {
      shift[1] = bounds[1] - coords[1];
      coords[3] -= shift[1];
      coords[1] = bounds[1];
   } else
      shift[1] = 0.f;

   shift[2] = bounds[2] - coords[2];
   shift[3] = bounds[3] - coords[3];
   /* bound width/height */
   coords[2] = MIN2(coords[2], bounds[2]);
   coords[3] = MIN2(coords[3], bounds[3]);

   /* bound x/y + width/height */
   if ((coords[0] + coords[2]) > (bounds[0] + bounds[2])) {
      coords[2] = (bounds[0] + bounds[2]) - coords[0];
   }
   if ((coords[1] + coords[3]) > (bounds[1] + bounds[3])) {
      coords[3] = (bounds[1] + bounds[3]) - coords[1];
   }

   /* if outside the bounds */
   if ((coords[0] + coords[2]) < bounds[0] ||
       (coords[1] + coords[3]) < bounds[1]) {
      coords[0] = 0.f;
      coords[1] = 0.f;
      coords[2] = 0.f;
      coords[3] = 0.f;
      return;
   }
}

static INLINE void sync_size(float *src_loc, float *dst_loc)
{
   src_loc[2] = MIN2(src_loc[2], dst_loc[2]);
   src_loc[3] = MIN2(src_loc[3], dst_loc[3]);
   dst_loc[2] = src_loc[2];
   dst_loc[3] = src_loc[3];
}

static void renderer_copy_texture(struct xorg_renderer *r,
                                  struct pipe_texture *src,
                                  float sx1, float sy1,
                                  float sx2, float sy2,
                                  struct pipe_texture *dst,
                                  float dx1, float dy1,
                                  float dx2, float dy2)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_buffer *buf;
   struct pipe_surface *dst_surf = screen->get_tex_surface(
      screen, dst, 0, 0, 0,
      PIPE_BUFFER_USAGE_GPU_WRITE);
   struct pipe_framebuffer_state fb;
   float s0, t0, s1, t1;
   struct xorg_shader shader;

   assert(src->width[0] != 0);
   assert(src->height[0] != 0);
   assert(dst->width[0] != 0);
   assert(dst->height[0] != 0);

#if 1
   s0 = sx1 / src->width[0];
   s1 = sx2 / src->width[0];
   t0 = sy1 / src->height[0];
   t1 = sy2 / src->height[0];
#else
   s0 = 0;
   s1 = 1;
   t0 = 0;
   t1 = 1;
#endif

#if 0
   debug_printf("copy texture src=[%f, %f, %f, %f], dst=[%f, %f, %f, %f], tex=[%f, %f, %f, %f]\n",
                sx1, sy1, sx2, sy2, dx1, dy1, dx2, dy2,
                s0, t0, s1, t1);
#endif

   assert(screen->is_format_supported(screen, dst_surf->format,
                                      PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET,
                                      0));

   /* save state (restored below) */
   cso_save_blend(r->cso);
   cso_save_samplers(r->cso);
   cso_save_sampler_textures(r->cso);
   cso_save_framebuffer(r->cso);
   cso_save_fragment_shader(r->cso);
   cso_save_vertex_shader(r->cso);

   cso_save_viewport(r->cso);


   /* set misc state we care about */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.colormask = PIPE_MASK_RGBA;
      cso_set_blend(r->cso, &blend);
   }

   /* sampler */
   {
      struct pipe_sampler_state sampler;
      memset(&sampler, 0, sizeof(sampler));
      sampler.wrap_s = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_t = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.wrap_r = PIPE_TEX_WRAP_CLAMP_TO_EDGE;
      sampler.min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
      sampler.min_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.mag_img_filter = PIPE_TEX_FILTER_NEAREST;
      sampler.normalized_coords = 1;
      cso_single_sampler(r->cso, 0, &sampler);
      cso_single_sampler_done(r->cso);
   }

   set_viewport(r, dst_surf->width, dst_surf->height, Y0_TOP);

   /* texture */
   cso_set_sampler_textures(r->cso, 1, &src);

   renderer_bind_rasterizer(r);

   /* shaders */
   shader = xorg_shaders_get(r->shaders,
                             VS_COMPOSITE,
                             FS_COMPOSITE);
   cso_set_vertex_shader_handle(r->cso, shader.vs);
   cso_set_fragment_shader_handle(r->cso, shader.fs);

   /* drawing dest */
   memset(&fb, 0, sizeof(fb));
   fb.width = dst_surf->width;
   fb.height = dst_surf->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = dst_surf;
   {
      int i;
      for (i = 1; i < PIPE_MAX_COLOR_BUFS; ++i)
         fb.cbufs[i] = 0;
   }
   cso_set_framebuffer(r->cso, &fb);
   setup_vs_constant_buffer(r, fb.width, fb.height);
   setup_fs_constant_buffer(r);

   /* draw quad */
   buf = setup_vertex_data_tex(r,
                               dx1, dy1,
                               dx2, dy2,
                               s0, t0, s1, t1,
                               0.0f);

   if (buf) {
      util_draw_vertex_buffer(r->pipe, buf, 0,
                              PIPE_PRIM_QUADS,
                              4,  /* verts */
                              2); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }

   /* restore state we changed */
   cso_restore_blend(r->cso);
   cso_restore_samplers(r->cso);
   cso_restore_sampler_textures(r->cso);
   cso_restore_framebuffer(r->cso);
   cso_restore_vertex_shader(r->cso);
   cso_restore_fragment_shader(r->cso);
   cso_restore_viewport(r->cso);

   pipe_surface_reference(&dst_surf, NULL);
}

static struct pipe_texture *
create_sampler_texture(struct xorg_renderer *r,
                       struct pipe_texture *src)
{
   enum pipe_format format;
   struct pipe_context *pipe = r->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *pt;
   struct pipe_texture templ;

   pipe->flush(pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   /* the coming in texture should already have that invariance */
   debug_assert(screen->is_format_supported(screen, src->format,
                                            PIPE_TEXTURE_2D,
                                            PIPE_TEXTURE_USAGE_SAMPLER, 0));

   format = src->format;

   memset(&templ, 0, sizeof(templ));
   templ.target = PIPE_TEXTURE_2D;
   templ.format = format;
   templ.last_level = 0;
   templ.width[0] = src->width[0];
   templ.height[0] = src->height[0];
   templ.depth[0] = 1;
   pf_get_block(format, &templ.block);
   templ.tex_usage = PIPE_TEXTURE_USAGE_SAMPLER;

   pt = screen->texture_create(screen, &templ);

   debug_assert(!pt || pipe_is_referenced(&pt->reference));

   if (!pt)
      return NULL;

   {
      /* copy source framebuffer surface into texture */
      struct pipe_surface *ps_read = screen->get_tex_surface(
         screen, src, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_READ);
      struct pipe_surface *ps_tex = screen->get_tex_surface(
         screen, pt, 0, 0, 0, PIPE_BUFFER_USAGE_GPU_WRITE );
      if (pipe->surface_copy) {
         pipe->surface_copy(pipe,
                ps_tex, /* dest */
                0, 0, /* destx/y */
                ps_read,
                0, 0, src->width[0], src->height[0]);
      } else {
          util_surface_copy(pipe, FALSE,
                ps_tex, /* dest */
                0, 0, /* destx/y */
                ps_read,
                0, 0, src->width[0], src->height[0]);
      }
      pipe_surface_reference(&ps_read, NULL);
      pipe_surface_reference(&ps_tex, NULL);
   }

   return pt;
}


void renderer_copy_pixmap(struct xorg_renderer *r,
                          struct exa_pixmap_priv *dst_priv, int dx, int dy,
                          struct exa_pixmap_priv *src_priv, int sx, int sy,
                          int width, int height)
{
   float dst_loc[4], src_loc[4];
   float dst_bounds[4], src_bounds[4];
   float src_shift[4], dst_shift[4], shift[4];
   struct pipe_texture *dst = dst_priv->tex;
   struct pipe_texture *src = src_priv->tex;

   if (r->pipe->is_texture_referenced(r->pipe, src, 0, 0) &
       PIPE_REFERENCED_FOR_WRITE)
      r->pipe->flush(r->pipe, PIPE_FLUSH_RENDER_CACHE, NULL);

   dst_loc[0] = dx;
   dst_loc[1] = dy;
   dst_loc[2] = width;
   dst_loc[3] = height;
   dst_bounds[0] = 0.f;
   dst_bounds[1] = 0.f;
   dst_bounds[2] = dst->width[0];
   dst_bounds[3] = dst->height[0];

   src_loc[0] = sx;
   src_loc[1] = sy;
   src_loc[2] = width;
   src_loc[3] = height;
   src_bounds[0] = 0.f;
   src_bounds[1] = 0.f;
   src_bounds[2] = src->width[0];
   src_bounds[3] = src->height[0];

   bound_rect(src_loc, src_bounds, src_shift);
   bound_rect(dst_loc, dst_bounds, dst_shift);
   shift[0] = src_shift[0] - dst_shift[0];
   shift[1] = src_shift[1] - dst_shift[1];

   if (shift[0] < 0)
      shift_rectx(src_loc, src_bounds, -shift[0]);
   else
      shift_rectx(dst_loc, dst_bounds, shift[0]);

   if (shift[1] < 0)
      shift_recty(src_loc, src_bounds, -shift[1]);
   else
      shift_recty(dst_loc, dst_bounds, shift[1]);

   sync_size(src_loc, dst_loc);

   if (src_loc[2] >= 0 && src_loc[3] >= 0 &&
       dst_loc[2] >= 0 && dst_loc[3] >= 0) {
      struct pipe_texture *temp_src = src;

      if (src == dst)
         temp_src = create_sampler_texture(r, src);

      renderer_copy_texture(r,
                            temp_src,
                            src_loc[0],
                            src_loc[1],
                            src_loc[0] + src_loc[2],
                            src_loc[1] + src_loc[3],
                            dst,
                            dst_loc[0],
                            dst_loc[1],
                            dst_loc[0] + dst_loc[2],
                            dst_loc[1] + dst_loc[3]);

      if (src == dst)
         pipe_texture_reference(&temp_src, NULL);
   }
}

void renderer_draw_yuv(struct xorg_renderer *r,
                       int src_x, int src_y, int src_w, int src_h,
                       int dst_x, int dst_y, int dst_w, int dst_h,
                       struct pipe_texture **textures)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_buffer *buf = 0;

   buf = setup_vertex_data_yuv(r,
                               src_x, src_y, src_w, src_h,
                               dst_x, dst_y, dst_w, dst_h,
                               textures);

   if (buf) {
      const int num_attribs = 2; /*pos + tex coord*/

      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_QUADS,
                              4,  /* verts */
                              num_attribs); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
}

void renderer_begin_solid(struct xorg_renderer *r)
{
   r->num_vertices = 0;
   r->num_attributes = 2;
}

void renderer_solid(struct xorg_renderer *r,
                    int x0, int y0,
                    int x1, int y1,
                    float *color)
{
   /*
   debug_printf("solid rect[(%d, %d), (%d, %d)], rgba[%f, %f, %f, %f]\n",
   x0, y0, x1, y1, color[0], color[1], color[2], color[3]);*/

   renderer_draw_conditional(r, 4 * 8);

   /* 1st vertex */
   add_vertex_color(r, x0, y0, color);
   /* 2nd vertex */
   add_vertex_color(r, x1, y0, color);
   /* 3rd vertex */
   add_vertex_color(r, x1, y1, color);
   /* 4th vertex */
   add_vertex_color(r, x0, y1, color);
}

void renderer_draw_flush(struct xorg_renderer *r)
{
   renderer_draw_conditional(r, 0);
}

void renderer_begin_textures(struct xorg_renderer *r,
                             struct pipe_texture **textures,
                             int num_textures)
{
   r->num_attributes = 1 + num_textures;
   r->num_vertices = 0;
}

void renderer_texture(struct xorg_renderer *r,
                      int *pos,
                      int width, int height,
                      struct pipe_texture **textures,
                      int num_textures,
                      float *src_matrix,
                      float *mask_matrix)
{

#if 0
   if (src_matrix) {
      debug_printf("src_matrix = \n");
      debug_printf("%f, %f, %f\n", src_matrix[0], src_matrix[1], src_matrix[2]);
      debug_printf("%f, %f, %f\n", src_matrix[3], src_matrix[4], src_matrix[5]);
      debug_printf("%f, %f, %f\n", src_matrix[6], src_matrix[7], src_matrix[8]);
   }
   if (mask_matrix) {
      debug_printf("mask_matrix = \n");
      debug_printf("%f, %f, %f\n", mask_matrix[0], mask_matrix[1], mask_matrix[2]);
      debug_printf("%f, %f, %f\n", mask_matrix[3], mask_matrix[4], mask_matrix[5]);
      debug_printf("%f, %f, %f\n", mask_matrix[6], mask_matrix[7], mask_matrix[8]);
   }
#endif

   switch(r->num_attributes) {
   case 2:
      renderer_draw_conditional(r, 4 * 8);
      add_vertex_data1(r,
                       pos[0], pos[1], /* src */
                       pos[4], pos[5], /* dst */
                       width, height,
                       textures[0], src_matrix);
      break;
   case 3:
      renderer_draw_conditional(r, 4 * 12);
      add_vertex_data2(r,
                       pos[0], pos[1], /* src */
                       pos[2], pos[3], /* mask */
                       pos[4], pos[5], /* dst */
                       width, height,
                       textures[0], textures[1],
                       src_matrix, mask_matrix);
      break;
   default:
      debug_assert(!"Unsupported number of textures");
      break;
   }
}
