/* $Id: fxapi.c,v 1.32 2001/09/23 16:50:01 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  4.0
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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

/* Authors:
 *    David Bucciarelli
 *    Brian Paul
 *    Daryll Strauss
 *    Keith Whitwell
 */


/* fxapi.c - public interface to FX/Mesa functions (fxmesa.h) */


#ifdef HAVE_CONFIG_H
#include "conf.h"
#endif

#if defined(FX)
#include "fxdrv.h"

static fxMesaContext fxMesaCurrentCtx = NULL;

/*
 * Status of 3Dfx hardware initialization
 */

static int glbGlideInitialized = 0;
static int glb3DfxPresent = 0;
static int glbTotNumCtx = 0;

GrHwConfiguration glbHWConfig;
int glbCurrentBoard = 0;


#if defined(__WIN32__)
static int
cleangraphics(void)
{
   glbTotNumCtx = 1;
   fxMesaDestroyContext(fxMesaCurrentCtx);

   return 0;
}
#elif defined(__linux__)
static void
cleangraphics(void)
{
   glbTotNumCtx = 1;
   fxMesaDestroyContext(fxMesaCurrentCtx);
}

static void
cleangraphics_handler(int s)
{
   fprintf(stderr, "fxmesa: Received a not handled signal %d\n", s);

   cleangraphics();
/*    abort(); */
   exit(1);
}
#endif


/*
 * Select the Voodoo board to use when creating
 * a new context.
 */
GLboolean GLAPIENTRY
fxMesaSelectCurrentBoard(int n)
{
   fxQueryHardware();

   if ((n < 0) || (n >= glbHWConfig.num_sst))
      return GL_FALSE;

   glbCurrentBoard = n;

   return GL_TRUE;
}


fxMesaContext GLAPIENTRY
fxMesaGetCurrentContext(void)
{
   return fxMesaCurrentCtx;
}


/*
 * The 3Dfx Global Palette extension for GLQuake.
 * More a trick than a real extesion, use the shared global
 * palette extension. 
 */
extern void GLAPIENTRY gl3DfxSetPaletteEXT(GLuint * pal);	/* silence warning */
void GLAPIENTRY
gl3DfxSetPaletteEXT(GLuint * pal)
{
   fxMesaContext fxMesa = fxMesaCurrentCtx;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      int i;

      fprintf(stderr, "fxmesa: gl3DfxSetPaletteEXT()\n");

      for (i = 0; i < 256; i++)
	 fprintf(stderr, "%x\n", pal[i]);
   }

   if (fxMesa) {
      fxMesa->haveGlobalPaletteTexture = 1;

      FX_grTexDownloadTable(GR_TMU0, GR_TEXTABLE_PALETTE,
			    (GuTexPalette *) pal);
      if (fxMesa->haveTwoTMUs)
	 FX_grTexDownloadTable(GR_TMU1, GR_TEXTABLE_PALETTE,
			       (GuTexPalette *) pal);
   }
}


static GrScreenResolution_t
fxBestResolution(int width, int height, int aux)
{
   static int resolutions[][5] = {
      {320, 200, GR_RESOLUTION_320x200, 2, 2},
      {320, 240, GR_RESOLUTION_320x240, 2, 2},
      {512, 384, GR_RESOLUTION_512x384, 2, 2},
      {640, 400, GR_RESOLUTION_640x400, 2, 2},
      {640, 480, GR_RESOLUTION_640x480, 2, 2},
      {800, 600, GR_RESOLUTION_800x600, 4, 2},
      {960, 720, GR_RESOLUTION_960x720, 6, 4}
#ifdef GR_RESOLUTION_1024x768
      , {1024, 768, GR_RESOLUTION_1024x768, 8, 4}
#endif
#ifdef GR_RESOLUTION_1280x1024
      , {1280, 1024, GR_RESOLUTION_1280x1024, 8, 8}
#endif
#ifdef GR_RESOLUTION_1600x1200
      , {1600, 1200, GR_RESOLUTION_1600x1200, 16, 8}
#endif
   };
   int NUM_RESOLUTIONS = sizeof(resolutions) / (sizeof(int) * 5);
   int i, fbmem;
   GrScreenResolution_t lastvalidres = resolutions[4][2];

   fxQueryHardware();

   if (glbHWConfig.SSTs[glbCurrentBoard].type == GR_SSTTYPE_VOODOO) {
      fbmem = glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.fbRam;

      if (glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.sliDetect)
	 fbmem *= 2;
   }
   else if (glbHWConfig.SSTs[glbCurrentBoard].type == GR_SSTTYPE_SST96)
      fbmem = glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config.fbRam;
   else
      fbmem = 2;

   /* A work around for BZFlag */

   if ((width == 1) && (height == 1)) {
      width = 640;
      height = 480;
   }

   for (i = 0; i < NUM_RESOLUTIONS; i++)
      if (resolutions[i][4 - aux] <= fbmem) {
	 if ((width <= resolutions[i][0]) && (height <= resolutions[i][1]))
	    return resolutions[i][2];

	 lastvalidres = resolutions[i][2];
      }

   return lastvalidres;
}


