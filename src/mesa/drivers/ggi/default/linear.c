/* GGI-Driver for MESA
 * 
 * Copyright (C) 1997  Uwe Maurer
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
 * ---------------------------------------------------------------------
 * This code was derived from the following source of information:
 *
 * svgamesa.c and ddsample.c by Brian Paul
 * 
 */

#include <ggi/mesa/ggimesa.h>
#include <ggi/mesa/ggimesa_int.h>

#define RMASK ((1<<R)-1)
#define GMASK ((1<<G)-1)
#define BMASK ((1<<B)-1)

#define RS (8-R)
#define GS (8-G)
#define BS (8-B)


/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/

void GGIwrite_ci32_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLuint ci[],
                         const GLubyte mask[])
{
	FB_TYPE *fb=LFB(FB_TYPE,x,FLIP(y));
	if (mask)
	{
		while (n--) {
			if (*mask++) *fb=*ci;
			fb++;
			ci++;
		}
	}
	else
	{
		while (n--) *fb++ = *ci++;
	}
}

void GGIwrite_ci8_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLubyte ci[],
                         const GLubyte mask[] )
{
	FB_TYPE *fb=LFB(FB_TYPE,x,FLIP(y));

	if (mask)
	{
		while (n--) {
			if (*mask++) *fb=*ci;
			fb++;
			ci++;
		}	
	}
	else
	{
		while (n--) *fb++ = *ci++;
	}
}


void GGIwrite_rgba_span(const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4],
                          const GLubyte mask[])
{
	FB_TYPE *fb=LFB(FB_TYPE,x,FLIP(y));

	if (mask)
	{
		while (n--) {
			if (*mask++) {
				*fb=	((rgba[0][RCOMP]>>RS) << (G+B)) | 
					((rgba[0][GCOMP]>>GS) << B) |
					((rgba[0][BCOMP]>>BS));
			}
			fb++;
			rgba++;
		}
	}
	else
	{
		while (n--) {
			*fb++=	((rgba[0][RCOMP]>>RS) << (G+B)) | 
				((rgba[0][GCOMP]>>GS) << B)|
				((rgba[0][BCOMP]>>BS));
			rgba++;
		}
	}
}

void GGIwrite_rgb_span( const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][3],
                          const GLubyte mask[] )
{
	FB_TYPE *fb=LFB(FB_TYPE,x,FLIP(y));
	if (mask)
	{
		while (n--) {
			if (*mask++) {
				*fb=	((rgba[0][RCOMP]>>RS) << (G+B)) | 
					((rgba[0][GCOMP]>>GS) << B) |
					((rgba[0][BCOMP]>>BS));
			}
			fb++;
			rgba++;
		}
	}
	else
	{
		while (n--) {
			*fb++=	((rgba[0][RCOMP]>>RS) << (G+B)) | 
				((rgba[0][GCOMP]>>GS) << B) |
				((rgba[0][BCOMP]>>BS));
			rgba++;
		}
	}
}


void GGIwrite_mono_span( const GLcontext *ctx,
                              GLuint n, GLint x, GLint y,
                              const GLubyte mask[])
{
	FB_TYPE *fb;
	FB_TYPE color;

	if (mask)
	{
		fb=LFB(FB_TYPE,x,FLIP(y));
		color=(FB_TYPE) GGICTX->color;

		while (n--) 
		{
			if (*mask++) *fb=color; 
			fb++;
		}
	}
	else
	{
		ggiDrawHLine(VIS,x,FLIP(y),n);
	}
}


/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/


void GGIread_ci32_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint ci[])
{
	FB_TYPE *fb=LFB(FB_TYPE,x,FLIP(y));
	while (n--)
		*ci++=(GLuint)*fb++;
}

void GGIread_rgba_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4])
{
	FB_TYPE *fb=LFB(FB_TYPE,x,FLIP(y));
	FB_TYPE color;

	while (n--)
	{
		color=*fb++;
		rgba[0][RCOMP] = (GLubyte) (color>>(G+B))<<RS;  
		rgba[0][GCOMP] = (GLubyte) ((color>>B)& ((1<<G)-1))<<GS;  
		rgba[0][BCOMP] = (GLubyte) (color & ((1<<B)-1))<<BS;  
		rgba[0][ACOMP] =0;
		rgba++;
	}
}

/**********************************************************************/
/*****                  Write arrays of pixels                    *****/
/**********************************************************************/

