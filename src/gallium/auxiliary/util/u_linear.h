
#ifndef U_LINEAR_H
#define U_LINEAR_H

#include "pipe/p_format.h"

struct pipe_tile_info
{
   unsigned size;
   unsigned stride;

   /* The number of tiles */
   unsigned tiles_x;
   unsigned tiles_y;

   /* size of each tile expressed in blocks */
   unsigned cols;
   unsigned rows;

   /* Describe the tile in pixels */
   struct pipe_format_block tile;

   /* Describe each block within the tile */
   struct pipe_format_block block;
};

void pipe_linear_to_tile(size_t src_stride, const void *src_ptr,
			 struct pipe_tile_info *t, void  *dst_ptr);

void pipe_linear_from_tile(struct pipe_tile_info *t, const void *src_ptr,
			   size_t dst_stride, void *dst_ptr);

/**
 * Convenience function to fillout a pipe_tile_info struct.
 * @t info to fill out.
 * @block block info about pixel layout
 * @tile_width the width of the tile in pixels
 * @tile_height the height of the tile in pixels
 * @tiles_x number of tiles in x axis
 * @tiles_y number of tiles in y axis
 */
void pipe_linear_fill_info(struct pipe_tile_info *t,
			   const struct pipe_format_block *block,
			   unsigned tile_width, unsigned tile_height,
			   unsigned tiles_x, unsigned tiles_y);

static INLINE boolean pipe_linear_check_tile(const struct pipe_tile_info *t)
{
   if (t->tile.size != t->block.size * t->cols * t->rows)
      return FALSE;

   if (t->stride != t->block.size * t->cols * t->tiles_x)
      return FALSE;

   if (t->size < t->stride * t->rows * t->tiles_y)
      return FALSE;

   return TRUE;
}

#endif /* U_LINEAR_H */
