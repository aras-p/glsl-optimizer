/*
 * Copyright (C) 2005 Aapo Tahkola.
 *
 * All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial
 * portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 * IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 */
 
/*
 * Authors:
 *   Aapo Tahkola <aet@rasterburn.org>
 */

#include "context.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "r300_ioctl.h"
#include "r300_maos.h"
#include "r300_state.h"
#include "radeon_mm.h"

#include "hash.h"
#include "dispatch.h"
#include "bufferobj.h"
#include "vtxfmt.h"
#include "api_validate.h"
#include "state.h"
#include "image.h"

#define CONV_VB(a, b) rvb->AttribPtr[(a)].size = vb->b->size, \
			rvb->AttribPtr[(a)].type = GL_FLOAT, \
			rvb->AttribPtr[(a)].stride = vb->b->stride, \
			rvb->AttribPtr[(a)].data = vb->b->data

void radeon_vb_to_rvb(r300ContextPtr rmesa, struct radeon_vertex_buffer *rvb, struct vertex_buffer *vb)
{
	int i;
	GLcontext *ctx;
	ctx = rmesa->radeon.glCtx;
	
	memset(rvb, 0, sizeof(*rvb));
	
	rvb->Elts = vb->Elts;
	rvb->elt_size = 4;
	rvb->elt_min = 0;
	rvb->elt_max = vb->Count;
	
	rvb->Count = vb->Count;
	
	CONV_VB(VERT_ATTRIB_POS, ObjPtr);
	CONV_VB(VERT_ATTRIB_NORMAL, NormalPtr);
	CONV_VB(VERT_ATTRIB_COLOR0, ColorPtr[0]);
	CONV_VB(VERT_ATTRIB_COLOR1, SecondaryColorPtr[0]);
	CONV_VB(VERT_ATTRIB_FOG, FogCoordPtr);
	
	for (i=0; i < ctx->Const.MaxTextureCoordUnits; i++)
		CONV_VB(VERT_ATTRIB_TEX0 + i, TexCoordPtr[i]);

	for (i=0; i < MAX_VERTEX_PROGRAM_ATTRIBS; i++)
		CONV_VB(VERT_ATTRIB_GENERIC0 + i, AttribPtr[VERT_ATTRIB_GENERIC0 + i]);
	
	rvb->Primitive = vb->Primitive;
	rvb->PrimitiveCount = vb->PrimitiveCount;
	rvb->LockFirst = rvb->LockCount = 0;
	rvb->lock_uptodate = GL_FALSE;
}

#ifdef RADEON_VTXFMT_A

extern void _tnl_array_init( GLcontext *ctx );

#define CONV(a, b) \
    do { \
	if (ctx->Array.ArrayObj->b.Enabled) { \
	    rmesa->state.VB.AttribPtr[(a)].size = ctx->Array.ArrayObj->b.Size; \
	    rmesa->state.VB.AttribPtr[(a)].data = ctx->Array.ArrayObj->b.BufferObj->Name \
	      ? (void *)ADD_POINTERS(ctx->Array.ArrayObj->b.Ptr, ctx->Array.ArrayObj->b.BufferObj->Data) \
	      : (void *)ctx->Array.ArrayObj->b.Ptr; \
	    rmesa->state.VB.AttribPtr[(a)].stride = ctx->Array.ArrayObj->b.StrideB; \
	    rmesa->state.VB.AttribPtr[(a)].type = ctx->Array.ArrayObj->b.Type; \
	    enabled |= 1 << (a); \
	} \
    } while (0)

