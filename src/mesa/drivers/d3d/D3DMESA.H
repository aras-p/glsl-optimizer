/*===========================================================================*/
/*                                                                           */
/* Mesa-3.0 DirectX 6 Driver                                                 */
/*                                                                           */
/* By Leigh McRae                                                            */
/*                                                                           */
/* http://www.altsoftware.com/                                               */
/*                                                                           */
/* Copyright (c) 1999-1998  alt.software inc.  All Rights Reserved           */
/*===========================================================================*/
#ifndef D3D_MESA_H
#define D3D_MESA_H
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include <windows.h>
#include <ddraw.h>
#include <d3d.h>
#include "matrix.h"
#include "context.h"
#include "mtypes.h"
#include "vb.h"
#include "D3DShared.h"
#include "Debug.h"
#include "NULLProcs.h"
/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/
#define FLIP(h,y)                       (h-y)
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
struct __extensions__
{
   PROC  proc;
   char  *name;
};

typedef GLbitfield   (*ClearPROC)( GLcontext *ctx, GLbitfield mask, GLboolean all, GLint x, GLint y, GLint width, GLint height );
typedef void         (*WSpanRGBPROC)( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgb[][3], const GLubyte mask[] );
typedef void         (*WSpanRGBAPROC)( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte rgba[][4], const GLubyte mask[] );
typedef void         (*WSpanRGBAMonoPROC)( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte mask[] );
typedef void         (*WPixelsRGBAPROC)( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte rgba[][4], const GLubyte mask[] );
typedef void         (*WPixelsRGBAMonoPROC)( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte mask[] );
typedef void         (*RSpanRGBAPROC)( const GLcontext* ctx, GLuint n, GLint x, GLint y, GLubyte rgba[][4] );
typedef void         (*RPixelsRGBAPROC)( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], GLubyte rgba[][4], const GLubyte mask[] );

typedef struct D3D_mesa_context 
{
  PMESAD3DSHARED        pShared;

  GLcontext             *gl_ctx;       /* The core GL/Mesa context */
  GLvisual              *gl_visual;    /* Describes the buffers */
  GLframebuffer *gl_buffer;    /* Depth, stencil, accum, etc buffers */

  HDC                   hdc;
  WNDPROC               hOldProc;

  UCHAR                 rClear,         /* Current clear colors. */
		    gClear,
		    bClear,
		    aClear,
		    rCurrent,           /* Current rendering colors. */
		    gCurrent,
		    bCurrent,
		    aCurrent;

   struct D3D_mesa_context *next;

} D3DMESACONTEXT, *PD3DMESACONTEXT;
/*===========================================================================*/
/* Extern function prototypes.                                               */
/*===========================================================================*/
extern void gl_Viewport( GLcontext *ctx, GLint x, GLint y, GLsizei width, GLsizei height );
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/
extern D3DTLVERTEX   D3DTLVertices[(VB_MAX*6)];

#endif

