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
  *   Brian Paul
  */

#include "main/imports.h"
#include "main/image.h"
#include "main/bufferobj.h"
#include "main/macros.h"
#include "main/texformat.h"
#include "shader/program.h"
#include "shader/prog_parameter.h"
#include "shader/prog_print.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_atom_constbuf.h"
#include "st_program.h"
#include "st_cb_bitmap.h"
#include "st_cb_program.h"
#include "st_mesa_to_tgsi.h"
#include "st_texture.h"
#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "util/u_tile.h"
#include "util/u_draw_quad.h"
#include "util/u_simple_shaders.h"
#include "shader/prog_instruction.h"
#include "cso_cache/cso_context.h"



/**
 * glBitmaps are drawn as textured quads.  The user's bitmap pattern
 * is stored in a texture image.  An alpha8 texture format is used.
 * The fragment shader samples a bit (texel) from the texture, then
 * discards the fragment if the bit is off.
 *
 * Note that we actually store the inverse image of the bitmap to
 * simplify the fragment program.  An "on" bit gets stored as texel=0x0
 * and an "off" bit is stored as texel=0xff.  Then we kill the
 * fragment if the negated texel value is less than zero.
 */


/**
 * The bitmap cache attempts to accumulate multiple glBitmap calls in a
 * buffer which is then rendered en mass upon a flush, state change, etc.
 * A wide, short buffer is used to target the common case of a series
 * of glBitmap calls being used to draw text.
 */
static GLboolean UseBitmapCache = GL_TRUE;


#define BITMAP_CACHE_WIDTH  512
#define BITMAP_CACHE_HEIGHT 32

struct bitmap_cache
{
   /** Window pos to render the cached image */
   GLint xpos, ypos;
   /** Bounds of region used in window coords */
   GLint xmin, ymin, xmax, ymax;

   GLfloat color[4];

   struct pipe_texture *texture;
   struct pipe_surface *surf;

   GLboolean empty;

   /** An I8 texture image: */
   ubyte *buffer;
};




/**
 * Make fragment program for glBitmap:
 *   Sample the texture and kill the fragment if the bit is 0.
 * This program will be combined with the user's fragment program.
 */
static struct st_fragment_program *
make_bitmap_fragment_program(GLcontext *ctx, GLuint samplerIndex)
{
   struct st_fragment_program *stfp;
   struct gl_program *p;
   GLuint ic = 0;

   p = ctx->Driver.NewProgram(ctx, GL_FRAGMENT_PROGRAM_ARB, 0);
   if (!p)
      return NULL;

   p->NumInstructions = 3;

   p->Instructions = _mesa_alloc_instructions(p->NumInstructions);
   if (!p->Instructions) {
      ctx->Driver.DeleteProgram(ctx, p);
      return NULL;
   }
   _mesa_init_instructions(p->Instructions, p->NumInstructions);

   /* TEX tmp0, fragment.texcoord[0], texture[0], 2D; */
   p->Instructions[ic].Opcode = OPCODE_TEX;
   p->Instructions[ic].DstReg.File = PROGRAM_TEMPORARY;
   p->Instructions[ic].DstReg.Index = 0;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_INPUT;
   p->Instructions[ic].SrcReg[0].Index = FRAG_ATTRIB_TEX0;
   p->Instructions[ic].TexSrcUnit = samplerIndex;
   p->Instructions[ic].TexSrcTarget = TEXTURE_2D_INDEX;
   ic++;

   /* KIL if -tmp0 < 0 # texel=0 -> keep / texel=0 -> discard */
   p->Instructions[ic].Opcode = OPCODE_KIL;
   p->Instructions[ic].SrcReg[0].File = PROGRAM_TEMPORARY;
   p->Instructions[ic].SrcReg[0].Index = 0;
   p->Instructions[ic].SrcReg[0].NegateBase = NEGATE_XYZW;
   ic++;

   /* END; */
   p->Instructions[ic++].Opcode = OPCODE_END;

   assert(ic == p->NumInstructions);

   p->InputsRead = FRAG_BIT_TEX0;
   p->OutputsWritten = 0x0;
   p->SamplersUsed = (1 << samplerIndex);

   stfp = (struct st_fragment_program *) p;
   stfp->Base.UsesKill = GL_TRUE;
   st_translate_fragment_program(ctx->st, stfp, NULL);

   return stfp;
}


