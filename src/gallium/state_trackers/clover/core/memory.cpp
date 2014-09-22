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

#include "core/memory.hpp"
#include "core/resource.hpp"
#include "util/u_format.h"

using namespace clover;

memory_obj::memory_obj(clover::context &ctx, cl_mem_flags flags,
                       size_t size, void *host_ptr) :
   context(ctx), _flags(flags),
   _size(size), _host_ptr(host_ptr) {
   if (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_USE_HOST_PTR))
      data.append((char *)host_ptr, size);
}

memory_obj::~memory_obj() {
   while (_destroy_notify.size()) {
      _destroy_notify.top()();
      _destroy_notify.pop();
   }
}

bool
memory_obj::operator==(const memory_obj &obj) const {
   return this == &obj;
}

void
memory_obj::destroy_notify(std::function<void ()> f) {
   _destroy_notify.push(f);
}

cl_mem_flags
memory_obj::flags() const {
   return _flags;
}

size_t
memory_obj::size() const {
   return _size;
}

void *
memory_obj::host_ptr() const {
   return _host_ptr;
}

buffer::buffer(clover::context &ctx, cl_mem_flags flags,
               size_t size, void *host_ptr) :
   memory_obj(ctx, flags, size, host_ptr) {
}

cl_mem_object_type
buffer::type() const {
   return CL_MEM_OBJECT_BUFFER;
}

root_buffer::root_buffer(clover::context &ctx, cl_mem_flags flags,
                         size_t size, void *host_ptr) :
   buffer(ctx, flags, size, host_ptr) {
}

resource &
root_buffer::resource(command_queue &q) {
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q.device())) {
      auto r = (!resources.empty() ?
                new root_resource(q.device(), *this,
                                  *resources.begin()->second) :
                new root_resource(q.device(), *this, q, data));

      resources.insert(std::make_pair(&q.device(),
                                      std::unique_ptr<root_resource>(r)));
      data.clear();
   }

   return *resources.find(&q.device())->second;
}

sub_buffer::sub_buffer(root_buffer &parent, cl_mem_flags flags,
                       size_t offset, size_t size) :
   buffer(parent.context(), flags, size,
          (char *)parent.host_ptr() + offset),
   parent(parent), _offset(offset) {
}

resource &
sub_buffer::resource(command_queue &q) {
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q.device())) {
      auto r = new sub_resource(parent().resource(q), {{ offset() }});

      resources.insert(std::make_pair(&q.device(),
                                      std::unique_ptr<sub_resource>(r)));
   }

   return *resources.find(&q.device())->second;
}

size_t
sub_buffer::offset() const {
   return _offset;
}

image::image(clover::context &ctx, cl_mem_flags flags,
             const cl_image_format *format,
             size_t width, size_t height, size_t depth,
             size_t row_pitch, size_t slice_pitch, size_t size,
             void *host_ptr) :
   memory_obj(ctx, flags, size, host_ptr),
   _format(*format), _width(width), _height(height), _depth(depth),
   _row_pitch(row_pitch), _slice_pitch(slice_pitch) {
}

resource &
image::resource(command_queue &q) {
   // Create a new resource if there's none for this device yet.
   if (!resources.count(&q.device())) {
      auto r = (!resources.empty() ?
                new root_resource(q.device(), *this,
                                  *resources.begin()->second) :
                new root_resource(q.device(), *this, q, data));

      resources.insert(std::make_pair(&q.device(),
                                      std::unique_ptr<root_resource>(r)));
      data.clear();
   }

   return *resources.find(&q.device())->second;
}

cl_image_format
image::format() const {
   return _format;
}

size_t
image::width() const {
   return _width;
}

size_t
image::height() const {
   return _height;
}

size_t
image::depth() const {
   return _depth;
}

size_t
image::pixel_size() const {
   return util_format_get_blocksize(translate_format(_format));
}

size_t
image::row_pitch() const {
   return _row_pitch;
}

size_t
image::slice_pitch() const {
   return _slice_pitch;
}

image2d::image2d(clover::context &ctx, cl_mem_flags flags,
                 const cl_image_format *format, size_t width,
                 size_t height, size_t row_pitch,
                 void *host_ptr) :
   image(ctx, flags, format, width, height, 0,
         row_pitch, 0, height * row_pitch, host_ptr) {
}

cl_mem_object_type
image2d::type() const {
   return CL_MEM_OBJECT_IMAGE2D;
}

image3d::image3d(clover::context &ctx, cl_mem_flags flags,
                 const cl_image_format *format,
                 size_t width, size_t height, size_t depth,
                 size_t row_pitch, size_t slice_pitch,
                 void *host_ptr) :
   image(ctx, flags, format, width, height, depth,
         row_pitch, slice_pitch, depth * slice_pitch,
         host_ptr) {
}

cl_mem_object_type
image3d::type() const {
   return CL_MEM_OBJECT_IMAGE3D;
}
