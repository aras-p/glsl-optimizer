#!/usr/bin/python2

# (C) Copyright IBM Corporation 2004
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
#    Ian Romanick <idr@us.ibm.com>

import gl_XML
import license
import sys, getopt

class PrintGlTable(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.header_tag = '_GLAPI_TABLE_H_'
		self.name = "gl_table.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
		return


	def printBody(self, api):
		for f in api.functionIterateByOffset():
			arg_string = f.get_parameter_string()
			print '   %s (GLAPIENTRYP %s)(%s); /* %d */' % (f.return_type, f.name, arg_string, f.offset)


	def printRealHeader(self):
		print '#ifndef GLAPIENTRYP'
		print '# ifndef GLAPIENTRY'
		print '#  define GLAPIENTRY'
		print '# endif'
		print ''
		print '# define GLAPIENTRYP GLAPIENTRY *'
		print '#endif'
		print ''
		print ''
		print 'struct _glapi_table'
		print '{'
		return


	def printRealFooter(self):
		print '};'
		return


class PrintRemapTable(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.header_tag = '_DISPATCH_H_'
		self.name = "gl_table.py (from Mesa)"
		self.license = license.bsd_license_template % ("(C) Copyright IBM Corporation 2005", "IBM")
		return


	def printRealHeader(self):
		print """
#include "glapitable.h"
/**
 * \\file dispatch.h
 * Macros for handling GL dispatch tables.
 *
 * For each known GL function, there are 3 macros in this file.  The first
 * macro is named CALL_FuncName and is used to call that GL function using
 * the specified dispatch table.  The other 2 macros, called GET_FuncName
 * can SET_FuncName, are used to get and set the dispatch pointer for the
 * named function in the specified dispatch table.
 */
"""
		
		return

	def printBody(self, api):
		print '#define CALL_by_offset(disp, cast, offset, parameters) \\'
		print '    (*(cast (GET_by_offset(disp, offset)))) parameters'
		print '#define GET_by_offset(disp, offset) \\'
		print '    (offset >= 0) ? (((_glapi_proc *)(disp))[offset]) : NULL'
		print '#define SET_by_offset(disp, offset, fn) \\'
		print '    do { \\'
		print '        if ( (offset) < 0 ) { \\'
		print '            /* fprintf( stderr, "[%s:%u] SET_by_offset(%p, %d, %s)!\\n", */ \\'
		print '            /*         __func__, __LINE__, disp, offset, # fn); */ \\'
		print '            /* abort(); */ \\'
		print '        } \\'
		print '        else { \\'
		print '            ( (_glapi_proc *) (disp) )[offset] = (_glapi_proc) fn; \\'
		print '        } \\'
		print '    } while(0)'
		print ''

		abi = [ "1.0", "1.1", "1.2", "GL_ARB_multitexture" ]

		functions = []
		abi_functions = []
		count = 0
		for f in api.functionIterateByOffset():
			[category, num] = api.get_category_for_name( f.name )
			if category not in abi:
				functions.append( [f, count] )
				count += 1
			else:
				abi_functions.append( f )


		for f in abi_functions:
			print '#define CALL_%s(disp, parameters) (*((disp)->%s)) parameters' % (f.name, f.name)
			print '#define GET_%s(disp) ((disp)->%s)' % (f.name, f.name)
			print '#define SET_%s(disp, fn) ((disp)->%s = fn)' % (f.name, f.name)


		print ''
		print '#if !defined(IN_DRI_DRIVER)'
		print ''

		for [f, index] in functions:
			print '#define CALL_%s(disp, parameters) (*((disp)->%s)) parameters' % (f.name, f.name)
			print '#define GET_%s(disp) ((disp)->%s)' % (f.name, f.name)
			print '#define SET_%s(disp, fn) ((disp)->%s = fn)' % (f.name, f.name)

		print ''
		print '#else'
		print ''
		print '#define driDispatchRemapTable_size %u' % (count)
		print 'extern int driDispatchRemapTable[ driDispatchRemapTable_size ];'
		print ''

		for [f, index] in functions:
			print '#define %s_remap_index %u' % (f.name, index)

		print ''

		for [f, index] in functions:
			arg_string = gl_XML.create_parameter_string( f.parameters, 0 )
			cast = '%s (GLAPIENTRYP)(%s)' % (f.return_type, arg_string)

			print '#define CALL_%s(disp, parameters) CALL_by_offset(disp, (%s), driDispatchRemapTable[%s_remap_index], parameters)' % (f.name, cast, f.name)
			print '#define GET_%s(disp) GET_by_offset(disp, driDispatchRemapTable[%s_remap_index])' % (f.name, f.name)
			print '#define SET_%s(disp, fn) SET_by_offset(disp, driDispatchRemapTable[%s_remap_index], fn)' % (f.name, f.name)


		print ''
		print '#endif /* !defined(IN_DRI_DRIVER) */'
		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-m mode]" % sys.argv[0]
	print "    -m mode   Mode can be 'table' or 'remap_table'."
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:")
	except Exception,e:
		show_usage()

	mode = "table"
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val

	if mode == "table":
		printer = PrintGlTable()
	elif mode == "remap_table":
		printer = PrintRemapTable()
	else:
		show_usage()

	api = gl_XML.parse_GL_API( file_name )

	printer.Print( api )