void GGIwrite_ci32_pixels(const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLuint ci[], const GLubyte mask[] )
{
	FB_TYPE *fb=LFB(FB_TYPE,0,0);

	while (n--) {
		if (*mask++) *(fb+ *x + FLIP(*y)*GGICTX->width)=*ci;
		ci++;
		x++;
		y++;
	}
}

void GGIwrite_mono_pixels(const GLcontext *ctx,
                                GLuint n,
                                const GLint x[], const GLint y[],
                                const GLubyte mask[] )
{
	FB_TYPE *fb=LFB(FB_TYPE,0,0);
	FB_TYPE color=(FB_TYPE) GGICTX->color;

	while (n--) {
		if (*mask++) *(fb+ *x + FLIP(*y)*GGICTX->width)=color;
		x++;
		y++;
	}
}

void GGIwrite_rgba_pixels(const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4],
                            const GLubyte mask[] )
{
	FB_TYPE *fb=LFB(FB_TYPE,0,0);
	FB_TYPE color;

	while (n--) {
		if (*mask++) {
			color=	((rgba[0][RCOMP]>>RS) << (G+B)) | 
				((rgba[0][GCOMP]>>GS) << B) |
				((rgba[0][BCOMP]>>BS));
			 *(fb+ *x + FLIP(*y)*GGICTX->width)=color;
		}
		x++;y++;
		rgba++;
	}
}


/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/

void GGIread_ci32_pixels(const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint ci[], const GLubyte mask[])
{
	FB_TYPE *fb=LFB(FB_TYPE,0,0);

	while (n--) {
		if (*mask++) 
			*ci=*(fb+ *x + FLIP(*y)*GGICTX->width);
		ci++;
		x++;
		y++;
	}
}

void GGIread_rgba_pixels(const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4],
                           const GLubyte mask[] )
{
	FB_TYPE *fb=LFB(FB_TYPE,0,0);
	FB_TYPE color;

	while (n--)
	{
		if (*mask++)
		{
			color=*(fb+ *x + FLIP(*y)*GGICTX->width);
			rgba[0][RCOMP] =(GLubyte)(color>>(G+B))<<RS;  
			rgba[0][GCOMP] =(GLubyte)((color>>B)& ((1<<G)-1))<<GS;  
			rgba[0][BCOMP] =(GLubyte) (color & ((1<<B)-1))<<BS;  
			rgba[0][ACOMP] =0;
		}	
		x++; y++;
		rgba++;
	}
}

int GGIsetup_driver(GGIMesaContext ggictx,struct ggi_mesa_info *info)
{
	GLcontext *ctx=ggictx->gl_ctx;

	ctx->Driver.WriteRGBASpan	= GGIwrite_rgba_span;
	ctx->Driver.WriteRGBSpan	= GGIwrite_rgb_span;
	ctx->Driver.WriteMonoRGBASpan	= GGIwrite_mono_span;
	ctx->Driver.WriteRGBAPixels	= GGIwrite_rgba_pixels;
	ctx->Driver.WriteMonoRGBAPixels = GGIwrite_mono_pixels;

	ctx->Driver.WriteCI32Span       = GGIwrite_ci32_span;
	ctx->Driver.WriteCI8Span       = GGIwrite_ci8_span;
	ctx->Driver.WriteMonoCISpan   = GGIwrite_mono_span;
	ctx->Driver.WriteCI32Pixels     = GGIwrite_ci32_pixels;
	ctx->Driver.WriteMonoCIPixels = GGIwrite_mono_pixels;

	ctx->Driver.ReadCI32Span = GGIread_ci32_span;
	ctx->Driver.ReadRGBASpan = GGIread_rgba_span;
	ctx->Driver.ReadCI32Pixels = GGIread_ci32_pixels;
	ctx->Driver.ReadRGBAPixels = GGIread_rgba_pixels;

	info->red_bits=R;
	info->green_bits=G;
	info->blue_bits=B;

	return 0;
}

static int GGIopen(ggi_visual_t vis,struct ggi_dlhandle *dlh,
			const char *args,void *argptr, uint32 *dlret)
{	
	LIBGGI_MESAEXT(vis)->setup_driver=GGIsetup_driver;

	*dlret = GGI_DL_OPDRAW;
	return 0;
}

int DLOPENFUNC(int func, void **funcptr)
{
	switch (func) {
		case GGIFUNC_open:
			*funcptr = GGIopen;
			return 0;
		case GGIFUNC_exit:
		case GGIFUNC_close:
			*funcptr = NULL;
			return 0;
		default:
			*funcptr = NULL;
	}
	return GGI_ENOTFOUND;
}

