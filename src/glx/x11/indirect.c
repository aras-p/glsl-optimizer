/* DO NOT EDIT - This file generated automatically by glX_proto_send.py (from Mesa) script */

/*
 * (C) Copyright IBM Corporation 2004
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


#include <GL/gl.h>
#include "indirect.h"
#include "glxclient.h"
#include "size.h"
#include <GL/glxproto.h>

#define __GLX_PAD(n) (((n) + 3) & ~3)

#  if defined(__i386__) && defined(__GNUC__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif
#  if defined(__GNUC__)
#    define NOINLINE __attribute__((noinline))
#  else
#    define NOINLINE
#  endif

#if !defined __GNUC__ || __GNUC__ < 3
#  define __builtin_expect(x, y) x
#endif

/* If the size and opcode values are known at compile-time, this will, on
 * x86 at least, emit them with a single instruction.
 */
#define emit_header(dest, op, size)            \
    do { union { short s[2]; int i; } temp;    \
         temp.s[0] = (size); temp.s[1] = (op); \
         *((int *)(dest)) = temp.i; } while(0)

static NOINLINE CARD32
read_reply( Display *dpy, size_t size, void * dest, GLboolean reply_is_always_array )
{
    xGLXSingleReply reply;
    
    (void) _XReply(dpy, (xReply *) & reply, 0, False);
    if (size != 0) {
	if ((reply.size >= 1) || reply_is_always_array) {
	    const GLint bytes = (reply_is_always_array) 
	      ? (4 * reply.length) : (reply.size * size);
	    const GLint extra = 4 - (bytes & 3);

	    _XRead(dpy, dest, bytes);
	    if ( extra != 0 ) {
		_XEatData(dpy, extra);
	    }
	}
	else {
	    (void) memcpy( dest, &(reply.pad3), size);
	}
    }

    return reply.retval;
}

#define X_GLXSingle 0

static NOINLINE FASTCALL GLubyte *
setup_single_request( __GLXcontext * gc, GLint sop, GLint cmdlen )
{
    xGLXSingleReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXSingle, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->contextTag = gc->currentContextTag;
    req->glxCode = sop;
    return (GLubyte *)(req) + sz_xGLXSingleReq;
}

static NOINLINE FASTCALL GLubyte *
setup_vendor_request( __GLXcontext * gc, GLint code, GLint vop, GLint cmdlen )
{
    xGLXVendorPrivateReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXVendorPrivate, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->glxCode = code;
    req->vendorCode = vop;
    req->contextTag = gc->currentContextTag;
    return (GLubyte *)(req) + sz_xGLXVendorPrivateReq;
}

static FASTCALL NOINLINE void
generic_3_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 7;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 3);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_4_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_6_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 10;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 6);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_8_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_12_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 12);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_16_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 16);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_24_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 24);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

static FASTCALL NOINLINE void
generic_32_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, 32);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_NewList 101
void
__indirect_glNewList(GLuint list, GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_NewList, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&list), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&mode), 4);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_EndList 102
void
__indirect_glEndList(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 0;
    if (__builtin_expect(dpy != NULL, 1)) {
        (void) setup_single_request(gc, X_GLsop_EndList, cmdlen);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_CallList 1
void
__indirect_glCallList(GLuint list)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_CallList, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&list), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CallLists 2
void
__indirect_glCallLists(GLsizei n, GLenum type, const GLvoid * lists)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glCallLists_size(type);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * n));
    if (__builtin_expect(gc->currentDpy != NULL, 1)) {
        if (cmdlen <= gc->maxSmallRenderCommandSize) {
            if ( (gc->pc + cmdlen) > gc->bufEnd ) {
                (void) __glXFlushRenderBuffer(gc, gc->pc);
            }
            emit_header(gc->pc, X_GLrop_CallLists, cmdlen);
            (void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
            (void) memcpy((void *)(gc->pc + 8), (void *)(&type), 4);
            (void) memcpy((void *)(gc->pc + 12), (void *)(lists), (compsize * n));
            gc->pc += cmdlen;
            if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
        }
        else {
            const GLint op = X_GLrop_CallLists;
            const GLuint cmdlenLarge = cmdlen + 4;
            GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
            (void) memcpy((void *)(pc + 0), (void *)(&op), 4);
            (void) memcpy((void *)(pc + 4), (void *)(&cmdlenLarge), 4);
            (void) memcpy((void *)(pc + 8), (void *)(&n), 4);
            (void) memcpy((void *)(pc + 12), (void *)(&type), 4);
            __glXSendLargeCommand(gc, pc, 16, lists, (compsize * n));
        }
    }
}

#define X_GLsop_DeleteLists 103
void
__indirect_glDeleteLists(GLuint list, GLsizei range)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_DeleteLists, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&list), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&range), 4);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GenLists 104
GLuint
__indirect_glGenLists(GLsizei range)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLuint retval = (GLuint) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GenLists, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&range), 4);
        retval = (GLuint) read_reply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_ListBase 3
void
__indirect_glListBase(GLuint base)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ListBase, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&base), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Begin 4
void
__indirect_glBegin(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Begin, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3bv 6
void
__indirect_glColor3b(GLbyte red, GLbyte green, GLbyte blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Color3bv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3bv 6
void
__indirect_glColor3bv(const GLbyte * v)
{
    generic_3_byte( X_GLrop_Color3bv, v );
}

#define X_GLrop_Color3dv 7
void
__indirect_glColor3d(GLdouble red, GLdouble green, GLdouble blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_Color3dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&green), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&blue), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3dv 7
void
__indirect_glColor3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_Color3dv, v );
}

#define X_GLrop_Color3fv 8
void
__indirect_glColor3f(GLfloat red, GLfloat green, GLfloat blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Color3fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3fv 8
void
__indirect_glColor3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_Color3fv, v );
}

#define X_GLrop_Color3iv 9
void
__indirect_glColor3i(GLint red, GLint green, GLint blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Color3iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3iv 9
void
__indirect_glColor3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_Color3iv, v );
}

#define X_GLrop_Color3sv 10
void
__indirect_glColor3s(GLshort red, GLshort green, GLshort blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Color3sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3sv 10
void
__indirect_glColor3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_Color3sv, v );
}

#define X_GLrop_Color3ubv 11
void
__indirect_glColor3ub(GLubyte red, GLubyte green, GLubyte blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Color3ubv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3ubv 11
void
__indirect_glColor3ubv(const GLubyte * v)
{
    generic_3_byte( X_GLrop_Color3ubv, v );
}

#define X_GLrop_Color3uiv 12
void
__indirect_glColor3ui(GLuint red, GLuint green, GLuint blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Color3uiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3uiv 12
void
__indirect_glColor3uiv(const GLuint * v)
{
    generic_12_byte( X_GLrop_Color3uiv, v );
}

#define X_GLrop_Color3usv 13
void
__indirect_glColor3us(GLushort red, GLushort green, GLushort blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Color3usv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color3usv 13
void
__indirect_glColor3usv(const GLushort * v)
{
    generic_6_byte( X_GLrop_Color3usv, v );
}

#define X_GLrop_Color4bv 14
void
__indirect_glColor4b(GLbyte red, GLbyte green, GLbyte blue, GLbyte alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Color4bv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    (void) memcpy((void *)(gc->pc + 7), (void *)(&alpha), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4bv 14
void
__indirect_glColor4bv(const GLbyte * v)
{
    generic_4_byte( X_GLrop_Color4bv, v );
}

#define X_GLrop_Color4dv 15
void
__indirect_glColor4d(GLdouble red, GLdouble green, GLdouble blue, GLdouble alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_Color4dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&green), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&blue), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&alpha), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4dv 15
void
__indirect_glColor4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_Color4dv, v );
}

#define X_GLrop_Color4fv 16
void
__indirect_glColor4f(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Color4fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4fv 16
void
__indirect_glColor4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_Color4fv, v );
}

#define X_GLrop_Color4iv 17
void
__indirect_glColor4i(GLint red, GLint green, GLint blue, GLint alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Color4iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4iv 17
void
__indirect_glColor4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_Color4iv, v );
}

