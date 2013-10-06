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

#ifndef CLOVER_UTIL_COMPAT_HPP
#define CLOVER_UTIL_COMPAT_HPP

#include <new>
#include <cstring>
#include <cstdlib>
#include <string>
#include <stdint.h>

namespace clover {
   namespace compat {
      // XXX - For cases where we can't rely on STL...  I.e. the
      //       interface between code compiled as C++98 and C++11
      //       source.  Get rid of this as soon as everything can be
      //       compiled as C++11.

      template<typename T>
      class vector {
      protected:
         static T *
         alloc(int n, const T *q, int m) {
            T *p = reinterpret_cast<T *>(std::malloc(n * sizeof(T)));

            for (int i = 0; i < m; ++i)
               new(&p[i]) T(q[i]);

            return p;
         }

         static void
         free(int n, T *p) {
            for (int i = 0; i < n; ++i)
               p[i].~T();

            std::free(p);
         }

      public:
         typedef T *iterator;
         typedef const T *const_iterator;
         typedef T value_type;
         typedef T &reference;
         typedef const T &const_reference;
         typedef std::ptrdiff_t difference_type;
         typedef std::size_t size_type;

         vector() : p(NULL), n(0) {
         }

         vector(const vector &v) : p(alloc(v.n, v.p, v.n)), n(v.n) {
         }

         vector(iterator p, size_type n) : p(alloc(n, p, n)), n(n) {
         }

         template<typename C>
         vector(const C &v) :
            p(alloc(v.size(), &*v.begin(), v.size())), n(v.size()) {
         }

         ~vector() {
            free(n, p);
         }

         vector &
         operator=(const vector &v) {
            free(n, p);

            p = alloc(v.n, v.p, v.n);
            n = v.n;

            return *this;
         }

         void
         reserve(size_type m) {
            if (n < m) {
               T *q = alloc(m, p, n);
               free(n, p);

               p = q;
               n = m;
            }
         }

         void
         resize(size_type m, T x = T()) {
            size_type n = size();

            reserve(m);

            for (size_type i = n; i < m; ++i)
               new(&p[i]) T(x);
         }

         void
         push_back(const T &x) {
            size_type n = size();
            reserve(n + 1);
            new(&p[n]) T(x);
         }

         size_type
         size() const {
            return n;
         }

         iterator
         begin() {
            return p;
         }

         const_iterator
         begin() const {
            return p;
         }

         iterator
         end() {
            return p + n;
         }

         const_iterator
         end() const {
            return p + n;
         }

         reference
         operator[](size_type i) {
            return p[i];
         }

         const_reference
         operator[](size_type i) const {
            return p[i];
         }

      private:
         iterator p;
         size_type n;
      };

      template<typename T>
      class vector_ref {
      public:
         typedef T *iterator;
         typedef const T *const_iterator;
         typedef T value_type;
         typedef T &reference;
         typedef const T &const_reference;
         typedef std::ptrdiff_t difference_type;
         typedef std::size_t size_type;

         vector_ref(iterator p, size_type n) : p(p), n(n) {
         }

         template<typename C>
         vector_ref(C &v) : p(&*v.begin()), n(v.size()) {
         }

         size_type
         size() const {
            return n;
         }

         iterator
         begin() {
            return p;
         }

         const_iterator
         begin() const {
            return p;
         }

         iterator
         end() {
            return p + n;
         }

         const_iterator
         end() const {
            return p + n;
         }

         reference
         operator[](int i) {
            return p[i];
         }

         const_reference
         operator[](int i) const {
            return p[i];
         }

      private:
         iterator p;
         size_type n;
      };

      class istream {
      public:
         typedef vector_ref<const unsigned char> buffer_t;

         class error {
         public:
            virtual ~error() {}
         };

         istream(const buffer_t &buf) : buf(buf), offset(0) {}

         void
         read(char *p, size_t n) {
            if (offset + n > buf.size())
               throw error();

            std::memcpy(p, buf.begin() + offset, n);
            offset += n;
         }

      private:
         const buffer_t &buf;
         size_t offset;
      };

      class ostream {
      public:
         typedef vector<unsigned char> buffer_t;

         ostream(buffer_t &buf) : buf(buf), offset(buf.size()) {}

         void
         write(const char *p, size_t n) {
            buf.resize(offset + n);
            std::memcpy(buf.begin() + offset, p, n);
            offset += n;
         }

      private:
         buffer_t &buf;
         size_t offset;
      };

      class string : public vector_ref<const char> {
      public:
         string(const char *p) : vector_ref(p, std::strlen(p)) {
         }

         template<typename C>
         string(const C &v) : vector_ref(v) {
         }

         operator std::string() const {
            return std::string(begin(), end());
         }

         const char *
         c_str() const {
            return begin();
         }

         const char *
         find(const string &s) const {
            for (size_t i = 0; i + s.size() < size(); ++i) {
               if (!std::memcmp(begin() + i, s.begin(), s.size()))
                  return begin() + i;
            }

            return end();
         }
      };

      template<typename T>
      bool
      operator==(const vector_ref<T> &a, const vector_ref<T> &b) {
         if (a.size() != b.size())
            return false;

         for (size_t i = 0; i < a.size(); ++i)
            if (a[i] != b[i])
               return false;

         return true;
      }

      class exception {
      public:
         exception() {}
         virtual ~exception();

         virtual const char *what() const;
      };

      class runtime_error : public exception {
      public:
         runtime_error(const string &what) : _what(what) {}

         virtual const char *what() const;

      protected:
         string _what;
      };
   }
}

#endif
