/* GGI-Driver for MESA
 * 
 * Copyright (C) 1997-1998  Uwe Maurer  -  uwe_maurer@t-online.de 
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

#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#ifdef GGI

#include <ggi/mesa/ggimesa_int.h>
#include <ggi/mesa/debug.h>

#undef VIS
#undef FLIP
#define VIS (GGIMesa->ggi_vis)
#define FLIP(y) (GGIMesa->flip_y-(y))

GGIMesaContext GGIMesa = NULL;    /* the current context */

ggi_extid ggiMesaID = -1;
static int _ggimesaLibIsUp = 0;
static void *_ggimesaConfigHandle;

/* FIXME: These should really be defined in the make system using -Dxxx */
#define GGIMESACONFFILE "/usr/local/etc/ggi/ggimesa.conf"
#define GGIMESATAGLEN 0
static char ggimesaconfstub[512] = GGIMESACONFFILE;
static char *ggimesaconffile = ggimesaconfstub + GGIMESATAGLEN;

int gl_ggi_init = GL_FALSE;
int gl_ggi_debug = GL_FALSE;

static void gl_ggiUpdateState(GLcontext *ctx);
static int changed(ggi_visual_t vis, int whatchanged);

#if 0
/* FIXME: Move this debugging stuff to include/ggi/mesa/internal/ */
void gl_ggiPrint(char *format,...)
{
	va_list args;
	va_start(args,format);
	fprintf(stderr,"GGIMesa: ");
	vfprintf(stderr,format,args);
	va_end(args);
}

void gl_ggiDEBUG(char *format,...)
{
	va_list args;
	if (gl_ggi_debug)
	{
		va_start(args,format);
		fprintf(stderr,"GGIMesa: ");
		vfprintf(stderr,format,args);
		va_end(args);
	}
}
#endif

static void gl_ggiGetSize(GLcontext *ctx, GLuint *width, GLuint *height)
{
	*width = GGIMesa->width; 
	*height = GGIMesa->height; 
}

static void gl_ggiSetIndex(GLcontext *ctx, GLuint ci)
{
	ggiSetGCForeground(VIS, ci);
	GGICTX->color = (ggi_pixel)ci;
}

static void gl_ggiSetClearIndex(GLcontext *ctx, GLuint ci)
{
	ggiSetGCForeground(VIS, ci);
	GGICTX->clearcolor = (ggi_pixel)ci;
}

static void gl_ggiSetColor(GLcontext *ctx, GLubyte red, GLubyte green,
			   GLubyte blue, GLubyte alpha)
{
	ggi_color rgb;
	ggi_pixel col;
	
	rgb.r = (uint16)red << SHIFT;
	rgb.g = (uint16)green << SHIFT;
	rgb.b = (uint16)blue << SHIFT;
	col = ggiMapColor(VIS, &rgb);
	ggiSetGCForeground(VIS, col);
	GGICTX->color = col;
}

static void gl_ggiSetClearColor(GLcontext *ctx, GLubyte red, GLubyte green,
				GLubyte blue, GLubyte alpha)
{
	ggi_color rgb;
	ggi_pixel col;
	rgb.r = (uint16)red << SHIFT;
	rgb.g = (uint16)green << SHIFT;
	rgb.b = (uint16)blue << SHIFT;
	col = ggiMapColor(VIS, &rgb);
	ggiSetGCForeground(VIS, col);
	GGICTX->clearcolor = col;
}

static GLbitfield gl_ggiClear(GLcontext *ctx,GLbitfield mask, GLboolean all,
			      GLint x, GLint y, GLint width, GLint height)
{

	if (mask & GL_COLOR_BUFFER_BIT) 
	{
		ggiSetGCForeground(VIS, GGICTX->clearcolor);

		if (all)
		{
			ggiDrawBox(VIS, 0, GGIMesa->origin.y,
				   GGIMesa->width, GGIMesa->height);
		}
		else
		{
			ggiDrawBox(VIS, x, FLIP(y), width, height);
		}

		ggiSetGCForeground(VIS, GGICTX->color);
	}
	return mask & (~GL_COLOR_BUFFER_BIT);
}

#if 0
static GLboolean gl_ggiSetBuffer(GLcontext *ctx, GLenum mode)
{
	if (mode == GL_FRONT)
	  GGICTX->active_buffer = 1;
	else
	  GGICTX->active_buffer = 0;
	
	return GL_TRUE;
}
#endif

/* Set the buffer used for drawing */
static GLboolean gl_ggiSetDrawBuffer(GLcontext *ctx, GLenum mode)
{
	if (mode == GL_FRONT_LEFT) 
	{
		GGICTX->active_buffer = 1;
		return GL_TRUE;
	}
	else if (mode == GL_BACK_LEFT) 
	{
		GGICTX->active_buffer = 0;
		return GL_TRUE;
	}
	else 
	{
		return GL_FALSE;
	}
}


