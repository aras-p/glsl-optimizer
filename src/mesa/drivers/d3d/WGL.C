/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 Makefile for DirectX 6                                           */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1998-1997  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#include "D3DMesa.h"
/*===========================================================================*/
/* Window managment.                                                         */
/*===========================================================================*/
static BOOL    InitOpenGL( HINSTANCE hInst );
static BOOL    TermOpenGL( HINSTANCE hInst );
static BOOL     ResizeContext( GLcontext *ctx );
static BOOL    MakeCurrent( D3DMESACONTEXT *pContext );
static void    DestroyContext( D3DMESACONTEXT *pContext );
static BOOL    UnBindWindow( D3DMESACONTEXT *pContext );
LONG APIENTRY  wglMonitorProc( HWND hwnd, UINT message, UINT wParam, LONG lParam );
/*===========================================================================*/
/* Mesa hooks.                                                               */
/*===========================================================================*/
static void SetupDDPointers( GLcontext *ctx );
static void SetupSWDDPointers( GLcontext *ctx );
static void SetupHWDDPointers( GLcontext *ctx );
static void SetupNULLDDPointers( GLcontext *ctx );
static const char *RendererString( void );

/* State Management hooks. */
static void       SetColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a );
static void       ClearColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a );
static GLboolean SetBuffer( GLcontext *ctx, GLenum buffer );

/* Window Management hooks. */
static void GetBufferSize( GLcontext *ctx, GLuint *width, GLuint *height );
static void SetViewport( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h );
static void Flush( GLcontext *ctx );

/* Span rendering hooks. */
void WSpanRGB( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgb[][3], const GLubyte mask[] );
void WSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgba[][4], const GLubyte mask[] );
void WSpanRGBAMono( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte mask[] );
void WPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte rgba[][4], const GLubyte mask[] );
void WPixelsRGBAMono( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte mask[] );
void RSpanRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, GLubyte rgba[][4] );
void RPixelsRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], GLubyte rgba[][4], const GLubyte mask[] );
GLbitfield ClearBuffers( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height );

/* Primitve rendering hooks. */
GLboolean  RenderVertexBuffer( GLcontext *ctx, GLboolean allDone );
void             RenderOneTriangle( GLcontext *ctx, GLuint v1, GLuint v2, GLuint v3, GLuint pv );
void     RenderOneLine( GLcontext *ctx, GLuint v1, GLuint v2, GLuint pv );
GLbitfield ClearBuffersD3D( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height );

/* Texture Management hooks. */
static void TextureBind( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj );
static void TextureLoad( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, GLint internalFormat, const struct gl_texture_image *image );
static void TextureSubImage( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLint internalFormat, const struct gl_texture_image *image );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
D3DMESACONTEXT *pD3DCurrent,     
	       *pD3DDefault;     /* Thin support context. */

struct __extensions__   ext[] = {

    { (PROC)glPolygonOffsetEXT,        "glPolygonOffsetEXT"       },
    { (PROC)glBlendEquationEXT,        "glBlendEquationEXT"       },
    { (PROC)glBlendColorEXT,           "glBlendColorExt"          },
    { (PROC)glVertexPointerEXT,        "glVertexPointerEXT"       },
    { (PROC)glNormalPointerEXT,        "glNormalPointerEXT"       },
    { (PROC)glColorPointerEXT,         "glColorPointerEXT"        },
    { (PROC)glIndexPointerEXT,         "glIndexPointerEXT"        },
    { (PROC)glTexCoordPointerEXT,      "glTexCoordPointer"        },
    { (PROC)glEdgeFlagPointerEXT,      "glEdgeFlagPointerEXT"     },
    { (PROC)glGetPointervEXT,          "glGetPointervEXT"         },
    { (PROC)glArrayElementEXT,         "glArrayElementEXT"        },
    { (PROC)glDrawArraysEXT,           "glDrawArrayEXT"           },
    { (PROC)glAreTexturesResidentEXT,  "glAreTexturesResidentEXT" },
    { (PROC)glBindTextureEXT,          "glBindTextureEXT"         },
    { (PROC)glDeleteTexturesEXT,       "glDeleteTexturesEXT"      },
    { (PROC)glGenTexturesEXT,          "glGenTexturesEXT"         },
    { (PROC)glIsTextureEXT,            "glIsTextureEXT"           },
    { (PROC)glPrioritizeTexturesEXT,   "glPrioritizeTexturesEXT"  },
    { (PROC)glCopyTexSubImage3DEXT,    "glCopyTexSubImage3DEXT"   },
    { (PROC)glTexImage3DEXT,           "glTexImage3DEXT"          },
    { (PROC)glTexSubImage3DEXT,        "glTexSubImage3DEXT"       },
};

int             qt_ext = sizeof(ext) / sizeof(ext[0]);
float   g_DepthScale,
	  g_MaxDepth;
