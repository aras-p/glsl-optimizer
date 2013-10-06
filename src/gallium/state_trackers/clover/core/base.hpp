//
// Copyright 2012 Francisco Jerez
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

#ifndef CLOVER_CORE_BASE_HPP
#define CLOVER_CORE_BASE_HPP

#include <stdexcept>
#include <atomic>
#include <cassert>
#include <tuple>
#include <vector>
#include <functional>

#include "CL/cl.h"
#include "util/pointer.hpp"

///
/// Main namespace of the CL state tracker.
///
namespace clover {
   ///
   /// Class that represents an error that can be converted to an
   /// OpenCL status code.
   ///
   class error : public std::runtime_error {
   public:
      error(cl_int code, std::string what = "") :
         std::runtime_error(what), code(code) {
      }

      cl_int get() const {
         return code;
      }

   protected:
      cl_int code;
   };
}

#endif