fxMesaContext GLAPIENTRY
fxMesaCreateBestContext(GLuint win, GLint width, GLint height,
			const GLint attribList[])
{
   GrScreenRefresh_t refresh;
   int i;
   int res, aux;
   refresh = GR_REFRESH_75Hz;

   if (getenv("SST_SCREENREFRESH")) {
      if (!strcmp(getenv("SST_SCREENREFRESH"), "60"))
	 refresh = GR_REFRESH_60Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "70"))
	 refresh = GR_REFRESH_70Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "72"))
	 refresh = GR_REFRESH_72Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "75"))
	 refresh = GR_REFRESH_75Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "80"))
	 refresh = GR_REFRESH_80Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "85"))
	 refresh = GR_REFRESH_85Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "90"))
	 refresh = GR_REFRESH_90Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "100"))
	 refresh = GR_REFRESH_100Hz;
      if (!strcmp(getenv("SST_SCREENREFRESH"), "120"))
	 refresh = GR_REFRESH_120Hz;
   }

   aux = 0;
   for (i = 0; attribList[i] != FXMESA_NONE; i++)
      if ((attribList[i] == FXMESA_ALPHA_SIZE) ||
	  (attribList[i] == FXMESA_DEPTH_SIZE)) {
	 if (attribList[++i] > 0) {
	    aux = 1;
	    break;
	 }
      }

   res = fxBestResolution(width, height, aux);

   return fxMesaCreateContext(win, res, refresh, attribList);
}


#if 0
void
fxsignals()
{
   signal(SIGINT, SIG_IGN);
   signal(SIGHUP, SIG_IGN);
   signal(SIGPIPE, SIG_IGN);
   signal(SIGFPE, SIG_IGN);
   signal(SIGBUS, SIG_IGN);
   signal(SIGILL, SIG_IGN);
   signal(SIGSEGV, SIG_IGN);
   signal(SIGTERM, SIG_IGN);
}
#endif

/*
 * Create a new FX/Mesa context and return a handle to it.
 */
