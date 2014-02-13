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

#include "core/kernel.hpp"
#include "core/resource.hpp"
#include "util/factor.hpp"
#include "util/u_math.h"
#include "pipe/p_context.h"

using namespace clover;

kernel::kernel(clover::program &prog, const std::string &name,
               const std::vector<module::argument> &margs) :
   program(prog), _name(name), exec(*this) {
   for (auto &marg : margs) {
      if (marg.type == module::argument::scalar)
         _args.emplace_back(new scalar_argument(marg.size));
      else if (marg.type == module::argument::global)
         _args.emplace_back(new global_argument);
      else if (marg.type == module::argument::local)
         _args.emplace_back(new local_argument);
      else if (marg.type == module::argument::constant)
         _args.emplace_back(new constant_argument);
      else if (marg.type == module::argument::image2d_rd ||
               marg.type == module::argument::image3d_rd)
         _args.emplace_back(new image_rd_argument);
      else if (marg.type == module::argument::image2d_wr ||
               marg.type == module::argument::image3d_wr)
         _args.emplace_back(new image_wr_argument);
      else if (marg.type == module::argument::sampler)
         _args.emplace_back(new sampler_argument);
      else
         throw error(CL_INVALID_KERNEL_DEFINITION);
   }
}

template<typename V>
static inline std::vector<uint>
pad_vector(command_queue &q, const V &v, uint x) {
   std::vector<uint> w { v.begin(), v.end() };
   w.resize(q.device().max_block_size().size(), x);
   return w;
}

void
kernel::launch(command_queue &q,
               const std::vector<size_t> &grid_offset,
               const std::vector<size_t> &grid_size,
               const std::vector<size_t> &block_size) {
   const auto m = program().binary(q.device());
   const auto reduced_grid_size =
      map(divides(), grid_size, block_size);
   void *st = exec.bind(&q);

   // The handles are created during exec_context::bind(), so we need make
   // sure to call exec_context::bind() before retrieving them.
   std::vector<uint32_t *> g_handles = map([&](size_t h) {
         return (uint32_t *)&exec.input[h];
      }, exec.g_handles);

   q.pipe->bind_compute_state(q.pipe, st);
   q.pipe->bind_sampler_states(q.pipe, PIPE_SHADER_COMPUTE,
                               0, exec.samplers.size(),
                               exec.samplers.data());

   q.pipe->set_sampler_views(q.pipe, PIPE_SHADER_COMPUTE, 0,
                             exec.sviews.size(), exec.sviews.data());
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(),
                                 exec.resources.data());
   q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(),
                              exec.g_buffers.data(), g_handles.data());

   q.pipe->launch_grid(q.pipe,
                       pad_vector(q, block_size, 1).data(),
                       pad_vector(q, reduced_grid_size, 1).data(),
                       find(name_equals(_name), m.syms).offset,
                       exec.input.data());

   q.pipe->set_global_binding(q.pipe, 0, exec.g_buffers.size(), NULL, NULL);
   q.pipe->set_compute_resources(q.pipe, 0, exec.resources.size(), NULL);
   q.pipe->set_sampler_views(q.pipe, PIPE_SHADER_COMPUTE, 0,
                             exec.sviews.size(), NULL);
   q.pipe->bind_sampler_states(q.pipe, PIPE_SHADER_COMPUTE, 0,
                               exec.samplers.size(), NULL);
   exec.unbind();
}

size_t
kernel::mem_local() const {
   size_t sz = 0;

   for (auto &arg : args()) {
      if (dynamic_cast<local_argument *>(&arg))
         sz += arg.storage();
   }

   return sz;
}

size_t
kernel::mem_private() const {
   return 0;
}

const std::string &
kernel::name() const {
   return _name;
}

std::vector<size_t>
kernel::optimal_block_size(const command_queue &q,
                           const std::vector<size_t> &grid_size) const {
   return factor::find_grid_optimal_factor<size_t>(
      q.device().max_threads_per_block(), q.device().max_block_size(),
      grid_size);
}

std::vector<size_t>
kernel::required_block_size() const {
   return { 0, 0, 0 };
}

kernel::argument_range
kernel::args() {
   return map(derefs(), _args);
}

kernel::const_argument_range
kernel::args() const {
   return map(derefs(), _args);
}

const module &
kernel::module(const command_queue &q) const {
   return program().binary(q.device());
}

kernel::exec_context::exec_context(kernel &kern) :
   kern(kern), q(NULL), mem_local(0), st(NULL), cs() {
}

