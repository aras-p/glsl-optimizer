/* $XFree86$ */
/**************************************************************************

Copyright 2002 Tungsten Graphics Inc., Cedar Park, Texas.

All Rights Reserved.

Permission is hereby granted, free of charge, to any person obtaining a
copy of this software and associated documentation files (the "Software"),
to deal in the Software without restriction, including without limitation
on the rights to use, copy, modify, merge, publish, distribute, sub
license, and/or sell copies of the Software, and to permit persons to whom
the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice (including the next
paragraph) shall be included in all copies or substantial portions of the
Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 *
 */

#ifndef __RADEON_VTXFMT_H__
#define __RADEON_VTXFMT_H__

#ifdef GLX_DIRECT_RENDERING

#include "_tnl__context.h"

extern void _tnl_UpdateVtxfmt( GLcontext *ctx );
extern void _tnl_InitVtxfmt( GLcontext *ctx );
extern void _tnl_InvalidateVtxfmt( GLcontext *ctx );
extern void _tnl_DestroyVtxfmt( GLcontext *ctx );

typedef void (*p4f)( GLfloat, GLfloat, GLfloat, GLfloat );
typedef void (*p3f)( GLfloat, GLfloat, GLfloat );
typedef void (*p2f)( GLfloat, GLfloat );
typedef void (*p1f)( GLfloat );
typedef void (*pe2f)( GLenum, GLfloat, GLfloat );
typedef void (*pe1f)( GLenum, GLfloat );
typedef void (*p4ub)( GLubyte, GLubyte, GLubyte, GLubyte );
typedef void (*p3ub)( GLubyte, GLubyte, GLubyte );
typedef void (*pfv)( const GLfloat * );
typedef void (*pefv)( GLenum, const GLfloat * );
typedef void (*pubv)( const GLubyte * );

/* Want to keep a cache of these around.  Each is parameterized by
 * only a single value which has only a small range.  Only expect a
 * few, so just rescan the list each time?
 */
struct dynfn {
   struct dynfn *next, *prev;
   int key;
   char *code;
};

struct dfn_lists {
   struct dynfn Vertex2f;
   struct dynfn Vertex2fv;
   struct dynfn Vertex3f;
   struct dynfn Vertex3fv;
   struct dynfn Color4ub;
   struct dynfn Color4ubv;
   struct dynfn Color3ub;
   struct dynfn Color3ubv;
   struct dynfn Color4f;
   struct dynfn Color4fv;
   struct dynfn Color3f;
   struct dynfn Color3fv;
   struct dynfn SecondaryColor3ubEXT;
   struct dynfn SecondaryColor3ubvEXT;
   struct dynfn SecondaryColor3fEXT;
   struct dynfn SecondaryColor3fvEXT;
   struct dynfn Normal3f;
   struct dynfn Normal3fv;
   struct dynfn TexCoord2f;
   struct dynfn TexCoord2fv;
   struct dynfn TexCoord1f;
   struct dynfn TexCoord1fv;
   struct dynfn MultiTexCoord2fARB;
   struct dynfn MultiTexCoord2fvARB;
   struct dynfn MultiTexCoord1fARB;
   struct dynfn MultiTexCoord1fvARB;
};

struct _vb;

struct dfn_generators {
   struct dynfn *(*Vertex2f)( struct _vb *, int );
   struct dynfn *(*Vertex2fv)( struct _vb *, int );
   struct dynfn *(*Vertex3f)( struct _vb *, int );
   struct dynfn *(*Vertex3fv)( struct _vb *, int );
   struct dynfn *(*Color4ub)( struct _vb *, int );
   struct dynfn *(*Color4ubv)( struct _vb *, int );
   struct dynfn *(*Color3ub)( struct _vb *, int );
   struct dynfn *(*Color3ubv)( struct _vb *, int );
   struct dynfn *(*Color4f)( struct _vb *, int );
   struct dynfn *(*Color4fv)( struct _vb *, int );
   struct dynfn *(*Color3f)( struct _vb *, int );
   struct dynfn *(*Color3fv)( struct _vb *, int );
   struct dynfn *(*SecondaryColor3ubEXT)( struct _vb *, int );
   struct dynfn *(*SecondaryColor3ubvEXT)( struct _vb *, int );
   struct dynfn *(*SecondaryColor3fEXT)( struct _vb *, int );
   struct dynfn *(*SecondaryColor3fvEXT)( struct _vb *, int );
   struct dynfn *(*Normal3f)( struct _vb *, int );
   struct dynfn *(*Normal3fv)( struct _vb *, int );
   struct dynfn *(*TexCoord2f)( struct _vb *, int );
   struct dynfn *(*TexCoord2fv)( struct _vb *, int );
   struct dynfn *(*TexCoord1f)( struct _vb *, int );
   struct dynfn *(*TexCoord1fv)( struct _vb *, int );
   struct dynfn *(*MultiTexCoord2fARB)( struct _vb *, int );
   struct dynfn *(*MultiTexCoord2fvARB)( struct _vb *, int );
   struct dynfn *(*MultiTexCoord1fARB)( struct _vb *, int );
   struct dynfn *(*MultiTexCoord1fvARB)( struct _vb *, int );
};