#define X_GLrop_Color4sv 18
void
__indirect_glColor4s(GLshort red, GLshort green, GLshort blue, GLshort alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Color4sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&alpha), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4sv 18
void
__indirect_glColor4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_Color4sv, v );
}

#define X_GLrop_Color4ubv 19
void
__indirect_glColor4ub(GLubyte red, GLubyte green, GLubyte blue, GLubyte alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Color4ubv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    (void) memcpy((void *)(gc->pc + 7), (void *)(&alpha), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4ubv 19
void
__indirect_glColor4ubv(const GLubyte * v)
{
    generic_4_byte( X_GLrop_Color4ubv, v );
}

#define X_GLrop_Color4uiv 20
void
__indirect_glColor4ui(GLuint red, GLuint green, GLuint blue, GLuint alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Color4uiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4uiv 20
void
__indirect_glColor4uiv(const GLuint * v)
{
    generic_16_byte( X_GLrop_Color4uiv, v );
}

#define X_GLrop_Color4usv 21
void
__indirect_glColor4us(GLushort red, GLushort green, GLushort blue, GLushort alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Color4usv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&alpha), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Color4usv 21
void
__indirect_glColor4usv(const GLushort * v)
{
    generic_8_byte( X_GLrop_Color4usv, v );
}

#define X_GLrop_EdgeFlagv 22
void
__indirect_glEdgeFlag(GLboolean flag)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_EdgeFlagv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&flag), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EdgeFlagv 22
void
__indirect_glEdgeFlagv(const GLboolean * flag)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_EdgeFlagv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(flag), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_End 23
void
__indirect_glEnd(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_End, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexdv 24
void
__indirect_glIndexd(GLdouble c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Indexdv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&c), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexdv 24
void
__indirect_glIndexdv(const GLdouble * c)
{
    generic_8_byte( X_GLrop_Indexdv, c );
}

#define X_GLrop_Indexfv 25
void
__indirect_glIndexf(GLfloat c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Indexfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&c), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexfv 25
void
__indirect_glIndexfv(const GLfloat * c)
{
    generic_4_byte( X_GLrop_Indexfv, c );
}

#define X_GLrop_Indexiv 26
void
__indirect_glIndexi(GLint c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Indexiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&c), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexiv 26
void
__indirect_glIndexiv(const GLint * c)
{
    generic_4_byte( X_GLrop_Indexiv, c );
}

#define X_GLrop_Indexsv 27
void
__indirect_glIndexs(GLshort c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Indexsv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&c), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexsv 27
void
__indirect_glIndexsv(const GLshort * c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Indexsv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(c), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3bv 28
void
__indirect_glNormal3b(GLbyte nx, GLbyte ny, GLbyte nz)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Normal3bv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&ny), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&nz), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3bv 28
void
__indirect_glNormal3bv(const GLbyte * v)
{
    generic_3_byte( X_GLrop_Normal3bv, v );
}

#define X_GLrop_Normal3dv 29
void
__indirect_glNormal3d(GLdouble nx, GLdouble ny, GLdouble nz)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_Normal3dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&ny), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&nz), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3dv 29
void
__indirect_glNormal3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_Normal3dv, v );
}

#define X_GLrop_Normal3fv 30
void
__indirect_glNormal3f(GLfloat nx, GLfloat ny, GLfloat nz)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Normal3fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&ny), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&nz), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3fv 30
void
__indirect_glNormal3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_Normal3fv, v );
}

#define X_GLrop_Normal3iv 31
void
__indirect_glNormal3i(GLint nx, GLint ny, GLint nz)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Normal3iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&ny), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&nz), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3iv 31
void
__indirect_glNormal3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_Normal3iv, v );
}

#define X_GLrop_Normal3sv 32
void
__indirect_glNormal3s(GLshort nx, GLshort ny, GLshort nz)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Normal3sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&nx), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&ny), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&nz), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Normal3sv 32
void
__indirect_glNormal3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_Normal3sv, v );
}

#define X_GLrop_RasterPos2dv 33
void
__indirect_glRasterPos2d(GLdouble x, GLdouble y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_RasterPos2dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2dv 33
void
__indirect_glRasterPos2dv(const GLdouble * v)
{
    generic_16_byte( X_GLrop_RasterPos2dv, v );
}

#define X_GLrop_RasterPos2fv 34
void
__indirect_glRasterPos2f(GLfloat x, GLfloat y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_RasterPos2fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2fv 34
void
__indirect_glRasterPos2fv(const GLfloat * v)
{
    generic_8_byte( X_GLrop_RasterPos2fv, v );
}

#define X_GLrop_RasterPos2iv 35
void
__indirect_glRasterPos2i(GLint x, GLint y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_RasterPos2iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2iv 35
void
__indirect_glRasterPos2iv(const GLint * v)
{
    generic_8_byte( X_GLrop_RasterPos2iv, v );
}

#define X_GLrop_RasterPos2sv 36
void
__indirect_glRasterPos2s(GLshort x, GLshort y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_RasterPos2sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos2sv 36
void
__indirect_glRasterPos2sv(const GLshort * v)
{
    generic_4_byte( X_GLrop_RasterPos2sv, v );
}

#define X_GLrop_RasterPos3dv 37
void
__indirect_glRasterPos3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_RasterPos3dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3dv 37
void
__indirect_glRasterPos3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_RasterPos3dv, v );
}

#define X_GLrop_RasterPos3fv 38
void
__indirect_glRasterPos3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_RasterPos3fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3fv 38
void
__indirect_glRasterPos3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_RasterPos3fv, v );
}

#define X_GLrop_RasterPos3iv 39
void
__indirect_glRasterPos3i(GLint x, GLint y, GLint z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_RasterPos3iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3iv 39
void
__indirect_glRasterPos3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_RasterPos3iv, v );
}

#define X_GLrop_RasterPos3sv 40
void
__indirect_glRasterPos3s(GLshort x, GLshort y, GLshort z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_RasterPos3sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos3sv 40
void
__indirect_glRasterPos3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_RasterPos3sv, v );
}

#define X_GLrop_RasterPos4dv 41
void
__indirect_glRasterPos4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_RasterPos4dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&w), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4dv 41
void
__indirect_glRasterPos4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_RasterPos4dv, v );
}

#define X_GLrop_RasterPos4fv 42
void
__indirect_glRasterPos4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_RasterPos4fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4fv 42
void
__indirect_glRasterPos4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_RasterPos4fv, v );
}

#define X_GLrop_RasterPos4iv 43
void
__indirect_glRasterPos4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_RasterPos4iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4iv 43
void
__indirect_glRasterPos4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_RasterPos4iv, v );
}

#define X_GLrop_RasterPos4sv 44
void
__indirect_glRasterPos4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_RasterPos4sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&w), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_RasterPos4sv 44
void
__indirect_glRasterPos4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_RasterPos4sv, v );
}

#define X_GLrop_Rectdv 45
void
__indirect_glRectd(GLdouble x1, GLdouble y1, GLdouble x2, GLdouble y2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_Rectdv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y1), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&x2), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&y2), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectdv 45
void
__indirect_glRectdv(const GLdouble * v1, const GLdouble * v2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_Rectdv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v1), 16);
    (void) memcpy((void *)(gc->pc + 20), (void *)(v2), 16);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectfv 46