static int setup_arrays(r300ContextPtr rmesa, GLint start)
{
	int i;
	struct dt def = { 4, GL_FLOAT, 0, NULL };
	GLcontext *ctx;
	GLuint enabled = 0;
	
	ctx = rmesa->radeon.glCtx;
	i = r300Fallback(ctx);
	if (i)
		return i;

	memset(rmesa->state.VB.AttribPtr, 0, VERT_ATTRIB_MAX*sizeof(struct dt));
	
	CONV(VERT_ATTRIB_POS, Vertex);
	CONV(VERT_ATTRIB_NORMAL, Normal);
	CONV(VERT_ATTRIB_COLOR0, Color);
	CONV(VERT_ATTRIB_COLOR1, SecondaryColor);
	CONV(VERT_ATTRIB_FOG, FogCoord);
	
	for (i=0; i < MAX_TEXTURE_COORD_UNITS; i++)
		CONV(VERT_ATTRIB_TEX0 + i, TexCoord[i]);
	
	if (ctx->VertexProgram._Enabled)
		for (i=0; i < VERT_ATTRIB_MAX; i++)
			CONV(i, VertexAttrib[i]);
	
	for (i=0; i < VERT_ATTRIB_MAX; i++) {
		if (enabled & (1 << i)) {
			rmesa->state.VB.AttribPtr[i].data += rmesa->state.VB.AttribPtr[i].stride * start;
		} else {
			def.data = ctx->Current.Attrib[i];
			memcpy(&rmesa->state.VB.AttribPtr[i], &def, sizeof(struct dt));
		}
		
		/*if(rmesa->state.VB.AttribPtr[i].data == ctx->Current.Attrib[i])
			fprintf(stderr, "%d is default coord\n", i);*/
	}
	
	for(i=0; i < VERT_ATTRIB_MAX; i++){
		if(rmesa->state.VB.AttribPtr[i].type != GL_UNSIGNED_BYTE &&
#if MESA_LITTLE_ENDIAN
		   rmesa->state.VB.AttribPtr[i].type != GL_SHORT &&
#endif
		   rmesa->state.VB.AttribPtr[i].type != GL_FLOAT){
			WARN_ONCE("Unsupported format %d at index %d\n", rmesa->state.VB.AttribPtr[i].type, i);
			return R300_FALLBACK_TCL;
		}
		
		/*fprintf(stderr, "%d: ", i);
		
		switch(rmesa->state.VB.AttribPtr[i].type){
		case GL_BYTE: fprintf(stderr, "byte "); break;
		case GL_UNSIGNED_BYTE: fprintf(stderr, "u byte "); break;
		case GL_SHORT: fprintf(stderr, "short "); break;
		case GL_UNSIGNED_SHORT: fprintf(stderr, "u short "); break;
		case GL_INT: fprintf(stderr, "int "); break;
		case GL_UNSIGNED_INT: fprintf(stderr, "u int "); break;
		case GL_FLOAT: fprintf(stderr, "float "); break;
		case GL_2_BYTES: fprintf(stderr, "2 bytes "); break;
		case GL_3_BYTES: fprintf(stderr, "3 bytes "); break;
		case GL_4_BYTES: fprintf(stderr, "4 bytes "); break;
		case GL_DOUBLE: fprintf(stderr, "double "); break;
		default: fprintf(stderr, "unknown "); break;
		}

		fprintf(stderr, "Size %d ", rmesa->state.VB.AttribPtr[i].size);
		fprintf(stderr, "Ptr %p ", rmesa->state.VB.AttribPtr[i].data);
		fprintf(stderr, "Stride %d ", rmesa->state.VB.AttribPtr[i].stride);
		fprintf(stderr, "\n");*/
	}
	return R300_FALLBACK_NONE;
}

void radeon_init_vtxfmt_a(r300ContextPtr rmesa);

static void radeonDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *c_indices )
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int elt_size;
	int i;
	unsigned int min = ~0, max = 0;
	struct tnl_prim prim;
	static void *ptr = NULL;
	struct r300_dma_region rvb;
	const GLvoid *indices = c_indices;
	
	if (count > 65535) {
		WARN_ONCE("Too many verts!\n");
		goto fallback;
	}
	
	if (ctx->Array.ElementArrayBufferObj->Name) {
		/* use indices in the buffer object */
		if (!ctx->Array.ElementArrayBufferObj->Data) {
			_mesa_warning(ctx, "DrawRangeElements with empty vertex elements buffer!");
			return;
		}
		/* actual address is the sum of pointers */
		indices = (GLvoid *)
		ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data, (const GLubyte *) c_indices);
	}
	
	if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
		return;
	
	FLUSH_CURRENT( ctx, 0 );
	
	memset(&rvb, 0, sizeof(rvb));
	switch (type) {
	case GL_UNSIGNED_BYTE:
		for (i=0; i < count; i++) {
			if(((unsigned char *)indices)[i] < min)
				min = ((unsigned char *)indices)[i];
			if(((unsigned char *)indices)[i] > max)
				max = ((unsigned char *)indices)[i];
		}
		
#ifdef FORCE_32BITS_ELTS
		elt_size = 4;
#else
		elt_size = 2;
#endif		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
			
#ifdef FORCE_32BITS_ELTS
		for (i=0; i < count; i++)
			((unsigned int *)ptr)[i] = ((unsigned char *)indices)[i] - min;
#else
		for (i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned char *)indices)[i] - min;
