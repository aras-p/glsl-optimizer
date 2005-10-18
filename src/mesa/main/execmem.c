#include "imports.h"
#include "glthread.h"



#ifdef __linux__

#include <unistd.h>
#include <sys/mman.h>
#include <mm.h>

#define EXEC_HEAP_SIZE (128*1024)

_glthread_DECLARE_STATIC_MUTEX(exec_mutex);

static memHeap_t *exec_heap = NULL;
static unsigned char *exec_mem = NULL;

static void init_heap( void )
{
   if (!exec_heap)
      exec_heap = mmInit( 0, EXEC_HEAP_SIZE );
   
   if (!exec_mem)
      exec_mem = (unsigned char *) mmap(0, EXEC_HEAP_SIZE, 
					PROT_EXEC | PROT_READ | PROT_WRITE, 
					MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
}


void *
_mesa_exec_malloc( GLuint size )
{
   PMemBlock block = NULL;
   void *addr = NULL;

   _glthread_LOCK_MUTEX(exec_mutex);

   init_heap();

   if (exec_heap) {
      size = (size + 31) & ~31;
      block = mmAllocMem( exec_heap, size, 32, 0 );
   }

   if (block)
      addr = exec_mem + block->ofs;

   _glthread_UNLOCK_MUTEX(exec_mutex);
   
   return addr;
}
 
void 
_mesa_exec_free(void *addr)
{
   _glthread_LOCK_MUTEX(exec_mutex);

   if (exec_heap) {
      PMemBlock block = mmFindBlock(exec_heap, (unsigned char *)addr - exec_mem);
   
      if (block)
	 mmFreeMem(block);
   }

   _glthread_UNLOCK_MUTEX(exec_mutex);
}

#else

void *
_mesa_exec_malloc( GLuint size )
{
   return _mesa_malloc( size );
}
 
void 
_mesa_exec_free(void *addr)
{
   _mesa_free(addr);
}

#endif
