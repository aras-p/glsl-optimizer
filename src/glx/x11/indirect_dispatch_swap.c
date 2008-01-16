/* DO NOT EDIT - This file generated automatically by glX_proto_recv.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2005
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sub license,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
 * IBM,
 * AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 * WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF
 * OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <X11/Xmd.h>
#include <GL/gl.h>
#include <GL/glxproto.h>
#include <inttypes.h>
#include "indirect_size.h"
#include "indirect_size_get.h"
#include "indirect_dispatch.h"
#include "glxserver.h"
#include "glxbyteorder.h"
#include "indirect_util.h"
#include "singlesize.h"
#include "glapitable.h"
#include "glapi.h"
#include "glthread.h"
#include "dispatch.h"

#define __GLX_PAD(x)  (((x) + 3) & ~3)

typedef struct {
    __GLX_PIXEL_3D_HDR;
} __GLXpixel3DHeader;

extern GLboolean __glXErrorOccured( void );
extern void __glXClearErrorOccured( void );

static const unsigned dummy_answer[2] = {0, 0};

static GLsizei
bswap_CARD32( const void * src )
{
    union { uint32_t dst; GLsizei ret; } x;
    x.dst = bswap_32( *(uint32_t *) src );
    return x.ret;
}

static GLshort
bswap_CARD16( const void * src )
{
    union { uint16_t dst; GLshort ret; } x;
    x.dst = bswap_16( *(uint16_t *) src );
    return x.ret;
}

static GLenum
bswap_ENUM( const void * src )
{
    union { uint32_t dst; GLenum ret; } x;
    x.dst = bswap_32( *(uint32_t *) src );
    return x.ret;
}

static GLdouble
bswap_FLOAT64( const void * src )
{
    union { uint64_t dst; GLdouble ret; } x;
    x.dst = bswap_64( *(uint64_t *) src );
    return x.ret;
}

static GLfloat
bswap_FLOAT32( const void * src )
{
    union { uint32_t dst; GLfloat ret; } x;
    x.dst = bswap_32( *(uint32_t *) src );
    return x.ret;
}

static void *
bswap_16_array( uint16_t * src, unsigned count )
{
    unsigned  i;

    for ( i = 0 ; i < count ; i++ ) {
        uint16_t temp = bswap_16( src[i] );
        src[i] = temp;
    }

    return src;
}

static void *
bswap_32_array( uint32_t * src, unsigned count )
{
    unsigned  i;

    for ( i = 0 ; i < count ; i++ ) {
        uint32_t temp = bswap_32( src[i] );
        src[i] = temp;
    }

    return src;
}

static void *
bswap_64_array( uint64_t * src, unsigned count )
{
    unsigned  i;

    for ( i = 0 ; i < count ; i++ ) {
        uint64_t temp = bswap_64( src[i] );
        src[i] = temp;
    }

    return src;
}

int __glXDispSwap_NewList(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_NewList( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
             (GLenum  )bswap_ENUM   ( pc +  4 )
        ) );
        error = Success;
    }

    return error;
}

int __glXDispSwap_EndList(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_EndList( GET_DISPATCH(), () );
        error = Success;
    }

    return error;
}

void __glXDispSwap_CallList(GLbyte * pc)
{
    CALL_CallList( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_CallLists(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );
    const GLenum type =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLvoid * lists;

    switch(type) {
    case GL_BYTE:
    case GL_UNSIGNED_BYTE:
    case GL_2_BYTES:
    case GL_3_BYTES:
    case GL_4_BYTES:
        lists = (const GLvoid *) (pc + 8); break;
    case GL_SHORT:
    case GL_UNSIGNED_SHORT:
        lists = (const GLvoid *) bswap_16_array( (uint16_t *) (pc + 8), n ); break;
    case GL_INT:
    case GL_UNSIGNED_INT:
    case GL_FLOAT:
        lists = (const GLvoid *) bswap_32_array( (uint32_t *) (pc + 8), n ); break;
    default:
        return;
    }

    CALL_CallLists( GET_DISPATCH(), (
        n,
        type,
        lists
    ) );
}

int __glXDispSwap_DeleteLists(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_DeleteLists( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
             (GLsizei )bswap_CARD32 ( pc +  4 )
        ) );
        error = Success;
    }

    return error;
}

int __glXDispSwap_GenLists(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLuint retval;
        retval = CALL_GenLists( GET_DISPATCH(), (
             (GLsizei )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_ListBase(GLbyte * pc)
{
    CALL_ListBase( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_Begin(GLbyte * pc)
{
    CALL_Begin( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_Bitmap(GLbyte * pc)
{
    const GLubyte * const bitmap = (const GLubyte *) (pc + 44);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_Bitmap( GET_DISPATCH(), (
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLsizei )bswap_CARD32 ( pc + 24 ),
         (GLfloat )bswap_FLOAT32( pc + 28 ),
         (GLfloat )bswap_FLOAT32( pc + 32 ),
         (GLfloat )bswap_FLOAT32( pc + 36 ),
         (GLfloat )bswap_FLOAT32( pc + 40 ),
        bitmap
    ) );
}

void __glXDispSwap_Color3bv(GLbyte * pc)
{
    CALL_Color3bv( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDispSwap_Color3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Color3dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Color3fv(GLbyte * pc)
{
    CALL_Color3fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Color3iv(GLbyte * pc)
{
    CALL_Color3iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Color3sv(GLbyte * pc)
{
    CALL_Color3sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Color3ubv(GLbyte * pc)
{
    CALL_Color3ubv( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDispSwap_Color3uiv(GLbyte * pc)
{
    CALL_Color3uiv( GET_DISPATCH(), (
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Color3usv(GLbyte * pc)
{
    CALL_Color3usv( GET_DISPATCH(), (
         (const GLushort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Color4bv(GLbyte * pc)
{
    CALL_Color4bv( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDispSwap_Color4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Color4dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Color4fv(GLbyte * pc)
{
    CALL_Color4fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Color4iv(GLbyte * pc)
{
    CALL_Color4iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Color4sv(GLbyte * pc)
{
    CALL_Color4sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Color4ubv(GLbyte * pc)
{
    CALL_Color4ubv( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDispSwap_Color4uiv(GLbyte * pc)
{
    CALL_Color4uiv( GET_DISPATCH(), (
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Color4usv(GLbyte * pc)
{
    CALL_Color4usv( GET_DISPATCH(), (
         (const GLushort *)bswap_16_array( (uint16_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_EdgeFlagv(GLbyte * pc)
{
    CALL_EdgeFlagv( GET_DISPATCH(), (
         (const GLboolean *)(pc +  0)
    ) );
}

void __glXDispSwap_End(GLbyte * pc)
{
    CALL_End( GET_DISPATCH(), () );
}

void __glXDispSwap_Indexdv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_Indexdv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_Indexfv(GLbyte * pc)
{
    CALL_Indexfv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_Indexiv(GLbyte * pc)
{
    CALL_Indexiv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_Indexsv(GLbyte * pc)
{
    CALL_Indexsv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_Normal3bv(GLbyte * pc)
{
    CALL_Normal3bv( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDispSwap_Normal3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Normal3dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Normal3fv(GLbyte * pc)
{
    CALL_Normal3fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Normal3iv(GLbyte * pc)
{
    CALL_Normal3iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Normal3sv(GLbyte * pc)
{
    CALL_Normal3sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_RasterPos2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_RasterPos2dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_RasterPos2fv(GLbyte * pc)
{
    CALL_RasterPos2fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_RasterPos2iv(GLbyte * pc)
{
    CALL_RasterPos2iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_RasterPos2sv(GLbyte * pc)
{
    CALL_RasterPos2sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_RasterPos3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_RasterPos3dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_RasterPos3fv(GLbyte * pc)
{
    CALL_RasterPos3fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_RasterPos3iv(GLbyte * pc)
{
    CALL_RasterPos3iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_RasterPos3sv(GLbyte * pc)
{
    CALL_RasterPos3sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_RasterPos4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_RasterPos4dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_RasterPos4fv(GLbyte * pc)
{
    CALL_RasterPos4fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_RasterPos4iv(GLbyte * pc)
{
    CALL_RasterPos4iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_RasterPos4sv(GLbyte * pc)
{
    CALL_RasterPos4sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Rectdv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Rectdv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 2 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc + 16), 2 )
    ) );
}

void __glXDispSwap_Rectfv(GLbyte * pc)
{
    CALL_Rectfv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 2 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 2 )
    ) );
}

void __glXDispSwap_Rectiv(GLbyte * pc)
{
    CALL_Rectiv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 2 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  8), 2 )
    ) );
}

void __glXDispSwap_Rectsv(GLbyte * pc)
{
    CALL_Rectsv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 2 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_TexCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_TexCoord1dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_TexCoord1fv(GLbyte * pc)
{
    CALL_TexCoord1fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_TexCoord1iv(GLbyte * pc)
{
    CALL_TexCoord1iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_TexCoord1sv(GLbyte * pc)
{
    CALL_TexCoord1sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_TexCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_TexCoord2dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_TexCoord2fv(GLbyte * pc)
{
    CALL_TexCoord2fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_TexCoord2iv(GLbyte * pc)
{
    CALL_TexCoord2iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_TexCoord2sv(GLbyte * pc)
{
    CALL_TexCoord2sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_TexCoord3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_TexCoord3dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_TexCoord3fv(GLbyte * pc)
{
    CALL_TexCoord3fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_TexCoord3iv(GLbyte * pc)
{
    CALL_TexCoord3iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_TexCoord3sv(GLbyte * pc)
{
    CALL_TexCoord3sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_TexCoord4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_TexCoord4dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_TexCoord4fv(GLbyte * pc)
{
    CALL_TexCoord4fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_TexCoord4iv(GLbyte * pc)
{
    CALL_TexCoord4iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_TexCoord4sv(GLbyte * pc)
{
    CALL_TexCoord4sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Vertex2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_Vertex2dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_Vertex2fv(GLbyte * pc)
{
    CALL_Vertex2fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_Vertex2iv(GLbyte * pc)
{
    CALL_Vertex2iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_Vertex2sv(GLbyte * pc)
{
    CALL_Vertex2sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_Vertex3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Vertex3dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Vertex3fv(GLbyte * pc)
{
    CALL_Vertex3fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Vertex3iv(GLbyte * pc)
{
    CALL_Vertex3iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Vertex3sv(GLbyte * pc)
{
    CALL_Vertex3sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_Vertex4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Vertex4dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Vertex4fv(GLbyte * pc)
{
    CALL_Vertex4fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Vertex4iv(GLbyte * pc)
{
    CALL_Vertex4iv( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_Vertex4sv(GLbyte * pc)
{
    CALL_Vertex4sv( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_ClipPlane(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_ClipPlane( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 32 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_ColorMaterial(GLbyte * pc)
{
    CALL_ColorMaterial( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 )
    ) );
}

void __glXDispSwap_CullFace(GLbyte * pc)
{
    CALL_CullFace( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_Fogf(GLbyte * pc)
{
    CALL_Fogf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_Fogfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 4), __glFogfv_size(pname) );

    CALL_Fogfv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDispSwap_Fogi(GLbyte * pc)
{
    CALL_Fogi( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 )
    ) );
}

void __glXDispSwap_Fogiv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 4), __glFogiv_size(pname) );

    CALL_Fogiv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDispSwap_FrontFace(GLbyte * pc)
{
    CALL_FrontFace( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_Hint(GLbyte * pc)
{
    CALL_Hint( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 )
    ) );
}

void __glXDispSwap_Lightf(GLbyte * pc)
{
    CALL_Lightf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_Lightfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glLightfv_size(pname) );

    CALL_Lightfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_Lighti(GLbyte * pc)
{
    CALL_Lighti( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_Lightiv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glLightiv_size(pname) );

    CALL_Lightiv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_LightModelf(GLbyte * pc)
{
    CALL_LightModelf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_LightModelfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 4), __glLightModelfv_size(pname) );

    CALL_LightModelfv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDispSwap_LightModeli(GLbyte * pc)
{
    CALL_LightModeli( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 )
    ) );
}

void __glXDispSwap_LightModeliv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 4), __glLightModeliv_size(pname) );

    CALL_LightModeliv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDispSwap_LineStipple(GLbyte * pc)
{
    CALL_LineStipple( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLushort)bswap_CARD16 ( pc +  4 )
    ) );
}

void __glXDispSwap_LineWidth(GLbyte * pc)
{
    CALL_LineWidth( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 )
    ) );
}

void __glXDispSwap_Materialf(GLbyte * pc)
{
    CALL_Materialf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_Materialfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glMaterialfv_size(pname) );

    CALL_Materialfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_Materiali(GLbyte * pc)
{
    CALL_Materiali( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_Materialiv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glMaterialiv_size(pname) );

    CALL_Materialiv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_PointSize(GLbyte * pc)
{
    CALL_PointSize( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 )
    ) );
}

void __glXDispSwap_PolygonMode(GLbyte * pc)
{
    CALL_PolygonMode( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 )
    ) );
}

void __glXDispSwap_PolygonStipple(GLbyte * pc)
{
    const GLubyte * const mask = (const GLubyte *) (pc + 20);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_PolygonStipple( GET_DISPATCH(), (
        mask
    ) );
}

void __glXDispSwap_Scissor(GLbyte * pc)
{
    CALL_Scissor( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLsizei )bswap_CARD32 ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 )
    ) );
}

void __glXDispSwap_ShadeModel(GLbyte * pc)
{
    CALL_ShadeModel( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_TexParameterf(GLbyte * pc)
{
    CALL_TexParameterf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_TexParameterfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glTexParameterfv_size(pname) );

    CALL_TexParameterfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_TexParameteri(GLbyte * pc)
{
    CALL_TexParameteri( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_TexParameteriv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glTexParameteriv_size(pname) );

    CALL_TexParameteriv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_TexImage1D(GLbyte * pc)
{
    const GLvoid * const pixels = (const GLvoid *) (pc + 52);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_TexImage1D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 ),
         (GLint   )bswap_CARD32 ( pc + 28 ),
         (GLsizei )bswap_CARD32 ( pc + 32 ),
         (GLint   )bswap_CARD32 ( pc + 40 ),
         (GLenum  )bswap_ENUM   ( pc + 44 ),
         (GLenum  )bswap_ENUM   ( pc + 48 ),
        pixels
    ) );
}

void __glXDispSwap_TexImage2D(GLbyte * pc)
{
    const GLvoid * const pixels = (const GLvoid *) (pc + 52);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_TexImage2D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 ),
         (GLint   )bswap_CARD32 ( pc + 28 ),
         (GLsizei )bswap_CARD32 ( pc + 32 ),
         (GLsizei )bswap_CARD32 ( pc + 36 ),
         (GLint   )bswap_CARD32 ( pc + 40 ),
         (GLenum  )bswap_ENUM   ( pc + 44 ),
         (GLenum  )bswap_ENUM   ( pc + 48 ),
        pixels
    ) );
}

void __glXDispSwap_TexEnvf(GLbyte * pc)
{
    CALL_TexEnvf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_TexEnvfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glTexEnvfv_size(pname) );

    CALL_TexEnvfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_TexEnvi(GLbyte * pc)
{
    CALL_TexEnvi( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_TexEnviv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glTexEnviv_size(pname) );

    CALL_TexEnviv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_TexGend(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_TexGend( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLenum  )bswap_ENUM   ( pc + 12 ),
         (GLdouble)bswap_FLOAT64( pc +  0 )
    ) );
}

void __glXDispSwap_TexGendv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLdouble * params;

#ifdef __GLX_ALIGN64
    const GLuint compsize = __glTexGendv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 8)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    params = (const GLdouble *) bswap_64_array( (uint64_t *) (pc + 8), __glTexGendv_size(pname) );

    CALL_TexGendv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_TexGenf(GLbyte * pc)
{
    CALL_TexGenf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_TexGenfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glTexGenfv_size(pname) );

    CALL_TexGenfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_TexGeni(GLbyte * pc)
{
    CALL_TexGeni( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_TexGeniv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glTexGeniv_size(pname) );

    CALL_TexGeniv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_InitNames(GLbyte * pc)
{
    CALL_InitNames( GET_DISPATCH(), () );
}

void __glXDispSwap_LoadName(GLbyte * pc)
{
    CALL_LoadName( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_PassThrough(GLbyte * pc)
{
    CALL_PassThrough( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 )
    ) );
}

void __glXDispSwap_PopName(GLbyte * pc)
{
    CALL_PopName( GET_DISPATCH(), () );
}

void __glXDispSwap_PushName(GLbyte * pc)
{
    CALL_PushName( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_DrawBuffer(GLbyte * pc)
{
    CALL_DrawBuffer( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_Clear(GLbyte * pc)
{
    CALL_Clear( GET_DISPATCH(), (
         (GLbitfield)bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_ClearAccum(GLbyte * pc)
{
    CALL_ClearAccum( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 ),
         (GLfloat )bswap_FLOAT32( pc + 12 )
    ) );
}

void __glXDispSwap_ClearIndex(GLbyte * pc)
{
    CALL_ClearIndex( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 )
    ) );
}

void __glXDispSwap_ClearColor(GLbyte * pc)
{
    CALL_ClearColor( GET_DISPATCH(), (
         (GLclampf)bswap_FLOAT32( pc +  0 ),
         (GLclampf)bswap_FLOAT32( pc +  4 ),
         (GLclampf)bswap_FLOAT32( pc +  8 ),
         (GLclampf)bswap_FLOAT32( pc + 12 )
    ) );
}

void __glXDispSwap_ClearStencil(GLbyte * pc)
{
    CALL_ClearStencil( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_ClearDepth(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_ClearDepth( GET_DISPATCH(), (
         (GLclampd)bswap_FLOAT64( pc +  0 )
    ) );
}

void __glXDispSwap_StencilMask(GLbyte * pc)
{
    CALL_StencilMask( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_ColorMask(GLbyte * pc)
{
    CALL_ColorMask( GET_DISPATCH(), (
        *(GLboolean *)(pc +  0),
        *(GLboolean *)(pc +  1),
        *(GLboolean *)(pc +  2),
        *(GLboolean *)(pc +  3)
    ) );
}

void __glXDispSwap_DepthMask(GLbyte * pc)
{
    CALL_DepthMask( GET_DISPATCH(), (
        *(GLboolean *)(pc +  0)
    ) );
}

void __glXDispSwap_IndexMask(GLbyte * pc)
{
    CALL_IndexMask( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_Accum(GLbyte * pc)
{
    CALL_Accum( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_Disable(GLbyte * pc)
{
    CALL_Disable( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_Enable(GLbyte * pc)
{
    CALL_Enable( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_PopAttrib(GLbyte * pc)
{
    CALL_PopAttrib( GET_DISPATCH(), () );
}

void __glXDispSwap_PushAttrib(GLbyte * pc)
{
    CALL_PushAttrib( GET_DISPATCH(), (
         (GLbitfield)bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_MapGrid1d(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_MapGrid1d( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 )
    ) );
}

void __glXDispSwap_MapGrid1f(GLbyte * pc)
{
    CALL_MapGrid1f( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_MapGrid2d(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 40);
        pc -= 4;
    }
#endif

    CALL_MapGrid2d( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc + 32 ),
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 36 ),
         (GLdouble)bswap_FLOAT64( pc + 16 ),
         (GLdouble)bswap_FLOAT64( pc + 24 )
    ) );
}

void __glXDispSwap_MapGrid2f(GLbyte * pc)
{
    CALL_MapGrid2f( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLfloat )bswap_FLOAT32( pc + 16 ),
         (GLfloat )bswap_FLOAT32( pc + 20 )
    ) );
}

void __glXDispSwap_EvalCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_EvalCoord1dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_EvalCoord1fv(GLbyte * pc)
{
    CALL_EvalCoord1fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_EvalCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_EvalCoord2dv( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_EvalCoord2fv(GLbyte * pc)
{
    CALL_EvalCoord2fv( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_EvalMesh1(GLbyte * pc)
{
    CALL_EvalMesh1( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_EvalPoint1(GLbyte * pc)
{
    CALL_EvalPoint1( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 )
    ) );
}

void __glXDispSwap_EvalMesh2(GLbyte * pc)
{
    CALL_EvalMesh2( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 )
    ) );
}

void __glXDispSwap_EvalPoint2(GLbyte * pc)
{
    CALL_EvalPoint2( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 )
    ) );
}

void __glXDispSwap_AlphaFunc(GLbyte * pc)
{
    CALL_AlphaFunc( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLclampf)bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_BlendFunc(GLbyte * pc)
{
    CALL_BlendFunc( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 )
    ) );
}

void __glXDispSwap_LogicOp(GLbyte * pc)
{
    CALL_LogicOp( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_StencilFunc(GLbyte * pc)
{
    CALL_StencilFunc( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLuint  )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_StencilOp(GLbyte * pc)
{
    CALL_StencilOp( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 )
    ) );
}

void __glXDispSwap_DepthFunc(GLbyte * pc)
{
    CALL_DepthFunc( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_PixelZoom(GLbyte * pc)
{
    CALL_PixelZoom( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_PixelTransferf(GLbyte * pc)
{
    CALL_PixelTransferf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_PixelTransferi(GLbyte * pc)
{
    CALL_PixelTransferi( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 )
    ) );
}

int __glXDispSwap_PixelStoref(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_PixelStoref( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLfloat )bswap_FLOAT32( pc +  4 )
        ) );
        error = Success;
    }

    return error;
}

int __glXDispSwap_PixelStorei(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_PixelStorei( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLint   )bswap_CARD32 ( pc +  4 )
        ) );
        error = Success;
    }

    return error;
}

void __glXDispSwap_PixelMapfv(GLbyte * pc)
{
    const GLsizei mapsize =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_PixelMapfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        mapsize,
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_PixelMapuiv(GLbyte * pc)
{
    const GLsizei mapsize =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_PixelMapuiv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        mapsize,
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_PixelMapusv(GLbyte * pc)
{
    const GLsizei mapsize =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_PixelMapusv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        mapsize,
         (const GLushort *)bswap_16_array( (uint16_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_ReadBuffer(GLbyte * pc)
{
    CALL_ReadBuffer( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_CopyPixels(GLbyte * pc)
{
    CALL_CopyPixels( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLsizei )bswap_CARD32 ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 ),
         (GLenum  )bswap_ENUM   ( pc + 16 )
    ) );
}

void __glXDispSwap_DrawPixels(GLbyte * pc)
{
    const GLvoid * const pixels = (const GLvoid *) (pc + 36);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_DrawPixels( GET_DISPATCH(), (
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLsizei )bswap_CARD32 ( pc + 24 ),
         (GLenum  )bswap_ENUM   ( pc + 28 ),
         (GLenum  )bswap_ENUM   ( pc + 32 ),
        pixels
    ) );
}

int __glXDispSwap_GetBooleanv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetBooleanv_size(pname);
        GLboolean answerBuffer[200];
        GLboolean * params = __glXGetAnswerBuffer(cl, compsize, answerBuffer, sizeof(answerBuffer), 1);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetBooleanv( GET_DISPATCH(), (
            pname,
            params
        ) );
        __glXSendReplySwap(cl->client, params, compsize, 1, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetClipPlane(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble equation[4];
        CALL_GetClipPlane( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            equation
        ) );
        (void) bswap_64_array( (uint64_t *) equation, 4 );
        __glXSendReplySwap(cl->client, equation, 4, 8, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetDoublev(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetDoublev_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetDoublev( GET_DISPATCH(), (
            pname,
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetError(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLenum retval;
        retval = CALL_GetError( GET_DISPATCH(), () );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetFloatv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetFloatv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetFloatv( GET_DISPATCH(), (
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetIntegerv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetIntegerv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetIntegerv( GET_DISPATCH(), (
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetLightfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetLightfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetLightfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetLightiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetLightiv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetLightiv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMapdv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum target =  (GLenum  )bswap_ENUM   ( pc +  0 );
        const GLenum query =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMapdv_size(target,query);
        GLdouble answerBuffer[200];
        GLdouble * v = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (v == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMapdv( GET_DISPATCH(), (
            target,
            query,
            v
        ) );
        (void) bswap_64_array( (uint64_t *) v, compsize );
        __glXSendReplySwap(cl->client, v, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMapfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum target =  (GLenum  )bswap_ENUM   ( pc +  0 );
        const GLenum query =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMapfv_size(target,query);
        GLfloat answerBuffer[200];
        GLfloat * v = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (v == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMapfv( GET_DISPATCH(), (
            target,
            query,
            v
        ) );
        (void) bswap_32_array( (uint32_t *) v, compsize );
        __glXSendReplySwap(cl->client, v, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMapiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum target =  (GLenum  )bswap_ENUM   ( pc +  0 );
        const GLenum query =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMapiv_size(target,query);
        GLint answerBuffer[200];
        GLint * v = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (v == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMapiv( GET_DISPATCH(), (
            target,
            query,
            v
        ) );
        (void) bswap_32_array( (uint32_t *) v, compsize );
        __glXSendReplySwap(cl->client, v, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMaterialfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMaterialfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMaterialfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMaterialiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMaterialiv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMaterialiv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetPixelMapfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum map =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetPixelMapfv_size(map);
        GLfloat answerBuffer[200];
        GLfloat * values = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (values == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetPixelMapfv( GET_DISPATCH(), (
            map,
            values
        ) );
        (void) bswap_32_array( (uint32_t *) values, compsize );
        __glXSendReplySwap(cl->client, values, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetPixelMapuiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum map =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetPixelMapuiv_size(map);
        GLuint answerBuffer[200];
        GLuint * values = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (values == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetPixelMapuiv( GET_DISPATCH(), (
            map,
            values
        ) );
        (void) bswap_32_array( (uint32_t *) values, compsize );
        __glXSendReplySwap(cl->client, values, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetPixelMapusv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum map =  (GLenum  )bswap_ENUM   ( pc +  0 );

        const GLuint compsize = __glGetPixelMapusv_size(map);
        GLushort answerBuffer[200];
        GLushort * values = __glXGetAnswerBuffer(cl, compsize * 2, answerBuffer, sizeof(answerBuffer), 2);

        if (values == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetPixelMapusv( GET_DISPATCH(), (
            map,
            values
        ) );
        (void) bswap_16_array( (uint16_t *) values, compsize );
        __glXSendReplySwap(cl->client, values, compsize, 2, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexEnvfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexEnvfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexEnvfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexEnviv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexEnviv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexEnviv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexGendv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexGendv_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexGendv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexGenfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexGenfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexGenfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexGeniv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexGeniv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexGeniv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetTexParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexLevelParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  8 );

        const GLuint compsize = __glGetTexLevelParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexLevelParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLint   )bswap_CARD32 ( pc +  4 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTexLevelParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  8 );

        const GLuint compsize = __glGetTexLevelParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexLevelParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLint   )bswap_CARD32 ( pc +  4 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsEnabled(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsEnabled( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsList(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsList( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_DepthRange(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_DepthRange( GET_DISPATCH(), (
         (GLclampd)bswap_FLOAT64( pc +  0 ),
         (GLclampd)bswap_FLOAT64( pc +  8 )
    ) );
}

void __glXDispSwap_Frustum(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 48);
        pc -= 4;
    }
#endif

    CALL_Frustum( GET_DISPATCH(), (
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 ),
         (GLdouble)bswap_FLOAT64( pc + 16 ),
         (GLdouble)bswap_FLOAT64( pc + 24 ),
         (GLdouble)bswap_FLOAT64( pc + 32 ),
         (GLdouble)bswap_FLOAT64( pc + 40 )
    ) );
}

void __glXDispSwap_LoadIdentity(GLbyte * pc)
{
    CALL_LoadIdentity( GET_DISPATCH(), () );
}

void __glXDispSwap_LoadMatrixf(GLbyte * pc)
{
    CALL_LoadMatrixf( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 16 )
    ) );
}

void __glXDispSwap_LoadMatrixd(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 128);
        pc -= 4;
    }
#endif

    CALL_LoadMatrixd( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 16 )
    ) );
}

void __glXDispSwap_MatrixMode(GLbyte * pc)
{
    CALL_MatrixMode( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_MultMatrixf(GLbyte * pc)
{
    CALL_MultMatrixf( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 16 )
    ) );
}

void __glXDispSwap_MultMatrixd(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 128);
        pc -= 4;
    }
#endif

    CALL_MultMatrixd( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 16 )
    ) );
}

void __glXDispSwap_Ortho(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 48);
        pc -= 4;
    }
#endif

    CALL_Ortho( GET_DISPATCH(), (
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 ),
         (GLdouble)bswap_FLOAT64( pc + 16 ),
         (GLdouble)bswap_FLOAT64( pc + 24 ),
         (GLdouble)bswap_FLOAT64( pc + 32 ),
         (GLdouble)bswap_FLOAT64( pc + 40 )
    ) );
}

void __glXDispSwap_PopMatrix(GLbyte * pc)
{
    CALL_PopMatrix( GET_DISPATCH(), () );
}

void __glXDispSwap_PushMatrix(GLbyte * pc)
{
    CALL_PushMatrix( GET_DISPATCH(), () );
}

void __glXDispSwap_Rotated(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Rotated( GET_DISPATCH(), (
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 ),
         (GLdouble)bswap_FLOAT64( pc + 16 ),
         (GLdouble)bswap_FLOAT64( pc + 24 )
    ) );
}

void __glXDispSwap_Rotatef(GLbyte * pc)
{
    CALL_Rotatef( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 ),
         (GLfloat )bswap_FLOAT32( pc + 12 )
    ) );
}

void __glXDispSwap_Scaled(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Scaled( GET_DISPATCH(), (
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 ),
         (GLdouble)bswap_FLOAT64( pc + 16 )
    ) );
}

void __glXDispSwap_Scalef(GLbyte * pc)
{
    CALL_Scalef( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_Translated(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Translated( GET_DISPATCH(), (
         (GLdouble)bswap_FLOAT64( pc +  0 ),
         (GLdouble)bswap_FLOAT64( pc +  8 ),
         (GLdouble)bswap_FLOAT64( pc + 16 )
    ) );
}

void __glXDispSwap_Translatef(GLbyte * pc)
{
    CALL_Translatef( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_Viewport(GLbyte * pc)
{
    CALL_Viewport( GET_DISPATCH(), (
         (GLint   )bswap_CARD32 ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLsizei )bswap_CARD32 ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 )
    ) );
}

void __glXDispSwap_BindTexture(GLbyte * pc)
{
    CALL_BindTexture( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 )
    ) );
}

void __glXDispSwap_Indexubv(GLbyte * pc)
{
    CALL_Indexubv( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDispSwap_PolygonOffset(GLbyte * pc)
{
    CALL_PolygonOffset( GET_DISPATCH(), (
         (GLfloat )bswap_FLOAT32( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

int __glXDispSwap_AreTexturesResident(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean * residences = __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);
        retval = CALL_AreTexturesResident( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 ),
            residences
        ) );
        __glXSendReplySwap(cl->client, residences, n, 1, GL_TRUE, retval);
        error = Success;
    }

    return error;
}

int __glXDispSwap_AreTexturesResidentEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean * residences = __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);
        retval = CALL_AreTexturesResident( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 ),
            residences
        ) );
        __glXSendReplySwap(cl->client, residences, n, 1, GL_TRUE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_CopyTexImage1D(GLbyte * pc)
{
    CALL_CopyTexImage1D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 )
    ) );
}

void __glXDispSwap_CopyTexImage2D(GLbyte * pc)
{
    CALL_CopyTexImage2D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLsizei )bswap_CARD32 ( pc + 24 ),
         (GLint   )bswap_CARD32 ( pc + 28 )
    ) );
}

void __glXDispSwap_CopyTexSubImage1D(GLbyte * pc)
{
    CALL_CopyTexSubImage1D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 )
    ) );
}

void __glXDispSwap_CopyTexSubImage2D(GLbyte * pc)
{
    CALL_CopyTexSubImage2D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLint   )bswap_CARD32 ( pc + 20 ),
         (GLsizei )bswap_CARD32 ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 )
    ) );
}

int __glXDispSwap_DeleteTextures(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        CALL_DeleteTextures( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
        ) );
        error = Success;
    }

    return error;
}

int __glXDispSwap_DeleteTexturesEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        CALL_DeleteTextures( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
        ) );
        error = Success;
    }

    return error;
}

int __glXDispSwap_GenTextures(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLuint answerBuffer[200];
        GLuint * textures = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenTextures( GET_DISPATCH(), (
            n,
            textures
        ) );
        (void) bswap_32_array( (uint32_t *) textures, n );
        __glXSendReplySwap(cl->client, textures, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GenTexturesEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLuint answerBuffer[200];
        GLuint * textures = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenTextures( GET_DISPATCH(), (
            n,
            textures
        ) );
        (void) bswap_32_array( (uint32_t *) textures, n );
        __glXSendReplySwap(cl->client, textures, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsTexture(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsTexture( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsTextureEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsTexture( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_PrioritizeTextures(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

    CALL_PrioritizeTextures( GET_DISPATCH(), (
        n,
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 ),
         (const GLclampf *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
    ) );
}

void __glXDispSwap_TexSubImage1D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 52);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 56);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_TexSubImage1D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 ),
         (GLint   )bswap_CARD32 ( pc + 28 ),
         (GLsizei )bswap_CARD32 ( pc + 36 ),
         (GLenum  )bswap_ENUM   ( pc + 44 ),
         (GLenum  )bswap_ENUM   ( pc + 48 ),
        pixels
    ) );
}

void __glXDispSwap_TexSubImage2D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 52);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 56);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_TexSubImage2D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 ),
         (GLint   )bswap_CARD32 ( pc + 28 ),
         (GLint   )bswap_CARD32 ( pc + 32 ),
         (GLsizei )bswap_CARD32 ( pc + 36 ),
         (GLsizei )bswap_CARD32 ( pc + 40 ),
         (GLenum  )bswap_ENUM   ( pc + 44 ),
         (GLenum  )bswap_ENUM   ( pc + 48 ),
        pixels
    ) );
}

void __glXDispSwap_BlendColor(GLbyte * pc)
{
    CALL_BlendColor( GET_DISPATCH(), (
         (GLclampf)bswap_FLOAT32( pc +  0 ),
         (GLclampf)bswap_FLOAT32( pc +  4 ),
         (GLclampf)bswap_FLOAT32( pc +  8 ),
         (GLclampf)bswap_FLOAT32( pc + 12 )
    ) );
}

void __glXDispSwap_BlendEquation(GLbyte * pc)
{
    CALL_BlendEquation( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_ColorTable(GLbyte * pc)
{
    const GLvoid * const table = (const GLvoid *) (pc + 40);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_ColorTable( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLenum  )bswap_ENUM   ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 ),
         (GLenum  )bswap_ENUM   ( pc + 32 ),
         (GLenum  )bswap_ENUM   ( pc + 36 ),
        table
    ) );
}

void __glXDispSwap_ColorTableParameterfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glColorTableParameterfv_size(pname) );

    CALL_ColorTableParameterfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_ColorTableParameteriv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glColorTableParameteriv_size(pname) );

    CALL_ColorTableParameteriv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_CopyColorTable(GLbyte * pc)
{
    CALL_CopyColorTable( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 )
    ) );
}

int __glXDispSwap_GetColorTableParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetColorTableParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetColorTableParameterfvSGI(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetColorTableParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetColorTableParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetColorTableParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetColorTableParameterivSGI(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetColorTableParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void __glXDispSwap_ColorSubTable(GLbyte * pc)
{
    const GLvoid * const data = (const GLvoid *) (pc + 40);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_ColorSubTable( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLsizei )bswap_CARD32 ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 ),
         (GLenum  )bswap_ENUM   ( pc + 32 ),
         (GLenum  )bswap_ENUM   ( pc + 36 ),
        data
    ) );
}

void __glXDispSwap_CopyColorSubTable(GLbyte * pc)
{
    CALL_CopyColorSubTable( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLsizei )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 )
    ) );
}

void __glXDispSwap_ConvolutionFilter1D(GLbyte * pc)
{
    const GLvoid * const image = (const GLvoid *) (pc + 44);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_ConvolutionFilter1D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLenum  )bswap_ENUM   ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 ),
         (GLenum  )bswap_ENUM   ( pc + 36 ),
         (GLenum  )bswap_ENUM   ( pc + 40 ),
        image
    ) );
}

void __glXDispSwap_ConvolutionFilter2D(GLbyte * pc)
{
    const GLvoid * const image = (const GLvoid *) (pc + 44);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_ConvolutionFilter2D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 20 ),
         (GLenum  )bswap_ENUM   ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 ),
         (GLsizei )bswap_CARD32 ( pc + 32 ),
         (GLenum  )bswap_ENUM   ( pc + 36 ),
         (GLenum  )bswap_ENUM   ( pc + 40 ),
        image
    ) );
}

void __glXDispSwap_ConvolutionParameterf(GLbyte * pc)
{
    CALL_ConvolutionParameterf( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLfloat )bswap_FLOAT32( pc +  8 )
    ) );
}

void __glXDispSwap_ConvolutionParameterfv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 8), __glConvolutionParameterfv_size(pname) );

    CALL_ConvolutionParameterfv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_ConvolutionParameteri(GLbyte * pc)
{
    CALL_ConvolutionParameteri( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 )
    ) );
}

void __glXDispSwap_ConvolutionParameteriv(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 8), __glConvolutionParameteriv_size(pname) );

    CALL_ConvolutionParameteriv( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
        pname,
        params
    ) );
}

void __glXDispSwap_CopyConvolutionFilter1D(GLbyte * pc)
{
    CALL_CopyConvolutionFilter1D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 )
    ) );
}

void __glXDispSwap_CopyConvolutionFilter2D(GLbyte * pc)
{
    CALL_CopyConvolutionFilter2D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 )
    ) );
}

int __glXDispSwap_GetConvolutionParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetConvolutionParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetConvolutionParameterfvEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetConvolutionParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetConvolutionParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetConvolutionParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetConvolutionParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetConvolutionParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetHistogramParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetHistogramParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetHistogramParameterfvEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetHistogramParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetHistogramParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetHistogramParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetHistogramParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetHistogramParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMinmaxParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMinmaxParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMinmaxParameterfvEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMinmaxParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameterfv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMinmaxParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMinmaxParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetMinmaxParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetMinmaxParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameteriv( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void __glXDispSwap_Histogram(GLbyte * pc)
{
    CALL_Histogram( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLsizei )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
        *(GLboolean *)(pc + 12)
    ) );
}

void __glXDispSwap_Minmax(GLbyte * pc)
{
    CALL_Minmax( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
        *(GLboolean *)(pc +  8)
    ) );
}

void __glXDispSwap_ResetHistogram(GLbyte * pc)
{
    CALL_ResetHistogram( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_ResetMinmax(GLbyte * pc)
{
    CALL_ResetMinmax( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_TexImage3D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 76);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 80);
    __GLXpixel3DHeader * const hdr = (__GLXpixel3DHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_IMAGE_HEIGHT, (GLint) bswap_CARD32( & hdr->imageHeight )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_IMAGES,  (GLint) bswap_CARD32( & hdr->skipImages )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_TexImage3D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 36 ),
         (GLint   )bswap_CARD32 ( pc + 40 ),
         (GLint   )bswap_CARD32 ( pc + 44 ),
         (GLsizei )bswap_CARD32 ( pc + 48 ),
         (GLsizei )bswap_CARD32 ( pc + 52 ),
         (GLsizei )bswap_CARD32 ( pc + 56 ),
         (GLint   )bswap_CARD32 ( pc + 64 ),
         (GLenum  )bswap_ENUM   ( pc + 68 ),
         (GLenum  )bswap_ENUM   ( pc + 72 ),
        pixels
    ) );
}

void __glXDispSwap_TexSubImage3D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 84);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 88);
    __GLXpixel3DHeader * const hdr = (__GLXpixel3DHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) bswap_CARD32( & hdr->rowLength )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_IMAGE_HEIGHT, (GLint) bswap_CARD32( & hdr->imageHeight )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) bswap_CARD32( & hdr->skipRows )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_IMAGES,  (GLint) bswap_CARD32( & hdr->skipImages )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) bswap_CARD32( & hdr->skipPixels )) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) bswap_CARD32( & hdr->alignment )) );

    CALL_TexSubImage3D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 36 ),
         (GLint   )bswap_CARD32 ( pc + 40 ),
         (GLint   )bswap_CARD32 ( pc + 44 ),
         (GLint   )bswap_CARD32 ( pc + 48 ),
         (GLint   )bswap_CARD32 ( pc + 52 ),
         (GLsizei )bswap_CARD32 ( pc + 60 ),
         (GLsizei )bswap_CARD32 ( pc + 64 ),
         (GLsizei )bswap_CARD32 ( pc + 68 ),
         (GLenum  )bswap_ENUM   ( pc + 76 ),
         (GLenum  )bswap_ENUM   ( pc + 80 ),
        pixels
    ) );
}

void __glXDispSwap_CopyTexSubImage3D(GLbyte * pc)
{
    CALL_CopyTexSubImage3D( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLint   )bswap_CARD32 ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 ),
         (GLsizei )bswap_CARD32 ( pc + 32 )
    ) );
}

void __glXDispSwap_ActiveTextureARB(GLbyte * pc)
{
    CALL_ActiveTextureARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_MultiTexCoord1dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 12);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord1dvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_MultiTexCoord1fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord1fvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_MultiTexCoord1ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord1ivARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_MultiTexCoord1svARB(GLbyte * pc)
{
    CALL_MultiTexCoord1svARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_MultiTexCoord2dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord2dvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 16 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 2 )
    ) );
}

void __glXDispSwap_MultiTexCoord2fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord2fvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_MultiTexCoord2ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord2ivARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_MultiTexCoord2svARB(GLbyte * pc)
{
    CALL_MultiTexCoord2svARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_MultiTexCoord3dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 28);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord3dvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 24 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_MultiTexCoord3fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord3fvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_MultiTexCoord3ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord3ivARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_MultiTexCoord3svARB(GLbyte * pc)
{
    CALL_MultiTexCoord3svARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_MultiTexCoord4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord4dvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc + 32 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_MultiTexCoord4fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord4fvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_MultiTexCoord4ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord4ivARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_MultiTexCoord4svARB(GLbyte * pc)
{
    CALL_MultiTexCoord4svARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_SampleCoverageARB(GLbyte * pc)
{
    CALL_SampleCoverageARB( GET_DISPATCH(), (
         (GLclampf)bswap_FLOAT32( pc +  0 ),
        *(GLboolean *)(pc +  4)
    ) );
}

void __glXDispSwap_CompressedTexImage1DARB(GLbyte * pc)
{
    const GLsizei imageSize =  (GLsizei )bswap_CARD32 ( pc + 20 );

    CALL_CompressedTexImage1DARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
        imageSize,
         (const GLvoid *)(pc + 24)
    ) );
}

void __glXDispSwap_CompressedTexImage2DARB(GLbyte * pc)
{
    const GLsizei imageSize =  (GLsizei )bswap_CARD32 ( pc + 24 );

    CALL_CompressedTexImage2DARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 ),
         (GLint   )bswap_CARD32 ( pc + 20 ),
        imageSize,
         (const GLvoid *)(pc + 28)
    ) );
}

void __glXDispSwap_CompressedTexImage3DARB(GLbyte * pc)
{
    const GLsizei imageSize =  (GLsizei )bswap_CARD32 ( pc + 28 );

    CALL_CompressedTexImage3DARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLint   )bswap_CARD32 ( pc + 24 ),
        imageSize,
         (const GLvoid *)(pc + 32)
    ) );
}

void __glXDispSwap_CompressedTexSubImage1DARB(GLbyte * pc)
{
    const GLsizei imageSize =  (GLsizei )bswap_CARD32 ( pc + 20 );

    CALL_CompressedTexSubImage1DARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 ),
         (GLenum  )bswap_ENUM   ( pc + 16 ),
        imageSize,
         (const GLvoid *)(pc + 24)
    ) );
}

void __glXDispSwap_CompressedTexSubImage2DARB(GLbyte * pc)
{
    const GLsizei imageSize =  (GLsizei )bswap_CARD32 ( pc + 28 );

    CALL_CompressedTexSubImage2DARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLsizei )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLenum  )bswap_ENUM   ( pc + 24 ),
        imageSize,
         (const GLvoid *)(pc + 32)
    ) );
}

void __glXDispSwap_CompressedTexSubImage3DARB(GLbyte * pc)
{
    const GLsizei imageSize =  (GLsizei )bswap_CARD32 ( pc + 36 );

    CALL_CompressedTexSubImage3DARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 ),
         (GLint   )bswap_CARD32 ( pc +  8 ),
         (GLint   )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLsizei )bswap_CARD32 ( pc + 20 ),
         (GLsizei )bswap_CARD32 ( pc + 24 ),
         (GLsizei )bswap_CARD32 ( pc + 28 ),
         (GLenum  )bswap_ENUM   ( pc + 32 ),
        imageSize,
         (const GLvoid *)(pc + 40)
    ) );
}

int __glXDispSwap_GetProgramEnvParameterdvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble params[4];
        CALL_GetProgramEnvParameterdvARB( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramEnvParameterfvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLfloat params[4];
        CALL_GetProgramEnvParameterfvARB( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramLocalParameterdvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble params[4];
        CALL_GetProgramLocalParameterdvARB( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramLocalParameterfvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLfloat params[4];
        CALL_GetProgramLocalParameterfvARB( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetProgramivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetProgramivARB( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetVertexAttribdvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetVertexAttribdvARB_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribdvARB( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetVertexAttribfvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetVertexAttribfvARB_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribfvARB( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetVertexAttribivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetVertexAttribivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribivARB( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void __glXDispSwap_ProgramEnvParameter4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 40);
        pc -= 4;
    }
#endif

    CALL_ProgramEnvParameter4dvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  8), 4 )
    ) );
}

void __glXDispSwap_ProgramEnvParameter4fvARB(GLbyte * pc)
{
    CALL_ProgramEnvParameter4fvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 4 )
    ) );
}

void __glXDispSwap_ProgramLocalParameter4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 40);
        pc -= 4;
    }
#endif

    CALL_ProgramLocalParameter4dvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  8), 4 )
    ) );
}

void __glXDispSwap_ProgramLocalParameter4fvARB(GLbyte * pc)
{
    CALL_ProgramLocalParameter4fvARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 4 )
    ) );
}

void __glXDispSwap_ProgramStringARB(GLbyte * pc)
{
    const GLsizei len =  (GLsizei )bswap_CARD32 ( pc +  8 );

    CALL_ProgramStringARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
        len,
         (const GLvoid *)(pc + 12)
    ) );
}

void __glXDispSwap_VertexAttrib1dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 12);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib1dvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_VertexAttrib1fvARB(GLbyte * pc)
{
    CALL_VertexAttrib1fvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_VertexAttrib1svARB(GLbyte * pc)
{
    CALL_VertexAttrib1svARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_VertexAttrib2dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib2dvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_VertexAttrib2fvARB(GLbyte * pc)
{
    CALL_VertexAttrib2fvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_VertexAttrib2svARB(GLbyte * pc)
{
    CALL_VertexAttrib2svARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_VertexAttrib3dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 28);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib3dvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_VertexAttrib3fvARB(GLbyte * pc)
{
    CALL_VertexAttrib3fvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_VertexAttrib3svARB(GLbyte * pc)
{
    CALL_VertexAttrib3svARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_VertexAttrib4NbvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NbvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLbyte *)(pc +  4)
    ) );
}

void __glXDispSwap_VertexAttrib4NivARB(GLbyte * pc)
{
    CALL_VertexAttrib4NivARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4NsvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NsvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4NubvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NubvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLubyte *)(pc +  4)
    ) );
}

void __glXDispSwap_VertexAttrib4NuivARB(GLbyte * pc)
{
    CALL_VertexAttrib4NuivARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4NusvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NusvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLushort *)bswap_16_array( (uint16_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4bvARB(GLbyte * pc)
{
    CALL_VertexAttrib4bvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLbyte *)(pc +  4)
    ) );
}

void __glXDispSwap_VertexAttrib4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib4dvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4fvARB(GLbyte * pc)
{
    CALL_VertexAttrib4fvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4ivARB(GLbyte * pc)
{
    CALL_VertexAttrib4ivARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4svARB(GLbyte * pc)
{
    CALL_VertexAttrib4svARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4ubvARB(GLbyte * pc)
{
    CALL_VertexAttrib4ubvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLubyte *)(pc +  4)
    ) );
}

void __glXDispSwap_VertexAttrib4uivARB(GLbyte * pc)
{
    CALL_VertexAttrib4uivARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4usvARB(GLbyte * pc)
{
    CALL_VertexAttrib4usvARB( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLushort *)bswap_16_array( (uint16_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_BeginQueryARB(GLbyte * pc)
{
    CALL_BeginQueryARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 )
    ) );
}

int __glXDispSwap_DeleteQueriesARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        CALL_DeleteQueriesARB( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
        ) );
        error = Success;
    }

    return error;
}

void __glXDispSwap_EndQueryARB(GLbyte * pc)
{
    CALL_EndQueryARB( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

int __glXDispSwap_GenQueriesARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLuint answerBuffer[200];
        GLuint * ids = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenQueriesARB( GET_DISPATCH(), (
            n,
            ids
        ) );
        (void) bswap_32_array( (uint32_t *) ids, n );
        __glXSendReplySwap(cl->client, ids, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetQueryObjectivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetQueryObjectivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetQueryObjectivARB( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetQueryObjectuivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetQueryObjectuivARB_size(pname);
        GLuint answerBuffer[200];
        GLuint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetQueryObjectuivARB( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetQueryivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetQueryivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetQueryivARB( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsQueryARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsQueryARB( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_DrawBuffersARB(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

    CALL_DrawBuffersARB( GET_DISPATCH(), (
        n,
         (const GLenum *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
    ) );
}

void __glXDispSwap_SampleMaskSGIS(GLbyte * pc)
{
    CALL_SampleMaskSGIS( GET_DISPATCH(), (
         (GLclampf)bswap_FLOAT32( pc +  0 ),
        *(GLboolean *)(pc +  4)
    ) );
}

void __glXDispSwap_SamplePatternSGIS(GLbyte * pc)
{
    CALL_SamplePatternSGIS( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

void __glXDispSwap_PointParameterfEXT(GLbyte * pc)
{
    CALL_PointParameterfEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLfloat )bswap_FLOAT32( pc +  4 )
    ) );
}

void __glXDispSwap_PointParameterfvEXT(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );
    const GLfloat * params;

    params = (const GLfloat *) bswap_32_array( (uint32_t *) (pc + 4), __glPointParameterfvEXT_size(pname) );

    CALL_PointParameterfvEXT( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDispSwap_SecondaryColor3bvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3bvEXT( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDispSwap_SecondaryColor3dvEXT(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_SecondaryColor3dvEXT( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_SecondaryColor3fvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3fvEXT( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_SecondaryColor3ivEXT(GLbyte * pc)
{
    CALL_SecondaryColor3ivEXT( GET_DISPATCH(), (
         (const GLint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_SecondaryColor3svEXT(GLbyte * pc)
{
    CALL_SecondaryColor3svEXT( GET_DISPATCH(), (
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_SecondaryColor3ubvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3ubvEXT( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDispSwap_SecondaryColor3uivEXT(GLbyte * pc)
{
    CALL_SecondaryColor3uivEXT( GET_DISPATCH(), (
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_SecondaryColor3usvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3usvEXT( GET_DISPATCH(), (
         (const GLushort *)bswap_16_array( (uint16_t *) (pc +  0), 3 )
    ) );
}

void __glXDispSwap_FogCoorddvEXT(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_FogCoorddvEXT( GET_DISPATCH(), (
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_FogCoordfvEXT(GLbyte * pc)
{
    CALL_FogCoordfvEXT( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 1 )
    ) );
}

void __glXDispSwap_BlendFuncSeparateEXT(GLbyte * pc)
{
    CALL_BlendFuncSeparateEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLenum  )bswap_ENUM   ( pc + 12 )
    ) );
}

void __glXDispSwap_WindowPos3fvMESA(GLbyte * pc)
{
    CALL_WindowPos3fvMESA( GET_DISPATCH(), (
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  0), 3 )
    ) );
}

int __glXDispSwap_AreProgramsResidentNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean * residences = __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);
        retval = CALL_AreProgramsResidentNV( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 ),
            residences
        ) );
        __glXSendReplySwap(cl->client, residences, n, 1, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_BindProgramNV(GLbyte * pc)
{
    CALL_BindProgramNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 )
    ) );
}

int __glXDispSwap_DeleteProgramsNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        CALL_DeleteProgramsNV( GET_DISPATCH(), (
            n,
             (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
        ) );
        error = Success;
    }

    return error;
}

void __glXDispSwap_ExecuteProgramNV(GLbyte * pc)
{
    CALL_ExecuteProgramNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 4 )
    ) );
}

int __glXDispSwap_GenProgramsNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLuint answerBuffer[200];
        GLuint * programs = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenProgramsNV( GET_DISPATCH(), (
            n,
            programs
        ) );
        (void) bswap_32_array( (uint32_t *) programs, n );
        __glXSendReplySwap(cl->client, programs, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramParameterdvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble params[4];
        CALL_GetProgramParameterdvNV( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
             (GLenum  )bswap_ENUM   ( pc +  8 ),
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramParameterfvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLfloat params[4];
        CALL_GetProgramParameterfvNV( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
             (GLenum  )bswap_ENUM   ( pc +  8 ),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramivNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetProgramivNV_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetProgramivNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetTrackMatrixivNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLint params[1];
        CALL_GetTrackMatrixivNV( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLuint  )bswap_CARD32 ( pc +  4 ),
             (GLenum  )bswap_ENUM   ( pc +  8 ),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 1 );
        __glXSendReplySwap(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetVertexAttribdvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetVertexAttribdvNV_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribdvNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetVertexAttribfvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetVertexAttribfvNV_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribfvNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetVertexAttribivNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  4 );

        const GLuint compsize = __glGetVertexAttribivNV_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribivNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            pname,
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, compsize );
        __glXSendReplySwap(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsProgramNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsProgramNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_LoadProgramNV(GLbyte * pc)
{
    const GLsizei len =  (GLsizei )bswap_CARD32 ( pc +  8 );

    CALL_LoadProgramNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
        len,
         (const GLubyte *)(pc + 12)
    ) );
}

void __glXDispSwap_ProgramParameters4dvNV(GLbyte * pc)
{
    const GLuint num =  (GLuint  )bswap_CARD32 ( pc +  8 );

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 16 + __GLX_PAD((num * 32)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_ProgramParameters4dvNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
        num,
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc + 12), 0 )
    ) );
}

void __glXDispSwap_ProgramParameters4fvNV(GLbyte * pc)
{
    const GLuint num =  (GLuint  )bswap_CARD32 ( pc +  8 );

    CALL_ProgramParameters4fvNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
        num,
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc + 12), 0 )
    ) );
}

void __glXDispSwap_RequestResidentProgramsNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

    CALL_RequestResidentProgramsNV( GET_DISPATCH(), (
        n,
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
    ) );
}

void __glXDispSwap_TrackMatrixNV(GLbyte * pc)
{
    CALL_TrackMatrixNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLenum  )bswap_ENUM   ( pc + 12 )
    ) );
}

void __glXDispSwap_VertexAttrib1dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 12);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib1dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_VertexAttrib1fvNV(GLbyte * pc)
{
    CALL_VertexAttrib1fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_VertexAttrib1svNV(GLbyte * pc)
{
    CALL_VertexAttrib1svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 1 )
    ) );
}

void __glXDispSwap_VertexAttrib2dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib2dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_VertexAttrib2fvNV(GLbyte * pc)
{
    CALL_VertexAttrib2fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_VertexAttrib2svNV(GLbyte * pc)
{
    CALL_VertexAttrib2svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 2 )
    ) );
}

void __glXDispSwap_VertexAttrib3dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 28);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib3dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_VertexAttrib3fvNV(GLbyte * pc)
{
    CALL_VertexAttrib3fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_VertexAttrib3svNV(GLbyte * pc)
{
    CALL_VertexAttrib3svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 3 )
    ) );
}

void __glXDispSwap_VertexAttrib4dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib4dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4fvNV(GLbyte * pc)
{
    CALL_VertexAttrib4fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4svNV(GLbyte * pc)
{
    CALL_VertexAttrib4svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  4), 4 )
    ) );
}

void __glXDispSwap_VertexAttrib4ubvNV(GLbyte * pc)
{
    CALL_VertexAttrib4ubvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
         (const GLubyte *)(pc +  4)
    ) );
}

void __glXDispSwap_VertexAttribs1dvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 8)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs1dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs1fvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs1fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs1svNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs1svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs2dvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 16)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs2dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs2fvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs2fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs2svNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs2svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs3dvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 24)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs3dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs3fvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs3fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs3svNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs3svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs4dvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 32)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs4dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs4fvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs4fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs4svNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs4svNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLshort *)bswap_16_array( (uint16_t *) (pc +  8), 0 )
    ) );
}

void __glXDispSwap_VertexAttribs4ubvNV(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_VertexAttribs4ubvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        n,
         (const GLubyte *)(pc +  8)
    ) );
}

void __glXDispSwap_PointParameteriNV(GLbyte * pc)
{
    CALL_PointParameteriNV( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLint   )bswap_CARD32 ( pc +  4 )
    ) );
}

void __glXDispSwap_PointParameterivNV(GLbyte * pc)
{
    const GLenum pname =  (GLenum  )bswap_ENUM   ( pc +  0 );
    const GLint * params;

    params = (const GLint *) bswap_32_array( (uint32_t *) (pc + 4), __glPointParameterivNV_size(pname) );

    CALL_PointParameterivNV( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDispSwap_ActiveStencilFaceEXT(GLbyte * pc)
{
    CALL_ActiveStencilFaceEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

int __glXDispSwap_GetProgramNamedParameterdvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei len =  (GLsizei )bswap_CARD32 ( pc +  4 );

        GLdouble params[4];
        CALL_GetProgramNamedParameterdvNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            len,
             (const GLubyte *)(pc +  8),
            params
        ) );
        (void) bswap_64_array( (uint64_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 8, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetProgramNamedParameterfvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei len =  (GLsizei )bswap_CARD32 ( pc +  4 );

        GLfloat params[4];
        CALL_GetProgramNamedParameterfvNV( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 ),
            len,
             (const GLubyte *)(pc +  8),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 4 );
        __glXSendReplySwap(cl->client, params, 4, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

void __glXDispSwap_ProgramNamedParameter4dvNV(GLbyte * pc)
{
    const GLsizei len =  (GLsizei )bswap_CARD32 ( pc + 36 );

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 44 + __GLX_PAD(len) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_ProgramNamedParameter4dvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc + 32 ),
        len,
         (const GLubyte *)(pc + 40),
         (const GLdouble *)bswap_64_array( (uint64_t *) (pc +  0), 4 )
    ) );
}

void __glXDispSwap_ProgramNamedParameter4fvNV(GLbyte * pc)
{
    const GLsizei len =  (GLsizei )bswap_CARD32 ( pc +  4 );

    CALL_ProgramNamedParameter4fvNV( GET_DISPATCH(), (
         (GLuint  )bswap_CARD32 ( pc +  0 ),
        len,
         (const GLubyte *)(pc + 24),
         (const GLfloat *)bswap_32_array( (uint32_t *) (pc +  8), 4 )
    ) );
}

void __glXDispSwap_BlendEquationSeparateEXT(GLbyte * pc)
{
    CALL_BlendEquationSeparateEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 )
    ) );
}

void __glXDispSwap_BindFramebufferEXT(GLbyte * pc)
{
    CALL_BindFramebufferEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 )
    ) );
}

void __glXDispSwap_BindRenderbufferEXT(GLbyte * pc)
{
    CALL_BindRenderbufferEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLuint  )bswap_CARD32 ( pc +  4 )
    ) );
}

int __glXDispSwap_CheckFramebufferStatusEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLenum retval;
        retval = CALL_CheckFramebufferStatusEXT( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_DeleteFramebuffersEXT(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

    CALL_DeleteFramebuffersEXT( GET_DISPATCH(), (
        n,
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
    ) );
}

void __glXDispSwap_DeleteRenderbuffersEXT(GLbyte * pc)
{
    const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

    CALL_DeleteRenderbuffersEXT( GET_DISPATCH(), (
        n,
         (const GLuint *)bswap_32_array( (uint32_t *) (pc +  4), 0 )
    ) );
}

void __glXDispSwap_FramebufferRenderbufferEXT(GLbyte * pc)
{
    CALL_FramebufferRenderbufferEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLuint  )bswap_CARD32 ( pc + 12 )
    ) );
}

void __glXDispSwap_FramebufferTexture1DEXT(GLbyte * pc)
{
    CALL_FramebufferTexture1DEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLuint  )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 )
    ) );
}

void __glXDispSwap_FramebufferTexture2DEXT(GLbyte * pc)
{
    CALL_FramebufferTexture2DEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLuint  )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 )
    ) );
}

void __glXDispSwap_FramebufferTexture3DEXT(GLbyte * pc)
{
    CALL_FramebufferTexture3DEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLenum  )bswap_ENUM   ( pc +  8 ),
         (GLuint  )bswap_CARD32 ( pc + 12 ),
         (GLint   )bswap_CARD32 ( pc + 16 ),
         (GLint   )bswap_CARD32 ( pc + 20 )
    ) );
}

int __glXDispSwap_GenFramebuffersEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLuint answerBuffer[200];
        GLuint * framebuffers = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenFramebuffersEXT( GET_DISPATCH(), (
            n,
            framebuffers
        ) );
        (void) bswap_32_array( (uint32_t *) framebuffers, n );
        __glXSendReplySwap(cl->client, framebuffers, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GenRenderbuffersEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n =  (GLsizei )bswap_CARD32 ( pc +  0 );

        GLuint answerBuffer[200];
        GLuint * renderbuffers = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenRenderbuffersEXT( GET_DISPATCH(), (
            n,
            renderbuffers
        ) );
        (void) bswap_32_array( (uint32_t *) renderbuffers, n );
        __glXSendReplySwap(cl->client, renderbuffers, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

void __glXDispSwap_GenerateMipmapEXT(GLbyte * pc)
{
    CALL_GenerateMipmapEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 )
    ) );
}

int __glXDispSwap_GetFramebufferAttachmentParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLint params[1];
        CALL_GetFramebufferAttachmentParameterivEXT( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLenum  )bswap_ENUM   ( pc +  4 ),
             (GLenum  )bswap_ENUM   ( pc +  8 ),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 1 );
        __glXSendReplySwap(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_GetRenderbufferParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLint params[1];
        CALL_GetRenderbufferParameterivEXT( GET_DISPATCH(), (
             (GLenum  )bswap_ENUM   ( pc +  0 ),
             (GLenum  )bswap_ENUM   ( pc +  4 ),
            params
        ) );
        (void) bswap_32_array( (uint32_t *) params, 1 );
        __glXSendReplySwap(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsFramebufferEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsFramebufferEXT( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDispSwap_IsRenderbufferEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, bswap_CARD32( &req->contextTag ), &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsRenderbufferEXT( GET_DISPATCH(), (
             (GLuint  )bswap_CARD32 ( pc +  0 )
        ) );
        __glXSendReplySwap(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDispSwap_RenderbufferStorageEXT(GLbyte * pc)
{
    CALL_RenderbufferStorageEXT( GET_DISPATCH(), (
         (GLenum  )bswap_ENUM   ( pc +  0 ),
         (GLenum  )bswap_ENUM   ( pc +  4 ),
         (GLsizei )bswap_CARD32 ( pc +  8 ),
         (GLsizei )bswap_CARD32 ( pc + 12 )
    ) );
}