void
__indirect_glRectf(GLfloat x1, GLfloat y1, GLfloat x2, GLfloat y2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Rectfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y1), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x2), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&y2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectfv 46
void
__indirect_glRectfv(const GLfloat * v1, const GLfloat * v2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Rectfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v1), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(v2), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectiv 47
void
__indirect_glRecti(GLint x1, GLint y1, GLint x2, GLint y2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Rectiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y1), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x2), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&y2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectiv 47
void
__indirect_glRectiv(const GLint * v1, const GLint * v2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Rectiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v1), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(v2), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectsv 48
void
__indirect_glRects(GLshort x1, GLshort y1, GLshort x2, GLshort y2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Rectsv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x1), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y1), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&x2), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&y2), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rectsv 48
void
__indirect_glRectsv(const GLshort * v1, const GLshort * v2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Rectsv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v1), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1dv 49
void
__indirect_glTexCoord1d(GLdouble s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_TexCoord1dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1dv 49
void
__indirect_glTexCoord1dv(const GLdouble * v)
{
    generic_8_byte( X_GLrop_TexCoord1dv, v );
}

#define X_GLrop_TexCoord1fv 50
void
__indirect_glTexCoord1f(GLfloat s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_TexCoord1fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1fv 50
void
__indirect_glTexCoord1fv(const GLfloat * v)
{
    generic_4_byte( X_GLrop_TexCoord1fv, v );
}

#define X_GLrop_TexCoord1iv 51
void
__indirect_glTexCoord1i(GLint s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_TexCoord1iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1iv 51
void
__indirect_glTexCoord1iv(const GLint * v)
{
    generic_4_byte( X_GLrop_TexCoord1iv, v );
}

#define X_GLrop_TexCoord1sv 52
void
__indirect_glTexCoord1s(GLshort s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_TexCoord1sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord1sv 52
void
__indirect_glTexCoord1sv(const GLshort * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_TexCoord1sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2dv 53
void
__indirect_glTexCoord2d(GLdouble s, GLdouble t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_TexCoord2dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2dv 53
void
__indirect_glTexCoord2dv(const GLdouble * v)
{
    generic_16_byte( X_GLrop_TexCoord2dv, v );
}

#define X_GLrop_TexCoord2fv 54
void
__indirect_glTexCoord2f(GLfloat s, GLfloat t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_TexCoord2fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2fv 54
void
__indirect_glTexCoord2fv(const GLfloat * v)
{
    generic_8_byte( X_GLrop_TexCoord2fv, v );
}

#define X_GLrop_TexCoord2iv 55
void
__indirect_glTexCoord2i(GLint s, GLint t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_TexCoord2iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2iv 55
void
__indirect_glTexCoord2iv(const GLint * v)
{
    generic_8_byte( X_GLrop_TexCoord2iv, v );
}

#define X_GLrop_TexCoord2sv 56
void
__indirect_glTexCoord2s(GLshort s, GLshort t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_TexCoord2sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&t), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord2sv 56
void
__indirect_glTexCoord2sv(const GLshort * v)
{
    generic_4_byte( X_GLrop_TexCoord2sv, v );
}

#define X_GLrop_TexCoord3dv 57
void
__indirect_glTexCoord3d(GLdouble s, GLdouble t, GLdouble r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_TexCoord3dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3dv 57
void
__indirect_glTexCoord3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_TexCoord3dv, v );
}

#define X_GLrop_TexCoord3fv 58
void
__indirect_glTexCoord3f(GLfloat s, GLfloat t, GLfloat r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexCoord3fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3fv 58
void
__indirect_glTexCoord3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_TexCoord3fv, v );
}

#define X_GLrop_TexCoord3iv 59
void
__indirect_glTexCoord3i(GLint s, GLint t, GLint r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexCoord3iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3iv 59
void
__indirect_glTexCoord3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_TexCoord3iv, v );
}

#define X_GLrop_TexCoord3sv 60
void
__indirect_glTexCoord3s(GLshort s, GLshort t, GLshort r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_TexCoord3sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&t), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&r), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord3sv 60
void
__indirect_glTexCoord3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_TexCoord3sv, v );
}

#define X_GLrop_TexCoord4dv 61
void
__indirect_glTexCoord4d(GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_TexCoord4dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&q), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4dv 61
void
__indirect_glTexCoord4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_TexCoord4dv, v );
}

#define X_GLrop_TexCoord4fv 62
void
__indirect_glTexCoord4f(GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_TexCoord4fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&q), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4fv 62
void
__indirect_glTexCoord4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_TexCoord4fv, v );
}

#define X_GLrop_TexCoord4iv 63
void
__indirect_glTexCoord4i(GLint s, GLint t, GLint r, GLint q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_TexCoord4iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&r), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&q), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4iv 63
void
__indirect_glTexCoord4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_TexCoord4iv, v );
}

#define X_GLrop_TexCoord4sv 64
void
__indirect_glTexCoord4s(GLshort s, GLshort t, GLshort r, GLshort q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_TexCoord4sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&t), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&r), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&q), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexCoord4sv 64
void
__indirect_glTexCoord4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_TexCoord4sv, v );
}

