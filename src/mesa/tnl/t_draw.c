
/*
 * Mesa 3-D graphics library
 * Version:  6.5
 *
 * Copyright (C) 1999-2006  Brian Paul   All Rights Reserved.
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
#include "imports.h"
#include "state.h"
#include "mtypes.h"
#include "macros.h"

#include "t_context.h"
#include "t_pipeline.h"
#include "t_vp_build.h"
#include "t_vertex.h"
#include "tnl.h"



static GLfloat *get_space(GLcontext *ctx, GLuint bytes)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLubyte *space = _mesa_malloc(bytes);
   
   tnl->block[tnl->nr_blocks++] = space;
   return (GLfloat *)space;
}


static void free_space(GLcontext *ctx)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   GLuint i;
   for (i = 0; i < tnl->nr_blocks; i++)
      _mesa_free(tnl->block[i]);
   tnl->nr_blocks = 0;
}


/* Convert the incoming array to GLfloats.  Understands the
 * array->Normalized flag and selects the correct conversion method.
 */
#define CONVERT( TYPE, MACRO ) do {		\
   GLuint i, j;					\
   if (input->Normalized) {			\
      for (i = 0; i < count; i++) {		\
	 const TYPE *in = (TYPE *)ptr;		\
	 for (j = 0; j < sz; j++) {		\
	    *fptr++ = MACRO(*in);		\
	    in++;				\
	 }					\
	 ptr += input->StrideB;			\
      }						\
   } else {					\
      for (i = 0; i < count; i++) {		\
	 const TYPE *in = (TYPE *)ptr;		\
	 for (j = 0; j < sz; j++) {		\
	    *fptr++ = (GLfloat)(*in);		\
	    in++;				\
	 }					\
	 ptr += input->StrideB;			\
      }						\
   }						\
} while (0)



/* Adjust pointer to point at first requested element, convert to
 * floating point, populate VB->AttribPtr[].
 */
static void _tnl_import_array( GLcontext *ctx,
			       GLuint attrib,
			       GLuint start,
			       GLuint end,
			       const struct gl_client_array *input,
			       const char *ptr )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   const GLuint count = end - start;
   GLuint stride = input->StrideB;

   ptr += start * stride;

   if (input->Type != GL_FLOAT) {
      const GLuint sz = input->Size;
      GLfloat *fptr = get_space(ctx, count * sz * sizeof(GLfloat));

      switch (input->Type) {
      case GL_BYTE: 
	 CONVERT(GLbyte, BYTE_TO_FLOAT); 
	 break;
      case GL_UNSIGNED_BYTE: 
	 CONVERT(GLubyte, UBYTE_TO_FLOAT); 
	 break;
      case GL_SHORT: 
	 CONVERT(GLshort, SHORT_TO_FLOAT); 
	 break;
      case GL_UNSIGNED_SHORT: 
	 CONVERT(GLushort, USHORT_TO_FLOAT); 
	 break;
      case GL_INT: 
	 CONVERT(GLint, INT_TO_FLOAT); 
	 break;
      case GL_UNSIGNED_INT: 
	 CONVERT(GLuint, UINT_TO_FLOAT); 
	 break;
      case GL_DOUBLE: 
	 CONVERT(GLdouble, (GLfloat)); 
	 break;
      default:
	 assert(0);
	 break;
      }

      ptr = (const char *)fptr;
      stride = sz * sizeof(GLfloat);
   }

   VB->AttribPtr[attrib] = &tnl->tmp_inputs[attrib];
   VB->AttribPtr[attrib]->data = (GLfloat (*)[4])ptr;
   VB->AttribPtr[attrib]->start = (GLfloat *)ptr;
   VB->AttribPtr[attrib]->count = count;
   VB->AttribPtr[attrib]->stride = stride;
   VB->AttribPtr[attrib]->size = input->Size;

   /* This should die, but so should the whole GLvector4f concept: 
    */
   VB->AttribPtr[attrib]->flags = (((1<<input->Size)-1) | 
				   VEC_NOT_WRITEABLE |
				   (stride == 4*sizeof(GLfloat) ? 0 : VEC_BAD_STRIDE));
   
   VB->AttribPtr[attrib]->storage = NULL;
}

#define CLIPVERTS  ((6 + MAX_CLIP_PLANES) * 2)


static GLboolean *_tnl_import_edgeflag( GLcontext *ctx,
					const GLvector4f *input,
					GLuint count)
{
   const GLubyte *ptr = (const GLubyte *)input->data;
   const GLuint stride = input->stride;
   GLboolean *space = (GLboolean *)get_space(ctx, count + CLIPVERTS);
   GLboolean *bptr = space;
   GLuint i;

   for (i = 0; i < count; i++) {
      *bptr++ = ((GLfloat *)ptr)[0] == 1.0;
      ptr += stride;
   }

   return space;
}


