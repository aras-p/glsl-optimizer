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
#include <unistd.h>

#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "r300_ioctl.h"
#include "radeon_mm.h"
#include "radeon_ioctl.h"

#ifdef USER_BUFFERS

static void resize_u_list(r300ContextPtr rmesa)
{
	void *temp;
	int nsize;
	
	temp = rmesa->rmm->u_list;
	nsize = rmesa->rmm->u_size * 2;
	
	rmesa->rmm->u_list = _mesa_malloc(nsize * sizeof(*rmesa->rmm->u_list));
	_mesa_memset(rmesa->rmm->u_list, 0, nsize * sizeof(*rmesa->rmm->u_list));
	
	if (temp) {
		r300FlushCmdBuf(rmesa, __FUNCTION__);
		
		_mesa_memcpy(rmesa->rmm->u_list, temp, rmesa->rmm->u_size * sizeof(*rmesa->rmm->u_list));
		_mesa_free(temp);
	}
	
	rmesa->rmm->u_size = nsize;
}

void radeon_mm_init(r300ContextPtr rmesa)
{
	rmesa->rmm = malloc(sizeof(struct radeon_memory_manager));
	memset(rmesa->rmm, 0, sizeof(struct radeon_memory_manager));
	
	rmesa->rmm->u_size = 128;
	resize_u_list(rmesa);
}

void radeon_mm_destroy(r300ContextPtr rmesa)
{
	_mesa_free(rmesa->rmm->u_list);
	rmesa->rmm->u_list = NULL;

	_mesa_free(rmesa->rmm);
	rmesa->rmm = NULL;
}

void *radeon_mm_ptr(r300ContextPtr rmesa, int id)
{
	assert(id <= rmesa->rmm->u_last);
	return rmesa->rmm->u_list[id].ptr;
}

int radeon_mm_find(r300ContextPtr rmesa, void *ptr)
{
	int i;
	
	for (i=1; i < rmesa->rmm->u_size+1; i++)
		if(rmesa->rmm->u_list[i].ptr &&
		   ptr >= rmesa->rmm->u_list[i].ptr &&
	    	   ptr < rmesa->rmm->u_list[i].ptr + rmesa->rmm->u_list[i].size)
			break;
	
	if (i < rmesa->rmm->u_size + 1)
		return i;
	
	fprintf(stderr, "%p failed\n", ptr);
	return 0;
}

