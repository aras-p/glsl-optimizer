/*
 * Mesa 3-D graphics library
 * Version:  5.0
 * 
 * Copyright (C) 1999-2002  Brian Paul   All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

/*
 * DOS/DJGPP device driver v1.3 for Mesa  --  MGA2064W triangle template
 *
 *  Copyright (c) 2003 - Borca Daniel
 *  Email : dborca@yahoo.com
 *  Web   : http://www.geocities.com/dborca
 */


/*
 * Triangle Rasterizer Template
 *
 * This file is #include'd to generate custom triangle rasterizers.
 *
 * The following macros may be defined to indicate what auxillary information
 * must be interplated across the triangle:
 *    INTERP_Z        - if defined, interpolate Z values
 *    INTERP_RGB      - if defined, interpolate RGB values
 *
 *    TAG             - function name
 *    CULL            - enable culling for: 0=no, 1=back, -1=front
 *
 *    SETUP_CODE      - to be executed once per triangle (usually HW init)
 *
 * For flatshaded primitives, the provoking vertex is the final one.
 * This code was designed for the origin to be in the upper-left corner.
 *
 * Inspired by triangle rasterizer code written by Brian Paul.
 */



#define TRI_SWAP(a, b)        \
do {                          \
    const MGAvertex *tmp = a; \
    a = b;                    \
    b = tmp;                  \
} while (0)

