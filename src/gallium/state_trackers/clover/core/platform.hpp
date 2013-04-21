//
// Copyright 2013 Francisco Jerez
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the "Software"),
// to deal in the Software without restriction, including without limitation
// the rights to use, copy, modify, merge, publish, distribute, sublicense,
// and/or sell copies of the Software, and to permit persons to whom the
// Software is furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
// THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//

#ifndef __CORE_PLATFORM_HPP__
#define __CORE_PLATFORM_HPP__

#include <vector>

#include "core/base.hpp"
#include "core/device.hpp"

namespace clover {
   typedef struct _cl_platform_id platform;
}

struct _cl_platform_id {
public:
   typedef std::vector<clover::device>::iterator iterator;

   _cl_platform_id();

   ///
   /// Container of all compute devices that are available in the platform.
   ///
   /// @{
   iterator begin() {
      return devs.begin();
   }

   iterator end() {
      return devs.end();
   }

   clover::device &front() {
      return devs.front();
   }

   clover::device &back() {
      return devs.back();
   }
   /// @}

protected:
   std::vector<clover::device> devs;
};

#endif
