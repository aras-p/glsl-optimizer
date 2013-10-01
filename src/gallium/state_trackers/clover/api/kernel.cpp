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
#include "core/kernel.hpp"
#include "core/event.hpp"

using namespace clover;

PUBLIC cl_kernel
clCreateKernel(cl_program prog, const char *name,
               cl_int *errcode_ret) try {
   if (!prog)
      throw error(CL_INVALID_PROGRAM);

   if (!name)
      throw error(CL_INVALID_VALUE);

   if (prog->binaries().empty())
      throw error(CL_INVALID_PROGRAM_EXECUTABLE);

   auto sym = prog->binaries().begin()->second.sym(name);

   ret_error(errcode_ret, CL_SUCCESS);
   return new kernel(*prog, name, { sym.args.begin(), sym.args.end() });

} catch (module::noent_error &e) {
   ret_error(errcode_ret, CL_INVALID_KERNEL_NAME);
   return NULL;

} catch(error &e) {
   ret_error(errcode_ret, e);
   return NULL;
}

PUBLIC cl_int
clCreateKernelsInProgram(cl_program prog, cl_uint count,
                         cl_kernel *kerns, cl_uint *count_ret) {
   if (!prog)
      throw error(CL_INVALID_PROGRAM);

   if (prog->binaries().empty())
      throw error(CL_INVALID_PROGRAM_EXECUTABLE);

   auto &syms = prog->binaries().begin()->second.syms;

   if (kerns && count < syms.size())
      throw error(CL_INVALID_VALUE);

   if (kerns)
      std::transform(syms.begin(), syms.end(), kerns,
                     [=](const module::symbol &sym) {
                        return new kernel(*prog, compat::string(sym.name),
                                          { sym.args.begin(), sym.args.end() });
                     });

   if (count_ret)
      *count_ret = syms.size();

   return CL_SUCCESS;
}

PUBLIC cl_int
clRetainKernel(cl_kernel kern) {
   if (!kern)
      return CL_INVALID_KERNEL;

   kern->retain();
   return CL_SUCCESS;
}

PUBLIC cl_int
clReleaseKernel(cl_kernel kern) {
   if (!kern)
      return CL_INVALID_KERNEL;

   if (kern->release())
      delete kern;

   return CL_SUCCESS;
}

