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
#include "pipe/p_winsys.h"
#include "util/p_tile.h"
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
   struct pipe_texture *texture;
   GLboolean empty;
   /** An I8 texture image: */
   GLubyte buffer[BITMAP_CACHE_HEIGHT][BITMAP_CACHE_WIDTH];
};




/**
 * Make fragment program for glBitmap:
 *   Sample the texture and kill the fragment if the bit is 0.
 * This program will be combined with the user's fragment program.
 */
static struct st_fragment_program *
make_bitmap_fragment_program(GLcontext *ctx)
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
   p->Instructions[ic].TexSrcUnit = 0;
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
   p->SamplersUsed = 0x1;  /* sampler 0 (bit 0) is used */

   stfp = (struct st_fragment_program *) p;
   stfp->Base.UsesKill = GL_TRUE;
   st_translate_fragment_program(ctx->st, stfp, NULL);

   return stfp;
}


/**
 * Combine basic bitmap fragment program with the user-defined program.
 */
static struct st_fragment_program *
combined_bitmap_fragment_program(GLcontext *ctx)
{
   struct st_context *st = ctx->st;
   struct st_fragment_program *stfp;

   if (!st->bitmap.program) {
      /* create the basic bitmap fragment program */
      st->bitmap.program = make_bitmap_fragment_program(ctx);
   }

   if (st->bitmap.user_prog_sn == st->fp->serialNo) {
      /* re-use */
      stfp = st->bitmap.combined_prog;
   }
   else {
      /* Concatenate the bitmap program with the current user-defined program.
       */
      stfp = (struct st_fragment_program *)
         _mesa_combine_programs(ctx,
                                &st->bitmap.program->Base.Base,
                                &st->fp->Base.Base);

#if 0
      {
         struct gl_program *p = &stfp->Base.Base;
         printf("Combined bitmap program:\n");
         _mesa_print_program(p);
         printf("InputsRead: 0x%x\n", p->InputsRead);
         printf("OutputsWritten: 0x%x\n", p->OutputsWritten);
         _mesa_print_parameter_list(p->Parameters);
      }
#endif

      /* translate to TGSI tokens */
      st_translate_fragment_program(st, stfp, NULL);

      /* save new program, update serial numbers */
      st->bitmap.user_prog_sn = st->fp->serialNo;
      st->bitmap.combined_prog = stfp;
   }

   /* Ideally we'd have updated the pipe constants during the normal
    * st/atom mechanism.  But we can't since this is specific to glBitmap.
    */
   st_upload_constants(st, stfp->Base.Base.Parameters, PIPE_SHADER_FRAGMENT);

   return stfp;
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
   uint format = 0, cpp, comp;
   ubyte *dest;
   struct pipe_texture *pt;
   int row, col;

   /* find a texture format we know */
   if (screen->is_format_supported( screen, PIPE_FORMAT_U_I8, PIPE_TEXTURE )) {
      format = PIPE_FORMAT_U_I8;
      cpp = 1;
      comp = 0;
   }
   else if (screen->is_format_supported( screen, PIPE_FORMAT_A8R8G8B8_UNORM, PIPE_TEXTURE )) {
      format = PIPE_FORMAT_A8R8G8B8_UNORM;
      cpp = 4;
      comp = 3; /* alpha channel */ /*XXX little-endian dependency */
   }
   else {
      /* XXX support more formats */
      assert( 0 );
   }

   /* PBO source... */
   bitmap = _mesa_map_bitmap_pbo(ctx, unpack, bitmap);
   if (!bitmap) {
      return NULL;
   }

   /**
    * Create texture to hold bitmap pattern.
    */
   pt = st_texture_create(ctx->st, PIPE_TEXTURE_2D, format, 0, width, height,
			  1, 0);
   if (!pt) {
      _mesa_unmap_bitmap_pbo(ctx, unpack);
      return NULL;
   }

   surface = screen->get_tex_surface(screen, pt, 0, 0, 0);

   /* map texture surface */
   dest = pipe_surface_map(surface);

   /* Put image into texture surface.
    * Note that the image is actually going to be upside down in
    * the texture.  We deal with that with texcoords.
    */

   for (row = 0; row < height; row++) {
      const GLubyte *src = (const GLubyte *) _mesa_image_address2d(unpack,
                 bitmap, width, height, GL_COLOR_INDEX, GL_BITMAP, row, 0);
      ubyte *destRow = dest + row * surface->pitch * cpp;

      if (unpack->LsbFirst) {
         /* Lsb first */
         GLubyte mask = 1U << (unpack->SkipPixels & 0x7);
         for (col = 0; col < width; col++) {

            /* set texel to 255 if bit is set */
            destRow[comp] = (*src & mask) ? 0x0 : 0xff;
            destRow += cpp;

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

            /* set texel to 255 if bit is set */
            destRow[comp] =(*src & mask) ? 0x0 : 0xff;
            destRow += cpp;

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

   _mesa_unmap_bitmap_pbo(ctx, unpack);

   /* Release surface */
   pipe_surface_unmap(surface);
   pipe_surface_reference(&surface, NULL);
   pipe->texture_update(pipe, pt, 0, 0x1);

   pt->format = format;

   return pt;
}


static void
setup_bitmap_vertex_data(struct st_context *st,
                         int x, int y, int width, int height,
                         float z, const float color[4])
{
   struct pipe_context *pipe = st->pipe;
   const struct gl_framebuffer *fb = st->ctx->DrawBuffer;
   const GLfloat fb_width = fb->Width;
   const GLfloat fb_height = fb->Height;
   const GLfloat x0 = x;
   const GLfloat x1 = x + width;
   const GLfloat y0 = y;
   const GLfloat y1 = y + height;
   const GLfloat bias = st->bitmap_texcoord_bias;
   const GLfloat xBias = bias / (x1-x0);
   const GLfloat yBias = bias / (y1-y0);
   const GLfloat sLeft = 0.0 + xBias, sRight = 1.0 + xBias;
   const GLfloat tTop = yBias, tBot = 1.0 - tTop - yBias;
   const GLfloat clip_x0 = x0 / fb_width * 2.0 - 1.0;
   const GLfloat clip_y0 = y0 / fb_height * 2.0 - 1.0;
   const GLfloat clip_x1 = x1 / fb_width * 2.0 - 1.0;
   const GLfloat clip_y1 = y1 / fb_height * 2.0 - 1.0;
   GLuint i;
   void *buf;

   if (!st->bitmap.vbuf) {
      st->bitmap.vbuf = pipe->winsys->buffer_create(pipe->winsys, 32,
                                                   PIPE_BUFFER_USAGE_VERTEX,
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
   buf = pipe->winsys->buffer_map(pipe->winsys, st->bitmap.vbuf,
                                  PIPE_BUFFER_USAGE_CPU_WRITE);
   memcpy(buf, st->bitmap.vertices, sizeof(st->bitmap.vertices));
   pipe->winsys->buffer_unmap(pipe->winsys, st->bitmap.vbuf);
}



/**
 * Render a glBitmap by drawing a textured quad
 */
static void
draw_bitmap_quad(GLcontext *ctx, GLint x, GLint y, GLfloat z,
                 GLsizei width, GLsizei height,
                 struct pipe_texture *pt)
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
   assert(width <= maxSize);
   assert(height <= maxSize);

   cso_save_rasterizer(cso);
   cso_save_samplers(cso);
   cso_save_sampler_textures(cso);
   cso_save_viewport(cso);

   /* rasterizer state: just scissor */
   st->bitmap.rasterizer.scissor = ctx->Scissor.Enabled;
   cso_set_rasterizer(cso, &st->bitmap.rasterizer);

   /* fragment shader state: TEX lookup program */
   pipe->bind_fs_state(pipe, stfp->driver_shader);

   /* vertex shader state: position + texcoord pass-through */
   pipe->bind_vs_state(pipe, st->bitmap.vs);

   /* sampler / texture state */
   cso_single_sampler(cso, 0, &st->bitmap.sampler);
   cso_single_sampler_done(cso);
   pipe->set_sampler_textures(pipe, 1, &pt);

   /* viewport state: viewport matching window dims */
   {
      const struct gl_framebuffer *fb = st->ctx->DrawBuffer;
      const GLboolean invert = (st_fb_orientation(fb) == Y_0_TOP);
      const float width = fb->Width;
      const float height = fb->Height;
      struct pipe_viewport_state vp;
      vp.scale[0] =  0.5 * width;
      vp.scale[1] = height * (invert ? -0.5 : 0.5);
      vp.scale[2] = 1.0;
      vp.scale[3] = 1.0;
      vp.translate[0] = 0.5 * width;
      vp.translate[1] = 0.5 * height;
      vp.translate[2] = 0.0;
      vp.translate[3] = 0.0;
      cso_set_viewport(cso, &vp);
   }

   /* draw textured quad */
   setup_bitmap_vertex_data(st, x, y, width, height,
                            ctx->Current.RasterPos[2],
                            ctx->Current.RasterColor);

   util_draw_vertex_buffer(pipe, st->bitmap.vbuf,
                           PIPE_PRIM_TRIANGLE_FAN,
                           4,  /* verts */
                           3); /* attribs/vert */


   /* restore state */
   cso_restore_rasterizer(cso);
   cso_restore_samplers(cso);
   cso_restore_sampler_textures(cso);
   cso_restore_viewport(cso);
   /* shaders don't go through cso yet */
   pipe->bind_fs_state(pipe, st->fp->driver_shader);
   pipe->bind_vs_state(pipe, st->vp->driver_shader);
}


static void
reset_cache(struct st_context *st)
{
   memset(st->bitmap.cache->buffer, 0xff, sizeof(st->bitmap.cache->buffer));
   st->bitmap.cache->empty = GL_TRUE;

   st->bitmap.cache->xmin = 1000000;
   st->bitmap.cache->xmax = -1000000;
   st->bitmap.cache->ymin = 1000000;
   st->bitmap.cache->ymax = -1000000;
}


static void
init_bitmap_cache(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   enum pipe_format format;

   st->bitmap.cache = CALLOC_STRUCT(bitmap_cache);
   if (!st->bitmap.cache)
      return;

   /* find a usable texture format */
   if (screen->is_format_supported(screen, PIPE_FORMAT_U_I8, PIPE_TEXTURE)) {
      format = PIPE_FORMAT_U_I8;
   }
   else {
      /* XXX support more formats */
      assert(0);
   }

   st->bitmap.cache->texture
      = st_texture_create(st, PIPE_TEXTURE_2D, format, 0,
                          BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT, 1, 0);
   if (!st->bitmap.cache->texture) {
      FREE(st->bitmap.cache);
      st->bitmap.cache = NULL;
      return;
   }

   reset_cache(st);
}


/**
 * If there's anything in the bitmap cache, draw/flush it now.
 */
void
st_flush_bitmap_cache(struct st_context *st)
{
   if (!st->bitmap.cache->empty) {
      struct bitmap_cache *cache = st->bitmap.cache;
      struct pipe_context *pipe = st->pipe;
      struct pipe_screen *screen = pipe->screen;
      struct pipe_surface *surf;
      void *dest;

      assert(cache->xmin <= cache->xmax);
      /*
      printf("flush size %d x %d  at %d, %d\n",
             cache->xmax - cache->xmin,
             cache->ymax - cache->ymin,
             cache->xpos, cache->ypos);
      */

      /* update the texture map image */
      surf = screen->get_tex_surface(screen, cache->texture, 0, 0, 0);
      dest = pipe_surface_map(surf);
      memcpy(dest, cache->buffer, sizeof(cache->buffer));
      pipe_surface_unmap(surf);
      pipe_surface_reference(&surf, NULL);

      pipe->texture_update(pipe, cache->texture, 0, 0x1);

      draw_bitmap_quad(st->ctx,
                       cache->xpos,
                       cache->ypos,
                       st->ctx->Current.RasterPos[2],
                       BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                       cache->texture);

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
   int row, col;
   int px = -999, py;

   if (width > BITMAP_CACHE_WIDTH ||
       height > BITMAP_CACHE_HEIGHT)
      return GL_FALSE; /* too big to cache */

   if (!cache->empty) {
      px = x - cache->xpos;  /* pos in buffer */
      py = y - cache->ypos;
      if (px < 0 || px + width > BITMAP_CACHE_WIDTH ||
          py < 0 || py + height > BITMAP_CACHE_HEIGHT) {
         /* This bitmap would extend beyond cache bounds,
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

   /* XXX try to combine this code with code in make_bitmap_texture() */
#define SET_PIXEL(COL, ROW) \
   cache->buffer[py + (ROW)][px + (COL)] = 0x0;

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
                       width, height, pt);
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

   init_bitmap_cache(st);
}


/** Per-context tear-down */
void
st_destroy_bitmap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;

   if (st->bitmap.combined_prog) {
      st_delete_program(st->ctx, &st->bitmap.combined_prog->Base.Base);
   }

   if (st->bitmap.program) {
      st_delete_program(st->ctx, &st->bitmap.program->Base.Base);
   }

   if (st->bitmap.vs) {
      pipe->delete_vs_state(pipe, st->bitmap.vs);
      st->bitmap.vs = NULL;
   }

   if (st->bitmap.vbuf) {
      pipe->winsys->buffer_destroy(pipe->winsys, st->bitmap.vbuf);
      st->bitmap.vbuf = NULL;
   }

   if (st->bitmap.cache) {
      pipe_texture_release(&st->bitmap.cache->texture);
      FREE(st->bitmap.cache);
      st->bitmap.cache = NULL;
   }
}
