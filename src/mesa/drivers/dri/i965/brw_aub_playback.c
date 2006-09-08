
#include <stdio.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

#include "brw_aub.h"
#include "brw_context.h"
#include "intel_ioctl.h"
#include "bufmgr.h"

struct aub_state {
   struct intel_context *intel;
   const char *map;
   unsigned int csr;
   unsigned int sz;
};


static int gobble( struct aub_state *s, int size )
{
   if (s->csr + size > s->sz) {
      DBG("EOF in %s\n", __FUNCTION__);
      return 1;
   }

   s->csr += size;
   return 0;
}

/* In order to work, the memory layout has to be the same as the X
 * server which created the aubfile.
 */
static int parse_block_header( struct aub_state *s )
{
   struct aub_block_header *bh = (struct aub_block_header *)(s->map + s->csr);
   void *data = (void *)(bh + 1);
   unsigned int len = (bh->length + 3) & ~3;

   DBG("block header at 0x%x\n", s->csr);

   if (s->csr + len + sizeof(*bh) > s->sz) {
      DBG("EOF in data in %s\n", __FUNCTION__);
      return 1;
   }

   if (bh->address_space == ADDR_GTT) {

      switch (bh->operation)
      {
      case BH_DATA_WRITE: {
	 void *dest = bmFindVirtual( s->intel, bh->address, len );
	 if (dest == NULL) {
	    _mesa_printf("Couldn't find virtual address for offset %x\n", bh->address);
	    return 1;
	 }

	 /* Just copy the data to the indicated place in agp memory:
	  */
	 memcpy(dest, data, len);
	 break;
      }
      case BH_COMMAND_WRITE:
	 /* For ring data, just send off immediately via an ioctl.
	  * This differs slightly from how the stream was executed
	  * initially as this would have been a batchbuffer.
	  */
	 intel_cmd_ioctl(s->intel, data, len, GL_TRUE);
	 break;
      default:
	 break;
      }
   }

   s->csr += sizeof(*bh) + len;
   return 0;
}


#define AUB_FILE_HEADER 0xe085000b
#define AUB_BLOCK_HEADER 0xe0c10003
#define AUB_DUMP_BMP 0xe09e0004

int brw_playback_aubfile(struct brw_context *brw,
			 const char *filename)
{
   struct intel_context *intel = &brw->intel;
   struct aub_state state;
   struct stat sb;
   int fd;
   int retval = 0;

   state.intel = intel;

   fd = open(filename, O_RDONLY, 0);
   if (fd < 0) {
      DBG("couldn't open aubfile: %s\n", filename);
      return 1;
   }

   if (fstat(fd, &sb) != 0) {
      DBG("couldn't open %s\n", filename);
      return 1;
   }

   state.csr = 0;
   state.sz = sb.st_size;
   state.map = mmap(NULL, sb.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
   
   if (state.map == NULL) {
      DBG("couldn't mmap %s\n", filename);
      return 1;
   }

   LOCK_HARDWARE(intel); 
   {
      /* Make sure we don't confuse anything that might happen to be
       * going on with the hardware:
       */
/*       bmEvictAll(intel); */
/*       intel->vtbl.lost_hardware(intel); */
      

      /* Replay the aubfile item by item: 
       */
      while (retval == 0 && 
	     state.csr != state.sz) {
	 unsigned int insn = *(unsigned int *)(state.map + state.csr);

	 switch (insn) {
	 case AUB_FILE_HEADER:
	    retval = gobble(&state, sizeof(struct aub_file_header));
	    break;
	 
	 case AUB_BLOCK_HEADER:   
	    retval = parse_block_header(&state);
	    break;
	 
	 case AUB_DUMP_BMP:
	    retval = gobble(&state, sizeof(struct aub_dump_bmp));
	    break;
	 
	 default:
	    DBG("unknown instruction %x\n", insn);
	    retval = 1;
	    break;
	 }
      }
   }
   UNLOCK_HARDWARE(intel);
   return retval;
}






		  
