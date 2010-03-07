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
#include <math.h>
#include <cos14_v.h>
#include <sin14_v.h>
#include <simdmath/exp2f4.h>
#include <simdmath/log2f4.h>
#include <simdmath/powf4.h>

#include "cell/common.h"
#include "spu_main.h"
#include "spu_funcs.h"
#include "spu_texture.h"


/** For "return"-ing four vectors */
struct vec_4x4
{
   vector float v[4];
};


static vector float
spu_cos(vector float x)
{
   return _cos14_v(x);
}

static vector float
spu_sin(vector float x)
{
   return _sin14_v(x);
}

static vector float
spu_pow(vector float x, vector float y)
{
   return _powf4(x, y);
}

static vector float
spu_exp2(vector float x)
{
   return _exp2f4(x);
}

static vector float
spu_log2(vector float x)
{
   return _log2f4(x);
}


static struct vec_4x4
spu_tex_2d(vector float s, vector float t, vector float r, vector float q,
           unsigned unit)
{
   struct vec_4x4 colors;
   (void) r;
   (void) q;
   spu.sample_texture_2d[unit](s, t, unit, 0, 0, colors.v);
   return colors;
}

static struct vec_4x4
spu_tex_3d(vector float s, vector float t, vector float r, vector float q,
           unsigned unit)
{
   struct vec_4x4 colors;
   (void) r;
   (void) q;
   spu.sample_texture_2d[unit](s, t, unit, 0, 0, colors.v);
   return colors;
}

static struct vec_4x4
spu_tex_cube(vector float s, vector float t, vector float r, vector float q,
           unsigned unit)
{
   struct vec_4x4 colors;
   (void) q;
   sample_texture_cube(s, t, r, unit, colors.v);
   return colors;
}


/**
 * Add named function to list of "exported" functions that will be
 * made available to the PPU-hosted code generator.
 */
static void
export_func(struct cell_spu_function_info *spu_functions,
            const char *name, void *addr)
{
   uint n = spu_functions->num;
   ASSERT(strlen(name) < 16);
   strcpy(spu_functions->names[n], name);
   spu_functions->addrs[n] = (uint) addr;
   spu_functions->num++;
   ASSERT(spu_functions->num <= 16);
}


/**
 * Return info about the SPU's function to the PPU / main memory.
 * The PPU needs to know the address of some SPU-side functions so
 * that we can generate shader code with function calls.
 */
void
return_function_info(void)
{
   PIPE_ALIGN_VAR(16) struct cell_spu_function_info funcs;
   int tag = TAG_MISC;

   ASSERT(sizeof(funcs) == 256); /* must be multiple of 16 bytes */

   funcs.num = 0;
   export_func(&funcs, "spu_cos", &spu_cos);
   export_func(&funcs, "spu_sin", &spu_sin);
   export_func(&funcs, "spu_pow", &spu_pow);
   export_func(&funcs, "spu_exp2", &spu_exp2);
   export_func(&funcs, "spu_log2", &spu_log2);
   export_func(&funcs, "spu_tex_2d", &spu_tex_2d);
   export_func(&funcs, "spu_tex_3d", &spu_tex_3d);
   export_func(&funcs, "spu_tex_cube", &spu_tex_cube);

   /* Send the function info back to the PPU / main memory */
   mfc_put((void *) &funcs,  /* src in local store */
           (unsigned int) spu.init.spu_functions, /* dst in main memory */
           sizeof(funcs),  /* bytes */
           tag,
           0, /* tid */
           0  /* rid */);
   wait_on_mask(1 << tag);
}



