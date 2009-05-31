/**************************************************************************
 *
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/

#include <windows.h>
#include <stdio.h>

#include "GL/gl.h"

#include "util/u_debug.h"
#include "pipe/p_thread.h"

#include "shared/stw_public.h"
#include "icd/stw_icd.h"

#define DBG 0


BOOL APIENTRY
DrvCopyContext(
   DHGLRC dhrcSource,
   DHGLRC dhrcDest,
   UINT fuMask )
{
   return stw_copy_context(dhrcSource, dhrcDest, fuMask);
}


DHGLRC APIENTRY
DrvCreateLayerContext(
   HDC hdc,
   INT iLayerPlane )
{
   DHGLRC r;
   
   r = stw_create_layer_context( hdc, iLayerPlane );
   
   if (DBG)
      debug_printf( "%s( %p, %i ) = %u\n",
                    __FUNCTION__, hdc, iLayerPlane, r );
   
   return r;
}

DHGLRC APIENTRY
DrvCreateContext(
   HDC hdc )
{
   return DrvCreateLayerContext( hdc, 0 );
}

BOOL APIENTRY
DrvDeleteContext(
   DHGLRC dhglrc )
{
   BOOL r;
   
   r = stw_delete_context( dhglrc );
   
   if (DBG)
      debug_printf( "%s( %u ) = %u\n",
                    __FUNCTION__, dhglrc, r );
   
   return r;
}

BOOL APIENTRY
DrvDescribeLayerPlane(
   HDC hdc,
   INT iPixelFormat,
   INT iLayerPlane,
   UINT nBytes,
   LPLAYERPLANEDESCRIPTOR plpd )
{
   if (DBG) 
      debug_printf( "%s\n", __FUNCTION__ );

   return FALSE;
}

LONG APIENTRY
DrvDescribePixelFormat(
   HDC hdc,
   INT iPixelFormat,
   ULONG cjpfd,
   PIXELFORMATDESCRIPTOR *ppfd )
{
   LONG r;

   r = stw_pixelformat_describe( hdc, iPixelFormat, cjpfd, ppfd );

   if (DBG)
      debug_printf( "%s( %p, %d, %u, %p ) = %d\n",
                    __FUNCTION__, hdc, iPixelFormat, cjpfd, ppfd, r );

   return r;
}

int APIENTRY
DrvGetLayerPaletteEntries(
   HDC hdc,
   INT iLayerPlane,
   INT iStart,
   INT cEntries,
   COLORREF *pcr )
{
   if (DBG)
      debug_printf( "%s\n", __FUNCTION__ );

   return 0;
}

PROC APIENTRY
DrvGetProcAddress(
   LPCSTR lpszProc )
{
   PROC r;

   r = stw_get_proc_address( lpszProc );

   if (DBG)
      debug_printf( "%s( \"%s\" ) = %p\n", __FUNCTION__, lpszProc, r );

   return r;
}

BOOL APIENTRY
DrvRealizeLayerPalette(
   HDC hdc,
   INT iLayerPlane,
   BOOL bRealize )
{
   if (DBG)
      debug_printf( "%s\n", __FUNCTION__ );

   return FALSE;
}

BOOL APIENTRY
DrvReleaseContext(
   DHGLRC dhglrc )
{
   return stw_release_context(dhglrc);
}

void APIENTRY
DrvSetCallbackProcs(
   INT nProcs,
   PROC *pProcs )
{
   if (DBG)
      debug_printf( "%s( %d, %p )\n", __FUNCTION__, nProcs, pProcs );

   return;
}


/**
 * Although WGL allows different dispatch entrypoints per context 
 */
