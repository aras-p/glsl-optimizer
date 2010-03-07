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
#include "shader/program.h"
#include "shader/prog_print.h"

#include "st_context.h"
#include "st_atom.h"
#include "st_atom_constbuf.h"
#include "st_program.h"
#include "st_cb_bitmap.h"
#include "st_texture.h"
#include "st_inlines.h"

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "util/u_inlines.h"
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

   /** Bitmap's Z position */
   GLfloat zpos;

   struct pipe_texture *texture;
   struct pipe_transfer *trans;

   GLboolean empty;

   /** An I8 texture image: */
   ubyte *buffer;
};


/** Epsilon for Z comparisons */
#define Z_EPSILON 1e-06


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

   if (ctx->st->bitmap.tex_format == PIPE_FORMAT_L8_UNORM)
      p->Instructions[ic].SrcReg[0].Swizzle = SWIZZLE_XXXX;

   p->Instructions[ic].SrcReg[0].Index = 0;
   p->Instructions[ic].SrcReg[0].Negate = NEGATE_XYZW;
   ic++;

   /* END; */
   p->Instructions[ic++].Opcode = OPCODE_END;

   assert(ic == p->NumInstructions);

   p->InputsRead = FRAG_BIT_TEX0;
   p->OutputsWritten = 0x0;
   p->SamplersUsed = (1 << samplerIndex);

   stfp = (struct st_fragment_program *) p;
   stfp->Base.UsesKill = GL_TRUE;

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
      st_translate_fragment_program(st, stfp->bitmap_program);
   }

   return stfp->bitmap_program;
}


/**
 * Copy user-provide bitmap bits into texture buffer, expanding
 * bits into texels.
 * "On" bits will set texels to 0x0.
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
   destBuffer += py * destStride + px;

   _mesa_expand_bitmap(width, height, unpack, bitmap,
                       destBuffer, destStride, 0x0);
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
   struct pipe_transfer *transfer;
   ubyte *dest;
   struct pipe_texture *pt;

   /* PBO source... */
   bitmap = _mesa_map_pbo_source(ctx, unpack, bitmap);
   if (!bitmap) {
      return NULL;
   }

   /**
    * Create texture to hold bitmap pattern.
    */
   pt = st_texture_create(ctx->st, PIPE_TEXTURE_2D, ctx->st->bitmap.tex_format,
                          0, width, height, 1,
                          PIPE_TEXTURE_USAGE_SAMPLER);
   if (!pt) {
      _mesa_unmap_pbo_source(ctx, unpack);
      return NULL;
   }

   transfer = st_no_flush_get_tex_transfer(st_context(ctx), pt, 0, 0, 0,
					   PIPE_TRANSFER_WRITE,
					   0, 0, width, height);

   dest = screen->transfer_map(screen, transfer);

   /* Put image into texture transfer */
   memset(dest, 0xff, height * transfer->stride);
   unpack_bitmap(ctx->st, 0, 0, width, height, unpack, bitmap,
                 dest, transfer->stride);

   _mesa_unmap_pbo_source(ctx, unpack);

   /* Release transfer */
   screen->transfer_unmap(screen, transfer);
   screen->tex_transfer_destroy(transfer);

   return pt;
}