static int
find_free_bit(uint bitfield)
{
   int i;
   for (i = 0; i < 32; i++) {
      if ((bitfield & (1 << i)) == 0) {
         return i;
      }
   }
   return -1;
}


/**
 * Combine basic bitmap fragment program with the user-defined program.
 */
static struct st_fragment_program *
combined_bitmap_fragment_program(GLcontext *ctx)
{
   struct st_context *st = ctx->st;
   struct st_fragment_program *stfp = st->fp;

   if (!stfp->bitmap_program) {
      /*
       * Generate new program which is the user-defined program prefixed
       * with the bitmap sampler/kill instructions.
       */
      struct st_fragment_program *bitmap_prog;
      uint sampler;

      sampler = find_free_bit(st->fp->Base.Base.SamplersUsed);
      bitmap_prog = make_bitmap_fragment_program(ctx, sampler);

      stfp->bitmap_program = (struct st_fragment_program *)
         _mesa_combine_programs(ctx,
                                &bitmap_prog->Base.Base, &stfp->Base.Base);
      stfp->bitmap_program->bitmap_sampler = sampler;

      /* done with this after combining */
      st_reference_fragprog(st, &bitmap_prog, NULL);

#if 0
      {
         struct gl_program *p = &stfp->bitmap_program->Base.Base;
         printf("Combined bitmap program:\n");
         _mesa_print_program(p);
         printf("InputsRead: 0x%x\n", p->InputsRead);
         printf("OutputsWritten: 0x%x\n", p->OutputsWritten);
         _mesa_print_parameter_list(p->Parameters);
      }
#endif

      /* translate to TGSI tokens */
      st_translate_fragment_program(st, stfp->bitmap_program, NULL);
   }

   /* Ideally we'd have updated the pipe constants during the normal
    * st/atom mechanism.  But we can't since this is specific to glBitmap.
    */
   st_upload_constants(st, stfp->Base.Base.Parameters, PIPE_SHADER_FRAGMENT);

   return stfp->bitmap_program;
}


/**
 * Copy user-provide bitmap bits into texture buffer, expanding
 * bits into texels.
 * "On" bits will set texels to 0xff.
 * "Off" bits will not modify texels.
 * Note that the image is actually going to be upside down in
 * the texture.  We deal with that with texcoords.
 */
static void
unpack_bitmap(struct st_context *st,
              GLint px, GLint py, GLsizei width, GLsizei height,
              const struct gl_pixelstore_attrib *unpack,
              const GLubyte *bitmap,
              ubyte *destBuffer, uint destStride)
{
   GLint row, col;

#define SET_PIXEL(COL, ROW) \
   destBuffer[(py + (ROW)) * destStride + px + (COL)] = 0x0;

   for (row = 0; row < height; row++) {
      const GLubyte *src = (const GLubyte *) _mesa_image_address2d(unpack,
                 bitmap, width, height, GL_COLOR_INDEX, GL_BITMAP, row, 0);

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            if (*src & mask) {
               SET_PIXEL(col, row);
            }

            if (mask == 128U) {
               src++;
               mask = 1U;
            }
            else {
               mask = mask << 1;
            }
         }

         /* get ready for next row */
         if (mask != 1)
            src++;
      }
      else {
         /* Msb first */
         GLubyte mask = 128U >> (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            if (*src & mask) {
               SET_PIXEL(col, row);
            }

            if (mask == 1U) {
               src++;
               mask = 128U;
            }
            else {
               mask = mask >> 1;
            }
         }

         /* get ready for next row */
         if (mask != 128)
            src++;
      }

   } /* row */

#undef SET_PIXEL
}


/**
 * Create a texture which represents a bitmap image.
 */
