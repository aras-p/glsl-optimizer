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


class SizeStubFunctionIterator(glX_XML.glXFunctionIterator):
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


	def next(self):
		if self.index == len(self.data):
			raise StopIteration

		f = self.data[ self.index ]
		self.index += 1

		return f


class glXServerEnumFunction(glX_XML.glXEnumFunction):
	def signature( self ):
		if self.sig == None:
			sig = glX_XML.glXEnumFunction.signature(self)

			f = self.context.find_function( self.name )
			p = f.variable_length_parameter()

			try:
				sig += "%u" % (p.p_type.size)
			except Exception,e:
				print '%s' % (self.name)
				raise e

			self.sig = sig

		return self.sig;


	def Print(self, name):
		f = self.context.find_function( self.name )
		self.context.common_func_print_just_header( f )

		fixup = []
		o = 0
		for p in f.parameterIterator(1, 1):
			if f.count_parameter_list.count(p.name) or p.name == f.counter:
				self.context.common_emit_one_arg(p, o, "pc", "    ", 0)
				fixup.append(p.name)

			o += p.size()

		print '    GLsizei compsize;'
		print ''

		self.context.common_emit_fixups(fixup)

		print ''
		print '    compsize = %s;' % (context.size_call(context, f))
		p = f.variable_length_parameter()
		print '    return __GLX_PAD(%s);' % (p.size_string())

		print '}'
		print ''


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
		if self.which_functions & self.do_get:
			print '#include "indirect_size_get.h"'
		
		print '#include "indirect_size.h"'

		print ''
		self.printHaveAlias()
		print ''
		self.printPure()
		print ''
		self.printFastcall()
		print ''
		self.printVisibility( "INTERNAL", "internal" )
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
		self.printPure();
		print ''
		self.printFastcall();
		print ''
		self.printVisibility( "INTERNAL", "internal" );
		print ''


	def printFunction(self, f):
		ef = self.glx_enum_functions[f.name]
		print 'extern INTERNAL PURE FASTCALL GLint __gl%s_size(GLenum);' % (f.name)


class PrintGlxReqSize_common(glX_XML.GlxProto):
	"""Common base class for PrintGlxSizeReq_h and PrintGlxSizeReq_h.

	The main purpose of this common base class is to provide the infrastructure
	for the derrived classes to iterate over the same set of functions.
	"""

	def __init__(self):
		glX_XML.GlxProto.__init__(self)
		self.name = "glX_proto_size.py (from Mesa)"
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2005", "IBM")
		self.aliases = []
		self.glx_enum_sigs = {}
		self.size_functions = []


	def endElement(self, name):
		if name == "function":
			f = self.current_object
			if f.glx_rop and not f.ignore and f.fn_alias == None and f.vectorequiv == None:

				if self.glx_enum_functions.has_key(f.name) or f.image or f.server_handcode:
					self.size_functions.append( f )
				else:
					for p in f.parameterIterator(1,2):
						if p.counter and not p.is_output:
							self.size_functions.append( f )
							break

		glX_XML.GlxProto.endElement(self, name)


	def functionIterator(self):
		return self.size_functions


class PrintGlxReqSize_h(PrintGlxReqSize_common):
	def __init__(self):
		PrintGlxReqSize_common.__init__(self)
		self.header_tag = "_INDIRECT_REQSIZE_H_"


	def printRealHeader(self):
		self.printVisibility("HIDDEN", "hidden")
		print ''
		self.printPure()
		print ''


	def printFunction(self, f):
		print 'extern PURE HIDDEN int __glX%sReqSize(const GLbyte *pc, Bool swap);' % (f.name)


