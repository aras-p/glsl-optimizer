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

using namespace clover;

namespace {
   platform _clover_platform;
}

CLOVER_API cl_int
clGetPlatformIDs(cl_uint num_entries, cl_platform_id *rd_platforms,
                 cl_uint *rnum_platforms) {
   if ((!num_entries && rd_platforms) ||
       (!rnum_platforms && !rd_platforms))
      return CL_INVALID_VALUE;

   if (rnum_platforms)
      *rnum_platforms = 1;
   if (rd_platforms)
      *rd_platforms = desc(_clover_platform);

   return CL_SUCCESS;
}

cl_int
clover::GetPlatformInfo(cl_platform_id d_platform, cl_platform_info param,
                        size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   obj(d_platform);

   switch (param) {
   case CL_PLATFORM_PROFILE:
      buf.as_string() = "FULL_PROFILE";
      break;

   case CL_PLATFORM_VERSION:
      buf.as_string() = "OpenCL 1.1 MESA " PACKAGE_VERSION;
      break;

   case CL_PLATFORM_NAME:
      buf.as_string() = "Clover";
      break;

   case CL_PLATFORM_VENDOR:
      buf.as_string() = "Mesa";
      break;

   case CL_PLATFORM_EXTENSIONS:
      buf.as_string() = "cl_khr_icd";
      break;

   case CL_PLATFORM_ICD_SUFFIX_KHR:
      buf.as_string() = "MESA";
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

void *
clover::GetExtensionFunctionAddress(const char *p_name) {
   std::string name { p_name };

   if (name == "clIcdGetPlatformIDsKHR")
      return reinterpret_cast<void *>(IcdGetPlatformIDsKHR);
   else
      return NULL;
}

cl_int
clover::IcdGetPlatformIDsKHR(cl_uint num_entries, cl_platform_id *rd_platforms,
                             cl_uint *rnum_platforms) {
   return clGetPlatformIDs(num_entries, rd_platforms, rnum_platforms);
}

CLOVER_ICD_API cl_int
clGetPlatformInfo(cl_platform_id d_platform, cl_platform_info param,
                  size_t size, void *r_buf, size_t *r_size) {
   return GetPlatformInfo(d_platform, param, size, r_buf, r_size);
}

CLOVER_ICD_API void *
clGetExtensionFunctionAddress(const char *p_name) {
   return GetExtensionFunctionAddress(p_name);
}

CLOVER_ICD_API cl_int
clIcdGetPlatformIDsKHR(cl_uint num_entries, cl_platform_id *rd_platforms,
                       cl_uint *rnum_platforms) {
   return IcdGetPlatformIDsKHR(num_entries, rd_platforms, rnum_platforms);
}
