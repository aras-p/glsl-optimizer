/* $Id: t_array_import.c,v 1.17 2001/05/17 11:33:33 keithw Exp $ */

/*
 * Mesa 3-D graphics library
 * Version:  3.5
 *
 * Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
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
 *    Keith Whitwell <keithw@valinux.com>
 */

#include "glheader.h"
#include "context.h"
#include "macros.h"
#include "mem.h"
#include "mmath.h"
#include "state.h"
#include "mtypes.h"

#include "array_cache/acache.h"
#include "math/m_translate.h"

#include "t_array_import.h"
#include "t_context.h"
#include "t_imm_debug.h"


static void _tnl_import_vertex( GLcontext *ctx,
				GLboolean writeable,
				GLboolean stride )
{
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;

   tmp = _ac_import_vertex(ctx,
			   GL_FLOAT,
			   stride ? 4*sizeof(GLfloat) : 0,
			   0,
			   writeable,
			   &is_writeable);

   inputs->Obj.data = (GLfloat (*)[4]) tmp->Ptr;
   inputs->Obj.start = (GLfloat *) tmp->Ptr;
   inputs->Obj.stride = tmp->StrideB;
   inputs->Obj.size = tmp->Size;
   inputs->Obj.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->Obj.stride != 4*sizeof(GLfloat))
      inputs->Obj.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->Obj.flags |= VEC_NOT_WRITEABLE;
}

static void _tnl_import_normal( GLcontext *ctx,
				GLboolean writeable,
				GLboolean stride )
{
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;

   tmp = _ac_import_normal(ctx, GL_FLOAT,
			   stride ? 3*sizeof(GLfloat) : 0, writeable,
			   &is_writeable);

   inputs->Normal.data = (GLfloat (*)[3]) tmp->Ptr;
   inputs->Normal.start = (GLfloat *) tmp->Ptr;
   inputs->Normal.stride = tmp->StrideB;
   inputs->Normal.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->Normal.stride != 3*sizeof(GLfloat))
      inputs->Normal.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->Normal.flags |= VEC_NOT_WRITEABLE;
}


static void _tnl_import_color( GLcontext *ctx,
			       GLenum type,
			       GLboolean writeable,
			       GLboolean stride )
{
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;

   tmp = _ac_import_color(ctx,
			  type,
			  stride ? 4*sizeof(GLfloat) : 0,
			  4,
			  writeable,
			  &is_writeable);

   inputs->Color = *tmp;
}


static void _tnl_import_secondarycolor( GLcontext *ctx,
					GLenum type,
					GLboolean writeable,
					GLboolean stride )
{
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;

   tmp = _ac_import_secondarycolor(ctx, 
				   type,
				   stride ? 4*sizeof(GLfloat) : 0,
				   4,
				   writeable,
				   &is_writeable);

   inputs->SecondaryColor = *tmp;
}

static void _tnl_import_fogcoord( GLcontext *ctx,
				  GLboolean writeable,
				  GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
    struct gl_client_array *tmp;
   GLboolean is_writeable = 0;

   tmp = _ac_import_fogcoord(ctx, GL_FLOAT,
			     stride ? sizeof(GLfloat) : 0, writeable,
			     &is_writeable);

   inputs->FogCoord.data = (GLfloat *) tmp->Ptr;
   inputs->FogCoord.start = (GLfloat *) tmp->Ptr;
   inputs->FogCoord.stride = tmp->StrideB;
   inputs->FogCoord.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->FogCoord.stride != sizeof(GLfloat))
      inputs->FogCoord.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->FogCoord.flags |= VEC_NOT_WRITEABLE;
}

static void _tnl_import_index( GLcontext *ctx,
			       GLboolean writeable,
			       GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;

   tmp = _ac_import_index(ctx, GL_UNSIGNED_INT,
			  stride ? sizeof(GLuint) : 0, writeable,
			  &is_writeable);

   inputs->Index.data = (GLuint *) tmp->Ptr;
   inputs->Index.start = (GLuint *) tmp->Ptr;
   inputs->Index.stride = tmp->StrideB;
   inputs->Index.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->Index.stride != sizeof(GLuint))
      inputs->Index.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->Index.flags |= VEC_NOT_WRITEABLE;
}


