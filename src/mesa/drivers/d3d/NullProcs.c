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
#ifndef NULL_MESA_PROCS_INC
#define NULL_MESA_PROCS_INC
/*===========================================================================*/
/* Includes.                                                                 */
/*===========================================================================*/
#include "matrix.h"
#include "context.h"
#include "mtypes.h"
#include "vb.h"
/*===========================================================================*/
/* Macros.                                                                   */
/*===========================================================================*/
/*===========================================================================*/
/* Magic numbers.                                                            */
/*===========================================================================*/
/*===========================================================================*/
/* Type defines.                                                             */
/*===========================================================================*/
void NULLSetColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a );
void NULLClearColor( GLcontext *ctx, GLubyte r, GLubyte g, GLubyte b, GLubyte a );
GLboolean NULLSetBuffer( GLcontext *ctx, GLframebuffer *buffer, GLuint bit );
void NULLGetBufferSize( GLcontext *ctx, GLuint *width, GLuint *height );
GLbitfield NULLClearBuffers( GLcontext *ctx, GLbitfield m, GLboolean a, GLint x, GLint y, GLint w, GLint h );
void NULLWrSpRGB( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte r[][3], const GLubyte m[] );
void NULLWrSpRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte r[][4], const GLubyte m[] );
void NULLWrSpRGBAMono( const GLcontext* ctx, GLuint n, GLint x, GLint y, const GLubyte m[] );
void NULLWrPiRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte r[][4], const GLubyte m[] );
void NULLWrPiRGBAMono( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], const GLubyte m[] );
void NULLReSpRGBA( const GLcontext* ctx, GLuint n, GLint x, GLint y, GLubyte r[][4] );
void NULLRePiRGBA( const GLcontext* ctx, GLuint n, const GLint x[], const GLint y[], GLubyte r[][4], const GLubyte m[] );
/*===========================================================================*/
/* Extern function prototypes.                                               */
/*===========================================================================*/
/*===========================================================================*/
/* Global variables.                                                         */
/*===========================================================================*/

#endif