#define X_GLrop_Vertex2dv 65
void
__indirect_glVertex2d(GLdouble x, GLdouble y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Vertex2dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2dv 65
void
__indirect_glVertex2dv(const GLdouble * v)
{
    generic_16_byte( X_GLrop_Vertex2dv, v );
}

#define X_GLrop_Vertex2fv 66
void
__indirect_glVertex2f(GLfloat x, GLfloat y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Vertex2fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2fv 66
void
__indirect_glVertex2fv(const GLfloat * v)
{
    generic_8_byte( X_GLrop_Vertex2fv, v );
}

#define X_GLrop_Vertex2iv 67
void
__indirect_glVertex2i(GLint x, GLint y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Vertex2iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2iv 67
void
__indirect_glVertex2iv(const GLint * v)
{
    generic_8_byte( X_GLrop_Vertex2iv, v );
}

#define X_GLrop_Vertex2sv 68
void
__indirect_glVertex2s(GLshort x, GLshort y)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Vertex2sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex2sv 68
void
__indirect_glVertex2sv(const GLshort * v)
{
    generic_4_byte( X_GLrop_Vertex2sv, v );
}

#define X_GLrop_Vertex3dv 69
void
__indirect_glVertex3d(GLdouble x, GLdouble y, GLdouble z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_Vertex3dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3dv 69
void
__indirect_glVertex3dv(const GLdouble * v)
{
    generic_24_byte( X_GLrop_Vertex3dv, v );
}

#define X_GLrop_Vertex3fv 70
void
__indirect_glVertex3f(GLfloat x, GLfloat y, GLfloat z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Vertex3fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3fv 70
void
__indirect_glVertex3fv(const GLfloat * v)
{
    generic_12_byte( X_GLrop_Vertex3fv, v );
}

#define X_GLrop_Vertex3iv 71
void
__indirect_glVertex3i(GLint x, GLint y, GLint z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Vertex3iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3iv 71
void
__indirect_glVertex3iv(const GLint * v)
{
    generic_12_byte( X_GLrop_Vertex3iv, v );
}

#define X_GLrop_Vertex3sv 72
void
__indirect_glVertex3s(GLshort x, GLshort y, GLshort z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Vertex3sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex3sv 72
void
__indirect_glVertex3sv(const GLshort * v)
{
    generic_6_byte( X_GLrop_Vertex3sv, v );
}

#define X_GLrop_Vertex4dv 73
void
__indirect_glVertex4d(GLdouble x, GLdouble y, GLdouble z, GLdouble w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_Vertex4dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&w), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4dv 73
void
__indirect_glVertex4dv(const GLdouble * v)
{
    generic_32_byte( X_GLrop_Vertex4dv, v );
}

#define X_GLrop_Vertex4fv 74
void
__indirect_glVertex4f(GLfloat x, GLfloat y, GLfloat z, GLfloat w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Vertex4fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4fv 74
void
__indirect_glVertex4fv(const GLfloat * v)
{
    generic_16_byte( X_GLrop_Vertex4fv, v );
}

#define X_GLrop_Vertex4iv 75
void
__indirect_glVertex4i(GLint x, GLint y, GLint z, GLint w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Vertex4iv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&w), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4iv 75
void
__indirect_glVertex4iv(const GLint * v)
{
    generic_16_byte( X_GLrop_Vertex4iv, v );
}

#define X_GLrop_Vertex4sv 76
void
__indirect_glVertex4s(GLshort x, GLshort y, GLshort z, GLshort w)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Vertex4sv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&y), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&z), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&w), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Vertex4sv 76
void
__indirect_glVertex4sv(const GLshort * v)
{
    generic_8_byte( X_GLrop_Vertex4sv, v );
}

#define X_GLrop_ClipPlane 77
void
__indirect_glClipPlane(GLenum plane, const GLdouble * equation)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
    emit_header(gc->pc, X_GLrop_ClipPlane, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(equation), 32);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&plane), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorMaterial 78
void
__indirect_glColorMaterial(GLenum face, GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_ColorMaterial, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CullFace 79
void
__indirect_glCullFace(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_CullFace, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogf 80
void
__indirect_glFogf(GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Fogf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogfv 81
void
__indirect_glFogfv(GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glFogfv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_Fogfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogi 82
void
__indirect_glFogi(GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Fogi, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Fogiv 83
void
__indirect_glFogiv(GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glFogiv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_Fogiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FrontFace 84
void
__indirect_glFrontFace(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_FrontFace, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Hint 85
void
__indirect_glHint(GLenum target, GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Hint, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lightf 86
void
__indirect_glLightf(GLenum light, GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Lightf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lightfv 87
void
__indirect_glLightfv(GLenum light, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_Lightfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lighti 88
void
__indirect_glLighti(GLenum light, GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Lighti, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Lightiv 89
void
__indirect_glLightiv(GLenum light, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightiv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_Lightiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&light), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModelf 90
void
__indirect_glLightModelf(GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_LightModelf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModelfv 91
void
__indirect_glLightModelfv(GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightModelfv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_LightModelfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModeli 92
void
__indirect_glLightModeli(GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_LightModeli, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LightModeliv 93
void
__indirect_glLightModeliv(GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glLightModeliv_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_LightModeliv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LineStipple 94
void
__indirect_glLineStipple(GLint factor, GLushort pattern)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_LineStipple, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&factor), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pattern), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LineWidth 95
void
__indirect_glLineWidth(GLfloat width)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_LineWidth, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&width), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materialf 96
void
__indirect_glMaterialf(GLenum face, GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Materialf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materialfv 97
void
__indirect_glMaterialfv(GLenum face, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glMaterialfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_Materialfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materiali 98
void
__indirect_glMateriali(GLenum face, GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Materiali, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Materialiv 99
void
__indirect_glMaterialiv(GLenum face, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glMaterialiv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_Materialiv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointSize 100
void
__indirect_glPointSize(GLfloat size)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_PointSize, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&size), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PolygonMode 101
void
__indirect_glPolygonMode(GLenum face, GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PolygonMode, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Scissor 103
void
__indirect_glScissor(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Scissor, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ShadeModel 104
void
__indirect_glShadeModel(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ShadeModel, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameterf 105
void
__indirect_glTexParameterf(GLenum target, GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexParameterf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameterfv 106
void
__indirect_glTexParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexParameterfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_TexParameterfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameteri 107
void
__indirect_glTexParameteri(GLenum target, GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexParameteri, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexParameteriv 108
void
__indirect_glTexParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexParameteriv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_TexParameteriv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnvf 111
void
__indirect_glTexEnvf(GLenum target, GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexEnvf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnvfv 112
void
__indirect_glTexEnvfv(GLenum target, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexEnvfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_TexEnvfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnvi 113
void
__indirect_glTexEnvi(GLenum target, GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexEnvi, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexEnviv 114
void
__indirect_glTexEnviv(GLenum target, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexEnviv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_TexEnviv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGend 115
void
__indirect_glTexGend(GLenum coord, GLenum pname, GLdouble param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_TexGend, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&param), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&coord), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&pname), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGendv 116
void
__indirect_glTexGendv(GLenum coord, GLenum pname, const GLdouble * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexGendv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 8));
    emit_header(gc->pc, X_GLrop_TexGendv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 8));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGenf 117
void
__indirect_glTexGenf(GLenum coord, GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexGenf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGenfv 118
void
__indirect_glTexGenfv(GLenum coord, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexGenfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_TexGenfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGeni 119
void
__indirect_glTexGeni(GLenum coord, GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_TexGeni, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_TexGeniv 120
void
__indirect_glTexGeniv(GLenum coord, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glTexGeniv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_TexGeniv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_InitNames 121
void
__indirect_glInitNames(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_InitNames, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadName 122
void
__indirect_glLoadName(GLuint name)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_LoadName, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&name), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PassThrough 123
void
__indirect_glPassThrough(GLfloat token)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_PassThrough, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&token), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PopName 124
void
__indirect_glPopName(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_PopName, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PushName 125
void
__indirect_glPushName(GLuint name)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_PushName, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&name), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_DrawBuffer 126
void
__indirect_glDrawBuffer(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_DrawBuffer, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Clear 127
void
__indirect_glClear(GLbitfield mask)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Clear, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearAccum 128
void
__indirect_glClearAccum(GLfloat red, GLfloat green, GLfloat blue, GLfloat alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_ClearAccum, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearIndex 129
void
__indirect_glClearIndex(GLfloat c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ClearIndex, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&c), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearColor 130
void
__indirect_glClearColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_ClearColor, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearStencil 131
void
__indirect_glClearStencil(GLint s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ClearStencil, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ClearDepth 132
void
__indirect_glClearDepth(GLclampd depth)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_ClearDepth, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&depth), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_StencilMask 133
void
__indirect_glStencilMask(GLuint mask)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_StencilMask, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorMask 134
void
__indirect_glColorMask(GLboolean red, GLboolean green, GLboolean blue, GLboolean alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ColorMask, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    (void) memcpy((void *)(gc->pc + 7), (void *)(&alpha), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_DepthMask 135
void
__indirect_glDepthMask(GLboolean flag)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_DepthMask, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&flag), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_IndexMask 136
void
__indirect_glIndexMask(GLuint mask)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_IndexMask, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Accum 137
void
__indirect_glAccum(GLenum op, GLfloat value)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_Accum, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&op), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&value), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PopAttrib 141
void
__indirect_glPopAttrib(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_PopAttrib, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PushAttrib 142
void
__indirect_glPushAttrib(GLbitfield mask)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_PushAttrib, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mask), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid1d 147
void
__indirect_glMapGrid1d(GLint un, GLdouble u1, GLdouble u2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MapGrid1d, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&u1), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&un), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid1f 148
void
__indirect_glMapGrid1f(GLint un, GLfloat u1, GLfloat u2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MapGrid1f, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&un), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&u1), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid2d 149
void
__indirect_glMapGrid2d(GLint un, GLdouble u1, GLdouble u2, GLint vn, GLdouble v1, GLdouble v2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 44;
    emit_header(gc->pc, X_GLrop_MapGrid2d, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&u1), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&v1), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&v2), 8);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&un), 4);
    (void) memcpy((void *)(gc->pc + 40), (void *)(&vn), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MapGrid2f 150
void
__indirect_glMapGrid2f(GLint un, GLfloat u1, GLfloat u2, GLint vn, GLfloat v1, GLfloat v2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_MapGrid2f, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&un), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&u1), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&u2), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&vn), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&v1), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&v2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord1dv 151
void
__indirect_glEvalCoord1d(GLdouble u)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_EvalCoord1dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&u), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord1dv 151
void
__indirect_glEvalCoord1dv(const GLdouble * u)
{
    generic_8_byte( X_GLrop_EvalCoord1dv, u );
}

#define X_GLrop_EvalCoord1fv 152
void
__indirect_glEvalCoord1f(GLfloat u)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_EvalCoord1fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&u), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord1fv 152
void
__indirect_glEvalCoord1fv(const GLfloat * u)
{
    generic_4_byte( X_GLrop_EvalCoord1fv, u );
}

#define X_GLrop_EvalCoord2dv 153
void
__indirect_glEvalCoord2d(GLdouble u, GLdouble v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_EvalCoord2dv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&u), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&v), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord2dv 153
void
__indirect_glEvalCoord2dv(const GLdouble * u)
{
    generic_16_byte( X_GLrop_EvalCoord2dv, u );
}

#define X_GLrop_EvalCoord2fv 154
void
__indirect_glEvalCoord2f(GLfloat u, GLfloat v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_EvalCoord2fv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&u), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&v), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalCoord2fv 154
void
__indirect_glEvalCoord2fv(const GLfloat * u)
{
    generic_8_byte( X_GLrop_EvalCoord2fv, u );
}

#define X_GLrop_EvalMesh1 155
void
__indirect_glEvalMesh1(GLenum mode, GLint i1, GLint i2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_EvalMesh1, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&i1), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&i2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalPoint1 156
void
__indirect_glEvalPoint1(GLint i)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_EvalPoint1, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&i), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalMesh2 157
void
__indirect_glEvalMesh2(GLenum mode, GLint i1, GLint i2, GLint j1, GLint j2)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_EvalMesh2, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&i1), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&i2), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&j1), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&j2), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_EvalPoint2 158
void
__indirect_glEvalPoint2(GLint i, GLint j)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_EvalPoint2, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&i), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&j), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_AlphaFunc 159
void
__indirect_glAlphaFunc(GLenum func, GLclampf ref)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_AlphaFunc, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&func), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&ref), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BlendFunc 160
void
__indirect_glBlendFunc(GLenum sfactor, GLenum dfactor)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_BlendFunc, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&sfactor), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&dfactor), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LogicOp 161
void
__indirect_glLogicOp(GLenum opcode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_LogicOp, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&opcode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_StencilFunc 162
void
__indirect_glStencilFunc(GLenum func, GLint ref, GLuint mask)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_StencilFunc, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&func), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&ref), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&mask), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_StencilOp 163
void
__indirect_glStencilOp(GLenum fail, GLenum zfail, GLenum zpass)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_StencilOp, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&fail), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&zfail), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&zpass), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_DepthFunc 164
void
__indirect_glDepthFunc(GLenum func)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_DepthFunc, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&func), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelZoom 165
void
__indirect_glPixelZoom(GLfloat xfactor, GLfloat yfactor)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PixelZoom, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&xfactor), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&yfactor), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelTransferf 166
void
__indirect_glPixelTransferf(GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PixelTransferf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelTransferi 167
void
__indirect_glPixelTransferi(GLenum pname, GLint param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PixelTransferi, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PixelMapfv 168
void
__indirect_glPixelMapfv(GLenum map, GLsizei mapsize, const GLfloat * values)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((mapsize * 4));
    if (__builtin_expect((mapsize >= 0) && (gc->currentDpy != NULL), 1)) {
        if (cmdlen <= gc->maxSmallRenderCommandSize) {
            if ( (gc->pc + cmdlen) > gc->bufEnd ) {
                (void) __glXFlushRenderBuffer(gc, gc->pc);
            }
            emit_header(gc->pc, X_GLrop_PixelMapfv, cmdlen);
            (void) memcpy((void *)(gc->pc + 4), (void *)(&map), 4);
            (void) memcpy((void *)(gc->pc + 8), (void *)(&mapsize), 4);
            (void) memcpy((void *)(gc->pc + 12), (void *)(values), (mapsize * 4));
            gc->pc += cmdlen;
            if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
        }
        else {
            const GLint op = X_GLrop_PixelMapfv;
            const GLuint cmdlenLarge = cmdlen + 4;
            GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
            (void) memcpy((void *)(pc + 0), (void *)(&op), 4);
            (void) memcpy((void *)(pc + 4), (void *)(&cmdlenLarge), 4);
            (void) memcpy((void *)(pc + 8), (void *)(&map), 4);
            (void) memcpy((void *)(pc + 12), (void *)(&mapsize), 4);
            __glXSendLargeCommand(gc, pc, 16, values, (mapsize * 4));
        }
    }
}

#define X_GLrop_PixelMapuiv 169
void
__indirect_glPixelMapuiv(GLenum map, GLsizei mapsize, const GLuint * values)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((mapsize * 4));
    if (__builtin_expect((mapsize >= 0) && (gc->currentDpy != NULL), 1)) {
        if (cmdlen <= gc->maxSmallRenderCommandSize) {
            if ( (gc->pc + cmdlen) > gc->bufEnd ) {
                (void) __glXFlushRenderBuffer(gc, gc->pc);
            }
            emit_header(gc->pc, X_GLrop_PixelMapuiv, cmdlen);
            (void) memcpy((void *)(gc->pc + 4), (void *)(&map), 4);
            (void) memcpy((void *)(gc->pc + 8), (void *)(&mapsize), 4);
            (void) memcpy((void *)(gc->pc + 12), (void *)(values), (mapsize * 4));
            gc->pc += cmdlen;
            if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
        }
        else {
            const GLint op = X_GLrop_PixelMapuiv;
            const GLuint cmdlenLarge = cmdlen + 4;
            GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
            (void) memcpy((void *)(pc + 0), (void *)(&op), 4);
            (void) memcpy((void *)(pc + 4), (void *)(&cmdlenLarge), 4);
            (void) memcpy((void *)(pc + 8), (void *)(&map), 4);
            (void) memcpy((void *)(pc + 12), (void *)(&mapsize), 4);
            __glXSendLargeCommand(gc, pc, 16, values, (mapsize * 4));
        }
    }
}