/* Set the buffer used for reading */
/* XXX support for separate read/draw buffers hasn't been tested */
static void gl_ggiSetReadBuffer(GLcontext *ctx, GLframebuffer *buffer, GLenum mode)
{
	if (mode == GL_FRONT_LEFT) 
	{
		GGICTX->active_buffer = 1;
	}
	else if (mode == GL_BACK_LEFT) 
	{
		GGICTX->active_buffer = 0;
	}
}


static const GLubyte * gl_ggiGetString(GLcontext *ctx, GLenum name)
{
	if (name == GL_RENDERER)
        	return (GLubyte *) "Mesa GGI";
	else
		return NULL;
}

static void gl_ggiFlush(GLcontext *ctx)
{
	ggiFlush(VIS);
}

static void gl_ggiSetupPointers( GLcontext *ctx )
{
	/* Initialize all the pointers in the DD struct.  Do this whenever */
	/* a new context is made current or we change buffers via set_buffer! */

	ctx->Driver.GetString = gl_ggiGetString;
	ctx->Driver.UpdateState = gl_ggiUpdateState;

	ctx->Driver.ClearIndex = gl_ggiSetClearIndex; 
	ctx->Driver.ClearColor = gl_ggiSetClearColor;
	ctx->Driver.Clear = gl_ggiClear;
	ctx->Driver.Index = gl_ggiSetIndex;
	ctx->Driver.Color = gl_ggiSetColor;
	
	ctx->Driver.SetDrawBuffer = gl_ggiSetDrawBuffer;
	ctx->Driver.SetReadBuffer = gl_ggiSetReadBuffer;
	
	ctx->Driver.GetBufferSize = gl_ggiGetSize;
	ctx->Driver.Finish = gl_ggiFlush;
	ctx->Driver.Flush = gl_ggiFlush;

}

static int gl_ggiInitInfo(GGIMesaContext ctx, struct ggi_mesa_info *info)
{
	ggi_mode mode;

	ggiGetMode(ctx->ggi_vis, &mode);
	
	info->depth_bits = DEFAULT_SOFTWARE_DEPTH_BITS;
	info->stencil_bits = STENCIL_BITS;
	info->accum_bits = ACCUM_BITS;

	if (info->rgb_flag)
	{		
		info->rgb_flag = GL_TRUE;
		info->alpha_flag = GL_FALSE;
		info->index_bits = 0;

		info->red_bits = 8;
		info->green_bits = 8;
		info->blue_bits = 8;

		info->alpha_bits = 0;
	}
	else
	{
		info->alpha_flag = GL_FALSE;

		info->index_bits = GT_SIZE(mode.graphtype);
		info->red_bits = info->green_bits = 
		  info->blue_bits = info->alpha_bits = 0;
	}

	return 0;
}

int ggiMesaInit()
{
	int err;
	
	_ggimesaLibIsUp++;
	if (_ggimesaLibIsUp > 1)
	  return 0; /* Initialize only at first call */
	
	err = ggLoadConfig(ggimesaconffile, &_ggimesaConfigHandle);
	if (err != GGI_OK)
	{
		fprintf(stderr, "GGIMesa: Couldn't open %s\n", ggimesaconffile);
		_ggimesaLibIsUp--;
		return err;
	}
	
	ggiMesaID = ggiExtensionRegister("GGIMesa", sizeof(struct mesa_ext), changed);
	
	if (ggiMesaID < 0)
	{
		fprintf(stderr, "GGIMesa: failed to register as extension\n");
		_ggimesaLibIsUp--;
		ggFreeConfig(_ggimesaConfigHandle);
		return ggiMesaID;
	}
	
	return 0;
}


GGIMesaContext GGIMesaCreateContext(void)
{
	GGIMesaContext ctx;
	char *s;

	s = getenv("GGIMESA_DEBUG");
	gl_ggi_debug = (s && atoi(s));

	if (ggiMesaInit() < 0) 		/* register extensions*/
	{
		return NULL;
	}

	ctx = (GGIMesaContext)calloc(1, sizeof(struct ggi_mesa_context));
	if (!ctx) 
	  return NULL;
	
	ctx->gl_vis = (GLvisual *)calloc(1, sizeof(GLvisual));
	if (!ctx->gl_vis) 
	  return NULL;

	ctx->viewport_init = GL_FALSE;	
	ctx->gl_vis->DBflag = GL_FALSE;

	ctx->gl_ctx = gl_create_context(ctx->gl_vis, NULL, (void *)ctx, GL_TRUE);
	if (!ctx->gl_ctx) 
	  return NULL;
	
	return ctx;
}

