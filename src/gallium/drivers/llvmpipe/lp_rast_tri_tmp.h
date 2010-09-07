/**************************************************************************
 *
 * Copyright 2007-2010 VMware, Inc.
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

/*
 * Rasterization for binned triangles within a tile
 */



/**
 * Prototype for a 8 plane rasterizer function.  Will codegenerate
 * several of these.
 *
 * XXX: Varients for more/fewer planes.
 * XXX: Need ways of dropping planes as we descend.
 * XXX: SIMD
 */
static void
TAG(do_block_4)(struct lp_rasterizer_task *task,
                const struct lp_rast_triangle *tri,
                const struct lp_rast_plane *plane,
                int x, int y,
                const int *c)
{
   unsigned mask = 0xffff;
   int j;

   for (j = 0; j < NR_PLANES; j++) {
      mask &= ~build_mask_linear(c[j] - 1, 
				 -plane[j].dcdx,
				 plane[j].dcdy);
   }

   /* Now pass to the shader:
    */
   if (mask)
      lp_rast_shade_quads_mask(task, &tri->inputs, x, y, mask);
}

/**
 * Evaluate a 16x16 block of pixels to determine which 4x4 subblocks are in/out
 * of the triangle's bounds.
 */
static void
TAG(do_block_16)(struct lp_rasterizer_task *task,
                 const struct lp_rast_triangle *tri,
                 const struct lp_rast_plane *plane,
                 int x, int y,
                 const int *c)
{
   unsigned outmask, inmask, partmask, partial_mask;
   unsigned j;

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   for (j = 0; j < NR_PLANES; j++) {
      const int dcdx = -plane[j].dcdx * 4;
      const int dcdy = plane[j].dcdy * 4;
      const int cox = plane[j].eo * 4;
      const int cio = plane[j].ei * 4 - 1;

      build_masks(c[j] + cox,
		  cio - cox,
		  dcdx, dcdy, 
		  &outmask,   /* sign bits from c[i][0..15] + cox */
		  &partmask); /* sign bits from c[i][0..15] + cio */
   }

   if (outmask == 0xffff)
      return;

   /* Mask of sub-blocks which are inside all trivial accept planes:
    */
   inmask = ~partmask & 0xffff;

   /* Mask of sub-blocks which are inside all trivial reject planes,
    * but outside at least one trivial accept plane:
    */
   partial_mask = partmask & ~outmask;

   assert((partial_mask & inmask) == 0);

   LP_COUNT_ADD(nr_empty_4, util_bitcount(0xffff & ~(partial_mask | inmask)));

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 
      int cx[NR_PLANES];

      partial_mask &= ~(1 << i);

      LP_COUNT(nr_partially_covered_4);

      for (j = 0; j < NR_PLANES; j++)
         cx[j] = (c[j] 
		  - plane[j].dcdx * ix
		  + plane[j].dcdy * iy);

      TAG(do_block_4)(task, tri, plane, px, py, cx);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int ix = (i & 3) * 4;
      int iy = (i >> 2) * 4;
      int px = x + ix;
      int py = y + iy; 

      inmask &= ~(1 << i);

      LP_COUNT(nr_fully_covered_4);
      block_full_4(task, tri, px, py);
   }
}


/**
 * Scan the tile in chunks and figure out which pixels to rasterize
 * for this triangle.
 */
void
TAG(lp_rast_triangle)(struct lp_rasterizer_task *task,
                      const union lp_rast_cmd_arg arg)
{
   const struct lp_rast_triangle *tri = arg.triangle.tri;
   unsigned plane_mask = arg.triangle.plane_mask;
   const int x = task->x, y = task->y;
   struct lp_rast_plane plane[NR_PLANES];
   int c[NR_PLANES];
   unsigned outmask, inmask, partmask, partial_mask;
   unsigned j = 0;

   if (tri->inputs.disable) {
      /* This triangle was partially binned and has been disabled */
      return;
   }

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   if (tri->inputs.disable) {
      /* This triangle was partially binned and has been disabled */
      return;
   }

   while (plane_mask) {
      int i = ffs(plane_mask) - 1;
      plane[j] = tri->plane[i];
      plane_mask &= ~(1 << i);
      c[j] = plane[j].c + plane[j].dcdy * y - plane[j].dcdx * x;

      {
	 const int dcdx = -plane[j].dcdx * 16;
	 const int dcdy = plane[j].dcdy * 16;
	 const int cox = plane[j].eo * 16;
	 const int cio = plane[j].ei * 16 - 1;

	 build_masks(c[j] + cox,
		     cio - cox,
		     dcdx, dcdy, 
		     &outmask,   /* sign bits from c[i][0..15] + cox */
		     &partmask); /* sign bits from c[i][0..15] + cio */
      }

      j++;
   }

   if (outmask == 0xffff)
      return;

   /* Mask of sub-blocks which are inside all trivial accept planes:
    */
   inmask = ~partmask & 0xffff;

   /* Mask of sub-blocks which are inside all trivial reject planes,
    * but outside at least one trivial accept plane:
    */
   partial_mask = partmask & ~outmask;

   assert((partial_mask & inmask) == 0);

   LP_COUNT_ADD(nr_empty_16, util_bitcount(0xffff & ~(partial_mask | inmask)));

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int ix = (i & 3) * 16;
      int iy = (i >> 2) * 16;
      int px = x + ix;
      int py = y + iy;
      int cx[NR_PLANES];

      for (j = 0; j < NR_PLANES; j++)
         cx[j] = (c[j]
		  - plane[j].dcdx * ix
		  + plane[j].dcdy * iy);

      partial_mask &= ~(1 << i);

      LP_COUNT(nr_partially_covered_16);
      TAG(do_block_16)(task, tri, plane, px, py, cx);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int ix = (i & 3) * 16;
      int iy = (i >> 2) * 16;
      int px = x + ix;
      int py = y + iy;

      inmask &= ~(1 << i);

      LP_COUNT(nr_fully_covered_16);
      block_full_16(task, tri, px, py);
   }
}

#undef TAG
#undef NR_PLANES