kernel::exec_context::~exec_context() {
   if (st)
      q->pipe->delete_compute_state(q->pipe, st);
}

void *
kernel::exec_context::bind(intrusive_ptr<command_queue> _q) {
   std::swap(q, _q);

   // Bind kernel arguments.
   auto &m = kern.program().binary(q->device());
   auto margs = find(name_equals(kern.name()), m.syms).args;
   auto msec = find(type_equals(module::section::text), m.secs);

   for_each([=](kernel::argument &karg, const module::argument &marg) {
               karg.bind(*this, marg);
            }, kern.args(), margs);

   // Create a new compute state if anything changed.
   if (!st || q != _q ||
       cs.req_local_mem != mem_local ||
       cs.req_input_mem != input.size()) {
      if (st)
         _q->pipe->delete_compute_state(_q->pipe, st);

      cs.prog = msec.data.begin();
      cs.req_local_mem = mem_local;
      cs.req_input_mem = input.size();
      st = q->pipe->create_compute_state(q->pipe, &cs);
   }

   return st;
}

void
kernel::exec_context::unbind() {
   for (auto &arg : kern.args())
      arg.unbind(*this);

   input.clear();
   samplers.clear();
   sviews.clear();
   resources.clear();
   g_buffers.clear();
   g_handles.clear();
   mem_local = 0;
}

namespace {
   template<typename T>
   std::vector<uint8_t>
   bytes(const T& x) {
      return { (uint8_t *)&x, (uint8_t *)&x + sizeof(x) };
   }

   ///
   /// Transform buffer \a v from the native byte order into the byte
   /// order specified by \a e.
   ///
   template<typename T>
   void
   byteswap(T &v, pipe_endian e) {
      if (PIPE_ENDIAN_NATIVE != e)
         std::reverse(v.begin(), v.end());
   }

   ///
   /// Pad buffer \a v to the next multiple of \a n.
   ///
   template<typename T>
   void
   align(T &v, size_t n) {
      v.resize(util_align_npot(v.size(), n));
   }

   bool
   msb(const std::vector<uint8_t> &s) {
      if (PIPE_ENDIAN_NATIVE == PIPE_ENDIAN_LITTLE)
         return s.back() & 0x80;
      else
         return s.front() & 0x80;
   }

   ///
   /// Resize buffer \a v to size \a n using sign or zero extension
   /// according to \a ext.
   ///
   template<typename T>
   void
   extend(T &v, enum module::argument::ext_type ext, size_t n) {
      const size_t m = std::min(v.size(), n);
      const bool sign_ext = (ext == module::argument::sign_ext);
      const uint8_t fill = (sign_ext && msb(v) ? ~0 : 0);
      T w(n, fill);

      if (PIPE_ENDIAN_NATIVE == PIPE_ENDIAN_LITTLE)
         std::copy_n(v.begin(), m, w.begin());
      else
         std::copy_n(v.end() - m, m, w.end() - m);

      std::swap(v, w);
   }

   ///
   /// Append buffer \a w to \a v.
   ///
   template<typename T>
   void
   insert(T &v, const T &w) {
      v.insert(v.end(), w.begin(), w.end());
   }

   ///
   /// Append \a n elements to the end of buffer \a v.
   ///
   template<typename T>
   size_t
   allocate(T &v, size_t n) {
      size_t pos = v.size();
      v.resize(pos + n);
      return pos;
   }
}

kernel::argument::argument() : _set(false) {
}

bool
kernel::argument::set() const {
   return _set;
}

size_t
kernel::argument::storage() const {
   return 0;
}

kernel::scalar_argument::scalar_argument(size_t size) : size(size) {
}

void
kernel::scalar_argument::set(size_t size, const void *value) {
   if (size != this->size)
      throw error(CL_INVALID_ARG_SIZE);

   v = { (uint8_t *)value, (uint8_t *)value + size };
   _set = true;
}

void
kernel::scalar_argument::bind(exec_context &ctx,
                              const module::argument &marg) {
   auto w = v;

   extend(w, marg.ext_type, marg.target_size);
   byteswap(w, ctx.q->device().endianness());
   align(ctx.input, marg.target_align);
   insert(ctx.input, w);
}

void
kernel::scalar_argument::unbind(exec_context &ctx) {
}

void
kernel::global_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   buf = pobj<buffer>(value ? *(cl_mem *)value : NULL);
   _set = true;
}