static struct pipe_texture *
make_bitmap_texture(GLcontext *ctx, GLsizei width, GLsizei height,
                    const struct gl_pixelstore_attrib *unpack,
                    const GLubyte *bitmap)
{
   struct pipe_context *pipe = ctx->st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct pipe_surface *surface;
   ubyte *dest;
   struct pipe_texture *pt;

   /* PBO source... */
   bitmap = _mesa_map_bitmap_pbo(ctx, unpack, bitmap);
   if (!bitmap) {
      return NULL;
   }

   /**
    * Create texture to hold bitmap pattern.
    */
   pt = st_texture_create(ctx->st, PIPE_TEXTURE_2D, ctx->st->bitmap.tex_format,
                          0, width, height, 1, 0,
                          PIPE_TEXTURE_USAGE_SAMPLER);
   if (!pt) {
      _mesa_unmap_bitmap_pbo(ctx, unpack);
      return NULL;
   }

   surface = screen->get_tex_surface(screen, pt, 0, 0, 0,
                                     PIPE_BUFFER_USAGE_CPU_WRITE);

   /* map texture surface */
   dest = screen->surface_map(screen, surface, PIPE_BUFFER_USAGE_CPU_WRITE);

   /* Put image into texture surface */
   memset(dest, 0xff, height * surface->stride);
   unpack_bitmap(ctx->st, 0, 0, width, height, unpack, bitmap,
                 dest, surface->stride);

   _mesa_unmap_bitmap_pbo(ctx, unpack);

   /* Release surface */
   screen->surface_unmap(screen, surface);
   pipe_surface_reference(&surface, NULL);

   return pt;
}


static void
setup_bitmap_vertex_data(struct st_context *st,
                         int x, int y, int width, int height,
                         float z, const float color[4])
{
   struct pipe_context *pipe = st->pipe;
   const struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   const GLfloat fb_width = (GLfloat)fb->Width;
   const GLfloat fb_height = (GLfloat)fb->Height;
   const GLfloat x0 = (GLfloat)x;
   const GLfloat x1 = (GLfloat)(x + width);
   const GLfloat y0 = (GLfloat)y;
   const GLfloat y1 = (GLfloat)(y + height);
   const GLfloat sLeft = (GLfloat)0.0, sRight = (GLfloat)1.0;
   const GLfloat tTop = (GLfloat)0.0, tBot = (GLfloat)1.0 - tTop;
   const GLfloat clip_x0 = (GLfloat)(x0 / fb_width * 2.0 - 1.0);
   const GLfloat clip_y0 = (GLfloat)(y0 / fb_height * 2.0 - 1.0);
   const GLfloat clip_x1 = (GLfloat)(x1 / fb_width * 2.0 - 1.0);
   const GLfloat clip_y1 = (GLfloat)(y1 / fb_height * 2.0 - 1.0);
   GLuint i;
   void *buf;

   if (!st->bitmap.vbuf) {
      st->bitmap.vbuf = pipe_buffer_create(pipe->screen, 32, PIPE_BUFFER_USAGE_VERTEX,
                                           sizeof(st->bitmap.vertices));
   }

   /* Positions are in clip coords since we need to do clipping in case
    * the bitmap quad goes beyond the window bounds.
    */
   st->bitmap.vertices[0][0][0] = clip_x0;
   st->bitmap.vertices[0][0][1] = clip_y0;
   st->bitmap.vertices[0][2][0] = sLeft;
   st->bitmap.vertices[0][2][1] = tTop;

   st->bitmap.vertices[1][0][0] = clip_x1;
   st->bitmap.vertices[1][0][1] = clip_y0;
   st->bitmap.vertices[1][2][0] = sRight;
   st->bitmap.vertices[1][2][1] = tTop;
   
   st->bitmap.vertices[2][0][0] = clip_x1;
   st->bitmap.vertices[2][0][1] = clip_y1;
   st->bitmap.vertices[2][2][0] = sRight;
   st->bitmap.vertices[2][2][1] = tBot;
   
   st->bitmap.vertices[3][0][0] = clip_x0;
   st->bitmap.vertices[3][0][1] = clip_y1;
   st->bitmap.vertices[3][2][0] = sLeft;
   st->bitmap.vertices[3][2][1] = tBot;
   
   /* same for all verts: */
   for (i = 0; i < 4; i++) {
      st->bitmap.vertices[i][0][2] = z;
      st->bitmap.vertices[i][0][3] = 1.0;
      st->bitmap.vertices[i][1][0] = color[0];
      st->bitmap.vertices[i][1][1] = color[1];
      st->bitmap.vertices[i][1][2] = color[2];
      st->bitmap.vertices[i][1][3] = color[3];
      st->bitmap.vertices[i][2][2] = 0.0; /*R*/
      st->bitmap.vertices[i][2][3] = 1.0; /*Q*/
   }

   /* put vertex data into vbuf */
   buf = pipe_buffer_map(pipe->screen, st->bitmap.vbuf, PIPE_BUFFER_USAGE_CPU_WRITE);
   memcpy(buf, st->bitmap.vertices, sizeof(st->bitmap.vertices));
   pipe_buffer_unmap(pipe->screen, st->bitmap.vbuf);
}



