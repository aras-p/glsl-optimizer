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

#include "util/u_math.h"
#include "api/util.hpp"
#include "core/memory.hpp"
#include "core/format.hpp"

using namespace clover;

CLOVER_API cl_mem
clCreateBuffer(cl_context d_ctx, cl_mem_flags flags, size_t size,
               void *host_ptr, cl_int *r_errcode) try {
   auto &ctx = obj(d_ctx);

   if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                                       CL_MEM_COPY_HOST_PTR)))
      throw error(CL_INVALID_HOST_PTR);

   if (!size ||
       size > fold(maximum(), cl_ulong(0),
                   map(std::mem_fn(&device::max_mem_alloc_size), ctx.devices())
          ))
      throw error(CL_INVALID_BUFFER_SIZE);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (util_bitcount(flags & (CL_MEM_READ_ONLY | CL_MEM_WRITE_ONLY |
                              CL_MEM_READ_WRITE)) > 1)
      throw error(CL_INVALID_VALUE);

   if ((flags & CL_MEM_USE_HOST_PTR) &&
       (flags & (CL_MEM_COPY_HOST_PTR | CL_MEM_ALLOC_HOST_PTR)))
      throw error(CL_INVALID_VALUE);

   ret_error(r_errcode, CL_SUCCESS);
   return new root_buffer(ctx, flags, size, host_ptr);

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

CLOVER_API cl_mem
clCreateSubBuffer(cl_mem d_mem, cl_mem_flags flags,
                  cl_buffer_create_type op,
                  const void *op_info, cl_int *r_errcode) try {
   auto &parent = obj<root_buffer>(d_mem);

   if ((flags & (CL_MEM_USE_HOST_PTR |
                 CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR)) ||
       (~flags & parent.flags() & (CL_MEM_READ_ONLY |
                                   CL_MEM_WRITE_ONLY)))
      throw error(CL_INVALID_VALUE);

   if (op == CL_BUFFER_CREATE_TYPE_REGION) {
      auto reg = reinterpret_cast<const cl_buffer_region *>(op_info);

      if (!reg ||
          reg->origin > parent.size() ||
          reg->origin + reg->size > parent.size())
         throw error(CL_INVALID_VALUE);

      if (!reg->size)
         throw error(CL_INVALID_BUFFER_SIZE);

      ret_error(r_errcode, CL_SUCCESS);
      return new sub_buffer(parent, flags, reg->origin, reg->size);

   } else {
      throw error(CL_INVALID_VALUE);
   }

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

CLOVER_API cl_mem
clCreateImage2D(cl_context d_ctx, cl_mem_flags flags,
                const cl_image_format *format,
                size_t width, size_t height, size_t row_pitch,
                void *host_ptr, cl_int *r_errcode) try {
   auto &ctx = obj(d_ctx);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (!format)
      throw error(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);

   if (width < 1 || height < 1)
      throw error(CL_INVALID_IMAGE_SIZE);

   if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                                       CL_MEM_COPY_HOST_PTR)))
      throw error(CL_INVALID_HOST_PTR);

   if (!supported_formats(ctx, CL_MEM_OBJECT_IMAGE2D).count(*format))
      throw error(CL_IMAGE_FORMAT_NOT_SUPPORTED);

   ret_error(r_errcode, CL_SUCCESS);
   return new image2d(ctx, flags, format, width, height,
                      row_pitch, host_ptr);

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

CLOVER_API cl_mem
clCreateImage3D(cl_context d_ctx, cl_mem_flags flags,
                const cl_image_format *format,
                size_t width, size_t height, size_t depth,
                size_t row_pitch, size_t slice_pitch,
                void *host_ptr, cl_int *r_errcode) try {
   auto &ctx = obj(d_ctx);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (!format)
      throw error(CL_INVALID_IMAGE_FORMAT_DESCRIPTOR);

   if (width < 1 || height < 1 || depth < 2)
      throw error(CL_INVALID_IMAGE_SIZE);

   if (bool(host_ptr) != bool(flags & (CL_MEM_USE_HOST_PTR |
                                       CL_MEM_COPY_HOST_PTR)))
      throw error(CL_INVALID_HOST_PTR);

   if (!supported_formats(ctx, CL_MEM_OBJECT_IMAGE3D).count(*format))
      throw error(CL_IMAGE_FORMAT_NOT_SUPPORTED);

   ret_error(r_errcode, CL_SUCCESS);
   return new image3d(ctx, flags, format, width, height, depth,
                      row_pitch, slice_pitch, host_ptr);

} catch (error &e) {
   ret_error(r_errcode, e);
   return NULL;
}

