#include "xorg_exa.h"
#include "xorg_renderer.h"

#include "xorg_exa_tgsi.h"

#include "cso_cache/cso_context.h"
#include "util/u_draw_quad.h"
#include "util/u_math.h"
#include "util/u_memory.h"
#include "util/u_rect.h"

#include "util/u_inlines.h"

#include <math.h>

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
                              r->buffer,
                              sizeof(float)*
                              r->buffer_size);
   r->buffer_size = 0;

   return buf;
}

static INLINE void
renderer_draw(struct xorg_renderer *r)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_buffer *buf = 0;
   int num_verts = r->buffer_size/(r->attrs_per_vertex * NUM_COMPONENTS);

   if (!r->buffer_size)
      return;

   buf = renderer_buffer_create(r);


   if (buf) {
      util_draw_vertex_buffer(pipe, buf, 0,
                              PIPE_PRIM_QUADS,
                              num_verts,  /* verts */
                              r->attrs_per_vertex); /* attribs/vert */

      pipe_buffer_reference(&buf, NULL);
   }
}

static INLINE void
renderer_draw_conditional(struct xorg_renderer *r,
                          int next_batch)
{
   if (r->buffer_size + next_batch >= BUF_SIZE ||
       (next_batch == 0 && r->buffer_size)) {
      renderer_draw(r);
   }
}

static void
renderer_init_state(struct xorg_renderer *r)
{
   struct pipe_depth_stencil_alpha_state dsa;
   struct pipe_rasterizer_state raster;

   /* set common initial clip state */
   memset(&dsa, 0, sizeof(struct pipe_depth_stencil_alpha_state));
   cso_set_depth_stencil_alpha(r->cso, &dsa);


   /* XXX: move to renderer_init_state? */
   memset(&raster, 0, sizeof(struct pipe_rasterizer_state));
   raster.gl_rasterization_rules = 1;
   cso_set_rasterizer(r->cso, &raster);

}


static INLINE void
add_vertex_color(struct xorg_renderer *r,
                 float x, float y,
                 float color[4])
{
   float *vertex = r->buffer + r->buffer_size;

   vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f; /*z*/
   vertex[3] = 1.f; /*w*/

   vertex[4] = color[0]; /*r*/
   vertex[5] = color[1]; /*g*/
   vertex[6] = color[2]; /*b*/
   vertex[7] = color[3]; /*a*/

   r->buffer_size += 8;
}

static INLINE void
add_vertex_1tex(struct xorg_renderer *r,
                float x, float y, float s, float t)
{
   float *vertex = r->buffer + r->buffer_size;

   vertex[0] = x;
   vertex[1] = y;
   vertex[2] = 0.f; /*z*/
   vertex[3] = 1.f; /*w*/

   vertex[4] = s;   /*s*/
   vertex[5] = t;   /*t*/
   vertex[6] = 0.f; /*r*/
   vertex[7] = 1.f; /*q*/

   r->buffer_size += 8;
}

static void
add_vertex_data1(struct xorg_renderer *r,
                 float srcX, float srcY,  float dstX, float dstY,
                 float width, float height,
                 struct pipe_texture *src, float *src_matrix)
{
   float s0, t0, s1, t1, s2, t2, s3, t3;
   float pt0[2], pt1[2], pt2[2], pt3[2];

   pt0[0] = srcX;
   pt0[1] = srcY;
   pt1[0] = (srcX + width);
   pt1[1] = srcY;
   pt2[0] = (srcX + width);
   pt2[1] = (srcY + height);
   pt3[0] = srcX;
   pt3[1] = (srcY + height);

   if (src_matrix) {
      map_point(src_matrix, pt0[0], pt0[1], &pt0[0], &pt0[1]);
      map_point(src_matrix, pt1[0], pt1[1], &pt1[0], &pt1[1]);
      map_point(src_matrix, pt2[0], pt2[1], &pt2[0], &pt2[1]);
      map_point(src_matrix, pt3[0], pt3[1], &pt3[0], &pt3[1]);
   }

