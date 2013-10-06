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

#include <cstring>

#include "api/util.hpp"
#include "core/event.hpp"
#include "core/resource.hpp"

using namespace clover;

namespace {
   typedef resource::vector vector_t;

   vector_t
   vector(const size_t *p) {
      return range(p, 3);
   }

   ///
   /// Common argument checking shared by memory transfer commands.
   ///
   void
   validate_common(command_queue &q,
                   std::initializer_list<std::reference_wrapper<memory_obj>> mems,
                   const ref_vector<event> &deps) {
      if (any_of([&](const event &ev) {
               return &ev.ctx != &q.ctx;
            }, deps))
         throw error(CL_INVALID_CONTEXT);

      if (any_of([&](const memory_obj &mem) {
               return &mem.ctx != &q.ctx;
            }, mems))
         throw error(CL_INVALID_CONTEXT);
   }

   ///
   /// Class that encapsulates the task of mapping an object of type
   /// \a T.  The return value of get() should be implicitly
   /// convertible to \a void *.
   ///
   template<typename T>
   struct _map {
      static mapping
      get(command_queue &q, T obj, cl_map_flags flags,
          size_t offset, size_t size) {
         return { q, obj->resource(q), flags, true,
                  {{ offset }}, {{ size, 1, 1 }} };
      }
   };

   template<>
   struct _map<void *> {
      static void *
      get(command_queue &q, void *obj, cl_map_flags flags,
          size_t offset, size_t size) {
         return (char *)obj + offset;
      }
   };

   template<>
   struct _map<const void *> {
      static const void *
      get(command_queue &q, const void *obj, cl_map_flags flags,
          size_t offset, size_t size) {
         return (const char *)obj + offset;
      }
   };

   ///
   /// Software copy from \a src_obj to \a dst_obj.  They can be
   /// either pointers or memory objects.
   ///
   template<typename T, typename S>
   std::function<void (event &)>
   soft_copy_op(command_queue &q,
                T dst_obj, const vector_t &dst_orig, const vector_t &dst_pitch,
                S src_obj, const vector_t &src_orig, const vector_t &src_pitch,
                const vector_t &region) {
      return [=, &q](event &) {
         auto dst = _map<T>::get(q, dst_obj, CL_MAP_WRITE,
                                 dot(dst_pitch, dst_orig),
                                 dst_pitch[2] * region[2]);
         auto src = _map<S>::get(q, src_obj, CL_MAP_READ,
                                 dot(src_pitch, src_orig),
                                 src_pitch[2] * region[2]);
         vector_t v = {};

         for (v[2] = 0; v[2] < region[2]; ++v[2]) {
            for (v[1] = 0; v[1] < region[1]; ++v[1]) {
               std::memcpy(
                  static_cast<char *>(dst) + dot(dst_pitch, v),
                  static_cast<const char *>(src) + dot(src_pitch, v),
                  src_pitch[0] * region[0]);
            }
         }
      };
   }

   ///
   /// Hardware copy from \a src_obj to \a dst_obj.
   ///
   template<typename T, typename S>
   std::function<void (event &)>
   hard_copy_op(command_queue &q, T dst_obj, const vector_t &dst_orig,
                S src_obj, const vector_t &src_orig, const vector_t &region) {
      return [=, &q](event &) {
         dst_obj->resource(q).copy(q, dst_orig, region,
                                   src_obj->resource(q), src_orig);
      };
   }
}

