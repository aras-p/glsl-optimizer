/* $XFree86*/ /* -*- c-basic-offset: 3 -*- */
/**************************************************************************

Copyright 2003 Eric Anholt
All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
ERIC ANHOLT BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Eric Anholt <anholt@FreeBSD.org>
 */

#define AGP_VERT_REG_COUNT (5 + \
   ((SIS_STATES & SIS_VERT_TEX0) != 0) * 2 + \
   ((SIS_STATES & SIS_VERT_TEX1) != 0) * 2)

static void TAG(sis_draw_quad_mmio)( sisContextPtr smesa,
				     sisVertexPtr v0,
				     sisVertexPtr v1,
				     sisVertexPtr v2,
				     sisVertexPtr v3 )
{
   float *MMIOBase = (float *)GET_IOBase (smesa);

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 6 + 1);
   ((GLint *) MMIOBase)[REG_3D_PrimitiveSet / 4] = smesa->dwPrimitiveSet;
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 0);
   SIS_MMIO_WRITE_VERTEX(v3, 2, 1);
   SIS_MMIO_WRITE_VERTEX(v1, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v2, 1, 0);
   SIS_MMIO_WRITE_VERTEX(v3, 2, 1);
}

static void TAG(sis_draw_tri_mmio)( sisContextPtr smesa,
				    sisVertexPtr v0,
				    sisVertexPtr v1,
				    sisVertexPtr v2 )
{
   float *MMIOBase = (float *)GET_IOBase (smesa);

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 3 + 1);
   ((GLint *) MMIOBase)[REG_3D_PrimitiveSet / 4] = smesa->dwPrimitiveSet;
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 0);
   SIS_MMIO_WRITE_VERTEX(v2, 2, 1);
}

static void TAG(sis_draw_line_mmio)( sisContextPtr smesa,
				     sisVertexPtr v0,
				     sisVertexPtr v1 )
{
   float *MMIOBase = (float *)GET_IOBase (smesa);

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 2 + 1);
   ((GLint *) MMIOBase)[REG_3D_PrimitiveSet / 4] = smesa->dwPrimitiveSet;
   SIS_MMIO_WRITE_VERTEX(v0, 0, 0);
   SIS_MMIO_WRITE_VERTEX(v1, 1, 1);
}

static void TAG(sis_draw_point_mmio)( sisContextPtr smesa,
				      sisVertexPtr v0 )
{
   float *MMIOBase = (float *)GET_IOBase (smesa);

   mWait3DCmdQueue (MMIO_VERT_REG_COUNT * 1 + 1);
   ((GLint *) MMIOBase)[REG_3D_PrimitiveSet / 4] = smesa->dwPrimitiveSet;
   SIS_MMIO_WRITE_VERTEX(v0, 1, 1);
}

static void TAG(sis_draw_quad_agp)( sisContextPtr smesa,
				     sisVertexPtr v0,
				     sisVertexPtr v1,
				     sisVertexPtr v2,
				     sisVertexPtr v3 )
{
   sisMakeRoomAGP(smesa, AGP_VERT_REG_COUNT * 6);

   SIS_AGP_WRITE_VERTEX(v0);
   SIS_AGP_WRITE_VERTEX(v1);
   SIS_AGP_WRITE_VERTEX(v3);
   SIS_AGP_WRITE_VERTEX(v1);
   SIS_AGP_WRITE_VERTEX(v2);
   SIS_AGP_WRITE_VERTEX(v3);
}

static void TAG(sis_draw_tri_agp)( sisContextPtr smesa,
				    sisVertexPtr v0,
				    sisVertexPtr v1,
				    sisVertexPtr v2 )
{
   sisMakeRoomAGP(smesa, AGP_VERT_REG_COUNT * 3);

   SIS_AGP_WRITE_VERTEX(v0);
   SIS_AGP_WRITE_VERTEX(v1);
   SIS_AGP_WRITE_VERTEX(v2);
}

static void TAG(sis_draw_line_agp)( sisContextPtr smesa,
				     sisVertexPtr v0,
				     sisVertexPtr v1 )
{
   sisMakeRoomAGP(smesa, AGP_VERT_REG_COUNT * 2);

   SIS_AGP_WRITE_VERTEX(v0);
   SIS_AGP_WRITE_VERTEX(v1);
}

static void TAG(sis_draw_point_agp)( sisContextPtr smesa,
				      sisVertexPtr v0 )
{
   sisMakeRoomAGP(smesa, AGP_VERT_REG_COUNT * 1);

   SIS_AGP_WRITE_VERTEX(v0);
}

static __inline void TAG(sis_vert_init)( void )
{
   sis_quad_func_agp[SIS_STATES] = TAG(sis_draw_quad_agp);
   sis_tri_func_agp[SIS_STATES] = TAG(sis_draw_tri_agp);
   sis_line_func_agp[SIS_STATES] = TAG(sis_draw_line_agp);
   sis_point_func_agp[SIS_STATES] = TAG(sis_draw_point_agp);
   sis_quad_func_mmio[SIS_STATES] = TAG(sis_draw_quad_mmio);
   sis_tri_func_mmio[SIS_STATES] = TAG(sis_draw_tri_mmio);
   sis_line_func_mmio[SIS_STATES] = TAG(sis_draw_line_mmio);
   sis_point_func_mmio[SIS_STATES] = TAG(sis_draw_point_mmio);
}

#undef AGP_VERT_REG_COUNT
#undef TAG
#undef SIS_STATES
