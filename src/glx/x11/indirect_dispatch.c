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

int __glXDisp_NewList(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_NewList( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            *(GLenum   *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

int __glXDisp_EndList(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_EndList( GET_DISPATCH(), () );
        error = Success;
    }

    return error;
}

void __glXDisp_CallList(GLbyte * pc)
{
    CALL_CallList( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0)
    ) );
}

void __glXDisp_CallLists(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  0);
    const GLenum type = *(GLenum   *)(pc +  4);
    const GLvoid * lists =  (const GLvoid *)(pc +  8);

    lists = (const GLvoid *) (pc + 8);

    CALL_CallLists( GET_DISPATCH(), (
        n,
        type,
        lists
    ) );
}

int __glXDisp_DeleteLists(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_DeleteLists( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            *(GLsizei  *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

int __glXDisp_GenLists(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLuint retval;
        retval = CALL_GenLists( GET_DISPATCH(), (
            *(GLsizei  *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_ListBase(GLbyte * pc)
{
    CALL_ListBase( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0)
    ) );
}

void __glXDisp_Begin(GLbyte * pc)
{
    CALL_Begin( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_Bitmap(GLbyte * pc)
{
    const GLubyte * const bitmap = (const GLubyte *) (pc + 44);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_Bitmap( GET_DISPATCH(), (
        *(GLsizei  *)(pc + 20),
        *(GLsizei  *)(pc + 24),
        *(GLfloat  *)(pc + 28),
        *(GLfloat  *)(pc + 32),
        *(GLfloat  *)(pc + 36),
        *(GLfloat  *)(pc + 40),
        bitmap
    ) );
}

void __glXDisp_Color3bv(GLbyte * pc)
{
    CALL_Color3bv( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDisp_Color3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Color3dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Color3fv(GLbyte * pc)
{
    CALL_Color3fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Color3iv(GLbyte * pc)
{
    CALL_Color3iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Color3sv(GLbyte * pc)
{
    CALL_Color3sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Color3ubv(GLbyte * pc)
{
    CALL_Color3ubv( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDisp_Color3uiv(GLbyte * pc)
{
    CALL_Color3uiv( GET_DISPATCH(), (
         (const GLuint *)(pc +  0)
    ) );
}

void __glXDisp_Color3usv(GLbyte * pc)
{
    CALL_Color3usv( GET_DISPATCH(), (
         (const GLushort *)(pc +  0)
    ) );
}

void __glXDisp_Color4bv(GLbyte * pc)
{
    CALL_Color4bv( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDisp_Color4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Color4dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Color4fv(GLbyte * pc)
{
    CALL_Color4fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Color4iv(GLbyte * pc)
{
    CALL_Color4iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Color4sv(GLbyte * pc)
{
    CALL_Color4sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Color4ubv(GLbyte * pc)
{
    CALL_Color4ubv( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDisp_Color4uiv(GLbyte * pc)
{
    CALL_Color4uiv( GET_DISPATCH(), (
         (const GLuint *)(pc +  0)
    ) );
}

void __glXDisp_Color4usv(GLbyte * pc)
{
    CALL_Color4usv( GET_DISPATCH(), (
         (const GLushort *)(pc +  0)
    ) );
}

void __glXDisp_EdgeFlagv(GLbyte * pc)
{
    CALL_EdgeFlagv( GET_DISPATCH(), (
         (const GLboolean *)(pc +  0)
    ) );
}

void __glXDisp_End(GLbyte * pc)
{
    CALL_End( GET_DISPATCH(), () );
}

void __glXDisp_Indexdv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_Indexdv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Indexfv(GLbyte * pc)
{
    CALL_Indexfv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Indexiv(GLbyte * pc)
{
    CALL_Indexiv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Indexsv(GLbyte * pc)
{
    CALL_Indexsv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Normal3bv(GLbyte * pc)
{
    CALL_Normal3bv( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDisp_Normal3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Normal3dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Normal3fv(GLbyte * pc)
{
    CALL_Normal3fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Normal3iv(GLbyte * pc)
{
    CALL_Normal3iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Normal3sv(GLbyte * pc)
{
    CALL_Normal3sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_RasterPos2dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos2fv(GLbyte * pc)
{
    CALL_RasterPos2fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos2iv(GLbyte * pc)
{
    CALL_RasterPos2iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos2sv(GLbyte * pc)
{
    CALL_RasterPos2sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_RasterPos3dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos3fv(GLbyte * pc)
{
    CALL_RasterPos3fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos3iv(GLbyte * pc)
{
    CALL_RasterPos3iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos3sv(GLbyte * pc)
{
    CALL_RasterPos3sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_RasterPos4dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos4fv(GLbyte * pc)
{
    CALL_RasterPos4fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos4iv(GLbyte * pc)
{
    CALL_RasterPos4iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_RasterPos4sv(GLbyte * pc)
{
    CALL_RasterPos4sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Rectdv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Rectdv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0),
         (const GLdouble *)(pc + 16)
    ) );
}

void __glXDisp_Rectfv(GLbyte * pc)
{
    CALL_Rectfv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0),
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_Rectiv(GLbyte * pc)
{
    CALL_Rectiv( GET_DISPATCH(), (
         (const GLint *)(pc +  0),
         (const GLint *)(pc +  8)
    ) );
}

void __glXDisp_Rectsv(GLbyte * pc)
{
    CALL_Rectsv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_TexCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_TexCoord1dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord1fv(GLbyte * pc)
{
    CALL_TexCoord1fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord1iv(GLbyte * pc)
{
    CALL_TexCoord1iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord1sv(GLbyte * pc)
{
    CALL_TexCoord1sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_TexCoord2dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord2fv(GLbyte * pc)
{
    CALL_TexCoord2fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord2iv(GLbyte * pc)
{
    CALL_TexCoord2iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord2sv(GLbyte * pc)
{
    CALL_TexCoord2sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_TexCoord3dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord3fv(GLbyte * pc)
{
    CALL_TexCoord3fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord3iv(GLbyte * pc)
{
    CALL_TexCoord3iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord3sv(GLbyte * pc)
{
    CALL_TexCoord3sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_TexCoord4dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord4fv(GLbyte * pc)
{
    CALL_TexCoord4fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord4iv(GLbyte * pc)
{
    CALL_TexCoord4iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_TexCoord4sv(GLbyte * pc)
{
    CALL_TexCoord4sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Vertex2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_Vertex2dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Vertex2fv(GLbyte * pc)
{
    CALL_Vertex2fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Vertex2iv(GLbyte * pc)
{
    CALL_Vertex2iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Vertex2sv(GLbyte * pc)
{
    CALL_Vertex2sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Vertex3dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Vertex3dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Vertex3fv(GLbyte * pc)
{
    CALL_Vertex3fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Vertex3iv(GLbyte * pc)
{
    CALL_Vertex3iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Vertex3sv(GLbyte * pc)
{
    CALL_Vertex3sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_Vertex4dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Vertex4dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Vertex4fv(GLbyte * pc)
{
    CALL_Vertex4fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_Vertex4iv(GLbyte * pc)
{
    CALL_Vertex4iv( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_Vertex4sv(GLbyte * pc)
{
    CALL_Vertex4sv( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_ClipPlane(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_ClipPlane( GET_DISPATCH(), (
        *(GLenum   *)(pc + 32),
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_ColorMaterial(GLbyte * pc)
{
    CALL_ColorMaterial( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4)
    ) );
}

void __glXDisp_CullFace(GLbyte * pc)
{
    CALL_CullFace( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_Fogf(GLbyte * pc)
{
    CALL_Fogf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

void __glXDisp_Fogfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  0);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 4);

    CALL_Fogfv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDisp_Fogi(GLbyte * pc)
{
    CALL_Fogi( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4)
    ) );
}

void __glXDisp_Fogiv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  0);
    const GLint * params;

    params = (const GLint *) (pc + 4);

    CALL_Fogiv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDisp_FrontFace(GLbyte * pc)
{
    CALL_FrontFace( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_Hint(GLbyte * pc)
{
    CALL_Hint( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4)
    ) );
}

void __glXDisp_Lightf(GLbyte * pc)
{
    CALL_Lightf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_Lightfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_Lightfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_Lighti(GLbyte * pc)
{
    CALL_Lighti( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_Lightiv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_Lightiv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_LightModelf(GLbyte * pc)
{
    CALL_LightModelf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

void __glXDisp_LightModelfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  0);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 4);

    CALL_LightModelfv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDisp_LightModeli(GLbyte * pc)
{
    CALL_LightModeli( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4)
    ) );
}

void __glXDisp_LightModeliv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  0);
    const GLint * params;

    params = (const GLint *) (pc + 4);

    CALL_LightModeliv( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDisp_LineStipple(GLbyte * pc)
{
    CALL_LineStipple( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLushort *)(pc +  4)
    ) );
}

void __glXDisp_LineWidth(GLbyte * pc)
{
    CALL_LineWidth( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0)
    ) );
}

void __glXDisp_Materialf(GLbyte * pc)
{
    CALL_Materialf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_Materialfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_Materialfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_Materiali(GLbyte * pc)
{
    CALL_Materiali( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_Materialiv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_Materialiv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_PointSize(GLbyte * pc)
{
    CALL_PointSize( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0)
    ) );
}

void __glXDisp_PolygonMode(GLbyte * pc)
{
    CALL_PolygonMode( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4)
    ) );
}

void __glXDisp_PolygonStipple(GLbyte * pc)
{
    const GLubyte * const mask = (const GLubyte *) (pc + 20);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_PolygonStipple( GET_DISPATCH(), (
        mask
    ) );
}

void __glXDisp_Scissor(GLbyte * pc)
{
    CALL_Scissor( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLsizei  *)(pc +  8),
        *(GLsizei  *)(pc + 12)
    ) );
}

void __glXDisp_ShadeModel(GLbyte * pc)
{
    CALL_ShadeModel( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_TexParameterf(GLbyte * pc)
{
    CALL_TexParameterf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_TexParameterfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_TexParameterfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_TexParameteri(GLbyte * pc)
{
    CALL_TexParameteri( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_TexParameteriv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_TexParameteriv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_TexImage1D(GLbyte * pc)
{
    const GLvoid * const pixels = (const GLvoid *) (pc + 52);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_TexImage1D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLint    *)(pc + 24),
        *(GLint    *)(pc + 28),
        *(GLsizei  *)(pc + 32),
        *(GLint    *)(pc + 40),
        *(GLenum   *)(pc + 44),
        *(GLenum   *)(pc + 48),
        pixels
    ) );
}

void __glXDisp_TexImage2D(GLbyte * pc)
{
    const GLvoid * const pixels = (const GLvoid *) (pc + 52);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_TexImage2D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLint    *)(pc + 24),
        *(GLint    *)(pc + 28),
        *(GLsizei  *)(pc + 32),
        *(GLsizei  *)(pc + 36),
        *(GLint    *)(pc + 40),
        *(GLenum   *)(pc + 44),
        *(GLenum   *)(pc + 48),
        pixels
    ) );
}

void __glXDisp_TexEnvf(GLbyte * pc)
{
    CALL_TexEnvf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_TexEnvfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_TexEnvfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_TexEnvi(GLbyte * pc)
{
    CALL_TexEnvi( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_TexEnviv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_TexEnviv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_TexGend(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_TexGend( GET_DISPATCH(), (
        *(GLenum   *)(pc +  8),
        *(GLenum   *)(pc + 12),
        *(GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_TexGendv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLdouble * params;

#ifdef __GLX_ALIGN64
    const GLuint compsize = __glTexGendv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 8)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    params = (const GLdouble *) (pc + 8);

    CALL_TexGendv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_TexGenf(GLbyte * pc)
{
    CALL_TexGenf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_TexGenfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_TexGenfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_TexGeni(GLbyte * pc)
{
    CALL_TexGeni( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_TexGeniv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_TexGeniv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_InitNames(GLbyte * pc)
{
    CALL_InitNames( GET_DISPATCH(), () );
}

void __glXDisp_LoadName(GLbyte * pc)
{
    CALL_LoadName( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0)
    ) );
}

void __glXDisp_PassThrough(GLbyte * pc)
{
    CALL_PassThrough( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0)
    ) );
}

void __glXDisp_PopName(GLbyte * pc)
{
    CALL_PopName( GET_DISPATCH(), () );
}

void __glXDisp_PushName(GLbyte * pc)
{
    CALL_PushName( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0)
    ) );
}

void __glXDisp_DrawBuffer(GLbyte * pc)
{
    CALL_DrawBuffer( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_Clear(GLbyte * pc)
{
    CALL_Clear( GET_DISPATCH(), (
        *(GLbitfield *)(pc +  0)
    ) );
}

void __glXDisp_ClearAccum(GLbyte * pc)
{
    CALL_ClearAccum( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0),
        *(GLfloat  *)(pc +  4),
        *(GLfloat  *)(pc +  8),
        *(GLfloat  *)(pc + 12)
    ) );
}

void __glXDisp_ClearIndex(GLbyte * pc)
{
    CALL_ClearIndex( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0)
    ) );
}

void __glXDisp_ClearColor(GLbyte * pc)
{
    CALL_ClearColor( GET_DISPATCH(), (
        *(GLclampf *)(pc +  0),
        *(GLclampf *)(pc +  4),
        *(GLclampf *)(pc +  8),
        *(GLclampf *)(pc + 12)
    ) );
}

void __glXDisp_ClearStencil(GLbyte * pc)
{
    CALL_ClearStencil( GET_DISPATCH(), (
        *(GLint    *)(pc +  0)
    ) );
}

void __glXDisp_ClearDepth(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_ClearDepth( GET_DISPATCH(), (
        *(GLclampd *)(pc +  0)
    ) );
}

void __glXDisp_StencilMask(GLbyte * pc)
{
    CALL_StencilMask( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0)
    ) );
}

void __glXDisp_ColorMask(GLbyte * pc)
{
    CALL_ColorMask( GET_DISPATCH(), (
        *(GLboolean *)(pc +  0),
        *(GLboolean *)(pc +  1),
        *(GLboolean *)(pc +  2),
        *(GLboolean *)(pc +  3)
    ) );
}

void __glXDisp_DepthMask(GLbyte * pc)
{
    CALL_DepthMask( GET_DISPATCH(), (
        *(GLboolean *)(pc +  0)
    ) );
}

void __glXDisp_IndexMask(GLbyte * pc)
{
    CALL_IndexMask( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0)
    ) );
}

void __glXDisp_Accum(GLbyte * pc)
{
    CALL_Accum( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

void __glXDisp_Disable(GLbyte * pc)
{
    CALL_Disable( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_Enable(GLbyte * pc)
{
    CALL_Enable( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_PopAttrib(GLbyte * pc)
{
    CALL_PopAttrib( GET_DISPATCH(), () );
}

void __glXDisp_PushAttrib(GLbyte * pc)
{
    CALL_PushAttrib( GET_DISPATCH(), (
        *(GLbitfield *)(pc +  0)
    ) );
}

void __glXDisp_MapGrid1d(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_MapGrid1d( GET_DISPATCH(), (
        *(GLint    *)(pc + 16),
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_MapGrid1f(GLbyte * pc)
{
    CALL_MapGrid1f( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLfloat  *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_MapGrid2d(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 40);
        pc -= 4;
    }
#endif

    CALL_MapGrid2d( GET_DISPATCH(), (
        *(GLint    *)(pc + 32),
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8),
        *(GLint    *)(pc + 36),
        *(GLdouble *)(pc + 16),
        *(GLdouble *)(pc + 24)
    ) );
}

void __glXDisp_MapGrid2f(GLbyte * pc)
{
    CALL_MapGrid2f( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLfloat  *)(pc +  4),
        *(GLfloat  *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLfloat  *)(pc + 16),
        *(GLfloat  *)(pc + 20)
    ) );
}

void __glXDisp_EvalCoord1dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_EvalCoord1dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_EvalCoord1fv(GLbyte * pc)
{
    CALL_EvalCoord1fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_EvalCoord2dv(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_EvalCoord2dv( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_EvalCoord2fv(GLbyte * pc)
{
    CALL_EvalCoord2fv( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_EvalMesh1(GLbyte * pc)
{
    CALL_EvalMesh1( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_EvalPoint1(GLbyte * pc)
{
    CALL_EvalPoint1( GET_DISPATCH(), (
        *(GLint    *)(pc +  0)
    ) );
}

void __glXDisp_EvalMesh2(GLbyte * pc)
{
    CALL_EvalMesh2( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16)
    ) );
}

void __glXDisp_EvalPoint2(GLbyte * pc)
{
    CALL_EvalPoint2( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLint    *)(pc +  4)
    ) );
}

void __glXDisp_AlphaFunc(GLbyte * pc)
{
    CALL_AlphaFunc( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLclampf *)(pc +  4)
    ) );
}

void __glXDisp_BlendFunc(GLbyte * pc)
{
    CALL_BlendFunc( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4)
    ) );
}

void __glXDisp_LogicOp(GLbyte * pc)
{
    CALL_LogicOp( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_StencilFunc(GLbyte * pc)
{
    CALL_StencilFunc( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLuint   *)(pc +  8)
    ) );
}

void __glXDisp_StencilOp(GLbyte * pc)
{
    CALL_StencilOp( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLenum   *)(pc +  8)
    ) );
}

void __glXDisp_DepthFunc(GLbyte * pc)
{
    CALL_DepthFunc( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_PixelZoom(GLbyte * pc)
{
    CALL_PixelZoom( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

void __glXDisp_PixelTransferf(GLbyte * pc)
{
    CALL_PixelTransferf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

void __glXDisp_PixelTransferi(GLbyte * pc)
{
    CALL_PixelTransferi( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4)
    ) );
}

int __glXDisp_PixelStoref(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_PixelStoref( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLfloat  *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

int __glXDisp_PixelStorei(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        CALL_PixelStorei( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLint    *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

void __glXDisp_PixelMapfv(GLbyte * pc)
{
    const GLsizei mapsize = *(GLsizei  *)(pc +  4);

    CALL_PixelMapfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        mapsize,
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_PixelMapuiv(GLbyte * pc)
{
    const GLsizei mapsize = *(GLsizei  *)(pc +  4);

    CALL_PixelMapuiv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        mapsize,
         (const GLuint *)(pc +  8)
    ) );
}

void __glXDisp_PixelMapusv(GLbyte * pc)
{
    const GLsizei mapsize = *(GLsizei  *)(pc +  4);

    CALL_PixelMapusv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        mapsize,
         (const GLushort *)(pc +  8)
    ) );
}

void __glXDisp_ReadBuffer(GLbyte * pc)
{
    CALL_ReadBuffer( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_CopyPixels(GLbyte * pc)
{
    CALL_CopyPixels( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLsizei  *)(pc +  8),
        *(GLsizei  *)(pc + 12),
        *(GLenum   *)(pc + 16)
    ) );
}

void __glXDisp_DrawPixels(GLbyte * pc)
{
    const GLvoid * const pixels = (const GLvoid *) (pc + 36);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_DrawPixels( GET_DISPATCH(), (
        *(GLsizei  *)(pc + 20),
        *(GLsizei  *)(pc + 24),
        *(GLenum   *)(pc + 28),
        *(GLenum   *)(pc + 32),
        pixels
    ) );
}

int __glXDisp_GetBooleanv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetBooleanv_size(pname);
        GLboolean answerBuffer[200];
        GLboolean * params = __glXGetAnswerBuffer(cl, compsize, answerBuffer, sizeof(answerBuffer), 1);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetBooleanv( GET_DISPATCH(), (
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 1, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetClipPlane(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble equation[4];
        CALL_GetClipPlane( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            equation
        ) );
        __glXSendReply(cl->client, equation, 4, 8, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetDoublev(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetDoublev_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetDoublev( GET_DISPATCH(), (
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetError(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLenum retval;
        retval = CALL_GetError( GET_DISPATCH(), () );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDisp_GetFloatv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetFloatv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetFloatv( GET_DISPATCH(), (
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetIntegerv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetIntegerv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetIntegerv( GET_DISPATCH(), (
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetLightfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetLightfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetLightfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetLightiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetLightiv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetLightiv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMapdv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum target = *(GLenum   *)(pc +  0);
        const GLenum query = *(GLenum   *)(pc +  4);

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
        __glXSendReply(cl->client, v, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMapfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum target = *(GLenum   *)(pc +  0);
        const GLenum query = *(GLenum   *)(pc +  4);

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
        __glXSendReply(cl->client, v, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMapiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum target = *(GLenum   *)(pc +  0);
        const GLenum query = *(GLenum   *)(pc +  4);

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
        __glXSendReply(cl->client, v, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMaterialfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetMaterialfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMaterialfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMaterialiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetMaterialiv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMaterialiv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetPixelMapfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum map = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetPixelMapfv_size(map);
        GLfloat answerBuffer[200];
        GLfloat * values = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (values == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetPixelMapfv( GET_DISPATCH(), (
            map,
            values
        ) );
        __glXSendReply(cl->client, values, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetPixelMapuiv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum map = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetPixelMapuiv_size(map);
        GLuint answerBuffer[200];
        GLuint * values = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (values == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetPixelMapuiv( GET_DISPATCH(), (
            map,
            values
        ) );
        __glXSendReply(cl->client, values, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetPixelMapusv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum map = *(GLenum   *)(pc +  0);

        const GLuint compsize = __glGetPixelMapusv_size(map);
        GLushort answerBuffer[200];
        GLushort * values = __glXGetAnswerBuffer(cl, compsize * 2, answerBuffer, sizeof(answerBuffer), 2);

        if (values == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetPixelMapusv( GET_DISPATCH(), (
            map,
            values
        ) );
        __glXSendReply(cl->client, values, compsize, 2, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexEnvfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexEnvfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexEnvfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexEnviv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexEnviv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexEnviv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexGendv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexGendv_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexGendv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexGenfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexGenfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexGenfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexGeniv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexGeniv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexGeniv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetTexParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexLevelParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  8);

        const GLuint compsize = __glGetTexLevelParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexLevelParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLint    *)(pc +  4),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTexLevelParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  8);

        const GLuint compsize = __glGetTexLevelParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetTexLevelParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLint    *)(pc +  4),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_IsEnabled(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsEnabled( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDisp_IsList(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsList( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_DepthRange(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 16);
        pc -= 4;
    }
#endif

    CALL_DepthRange( GET_DISPATCH(), (
        *(GLclampd *)(pc +  0),
        *(GLclampd *)(pc +  8)
    ) );
}

void __glXDisp_Frustum(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 48);
        pc -= 4;
    }
#endif

    CALL_Frustum( GET_DISPATCH(), (
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8),
        *(GLdouble *)(pc + 16),
        *(GLdouble *)(pc + 24),
        *(GLdouble *)(pc + 32),
        *(GLdouble *)(pc + 40)
    ) );
}

void __glXDisp_LoadIdentity(GLbyte * pc)
{
    CALL_LoadIdentity( GET_DISPATCH(), () );
}

void __glXDisp_LoadMatrixf(GLbyte * pc)
{
    CALL_LoadMatrixf( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_LoadMatrixd(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 128);
        pc -= 4;
    }
#endif

    CALL_LoadMatrixd( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_MatrixMode(GLbyte * pc)
{
    CALL_MatrixMode( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_MultMatrixf(GLbyte * pc)
{
    CALL_MultMatrixf( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_MultMatrixd(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 128);
        pc -= 4;
    }
#endif

    CALL_MultMatrixd( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_Ortho(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 48);
        pc -= 4;
    }
#endif

    CALL_Ortho( GET_DISPATCH(), (
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8),
        *(GLdouble *)(pc + 16),
        *(GLdouble *)(pc + 24),
        *(GLdouble *)(pc + 32),
        *(GLdouble *)(pc + 40)
    ) );
}

void __glXDisp_PopMatrix(GLbyte * pc)
{
    CALL_PopMatrix( GET_DISPATCH(), () );
}

void __glXDisp_PushMatrix(GLbyte * pc)
{
    CALL_PushMatrix( GET_DISPATCH(), () );
}

void __glXDisp_Rotated(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 32);
        pc -= 4;
    }
#endif

    CALL_Rotated( GET_DISPATCH(), (
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8),
        *(GLdouble *)(pc + 16),
        *(GLdouble *)(pc + 24)
    ) );
}

void __glXDisp_Rotatef(GLbyte * pc)
{
    CALL_Rotatef( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0),
        *(GLfloat  *)(pc +  4),
        *(GLfloat  *)(pc +  8),
        *(GLfloat  *)(pc + 12)
    ) );
}

void __glXDisp_Scaled(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Scaled( GET_DISPATCH(), (
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8),
        *(GLdouble *)(pc + 16)
    ) );
}

void __glXDisp_Scalef(GLbyte * pc)
{
    CALL_Scalef( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0),
        *(GLfloat  *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_Translated(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_Translated( GET_DISPATCH(), (
        *(GLdouble *)(pc +  0),
        *(GLdouble *)(pc +  8),
        *(GLdouble *)(pc + 16)
    ) );
}

void __glXDisp_Translatef(GLbyte * pc)
{
    CALL_Translatef( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0),
        *(GLfloat  *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_Viewport(GLbyte * pc)
{
    CALL_Viewport( GET_DISPATCH(), (
        *(GLint    *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLsizei  *)(pc +  8),
        *(GLsizei  *)(pc + 12)
    ) );
}

void __glXDisp_BindTexture(GLbyte * pc)
{
    CALL_BindTexture( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4)
    ) );
}

void __glXDisp_Indexubv(GLbyte * pc)
{
    CALL_Indexubv( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDisp_PolygonOffset(GLbyte * pc)
{
    CALL_PolygonOffset( GET_DISPATCH(), (
        *(GLfloat  *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

int __glXDisp_AreTexturesResident(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean * residences = __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);
        retval = CALL_AreTexturesResident( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4),
            residences
        ) );
        __glXSendReply(cl->client, residences, n, 1, GL_TRUE, retval);
        error = Success;
    }

    return error;
}

int __glXDisp_AreTexturesResidentEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean * residences = __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);
        retval = CALL_AreTexturesResident( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4),
            residences
        ) );
        __glXSendReply(cl->client, residences, n, 1, GL_TRUE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_CopyTexImage1D(GLbyte * pc)
{
    CALL_CopyTexImage1D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLsizei  *)(pc + 20),
        *(GLint    *)(pc + 24)
    ) );
}

void __glXDisp_CopyTexImage2D(GLbyte * pc)
{
    CALL_CopyTexImage2D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLsizei  *)(pc + 20),
        *(GLsizei  *)(pc + 24),
        *(GLint    *)(pc + 28)
    ) );
}

void __glXDisp_CopyTexSubImage1D(GLbyte * pc)
{
    CALL_CopyTexSubImage1D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLsizei  *)(pc + 20)
    ) );
}

void __glXDisp_CopyTexSubImage2D(GLbyte * pc)
{
    CALL_CopyTexSubImage2D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLint    *)(pc + 20),
        *(GLsizei  *)(pc + 24),
        *(GLsizei  *)(pc + 28)
    ) );
}

int __glXDisp_DeleteTextures(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        CALL_DeleteTextures( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

int __glXDisp_DeleteTexturesEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        CALL_DeleteTextures( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

int __glXDisp_GenTextures(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLuint answerBuffer[200];
        GLuint * textures = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenTextures( GET_DISPATCH(), (
            n,
            textures
        ) );
        __glXSendReply(cl->client, textures, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GenTexturesEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLuint answerBuffer[200];
        GLuint * textures = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenTextures( GET_DISPATCH(), (
            n,
            textures
        ) );
        __glXSendReply(cl->client, textures, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_IsTexture(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsTexture( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDisp_IsTextureEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsTexture( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_PrioritizeTextures(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  0);

    CALL_PrioritizeTextures( GET_DISPATCH(), (
        n,
         (const GLuint *)(pc +  4),
         (const GLclampf *)(pc +  4)
    ) );
}

void __glXDisp_TexSubImage1D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 52);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 56);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_TexSubImage1D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLint    *)(pc + 24),
        *(GLint    *)(pc + 28),
        *(GLsizei  *)(pc + 36),
        *(GLenum   *)(pc + 44),
        *(GLenum   *)(pc + 48),
        pixels
    ) );
}

void __glXDisp_TexSubImage2D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 52);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 56);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_TexSubImage2D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLint    *)(pc + 24),
        *(GLint    *)(pc + 28),
        *(GLint    *)(pc + 32),
        *(GLsizei  *)(pc + 36),
        *(GLsizei  *)(pc + 40),
        *(GLenum   *)(pc + 44),
        *(GLenum   *)(pc + 48),
        pixels
    ) );
}

void __glXDisp_BlendColor(GLbyte * pc)
{
    CALL_BlendColor( GET_DISPATCH(), (
        *(GLclampf *)(pc +  0),
        *(GLclampf *)(pc +  4),
        *(GLclampf *)(pc +  8),
        *(GLclampf *)(pc + 12)
    ) );
}

void __glXDisp_BlendEquation(GLbyte * pc)
{
    CALL_BlendEquation( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_ColorTable(GLbyte * pc)
{
    const GLvoid * const table = (const GLvoid *) (pc + 40);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_ColorTable( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLenum   *)(pc + 24),
        *(GLsizei  *)(pc + 28),
        *(GLenum   *)(pc + 32),
        *(GLenum   *)(pc + 36),
        table
    ) );
}

void __glXDisp_ColorTableParameterfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_ColorTableParameterfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_ColorTableParameteriv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_ColorTableParameteriv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_CopyColorTable(GLbyte * pc)
{
    CALL_CopyColorTable( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLsizei  *)(pc + 16)
    ) );
}

int __glXDisp_GetColorTableParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetColorTableParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetColorTableParameterfvSGI(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetColorTableParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetColorTableParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetColorTableParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetColorTableParameterivSGI(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetColorTableParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetColorTableParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void __glXDisp_ColorSubTable(GLbyte * pc)
{
    const GLvoid * const data = (const GLvoid *) (pc + 40);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_ColorSubTable( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLsizei  *)(pc + 24),
        *(GLsizei  *)(pc + 28),
        *(GLenum   *)(pc + 32),
        *(GLenum   *)(pc + 36),
        data
    ) );
}

void __glXDisp_CopyColorSubTable(GLbyte * pc)
{
    CALL_CopyColorSubTable( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLsizei  *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLsizei  *)(pc + 16)
    ) );
}

void __glXDisp_ConvolutionFilter1D(GLbyte * pc)
{
    const GLvoid * const image = (const GLvoid *) (pc + 44);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_ConvolutionFilter1D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLenum   *)(pc + 24),
        *(GLsizei  *)(pc + 28),
        *(GLenum   *)(pc + 36),
        *(GLenum   *)(pc + 40),
        image
    ) );
}

void __glXDisp_ConvolutionFilter2D(GLbyte * pc)
{
    const GLvoid * const image = (const GLvoid *) (pc + 44);
    __GLXpixelHeader * const hdr = (__GLXpixelHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_ConvolutionFilter2D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 20),
        *(GLenum   *)(pc + 24),
        *(GLsizei  *)(pc + 28),
        *(GLsizei  *)(pc + 32),
        *(GLenum   *)(pc + 36),
        *(GLenum   *)(pc + 40),
        image
    ) );
}

void __glXDisp_ConvolutionParameterf(GLbyte * pc)
{
    CALL_ConvolutionParameterf( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLfloat  *)(pc +  8)
    ) );
}

void __glXDisp_ConvolutionParameterfv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 8);

    CALL_ConvolutionParameterfv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_ConvolutionParameteri(GLbyte * pc)
{
    CALL_ConvolutionParameteri( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8)
    ) );
}

void __glXDisp_ConvolutionParameteriv(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  4);
    const GLint * params;

    params = (const GLint *) (pc + 8);

    CALL_ConvolutionParameteriv( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        pname,
        params
    ) );
}

void __glXDisp_CopyConvolutionFilter1D(GLbyte * pc)
{
    CALL_CopyConvolutionFilter1D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLsizei  *)(pc + 16)
    ) );
}

void __glXDisp_CopyConvolutionFilter2D(GLbyte * pc)
{
    CALL_CopyConvolutionFilter2D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLsizei  *)(pc + 16),
        *(GLsizei  *)(pc + 20)
    ) );
}

int __glXDisp_GetConvolutionParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetConvolutionParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetConvolutionParameterfvEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetConvolutionParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetConvolutionParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetConvolutionParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetConvolutionParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetConvolutionParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetConvolutionParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetHistogramParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetHistogramParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetHistogramParameterfvEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetHistogramParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetHistogramParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetHistogramParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetHistogramParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetHistogramParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetHistogramParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMinmaxParameterfv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetMinmaxParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMinmaxParameterfvEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetMinmaxParameterfv_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameterfv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMinmaxParameteriv(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetMinmaxParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetMinmaxParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetMinmaxParameteriv_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetMinmaxParameteriv( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void __glXDisp_Histogram(GLbyte * pc)
{
    CALL_Histogram( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLsizei  *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLboolean *)(pc + 12)
    ) );
}

void __glXDisp_Minmax(GLbyte * pc)
{
    CALL_Minmax( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLboolean *)(pc +  8)
    ) );
}

void __glXDisp_ResetHistogram(GLbyte * pc)
{
    CALL_ResetHistogram( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_ResetMinmax(GLbyte * pc)
{
    CALL_ResetMinmax( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_TexImage3D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 76);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 80);
    __GLXpixel3DHeader * const hdr = (__GLXpixel3DHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_IMAGE_HEIGHT, (GLint) hdr->imageHeight) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_IMAGES,  (GLint) hdr->skipImages) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_TexImage3D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 36),
        *(GLint    *)(pc + 40),
        *(GLint    *)(pc + 44),
        *(GLsizei  *)(pc + 48),
        *(GLsizei  *)(pc + 52),
        *(GLsizei  *)(pc + 56),
        *(GLint    *)(pc + 64),
        *(GLenum   *)(pc + 68),
        *(GLenum   *)(pc + 72),
        pixels
    ) );
}

void __glXDisp_TexSubImage3D(GLbyte * pc)
{
    const CARD32 ptr_is_null = *(CARD32 *)(pc + 84);
    const GLvoid * const pixels = (const GLvoid *) (ptr_is_null != 0) ? NULL : (pc + 88);
    __GLXpixel3DHeader * const hdr = (__GLXpixel3DHeader *)(pc);

    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SWAP_BYTES,   hdr->swapBytes) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_LSB_FIRST,    hdr->lsbFirst) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ROW_LENGTH,   (GLint) hdr->rowLength) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_IMAGE_HEIGHT, (GLint) hdr->imageHeight) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_ROWS,    (GLint) hdr->skipRows) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_IMAGES,  (GLint) hdr->skipImages) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_SKIP_PIXELS,  (GLint) hdr->skipPixels) );
    CALL_PixelStorei( GET_DISPATCH(), (GL_UNPACK_ALIGNMENT,    (GLint) hdr->alignment) );

    CALL_TexSubImage3D( GET_DISPATCH(), (
        *(GLenum   *)(pc + 36),
        *(GLint    *)(pc + 40),
        *(GLint    *)(pc + 44),
        *(GLint    *)(pc + 48),
        *(GLint    *)(pc + 52),
        *(GLsizei  *)(pc + 60),
        *(GLsizei  *)(pc + 64),
        *(GLsizei  *)(pc + 68),
        *(GLenum   *)(pc + 76),
        *(GLenum   *)(pc + 80),
        pixels
    ) );
}

void __glXDisp_CopyTexSubImage3D(GLbyte * pc)
{
    CALL_CopyTexSubImage3D( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLint    *)(pc + 20),
        *(GLint    *)(pc + 24),
        *(GLsizei  *)(pc + 28),
        *(GLsizei  *)(pc + 32)
    ) );
}

void __glXDisp_ActiveTextureARB(GLbyte * pc)
{
    CALL_ActiveTextureARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_MultiTexCoord1dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 12);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord1dvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  8),
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_MultiTexCoord1fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord1fvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord1ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord1ivARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLint *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord1svARB(GLbyte * pc)
{
    CALL_MultiTexCoord1svARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord2dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord2dvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc + 16),
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_MultiTexCoord2fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord2fvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord2ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord2ivARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLint *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord2svARB(GLbyte * pc)
{
    CALL_MultiTexCoord2svARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord3dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 28);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord3dvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc + 24),
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_MultiTexCoord3fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord3fvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord3ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord3ivARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLint *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord3svARB(GLbyte * pc)
{
    CALL_MultiTexCoord3svARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_MultiTexCoord4dvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc + 32),
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_MultiTexCoord4fvARB(GLbyte * pc)
{
    CALL_MultiTexCoord4fvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord4ivARB(GLbyte * pc)
{
    CALL_MultiTexCoord4ivARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLint *)(pc +  4)
    ) );
}

void __glXDisp_MultiTexCoord4svARB(GLbyte * pc)
{
    CALL_MultiTexCoord4svARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_SampleCoverageARB(GLbyte * pc)
{
    CALL_SampleCoverageARB( GET_DISPATCH(), (
        *(GLclampf *)(pc +  0),
        *(GLboolean *)(pc +  4)
    ) );
}

void __glXDisp_CompressedTexImage1DARB(GLbyte * pc)
{
    const GLsizei imageSize = *(GLsizei  *)(pc + 20);

    CALL_CompressedTexImage1DARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLsizei  *)(pc + 12),
        *(GLint    *)(pc + 16),
        imageSize,
         (const GLvoid *)(pc + 24)
    ) );
}

void __glXDisp_CompressedTexImage2DARB(GLbyte * pc)
{
    const GLsizei imageSize = *(GLsizei  *)(pc + 24);

    CALL_CompressedTexImage2DARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLsizei  *)(pc + 12),
        *(GLsizei  *)(pc + 16),
        *(GLint    *)(pc + 20),
        imageSize,
         (const GLvoid *)(pc + 28)
    ) );
}

void __glXDisp_CompressedTexImage3DARB(GLbyte * pc)
{
    const GLsizei imageSize = *(GLsizei  *)(pc + 28);

    CALL_CompressedTexImage3DARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLsizei  *)(pc + 12),
        *(GLsizei  *)(pc + 16),
        *(GLsizei  *)(pc + 20),
        *(GLint    *)(pc + 24),
        imageSize,
         (const GLvoid *)(pc + 32)
    ) );
}

void __glXDisp_CompressedTexSubImage1DARB(GLbyte * pc)
{
    const GLsizei imageSize = *(GLsizei  *)(pc + 20);

    CALL_CompressedTexSubImage1DARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLsizei  *)(pc + 12),
        *(GLenum   *)(pc + 16),
        imageSize,
         (const GLvoid *)(pc + 24)
    ) );
}

