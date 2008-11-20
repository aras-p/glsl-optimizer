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
#include "GL/mesa_wgl.h"

#include "pipe/p_debug.h"

#include "stw_device.h"
#include "stw_icd.h"


static HGLRC
_drv_lookup_hglrc( DHGLRC dhglrc )
{
   if (dhglrc == 0 || dhglrc >= DRV_CONTEXT_MAX)
      return NULL;
   return stw_dev->ctx_array[dhglrc - 1].hglrc;
}

BOOL APIENTRY
DrvCopyContext(
   DHGLRC dhrcSource,
   DHGLRC dhrcDest,
   UINT fuMask )
{
   debug_printf( "%s\n", __FUNCTION__ );

   return FALSE;
}

DHGLRC APIENTRY
DrvCreateLayerContext(
   HDC hdc,
   INT iLayerPlane )
{
   DHGLRC dhglrc = 0;

   if (iLayerPlane == 0) {
      DWORD i;

      for (i = 0; i < DRV_CONTEXT_MAX; i++) {
         if (stw_dev->ctx_array[i].hglrc == NULL)
            break;
      }

      if (i < DRV_CONTEXT_MAX) {
         stw_dev->ctx_array[i].hglrc = wglCreateContext( hdc );
         if (stw_dev->ctx_array[i].hglrc != NULL)
            dhglrc = i + 1;
      }
   }

   debug_printf( "%s( 0x%p, %d ) = %u\n", __FUNCTION__, hdc, iLayerPlane, dhglrc );

   return dhglrc;
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
   HGLRC hglrc = _drv_lookup_hglrc( dhglrc );
   BOOL success = FALSE;

   if (hglrc != NULL) {
      success = wglDeleteContext( hglrc );
      if (success)
         stw_dev->ctx_array[dhglrc - 1].hglrc = NULL;
   }

   debug_printf( "%s( %u ) = %s\n", __FUNCTION__, dhglrc, success ? "TRUE" : "FALSE" );

   return success;
}

BOOL APIENTRY
DrvDescribeLayerPlane(
   HDC hdc,
   INT iPixelFormat,
   INT iLayerPlane,
   UINT nBytes,
   LPLAYERPLANEDESCRIPTOR plpd )
{
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

   r = wglDescribePixelFormat( hdc, iPixelFormat, cjpfd, ppfd );

   debug_printf( "%s( 0x%p, %d, %u, 0x%p ) = %d\n", __FUNCTION__, hdc, iPixelFormat, cjpfd, ppfd, r );

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
   debug_printf( "%s\n", __FUNCTION__ );

   return 0;
}

PROC APIENTRY
DrvGetProcAddress(
   LPCSTR lpszProc )
{
   PROC r;

   r = wglGetProcAddress( lpszProc );

   debug_printf( "%s( \", __FUNCTION__%s\" ) = 0x%p\n", lpszProc, r );

   return r;
}

BOOL APIENTRY
DrvRealizeLayerPalette(
   HDC hdc,
   INT iLayerPlane,
   BOOL bRealize )
{
   debug_printf( "%s\n", __FUNCTION__ );

   return FALSE;
}

BOOL APIENTRY
DrvReleaseContext(
   DHGLRC dhglrc )
{
   BOOL success = FALSE;

   if (dhglrc == stw_dev->ctx_current) {
      HGLRC hglrc = _drv_lookup_hglrc( dhglrc );

      if (hglrc != NULL) {
         success = wglMakeCurrent( NULL, NULL );
         if (success)
            stw_dev->ctx_current = 0;
      }
   }

   debug_printf( "%s( %u ) = %s\n", __FUNCTION__, dhglrc, success ? "TRUE" : "FALSE" );

   return success;
}

void APIENTRY
DrvSetCallbackProcs(
   INT nProcs,
   PROC *pProcs )
{
   debug_printf( "%s( %d, 0x%p )\n", __FUNCTION__, nProcs, pProcs );

   return;
}

#define GPA_GL( NAME ) disp->NAME = gl##NAME

static GLCLTPROCTABLE cpt;

