/**************************************************************************
 * 
 * Copyright 2009 VMware, Inc.
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

#include "sl_pp_context.h"
#include "sl_pp_dict.h"


#define ADD_NAME_STR(CTX, NAME, STR)\
   do {\
      (CTX)->dict.NAME = sl_pp_context_add_unique_str((CTX), (STR));\
      if ((CTX)->dict.NAME == -1) {\
         return -1;\
      }\
   } while (0)

#define ADD_NAME(CTX, NAME) ADD_NAME_STR(CTX, NAME, #NAME)


int
sl_pp_dict_init(struct sl_pp_context *context)
{
   ADD_NAME(context, all);
   ADD_NAME_STR(context, _GL_ARB_draw_buffers, "GL_ARB_draw_buffers");
   ADD_NAME_STR(context, _GL_ARB_texture_rectangle, "GL_ARB_texture_rectangle");

   ADD_NAME(context, require);
   ADD_NAME(context, enable);
   ADD_NAME(context, warn);
   ADD_NAME(context, disable);

   return 0;
}
