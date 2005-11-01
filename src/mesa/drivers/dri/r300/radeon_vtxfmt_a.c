#include "context.h"
#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "radeon_mm.h"

#ifdef RADEON_VTXFMT_A

#define CONV(a, b) rmesa->state.VB.AttribPtr[(a)].size = ctx->Array.b.Size, \
			rmesa->state.VB.AttribPtr[(a)].data = ctx->Array.b.BufferObj->Name ? \
			ADD_POINTERS(ctx->Array.b.Ptr, ctx->Array.b.BufferObj->Data) : ctx->Array.b.Ptr, \
			rmesa->state.VB.AttribPtr[(a)].stride = ctx->Array.b.StrideB, \
			rmesa->state.VB.AttribPtr[(a)].type = ctx->Array.b.Type

static int setup_arrays(r300ContextPtr rmesa, GLint start)
{
	int i;
	struct dt def = { 4, GL_FLOAT, 0, NULL };
	GLcontext *ctx;
	GLuint enabled = 0;
	
	ctx = rmesa->radeon.glCtx; 

	memset(rmesa->state.VB.AttribPtr, 0, VERT_ATTRIB_MAX*sizeof(struct dt));
	
	CONV(VERT_ATTRIB_POS, Vertex);
	if (ctx->Array.Vertex.Enabled)
		enabled |= 1 << VERT_ATTRIB_POS;
	
	CONV(VERT_ATTRIB_NORMAL, Normal);
	if (ctx->Array.Normal.Enabled)
		enabled |= 1 << VERT_ATTRIB_NORMAL;
	
	CONV(VERT_ATTRIB_COLOR0, Color);
	if (ctx->Array.Color.Enabled)
		enabled |= 1 << VERT_ATTRIB_COLOR0;
	
	CONV(VERT_ATTRIB_COLOR1, SecondaryColor);
	if (ctx->Array.SecondaryColor.Enabled)
		enabled |= 1 << VERT_ATTRIB_COLOR1;
	
	CONV(VERT_ATTRIB_FOG, FogCoord);
	if (ctx->Array.FogCoord.Enabled)
		enabled |= 1 << VERT_ATTRIB_FOG;
	
	for (i=0; i < MAX_TEXTURE_COORD_UNITS; i++) {
		CONV(VERT_ATTRIB_TEX0 + i, TexCoord[i]);
		
		if(ctx->Array.TexCoord[i].Enabled) {
			enabled |= 1 << (VERT_ATTRIB_TEX0+i);
		}
		
	}

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
			rmesa->state.VB.AttribPtr[i].type != GL_FLOAT){
			WARN_ONCE("Unsupported format %d at index %d\n", rmesa->state.VB.AttribPtr[i].type, i);
			return -1;
		}
		if(rmesa->state.VB.AttribPtr[i].type == GL_UNSIGNED_BYTE &&
			rmesa->state.VB.AttribPtr[i].size != 4){
			WARN_ONCE("Unsupported component count for ub colors\n");
			return -1;
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
	return 0;
}

