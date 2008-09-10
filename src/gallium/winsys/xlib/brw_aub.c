/*
 Copyright (C) Intel Corp.  2006.  All Rights Reserved.
 Intel funded Tungsten Graphics (http://www.tungstengraphics.com) to
 develop this 3D driver.
 
 Permission is hereby granted, free of charge, to any person obtaining
 a copy of this software and associated documentation files (the
 "Software"), to deal in the Software without restriction, including
 without limitation the rights to use, copy, modify, merge, publish,
 distribute, sublicense, and/or sell copies of the Software, and to
 permit persons to whom the Software is furnished to do so, subject to
 the following conditions:
 
 The above copyright notice and this permission notice (including the
 next paragraph) shall be included in all copies or substantial
 portions of the Software.
 
 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
 IN NO EVENT SHALL THE COPYRIGHT OWNER(S) AND/OR ITS SUPPLIERS BE
 LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 
 **********************************************************************/
 /*
  * Authors:
  *   Keith Whitwell <keith@tungstengraphics.com>
  */

#include <stdio.h>
#include <stdlib.h>
#include "brw_aub.h"
#include "pipe/p_context.h"
#include "pipe/p_state.h"
#include "pipe/p_debug.h"
#include "util/u_memory.h"


struct brw_aubfile {
   FILE *file;
   unsigned next_free_page;
};


extern char *__progname;


struct aub_file_header {
   unsigned int instruction_type;
   unsigned int pad0:16;
   unsigned int minor:8;
   unsigned int major:8;
   unsigned char application[8*4];
   unsigned int day:8;
   unsigned int month:8;
   unsigned int year:16;
   unsigned int timezone:8;
   unsigned int second:8;
   unsigned int minute:8;
   unsigned int hour:8;
   unsigned int comment_length:16;   
   unsigned int pad1:16;
};

struct aub_block_header {
   unsigned int instruction_type;
   unsigned int operation:8;
   unsigned int type:8;
   unsigned int address_space:8;
   unsigned int pad0:8;
   unsigned int general_state_type:8;
   unsigned int surface_state_type:8;
   unsigned int pad1:16;
   unsigned int address;
   unsigned int length;
};

struct aub_dump_bmp {
   unsigned int instruction_type;
   unsigned int xmin:16;
   unsigned int ymin:16;
   unsigned int pitch:16;
   unsigned int bpp:8;
   unsigned int format:8;
   unsigned int xsize:16;
   unsigned int ysize:16;
   unsigned int addr;
   unsigned int unknown;
};

enum bh_operation {
   BH_COMMENT,
   BH_DATA_WRITE,
   BH_COMMAND_WRITE,
   BH_MMI0_WRITE32,
   BH_END_SCENE,
   BH_CONFIG_MEMORY_MAP,
   BH_MAX_OPERATION
};

enum command_write_type {
   CW_HWB_RING = 1,
   CW_PRIMARY_RING_A,
   CW_PRIMARY_RING_B,		/* XXX - disagreement with listaub! */
   CW_PRIMARY_RING_C,
   CW_MAX_TYPE
};

enum memory_map_type {
   MM_DEFAULT,
   MM_DYNAMIC,
   MM_MAX_TYPE
};

enum address_space {
   ADDR_GTT,
   ADDR_LOCAL,
   ADDR_MAIN,
   ADDR_MAX
};


#define AUB_FILE_HEADER 0xe085000b
#define AUB_BLOCK_HEADER 0xe0c10003
#define AUB_DUMP_BMP 0xe09e0004

/* Registers to control page table
 */
#define PGETBL_CTL       0x2020
#define PGETBL_ENABLED   0x1

#define NR_GTT_ENTRIES  65536	/* 256 mb */

#define FAIL										\
do {											\
   fprintf(stderr, "failed to write aub data at %s/%d\n", __FUNCTION__, __LINE__);	\
   exit(1);										\
} while (0)


/* Emit the headers at the top of each aubfile.  Initialize the GTT.
 */
static void init_aubfile( FILE *aub_file )
{   
   struct aub_file_header fh;
   struct aub_block_header bh;
   unsigned int data;

   static int nr;
   
   nr++;

   /* Emit the aub header:
    */
   memset(&fh, 0, sizeof(fh));

   fh.instruction_type = AUB_FILE_HEADER;
   fh.minor = 0x0;
   fh.major = 0x7;
   memcpy(fh.application, __progname, sizeof(fh.application));
   fh.day = (nr>>24) & 0xff;
   fh.month = 0x0;
   fh.year = 0x0;
   fh.timezone = 0x0;
   fh.second = nr & 0xff;
   fh.minute = (nr>>8) & 0xff;
   fh.hour = (nr>>16) & 0xff;
   fh.comment_length = 0x0;   

   if (fwrite(&fh, sizeof(fh), 1, aub_file) < 0) 
      FAIL;
         
   /* Setup the GTT starting at main memory address zero (!):
    */
   memset(&bh, 0, sizeof(bh));
   
   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_MMI0_WRITE32;
   bh.type = 0x0;
   bh.address_space = ADDR_GTT;	/* ??? */
   bh.general_state_type = 0x0;
   bh.surface_state_type = 0x0;
   bh.address = PGETBL_CTL;
   bh.length = 0x4;

   if (fwrite(&bh, sizeof(bh), 1, aub_file) < 0) 
      FAIL;

   data = 0x0 | PGETBL_ENABLED;

   if (fwrite(&data, sizeof(data), 1, aub_file) < 0) 
      FAIL;
}


