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

import gl_XML
import license
import sys, getopt


def printPure():
	print """#  if __GNUC__ > 2 || (__GNUC__ == 2 && __GNUC_MINOR__ >= 96)
#    define PURE __attribute__((pure))
#  else
#    define PURE
#  endif"""

def printFastcall():
	print """#  if defined(__i386__) && defined(__GNUC__)
#    define FASTCALL __attribute__((fastcall))
#  else
#    define FASTCALL
#  endif"""

def printVisibility(S, s):
	print """#  if __GNUC__ > 3 || (__GNUC__ == 3 && __GNUC_MINOR__ >= 3)
#    define %s  __attribute__((visibility("%s")))
#  else
#    define %s
#  endif""" % (S, s, S)

def printNoinline():
	print """#  if defined(__GNUC__)
#    define NOINLINE __attribute__((noinline))
#  else
#    define NOINLINE
#  endif"""


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
	def __init__(self, name):
		self.name = name
		
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
		sig = ""
		for i in self.count:
			for e in self.count[i]:
				sig += "%04x,%u," % (e, i)
	
		return sig;


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

			for i in range(0, mask):
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

	def PrintUsingSwitch(self):
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
					
			print '            return %u;' % (c)
					
		print '        default: return 0;'
		print '    }'


	def Print(self, name):
		print 'INTERNAL PURE FASTCALL GLint'
		print '__gl%s_size( GLenum e )' % (name)
		print '{'

		if not self.PrintUsingTable():
			self.PrintUsingSwitch()

		print '}'
		print ''



class glXEnum(gl_XML.glEnum):
	def __init__(self, context, name, attrs):
		gl_XML.glEnum.__init__(self, context, name, attrs)
		self.glx_functions = []

	def startElement(self, name, attrs):
		if name == "size":
			n = attrs.get('name', None)
			if not self.context.glx_enum_functions.has_key( n ):
				f = glXEnumFunction( n )
				self.context.glx_enum_functions[ f.name ] = f

			temp = attrs.get('count', None)
			try:
				c = int(temp)
			except Exception,e:
				raise RuntimeError('Invalid count value "%s" for enum "%s" in function "%s" when an integer was expected.' % (temp, self.name, n))

			self.context.glx_enum_functions[ n ].append( c, self.value, self.name )
		else:
			gl_XML.glEnum.startElement(self, context, name, attrs)
		return


class glXParameter(gl_XML.glParameter):
	def __init__(self, context, name, attrs):
		self.order = 1;
		gl_XML.glParameter.__init__(self, context, name, attrs);


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
	handcode = 0
	ignore = 0
	can_be_large = 0

	def __init__(self, context, name, attrs):
		self.vectorequiv = attrs.get('vectorequiv', None)
		self.count_parameters = None
		self.counter = None
		self.output = None
		self.can_be_large = 0
		self.reply_always_array = 0

		gl_XML.glFunction.__init__(self, context, name, attrs)
		return

	def startElement(self, name, attrs):
		"""Process elements within a function that are specific to GLX."""

		if name == "glx":
			self.glx_rop = int(attrs.get('rop', "0"))
			self.glx_sop = int(attrs.get('sop', "0"))
			self.glx_vendorpriv = int(attrs.get('vendorpriv', "0"))

			if attrs.get('handcode', "false") == "true":
				self.handcode = 1
			else:
				self.handcode = 0

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

		else:
			gl_XML.glFunction.startElement(self, name, attrs)


	def append(self, tag_name, p):
		gl_XML.glFunction.append(self, tag_name, p)

		if p.is_variable_length_array():
			p.order = 2;
		elif not self.glx_doubles_in_order and p.p_type.size == 8:
			p.order = 0;

		if p.p_count_parameters != None:
			self.count_parameters = p.p_count_parameters
		
		if p.is_counter:
			self.counter = p.name
			
		if p.is_output:
			self.output = p

		return

	def variable_length_parameter(self):
		for param in self.fn_parameters:
			if param.is_variable_length_array():
				return param
			
		return None


	def command_payload_length(self):
		size = 0
		size_string = ""
		for p in self:
			if p.is_output: continue
			temp = p.size_string()
			try:
				s = int(temp)
				size += s
			except Exception,e:
				size_string = size_string + " + __GLX_PAD(%s)" % (temp)

		return [size, size_string]

	def command_length(self):
		[size, size_string] = self.command_payload_length()

		if self.glx_rop != 0:
			size += 4

		size = ((size + 3) & ~3)
		return "%u%s" % (size, size_string)


	def opcode_value(self):
		if self.glx_rop != 0:
			return self.glx_rop
		elif self.glx_sop != 0:
			return self.glx_sop
		elif self.glx_vendorpriv != 0:
			return self.glx_vendorpriv
		else:
			return -1
	
	def opcode_rop_basename(self):
		if self.vectorequiv == None:
			return self.name
		else:
			return self.vectorequiv

	def opcode_name(self):
		if self.glx_rop != 0:
			return "X_GLrop_%s" % (self.opcode_rop_basename())
		elif self.glx_sop != 0:
			return "X_GLsop_%s" % (self.name)
		elif self.glx_vendorpriv != 0:
			return "X_GLvop_%s" % (self.name)
		else:
			return "ERROR"

	def opcode_real_name(self):
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


