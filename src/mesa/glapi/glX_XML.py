#!/usr/bin/python2

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
import license
import sys, getopt, string


class glXItemFactory(gl_XML.glItemFactory):
	"""Factory to create GLX protocol oriented objects derived from glItem."""
    
	def create(self, context, name, attrs):
		if name == "function":
			return glXFunction(context, name, attrs)
		elif name == "enum":
			return glXEnum(context, name, attrs)
		elif name == "param":
			return glXParameter(context, name, attrs)
		else:
			return gl_XML.glItemFactory.create(self, context, name, attrs)

class glXEnumFunction:
	def __init__(self, name, context):
		self.name = name
		self.context = context
		self.mode = 0
		self.sig = None

		# "enums" is a set of lists.  The element in the set is the
		# value of the enum.  The list is the list of names for that
		# value.  For example, [0x8126] = {"POINT_SIZE_MIN",
		# "POINT_SIZE_MIN_ARB", "POINT_SIZE_MIN_EXT",
		# "POINT_SIZE_MIN_SGIS"}.

		self.enums = {}

		# "count" is indexed by count values.  Each element of count
		# is a list of index to "enums" that have that number of
		# associated data elements.  For example, [4] = 
		# {GL_AMBIENT, GL_DIFFUSE, GL_SPECULAR, GL_EMISSION,
		# GL_AMBIENT_AND_DIFFUSE} (the enum names are used here,
		# but the actual hexadecimal values would be in the array).

		self.count = {}


	def append(self, count, value, name):
		if self.enums.has_key( value ):
			self.enums[value].append(name)
		else:
			if not self.count.has_key(count):
				self.count[count] = []

			self.enums[value] = []
			self.enums[value].append(name)
			self.count[count].append(value)


	def signature( self ):
		if self.sig == None:
			self.sig = ""
			for i in self.count:
				self.count[i].sort()
				for e in self.count[i]:
					self.sig += "%04x,%u," % (e, i)
	
		return self.sig


	def set_mode( self, mode ):
		"""Mark an enum-function as a 'set' function."""

		self.mode = mode


	def is_set( self ):
		return self.mode


	def PrintUsingTable(self):
		"""Emit the body of the __gl*_size function using a pair
		of look-up tables and a mask.  The mask is calculated such
		that (e & mask) is unique for all the valid values of e for
		this function.  The result of (e & mask) is used as an index
		into the first look-up table.  If it matches e, then the
		same entry of the second table is returned.  Otherwise zero
		is returned.
		
		It seems like this should cause better code to be generated.
		However, on x86 at least, the resulting .o file is about 20%
		larger then the switch-statment version.  I am leaving this
		code in because the results may be different on other
		platforms (e.g., PowerPC or x86-64)."""

		return 0
		count = 0
		for a in self.enums:
			count += 1

		if self.count.has_key(-1):
			return 0

		# Determine if there is some mask M, such that M = (2^N) - 1,
		# that will generate unique values for all of the enums.

		mask = 0
		for i in [1, 2, 3, 4, 5, 6, 7, 8]:
			mask = (1 << i) - 1

			fail = 0;
			for a in self.enums:
				for b in self.enums:
					if a != b:
						if (a & mask) == (b & mask):
							fail = 1;

			if not fail:
				break;
			else:
				mask = 0

		if (mask != 0) and (mask < (2 * count)):
			masked_enums = {}
			masked_count = {}

			for i in range(0, mask + 1):
				masked_enums[i] = "0";
				masked_count[i] = 0;

			for c in self.count:
				for e in self.count[c]:
					i = e & mask
					masked_enums[i] = '0x%04x /* %s */' % (e, self.enums[e][0])
					masked_count[i] = c


			print '    static const GLushort a[%u] = {' % (mask + 1)
			for e in masked_enums:
				print '        %s, ' % (masked_enums[e])
			print '    };'

			print '    static const GLubyte b[%u] = {' % (mask + 1)
			for c in masked_count:
				print '        %u, ' % (masked_count[c])
			print '    };'

			print '    const unsigned idx = (e & 0x%02xU);' % (mask)
			print ''
			print '    return (e == a[idx]) ? (GLint) b[idx] : 0;'
			return 1;
		else:
			return 0;

	def PrintUsingSwitch(self, name):
		"""Emit the body of the __gl*_size function using a 
		switch-statement."""

		print '    switch( e ) {'

		for c in self.count:
			for e in self.count[c]:
				first = 1

				# There may be multiple enums with the same
				# value.  This happens has extensions are
				# promoted from vendor-specific or EXT to
				# ARB and to the core.  Emit the first one as
				# a case label, and emit the others as
				# commented-out case labels.

				for j in self.enums[e]:
					if first:
						print '        case %s:' % (j)
						first = 0
					else:
						print '/*      case %s:*/' % (j)
					
			if c == -1:
				print '            return __gl%s_variable_size( e );' % (name)
			else:
				print '            return %u;' % (c)
					
		print '        default: return 0;'
		print '    }'


	def Print(self, name):
		print 'INTERNAL PURE FASTCALL GLint'
		print '__gl%s_size( GLenum e )' % (name)
		print '{'

		if not self.PrintUsingTable():
			self.PrintUsingSwitch(name)

		print '}'
		print ''