static void _tnl_import_texcoord( GLcontext *ctx,
				  GLuint i,
				  GLboolean writeable,
				  GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;

   tmp = _ac_import_texcoord(ctx, i, GL_FLOAT,
			     stride ? 4*sizeof(GLfloat) : 0,
			     0,
			     writeable,
			     &is_writeable);

   inputs->TexCoord[i].data = (GLfloat (*)[4]) tmp->Ptr;
   inputs->TexCoord[i].start = (GLfloat *) tmp->Ptr;
   inputs->TexCoord[i].stride = tmp->StrideB;
   inputs->TexCoord[i].size = tmp->Size;
   inputs->TexCoord[i].flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->TexCoord[i].stride != 4*sizeof(GLfloat))
      inputs->TexCoord[i].flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->TexCoord[i].flags |= VEC_NOT_WRITEABLE;
}


static void _tnl_import_edgeflag( GLcontext *ctx,
				  GLboolean writeable,
				  GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;

   tmp = _ac_import_edgeflag(ctx, GL_UNSIGNED_BYTE,
			     stride ? sizeof(GLubyte) : 0,
			     0,
			     &is_writeable);

   inputs->EdgeFlag.data = (GLubyte *) tmp->Ptr;
   inputs->EdgeFlag.start = (GLubyte *) tmp->Ptr;
   inputs->EdgeFlag.stride = tmp->StrideB;
   inputs->EdgeFlag.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->EdgeFlag.stride != sizeof(GLubyte))
      inputs->EdgeFlag.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->EdgeFlag.flags |= VEC_NOT_WRITEABLE;
}



/* Callback for VB stages that need to improve the quality of arrays
 * bound to the VB.  This is only necessary for client arrays which
 * have not been transformed at any point in the pipeline.
 */
static void _tnl_upgrade_client_data( GLcontext *ctx,
				      GLuint required,
				      GLuint flags )
{
   GLuint i;
   struct vertex_buffer *VB = &TNL_CONTEXT(ctx)->vb;
   GLboolean writeable = (flags & VEC_NOT_WRITEABLE) != 0;
   GLboolean stride = (flags & VEC_BAD_STRIDE) != 0;
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   GLuint ca_flags = 0;
   (void) inputs;

   if (writeable || stride) ca_flags |= CA_CLIENT_DATA;

   if ((required & VERT_CLIP) && VB->ClipPtr == VB->ObjPtr)
      required |= VERT_OBJ;

/*     _tnl_print_vert_flags("_tnl_upgrade_client_data", required); */

   if ((required & VERT_OBJ) && (VB->ObjPtr->flags & flags)) {
      ASSERT(VB->ObjPtr == &inputs->Obj);
      _tnl_import_vertex( ctx, writeable, stride );
      VB->importable_data &= ~(VERT_OBJ|VERT_CLIP);
   }

   if ((required & VERT_NORM) && (VB->NormalPtr->flags & flags)) {
      ASSERT(VB->NormalPtr == &inputs->Normal);
      _tnl_import_normal( ctx, writeable, stride );
      VB->importable_data &= ~VERT_NORM;
   }

   if ((required & VERT_RGBA) && (VB->ColorPtr[0]->Flags & ca_flags)) {
      ASSERT(VB->ColorPtr[0] == &inputs->Color);
      _tnl_import_color( ctx, GL_FLOAT, writeable, stride );
      VB->importable_data &= ~VERT_RGBA;
   }

   if ((required & VERT_SPEC_RGB) && 
       (VB->SecondaryColorPtr[0]->Flags & ca_flags)) {
      ASSERT(VB->SecondaryColorPtr[0] == &inputs->SecondaryColor);
      _tnl_import_secondarycolor( ctx, GL_FLOAT, writeable, stride );
      VB->importable_data &= ~VERT_SPEC_RGB;
   }

   if ((required & VERT_FOG_COORD) && (VB->FogCoordPtr->flags & flags)) {
      ASSERT(VB->FogCoordPtr == &inputs->FogCoord);
      _tnl_import_fogcoord( ctx, writeable, stride );
      VB->importable_data &= ~VERT_FOG_COORD;
   }

   if ((required & VERT_INDEX) && (VB->IndexPtr[0]->flags & flags)) {
      ASSERT(VB->IndexPtr[0] == &inputs->Index);
      _tnl_import_index( ctx, writeable, stride );
      VB->importable_data &= ~VERT_INDEX;
   }

   if (required & VERT_TEX_ANY)
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
	 if ((required & VERT_TEX(i)) && (VB->TexCoordPtr[i]->flags & flags)) {
	    ASSERT(VB->TexCoordPtr[i] == &inputs->TexCoord[i]);
	    _tnl_import_texcoord( ctx, i, writeable, stride );
	    VB->importable_data &= ~VERT_TEX(i);
	 }

}