fxMesaContext GLAPIENTRY
fxMesaCreateContext(GLuint win,
		    GrScreenResolution_t res,
		    GrScreenRefresh_t ref, const GLint attribList[])
{
   fxMesaContext fxMesa = NULL;
   int i, type;
   int aux;
   GLboolean doubleBuffer = GL_FALSE;
   GLboolean alphaBuffer = GL_FALSE;
   GLboolean verbose = GL_FALSE;
   GLint depthSize = 0;
   GLint stencilSize = 0;
   GLint accumSize = 0;
   GLcontext *shareCtx = NULL;
   GLcontext *ctx = 0;
   /*FX_GrContext_t glideContext = 0; */
   char *errorstr;
   GLboolean useBGR;
   char *system = NULL;

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxMesaCreateContext() Start\n");
   }

   if (getenv("MESA_FX_INFO"))
      verbose = GL_TRUE;

   aux = 0;
   i = 0;
   while (attribList[i] != FXMESA_NONE) {
      switch (attribList[i]) {
      case FXMESA_DOUBLEBUFFER:
	 doubleBuffer = GL_TRUE;
	 break;
      case FXMESA_ALPHA_SIZE:
	 i++;
	 alphaBuffer = attribList[i] > 0;
	 if (alphaBuffer)
	    aux = 1;
	 break;
      case FXMESA_DEPTH_SIZE:
	 i++;
	 depthSize = attribList[i];
	 if (depthSize) {
	    aux = 1;
	    depthSize = 16;
	 }
	 break;
      case FXMESA_STENCIL_SIZE:
	 i++;
	 stencilSize = attribList[i];
	 break;
      case FXMESA_ACCUM_SIZE:
	 i++;
	 accumSize = attribList[i];
	 break;
	 /* XXX ugly hack here for sharing display lists */
#define FXMESA_SHARE_CONTEXT 990099	/* keep in sync with xmesa1.c! */
      case FXMESA_SHARE_CONTEXT:
	 i++;
	 {
	    const void *vPtr = &attribList[i];
	    GLcontext **ctx = (GLcontext **) vPtr;
	    shareCtx = *ctx;
	 }
	 break;
      default:
	 if (MESA_VERBOSE & VERBOSE_DRIVER) {
	    fprintf(stderr, "fxmesa: fxMesaCreateContext() End (defualt)\n");
	 }
	 return NULL;
      }
      i++;
   }

   /* A workaround for Linux GLQuake */
   if (depthSize && alphaBuffer)
      alphaBuffer = 0;

   if ((type = fxQueryHardware()) < 0) {
      fprintf(stderr,
	      "fx Driver: ERROR no Voodoo1/2 Graphics or Voodoo Rush !\n");
      return NULL;
   }

   if (type == GR_SSTTYPE_VOODOO)
      win = 0;

   grSstSelect(glbCurrentBoard);

   fxMesa = (fxMesaContext) calloc(1, sizeof(struct tfxMesaContext));
   if (!fxMesa) {
      errorstr = "malloc";
      goto errorhandler;
   }

   if (glbHWConfig.SSTs[glbCurrentBoard].type == GR_SSTTYPE_VOODOO)
      fxMesa->haveTwoTMUs =
	 (glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig.nTexelfx >
	  1);
   else if (glbHWConfig.SSTs[glbCurrentBoard].type == GR_SSTTYPE_SST96)
      fxMesa->haveTwoTMUs =
	 (glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config.nTexelfx >
	  1);
   else
      fxMesa->haveTwoTMUs = GL_FALSE;

   fxMesa->haveDoubleBuffer = doubleBuffer;
   fxMesa->haveAlphaBuffer = alphaBuffer;
   fxMesa->haveGlobalPaletteTexture = GL_FALSE;
   fxMesa->haveZBuffer = depthSize ? 1 : 0;
   fxMesa->verbose = verbose;
   fxMesa->board = glbCurrentBoard;


   fxMesa->glideContext = FX_grSstWinOpen((FxU32) win, res, ref,
#ifdef  FXMESA_USE_ARGB
					  GR_COLORFORMAT_ARGB,
#else
					  GR_COLORFORMAT_ABGR,
#endif
					  GR_ORIGIN_LOWER_LEFT, 2, aux);
   if (!fxMesa->glideContext) {
      errorstr = "grSstWinOpen";
      goto errorhandler;
   }

   /*
    * Pixel tables are used during pixel read-back
    * Either initialize them for RGB or BGR order.
    * Also determine if we need vertex snapping.
    */

   fxMesa->snapVertices = GL_TRUE; /* play it safe */

#ifdef FXMESA_USE_ARGB
   useBGR = GL_FALSE;		/* Force RGB pixel order */
   system = "FXMESA_USE_ARGB";