void
kernel::global_argument::bind(exec_context &ctx,
                              const module::argument &marg) {
   align(ctx.input, marg.target_align);

   if (buf) {
      const resource &r = buf->resource(*ctx.q);
      ctx.g_handles.push_back(ctx.input.size());
      ctx.g_buffers.push_back(r.pipe);

      // How to handle multi-demensional offsets?
      // We don't need to.  Buffer offsets are always
      // one-dimensional.
      auto v = bytes(r.offset[0]);
      extend(v, marg.ext_type, marg.target_size);
      byteswap(v, ctx.q->device().endianness());
      insert(ctx.input, v);
   } else {
      // Null pointer.
      allocate(ctx.input, marg.target_size);
   }
}

void
kernel::global_argument::unbind(exec_context &ctx) {
}

size_t
kernel::local_argument::storage() const {
   return _storage;
}

void
kernel::local_argument::set(size_t size, const void *value) {
   if (value)
      throw error(CL_INVALID_ARG_VALUE);

   _storage = size;
   _set = true;
}

void
kernel::local_argument::bind(exec_context &ctx,
                             const module::argument &marg) {
   auto v = bytes(ctx.mem_local);

   extend(v, module::argument::zero_ext, marg.target_size);
   byteswap(v, ctx.q->device().endianness());
   align(ctx.input, marg.target_align);
   insert(ctx.input, v);

   ctx.mem_local += _storage;
}

void
kernel::local_argument::unbind(exec_context &ctx) {
}

void
kernel::constant_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   buf = pobj<buffer>(value ? *(cl_mem *)value : NULL);
   _set = true;
}

void
kernel::constant_argument::bind(exec_context &ctx,
                                const module::argument &marg) {
   align(ctx.input, marg.target_align);

   if (buf) {
      resource &r = buf->resource(*ctx.q);
      auto v = bytes(ctx.resources.size() << 24 | r.offset[0]);

      extend(v, module::argument::zero_ext, marg.target_size);
      byteswap(v, ctx.q->device().endianness());
      insert(ctx.input, v);

      st = r.bind_surface(*ctx.q, false);
      ctx.resources.push_back(st);
   } else {
      // Null pointer.
      allocate(ctx.input, marg.target_size);
   }
}

void
kernel::constant_argument::unbind(exec_context &ctx) {
   if (buf)
      buf->resource(*ctx.q).unbind_surface(*ctx.q, st);
}

void
kernel::image_rd_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   img = &obj<image>(*(cl_mem *)value);
   _set = true;
}

void
kernel::image_rd_argument::bind(exec_context &ctx,
                                const module::argument &marg) {
   auto v = bytes(ctx.sviews.size());

   extend(v, module::argument::zero_ext, marg.target_size);
   byteswap(v, ctx.q->device().endianness());
   align(ctx.input, marg.target_align);
   insert(ctx.input, v);

   st = img->resource(*ctx.q).bind_sampler_view(*ctx.q);
   ctx.sviews.push_back(st);
}

void
kernel::image_rd_argument::unbind(exec_context &ctx) {
   img->resource(*ctx.q).unbind_sampler_view(*ctx.q, st);
}

void
kernel::image_wr_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_mem))
      throw error(CL_INVALID_ARG_SIZE);

   img = &obj<image>(*(cl_mem *)value);
   _set = true;
}

void
kernel::image_wr_argument::bind(exec_context &ctx,
                                const module::argument &marg) {
   auto v = bytes(ctx.resources.size());

   extend(v, module::argument::zero_ext, marg.target_size);
   byteswap(v, ctx.q->device().endianness());
   align(ctx.input, marg.target_align);
   insert(ctx.input, v);

   st = img->resource(*ctx.q).bind_surface(*ctx.q, true);
   ctx.resources.push_back(st);
}

void
kernel::image_wr_argument::unbind(exec_context &ctx) {
   img->resource(*ctx.q).unbind_surface(*ctx.q, st);
}

void
kernel::sampler_argument::set(size_t size, const void *value) {
   if (size != sizeof(cl_sampler))
      throw error(CL_INVALID_ARG_SIZE);

   s = &obj(*(cl_sampler *)value);
   _set = true;
}

void
kernel::sampler_argument::bind(exec_context &ctx,
                               const module::argument &marg) {
   st = s->bind(*ctx.q);
   ctx.samplers.push_back(st);
}

void
kernel::sampler_argument::unbind(exec_context &ctx) {
   s->unbind(*ctx.q, st);
}
