/*
 * Mesa 3-D graphics library
 * Version:  6.1
 *
 * Copyright (C) 1999-2004  Brian Paul   All Rights Reserved.
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


/* Minimal swrast-based dri loadable driver.
 *
 * Todo:
 *   -- Use malloced (rather than framebuffer) memory for backbuffer
 *   -- 32bpp is hardwared -- fix
 *
 * NOTES:
 *   -- No mechanism for cliprects or resize notification --
 *      assumes this is a fullscreen device.  
 *   -- No locking -- assumes this is the only driver accessing this 
 *      device.
 *   -- Doesn't (yet) make use of any acceleration or other interfaces
 *      provided by fb.  Would be entirely happy working against any 
 *	fullscreen interface.
 *   -- HOWEVER: only a small number of pixelformats are supported, and
 *      the mechanism for choosing between them makes some assumptions
 *      that may not be valid everywhere.
 */

#include <assert.h>
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <linux/kd.h>
#include <linux/vt.h>

#include "GL/miniglx.h"         /* window-system-specific */
#include "miniglxP.h" 		/* window-system-specific */
#include "dri_util.h"		/* window-system-specific-ish */

#include "context.h"
#include "extensions.h"
#include "imports.h"
#include "matrix.h"
#include "texformat.h"
#include "texstore.h"
#include "teximage.h"
#include "array_cache/acache.h"
#include "swrast/swrast.h"
#include "swrast_setup/swrast_setup.h"
#include "tnl/tnl.h"
#include "tnl/t_context.h"
#include "tnl/t_pipeline.h"
#include "drivers/common/driverfuncs.h"



typedef struct {
   GLcontext *glCtx;		/* Mesa context */

   struct {
      __DRIcontextPrivate *context;	
      __DRIscreenPrivate *screen;	
      __DRIdrawablePrivate *drawable; /* drawable bound to this ctx */
   } dri;
} fbContext, *fbContextPtr;


#define FB_CONTEXT(ctx)		((fbContextPtr)(ctx->DriverCtx))


static const GLubyte *
get_string(GLcontext *ctx, GLenum pname)
{
   (void) ctx;
   switch (pname) {
      case GL_RENDERER:
         return (const GLubyte *) "Mesa dumb framebuffer";
      default:
         return NULL;
   }
}


static void
update_state( GLcontext *ctx, GLuint new_state )
{
   /* not much to do here - pass it on */
   _swrast_InvalidateState( ctx, new_state );
   _swsetup_InvalidateState( ctx, new_state );
   _ac_InvalidateState( ctx, new_state );
   _tnl_InvalidateState( ctx, new_state );
}


/**
 * Called by ctx->Driver.GetBufferSize from in core Mesa to query the
 * current framebuffer size.
 */
static void
get_buffer_size( GLframebuffer *buffer, GLuint *width, GLuint *height )
{
   GET_CURRENT_CONTEXT(ctx);
   fbContextPtr fbmesa = FB_CONTEXT(ctx);

   *width  = fbmesa->dri.drawable->w;
   *height = fbmesa->dri.drawable->h;
}


/* specifies the buffer for swrast span rendering/reading */
static void
set_buffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bufferBit )
{
   fbContextPtr fbdevctx = FB_CONTEXT(ctx);
   __DRIdrawablePrivate *dPriv = fbdevctx->dri.drawable;
    
   /* What a twisted mess of private structs
    */
   assert(buffer == dPriv->driverPrivate);


   switch (bufferBit) {
   case DD_FRONT_LEFT_BIT:
      dPriv->currentBuffer = dPriv->frontBuffer;
      break;
   case DD_BACK_LEFT_BIT:
      dPriv->currentBuffer = dPriv->backBuffer;
      break;
   default:
      /* This happens a lot if the client renders to the frontbuffer */
      if (0) _mesa_problem(ctx, "bad bufferBit in set_buffer()");
   }
}