#else
   if (glbHWConfig.SSTs[glbCurrentBoard].type == GR_SSTTYPE_VOODOO) {
      /* jk991130 - Voodoo 3s don't use BGR. Query the # of TMUs
       * as Voodoo3s have 2 TMUs on board, Banshee has only 1
       * bk000413 - another suggestion from Joseph Kain is using
       *  VendorID 0x121a for all 3dfx boards
       *   DeviceID VG  1/V2  2/VB  3/V3  5
       * For now we cehck for known BGR devices, and presume
       *  everything else to be a V3/RGB.
       */
      GrVoodooConfig_t *voodoo;
      voodoo = &glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig;

      if (voodoo->nTexelfx == 1) {
	 /* Voodoo1 or Banshee */
	 useBGR = GL_TRUE;
	 system = "Voodoo1";
      }
      else if (voodoo->nTexelfx == 2 &&
	       voodoo->fbiRev == 260 &&
	       voodoo->tmuConfig[0].tmuRev == 4 &&
	       (voodoo->tmuConfig[0].tmuRam == 2 ||
		voodoo->tmuConfig[0].tmuRam == 4)) {
	 /* Voodoo 2 */
	 useBGR = GL_TRUE;
	 system = "Voodoo2";
         fxMesa->snapVertices = GL_FALSE;
      }
      else if (voodoo->nTexelfx == 2 &&
	       voodoo->fbiRev == 2 &&
	       voodoo->tmuConfig[0].tmuRev == 1 &&
	       voodoo->tmuConfig[0].tmuRam == 4) {
	 /* Quantum3D Obsidian 50/100 */
	 useBGR = GL_TRUE;
	 system = "Quantum3D Obsidian";
      }
      else
	 /* Brian
	  *       (voodoo->nTexelfx == 2 &&
	  *        voodoo->fbiRev == 0 &&
	  *        voodoo->tmuConfig[0].tmuRev == 148441048 &&
	  *        voodoo->tmuConfig[0].tmuRam == 3)
	  * Bernd 
	  *       (voodoo->nTexelfx == 2 &&
	  *        voodoo->fbiRev ==  69634 &&
	  *        voodoo->tmuConfig[0].tmuRev == 69634 &&
	  *        voodoo->tmuConfig[0].tmuRam == 2 )
	  */
      {
	 /* Presumed Voodoo3 */
	 useBGR = GL_FALSE;
	 system = "Voodoo3";
         fxMesa->snapVertices = GL_FALSE;
      }
      if (verbose) {
	 fprintf(stderr,
           "Voodoo: Texelfx: %d / FBI Rev.: %d / TMU Rev.: %d / TMU RAM: %d\n",
                 voodoo->nTexelfx, voodoo->fbiRev, voodoo->tmuConfig[0].tmuRev,
                 voodoo->tmuConfig[0].tmuRam);
      }
   }
   else {
      useBGR = GL_FALSE;	/* use RGB pixel order otherwise */
      system = "non-voodoo";
      fxMesa->snapVertices = GL_FALSE;
   }
#endif /*FXMESA_USE_ARGB */

   if (verbose) {
      fprintf(stderr, "Voodoo pixel order: %s (%s)\n",
              useBGR ? "BGR" : "RGB", system);
      fprintf(stderr, "Vertex snapping: %d\n", fxMesa->snapVertices);
   }

   fxInitPixelTables(fxMesa, useBGR);

   fxMesa->width = FX_grSstScreenWidth();
   fxMesa->height = FX_grSstScreenHeight();

   fxMesa->clipMinX = 0;
   fxMesa->clipMaxX = fxMesa->width;
   fxMesa->clipMinY = 0;
   fxMesa->clipMaxY = fxMesa->height;

   fxMesa->screen_width = fxMesa->width;
   fxMesa->screen_height = fxMesa->height;

   fxMesa->new_state = ~0;

   if (verbose)
      fprintf(stderr, "Voodoo Glide screen size: %dx%d\n",
	      (int) FX_grSstScreenWidth(), (int) FX_grSstScreenHeight());

   fxMesa->glVis = _mesa_create_visual(GL_TRUE,	/* RGB mode */
				       doubleBuffer, GL_FALSE,	/* stereo */
				       5, 6, 5, 0,	/* RGBA bits */
				       0,	/* index bits */
				       depthSize,	/* depth_size */
				       stencilSize,	/* stencil_size */
				       accumSize, accumSize, accumSize,
				       accumSize, 1);
   if (!fxMesa->glVis) {
      errorstr = "_mesa_create_visual";
      goto errorhandler;
   }

   ctx = fxMesa->glCtx = _mesa_create_context(fxMesa->glVis, shareCtx,	/* share list context */
					      (void *) fxMesa, GL_TRUE);
   if (!ctx) {
      errorstr = "_mesa_create_context";
      goto errorhandler;
   }


   if (!fxDDInitFxMesaContext(fxMesa)) {
      errorstr = "fxDDInitFxMesaContext failed";
      goto errorhandler;
   }


   fxMesa->glBuffer = _mesa_create_framebuffer(fxMesa->glVis, GL_FALSE,	/* no software depth */
					       fxMesa->glVis->stencilBits > 0,
					       fxMesa->glVis->accumRedBits >
					       0,
					       fxMesa->glVis->alphaBits > 0);
   if (!fxMesa->glBuffer) {
      errorstr = "_mesa_create_framebuffer";
      goto errorhandler;
   }

   glbTotNumCtx++;

   /* install signal handlers */
