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
#include "context.h"
#include "macros.h"
#include "imports.h"
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
   GLubyte *data;

   tmp = _ac_import_vertex(ctx,
			   GL_FLOAT,
			   stride ? 4*sizeof(GLfloat) : 0,
			   0,
			   writeable,
			   &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->Obj.data = (GLfloat (*)[4]) data;
   inputs->Obj.start = (GLfloat *) data;
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
   GLubyte *data;

   tmp = _ac_import_normal(ctx, GL_FLOAT,
			   stride ? 3*sizeof(GLfloat) : 0, writeable,
			   &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->Normal.data = (GLfloat (*)[4]) data;
   inputs->Normal.start = (GLfloat *) data;
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
   GLubyte *data;

   tmp = _ac_import_fogcoord(ctx, GL_FLOAT,
			     stride ? sizeof(GLfloat) : 0, writeable,
			     &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->FogCoord.data = (GLfloat (*)[4]) data;
   inputs->FogCoord.start = (GLfloat *) data;
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
   GLubyte *data;

   tmp = _ac_import_index(ctx, GL_UNSIGNED_INT,
			  stride ? sizeof(GLuint) : 0, writeable,
			  &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->Index.data = (GLuint *) data;
   inputs->Index.start = (GLuint *) data;
   inputs->Index.stride = tmp->StrideB;
   inputs->Index.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->Index.stride != sizeof(GLuint))
      inputs->Index.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->Index.flags |= VEC_NOT_WRITEABLE;
}


static void _tnl_import_texcoord( GLcontext *ctx,
				  GLuint unit,
				  GLboolean writeable,
				  GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   GLubyte *data;

   tmp = _ac_import_texcoord(ctx, unit, GL_FLOAT,
			     stride ? 4 * sizeof(GLfloat) : 0,
			     0,
			     writeable,
			     &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->TexCoord[unit].data = (GLfloat (*)[4]) data;
   inputs->TexCoord[unit].start = (GLfloat *) data;
   inputs->TexCoord[unit].stride = tmp->StrideB;
   inputs->TexCoord[unit].size = tmp->Size;
   inputs->TexCoord[unit].flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->TexCoord[unit].stride != 4*sizeof(GLfloat))
      inputs->TexCoord[unit].flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->TexCoord[unit].flags |= VEC_NOT_WRITEABLE;
}


static void _tnl_import_edgeflag( GLcontext *ctx,
				  GLboolean writeable,
				  GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   GLubyte *data;

   tmp = _ac_import_edgeflag(ctx, GL_UNSIGNED_BYTE,
			     stride ? sizeof(GLubyte) : 0,
			     0,
			     &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->EdgeFlag.data = (GLubyte *) data;
   inputs->EdgeFlag.start = (GLubyte *) data;
   inputs->EdgeFlag.stride = tmp->StrideB;
   inputs->EdgeFlag.flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->EdgeFlag.stride != sizeof(GLubyte))
      inputs->EdgeFlag.flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->EdgeFlag.flags |= VEC_NOT_WRITEABLE;
}



static void _tnl_import_attrib( GLcontext *ctx,
                                GLuint index,
                                GLboolean writeable,
                                GLboolean stride )
{
   struct vertex_arrays *inputs = &TNL_CONTEXT(ctx)->array_inputs;
   struct gl_client_array *tmp;
   GLboolean is_writeable = 0;
   GLubyte *data;

   tmp = _ac_import_attrib(ctx, index, GL_FLOAT,
                           stride ? 4 * sizeof(GLfloat) : 0,
                           4,  /* want GLfloat[4] */
                           writeable,
                           &is_writeable);

   data = ADD_POINTERS(tmp->Ptr, tmp->BufferObj->Data);
   inputs->Attribs[index].data = (GLfloat (*)[4]) data;
   inputs->Attribs[index].start = (GLfloat *) data;
   inputs->Attribs[index].stride = tmp->StrideB;
   inputs->Attribs[index].size = tmp->Size;
   inputs->Attribs[index].flags &= ~(VEC_BAD_STRIDE|VEC_NOT_WRITEABLE);
   if (inputs->Attribs[index].stride != 4 * sizeof(GLfloat))
      inputs->Attribs[index].flags |= VEC_BAD_STRIDE;
   if (!is_writeable)
      inputs->Attribs[index].flags |= VEC_NOT_WRITEABLE;
}



/**
 * Callback for VB stages that need to improve the quality of arrays
 * bound to the VB.  This is only necessary for client arrays which
 * have not been transformed at any point in the pipeline.
 * \param required - bitmask of VERT_*_BIT flags
 * \param flags - bitmask of VEC_* flags (ex: VEC_NOT_WRITABLE)
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

   if ((required & VERT_BIT_CLIP) && VB->ClipPtr == VB->ObjPtr)
      required |= VERT_BIT_POS;

/*     _tnl_print_vert_flags("_tnl_upgrade_client_data", required); */

   if ((required & VERT_BIT_POS) && (VB->ObjPtr->flags & flags)) {
      ASSERT(VB->ObjPtr == &inputs->Obj);
      _tnl_import_vertex( ctx, writeable, stride );
      VB->importable_data &= ~(VERT_BIT_POS|VERT_BIT_CLIP);
   }

   if ((required & VERT_BIT_NORMAL) && (VB->NormalPtr->flags & flags)) {
      ASSERT(VB->NormalPtr == &inputs->Normal);
      _tnl_import_normal( ctx, writeable, stride );
      VB->importable_data &= ~VERT_BIT_NORMAL;
   }

   if ((required & VERT_BIT_COLOR0) && (VB->ColorPtr[0]->Flags & ca_flags)) {
      ASSERT(VB->ColorPtr[0] == &inputs->Color);
      _tnl_import_color( ctx, GL_FLOAT, writeable, stride );
      VB->importable_data &= ~VERT_BIT_COLOR0;
   }

   if ((required & VERT_BIT_COLOR1) && 
       (VB->SecondaryColorPtr[0]->Flags & ca_flags)) {
      ASSERT(VB->SecondaryColorPtr[0] == &inputs->SecondaryColor);
      _tnl_import_secondarycolor( ctx, GL_FLOAT, writeable, stride );
      VB->importable_data &= ~VERT_BIT_COLOR1;
   }

   if ((required & VERT_BIT_FOG)
       && (VB->FogCoordPtr->flags & flags)) {
      ASSERT(VB->FogCoordPtr == &inputs->FogCoord);
      _tnl_import_fogcoord( ctx, writeable, stride );
      VB->importable_data &= ~VERT_BIT_FOG;
   }

   if ((required & VERT_BIT_INDEX) && (VB->IndexPtr[0]->flags & flags)) {
      ASSERT(VB->IndexPtr[0] == &inputs->Index);
      _tnl_import_index( ctx, writeable, stride );
      VB->importable_data &= ~VERT_BIT_INDEX;
   }

   if (required & VERT_BITS_TEX_ANY)
      for (i = 0 ; i < ctx->Const.MaxTextureUnits ; i++)
	 if ((required & VERT_BIT_TEX(i)) && (VB->TexCoordPtr[i]->flags & flags)) {
	    ASSERT(VB->TexCoordPtr[i] == &inputs->TexCoord[i]);
	    _tnl_import_texcoord( ctx, i, writeable, stride );
	    VB->importable_data &= ~VERT_BIT_TEX(i);
	 }

   /* XXX not sure what to do here for vertex program arrays */
}



void _tnl_vb_bind_arrays( GLcontext *ctx, GLint start, GLsizei count )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint inputs = tnl->pipeline.inputs;
   struct vertex_arrays *tmp = &tnl->array_inputs;

/*        _mesa_debug(ctx, "%s %d..%d // %d..%d\n", __FUNCTION__, */
/*  	      start, count, ctx->Array.LockFirst, ctx->Array.LockCount);  */
/*        _tnl_print_vert_flags("    inputs", inputs);  */
/*        _tnl_print_vert_flags("    _Enabled", ctx->Array._Enabled); */
/*        _tnl_print_vert_flags("    importable", inputs & VERT_BITS_FIXUP); */

   VB->Count = count - start;
   VB->FirstClipped = VB->Count;
   VB->Elts = NULL;
   VB->MaterialMask = NULL;
   VB->Material = NULL;
   VB->Flag = NULL;
   VB->Primitive = tnl->tmp_primitive;
   VB->PrimitiveLength = tnl->tmp_primitive_length;
   VB->import_data = _tnl_upgrade_client_data;
   VB->importable_data = inputs & VERT_BITS_FIXUP;

   if (ctx->Array.LockCount) {
      ASSERT(start == (GLint) ctx->Array.LockFirst);
      ASSERT(count == (GLint) ctx->Array.LockCount);
   }

   _ac_import_range( ctx, start, count );

   /* When vertex program mode is enabled, the generic vertex program
    * attribute arrays have priority over the conventional attributes.
    * Try to use them now.
    */
   if (ctx->VertexProgram.Enabled) {
      GLuint index;
      for (index = 0; index < VERT_ATTRIB_MAX; index++) {
         /* XXX check program->InputsRead to reduce work here */
         _tnl_import_attrib( ctx, index, GL_FALSE, GL_TRUE );
         VB->AttribPtr[index] = &tmp->Attribs[index];
      }
   }

   /*
    * Conventional attributes
    */
   if (inputs & VERT_BIT_POS) {
      _tnl_import_vertex( ctx, 0, 0 );
      tmp->Obj.count = VB->Count;
      VB->ObjPtr = &tmp->Obj;
   }

   if (inputs & VERT_BIT_NORMAL) {
      _tnl_import_normal( ctx, 0, 0 );
      tmp->Normal.count = VB->Count;
      VB->NormalPtr = &tmp->Normal;
   }

   if (inputs & VERT_BIT_COLOR0) {
      _tnl_import_color( ctx, 0, 0, 0 );
      VB->ColorPtr[0] = &tmp->Color;
      VB->ColorPtr[1] = 0;
   }

   if (inputs & VERT_BITS_TEX_ANY) {
      GLuint unit;
      for (unit = 0; unit < ctx->Const.MaxTextureUnits; unit++) {
	 if (inputs & VERT_BIT_TEX(unit)) {
	    _tnl_import_texcoord( ctx, unit, GL_FALSE, GL_FALSE );
	    tmp->TexCoord[unit].count = VB->Count;
	    VB->TexCoordPtr[unit] = &tmp->TexCoord[unit];
	 }
      }
   }

   if (inputs & (VERT_BIT_INDEX | VERT_BIT_FOG |
                 VERT_BIT_EDGEFLAG | VERT_BIT_COLOR1)) {
      if (inputs & VERT_BIT_INDEX) {
	 _tnl_import_index( ctx, 0, 0 );
	 tmp->Index.count = VB->Count;
	 VB->IndexPtr[0] = &tmp->Index;
	 VB->IndexPtr[1] = 0;
      }

      if (inputs & VERT_BIT_FOG) {
	 _tnl_import_fogcoord( ctx, 0, 0 );
	 tmp->FogCoord.count = VB->Count;
	 VB->FogCoordPtr = &tmp->FogCoord;
      }

      if (inputs & VERT_BIT_EDGEFLAG) {
	 _tnl_import_edgeflag( ctx, GL_TRUE, sizeof(GLboolean) );
	 VB->EdgeFlag = (GLboolean *) tmp->EdgeFlag.data;
      }

      if (inputs & VERT_BIT_COLOR1) {
	 _tnl_import_secondarycolor( ctx, 0, 0, 0 );
	 VB->SecondaryColorPtr[0] = &tmp->SecondaryColor;
	 VB->SecondaryColorPtr[1] = 0;
      }
   }
}