static void
init_core_functions( struct dd_function_table *functions )
{
   functions->GetString = get_string;
   functions->UpdateState = update_state;
   functions->ResizeBuffers = _swrast_alloc_buffers;
   functions->GetBufferSize = get_buffer_size;

   functions->Clear = _swrast_Clear;  /* could accelerate with blits */
}


/*
 * Generate code for span functions.
 */

/* 24-bit BGR */
#define NAME(PREFIX) PREFIX##_B8G8R8
#define SPAN_VARS \
   const fbContextPtr fbdevctx = FB_CONTEXT(ctx); \
   __DRIdrawablePrivate *dPriv = fbdevctx->dri.drawable; 
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)dPriv->currentBuffer + (Y) * dPriv->currentPitch + (X) * 3
#define INC_PIXEL_PTR(P) P += 3
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = B;  P[1] = G;  P[2] = R
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = B;  P[1] = G;  P[2] = R
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[2];  G = P[1];  B = P[0];  A = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 32-bit BGRA */
#define NAME(PREFIX) PREFIX##_B8G8R8A8
#define SPAN_VARS \
   const fbContextPtr fbdevctx = FB_CONTEXT(ctx); \
   __DRIdrawablePrivate *dPriv = fbdevctx->dri.drawable; 
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)dPriv->currentBuffer + (Y) * dPriv->currentPitch + (X) * 4;
#define INC_PIXEL_PTR(P) P += 4
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   P[0] = B;  P[1] = G;  P[2] = R;  P[3] = 255
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   P[0] = B;  P[1] = G;  P[2] = R;  P[3] = A
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = P[2];  G = P[1];  B = P[0];  A = P[3]

#include "swrast/s_spantemp.h"


/* 16-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G6R5
#define SPAN_VARS \
   const fbContextPtr fbdevctx = FB_CONTEXT(ctx); \
   __DRIdrawablePrivate *dPriv = fbdevctx->dri.drawable;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) ((char *)dPriv->currentBuffer + (Y) * dPriv->currentPitch + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   *P = ( (((R) & 0xf8) << 8) | (((G) & 0xfc) << 3) | ((B) >> 3) )
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   *P = ( (((R) & 0xf8) << 8) | (((G) & 0xfc) << 3) | ((B) >> 3) )
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = ( (((*P) >> 8) & 0xf8) | (((*P) >> 11) & 0x7) ); \
   G = ( (((*P) >> 3) & 0xfc) | (((*P) >>  5) & 0x3) ); \
   B = ( (((*P) << 3) & 0xf8) | (((*P)      ) & 0x7) ); \
   A = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 15-bit BGR (XXX implement dithering someday) */
#define NAME(PREFIX) PREFIX##_B5G5R5
#define SPAN_VARS \
   const fbContextPtr fbdevctx = FB_CONTEXT(ctx); \
   __DRIdrawablePrivate *dPriv = fbdevctx->dri.drawable;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLushort *P = (GLushort *) ((char *)dPriv->currentBuffer + (Y) * dPriv->currentPitch + (X) * 2)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_RGB_PIXEL(P, X, Y, R, G, B) \
   *P = ( (((R) & 0xf8) << 7) | (((G) & 0xf8) << 2) | ((B) >> 3) )
#define STORE_RGBA_PIXEL(P, X, Y, R, G, B, A) \
   *P = ( (((R) & 0xf8) << 7) | (((G) & 0xf8) << 2) | ((B) >> 3) )
#define FETCH_RGBA_PIXEL(R, G, B, A, P) \
   R = ( (((*P) >> 7) & 0xf8) | (((*P) >> 10) & 0x7) ); \
   G = ( (((*P) >> 2) & 0xf8) | (((*P) >>  5) & 0x7) ); \
   B = ( (((*P) << 3) & 0xf8) | (((*P)      ) & 0x7) ); \
   A = CHAN_MAX

#include "swrast/s_spantemp.h"


/* 8-bit color index */
#define NAME(PREFIX) PREFIX##_CI8
#define SPAN_VARS \
   const fbContextPtr fbdevctx = FB_CONTEXT(ctx); \
   __DRIdrawablePrivate *dPriv = fbdevctx->dri.drawable;