class glXEnum(gl_XML.glEnum):
	def __init__(self, context, name, attrs):
		gl_XML.glEnum.__init__(self, context, name, attrs)


	def startElement(self, name, attrs):
		if name == "size":
			[temp_n, c, mode] = self.process_attributes(attrs)

			if temp_n == "Get":
				names = ["GetIntegerv", "GetBooleanv", "GetFloatv", "GetDoublev" ]
			else:
				names = [ temp_n ]

			for n in names:
				if not self.context.glx_enum_functions.has_key( n ):
					f = self.context.createEnumFunction( n )
					f.set_mode( mode )
					self.context.glx_enum_functions[ f.name ] = f

				self.context.glx_enum_functions[ n ].append( c, self.value, self.name )
		else:
			gl_XML.glEnum.startElement(self, context, name, attrs)
		return


class glXParameter(gl_XML.glParameter):
	def __init__(self, context, name, attrs):
		self.order = 1;
		gl_XML.glParameter.__init__(self, context, name, attrs);


class glXParameterIterator:
	"""Class to iterate over a list of glXParameters.

	Objects of this class are returned by the parameterIterator method of
	the glXFunction class.  They are used to iterate over the list of
	parameters to the function."""

	def __init__(self, data, skip_output, max_order):
		self.data = data
		self.index = 0
		self.order = 0
		self.skip_output = skip_output
		self.max_order = max_order

	def __iter__(self):
		return self

	def next(self):
		if len( self.data ) == 0:
			raise StopIteration

		while 1:
			if self.index == len( self.data ):
				if self.order == self.max_order:
					raise StopIteration
				else:
					self.order += 1
					self.index = 0

			i = self.index
			self.index += 1

			if self.data[i].order == self.order and not (self.data[i].is_output and self.skip_output):
				return self.data[i]