PGLCLTPROCTABLE APIENTRY
DrvSetContext(
   HDC hdc,
   DHGLRC dhglrc,
   PFN_SETPROCTABLE pfnSetProcTable )
{
   HGLRC hglrc = _drv_lookup_hglrc( dhglrc );
   GLDISPATCHTABLE *disp = &cpt.glDispatchTable;

   debug_printf( "%s( 0x%p, %u, 0x%p )\n", __FUNCTION__, hdc, dhglrc, pfnSetProcTable );

   if (hglrc == NULL)
      return NULL;

   if (!wglMakeCurrent( hdc, hglrc ))
      return NULL;

   memset( &cpt, 0, sizeof( cpt ) );
   cpt.cEntries = OPENGL_VERSION_110_ENTRIES;

   GPA_GL( NewList );
   GPA_GL( EndList );
   GPA_GL( CallList );
   GPA_GL( CallLists );
   GPA_GL( DeleteLists );
   GPA_GL( GenLists );
   GPA_GL( ListBase );
   GPA_GL( Begin );
   GPA_GL( Bitmap );
   GPA_GL( Color3b );
   GPA_GL( Color3bv );
   GPA_GL( Color3d );
   GPA_GL( Color3dv );
   GPA_GL( Color3f );
   GPA_GL( Color3fv );
   GPA_GL( Color3i );
   GPA_GL( Color3iv );
   GPA_GL( Color3s );
   GPA_GL( Color3sv );
   GPA_GL( Color3ub );
   GPA_GL( Color3ubv );
   GPA_GL( Color3ui );
   GPA_GL( Color3uiv );
   GPA_GL( Color3us );
   GPA_GL( Color3usv );
   GPA_GL( Color4b );
   GPA_GL( Color4bv );
   GPA_GL( Color4d );
   GPA_GL( Color4dv );
   GPA_GL( Color4f );
   GPA_GL( Color4fv );
   GPA_GL( Color4i );
   GPA_GL( Color4iv );
   GPA_GL( Color4s );
   GPA_GL( Color4sv );
   GPA_GL( Color4ub );
   GPA_GL( Color4ubv );
   GPA_GL( Color4ui );
   GPA_GL( Color4uiv );
   GPA_GL( Color4us );
   GPA_GL( Color4usv );
   GPA_GL( EdgeFlag );
   GPA_GL( EdgeFlagv );
   GPA_GL( End );
   GPA_GL( Indexd );
   GPA_GL( Indexdv );
   GPA_GL( Indexf );
   GPA_GL( Indexfv );
   GPA_GL( Indexi );
   GPA_GL( Indexiv );
   GPA_GL( Indexs );
   GPA_GL( Indexsv );
   GPA_GL( Normal3b );
   GPA_GL( Normal3bv );
   GPA_GL( Normal3d );
   GPA_GL( Normal3dv );
   GPA_GL( Normal3f );
   GPA_GL( Normal3fv );
   GPA_GL( Normal3i );
   GPA_GL( Normal3iv );
   GPA_GL( Normal3s );
   GPA_GL( Normal3sv );
   GPA_GL( RasterPos2d );
   GPA_GL( RasterPos2dv );
   GPA_GL( RasterPos2f );
   GPA_GL( RasterPos2fv );
   GPA_GL( RasterPos2i );
   GPA_GL( RasterPos2iv );
   GPA_GL( RasterPos2s );
   GPA_GL( RasterPos2sv );
   GPA_GL( RasterPos3d );
   GPA_GL( RasterPos3dv );
   GPA_GL( RasterPos3f );
   GPA_GL( RasterPos3fv );
   GPA_GL( RasterPos3i );
   GPA_GL( RasterPos3iv );
   GPA_GL( RasterPos3s );
   GPA_GL( RasterPos3sv );
   GPA_GL( RasterPos4d );
   GPA_GL( RasterPos4dv );
   GPA_GL( RasterPos4f );
   GPA_GL( RasterPos4fv );
   GPA_GL( RasterPos4i );
   GPA_GL( RasterPos4iv );
   GPA_GL( RasterPos4s );
   GPA_GL( RasterPos4sv );
   GPA_GL( Rectd );
   GPA_GL( Rectdv );
   GPA_GL( Rectf );
   GPA_GL( Rectfv );
   GPA_GL( Recti );
   GPA_GL( Rectiv );
   GPA_GL( Rects );
   GPA_GL( Rectsv );
   GPA_GL( TexCoord1d );
   GPA_GL( TexCoord1dv );
   GPA_GL( TexCoord1f );
   GPA_GL( TexCoord1fv );
   GPA_GL( TexCoord1i );
   GPA_GL( TexCoord1iv );
   GPA_GL( TexCoord1s );
   GPA_GL( TexCoord1sv );
   GPA_GL( TexCoord2d );
   GPA_GL( TexCoord2dv );
   GPA_GL( TexCoord2f );
   GPA_GL( TexCoord2fv );
   GPA_GL( TexCoord2i );
   GPA_GL( TexCoord2iv );
   GPA_GL( TexCoord2s );
   GPA_GL( TexCoord2sv );
   GPA_GL( TexCoord3d );
   GPA_GL( TexCoord3dv );
   GPA_GL( TexCoord3f );
   GPA_GL( TexCoord3fv );
   GPA_GL( TexCoord3i );
   GPA_GL( TexCoord3iv );
   GPA_GL( TexCoord3s );
   GPA_GL( TexCoord3sv );
   GPA_GL( TexCoord4d );
   GPA_GL( TexCoord4dv );
   GPA_GL( TexCoord4f );
   GPA_GL( TexCoord4fv );
   GPA_GL( TexCoord4i );
   GPA_GL( TexCoord4iv );
   GPA_GL( TexCoord4s );
   GPA_GL( TexCoord4sv );
   GPA_GL( Vertex2d );
   GPA_GL( Vertex2dv );
   GPA_GL( Vertex2f );
   GPA_GL( Vertex2fv );
   GPA_GL( Vertex2i );
   GPA_GL( Vertex2iv );
   GPA_GL( Vertex2s );
   GPA_GL( Vertex2sv );
   GPA_GL( Vertex3d );
   GPA_GL( Vertex3dv );
   GPA_GL( Vertex3f );
   GPA_GL( Vertex3fv );
   GPA_GL( Vertex3i );
   GPA_GL( Vertex3iv );
   GPA_GL( Vertex3s );
   GPA_GL( Vertex3sv );
   GPA_GL( Vertex4d );
   GPA_GL( Vertex4dv );
   GPA_GL( Vertex4f );
   GPA_GL( Vertex4fv );
   GPA_GL( Vertex4i );
   GPA_GL( Vertex4iv );
   GPA_GL( Vertex4s );
   GPA_GL( Vertex4sv );
   GPA_GL( ClipPlane );
   GPA_GL( ColorMaterial );
   GPA_GL( CullFace );
   GPA_GL( Fogf );
   GPA_GL( Fogfv );
   GPA_GL( Fogi );
   GPA_GL( Fogiv );
   GPA_GL( FrontFace );
   GPA_GL( Hint );
   GPA_GL( Lightf );
   GPA_GL( Lightfv );
   GPA_GL( Lighti );
   GPA_GL( Lightiv );
   GPA_GL( LightModelf );
   GPA_GL( LightModelfv );
   GPA_GL( LightModeli );
   GPA_GL( LightModeliv );
   GPA_GL( LineStipple );
   GPA_GL( LineWidth );
   GPA_GL( Materialf );
   GPA_GL( Materialfv );
   GPA_GL( Materiali );
   GPA_GL( Materialiv );
   GPA_GL( PointSize );
   GPA_GL( PolygonMode );
   GPA_GL( PolygonStipple );
   GPA_GL( Scissor );
   GPA_GL( ShadeModel );
   GPA_GL( TexParameterf );
   GPA_GL( TexParameterfv );
   GPA_GL( TexParameteri );
   GPA_GL( TexParameteriv );
   GPA_GL( TexImage1D );
   GPA_GL( TexImage2D );
   GPA_GL( TexEnvf );
   GPA_GL( TexEnvfv );
   GPA_GL( TexEnvi );
   GPA_GL( TexEnviv );
   GPA_GL( TexGend );
   GPA_GL( TexGendv );
   GPA_GL( TexGenf );
   GPA_GL( TexGenfv );
   GPA_GL( TexGeni );
   GPA_GL( TexGeniv );
   GPA_GL( FeedbackBuffer );
   GPA_GL( SelectBuffer );
   GPA_GL( RenderMode );
   GPA_GL( InitNames );
   GPA_GL( LoadName );
   GPA_GL( PassThrough );
   GPA_GL( PopName );
   GPA_GL( PushName );
   GPA_GL( DrawBuffer );
   GPA_GL( Clear );
   GPA_GL( ClearAccum );
   GPA_GL( ClearIndex );
   GPA_GL( ClearColor );
   GPA_GL( ClearStencil );
   GPA_GL( ClearDepth );
   GPA_GL( StencilMask );
   GPA_GL( ColorMask );
   GPA_GL( DepthMask );
   GPA_GL( IndexMask );
   GPA_GL( Accum );
   GPA_GL( Disable );
   GPA_GL( Enable );
   GPA_GL( Finish );
   GPA_GL( Flush );
   GPA_GL( PopAttrib );
   GPA_GL( PushAttrib );
   GPA_GL( Map1d );
   GPA_GL( Map1f );
   GPA_GL( Map2d );
   GPA_GL( Map2f );
   GPA_GL( MapGrid1d );
   GPA_GL( MapGrid1f );
   GPA_GL( MapGrid2d );
   GPA_GL( MapGrid2f );
   GPA_GL( EvalCoord1d );
   GPA_GL( EvalCoord1dv );
   GPA_GL( EvalCoord1f );
   GPA_GL( EvalCoord1fv );
   GPA_GL( EvalCoord2d );
   GPA_GL( EvalCoord2dv );
   GPA_GL( EvalCoord2f );
   GPA_GL( EvalCoord2fv );
   GPA_GL( EvalMesh1 );
   GPA_GL( EvalPoint1 );
   GPA_GL( EvalMesh2 );
   GPA_GL( EvalPoint2 );
   GPA_GL( AlphaFunc );
   GPA_GL( BlendFunc );
   GPA_GL( LogicOp );
   GPA_GL( StencilFunc );
   GPA_GL( StencilOp );
   GPA_GL( DepthFunc );
   GPA_GL( PixelZoom );
   GPA_GL( PixelTransferf );
   GPA_GL( PixelTransferi );
   GPA_GL( PixelStoref );
   GPA_GL( PixelStorei );
   GPA_GL( PixelMapfv );
   GPA_GL( PixelMapuiv );
   GPA_GL( PixelMapusv );
   GPA_GL( ReadBuffer );
   GPA_GL( CopyPixels );
   GPA_GL( ReadPixels );
   GPA_GL( DrawPixels );
   GPA_GL( GetBooleanv );
   GPA_GL( GetClipPlane );
   GPA_GL( GetDoublev );
   GPA_GL( GetError );
   GPA_GL( GetFloatv );
   GPA_GL( GetIntegerv );
   GPA_GL( GetLightfv );
   GPA_GL( GetLightiv );
   GPA_GL( GetMapdv );
   GPA_GL( GetMapfv );
   GPA_GL( GetMapiv );
   GPA_GL( GetMaterialfv );
   GPA_GL( GetMaterialiv );
   GPA_GL( GetPixelMapfv );
   GPA_GL( GetPixelMapuiv );
   GPA_GL( GetPixelMapusv );
   GPA_GL( GetPolygonStipple );
   GPA_GL( GetString );
   GPA_GL( GetTexEnvfv );
   GPA_GL( GetTexEnviv );
   GPA_GL( GetTexGendv );
   GPA_GL( GetTexGenfv );
   GPA_GL( GetTexGeniv );
   GPA_GL( GetTexImage );
   GPA_GL( GetTexParameterfv );
   GPA_GL( GetTexParameteriv );
   GPA_GL( GetTexLevelParameterfv );
   GPA_GL( GetTexLevelParameteriv );
   GPA_GL( IsEnabled );
   GPA_GL( IsList );
   GPA_GL( DepthRange );
   GPA_GL( Frustum );
   GPA_GL( LoadIdentity );
   GPA_GL( LoadMatrixf );
   GPA_GL( LoadMatrixd );
   GPA_GL( MatrixMode );
   GPA_GL( MultMatrixf );
   GPA_GL( MultMatrixd );
   GPA_GL( Ortho );
   GPA_GL( PopMatrix );
   GPA_GL( PushMatrix );
   GPA_GL( Rotated );
   GPA_GL( Rotatef );
   GPA_GL( Scaled );
   GPA_GL( Scalef );
   GPA_GL( Translated );
   GPA_GL( Translatef );
   GPA_GL( Viewport );
   GPA_GL( ArrayElement );
   GPA_GL( BindTexture );
   GPA_GL( ColorPointer );
   GPA_GL( DisableClientState );
   GPA_GL( DrawArrays );
   GPA_GL( DrawElements );
   GPA_GL( EdgeFlagPointer );
   GPA_GL( EnableClientState );
   GPA_GL( IndexPointer );
   GPA_GL( Indexub );
   GPA_GL( Indexubv );
   GPA_GL( InterleavedArrays );
   GPA_GL( NormalPointer );
   GPA_GL( PolygonOffset );
   GPA_GL( TexCoordPointer );
   GPA_GL( VertexPointer );
   GPA_GL( AreTexturesResident );
   GPA_GL( CopyTexImage1D );
   GPA_GL( CopyTexImage2D );
   GPA_GL( CopyTexSubImage1D );
   GPA_GL( CopyTexSubImage2D );
   GPA_GL( DeleteTextures );
   GPA_GL( GenTextures );
   GPA_GL( GetPointerv );
   GPA_GL( IsTexture );
   GPA_GL( PrioritizeTextures );
   GPA_GL( TexSubImage1D );
   GPA_GL( TexSubImage2D );
   GPA_GL( PopClientAttrib );
   GPA_GL( PushClientAttrib );

   return &cpt;
}