//#define MM_DEBUG
int radeon_mm_alloc(r300ContextPtr rmesa, int alignment, int size)
{
	drm_radeon_mem_alloc_t alloc;
	int offset = 0, ret;
	int i, free=-1;
	int done_age;
	drm_radeon_mem_free_t memfree;
	int tries=0;
	static int bytes_wasted=0, allocated=0;
	
	if(size < 4096)
		bytes_wasted += 4096 - size;
	
	allocated += size;
	
#if 0
	static int t=0;
	if (t != time(NULL)) {
		t = time(NULL);
		fprintf(stderr, "slots used %d, wasted %d kb, allocated %d\n", rmesa->rmm->u_last, bytes_wasted/1024, allocated/1024);
	}
#endif
	
	memfree.region = RADEON_MEM_REGION_GART;
			
	again:
	
	done_age = radeonGetAge((radeonContextPtr)rmesa);
	
	if (rmesa->rmm->u_last + 1 >= rmesa->rmm->u_size)
		resize_u_list(rmesa);
	
	for (i = rmesa->rmm->u_last + 1; i > 0; i --) {
		if (rmesa->rmm->u_list[i].ptr == NULL) {
			free = i;
			continue;
		}
		
		if (rmesa->rmm->u_list[i].h_pending == 0 &&
			rmesa->rmm->u_list[i].pending && rmesa->rmm->u_list[i].age <= done_age) {
			memfree.region_offset = (char *)rmesa->rmm->u_list[i].ptr -
						(char *)rmesa->radeon.radeonScreen->gartTextures.map;

			ret = drmCommandWrite(rmesa->radeon.radeonScreen->driScreen->fd,
					      DRM_RADEON_FREE, &memfree, sizeof(memfree));

			if (ret) {
				fprintf(stderr, "Failed to free at %p\n", rmesa->rmm->u_list[i].ptr);
				fprintf(stderr, "ret = %s\n", strerror(-ret));
				exit(1);
			} else {
#ifdef MM_DEBUG
				fprintf(stderr, "really freed %d at age %x\n", i, radeonGetAge((radeonContextPtr)rmesa));
#endif
				if (i == rmesa->rmm->u_last)
					rmesa->rmm->u_last --;
				
					if(rmesa->rmm->u_list[i].size < 4096)
						bytes_wasted -= 4096 - rmesa->rmm->u_list[i].size;

				allocated -= rmesa->rmm->u_list[i].size;
				rmesa->rmm->u_list[i].pending = 0;
				rmesa->rmm->u_list[i].ptr = NULL;
				
				if (rmesa->rmm->u_list[i].fb) {
					LOCK_HARDWARE(&(rmesa->radeon));
					ret = mmFreeMem(rmesa->rmm->u_list[i].fb);
					UNLOCK_HARDWARE(&(rmesa->radeon));
					
					if (ret != 0)
						fprintf(stderr, "failed to free!\n");
					rmesa->rmm->u_list[i].fb = NULL;
				}
				rmesa->rmm->u_list[i].ref_count = 0;
				free = i;
			}
		}
	}
	rmesa->rmm->u_head = i;
	
	if (free == -1) {
		WARN_ONCE("Ran out of slots!\n");
		//usleep(100);
		r300FlushCmdBuf(rmesa, __FUNCTION__);
		tries++;
		if(tries>100){
			WARN_ONCE("Ran out of slots!\n");
			exit(1);
		}
		goto again;
	}
		
	alloc.region = RADEON_MEM_REGION_GART;
	alloc.alignment = alignment;
	alloc.size = size;
	alloc.region_offset = &offset;

	ret = drmCommandWriteRead( rmesa->radeon.dri.fd, DRM_RADEON_ALLOC, &alloc, sizeof(alloc));
   	if (ret) {
#if 0
		WARN_ONCE("Ran out of mem!\n");
		r300FlushCmdBuf(rmesa, __FUNCTION__);
		//usleep(100);
		tries2++;
		tries = 0;
		if(tries2>100){
			WARN_ONCE("Ran out of GART memory!\n");
			exit(1);
		}
		goto again;
#else
		WARN_ONCE("Ran out of GART memory (for %d)!\nPlease consider adjusting GARTSize option.\n", size);
		return 0;
#endif
	}
	
	i = free;
	
	if (i > rmesa->rmm->u_last)
		rmesa->rmm->u_last = i;
	
	rmesa->rmm->u_list[i].ptr = ((GLubyte *)rmesa->radeon.radeonScreen->gartTextures.map) + offset;
	rmesa->rmm->u_list[i].size = size;
	rmesa->rmm->u_list[i].age = 0;
	rmesa->rmm->u_list[i].fb = NULL;
	//fprintf(stderr, "alloc %p at id %d\n", rmesa->rmm->u_list[i].ptr, i);
	
#ifdef MM_DEBUG
	fprintf(stderr, "allocated %d at age %x\n", i, radeonGetAge((radeonContextPtr)rmesa));
#endif
	
	return i;
}

#include "r300_emit.h"
static void emit_lin_cp(r300ContextPtr rmesa, unsigned long dst, unsigned long src, unsigned long size)
{
	int cmd_reserved = 0;
	int cmd_written = 0;
	drm_radeon_cmd_header_t *cmd = NULL;
	int cp_size;
	
	
	while (size > 0){
		cp_size = size;
		if(cp_size > /*8190*/4096)
			cp_size = /*8190*/4096;
		
		reg_start(0x146c,1);
		e32(0x52cc32fb);
	
		reg_start(0x15ac,1);
		e32(src);
		e32(cp_size);
	
		reg_start(0x1704,0);
		e32(0x0);

		reg_start(0x1404,1);
		e32(dst);
		e32(cp_size);
	
		reg_start(0x1700,0);
		e32(0x0);
	
		reg_start(0x1640,3);
		e32(0x00000000);
		e32(0x00001fff);
		e32(0x00000000);
		e32(0x00001fff);

		start_packet3(RADEON_CP_PACKET3_UNK1B, 2);
		e32(0 << 16 | 0);
		e32(0 << 16 | 0);
		e32(cp_size << 16 | 0x1);
		
		dst += cp_size;
		src += cp_size;
		size -= cp_size;
	}
	
	reg_start(0x4e4c,0);
	e32(0x0000000a);
	
	reg_start(0x342c,0);
	e32(0x00000005);
	
	reg_start(0x1720,0);
	e32(0x00010000);
}

