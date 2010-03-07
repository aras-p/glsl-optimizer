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

#include "sl_pp_public.h"
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

   ADD_NAME(context, require);
   ADD_NAME(context, enable);
   ADD_NAME(context, warn);
   ADD_NAME(context, disable);

   ADD_NAME(context, defined);

   ADD_NAME_STR(context, ___LINE__, "__LINE__");
   ADD_NAME_STR(context, ___FILE__, "__FILE__");
   ADD_NAME_STR(context, ___VERSION__, "__VERSION__");

   ADD_NAME(context, optimize);
   ADD_NAME(context, debug);

   ADD_NAME(context, off);
   ADD_NAME(context, on);

   ADD_NAME(context, define);
   ADD_NAME(context, elif);
   ADD_NAME_STR(context, _else, "else");
   ADD_NAME(context, endif);
   ADD_NAME(context, error);
   ADD_NAME(context, extension);
   ADD_NAME_STR(context, _if, "if");
   ADD_NAME(context, ifdef);
   ADD_NAME(context, ifndef);
   ADD_NAME(context, line);
   ADD_NAME(context, pragma);
   ADD_NAME(context, undef);

   ADD_NAME(context, version);

   ADD_NAME_STR(context, _0, "0");
   ADD_NAME_STR(context, _1, "1");

   return 0;
}
