#!/usr/bin/env python

# Copyright (C) 2006  Thomas Sondergaard
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# on the rights to use, copy, modify, merge, publish, distribute, sub
# license, and/or sell copies of the Software, and to permit persons to whom
# the Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.  IN NO EVENT SHALL
# IBM AND/OR ITS SUPPLIERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.
#
# Authors:
#    Thomas Sondergaard <ts@medical-insight.com>

import gl_XML, glX_XML, glX_proto_common, license
import sys, getopt, copy, string

def create_argument_string(parameters):
	"""Create a parameter string from a list of gl_parameters."""

	list = []
	for p in parameters:
		list.append( p.name )
	#if len(list) == 0: list = ["void"]

	return string.join(list, ", ")

def create_logfunc_string(func, name):
	"""Create a parameter string from a list of gl_parameters."""

	list = []
	list.append('"gl' + name + '("')
	sep = None
	for p in func.parameters:
		if (sep):
			list.append(sep)
		list.append( p.name )
		sep = '", "'
	list.append('");"')
	#if len(list) == 0: list = ["void"]

	return "if (config.logCalls) GLTRACE_LOG(" + string.join(list, " << ")+");";

class PrintGltrace(glX_proto_common.glx_print_proto): #(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "gltrace.py"
		self.license = license.bsd_license_template % ( \
"""Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
(C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")
		#self.header_tag = "_INDIRECT_H_"

		self.last_category = ""
		return


	def printRealHeader(self):
		print """/**
 * \\file
 * gl and glX wrappers for tracing
 *
 * \\author Thomas Sondergaard <ts@medical-insight.com>
 */
"""
		#self.printVisibility( "HIDDEN", "hidden" )
		#self.printFastcall()
		#self.printNoinline()

		print """
#include <GL/gl.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <dlfcn.h>
#include "gltrace_support.h"

using namespace gltrace;

static GLenum real_glGetError() {
  static GLenum (*real_func)(void) = 0;
  if (!real_func) real_func = (GLenum (*)(void)) dlsym(RTLD_NEXT, "glGetError");
  return real_func();
}

bool betweenGLBeginEnd = false;

extern "C" {


__GLXextFuncPtr real_glXGetProcAddressARB(const GLubyte *func_name) {
  static __GLXextFuncPtr (*real_func)(const GLubyte *func_name) = 0;
  if (!real_func) real_func = (__GLXextFuncPtr (*)(const GLubyte *func_name)) dlsym(RTLD_NEXT, "glXGetProcAddressARB");

  return real_func(func_name);
}

__GLXextFuncPtr glXGetProcAddressARB(const GLubyte *func_name_ubyte) {
  std::string func_name =
    std::string("gltrace_")+reinterpret_cast<const char*>(func_name_ubyte);
  
  __GLXextFuncPtr f = (__GLXextFuncPtr) dlsym(RTLD_DEFAULT, func_name.c_str());
  if (!f) {
    GLTRACE_LOG("warning: Could not resolve '" << func_name << "' - function will not be intercepted");
    return real_glXGetProcAddressARB(func_name_ubyte);
  }
  return f;
}

"""

	def printRealFooter(self):
		print "} // Extern \"C\""

	def printBody(self, api):
		for func in api.functionIterateGlx():
			for func_name in func.entry_points:
				functionPrefix = ""
				use_dlsym = True
				if (api.get_category_for_name(func.name)[1] != None):
					functionPrefix = "gltrace_"
					use_dlsym = False
				
				print '%s %sgl%s(%s) {' % (func.return_type, functionPrefix, func_name, func.get_parameter_string())
				if (use_dlsym):
					print '  static %s (*real_func)(%s) = 0;' % (func.return_type, func.get_parameter_string())
					print '  if (!real_func) real_func = (%s (*)(%s)) dlsym(RTLD_NEXT, "gl%s");' % (func.return_type, func.get_parameter_string(), func_name)
				else: # use glXGetProcAddressArb
					print '  static %s (*real_func)(%s) = 0;' % (func.return_type, func.get_parameter_string())
					print '  if (!real_func) real_func = (%s (*)(%s)) real_glXGetProcAddressARB((GLubyte *)"gl%s");' % (func.return_type, func.get_parameter_string(), func_name)
				print '  ' + create_logfunc_string(func, func_name)
				if (func.return_type == "void"):
					print '  real_func(%s);' % (create_argument_string(func.parameters))
				else:
					print '  %s retval = real_func(%s);' % (func.return_type, create_argument_string(func.parameters))
				if (func.name == "Begin"):
					print '  betweenGLBeginEnd = true;'
				elif (func.name == "End"):
					print '  betweenGLBeginEnd = false;'
				print '  if (!betweenGLBeginEnd && config.checkErrors) {'
				print '    GLenum res;'
				print '    while ((res = real_glGetError ()) != GL_NO_ERROR) '
				print '      GLTRACE_LOG("OpenGL Error (" << res << "): <" << gluErrorString(res) << "> at " << gltrace::getStackTrace());'
				print '  }'
				if (func.return_type != "void"):
					print "  return retval;"
				print '}'


def show_usage():
	print "Usage: %s [-f input_file_name] [-m output_mode] [-d]" % sys.argv[0]
	print "    -m output_mode   Output mode can be one of 'proto', 'init_c' or 'init_h'."
	print "    -d               Enable extra debug information in the generated code."
	sys.exit(1)


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:d")
	except Exception,e:
		show_usage()

	debug = 0
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-d":
			debug = 1

	printer = PrintGltrace()

	printer.debug = debug
	api = gl_XML.parse_GL_API( file_name, glX_XML.glx_item_factory() )

	printer.Print( api )
