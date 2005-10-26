#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/sem.h>

#include "r300_context.h"
#include "r300_cmdbuf.h"
#include "radeon_mm.h"

#ifdef USER_BUFFERS
void radeon_mm_reset(r300ContextPtr rmesa)
{
	drm_r300_cmd_header_t *cmd;
	int ret;
	
	memset(rmesa->rmm, 0, sizeof(struct radeon_memory_manager));
		
	rmesa->rmm->u_size = 1024; //2048;
	//rmesa->radeon.radeonScreen->scratch[2] = rmesa->rmm->vb_age;
#if 0 /* FIXME */
	cmd = r300AllocCmdBuf(rmesa, 4, __FUNCTION__);
	cmd[0].scratch.cmd_type = R300_CMD_SCRATCH;
	cmd[0].scratch.reg = 2;
	cmd[0].scratch.n_bufs = 1;
	cmd[0].scratch.flags = 0;
	cmd[1].u = (unsigned long)(&rmesa->rmm->vb_age);
	cmd[2].u = (unsigned long)(&rmesa->rmm->u_list[0].age);
	cmd[3].u = /*id*/0;

	/* Protect from DRM. */	
	LOCK_HARDWARE(&rmesa->radeon);
	rmesa->rmm->u_list[0].h_pending ++;
	ret = r300FlushCmdBufLocked(rmesa, __FUNCTION__);
	UNLOCK_HARDWARE(&rmesa->radeon);

	if (ret) {
		WARN_ONCE("r300FlushCmdBufLocked\n");
		exit(1);
	}
#endif
}

void radeon_mm_init(r300ContextPtr rmesa)
{
	
	rmesa->mm_ipc_key = 0xdeadbeed; //ftok("/tmp/.r300.mm_lock", "x");
	if(rmesa->mm_ipc_key == -1){
		perror("ftok");
		exit(1);
	}
	
	rmesa->mm_shm_id = shmget(rmesa->mm_ipc_key, sizeof(struct radeon_memory_manager), 0644);
	if (rmesa->mm_shm_id == -1) {
		rmesa->mm_shm_id = shmget(rmesa->mm_ipc_key, sizeof(struct radeon_memory_manager), 0644 | IPC_CREAT);
		
		rmesa->rmm = shmat(rmesa->mm_shm_id, (void *)0, 0);
		if (rmesa->rmm == (char *)(-1)) {
			perror("shmat");
			exit(1);
		}
		
		radeon_mm_reset(rmesa);
		
		rmesa->mm_sem_id = semget(rmesa->mm_ipc_key, 2, 0666 | IPC_CREAT);
		if (rmesa->mm_sem_id == -1) {
			perror("semget");
			exit(1);
		}
		
		return ;
	}
	
	rmesa->rmm = shmat(rmesa->mm_shm_id, (void *)0, 0);
	if (rmesa->rmm == (char *)(-1)) {
		perror("shmat");
		exit(1);
	}
	/* FIXME */
	radeon_mm_reset(rmesa);
			
	rmesa->mm_sem_id = semget(rmesa->mm_ipc_key, 2, 0666);
	if (rmesa->mm_sem_id == -1) {
		perror("semget");
		exit(1);
	}
}

static void radeon_mm_lock(r300ContextPtr rmesa, int sem)
{
	struct sembuf sb = { 0, 1, 0 };
	
	sb.sem_num = sem;
	
	if (semop(rmesa->mm_sem_id, &sb, 1) == -1) {
		perror("semop");
		exit(1);
	}
}

static void radeon_mm_unlock(r300ContextPtr rmesa, int sem)
{
	struct sembuf sb = { 0, -1, 0 };
	
	sb.sem_num = sem;
			
	if (semop(rmesa->mm_sem_id, &sb, 1) == -1) {
		perror("semop");
		exit(1);
	}	
}

void *radeon_mm_ptr(r300ContextPtr rmesa, int id)
{
	return rmesa->rmm->u_list[id].ptr;
}

//#define MM_DEBUG
int radeon_mm_alloc(r300ContextPtr rmesa, int alignment, int size)
{
	drm_radeon_mem_alloc_t alloc;
	int offset, ret;
	int i, end, free=-1;
	int done_age;
	drm_radeon_mem_free_t memfree;
	int tries=0, tries2=0;
	
	memfree.region = RADEON_MEM_REGION_GART;
			
	radeon_mm_lock(rmesa, RADEON_MM_UL);
	
	again:
	
	done_age = rmesa->radeon.radeonScreen->scratch[2];
	
	i = 1; //rmesa->rmm->u_head + 1;
	//i &= rmesa->rmm->u_size - 1;
	
	end = i + rmesa->rmm->u_size;
	//end &= rmesa->rmm->u_size - 1;
	
	for (; i != end; i ++/*, i &= rmesa->rmm->u_size-1*/) {
		if (rmesa->rmm->u_list[i].ptr == NULL){
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
				//fprintf(stderr, "Failed to free at %p\n", rmesa->rmm->u_list[i].ptr);
				//fprintf(stderr, "ret = %s\n", strerror(-ret));
				
				//radeon_mm_unlock(rmesa, RADEON_MM_UL);
				//exit(1);
			} else {
#ifdef MM_DEBUG
				fprintf(stderr, "really freed %d at age %x\n", i, rmesa->radeon.radeonScreen->scratch[2]);
#endif
				rmesa->rmm->u_list[i].pending = 0;
				rmesa->rmm->u_list[i].ptr = NULL;
				free = i;
			}
		}
	}
	done:
	rmesa->rmm->u_head = i;
	
	if (free == -1) {
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
		r300FlushCmdBuf(rmesa, __FUNCTION__);
		//usleep(100);
		tries2++;
		tries = 0;
		if(tries2>100){
			WARN_ONCE("Ran out of GART memory!\n");
			exit(1);
		}
		goto again;
	}
	
	i = free;
	rmesa->rmm->u_list[i].ptr = ((GLubyte *)rmesa->radeon.radeonScreen->gartTextures.map) + offset;
	rmesa->rmm->u_list[i].size = size;
	rmesa->rmm->u_list[i].age = 0;
	
