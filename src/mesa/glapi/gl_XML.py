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

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces

import sys, re

class glItem:
	"""Generic class on which all other API entity types are based."""

	name = ""
	category = ""
	context = None
	tag_name = ""

	def __init__(self, tag_name, name, context):
		self.name = name
		self.category = context.get_category_define()
		self.context = context
		self.tag_name = tag_name
		
		context.append(tag_name, self)
		return
	
	def startElement(self, name, attrs):
		return

	def endElement(self, name):
		"""Generic endElement handler.

		Generic endElement handler.    Returns 1 if the tag containing
		the object is complete.  Otherwise 0 is returned.  All
		derived class endElement handlers should call this method.  If
		the name of the ending tag is the same as the tag that
		started this object, the object is assumed to be complete.
		
		This fails if a tag can contain another tag with the same
		name.  The XML "<foo><foo/><bar/></foo>" would fail.  The
		object would end before the bar tag was processed."""

		if name == self.tag_name:
			return 1
		else:
			return 0
		return


class glEnum( glItem ):
	def __init__(self, context, name, attrs):
		self.value = int(attrs.get('value', "0x0000"), 0)
		self.functions = {}

		enum_name = "GL_" + attrs.get('name', None)
		glItem.__init__(self, name, enum_name, context)

	def startElement(self, name, attrs):
		if name == "size":
			name = attrs.get('name', None)
			count = int(attrs.get('count', "0"), 0)
			self.functions[name] = count

		return


class glType( glItem ):
	def __init__(self, context, name, attrs):
		self.size = int(attrs.get('size', "0"))

		type_name = "GL" + attrs.get('name', None)
		glItem.__init__(self, name, type_name, context)


class glParameter:
	p_type = None
	p_type_string = ""
	p_name = None
	p_count = 0
	p_count_parameters = None
	counter = None
	is_output = 0
	is_counter = 0
	is_pointer = 0

	def __init__(self, t, ts, n, c, p, is_output):
		self.counter = None

		try: 
			self.p_count = int(c)
		except Exception,e:
			self.p_count = 0
			self.counter = c

		if is_output == "true":
			self.is_output = 1
		else:
			self.is_output = 0

		if self.p_count > 0 or self.counter != None or p != None :
			has_count = 1
		else:
			has_count = 0

		self.p_type = t
		self.p_type_string = ts
		self.p_name = n
		self.p_count_parameters = p
		
		# If there is a * anywhere in the parameter's type, then it
		# is a pointer.

		if re.compile("[*]").search(ts):
			# We could do some other validation here.  For
			# example, an output parameter should not be const,
			# but every non-output parameter should.

			self.is_pointer = 1;
		else:
			# If a parameter is not a pointer, then there cannot
			# be an associated count (either fixed size or
			# variable) and the parameter cannot be an output.

			if has_count or self.is_output:
				raise RuntimeError("Non-pointer type has count or is output.")
			self.is_pointer = 0;

	def is_variable_length_array(self):
		return self.p_count_parameters != None

	def is_array(self):
		return self.is_pointer

	def count_string(self):
		"""Return a string representing the number of items
		
		Returns a string representing the number of items in a
		parameter.  For scalar types this will always be "1".  For
		vector types, it will depend on whether or not it is a
		fixed length vector (like the parameter of glVertex3fv),
		a counted length (like the vector parameter of
		glDeleteTextures), or a general variable length vector."""

		if self.is_array():
			if self.is_variable_length_array():
				return "compsize"
			elif self.counter != None:
				return self.counter
			else:
				return str(self.p_count)
		else:
			return "1"

	def size(self):
		if self.is_variable_length_array():
			return 0
		elif self.p_count == 0:
			return self.p_type.size
		else:
			return self.p_type.size * self.p_count

class glParameterIterator:
	def __init__(self, data):
		self.data = data
		self.index = 0
		
	def next(self):
		if self.index == len( self.data ):
			raise StopIteration
		i = self.index
		self.index += 1
		return self.data[i]

