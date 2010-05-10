/**************************************************************************
 *
 * Copyright 2010 VMware, Inc.
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
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT. IN NO EVENT SHALL
 * THE COPYRIGHT HOLDERS, AUTHORS AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE
 * USE OR OTHER DEALINGS IN THE SOFTWARE.
 *
 * The above copyright notice and this permission notice (including the
 * next paragraph) shall be included in all copies or substantial portions
 * of the Software.
 *
 **************************************************************************/


#ifndef __STDC_LIMIT_MACROS
#define __STDC_LIMIT_MACROS
#endif

#ifndef __STDC_CONSTANT_MACROS
#define __STDC_CONSTANT_MACROS
#endif

#include "llvm-c/Core.h"

#include "util/u_debug.h"


#if (defined(PIPE_OS_WINDOWS) && !defined(PIPE_CC_MSVC)) || defined(PIPE_OS_EMBDDED)

#include "llvm/Support/raw_ostream.h"

class raw_debug_ostream :
   public llvm::raw_ostream
{
   uint64_t pos;

   void write_impl(const char *Ptr, size_t Size);
   uint64_t current_pos() { return pos; }
   uint64_t current_pos() const { return pos; }

#if HAVE_LLVM >= 0x207
   uint64_t preferred_buffer_size() { return 512; }
#else
   size_t preferred_buffer_size() { return 512; }
#endif
};


void
raw_debug_ostream::write_impl(const char *Ptr, size_t Size)
{
   if (Size > 0) {
      char *lastPtr = (char *)&Ptr[Size];
      char last = *lastPtr;
      *lastPtr = 0;
      _debug_printf("%*s", Size, Ptr);
      *lastPtr = last;
      pos += Size;
   }
}


/**
 * Same as LLVMDumpValue, but through our debugging channels.
 */
extern "C" void
lp_debug_dump_value(LLVMValueRef value)
{
   raw_debug_ostream os;
   llvm::unwrap(value)->print(os);
   os.flush();
}


#else


extern "C" void
lp_debug_dump_value(LLVMValueRef value)
{
   LLVMDumpValue(value);
}


#endif