int APIENTRY
DrvSetLayerPaletteEntries(
   HDC hdc,
   INT iLayerPlane,
   INT iStart,
   INT cEntries,
   CONST COLORREF *pcr )
{
   debug_printf( "%s\n", __FUNCTION__ );

   return 0;
}

BOOL APIENTRY
DrvSetPixelFormat(
   HDC hdc,
   LONG iPixelFormat )
{
   PIXELFORMATDESCRIPTOR pfd;
   BOOL r;

   wglDescribePixelFormat( hdc, iPixelFormat, sizeof( pfd ), &pfd );
   r = wglSetPixelFormat( hdc, iPixelFormat, &pfd );

   debug_printf( "%s( 0x%p, %d ) = %s\n", __FUNCTION__, hdc, iPixelFormat, r ? "TRUE" : "FALSE" );

   return r;
}

BOOL APIENTRY
DrvShareLists(
   DHGLRC dhglrc1,
   DHGLRC dhglrc2 )
{
   debug_printf( "%s\n", __FUNCTION__ );

   return FALSE;
}

BOOL APIENTRY
DrvSwapBuffers(
   HDC hdc )
{
   debug_printf( "%s( 0x%p )\n", __FUNCTION__, hdc );

   return wglSwapBuffers( hdc );
}

BOOL APIENTRY
DrvSwapLayerBuffers(
   HDC hdc,
   UINT fuPlanes )
{
   debug_printf( "%s\n", __FUNCTION__ );

   return FALSE;
}

BOOL APIENTRY
DrvValidateVersion(
   ULONG ulVersion )
{
   debug_printf( "%s( %u )\n", __FUNCTION__, ulVersion );

   return ulVersion == 1;
}