#if defined(__linux__)
   /* Only install if environment var. is not set. */
   if (fxMesa->glCtx->CatchSignals && !getenv("MESA_FX_NO_SIGNALS")) {
      signal(SIGINT, cleangraphics_handler);
      signal(SIGHUP, cleangraphics_handler);
      signal(SIGPIPE, cleangraphics_handler);
      signal(SIGFPE, cleangraphics_handler);
      signal(SIGBUS, cleangraphics_handler);
      signal(SIGILL, cleangraphics_handler);
      signal(SIGSEGV, cleangraphics_handler);
      signal(SIGTERM, cleangraphics_handler);
   }
#endif

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxMesaCreateContext() End\n");
   }

   return fxMesa;

 errorhandler:
   if (fxMesa) {
      if (fxMesa->glideContext)
	 FX_grSstWinClose(fxMesa->glideContext);
      fxMesa->glideContext = 0;

      if (fxMesa->state)
	 free(fxMesa->state);
      if (fxMesa->fogTable)
	 free(fxMesa->fogTable);
      if (fxMesa->glBuffer)
	 _mesa_destroy_framebuffer(fxMesa->glBuffer);
      if (fxMesa->glVis)
	 _mesa_destroy_visual(fxMesa->glVis);
      if (fxMesa->glCtx)
	 _mesa_destroy_context(fxMesa->glCtx);
      free(fxMesa);
   }

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxMesaCreateContext() End (%s)\n", errorstr);
   }
   return NULL;
}


/*
 * Function to set the new window size in the context (mainly for the Voodoo Rush)
 */
void GLAPIENTRY
fxMesaUpdateScreenSize(fxMesaContext fxMesa)
{
   fxMesa->width = FX_grSstScreenWidth();
   fxMesa->height = FX_grSstScreenHeight();
}


/*
 * Destroy the given FX/Mesa context.
 */
void GLAPIENTRY
fxMesaDestroyContext(fxMesaContext fxMesa)
{
   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxMesaDestroyContext()\n");
   }

   if (!fxMesa)
      return;

   if (fxMesa->verbose) {
      fprintf(stderr, "Misc Stats:\n");
      fprintf(stderr, "  # swap buffer: %u\n", fxMesa->stats.swapBuffer);

      if (!fxMesa->stats.swapBuffer)
	 fxMesa->stats.swapBuffer = 1;

      fprintf(stderr, "Textures Stats:\n");
      fprintf(stderr, "  Free texture memory on TMU0: %d:\n",
	      fxMesa->freeTexMem[FX_TMU0]);
      if (fxMesa->haveTwoTMUs)
	 fprintf(stderr, "  Free texture memory on TMU1: %d:\n",
		 fxMesa->freeTexMem[FX_TMU1]);
      fprintf(stderr, "  # request to TMM to upload a texture objects: %u\n",
	      fxMesa->stats.reqTexUpload);
      fprintf(stderr,
	      "  # request to TMM to upload a texture objects per swapbuffer: %.2f\n",
	      fxMesa->stats.reqTexUpload / (float) fxMesa->stats.swapBuffer);
      fprintf(stderr, "  # texture objects uploaded: %u\n",
	      fxMesa->stats.texUpload);
      fprintf(stderr, "  # texture objects uploaded per swapbuffer: %.2f\n",
	      fxMesa->stats.texUpload / (float) fxMesa->stats.swapBuffer);
      fprintf(stderr, "  # MBs uploaded to texture memory: %.2f\n",
	      fxMesa->stats.memTexUpload / (float) (1 << 20));
      fprintf(stderr,
	      "  # MBs uploaded to texture memory per swapbuffer: %.2f\n",
	      (fxMesa->stats.memTexUpload /
	       (float) fxMesa->stats.swapBuffer) / (float) (1 << 20));
   }

   glbTotNumCtx--;

   fxDDDestroyFxMesaContext(fxMesa);
   _mesa_destroy_visual(fxMesa->glVis);
   _mesa_destroy_context(fxMesa->glCtx);
   _mesa_destroy_framebuffer(fxMesa->glBuffer);

   fxCloseHardware();
   FX_grSstWinClose(fxMesa->glideContext);

   free(fxMesa);

   if (fxMesa == fxMesaCurrentCtx)
      fxMesaCurrentCtx = NULL;
}