class PrintGlxProtoStubs(GlxProto):
	def __init__(self):
		GlxProto.__init__(self)
		self.last_category = ""
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004", "IBM")
		self.generic_sizes = [3, 4, 6, 8, 12, 16, 24, 32]
		return

	def printRealHeader(self):
		print ''
		print '#include <GL/gl.h>'
		print '#include "indirect.h"'
		print '#include "glxclient.h"'
		print '#include "size.h"'
		print '#include <GL/glxproto.h>'
		print ''
		print '#define __GLX_PAD(n) (((n) + 3) & ~3)'
		print ''
		printFastcall()
		printNoinline()
		print ''
		print '/* If the size and opcode values are known at compile-time, this will, on'
		print ' * x86 at least, emit them with a single instruction.'
		print ' */'
		print '#define emit_header(dest, op, size)            \\'
		print '    do { union { short s[2]; int i; } temp;    \\'
		print '         temp.s[0] = (size); temp.s[1] = (op); \\'
		print '         *((int *)(dest)) = temp.i; } while(0)'
		print ''
		print """static NOINLINE CARD32
read_reply( Display *dpy, size_t size, void * dest, GLboolean reply_is_always_array )
{
    xGLXSingleReply reply;
    
    (void) _XReply(dpy, (xReply *) & reply, 0, False);
    if (size != 0) {
	if ((reply.size >= 1) || reply_is_always_array) {
	    const GLint bytes = (reply_is_always_array) 
	      ? (4 * reply.length) : (reply.size * size);
	    const GLint extra = 4 - (bytes & 3);

	    _XRead(dpy, dest, bytes);
	    if ( extra != 0 ) {
		_XEatData(dpy, extra);
	    }
	}
	else {
	    (void) memcpy( dest, &(reply.pad3), size);
	}
    }

    return reply.retval;
}

#define X_GLXSingle 0

static NOINLINE GLubyte *
setup_single_request( __GLXcontext * gc, GLint sop, GLint cmdlen )
{
    xGLXSingleReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXSingle, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->contextTag = gc->currentContextTag;
    req->glxCode = sop;
    return (GLubyte *)(req) + sz_xGLXSingleReq;
}

static NOINLINE GLubyte *
setup_vendor_request( __GLXcontext * gc, GLint code, GLint vop, GLint cmdlen )
{
    xGLXVendorPrivateReq * req;
    Display * const dpy = gc->currentDpy;

    (void) __glXFlushRenderBuffer(gc, gc->pc);
    LockDisplay(dpy);
    GetReqExtra(GLXVendorPrivate, cmdlen, req);
    req->reqType = gc->majorOpcode;
    req->glxCode = code;
    req->vendorCode = vop;
    req->contextTag = gc->currentContextTag;
    return (GLubyte *)(req) + sz_xGLXVendorPrivateReq;
}
"""

		for size in self.generic_sizes:
			self.print_generic_function(size)
		return

	def printFunction(self, f):
		if f.fn_offset < 0 or f.handcode or f.ignore: return

		if f.glx_rop != 0 or f.vectorequiv != None:
			self.printRenderFunction(f)
		elif f.glx_sop != 0 or f.glx_vendorpriv != 0:
			self.printSingleFunction(f)
		else:
			print "/* Missing GLX protocol for %s. */" % (f.name)

	def print_generic_function(self, n):
		print """static FASTCALL NOINLINE void
generic_%u_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = %u;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, %u);
    gc->pc += cmdlen;
    if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
""" % (n, n + 4, n)


	def common_emit_one_arg(self, p, offset, pc, indent, adjust):
		if p.is_output: return

		t = p.p_type
		if p.is_array():
			src_ptr = p.name
		else:
			src_ptr = "&" + p.name

		print '%s    (void) memcpy((void *)(%s + %u), (void *)(%s), %s);' \
			% (indent, pc, offset + adjust, src_ptr, p.size_string() )

	def common_emit_args(self, f, pc, indent, adjust, skip_vla):
		# First emit all of the fixed-length 8-byte (i.e., GLdouble)
		# parameters.

		offset = 0

		if skip_vla:
			r = [0, 1]
		else:
			r = [0, 1, 2]

		for order in r:
			for p in f:
				if p.is_output or p.order != order: continue

				self.common_emit_one_arg(p, offset, pc, indent, adjust)
				offset += p.size()


		return offset


	def common_func_print_just_header(self, f):
		print '#define %s %d' % (f.opcode_name(), f.opcode_value())

		print '%s' % (f.fn_return_type)
		print '__indirect_gl%s(%s)' % (f.name, f.get_parameter_string())
		print '{'


	def common_func_print_header(self, f):
		self.common_func_print_just_header(f)

		print '    __GLXcontext * const gc = __glXGetCurrentContext();'
		print '    Display * const dpy = gc->currentDpy;'
			
		if f.fn_return_type != 'void':
			print '    %s retval = (%s) 0;' % (f.fn_return_type, f.fn_return_type)

		if f.count_parameters != None:
			print '    const GLuint compsize = __gl%s_size(%s);' % (f.name, f.count_parameters)

		print '    const GLuint cmdlen = %s;' % (f.command_length())

		if f.counter != None:
			print '    if (%s < 0) %s' % (f.counter, f.return_string())

		if f.can_be_large:
			print '    if (dpy == NULL) return;'
			print '    if ( ((gc->pc + cmdlen) > gc->bufEnd)'
			print '         || (cmdlen > gc->maxSmallRenderCommandSize)) {'
			print '        (void) __glXFlushRenderBuffer(gc, gc->pc);'
			print '    }'
		else:
			print '    (void) dpy;'

		return


	def printSingleFunction(self, f):
		self.common_func_print_header(f)

		print '    if (dpy != NULL) {'

		if f.fn_parameters != []:
			pc_decl = "GLubyte const * pc ="
		else:
			pc_decl = "(void)"

		if f.glx_vendorpriv != 0:
			print '        %s setup_vendor_request(gc, %s, %s, cmdlen);' % (pc_decl, f.opcode_real_name(), f.opcode_name())
		else:
			print '        %s setup_single_request(gc, %s, cmdlen);' % (pc_decl, f.opcode_name())

		self.common_emit_args(f, "pc", "    ", 0, 0)

		if f.needs_reply():
			if f.output != None:
				output_size = f.output.p_type.size
				output_str = f.output.name
			else:
				output_size = 0
				output_str = "NULL"

			if f.fn_return_type != 'void':
				return_str = " retval = (%s)" % (f.fn_return_type)
			else:
				return_str = " (void)"

			if f.reply_always_array:
				aa = "GL_TRUE"
			else:
				aa = "GL_FALSE"

			print "       %s read_reply(gc->currentDpy, %s, %s, %s);" % (return_str, output_size, output_str, aa)

		print '        UnlockDisplay(gc->currentDpy); SyncHandle();'
		print '    }'
		print '    %s' % f.return_string()
		print '}'
		print ''
		return


	def printRenderFunction(self, f):
		if f.variable_length_parameter() == None and len(f.fn_parameters) == 1:
			p = f.fn_parameters[0]
			if p.is_pointer:
				[cmdlen, size_string] = f.command_payload_length()
				if cmdlen in self.generic_sizes:
					self.common_func_print_just_header(f)
					print '    generic_%u_byte( %s, %s );' % (cmdlen, f.opcode_real_name(), p.name)
					print '}'
					print ''
					return

		self.common_func_print_header(f)

		if f.can_be_large:
			print '    if (cmdlen <= gc->maxSmallRenderCommandSize) {'
			indent = "    "
		else:
			indent = ""

		print '%s    emit_header(gc->pc, %s, cmdlen);' % (indent, f.opcode_real_name())

		self.common_emit_args(f, "gc->pc", indent, 4, 0)
		print '%s    gc->pc += cmdlen;' % (indent)
		print '%s    if (gc->pc > gc->limit) { (void) __glXFlushRenderBuffer(gc, gc->pc); }' % (indent)

		if f.can_be_large:
			print '    }'
			print '    else {'
			print '        const GLint op = %s;' % (f.opcode_real_name())
			print '        const GLuint cmdlenLarge = cmdlen + 4;'
			print '        (void) memcpy((void *)(gc->pc + 0), (void *)(&op), 4);'
			print '        (void) memcpy((void *)(gc->pc + 4), (void *)(&cmdlenLarge), 4);'
			offset = self.common_emit_args(f, "gc->pc", indent, 8, 1)
			
			p = f.variable_length_parameter()
			print '        __glXSendLargeCommand(gc, gc->pc, %u, %s, %s);' % (offset + 8, p.name, p.size_string())
			print '    }'

		print '}'
		print ''
		return