static void init_aub_gtt( struct brw_aubfile *aubfile,
			  unsigned start_offset, 
			  unsigned size )
{
   FILE *aub_file = aubfile->file;
   struct aub_block_header bh;
   unsigned int i;

   assert(start_offset + size < NR_GTT_ENTRIES * 4096);


   memset(&bh, 0, sizeof(bh));
   
   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_DATA_WRITE;
   bh.type = 0x0;
   bh.address_space = ADDR_MAIN;
   bh.general_state_type = 0x0;
   bh.surface_state_type = 0x0;
   bh.address =  start_offset / 4096 * 4;
   bh.length = size / 4096 * 4;

   if (fwrite(&bh, sizeof(bh), 1, aub_file) < 0) 
      FAIL;

   for (i = 0; i < size / 4096; i++) {
      unsigned data = aubfile->next_free_page | 1;

      aubfile->next_free_page += 4096;

      if (fwrite(&data, sizeof(data), 1, aub_file) < 0) 
	 FAIL;
   }

}

static void write_block_header( FILE *aub_file,
				struct aub_block_header *bh,
				const unsigned *data,
				unsigned sz )
{
   sz = (sz + 3) & ~3;

   if (fwrite(bh, sizeof(*bh), 1, aub_file) < 0) 
      FAIL;

   if (fwrite(data, sz, 1, aub_file) < 0) 
      FAIL;

   fflush(aub_file);
}


static void write_dump_bmp( FILE *aub_file,
			    struct aub_dump_bmp *db )
{
   if (fwrite(db, sizeof(*db), 1, aub_file) < 0) 
      FAIL;

   fflush(aub_file);
}



void brw_aub_gtt_data( struct brw_aubfile *aubfile,
		       unsigned offset,
		       const void *data,
		       unsigned sz,
		       unsigned type,
		       unsigned state_type )
{
   struct aub_block_header bh;

   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_DATA_WRITE;
   bh.type = type;
   bh.address_space = ADDR_GTT;
   bh.pad0 = 0;

   if (type == DW_GENERAL_STATE) {
      bh.general_state_type = state_type;
      bh.surface_state_type = 0;
   }
   else {
      bh.general_state_type = 0;
      bh.surface_state_type = state_type;
   }

   bh.pad1 = 0;
   bh.address = offset;
   bh.length = sz;

   write_block_header(aubfile->file, &bh, data, sz);
}



void brw_aub_gtt_cmds( struct brw_aubfile *aubfile,
		       unsigned offset,
		       const void *data,
		       unsigned sz )
{
   struct aub_block_header bh;   
   unsigned type = CW_PRIMARY_RING_A;
   

   bh.instruction_type = AUB_BLOCK_HEADER;
   bh.operation = BH_COMMAND_WRITE;
   bh.type = type;
   bh.address_space = ADDR_GTT;
   bh.pad0 = 0;
   bh.general_state_type = 0;
   bh.surface_state_type = 0;
   bh.pad1 = 0;
   bh.address = offset;
   bh.length = sz;

   write_block_header(aubfile->file, &bh, data, sz);
}

void brw_aub_dump_bmp( struct brw_aubfile *aubfile,
		       struct pipe_surface *surface,
		       unsigned gtt_offset )
{
   struct aub_dump_bmp db;
   unsigned format;

   assert(surface->block.width == 1);
   assert(surface->block.height == 1);
   
   if (surface->block.size == 4)
      format = 0x7;
   else
      format = 0x3;

   db.instruction_type = AUB_DUMP_BMP;
   db.xmin = 0;
   db.ymin = 0;
   db.format = format;
   db.bpp = surface->block.size * 8;
   db.pitch = surface->stride/surface->block.size;
   db.xsize = surface->width;
   db.ysize = surface->height;
   db.addr = gtt_offset;
   db.unknown = /* surface->tiled ? 0x4 : */ 0x0;

   write_dump_bmp(aubfile->file, &db);
}



struct brw_aubfile *brw_aubfile_create( void )
{
   struct brw_aubfile *aubfile = CALLOC_STRUCT(brw_aubfile);
   char filename[80];
   int val;
   static int i = 0;

   i++;

   if (getenv("INTEL_AUBFILE")) {
      val = snprintf(filename, sizeof(filename), "%s%d.aub", getenv("INTEL_AUBFILE"), i%4);
      debug_printf("--> Aub file: %s\n", filename);
      aubfile->file = fopen(filename, "w");
   }
   else {
      val = snprintf(filename, sizeof(filename), "%s.aub", __progname);
      if (val < 0 || val > sizeof(filename)) 
	 strcpy(filename, "default.aub");   
   
      debug_printf("--> Aub file: %s\n", filename);
      aubfile->file = fopen(filename, "w");
   }

   if (!aubfile->file) {
      debug_printf("couldn't open aubfile\n");
      exit(1);
   }

   init_aubfile(aubfile->file);

   /* The GTT is located starting address zero in main memory.  Pages
    * to populate the gtt start after this point.
    */
   aubfile->next_free_page = (NR_GTT_ENTRIES * 4 + 4095) & ~4095;

   /* More or less correspond with all the agp regions mapped by the
    * driver:
    */
   init_aub_gtt(aubfile, 0, 4096*4);
   init_aub_gtt(aubfile, AUB_BUF_START, AUB_BUF_SIZE);

   return aubfile;
}

void brw_aub_destroy( struct brw_aubfile *aubfile )
{
   fclose(aubfile->file);
   FREE(aubfile);
}