/**
 * Render a glBitmap by drawing a textured quad
 */
static void
draw_bitmap_quad(GLcontext *ctx, GLint x, GLint y, GLfloat z,
                 GLsizei width, GLsizei height,
                 struct pipe_texture *pt,
                 const GLfloat *color)
{
   struct st_context *st = ctx->st;
   struct pipe_context *pipe = ctx->st->pipe;
   struct cso_context *cso = ctx->st->cso_context;
   struct st_fragment_program *stfp;
   GLuint maxSize;

   stfp = combined_bitmap_fragment_program(ctx);

   /* limit checks */
   /* XXX if the bitmap is larger than the max texture size, break
    * it up into chunks.
    */
   maxSize = 1 << (pipe->screen->get_param(pipe->screen, PIPE_CAP_MAX_TEXTURE_2D_LEVELS) - 1);
   assert(width <= (GLsizei)maxSize);
   assert(height <= (GLsizei)maxSize);

   cso_save_rasterizer(cso);
   cso_save_samplers(cso);
   cso_save_sampler_textures(cso);
   cso_save_viewport(cso);
   cso_save_fragment_shader(cso);
   cso_save_vertex_shader(cso);

   /* rasterizer state: just scissor */
   st->bitmap.rasterizer.scissor = ctx->Scissor.Enabled;
   cso_set_rasterizer(cso, &st->bitmap.rasterizer);

   /* fragment shader state: TEX lookup program */
   cso_set_fragment_shader_handle(cso, stfp->driver_shader);

   /* vertex shader state: position + texcoord pass-through */
   cso_set_vertex_shader_handle(cso, st->bitmap.vs);

   /* user samplers, plus our bitmap sampler */
   {
      struct pipe_sampler_state *samplers[PIPE_MAX_SAMPLERS];
      uint num = MAX2(stfp->bitmap_sampler + 1, st->state.num_samplers);
      uint i;
      for (i = 0; i < st->state.num_samplers; i++) {
         samplers[i] = &st->state.samplers[i];
      }
      samplers[stfp->bitmap_sampler] = &st->bitmap.sampler;
      cso_set_samplers(cso, num, (const struct pipe_sampler_state **) samplers);   }

   /* user textures, plus the bitmap texture */
   {
      struct pipe_texture *textures[PIPE_MAX_SAMPLERS];
      uint num = MAX2(stfp->bitmap_sampler + 1, st->state.num_textures);
      memcpy(textures, st->state.sampler_texture, sizeof(textures));
      textures[stfp->bitmap_sampler] = pt;
      cso_set_sampler_textures(cso, num, textures);
   }

   /* viewport state: viewport matching window dims */
   {
      const struct gl_framebuffer *fb = st->ctx->DrawBuffer;
      const GLboolean invert = (st_fb_orientation(fb) == Y_0_TOP);
      const GLfloat width = (GLfloat)fb->Width;
      const GLfloat height = (GLfloat)fb->Height;
      struct pipe_viewport_state vp;
      vp.scale[0] =  0.5f * width;
      vp.scale[1] = height * (invert ? -0.5f : 0.5f);
      vp.scale[2] = 1.0f;
      vp.scale[3] = 1.0f;
      vp.translate[0] = 0.5f * width;
      vp.translate[1] = 0.5f * height;
      vp.translate[2] = 0.0f;
      vp.translate[3] = 0.0f;
      cso_set_viewport(cso, &vp);
   }

   /* draw textured quad */
   setup_bitmap_vertex_data(st, x, y, width, height,
                            ctx->Current.RasterPos[2],
                            color);

   util_draw_vertex_buffer(pipe, st->bitmap.vbuf,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           3); /* attribs/vert */


   /* restore state */
   cso_restore_rasterizer(cso);
   cso_restore_samplers(cso);
   cso_restore_sampler_textures(cso);
   cso_restore_viewport(cso);
   cso_restore_fragment_shader(cso);
   cso_restore_vertex_shader(cso);
}