void radeonDrawElements( GLenum mode, GLsizei count, GLenum type, const GLvoid *indices )
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int elt_size;
	int i;
	unsigned int min = ~0, max = 0;
	struct tnl_prim prim;
	static void *ptr = NULL;
	static struct r300_dma_region rvb;
	
	if (ctx->Array.ElementArrayBufferObj->Name) {
		/* use indices in the buffer object */
		if (!ctx->Array.ElementArrayBufferObj->Data) {
			_mesa_warning(ctx, "DrawRangeElements with empty vertex elements buffer!");
			return;
		}
		/* actual address is the sum of pointers */
		indices = (const GLvoid *)
		ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data, (const GLubyte *) indices);
	}
	
	if (!_mesa_validate_DrawElements( ctx, mode, count, type, indices ))
		return;
	
	FLUSH_CURRENT( ctx, 0 );
	/*
		fprintf(stderr, "dt at %s:\n", __FUNCTION__);
		for(i=0; i < VERT_ATTRIB_MAX; i++){
			fprintf(stderr, "dt %d:", i);
			dump_dt(&rmesa->state.VB.AttribPtr[i], rmesa->state.VB.Count);
		}*/
	r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
	
	switch (type) {
	case GL_UNSIGNED_BYTE:
		for (i=0; i < count; i++) {
			if(((unsigned char *)indices)[i] < min)
				min = ((unsigned char *)indices)[i];
			if(((unsigned char *)indices)[i] > max)
				max = ((unsigned char *)indices)[i];
		}
		
		elt_size = 2;
		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
			
		for (i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned char *)indices)[i] - min;
	break;
		
	case GL_UNSIGNED_SHORT:
		for (i=0; i < count; i++) {
			if(((unsigned short int *)indices)[i] < min)
				min = ((unsigned short int *)indices)[i];
			if(((unsigned short int *)indices)[i] > max)
				max = ((unsigned short int *)indices)[i];
		}
		
		elt_size = 2;
		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
		for (i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned short int *)indices)[i] - min;
	break;
	
	case GL_UNSIGNED_INT:
		for (i=0; i < count; i++) {
			if(((unsigned int *)indices)[i] < min)
				min = ((unsigned int *)indices)[i];
			if(((unsigned int *)indices)[i] > max)
				max = ((unsigned int *)indices)[i];
		}
		
		if (max - min <= 65535)
			elt_size = 2;
		else 
			elt_size = 4;
		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
		
		if (max - min <= 65535)
			for (i=0; i < count; i++)
				((unsigned short int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
		else
			for (i=0; i < count; i++)
				((unsigned int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
	break;
	
	default:
		fprintf(stderr, "Unknown elt type!\n");
	return;
	

	}
	
	if (ctx->NewState) 
		_mesa_update_state( ctx );
	
	r300UpdateShaders(rmesa);
	
	if (rmesa->state.VB.LockCount) {
		if (rmesa->state.VB.lock_uptodate == GL_FALSE) {
			if (setup_arrays(rmesa, rmesa->state.VB.LockFirst))
				return;
			
			rmesa->state.VB.Count = rmesa->state.VB.LockCount;
			
			r300ReleaseArrays(ctx);
			r300EmitArraysVtx(ctx, GL_FALSE);
			
			rmesa->state.VB.lock_uptodate = GL_TRUE;
		}
		
		if (min < rmesa->state.VB.LockFirst) {
			WARN_ONCE("Out of range min %d vs %d!\n", min, rmesa->state.VB.LockFirst);
			return;
		}
		
		if (max >= rmesa->state.VB.LockFirst + rmesa->state.VB.LockCount) {
			WARN_ONCE("Out of range max %d vs %d!\n", max, rmesa->state.VB.LockFirst +
					rmesa->state.VB.LockCount);
			return;
		}
	} else {
		if (setup_arrays(rmesa, min))
			return;
		rmesa->state.VB.Count = max - min + 1;
	}
	
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
	
	r300_run_vb_render_vtxfmt_a(ctx, NULL);
	
	if(rvb.buf)
		radeon_mm_use(rmesa, rvb.buf->id);
}

void radeonDrawRangeElements(GLenum mode, GLuint min, GLuint max, GLsizei count, GLenum type, const GLvoid *indices)
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct tnl_prim prim;
	int elt_size;
	int i;
	void *ptr = NULL;
	static struct r300_dma_region rvb;
	
	if (ctx->Array.ElementArrayBufferObj->Name) {
		/* use indices in the buffer object */
		if (!ctx->Array.ElementArrayBufferObj->Data) {
			_mesa_warning(ctx, "DrawRangeElements with empty vertex elements buffer!");
			return;
		}
		/* actual address is the sum of pointers */
		indices = (const GLvoid *)
		ADD_POINTERS(ctx->Array.ElementArrayBufferObj->Data, (const GLubyte *) indices);
	}
	
	if (!_mesa_validate_DrawRangeElements( ctx, mode, min, max, count, type, indices ))
		return;
	
	FLUSH_CURRENT( ctx, 0 );
#ifdef OPTIMIZE_ELTS
	min = 0;
#endif
	r300ReleaseDmaRegion(rmesa, &rvb, __FUNCTION__);
	
	switch(type){
	case GL_UNSIGNED_BYTE:
		elt_size = 2;
		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
		for(i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned char *)indices)[i] - min;
	break;
	
	case GL_UNSIGNED_SHORT:
		elt_size = 2;
		
#ifdef OPTIMIZE_ELTS
		if (min == 0 && ctx->Array.ElementArrayBufferObj->Name){
			ptr = indices;
			break;
		}
#endif
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;

		for(i=0; i < count; i++)
			((unsigned short int *)ptr)[i] = ((unsigned short int *)indices)[i] - min;
	break;
	
	case GL_UNSIGNED_INT:
		if (max - min <= 65535)
			elt_size = 2;
		else 
			elt_size = 4;
		
		r300AllocDmaRegion(rmesa, &rvb, count * elt_size, elt_size);
		rvb.aos_offset = GET_START(&rvb);
		ptr = rvb.address + rvb.start;
		
		if (max - min <= 65535)
			for (i=0; i < count; i++)
				((unsigned short int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
		else
			for (i=0; i < count; i++)
				((unsigned int *)ptr)[i] = ((unsigned int *)indices)[i] - min;
	break;
	
	default:
		fprintf(stderr, "Unknown elt type!\n");
	return;
	
	}
	
	/* XXX: setup_arrays before state update? */
	
	if (ctx->NewState) 
		_mesa_update_state( ctx );
	
	r300UpdateShaders(rmesa);

	if (rmesa->state.VB.LockCount) {
		if (rmesa->state.VB.lock_uptodate == GL_FALSE) {
			if (setup_arrays(rmesa, rmesa->state.VB.LockFirst))
				return;
			
			rmesa->state.VB.Count = rmesa->state.VB.LockCount;
			
			r300ReleaseArrays(ctx);
			r300EmitArraysVtx(ctx, GL_FALSE);
			
			rmesa->state.VB.lock_uptodate = GL_TRUE;
		}
		
		if (min < rmesa->state.VB.LockFirst) {
			WARN_ONCE("Out of range min %d vs %d!\n", min, rmesa->state.VB.LockFirst);
			return;
		}
		
		/*if (max >= rmesa->state.VB.LockFirst + rmesa->state.VB.LockCount) {
			WARN_ONCE("Out of range max %d vs %d!\n", max, rmesa->state.VB.LockFirst +
					rmesa->state.VB.LockCount);
			return;
		}*/
	} else {
		if (setup_arrays(rmesa, min))
			return;
		rmesa->state.VB.Count = max - min + 1;
	}
	
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
	
	r300_run_vb_render_vtxfmt_a(ctx, NULL);
	
	if(rvb.buf)
		radeon_mm_use(rmesa, rvb.buf->id);
}

void radeonDrawArrays( GLenum mode, GLint start, GLsizei count )
{
	GET_CURRENT_CONTEXT(ctx);
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct tnl_prim prim;
	
	if (!_mesa_validate_DrawArrays( ctx, mode, start, count ))
		return;
	
	FLUSH_CURRENT( ctx, 0 );
	
	if (ctx->NewState) 
		_mesa_update_state( ctx );
	
	/* XXX: setup_arrays before state update? */
	
	r300UpdateShaders(rmesa);

	if (rmesa->state.VB.LockCount) {
		if (rmesa->state.VB.lock_uptodate == GL_FALSE) {
			if (setup_arrays(rmesa, rmesa->state.VB.LockFirst))
				return;
			
			rmesa->state.VB.Count = rmesa->state.VB.LockCount;
			
			r300ReleaseArrays(ctx);
			r300EmitArraysVtx(ctx, GL_FALSE);
			
			rmesa->state.VB.lock_uptodate = GL_TRUE;
		}
		
		if (start < rmesa->state.VB.LockFirst) {
			WARN_ONCE("Out of range min %d vs %d!\n", start, rmesa->state.VB.LockFirst);
			return;
		}
		
		if (start + count - 1 >= rmesa->state.VB.LockFirst + rmesa->state.VB.LockCount) { /* XXX */
			WARN_ONCE("Out of range max %d vs %d!\n", start + count - 1, rmesa->state.VB.LockFirst +
					rmesa->state.VB.LockCount);
			return;
		}
	} else {
		if (setup_arrays(rmesa, start))
			return;
		rmesa->state.VB.Count = count;
	}

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
	
	r300_run_vb_render_vtxfmt_a(ctx, NULL);
}

void radeon_init_vtxfmt_a(r300ContextPtr rmesa)
{
	GLcontext *ctx;
	GLvertexformat *vfmt;
	
	ctx = rmesa->radeon.glCtx; 
	vfmt = ctx->TnlModule.Current;
   
	vfmt->DrawElements = radeonDrawElements;
	vfmt->DrawArrays = radeonDrawArrays;
	vfmt->DrawRangeElements = radeonDrawRangeElements;
	
}
#endif

#ifdef HW_VBOS

void radeonLockArraysEXT(GLcontext *ctx, GLint first, GLsizei count)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	int i;
	
	/* Only when CB_DPATH is defined.
	   r300Clear tampers over the aos setup without it.
	   (r300ResetHwState cannot call r300EmitArrays)
	 */
#ifndef CB_DPATH
	first = 0; count = 0;
#endif
	
	if (first < 0 || count <= 0) {
		rmesa->state.VB.LockFirst = 0;
		rmesa->state.VB.LockCount = 0;
		rmesa->state.VB.lock_uptodate = GL_FALSE;
		return ;
	}
	
	rmesa->state.VB.LockFirst = first;
	rmesa->state.VB.LockCount = count;
	rmesa->state.VB.lock_uptodate = GL_FALSE;
}

void radeonUnlockArraysEXT(GLcontext *ctx)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	
	rmesa->state.VB.LockFirst = 0;
	rmesa->state.VB.LockCount = 0;
	rmesa->state.VB.lock_uptodate = GL_FALSE;
}