class glXFunction(gl_XML.glFunction):
	glx_rop = 0
	glx_sop = 0
	glx_vendorpriv = 0

	# If this is set to true, it means that GLdouble parameters should be
	# written to the GLX protocol packet in the order they appear in the
	# prototype.  This is different from the "classic" ordering.  In the
	# classic ordering GLdoubles are written to the protocol packet first,
	# followed by non-doubles.  NV_vertex_program was the first extension
	# to break with this tradition.

	glx_doubles_in_order = 0

	vectorequiv = None
	can_be_large = 0

	def __init__(self, context, name, attrs):
		self.vectorequiv = attrs.get('vectorequiv', None)
		self.counter = None
		self.output = None
		self.can_be_large = 0
		self.reply_always_array = 0
		self.dimensions_in_reply = 0
		self.img_reset = None

		self.server_handcode = 0
		self.client_handcode = 0
		self.ignore = 0

		gl_XML.glFunction.__init__(self, context, name, attrs)
		return


	def parameterIterator(self, skip_output, max_order):
		return glXParameterIterator(self.fn_parameters, skip_output, max_order)

		
	def startElement(self, name, attrs):
		"""Process elements within a function that are specific to GLX."""

		if name == "glx":
			self.glx_rop = int(attrs.get('rop', "0"))
			self.glx_sop = int(attrs.get('sop', "0"))
			self.glx_vendorpriv = int(attrs.get('vendorpriv', "0"))
			self.img_reset = attrs.get('img_reset', None)

			# The 'handcode' attribute can be one of 'true',
			# 'false', 'client', or 'server'.

			handcode = attrs.get('handcode', "false")
			if handcode == "false":
				self.server_handcode = 0
				self.client_handcode = 0
			elif handcode == "true":
				self.server_handcode = 1
				self.client_handcode = 1
			elif handcode == "client":
				self.server_handcode = 0
				self.client_handcode = 1
			elif handcode == "server":
				self.server_handcode = 1
				self.client_handcode = 0
			else:
				raise RuntimeError('Invalid handcode mode "%s" in function "%s".' % (handcode, self.name))


			if attrs.get('ignore', "false") == "true":
				self.ignore = 1
			else:
				self.ignore = 0

			if attrs.get('large', "false") == "true":
				self.can_be_large = 1
			else:
				self.can_be_large = 0

			if attrs.get('doubles_in_order', "false") == "true":
				self.glx_doubles_in_order = 1
			else:
				self.glx_doubles_in_order = 0

			if attrs.get('always_array', "false") == "true":
				self.reply_always_array = 1
			else:
				self.reply_always_array = 0

			if attrs.get('dimensions_in_reply', "false") == "true":
				self.dimensions_in_reply = 1
			else:
				self.dimensions_in_reply = 0
		else:
			gl_XML.glFunction.startElement(self, name, attrs)


	def endElement(self, name):
		if name == "function":
			# Mark any function that does not have GLX protocol
			# defined as "ignore".  This prevents bad things from
			# happening when people add new functions to the GL
			# API XML without adding any GLX section.
			#
			# This will also mark functions that don't have a
			# dispatch offset at ignored.

			if (self.fn_offset == -1 and not self.fn_alias) or not (self.client_handcode or self.server_handcode or self.glx_rop or self.glx_sop or self.glx_vendorpriv or self.vectorequiv or self.fn_alias):
				#if not self.ignore:
				#	if self.fn_offset == -1:
				#		print '/* %s ignored becuase no offset assigned. */' % (self.name)
				#	else:
				#		print '/* %s ignored becuase no GLX opcode assigned. */' % (self.name)

				self.ignore = 1

		return gl_XML.glFunction.endElement(self, name)


	def append(self, tag_name, p):
		gl_XML.glFunction.append(self, tag_name, p)

		if p.is_variable_length_array():
			p.order = 2;
		elif not self.glx_doubles_in_order and p.p_type.size == 8:
			p.order = 0;

		if p.is_counter:
			self.counter = p.name
			
		if p.is_output:
			self.output = p

		return


	def variable_length_parameter(self):
		if len(self.variable_length_parameters):
			return self.variable_length_parameters[0]

		return None


	def output_parameter(self):
		for param in self.fn_parameters:
			if param.is_output:
				return param

		return None


	def offset_of_first_parameter(self):
		"""Get the offset of the first parameter in the command.

		Gets the offset of the first function parameter in the GLX
		command packet.  This byte offset is measured from the end
		of the Render / RenderLarge header.  The offset for all non-
		pixel commends is zero.  The offset for pixel commands depends
		on the number of dimensions of the pixel data."""

		if self.image and not self.image.is_output:
			[dim, junk, junk, junk, junk] = self.dimensions()

			# The base size is the size of the pixel pack info
			# header used by images with the specified number
			# of dimensions.

			if dim <=  2:
				return 20
			elif dim <= 4:
				return 36
			else:
				raise RuntimeError('Invalid number of dimensions %u for parameter "%s" in function "%s".' % (dim, self.image.name, self.name))
		else:
			return 0


	def command_fixed_length(self):
		"""Return the length, in bytes as an integer, of the
		fixed-size portion of the command."""

		size = self.offset_of_first_parameter()

		for p in gl_XML.glFunction.parameterIterator(self):
			if not p.is_output and p.name != self.img_reset:
				size += p.size()
				if self.pad_after(p):
					size += 4

		if self.image and (self.image.img_null_flag or self.image.is_output):
			size += 4

		return size


	def command_variable_length(self):
		"""Return the length, as a string, of the variable-sized
		portion of the command."""

		size_string = ""
		for p in gl_XML.glFunction.parameterIterator(self):
			if (not p.is_output) and (p.size() == 0):
				size_string = size_string + " + __GLX_PAD(%s)" % (p.size_string())

		return size_string


	def command_length(self):
		size = self.command_fixed_length()

		if self.glx_rop != 0:
			size += 4

		size = ((size + 3) & ~3)
		return "%u%s" % (size, self.command_variable_length())


	def opcode_real_value(self):
		"""Get the true numeric value of the GLX opcode
		
		Behaves similarly to opcode_value, except for
		X_GLXVendorPrivate and X_GLXVendorPrivateWithReply commands.
		In these cases the value for the GLX opcode field (i.e.,
		16 for X_GLXVendorPrivate or 17 for
		X_GLXVendorPrivateWithReply) is returned.  For other 'single'
		commands, the opcode for the command (e.g., 101 for
		X_GLsop_NewList) is returned."""

		if self.glx_vendorpriv != 0:
			if self.needs_reply():
				return 17
			else:
				return 16
		else:
			return self.opcode_value()

	def opcode_value(self):
		"""Get the unique protocol opcode for the glXFunction"""

		if self.glx_rop != 0:
			return self.glx_rop
		elif self.glx_sop != 0:
			return self.glx_sop
		elif self.glx_vendorpriv != 0:
			return self.glx_vendorpriv
		else:
			return -1
	
	def opcode_rop_basename(self):
		"""Return either the name to be used for GLX protocol enum.
		
		Returns either the name of the function or the name of the
		name of the equivalent vector (e.g., glVertex3fv for
		glVertex3f) function."""

		if self.vectorequiv == None:
			return self.name
		else:
			return self.vectorequiv

	def opcode_name(self):
		"""Get the unique protocol enum name for the glXFunction"""

		if self.glx_rop != 0:
			return "X_GLrop_%s" % (self.opcode_rop_basename())
		elif self.glx_sop != 0:
			return "X_GLsop_%s" % (self.name)
		elif self.glx_vendorpriv != 0:
			return "X_GLvop_%s" % (self.name)
		else:
			raise RuntimeError('Function "%s" has no opcode.' % (self.name))


	def opcode_real_name(self):
		"""Get the true protocol enum name for the GLX opcode
		
		Behaves similarly to opcode_name, except for
		X_GLXVendorPrivate and X_GLXVendorPrivateWithReply commands.
		In these cases the string 'X_GLXVendorPrivate' or
		'X_GLXVendorPrivateWithReply' is returned.  For other
		single or render commands 'X_GLsop' or 'X_GLrop' plus the
		name of the function returned."""

		if self.glx_vendorpriv != 0:
			if self.needs_reply():
				return "X_GLXVendorPrivateWithReply"
			else:
				return "X_GLXVendorPrivate"
		else:
			return self.opcode_name()


	def return_string(self):
		if self.fn_return_type != 'void':
			return "return retval;"
		else:
			return "return;"


	def needs_reply(self):
		return self.fn_return_type != 'void' or self.output != None


	def dimensions(self):
		"""Determine the dimensions of an image.
		
		Returns a tuple representing the number of dimensions and the
		string name of each of the dimensions of an image,  If the
		function is not a pixel function, the number of dimensions
		will be zero."""

		if not self.image:
			return [0, "0", "0", "0", "0"]
		else:
			dim = 1
			w = self.image.width

			if self.image.height:
				dim = 2
				h = self.image.height
			else:
				h = "1"

			if self.image.depth:
				dim = 3
				d = self.image.depth
			else:
				d = "1"
			
			if self.image.extent:
				dim = 4
				e = self.image.extent
			else:
				e = "1"

			return [dim, w, h, d, e]


	def pad_after(self, p):
		"""Returns the name of the field inserted after the
		specified field to pad out the command header."""

		if self.image and self.image.img_pad_dimensions:
			if not self.image.height:
				if p.name == self.image.width:
					return "height"
				elif p.name == self.image.img_xoff:
					return "yoffset"
			elif not self.image.extent:
				if p.name == self.image.depth:
					# Should this be "size4d"?
					return "extent"
				elif p.name == self.image.img_zoff:
					return "woffset"
		return None