#define X_GLrop_PixelMapusv 170
void
__indirect_glPixelMapusv(GLenum map, GLsizei mapsize, const GLushort * values)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12 + __GLX_PAD((mapsize * 2));
    if (__builtin_expect((mapsize >= 0) && (gc->currentDpy != NULL), 1)) {
        if (cmdlen <= gc->maxSmallRenderCommandSize) {
            if ( (gc->pc + cmdlen) > gc->bufEnd ) {
                (void) __glXFlushRenderBuffer(gc, gc->pc);
            }
            emit_header(gc->pc, X_GLrop_PixelMapusv, cmdlen);
            (void) memcpy((void *)(gc->pc + 4), (void *)(&map), 4);
            (void) memcpy((void *)(gc->pc + 8), (void *)(&mapsize), 4);
            (void) memcpy((void *)(gc->pc + 12), (void *)(values), (mapsize * 2));
            gc->pc += cmdlen;
            if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
        }
        else {
            const GLint op = X_GLrop_PixelMapusv;
            const GLuint cmdlenLarge = cmdlen + 4;
            GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);
            (void) memcpy((void *)(pc + 0), (void *)(&op), 4);
            (void) memcpy((void *)(pc + 4), (void *)(&cmdlenLarge), 4);
            (void) memcpy((void *)(pc + 8), (void *)(&map), 4);
            (void) memcpy((void *)(pc + 12), (void *)(&mapsize), 4);
            __glXSendLargeCommand(gc, pc, 16, values, (mapsize * 2));
        }
    }
}