static const GLCLTPROCTABLE cpt =
{
   OPENGL_VERSION_110_ENTRIES,
   {
      &glNewList,
      &glEndList,
      &glCallList,
      &glCallLists,
      &glDeleteLists,
      &glGenLists,
      &glListBase,
      &glBegin,
      &glBitmap,
      &glColor3b,
      &glColor3bv,
      &glColor3d,
      &glColor3dv,
      &glColor3f,
      &glColor3fv,
      &glColor3i,
      &glColor3iv,
      &glColor3s,
      &glColor3sv,
      &glColor3ub,
      &glColor3ubv,
      &glColor3ui,
      &glColor3uiv,
      &glColor3us,
      &glColor3usv,
      &glColor4b,
      &glColor4bv,
      &glColor4d,
      &glColor4dv,
      &glColor4f,
      &glColor4fv,
      &glColor4i,
      &glColor4iv,
      &glColor4s,
      &glColor4sv,
      &glColor4ub,
      &glColor4ubv,
      &glColor4ui,
      &glColor4uiv,
      &glColor4us,
      &glColor4usv,
      &glEdgeFlag,
      &glEdgeFlagv,
      &glEnd,
      &glIndexd,
      &glIndexdv,
      &glIndexf,
      &glIndexfv,
      &glIndexi,
      &glIndexiv,
      &glIndexs,
      &glIndexsv,
      &glNormal3b,
      &glNormal3bv,
      &glNormal3d,
      &glNormal3dv,
      &glNormal3f,
      &glNormal3fv,
      &glNormal3i,
      &glNormal3iv,
      &glNormal3s,
      &glNormal3sv,
      &glRasterPos2d,
      &glRasterPos2dv,
      &glRasterPos2f,
      &glRasterPos2fv,
      &glRasterPos2i,
      &glRasterPos2iv,
      &glRasterPos2s,
      &glRasterPos2sv,
      &glRasterPos3d,
      &glRasterPos3dv,
      &glRasterPos3f,
      &glRasterPos3fv,
      &glRasterPos3i,
      &glRasterPos3iv,
      &glRasterPos3s,
      &glRasterPos3sv,
      &glRasterPos4d,
      &glRasterPos4dv,
      &glRasterPos4f,
      &glRasterPos4fv,
      &glRasterPos4i,
      &glRasterPos4iv,
      &glRasterPos4s,
      &glRasterPos4sv,
      &glRectd,
      &glRectdv,
      &glRectf,
      &glRectfv,
      &glRecti,
      &glRectiv,
      &glRects,
      &glRectsv,
      &glTexCoord1d,
      &glTexCoord1dv,
      &glTexCoord1f,
      &glTexCoord1fv,
      &glTexCoord1i,
      &glTexCoord1iv,
      &glTexCoord1s,
      &glTexCoord1sv,
      &glTexCoord2d,
      &glTexCoord2dv,
      &glTexCoord2f,
      &glTexCoord2fv,
      &glTexCoord2i,
      &glTexCoord2iv,
      &glTexCoord2s,
      &glTexCoord2sv,
      &glTexCoord3d,
      &glTexCoord3dv,
      &glTexCoord3f,
      &glTexCoord3fv,
      &glTexCoord3i,
      &glTexCoord3iv,
      &glTexCoord3s,
      &glTexCoord3sv,
      &glTexCoord4d,
      &glTexCoord4dv,
      &glTexCoord4f,
      &glTexCoord4fv,
      &glTexCoord4i,
      &glTexCoord4iv,
      &glTexCoord4s,
      &glTexCoord4sv,
      &glVertex2d,
      &glVertex2dv,
      &glVertex2f,
      &glVertex2fv,
      &glVertex2i,
      &glVertex2iv,
      &glVertex2s,
      &glVertex2sv,
      &glVertex3d,
      &glVertex3dv,
      &glVertex3f,
      &glVertex3fv,
      &glVertex3i,
      &glVertex3iv,
      &glVertex3s,
      &glVertex3sv,
      &glVertex4d,
      &glVertex4dv,
      &glVertex4f,
      &glVertex4fv,
      &glVertex4i,
      &glVertex4iv,
      &glVertex4s,
      &glVertex4sv,
      &glClipPlane,
      &glColorMaterial,
      &glCullFace,
      &glFogf,
      &glFogfv,
      &glFogi,
      &glFogiv,
      &glFrontFace,
      &glHint,
      &glLightf,
      &glLightfv,
      &glLighti,
      &glLightiv,
      &glLightModelf,
      &glLightModelfv,
      &glLightModeli,
      &glLightModeliv,
      &glLineStipple,
      &glLineWidth,
      &glMaterialf,
      &glMaterialfv,
      &glMateriali,
      &glMaterialiv,
      &glPointSize,
      &glPolygonMode,
      &glPolygonStipple,
      &glScissor,
      &glShadeModel,
      &glTexParameterf,
      &glTexParameterfv,
      &glTexParameteri,
      &glTexParameteriv,
      &glTexImage1D,
      &glTexImage2D,
      &glTexEnvf,
      &glTexEnvfv,
      &glTexEnvi,
      &glTexEnviv,
      &glTexGend,
      &glTexGendv,
      &glTexGenf,
      &glTexGenfv,
      &glTexGeni,
      &glTexGeniv,
      &glFeedbackBuffer,
      &glSelectBuffer,
      &glRenderMode,
      &glInitNames,
      &glLoadName,
      &glPassThrough,
      &glPopName,
      &glPushName,
      &glDrawBuffer,
      &glClear,
      &glClearAccum,
      &glClearIndex,
      &glClearColor,
      &glClearStencil,
      &glClearDepth,
      &glStencilMask,
      &glColorMask,
      &glDepthMask,
      &glIndexMask,
      &glAccum,
      &glDisable,
      &glEnable,
      &glFinish,
      &glFlush,
      &glPopAttrib,
      &glPushAttrib,
      &glMap1d,
      &glMap1f,
      &glMap2d,
      &glMap2f,
      &glMapGrid1d,
      &glMapGrid1f,
      &glMapGrid2d,
      &glMapGrid2f,
      &glEvalCoord1d,
      &glEvalCoord1dv,
      &glEvalCoord1f,
      &glEvalCoord1fv,
      &glEvalCoord2d,
      &glEvalCoord2dv,
      &glEvalCoord2f,
      &glEvalCoord2fv,
      &glEvalMesh1,
      &glEvalPoint1,
      &glEvalMesh2,
      &glEvalPoint2,
      &glAlphaFunc,
      &glBlendFunc,
      &glLogicOp,
      &glStencilFunc,
      &glStencilOp,
      &glDepthFunc,
      &glPixelZoom,
      &glPixelTransferf,
      &glPixelTransferi,
      &glPixelStoref,
      &glPixelStorei,
      &glPixelMapfv,
      &glPixelMapuiv,
      &glPixelMapusv,
      &glReadBuffer,
      &glCopyPixels,
      &glReadPixels,
      &glDrawPixels,
      &glGetBooleanv,
      &glGetClipPlane,
      &glGetDoublev,
      &glGetError,
      &glGetFloatv,
      &glGetIntegerv,
      &glGetLightfv,
      &glGetLightiv,
      &glGetMapdv,
      &glGetMapfv,
      &glGetMapiv,
      &glGetMaterialfv,
      &glGetMaterialiv,
      &glGetPixelMapfv,
      &glGetPixelMapuiv,
      &glGetPixelMapusv,
      &glGetPolygonStipple,
      &glGetString,
      &glGetTexEnvfv,
      &glGetTexEnviv,
      &glGetTexGendv,
      &glGetTexGenfv,
      &glGetTexGeniv,
      &glGetTexImage,
      &glGetTexParameterfv,
      &glGetTexParameteriv,
      &glGetTexLevelParameterfv,
      &glGetTexLevelParameteriv,
      &glIsEnabled,
      &glIsList,
      &glDepthRange,
      &glFrustum,
      &glLoadIdentity,
      &glLoadMatrixf,
      &glLoadMatrixd,
      &glMatrixMode,
      &glMultMatrixf,
      &glMultMatrixd,
      &glOrtho,
      &glPopMatrix,
      &glPushMatrix,
      &glRotated,
      &glRotatef,
      &glScaled,
      &glScalef,
      &glTranslated,
      &glTranslatef,
      &glViewport,
      &glArrayElement,
      &glBindTexture,
      &glColorPointer,
      &glDisableClientState,
      &glDrawArrays,
      &glDrawElements,
      &glEdgeFlagPointer,
      &glEnableClientState,
      &glIndexPointer,
      &glIndexub,
      &glIndexubv,
      &glInterleavedArrays,
      &glNormalPointer,
      &glPolygonOffset,
      &glTexCoordPointer,
      &glVertexPointer,
      &glAreTexturesResident,
      &glCopyTexImage1D,
      &glCopyTexImage2D,
      &glCopyTexSubImage1D,
      &glCopyTexSubImage2D,
      &glDeleteTextures,
      &glGenTextures,
      &glGetPointerv,
      &glIsTexture,
      &glPrioritizeTextures,
      &glTexSubImage1D,
      &glTexSubImage2D,
      &glPopClientAttrib,
      &glPushClientAttrib
   }
};


