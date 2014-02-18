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

#ifndef CLOVER_CORE_PROGRAM_HPP
#define CLOVER_CORE_PROGRAM_HPP

#include <map>

#include "core/object.hpp"
#include "core/context.hpp"
#include "core/module.hpp"

namespace clover {
   class program : public ref_counter, public _cl_program {
   private:
      typedef adaptor_range<
         evals, const std::vector<intrusive_ref<device>> &> device_range;

   public:
      program(clover::context &ctx,
              const std::string &source);
      program(clover::context &ctx,
              const ref_vector<device> &devs,
              const std::vector<module> &binaries);

      program(const program &prog) = delete;
      program &
      operator=(const program &prog) = delete;

      void build(const ref_vector<device> &devs, const char *opts);

      const bool has_source;
      const std::string &source() const;

      device_range devices() const;

      const module &binary(const device &dev) const;
      cl_build_status build_status(const device &dev) const;
      std::string build_opts(const device &dev) const;
      std::string build_log(const device &dev) const;

      const compat::vector<module::symbol> &symbols() const;

      const intrusive_ref<clover::context> context;

   private:
      std::vector<intrusive_ref<device>> _devices;
      std::map<const device *, module> _binaries;
      std::map<const device *, std::string> _logs;
      std::map<const device *, std::string> _opts;
      std::string _source;
   };
}

#endif
