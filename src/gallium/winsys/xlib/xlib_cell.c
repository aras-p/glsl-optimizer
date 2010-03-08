/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Bismarck, ND., USA
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR 
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE 
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * 
 **************************************************************************/

/*
 * Authors:
 *   Keith Whitwell
 *   Brian Paul
 */




#include "xlib.h"


#if defined(GALLIUM_CELL)

#include "cell/ppu/cell_texture.h"
#include "cell/ppu/cell_screen.h"
#include "state_tracker/sw_winsys.h"
#include "util/u_debug.h"



/**
 * For Cell.  Basically, rearrange the pixels/quads from this layout:
 *  +--+--+--+--+
 *  |p0|p1|p2|p3|....
 *  +--+--+--+--+
 *
 * to this layout:
 *  +--+--+
 *  |p0|p1|....
 *  +--+--+
 *  |p2|p3|
 *  +--+--+
 */
static void
twiddle_tile(const uint *tileIn, uint *tileOut)
{
   int y, x;

   for (y = 0; y < TILE_SIZE; y+=2) {
      for (x = 0; x < TILE_SIZE; x+=2) {
         int k = 4 * (y/2 * TILE_SIZE/2 + x/2);
         tileOut[y * TILE_SIZE + (x + 0)] = tileIn[k];
         tileOut[y * TILE_SIZE + (x + 1)] = tileIn[k+1];
         tileOut[(y + 1) * TILE_SIZE + (x + 0)] = tileIn[k+2];
         tileOut[(y + 1) * TILE_SIZE + (x + 1)] = tileIn[k+3];
      }
   }
}

/**
 * Display a surface that's in a tiled configuration.  That is, all the
 * pixels for a TILE_SIZExTILE_SIZE block are contiguous in memory.
 */
static void
xm_displaytarget_display(struct sw_winsys *ws,
                         struct sw_displaytarget *dt,
                         void *winsys_drawable)
{
   struct xmesa_buffer *xm_buffer = (struct xm_drawable *)winsys_drawable;
   XImage *ximage;
   struct xm_buffer *xm_buf = xm_buffer(
      cell_texture(surf->texture)->buffer);
   const uint tilesPerRow = (surf->width + TILE_SIZE - 1) / TILE_SIZE;
   uint x, y;

   ximage = b->tempImage;

   /* check that the XImage has been previously initialized */
   assert(ximage->format);
   assert(ximage->bitmap_unit);

   /* update XImage's fields */
   ximage->width = TILE_SIZE;
   ximage->height = TILE_SIZE;
   ximage->bytes_per_line = TILE_SIZE * 4;

   for (y = 0; y < surf->height; y += TILE_SIZE) {
      for (x = 0; x < surf->width; x += TILE_SIZE) {
         uint tmpTile[TILE_SIZE * TILE_SIZE];
         int tx = x / TILE_SIZE;
         int ty = y / TILE_SIZE;
         int offset = ty * tilesPerRow + tx;
         int w = TILE_SIZE;
         int h = TILE_SIZE;

         if (y + h > surf->height)
            h = surf->height - y;
         if (x + w > surf->width)
            w = surf->width - x;

         /* offset in pixels */
         offset *= TILE_SIZE * TILE_SIZE;

         /* twiddle from ximage buffer to temp tile */
         twiddle_tile((uint *) xm_buf->data + offset, tmpTile);
         /* display temp tile data */
         ximage->data = (char *) tmpTile;
         XPutImage(b->xm_visual->display, b->drawable, b->gc,
                   ximage, 0, 0, x, y, w, h);
      }
   }
}



static struct pipe_screen *
xlib_create_cell_screen( Display *dpy )
{
   struct sw_winsys *winsys;
   struct pipe_screen *screen;

   winsys = xlib_create_sw_winsys( dpy );
   if (winsys == NULL)
      return NULL;

   /* Plug in a little cell-specific code:
    */

   ws->base.displaytarget_display = xm_cell_displaytarget_display;

   screen = cell_create_screen(winsys);
   if (screen == NULL)
      goto fail;

   return screen;

fail:
   if (winsys)
      winsys->destroy( winsys );

   return NULL;
}




struct xm_driver xlib_cell_driver = 
{
   .create_pipe_screen = xlib_create_cell_screen,
};



#endif /* GALLIUM_CELL */