class PrintGlxProtoInit_c(GlxProto):
	def __init__(self):
		GlxProto.__init__(self)
		self.last_category = ""
		self.license = license.bsd_license_template % ( \
"""Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
(C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")


	def printRealHeader(self):
		print """/**
 * \\file indirect_init.c
 * Initialize indirect rendering dispatch table.
 *
 * \\author Kevin E. Martin <kevin@precisioninsight.com>
 * \\author Brian Paul <brian@precisioninsight.com>
 * \\author Ian Romanick <idr@us.ibm.com>
 */

#include "indirect_init.h"
#include "indirect.h"
#include "glapi.h"


/**
 * No-op function used to initialize functions that have no GLX protocol
 * support.
 */
static int NoOp(void)
{
    return 0;
}

/**
 * Create and initialize a new GL dispatch table.  The table is initialized
 * with GLX indirect rendering protocol functions.
 */
__GLapi * __glXNewIndirectAPI( void )
{
    __GLapi *glAPI;
    GLuint entries;

    entries = _glapi_get_dispatch_table_size();
    glAPI = (__GLapi *) Xmalloc(entries * sizeof(void *));

    /* first, set all entries to point to no-op functions */
    {
       int i;
       void **dispatch = (void **) glAPI;
       for (i = 0; i < entries; i++) {
          dispatch[i] = (void *) NoOp;
       }
    }

    /* now, initialize the entries we understand */"""

	def printRealFooter(self):
		print """
    return glAPI;
}
"""

	def printFunction(self, f):
		if f.fn_offset < 0 or f.ignore: return
		
		if f.category != self.last_category:
			self.last_category = f.category
			print ''
			print '    /* %s */' % (self.last_category)
			print ''
			
		print '    glAPI->%s = __indirect_gl%s;' % (f.name, f.name)


class PrintGlxProtoInit_h(GlxProto):
	def __init__(self):
		GlxProto.__init__(self)
		self.last_category = ""
		self.license = license.bsd_license_template % ( \
"""Copyright 1998-1999 Precision Insight, Inc., Cedar Park, Texas.
(C) Copyright IBM Corporation 2004""", "PRECISION INSIGHT, IBM")


	def printRealHeader(self):
		print """
/**
 * \\file
 * Prototypes for indirect rendering functions.
 *
 * \\author Kevin E. Martin <kevin@precisioninsight.com>
 * \\author Ian Romanick <idr@us.ibm.com>
 */

#if !defined( _INDIRECT_H_ )
#  define _INDIRECT_H_

"""
		printVisibility( "HIDDEN", "hidden" )


	def printRealFooter(self):
		print "#  undef HIDDEN"
		print "#endif /* !defined( _INDIRECT_H_ ) */"

	def printFunction(self, f):
		if f.fn_offset < 0 or f.ignore: return
		print 'extern HIDDEN %s __indirect_gl%s(%s);' % (f.fn_return_type, f.name, f.get_parameter_string())


class PrintGlxSizeStubs(GlxProto):
	def __init__(self):
		GlxProto.__init__(self)
		self.license = license.bsd_license_template % ( "(C) Copyright IBM Corporation 2004", "IBM")
		self.aliases = []
		self.glx_enum_sigs = {}

	def printRealHeader(self):
		print ''
		print '#include <GL/gl.h>'
		print '#include "indirect_size.h"'
		
		print ''
		printPure()
		print ''
		printFastcall()
		print ''
		printVisibility( "INTERNAL", "internal" )
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


				
class PrintGlxSizeStubs_h(GlxProto):
	def __init__(self):
		GlxProto.__init__(self)
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
		printPure();
		print ''
		printFastcall();
		print ''
		printVisibility( "INTERNAL", "internal" );
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
			print 'extern INTERNAL GLint __gl%s_size(GLenum) PURE FASTCALL;' % (f.name)


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

	if mode == "proto":
		dh = PrintGlxProtoStubs()
	elif mode == "init_c":
		dh = PrintGlxProtoInit_c()
	elif mode == "init_h":
		dh = PrintGlxProtoInit_h()
	elif mode == "size_c":
		dh = PrintGlxSizeStubs()
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