#define INIT_PIXEL_PTR(P, X, Y) \
   GLubyte *P = (GLubyte *)dPriv->currentBuffer + (Y) * dPriv->currentPitch + (X)
#define INC_PIXEL_PTR(P) P += 1
#define STORE_CI_PIXEL(P, CI) \
   P[0] = CI
#define FETCH_CI_PIXEL(CI, P) \
   CI = P[0]

 #include "swrast/s_spantemp.h"



/* Initialize the driver specific screen private data.
 */
static GLboolean
fbInitDriver( __DRIscreenPrivate *sPriv )
{
   sPriv->private = NULL;
   return GL_TRUE;
}

static void
fbDestroyScreen( __DRIscreenPrivate *sPriv )
{
}

/* Create the device specific context.
 */
static GLboolean
fbCreateContext( const __GLcontextModes *glVisual,
		 __DRIcontextPrivate *driContextPriv,
		 void *sharedContextPrivate)
{
   fbContextPtr fbmesa;
   GLcontext *ctx, *shareCtx;
   struct dd_function_table functions;

   assert(glVisual);
   assert(driContextPriv);

   /* Allocate the Fb context */
   fbmesa = (fbContextPtr) _mesa_calloc( sizeof(*fbmesa) );
   if ( !fbmesa )
      return GL_FALSE;

   /* Init default driver functions then plug in our FBdev-specific functions
    */
   _mesa_init_driver_functions(&functions);
   init_core_functions(&functions);

   /* Allocate the Mesa context */
   if (sharedContextPrivate)
      shareCtx = ((fbContextPtr) sharedContextPrivate)->glCtx;
   else
      shareCtx = NULL;

   ctx = fbmesa->glCtx = _mesa_create_context(glVisual, shareCtx, 
					      &functions, (void *) fbmesa);
   if (!fbmesa->glCtx) {
      _mesa_free(fbmesa);
      return GL_FALSE;
   }
   driContextPriv->driverPrivate = fbmesa;

   /* Create module contexts */
   _swrast_CreateContext( ctx );
   _ac_CreateContext( ctx );
   _tnl_CreateContext( ctx );
   _swsetup_CreateContext( ctx );
   _swsetup_Wakeup( ctx );


   /* swrast init -- need to verify these tests - I just plucked the
    * numbers out of the air.  (KW)
    */
   {
      struct swrast_device_driver *swdd;
      swdd = _swrast_GetDeviceDriverReference( ctx );
      swdd->SetBuffer = set_buffer;
      if (!glVisual->rgbMode) {
         swdd->WriteCI32Span = write_index32_span_CI8;
         swdd->WriteCI8Span = write_index8_span_CI8;
         swdd->WriteMonoCISpan = write_monoindex_span_CI8;
         swdd->WriteCI32Pixels = write_index_pixels_CI8;
         swdd->WriteMonoCIPixels = write_monoindex_pixels_CI8;
         swdd->ReadCI32Span = read_index_span_CI8;
         swdd->ReadCI32Pixels = read_index_pixels_CI8;
      }
      else if (glVisual->rgbBits == 24 &&
	       glVisual->alphaBits == 0) {
         swdd->WriteRGBASpan = write_rgba_span_B8G8R8;
         swdd->WriteRGBSpan = write_rgb_span_B8G8R8;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B8G8R8;
         swdd->WriteRGBAPixels = write_rgba_pixels_B8G8R8;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B8G8R8;
         swdd->ReadRGBASpan = read_rgba_span_B8G8R8;
         swdd->ReadRGBAPixels = read_rgba_pixels_B8G8R8;
      }
      else if (glVisual->rgbBits == 32 &&
	       glVisual->alphaBits == 8) {
         swdd->WriteRGBASpan = write_rgba_span_B8G8R8A8;
         swdd->WriteRGBSpan = write_rgb_span_B8G8R8A8;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B8G8R8A8;
         swdd->WriteRGBAPixels = write_rgba_pixels_B8G8R8A8;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B8G8R8A8;
         swdd->ReadRGBASpan = read_rgba_span_B8G8R8A8;
         swdd->ReadRGBAPixels = read_rgba_pixels_B8G8R8A8;
      }
      else if (glVisual->rgbBits == 16 &&
	       glVisual->alphaBits == 0) {
         swdd->WriteRGBASpan = write_rgba_span_B5G6R5;
         swdd->WriteRGBSpan = write_rgb_span_B5G6R5;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B5G6R5;
         swdd->WriteRGBAPixels = write_rgba_pixels_B5G6R5;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B5G6R5;
         swdd->ReadRGBASpan = read_rgba_span_B5G6R5;
         swdd->ReadRGBAPixels = read_rgba_pixels_B5G6R5;
      }
      else if (glVisual->rgbBits == 15 &&
	       glVisual->alphaBits == 0) {
         swdd->WriteRGBASpan = write_rgba_span_B5G5R5;
         swdd->WriteRGBSpan = write_rgb_span_B5G5R5;
         swdd->WriteMonoRGBASpan = write_monorgba_span_B5G5R5;
         swdd->WriteRGBAPixels = write_rgba_pixels_B5G5R5;
         swdd->WriteMonoRGBAPixels = write_monorgba_pixels_B5G5R5;
         swdd->ReadRGBASpan = read_rgba_span_B5G5R5;
         swdd->ReadRGBAPixels = read_rgba_pixels_B5G5R5;
      }
      else {
         _mesa_printf("bad pixelformat rgb %d alpha %d\n",
		      glVisual->rgbBits, 
		      glVisual->alphaBits );
      }
   }

   /* use default TCL pipeline */
   {
      TNLcontext *tnl = TNL_CONTEXT(ctx);
      tnl->Driver.RunPipeline = _tnl_run_pipeline;
   }

   _mesa_enable_sw_extensions(ctx);

   return GL_TRUE;
}