PGLCLTPROCTABLE APIENTRY
DrvSetContext(
   HDC hdc,
   DHGLRC dhglrc,
   PFN_SETPROCTABLE pfnSetProcTable )
{
   PGLCLTPROCTABLE r = (PGLCLTPROCTABLE)&cpt;
   
   if (!stw_make_current( hdc, dhglrc ))
      r = NULL;
      
   if (DBG)
      debug_printf( "%s( 0x%p, %u, 0x%p ) = %p\n", 
                    __FUNCTION__, hdc, dhglrc, pfnSetProcTable, r );

   return r;
}

int APIENTRY
DrvSetLayerPaletteEntries(
   HDC hdc,
   INT iLayerPlane,
   INT iStart,
   INT cEntries,
   CONST COLORREF *pcr )
{
   if (DBG)
      debug_printf( "%s\n", __FUNCTION__ );

   return 0;
}

BOOL APIENTRY
DrvSetPixelFormat(
   HDC hdc,
   LONG iPixelFormat )
{
   BOOL r;

   r = stw_pixelformat_set( hdc, iPixelFormat );

   if (DBG)
      debug_printf( "%s( %p, %d ) = %s\n", __FUNCTION__, hdc, iPixelFormat, r ? "TRUE" : "FALSE" );

   return r;
}

BOOL APIENTRY
DrvShareLists(
   DHGLRC dhglrc1,
   DHGLRC dhglrc2 )
{
   if (DBG)
      debug_printf( "%s\n", __FUNCTION__ );

   return stw_share_lists(dhglrc1, dhglrc2);
}

BOOL APIENTRY
DrvSwapBuffers(
   HDC hdc )
{
   if (DBG)
      debug_printf( "%s( %p )\n", __FUNCTION__, hdc );

   return stw_swap_buffers( hdc );
}

BOOL APIENTRY
DrvSwapLayerBuffers(
   HDC hdc,
   UINT fuPlanes )
{
   if (DBG)
      debug_printf( "%s\n", __FUNCTION__ );

   return stw_swap_layer_buffers( hdc, fuPlanes );
}

BOOL APIENTRY
DrvValidateVersion(
   ULONG ulVersion )
{
   if (DBG)
      debug_printf( "%s( %u )\n", __FUNCTION__, ulVersion );

   /* TODO: get the expected version from the winsys */
   
   return ulVersion == 1;
}