void GGIMesaDestroyContext(GGIMesaContext ctx)
{
	if (ctx) 
	{
		gl_destroy_visual(ctx->gl_vis);
		gl_destroy_context(ctx->gl_ctx);
		gl_destroy_framebuffer(ctx->gl_buffer);
		if (ctx == GGIMesa) 
		  GGIMesa = NULL;
		if (ctx->ggi_vis) 
		  ggiExtensionDetach(ctx->ggi_vis, ggiMesaID);
		ggiExtensionUnregister(ggiMesaID);
		free(ctx);
	}
}

int GGIMesaSetVisual(GGIMesaContext ctx, ggi_visual_t vis,
		     GLboolean rgb_flag, GLboolean db_flag)
{
	struct ggi_mesa_info info;
	int err;
	uint16 r,g,b;
	ggi_color pal[256];
	int i;
	void *func;
	ggi_mode mode;
	int num_buf;

	if (!ctx) return -1;
	if (!vis) return -1;
	
	if (ctx->ggi_vis)
	  ggiExtensionDetach(ctx->ggi_vis, ggiMesaID);

	ctx->ggi_vis=vis;

	err = ggiExtensionAttach(vis, ggiMesaID);
	if (err < 0) 
	  return -1;
	if (err == 0) 
	  changed(vis, GGI_CHG_APILIST);
	
	if (ctx->gl_vis)
	  gl_destroy_visual(ctx->gl_vis);

	if (ctx->gl_buffer)
	  gl_destroy_framebuffer(ctx->gl_buffer);

	info.rgb_flag = rgb_flag;
	info.db_flag = db_flag;

	err = gl_ggiInitInfo(ctx, &info);
	if (err) 
	  return -1;

	gl_ggiSetupPointers(ctx->gl_ctx);

	func = (void *)LIBGGI_MESAEXT(ctx->ggi_vis)->setup_driver;

	if (!func)
	{
		fprintf(stderr, "setup_driver==NULL!\n");
		fprintf(stderr, "Please check your config files!\n");
		return -1;
	}

	err = LIBGGI_MESAEXT(ctx->ggi_vis)->setup_driver(ctx, &info);
	if (err) 
	  return -1;

	ctx->gl_vis = gl_create_visual(info.rgb_flag,
				       info.alpha_flag,
				       info.db_flag,
				       GL_FALSE,
				       info.depth_bits, 
				       info.stencil_bits,
				       info.accum_bits,
				       info.index_bits,
				       info.red_bits, info.green_bits,
				       info.blue_bits, info.alpha_bits);
	if (!ctx->gl_vis) 
	{
		fprintf(stderr, "Can't create gl_visual!\n");
		return -1;
	}

	ctx->gl_buffer = gl_create_framebuffer(ctx->gl_vis,
					       ctx->gl_vis->DepthBits > 0,
					       ctx->gl_vis->StencilBits > 0,
					       ctx->gl_vis->AccumRedBits > 0,
					       ctx->gl_vis->AlphaBits > 0);
					       

	if (!ctx->gl_buffer) 
	{
		fprintf(stderr, "Can't create gl_buffer!\n");
		return -1;
	}
	
	ggiGetMode(ctx->ggi_vis, &mode);
	ctx->width = mode.visible.x;
	ctx->height = mode.visible.y;
	ctx->stride = mode.virt.x;
	ctx->origin.x = 0;
	ctx->origin.y = 0;
	ctx->flip_y = ctx->origin.y + ctx->height - 1; 

	ctx->color = 0;

	ctx->lfb[0] = ctx->lfb[1] = NULL;
	num_buf = ggiDBGetNumBuffers(ctx->ggi_vis);
	
	for (i = 0; i < num_buf; i++)
	{
		if (ggiDBGetBuffer(ctx->ggi_vis,i)->layout == blPixelLinearBuffer)
		{
			ctx->stride = ggiDBGetBuffer(ctx->ggi_vis, i)->buffer.plb.stride /
			(ggiDBGetBuffer(ctx->ggi_vis, i)->buffer.plb.pixelformat->size / 8);
			ctx->lfb[0] = ggiDBGetBuffer(ctx->ggi_vis, i)->write;
		}
	}
	
	if (ctx->lfb[0] == NULL)
	{
		fprintf(stderr, "No linear frame buffer!\n");
		return -1;
	}
	
	/* FIXME: Use separate buffers */
	ctx->lfb[1] = malloc(ctx->stride * ctx->height);
	ctx->bufsize = (ctx->stride * ctx->height);
	
	ctx->gl_ctx->Visual = ctx->gl_vis;
	ctx->gl_ctx->Pixel.ReadBuffer = 
	ctx->gl_ctx->Color.DrawBuffer = (db_flag) ? GL_BACK : GL_FRONT;

	if (GGIMesa == ctx)
	  gl_make_current(ctx->gl_ctx, ctx->gl_buffer);

	if (rgb_flag && mode.graphtype==GT_8BIT)
	{
		for (i = r = 0; r < 8; r++)
		  for (g = 0; g < 8; g++)
		    for (b = 0; b < 4; b++, i++)
		{
			pal[i].r = r << (GGI_COLOR_PRECISION - 3);	
			pal[i].g = g << (GGI_COLOR_PRECISION - 3);	
			pal[i].b = b << (GGI_COLOR_PRECISION - 2);	
		}
		ggiSetPalette(ctx->ggi_vis, 0, 256, pal);	
	}

	return 0;
}

