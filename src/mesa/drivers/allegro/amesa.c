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

#include <stdio.h>
#include <stdlib.h>
#include <allegro.h>
#include "main/buffers.h"
#include "main/context.h"
#include "main/imports.h"
#include "main/matrix.h"
#include "main/mtypes.h"
#include "GL/amesa.h"


struct amesa_visual
    {
    GLvisual   *GLVisual;       /* inherit from GLvisual      */
    GLboolean   DBFlag;         /* double buffered?           */
    GLuint      Depth;          /* bits per pixel ( >= 15 )   */
    };


struct amesa_buffer
    {
    GLframebuffer *GLBuffer;    /* inherit from GLframebuffer */
    GLuint         Width, Height;
    BITMAP        *Screen;
	BITMAP        *Background;
	BITMAP        *Active;
    };


struct amesa_context
    {
    GLcontext   *GLContext;     /* inherit from GLcontext     */
    AMesaVisual  Visual;
	AMesaBuffer  Buffer;
    GLuint       ClearColor;
    GLuint       CurrentColor;
    };


static void setup_dd_pointers(GLcontext *ctx);


/**********************************************************************/
/*****                   drawing functions                        *****/
/**********************************************************************/

#define FLIP(context, y)  (context->Buffer->Height - (y) - 1)

#include "allegro/generic.h"
#include "allegro/direct.h"


/**********************************************************************/
/*****            15-bit accelerated drawing funcs                *****/
/**********************************************************************/

IMPLEMENT_WRITE_RGBA_SPAN(15, unsigned short)
IMPLEMENT_WRITE_RGB_SPAN(15, unsigned short)
IMPLEMENT_WRITE_MONO_RGBA_SPAN(15, unsigned short)
IMPLEMENT_READ_RGBA_SPAN(15, unsigned short)
IMPLEMENT_WRITE_RGBA_PIXELS(15, unsigned short)
IMPLEMENT_WRITE_MONO_RGBA_PIXELS(15, unsigned short)
IMPLEMENT_READ_RGBA_PIXELS(15, unsigned short)


/**********************************************************************/
/*****            16-bit accelerated drawing funcs                *****/
/**********************************************************************/

IMPLEMENT_WRITE_RGBA_SPAN(16, unsigned short)
IMPLEMENT_WRITE_RGB_SPAN(16, unsigned short)
IMPLEMENT_WRITE_MONO_RGBA_SPAN(16, unsigned short)
IMPLEMENT_READ_RGBA_SPAN(16, unsigned short)
IMPLEMENT_WRITE_RGBA_PIXELS(16, unsigned short)
IMPLEMENT_WRITE_MONO_RGBA_PIXELS(16, unsigned short)
IMPLEMENT_READ_RGBA_PIXELS(16, unsigned short)


/**********************************************************************/
/*****            32-bit accelerated drawing funcs                *****/
/**********************************************************************/

IMPLEMENT_WRITE_RGBA_SPAN(32, unsigned long)
IMPLEMENT_WRITE_RGB_SPAN(32, unsigned long)
IMPLEMENT_WRITE_MONO_RGBA_SPAN(32, unsigned long)
IMPLEMENT_READ_RGBA_SPAN(32, unsigned long)
IMPLEMENT_WRITE_RGBA_PIXELS(32, unsigned long)
IMPLEMENT_WRITE_MONO_RGBA_PIXELS(32, unsigned long)
IMPLEMENT_READ_RGBA_PIXELS(32, unsigned long)


/**********************************************************************/
/*****              Miscellaneous device driver funcs             *****/
/**********************************************************************/

static GLboolean set_buffer(GLcontext *ctx, GLframebuffer *buffer, GLuint bit)
    {
    AMesaContext context = (AMesaContext)(ctx->DriverCtx);
    GLboolean    ok      = GL_TRUE;

    if (bit == DD_FRONT_LEFT_BIT)
        context->Buffer->Active = context->Buffer->Screen;

    else if (bit == DD_BACK_LEFT)
        {
        if (context->Buffer->Background)
            context->Buffer->Active = context->Buffer->Background;
        else
            ok = GL_FALSE;
        }

    else
        ok = GL_FALSE;

    return ok;
    }


static void get_buffer_size(GLcontext *ctx, GLuint *width, GLuint *height)
    {
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

    *width  = context->Buffer->Width;
    *height = context->Buffer->Height;
    }


/**
 * We only implement this function as a mechanism to check if the
 * framebuffer size has changed (and update corresponding state).
 */
