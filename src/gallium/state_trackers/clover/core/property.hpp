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

#ifndef CLOVER_CORE_PROPERTY_HPP
#define CLOVER_CORE_PROPERTY_HPP

#include <map>

#include "util/range.hpp"
#include "util/algorithm.hpp"

namespace clover {
   class property_buffer;

   namespace detail {
      template<typename T>
      class property_scalar {
      public:
         property_scalar(property_buffer &buf) : buf(buf) {
         }

         inline property_scalar &
         operator=(const T &x);

      private:
         property_buffer &buf;
      };

      template<typename T>
      class property_vector {
      public:
         property_vector(property_buffer &buf) : buf(buf) {
         }

         template<typename S>
         inline property_vector &
         operator=(const S &v);

      private:
         property_buffer &buf;
      };

      template<typename T>
      class property_matrix {
      public:
         property_matrix(property_buffer &buf) : buf(buf) {
         }

         template<typename S>
         inline property_matrix &
         operator=(const S &v);

      private:
         property_buffer &buf;
      };

      class property_string {
      public:
         property_string(property_buffer &buf) : buf(buf) {
         }

         inline property_string &
         operator=(const std::string &v);

      private:
         property_buffer &buf;
      };
   };

   ///
   /// Return value buffer used by the CL property query functions.
   ///
   class property_buffer {
   public:
      property_buffer(void *r_buf, size_t size, size_t *r_size) :
         r_buf(r_buf), size(size), r_size(r_size) {
      }

      template<typename T>
      detail::property_scalar<T>
      as_scalar() {
         return { *this };
      }

      template<typename T>
      detail::property_vector<T>
      as_vector() {
         return { *this };
      }

      template<typename T>
      detail::property_matrix<T>
      as_matrix() {
         return { *this };
      }

      detail::property_string
      as_string() {
         return { *this };
      }

      template<typename T>
      iterator_range<T *>
      allocate(size_t n) {
         if (r_buf && size < n * sizeof(T))
            throw error(CL_INVALID_VALUE);

         if (r_size)
            *r_size = n * sizeof(T);

         if (r_buf)
            return range((T *)r_buf, n);
         else
            return { };
      }

   private:
      void *const r_buf;
      const size_t size;
      size_t *const r_size;
   };

   namespace detail {
      template<typename T>
      inline property_scalar<T> &
      property_scalar<T>::operator=(const T &x) {
         auto r = buf.allocate<T>(1);

         if (!r.empty())
            r.front() = x;

         return *this;
      }

      template<typename T>
      template<typename S>
      inline property_vector<T> &
      property_vector<T>::operator=(const S &v) {
         auto r = buf.allocate<T>(v.size());

         if (!r.empty())
            copy(v, r.begin());

         return *this;
      }

      template<typename T>
      template<typename S>
      inline property_matrix<T> &
      property_matrix<T>::operator=(const S &v) {
         auto r = buf.allocate<T *>(v.size());

         if (!r.empty())
            for_each([](typename S::value_type src, T *dst) {
                  if (dst)
                     copy(src, dst);
               }, v, r);

         return *this;
      }

      inline property_string &
      property_string::operator=(const std::string &v) {
         auto r = buf.allocate<char>(v.size() + 1);

         if (!r.empty())
            copy(range(v.begin(), r.size()), r.begin());

         return *this;
      }
   };
}

#endif
