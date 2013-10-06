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

#ifndef CLOVER_UTIL_POINTER_HPP
#define CLOVER_UTIL_POINTER_HPP

#include <atomic>

namespace clover {
   ///
   /// Base class for objects that support reference counting.
   ///
   class ref_counter {
   public:
      ref_counter() : _ref_count(1) {}

      unsigned
      ref_count() {
         return _ref_count;
      }

      void
      retain() {
         _ref_count++;
      }

      bool
      release() {
         return (--_ref_count) == 0;
      }

   private:
      std::atomic<unsigned> _ref_count;
   };

   ///
   /// Intrusive smart pointer for objects that implement the
   /// clover::ref_counter interface.
   ///
   template<typename T>
   class ref_ptr {
   public:
      ref_ptr(T *q = NULL) : p(NULL) {
         reset(q);
      }

      ref_ptr(const ref_ptr<T> &ref) : p(NULL) {
         reset(ref.p);
      }

      ~ref_ptr() {
         reset(NULL);
      }

      void
      reset(T *q = NULL) {
         if (q)
            q->retain();
         if (p && p->release())
            delete p;
         p = q;
      }

      ref_ptr &
      operator=(const ref_ptr &ref) {
         reset(ref.p);
         return *this;
      }

      T &
      operator*() const {
         return *p;
      }

      T *
      operator->() const {
         return p;
      }

      explicit operator bool() const {
         return p;
      }

   private:
      T *p;
   };

   ///
   /// Transfer the caller's ownership of a reference-counted object
   /// to a clover::ref_ptr smart pointer.
   ///
   template<typename T>
   inline ref_ptr<T>
   transfer(T *p) {
      ref_ptr<T> ref { p };
      p->release();
      return ref;
   }

   ///
   /// Class that implements the usual pointer interface but in fact
   /// contains the object it seems to be pointing to.
   ///
   template<typename T>
   class pseudo_ptr {
   public:
      pseudo_ptr(T x) : x(x) {
      }

      pseudo_ptr(const pseudo_ptr &p) : x(p.x) {
      }

      pseudo_ptr &
      operator=(const pseudo_ptr &p) {
         x = p.x;
         return *this;
      }

      T &
      operator*() {
         return x;
      }

      T *
      operator->() {
         return &x;
      }

      explicit operator bool() const {
         return true;
      }

   private:
      T x;
   };
}

#endif