/*===========================================================================*/
/*  When a process loads this DLL we will setup the linked list for context  */
/* management and create a default context that will support the API until   */
/* the user creates and binds thier own.  This THIN default context is useful*/
/* to have around.                                                           */
/*  When the process terminates we will clean up all resources here.         */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY   DllMain( HINSTANCE hInst, DWORD reason, LPVOID reserved )
{
  switch( reason ) 
  {
    case DLL_PROCESS_ATTACH:
	 return InitOpenGL( hInst );

    case DLL_PROCESS_DETACH:
	 return TermOpenGL( hInst );
  }     

  return TRUE;
}
/*===========================================================================*/
/*  The first thing we do when this dll is hit is connect to the dll that has*/
/* handles all the DirectX 6 rendering.  I decided to use another dll as DX6 */
/* is all C++ and Mesa-3.0 is C (thats a good thing).  This way I can write  */
/* the DX6 in C++ and Mesa-3.0 in C without having to worry about linkage.   */
/* I feel this is easy and better then using static wrappers as it is likely */
/* faster and it allows me to just develope the one without compiling the    */
/* other.                                                                    */
/*  NOTE that at this point we don't have much other than a very thin context*/
/* that will support the API calls only to the point of not causing the app  */
/* to crash from the API table being empty.                                  */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static BOOL  InitOpenGL( HINSTANCE hInst )
{
  /* Allocate and clear the default context. */
  pD3DDefault = (PD3DMESACONTEXT)ALLOC( sizeof(D3DMESACONTEXT) );
  if ( pD3DDefault == NULL )
    return FALSE;
  memset( pD3DDefault, 0, sizeof(D3DMESACONTEXT) );

  /*  Clear the D3D vertex buffer so that values not used will be zero.  This */
  /* save me from some redundant work.                                        */
  memset( &D3DTLVertices, 0, sizeof(D3DTLVertices) );

  /*  Update the link.  We uses a circular list so that it is easy to  */
  /* add and search.  This context will also be used for head and tail.*/
  pD3DDefault->next = pD3DDefault;

  /*========================================================================*/
  /* Do all core Mesa stuff.                                                */
  /*========================================================================*/
  pD3DDefault->gl_visual = _mesa_create_visual( TRUE,
						FALSE,      /* db_flag */
						GL_FALSE,   /* stereo */
						8,8,8,8,    /* r, g, b, a bits */
						0,          /* index bits */
						16,         /* depth_bits */
						8,          /* stencil_bits */
						8,8,8,8,    /* accum_bits */
                                                1 );

  if ( pD3DDefault->gl_visual == NULL)  
  {
    FREE( pD3DDefault );
    return FALSE;
  }

  /* Allocate a new Mesa context */
  pD3DDefault->gl_ctx = _mesa_create_context( pD3DDefault->gl_visual, NULL, pD3DDefault, GL_TRUE );
  if ( pD3DDefault->gl_ctx == NULL ) 
  {
    _mesa_destroy_visual( pD3DDefault->gl_visual );
    FREE( pD3DDefault );
    return FALSE;
  }

  /* Allocate a new Mesa frame buffer */
  pD3DDefault->gl_buffer = _mesa_create_framebuffer( pD3DDefault->gl_visual );
  if ( pD3DDefault->gl_buffer == NULL )
  {
    _mesa_destroy_visual( pD3DDefault->gl_visual );
    _mesa_destroy_context( pD3DDefault->gl_ctx );
    FREE( pD3DDefault );
    return FALSE;
  }
  SetupDDPointers( pD3DDefault->gl_ctx );
  _mesa_make_current( pD3DDefault->gl_ctx, pD3DDefault->gl_buffer );

  return TRUE;
}
/*===========================================================================*/
/*  This function will create a new D3D context but will not create the D3D  */
/* surfaces or even an instance of D3D (see at GetBufferSize). The only stuff*/
/* done here is the internal Mesa stuff and some Win32 handles.              */
/*===========================================================================*/
/* RETURN: casted pointer to the context, NULL.                              */
/*===========================================================================*/
HGLRC APIENTRY wglCreateContext( HDC hdc )
{
  D3DMESACONTEXT        *pNewContext;
  DWORD                 dwCoopFlags = DDSCL_NORMAL;
  RECT                  rectClient;
  POINT                 pt;

  /* ALLOC and clear the new context. */
  pNewContext = (PD3DMESACONTEXT)ALLOC( sizeof(D3DMESACONTEXT) );
  if ( pNewContext == NULL )
  {
    SetLastError( 0 );
    return (HGLRC)NULL;
  }
  memset( pNewContext, 0, sizeof(D3DMESACONTEXT) );

  /*========================================================================*/
  /* Do all core Mesa stuff.                                                */
  /*========================================================================*/

  /* TODO: support more then one visual. */
  pNewContext->gl_visual = _mesa_create_visual( TRUE,
                                                TRUE,       /* db_flag */
                                                GL_FALSE,   /* stereo */
                                                8,8,8,8,    /* r, g, b, a bits */
                                                0,          /* index bits */
                                                16,         /* depth_bits */
                                                8,          /* stencil_bits */
                                                16,16,16,16,/* accum_bits */
                                                1 );
  if ( pNewContext->gl_visual == NULL) 
  {
    FREE( pNewContext );
    SetLastError( 0 );
    return (HGLRC)NULL;
  }

  /* Allocate a new Mesa context */
  pNewContext->gl_ctx = _mesa_create_context( pNewContext->gl_visual, NULL, pNewContext, GL_TRUE );
  if ( pNewContext->gl_ctx == NULL ) 
  {
    _mesa_destroy_visual( pNewContext->gl_visual );
    FREE( pNewContext );
    SetLastError( 0 );
    return (HGLRC)NULL;
  }

  /* Allocate a new Mesa frame buffer */
  pNewContext->gl_buffer = _mesa_create_framebuffer( pNewContext->gl_visual );
  if ( pNewContext->gl_buffer == NULL )
  {
    _mesa_destroy_visual( pNewContext->gl_visual );
    _mesa_destroy_context( pNewContext->gl_ctx );
    FREE( pNewContext );
    SetLastError( 0 );
    return (HGLRC)NULL;
  }

  /*========================================================================*/
  /* Do all the driver stuff.                                               */
  /*========================================================================*/
  pNewContext->hdc  = hdc;
  pNewContext->next = pD3DDefault->next;
  pD3DDefault->next = pNewContext; /* Add to circular list. */

  /* Create the HAL for the new context. */
  pNewContext->pShared = InitHAL( WindowFromDC(hdc) );

  return (HGLRC)pNewContext;
}
/*===========================================================================*/
/*  This is a wrapper function that is supported by MakeCurrent.             */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY  wglMakeCurrent( HDC hdc, HGLRC hglrc )
{
   return MakeCurrent((D3DMESACONTEXT *)hglrc);
}
/*===========================================================================*/
/*  MakeCurrent will unbind whatever context is current (if any) & then bind */
/* the supplied context.  A context that is bound has it's window proc hooked*/
/* with the wglMonitorProc and the context pointer is saved in pD3DCurrent.    */
/* Once the context is bound we update the Mesa-3.0 hooks (SetDDPointers) and*/
/* the viewport (Mesa-.30 and DX6).                                          */
/*                                                                           */
/* TODO: this function can't fail.                                           */
/*===========================================================================*/
/* RETURN: TRUE                                                              */
/*===========================================================================*/
static BOOL MakeCurrent( D3DMESACONTEXT *pContext )
{
   D3DMESACONTEXT *pNext;

   /*====================================================================*/
   /* This is a special case that is a request to have no context bound. */
   /*====================================================================*/
   if ( pContext == NULL )
   {
	/* Walk the whole list. We start and end at the Default context. */
	for( pNext = pD3DDefault->next; pNext != pD3DDefault; pNext = pNext->next )
	  UnBindWindow( pNext );
      
	return TRUE;
   }

   /*=================================================*/
   /* Make for a fast redundant use of this function. */
   /*=================================================*/
   if ( pD3DCurrent == pContext )
      return TRUE;

   /*=============================*/
   /* Unbind the current context. */
   /*=============================*/
   UnBindWindow( pD3DCurrent );

   /*=====================================*/
   /* Let Mesa-3.0 we have a new context. */
   /*=====================================*/
   SetupDDPointers( pContext->gl_ctx );
   _mesa_make_current( pContext->gl_ctx, pContext->gl_buffer );

   /*  We are done so set the internal current context. */
   if ( pContext != pD3DDefault )
   {
	ResizeContext( pContext->gl_ctx );
	pContext->hOldProc = (WNDPROC)GetWindowLong( pContext->pShared->hwnd, GWL_WNDPROC );
	SetWindowLong( pContext->pShared->hwnd, GWL_WNDPROC, (LONG)wglMonitorProc );
   }
   pD3DCurrent = pContext;

   return TRUE;
}
/*===========================================================================*/
/*  This function will only return the current window size.  I have re-done  */   
/* this function so that it doesn't check the current size and react to it as*/   
/* I should be able to have all the react code in the WM_SIZE message.  The  */   
/* old version would check the current window size and create/resize the HAL */   
/* surfaces if they have changed.  I needed to delay the creation if the     */   
/* surfaces because sometimes I wouldn't have a window size so this is where */   
/* I delayed it.  If you are reading this then all went ok!                  */   
/*  The default context will return a zero sized window and I'm not sure if  */
/* this is ok at this point (TODO).                                          */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void GetBufferSize( GLcontext *ctx, GLuint *width, GLuint *height )
{
  D3DMESACONTEXT        *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  /* Fall through for the default because that is one of the uses for it. */
  if ( pContext == pD3DDefault )
  {
    *width  = 0;
    *height = 0;
  }
  else
  {
    *width  = pContext->pShared->dwWidth;
    *height = pContext->pShared->dwHeight;
  }
}
/*===========================================================================*/
/*                                                                           */
/*                                                                           */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static BOOL ResizeContext( GLcontext *ctx )
{
  D3DMESACONTEXT        *pContext = (D3DMESACONTEXT *)ctx->DriverCtx,
		    *pCurrentTemp;
  RECT                  rectClient;
  POINT                 pt;
  DWORD                 dwWidth,
		    dwHeight;
  static BOOL           bDDrawLock = FALSE;

  /* Make sure we have some values. */
  if ( (pContext->hdc == NULL ) || 
	  (pContext->pShared->hwnd != WindowFromDC(pContext->hdc)) ||
       (pContext == pD3DDefault) )
    return FALSE;

  /* Having problems with DDraw sending resize messages before I was done. */
  if( bDDrawLock == TRUE )
    return FALSE;

  // TODO: don't think I need this anymore.
  pCurrentTemp = pD3DCurrent;
  pD3DCurrent = pD3DDefault;
  bDDrawLock = TRUE;

  /* Get the current window dimentions. */
  UpdateScreenPosHAL( pContext->pShared );
  dwWidth  = pContext->pShared->rectW.right - pContext->pShared->rectW.left;
  dwHeight = pContext->pShared->rectW.bottom - pContext->pShared->rectW.top;

  /* Is the size of the OffScreen Render different? */
  if ( (dwWidth != pContext->pShared->dwWidth) || (dwHeight != pContext->pShared->dwHeight) )
  {
    /* Create all the D3D surfaces and device. */
    CreateHAL( pContext->pShared );

    /*  I did this so that software rendering would still work as */
    /* I don't need to scale the z values twice.                  */
    g_DepthScale        = (pContext->pShared->bHardware) ? 1.0 : ((float)0x00FFFFFF);
    g_MaxDepth          = (pContext->pShared->bHardware) ? 1.0 : ((float)0x00FFFFFF);
    gl_DepthRange( pContext->gl_ctx, ctx->Viewport.Near, ctx->Viewport.Far );

    /* Make sure we have a viewport. */
    gl_Viewport( pContext->gl_ctx, 0, 0, dwWidth, dwHeight );

    /* Update Mesa as we might have changed from SW <-> HW. */
    SetupDDPointers( pContext->gl_ctx );
    _mesa_make_current( pContext->gl_ctx, pContext->gl_buffer );

    /*  If we are in HW we need to load the current texture if there is one already. */
    //    if ( (ctx->Texture.Set[ctx->Texture.CurrentSet].Current != NULL) &&
    //      (pContext->pShared->bHardware == TRUE) )
    //    {
    //   CreateTMgrHAL( pContext->pShared,
    //                           ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Name,
    //                           0,     
    //                           ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0]->Format,
    //                           (RECT *)NULL,
    //                           ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0]->Width,
    //                           ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0]->Height,
    //                           TM_ACTION_BIND,
    //                           (void *)ctx->Texture.Set[ctx->Texture.CurrentSet].Current->Image[0]->Data );
    //    }
  }

  // TODO: don't think I need this anymore.
  pD3DCurrent = pCurrentTemp;
  bDDrawLock = FALSE;

  return TRUE;
}