#endif
	break;
		
	case GL_UNSIGNED_SHORT:
		for (i=0; i < count; i++) {
			if(((unsigned short int *)indices)[i] < min)
				min = ((unsigned short int *)indices)[i];
			if(((unsigned short int *)indices)[i] > max)
				max = ((unsigned short int *)indices)[i];
		}
		
#ifdef FORCE_32BITS_ELTS
		elt_size = 4;
#else
		elt_size = 2;
#endif
		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
#ifdef FORCE_32BITS_ELTS
		for (i=0; i < count; i++)
			((unsigned int *)ptr)[i] = ((unsigned short int *)indices)[i] - min;
#else
		for (i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned short int *)indices)[i] - min;
#endif
	break;
	
	case GL_UNSIGNED_INT:
		for (i=0; i < count; i++) {
			if(((unsigned int *)indices)[i] < min)
				min = ((unsigned int *)indices)[i];
			if(((unsigned int *)indices)[i] > max)
				max = ((unsigned int *)indices)[i];
		}
		
#ifdef FORCE_32BITS_ELTS
		elt_size = 4;
#else
		if (max - min <= 65535)
			elt_size = 2;
		else 
			elt_size = 4;
#endif
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
		
		if (elt_size == 2)
			for (i=0; i < count; i++)
				((unsigned short int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
		else
			for (i=0; i < count; i++)
				((unsigned int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
	break;
	
	default:
		WARN_ONCE("Unknown elt type!\n");
	goto fallback;
	}
	
	if (ctx->NewState) 
		_mesa_update_state( ctx );
	
	r300UpdateShaders(rmesa);
	
	if (setup_arrays(rmesa, min) >= R300_FALLBACK_TCL) {
		r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
		goto fallback;
	}
	
	rmesa->state.VB.Count = max - min + 1;
	
	r300UpdateShaderStates(rmesa);
	
	rmesa->state.VB.Primitive = &prim;
	rmesa->state.VB.PrimitiveCount = 1;
	
	prim.mode = mode | PRIM_BEGIN | PRIM_END;
	if (rmesa->state.VB.LockCount)
		prim.start = min - rmesa->state.VB.LockFirst;
	else
		prim.start = 0;
	prim.count = count;
	
	rmesa->state.VB.Elts = ptr;
	rmesa->state.VB.elt_size = elt_size;
	
	if (r300_run_vb_render(ctx, NULL)) {
		r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
		goto fallback;
	}
	
	if(rvb.buf)
		radeon_mm_use(rmesa, rvb.buf->id);
	
	r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
	return;
	
	fallback:
	_tnl_array_init(ctx);
	_mesa_install_exec_vtxfmt( ctx, &TNL_CONTEXT(ctx)->exec_vtxfmt );
	CALL_DrawElements(GET_DISPATCH(), (mode, count, type, c_indices));
	radeon_init_vtxfmt_a(rmesa);
	_mesa_install_exec_vtxfmt( ctx, &TNL_CONTEXT(ctx)->exec_vtxfmt );
}

static void radeonDrawRangeElements(GLenum mode, GLuint min, GLuint max, GLsizei count, GLenum type, const GLvoid *c_indices)
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct tnl_prim prim;
	int elt_size;
	int i;
	void *ptr = NULL;
	struct r300_dma_region rvb;
	const GLvoid *indices = c_indices;
	
	if (count > 65535) {
		/* TODO */
		if (mode == GL_POINTS ||
		    mode == GL_LINES ||
		    mode == GL_QUADS ||
		    mode == GL_TRIANGLES) {
			
			while (count) {
				i = r300_get_num_verts(rmesa, MIN2(count, 65535), mode);
				
				radeonDrawRangeElements(mode, min, max, i, type, indices);
				
				indices += i * _mesa_sizeof_type(type);
				count -= i;
			}
			return ;
		}
		WARN_ONCE("Too many verts!\n");
		goto fallback;
	}
	
	if (ctx->Array.ElementArrayBufferObj->Name) {
		/* use indices in the buffer object */
		if (!ctx->Array.ElementArrayBufferObj->Data) {
			_mesa_warning(ctx, "DrawRangeElements with empty vertex elements buffer!");
			return;
		}
		/* actual address is the sum of pointers */
		indices = (GLvoid *)
		ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data, (const GLubyte *) c_indices);
	}
	
	if (!_mesa_validate_DrawRangeElements( ctx, mode, min, max, count, type, indices ))
		return;
	
	FLUSH_CURRENT( ctx, 0 );
#ifdef OPTIMIZE_ELTS
	min = 0;
#endif
	
	memset(&rvb, 0, sizeof(rvb));
	switch (type){
	case GL_UNSIGNED_BYTE:
#ifdef FORCE_32BITS_ELTS
		elt_size = 4;
#else
		elt_size = 2;
#endif	
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
#ifdef FORCE_32BITS_ELTS
		for(i=0; i < count; i++)
			((unsigned int *)ptr)[i] = ((unsigned char *)indices)[i] - min;
#else
		for(i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned char *)indices)[i] - min;
#endif
	break;
	
	case GL_UNSIGNED_SHORT:
#ifdef FORCE_32BITS_ELTS
		elt_size = 4;
#else
		elt_size = 2;
#endif	
#ifdef OPTIMIZE_ELTS
		if (min == 0 && ctx->Array.ElementArrayBufferObj->Name){
			ptr = indices;
			break;
		}
#endif
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;

#ifdef FORCE_32BITS_ELTS
		for(i=0; i < count; i++)
			((unsigned int *)ptr)[i] = ((unsigned short int *)indices)[i] - min;
#else
		for(i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned short int *)indices)[i] - min;