#define X_GLrop_ReadBuffer 171
void
__indirect_glReadBuffer(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ReadBuffer, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyPixels 172
void
__indirect_glCopyPixels(GLint x, GLint y, GLsizei width, GLsizei height, GLenum type)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_CopyPixels, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&type), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_GetClipPlane 113
void
__indirect_glGetClipPlane(GLenum plane, GLdouble * equation)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetClipPlane, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&plane), 4);
        (void) read_reply(dpy, 8, equation, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetLightfv 118
void
__indirect_glGetLightfv(GLenum light, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetLightfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&light), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetLightiv 119
void
__indirect_glGetLightiv(GLenum light, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetLightiv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&light), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMapdv 120
void
__indirect_glGetMapdv(GLenum target, GLenum query, GLdouble * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMapdv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&query), 4);
        (void) read_reply(dpy, 8, v, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMapfv 121
void
__indirect_glGetMapfv(GLenum target, GLenum query, GLfloat * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMapfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&query), 4);
        (void) read_reply(dpy, 4, v, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMapiv 122
void
__indirect_glGetMapiv(GLenum target, GLenum query, GLint * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMapiv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&query), 4);
        (void) read_reply(dpy, 4, v, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMaterialfv 123
void
__indirect_glGetMaterialfv(GLenum face, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMaterialfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&face), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMaterialiv 124
void
__indirect_glGetMaterialiv(GLenum face, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMaterialiv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&face), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetPixelMapfv 125
void
__indirect_glGetPixelMapfv(GLenum map, GLfloat * values)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetPixelMapfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&map), 4);
        (void) read_reply(dpy, 4, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetPixelMapuiv 126
void
__indirect_glGetPixelMapuiv(GLenum map, GLuint * values)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetPixelMapuiv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&map), 4);
        (void) read_reply(dpy, 4, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetPixelMapusv 127
void
__indirect_glGetPixelMapusv(GLenum map, GLushort * values)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetPixelMapusv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&map), 4);
        (void) read_reply(dpy, 2, values, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexEnvfv 130
void
__indirect_glGetTexEnvfv(GLenum target, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexEnvfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexEnviv 131
void
__indirect_glGetTexEnviv(GLenum target, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexEnviv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexGendv 132
void
__indirect_glGetTexGendv(GLenum coord, GLenum pname, GLdouble * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexGendv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&coord), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 8, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexGenfv 133
void
__indirect_glGetTexGenfv(GLenum coord, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexGenfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&coord), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexGeniv 134
void
__indirect_glGetTexGeniv(GLenum coord, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexGeniv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&coord), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexParameterfv 136
void
__indirect_glGetTexParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexParameterfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexParameteriv 137
void
__indirect_glGetTexParameteriv(GLenum target, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexParameteriv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexLevelParameterfv 138
void
__indirect_glGetTexLevelParameterfv(GLenum target, GLint level, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 12;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexLevelParameterfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&level), 4);
        (void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetTexLevelParameteriv 139
void
__indirect_glGetTexLevelParameteriv(GLenum target, GLint level, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 12;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetTexLevelParameteriv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&level), 4);
        (void) memcpy((void *)(pc + 8), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_IsList 141
GLboolean
__indirect_glIsList(GLuint list)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_IsList, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&list), 4);
        retval = (GLboolean) read_reply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_DepthRange 174
void
__indirect_glDepthRange(GLclampd zNear, GLclampd zFar)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_DepthRange, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&zNear), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&zFar), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Frustum 175
void
__indirect_glFrustum(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 52;
    emit_header(gc->pc, X_GLrop_Frustum, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&left), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&right), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&bottom), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&top), 8);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&zNear), 8);
    (void) memcpy((void *)(gc->pc + 44), (void *)(&zFar), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadIdentity 176
void
__indirect_glLoadIdentity(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_LoadIdentity, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadMatrixf 177
void
__indirect_glLoadMatrixf(const GLfloat * m)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 68;
    emit_header(gc->pc, X_GLrop_LoadMatrixf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(m), 64);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_LoadMatrixd 178
void
__indirect_glLoadMatrixd(const GLdouble * m)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 132;
    emit_header(gc->pc, X_GLrop_LoadMatrixd, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(m), 128);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MatrixMode 179
void
__indirect_glMatrixMode(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_MatrixMode, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultMatrixf 180
void
__indirect_glMultMatrixf(const GLfloat * m)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 68;
    emit_header(gc->pc, X_GLrop_MultMatrixf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(m), 64);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultMatrixd 181
void
__indirect_glMultMatrixd(const GLdouble * m)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 132;
    emit_header(gc->pc, X_GLrop_MultMatrixd, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(m), 128);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Ortho 182
void
__indirect_glOrtho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 52;
    emit_header(gc->pc, X_GLrop_Ortho, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&left), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&right), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&bottom), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&top), 8);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&zNear), 8);
    (void) memcpy((void *)(gc->pc + 44), (void *)(&zFar), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PopMatrix 183
void
__indirect_glPopMatrix(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_PopMatrix, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PushMatrix 184
void
__indirect_glPushMatrix(void)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 4;
    emit_header(gc->pc, X_GLrop_PushMatrix, cmdlen);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rotated 185
void
__indirect_glRotated(GLdouble angle, GLdouble x, GLdouble y, GLdouble z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_Rotated, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&angle), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&z), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Rotatef 186
void
__indirect_glRotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Rotatef, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&angle), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Scaled 187
void
__indirect_glScaled(GLdouble x, GLdouble y, GLdouble z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_Scaled, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Scalef 188
void
__indirect_glScalef(GLfloat x, GLfloat y, GLfloat z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Scalef, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Translated 189
void
__indirect_glTranslated(GLdouble x, GLdouble y, GLdouble z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_Translated, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&y), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&z), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Translatef 190
void
__indirect_glTranslatef(GLfloat x, GLfloat y, GLfloat z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Translatef, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Viewport 191
void
__indirect_glViewport(GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Viewport, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&height), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BindTexture 4117
void
__indirect_glBindTexture(GLenum target, GLuint texture)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_BindTexture, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&texture), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexubv 194
void
__indirect_glIndexub(GLubyte c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Indexubv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&c), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Indexubv 194
void
__indirect_glIndexubv(const GLubyte * c)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_Indexubv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(c), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PolygonOffset 192
void
__indirect_glPolygonOffset(GLfloat factor, GLfloat units)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PolygonOffset, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&factor), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&units), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_AreTexturesResident 143
GLboolean
__indirect_glAreTexturesResident(GLsizei n, const GLuint * textures, GLboolean * residences)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_AreTexturesResident, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) memcpy((void *)(pc + 4), (void *)(textures), (n * 4));
        retval = (GLboolean) read_reply(dpy, 1, residences, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_CopyTexImage1D 4119
void
__indirect_glCopyTexImage1D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLint border)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
    emit_header(gc->pc, X_GLrop_CopyTexImage1D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&border), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexImage2D 4120
void
__indirect_glCopyTexImage2D(GLenum target, GLint level, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height, GLint border)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_CopyTexImage2D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&height), 4);
    (void) memcpy((void *)(gc->pc + 32), (void *)(&border), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexSubImage1D 4121
void
__indirect_glCopyTexSubImage1D(GLenum target, GLint level, GLint xoffset, GLint x, GLint y, GLsizei width)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_CopyTexSubImage1D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&xoffset), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&width), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexSubImage2D 4122
void
__indirect_glCopyTexSubImage2D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 36;
    emit_header(gc->pc, X_GLrop_CopyTexSubImage2D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&xoffset), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&yoffset), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 32), (void *)(&height), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_DeleteTextures 144
void
__indirect_glDeleteTextures(GLsizei n, const GLuint * textures)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_DeleteTextures, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) memcpy((void *)(pc + 4), (void *)(textures), (n * 4));
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GenTextures 145
void
__indirect_glGenTextures(GLsizei n, GLuint * textures)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GenTextures, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) read_reply(dpy, 4, textures, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_IsTexture 146
GLboolean
__indirect_glIsTexture(GLuint texture)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_IsTexture, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&texture), 4);
        retval = (GLboolean) read_reply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_PrioritizeTextures 4118
void
__indirect_glPrioritizeTextures(GLsizei n, const GLuint * textures, const GLclampf * priorities)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8 + __GLX_PAD((n * 4)) + __GLX_PAD((n * 4));
    if (__builtin_expect(n >= 0, 1)) {
        emit_header(gc->pc, X_GLrop_PrioritizeTextures, cmdlen);
        (void) memcpy((void *)(gc->pc + 4), (void *)(&n), 4);
        (void) memcpy((void *)(gc->pc + 8), (void *)(textures), (n * 4));
        (void) memcpy((void *)(gc->pc + 8), (void *)(priorities), (n * 4));
        gc->pc += cmdlen;
        if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
    }
}