/*===========================================================================*
/*  This function will Blt the render buffer to the PRIMARY surface. I repeat*/
/* this code for the other SwapBuffer like functions and the flush (didn't   */
/* want the function calling overhead).  Thsi could have been a macro...     */
/*                                                                           */
/* TODO: there are some problems with viewport/scissoring.                   */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY  wglSwapBuffers( HDC hdc )
{
  /* Fall through for the default because that is one of the uses for it. */
  if ( pD3DCurrent == pD3DDefault )
    return FALSE;

  SwapBuffersHAL( pD3DCurrent->pShared );

  return TRUE;
}
/*===========================================================================*/
/*  Same as wglSwapBuffers.                                                  */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY  SwapBuffers( HDC hdc )
{
  /* Fall through for the default because that is one of the uses for it. */
  if ( pD3DCurrent == pD3DDefault )
    return FALSE;

  SwapBuffersHAL( pD3DCurrent->pShared );

  return TRUE;
}
/*===========================================================================*/
/*  This should be ok as none of the SwapBuffers will cause a redundant Blt  */
/* as none of my Swap functions will call flush.  This should also allow     */
/* sinlge buffered applications to work (not really worried though).  Some   */
/* applications may flush then swap but then this is there fault IMHO.       */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void Flush( GLcontext *ctx )
{
  /* Fall through for the default because that is one of the uses for it. */
  if ( pD3DCurrent == pD3DDefault )
    return;

  SwapBuffersHAL( pD3DCurrent->pShared );
}
/*===========================================================================*/
/*  For now this function will ignore the supplied PF. If I'm going to allow */
/* the user to choice the mode and device at startup I'm going to have to do */
/* something different.                                                      */
/*                                                                           */
/* TODO: use the linked list of modes to build a pixel format to be returned */
/*      to the caller.                                                       */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY   wglChoosePixelFormat( HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
   return 1;
}
/*===========================================================================*/
/*  See wglChoosePixelFormat.                                                */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY   ChoosePixelFormat( HDC hdc, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
  return wglChoosePixelFormat(hdc,ppfd);
}
/*===========================================================================*/
/*  This function (for now) returns a static PF everytime.  This is just to  */
/* allow things to continue.                                                 */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY   wglDescribePixelFormat( HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd )
{
   static PIXELFORMATDESCRIPTOR  pfd = 
   {
      sizeof(PIXELFORMATDESCRIPTOR),   /* size */
      1,                               /* version */
      PFD_SUPPORT_OPENGL |
      PFD_DRAW_TO_WINDOW |
      PFD_DOUBLEBUFFER,                /* support double-buffering */
      PFD_TYPE_RGBA,                   /* color type */
      16,                              /* prefered color depth */
      0, 0, 0, 0, 0, 0,                /* color bits (ignored) */
      0,                               /* no alpha buffer */
      0,                               /* alpha bits (ignored) */
      0,                               /* no accumulation buffer */
      0, 0, 0, 0,                      /* accum bits (ignored) */
      16,                              /* depth buffer */
      0,                               /* no stencil buffer */
      0,                               /* no auxiliary buffers */
      PFD_MAIN_PLANE,                  /* main layer */
      0,                               /* reserved */
      0, 0, 0,                         /* no layer, visible, damage masks */
   };

   /* Return the address of this static PF if one was requested. */
   if ( ppfd != NULL )
      memcpy( ppfd, &pfd, sizeof(PIXELFORMATDESCRIPTOR) );

  return 1;
}
/*===========================================================================*/
/*  See wglDescribePixelFormat.                                              */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY   DescribePixelFormat( HDC hdc, int iPixelFormat, UINT nBytes, LPPIXELFORMATDESCRIPTOR ppfd )
{
  return wglDescribePixelFormat(hdc,iPixelFormat,nBytes,ppfd);
}
/*===========================================================================*/
/*  This function will always return 1 for now.  Just to allow for support.  */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY   wglGetPixelFormat( HDC hdc )
{
   return 1;
}
/*===========================================================================*/
/*  See wglGetPixelFormat.                                                   */
/*===========================================================================*/
/* RETURN: 1.                                                                */
/*===========================================================================*/
int APIENTRY   GetPixelFormat( HDC hdc )
{
   return wglGetPixelFormat(hdc);
}
/*===========================================================================*/
/*  This will aways work for now.                                            */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL APIENTRY  wglSetPixelFormat( HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
   return TRUE;
}
/*===========================================================================*/
/*  See wglSetPixelFormat.                                                   */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
BOOL APIENTRY  SetPixelFormat( HDC hdc, int iPixelFormat, CONST PIXELFORMATDESCRIPTOR *ppfd )
{
   return wglSetPixelFormat(hdc,iPixelFormat,ppfd);
}
/*===========================================================================*/
/*  This is a wrapper function that is supported by my own internal function.*/
/* that takes my own D3D Mesa context structure.  This so I can reuse the    */
/* function (no need for speed).                                             */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
BOOL APIENTRY  wglDeleteContext( HGLRC hglrc )
{
   DestroyContext( (D3DMESACONTEXT *)hglrc );

   return TRUE;
}
/*===========================================================================*/
/*  Simple getter function that uses a cast.                                 */
/*===========================================================================*/
/* RETURN: casted pointer to the context, NULL.                              */
/*===========================================================================*/
HGLRC APIENTRY wglGetCurrentContext( VOID )
{
   return (pD3DCurrent) ? (HGLRC)pD3DCurrent : (HGLRC)NULL;
}
/*===========================================================================*/
/* No support.                                                               */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglCopyContext( HGLRC hglrcSrc, HGLRC hglrcDst, UINT mask )
{
   SetLastError( 0 );
   return FALSE;
}
/*===========================================================================*/
/* No support.                                                               */
/*===========================================================================*/
/* RETURN: NULL.                                                             */
/*===========================================================================*/
HGLRC APIENTRY wglCreateLayerContext( HDC hdc,int iLayerPlane )
{
   SetLastError( 0 );
   return (HGLRC)NULL;
}
/*===========================================================================*/
/*  Simple getter function.                                                  */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
HDC APIENTRY   wglGetCurrentDC( VOID )
{
   return (pD3DCurrent) ? pD3DCurrent->hdc : (HDC)NULL;
}
/*===========================================================================*/
/*  Simply call that searches the supported extensions for a match & returns */
/* the pointer to the function that lends support.                           */
/*===========================================================================*/
/* RETURN: pointer to API call, NULL.                                        */
/*===========================================================================*/
PROC APIENTRY  wglGetProcAddress( LPCSTR lpszProc )
{
   int   index;

   for( index = 0; index < qt_ext; index++ )
      if( !strcmp(lpszProc,ext[index].name) )
	 return ext[index].proc;

   SetLastError( 0 );
   return NULL;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglShareLists( HGLRC hglrc1, HGLRC hglrc2 )
{
   SetLastError( 0 );
   return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglUseFontBitmaps( HDC fontDevice, DWORD firstChar, DWORD numChars, DWORD listBase )
{
   SetLastError( 0 );
   return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglUseFontBitmapsW( HDC hdc,DWORD first,DWORD count,DWORD listBase )
{
   SetLastError( 0 );
   return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglUseFontOutlinesA( HDC hdc, DWORD first, DWORD count, DWORD listBase, FLOAT deviation, FLOAT extrusion, int format, LPGLYPHMETRICSFLOAT lpgmf )
{
   SetLastError( 0 );
   return FALSE;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglUseFontOutlinesW( HDC hdc,DWORD first,DWORD count, DWORD listBase,FLOAT deviation, FLOAT extrusion,int format, LPGLYPHMETRICSFLOAT lpgmf )
{
   SetLastError( 0 );
   return FALSE ;
}
/*===========================================================================*/
/*  No support.                                                              */
/*===========================================================================*/
/* RETURN: FALSE.                                                            */
/*===========================================================================*/
BOOL APIENTRY  wglSwapLayerBuffers( HDC hdc, UINT fuPlanes )
{
   SetLastError( 0 );
   return FALSE;
}
/*===========================================================================*/
/*  This function will be hooked into the window that has been bound.  Right */
/* now it is used to track the window size and position.  Also the we clean  */
/* up the currrent context when the window is close/destroyed.               */
/*                                                                           */
/* TODO: there might be something wrong here as some games (Heretic II) don't*/
/*      track the window quit right.                                         */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
LONG APIENTRY  wglMonitorProc( HWND hwnd, UINT message, UINT wParam, LONG lParam )
{
  WNDPROC       hOldProc;
  GLint width,
	  height;

  switch( message ) 
  {
//      case WM_PAINT:
//        break;
//      case WM_ACTIVATE:
//         break;
//      case WM_SHOWWINDOW:
//         break;

    case UM_FATALSHUTDOWN:
	 /* Support the API until we die... */
	 MakeCurrent( pD3DDefault );
	 break;

    case WM_MOVE:
    case WM_DISPLAYCHANGE:
    case WM_SIZE:
	 ResizeContext( pD3DCurrent->gl_ctx );
	 break;

    case WM_CLOSE:
    case WM_DESTROY:
	 /* Support the API until we die... */
	 hOldProc = pD3DCurrent->hOldProc;
	 DestroyContext( pD3DCurrent );
	 return (hOldProc)(hwnd,message,wParam,lParam);
  }

  return (pD3DCurrent->hOldProc)(hwnd,message,wParam,lParam);
}

/**********************************************************************/
/*****              Miscellaneous device driver funcs             *****/
/**********************************************************************/

/*===========================================================================*/
/*  Not reacting to this as I'm only supporting drawing to the back buffer   */
/* right now.                                                                */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
static GLboolean SetBuffer( GLcontext *ctx, GLenum buffer )
{
   if (buffer == GL_BACK_LEFT)
      return GL_TRUE;
   else
      return GL_FALSE;
}
/*===========================================================================*/
/*  This proc will be called by Mesa when the viewport has been set.  So if  */
/* we have a context and it isn't the default then we should let D3D know of */
/* the change.                                                               */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void SetViewport( GLcontext *ctx, GLint x, GLint y, GLsizei w, GLsizei h )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
   RECT           rect;

   /* Make sure we can set a viewport. */
   if ( pContext->pShared && (pContext != pD3DDefault) )
   {
	 // TODO: might be needed.
     UpdateScreenPosHAL( pContext->pShared );
	rect.left   = x;
	rect.right  = x + w;
	rect.top    = y;
	rect.bottom = y + h;

	// TODO: shared struct should make this call smaller
     SetViewportHAL( pContext->pShared, &rect, 0.0F, 1.0F );
   }
}
/*===========================================================================*/
/*  This function could be better I guess but I decided just to grab the four*/
/* components and store then seperately.  Makes it easier to use IMHO.       */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void ClearColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

   pContext->aClear = a;
   pContext->bClear = b;
   pContext->gClear = g;
   pContext->rClear = r;
}
/*===========================================================================*/
/*  This function could be better I guess but I decided just to grab the four*/
/* components and store then seperately.  Makes it easier to use IMHO.       */
/* (is there an echo in here?)                                               */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void SetColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

   pContext->aCurrent = a;
   pContext->bCurrent = b;
   pContext->gCurrent = g;
   pContext->rCurrent = r;
}
/*===========================================================================*/
/*                                                                           */
/*                                                                           */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static const char *RendererString( void )
{
  static char pszRender[64];

  strcpy( pszRender, "altD3D " );

  if ( pD3DCurrent->pShared->bHardware )
    strcat( pszRender, "(HW)");
  else
    strcat( pszRender, "(SW)");

  return (const char *)pszRender;
}
/*===========================================================================*/
/*  This function will choose which set of pointers Mesa will use based on   */
/* whether we hard using hardware or software.  I have added another set of  */
/* pointers that will do nothing but stop the API from crashing.             */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void SetupDDPointers( GLcontext *ctx )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

   // TODO: write a generic NULL support for the span render. 
   if ( pContext->pShared && pContext->pShared->bHardware )
   {
	ctx->Driver.UpdateState = SetupHWDDPointers;
   }
   else if ( pContext == pD3DDefault )
   {
	ctx->Driver.UpdateState = SetupNULLDDPointers;
   }
   else
   {
	ctx->Driver.UpdateState = SetupSWDDPointers;
   }
}
/*===========================================================================*/
/*  This function will populate all the Mesa driver hooks. This version of   */
/* hooks will do nothing but support the API when we don't have a valid      */
/* context bound.  This is mostly for applications that don't behave right   */
/* and also to help exit as clean as possable when we have a FatalError.     */
/*===========================================================================*/
/* RETURN: pointer to the specific function.                                 */
/*===========================================================================*/
static void SetupNULLDDPointers( GLcontext *ctx )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

   /* Initialize all the pointers in the DD struct.  Do this whenever */
   /* a new context is made current or we change buffers via set_buffer! */
   ctx->Driver.UpdateState          = SetupNULLDDPointers;

   /* State management hooks. */
   ctx->Driver.Color                = NULLSetColor;
   ctx->Driver.ClearColor           = NULLClearColor;
   ctx->Driver.Clear                = NULLClearBuffers;
   ctx->Driver.SetBuffer            = NULLSetBuffer;

   /* Window management hooks. */
   ctx->Driver.GetBufferSize        = NULLGetBufferSize;

   /* Primitive rendering hooks. */
   ctx->Driver.TriangleFunc         = NULL;
   ctx->Driver.RenderVB             = NULL;

   /* Pixel/span writing functions: */
   ctx->Driver.WriteRGBASpan        = NULLWrSpRGBA;
   ctx->Driver.WriteRGBSpan         = NULLWrSpRGB;
   ctx->Driver.WriteMonoRGBASpan    = NULLWrSpRGBAMono;
   ctx->Driver.WriteRGBAPixels      = NULLWrPiRGBA;
   ctx->Driver.WriteMonoRGBAPixels  = NULLWrPiRGBAMono;

   /* Pixel/span reading functions: */
   ctx->Driver.ReadRGBASpan         = NULLReSpRGBA;
   ctx->Driver.ReadRGBAPixels       = NULLRePiRGBA;

   /* Misc. hooks. */
   ctx->Driver.RendererString    = RendererString;
}
/*===========================================================================*/
/*  This function will populate all the Mesa driver hooks. There are two of  */
/* these functions.  One if we have hardware support and one is there is only*/
/* software.  These functions will be called by Mesa and by the wgl.c when we*/
/* have resized (or created) the buffers.  The thing is that if a window gets*/
/* resized we may loose hardware support or gain it...                       */
/*===========================================================================*/
/* RETURN: pointer to the specific function.                                 */
/*===========================================================================*/
static void SetupSWDDPointers( GLcontext *ctx )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

   /* Initialize all the pointers in the DD struct.  Do this whenever */
   /* a new context is made current or we change buffers via set_buffer! */
   ctx->Driver.UpdateState          = SetupSWDDPointers;

   /* State management hooks. */
   ctx->Driver.Color                = SetColor;
   ctx->Driver.ClearColor           = ClearColor;
   ctx->Driver.Clear                = ClearBuffers;
   ctx->Driver.SetBuffer            = SetBuffer;

   /* Window management hooks. */
   ctx->Driver.GetBufferSize        = GetBufferSize;
   ctx->Driver.Viewport             = SetViewport;

   /* Primitive rendering hooks. */
   ctx->Driver.TriangleFunc         = NULL;
   ctx->Driver.RenderVB             = NULL;

   /* Texture management hooks. */

   /* Pixel/span writing functions: */
   ctx->Driver.WriteRGBASpan        = WSpanRGBA;
   ctx->Driver.WriteRGBSpan         = WSpanRGB;
   ctx->Driver.WriteMonoRGBASpan    = WSpanRGBAMono;
   ctx->Driver.WriteRGBAPixels      = WPixelsRGBA;
   ctx->Driver.WriteMonoRGBAPixels  = WPixelsRGBAMono;

   /* Pixel/span reading functions: */
   ctx->Driver.ReadRGBASpan         = RSpanRGBA;
   ctx->Driver.ReadRGBAPixels       = RPixelsRGBA;

   /* Misc. hooks. */
   ctx->Driver.Flush                = Flush;
   ctx->Driver.RendererString    = RendererString;
}
/*===========================================================================*/
/*  This function will populate all the Mesa driver hooks. There are two of  */
/* these functions.  One if we have hardware support and one is there is only*/
/* software.  These functions will be called by Mesa and by the wgl.c when we*/
/* have resized (or created) the buffers.  The thing is that if a window gets*/
/* resized we may loose hardware support or gain it...                       */
/*===========================================================================*/
/* RETURN: pointer to the specific function.                                 */
/*===========================================================================*/
static void SetupHWDDPointers( GLcontext *ctx )
{
   D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

   /* Initialize all the pointers in the DD struct.  Do this whenever */
   /* a new context is made current or we change buffers via set_buffer! */
   ctx->Driver.UpdateState          = SetupHWDDPointers;

   /* State management hooks. */
   ctx->Driver.Color                = SetColor;
   ctx->Driver.ClearColor           = ClearColor;
   ctx->Driver.Clear                = ClearBuffersD3D;
   ctx->Driver.SetBuffer            = SetBuffer;

   /* Window management hooks. */
   ctx->Driver.GetBufferSize        = GetBufferSize;
   ctx->Driver.Viewport             = SetViewport;

   /* Primitive rendering hooks. */
   ctx->Driver.TriangleFunc         = RenderOneTriangle;
   ctx->Driver.LineFunc                          = RenderOneLine;
   ctx->Driver.RenderVB             = RenderVertexBuffer;

   /* Pixel/span writing functions: */
   ctx->Driver.WriteRGBASpan        = WSpanRGBA;
   ctx->Driver.WriteRGBSpan         = WSpanRGB;
   ctx->Driver.WriteMonoRGBASpan    = WSpanRGBAMono;
   ctx->Driver.WriteRGBAPixels      = WPixelsRGBA;
   ctx->Driver.WriteMonoRGBAPixels  = WPixelsRGBAMono;

   /* Pixel/span reading functions: */
   ctx->Driver.ReadRGBASpan         = RSpanRGBA;
   ctx->Driver.ReadRGBAPixels       = RPixelsRGBA;

   /* Texture management hooks. */
   //   ctx->Driver.BindTexture          = TextureBind;
   ctx->Driver.TexImage             = TextureLoad;
   ctx->Driver.TexSubImage          = TextureSubImage;

   /* Misc. hooks. */
   ctx->Driver.Flush                = Flush;
   ctx->Driver.RendererString    = RendererString;
}
/*===========================================================================*/
/*  This function will release all resources used by the DLL.  Every context */
/* will be clobbered by releaseing all driver desources and then freeing the */
/* context memory.  Most all the work is done in DestroyContext.             */
/*===========================================================================*/
/* RETURN: TRUE.                                                             */
/*===========================================================================*/
static BOOL  TermOpenGL( HINSTANCE hInst )
{
  D3DMESACONTEXT *pTmp,
		 *pNext;

  /* Just incase we are still getting paint msg. */
  MakeCurrent( pD3DDefault );

  /* Walk the list until we get back to the default context. */
  for( pTmp = pD3DDefault->next; pTmp != pD3DDefault; pTmp = pNext )
  {
    pNext = pTmp->next;
    DestroyContext( pTmp );
  }
  DestroyContext( pD3DDefault );

  return TRUE;
}
/*===========================================================================*/
/*  This function is an internal function that will clean up all the Mesa    */
/* context bound to this D3D context.  Also any D3D stuff that this context  */
/* uses will be unloaded.                                                    */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static void DestroyContext( D3DMESACONTEXT *pContext )
{
  D3DMESACONTEXT        *pTmp;

  /* Walk the list until we find the context before this one. */
  for( pTmp = pD3DDefault; pTmp && (pTmp->next != pContext); pTmp = pTmp->next )
    if ( pTmp == pTmp->next )
	 break;

  /* If we never found it it must already be deleted. */
  if ( pTmp->next != pContext )
    return;

  /* Make sure we are not using this context. */
  if ( pContext == pD3DCurrent )
    MakeCurrent( pD3DDefault );

   /* Free the Mesa stuff. */
   if ( pContext->gl_visual ) 
   {
      _mesa_destroy_visual( pContext->gl_visual );
      pContext->gl_visual = NULL;
   }
   if ( pContext->gl_buffer ) 
   {
      _mesa_destroy_framebuffer( pContext->gl_buffer );
      pContext->gl_buffer = NULL;
   }
   if ( pContext->gl_ctx )    
   {
      _mesa_destroy_context( pContext->gl_ctx );
      pContext->gl_ctx = NULL;
   }

   /* Now dump the D3D. */
   if ( pContext->pShared )
	TermHAL( pContext->pShared );

   /* Update the previous context's link. */
   pTmp->next = pContext->next;

   /* Gonzo. */
   FREE( pContext );
}
/*===========================================================================*/
/*  This function will pull the supplied context away from Win32.  Basicly it*/
/* will remove the hook from the window Proc.                                */
/*                                                                           */
/* TODO: might want to serialize this stuff...                               */
/*===========================================================================*/
/* RETURN: TRUE, FALSE.                                                      */
/*===========================================================================*/
static BOOL UnBindWindow( D3DMESACONTEXT *pContext )
{
  if ( pContext == NULL )
    return FALSE;

  if ( pContext == pD3DDefault )
    return TRUE;

  /* Make sure we always have a context bound. */
  if ( pContext == pD3DCurrent )
    pD3DCurrent = pD3DDefault;

  SetWindowLong( pContext->pShared->hwnd, GWL_WNDPROC, (LONG)pContext->hOldProc );
  pContext->hOldProc   = NULL;

  return TRUE;
}
/*===========================================================================*/
/*  There are two cases that allow for a faster clear when we know that the  */
/* whole buffer is cleared and that there is no clipping.                    */
/*===========================================================================*/
/* RETURN: the original mask with the bits cleared that represents the buffer*
/* or buffers we just cleared.                                               */
/*===========================================================================*/
GLbitfield ClearBuffersD3D( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  DWORD          dwFlags = 0;

  if ( mask & GL_COLOR_BUFFER_BIT )
  {
    dwFlags |= D3DCLEAR_TARGET;
    mask &= ~GL_COLOR_BUFFER_BIT;
  }
  if ( mask & GL_DEPTH_BUFFER_BIT )
  {
    dwFlags |= D3DCLEAR_ZBUFFER;
    mask &= ~GL_DEPTH_BUFFER_BIT;
  }
  if ( dwFlags == 0 )
    return mask;

  ClearHAL( pContext->pShared, 
		  dwFlags, 
		  all, 
		  x, y, 
		  width, height, 
		  ((pContext->aClear<<24) | (pContext->rClear<<16) | (pContext->gClear<<8) | (pContext->bClear)), 
		  ctx->Depth.Clear, 
		  0 );

  return mask;
}