static void bind_inputs( GLcontext *ctx, 
			 const struct gl_client_array *inputs[],
			 GLint start, GLint end,
			 struct gl_buffer_object **bo,
			 GLuint *nr_bo )
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;
   GLuint i;

   /* Map all the VBOs
    */
   for (i = 0; i < VERT_ATTRIB_MAX; i++) {
      const void *ptr;

      if (inputs[i]->BufferObj->Name) { 
	 if (!inputs[i]->BufferObj->Pointer) {
	    bo[*nr_bo] = inputs[i]->BufferObj;
	    *nr_bo++;
	    ctx->Driver.MapBuffer(ctx, 
				  GL_ARRAY_BUFFER,
				  GL_READ_ONLY_ARB,
				  inputs[i]->BufferObj);
	    
	    assert(inputs[i]->BufferObj->Pointer);
	 }
	 
	 ptr = ADD_POINTERS(inputs[i]->BufferObj->Pointer,
			    inputs[i]->Ptr);
      }
      else
	 ptr = inputs[i]->Ptr;

      /* Just make sure the array is floating point, otherwise convert to
       * temporary storage.  Rebase arrays so that 'start' becomes
       * element zero.
       *
       * XXX: remove the GLvector4f type at some stage and just use
       * client arrays.
       */
      _tnl_import_array(ctx, i, start, end, inputs[i], ptr);
   }

   /* Legacy pointers -- remove one day.
    */
   VB->ObjPtr = VB->AttribPtr[_TNL_ATTRIB_POS];
   VB->NormalPtr = VB->AttribPtr[_TNL_ATTRIB_NORMAL];
   VB->ColorPtr[0] = VB->AttribPtr[_TNL_ATTRIB_COLOR0];
   VB->ColorPtr[1] = NULL;
   VB->IndexPtr[0] = VB->AttribPtr[_TNL_ATTRIB_COLOR_INDEX];
   VB->IndexPtr[1] = NULL;
   VB->SecondaryColorPtr[0] = VB->AttribPtr[_TNL_ATTRIB_COLOR1];
   VB->SecondaryColorPtr[1] = NULL;
   VB->FogCoordPtr = VB->AttribPtr[_TNL_ATTRIB_FOG];

   for (i = 0; i < ctx->Const.MaxTextureCoordUnits; i++) {
      VB->TexCoordPtr[i] = VB->AttribPtr[_TNL_ATTRIB_TEX0 + i];
   }

   /* Clipping and drawing code still requires this to be a packed
    * array of ubytes which can be written into.  TODO: Fix and
    * remove.
    */
   if (ctx->Polygon.FrontMode != GL_FILL ||
       ctx->Polygon.BackMode != GL_FILL)
   {
      VB->EdgeFlag = _tnl_import_edgeflag( ctx, 
					   VB->AttribPtr[_TNL_ATTRIB_EDGEFLAG],
					   VB->Count );
   }

}


/* Translate indices to GLuints and store in VB->Elts.
 */
static void bind_indicies( GLcontext *ctx,
			   const struct _mesa_index_buffer *ib,
			   struct gl_buffer_object **bo,
			   GLuint *nr_bo)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   if (!ib)
      return;

   if (ib->obj->Name && !ib->obj->Pointer) {
      bo[*nr_bo] = ib->obj;
      *nr_bo++;
      ctx->Driver.MapBuffer(ctx, 
			    GL_ELEMENT_ARRAY_BUFFER,
			    GL_READ_ONLY_ARB,
			    ib->obj);

      assert(ib->obj->Pointer);
   }

   VB->Elts = (GLuint *)ADD_POINTERS(ib->obj->Pointer, 
				     ib->ptr);
   
   VB->Elts += ib->rebase;

   switch (ib->type) {
   case GL_UNSIGNED_INT:
      return;
   case GL_UNSIGNED_SHORT:
      break;
   case GL_UNSIGNED_BYTE:
      break;
   }
}

static void unmap_vbos( GLcontext *ctx,
			struct gl_buffer_object **bo,
			GLuint nr_bo )
{
   GLuint i;
   for (i = 0; i < nr_bo; i++) { 
      ctx->Driver.UnmapBuffer(ctx, 
			      0, /* target -- I don't see why this would be needed */
			      bo[i]);
   }
}



/* This is the main entrypoint into the slimmed-down software tnl
 * module.  In a regular swtnl driver, this can be plugged straight
 * into the vbo->Driver.DrawPrims() callback.
 */
void _tnl_draw_prims( GLcontext *ctx,
		      const struct gl_client_array *arrays[],
		      const struct _mesa_prim *prim,
		      GLuint nr_prims,
		      const struct _mesa_index_buffer *ib,
		      GLuint min_index,
		      GLuint max_index)
{
   TNLcontext *tnl = TNL_CONTEXT(ctx);
   struct vertex_buffer *VB = &tnl->vb;

   /* May need to map a vertex buffer object for every attribute plus
    * one for the index buffer.
    */
   struct gl_buffer_object *bo[VERT_ATTRIB_MAX + 1];
   GLuint nr_bo = 0;

   /* Binding inputs may imply mapping some vertex buffer objects.
    * They will need to be unmapped below.
    */
   bind_inputs(ctx, arrays, min_index, max_index, bo, &nr_bo);
   bind_indicies(ctx, ib, bo, &nr_bo);

   VB->Primitive = prim;
   VB->PrimitiveCount = nr_prims;
   VB->Count = max_index - min_index;

   TNL_CONTEXT(ctx)->Driver.RunPipeline(ctx);

   unmap_vbos(ctx, bo, nr_bo);
   free_space(ctx);
}