static GLuint
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

   /* XXX: Need to improve buffer_write to allow NO_WAIT (as well as
    * no_flush) updates to buffers where we know there is no conflict
    * with previous data.  Currently using max_slots > 1 will cause
    * synchronous rendering if the driver flushes its command buffers
    * between one bitmap and the next.  Our flush hook below isn't
    * sufficient to catch this as the driver doesn't tell us when it
    * flushes its own command buffers.  Until this gets fixed, pay the
    * price of allocating a new buffer for each bitmap cache-flush to
    * avoid synchronous rendering.
    */
   const GLuint max_slots = 1; /* 4096 / sizeof(st->bitmap.vertices); */
   GLuint i;

   if (st->bitmap.vbuf_slot >= max_slots) {
      pipe_buffer_reference(&st->bitmap.vbuf, NULL);
      st->bitmap.vbuf_slot = 0;
   }

   if (!st->bitmap.vbuf) {
      st->bitmap.vbuf = pipe_buffer_create(pipe->screen, 32, 
                                           PIPE_BUFFER_USAGE_VERTEX,
                                           max_slots * sizeof(st->bitmap.vertices));
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
   st_no_flush_pipe_buffer_write_nooverlap(st,
                                           st->bitmap.vbuf,
                                           st->bitmap.vbuf_slot * sizeof st->bitmap.vertices,
                                           sizeof st->bitmap.vertices,
                                           st->bitmap.vertices);

   return st->bitmap.vbuf_slot++ * sizeof st->bitmap.vertices;
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
   GLuint offset;

   stfp = combined_bitmap_fragment_program(ctx);

   /* As an optimization, Mesa's fragment programs will sometimes get the
    * primary color from a statevar/constant rather than a varying variable.
    * when that's the case, we need to ensure that we use the 'color'
    * parameter and not the current attribute color (which may have changed
    * through glRasterPos and state validation.
    * So, we force the proper color here.  Not elegant, but it works.
    */
   {
      GLfloat colorSave[4];
      COPY_4V(colorSave, ctx->Current.Attrib[VERT_ATTRIB_COLOR0]);
      COPY_4V(ctx->Current.Attrib[VERT_ATTRIB_COLOR0], color);
      st_upload_constants(st, stfp->Base.Base.Parameters, PIPE_SHADER_FRAGMENT);
      COPY_4V(ctx->Current.Attrib[VERT_ATTRIB_COLOR0], colorSave);
   }


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
      cso_set_samplers(cso, num, (const struct pipe_sampler_state **) samplers);
   }

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
      vp.scale[2] = 0.5f;
      vp.scale[3] = 1.0f;
      vp.translate[0] = 0.5f * width;
      vp.translate[1] = 0.5f * height;
      vp.translate[2] = 0.5f;
      vp.translate[3] = 0.0f;
      cso_set_viewport(cso, &vp);
   }

   /* convert Z from [0,1] to [-1,-1] to match viewport Z scale/bias */
   z = z * 2.0 - 1.0;

   /* draw textured quad */
   offset = setup_bitmap_vertex_data(st, x, y, width, height, z, color);

   util_draw_vertex_buffer(pipe, st->bitmap.vbuf, offset,
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

   /*memset(cache->buffer, 0xff, sizeof(cache->buffer));*/
   cache->empty = GL_TRUE;

   cache->xmin = 1000000;
   cache->xmax = -1000000;
   cache->ymin = 1000000;
   cache->ymax = -1000000;

   if (cache->trans) {
      screen->tex_transfer_destroy(cache->trans);
      cache->trans = NULL;
   }

   assert(!cache->texture);

   /* allocate a new texture */
   cache->texture = st_texture_create(st, PIPE_TEXTURE_2D,
                                      st->bitmap.tex_format, 0,
                                      BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                                      1, PIPE_TEXTURE_USAGE_SAMPLER);
}


/** Print bitmap image to stdout (debug) */
static void
print_cache(const struct bitmap_cache *cache)
{
   int i, j, k;

   for (i = 0; i < BITMAP_CACHE_HEIGHT; i++) {
      k = BITMAP_CACHE_WIDTH * (BITMAP_CACHE_HEIGHT - i - 1);
      for (j = 0; j < BITMAP_CACHE_WIDTH; j++) {
         if (cache->buffer[k])
            printf("X");
         else
            printf(" ");
         k++;
      }
      printf("\n");
   }
}


static void
create_cache_trans(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct bitmap_cache *cache = st->bitmap.cache;

   if (cache->trans)
      return;

   /* Map the texture transfer.
    * Subsequent glBitmap calls will write into the texture image.
    */
   cache->trans = st_no_flush_get_tex_transfer(st, cache->texture, 0, 0, 0,
					       PIPE_TRANSFER_WRITE, 0, 0,
					       BITMAP_CACHE_WIDTH,
					       BITMAP_CACHE_HEIGHT);
   cache->buffer = screen->transfer_map(screen, cache->trans);

   /* init image to all 0xff */
   memset(cache->buffer, 0xff, cache->trans->stride * BITMAP_CACHE_HEIGHT);
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
 
/*         printf("flush size %d x %d  at %d, %d\n",
                cache->xmax - cache->xmin,
                cache->ymax - cache->ymin,
                cache->xpos, cache->ypos);
*/

         /* The texture transfer has been mapped until now.
          * So unmap and release the texture transfer before drawing.
          */
         if (cache->trans) {
            if (0)
               print_cache(cache);
            screen->transfer_unmap(screen, cache->trans);
            cache->buffer = NULL;

            screen->tex_transfer_destroy(cache->trans);
            cache->trans = NULL;
         }

         draw_bitmap_quad(st->ctx,
                          cache->xpos,
                          cache->ypos,
                          cache->zpos,
                          BITMAP_CACHE_WIDTH, BITMAP_CACHE_HEIGHT,
                          cache->texture,
                          cache->color);
      }

      /* release/free the texture */
      pipe_texture_reference(&cache->texture, NULL);

      reset_cache(st);
   }
}

