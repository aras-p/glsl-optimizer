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

#include <stdio.h>

#include <ggi/internal/ggi-dl.h>
#include <ggi/mesa/ggimesa_int.h>

/**********************************************************************/
/*****            Write spans of pixels                           *****/
/**********************************************************************/

void GGIwrite_ci32_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLuint ci[],
                         const GLubyte mask[] )
{
	y=FLIP(y);
	if (mask)
	{
		while (n--) {
			if (*mask++) ggiPutPixel(VIS,x,y,*ci);
			x++;
			ci++;
		}
	}
	else
	{
		while (n--) ggiPutPixel(VIS,x++,y,*ci++);
	}
}

void GGIwrite_ci8_span(const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLubyte ci[],
                         const GLubyte mask[] )
{
	y=FLIP(y);
	if (mask)
	{
		while (n--) {
			if (*mask++) ggiPutPixel(VIS,x,y,*ci);
			x++;
			ci++;
		}
	}
	else
	{
		while (n--) ggiPutPixel(VIS,x++,y,*ci++);
	}
}

void GGIwrite_mono_span( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         const GLubyte mask[] )
{
	y=FLIP(y);
	if (mask)
	{
		while (n--) {
			if (*mask++) ggiDrawPixel(VIS,x,y);
			x++;
		}
	}
	else
	{
		ggiDrawHLine(VIS,x,y,n);
	}
}

void GGIwrite_rgba_span( const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][4],
                          const GLubyte mask[])
{
	ggi_color rgb;
	ggi_pixel col;	
	y=FLIP(y);

	if (mask)
	{
		while (n--) {
			if (*mask++) 
			{
				rgb.r=(uint16)(rgba[0][RCOMP]) << SHIFT;
				rgb.g=(uint16)(rgba[0][GCOMP]) << SHIFT;
				rgb.b=(uint16)(rgba[0][BCOMP]) << SHIFT;
				col=ggiMapColor(VIS,&rgb);
				ggiPutPixel(VIS,x,y,col);
			}
			x++;
			rgba++;
		}
	}
	else
	{
		while (n--)
		{
			rgb.r=(uint16)(rgba[0][RCOMP]) << SHIFT;
			rgb.g=(uint16)(rgba[0][GCOMP]) << SHIFT;
			rgb.b=(uint16)(rgba[0][BCOMP]) << SHIFT;
			col=ggiMapColor(VIS,&rgb);
			ggiPutPixel(VIS,x++,y,col);
			rgba++;
		}
	}
}
void GGIwrite_rgb_span( const GLcontext *ctx,
                          GLuint n, GLint x, GLint y,
                          const GLubyte rgba[][3],
                          const GLubyte mask[] )
{
	ggi_color rgb;
	ggi_pixel col;	
	y=FLIP(y);

	if (mask)
	{
		while (n--) {
			if (*mask++) 
			{
				rgb.r=(uint16)(rgba[0][RCOMP]) << SHIFT;
				rgb.g=(uint16)(rgba[0][GCOMP]) << SHIFT;
				rgb.b=(uint16)(rgba[0][BCOMP]) << SHIFT;
				col=ggiMapColor(VIS,&rgb);
				ggiPutPixel(VIS,x,y,col);
			}
			x++;
			rgba++;
		}
	}
	else
	{
		while (n--)
		{
			rgb.r=(uint16)(rgba[0][RCOMP]) << SHIFT;
			rgb.g=(uint16)(rgba[0][GCOMP]) << SHIFT;
			rgb.b=(uint16)(rgba[0][BCOMP]) << SHIFT;
			col=ggiMapColor(VIS,&rgb);
			ggiPutPixel(VIS,x++,y,col);
			rgba++;
		}
	}
}



/**********************************************************************/
/*****                 Read spans of pixels                       *****/
/**********************************************************************/


void GGIread_ci32_span( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y, GLuint ci[])
{
	y = FLIP(y);
	while (n--)
		ggiGetPixel(VIS, x++, y,ci++);
}

