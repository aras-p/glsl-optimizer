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
#include "core/program.hpp"

using namespace clover;

PUBLIC cl_program
clCreateProgramWithSource(cl_context d_ctx, cl_uint count,
                          const char **strings, const size_t *lengths,
                          cl_int *errcode_ret) try {
   auto &ctx = obj(d_ctx);
   std::string source;

   if (!count || !strings ||
       any_of(is_zero(), range(strings, count)))
      throw error(CL_INVALID_VALUE);

   // Concatenate all the provided fragments together
   for (unsigned i = 0; i < count; ++i)
         source += (lengths && lengths[i] ?
                    std::string(strings[i], strings[i] + lengths[i]) :
                    std::string(strings[i]));

   // ...and create a program object for them.
   ret_error(errcode_ret, CL_SUCCESS);
   return new program(ctx, source);

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_program
clCreateProgramWithBinary(cl_context d_ctx, cl_uint n,
                          const cl_device_id *d_devs, const size_t *lengths,
                          const unsigned char **binaries, cl_int *status_ret,
                          cl_int *errcode_ret) try {
   auto &ctx = obj(d_ctx);
   auto devs = objs(d_devs, n);

   if (!lengths || !binaries)
      throw error(CL_INVALID_VALUE);

   if (any_of([&](device &dev) {
            return !ctx.has_device(dev);
         }, devs))
      throw error(CL_INVALID_DEVICE);

   // Deserialize the provided binaries,
   auto modules = map(
      [](const unsigned char *p, size_t l) -> std::pair<cl_int, module> {
         if (!p || !l)
            return { CL_INVALID_VALUE, {} };

         try {
            compat::istream::buffer_t bin(p, l);
            compat::istream s(bin);

            return { CL_SUCCESS, module::deserialize(s) };

         } catch (compat::istream::error &e) {
            return { CL_INVALID_BINARY, {} };
         }
      },
      range(binaries, n),
      range(lengths, n));

   // update the status array,
   if (status_ret)
      copy(map(keys(), modules), status_ret);

   if (any_of(key_equals(CL_INVALID_VALUE), modules))
      throw error(CL_INVALID_VALUE);

   if (any_of(key_equals(CL_INVALID_BINARY), modules))
      throw error(CL_INVALID_BINARY);

   // initialize a program object with them.
   ret_error(errcode_ret, CL_SUCCESS);
   return new program(ctx, map(addresses(), devs), map(values(), modules));

} catch (error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clRetainProgram(cl_program prog) {
   if (!prog)
      return CL_INVALID_PROGRAM;

   prog->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseProgram(cl_program prog) {
   if (!prog)
      return CL_INVALID_PROGRAM;

   if (prog->release())
      delete prog;

   return CL_SUCCESS;
}

PUBLIC cl_int
clBuildProgram(cl_program prog, cl_uint count, const cl_device_id *devs,
               const char *opts, void (*pfn_notify)(cl_program, void *),
               void *user_data) try {
   if (!prog)
      throw error(CL_INVALID_PROGRAM);

   if (bool(count) != bool(devs) ||
       (!pfn_notify && user_data))
      throw error(CL_INVALID_VALUE);

   if (!opts)
      opts = "";

   if (devs) {
      if (any_of([&](const cl_device_id dev) {
               return !prog->ctx.has_device(obj(dev));
            }, range(devs, count)))
         throw error(CL_INVALID_DEVICE);

      prog->build(map(addresses(), objs(devs, count)), opts);
   } else {
      prog->build(prog->ctx.devs, opts);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clUnloadCompiler() {
   return CL_SUCCESS;
}

PUBLIC cl_int
clGetProgramInfo(cl_program prog, cl_program_info param,
                 size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   if (!prog)
      return CL_INVALID_PROGRAM;

   switch (param) {
   case CL_PROGRAM_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = prog->ref_count();
      break;

   case CL_PROGRAM_CONTEXT:
      buf.as_scalar<cl_context>() = &prog->ctx;
      break;

   case CL_PROGRAM_NUM_DEVICES:
      buf.as_scalar<cl_uint>() = prog->binaries().size();
      break;

   case CL_PROGRAM_DEVICES:
      buf.as_vector<cl_device_id>() = map(keys(), prog->binaries());
      break;

   case CL_PROGRAM_SOURCE:
      buf.as_string() = prog->source();
      break;

   case CL_PROGRAM_BINARY_SIZES:
      buf.as_vector<size_t>() =
         map([](const std::pair<device *, module> &ent) {
               compat::ostream::buffer_t bin;
               compat::ostream s(bin);
               ent.second.serialize(s);
               return bin.size();
            },
            prog->binaries());
      break;

   case CL_PROGRAM_BINARIES:
      buf.as_matrix<unsigned char>() =
         map([](const std::pair<device *, module> &ent) {
               compat::ostream::buffer_t bin;
               compat::ostream s(bin);
               ent.second.serialize(s);
               return bin;
            },
            prog->binaries());
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clGetProgramBuildInfo(cl_program prog, cl_device_id dev,
                      cl_program_build_info param,
                      size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   if (!prog)
      return CL_INVALID_PROGRAM;

   if (!prog->ctx.has_device(obj(dev)))
      return CL_INVALID_DEVICE;

   switch (param) {
   case CL_PROGRAM_BUILD_STATUS:
      buf.as_scalar<cl_build_status>() = prog->build_status(pobj(dev));
      break;

   case CL_PROGRAM_BUILD_OPTIONS:
      buf.as_string() = prog->build_opts(pobj(dev));
      break;

   case CL_PROGRAM_BUILD_LOG:
      buf.as_string() = prog->build_log(pobj(dev));
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}