/* Flush bitmap cache and release vertex buffer.
 */
void
st_flush_bitmap( struct st_context *st )
{
   st_flush_bitmap_cache(st);

   /* Release vertex buffer to avoid synchronous rendering if we were
    * to map it in the next frame.
    */
   pipe_buffer_reference(&st->bitmap.vbuf, NULL);
   st->bitmap.vbuf_slot = 0;
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
   int px = -999, py = -999;
   const GLfloat z = st->ctx->Current.RasterPos[2];

   if (width > BITMAP_CACHE_WIDTH ||
       height > BITMAP_CACHE_HEIGHT)
      return GL_FALSE; /* too big to cache */

   if (!cache->empty) {
      px = x - cache->xpos;  /* pos in buffer */
      py = y - cache->ypos;
      if (px < 0 || px + width > BITMAP_CACHE_WIDTH ||
          py < 0 || py + height > BITMAP_CACHE_HEIGHT ||
          !TEST_EQ_4V(st->ctx->Current.RasterColor, cache->color) ||
          ((fabs(z - cache->zpos) > Z_EPSILON))) {
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
      cache->zpos = z;
      cache->empty = GL_FALSE;
      COPY_4FV(cache->color, st->ctx->Current.RasterColor);
   }

   assert(px != -999);
   assert(py != -999);

   if (x < cache->xmin)
      cache->xmin = x;
   if (y < cache->ymin)
      cache->ymin = y;
   if (x + width > cache->xmax)
      cache->xmax = x + width;
   if (y + height > cache->ymax)
      cache->ymax = y + height;

   /* create the transfer if needed */
   create_cache_trans(st);

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
                                                          semantic_indexes);
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

   /* find a usable texture format */
   if (screen->is_format_supported(screen, PIPE_FORMAT_I8_UNORM, PIPE_TEXTURE_2D, 
                                   PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      st->bitmap.tex_format = PIPE_FORMAT_I8_UNORM;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_A8_UNORM, PIPE_TEXTURE_2D, 
                                        PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      st->bitmap.tex_format = PIPE_FORMAT_A8_UNORM;
   }
   else if (screen->is_format_supported(screen, PIPE_FORMAT_L8_UNORM, PIPE_TEXTURE_2D, 
                                        PIPE_TEXTURE_USAGE_SAMPLER, 0)) {
      st->bitmap.tex_format = PIPE_FORMAT_L8_UNORM;
   }
   else {
      /* XXX support more formats */
      assert(0);
   }

   /* alloc bitmap cache object */
   st->bitmap.cache = ST_CALLOC_STRUCT(bitmap_cache);

   reset_cache(st);
}


/** Per-context tear-down */
void
st_destroy_bitmap(struct st_context *st)
{
   struct pipe_context *pipe = st->pipe;
   struct pipe_screen *screen = pipe->screen;
   struct bitmap_cache *cache = st->bitmap.cache;



   if (st->bitmap.vs) {
      cso_delete_vertex_shader(st->cso_context, st->bitmap.vs);
      st->bitmap.vs = NULL;
   }

   if (st->bitmap.vbuf) {
      pipe_buffer_reference(&st->bitmap.vbuf, NULL);
      st->bitmap.vbuf = NULL;
   }

   if (cache) {
      if (cache->trans) {
         screen->transfer_unmap(screen, cache->trans);
         screen->tex_transfer_destroy(cache->trans);
      }
      pipe_texture_reference(&st->bitmap.cache->texture, NULL);
      free(st->bitmap.cache);
      st->bitmap.cache = NULL;
   }
}
