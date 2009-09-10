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
 * IN NO EVENT SHALL VMWARE AND/OR ITS SUPPLIERS BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
 * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
 * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 **************************************************************************/


#ifndef LP_BLD_DEBUG_H
#define LP_BLD_DEBUG_H


#include <llvm-c/Core.h>

#include "pipe/p_compiler.h"
#include "util/u_string.h"


static INLINE void
lp_build_name(LLVMValueRef val, const char *format, ...)
{
#ifdef DEBUG
   char name[32];
   va_list ap;
   va_start(ap, format);
   util_vsnprintf(name, sizeof name, format, ap);
   va_end(ap);
   LLVMSetValueName(val, name);
#else
   (void)val;
   (void)format;
#endif
}


boolean
lp_check_alignment(const void *ptr, unsigned alignment);


void
lp_disassemble(const void* func);


#endif /* !LP_BLD_DEBUG_H */