static void
fbDestroyContext( __DRIcontextPrivate *driContextPriv )
{
   GET_CURRENT_CONTEXT(ctx);
   fbContextPtr fbmesa = (fbContextPtr) driContextPriv->driverPrivate;
   fbContextPtr current = ctx ? FB_CONTEXT(ctx) : NULL;

   /* check if we're deleting the currently bound context */
   if (fbmesa == current) {
      _mesa_make_current2(NULL, NULL, NULL);
   }

   /* Free fb context resources */
   if ( fbmesa ) {
      _swsetup_DestroyContext( fbmesa->glCtx );
      _tnl_DestroyContext( fbmesa->glCtx );
      _ac_DestroyContext( fbmesa->glCtx );
      _swrast_DestroyContext( fbmesa->glCtx );

      /* free the Mesa context */
      fbmesa->glCtx->DriverCtx = NULL;
      _mesa_destroy_context( fbmesa->glCtx );

      _mesa_free( fbmesa );
   }
}


/* Create and initialize the Mesa and driver specific pixmap buffer
 * data.
 */
static GLboolean
fbCreateBuffer( __DRIscreenPrivate *driScrnPriv,
		__DRIdrawablePrivate *driDrawPriv,
		const __GLcontextModes *mesaVis,
		GLboolean isPixmap )
{
   if (isPixmap) {
      return GL_FALSE; /* not implemented */
   }
   else {
      const GLboolean swDepth = mesaVis->depthBits > 0;
      const GLboolean swAlpha = mesaVis->alphaBits > 0;
      const GLboolean swAccum = mesaVis->accumRedBits > 0;
      const GLboolean swStencil = mesaVis->stencilBits > 0;
      driDrawPriv->driverPrivate = (void *)
         _mesa_create_framebuffer( mesaVis,
                                   swDepth,
                                   swStencil,
                                   swAccum,
                                   swAlpha );

      if (!driDrawPriv->driverPrivate)
	 return 0;
      
      /* Replace the framebuffer back buffer with a malloc'ed one --
       * big speedup.
       */
      if (driDrawPriv->backBuffer)
	 driDrawPriv->backBuffer = _mesa_malloc(driDrawPriv->currentPitch * driDrawPriv->h);

      return 1;
   }
}