void __glXDisp_CompressedTexSubImage2DARB(GLbyte * pc)
{
    const GLsizei imageSize = *(GLsizei  *)(pc + 28);

    CALL_CompressedTexSubImage2DARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLsizei  *)(pc + 16),
        *(GLsizei  *)(pc + 20),
        *(GLenum   *)(pc + 24),
        imageSize,
         (const GLvoid *)(pc + 32)
    ) );
}

void __glXDisp_CompressedTexSubImage3DARB(GLbyte * pc)
{
    const GLsizei imageSize = *(GLsizei  *)(pc + 36);

    CALL_CompressedTexSubImage3DARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4),
        *(GLint    *)(pc +  8),
        *(GLint    *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLsizei  *)(pc + 20),
        *(GLsizei  *)(pc + 24),
        *(GLsizei  *)(pc + 28),
        *(GLenum   *)(pc + 32),
        imageSize,
         (const GLvoid *)(pc + 40)
    ) );
}

int __glXDisp_GetProgramEnvParameterdvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble params[4];
        CALL_GetProgramEnvParameterdvARB( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramEnvParameterfvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLfloat params[4];
        CALL_GetProgramEnvParameterfvARB( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramLocalParameterdvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble params[4];
        CALL_GetProgramLocalParameterdvARB( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramLocalParameterfvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLfloat params[4];
        CALL_GetProgramLocalParameterfvARB( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetProgramivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetProgramivARB( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetVertexAttribdvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetVertexAttribdvARB_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribdvARB( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetVertexAttribfvARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetVertexAttribfvARB_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribfvARB( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetVertexAttribivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetVertexAttribivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribivARB( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

void __glXDisp_ProgramEnvParameter4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 40);
        pc -= 4;
    }
#endif

    CALL_ProgramEnvParameter4dvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
         (const GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_ProgramEnvParameter4fvARB(GLbyte * pc)
{
    CALL_ProgramEnvParameter4fvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_ProgramLocalParameter4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 40);
        pc -= 4;
    }
#endif

    CALL_ProgramLocalParameter4dvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
         (const GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_ProgramLocalParameter4fvARB(GLbyte * pc)
{
    CALL_ProgramLocalParameter4fvARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_ProgramStringARB(GLbyte * pc)
{
    const GLsizei len = *(GLsizei  *)(pc +  8);

    CALL_ProgramStringARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        len,
         (const GLvoid *)(pc + 12)
    ) );
}

void __glXDisp_VertexAttrib1dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 12);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib1dvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib1fvARB(GLbyte * pc)
{
    CALL_VertexAttrib1fvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib1svARB(GLbyte * pc)
{
    CALL_VertexAttrib1svARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib2dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib2dvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib2fvARB(GLbyte * pc)
{
    CALL_VertexAttrib2fvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib2svARB(GLbyte * pc)
{
    CALL_VertexAttrib2svARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib3dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 28);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib3dvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib3fvARB(GLbyte * pc)
{
    CALL_VertexAttrib3fvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib3svARB(GLbyte * pc)
{
    CALL_VertexAttrib3svARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4NbvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NbvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLbyte *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4NivARB(GLbyte * pc)
{
    CALL_VertexAttrib4NivARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLint *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4NsvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NsvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4NubvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NubvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLubyte *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4NuivARB(GLbyte * pc)
{
    CALL_VertexAttrib4NuivARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLuint *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4NusvARB(GLbyte * pc)
{
    CALL_VertexAttrib4NusvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLushort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4bvARB(GLbyte * pc)
{
    CALL_VertexAttrib4bvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLbyte *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4dvARB(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib4dvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4fvARB(GLbyte * pc)
{
    CALL_VertexAttrib4fvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4ivARB(GLbyte * pc)
{
    CALL_VertexAttrib4ivARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLint *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4svARB(GLbyte * pc)
{
    CALL_VertexAttrib4svARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4ubvARB(GLbyte * pc)
{
    CALL_VertexAttrib4ubvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLubyte *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4uivARB(GLbyte * pc)
{
    CALL_VertexAttrib4uivARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLuint *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4usvARB(GLbyte * pc)
{
    CALL_VertexAttrib4usvARB( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLushort *)(pc +  4)
    ) );
}

void __glXDisp_BeginQueryARB(GLbyte * pc)
{
    CALL_BeginQueryARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4)
    ) );
}

int __glXDisp_DeleteQueriesARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        CALL_DeleteQueriesARB( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

void __glXDisp_EndQueryARB(GLbyte * pc)
{
    CALL_EndQueryARB( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

int __glXDisp_GenQueriesARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLuint answerBuffer[200];
        GLuint * ids = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenQueriesARB( GET_DISPATCH(), (
            n,
            ids
        ) );
        __glXSendReply(cl->client, ids, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetQueryObjectivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetQueryObjectivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetQueryObjectivARB( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetQueryObjectuivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetQueryObjectuivARB_size(pname);
        GLuint answerBuffer[200];
        GLuint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetQueryObjectuivARB( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetQueryivARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetQueryivARB_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetQueryivARB( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_IsQueryARB(__GLXclientState *cl, GLbyte *pc)
{
    xGLXSingleReq * const req = (xGLXSingleReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_SINGLE_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsQueryARB( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_DrawBuffersARB(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  0);

    CALL_DrawBuffersARB( GET_DISPATCH(), (
        n,
         (const GLenum *)(pc +  4)
    ) );
}

void __glXDisp_SampleMaskSGIS(GLbyte * pc)
{
    CALL_SampleMaskSGIS( GET_DISPATCH(), (
        *(GLclampf *)(pc +  0),
        *(GLboolean *)(pc +  4)
    ) );
}

void __glXDisp_SamplePatternSGIS(GLbyte * pc)
{
    CALL_SamplePatternSGIS( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

void __glXDisp_PointParameterfEXT(GLbyte * pc)
{
    CALL_PointParameterfEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLfloat  *)(pc +  4)
    ) );
}

void __glXDisp_PointParameterfvEXT(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  0);
    const GLfloat * params;

    params = (const GLfloat *) (pc + 4);

    CALL_PointParameterfvEXT( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDisp_SecondaryColor3bvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3bvEXT( GET_DISPATCH(), (
         (const GLbyte *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3dvEXT(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 24);
        pc -= 4;
    }
#endif

    CALL_SecondaryColor3dvEXT( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3fvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3fvEXT( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3ivEXT(GLbyte * pc)
{
    CALL_SecondaryColor3ivEXT( GET_DISPATCH(), (
         (const GLint *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3svEXT(GLbyte * pc)
{
    CALL_SecondaryColor3svEXT( GET_DISPATCH(), (
         (const GLshort *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3ubvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3ubvEXT( GET_DISPATCH(), (
         (const GLubyte *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3uivEXT(GLbyte * pc)
{
    CALL_SecondaryColor3uivEXT( GET_DISPATCH(), (
         (const GLuint *)(pc +  0)
    ) );
}

void __glXDisp_SecondaryColor3usvEXT(GLbyte * pc)
{
    CALL_SecondaryColor3usvEXT( GET_DISPATCH(), (
         (const GLushort *)(pc +  0)
    ) );
}

void __glXDisp_FogCoorddvEXT(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 8);
        pc -= 4;
    }
#endif

    CALL_FogCoorddvEXT( GET_DISPATCH(), (
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_FogCoordfvEXT(GLbyte * pc)
{
    CALL_FogCoordfvEXT( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

void __glXDisp_BlendFuncSeparateEXT(GLbyte * pc)
{
    CALL_BlendFuncSeparateEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLenum   *)(pc + 12)
    ) );
}

void __glXDisp_WindowPos3fvMESA(GLbyte * pc)
{
    CALL_WindowPos3fvMESA( GET_DISPATCH(), (
         (const GLfloat *)(pc +  0)
    ) );
}

int __glXDisp_AreProgramsResidentNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLboolean retval;
        GLboolean answerBuffer[200];
        GLboolean * residences = __glXGetAnswerBuffer(cl, n, answerBuffer, sizeof(answerBuffer), 1);
        retval = CALL_AreProgramsResidentNV( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4),
            residences
        ) );
        __glXSendReply(cl->client, residences, n, 1, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_BindProgramNV(GLbyte * pc)
{
    CALL_BindProgramNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4)
    ) );
}

int __glXDisp_DeleteProgramsNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        CALL_DeleteProgramsNV( GET_DISPATCH(), (
            n,
             (const GLuint *)(pc +  4)
        ) );
        error = Success;
    }

    return error;
}

void __glXDisp_ExecuteProgramNV(GLbyte * pc)
{
    CALL_ExecuteProgramNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
         (const GLfloat *)(pc +  8)
    ) );
}

int __glXDisp_GenProgramsNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLuint answerBuffer[200];
        GLuint * programs = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenProgramsNV( GET_DISPATCH(), (
            n,
            programs
        ) );
        __glXSendReply(cl->client, programs, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramParameterdvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLdouble params[4];
        CALL_GetProgramParameterdvNV( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            *(GLenum   *)(pc +  8),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramParameterfvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLfloat params[4];
        CALL_GetProgramParameterfvNV( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            *(GLenum   *)(pc +  8),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramivNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetProgramivNV_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetProgramivNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetTrackMatrixivNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLint params[1];
        CALL_GetTrackMatrixivNV( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLuint   *)(pc +  4),
            *(GLenum   *)(pc +  8),
            params
        ) );
        __glXSendReply(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetVertexAttribdvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetVertexAttribdvNV_size(pname);
        GLdouble answerBuffer[200];
        GLdouble * params = __glXGetAnswerBuffer(cl, compsize * 8, answerBuffer, sizeof(answerBuffer), 8);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribdvNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 8, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetVertexAttribfvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetVertexAttribfvNV_size(pname);
        GLfloat answerBuffer[200];
        GLfloat * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribfvNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetVertexAttribivNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLenum pname = *(GLenum   *)(pc +  4);

        const GLuint compsize = __glGetVertexAttribivNV_size(pname);
        GLint answerBuffer[200];
        GLint * params = __glXGetAnswerBuffer(cl, compsize * 4, answerBuffer, sizeof(answerBuffer), 4);

        if (params == NULL) return BadAlloc;
        __glXClearErrorOccured();

        CALL_GetVertexAttribivNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            pname,
            params
        ) );
        __glXSendReply(cl->client, params, compsize, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_IsProgramNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsProgramNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_LoadProgramNV(GLbyte * pc)
{
    const GLsizei len = *(GLsizei  *)(pc +  8);

    CALL_LoadProgramNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
        len,
         (const GLubyte *)(pc + 12)
    ) );
}

void __glXDisp_ProgramParameters4dvNV(GLbyte * pc)
{
    const GLuint num = *(GLuint   *)(pc +  8);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 16 + __GLX_PAD((num * 32)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_ProgramParameters4dvNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
        num,
         (const GLdouble *)(pc + 12)
    ) );
}

void __glXDisp_ProgramParameters4fvNV(GLbyte * pc)
{
    const GLuint num = *(GLuint   *)(pc +  8);

    CALL_ProgramParameters4fvNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
        num,
         (const GLfloat *)(pc + 12)
    ) );
}

void __glXDisp_RequestResidentProgramsNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  0);

    CALL_RequestResidentProgramsNV( GET_DISPATCH(), (
        n,
         (const GLuint *)(pc +  4)
    ) );
}

void __glXDisp_TrackMatrixNV(GLbyte * pc)
{
    CALL_TrackMatrixNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLenum   *)(pc + 12)
    ) );
}

void __glXDisp_VertexAttrib1dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 12);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib1dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib1fvNV(GLbyte * pc)
{
    CALL_VertexAttrib1fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib1svNV(GLbyte * pc)
{
    CALL_VertexAttrib1svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib2dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 20);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib2dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib2fvNV(GLbyte * pc)
{
    CALL_VertexAttrib2fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib2svNV(GLbyte * pc)
{
    CALL_VertexAttrib2svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib3dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 28);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib3dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib3fvNV(GLbyte * pc)
{
    CALL_VertexAttrib3fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib3svNV(GLbyte * pc)
{
    CALL_VertexAttrib3svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4dvNV(GLbyte * pc)
{
#ifdef __GLX_ALIGN64
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, 36);
        pc -= 4;
    }
#endif

    CALL_VertexAttrib4dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLdouble *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4fvNV(GLbyte * pc)
{
    CALL_VertexAttrib4fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLfloat *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4svNV(GLbyte * pc)
{
    CALL_VertexAttrib4svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLshort *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttrib4ubvNV(GLbyte * pc)
{
    CALL_VertexAttrib4ubvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
         (const GLubyte *)(pc +  4)
    ) );
}

void __glXDisp_VertexAttribs1dvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 8)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs1dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs1fvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs1fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs1svNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs1svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLshort *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs2dvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 16)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs2dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs2fvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs2fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs2svNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs2svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLshort *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs3dvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 24)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs3dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs3fvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs3fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs3svNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs3svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLshort *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs4dvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 12 + __GLX_PAD((n * 32)) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_VertexAttribs4dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLdouble *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs4fvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs4fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs4svNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs4svNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLshort *)(pc +  8)
    ) );
}

void __glXDisp_VertexAttribs4ubvNV(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  4);

    CALL_VertexAttribs4ubvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        n,
         (const GLubyte *)(pc +  8)
    ) );
}

void __glXDisp_PointParameteriNV(GLbyte * pc)
{
    CALL_PointParameteriNV( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLint    *)(pc +  4)
    ) );
}

void __glXDisp_PointParameterivNV(GLbyte * pc)
{
    const GLenum pname = *(GLenum   *)(pc +  0);
    const GLint * params;

    params = (const GLint *) (pc + 4);

    CALL_PointParameterivNV( GET_DISPATCH(), (
        pname,
        params
    ) );
}

void __glXDisp_ActiveStencilFaceEXT(GLbyte * pc)
{
    CALL_ActiveStencilFaceEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

int __glXDisp_GetProgramNamedParameterdvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei len = *(GLsizei  *)(pc +  4);

        GLdouble params[4];
        CALL_GetProgramNamedParameterdvNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            len,
             (const GLubyte *)(pc +  8),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 8, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetProgramNamedParameterfvNV(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei len = *(GLsizei  *)(pc +  4);

        GLfloat params[4];
        CALL_GetProgramNamedParameterfvNV( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0),
            len,
             (const GLubyte *)(pc +  8),
            params
        ) );
        __glXSendReply(cl->client, params, 4, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

void __glXDisp_ProgramNamedParameter4dvNV(GLbyte * pc)
{
    const GLsizei len = *(GLsizei  *)(pc + 36);

#ifdef __GLX_ALIGN64
    const GLuint cmdlen = 44 + __GLX_PAD(len) - 4;
    if ((unsigned long)(pc) & 7) {
        (void) memmove(pc-4, pc, cmdlen);
        pc -= 4;
    }
#endif

    CALL_ProgramNamedParameter4dvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc + 32),
        len,
         (const GLubyte *)(pc + 40),
         (const GLdouble *)(pc +  0)
    ) );
}

void __glXDisp_ProgramNamedParameter4fvNV(GLbyte * pc)
{
    const GLsizei len = *(GLsizei  *)(pc +  4);

    CALL_ProgramNamedParameter4fvNV( GET_DISPATCH(), (
        *(GLuint   *)(pc +  0),
        len,
         (const GLubyte *)(pc + 24),
         (const GLfloat *)(pc +  8)
    ) );
}

void __glXDisp_BlendEquationSeparateEXT(GLbyte * pc)
{
    CALL_BlendEquationSeparateEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4)
    ) );
}

void __glXDisp_BindFramebufferEXT(GLbyte * pc)
{
    CALL_BindFramebufferEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4)
    ) );
}

void __glXDisp_BindRenderbufferEXT(GLbyte * pc)
{
    CALL_BindRenderbufferEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLuint   *)(pc +  4)
    ) );
}

int __glXDisp_CheckFramebufferStatusEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLenum retval;
        retval = CALL_CheckFramebufferStatusEXT( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_DeleteFramebuffersEXT(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  0);

    CALL_DeleteFramebuffersEXT( GET_DISPATCH(), (
        n,
         (const GLuint *)(pc +  4)
    ) );
}

void __glXDisp_DeleteRenderbuffersEXT(GLbyte * pc)
{
    const GLsizei n = *(GLsizei  *)(pc +  0);

    CALL_DeleteRenderbuffersEXT( GET_DISPATCH(), (
        n,
         (const GLuint *)(pc +  4)
    ) );
}

void __glXDisp_FramebufferRenderbufferEXT(GLbyte * pc)
{
    CALL_FramebufferRenderbufferEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLuint   *)(pc + 12)
    ) );
}