static void viewport(GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h)
{
   /* poll for window size change and realloc software Z/stencil/etc if needed */
   GLuint newWidth, newHeight;
   GLframebuffer *buffer = ctx->WinSysDrawBuffer;
   get_buffer_size( &newWidth, &newHeight );
   if (buffer->Width != newWidth || buffer->Height != newHeight) {
      _mesa_resize_framebuffer(ctx, buffer, newWidth, newHeight );
   }

}


/**********************************************************************/
/**********************************************************************/

static void setup_dd_pointers(GLcontext *ctx)
	{
	AMesaContext context = (AMesaContext)(ctx->DriverCtx);

   	/* Initialize all the pointers in the driver struct. Do this whenever */
   	/* a new context is made current or we change buffers via set_buffer! */

    ctx->Driver.UpdateState   = setup_dd_pointers;
    ctx->Driver.SetBuffer     = set_buffer;
    ctx->Driver.GetBufferSize = get_buffer_size;
    ctx->Driver.Viewport      = viewport;

    ctx->Driver.Color               = set_color_generic;
    ctx->Driver.ClearColor          = clear_color_generic;
    ctx->Driver.Clear               = clear_generic;
    ctx->Driver.WriteRGBASpan       = write_rgba_span_generic;
    ctx->Driver.WriteRGBSpan        = write_rgb_span_generic;
    ctx->Driver.WriteMonoRGBASpan   = write_mono_rgba_span_generic;
    ctx->Driver.WriteRGBAPixels     = write_rgba_pixels_generic;
    ctx->Driver.WriteMonoRGBAPixels = write_mono_rgba_pixels_generic;
    ctx->Driver.ReadRGBASpan        = read_rgba_span_generic;
    ctx->Driver.ReadRGBAPixels      = read_rgba_pixels_generic;

    if (context->Buffer->Active != screen)
        {
        switch (context->Visual->Depth)
            {
            case 15:
                ctx->Driver.WriteRGBASpan       = write_rgba_span_15;
                ctx->Driver.WriteRGBSpan        = write_rgb_span_15;
                ctx->Driver.WriteMonoRGBASpan   = write_mono_rgba_span_15;
                ctx->Driver.WriteRGBAPixels     = write_rgba_pixels_15;
                ctx->Driver.WriteMonoRGBAPixels = write_mono_rgba_pixels_15;
                ctx->Driver.ReadRGBASpan        = read_rgba_span_15;
                ctx->Driver.ReadRGBAPixels      = read_rgba_pixels_15;
                break;

            case 16:
                ctx->Driver.WriteRGBASpan       = write_rgba_span_16;
                ctx->Driver.WriteRGBSpan        = write_rgb_span_16;
                ctx->Driver.WriteMonoRGBASpan   = write_mono_rgba_span_16;
                ctx->Driver.WriteRGBAPixels     = write_rgba_pixels_16;
                ctx->Driver.WriteMonoRGBAPixels = write_mono_rgba_pixels_16;
                ctx->Driver.ReadRGBASpan        = read_rgba_span_16;
                ctx->Driver.ReadRGBAPixels      = read_rgba_pixels_16;
                break;

            case 32:
                ctx->Driver.WriteRGBASpan       = write_rgba_span_32;
                ctx->Driver.WriteRGBSpan        = write_rgb_span_32;
                ctx->Driver.WriteMonoRGBASpan   = write_mono_rgba_span_32;
                ctx->Driver.WriteRGBAPixels     = write_rgba_pixels_32;
                ctx->Driver.WriteMonoRGBAPixels = write_mono_rgba_pixels_32;
                ctx->Driver.ReadRGBASpan        = read_rgba_span_32;
                ctx->Driver.ReadRGBAPixels      = read_rgba_pixels_32;
                break;
            }
        }
	}


/**********************************************************************/
/*****                AMesa Public API Functions                  *****/
/**********************************************************************/


