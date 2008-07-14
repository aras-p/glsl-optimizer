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

#include "gltrace_support.h"
#include <cstdlib>
#include <cstring>
#include <assert.h>
#include <sstream>
#include <fstream>
#include <iomanip>
#include <execinfo.h>
#include <cxxabi.h>
#include <sys/time.h>

namespace {

  const char *
  demangle (const char * mangled) throw()
  {
    static char buf[4096];
    int status;
    size_t length = sizeof(buf)-1;
    
    memset (buf, 0, sizeof(buf));
    
    if (!mangled)
      return 0;
    
    char * demangled =  __cxxabiv1::__cxa_demangle(mangled,
                                                   buf,
                                                   &length, 
                                                   &status);
    if (demangled && !status)
      return demangled;
    else
      return mangled;    
  }

  void
  printStackTrace (void **stackframes,
		   int stackframe_size,
		   std::ostream & out  ) 
  {
    char **strings = 0;
    std::stringstream ss; 
    
    // this might actually fail if memory is tight or we are in a
    // signal handler
    strings = backtrace_symbols (stackframes, stackframe_size);
    
    ss <<  "Backtrace :";
    
    if (stackframe_size == gltrace::MAX_STACKFRAMES)
      ss << "(possibly incomplete maximal number of frames exceeded):" << std::endl;
    else
      ss << std::endl;
    
    out << ss.str();
    
    // the first frame is the constructor of the exception
    // the last frame always seem to be bogus?
    for (int i = 0; strings && i < stackframe_size-1; ++i) {
      char libname[257], funcname[2049];
      unsigned int address=0, funcoffset = 0x0;
      
      memset (libname,0,sizeof(libname));
      memset (funcname,0,sizeof(funcname));
      
      strcpy (funcname,"??");
      strcpy (libname, "??");
      
      int scanned = sscanf (strings[i], "%256[^(] ( %2048[^+] + %x ) [ %x ]",
			    libname,
			    funcname,
			    &funcoffset, 
			    &address);
      
      /* ok, so no function was mentioned in the backtrace */
      if (scanned < 4) {
	scanned = sscanf (strings[i], "%256[^([] [ %x ]",
			  libname,
			  &address);
      }
      
      if (funcname[0] == '_') {
	const char * demangled; 
	if ((demangled = demangle(funcname) ) != funcname) {
	  strncpy (funcname, demangled, sizeof(funcname)-1); 
	}
      }
      else
	strcat (funcname," ()");
      
      out << "\t#" << i << std::hex << " 0x" << address << " in " << funcname
	  << " at 0x" << funcoffset << " (from " << libname << ")" << std::endl;                       
    }
    
    free (strings);
  }

  
} // anon namespace

namespace gltrace {
  
  std::string getStackTrace(int count, int first) {
    ++first;
    std::stringstream ss; 
    const int BA_MAX = 1000;
    assert(count + first <= BA_MAX);
    void *ba[BA_MAX];
    int n = backtrace(ba, count+first);
    
    printStackTrace( &ba[first], n-first, ss);
    
    return ss.str();
  }

  std::ostream &timeNow(std::ostream &os) {

    struct timeval now;
    struct tm t;
    static char const *months[12] = 
      { 
	"Jan", "Feb", "Mar", "Apr", "May", "Jun", 
	"Jul", "Aug", "Sep", "Oct", "Nov", "Dec" 
      };
    
    gettimeofday (&now, 0);
    localtime_r  ((time_t*) &now.tv_sec, &t);
    
    os 
      << months[t.tm_mon] << " " 
      << std::setw(2) << t.tm_mday << " " 
      << std::setw(2) << t.tm_hour << ":" 
      << std::setw(2) << t.tm_min  << ":" 
      << std::setw(2) << t.tm_sec  << "." 
      << std::setw(3) << now.tv_usec/1000;
    return os;
  }

  logstream::logstream(const char *filename) {
    if (!filename)
      init(std::cerr.rdbuf());
    else {
      file_os.reset(new std::ofstream(filename));
      if (file_os->good()) 
	init(file_os->rdbuf());
      else {
	std::cerr << "ERROR: gltrace: Failed to open '" << filename 
		  <<  "' for writing. Falling back to stderr." << std::endl;
	init(std::cerr.rdbuf());
      }
    }
    *this << std::setfill('0'); // setw used in timeNow
  }


  Config::Config() : 
    logCalls(true), 
    checkErrors(true),
    logTime(true),
    log(getenv("GLTRACE_LOGFILE")) {
    if (const char *v = getenv("GLTRACE_LOG_CALLS"))
      logCalls = strncmp("1", v, 1) == 0;
    if (const char *v = getenv("GLTRACE_CHECK_ERRORS"))
      checkErrors = strncmp("1", v, 1) == 0;
    if (const char *v = getenv("GLTRACE_LOG_TIME"))
      logTime = strncmp("1", v, 1) == 0;
  }

  // *The* config
  Config config;

} // namespace gltrace
