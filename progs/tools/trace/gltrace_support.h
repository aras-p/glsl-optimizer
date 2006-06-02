// -*- c++ -*-   (emacs c++ mode)
/*
 * Copyright (C) 2006  Thomas Sondergaard   All Rights Reserved.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included
 * in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
 * AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
#ifndef GLTRACE_SUPPORT_H
#define GLTRACE_SUPPORT_H

#include <string>
#include <iostream>
#include <memory>

namespace gltrace {

  const int MAX_STACKFRAMES = 100;

  /// Returns the stack trace of the current thread
  std::string getStackTrace(int count = MAX_STACKFRAMES, int first = 0);
  
  std::ostream &timeNow(std::ostream &os);

  struct logstream : public std::ostream {
    
    /// Opens a logstream - if filename is null, stderr will be used
    logstream(const char *filename = 0);
    
  private:
    std::auto_ptr<std::ofstream> file_os;
  };

  struct Config {
    bool logCalls;
    bool checkErrors;
    bool logTime;
    logstream log;
    
    Config();
  };

  extern Config config;

} // namespace gltrace

#define GLTRACE_LOG(x) \
   { if (config.logTime) config.log << timeNow << ": "; config.log << x << "\n"; }

#endif // GLTRACE_SUPPORT_H