void TAG (int cull, const MGAvertex *v1, const MGAvertex *v2, const MGAvertex *v3)
{
 int area;
 int x1, y1, x2, y2, x3, y3;
 int eMaj_dx, eMaj_dy, eBot_dx, eBot_dy, eTop_dx, eTop_dy;
#ifdef INTERP_RGB
#define FIFO_CNT_RGB 3
 int eMaj_dr, eBot_dr, eMaj_dg, eBot_dg, eMaj_db, eBot_db;
 int drdx, drdy, dgdx, dgdy, dbdx, dbdy;
#else
#define FIFO_CNT_RGB 0
#endif
#ifdef INTERP_Z
#define FIFO_CNT_Z 1
 int dzdx, dzdy;
 int eMaj_dz, eBot_dz;
 int z1, z2, z3;
#else
#define FIFO_CNT_Z 0
#endif

#if defined(INTERP_Z) || defined(INTERP_RGB)
 double one_area;
#ifndef INTERP_RGB
 int red = v3->color[0];
 int green = v3->color[1];
 int blue = v3->color[2];
#endif
#else
 unsigned long color = mga_mixrgb_full(v3->color);
#endif

 int sgn = 0;

 /* sort along the vertical axis */
 if (v2->win[1] < v1->win[1]) {
    TRI_SWAP(v1, v2);
#ifdef CULL
    cull = -cull;
#endif
 }

 if (v3->win[1] < v1->win[1]) {
    TRI_SWAP(v1, v3);
    TRI_SWAP(v2, v3);
 } else if (v3->win[1] < v2->win[1]) {
    TRI_SWAP(v2, v3);
#ifdef CULL
    cull = -cull;
#endif    
 }

 x1 = v1->win[0];
 y1 = v1->win[1];
 x2 = v2->win[0];
 y2 = v2->win[1];
 x3 = v3->win[0];
 y3 = v3->win[1];

 /* compute deltas for each edge */
 eMaj_dx = x3 - x1;
 eMaj_dy = y3 - y1;
 eBot_dx = x2 - x1;
 eBot_dy = y2 - y1;
 eTop_dx = x3 - x2;
 eTop_dy = y3 - y2;

 /* compute area */
 if ((area = eMaj_dx * eBot_dy - eBot_dx * eMaj_dy) == 0) {
    return;
 }
#ifdef CULL
 if ((area * cull) > 0) {
    return;
 }
#endif

 mga_select();

 /* set engine state */
#ifdef SETUP_CODE
 SETUP_CODE
#endif

 /* draw lower triangle */
#if defined(INTERP_Z) || defined(INTERP_RGB)
 one_area = (double)(1<<15) / (double)area;
 mga_fifo(1);
#else
 mga_fifo(2);
 mga_outl(M_FCOL, color);
#endif
 mga_outl(M_YDST, y1);

#ifdef INTERP_Z
 z1 = v1->win[2];
 z2 = v2->win[2];
 z3 = v3->win[2];

 /* compute d?/dx and d?/dy derivatives */
 eMaj_dz = z3 - z1;
 eBot_dz = z2 - z1;
 dzdx = (eMaj_dz * eBot_dy - eMaj_dy * eBot_dz) * one_area;
 dzdy = (eMaj_dx * eBot_dz - eMaj_dz * eBot_dx) * one_area;

#ifndef INTERP_RGB
 mga_fifo(11);
 mga_outl(M_DR2, dzdx);
 mga_outl(M_DR3, dzdy);
 mga_outl(M_DR4, red<<15);
 mga_outl(M_DR6, 0);
 mga_outl(M_DR7, 0);
 mga_outl(M_DR8, green<<15);
 mga_outl(M_DR10, 0);
 mga_outl(M_DR11, 0);
 mga_outl(M_DR12, blue<<15);
 mga_outl(M_DR14, 0);
 mga_outl(M_DR15, 0);
#else
 mga_fifo(2);
 mga_outl(M_DR2, dzdx);
 mga_outl(M_DR3, dzdy);
#endif
#endif

#ifdef INTERP_RGB
 /* compute color deltas */
 eMaj_dr = v3->color[0] - v1->color[0];
 eBot_dr = v2->color[0] - v1->color[0];
 eMaj_dg = v3->color[1] - v1->color[1];
 eBot_dg = v2->color[1] - v1->color[1];
 eMaj_db = v3->color[2] - v1->color[2];
 eBot_db = v2->color[2] - v1->color[2];

 /* compute color increments */
 drdx = (eMaj_dr * eBot_dy - eMaj_dy * eBot_dr) * one_area;
 drdy = (eMaj_dx * eBot_dr - eMaj_dr * eBot_dx) * one_area;
 dgdx = (eMaj_dg * eBot_dy - eMaj_dy * eBot_dg) * one_area;
 dgdy = (eMaj_dx * eBot_dg - eMaj_dg * eBot_dx) * one_area;
 dbdx = (eMaj_db * eBot_dy - eMaj_dy * eBot_db) * one_area;
 dbdy = (eMaj_dx * eBot_db - eMaj_db * eBot_dx) * one_area;

 mga_fifo(6);
 mga_outl(M_DR6, drdx);
 mga_outl(M_DR7, drdy);
 mga_outl(M_DR10, dgdx);
 mga_outl(M_DR11, dgdy);
 mga_outl(M_DR14, dbdx);
 mga_outl(M_DR15, dbdy);
#endif

 if (area > 0) { /* major edge on the right */
    if (eBot_dy) { /* have lower triangle */
       mga_fifo(9 + FIFO_CNT_Z + FIFO_CNT_RGB);

       mga_outl(M_AR0, eBot_dy);
       if (x2 < x1) {
          mga_outl(M_AR1, eBot_dx + eBot_dy - 1);
          mga_outl(M_AR2, eBot_dx);
          sgn |= M_SDXL;
       } else {
          mga_outl(M_AR1, -eBot_dx);
          mga_outl(M_AR2, -eBot_dx);
       }

       mga_outl(M_AR6, eMaj_dy);
       if (x3 < x1) {
          mga_outl(M_AR4, eMaj_dx + eMaj_dy - 1);
          mga_outl(M_AR5, eMaj_dx);
          sgn |= M_SDXR;
       } else {
          mga_outl(M_AR4, -eMaj_dx);
          mga_outl(M_AR5, -eMaj_dx);
       }

       mga_outl(M_FXBNDRY, (x1<<16) | x1);
#ifdef INTERP_Z
       mga_outl(M_DR0, z1<<15);
#endif
#ifdef INTERP_RGB
       mga_outl(M_DR4, v1->color[0]<<15);
       mga_outl(M_DR8, v1->color[1]<<15);
       mga_outl(M_DR12, v1->color[2]<<15);
#endif
       mga_outl(M_SGN, sgn);
       mga_outl(M_LEN | M_EXEC, eBot_dy);
    } else { /* no lower triangle */
       mga_fifo(4 + FIFO_CNT_Z + FIFO_CNT_RGB);

       mga_outl(M_AR6, eMaj_dy);
       if (x3 < x1) {
          mga_outl(M_AR4, eMaj_dx + eMaj_dy - 1);
          mga_outl(M_AR5, eMaj_dx);
          sgn |= M_SDXR;
       } else {
          mga_outl(M_AR4, -eMaj_dx);
          mga_outl(M_AR5, -eMaj_dx);
       }

       mga_outl(M_FXBNDRY, (x1<<16) | x2);
#ifdef INTERP_Z
       mga_outl(M_DR0, z2<<15);
#endif
#ifdef INTERP_RGB
       mga_outl(M_DR4, v2->color[0]<<15);
       mga_outl(M_DR8, v2->color[1]<<15);
       mga_outl(M_DR12, v2->color[2]<<15);
#endif
    }

    /* draw upper triangle */
    if (eTop_dy) {
       mga_fifo(5);
       mga_outl(M_AR0, eTop_dy);
       if (x3 < x2) {
          mga_outl(M_AR1, eTop_dx + eTop_dy - 1);
          mga_outl(M_AR2, eTop_dx);
          sgn |= M_SDXL;
       } else {
          mga_outl(M_AR1, -eTop_dx);
          mga_outl(M_AR2, -eTop_dx);
          sgn &= ~M_SDXL;
       }
       mga_outl(M_SGN, sgn);
       mga_outl(M_LEN | M_EXEC, eTop_dy);
    }
 } else { /* major edge on the left */
    if (eBot_dy) { /* have lower triangle */
       mga_fifo(9 + FIFO_CNT_Z + FIFO_CNT_RGB);

       mga_outl(M_AR0, eMaj_dy);
       if (x3 < x1) {
          mga_outl(M_AR1, eMaj_dx + eMaj_dy - 1);
          mga_outl(M_AR2, eMaj_dx);
          sgn |= M_SDXL;
       } else {
          mga_outl(M_AR1, -eMaj_dx);
          mga_outl(M_AR2, -eMaj_dx);
       }

       mga_outl(M_AR6, eBot_dy);
       if (x2 < x1) {
          mga_outl(M_AR4, eBot_dx + eBot_dy - 1);
          mga_outl(M_AR5, eBot_dx);
          sgn |= M_SDXR;
       } else {
          mga_outl(M_AR4, -eBot_dx);
          mga_outl(M_AR5, -eBot_dx);
       }

       mga_outl(M_FXBNDRY, (x1<<16) | x1);
#ifdef INTERP_Z
       mga_outl(M_DR0, z1<<15);
#endif
#ifdef INTERP_RGB
       mga_outl(M_DR4, v1->color[0]<<15);
       mga_outl(M_DR8, v1->color[1]<<15);
       mga_outl(M_DR12, v1->color[2]<<15);
#endif
       mga_outl(M_SGN, sgn);
       mga_outl(M_LEN | M_EXEC, eBot_dy);
    } else { /* no lower triangle */
       mga_fifo(4 + FIFO_CNT_Z + FIFO_CNT_RGB);

       mga_outl(M_AR0, eMaj_dy);
       if (x3 < x1) {
          mga_outl(M_AR1, eMaj_dx + eMaj_dy - 1);
          mga_outl(M_AR2, eMaj_dx);
          sgn |= M_SDXL;
       } else {
          mga_outl(M_AR1, -eMaj_dx);
          mga_outl(M_AR2, -eMaj_dx);
       }

       mga_outl(M_FXBNDRY, (x2<<16) | x1);
#ifdef INTERP_Z
       mga_outl(M_DR0, z1<<15);
#endif
#ifdef INTERP_RGB
       mga_outl(M_DR4, v1->color[0]<<15);
       mga_outl(M_DR8, v1->color[1]<<15);
       mga_outl(M_DR12, v1->color[2]<<15);
#endif
    }

    /* draw upper triangle */
    if (eTop_dy) {
       mga_fifo(5);
       mga_outl(M_AR6, eTop_dy);
       if (x3 < x2) {
          mga_outl(M_AR4, eTop_dx + eTop_dy - 1);
          mga_outl(M_AR5, eTop_dx);
          sgn |= M_SDXR;
       } else {
          mga_outl(M_AR4, -eTop_dx);
          mga_outl(M_AR5, -eTop_dx);
          sgn &= ~M_SDXR;
       }
       mga_outl(M_SGN, sgn);
       mga_outl(M_LEN | M_EXEC, eTop_dy);
    }
 }
}

#undef FIFO_CNT_RGB
#undef FIFO_CNT_Z

#undef TRI_SWAP

#undef SETUP_CODE
#undef INTERP_RGB
#undef INTERP_Z
#undef CULL
#undef TAG
