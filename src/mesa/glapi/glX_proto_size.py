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

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces

import gl_XML
import glX_XML
import license
import sys, getopt, copy


class SizeStubFunctionIterator:
	"""Iterate over functions that need "size" information.
	
	Iterate over the functions that have variable sized data.  First the
	"set"-type functions are iterated followed by the "get"-type
	functions.
	"""

	def __init__(self, context):
		self.data = []
		self.index = 0

		set_functions = []
		get_functions = []
		extra_data = []

		for f in gl_XML.glFunctionIterator(context):
			if f.fn_offset < 0: break

			if context.glx_enum_functions.has_key(f.name):
				ef = context.glx_enum_functions[f.name]
				if ef.is_set():
					set_functions.append(f)
				else:
					get_functions.append(f)


		if (context.which_functions & PrintGlxSizeStubs_c.do_set) != 0:
			self.data += set_functions
		elif context.get_alias_set:
			extra_data = set_functions

		if (context.which_functions & PrintGlxSizeStubs_c.do_get) != 0:
			self.data += get_functions


		for f in extra_data + self.data:
			sig = context.glx_enum_functions[f.name].signature()

			if not context.glx_enum_sigs.has_key(sig):
				context.glx_enum_sigs[sig] = f.name;

		return


	def __iter__(self):
		return self


	def next(self):
		if self.index == len(self.data):
			raise StopIteration

		f = self.data[ self.index ]
		self.index += 1

		return f


class PrintGlxSizeStubs_common(glX_XML.GlxProto):
	do_get = (1 << 0)
	do_set = (1 << 1)
	do_get_alias_set = (1 << 2)

	def __init__(self, which_functions):
		glX_XML.GlxProto.__init__(self)
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004", "IBM")
		self.aliases = []
		self.glx_enum_sigs = {}
		self.name = "glX_proto_size.py (from Mesa)"
		self.which_functions = which_functions

		if (((which_functions & PrintGlxSizeStubs_common.do_set) != 0) and ((which_functions & PrintGlxSizeStubs_common.do_get) != 0)) or ((which_functions & PrintGlxSizeStubs_common.do_get_alias_set) != 0):
			self.get_alias_set = 1
		else:
			self.get_alias_set = 0


	def functionIterator(self):
		return SizeStubFunctionIterator(self)


class PrintGlxSizeStubs_c(PrintGlxSizeStubs_common):
	def printRealHeader(self):
		print ''
		print '#include <GL/gl.h>'
		print '#include "indirect_size.h"'
		
		print ''
		glX_XML.printHaveAlias()
		print ''
		glX_XML.printPure()
		print ''
		glX_XML.printFastcall()
		print ''
		glX_XML.printVisibility( "INTERNAL", "internal" )
		print ''
		print ''
		print '#ifdef HAVE_ALIAS'
		print '#  define ALIAS2(from,to) \\'
		print '    INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \\'
		print '        __attribute__ ((alias( # to )));'
		print '#  define ALIAS(from,to) ALIAS2( from, __gl ## to ## _size )'
		print '#else'
		print '#  define ALIAS(from,to) \\'
		print '    INTERNAL PURE FASTCALL GLint __gl ## from ## _size( GLenum e ) \\'
		print '    { return __gl ## to ## _size( e ); }'
		print '#endif'
		print ''
		print ''


	def printRealFooter(self):
		for a in self.aliases:
			print a


	def printFunction(self, f):
		ef = self.glx_enum_functions[f.name]
		n = self.glx_enum_sigs[ ef.signature() ];

		if n != f.name:
			a = 'ALIAS( %s, %s )' % (f.name, n)
			self.aliases.append(a)
		else:
			ef.Print( f.name )


				
class PrintGlxSizeStubs_h(PrintGlxSizeStubs_common):
	def printRealHeader(self):
		print """/**
 * \\file
 * Prototypes for functions used to determine the number of data elements in
 * various GLX protocol messages.
 *
 * \\author Ian Romanick <idr@us.ibm.com>
 */
"""
		glX_XML.printPure();
		print ''
		glX_XML.printFastcall();
		print ''
		glX_XML.printVisibility( "INTERNAL", "internal" );
		print ''

	def printRealFooter(self):
		print ''
		print "#  undef INTERNAL"
		print "#  undef PURE"
		print "#  undef FASTCALL"


	def printFunction(self, f):
		ef = self.glx_enum_functions[f.name]
		print 'extern INTERNAL PURE FASTCALL GLint __gl%s_size(GLenum);' % (f.name)


def show_usage():
	print "Usage: %s [-f input_file_name] -m output_mode [--only-get | --only-set] [--get-alias-set]" % sys.argv[0]
	print "    -m output_mode   Output mode can be one of 'size_c' or 'size_h'."
	print "    --only-get       Only emit 'get'-type functions."
	print "    --only-set       Only emit 'set'-type functions."
	print "    --get-alias-set  When only 'get'-type functions are emitted, allow them"
	print "                     to be aliases to 'set'-type funcitons."
	print ""
	print "By default, both 'get' and 'set'-type functions are emitted."
	sys.exit(1)


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:h:", ["only-get", "only-set", "get-alias-set", "header-tag"])
	except Exception,e:
		show_usage()

	mode = None
	header_tag = None
	which_functions = PrintGlxSizeStubs_common.do_get | PrintGlxSizeStubs_common.do_set

	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val
		elif arg == "--only-get":
			which_functions = PrintGlxSizeStubs_common.do_get
		elif arg == "--only-set":
			which_functions = PrintGlxSizeStubs_common.do_set
		elif arg == "--get-alias-set":
			which_functions |= PrintGlxSizeStubs_common.do_get_alias_set
		elif (arg == '-h') or (arg == "--header-tag"):
			header_tag = val

	if mode == "size_c":
		dh = PrintGlxSizeStubs_c( which_functions )
	elif mode == "size_h":
		dh = PrintGlxSizeStubs_h( which_functions )
		if header_tag:
			dh.header_tag = header_tag
	else:
		show_usage()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