void _tnl_vb_bind_arrays( GLcontext *ctx, GLint start, GLsizei count )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint inputs = tnl->pipeline.inputs;
   struct vertex_arrays *tmp = &tnl->array_inputs;
   GLuint i;

/*        fprintf(stderr, "%s %d..%d // %d..%d\n", __FUNCTION__, */
/*  	      start, count, ctx->Array.LockFirst, ctx->Array.LockCount);  */
/*        _tnl_print_vert_flags("    inputs", inputs);  */
/*        _tnl_print_vert_flags("    _Enabled", ctx->Array._Enabled); */
/*        _tnl_print_vert_flags("    importable", inputs & VERT_FIXUP); */

   VB->Count = count - start;
   VB->FirstClipped = VB->Count;
   VB->Elts = 0;
   VB->MaterialMask = 0;
   VB->Material = 0;
   VB->Flag = 0;
   VB->Primitive = tnl->tmp_primitive;
   VB->PrimitiveLength = tnl->tmp_primitive_length;
   VB->import_data = _tnl_upgrade_client_data;
   VB->importable_data = inputs & VERT_FIXUP;

   if (ctx->Array.LockCount) {
      ASSERT(start == (GLint) ctx->Array.LockFirst);
      ASSERT(count == (GLint) ctx->Array.LockCount);
   }

   _ac_import_range( ctx, start, count );

   if (inputs & VERT_OBJ) {
      _tnl_import_vertex( ctx, 0, 0 );
      tmp->Obj.count = VB->Count;
      VB->ObjPtr = &tmp->Obj;
   }

   if (inputs & VERT_NORM) {
      _tnl_import_normal( ctx, 0, 0 );
      tmp->Normal.count = VB->Count;
      VB->NormalPtr = &tmp->Normal;
   }

   if (inputs & VERT_RGBA) {
      _tnl_import_color( ctx, 0, 0, 0 );
      VB->ColorPtr[0] = &tmp->Color;
      VB->ColorPtr[1] = 0;
   }

   if (inputs & VERT_TEX_ANY) {
      for (i = 0; i < ctx->Const.MaxTextureUnits ; i++) {
	 if (inputs & VERT_TEX(i)) {
	    _tnl_import_texcoord( ctx, i, 0, 0 );
	    tmp->TexCoord[i].count = VB->Count;
	    VB->TexCoordPtr[i] = &tmp->TexCoord[i];
	 }
      }
   }

   if (inputs & (VERT_INDEX|VERT_FOG_COORD|VERT_EDGE|VERT_SPEC_RGB)) {
      if (inputs & VERT_INDEX) {
	 _tnl_import_index( ctx, 0, 0 );
	 tmp->Index.count = VB->Count;
	 VB->IndexPtr[0] = &tmp->Index;
	 VB->IndexPtr[1] = 0;
      }

      if (inputs & VERT_FOG_COORD) {
	 _tnl_import_fogcoord( ctx, 0, 0 );
	 tmp->FogCoord.count = VB->Count;
	 VB->FogCoordPtr = &tmp->FogCoord;
      }

      if (inputs & VERT_EDGE) {
	 _tnl_import_edgeflag( ctx, GL_TRUE, sizeof(GLboolean) );
	 VB->EdgeFlag = (GLboolean *) tmp->EdgeFlag.data;
      }

      if (inputs & VERT_SPEC_RGB) {
	 _tnl_import_secondarycolor( ctx, 0, 0, 0 );
	 VB->SecondaryColorPtr[0] = &tmp->SecondaryColor;
	 VB->SecondaryColorPtr[1] = 0;
      }
   }
}