class glFunction( glItem ):
	real_name = ""
	fn_alias = None
	fn_offset = -1
	fn_return_type = "void"
	fn_parameters = []

	def __init__(self, context, name, attrs):
		self.fn_alias = attrs.get('alias', None)
		self.fn_parameters = []

		temp = attrs.get('offset', None)
		if temp == None or temp == "?":
			self.fn_offset = -1
		else:
			self.fn_offset = int(temp)

		fn_name = attrs.get('name', None)
		if self.fn_alias != None:
			self.real_name = self.fn_alias
		else:
			self.real_name = fn_name

		glItem.__init__(self, name, fn_name, context)
		return


	def __iter__(self):
		return glParameterIterator(self.fn_parameters)


	def startElement(self, name, attrs):
		if name == "param":
			p_name = attrs.get('name', None)
			p_type = attrs.get('type', None)
			p_count = attrs.get('count', "0")
			p_param = attrs.get('variable_param', None)
			is_output = attrs.get('output', "false")
			is_counter = attrs.get('counter', "false")

			t = self.context.find_type(p_type)
			if t == None:
				raise RuntimeError("Unknown type '%s' in function '%s'." % (p_type, self.name)) 

			try:
				p = glParameter(t, p_type, p_name, p_count, p_param, is_output)
			except RuntimeError:
				print "Error with parameter '%s' in function '%s'." \
					% (p_name, self.name)
				raise

			if is_counter == "true": p.is_counter = 1

			self.add_parameter(p)
		elif name == "return":
			self.set_return_type(attrs.get('type', None))


	def add_parameter(self, p):
		self.fn_parameters.append(p)

	def set_return_type(self, t):
		self.fn_return_type = t

	def get_parameter_string(self):
		arg_string = ""
		comma = ""
		for p in self:
			arg_string = arg_string + comma + p.p_type_string + " " + p.p_name
			comma = ", "

		if arg_string == "":
			arg_string = "void"

		return arg_string


class glItemFactory:
	"""Factory to create objects derived from glItem."""
    
	def create(self, context, name, attrs):
		if name == "function":
			return glFunction(context, name, attrs)
		elif name == "type":
			return glType(context, name, attrs)
		elif name == "enum":
			return glEnum(context, name, attrs)
		else:
			return None


class FilterGLAPISpecBase(saxutils.XMLFilterBase):
	name = "a"
	license = "The license for this file is unspecified."
	functions = {}
	next_alias = -2
	types = {}
	xref = {}
	current_object = None
	factory = None
	current_category = ""

	def __init__(self):
		saxutils.XMLFilterBase.__init__(self)
		self.functions = {}
		self.types = {}
		self.xref = {}
		self.factory = glItemFactory()

	def find_type(self,type_name):
		for t in self.types:
			if re.compile(t).search(type_name):
				return self.types[t]
		print "Unable to find base type matching \"%s\"." % (type_name)
		return None

	def find_function(self,function_name):
		index = self.xref[function_name]
		return self.functions[index]

	def printFunctions(self):
		keys = self.functions.keys()
		keys.sort()
		prevk = -1
		for k in keys:
			if k < 0: continue

			if self.functions[k].fn_alias == None:
				if k != prevk + 1:
					#print 'Missing offset %d' % (prevk)
					pass
			prevk = int(k)
			self.printFunction(self.functions[k])

		keys.reverse()
		for k in keys:
			if self.functions[k].fn_alias != None:
				self.printFunction(self.functions[k])

		return

	def printHeader(self):
		print '/* DO NOT EDIT - This file generated automatically by %s script */' \
			% (self.name)
		print ''
		print '/*'
		print ' * ' + self.license.replace('\n', '\n * ')
		print ' */'
		print ''
		self.printRealHeader();
		return

	def printFooter(self):
		self.printFunctions()
		self.printRealFooter()


	def get_category_define(self):
		if re.compile("[1-9][0-9]*[.][0-9]+").match(self.current_category):
			s = self.current_category
			return "GL_VERSION_" + s.replace(".", "_")
		else:
			return self.current_category


	def append(self, object_type, obj):
		if object_type == "function":
			# If the function is not an alias and has a negative
			# offset, then we do not need to track it.  These are
			# functions that don't have an assigned offset

			if obj.fn_offset >= 0 or obj.fn_alias != None:
				if obj.fn_offset >= 0:
					index = obj.fn_offset
				else:
					index = self.next_alias
					self.next_alias -= 1

				self.functions[index] = obj
				self.xref[obj.name] = index
		elif object_type == "type":
			self.types[obj.name] = obj

		return


	def startElement(self, name, attrs):
		if self.current_object != None:
			self.current_object.startElement(name, attrs)
		elif name == "category":
			self.current_category = attrs.get('name', "")
		else:
			self.current_object = self.factory.create(self, name, attrs)
		return

	def endElement(self, name):
		if self.current_object != None:
			if self.current_object.endElement(name):
				self.current_object = None
		return

	def printFunction(self,offset):
		return
    
	def printRealHeader(self):
		return

	def printRealFooter(self):
		return
