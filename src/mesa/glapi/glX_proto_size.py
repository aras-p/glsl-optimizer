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


class PrintGlxSizeStubs_c(glX_XML.GlxProto):
	def __init__(self):
		glX_XML.GlxProto.__init__(self)
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004", "IBM")
		self.aliases = []
		self.glx_enum_sigs = {}

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
		if self.glx_enum_functions.has_key(f.name):
			ef = self.glx_enum_functions[f.name]

			sig = ef.signature();
			if self.glx_enum_sigs.has_key(sig):
				n = self.glx_enum_sigs[sig];
				a = 'ALIAS( %s, %s )' % (f.name, n)
				self.aliases.append(a)
			else:
				ef.Print( f.name )
				self.glx_enum_sigs[sig] = f.name;


				
class PrintGlxSizeStubs_h(glX_XML.GlxProto):
	def __init__(self):
		glX_XML.GlxProto.__init__(self)
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004", "IBM")
		self.aliases = []
		self.glx_enum_sigs = {}

	def printRealHeader(self):
		print """
/**
 * \\file
 * Prototypes for functions used to determine the number of data elements in
 * various GLX protocol messages.
 *
 * \\author Ian Romanick <idr@us.ibm.com>
 */

#if !defined( _GLXSIZE_H_ )
#  define _GLXSIZE_H_

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
		print "#endif /* !defined( _GLXSIZE_H_ ) */"


	def printFunction(self, f):
		if self.glx_enum_functions.has_key(f.name):
			ef = self.glx_enum_functions[f.name]
			print 'extern INTERNAL PURE FASTCALL GLint __gl%s_size(GLenum);' % (f.name)


def show_usage():
	print "Usage: %s [-f input_file_name] [-m output_mode]" % sys.argv[0]
	sys.exit(1)


if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:m:")
	except Exception,e:
		show_usage()

	mode = "proto"
	for (arg,val) in args:
		if arg == "-f":
			file_name = val
		elif arg == "-m":
			mode = val

	if mode == "size_c":
		dh = PrintGlxSizeStubs_c()
	elif mode == "size_h":
		dh = PrintGlxSizeStubs_h()
	else:
		show_usage()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
