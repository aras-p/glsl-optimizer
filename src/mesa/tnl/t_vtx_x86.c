/**************************************************************************

Copyright 2004 Tungsten Graphics Inc., Cedar Park, Texas.

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
ATI, TUNGSTEN GRAPHICS AND/OR THEIR SUPPLIERS BE LIABLE FOR ANY CLAIM,
DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
USE OR OTHER DEALINGS IN THE SOFTWARE.

**************************************************************************/

/*
 * Authors:
 *   Keith Whitwell <keith@tungstengraphics.com>
 */


#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "vtxfmt.h"
#include "dlist.h"
#include "state.h"
#include "light.h"
#include "api_arrayelt.h"
#include "api_noop.h"
#include "t_vtx_api.h"
#include "simple_list.h"


#if defined(USE_X86_ASM) && !defined(HAVE_NONSTANDARD_GLAPIENTRY)

#define EXTERN( FUNC )		\
extern const char *FUNC;	\
extern const char *FUNC##_end

EXTERN( _x86_Attribute1fv );
EXTERN( _x86_Attribute2fv );
EXTERN( _x86_Attribute3fv );
EXTERN( _x86_Attribute4fv );
EXTERN( _x86_Vertex1fv );
EXTERN( _x86_Vertex2fv );
EXTERN( _x86_Vertex3fv );
EXTERN( _x86_Vertex4fv );

EXTERN( _x86_dispatch_attrf );
EXTERN( _x86_dispatch_attrfv );
EXTERN( _x86_dispatch_multitexcoordf );
EXTERN( _x86_dispatch_multitexcoordfv );
EXTERN( _x86_dispatch_vertexattribf );
EXTERN( _x86_dispatch_vertexattribfv );


static void notify( void )
{
   GET_CURRENT_CONTEXT( ctx );
   _tnl_wrap_filled_vertex( ctx );
}

#define DONT_KNOW_OFFSETS 1


#define DFN( FUNC, CACHE, KEY )				\
   struct _tnl_dynfn *dfn = MALLOC_STRUCT( _tnl_dynfn );          \
   char *start = (char *)&FUNC;				\
   char *end = (char *)&FUNC##_end;			\
   int offset = 0;               			\
   insert_at_head( &CACHE, dfn );			\
   dfn->key = KEY;					\
   dfn->code = ALIGN_MALLOC( end - start, 16 );		\
   memcpy (dfn->code, start, end - start)



#define FIXUP( CODE, KNOWN_OFFSET, CHECKVAL, NEWVAL )	\
do {							\
   GLuint subst = 0x10101010 + CHECKVAL;		\
							\
   if (DONT_KNOW_OFFSETS) {				\
      while (*(int *)(CODE+offset) != subst) offset++;	\
      *(int *)(CODE+offset) = (int)(NEWVAL);		\
      if (0) fprintf(stderr, "%s/%d: offset %d, new value: 0x%x\n", __FILE__, __LINE__, offset, (int)(NEWVAL)); \
      offset += 4;					\
   }							\
   else {						\
      int *icode = (int *)(CODE+KNOWN_OFFSET);		\
      assert (*icode == subst);				\
      *icode = (int)NEWVAL;				\
   }							\
} while (0)



#define FIXUPREL( CODE, KNOWN_OFFSET, CHECKVAL, NEWVAL )\
do {							\
   GLuint subst = 0x10101010 + CHECKVAL;		\
							\
   if (DONT_KNOW_OFFSETS) {				\
      while (*(int *)(CODE+offset) != subst) offset++;	\
      *(int *)(CODE+offset) = (int)(NEWVAL) - ((int)(CODE)+offset) - 4; \
      if (0) fprintf(stderr, "%s/%d: offset %d, new value: 0x%x\n", __FILE__, __LINE__, offset, (int)(NEWVAL) - ((int)(CODE)+offset) - 4); \
      offset += 4;					\
   }							\
   else {						\
      int *icode = (int *)(CODE+KNOWN_OFFSET);		\
      assert (*icode == subst);				\
      *icode = (int)(NEWVAL) - (int)(icode) - 4;	\
   }							\
} while (0)




