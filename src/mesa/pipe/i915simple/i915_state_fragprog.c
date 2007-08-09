/**************************************************************************
 * 
 * Copyright 2003 Tungsten Graphics, Inc., Cedar Park, Texas.
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

#include "i915_reg.h"
#include "i915_context.h"
#include "i915_debug.h"


static unsigned passthrough[] = 
{
   _3DSTATE_PIXEL_SHADER_PROGRAM | ((2*3)-1),

   /* declare input color:
    */
   (D0_DCL | 
    (REG_TYPE_T << D0_TYPE_SHIFT) | 
    (T_DIFFUSE << D0_NR_SHIFT) | 
    D0_CHANNEL_ALL),
   0,
   0,

   /* move to output color:
    */
   (A0_MOV | 
    (REG_TYPE_OC << A0_DEST_TYPE_SHIFT) | 
    A0_DEST_CHANNEL_ALL | 
    (REG_TYPE_T << A0_SRC0_TYPE_SHIFT) |
    (T_DIFFUSE << A0_SRC0_NR_SHIFT)),
   0x01230000,			/* .xyzw */
   0
};

unsigned *i915_passthrough_program( unsigned *dwords )
{
   i915_disassemble_program( passthrough, Elements(passthrough) );

   *dwords = Elements(passthrough);
   return passthrough;
}