#define X_GLrop_BlendColor 4096
void
__indirect_glBlendColor(GLclampf red, GLclampf green, GLclampf blue, GLclampf alpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_BlendColor, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&alpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_BlendEquation 4097
void
__indirect_glBlendEquation(GLenum mode)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_BlendEquation, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&mode), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorTableParameterfv 2054
void
__indirect_glColorTableParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glColorTableParameterfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_ColorTableParameterfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ColorTableParameteriv 2055
void
__indirect_glColorTableParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glColorTableParameteriv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_ColorTableParameteriv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyColorTable 2056
void
__indirect_glCopyColorTable(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_CopyColorTable, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_GetColorTableParameterfv 148
void
__indirect_glGetColorTableParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetColorTableParameterfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetColorTableParameteriv 149
void
__indirect_glGetColorTableParameteriv(GLenum target, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetColorTableParameteriv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_CopyColorSubTable 196
void
__indirect_glCopyColorSubTable(GLenum target, GLsizei start, GLint x, GLint y, GLsizei width)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_CopyColorSubTable, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&start), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameterf 4103
void
__indirect_glConvolutionParameterf(GLenum target, GLenum pname, GLfloat params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_ConvolutionParameterf, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&params), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameterfv 4104
void
__indirect_glConvolutionParameterfv(GLenum target, GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glConvolutionParameterfv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_ConvolutionParameterfv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameteri 4105
void
__indirect_glConvolutionParameteri(GLenum target, GLenum pname, GLint params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_ConvolutionParameteri, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&params), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ConvolutionParameteriv 4106
void
__indirect_glConvolutionParameteriv(GLenum target, GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glConvolutionParameteriv_size(pname);
    const GLuint cmdlen = 12 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_ConvolutionParameteriv, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyConvolutionFilter1D 4107
void
__indirect_glCopyConvolutionFilter1D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_CopyConvolutionFilter1D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyConvolutionFilter2D 4108
void
__indirect_glCopyConvolutionFilter2D(GLenum target, GLenum internalformat, GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_CopyConvolutionFilter2D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&height), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLsop_GetConvolutionParameterfv 151
void
__indirect_glGetConvolutionParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetConvolutionParameterfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetConvolutionParameteriv 152
void
__indirect_glGetConvolutionParameteriv(GLenum target, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetConvolutionParameteriv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetHistogramParameterfv 155
void
__indirect_glGetHistogramParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetHistogramParameterfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetHistogramParameteriv 156
void
__indirect_glGetHistogramParameteriv(GLenum target, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetHistogramParameteriv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMinmaxParameterfv 158
void
__indirect_glGetMinmaxParameterfv(GLenum target, GLenum pname, GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMinmaxParameterfv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLsop_GetMinmaxParameteriv 159
void
__indirect_glGetMinmaxParameteriv(GLenum target, GLenum pname, GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 8;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_single_request(gc, X_GLsop_GetMinmaxParameteriv, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&target), 4);
        (void) memcpy((void *)(pc + 4), (void *)(&pname), 4);
        (void) read_reply(dpy, 4, params, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLrop_Histogram 4110
void
__indirect_glHistogram(GLenum target, GLsizei width, GLenum internalformat, GLboolean sink)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_Histogram, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&sink), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_Minmax 4111
void
__indirect_glMinmax(GLenum target, GLenum internalformat, GLboolean sink)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_Minmax, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&internalformat), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&sink), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ResetHistogram 4112
void
__indirect_glResetHistogram(GLenum target)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ResetHistogram, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ResetMinmax 4113
void
__indirect_glResetMinmax(GLenum target)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ResetMinmax, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_CopyTexSubImage3D 4123
void
__indirect_glCopyTexSubImage3D(GLenum target, GLint level, GLint xoffset, GLint yoffset, GLint zoffset, GLint x, GLint y, GLsizei width, GLsizei height)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
    emit_header(gc->pc, X_GLrop_CopyTexSubImage3D, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&level), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&xoffset), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&yoffset), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&zoffset), 4);
    (void) memcpy((void *)(gc->pc + 24), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 32), (void *)(&width), 4);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&height), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ActiveTextureARB 197
void
__indirect_glActiveTextureARB(GLenum texture)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ActiveTextureARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&texture), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1dvARB 198
void
__indirect_glMultiTexCoord1dARB(GLenum target, GLdouble s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1dvARB 198
void
__indirect_glMultiTexCoord1dvARB(GLenum target, const GLdouble * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1fvARB 199
void
__indirect_glMultiTexCoord1fARB(GLenum target, GLfloat s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1fvARB 199
void
__indirect_glMultiTexCoord1fvARB(GLenum target, const GLfloat * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1ivARB 200
void
__indirect_glMultiTexCoord1iARB(GLenum target, GLint s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1ivARB 200
void
__indirect_glMultiTexCoord1ivARB(GLenum target, const GLint * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1svARB 201
void
__indirect_glMultiTexCoord1sARB(GLenum target, GLshort s)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord1svARB 201
void
__indirect_glMultiTexCoord1svARB(GLenum target, const GLshort * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord1svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2dvARB 202
void
__indirect_glMultiTexCoord2dARB(GLenum target, GLdouble s, GLdouble t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2dvARB 202
void
__indirect_glMultiTexCoord2dvARB(GLenum target, const GLdouble * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v), 16);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2fvARB 203
void
__indirect_glMultiTexCoord2fARB(GLenum target, GLfloat s, GLfloat t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2fvARB 203
void
__indirect_glMultiTexCoord2fvARB(GLenum target, const GLfloat * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2ivARB 204
void
__indirect_glMultiTexCoord2iARB(GLenum target, GLint s, GLint t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2ivARB 204
void
__indirect_glMultiTexCoord2ivARB(GLenum target, const GLint * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2svARB 205
void
__indirect_glMultiTexCoord2sARB(GLenum target, GLshort s, GLshort t)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&t), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord2svARB 205
void
__indirect_glMultiTexCoord2svARB(GLenum target, const GLshort * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_MultiTexCoord2svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3dvARB 206
void
__indirect_glMultiTexCoord3dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3dvARB 206
void
__indirect_glMultiTexCoord3dvARB(GLenum target, const GLdouble * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 32;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v), 24);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3fvARB 207
void
__indirect_glMultiTexCoord3fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3fvARB 207
void
__indirect_glMultiTexCoord3fvARB(GLenum target, const GLfloat * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 12);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3ivARB 208
void
__indirect_glMultiTexCoord3iARB(GLenum target, GLint s, GLint t, GLint r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3ivARB 208
void
__indirect_glMultiTexCoord3ivARB(GLenum target, const GLint * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 12);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3svARB 209
void
__indirect_glMultiTexCoord3sARB(GLenum target, GLshort s, GLshort t, GLshort r)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&t), 2);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&r), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord3svARB 209
void
__indirect_glMultiTexCoord3svARB(GLenum target, const GLshort * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord3svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 6);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4dvARB 210
void
__indirect_glMultiTexCoord4dARB(GLenum target, GLdouble s, GLdouble t, GLdouble r, GLdouble q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&s), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&r), 8);
    (void) memcpy((void *)(gc->pc + 28), (void *)(&q), 8);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4dvARB 210
void
__indirect_glMultiTexCoord4dvARB(GLenum target, const GLdouble * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 40;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4dvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(v), 32);
    (void) memcpy((void *)(gc->pc + 36), (void *)(&target), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4fvARB 211
void
__indirect_glMultiTexCoord4fARB(GLenum target, GLfloat s, GLfloat t, GLfloat r, GLfloat q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&q), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4fvARB 211
void
__indirect_glMultiTexCoord4fvARB(GLenum target, const GLfloat * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4fvARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4ivARB 212
void
__indirect_glMultiTexCoord4iARB(GLenum target, GLint s, GLint t, GLint r, GLint q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&t), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&r), 4);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&q), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4ivARB 212
void
__indirect_glMultiTexCoord4ivARB(GLenum target, const GLint * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 24;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4ivARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 16);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4svARB 213
void
__indirect_glMultiTexCoord4sARB(GLenum target, GLshort s, GLshort t, GLshort r, GLshort q)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&s), 2);
    (void) memcpy((void *)(gc->pc + 10), (void *)(&t), 2);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&r), 2);
    (void) memcpy((void *)(gc->pc + 14), (void *)(&q), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_MultiTexCoord4svARB 213
void
__indirect_glMultiTexCoord4svARB(GLenum target, const GLshort * v)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_MultiTexCoord4svARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&target), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(v), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SampleCoverageARB 229
void
__indirect_glSampleCoverageARB(GLclampf value, GLboolean invert)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_SampleCoverageARB, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&value), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&invert), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLvop_AreTexturesResidentEXT 11
GLboolean
__indirect_glAreTexturesResidentEXT(GLsizei n, const GLuint * textures, GLboolean * residences)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4 + __GLX_PAD((n * 4));
    if (__builtin_expect((n >= 0) && (dpy != NULL), 1)) {
        GLubyte const * pc = setup_vendor_request(gc, X_GLXVendorPrivateWithReply, X_GLvop_AreTexturesResidentEXT, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) memcpy((void *)(pc + 4), (void *)(textures), (n * 4));
        retval = (GLboolean) read_reply(dpy, 1, residences, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLvop_GenTexturesEXT 13
void
__indirect_glGenTexturesEXT(GLsizei n, GLuint * textures)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_vendor_request(gc, X_GLXVendorPrivateWithReply, X_GLvop_GenTexturesEXT, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&n), 4);
        (void) read_reply(dpy, 4, textures, GL_TRUE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return;
}

#define X_GLvop_IsTextureEXT 14
GLboolean
__indirect_glIsTextureEXT(GLuint texture)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    Display * const dpy = gc->currentDpy;
    GLboolean retval = (GLboolean) 0;
    const GLuint cmdlen = 4;
    if (__builtin_expect(dpy != NULL, 1)) {
        GLubyte const * pc = setup_vendor_request(gc, X_GLXVendorPrivateWithReply, X_GLvop_IsTextureEXT, cmdlen);
        (void) memcpy((void *)(pc + 0), (void *)(&texture), 4);
        retval = (GLboolean) read_reply(dpy, 0, NULL, GL_FALSE);
        UnlockDisplay(dpy); SyncHandle();
    }
    return retval;
}

