/* $Id: t_imm_alloc.c,v 1.18 2003/03/28 01:39:05 brianp Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  5.1
 *
 * Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * Authors:
 *    Keith Whitwell <keith@tungstengraphics.com>
 */

#include "glheader.h"
#include "imports.h"
#include "mtypes.h"

#include "t_imm_alloc.h"


static int id = 0;  /* give each struct immediate a unique ID number */


static struct immediate *
real_alloc_immediate( GLcontext *ctx )
{
   struct immediate *immed = ALIGN_CALLOC_STRUCT( immediate, 32 );

   printf("Sizeof(struct immed) = %d\n", sizeof(struct immediate));

   if (!immed)
      return NULL;

   immed->id = id++;
   immed->ref_count = 0;
   immed->FlushElt = 0;
   immed->LastPrimitive = IMM_MAX_COPIED_VERTS;
   immed->Count = IMM_MAX_COPIED_VERTS;
   immed->Start = IMM_MAX_COPIED_VERTS;
   immed->Material = 0;
   immed->MaterialMask = 0;
   immed->MaxTextureUnits = ctx->Const.MaxTextureCoordUnits;
   immed->TexSize = 0;
   immed->NormalLengthPtr = 0;

   immed->CopyTexSize = 0;
   immed->CopyStart = immed->Start;

   return immed;
}


static void
real_free_immediate( struct immediate *immed )
{
   static int freed = 0;

   if (immed->Material) {
      FREE( immed->Material );
      FREE( immed->MaterialMask );
      immed->Material = 0;
      immed->MaterialMask = 0;
   }

   if (immed->NormalLengthPtr)
      ALIGN_FREE( immed->NormalLengthPtr );

   ALIGN_FREE( immed );
   freed++;
/*     printf("outstanding %d\n", id - freed);    */
}


/**
 * Return a pointer to a new 'struct immediate' object.
 * We actually keep a spare/cached one to reduce malloc calls.
 */
struct immediate *
_tnl_alloc_immediate( GLcontext *ctx )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct immediate *tmp = tnl->freed_immediate;
   
   if (tmp) {
      tnl->freed_immediate = 0;
      return tmp;
   }
   else
      return real_alloc_immediate( ctx );
}

/**
 * Free a 'struct immediate' object.
 * May be called after tnl is destroyed.
 */
void
_tnl_free_immediate( GLcontext *ctx, struct immediate *immed )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);

   ASSERT(immed->ref_count == 0);

   if (immed->NormalLengthPtr) {
      ALIGN_FREE(immed->NormalLengthPtr);
      immed->NormalLengthPtr = NULL;
   }

   if (!tnl) {
      real_free_immediate( immed );
   } 
   else {
      if (tnl->freed_immediate)
	 real_free_immediate( tnl->freed_immediate );
      
      tnl->freed_immediate = immed;
   }
}
