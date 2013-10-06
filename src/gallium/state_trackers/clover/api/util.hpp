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

#ifndef CLOVER_API_UTIL_HPP
#define CLOVER_API_UTIL_HPP

#include <cstdint>
#include <cstring>
#include <algorithm>
#include <map>

#include "core/base.hpp"
#include "core/property.hpp"
#include "util/algorithm.hpp"
#include "pipe/p_compiler.h"

namespace clover {
   ///
   /// Convert a NULL-terminated property list into an std::map.
   ///
   template<typename T>
   std::map<T, T>
   property_map(const T *props) {
      std::map<T, T> m;

      while (props && *props) {
         T key = *props++;
         T value = *props++;

         if (m.count(key))
            throw clover::error(CL_INVALID_PROPERTY);

         m.insert({ key, value });
      }

      return m;
   }

   ///
   /// Convert an std::map into a NULL-terminated property list.
   ///
   template<typename T>
   std::vector<T>
   property_vector(const std::map<T, T> &m) {
      std::vector<T> v;

      for (auto &p : m) {
         v.push_back(p.first);
         v.push_back(p.second);
      }

      v.push_back(0);
      return v;
   }

   ///
   /// Return an error code in \a p if non-zero.
   ///
   inline void
   ret_error(cl_int *p, const clover::error &e) {
      if (p)
         *p = e.get();
   }

   ///
   /// Return a reference-counted object in \a p if non-zero.
   /// Otherwise release object ownership.
   ///
   template<typename T, typename S>
   void
   ret_object(T p, S v) {
      if (p)
         *p = v;
      else
         v->release();
   }
}

#endif
