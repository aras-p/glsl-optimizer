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

#include "core/device.hpp"
#include "core/platform.hpp"
#include "pipe/p_screen.h"
#include "pipe/p_state.h"

using namespace clover;

namespace {
   template<typename T>
   std::vector<T>
   get_compute_param(pipe_screen *pipe, pipe_compute_cap cap) {
      int sz = pipe->get_compute_param(pipe, cap, NULL);
      std::vector<T> v(sz / sizeof(T));

      pipe->get_compute_param(pipe, cap, &v.front());
      return v;
   }
}

device::device(clover::platform &platform, pipe_loader_device *ldev) :
   platform(platform), ldev(ldev) {
   pipe = pipe_loader_create_screen(ldev, PIPE_SEARCH_DIR);
   if (!pipe || !pipe->get_param(pipe, PIPE_CAP_COMPUTE)) {
      if (pipe)
         pipe->destroy(pipe);
      throw error(CL_INVALID_DEVICE);
   }
}

device::~device() {
   if (pipe)
      pipe->destroy(pipe);
   if (ldev)
      pipe_loader_release(&ldev, 1);
}

bool
device::operator==(const device &dev) const {
   return this == &dev;
}

cl_device_type
device::type() const {
   switch (ldev->type) {
   case PIPE_LOADER_DEVICE_SOFTWARE:
      return CL_DEVICE_TYPE_CPU;
   case PIPE_LOADER_DEVICE_PCI:
   case PIPE_LOADER_DEVICE_PLATFORM:
      return CL_DEVICE_TYPE_GPU;
   default:
      assert(0);
      return 0;
   }
}

cl_uint
device::vendor_id() const {
   switch (ldev->type) {
   case PIPE_LOADER_DEVICE_SOFTWARE:
   case PIPE_LOADER_DEVICE_PLATFORM:
      return 0;
   case PIPE_LOADER_DEVICE_PCI:
      return ldev->u.pci.vendor_id;
   default:
      assert(0);
      return 0;
   }
}

size_t
device::max_images_read() const {
   return PIPE_MAX_SHADER_RESOURCES;
}

size_t
device::max_images_write() const {
   return PIPE_MAX_SHADER_RESOURCES;
}

cl_uint
device::max_image_levels_2d() const {
   return pipe->get_param(pipe, PIPE_CAP_MAX_TEXTURE_2D_LEVELS);
}

cl_uint
device::max_image_levels_3d() const {
   return pipe->get_param(pipe, PIPE_CAP_MAX_TEXTURE_3D_LEVELS);
}

cl_uint
device::max_samplers() const {
   return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
                                 PIPE_SHADER_CAP_MAX_TEXTURE_SAMPLERS);
}

cl_ulong
device::max_mem_global() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_GLOBAL_SIZE)[0];
}

cl_ulong
device::max_mem_local() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_LOCAL_SIZE)[0];
}

cl_ulong
device::max_mem_input() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_INPUT_SIZE)[0];
}

cl_ulong
device::max_const_buffer_size() const {
   return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
                                 PIPE_SHADER_CAP_MAX_CONSTS) * 16;
}

cl_uint
device::max_const_buffers() const {
   return pipe->get_shader_param(pipe, PIPE_SHADER_COMPUTE,
                                 PIPE_SHADER_CAP_MAX_CONST_BUFFERS);
}

size_t
device::max_threads_per_block() const {
   return get_compute_param<uint64_t>(
      pipe, PIPE_COMPUTE_CAP_MAX_THREADS_PER_BLOCK)[0];
}

cl_ulong
device::max_mem_alloc_size() const {
   return get_compute_param<uint64_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_MEM_ALLOC_SIZE)[0];
}

cl_uint
device::max_clock_frequency() const {
   return get_compute_param<uint32_t>(pipe,
                                      PIPE_COMPUTE_CAP_MAX_CLOCK_FREQUENCY)[0];
}

std::vector<size_t>
device::max_block_size() const {
   auto v = get_compute_param<uint64_t>(pipe, PIPE_COMPUTE_CAP_MAX_BLOCK_SIZE);
   return { v.begin(), v.end() };
}

std::string
device::device_name() const {
   return pipe->get_name(pipe);
}

std::string
device::vendor_name() const {
   return pipe->get_vendor(pipe);
}

enum pipe_shader_ir
device::ir_format() const {
   return (enum pipe_shader_ir) pipe->get_shader_param(
      pipe, PIPE_SHADER_COMPUTE, PIPE_SHADER_CAP_PREFERRED_IR);
}

std::string
device::ir_target() const {
   std::vector<char> target = get_compute_param<char>(
      pipe, PIPE_COMPUTE_CAP_IR_TARGET);
   return { target.data() };
}

enum pipe_endian
device::endianness() const {
   return (enum pipe_endian)pipe->get_param(pipe, PIPE_CAP_ENDIANNESS);
}