static void
reset_cache(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct bitmap_cache *cache = st->bitmap.cache;

   //memset(cache->buffer, 0xff, sizeof(cache->buffer));
   cache->empty = GL_TRUE;

   cache->xmin = 1000000;
   cache->xmax = -1000000;
   cache->ymin = 1000000;
   cache->ymax = -1000000;

   if (cache->surf)
      screen->tex_surface_release(screen, &cache->surf);

   assert(!cache->texture);

   /* allocate a new texture */
   cache->texture = st_texture_create(st, PIPE_TEXTURE_2D,
                                      st->bitmap.tex_format, 0,
                                      BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                                      1, 0,
                                      PIPE_TEXTURE_USAGE_SAMPLER);

   /* Map the texture surface.
    * Subsequent glBitmap calls will write into the texture image.
    */
   cache->surf = screen->get_tex_surface(screen, cache->texture, 0, 0, 0,
                                         PIPE_BUFFER_USAGE_CPU_WRITE);
   cache->buffer = screen->surface_map(screen, cache->surf,
                                       PIPE_BUFFER_USAGE_CPU_WRITE);

   /* init image to all 0xff */
   memset(cache->buffer, 0xff, BITMAP_CACHE_WIDTH * BITMAP_CACHE_HEIGHT);
}


/**
 * If there's anything in the bitmap cache, draw/flush it now.
 */
void
st_flush_bitmap_cache(struct st_context *st)
{
   if (!st->bitmap.cache->empty) {
      struct bitmap_cache *cache = st->bitmap.cache;

      if (st->ctx->DrawBuffer) {
         struct pipe_context *pipe = st->pipe;
         struct pipe_screen *screen = pipe->screen;

         assert(cache->xmin <= cache->xmax);
         /*
         printf("flush size %d x %d  at %d, %d\n",
                cache->xmax - cache->xmin,
                cache->ymax - cache->ymin,
                cache->xpos, cache->ypos);
         */

         /* The texture surface has been mapped until now.
          * So unmap and release the texture surface before drawing.
          */
         screen->surface_unmap(screen, cache->surf);
         cache->buffer = NULL;

         screen->tex_surface_release(screen, &cache->surf);

         draw_bitmap_quad(st->ctx,
                          cache->xpos,
                          cache->ypos,
                          st->ctx->Current.RasterPos[2],
                          BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                          cache->texture,
                          cache->color);
      }

      /* release/free the texture */
      pipe_texture_reference(&cache->texture, NULL);

      reset_cache(st);
   }
}


/**
 * Try to accumulate this glBitmap call in the bitmap cache.
 * \return  GL_TRUE for success, GL_FALSE if bitmap is too large, etc.
 */
static GLboolean
accum_bitmap(struct st_context *st,
             GLint x, GLint y, GLsizei width, GLsizei height,
             const struct gl_pixelstore_attrib *unpack,
             const GLubyte *bitmap )
{
   struct bitmap_cache *cache = st->bitmap.cache;
   int px = -999, py;

   if (width > BITMAP_CACHE_WIDTH ||
       height > BITMAP_CACHE_HEIGHT)
      return GL_FALSE; /* too big to cache */

   if (!cache->empty) {
      px = x - cache->xpos;  /* pos in buffer */
      py = y - cache->ypos;
      if (px < 0 || px + width > BITMAP_CACHE_WIDTH ||
          py < 0 || py + height > BITMAP_CACHE_HEIGHT ||
          !TEST_EQ_4V(st->ctx->Current.RasterColor, cache->color)) {
         /* This bitmap would extend beyond cache bounds, or the bitmap
          * color is changing
          * so flush and continue.
          */
         st_flush_bitmap_cache(st);
      }
   }

   if (cache->empty) {
      /* Initialize.  Center bitmap vertically in the buffer. */
      px = 0;
      py = (BITMAP_CACHE_HEIGHT - height) / 2;
      cache->xpos = x;
      cache->ypos = y - py;
      cache->empty = GL_FALSE;
      COPY_4FV(cache->color, st->ctx->Current.RasterColor);
   }

   assert(px != -999);

   if (x < cache->xmin)
      cache->xmin = x;
   if (y < cache->ymin)
      cache->ymin = y;
   if (x + width > cache->xmax)
      cache->xmax = x + width;
   if (y + height > cache->ymax)
      cache->ymax = y + height;

   unpack_bitmap(st, px, py, width, height, unpack, bitmap,
                 cache->buffer, BITMAP_CACHE_WIDTH);

   return GL_TRUE; /* accumulated */
}