CLOVER_API cl_int
clGetSupportedImageFormats(cl_context d_ctx, cl_mem_flags flags,
                           cl_mem_object_type type, cl_uint count,
                           cl_image_format *r_buf, cl_uint *r_count) try {
   auto &ctx = obj(d_ctx);

   if (flags & ~(CL_MEM_READ_WRITE | CL_MEM_WRITE_ONLY | CL_MEM_READ_ONLY |
                 CL_MEM_USE_HOST_PTR | CL_MEM_ALLOC_HOST_PTR |
                 CL_MEM_COPY_HOST_PTR))
      throw error(CL_INVALID_VALUE);

   if (r_buf && !r_count)
      throw error(CL_INVALID_VALUE);

   auto formats = supported_formats(ctx, type);

   if (r_buf)
      std::copy_n(formats.begin(),
                  std::min((cl_uint)formats.size(), count),
                  r_buf);

   if (r_count)
      *r_count = formats.size();

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clGetMemObjectInfo(cl_mem d_mem, cl_mem_info param,
                   size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };
   auto &mem = obj(d_mem);

   switch (param) {
   case CL_MEM_TYPE:
      buf.as_scalar<cl_mem_object_type>() = mem.type();
      break;

   case CL_MEM_FLAGS:
      buf.as_scalar<cl_mem_flags>() = mem.flags();
      break;

   case CL_MEM_SIZE:
      buf.as_scalar<size_t>() = mem.size();
      break;

   case CL_MEM_HOST_PTR:
      buf.as_scalar<void *>() = mem.host_ptr();
      break;

   case CL_MEM_MAP_COUNT:
      buf.as_scalar<cl_uint>() = 0;
      break;

   case CL_MEM_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = mem.ref_count();
      break;

   case CL_MEM_CONTEXT:
      buf.as_scalar<cl_context>() = desc(mem.context());
      break;

   case CL_MEM_ASSOCIATED_MEMOBJECT: {
      sub_buffer *sub = dynamic_cast<sub_buffer *>(&mem);
      buf.as_scalar<cl_mem>() = (sub ? desc(sub->parent()) : NULL);
      break;
   }
   case CL_MEM_OFFSET: {
      sub_buffer *sub = dynamic_cast<sub_buffer *>(&mem);
      buf.as_scalar<size_t>() = (sub ? sub->offset() : 0);
      break;
   }
   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clGetImageInfo(cl_mem d_mem, cl_image_info param,
               size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };
   auto &img = obj<image>(d_mem);

   switch (param) {
   case CL_IMAGE_FORMAT:
      buf.as_scalar<cl_image_format>() = img.format();
      break;

   case CL_IMAGE_ELEMENT_SIZE:
      buf.as_scalar<size_t>() = 0;
      break;

   case CL_IMAGE_ROW_PITCH:
      buf.as_scalar<size_t>() = img.row_pitch();
      break;

   case CL_IMAGE_SLICE_PITCH:
      buf.as_scalar<size_t>() = img.slice_pitch();
      break;

   case CL_IMAGE_WIDTH:
      buf.as_scalar<size_t>() = img.width();
      break;

   case CL_IMAGE_HEIGHT:
      buf.as_scalar<size_t>() = img.height();
      break;

   case CL_IMAGE_DEPTH:
      buf.as_scalar<size_t>() = img.depth();
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clRetainMemObject(cl_mem d_mem) try {
   obj(d_mem).retain();
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clReleaseMemObject(cl_mem d_mem) try {
   if (obj(d_mem).release())
      delete pobj(d_mem);

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clSetMemObjectDestructorCallback(cl_mem d_mem,
                                 void (CL_CALLBACK *pfn_notify)(cl_mem, void *),
                                 void *user_data) try {
   auto &mem = obj(d_mem);

   if (!pfn_notify)
      return CL_INVALID_VALUE;

   mem.destroy_notify([=]{ pfn_notify(d_mem, user_data); });

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_mem
clCreateImage(cl_context d_ctx, cl_mem_flags flags,
              const cl_image_format *format,
              const cl_image_desc *image_desc,
              void *host_ptr, cl_int *r_errcode) {
   // This function was added in OpenCL 1.2
   std::cerr << "CL user error: clCreateImage() not supported by OpenCL 1.1." <<
                std::endl;
   ret_error(r_errcode, CL_INVALID_OPERATION);
   return NULL;
}