static void
fbDestroyBuffer(__DRIdrawablePrivate *driDrawPriv)
{
   _mesa_destroy_framebuffer((GLframebuffer *) (driDrawPriv->driverPrivate));
   _mesa_free(driDrawPriv->backBuffer);
}



/* If the backbuffer is on a videocard, this is extraordinarily slow!
 */
static void
fbSwapBuffers( __DRIdrawablePrivate *dPriv )
{

   if (dPriv->driContextPriv && dPriv->driContextPriv->driverPrivate) {
      fbContextPtr fbmesa;
      GLcontext *ctx;
      fbmesa = (fbContextPtr) dPriv->driContextPriv->driverPrivate;
      ctx = fbmesa->glCtx;
      if (ctx->Visual.doubleBufferMode) {
	 int i;
	 int offset = 0;
	 char *tmp = _mesa_malloc(dPriv->currentPitch);

         _mesa_notifySwapBuffers( ctx );  /* flush pending rendering comands */

	 ASSERT(dPriv->frontBuffer);
	 ASSERT(dPriv->backBuffer);

	 for (i = 0; i < dPriv->h; i++) {
	    _mesa_memcpy(tmp, (char *) dPriv->backBuffer + offset,
                         dPriv->currentPitch);
	    _mesa_memcpy((char *) dPriv->frontBuffer + offset, tmp,
                         dPriv->currentPitch);
	    offset += dPriv->currentPitch;
	 }
	    
	 _mesa_free(tmp);
      }
   }
   else {
      /* XXX this shouldn't be an error but we can't handle it for now */
      _mesa_problem(NULL, "fbSwapBuffers: drawable has no context!\n");
   }
}


/* Force the context `c' to be the current context and associate with it
 * buffer `b'.
 */
static GLboolean
fbMakeCurrent( __DRIcontextPrivate *driContextPriv,
	       __DRIdrawablePrivate *driDrawPriv,
	       __DRIdrawablePrivate *driReadPriv )
{
   if ( driContextPriv ) {
      fbContextPtr newFbCtx = 
	 (fbContextPtr) driContextPriv->driverPrivate;

      newFbCtx->dri.drawable = driDrawPriv;

      _mesa_make_current2( newFbCtx->glCtx,
			   (GLframebuffer *) driDrawPriv->driverPrivate,
			   (GLframebuffer *) driReadPriv->driverPrivate );

      if ( !newFbCtx->glCtx->Viewport.Width ) {
	 _mesa_set_viewport( newFbCtx->glCtx, 0, 0,
			     driDrawPriv->w, driDrawPriv->h );
      }
   } else {
      _mesa_make_current( 0, 0 );
   }

   return GL_TRUE;
}


/* Force the context `c' to be unbound from its buffer.
 */
static GLboolean
fbUnbindContext( __DRIcontextPrivate *driContextPriv )
{
   return GL_TRUE;
}

static struct __DriverAPIRec fbAPI = {
   fbInitDriver,
   fbDestroyScreen,
   fbCreateContext,
   fbDestroyContext,
   fbCreateBuffer,
   fbDestroyBuffer,
   fbSwapBuffers,
   fbMakeCurrent,
   fbUnbindContext
};


/*
 * This is the bootstrap function for the driver.
 * The __driCreateScreen name is the symbol that libGL.so fetches.
 * Return:  pointer to a __DRIscreenPrivate.
 */
void *
__driCreateScreen(struct DRIDriverRec *driver,
                  struct DRIDriverContextRec *driverContext)
{
   __DRIscreenPrivate *psp;
   psp = __driUtilCreateScreenNoDRM(driver, driverContext, &fbAPI);
   return (void *) psp;
}