   s0 =  pt0[0] / src->width0;
   s1 =  pt1[0] / src->width0;
   s2 =  pt2[0] / src->width0;
   s3 =  pt3[0] / src->width0;
   t0 =  pt0[1] / src->height0;
   t1 =  pt1[1] / src->height0;
   t2 =  pt2[1] / src->height0;
   t3 =  pt3[1] / src->height0;

   /* 1st vertex */
   add_vertex_1tex(r, dstX, dstY, s0, t0);
   /* 2nd vertex */
   add_vertex_1tex(r, dstX + width, dstY, s1, t1);
   /* 3rd vertex */
   add_vertex_1tex(r, dstX + width, dstY + height, s2, t2);
   /* 4th vertex */
   add_vertex_1tex(r, dstX, dstY + height, s3, t3);
}


static INLINE void
add_vertex_2tex(struct xorg_renderer *r,
                float x, float y,
                float s0, float t0, float s1, float t1)
{
   float *vertex = r->buffer + r->buffer_size;

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

   r->buffer_size += 12;
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

   src_s0 = spt0[0] / src->width0;
   src_t0 = spt0[1] / src->height0;
   src_s1 = spt1[0] / src->width0;
   src_t1 = spt1[1] / src->height0;

   mask_s0 = mpt0[0] / mask->width0;
   mask_t0 = mpt0[1] / mask->height0;
   mask_s1 = mpt1[0] / mask->width0;
   mask_t1 = mpt1[1] / mask->height0;

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

   s0 = spt0[0] / tex[0]->width0;
   t0 = spt0[1] / tex[0]->height0;
   s1 = spt1[0] / tex[0]->width0;
   t1 = spt1[1] / tex[0]->height0;

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



/* Set up framebuffer, viewport and vertex shader constant buffer
 * state for a particular destinaton surface.  In all our rendering,
 * these concepts are linked.
 */
void renderer_bind_destination(struct xorg_renderer *r,
                               struct pipe_surface *surface,
                               int width,
                               int height )
{

   struct pipe_framebuffer_state fb;
   struct pipe_viewport_state viewport;

   /* Framebuffer uses actual surface width/height
    */
   memset(&fb, 0, sizeof fb);
   fb.width  = surface->width;
   fb.height = surface->height;
   fb.nr_cbufs = 1;
   fb.cbufs[0] = surface;
   fb.zsbuf = 0;

   /* Viewport just touches the bit we're interested in:
    */
   viewport.scale[0] =  width / 2.f;
   viewport.scale[1] =  height / 2.f;
   viewport.scale[2] =  1.0;
   viewport.scale[3] =  1.0;
   viewport.translate[0] = width / 2.f;
   viewport.translate[1] = height / 2.f;
   viewport.translate[2] = 0.0;
   viewport.translate[3] = 0.0;

   /* Constant buffer set up to match viewport dimensions:
    */
   if (r->fb_width != width ||
       r->fb_height != height) 
   {
      float vs_consts[8] = {
         2.f/width, 2.f/height, 1, 1,
         -1, -1, 0, 0
      };

      r->fb_width = width;
      r->fb_height = height;

      renderer_set_constants(r, PIPE_SHADER_VERTEX,
                             vs_consts, sizeof vs_consts);
   }

   cso_set_framebuffer(r->cso, &fb);
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
   struct pipe_buffer **vsbuf = &r->vs_const_buffer;
   struct pipe_buffer **fsbuf = &r->fs_const_buffer;

   if (*vsbuf)
      pipe_buffer_reference(vsbuf, NULL);

   if (*fsbuf)
      pipe_buffer_reference(fsbuf, NULL);

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





void renderer_set_constants(struct xorg_renderer *r,
                            int shader_type,
                            const float *params,
                            int param_bytes)
{
   struct pipe_buffer **cbuf =
      (shader_type == PIPE_SHADER_VERTEX) ? &r->vs_const_buffer :
      &r->fs_const_buffer;

   pipe_buffer_reference(cbuf, NULL);
   *cbuf = pipe_buffer_create(r->pipe->screen, 16,
                              PIPE_BUFFER_USAGE_CONSTANT,
                              param_bytes);

   if (*cbuf) {
      pipe_buffer_write(r->pipe->screen, *cbuf,
                        0, param_bytes, params);
   }
   r->pipe->set_constant_buffer(r->pipe, shader_type, 0, *cbuf);
}


void renderer_copy_prepare(struct xorg_renderer *r,
                           struct pipe_surface *dst_surface,
                           struct pipe_texture *src_texture)
{
   struct pipe_context *pipe = r->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct xorg_shader shader;

   assert(screen->is_format_supported(screen, dst_surface->format,
                                      PIPE_TEXTURE_2D,
                                      PIPE_TEXTURE_USAGE_RENDER_TARGET,
                                      0));
   (void) screen;


   /* set misc state we care about */
   {
      struct pipe_blend_state blend;
      memset(&blend, 0, sizeof(blend));
      blend.rt[0].rgb_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].alpha_src_factor = PIPE_BLENDFACTOR_ONE;
      blend.rt[0].rgb_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].alpha_dst_factor = PIPE_BLENDFACTOR_ZERO;
      blend.rt[0].colormask = PIPE_MASK_RGBA;
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

   renderer_bind_destination(r, dst_surface, 
                             dst_surface->width,
                             dst_surface->height);

   /* texture */
   cso_set_sampler_textures(r->cso, 1, &src_texture);

   /* shaders */
   shader = xorg_shaders_get(r->shaders,
                             VS_COMPOSITE,
                             FS_COMPOSITE);
   cso_set_vertex_shader_handle(r->cso, shader.vs);
   cso_set_fragment_shader_handle(r->cso, shader.fs);

   r->buffer_size = 0;
   r->attrs_per_vertex = 2;
}

struct pipe_texture *
renderer_clone_texture(struct xorg_renderer *r,
                       struct pipe_texture *src)
{
   enum pipe_format format;
   struct pipe_context *pipe = r->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_texture *pt;
   struct pipe_texture templ;

   if (pipe->is_texture_referenced(pipe, src, 0, 0) &
       PIPE_REFERENCED_FOR_WRITE)
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
   templ.width0 = src->width0;
   templ.height0 = src->height0;
   templ.depth0 = 1;
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
                0, 0, src->width0, src->height0);
      } else {
          util_surface_copy(pipe, FALSE,
                ps_tex, /* dest */
                0, 0, /* destx/y */
                ps_read,
                0, 0, src->width0, src->height0);
      }
      pipe_surface_reference(&ps_read, NULL);
      pipe_surface_reference(&ps_tex, NULL);
   }

   return pt;
}


void renderer_copy_pixmap(struct xorg_renderer *r,
                          int dx, int dy,
                          int sx, int sy,
                          int width, int height,
                          float src_width,
                          float src_height)
{
   float s0, t0, s1, t1;
   float x0, y0, x1, y1;


   /* XXX: could put the texcoord scaling calculation into the vertex
    * shader.
    */
   s0 = sx            / src_width;
   s1 = (sx + width)  / src_width;
   t0 = sy            / src_height;
   t1 = (sy + height) / src_height;

   x0 = dx;
   x1 = dx + width;
   y0 = dy;
   y1 = dy + height;

   /* draw quad */
   renderer_draw_conditional(r, 4*8);
   add_vertex_1tex(r, x0, y0, s0, t0);
   add_vertex_1tex(r, x1, y0, s1, t0);
   add_vertex_1tex(r, x1, y1, s1, t1);
   add_vertex_1tex(r, x0, y1, s0, t1);
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
   r->buffer_size = 0;
   r->attrs_per_vertex = 2;
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
   r->attrs_per_vertex = 1 + num_textures;
   r->buffer_size = 0;
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

   switch(r->attrs_per_vertex) {
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