void GGIMesaMakeCurrent(GGIMesaContext ctx)
{
	if (!ctx->ggi_vis) 
	  return;
	
	GGIMesa = ctx;
	gl_make_current(ctx->gl_ctx, ctx->gl_buffer);
	
	if (!ctx->viewport_init)
	{
		gl_Viewport(ctx->gl_ctx, 0, 0, ctx->width, ctx->height);
		ctx->viewport_init = GL_TRUE;
	}
}


GGIMesaContext GGIMesaGetCurrentContext(void)
{
	return GGIMesa;
}

/*
 * Swap front/back buffers for current context if double buffered.
 */
void GGIMesaSwapBuffers(void)
{
	GGIMESADPRINT_CORE("GGIMesaSwapBuffers\n");
	
	FLUSH_VB(GGIMesa->gl_ctx, "swap buffers");
	gl_ggiFlush(GGIMesa->gl_ctx);
	
	if (GGIMesa->gl_vis->DBflag)
	{
		memcpy(GGIMesa->lfb[0], GGIMesa->lfb[1], GGIMesa->bufsize);
	}
}

static void gl_ggiUpdateState(GLcontext *ctx)
{
	void *func;

	func = (void *)CTX_OPMESA(ctx)->update_state;

	if (!func) {
		fprintf(stderr, "update_state == NULL!\n");
		fprintf(stderr, "Please check your config files!\n");
		ggiPanic("");
	}

	CTX_OPMESA(ctx)->update_state(ctx);
}

static int changed(ggi_visual_t vis, int whatchanged)
{
	switch (whatchanged)
	{
		case GGI_CHG_APILIST:
		{
			char api[256];
			char args[256];
			int i;
			const char *fname;
			ggi_dlhandle *lib;
			
			for (i = 0; ggiGetAPI(vis, i, api, args) == 0; i++)
			{
				strcat(api, "-mesa");
				fname = ggMatchConfig(_ggimesaConfigHandle, api, NULL);
				if (fname == NULL)
				{
					/* No special implementation for this sublib */
					continue;
				}
				lib = ggiExtensionLoadDL(vis, fname, args, NULL, GGI_SYMNAME_PREFIX);
			}
		} 
		break;
	}
	return 0;
}


int ggiMesaExit(void)
{
	int rc;
	
	if (!_ggimesaLibIsUp)
	  return -1;
	
	if (_ggimesaLibIsUp > 1)
	{
		/* Exit only at last call */
		_ggimesaLibIsUp--;
		return 0;
	}
	
	rc = ggiExtensionUnregister(ggiMesaID);
	ggFreeConfig(_ggimesaConfigHandle);
	
	_ggimesaLibIsUp = 0;
	
	return rc;
}

static int _ggi_error(void)
{
	return -1;
}

int ggiMesaAttach(ggi_visual_t vis)
{
	int rc;
	
	rc = ggiExtensionAttach(vis, ggiMesaID);
	if (rc == 0)
	{
		/* We are creating the primary instance */
		memset(LIBGGI_MESAEXT(vis), 0, sizeof(mesaext));
		LIBGGI_MESAEXT(vis)->update_state = (void *)_ggi_error;
		LIBGGI_MESAEXT(vis)->setup_driver = (void *)_ggi_error;
		
		/* Now fake an "API change" so the right libs get loaded */
		changed(vis, GGI_CHG_APILIST);
	}
	
	return rc;
}

int ggiMesaDetach(ggi_visual_t vis)
{
	return ggiExtensionDetach(vis, ggiMesaID);
}


#else
/*
 * Need this to provide at least one external definition when GGI is
 * not defined on the compiler command line.
 */

int gl_ggi_dummy_function(void)
{
	return 0;
}

#endif
