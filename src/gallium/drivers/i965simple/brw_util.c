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


#include "brw_util.h"
#include "brw_defines.h"

#include "pipe/p_defines.h"

unsigned brw_count_bits( unsigned val )
{
   unsigned i;
   for (i = 0; val ; val >>= 1)
      if (val & 1)
	 i++;
   return i;
}


unsigned brw_translate_blend_equation( int mode )
{
   switch (mode) {
   case PIPE_BLEND_ADD:
      return BRW_BLENDFUNCTION_ADD;
   case PIPE_BLEND_MIN:
      return BRW_BLENDFUNCTION_MIN;
   case PIPE_BLEND_MAX:
      return BRW_BLENDFUNCTION_MAX;
   case PIPE_BLEND_SUBTRACT:
      return BRW_BLENDFUNCTION_SUBTRACT;
   case PIPE_BLEND_REVERSE_SUBTRACT:
      return BRW_BLENDFUNCTION_REVERSE_SUBTRACT;
   default:
      assert(0);
      return BRW_BLENDFUNCTION_ADD;
   }
}

unsigned brw_translate_blend_factor( int factor )
{
   switch(factor) {
   case PIPE_BLENDFACTOR_ZERO:
      return BRW_BLENDFACTOR_ZERO;
   case PIPE_BLENDFACTOR_SRC_ALPHA:
      return BRW_BLENDFACTOR_SRC_ALPHA;
   case PIPE_BLENDFACTOR_ONE:
      return BRW_BLENDFACTOR_ONE;
   case PIPE_BLENDFACTOR_SRC_COLOR:
      return BRW_BLENDFACTOR_SRC_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC_COLOR:
      return BRW_BLENDFACTOR_INV_SRC_COLOR;
   case PIPE_BLENDFACTOR_DST_COLOR:
      return BRW_BLENDFACTOR_DST_COLOR;
   case PIPE_BLENDFACTOR_INV_DST_COLOR:
      return BRW_BLENDFACTOR_INV_DST_COLOR;
   case PIPE_BLENDFACTOR_INV_SRC_ALPHA:
      return BRW_BLENDFACTOR_INV_SRC_ALPHA;
   case PIPE_BLENDFACTOR_DST_ALPHA:
      return BRW_BLENDFACTOR_DST_ALPHA;
   case PIPE_BLENDFACTOR_INV_DST_ALPHA:
      return BRW_BLENDFACTOR_INV_DST_ALPHA;
   case PIPE_BLENDFACTOR_SRC_ALPHA_SATURATE:
      return BRW_BLENDFACTOR_SRC_ALPHA_SATURATE;
   case PIPE_BLENDFACTOR_CONST_COLOR:
      return BRW_BLENDFACTOR_CONST_COLOR;
   case PIPE_BLENDFACTOR_INV_CONST_COLOR:
      return BRW_BLENDFACTOR_INV_CONST_COLOR;
   case PIPE_BLENDFACTOR_CONST_ALPHA:
      return BRW_BLENDFACTOR_CONST_ALPHA;
   case PIPE_BLENDFACTOR_INV_CONST_ALPHA:
      return BRW_BLENDFACTOR_INV_CONST_ALPHA;
   default:
      assert(0);
      return BRW_BLENDFACTOR_ZERO;
   }
}