struct prim {
   GLuint start;
   GLuint end;
   GLuint prim;
};

#define _TNL__MAX_PRIMS 64



struct tnl_vbinfo {
   /* Keep these first: referenced from codegen templates:
    */
   GLint counter;
   GLint *dmaptr;
   void (*notify)( void );
   union { float f; int i; GLubyte ub4[4]; } vertex[16*4];

   GLfloat *attrptr[16];
   GLuint size[16];

   GLenum *prim;		/* &ctx->Driver.CurrentExecPrimitive */
   GLuint primflags;

   GLboolean installed;
   GLboolean recheck;

   GLint vertex_size;
   GLint initial_counter;
   GLint nrverts;
   GLuint vertex_format;

   GLuint installed_vertex_format;

   struct prim primlist[RADEON_MAX_PRIMS];
   int nrprims;

   struct dfn_lists dfn_cache;
   struct dfn_generators codegen;
   GLvertexformat vtxfmt;
};


extern void _tnl_InitVtxfmtChoosers( GLvertexformat *vfmt );


#define FIXUP( CODE, OFFSET, CHECKVAL, NEWVAL )	\
do {						\
   int *icode = (int *)(CODE+OFFSET);		\
   assert (*icode == CHECKVAL);			\
   *icode = (int)NEWVAL;			\
} while (0)


/* Useful for figuring out the offsets:
 */
#define FIXUP2( CODE, OFFSET, CHECKVAL, NEWVAL )		\
do {								\
   while (*(int *)(CODE+OFFSET) != CHECKVAL) OFFSET++;		\
   fprintf(stderr, "%s/%d CVAL %x OFFSET %d\n", __FUNCTION__,	\
	   __LINE__, CHECKVAL, OFFSET);				\
   *(int *)(CODE+OFFSET) = (int)NEWVAL;				\
   OFFSET += 4;							\
} while (0)

/* 
 */
void _tnl_InitCodegen( struct dfn_generators *gen );
void _tnl_InitX86Codegen( struct dfn_generators *gen );
void _tnl_InitSSECodegen( struct dfn_generators *gen );

void _tnl_copy_to_current( GLcontext *ctx );


/* Defined in tnl_vtxfmt_c.c.
 */
struct dynfn *tnl_makeX86Vertex2f( TNLcontext *, int );
struct dynfn *tnl_makeX86Vertex2fv( TNLcontext *, int );
struct dynfn *tnl_makeX86Vertex3f( TNLcontext *, int );
struct dynfn *tnl_makeX86Vertex3fv( TNLcontext *, int );
struct dynfn *tnl_makeX86Color4ub( TNLcontext *, int );
struct dynfn *tnl_makeX86Color4ubv( TNLcontext *, int );
struct dynfn *tnl_makeX86Color3ub( TNLcontext *, int );
struct dynfn *tnl_makeX86Color3ubv( TNLcontext *, int );
struct dynfn *tnl_makeX86Color4f( TNLcontext *, int );
struct dynfn *tnl_makeX86Color4fv( TNLcontext *, int );
struct dynfn *tnl_makeX86Color3f( TNLcontext *, int );
struct dynfn *tnl_makeX86Color3fv( TNLcontext *, int );
struct dynfn *tnl_makeX86SecondaryColor3ubEXT( TNLcontext *, int );
struct dynfn *tnl_makeX86SecondaryColor3ubvEXT( TNLcontext *, int );
struct dynfn *tnl_makeX86SecondaryColor3fEXT( TNLcontext *, int );
struct dynfn *tnl_makeX86SecondaryColor3fvEXT( TNLcontext *, int );
struct dynfn *tnl_makeX86Normal3f( TNLcontext *, int );
struct dynfn *tnl_makeX86Normal3fv( TNLcontext *, int );
struct dynfn *tnl_makeX86TexCoord2f( TNLcontext *, int );
struct dynfn *tnl_makeX86TexCoord2fv( TNLcontext *, int );
struct dynfn *tnl_makeX86TexCoord1f( TNLcontext *, int );
struct dynfn *tnl_makeX86TexCoord1fv( TNLcontext *, int );
struct dynfn *tnl_makeX86MultiTexCoord2fARB( TNLcontext *, int );
struct dynfn *tnl_makeX86MultiTexCoord2fvARB( TNLcontext *, int );
struct dynfn *tnl_makeX86MultiTexCoord1fARB( TNLcontext *, int );
struct dynfn *tnl_makeX86MultiTexCoord1fvARB( TNLcontext *, int );


#endif
#endif