CLOVER_API cl_int
clEnqueueReadBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                    size_t offset, size_t size, void *ptr,
                    cl_uint num_deps, const cl_event *d_deps,
                    cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { mem }, deps);

   if (!ptr || offset > mem.size() || offset + size > mem.size())
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_READ_BUFFER, deps,
      soft_copy_op(q,
                   ptr, {{ 0 }}, {{ 1 }},
                   &mem, {{ offset }}, {{ 1 }},
                   {{ size, 1, 1 }}));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueWriteBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                     size_t offset, size_t size, const void *ptr,
                     cl_uint num_deps, const cl_event *d_deps,
                     cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { mem }, deps);

   if (!ptr || offset > mem.size() || offset + size > mem.size())
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_WRITE_BUFFER, deps,
      soft_copy_op(q,
                   &mem, {{ offset }}, {{ 1 }},
                   ptr, {{ 0 }}, {{ 1 }},
                   {{ size, 1, 1 }}));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueReadBufferRect(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                        const size_t *obj_origin,
                        const size_t *host_origin,
                        const size_t *region,
                        size_t obj_row_pitch, size_t obj_slice_pitch,
                        size_t host_row_pitch, size_t host_slice_pitch,
                        void *ptr,
                        cl_uint num_deps, const cl_event *d_deps,
                        cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { mem }, deps);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_READ_BUFFER_RECT, deps,
      soft_copy_op(q,
                   ptr, vector(host_origin),
                   {{ 1, host_row_pitch, host_slice_pitch }},
                   &mem, vector(obj_origin),
                   {{ 1, obj_row_pitch, obj_slice_pitch }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueWriteBufferRect(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                         const size_t *obj_origin,
                         const size_t *host_origin,
                         const size_t *region,
                         size_t obj_row_pitch, size_t obj_slice_pitch,
                         size_t host_row_pitch, size_t host_slice_pitch,
                         const void *ptr,
                         cl_uint num_deps, const cl_event *d_deps,
                         cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { mem }, deps);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_WRITE_BUFFER_RECT, deps,
      soft_copy_op(q,
                   &mem, vector(obj_origin),
                   {{ 1, obj_row_pitch, obj_slice_pitch }},
                   ptr, vector(host_origin),
                   {{ 1, host_row_pitch, host_slice_pitch }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueCopyBuffer(cl_command_queue d_q, cl_mem d_src_mem, cl_mem d_dst_mem,
                    size_t src_offset, size_t dst_offset, size_t size,
                    cl_uint num_deps, const cl_event *d_deps,
                    cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &src_mem = obj(d_src_mem);
   auto &dst_mem = obj(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { src_mem, dst_mem }, deps);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_COPY_BUFFER, deps,
      hard_copy_op(q, &dst_mem, {{ dst_offset }},
                   &src_mem, {{ src_offset }},
                   {{ size, 1, 1 }}));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueCopyBufferRect(cl_command_queue d_q, cl_mem d_src_mem,
                        cl_mem d_dst_mem,
                        const size_t *src_origin, const size_t *dst_origin,
                        const size_t *region,
                        size_t src_row_pitch, size_t src_slice_pitch,
                        size_t dst_row_pitch, size_t dst_slice_pitch,
                        cl_uint num_deps, const cl_event *d_deps,
                        cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &src_mem = obj(d_src_mem);
   auto &dst_mem = obj(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { src_mem, dst_mem }, deps);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_COPY_BUFFER_RECT, deps,
      soft_copy_op(q,
                   &dst_mem, vector(dst_origin),
                   {{ 1, dst_row_pitch, dst_slice_pitch }},
                   &src_mem, vector(src_origin),
                   {{ 1, src_row_pitch, src_slice_pitch }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueReadImage(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                   const size_t *origin, const size_t *region,
                   size_t row_pitch, size_t slice_pitch, void *ptr,
                   cl_uint num_deps, const cl_event *d_deps,
                   cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { img }, deps);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_READ_IMAGE, deps,
      soft_copy_op(q,
                   ptr, {},
                   {{ 1, row_pitch, slice_pitch }},
                   &img, vector(origin),
                   {{ 1, img.row_pitch(), img.slice_pitch() }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueWriteImage(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                    const size_t *origin, const size_t *region,
                    size_t row_pitch, size_t slice_pitch, const void *ptr,
                    cl_uint num_deps, const cl_event *d_deps,
                    cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { img }, deps);

   if (!ptr)
      throw error(CL_INVALID_VALUE);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_WRITE_IMAGE, deps,
      soft_copy_op(q,
                   &img, vector(origin),
                   {{ 1, img.row_pitch(), img.slice_pitch() }},
                   ptr, {},
                   {{ 1, row_pitch, slice_pitch }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueCopyImage(cl_command_queue d_q, cl_mem d_src_mem, cl_mem d_dst_mem,
                   const size_t *src_origin, const size_t *dst_origin,
                   const size_t *region,
                   cl_uint num_deps, const cl_event *d_deps,
                   cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &src_img = obj<image>(d_src_mem);
   auto &dst_img = obj<image>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { src_img, dst_img }, deps);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_COPY_IMAGE, deps,
      hard_copy_op(q,
                   &dst_img, vector(dst_origin),
                   &src_img, vector(src_origin),
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueCopyImageToBuffer(cl_command_queue d_q,
                           cl_mem d_src_mem, cl_mem d_dst_mem,
                           const size_t *src_origin, const size_t *region,
                           size_t dst_offset,
                           cl_uint num_deps, const cl_event *d_deps,
                           cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &src_img = obj<image>(d_src_mem);
   auto &dst_mem = obj(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { src_img, dst_mem }, deps);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_COPY_IMAGE_TO_BUFFER, deps,
      soft_copy_op(q,
                   &dst_mem, {{ dst_offset }},
                   {{ 0, 0, 0 }},
                   &src_img, vector(src_origin),
                   {{ 1, src_img.row_pitch(), src_img.slice_pitch() }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clEnqueueCopyBufferToImage(cl_command_queue d_q,
                           cl_mem d_src_mem, cl_mem d_dst_mem,
                           size_t src_offset,
                           const size_t *dst_origin, const size_t *region,
                           cl_uint num_deps, const cl_event *d_deps,
                           cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &src_mem = obj(d_src_mem);
   auto &dst_img = obj<image>(d_dst_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { src_mem, dst_img }, deps);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_COPY_BUFFER_TO_IMAGE, deps,
      soft_copy_op(q,
                   &dst_img, vector(dst_origin),
                   {{ 1, dst_img.row_pitch(), dst_img.slice_pitch() }},
                   &src_mem, {{ src_offset }},
                   {{ 0, 0, 0 }},
                   vector(region)));

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API void *
clEnqueueMapBuffer(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                   cl_map_flags flags, size_t offset, size_t size,
                   cl_uint num_deps, const cl_event *d_deps,
                   cl_event *rd_ev, cl_int *r_errcode) try {
   auto &q = obj(d_q);
   auto &mem = obj(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { mem }, deps);

   if (offset > mem.size() || offset + size > mem.size())
      throw error(CL_INVALID_VALUE);

   void *map = mem.resource(q).add_map(
      q, flags, blocking, {{ offset }}, {{ size }});

   ret_object(rd_ev, new hard_event(q, CL_COMMAND_MAP_BUFFER, deps));
   ret_error(r_errcode, CL_SUCCESS);
   return map;

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

CLOVER_API void *
clEnqueueMapImage(cl_command_queue d_q, cl_mem d_mem, cl_bool blocking,
                  cl_map_flags flags,
                  const size_t *origin, const size_t *region,
                  size_t *row_pitch, size_t *slice_pitch,
                  cl_uint num_deps, const cl_event *d_deps,
                  cl_event *rd_ev, cl_int *r_errcode) try {
   auto &q = obj(d_q);
   auto &img = obj<image>(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { img }, deps);

   void *map = img.resource(q).add_map(
      q, flags, blocking, vector(origin), vector(region));

   ret_object(rd_ev, new hard_event(q, CL_COMMAND_MAP_IMAGE, deps));
   ret_error(r_errcode, CL_SUCCESS);
   return map;

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

CLOVER_API cl_int
clEnqueueUnmapMemObject(cl_command_queue d_q, cl_mem d_mem, void *ptr,
                        cl_uint num_deps, const cl_event *d_deps,
                        cl_event *rd_ev) try {
   auto &q = obj(d_q);
   auto &mem = obj(d_mem);
   auto deps = objs<wait_list_tag>(d_deps, num_deps);

   validate_common(q, { mem }, deps);

   hard_event *hev = new hard_event(
      q, CL_COMMAND_UNMAP_MEM_OBJECT, deps,
      [=, &q, &mem](event &) {
         mem.resource(q).del_map(ptr);
      });

   ret_object(rd_ev, hev);
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}