struct gl_buffer_object *
r300NewBufferObject(GLcontext *ctx, GLuint name, GLenum target )
{
	struct r300_buffer_object *obj;

	(void) ctx;

	obj = MALLOC_STRUCT(r300_buffer_object);
	_mesa_initialize_buffer_object(&obj->mesa_obj, name, target);
	return &obj->mesa_obj;
}

void r300BufferData(GLcontext *ctx, GLenum target, GLsizeiptrARB size,
		const GLvoid *data, GLenum usage, struct gl_buffer_object *obj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)obj;
	drm_radeon_mem_alloc_t alloc;
	int offset, ret;

	/* Free previous buffer */
	if (obj->OnCard) {
		radeon_mm_free(rmesa, r300_obj->id);
		obj->OnCard = GL_FALSE;
	} else {
		if (obj->Data)
			free(obj->Data);
	}
#ifdef OPTIMIZE_ELTS
	if (0) {
#else
	if (target == GL_ELEMENT_ARRAY_BUFFER_ARB) {
#endif
		obj->Data = malloc(size);
		
		if (data)
			memcpy(obj->Data, data, size);
		
		obj->OnCard = GL_FALSE;
	} else {
		r300_obj->id = radeon_mm_alloc(rmesa, 4, size);
		obj->Data = radeon_mm_map(rmesa, r300_obj->id, RADEON_MM_W);
	
		if (data)
			memcpy(obj->Data, data, size);
	
		radeon_mm_unmap(rmesa, r300_obj->id);
		obj->OnCard = GL_TRUE;
	}
	
	obj->Size = size;
	obj->Usage = usage;
}