#ifdef MM_DEBUG
	fprintf(stderr, "allocated %d at age %x\n", i, rmesa->radeon.radeonScreen->scratch[2]);
#endif
	
	radeon_mm_unlock(rmesa, RADEON_MM_UL);
	
	return i;
}

void radeon_mm_use(r300ContextPtr rmesa, int id)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, rmesa->radeon.radeonScreen->scratch[2]);
#endif	
	drm_r300_cmd_header_t *cmd;
	
	if(id == 0)
		return;
	
	radeon_mm_lock(rmesa, RADEON_MM_UL);
		
	cmd = r300AllocCmdBuf(rmesa, 4, __FUNCTION__);
	cmd[0].scratch.cmd_type = R300_CMD_SCRATCH;
	cmd[0].scratch.reg = 2;
	cmd[0].scratch.n_bufs = 1;
	cmd[0].scratch.flags = 0;
	cmd[1].u = (unsigned long)(&rmesa->rmm->vb_age);
	cmd[2].u = (unsigned long)(&rmesa->rmm->u_list[id].age);
	cmd[3].u = /*id*/0;
	
	LOCK_HARDWARE(&rmesa->radeon); /* Protect from DRM. */
	rmesa->rmm->u_list[id].h_pending ++;
	UNLOCK_HARDWARE(&rmesa->radeon);
			
	radeon_mm_unlock(rmesa, RADEON_MM_UL);
}

void *radeon_mm_map(r300ContextPtr rmesa, int id, int access)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, rmesa->radeon.radeonScreen->scratch[2]);
#endif	
	void *ptr;
	int tries = 0;
	
	if (access == RADEON_MM_R) {
		radeon_mm_lock(rmesa, RADEON_MM_UL);
		
		if(rmesa->rmm->u_list[id].mapped == 1)
			WARN_ONCE("buffer %d already mapped\n", id);
	
		rmesa->rmm->u_list[id].mapped = 1;
		ptr = radeon_mm_ptr(rmesa, id);
		
		radeon_mm_unlock(rmesa, RADEON_MM_UL);
		
		return ptr;
	}
	
	radeon_mm_lock(rmesa, RADEON_MM_UL);
	
	if (rmesa->rmm->u_list[id].h_pending)
		r300FlushCmdBuf(rmesa, __FUNCTION__);
	
	if (rmesa->rmm->u_list[id].h_pending) {
		radeon_mm_unlock(rmesa, RADEON_MM_UL);
		return NULL;
	}
	
	while(rmesa->rmm->u_list[id].age > rmesa->radeon.radeonScreen->scratch[2] && tries++ < 1000)
		usleep(10);
	
	if (tries >= 1000) {
		fprintf(stderr, "Idling failed (%x vs %x)\n",
				rmesa->rmm->u_list[id].age, rmesa->radeon.radeonScreen->scratch[2]);
		radeon_mm_unlock(rmesa, RADEON_MM_UL);
		return NULL;
	}
	
	if(rmesa->rmm->u_list[id].mapped == 1)
		WARN_ONCE("buffer %d already mapped\n", id);
	
	rmesa->rmm->u_list[id].mapped = 1;
	ptr = radeon_mm_ptr(rmesa, id);
	
	radeon_mm_unlock(rmesa, RADEON_MM_UL);
	
	return ptr;
}

void radeon_mm_unmap(r300ContextPtr rmesa, int id)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, rmesa->radeon.radeonScreen->scratch[2]);
#endif	
	
	radeon_mm_lock(rmesa, RADEON_MM_UL);
	
	if(rmesa->rmm->u_list[id].mapped == 0)
		WARN_ONCE("buffer %d not mapped\n", id);
	
	rmesa->rmm->u_list[id].mapped = 0;
	
	radeon_mm_unlock(rmesa, RADEON_MM_UL);
}

void radeon_mm_free(r300ContextPtr rmesa, int id)
{
#ifdef MM_DEBUG
	fprintf(stderr, "%s: %d at age %x\n", __FUNCTION__, id, rmesa->radeon.radeonScreen->scratch[2]);
#endif	
	
	if(id == 0)
		return;
	
	radeon_mm_lock(rmesa, RADEON_MM_UL);
	if(rmesa->rmm->u_list[id].ptr == NULL){
		radeon_mm_unlock(rmesa, RADEON_MM_UL);
		WARN_ONCE("Not allocated!\n");
		return ;
	}
	
	if(rmesa->rmm->u_list[id].pending){
		radeon_mm_unlock(rmesa, RADEON_MM_UL);
		WARN_ONCE("%p already pended!\n", rmesa->rmm->u_list[id].ptr);
		return ;
	}
			
	rmesa->rmm->u_list[id].pending = 1;
	radeon_mm_unlock(rmesa, RADEON_MM_UL);
}
#endif
