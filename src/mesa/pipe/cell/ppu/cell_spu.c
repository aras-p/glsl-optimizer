/**************************************************************************
 * 
 * Copyright 2007 Tungsten Graphics, Inc., Cedar Park, Texas.
 * All Rights Reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sub license, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 * 
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
 * IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 * 
 **************************************************************************/


#include <cbe_mfc.h>

#include "cell_spu.h"
#include "pipe/p_format.h"
#include "pipe/p_state.h"
#include "pipe/cell/common.h"


/*
helpful headers:
/opt/ibm/cell-sdk/prototype/src/include/ppu/cbe_mfc.h
*/


/**
 * SPU/SPE handles, etc
 */
speid_t speid[MAX_SPUS];
spe_spu_control_area_t *control_ps_area[MAX_SPUS];


/**
 * Data sent to SPUs
 */
struct cell_init_info inits[MAX_SPUS] ALIGN16;
struct cell_command command[MAX_SPUS] ALIGN16;


/**
 * Write a 1-word message to the given SPE mailbox.
 */
void
send_mbox_message(spe_spu_control_area_t *ca, unsigned int msg)
{
   while (_spe_in_mbox_status(ca) < 1)
      ;
   _spe_in_mbox_write(ca, msg);
}


/**
 * Wait for a 1-word message to arrive in given mailbox.
 */
uint
wait_mbox_message(spe_spu_control_area_t *ca)
{
   uint k;
   while (_spe_out_mbox_status(ca) < 1)
      ;
   k = _spe_out_mbox_read(ca);
   return k;
}


/**
 * Create the SPU threads
 */
void
cell_start_spus(uint num_spus)
{
   uint i;

   assert((sizeof(struct cell_command) & 0xf) == 0);
   ASSERT_ALIGN16(&command[0]);
   ASSERT_ALIGN16(&command[1]);

   assert((sizeof(struct cell_init_info) & 0xf) == 0);
   ASSERT_ALIGN16(&inits[0]);
   ASSERT_ALIGN16(&inits[1]);

   /* XXX do we need to create a gid with spe_create_group()? */

   for (i = 0; i < num_spus; i++) {
      inits[i].id = i;
      inits[i].num_spus = num_spus;
      inits[i].cmd = &command[i];

      speid[i] = spe_create_thread(0,          /* gid */
                                   &g3d_spu,   /* spe program handle */
                                   &inits[i],  /* argp */
                                   NULL,       /* envp */
                                   -1,         /* mask */
                                   SPE_MAP_PS/*0*/ );        /* flags */

      control_ps_area[i] = spe_get_ps_area(speid[i], SPE_CONTROL_AREA);
      assert(control_ps_area[i]);
   }
}


/** wait for all SPUs to finish working */
/** XXX temporary */
void
finish_all(uint num_spus)
{
   uint i;

   for (i = 0; i < num_spus; i++) {
      send_mbox_message(control_ps_area[i], CELL_CMD_FINISH);
   }
   for (i = 0; i < num_spus; i++) {
      /* wait for mbox message */
      unsigned k;
      while (_spe_out_mbox_status(control_ps_area[i]) < 1)
         ;
      k = _spe_out_mbox_read(control_ps_area[i]);
      assert(k == CELL_CMD_FINISH);
   }
}


/**
 ** Send test commands (XXX temporary)
 **/
void
test_spus(struct cell_context *cell)
{
   uint i;
   struct pipe_surface *surf = cell->framebuffer.cbufs[0];

   printf("PPU: sleep(2)\n\n\n");
   sleep(2);

   for (i = 0; i < cell->num_spus; i++) {
      command[i].fb.start = surf->map;
      command[i].fb.width = surf->width;
      command[i].fb.height = surf->height;
      command[i].fb.format = PIPE_FORMAT_A8R8G8B8_UNORM;
      send_mbox_message(control_ps_area[i], CELL_CMD_FRAMEBUFFER);
   }

   for (i = 0; i < cell->num_spus; i++) {
      command[i].clear.value = 0xff880044; /* XXX */
      send_mbox_message(control_ps_area[i], CELL_CMD_CLEAR_TILES);
   }

   finish_all(cell->num_spus);

   {
      uint *b = (uint*) surf->map;
      printf("PPU: Clear results: 0x%x 0x%x 0x%x 0x%x\n",
             b[0], b[1000], b[2000], b[3000]);
   }

   for (i = 0; i < cell->num_spus; i++) {
      send_mbox_message(control_ps_area[i], CELL_CMD_INVERT_TILES);
   }

   finish_all(cell->num_spus);

   {
      uint *b = (uint*) surf->map;
      printf("PPU: Inverted results: 0x%x 0x%x 0x%x 0x%x\n",
             b[0], b[1000], b[2000], b[3000]);
   }



   for (i = 0; i < cell->num_spus; i++) {
      send_mbox_message(control_ps_area[i], CELL_CMD_EXIT);
   }
}


/**
 * Wait for all SPUs to exit/return.
 */
void
wait_spus(uint num_spus)
{
   int i, status;

   for (i = 0; i < num_spus; i++) {
      spe_wait( speid[i], &status, 1 );
   }
}


/**
 * Tell all the SPUs to stop/exit.
 */
void
cell_spu_exit(struct cell_context *cell)
{
   uint i;
   int status;

   for (i = 0; i < cell->num_spus; i++) {
      send_mbox_message(control_ps_area[i], CELL_CMD_EXIT);
   }

   for (i = 0; i < cell->num_spus; i++) {
      spe_wait( speid[i], &status, 1 );
   }
}
