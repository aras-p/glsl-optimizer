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

#include "core/program.hpp"
#include "core/compiler.hpp"

using namespace clover;

program::program(clover::context &ctx, const std::string &source) :
   has_source(true), context(ctx), _source(source) {
}

program::program(clover::context &ctx,
                 const ref_vector<device> &devs,
                 const std::vector<module> &binaries) :
   has_source(false), context(ctx),
   _devices(devs) {
   for_each([&](device &dev, const module &bin) {
         _binaries.insert({ &dev, bin });
      },
      devs, binaries);
}

void
program::build(const ref_vector<device> &devs, const char *opts) {
   if (has_source) {
      _devices = devs;

      for (auto &dev : devs) {
         _binaries.erase(&dev);
         _logs.erase(&dev);
         _opts.erase(&dev);

         _opts.insert({ &dev, opts });

         try {
            auto module = (dev.ir_format() == PIPE_SHADER_IR_TGSI ?
                           compile_program_tgsi(_source) :
                           compile_program_llvm(_source, dev.ir_format(),
                                                dev.ir_target(), build_opts(dev)));
            _binaries.insert({ &dev, module });

         } catch (build_error &e) {
            _logs.insert({ &dev, e.what() });
            throw;
         }
      }
   }
}

const std::string &
program::source() const {
   return _source;
}

program::device_range
program::devices() const {
   return map(evals(), _devices);
}

const module &
program::binary(const device &dev) const {
   return _binaries.find(&dev)->second;
}

cl_build_status
program::build_status(const device &dev) const {
   if (_binaries.count(&dev))
      return CL_BUILD_SUCCESS;
   else
      return CL_BUILD_NONE;
}

std::string
program::build_opts(const device &dev) const {
   return _opts.count(&dev) ? _opts.find(&dev)->second : "";
}

std::string
program::build_log(const device &dev) const {
   return _logs.count(&dev) ? _logs.find(&dev)->second : "";
}

const compat::vector<module::symbol> &
program::symbols() const {
   if (_binaries.empty())
      throw error(CL_INVALID_PROGRAM_EXECUTABLE);

   return _binaries.begin()->second.syms;
}