void GGIread_rgba_span( const GLcontext *ctx,
                         GLuint n, GLint x, GLint y,
                         GLubyte rgba[][4])
{
	ggi_color rgb;
	ggi_pixel col;

	y=FLIP(y);

	while (n--)
	{
		ggiGetPixel(VIS,x++,y,&col);
		ggiUnmapPixel(VIS,col,&rgb);
		rgba[0][RCOMP] = (GLubyte) (rgb.r >> SHIFT);
		rgba[0][GCOMP] = (GLubyte) (rgb.g >> SHIFT);
		rgba[0][BCOMP] = (GLubyte) (rgb.b >> SHIFT);
		rgba[0][ACOMP] = 0;
		rgba++;
	}
}

/**********************************************************************/
/*****                  Write arrays of pixels                    *****/
/**********************************************************************/

void GGIwrite_ci32_pixels( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLuint ci[], const GLubyte mask[] )
{
	while (n--) {
		if (*mask++) ggiPutPixel(VIS,*x, FLIP(*y),*ci);
		ci++;
		x++;
		y++;
	}
}

void GGIwrite_mono_pixels( const GLcontext *ctx,
                                GLuint n,
                                const GLint x[], const GLint y[],
                                const GLubyte mask[] )
{
	while (n--) {
		if (*mask++) ggiDrawPixel(VIS,*x,FLIP(*y));
		x++;
		y++;
	}
}

void GGIwrite_rgba_pixels( const GLcontext *ctx,
                            GLuint n, const GLint x[], const GLint y[],
                            const GLubyte rgba[][4],
                            const GLubyte mask[] )
{
	ggi_pixel col;
	ggi_color rgb;
	while (n--) {
		if (*mask++) {
			rgb.r=(uint16)(rgba[0][RCOMP]) << SHIFT;
			rgb.g=(uint16)(rgba[0][GCOMP]) << SHIFT;
			rgb.b=(uint16)(rgba[0][BCOMP]) << SHIFT;
			col=ggiMapColor(VIS,&rgb);
			ggiPutPixel(VIS,*x,FLIP(*y),col);
		}
		x++;y++;
		rgba++;
	}
}


/**********************************************************************/
/*****                   Read arrays of pixels                    *****/
/**********************************************************************/

void GGIread_ci32_pixels( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLuint ci[], const GLubyte mask[])
{
	while (n--) {
		if (*mask++) 
			ggiGetPixel(VIS, *x, FLIP(*y) ,ci);
		ci++;
		x++;
		y++;
	}
}

void GGIread_rgba_pixels( const GLcontext *ctx,
                           GLuint n, const GLint x[], const GLint y[],
                           GLubyte rgba[][4],
                           const GLubyte mask[] )
{
	ggi_color rgb;
	ggi_pixel col;

	while (n--)
	{
		if (*mask++)
		{
			ggiGetPixel(VIS,*x,FLIP(*y),&col);
			ggiUnmapPixel(VIS,col,&rgb);
			rgba[0][RCOMP]= rgb.r >> SHIFT; 
			rgba[0][GCOMP]= rgb.g >> SHIFT;
			rgba[0][BCOMP]= rgb.b >> SHIFT;
			rgba[0][ACOMP]=0;
		}	
		x++; y++;
		rgba++;
	}
}


triangle_func ggiGetTriangleFunc(GLcontext *ctx);

int GGIsetup_driver(GGIMesaContext ggictx,struct ggi_mesa_info *info)
{
	GLcontext *ctx=ggictx->gl_ctx;

	ctx->Driver.WriteRGBASpan	= GGIwrite_rgba_span;
	ctx->Driver.WriteRGBSpan	= GGIwrite_rgb_span;
	ctx->Driver.WriteMonoRGBASpan   = GGIwrite_mono_span;
	ctx->Driver.WriteRGBAPixels     = GGIwrite_rgba_pixels;
	ctx->Driver.WriteMonoRGBAPixels = GGIwrite_mono_pixels;

	ctx->Driver.WriteCI32Span       = GGIwrite_ci32_span;
	ctx->Driver.WriteCI8Span       	= GGIwrite_ci8_span;
	ctx->Driver.WriteMonoCISpan   	= GGIwrite_mono_span;
	ctx->Driver.WriteCI32Pixels     = GGIwrite_ci32_pixels;
	ctx->Driver.WriteMonoCIPixels 	= GGIwrite_mono_pixels;

	ctx->Driver.ReadCI32Span 	= GGIread_ci32_span;
	ctx->Driver.ReadRGBASpan	= GGIread_rgba_span;
	ctx->Driver.ReadCI32Pixels	= GGIread_ci32_pixels;
	ctx->Driver.ReadRGBAPixels	= GGIread_rgba_pixels;

	return 0;
}

