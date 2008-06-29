/**************************************************************************
 * 
 * Copyright 2006 Tungsten Graphics, Inc., Cedar Park, Texas.
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
  *   Keith Whitwell <keith@tungstengraphics.com>
  *   Michel Dänzer <michel@tungstengraphics.com>
  */

#include "pipe/p_context.h"
#include "pipe/p_defines.h"
#include "pipe/p_inlines.h"
#include "pipe/p_util.h"
#include "pipe/p_winsys.h"

#include "cell_context.h"
#include "cell_state.h"
#include "cell_texture.h"


/* Simple, maximally packed layout.
 */

static unsigned minify( unsigned d )
{
   return MAX2(1, d>>1);
}


static void
cell_texture_layout(struct cell_texture * spt)
{
   struct pipe_texture *pt = &spt->base;
   unsigned level;
   unsigned width = pt->width[0];
   unsigned height = pt->height[0];
   unsigned depth = pt->depth[0];

   spt->buffer_size = 0;

   for ( level = 0 ; level <= pt->last_level ; level++ ) {
      pt->width[level] = width;
      pt->height[level] = height;
      pt->depth[level] = depth;
      pt->nblocksx[level] = pf_get_nblocksx(&pt->block, width);  
      pt->nblocksy[level] = pf_get_nblocksy(&pt->block, height);  

      spt->level_offset[level] = spt->buffer_size;

      spt->buffer_size += (pt->nblocksy[level] *
			  ((pt->target == PIPE_TEXTURE_CUBE) ? 6 : depth) *
			  pt->nblocksx[level] * pt->block.size;

      width  = minify(width);
      height = minify(height);
      depth = minify(depth);
   }
}


static struct pipe_texture *
cell_texture_create_screen(struct pipe_screen *screen,
                           const struct pipe_texture *templat)
{
   struct pipe_winsys *ws = screen->winsys;
   struct cell_texture *spt = CALLOC_STRUCT(cell_texture);
   if (!spt)
      return NULL;

   spt->base = *templat;
   spt->base.refcount = 1;
   spt->base.screen = screen;

   cell_texture_layout(spt);

   spt->buffer = ws->buffer_create(ws, 32,
                                   PIPE_BUFFER_USAGE_PIXEL,
                                   spt->buffer_size);

   if (!spt->buffer) {
      FREE(spt);
      return NULL;
   }

   return &spt->base;
}


static void
cell_texture_release_screen(struct pipe_screen *screen,
                            struct pipe_texture **pt)
{
   if (!*pt)
      return;

   /*
   DBG("%s %p refcount will be %d\n",
       __FUNCTION__, (void *) *pt, (*pt)->refcount - 1);
   */
   if (--(*pt)->refcount <= 0) {
      struct cell_texture *spt = cell_texture(*pt);

      /*
      DBG("%s deleting %p\n", __FUNCTION__, (void *) spt);
      */

      pipe_buffer_reference(screen->winsys, &spt->buffer, NULL);

      FREE(spt);
   }
   *pt = NULL;
}


static void
cell_texture_update(struct pipe_context *pipe, struct pipe_texture *texture,
                    uint face, uint levelsMask)
{
   /* XXX TO DO:  re-tile the texture data ... */

}


static struct pipe_surface *
cell_get_tex_surface_screen(struct pipe_screen *screen,
                            struct pipe_texture *pt,
                            unsigned face, unsigned level, unsigned zslice)
{
   struct pipe_winsys *ws = screen->winsys;
   struct cell_texture *spt = cell_texture(pt);
   struct pipe_surface *ps;

   ps = ws->surface_alloc(ws);
   if (ps) {
      assert(ps->refcount);
      assert(ps->winsys);
      pipe_buffer_reference(ws, &ps->buffer, spt->buffer);
      ps->format = pt->format;
      ps->block = pt->block;
      ps->width = pt->width[level];
      ps->height = pt->height[level];
      ps->nblocksx = pt->nblocksx[level];
      ps->nblocksy = pt->nblocksy[level];
      ps->stride = spt->stride[level];
      ps->offset = spt->level_offset[level];

      if (pt->target == PIPE_TEXTURE_CUBE || pt->target == PIPE_TEXTURE_3D) {
	 ps->offset += ((pt->target == PIPE_TEXTURE_CUBE) ? face : zslice) *
		       ps->nblocksy *
		       ps->stride;
      } else {
	 assert(face == 0);
	 assert(zslice == 0);
      }
   }
   return ps;
}



static void
tile_copy_data(uint w, uint h, uint tile_size, uint *dst, const uint *src)
{
   const uint tile_size2 = tile_size * tile_size;
   const uint h_t = h / tile_size, w_t = w / tile_size;

   uint it, jt;  /* tile counters */
   uint i, j;    /* intra-tile counters */

   /* loop over dest tiles */
   for (it = 0; it < h_t; it++) {
      for (jt = 0; jt < w_t; jt++) {
         /* start of dest tile: */
         uint *tdst = dst + (it * w_t + jt) * tile_size2;
         /* loop over texels in the tile */
         for (i = 0; i < tile_size; i++) {
            for (j = 0; j < tile_size; j++) {
               const uint srci = it * tile_size + i;
               const uint srcj = jt * tile_size + j;
               *tdst++ = src[srci * w + srcj];
            }
         }
      }
   }
}



/**
 * Convert linear texture image data to tiled format for SPU usage.
 */
static void
cell_tile_texture(struct cell_context *cell,
                  struct cell_texture *texture)
{
   struct pipe_screen *screen = cell->pipe.screen;
   uint face = 0, level = 0, zslice = 0;
   struct pipe_surface *surf;
   const uint w = texture->base.width[0], h = texture->base.height[0];
   const uint *src;

   /* temporary restrictions: */
   assert(w >= TILE_SIZE);
   assert(h >= TILE_SIZE);
   assert(w % TILE_SIZE == 0);
   assert(h % TILE_SIZE == 0);

   surf = screen->get_tex_surface(screen, &texture->base, face, level, zslice);
   ASSERT(surf);

   src = (const uint *) pipe_surface_map(surf);

   if (texture->tiled_data) {
      align_free(texture->tiled_data);
   }
   texture->tiled_data = align_malloc(w * h * 4, 16);

   tile_copy_data(w, h, TILE_SIZE, texture->tiled_data, src);

   pipe_surface_unmap(surf);

   pipe_surface_reference(&surf, NULL);
}



void
cell_update_texture_mapping(struct cell_context *cell)
{
   uint face = 0, level = 0, zslice = 0;
   uint i;

   for (i = 0; i < CELL_MAX_SAMPLERS; i++) {
      if (cell->texture[i])
         cell_tile_texture(cell, cell->texture[i]);
   }

#if 0
   if (cell->tex_surf && cell->tex_map) {
      pipe_surface_unmap(cell->tex_surf);
      cell->tex_map = NULL;
   }

   /* XXX free old surface */

   cell->tex_surf = cell_get_tex_surface(&cell->pipe,
                                         &cell->texture[0]->base,
                                         face, level, zslice);

   cell->tex_map = pipe_surface_map(cell->tex_surf);
#endif
}


void
cell_init_texture_functions(struct cell_context *cell)
{
   cell->pipe.texture_update = cell_texture_update;
}

void
cell_init_screen_texture_funcs(struct pipe_screen *screen)
{
   screen->texture_create = cell_texture_create_screen;
   screen->texture_release = cell_texture_release_screen;
   screen->get_tex_surface = cell_get_tex_surface_screen;
}