PUBLIC cl_int
clSetKernelArg(cl_kernel kern, cl_uint idx, size_t size,
               const void *value) try {
   if (!kern)
      throw error(CL_INVALID_KERNEL);

   if (idx >= kern->args.size())
      throw error(CL_INVALID_ARG_INDEX);

   kern->args[idx]->set(size, value);

   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clGetKernelInfo(cl_kernel kern, cl_kernel_info param,
                size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   if (!kern)
      return CL_INVALID_KERNEL;

   switch (param) {
   case CL_KERNEL_FUNCTION_NAME:
      buf.as_string() = kern->name();
      break;

   case CL_KERNEL_NUM_ARGS:
      buf.as_scalar<cl_uint>() = kern->args.size();
      break;

   case CL_KERNEL_REFERENCE_COUNT:
      buf.as_scalar<cl_uint>() = kern->ref_count();
      break;

   case CL_KERNEL_CONTEXT:
      buf.as_scalar<cl_context>() = &kern->prog.ctx;
      break;

   case CL_KERNEL_PROGRAM:
      buf.as_scalar<cl_program>() = &kern->prog;
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

PUBLIC cl_int
clGetKernelWorkGroupInfo(cl_kernel kern, cl_device_id dev,
                         cl_kernel_work_group_info param,
                         size_t size, void *r_buf, size_t *r_size) try {
   property_buffer buf { r_buf, size, r_size };

   if (!kern)
      return CL_INVALID_KERNEL;

   if ((!dev && kern->prog.binaries().size() != 1) ||
       (dev && !kern->prog.binaries().count(pobj(dev))))
      return CL_INVALID_DEVICE;

   switch (param) {
   case CL_KERNEL_WORK_GROUP_SIZE:
      buf.as_scalar<size_t>() = kern->max_block_size();
      break;

   case CL_KERNEL_COMPILE_WORK_GROUP_SIZE:
      buf.as_vector<size_t>() = kern->block_size();
      break;

   case CL_KERNEL_LOCAL_MEM_SIZE:
      buf.as_scalar<cl_ulong>() = kern->mem_local();
      break;

   case CL_KERNEL_PREFERRED_WORK_GROUP_SIZE_MULTIPLE:
      buf.as_scalar<size_t>() = 1;
      break;

   case CL_KERNEL_PRIVATE_MEM_SIZE:
      buf.as_scalar<cl_ulong>() = kern->mem_private();
      break;

   default:
      throw error(CL_INVALID_VALUE);
   }

   return CL_SUCCESS;

} catch (error &e) {
   return e.get();
}

namespace {
   ///
   /// Common argument checking shared by kernel invocation commands.
   ///
   void
   kernel_validate(cl_command_queue q, cl_kernel kern,
                   cl_uint dims, const size_t *grid_offset,
                   const size_t *grid_size, const size_t *block_size,
                   cl_uint num_deps, const cl_event *deps,
                   cl_event *ev) {
      if (!q)
         throw error(CL_INVALID_COMMAND_QUEUE);

      if (!kern)
         throw error(CL_INVALID_KERNEL);

      if (&kern->prog.ctx != &q->ctx ||
          any_of([&](const cl_event ev) {
                return &obj(ev).ctx != &q->ctx;
             }, range(deps, num_deps)))
         throw error(CL_INVALID_CONTEXT);

      if (bool(num_deps) != bool(deps) ||
          any_of(is_zero(), range(deps, num_deps)))
         throw error(CL_INVALID_EVENT_WAIT_LIST);

      if (any_of([](std::unique_ptr<kernel::argument> &arg) {
               return !arg->set();
            }, kern->args))
         throw error(CL_INVALID_KERNEL_ARGS);

      if (!kern->prog.binaries().count(&q->dev))
         throw error(CL_INVALID_PROGRAM_EXECUTABLE);

      if (dims < 1 || dims > q->dev.max_block_size().size())
         throw error(CL_INVALID_WORK_DIMENSION);

      if (!grid_size || any_of(is_zero(), range(grid_size, dims)))
         throw error(CL_INVALID_GLOBAL_WORK_SIZE);

      if (block_size) {
         if (any_of([](size_t b, size_t max) {
                  return b == 0 || b > max;
               }, range(block_size, dims),
               q->dev.max_block_size()))
            throw error(CL_INVALID_WORK_ITEM_SIZE);

         if (any_of(modulus(), range(grid_size, dims),
                    range(block_size, dims)))
            throw error(CL_INVALID_WORK_GROUP_SIZE);

         if (fold(multiplies(), 1u, range(block_size, dims)) >
             q->dev.max_threads_per_block())
            throw error(CL_INVALID_WORK_GROUP_SIZE);
      }
   }

   ///
   /// Common event action shared by kernel invocation commands.
   ///
   std::function<void (event &)>
   kernel_op(cl_command_queue q, cl_kernel kern,
             const std::vector<size_t> &grid_offset,
             const std::vector<size_t> &grid_size,
             const std::vector<size_t> &block_size) {
      const std::vector<size_t> reduced_grid_size =
         map(divides(), grid_size, block_size);

      return [=](event &) {
         kern->launch(*q, grid_offset, reduced_grid_size, block_size);
      };
   }

   std::vector<size_t>
   opt_vector(const size_t *p, unsigned n, size_t x) {
      if (p)
         return { p, p + n };
      else
         return { n, x };
   }
}

PUBLIC cl_int
clEnqueueNDRangeKernel(cl_command_queue q, cl_kernel kern,
                       cl_uint dims, const size_t *pgrid_offset,
                       const size_t *pgrid_size, const size_t *pblock_size,
                       cl_uint num_deps, const cl_event *d_deps,
                       cl_event *ev) try {
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   auto grid_offset = opt_vector(pgrid_offset, dims, 0);
   auto grid_size = opt_vector(pgrid_size, dims, 1);
   auto block_size = opt_vector(pblock_size, dims, 1);

   kernel_validate(q, kern, dims, pgrid_offset, pgrid_size, pblock_size,
                   num_deps, d_deps, ev);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_NDRANGE_KERNEL, deps,
      kernel_op(q, kern, grid_offset, grid_size, block_size));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueTask(cl_command_queue q, cl_kernel kern,
              cl_uint num_deps, const cl_event *d_deps,
              cl_event *ev) try {
   auto deps = objs<wait_list_tag>(d_deps, num_deps);
   const std::vector<size_t> grid_offset = { 0 };
   const std::vector<size_t> grid_size = { 1 };
   const std::vector<size_t> block_size = { 1 };

   kernel_validate(q, kern, 1, grid_offset.data(), grid_size.data(),
                   block_size.data(), num_deps, d_deps, ev);

   hard_event *hev = new hard_event(
      *q, CL_COMMAND_TASK, deps,
      kernel_op(q, kern, grid_offset, grid_size, block_size));

   ret_object(ev, hev);
   return CL_SUCCESS;

} catch(error &e) {
   return e.get();
}

PUBLIC cl_int
clEnqueueNativeKernel(cl_command_queue q, void (*func)(void *),
                      void *args, size_t args_size,
                      cl_uint obj_count, const cl_mem *obj_list,
                      const void **obj_args, cl_uint num_deps,
                      const cl_event *deps, cl_event *ev) {
   return CL_INVALID_OPERATION;
}