AMesaVisual AMesaCreateVisual(GLboolean dbFlag, GLint depth,
                              GLint depthSize, GLint stencilSize, GLint accumSize)
	{
   	AMesaVisual visual;
    GLbyte      redBits, greenBits, blueBits;

    visual = (AMesaVisual)calloc(1, sizeof(struct amesa_visual));
   	if (!visual)
    	return NULL;

    switch (depth)
        {
        case 15:
            redBits   = 5;
            greenBits = 5;
            blueBits  = 5;
            break;

        case 16:
            redBits   = 5;
            greenBits = 6;
            blueBits  = 5;
            break;

        case 24: case 32:
            redBits   = 8;
            greenBits = 8;
            blueBits  = 8;
            break;

        default:
            free(visual);
            return NULL;
        }

    visual->DBFlag   = dbFlag;
    visual->Depth    = depth;
    visual->GLVisual = _mesa_create_visual(GL_TRUE,    /* rgb mode       */
                                           dbFlag,     /* db_flag        */
                                           GL_FALSE,   /* stereo         */
                                           redBits, greenBits, blueBits, 8,
                                           0,          /* index bits     */
                                           depthSize,  /* depth bits     */
                                           stencilSize,/* stencil bits   */
                                           accumSize,  /* accum bits     */
                                           accumSize,  /* accum bits     */
                                           accumSize,  /* accum bits     */
                                           accumSize,  /* accum bits     */
                                           1 );
    if (!visual->GLVisual)
        {
        free(visual);
        return NULL;
        }

  	return visual;
	}


void AMesaDestroyVisual(AMesaVisual visual)
	{
           _mesa_destroy_visual(visual->GLVisual);
           free(visual);
	}


AMesaBuffer AMesaCreateBuffer(AMesaVisual visual,
							  GLint width, GLint height)
	{
   	AMesaBuffer buffer;

    buffer = (AMesaBuffer)calloc(1, sizeof(struct amesa_buffer));
   	if (!buffer)
     	return NULL;

	buffer->Screen     = NULL;
	buffer->Background = NULL;
	buffer->Active     = NULL;
	buffer->Width      = width;
	buffer->Height     = height;

	if (visual->DBFlag)
		{
		buffer->Background = create_bitmap_ex(visual->Depth, width, height);
		if (!buffer->Background)
			{
			free(buffer);
			return NULL;
			}
		}

    buffer->GLBuffer = _mesa_create_framebuffer(visual->GLVisual);
    if (!buffer->GLBuffer)
        {
        if (buffer->Background) destroy_bitmap(buffer->Background);
        free(buffer);
        return NULL;
        }

   	return buffer;
	}


void AMesaDestroyBuffer(AMesaBuffer buffer)
{
   if (buffer->Screen)     destroy_bitmap(buffer->Screen);
   if (buffer->Background) destroy_bitmap(buffer->Background);
   _mesa_unreference_framebuffer(&buffer->GLBuffer);
   free(buffer);
}


AMesaContext AMesaCreateContext(AMesaVisual visual,
                                AMesaContext share)
{
   	AMesaContext context;
   	GLboolean    direct = GL_FALSE;

	context = (AMesaContext)calloc(1, sizeof(struct amesa_context));
	if (!context)
		return NULL;

	context->Visual       = visual;
	context->Buffer		  = NULL;
	context->ClearColor   = 0;
	context->CurrentColor = 0;
	context->GLContext    = _mesa_create_context(visual->GLVisual,
                                              share ? share->GLContext : NULL,
                                  		(void *) context, GL_FALSE );
	if (!context->GLContext)
        {
        	free(context);
	        return NULL;
        }

   	return context;
}


void AMesaDestroyContext(AMesaContext context)
{
   _mesa_destroy_context(context->GLContext);
   free(context);
}


GLboolean AMesaMakeCurrent(AMesaContext context, AMesaBuffer buffer)
{
   if (context && buffer) {
      set_color_depth(context->Visual->Depth);
      if (set_gfx_mode(GFX_AUTODETECT, buffer->Width, buffer->Height, 0, 0) != 0)
         return GL_FALSE;

      context->Buffer = buffer;
      buffer->Screen  = screen;
      buffer->Active  = buffer->Background ? buffer->Background : screen;
        
      setup_dd_pointers(context->GLContext);
      _mesa_make_current(context->GLContext, buffer->GLBuffer);
   }
   else {
      /* XXX I don't think you want to destroy anything here! */
      destroy_bitmap(context->Buffer->Screen);
      context->Buffer->Screen = NULL;
      context->Buffer->Active = NULL;
      context->Buffer         = NULL;
      _mesa_make_current(NULL, NULL);
   }

   return GL_TRUE;
}


void AMesaSwapBuffers(AMesaBuffer buffer)
{
   if (buffer->Background) {
      blit(buffer->Background, buffer->Screen,
           0, 0, 0, 0,
           buffer->Width, buffer->Height);
   }
}
