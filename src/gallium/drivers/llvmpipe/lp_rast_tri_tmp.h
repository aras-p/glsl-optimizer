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
 * Prototype for a 7 plane rasterizer function.  Will codegenerate
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
   unsigned mask = 0;
   int i;

   for (i = 0; i < 16; i++) {
      int any_negative = 0;
      int j;

      for (j = 0; j < NR_PLANES; j++) 
         any_negative |= (c[j] - 1 + plane[j].step[i]);
         
      any_negative >>= 31;

      mask |= (~any_negative) & (1 << i);
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
   unsigned i, j;

   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   for (j = 0; j < NR_PLANES; j++) {
      const int *step = plane[j].step;
      const int eo = plane[j].eo * 4;
      const int ei = plane[j].ei * 4;
      const int cox = c[j] + eo;
      const int cio = ei - 1 - eo;

      for (i = 0; i < 16; i++) {
         int out = cox + step[i] * 4;
         int part = out + cio;
         outmask  |= (out >> 31) & (1 << i);
         partmask |= (part >> 31) & (1 << i);
      }
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

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int px = x + pos_table4[i][0];
      int py = y + pos_table4[i][1];
      int cx[NR_PLANES];

      for (j = 0; j < NR_PLANES; j++)
         cx[j] = c[j] + plane[j].step[i] * 4;

      partial_mask &= ~(1 << i);

      TAG(do_block_4)(task, tri, plane, px, py, cx);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int px = x + pos_table4[i][0];
      int py = y + pos_table4[i][1];

      inmask &= ~(1 << i);

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
   unsigned i, j, nr_planes = 0;

   while (plane_mask) {
      int i = ffs(plane_mask) - 1;
      plane[nr_planes] = tri->plane[i];
      plane_mask &= ~(1 << i);
      nr_planes++;
   };

   assert(nr_planes == NR_PLANES);
   outmask = 0;                 /* outside one or more trivial reject planes */
   partmask = 0;                /* outside one or more trivial accept planes */

   for (j = 0; j < NR_PLANES; j++) {
      const int *step = plane[j].step;
      const int eo = plane[j].eo * 16;
      const int ei = plane[j].ei * 16;
      int cox, cio;

      c[j] = plane[j].c + plane[j].dcdy * y - plane[j].dcdx * x;
      cox = c[j] + eo;
      cio = ei - 1 - eo;

      for (i = 0; i < 16; i++) {
         int out = cox + step[i] * 16;
         int part = out + cio;
         outmask  |= (out >> 31) & (1 << i);
         partmask |= (part >> 31) & (1 << i);
      }
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

   /* Iterate over partials:
    */
   while (partial_mask) {
      int i = ffs(partial_mask) - 1;
      int px = x + pos_table16[i][0];
      int py = y + pos_table16[i][1];
      int cx[NR_PLANES];

      for (j = 0; j < NR_PLANES; j++)
         cx[j] = c[j] + plane[j].step[i] * 16;

      partial_mask &= ~(1 << i);

      LP_COUNT(nr_partially_covered_16);
      TAG(do_block_16)(task, tri, plane, px, py, cx);
   }

   /* Iterate over fulls: 
    */
   while (inmask) {
      int i = ffs(inmask) - 1;
      int px = x + pos_table16[i][0];
      int py = y + pos_table16[i][1];

      inmask &= ~(1 << i);

      LP_COUNT(nr_fully_covered_16);
      block_full_16(task, tri, px, py);
   }
}

#undef TAG
#undef NR_PLANES