#define X_GLrop_SampleMaskSGIS 2048
void
__indirect_glSampleMaskSGIS(GLclampf value, GLboolean invert)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_SampleMaskSGIS, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&value), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&invert), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SamplePatternSGIS 2049
void
__indirect_glSamplePatternSGIS(GLenum pattern)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_SamplePatternSGIS, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pattern), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointParameterfEXT 2065
void
__indirect_glPointParameterfEXT(GLenum pname, GLfloat param)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PointParameterfEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&param), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointParameterfvEXT 2066
void
__indirect_glPointParameterfvEXT(GLenum pname, const GLfloat * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glPointParameterfvEXT_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_PointParameterfvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_WindowPos3fvMESA 230
void
__indirect_glWindowPos3fMESA(GLfloat x, GLfloat y, GLfloat z)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_WindowPos3fvMESA, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&x), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&y), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&z), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_WindowPos3fvMESA 230
void
__indirect_glWindowPos3fvMESA(const GLfloat * v)
{
    generic_12_byte( X_GLrop_WindowPos3fvMESA, v );
}

#define X_GLrop_BlendFuncSeparateEXT 4134
void
__indirect_glBlendFuncSeparateEXT(GLenum sfactorRGB, GLenum dfactorRGB, GLenum sfactorAlpha, GLenum dfactorAlpha)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 20;
    emit_header(gc->pc, X_GLrop_BlendFuncSeparateEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&sfactorRGB), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&dfactorRGB), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&sfactorAlpha), 4);
    (void) memcpy((void *)(gc->pc + 16), (void *)(&dfactorAlpha), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FogCoordfvEXT 4124
void
__indirect_glFogCoordfEXT(GLfloat coord)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_FogCoordfvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FogCoordfvEXT 4124
void
__indirect_glFogCoordfvEXT(const GLfloat * coord)
{
    generic_4_byte( X_GLrop_FogCoordfvEXT, coord );
}

#define X_GLrop_FogCoorddvEXT 4125
void
__indirect_glFogCoorddEXT(GLdouble coord)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_FogCoorddvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&coord), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_FogCoorddvEXT 4125
void
__indirect_glFogCoorddvEXT(const GLdouble * coord)
{
    generic_8_byte( X_GLrop_FogCoorddvEXT, coord );
}

#define X_GLrop_SecondaryColor3bvEXT 4126
void
__indirect_glSecondaryColor3bEXT(GLbyte red, GLbyte green, GLbyte blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_SecondaryColor3bvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3bvEXT 4126
void
__indirect_glSecondaryColor3bvEXT(const GLbyte * v)
{
    generic_3_byte( X_GLrop_SecondaryColor3bvEXT, v );
}

#define X_GLrop_SecondaryColor3dvEXT 4130
void
__indirect_glSecondaryColor3dEXT(GLdouble red, GLdouble green, GLdouble blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 28;
    emit_header(gc->pc, X_GLrop_SecondaryColor3dvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 8);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&green), 8);
    (void) memcpy((void *)(gc->pc + 20), (void *)(&blue), 8);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3dvEXT 4130
void
__indirect_glSecondaryColor3dvEXT(const GLdouble * v)
{
    generic_24_byte( X_GLrop_SecondaryColor3dvEXT, v );
}

#define X_GLrop_SecondaryColor3fvEXT 4129
void
__indirect_glSecondaryColor3fEXT(GLfloat red, GLfloat green, GLfloat blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_SecondaryColor3fvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3fvEXT 4129
void
__indirect_glSecondaryColor3fvEXT(const GLfloat * v)
{
    generic_12_byte( X_GLrop_SecondaryColor3fvEXT, v );
}

#define X_GLrop_SecondaryColor3ivEXT 4128
void
__indirect_glSecondaryColor3iEXT(GLint red, GLint green, GLint blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_SecondaryColor3ivEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3ivEXT 4128
void
__indirect_glSecondaryColor3ivEXT(const GLint * v)
{
    generic_12_byte( X_GLrop_SecondaryColor3ivEXT, v );
}

#define X_GLrop_SecondaryColor3svEXT 4128
void
__indirect_glSecondaryColor3sEXT(GLshort red, GLshort green, GLshort blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_SecondaryColor3svEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3svEXT 4128
void
__indirect_glSecondaryColor3svEXT(const GLshort * v)
{
    generic_6_byte( X_GLrop_SecondaryColor3svEXT, v );
}

#define X_GLrop_SecondaryColor3ubvEXT 4131
void
__indirect_glSecondaryColor3ubEXT(GLubyte red, GLubyte green, GLubyte blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_SecondaryColor3ubvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 1);
    (void) memcpy((void *)(gc->pc + 5), (void *)(&green), 1);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&blue), 1);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3ubvEXT 4131
void
__indirect_glSecondaryColor3ubvEXT(const GLubyte * v)
{
    generic_3_byte( X_GLrop_SecondaryColor3ubvEXT, v );
}

#define X_GLrop_SecondaryColor3uivEXT 4133
void
__indirect_glSecondaryColor3uiEXT(GLuint red, GLuint green, GLuint blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 16;
    emit_header(gc->pc, X_GLrop_SecondaryColor3uivEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&green), 4);
    (void) memcpy((void *)(gc->pc + 12), (void *)(&blue), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3uivEXT 4133
void
__indirect_glSecondaryColor3uivEXT(const GLuint * v)
{
    generic_12_byte( X_GLrop_SecondaryColor3uivEXT, v );
}

#define X_GLrop_SecondaryColor3usvEXT 4132
void
__indirect_glSecondaryColor3usEXT(GLushort red, GLushort green, GLushort blue)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_SecondaryColor3usvEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&red), 2);
    (void) memcpy((void *)(gc->pc + 6), (void *)(&green), 2);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&blue), 2);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_SecondaryColor3usvEXT 4132
void
__indirect_glSecondaryColor3usvEXT(const GLushort * v)
{
    generic_6_byte( X_GLrop_SecondaryColor3usvEXT, v );
}

#define X_GLrop_PointParameteriNV 4221
void
__indirect_glPointParameteriNV(GLenum pname, GLint params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 12;
    emit_header(gc->pc, X_GLrop_PointParameteriNV, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(&params), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_PointParameterivNV 4222
void
__indirect_glPointParameterivNV(GLenum pname, const GLint * params)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint compsize = __glPointParameterivNV_size(pname);
    const GLuint cmdlen = 8 + __GLX_PAD((compsize * 4));
    emit_header(gc->pc, X_GLrop_PointParameterivNV, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&pname), 4);
    (void) memcpy((void *)(gc->pc + 8), (void *)(params), (compsize * 4));
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

#define X_GLrop_ActiveStencilFaceEXT 4220
void
__indirect_glActiveStencilFaceEXT(GLenum face)
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = 8;
    emit_header(gc->pc, X_GLrop_ActiveStencilFaceEXT, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), (void *)(&face), 4);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}

