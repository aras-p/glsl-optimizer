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

#include "api/util.hpp"
#include "core/platform.hpp"
#include "core/device.hpp"

using namespace clover;

CLOVER_API cl_int
clGetDeviceIDs(cl_platform_id d_platform, cl_device_type device_type,
               cl_uint num_entries, cl_device_id *rd_devices,
               cl_uint *rnum_devices) try {
   auto &platform = obj(d_platform);
   std::vector<cl_device_id> d_devs;

   if ((!num_entries && rd_devices) ||
       (!rnum_devices && !rd_devices))
      throw error(CL_INVALID_VALUE);

   // Collect matching devices
   for (device &dev : platform) {
      if (((device_type & CL_DEVICE_TYPE_DEFAULT) &&
           dev == platform.front()) ||
          (device_type & dev.type()))
         d_devs.push_back(desc(dev));
   }

   if (d_devs.empty())
      throw error(CL_DEVICE_NOT_FOUND);

   // ...and return the requested data.
   if (rnum_devices)
      *rnum_devices = d_devs.size();
   if (rd_devices)
      copy(range(d_devs.begin(),
                 std::min((unsigned)d_devs.size(), num_entries)),
           rd_devices);

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clCreateSubDevices(cl_device_id d_dev,
                   const cl_device_partition_property *props,
                   cl_uint num_devs, cl_device_id *rd_devs,
                   cl_uint *rnum_devs) {
   // There are no currently supported partitioning schemes.
   return CL_INVALID_VALUE;
}

CLOVER_API cl_int
clRetainDevice(cl_device_id d_dev) try {
   obj(d_dev);

   // The reference count doesn't change for root devices.
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clReleaseDevice(cl_device_id d_dev) try {
   obj(d_dev);

   // The reference count doesn't change for root devices.
   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

CLOVER_API cl_int
clGetDeviceInfo(cl_device_id d_dev, cl_device_info param,
                size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };
   auto &dev = obj(d_dev);

   switch (param) {
   case CL_DEVICE_TYPE:
      buf.as_scalar<cl_device_type>() = dev.type();
      break;

   case CL_DEVICE_VENDOR_ID:
      buf.as_scalar<cl_uint>() = dev.vendor_id();
      break;

   case CL_DEVICE_MAX_COMPUTE_UNITS:
      buf.as_scalar<cl_uint>() = 1;
      break;

   case CL_DEVICE_MAX_WORK_ITEM_DIMENSIONS:
      buf.as_scalar<cl_uint>() = dev.max_block_size().size();
      break;

   case CL_DEVICE_MAX_WORK_ITEM_SIZES:
      buf.as_vector<size_t>() = dev.max_block_size();
      break;

   case CL_DEVICE_MAX_WORK_GROUP_SIZE:
      buf.as_scalar<size_t>() = dev.max_threads_per_block();
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_CHAR:
      buf.as_scalar<cl_uint>() = 16;
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_SHORT:
      buf.as_scalar<cl_uint>() = 8;
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_INT:
      buf.as_scalar<cl_uint>() = 4;
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_LONG:
      buf.as_scalar<cl_uint>() = 2;
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_FLOAT:
      buf.as_scalar<cl_uint>() = 4;
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_DOUBLE:
      buf.as_scalar<cl_uint>() = 2;
      break;

   case CL_DEVICE_PREFERRED_VECTOR_WIDTH_HALF:
      buf.as_scalar<cl_uint>() = 0;
      break;

   case CL_DEVICE_MAX_CLOCK_FREQUENCY:
      buf.as_scalar<cl_uint>() = dev.max_clock_frequency();
      break;

   case CL_DEVICE_ADDRESS_BITS:
      buf.as_scalar<cl_uint>() = 32;
      break;

   case CL_DEVICE_MAX_READ_IMAGE_ARGS:
      buf.as_scalar<cl_uint>() = dev.max_images_read();
      break;

   case CL_DEVICE_MAX_WRITE_IMAGE_ARGS:
      buf.as_scalar<cl_uint>() = dev.max_images_write();
      break;

   case CL_DEVICE_MAX_MEM_ALLOC_SIZE:
      buf.as_scalar<cl_ulong>() = dev.max_mem_alloc_size();
      break;

   case CL_DEVICE_IMAGE2D_MAX_WIDTH:
   case CL_DEVICE_IMAGE2D_MAX_HEIGHT:
      buf.as_scalar<size_t>() = 1 << dev.max_image_levels_2d();
      break;

   case CL_DEVICE_IMAGE3D_MAX_WIDTH:
   case CL_DEVICE_IMAGE3D_MAX_HEIGHT:
   case CL_DEVICE_IMAGE3D_MAX_DEPTH:
      buf.as_scalar<size_t>() = 1 << dev.max_image_levels_3d();
      break;

   case CL_DEVICE_IMAGE_SUPPORT:
      buf.as_scalar<cl_bool>() = CL_TRUE;
      break;

   case CL_DEVICE_MAX_PARAMETER_SIZE:
      buf.as_scalar<size_t>() = dev.max_mem_input();
      break;

   case CL_DEVICE_MAX_SAMPLERS:
      buf.as_scalar<cl_uint>() = dev.max_samplers();
      break;

   case CL_DEVICE_MEM_BASE_ADDR_ALIGN:
   case CL_DEVICE_MIN_DATA_TYPE_ALIGN_SIZE:
      buf.as_scalar<cl_uint>() = 128;
      break;

   case CL_DEVICE_SINGLE_FP_CONFIG:
      buf.as_scalar<cl_device_fp_config>() =
         CL_FP_DENORM | CL_FP_INF_NAN | CL_FP_ROUND_TO_NEAREST;
      break;

   case CL_DEVICE_GLOBAL_MEM_CACHE_TYPE:
      buf.as_scalar<cl_device_mem_cache_type>() = CL_NONE;
      break;

   case CL_DEVICE_GLOBAL_MEM_CACHELINE_SIZE:
      buf.as_scalar<cl_uint>() = 0;
      break;

   case CL_DEVICE_GLOBAL_MEM_CACHE_SIZE:
      buf.as_scalar<cl_ulong>() = 0;
      break;

   case CL_DEVICE_GLOBAL_MEM_SIZE:
      buf.as_scalar<cl_ulong>() = dev.max_mem_global();
      break;

   case CL_DEVICE_MAX_CONSTANT_BUFFER_SIZE:
      buf.as_scalar<cl_ulong>() = dev.max_const_buffer_size();
      break;

   case CL_DEVICE_MAX_CONSTANT_ARGS:
      buf.as_scalar<cl_uint>() = dev.max_const_buffers();
      break;

   case CL_DEVICE_LOCAL_MEM_TYPE:
      buf.as_scalar<cl_device_local_mem_type>() = CL_LOCAL;
      break;

   case CL_DEVICE_LOCAL_MEM_SIZE:
      buf.as_scalar<cl_ulong>() = dev.max_mem_local();
      break;

   case CL_DEVICE_ERROR_CORRECTION_SUPPORT:
      buf.as_scalar<cl_bool>() = CL_FALSE;
      break;

   case CL_DEVICE_PROFILING_TIMER_RESOLUTION:
      buf.as_scalar<size_t>() = 0;
      break;

   case CL_DEVICE_ENDIAN_LITTLE:
      buf.as_scalar<cl_bool>() = (dev.endianness() == PIPE_ENDIAN_LITTLE);
      break;

   case CL_DEVICE_AVAILABLE:
   case CL_DEVICE_COMPILER_AVAILABLE:
      buf.as_scalar<cl_bool>() = CL_TRUE;
      break;

   case CL_DEVICE_EXECUTION_CAPABILITIES:
      buf.as_scalar<cl_device_exec_capabilities>() = CL_EXEC_KERNEL;
      break;

   case CL_DEVICE_QUEUE_PROPERTIES:
      buf.as_scalar<cl_command_queue_properties>() = CL_QUEUE_PROFILING_ENABLE;
      break;

   case CL_DEVICE_NAME:
      buf.as_string() = dev.device_name();
      break;

   case CL_DEVICE_VENDOR:
      buf.as_string() = dev.vendor_name();
      break;

   case CL_DRIVER_VERSION:
      buf.as_string() = PACKAGE_VERSION;
      break;

   case CL_DEVICE_PROFILE:
      buf.as_string() = "FULL_PROFILE";
      break;

   case CL_DEVICE_VERSION:
      buf.as_string() = "OpenCL 1.1 MESA " PACKAGE_VERSION;
      break;

   case CL_DEVICE_EXTENSIONS:
      buf.as_string() = "";
      break;

   case CL_DEVICE_PLATFORM:
      buf.as_scalar<cl_platform_id>() = desc(dev.platform);
      break;

   case CL_DEVICE_HOST_UNIFIED_MEMORY:
      buf.as_scalar<cl_bool>() = CL_TRUE;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_CHAR:
      buf.as_scalar<cl_uint>() = 16;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_SHORT:
      buf.as_scalar<cl_uint>() = 8;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_INT:
      buf.as_scalar<cl_uint>() = 4;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_LONG:
      buf.as_scalar<cl_uint>() = 2;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_FLOAT:
      buf.as_scalar<cl_uint>() = 4;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_DOUBLE:
      buf.as_scalar<cl_uint>() = 2;
      break;

   case CL_DEVICE_NATIVE_VECTOR_WIDTH_HALF:
      buf.as_scalar<cl_uint>() = 0;
      break;

   case CL_DEVICE_OPENCL_C_VERSION:
      buf.as_string() = "OpenCL C 1.1";
      break;

   case CL_DEVICE_PARENT_DEVICE:
      buf.as_scalar<cl_device_id>() = NULL;
      break;

   case CL_DEVICE_PARTITION_MAX_SUB_DEVICES:
      buf.as_scalar<cl_uint>() = 0;
      break;

   case CL_DEVICE_PARTITION_PROPERTIES:
      buf.as_vector<cl_device_partition_property>() =
         desc(property_list<cl_device_partition_property>());
      break;

   case CL_DEVICE_PARTITION_AFFINITY_DOMAIN:
      buf.as_scalar<cl_device_affinity_domain>() = 0;
      break;

   case CL_DEVICE_PARTITION_TYPE:
      buf.as_vector<cl_device_partition_property>() =
         desc(property_list<cl_device_partition_property>());
      break;

   case CL_DEVICE_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = 1;
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}