/*===========================================================================*/
/*  TEXTURE MANAGER: ok here is how I did textures.  Mesa-3.0 will keep track*/
/* of all the textures for us.  So this means that at anytime we can go to   */
/* the Mesa context and get the current texture.  With this in mind this is  */
/* what I did.  I really don't care about what textures get or are loaded    */
/* until I actually have to draw a tri that is textured.  At this point I    */
/* must have the texture so I demand the texture by destorying all other     */
/* texture surfaces if need be and load the current one.  This allows for the*/
/* best preformance on low memory cards as time is not wasted loading and    */
/* unload textures.                                                          */
/*===========================================================================*/





/*===========================================================================*/
/*  TextureLoad will try and create a D3D surface from the supplied texture  */
/* object if its level 0 (first).  The surface will be fully filled with the */
/* texture.                                                                  */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void TextureLoad( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, GLint internalFormat, const struct gl_texture_image *image )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  /* TODO: only doing first LOD. */
  if ( (ctx->DriverCtx == NULL) || (level != 0) )
    return;

  CreateTMgrHAL( pContext->pShared, 
			  tObj->Name, 
			  level,
			  tObj->Image[level]->Format,
			  (RECT *)NULL,
			  tObj->Image[level]->Width, 
			  tObj->Image[level]->Height,
			  TM_ACTION_LOAD,
			  (void *)tObj->Image[level]->Data );
}
/*===========================================================================*/
/*  TextureBind make sure that the texture is on the card.  Thats it.        */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void TextureBind( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;

  /* TODO: only doing first LOD. */
  if ( (tObj->Image[0] == NULL) || (ctx->DriverCtx == NULL) )
    return;

  CreateTMgrHAL( pContext->pShared, 
			  tObj->Name, 
			  0,
			  tObj->Image[0]->Format,
			  (RECT *)NULL,
			  tObj->Image[0]->Width, 
			  tObj->Image[0]->Height,
			  TM_ACTION_BIND,
			  (void *)tObj->Image[0]->Data );
}
/*===========================================================================*/
/*  TextureSubImage will make sure that the texture being updated is updated */
/* if its on the card.                                                       */
/*===========================================================================*/
/* RETURN:                                                                   */
/*===========================================================================*/
static void TextureSubImage( GLcontext *ctx, GLenum target, struct gl_texture_object *tObj, GLint level, GLint xoffset, GLint yoffset, GLsizei width, GLsizei height, GLint internalFormat, const struct gl_texture_image *image )
{
  D3DMESACONTEXT *pContext = (D3DMESACONTEXT *)ctx->DriverCtx;
  RECT           rect;

  /* TODO: only doing first LOD. */
  if ( (ctx->DriverCtx == NULL) || (level > 0) )
    return;

  /* Create a dirty rectangle structure. */
  rect.left   = xoffset;
  rect.right  = xoffset + width;
  rect.top    = yoffset;
  rect.bottom = yoffset + height;
  
  CreateTMgrHAL( pContext->pShared, 
			  tObj->Name, 
			  0,
			  tObj->Image[0]->Format, 
			  &rect,
			  tObj->Image[0]->Width, 
			  tObj->Image[0]->Height,
			  TM_ACTION_UPDATE,
			  (void *)tObj->Image[0]->Data );
}