class glXFunctionIterator(gl_XML.glFunctionIterator):
	"""Class to iterate over a list of glXFunctions"""

	def __init__(self, context):
		self.context = context
		self.keys = context.functions.keys()
		self.keys.sort()

		for self.index in range(0, len(self.keys)):
			if self.keys[ self.index ] >= 0: break

		return


	def next(self):
		if self.index == len(self.keys):
			raise StopIteration

		f = self.context.functions[ self.keys[ self.index ] ]
		self.index += 1

		if f.ignore:
			return self.next()
		else:
			return f


class GlxProto(gl_XML.FilterGLAPISpecBase):
	name = "glX_proto_send.py (from Mesa)"

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.factory = glXItemFactory()
		self.glx_enum_functions = {}


	def endElement(self, name):
		if name == 'OpenGLAPI':
			# Once all the parsing is done, we have to go back and
			# fix-up some cross references between different
			# functions.

			for k in self.functions:
				f = self.functions[k]
				if f.vectorequiv != None:
					equiv = self.find_function(f.vectorequiv)
					if equiv != None:
						f.glx_doubles_in_order = equiv.glx_doubles_in_order
						f.glx_rop = equiv.glx_rop
					else:
						raise RuntimeError("Could not find the vector equiv. function %s for %s!" % (f.name, f.vectorequiv))
		else:
			gl_XML.FilterGLAPISpecBase.endElement(self, name)
		return


	def createEnumFunction(self, n):
		return glXEnumFunction(n, self)


	def functionIterator(self):
		return glXFunctionIterator(self)

	
	def size_call(self, func):
		"""Create C code to calculate 'compsize'.

		Creates code to calculate 'compsize'.  If the function does
		not need 'compsize' to be calculated, None will be
		returned."""

		if not func.image and not func.count_parameter_list:
			return None

		if not func.image:
			parameters = string.join( func.count_parameter_list, "," )
			compsize = "__gl%s_size(%s)" % (func.name, parameters)
		else:
			[dim, w, h, d, junk] = func.dimensions()

			compsize = '__glImageSize(%s, %s, %s, %s, %s, %s)' % (w, h, d, func.image.img_format, func.image.img_type, func.image.img_target)
			if not func.image.img_send_null:
				compsize = '(%s != NULL) ? %s : 0' % (func.image.name, compsize)

		return compsize
