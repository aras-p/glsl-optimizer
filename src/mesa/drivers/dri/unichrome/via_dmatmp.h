/*
 * Copyright 1998-2003 VIA Technologies, Inc. All Rights Reserved.
 * Copyright 2001-2003 S3 Graphics, Inc. All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * VIA, S3 GRAPHICS, AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

/* Template for render stages which build and emit vertices directly
 * to fixed-size dma buffers.  Useful for rendering strips and other
 * native primitives where clipping and per-vertex tweaks such as
 * those in t_dd_tritmp.h are not required.
 *
 * Produces code for both inline triangles and indexed triangles.
 * Where various primitive types are unaccelerated by hardware, the
 * code attempts to fallback to other primitive types (quadstrips to
 * tristrips, lineloops to linestrips), or to indexed vertices.
 * Ultimately, a FALLBACK() macro is invoked if there is no way to
 * render the primitive natively.
 */

#if !defined(HAVE_TRIANGLES)
#error "must have at least triangles to use render template"
#endif

#if !HAVE_ELTS
#define ELTS_VARS
#define ALLOC_ELTS(nr)
#define EMIT_ELT(offset, elt)
#define EMIT_TWO_ELTS(offset, elt0, elt1)
#define INCR_ELTS(nr)
#define ELT_INIT(prim)
#define GET_CURRENT_VB_MAX_ELTS() 0
#define GET_SUBSEQUENT_VB_MAX_ELTS() 0
#define ALLOC_ELTS_NEW_PRIMITIVE(nr)
#define RELEASE_ELT_VERTS()
#define EMIT_INDEXED_VERTS(ctx, start, count)
#endif

#ifndef EMIT_TWO_ELTS
#define EMIT_TWO_ELTS(offset, elt0, elt1)           \
    do {                                            \
        EMIT_ELT(offset, elt0);                     \
        EMIT_ELT(offset + 1, elt1);                 \
    } while (0)
#endif

#ifndef FINISH
#define FINISH
#endif

/***********************************************************************
 *                    Render non-indexed primitives.
 ***********************************************************************/