/**
 * Called via ctx->Driver.Bitmap()
 */
static void
st_Bitmap(GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height,
          const struct gl_pixelstore_attrib *unpack, const GLubyte *bitmap )
{
   struct st_context *st = ctx->st;
   struct pipe_texture *pt;

   if (width == 0 || height == 0)
      return;

   st_validate_state(st);

   if (!st->bitmap.vs) {
      /* create pass-through vertex shader now */
      const uint semantic_names[] = { TGSI_SEMANTIC_POSITION,
                                      TGSI_SEMANTIC_COLOR,
                                      TGSI_SEMANTIC_GENERIC };
      const uint semantic_indexes[] = { 0, 0, 0 };
      st->bitmap.vs = util_make_vertex_passthrough_shader(st->pipe, 3,
                                                          semantic_names,
                                                          semantic_indexes,
                                                          &st->bitmap.vert_shader);
   }

   if (UseBitmapCache && accum_bitmap(st, x, y, width, height, unpack, bitmap))
      return;

   pt = make_bitmap_texture(ctx, width, height, unpack, bitmap);
   if (pt) {
      assert(pt->target == PIPE_TEXTURE_2D);
      draw_bitmap_quad(ctx, x, y, ctx->Current.RasterPos[2],
                       width, height, pt,
                       st->ctx->Current.RasterColor);
      /* release/free the texture */
      pipe_texture_reference(&pt, NULL);
   }
}


/** Per-context init */
void
st_init_bitmap_functions(struct dd_function_table *functions)
{
   functions->Bitmap = st_Bitmap;
}


/** Per-context init */
void
st_init_bitmap(struct st_context *st)
{
   struct pipe_sampler_state *sampler = &st->bitmap.sampler;
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;

   /* init sampler state once */
   memset(sampler, 0, sizeof(*sampler));
   sampler->wrap_s = PIPE_TEX_WRAP_CLAMP;
   sampler->wrap_t = PIPE_TEX_WRAP_CLAMP;
   sampler->wrap_r = PIPE_TEX_WRAP_CLAMP;
   sampler->min_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler->min_mip_filter = PIPE_TEX_MIPFILTER_NONE;
   sampler->mag_img_filter = PIPE_TEX_FILTER_NEAREST;
   sampler->normalized_coords = 1;

   /* init baseline rasterizer state once */
   memset(&st->bitmap.rasterizer, 0, sizeof(st->bitmap.rasterizer));
   st->bitmap.rasterizer.gl_rasterization_rules = 1;
   st->bitmap.rasterizer.bypass_vs = 1;

   /* find a usable texture format */
   if (screen->is_format_supported(screen, PIPE_FORMAT_I8_UNORM, PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      st->bitmap.tex_format = PIPE_FORMAT_I8_UNORM;
   }
   else {
      /* XXX support more formats */
      assert(0);
   }

   /* alloc bitmap cache object */
   st->bitmap.cache = CALLOC_STRUCT(bitmap_cache);

   reset_cache(st);
}


/** Per-context tear-down */
void
st_destroy_bitmap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct bitmap_cache *cache = st->bitmap.cache;

   screen->surface_unmap(screen, cache->surf);
   screen->tex_surface_release(screen, &cache->surf);

   if (st->bitmap.vs) {
      cso_delete_vertex_shader(st->cso_context, st->bitmap.vs);
      st->bitmap.vs = NULL;
   }

   if (st->bitmap.vbuf) {
      pipe_buffer_destroy(pipe->screen, st->bitmap.vbuf);
      st->bitmap.vbuf = NULL;
   }

   if (st->bitmap.cache) {
      pipe_texture_release(&st->bitmap.cache->texture);
      FREE(st->bitmap.cache);
      st->bitmap.cache = NULL;
   }
}