void r300BufferSubData(GLcontext *ctx, GLenum target, GLintptrARB offset,
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

void *r300MapBuffer(GLcontext *ctx, GLenum target, GLenum access,
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

GLboolean r300UnmapBuffer(GLcontext *ctx, GLenum target, struct gl_buffer_object *bufObj)
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

void r300DeleteBuffer(GLcontext *ctx, struct gl_buffer_object *obj)
{
	r300ContextPtr rmesa = R300_CONTEXT(ctx);
	struct r300_buffer_object *r300_obj = (struct r300_buffer_object *)obj;
	
	if (obj->OnCard) {
		radeon_mm_free(rmesa, r300_obj->id);
		obj->Data = NULL;
	}
	_mesa_delete_buffer_object(ctx, obj);
}

void r300_init_vbo_funcs(struct dd_function_table *functions)
{
	functions->NewBufferObject = r300NewBufferObject;
	functions->BufferData = r300BufferData;
	functions->BufferSubData = r300BufferSubData;
	functions->MapBuffer = r300MapBuffer;
	functions->UnmapBuffer = r300UnmapBuffer;
	functions->DeleteBuffer = r300DeleteBuffer;
	
	functions->LockArraysEXT = radeonLockArraysEXT;
	functions->UnlockArraysEXT = radeonUnlockArraysEXT;
}

#endif