/* Function to fill an immediate struct with the effects of
 * consecutive calls to ArrayElement with consecutive indices.  Used
 * for display lists and very large fan-like primitives only.
 */
void _tnl_fill_immediate_drawarrays( GLcontext *ctx, struct immediate *IM,
				     GLuint start, GLuint count )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint required = ctx->Array._Enabled;
   GLuint n = count - start;
   GLuint i;

   if (!ctx->CompileFlag)
      required &= tnl->pipeline.inputs;

   if (MESA_VERBOSE&VERBOSE_IMMEDIATE)
      fprintf(stderr, "exec_full_array_elements %d .. %d\n", start, count);

   _math_trans_4f( IM->Obj + IM->Start,
		   ctx->Array.Vertex.Ptr,
		   ctx->Array.Vertex.StrideB,
		   ctx->Array.Vertex.Type,
		   ctx->Array.Vertex.Size,
		   start, n );

   if (ctx->Array.Vertex.Size == 4)
      required |= VERT_OBJ_234;
   else if (ctx->Array.Vertex.Size == 3)
      required |= VERT_OBJ_23;


   if (required & VERT_NORM) {
      _math_trans_3f( IM->Normal + IM->Start,
		      ctx->Array.Normal.Ptr,
		      ctx->Array.Normal.StrideB,
		      ctx->Array.Normal.Type,
		      start, n );
   }

   if (required & VERT_EDGE) {
      _math_trans_1ub( IM->EdgeFlag + IM->Start,
		       ctx->Array.EdgeFlag.Ptr,
		       ctx->Array.EdgeFlag.StrideB,
		       ctx->Array.EdgeFlag.Type,
		       start, n );
   }

   if (required & VERT_RGBA) {
      _math_trans_4f( IM->Color + IM->Start,
		      ctx->Array.Color.Ptr,
		      ctx->Array.Color.StrideB,
		      ctx->Array.Color.Type,
		      ctx->Array.Color.Size,
		      start, n );
   }

   if (required & VERT_SPEC_RGB) {
      _math_trans_4f( IM->SecondaryColor + IM->Start,
		      ctx->Array.SecondaryColor.Ptr,
		      ctx->Array.SecondaryColor.StrideB,
		      ctx->Array.SecondaryColor.Type,
		      ctx->Array.SecondaryColor.Size,
		      start, n );
   }

   if (required & VERT_FOG_COORD) {
      _math_trans_1f( IM->FogCoord + IM->Start,
		      ctx->Array.FogCoord.Ptr,
		      ctx->Array.FogCoord.StrideB,
		      ctx->Array.FogCoord.Type,
		      start, n );
   }

   if (required & VERT_INDEX) {
      _math_trans_1ui( IM->Index + IM->Start,
		       ctx->Array.Index.Ptr,
		       ctx->Array.Index.StrideB,
		       ctx->Array.Index.Type,
		       start, n );
   }

   if (required & VERT_TEX_ANY) {
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++) {
	 if (required & VERT_TEX(i)) {
	    _math_trans_4f( IM->TexCoord[i] + IM->Start,
			    ctx->Array.TexCoord[i].Ptr,
			    ctx->Array.TexCoord[i].StrideB,
			    ctx->Array.TexCoord[i].Type,
			    ctx->Array.TexCoord[i].Size,
			    start, n );

	    if (ctx->Array.TexCoord[i].Size == 4)
	       IM->TexSize |= TEX_SIZE_4(i);
	    else if (ctx->Array.TexCoord[i].Size == 3)
	       IM->TexSize |= TEX_SIZE_3(i);
	 }
      }
   }

   IM->Count = IM->Start + n;
   IM->Flag[IM->Start] &= IM->ArrayEltFlags;
   IM->Flag[IM->Start] |= required;
   for (i = IM->Start+1 ; i < IM->Count ; i++)
      IM->Flag[i] = required;
}