/* Build specialized versions of the immediate calls on the fly for
 * the current state.  Generic x86 versions.
 */

static struct _tnl_dynfn *makeX86Vertex1fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Vertex1fv, tnl->vtx.cache.Vertex[1-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 1);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[1]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUPREL(dfn->code, 0, 4, (int)&notify);

   return dfn;
}

static struct _tnl_dynfn *makeX86Vertex2fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Vertex2fv, tnl->vtx.cache.Vertex[2-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 2);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[2]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUPREL(dfn->code, 0, 4, (int)&notify);

   return dfn;
}

static struct _tnl_dynfn *makeX86Vertex3fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Vertex3fv, tnl->vtx.cache.Vertex[3-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 3);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[3]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUPREL(dfn->code, 0, 4, (int)&notify);
   return dfn;
}

static struct _tnl_dynfn *makeX86Vertex4fv( GLcontext *ctx, int vertex_size )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Vertex4fv, tnl->vtx.cache.Vertex[4-1], vertex_size );

   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 1, vertex_size - 4);
   FIXUP(dfn->code, 0, 2, (int)&tnl->vtx.vertex[4]);
   FIXUP(dfn->code, 0, 0, (int)&tnl->vtx.vbptr);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUP(dfn->code, 0, 3, (int)&tnl->vtx.counter);
   FIXUPREL(dfn->code, 0, 4, (int)&notify);

   return dfn;
}


static struct _tnl_dynfn *makeX86Attribute1fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Attribute1fv, tnl->vtx.cache.Attribute[1-1], dest );

   FIXUP(dfn->code, 0, 0, dest); 

   return dfn;
}

static struct _tnl_dynfn *makeX86Attribute2fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Attribute2fv, tnl->vtx.cache.Attribute[2-1], dest );

   FIXUP(dfn->code, 0, 0, dest); 
   FIXUP(dfn->code, 0, 1, 4+dest); 

   return dfn;
}

static struct _tnl_dynfn *makeX86Attribute3fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Attribute3fv, tnl->vtx.cache.Attribute[3-1], dest );

   FIXUP(dfn->code, 0, 0, dest); 
   FIXUP(dfn->code, 0, 1, 4+dest); 
   FIXUP(dfn->code, 0, 2, 8+dest);

   return dfn;
}

static struct _tnl_dynfn *makeX86Attribute4fv( GLcontext *ctx, int dest )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   DFN ( _x86_Attribute4fv, tnl->vtx.cache.Attribute[4-1], dest );

   FIXUP(dfn->code, 0, 0, dest); 
   FIXUP(dfn->code, 0, 1, 4+dest); 
   FIXUP(dfn->code, 0, 2, 8+dest);
   FIXUP(dfn->code, 0, 3, 12+dest);

   return dfn;
}


void _tnl_InitX86Codegen( struct _tnl_dynfn_generators *gen )
{
   gen->Vertex[0] = makeX86Vertex1fv;
   gen->Vertex[1] = makeX86Vertex2fv;
   gen->Vertex[2] = makeX86Vertex3fv;
   gen->Vertex[3] = makeX86Vertex4fv;
   gen->Attribute[0] = makeX86Attribute1fv;
   gen->Attribute[1] = makeX86Attribute2fv;
   gen->Attribute[2] = makeX86Attribute3fv;
   gen->Attribute[3] = makeX86Attribute4fv;
}

void _do_choose( void )
{
}


/* [dBorca] I purposely avoided one single macro, since they might need to
 * be handled in different ways. Ohwell, once things get much clearer, they
 * could collapse...
 */