#endif
	break;
	
	case GL_UNSIGNED_INT:
#ifdef FORCE_32BITS_ELTS
		elt_size = 4;
#else
		if (max - min <= 65535)
			elt_size = 2;
		else 
			elt_size = 4;
#endif	
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
		if (elt_size == 2)
			for (i=0; i < count; i++)
				((unsigned short int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
		else
			for (i=0; i < count; i++)
				((unsigned int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
	break;
	
	default:
		WARN_ONCE("Unknown elt type!\n");
	goto fallback;
	}
	
	/* XXX: setup_arrays before state update? */
	
	if (ctx->NewState) 
		_mesa_update_state( ctx );
	
	r300UpdateShaders(rmesa);

	if (setup_arrays(rmesa, min) >= R300_FALLBACK_TCL) {
		r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
		goto fallback;
	}

	rmesa->state.VB.Count = max - min + 1;
	
	r300UpdateShaderStates(rmesa);
	
	rmesa->state.VB.Primitive = &prim;
	rmesa->state.VB.PrimitiveCount = 1;
	
	prim.mode = mode | PRIM_BEGIN | PRIM_END;
	if (rmesa->state.VB.LockCount)
		prim.start = min - rmesa->state.VB.LockFirst;
	else
		prim.start = 0;
	prim.count = count;
	
	rmesa->state.VB.Elts = ptr;
	rmesa->state.VB.elt_size = elt_size;
	rmesa->state.VB.elt_min = min;
	rmesa->state.VB.elt_max = max;
	
	if (r300_run_vb_render(ctx, NULL)) {
		r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
		goto fallback;
	}
	
	if(rvb.buf)
		radeon_mm_use(rmesa, rvb.buf->id);
	
	r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
	return ;
	
	fallback:
	_tnl_array_init(ctx);
	_mesa_install_exec_vtxfmt( ctx, &TNL_CONTEXT(ctx)->exec_vtxfmt );
	CALL_DrawRangeElements(GET_DISPATCH(), (mode, min, max, count, type, c_indices));
	radeon_init_vtxfmt_a(rmesa);
	_mesa_install_exec_vtxfmt( ctx, &TNL_CONTEXT(ctx)->exec_vtxfmt );
}

static void radeonDrawArrays( GLenum mode, GLint start, GLsizei count )
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct tnl_prim prim;
	
	if (count > 65535) {
		WARN_ONCE("Too many verts!\n");
		goto fallback;
	}
	
	if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
		return;
	
	FLUSH_CURRENT( ctx, 0 );
	
	if (ctx->NewState) 
		_mesa_update_state( ctx );
	
	/* XXX: setup_arrays before state update? */
	
	r300UpdateShaders(rmesa);

	if (setup_arrays(rmesa, start) >= R300_FALLBACK_TCL)
		goto fallback;

	rmesa->state.VB.Count = count;

	r300UpdateShaderStates(rmesa);
	
	rmesa->state.VB.Primitive = &prim;
	rmesa->state.VB.PrimitiveCount = 1;
	
	prim.mode = mode | PRIM_BEGIN | PRIM_END;
	if (rmesa->state.VB.LockCount)
		prim.start = start - rmesa->state.VB.LockFirst;
	else
		prim.start = 0;
	prim.count = count;
	
	rmesa->state.VB.Elts = NULL;
	rmesa->state.VB.elt_size = 0;
	rmesa->state.VB.elt_min = 0;
	rmesa->state.VB.elt_max = 0;
	
	if (r300_run_vb_render(ctx, NULL))
		goto fallback;

	return ;
	
	fallback:
	_tnl_array_init(ctx);
	_mesa_install_exec_vtxfmt( ctx, &TNL_CONTEXT(ctx)->exec_vtxfmt );
	CALL_DrawArrays(GET_DISPATCH(), (mode, start, count));
	radeon_init_vtxfmt_a(rmesa);
	_mesa_install_exec_vtxfmt( ctx, &TNL_CONTEXT(ctx)->exec_vtxfmt );
}

void radeon_init_vtxfmt_a(r300ContextPtr rmesa)
{
	GLcontext *ctx;
	GLvertexformat *vfmt;
	
	ctx = rmesa->radeon.glCtx; 
	vfmt = (GLvertexformat *)ctx->TnlModule.Current;
   
	vfmt->DrawElements = radeonDrawElements;
	vfmt->DrawArrays = radeonDrawArrays;
	vfmt->DrawRangeElements = radeonDrawRangeElements;
	
}
#endif

#ifdef HW_VBOS

static struct gl_buffer_object *
r300NewBufferObject(GLcontext *ctx, GLuint name, GLenum target )
{
	struct r300_buffer_object *obj;

	(void) ctx;

	obj = MALLOC_STRUCT(r300_buffer_object);
	_mesa_initialize_buffer_object(&obj->mesa_obj, name, target);
	return &obj->mesa_obj;
}

static void r300BufferData(GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		const GLvoid *data, GLenum usage, struct gl_buffer_object *obj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)obj;

	/* Free previous buffer */
	if (obj->OnCard) {
		radeon_mm_free(rmesa, r300_obj->id);
		obj->OnCard = GL_FALSE;
	} else {
		if (obj->Data)
			_mesa_free(obj->Data);
	}
#ifdef OPTIMIZE_ELTS
	if (0) {
#else
	if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {
#endif
		fallback:
		obj->Data = malloc(size);
		
		if (data)
			_mesa_memcpy(obj->Data, data, size);
		
		obj->OnCard = GL_FALSE;
	} else {
		r300_obj->id = radeon_mm_alloc(rmesa, 4, size);
		if (r300_obj->id == 0)
			goto fallback;
		
		obj->Data = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_W);
	
		if (data)
			_mesa_memcpy(obj->Data, data, size);
	
		radeon_mm_unmap(rmesa, r300_obj->id);
		obj->OnCard = GL_TRUE;
	}
	
	obj->Size = size;
	obj->Usage = usage;
}

