/*
 * Mesa 3-D graphics library
 * Version:  6.3
 *
 * Copyright (C) 1999-2005  Brian Paul   All Rights Reserved.
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


#include "api_arrayelt.h"
#include "glheader.h"
#include "imports.h"
#include "context.h"
#include "macros.h"
#include "mtypes.h"
#include "dlist.h"
#include "vtxfmt.h"

#include "vbo_context.h"


#define NR_LEGACY_ATTRIBS 16
#define NR_GENERIC_ATTRIBS 16
#define NR_MAT_ATTRIBS 12

static void init_legacy_currval(GLcontext *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   struct gl_client_array *arrays = exec->legacy_currval;
   GLuint i;

   memset(arrays, 0, sizeof(*arrays) * NR_LEGACY_ATTRIBS);

   /* Set up a constant (StrideB == 0) array for each current
    * attribute:
    */
   for (i = 0; i < NR_LEGACY_ATTRIBS; i++) {
      struct gl_client_array *cl = &arrays[i];

      switch (i) {
      case VBO_ATTRIB_EDGEFLAG:
	 cl->Type = GL_UNSIGNED_BYTE;
	 cl->Ptr = (const void *)&ctx->Current.EdgeFlag;
	 break;
      case VBO_ATTRIB_INDEX:
	 cl->Type = GL_FLOAT;
	 cl->Ptr = (const void *)&ctx->Current.Index;
	 break;
      default:
	 cl->Type = GL_FLOAT;
	 cl->Ptr = (const void *)ctx->Current.Attrib[i];
	 break;
      }

      /* This will have to be determined at runtime:
       */
      cl->Size = 1;
      cl->Stride = 0;
      cl->StrideB = 0;
      cl->Enabled = 1;
      cl->BufferObj = ctx->Array.NullBufferObj;
   }
}


static void init_generic_currval(GLcontext *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   struct gl_client_array *arrays = exec->generic_currval;
   GLuint i;

   memset(arrays, 0, sizeof(*arrays) * NR_GENERIC_ATTRIBS);

   for (i = 0; i < NR_GENERIC_ATTRIBS; i++) {
      struct gl_client_array *cl = &arrays[i];

      /* This will have to be determined at runtime:
       */
      cl->Size = 1;

      cl->Type = GL_FLOAT;
      cl->Ptr = (const void *)ctx->Current.Attrib[VERT_ATTRIB_GENERIC0 + i];
      cl->Stride = 0;
      cl->StrideB = 0;
      cl->Enabled = 1;
      cl->BufferObj = ctx->Array.NullBufferObj;
   }
}


static void init_mat_currval(GLcontext *ctx)
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;
   struct gl_client_array *arrays = exec->mat_currval;
   GLuint i;

   memset(arrays, 0, sizeof(*arrays) * NR_GENERIC_ATTRIBS);

   /* Set up a constant (StrideB == 0) array for each current
    * attribute:
    */
   for (i = 0; i < NR_GENERIC_ATTRIBS; i++) {
      struct gl_client_array *cl = &arrays[i];

      /* Size is fixed for the material attributes, for others will
       * be determined at runtime:
       */
      switch (i - VERT_ATTRIB_GENERIC0) {
      case MAT_ATTRIB_FRONT_SHININESS:
      case MAT_ATTRIB_BACK_SHININESS:
	 cl->Size = 1;
	 break;
      case MAT_ATTRIB_FRONT_INDEXES:
      case MAT_ATTRIB_BACK_INDEXES:
	 cl->Size = 3;
	 break;
      default:
	 cl->Size = 4;
	 break;
      }

      if (i < MAT_ATTRIB_MAX)
	 cl->Ptr = (const void *)ctx->Light.Material.Attrib[i];
      else 
	 cl->Ptr = (const void *)ctx->Current.Attrib[VERT_ATTRIB_GENERIC0 + i];

      cl->Type = GL_FLOAT;
      cl->Stride = 0;
      cl->StrideB = 0;
      cl->Enabled = 1;
      cl->BufferObj = ctx->Array.NullBufferObj;
   }
}


void vbo_exec_init( GLcontext *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   exec->ctx = ctx;

   /* Initialize the arrayelt helper
    */
   if (!ctx->aelt_context &&
       !_ae_create_context( ctx )) 
      return;

   vbo_exec_vtx_init( exec );
   vbo_exec_array_init( exec );

   init_legacy_currval( ctx );
   init_generic_currval( ctx );
   init_mat_currval( ctx );

   ctx->Driver.NeedFlush = 0;
   ctx->Driver.CurrentExecPrimitive = PRIM_OUTSIDE_BEGIN_END;
   ctx->Driver.FlushVertices = vbo_exec_FlushVertices;

   exec->eval.recalculate_maps = 1;
}


void vbo_exec_destroy( GLcontext *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (ctx->aelt_context) {
      _ae_destroy_context( ctx );
      ctx->aelt_context = NULL;
   }

   vbo_exec_vtx_destroy( exec );
   vbo_exec_array_destroy( exec );
}

/* Really want to install these callbacks to a central facility to be
 * invoked according to the state flags.  That will have to wait for a
 * mesa rework:
 */ 
void vbo_exec_invalidate_state( GLcontext *ctx, GLuint new_state )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   if (new_state & (_NEW_PROGRAM|_NEW_EVAL))
      exec->eval.recalculate_maps = 1;

   _ae_invalidate_state(ctx, new_state);
}


void vbo_exec_wakeup( GLcontext *ctx )
{
   struct vbo_exec_context *exec = &vbo_context(ctx)->exec;

   ctx->Driver.FlushVertices = vbo_exec_FlushVertices;
   ctx->Driver.NeedFlush |= FLUSH_UPDATE_CURRENT;

   /* Hook our functions into exec and compile dispatch tables.
    */
   _mesa_install_exec_vtxfmt( ctx, &exec->vtxfmt );

   /* Assume we haven't been getting state updates either:
    */
   vbo_exec_invalidate_state( ctx, ~0 );
}



