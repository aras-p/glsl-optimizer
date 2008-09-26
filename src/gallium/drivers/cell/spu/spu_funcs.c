/**************************************************************************
 * 
 * Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
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


/**
 * SPU functions accessed by shaders.
 *
 * Authors: Brian Paul
 */


#include <string.h>
#include <libmisc.h>
#include <cos8_v.h>
#include <sin8_v.h>

#include "cell/common.h"
#include "spu_main.h"
#include "spu_funcs.h"


#define M_PI 3.1415926


static vector float
spu_cos(vector float x)
{
#if 0
   static const float scale = 1.0 / (2.0 * M_PI);
   x = x * spu_splats(scale); /* normalize */
   return _cos8_v(x);
#else
   /* just pass-through to avoid trashing caller's stack */
   return x;
#endif
}

static vector float
spu_sin(vector float x)
{
#if 0
   static const float scale = 1.0 / (2.0 * M_PI);
   x = x * spu_splats(scale); /* normalize */
   return _sin8_v(x);   /* 8-bit accuracy enough?? */
#else
   /* just pass-through to avoid trashing caller's stack */
   return x;
#endif
}


static void
add_func(struct cell_spu_function_info *spu_functions,
             const char *name, void *addr)
{
   uint n = spu_functions->num;
   ASSERT(strlen(name) < 16);
   strcpy(spu_functions->names[n], name);
   spu_functions->addrs[n] = (uint) addr;
   spu_functions->num++;
}


/**
 * Return info about the SPU's function to the PPU / main memory.
 * The PPU needs to know the address of some SPU-side functions so
 * that we can generate shader code with function calls.
 */
void
return_function_info(void)
{
   struct cell_spu_function_info funcs ALIGN16_ATTRIB;
   int tag = TAG_MISC;

   ASSERT(sizeof(funcs) == 256); /* must be multiple of 16 bytes */

   funcs.num = 0;
   add_func(&funcs, "spu_cos", &spu_cos);
   add_func(&funcs, "spu_sin", &spu_sin);

   /* Send the function info back to the PPU / main memory */
   mfc_put((void *) &funcs,  /* src in local store */
           (unsigned int) spu.init.spu_functions, /* dst in main memory */
           sizeof(funcs),  /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << tag);
}