#define MAKE_DISPATCH_ATTR(FUNC, SIZE, TYPE, ATTR)			\
do {									\
   char *code;								\
   char *start = (char *)&_x86_dispatch_attr##TYPE;			\
   char *end = (char *)&_x86_dispatch_attr##TYPE##_end;			\
   int offset = 0;							\
   code = ALIGN_MALLOC( end - start, 16 );				\
   memcpy (code, start, end - start);					\
   FIXUP(code, 0, 0, (int)&(TNL_CONTEXT(ctx)->vtx.tabfv[ATTR][SIZE-1]));\
   vfmt->FUNC##SIZE##TYPE = code;					\
} while (0)


#define MAKE_DISPATCH_MULTITEXCOORD(FUNC, SIZE, TYPE, ATTR)		\
do {									\
   char *code;								\
   char *start = (char *)&_x86_dispatch_multitexcoord##TYPE;		\
   char *end = (char *)&_x86_dispatch_multitexcoord##TYPE##_end;	\
   int offset = 0;							\
   code = ALIGN_MALLOC( end - start, 16 );				\
   memcpy (code, start, end - start);					\
   FIXUP(code, 0, 0, (int)&(TNL_CONTEXT(ctx)->vtx.tabfv[_TNL_ATTRIB_TEX0][SIZE-1]));\
   vfmt->FUNC##SIZE##TYPE##ARB = code;					\
} while (0)


#define MAKE_DISPATCH_VERTEXATTRIB(FUNC, SIZE, TYPE, ATTR)		\
do {									\
   char *code;								\
   char *start = (char *)&_x86_dispatch_vertexattrib##TYPE;		\
   char *end = (char *)&_x86_dispatch_vertexattrib##TYPE##_end;		\
   int offset = 0;							\
   code = ALIGN_MALLOC( end - start, 16 );				\
   memcpy (code, start, end - start);					\
   FIXUP(code, 0, 0, (int)&(TNL_CONTEXT(ctx)->vtx.tabfv[0][SIZE-1]));	\
   vfmt->FUNC##SIZE##TYPE##NV = code;					\
} while (0)

/* [dBorca] Install the codegen'ed versions of the 2nd level dispatch
 * functions.  We should keep a list and free them in the end...
 */
void _tnl_x86_exec_vtxfmt_init( GLcontext *ctx )
{
   GLvertexformat *vfmt = &(TNL_CONTEXT(ctx)->exec_vtxfmt);

   MAKE_DISPATCH_ATTR(Color,3,f,     _TNL_ATTRIB_COLOR0);
   MAKE_DISPATCH_ATTR(Color,3,fv,    _TNL_ATTRIB_COLOR0);
   MAKE_DISPATCH_ATTR(Color,4,f,     _TNL_ATTRIB_COLOR0);
   MAKE_DISPATCH_ATTR(Color,4,fv,    _TNL_ATTRIB_COLOR0);
   MAKE_DISPATCH_ATTR(Normal,3,f,    _TNL_ATTRIB_NORMAL);
   MAKE_DISPATCH_ATTR(Normal,3,fv,   _TNL_ATTRIB_NORMAL);
   MAKE_DISPATCH_ATTR(TexCoord,2,f,  _TNL_ATTRIB_TEX0);
   MAKE_DISPATCH_ATTR(TexCoord,2,fv, _TNL_ATTRIB_TEX0);
   MAKE_DISPATCH_ATTR(Vertex,3,f,    _TNL_ATTRIB_POS);
   MAKE_DISPATCH_ATTR(Vertex,3,fv,   _TNL_ATTRIB_POS);
   /* just add more */

   MAKE_DISPATCH_MULTITEXCOORD(MultiTexCoord,2,f,  0);
   MAKE_DISPATCH_MULTITEXCOORD(MultiTexCoord,2,fv, 0);
   /* just add more */

   MAKE_DISPATCH_VERTEXATTRIB(VertexAttrib,2,f,  0);
   MAKE_DISPATCH_VERTEXATTRIB(VertexAttrib,2,fv, 0);
   /* just add more */
}

#else 

void _tnl_InitX86Codegen( struct _tnl_dynfn_generators *gen )
{
   (void) gen;
}

void _tnl_x86_exec_vtxfmt_init( GLcontext *ctx )
{
   (void) ctx;
}

#endif