void __glXDisp_FramebufferTexture1DEXT(GLbyte * pc)
{
    CALL_FramebufferTexture1DEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLuint   *)(pc + 12),
        *(GLint    *)(pc + 16)
    ) );
}

void __glXDisp_FramebufferTexture2DEXT(GLbyte * pc)
{
    CALL_FramebufferTexture2DEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLuint   *)(pc + 12),
        *(GLint    *)(pc + 16)
    ) );
}

void __glXDisp_FramebufferTexture3DEXT(GLbyte * pc)
{
    CALL_FramebufferTexture3DEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLenum   *)(pc +  8),
        *(GLuint   *)(pc + 12),
        *(GLint    *)(pc + 16),
        *(GLint    *)(pc + 20)
    ) );
}

int __glXDisp_GenFramebuffersEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLuint answerBuffer[200];
        GLuint * framebuffers = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenFramebuffersEXT( GET_DISPATCH(), (
            n,
            framebuffers
        ) );
        __glXSendReply(cl->client, framebuffers, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GenRenderbuffersEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        const GLsizei n = *(GLsizei  *)(pc +  0);

        GLuint answerBuffer[200];
        GLuint * renderbuffers = __glXGetAnswerBuffer(cl, n * 4, answerBuffer, sizeof(answerBuffer), 4);
        CALL_GenRenderbuffersEXT( GET_DISPATCH(), (
            n,
            renderbuffers
        ) );
        __glXSendReply(cl->client, renderbuffers, n, 4, GL_TRUE, 0);
        error = Success;
    }

    return error;
}

