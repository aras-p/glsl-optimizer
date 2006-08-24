#!/usr/bin/env python

# (C) Copyright IBM Corporation 2004, 2005
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

class PrintGlOffsets(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "gl_offsets.py (from Mesa)"
		self.header_tag = '_GLAPI_OFFSETS_H_'
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")
		return

	def printBody(self, api):
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
			print '#define _gloffset_%s %d' % (f.name, f.offset)
			last_static = f.offset

		print ''
		print '#if !defined(IN_DRI_DRIVER)'
		print ''

		for [f, index] in functions:
			print '#define _gloffset_%s %d' % (f.name, f.offset)

		print '#define _gloffset_FIRST_DYNAMIC %d' % (api.next_offset)

		print ''
		print '#else'
		print ''

		for [f, index] in functions:
			print '#define _gloffset_%s driDispatchRemapTable[%s_remap_index]' % (f.name, f.name)

		print ''
		print '#endif /* !defined(IN_DRI_DRIVER) */'

		return


def show_usage():
	print "Usage: %s [-f input_file_name]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	api = gl_XML.parse_GL_API( file_name )

	printer = PrintGlOffsets()
	printer.Print( api )
