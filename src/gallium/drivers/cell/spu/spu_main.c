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


/* main() for Cell SPU code */


#include <stdio.h>
#include <libmisc.h>

#include "pipe/p_defines.h"

#include "spu_funcs.h"
#include "spu_command.h"
#include "spu_main.h"
#include "spu_per_fragment_op.h"
#include "spu_texture.h"
//#include "spu_test.h"
#include "cell/common.h"


/*
helpful headers:
/usr/lib/gcc/spu/4.1.1/include/spu_mfcio.h
/opt/cell/sdk/usr/include/libmisc.h
*/

struct spu_global spu;


static void
one_time_init(void)
{
   memset(spu.ctile_status, TILE_STATUS_DEFINED, sizeof(spu.ctile_status));
   memset(spu.ztile_status, TILE_STATUS_DEFINED, sizeof(spu.ztile_status));
   invalidate_tex_cache();
}

/* In some versions of the SDK the SPE main takes 'unsigned long' as a
 * parameter.  In others it takes 'unsigned long long'.  Use a define to
 * select between the two.
 */
#ifdef SPU_MAIN_PARAM_LONG_LONG
typedef unsigned long long main_param_t;
#else
typedef unsigned long main_param_t;
#endif

/**
 * SPE entrypoint.
 */
int
main(main_param_t speid, main_param_t argp)
{
   int tag = 0;

   (void) speid;

   ASSERT(sizeof(tile_t) == TILE_SIZE * TILE_SIZE * 4);
   ASSERT(sizeof(struct cell_command_render) % 8 == 0);
   ASSERT(sizeof(struct cell_command_fragment_ops) % 8 == 0);
   ASSERT(((unsigned long) &spu.fragment_program_code) % 8 == 0);

   one_time_init();
   spu_command_init();

   D_PRINTF(CELL_DEBUG_CMD, "main() speid=%lu\n", (unsigned long) speid);
   D_PRINTF(CELL_DEBUG_FRAGMENT_OP_FALLBACK, "using fragment op fallback\n");

   /* get initialization data */
   mfc_get(&spu.init,  /* dest */
           (unsigned int) argp, /* src */
           sizeof(struct cell_init_info), /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask( 1 << tag );

   if (spu.init.id == 0) {
      return_function_info();
   }

#if 0
   if (spu.init.id==0)
      spu_test_misc(spu.init.id);
#endif

   command_loop();

   spu_command_close();

   return 0;
}