/*
 * Make the specified FX/Mesa context the current one.
 */
void GLAPIENTRY
fxMesaMakeCurrent(fxMesaContext fxMesa)
{
   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxMesaMakeCurrent(...) Start\n");
   }

   if (!fxMesa) {
      _mesa_make_current(NULL, NULL);
      fxMesaCurrentCtx = NULL;

      if (MESA_VERBOSE & VERBOSE_DRIVER) {
	 fprintf(stderr, "fxmesa: fxMesaMakeCurrent(NULL) End\n");
      }

      return;
   }

   /* if this context is already the current one, we can return early */
   if (fxMesaCurrentCtx == fxMesa
       && fxMesaCurrentCtx->glCtx == _mesa_get_current_context()) {
      if (MESA_VERBOSE & VERBOSE_DRIVER) {
	 fprintf(stderr,
		 "fxmesa: fxMesaMakeCurrent(fxMesaCurrentCtx==fxMesa) End\n");
      }

      return;
   }

   if (fxMesaCurrentCtx)
      grGlideGetState((GrState *) fxMesaCurrentCtx->state);

   fxMesaCurrentCtx = fxMesa;

   grSstSelect(fxMesa->board);
   grGlideSetState((GrState *) fxMesa->state);

   _mesa_make_current(fxMesa->glCtx, fxMesa->glBuffer);

   fxSetupDDPointers(fxMesa->glCtx);

   /* The first time we call MakeCurrent we set the initial viewport size */
   if (fxMesa->glCtx->Viewport.Width == 0)
      _mesa_set_viewport(fxMesa->glCtx, 0, 0, fxMesa->width, fxMesa->height);

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxMesaMakeCurrent(...) End\n");
   }
}


#if 0
static void
QueryCounters(void)
{
   static GLuint prevPassed = 0;
   static GLuint prevFailed = 0;
   GLuint failed, passed;
   GrSstPerfStats_t st;

   FX_grSstPerfStats(&st);
   failed = st.zFuncFail - st.aFuncFail - st.chromaFail;
   passed = st.pixelsIn - failed;
   printf("failed: %d  passed: %d\n", failed - prevFailed,
	  passed - prevPassed);

   prevPassed = passed;
   prevFailed = failed;
}
#endif


/*
 * Swap front/back buffers for current context if double buffered.
 */
void GLAPIENTRY
fxMesaSwapBuffers(void)
{
   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr,
	      "fxmesa: ------------------------------- fxMesaSwapBuffers() -------------------------------\n");
   }

   if (fxMesaCurrentCtx) {
      _mesa_swapbuffers(fxMesaCurrentCtx->glCtx);

      if (fxMesaCurrentCtx->haveDoubleBuffer) {

	 grBufferSwap(fxMesaCurrentCtx->swapInterval);

	 /*
	  * Don't allow swap buffer commands to build up!
	  */
	 while (FX_grGetInteger(FX_PENDING_BUFFERSWAPS) >
		fxMesaCurrentCtx->maxPendingSwapBuffers)
	    /* The driver is able to sleep when waiting for the completation
	       of multiple swapbuffer operations instead of wasting
	       CPU time (NOTE: you must uncomment the following line in the
	       in order to enable this option) */
	    /* usleep(10000); */
	    ;

	 fxMesaCurrentCtx->stats.swapBuffer++;
      }
   }
}


/*
 * Query 3Dfx hardware presence/kind
 */