void radeon_mm_use(r300ContextPtr rmesa, int id)
{
	uint64_t ull;
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, radeonGetAge((radeonContextPtr)rmesa));
#endif	
	drm_r300_cmd_header_t *cmd;
	
	assert(id <= rmesa->rmm->u_last);
	
	if(id == 0)
		return;
	
#if 0 /* FB VBOs. Needs further changes... */
	rmesa->rmm->u_list[id].ref_count ++;
	if (rmesa->rmm->u_list[id].ref_count > 100 && rmesa->rmm->u_list[id].fb == NULL &&
		rmesa->rmm->u_list[id].size != RADEON_BUFFER_SIZE*16 /*&& rmesa->rmm->u_list[id].size > 40*/) {
		driTexHeap *heap;
		struct mem_block *mb;
				
		LOCK_HARDWARE(&(rmesa->radeon));
	
		heap = rmesa->texture_heaps[0];
			
		mb = mmAllocMem(heap->memory_heap, rmesa->rmm->u_list[id].size, heap->alignmentShift, 0);
	
		UNLOCK_HARDWARE(&(rmesa->radeon));
	
		if (mb) {
			rmesa->rmm->u_list[id].fb = mb;
			
			emit_lin_cp(rmesa, rmesa->radeon.radeonScreen->texOffset[0] + rmesa->rmm->u_list[id].fb->ofs,
					r300GartOffsetFromVirtual(rmesa, rmesa->rmm->u_list[id].ptr),
					rmesa->rmm->u_list[id].size);
		} else {
			WARN_ONCE("Upload to fb failed, %d, %d\n", rmesa->rmm->u_list[id].size, id);
		}
		//fprintf(stderr, "Upload to fb! %d, %d\n", rmesa->rmm->u_list[id].ref_count, id);
	}
	/*if (rmesa->rmm->u_list[id].fb) {
		emit_lin_cp(rmesa, rmesa->radeon.radeonScreen->texOffset[0] + rmesa->rmm->u_list[id].fb->ofs,
				r300GartOffsetFromVirtual(rmesa, rmesa->rmm->u_list[id].ptr),
				rmesa->rmm->u_list[id].size);
	}*/
#endif
		
	cmd = (drm_r300_cmd_header_t *)r300AllocCmdBuf(rmesa, 2 + sizeof(ull) / 4, __FUNCTION__);
	cmd[0].scratch.cmd_type = R300_CMD_SCRATCH;
	cmd[0].scratch.reg = RADEON_MM_SCRATCH;
	cmd[0].scratch.n_bufs = 1;
	cmd[0].scratch.flags = 0;
	cmd ++;
	
	ull = (uint64_t)(intptr_t)&rmesa->rmm->u_list[id].age;
	_mesa_memcpy(cmd, &ull, sizeof(ull));
	cmd += sizeof(ull) / 4;
	
	cmd[0].u = /*id*/0;
	
	LOCK_HARDWARE(&rmesa->radeon); /* Protect from DRM. */
	rmesa->rmm->u_list[id].h_pending ++;
	UNLOCK_HARDWARE(&rmesa->radeon);
}