void GGIupdate_state(GLcontext *ctx)
{
	ctx->Driver.TriangleFunc = ggiGetTriangleFunc(ctx);
}


void GGItriangle_flat(GLcontext *ctx,GLuint v0,GLuint v1,GLuint v2,GLuint pv)
{
//#define INTERP_Z 1
//#define INTERP_RGB 1
//#define INTERP_ALPHA 1
	
#define SETUP_CODE			\
	GLubyte r = VB->ColorPtr->data[pv][0];	\
	GLubyte g = VB->ColorPtr->data[pv][1];	\
	GLubyte b = VB->ColorPtr->data[pv][2];	\
	GLubyte a = VB->ColorPtr->data[pv][3];	\
	(*ctx->Driver.Color)(ctx,r,g,b,a);

#define INNER_LOOP(LEFT,RIGHT,Y)				\
		ggiDrawHLine(VIS,LEFT,FLIP(Y),RIGHT-LEFT);	
		
#include "tritemp.h"
}


void GGItriangle_flat_depth(GLcontext *ctx, GLuint v0, GLuint v1, GLuint v2, GLuint pv)
{
#define INTERP_Z 1
#define DEPTH_TYPE DEFAULT_SOFTWARE_DEPTH_TYPE

#define SETUP_CODE			\
	GLubyte r = VB->ColorPtr->data[pv][0];	\
	GLubyte g = VB->ColorPtr->data[pv][1];	\
	GLubyte b = VB->ColorPtr->data[pv][2];	\
	GLubyte a = VB->ColorPtr->data[pv][3];	\
	(*ctx->Driver.Color)(ctx,r,g,b,a);
	
#define INNER_LOOP(LEFT,RIGHT,Y)					\
	{								\
	GLint i,xx=LEFT,yy=FLIP(Y),n=RIGHT-LEFT,length=0;		\
	GLint startx=xx;						\
	for (i=0;i<n;i++){						\
		GLdepth z=FixedToDepth(ffz);				\
		if (z<zRow[i])						\
		{							\
			zRow[i]=z;					\
			length++;					\
		}							\
		else							\
		{							\
			if (length)					\
			{ 						\
				ggiDrawHLine(VIS,startx,yy,length);	\
				length=0;				\
			}						\
			startx=xx+i+1;					\
		}							\
		ffz+=fdzdx;						\
	}								\
	if (length) ggiDrawHLine(VIS,startx,yy,length);			\
	}

#include "tritemp.h"	
}


triangle_func ggiGetTriangleFunc(GLcontext *ctx)
{
	if (ctx->Stencil.Enabled) return NULL;
	if (ctx->Polygon.SmoothFlag) return NULL;
	if (ctx->Polygon.StippleFlag) return NULL;
	if (ctx->Texture.ReallyEnabled) return NULL;  
	if (ctx->Light.ShadeModel==GL_SMOOTH) return NULL;
	if (ctx->Depth.Test && ctx->Depth.Func != GL_LESS) return NULL;

	if (ctx->Depth.Test) 
	  return GGItriangle_flat_depth;

	return GGItriangle_flat;	
}

static int GGIopen(ggi_visual_t vis, struct ggi_dlhandle *dlh,
			const char *args, void *argptr, uint32 *dlret)
{ 
       	LIBGGI_MESAEXT(vis)->update_state = GGIupdate_state;
	LIBGGI_MESAEXT(vis)->setup_driver = GGIsetup_driver;

	*dlret = GGI_DL_OPDRAW;
	return 0;
}

int MesaGGIdl_stubs(int func, void **funcptr)
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
