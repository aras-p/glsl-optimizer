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
import glX_XML
import license
import sys, getopt


class PrintGlxProtoStubs(glX_XML.GlxProto):
	def __init__(self):
		glX_XML.GlxProto.__init__(self)
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
		glX_XML.printFastcall()
		glX_XML.printNoinline()
		print ''
		print '#if !defined __GNUC__ || __GNUC__ < 3'
		print '#  define __builtin_expect(x, y) x'
		print '#endif'
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
	if ((reply.length > 0) || reply_is_always_array) {
	    const GLint bytes = (reply_is_always_array) 
	      ? (4 * reply.length) : (reply.size * size);
	    const GLint extra = 4 - (bytes & 3);

	    _XRead(dpy, dest, bytes);
	    if ( extra < 4 ) {
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

static NOINLINE FASTCALL GLubyte *
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

static NOINLINE FASTCALL GLubyte *
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
		size = (n + 3) & ~3
		print """static FASTCALL NOINLINE void
generic_%u_byte( GLint rop, const void * ptr )
{
    __GLXcontext * const gc = __glXGetCurrentContext();
    const GLuint cmdlen = %u;

    emit_header(gc->pc, rop, cmdlen);
    (void) memcpy((void *)(gc->pc + 4), ptr, %u);
    gc->pc += cmdlen;
    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }
}
""" % (n, size + 4, size)


	def common_emit_one_arg(self, p, offset, pc, indent, adjust):
		t = p.p_type
		if p.is_array():
			src_ptr = p.name
		else:
			src_ptr = "&" + p.name

		print '%s    (void) memcpy((void *)(%s + %u), (void *)(%s), %s);' \
			% (indent, pc, offset + adjust, src_ptr, p.size_string() )

	def common_emit_args(self, f, pc, indent, adjust, skip_vla):
		offset = 0

		if skip_vla:
			r = 1
		else:
			r = 2

		for p in f.parameterIterator(1, r):
			self.common_emit_one_arg(p, offset, pc, indent, adjust)
			offset += p.size()

		return offset


	def large_emit_begin(self, indent, f):
		print '%s    const GLint op = %s;' % (indent, f.opcode_real_name())
		print '%s    const GLuint cmdlenLarge = cmdlen + 4;' % (indent)
		print '%s    GLubyte * const pc = __glXFlushRenderBuffer(gc, gc->pc);' % (indent)
		print '%s    (void) memcpy((void *)(pc + 0), (void *)(&cmdlenLarge), 4);' % (indent)
		print '%s    (void) memcpy((void *)(pc + 4), (void *)(&op), 4);' % (indent)
		return


	def common_func_print_just_header(self, f):
		print '#define %s %d' % (f.opcode_name(), f.opcode_value())

		print '%s' % (f.fn_return_type)
		print '__indirect_gl%s(%s)' % (f.name, f.get_parameter_string())
		print '{'


	def common_func_print_header(self, f):
		self.common_func_print_just_header(f)

		print '    __GLXcontext * const gc = __glXGetCurrentContext();'
		
		# The only reason that single and vendor private commands need
		# a variable called 'dpy' is becuase they use the SyncHandle
		# macro.  For whatever brain-dead reason, that macro is hard-
		# coded to use a variable called 'dpy' instead of taking a
		# parameter.

		if not f.glx_rop:
			print '    Display * const dpy = gc->currentDpy;'
			skip_condition = "dpy != NULL"
		elif f.can_be_large:
			skip_condition = "gc->currentDpy != NULL"
		else:
			skip_condition = None


		if f.fn_return_type != 'void':
			print '    %s retval = (%s) 0;' % (f.fn_return_type, f.fn_return_type)

		if f.count_parameters != None:
			print '    const GLuint compsize = __gl%s_size(%s);' % (f.name, f.count_parameters)

		print '    const GLuint cmdlen = %s;' % (f.command_length())

		if f.counter:
			if skip_condition:
				skip_condition = "(%s >= 0) && (%s)" % (f.counter, skip_condition)
			else:
				skip_condition = "%s >= 0" % (f.counter)


		if skip_condition:
			print '    if (__builtin_expect(%s, 1)) {' % (skip_condition)
			return 1
		else:
			return 0


	def printSingleFunction(self, f):
		self.common_func_print_header(f)

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

			print "       %s read_reply(dpy, %s, %s, %s);" % (return_str, output_size, output_str, aa)

		print '        UnlockDisplay(dpy); SyncHandle();'
		print '    }'
		print '    %s' % f.return_string()
		print '}'
		print ''
		return


	def printRenderFunction(self, f):
		# There is a class of GL functions that take a single pointer
		# as a parameter.  This pointer points to a fixed-size chunk
		# of data, and the protocol for this functions is very
		# regular.  Since they are so regular and there are so many
		# of them, special case them with generic functions.  On
		# x86, this saves about 26KB in the libGL.so binary.

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

		if self.common_func_print_header(f):
			indent = "    "
			trailer = "    }"
		else:
			indent = ""
			trailer = None

		if f.can_be_large:
			print '%s    if (cmdlen <= gc->maxSmallRenderCommandSize) {' % (indent)
			print '%s        if ( (gc->pc + cmdlen) > gc->bufEnd ) {' % (indent)
			print '%s            (void) __glXFlushRenderBuffer(gc, gc->pc);' % (indent)
			print '%s        }' % (indent)
			indent += "    "

		print '%s    emit_header(gc->pc, %s, cmdlen);' % (indent, f.opcode_real_name())

		self.common_emit_args(f, "gc->pc", indent, 4, 0)
		print '%s    gc->pc += cmdlen;' % (indent)
		print '%s    if (__builtin_expect(gc->pc > gc->limit, 0)) { (void) __glXFlushRenderBuffer(gc, gc->pc); }' % (indent)

		if f.can_be_large:
			print '%s}' % (indent)
			print '%selse {' % (indent)

			self.large_emit_begin(indent, f)
			offset = self.common_emit_args(f, "pc", indent, 8, 1)
			
			p = f.variable_length_parameter()
			print '%s    __glXSendLargeCommand(gc, pc, %u, %s, %s);' % (indent, offset + 8, p.name, p.size_string())
			print '%s}' % (indent)

		if trailer: print trailer
		print '}'
		print ''
		return


class PrintGlxProtoInit_c(glX_XML.GlxProto):
	def __init__(self):
		glX_XML.GlxProto.__init__(self)
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


class PrintGlxProtoInit_h(glX_XML.GlxProto):
	def __init__(self):
		glX_XML.GlxProto.__init__(self)
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
		glX_XML.printVisibility( "HIDDEN", "hidden" )


	def printRealFooter(self):
		print "#  undef HIDDEN"
		print "#endif /* !defined( _INDIRECT_H_ ) */"

	def printFunction(self, f):
		if f.fn_offset < 0 or f.ignore: return
		print 'extern HIDDEN %s __indirect_gl%s(%s);' % (f.fn_return_type, f.name, f.get_parameter_string())


class PrintGlxSizeStubs(glX_XML.GlxProto):
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