static void TAG(render_points_verts)(GLcontext *ctx,
                                     GLuint start,
                                     GLuint count,
                                     GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_POINTS) {
        LOCAL_VARS;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();
        GLuint j, nr;

        INIT(GL_POINTS);

        if (currentsz < 8)
            currentsz = dmasz;

        for (j = start; j < count; j += nr) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        FINISH;
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

static void TAG(render_lines_verts)(GLcontext *ctx,
                                    GLuint start,
                                    GLuint count,
                                    GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_LINES) {
        LOCAL_VARS;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();
        GLuint j, nr;

        INIT(GL_LINES);

        /* Emit whole number of lines in total and in each buffer:
         */
        count -= (count - start) & 1;
        currentsz -= currentsz & 1;
        dmasz -= dmasz & 1;

        if (currentsz < 8)
            currentsz = dmasz;

        for (j = start; j < count; j += nr) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        FINISH;
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}


static void TAG(render_line_strip_verts)(GLcontext *ctx,
                                         GLuint start,
                                         GLuint count,
                                         GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_LINE_STRIPS) {
        LOCAL_VARS;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();
        GLuint j, nr;

        INIT(GL_LINE_STRIP);

        if (currentsz < 8)
            currentsz = dmasz;

        for (j = start; j + 1 < count; j += nr - 1) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        FINISH;
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}


static void TAG(render_line_loop_verts)(GLcontext *ctx,
                                        GLuint start,
                                        GLuint count,
                                        GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_LINE_STRIPS) {
        LOCAL_VARS;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();
        GLuint j, nr;

        INIT(GL_LINE_STRIP);

        if (flags & PRIM_BEGIN)
            j = start;
        else
            j = start + 1;

        /* Ensure last vertex won't wrap buffers:
         */
        currentsz--;
        dmasz--;

        if (currentsz < 8)
            currentsz = dmasz;

        for (; j + 1 < count; j += nr - 1) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        if (start < count - 1 && (flags & PRIM_END))
            EMIT_VERTS(ctx, start, 1);

        FINISH;
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}


static void TAG(render_triangles_verts)(GLcontext *ctx,
                                        GLuint start,
                                        GLuint count,
                                        GLuint flags)
{
    LOCAL_VARS;
    int dmasz = (GET_SUBSEQUENT_VB_MAX_VERTS() / 3) * 3;
    int currentsz = (GET_CURRENT_VB_MAX_VERTS() / 3) * 3;
    GLuint j, nr;
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    INIT(GL_TRIANGLES);

    /* Emit whole number of tris in total.  dmasz is already a multiple
     * of 3.
     */
    count -= (count - start) % 3;

    if (currentsz < 8)
        currentsz = dmasz;

     for (j = start; j < count; j += nr) {
         nr = MIN2(currentsz, count - j);
         EMIT_VERTS(ctx, j, nr);
         currentsz = dmasz;
     }
     FINISH;
     if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


static void TAG(render_tri_strip_verts)(GLcontext *ctx,
                                        GLuint start,
                                        GLuint count,
                                        GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_TRI_STRIPS) {
        LOCAL_VARS;
        GLuint j, nr;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();

        INIT(GL_TRIANGLE_STRIP);

        if (currentsz < 8) {
            NEW_BUFFER();
            currentsz = dmasz;
        }

        /* From here on emit even numbers of tris when wrapping over buffers:
         */
        dmasz -= (dmasz & 1);
        currentsz -= (currentsz & 1);

        for (j = start; j + 2 < count; j += nr - 2) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        FINISH;
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

static void TAG(render_tri_fan_verts)(GLcontext *ctx,
                                      GLuint start,
                                      GLuint count,
                                      GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (HAVE_TRI_FANS) {
	LOCAL_VARS;
        GLuint j, nr;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();

        INIT(GL_TRIANGLE_FAN);

        if (currentsz < 8) {
            NEW_BUFFER();
            currentsz = dmasz;
        }

        for (j = start + 1; j + 1 < count; j += nr - 1) {
            nr = MIN2(currentsz, count - j + 1);
            EMIT_VERTS(ctx, start, 1);
            EMIT_VERTS(ctx, j, nr - 1);
            currentsz = dmasz;
        }

        FINISH;
    }
    else {
        /* Could write code to emit these as indexed vertices (for the
         * g400, for instance).
         */
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}


static void TAG(render_poly_verts)(GLcontext *ctx,
                                   GLuint start,
                                   GLuint count,
                                   GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_POLYGONS) {
        LOCAL_VARS;
        GLuint j, nr;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();

        INIT(GL_POLYGON);

        if (currentsz < 8) {
            NEW_BUFFER();
            currentsz = dmasz;
        }

        for (j = start + 1; j + 1 < count; j += nr - 1) {
            nr = MIN2(currentsz, count - j + 1);
            EMIT_VERTS(ctx, start, 1);
            EMIT_VERTS(ctx, j, nr - 1);
            currentsz = dmasz;
        }

        FINISH;
    }
    else if (HAVE_TRI_FANS && !(ctx->_TriangleCaps & DD_FLATSHADE)) {
        TAG(render_tri_fan_verts)(ctx, start, count, flags);
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}

static void TAG(render_quad_strip_verts)(GLcontext *ctx,
                                         GLuint start,
                                         GLuint count,
                                         GLuint flags)
{
    GLuint j, nr;
    if (VIA_DEBUG) fprintf(stderr, "%s in\n", __FUNCTION__);
    if (HAVE_QUAD_STRIPS) {
        LOCAL_VARS;
        GLuint j, nr;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz;

        INIT(GL_QUAD_STRIP);

        currentsz = GET_CURRENT_VB_MAX_VERTS();

        if (currentsz < 8) {
            NEW_BUFFER();
            currentsz = dmasz;
        }

        dmasz -= (dmasz & 2);
        currentsz -= (currentsz & 2);

        for (j = start; j + 3 < count; j += nr - 2) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        FINISH;
    }
    else if (HAVE_TRI_STRIPS) {
        LOCAL_VARS;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz = GET_CURRENT_VB_MAX_VERTS();

        /* Emit smooth-shaded quadstrips as tristrips:
         */
        INIT(GL_TRIANGLE_STRIP);

        /* Emit whole number of quads in total, and in each buffer.
         */
        dmasz -= dmasz & 1;
        currentsz -= currentsz & 1;
        count -= (count - start) & 1;

        if (currentsz < 8) {
            NEW_BUFFER();
            currentsz = dmasz;
        }

        for (j = start; j + 3 < count; j += nr - 2) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }

        FINISH;
    }
    else {
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s out\n", __FUNCTION__);
}


static void TAG(render_quads_verts)(GLcontext *ctx,
                                    GLuint start,
                                    GLuint count,
                                    GLuint flags)
{
    if (VIA_DEBUG) fprintf(stderr, "%s - in\n", __FUNCTION__);    
    if (HAVE_QUADS) {
        LOCAL_VARS;
        int dmasz = (GET_SUBSEQUENT_VB_MAX_VERTS() / 4) * 4;
        int currentsz = (GET_CURRENT_VB_MAX_VERTS() / 4) * 4;
        GLuint j, nr;

        INIT(GL_QUADS);

        /* Emit whole number of quads in total.  dmasz is already a multiple
         * of 4.
         */
        count -= (count - start) % 4;

        if (currentsz < 8)
            currentsz = dmasz;

        for (j = start; j < count; j += nr) {
            nr = MIN2(currentsz, count - j);
            EMIT_VERTS(ctx, j, nr);
            currentsz = dmasz;
        }
        FINISH;
    }
    else if (HAVE_TRIANGLES) {
        /* Hardware doesn't have a quad primitive type -- try to
         * simulate it using triangle primitive.
         */
        LOCAL_VARS;
        int dmasz = GET_SUBSEQUENT_VB_MAX_VERTS();
        int currentsz;
        GLuint j;

        INIT(GL_TRIANGLES);

        currentsz = GET_CURRENT_VB_MAX_VERTS();

        /* Emit whole number of quads in total, and in each buffer.
         */
        dmasz -= dmasz & 3;
        count -= (count - start) & 3;
        currentsz -= currentsz & 3;

        /* Adjust for rendering as triangles:
         */
        currentsz = currentsz / 6 * 4;
        dmasz = dmasz / 6 * 4;

        if (currentsz < 8)
            currentsz = dmasz;

        for (j = start; j < count; j += 4) {
            /* Send v0, v1, v3
             */
            EMIT_VERTS(ctx, j, 2);
            EMIT_VERTS(ctx, j + 3, 1);
            /* Send v1, v2, v3
             */
            EMIT_VERTS(ctx, j + 1, 3);
        }
        FINISH;
    }
    else {
        /* Vertices won't fit in a single buffer, fallback.
         */
        VERT_FALLBACK(ctx, start, count, flags);
    }
    if (VIA_DEBUG) fprintf(stderr, "%s - out\n", __FUNCTION__);    
}

static void TAG(render_noop)(GLcontext *ctx,
                             GLuint start,
                             GLuint count,
                             GLuint flags)
{
}


static tnl_render_func TAG(render_tab_verts)[GL_POLYGON + 2] =
{
    TAG(render_points_verts),
    TAG(render_lines_verts),
    TAG(render_line_loop_verts),
    TAG(render_line_strip_verts),
    TAG(render_triangles_verts),
    TAG(render_tri_strip_verts),
    TAG(render_tri_fan_verts),
    TAG(render_quads_verts),
    TAG(render_quad_strip_verts),
    TAG(render_poly_verts),
    TAG(render_noop),
};