unsigned long radeon_mm_offset(r300ContextPtr rmesa, int id)
{
	unsigned long offset;
	
	assert(id <= rmesa->rmm->u_last);
	
	if (rmesa->rmm->u_list[id].fb) {
		offset = rmesa->radeon.radeonScreen->texOffset[0] + rmesa->rmm->u_list[id].fb->ofs;
	} else {
		offset = (char *)rmesa->rmm->u_list[id].ptr -
			(char *)rmesa->radeon.radeonScreen->gartTextures.map;
		offset += rmesa->radeon.radeonScreen->gart_texture_offset;
	}
	
	return offset;
}

int radeon_mm_on_card(r300ContextPtr rmesa, int id)
{
	assert(id <= rmesa->rmm->u_last);
	
	if (rmesa->rmm->u_list[id].fb)
		return GL_TRUE;
	
	return GL_FALSE;
}
		
void *radeon_mm_map(r300ContextPtr rmesa, int id, int access)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, radeonGetAge((radeonContextPtr)rmesa));
#endif	
	void *ptr;
	int tries = 0;
	
	assert(id <= rmesa->rmm->u_last);
	
	rmesa->rmm->u_list[id].ref_count = 0;
	if (rmesa->rmm->u_list[id].fb) {
		WARN_ONCE("Mapping fb!\n");
		/* Idle gart only and do upload on unmap */
		//rmesa->rmm->u_list[id].fb = NULL;
		
		
		if(rmesa->rmm->u_list[id].mapped == 1)
			WARN_ONCE("buffer %d already mapped\n", id);
	
		rmesa->rmm->u_list[id].mapped = 1;
		ptr = radeon_mm_ptr(rmesa, id);
		
		return ptr;
	}
	
	if (access == RADEON_MM_R) {
		
		if(rmesa->rmm->u_list[id].mapped == 1)
			WARN_ONCE("buffer %d already mapped\n", id);
	
		rmesa->rmm->u_list[id].mapped = 1;
		ptr = radeon_mm_ptr(rmesa, id);
		
		return ptr;
	}
	
	
	if (rmesa->rmm->u_list[id].h_pending)
		r300FlushCmdBuf(rmesa, __FUNCTION__);
	
	if (rmesa->rmm->u_list[id].h_pending) {
		return NULL;
	}
	
	while(rmesa->rmm->u_list[id].age > radeonGetAge((radeonContextPtr)rmesa) && tries++ < 1000)
		usleep(10);
	
	if (tries >= 1000) {
		fprintf(stderr, "Idling failed (%x vs %x)\n",
				rmesa->rmm->u_list[id].age, radeonGetAge((radeonContextPtr)rmesa));
		return NULL;
	}
	
	if(rmesa->rmm->u_list[id].mapped == 1)
		WARN_ONCE("buffer %d already mapped\n", id);
	
	rmesa->rmm->u_list[id].mapped = 1;
	ptr = radeon_mm_ptr(rmesa, id);
	
	return ptr;
}

void radeon_mm_unmap(r300ContextPtr rmesa, int id)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, radeonGetAge((radeonContextPtr)rmesa));
#endif	
	
	assert(id <= rmesa->rmm->u_last);
	
	if(rmesa->rmm->u_list[id].mapped == 0)
		WARN_ONCE("buffer %d not mapped\n", id);
	
	rmesa->rmm->u_list[id].mapped = 0;
	
	if (rmesa->rmm->u_list[id].fb)
		emit_lin_cp(rmesa, rmesa->radeon.radeonScreen->texOffset[0] + rmesa->rmm->u_list[id].fb->ofs,
				r300GartOffsetFromVirtual(rmesa, rmesa->rmm->u_list[id].ptr),
				rmesa->rmm->u_list[id].size);
}

void radeon_mm_free(r300ContextPtr rmesa, int id)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, radeonGetAge((radeonContextPtr)rmesa));
#endif	
	
	assert(id <= rmesa->rmm->u_last);
	
	if(id == 0)
		return;
	
	if(rmesa->rmm->u_list[id].ptr == NULL){
		WARN_ONCE("Not allocated!\n");
		return ;
	}
	
	if(rmesa->rmm->u_list[id].pending){
		WARN_ONCE("%p already pended!\n", rmesa->rmm->u_list[id].ptr);
		return ;
	}
			
	rmesa->rmm->u_list[id].pending = 1;
}
#endif
