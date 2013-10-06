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
#include "util/algorithm.hpp"

using namespace clover;

_cl_program::_cl_program(clover::context &ctx,
                         const std::string &source) :
   ctx(ctx), _source(source) {
}

_cl_program::_cl_program(clover::context &ctx,
                         const std::vector<clover::device *> &devs,
                         const std::vector<clover::module> &binaries) :
   ctx(ctx) {
   for_each([&](clover::device *dev, const clover::module &bin) {
         _binaries.insert({ dev, bin });
      },
      devs, binaries);
}

void
_cl_program::build(const std::vector<clover::device *> &devs,
                   const char *opts) {

   for (auto dev : devs) {
      _binaries.erase(dev);
      _logs.erase(dev);
      _opts.erase(dev);

      _opts.insert({ dev, opts });
      try {
         auto module = (dev->ir_format() == PIPE_SHADER_IR_TGSI ?
                        compile_program_tgsi(_source) :
                        compile_program_llvm(_source, dev->ir_format(),
                        dev->ir_target(), build_opts(dev)));
         _binaries.insert({ dev, module });

      } catch (build_error &e) {
         _logs.insert({ dev, e.what() });
         throw error(CL_BUILD_PROGRAM_FAILURE);
      } catch (invalid_option_error &e) {
         throw error(CL_INVALID_BUILD_OPTIONS);
      }
   }
}

const std::string &
_cl_program::source() const {
   return _source;
}

const std::map<clover::device *, clover::module> &
_cl_program::binaries() const {
   return _binaries;
}

cl_build_status
_cl_program::build_status(clover::device *dev) const {
   return _binaries.count(dev) ? CL_BUILD_SUCCESS : CL_BUILD_NONE;
}

std::string
_cl_program::build_opts(clover::device *dev) const {
   return _opts.count(dev) ? _opts.find(dev)->second : "";
}

std::string
_cl_program::build_log(clover::device *dev) const {
   return _logs.count(dev) ? _logs.find(dev)->second : "";
}