int GLAPIENTRY
fxQueryHardware(void)
{
   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxQueryHardware() Start\n");
   }

   if (!glbGlideInitialized) {
      grGlideInit();
      if (FX_grSstQueryHardware(&glbHWConfig)) {
	 grSstSelect(glbCurrentBoard);
	 glb3DfxPresent = 1;

	 if (getenv("MESA_FX_INFO")) {
	    char buf[80];

	    FX_grGlideGetVersion(buf);
	    fprintf(stderr, "Voodoo Using Glide V%s\n", buf);
	    fprintf(stderr, "Voodoo Number of boards: %d\n",
		    glbHWConfig.num_sst);

	    if (glbHWConfig.SSTs[glbCurrentBoard].type == GR_SSTTYPE_VOODOO) {
	       GrVoodooConfig_t *voodoo;
	       voodoo =
		  &glbHWConfig.SSTs[glbCurrentBoard].sstBoard.VoodooConfig;

	       fprintf(stderr, "Voodoo Framebuffer RAM: %d\n",
		       voodoo->sliDetect ? (voodoo->fbRam *
					    2) : voodoo->fbRam);
	       fprintf(stderr, "Voodoo Number of TMUs: %d\n",
		       voodoo->nTexelfx);
	       fprintf(stderr, "Voodoo fbRam: %d\n", voodoo->fbRam);
	       fprintf(stderr, "Voodoo fbiRev: %d\n", voodoo->fbiRev);

	       fprintf(stderr, "Voodoo SLI detected: %d\n",
		       voodoo->sliDetect);
	    }
	    else if (glbHWConfig.SSTs[glbCurrentBoard].type ==
		     GR_SSTTYPE_SST96) {
	       GrSst96Config_t *sst96;
	       sst96 =
		  &glbHWConfig.SSTs[glbCurrentBoard].sstBoard.SST96Config;
	       fprintf(stderr, "Voodoo Framebuffer RAM: %d\n", sst96->fbRam);
	       fprintf(stderr, "Voodoo Number of TMUs: %d\n",
		       sst96->nTexelfx);
	    }

	 }
      }
      else {
	 glb3DfxPresent = 0;
      }

      glbGlideInitialized = 1;

#if defined(__WIN32__)
      onexit((_onexit_t) cleangraphics);
#elif defined(__linux__)
      /* Only register handler if environment variable is not defined. */
      if (!getenv("MESA_FX_NO_SIGNALS")) {
	 atexit(cleangraphics);
      }
#endif
   }

   if (MESA_VERBOSE & VERBOSE_DRIVER) {
      fprintf(stderr, "fxmesa: fxQueryHardware() End (voodooo)\n");
   }

   return glbHWConfig.SSTs[glbCurrentBoard].type;
}


/*
 * Shutdown Glide library
 */
void GLAPIENTRY
fxCloseHardware(void)
{
   if (glbGlideInitialized) {
      if (getenv("MESA_FX_INFO")) {
	 GrSstPerfStats_t st;

	 FX_grSstPerfStats(&st);
	 fprintf(stderr, "Pixels Stats:\n");
	 fprintf(stderr, "  # pixels processed (minus buffer clears): %u\n",
		 (unsigned) st.pixelsIn);
	 fprintf(stderr,
		 "  # pixels not drawn due to chroma key test failure: %u\n",
		 (unsigned) st.chromaFail);
	 fprintf(stderr,
		 "  # pixels not drawn due to depth test failure: %u\n",
		 (unsigned) st.zFuncFail);
	 fprintf(stderr,
		 "  # pixels not drawn due to alpha test failure: %u\n",
		 (unsigned) st.aFuncFail);
	 fprintf(stderr,
		 "  # pixels drawn (including buffer clears and LFB writes): %u\n",
		 (unsigned) st.pixelsOut);
      }

      if (glbTotNumCtx == 0) {
	 grGlideShutdown();
	 glbGlideInitialized = 0;
      }
   }
}


#else


/*
 * Need this to provide at least one external definition.
 */
extern int gl_fx_dummy_function_api(void);
int
gl_fx_dummy_function_api(void)
{
   return 0;
}

#endif /* FX */