void __glXDisp_GenerateMipmapEXT(GLbyte * pc)
{
    CALL_GenerateMipmapEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0)
    ) );
}

int __glXDisp_GetFramebufferAttachmentParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLint params[1];
        CALL_GetFramebufferAttachmentParameterivEXT( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLenum   *)(pc +  4),
            *(GLenum   *)(pc +  8),
            params
        ) );
        __glXSendReply(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_GetRenderbufferParameterivEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLint params[1];
        CALL_GetRenderbufferParameterivEXT( GET_DISPATCH(), (
            *(GLenum   *)(pc +  0),
            *(GLenum   *)(pc +  4),
            params
        ) );
        __glXSendReply(cl->client, params, 1, 4, GL_FALSE, 0);
        error = Success;
    }

    return error;
}

int __glXDisp_IsFramebufferEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsFramebufferEXT( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

int __glXDisp_IsRenderbufferEXT(__GLXclientState *cl, GLbyte *pc)
{
    xGLXVendorPrivateReq * const req = (xGLXVendorPrivateReq *) pc;
    int error;
    __GLXcontext * const cx = __glXForceCurrent(cl, req->contextTag, &error);

    pc += __GLX_VENDPRIV_HDR_SIZE;
    if ( cx != NULL ) {
        GLboolean retval;
        retval = CALL_IsRenderbufferEXT( GET_DISPATCH(), (
            *(GLuint   *)(pc +  0)
        ) );
        __glXSendReply(cl->client, dummy_answer, 0, 0, GL_FALSE, retval);
        error = Success;
    }

    return error;
}

void __glXDisp_RenderbufferStorageEXT(GLbyte * pc)
{
    CALL_RenderbufferStorageEXT( GET_DISPATCH(), (
        *(GLenum   *)(pc +  0),
        *(GLenum   *)(pc +  4),
        *(GLsizei  *)(pc +  8),
        *(GLsizei  *)(pc + 12)
    ) );
}

