/*
 * Mesa 3-D graphics library
 * Version:  3.0
 * Copyright (C) 1995-1998  Brian Paul
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the Free
 * Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */

static void clear_color_generic(GLcontext *ctx, const GLfloat color[4])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    GLubyte r, g, b;
    CLAMPED_FLOAT_TO_UBYTE(r, color[0]);
    CLAMPED_FLOAT_TO_UBYTE(g, color[1]);
    CLAMPED_FLOAT_TO_UBYTE(b, color[2]);
    context->ClearColor = makecol(r, g, b);
    }


static void set_color_generic(GLcontext *ctx,
                              GLubyte red,  GLubyte green,
                              GLubyte blue, GLubyte alpha)
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);

    context->CurrentColor = makecol(red, green, blue);
    }


static GLbitfield clear_generic(GLcontext *ctx,
                                GLbitfield mask, GLboolean all,
                                GLint x, GLint y,
                                GLint width, GLint height)
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);

    if (mask & GL_COLOR_BUFFER_BIT)
        {
        if (all)
            clear_to_color(context->Buffer->Active, context->ClearColor);
        else
            rect(context->Buffer->Active,
                 x, y, x+width-1, y+height-1,
                 context->ClearColor);
        }

    return mask & (~GL_COLOR_BUFFER_BIT);
    }


static void write_rgba_span_generic(const GLcontext *ctx,
                                    GLuint n, GLint x, GLint y,
                                    const GLubyte rgba[][4],
                                    const GLubyte mask[])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;

    y = FLIP(context, y);

    if (mask)
        {
        while (n--)
            {
            if (mask[0]) putpixel(bmp, x, y, makecol(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]));
            x++; mask++; rgba++;
            }
        }
    else
        {
        while (n--)
            {
            putpixel(bmp, x, y, makecol(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]));
            x++; rgba++;
            }
        }
    }


static void write_rgb_span_generic(const GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   const GLubyte rgb[][3],
                                   const GLubyte mask[])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;

    y = FLIP(context, y);

    if (mask)
        {
        while(n--)
            {
            if (mask[0]) putpixel(bmp, x, y, makecol(rgb[0][RCOMP], rgb[0][GCOMP], rgb[0][BCOMP]));
            x++; mask++; rgb++;
            }
        }
    else
        {
        while (n--)
            {
            putpixel(bmp, x, y, makecol(rgb[0][RCOMP], rgb[0][GCOMP], rgb[0][BCOMP]));
            x++; rgb++;
            }
        }
    }


static void write_mono_rgba_span_generic(const GLcontext *ctx,
                                         GLuint n, GLint x, GLint y,
                                         const GLubyte mask[])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;
    int            color = context->CurrentColor;

    y = FLIP(context, y);

    if (mask)
        {
        while(n--)
            {
            if (mask[0]) putpixel(bmp, x, y, color);
            x++; mask++;
            }
        }
    else
        {
        while(n--)
            {
            putpixel(bmp, x, y, color);
            x++;
            }
        }
    }


static void read_rgba_span_generic(const GLcontext *ctx,
                                   GLuint n, GLint x, GLint y,
                                   GLubyte rgba[][4])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;

    y = FLIP(context, y);

    while (n--)
        {
        int color = getpixel(bmp, x, y);

        rgba[0][RCOMP] = getr(color);
        rgba[0][GCOMP] = getg(color);
        rgba[0][BCOMP] = getb(color);
        rgba[0][ACOMP] = 255;

        x++; rgba++;
        }
    }


static void write_rgba_pixels_generic(const GLcontext *ctx,
                                      GLuint n,
                                      const GLint x[],
                                      const GLint y[],
                                      const GLubyte rgba[][4],
                                      const GLubyte mask[])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;

    while (n--)
        {
        if (mask[0]) putpixel(bmp, x[0], FLIP(context, y[0]), makecol(rgba[0][RCOMP], rgba[0][GCOMP], rgba[0][BCOMP]));
        x++; y++; mask++;
        }
    }


static void write_mono_rgba_pixels_generic(const GLcontext *ctx,
                                           GLuint n,
                                           const GLint x[],
                                           const GLint y[],
                                           const GLubyte mask[])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;
    int            color = context->CurrentColor;

    while (n--)
        {
        if (mask[0]) putpixel(bmp, x[0], FLIP(context, y[0]), color);
        x++; y++; mask++;
        }
    }


static void read_rgba_pixels_generic(const GLcontext *ctx,
                                     GLuint n,
                                     const GLint x[],
                                     const GLint y[],
                                     GLubyte rgba[][4],
                                     const GLubyte mask[])
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    BITMAP          *bmp = context->Buffer->Active;

    while (n--)
        {
        if (mask[0])
            {
            int color = getpixel(bmp, x[0], FLIP(context, y[0]));

            rgba[0][RCOMP] = getr(color);
            rgba[0][GCOMP] = getg(color);
            rgba[0][BCOMP] = getb(color);
            rgba[0][ACOMP] = 255;
            }

        x++; y++; mask++; rgba++;
        }
    }