static void r300BufferSubData(GLcontext *ctx, GLenum target, GLintptrARB offset,
		GLsizeiptrARB size, const GLvoid * data, struct gl_buffer_object * bufObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)bufObj;
	(void) ctx; (void) target;
	void *ptr;

	if (bufObj->Data && ((GLuint) (size + offset) <= bufObj->Size)) {
		if (bufObj->OnCard){
			ptr = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_W);
		
			_mesa_memcpy( (GLubyte *) ptr + offset, data, size );
		
			radeon_mm_unmap(rmesa, r300_obj->id);
		} else {
			_mesa_memcpy( (GLubyte *) bufObj->Data + offset, data, size );
		}
	}
}

static void *r300MapBuffer(GLcontext *ctx, GLenum target, GLenum access,
		struct gl_buffer_object *bufObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)bufObj;
	
	(void) ctx;
	(void) target;
	(void) access;
	//ASSERT(!bufObj->OnCard);
	/* Just return a direct pointer to the data */
	if (bufObj->Pointer) {
		/* already mapped! */
		return NULL;
	}
	
	if (!bufObj->OnCard) {
		bufObj->Pointer = bufObj->Data;
		return bufObj->Pointer;
	}
	
	switch (access) {
	case GL_READ_ONLY:
		bufObj->Pointer = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_R);
	break;
	
	case GL_WRITE_ONLY:
		bufObj->Pointer = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_W);
	break;
	
	case GL_READ_WRITE:
		bufObj->Pointer = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_RW);
	break;
	
	default:
		WARN_ONCE("Unknown access type\n");
		bufObj->Pointer = NULL;
	break;
	}
	
	return bufObj->Pointer;
}