/**
 * \brief Establish the set of modes available for the display.
 *
 * \param ctx display handle.
 * \param numModes will receive the number of supported modes.
 * \param modes will point to the list of supported modes.
 *
 * \return one on success, or zero on failure.
 * 
 * Allocates a single visual and fills it with information according to the
 * display bit depth. Supports only 16 and 32 bpp bit depths, aborting
 * otherwise.
 */
const __GLcontextModes __glModes[] = {
    
    /* 32 bit, RGBA Depth=24 Stencil=8 */
    {.rgbMode = GL_TRUE, .colorIndexMode = GL_FALSE, .doubleBufferMode = GL_TRUE, .stereoMode = GL_FALSE,
     .haveAccumBuffer = GL_FALSE, .haveDepthBuffer = GL_TRUE, .haveStencilBuffer = GL_TRUE,
     .redBits = 8, .greenBits = 8, .blueBits = 8, .alphaBits = 8,
     .redMask = 0xff0000, .greenMask = 0xff00, .blueMask = 0xff, .alphaMask = 0xff000000,
     .rgbBits = 32, .indexBits = 0,
     .accumRedBits = 0, .accumGreenBits = 0, .accumBlueBits = 0, .accumAlphaBits = 0,
     .depthBits = 24, .stencilBits = 8,
     .numAuxBuffers= 0, .level = 0, .pixmapMode = GL_FALSE, },

    /* 16 bit, RGB Depth=16 */
    {.rgbMode = GL_TRUE, .colorIndexMode = GL_FALSE, .doubleBufferMode = GL_TRUE, .stereoMode = GL_FALSE,
     .haveAccumBuffer = GL_FALSE, .haveDepthBuffer = GL_TRUE, .haveStencilBuffer = GL_FALSE,
     .redBits = 5, .greenBits = 6, .blueBits = 5, .alphaBits = 0,
     .redMask = 0xf800, .greenMask = 0x07e0, .blueMask = 0x001f, .alphaMask = 0x0,
     .rgbBits = 16, .indexBits = 0,
     .accumRedBits = 0, .accumGreenBits = 0, .accumBlueBits = 0, .accumAlphaBits = 0,
     .depthBits = 16, .stencilBits = 0,
     .numAuxBuffers= 0, .level = 0, .pixmapMode = GL_FALSE, },
};


static int
__driInitScreenModes( const DRIDriverContext *ctx,
                      int *numModes, const __GLcontextModes **modes)
{
   *numModes = sizeof(__glModes)/sizeof(__GLcontextModes *);
   *modes = &__glModes[0];
   return 1;
}



static int
__driValidateMode(const DRIDriverContext *ctx )
{
   return 1;
}

static int
__driInitFBDev( struct DRIDriverContextRec *ctx )
{
   int id;
   ctx->shared.hFrameBuffer = ctx->FBStart;
   ctx->shared.fbSize = ctx->FBSize;
   ctx->shared.hSAREA = 0xB37D;
   ctx->shared.SAREASize = SAREA_MAX;
   id = shmget(ctx->shared.hSAREA, ctx->shared.SAREASize, IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR);
   if (id == -1) {
       /* segment will already exist if previous server segfaulted */
       id = shmget(ctx->shared.hSAREA, ctx->shared.SAREASize, 0);
       if (id == -1) {
	   fprintf(stderr, "fb: shmget failed\n");
	   return 0;
       }
   }
   ctx->pSAREA = shmat(id, NULL, 0);
   if (ctx->pSAREA == (void *)-1) {
       fprintf(stderr, "fb: shmat failed\n");
       return 0;
   }
   memset(ctx->pSAREA, 0, SAREA_MAX);
   return 1;
}

static void
__driHaltFBDev( struct DRIDriverContextRec *ctx )
{
}



struct DRIDriverRec __driDriver = {
   __driInitScreenModes,
   __driValidateMode,
   __driValidateMode,
   __driInitFBDev,
   __driHaltFBDev
};

