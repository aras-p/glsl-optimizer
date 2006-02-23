/**************************************************************************

Copyright 2006 Stephane Marchesin
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
ERIC ANHOLT OR SILICON INTEGRATED SYSTEMS CORP BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

#include "nouveau_context.h"
#include "nouveau_ioctl.h"
//#include "nouveau_state.h"
#include "nouveau_driver.h"
#include "swrast/swrast.h"

#include "context.h"
#include "framebuffer.h"

#include "utils.h"


/* Return the width and height of the current color buffer */
static void nouveauGetBufferSize( GLframebuffer *buffer,
		GLuint *width, GLuint *height )
{
	GET_CURRENT_CONTEXT(ctx);
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);

	LOCK_HARDWARE( nmesa );
	*width  = nmesa->driDrawable->w;
	*height = nmesa->driDrawable->h;
	UNLOCK_HARDWARE( nmesa );
}

/* glGetString */
static const GLubyte *nouveauGetString( GLcontext *ctx, GLenum name )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	static char buffer[128];
	const char * card_name = "Unknown";
	GLuint agp_mode = 0;

	switch ( name ) {
		case GL_VENDOR:
			return (GLubyte *)DRIVER_AUTHOR;

		case GL_RENDERER:
			switch(nmesa->screen->card_type)
			{
				case NV_03:
					card_name="Riva 128";
					break;
				case NV_04:
					card_name="TNT";
					break;
				case NV_05:
					card_name="TNT2";
					break;
				case NV_10:
					card_name="GeForce 1/2/4Mx";
					break;
				case NV_20:
					card_name="GeForce 3/4Ti";
					break;
				case NV_30:
					card_name="GeForce FX 5x00";
					break;
				case NV_40:
					card_name="GeForce FX 6x00";
					break;
				case G_70:
					card_name="GeForce FX 7x00";
					break;
				default:
					break;
			}

			switch(nmesa->screen->bus_type)
			{
				case NV_PCI:
				case NV_PCIE:
				default:
					agp_mode=0;
					break;
				case NV_AGP:
					nmesa->screen->agp_mode;
					break;
			}
			driGetRendererString( buffer, card_name, DRIVER_DATE,
					agp_mode );
			return (GLubyte *)buffer;
		default:
			return NULL;
	}
}

/* glFlush */
static void nouveauFlush( GLcontext *ctx )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	FIRE_RING( nmesa );
}

/* glFinish */
static void nouveauFinish( GLcontext *ctx )
{
	nouveauContextPtr nmesa = NOUVEAU_CONTEXT(ctx);
	nouveauFlush( ctx );
	nouveauWaitForIdle( nmesa );
}

/* glClear */
static void nouveauClear( GLcontext *ctx, GLbitfield mask, GLboolean all,
		GLint cx, GLint cy, GLint cw, GLint ch )
{
	// XXX we really should do something here...
}

void nouveauDriverInitFunctions( struct dd_function_table *functions )
{
	functions->GetBufferSize	= nouveauGetBufferSize;
	functions->ResizeBuffers	= _mesa_resize_framebuffer;
	functions->GetString		= nouveauGetString;
	functions->Flush		= nouveauFlush;
	functions->Finish		= nouveauFinish;
	functions->Clear		= nouveauClear;
}