class PrintGlxReqSize_c(PrintGlxReqSize_common):
	def __init__(self):
		PrintGlxReqSize_common.__init__(self)
		self.counter_sigs = {}


	def createEnumFunction(self, n):
		return glXServerEnumFunction(n, self)


	def printRealHeader(self):
		print ''
		print '#include <GL/gl.h>'
		print '#include <byteswap.h>'
		print '#include "glxserver.h"'
		print '#include "indirect_size.h"'
		print '#include "indirect_reqsize.h"'
		
		print ''
		print '#define __GLX_PAD(x)  (((x) + 3) & ~3)'
		print ''
		self.printHaveAlias()
		print ''
		print '#ifdef HAVE_ALIAS'
		print '#  define ALIAS2(from,to) \\'
		print '    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap ) \\'
		print '        __attribute__ ((alias( # to )));'
		print '#  define ALIAS(from,to) ALIAS2( from, __glX ## to ## ReqSize )'
		print '#else'
		print '#  define ALIAS(from,to) \\'
		print '    GLint __glX ## from ## ReqSize( const GLbyte * pc, Bool swap ) \\'
		print '    { return __glX ## to ## ReqSize( pc, swap ); }'
		print '#endif'
		print ''
		print ''


	def printRealFooter(self):
		for a in self.aliases:
			print a


	def printFunction(self, f):
		# Even though server-handcode fuctions are on "the list",
		# and prototypes are generated for them, there isn't enough
		# information to generate a size function.  If there was
		# enough information, they probably wouldn't need to be
		# handcoded in the first place!

		if f.server_handcode: return

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
		elif f.image:
			self.printPixelFunction(f)
		else:
			for p in f.parameterIterator(1,2):
				if p.counter and not p.is_output:
					self.printCountedFunction(f)
					break


	def common_emit_fixups(self, fixup):
		"""Utility function to emit conditional byte-swaps."""

		if fixup:
			print '    if (swap) {'
			for name in fixup:
				print '        %-14s = bswap_32( %s );' % (name, name)
			print '    }'

		return


	def common_emit_one_arg(self, p, offset, pc, indent, adjust):
		dst = '%s %s' % (p.p_type_string, p.name)
		src = '(%s *)' % (p.p_type_string)
		print '%s%-18s = *%11s(%s + %u);' % (indent, dst, src, pc, offset + adjust);
		return


	def common_func_print_just_header(self, f):
		print 'int'
		print '__glX%sReqSize( const GLbyte * pc, Bool swap )' % (f.name)
		print '{'


	def printPixelFunction(self, f):
		self.common_func_print_just_header(f)
		
		[dim, w, h, d, junk] = f.dimensions()

		offset = f.offset_of_first_parameter()

		print '    GLint row_length   = *  (GLint *)(pc +  4);'

		if dim < 3:
			fixup = ['row_length', 'skip_rows', 'alignment']
			print '    GLint image_height = 0;'
			print '    GLint skip_images  = 0;'
			print '    GLint skip_rows    = *  (GLint *)(pc +  8);'
			print '    GLint alignment    = *  (GLint *)(pc + 16);'
		else:
			fixup = ['row_length', 'image_height', 'skip_rows', 'skip_images', 'alignment']
			print '    GLint image_height = *  (GLint *)(pc +  8);'
			print '    GLint skip_rows    = *  (GLint *)(pc + 16);'
			print '    GLint skip_images  = *  (GLint *)(pc + 20);'
			print '    GLint alignment    = *  (GLint *)(pc + 32);'

		for p in f.parameterIterator(1, 2):
			if p.name in [w, h, d, f.image.img_format, f.image.img_type, f.image.img_target]:
				self.common_emit_one_arg(p, offset, "pc", "    ", 0 )
				fixup.append( p.name )

			offset += p.size()

		print ''

		self.common_emit_fixups(fixup)

		print ''
		print '    return __glXImageSize(%s, %s, %s, %s, %s, %s,' % (f.image.img_format, f.image.img_type, f.image.img_target, w, h, d )
		print '                          image_height, row_length, skip_images,'
		print '                          skip_rows, alignment);'
		print '}'
		print ''
		return


	def printCountedFunction(self, f):

		sig = ""
		offset = 0
		fixup = []
		params = []
		plus = ''
		size = ''
		param_offsets = {}

		# Calculate the offset of each counter parameter and the
		# size string for the variable length parameter(s).  While
		# that is being done, calculate a unique signature for this
		# function.

		for p in f.parameterIterator(1,2):
			if p.is_counter:
				param_offsets[ p.name ] = offset
				fixup.append( p.name )
				params.append( [p, offset] )
			elif p.counter:
				s = p.p_type.size
				if s == 0: s = 1

				sig += "(%u,%u)" % (param_offsets[p.counter], s)
				size += '%s%s' % (plus, p.size_string())
				plus = ' + '


			offset += p.size()


		# If the calculated signature matches a function that has
		# already be emitted, don't emit this function.  Instead, add
		# it to the list of function aliases.

		if self.counter_sigs.has_key(sig):
			n = self.counter_sigs[sig];
			a = 'ALIAS( %s, %s )' % (f.name, n)
			self.aliases.append(a)
		else:
			self.counter_sigs[sig] = f.name

			self.common_func_print_just_header(f)

			for [p, offset] in params:
				self.common_emit_one_arg(p, offset, "pc", "    ", 0 )


			print ''
			self.common_emit_fixups(fixup)
			print ''

			print '    return __GLX_PAD(%s);' % (size)
			print '}'
			print ''

		return


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
	elif mode == "reqsize_c":
		dh = PrintGlxReqSize_c()
	elif mode == "reqsize_h":
		dh = PrintGlxReqSize_h()
	else:
		show_usage()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