static GLboolean r300UnmapBuffer(GLcontext *ctx, GLenum target, struct gl_buffer_object *bufObj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)bufObj;
	
	(void) ctx;
	(void) target;
	//ASSERT(!bufObj->OnCard);
	/* XXX we might assert here that bufObj->Pointer is non-null */
	if (!bufObj->OnCard) {
		bufObj->Pointer = NULL;
		return GL_TRUE;
	}
	radeon_mm_unmap(rmesa, r300_obj->id);
	
	bufObj->Pointer = NULL;
	return GL_TRUE;
}

static void r300DeleteBuffer(GLcontext *ctx, struct gl_buffer_object *obj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)obj;
	
	if (obj->OnCard) {
		radeon_mm_free(rmesa, r300_obj->id);
		obj->Data = NULL;
	}
	_mesa_delete_buffer_object(ctx, obj);
}

void r300_evict_vbos(GLcontext *ctx, int amount)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct _mesa_HashTable *hash = ctx->Shared->BufferObjects;
	GLuint k = _mesa_HashFirstEntry(hash);
	
	while (amount > 0 && k) {
		struct gl_buffer_object *obj = _mesa_lookup_bufferobj(ctx, k);
		struct r300_buffer_object *r300_obj
			= (struct r300_buffer_object *) obj;
		
		if (obj->OnCard && obj->Size) {
			GLvoid *data;
			obj->Data = _mesa_malloc(obj->Size);
			
			data = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_R);
			_mesa_memcpy(obj->Data, data, obj->Size);
			radeon_mm_unmap(rmesa, r300_obj->id);
			
			radeon_mm_free(rmesa, r300_obj->id);
			r300_obj->id = 0;
			obj->OnCard = GL_FALSE;
			
			amount -= obj->Size;
		}
		
		k = _mesa_HashNextEntry(hash, k);
	}
	
}

void r300_init_vbo_funcs(struct dd_function_table *functions)
{
	functions->NewBufferObject = r300NewBufferObject;
	functions->BufferData = r300BufferData;
	functions->BufferSubData = r300BufferSubData;
	functions->MapBuffer = r300MapBuffer;
	functions->UnmapBuffer = r300UnmapBuffer;
	functions->DeleteBuffer = r300DeleteBuffer;
}

#endif
