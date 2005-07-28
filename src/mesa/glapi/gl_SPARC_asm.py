#!/usr/bin/env python

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

import gl_XML, license
import sys, getopt

class PrintGenericStubs(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)
		self.name = "gl_SPARC_asm.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2003  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")


	def printRealHeader(self):
		print '#include "glapioffsets.h"'
		print ''
		print '#ifdef __arch64__'
		print '#  define GL_STUB(fn,off)\t\t\t\t\\'
		print 'fn:\t\t\t\t\t\\'
		print '\tsethi\t%hi(0xDEADBEEF), %g4 ;\t\t\t\\'
		print '\tsethi\t%hi(0xDEADBEEF), %g1 ;\t\t\t\\'
		print '\tor\t%g4, %lo(0xDEADBEEF), %g4 ;\t\t\\'
		print '\tor\t%g1, %lo(0xDEADBEEF), %g1 ;\t\t\\'
		print '\tsllx\t%g4, 32, %g4 ;\t\t\t\t\\'
		print '\tldx\t[%g1 + %g4], %g1 ;\t\t\t\\'
		print '\tsethi\t%hi(8 * off), %g4 ;\t\t\t\\'
		print '\tor\t%g4, %lo(8 * off), %g4 ;\t\t\\'
		print '\tldx\t[%g1 + %g4], %g5 ;\t\t\t\\'
		print '\tjmpl\t%g5, %g0 ;\t\t\t\t\\'
		print '\tnop'
		print '#else'
		print '#  define GL_STUB(fn,off)\t\t\t\t\\'
		print 'fn:\t\t\t\t\t\\'
		print '\tsethi\t%hi(0xDEADBEEF), %g1 ;\t\t\t\\'
		print '\tld\t[%g1 + %lo(0xDEADBEEF)], %g1 ;\t\t\\'
		print '\tld\t[%g1 + (4 * off)], %g5 ;\t\t\\'
		print '\tjmpl\t%g5, %g0 ;\t\t\t\t\\'
		print '\tnop'
		print '#endif'
		print ''
		print '#define GL_STUB_ALIAS(fn,alias) fn = alias'
		print ''
		print '.text'
		print '.align 32'
		print '\t\t.globl __glapi_sparc_icache_flush ; .type __glapi_sparc_icache_flush,#function'
		print '__glapi_sparc_icache_flush: /* %o0 = insn_addr */'
		print '\tflush\t%o0'
		print '\tretl'
		print '\tnop'
		print ''
		print '.data'
		print '.align 64'
		print ''
		return


	def printBody(self, api):
		for f in api.functionIterateByOffset():
			print '\t\t.globl gl%s ; .type gl%s,#function' % (f.name, f.name)

		print '\t\t.globl _mesa_sparc_glapi_begin ; .type _mesa_sparc_glapi_begin,#function'
		print '_mesa_sparc_glapi_begin:'
		print ''

		for f in api.functionIterateByOffset():
			print '\tGL_STUB(gl%s, _gloffset_%s)' % (f.name, f.name)

		print ''
		print '\t\t.globl _mesa_sparc_glapi_end ; .type _mesa_sparc_glapi_end,#function'
		print '_mesa_sparc_glapi_end:'
		print ''


		for f in api.functionIterateByOffset():
			for n in f.entry_points:
				if n != f.name:
					print '\t.globl gl%s ; .type gl%s,#function ; gl%s = gl%s' % (n, n, n, f.name)

		return


def show_usage():
	print "Usage: %s [-f input_file_name] [-m output_mode]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
	mode = "generic"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "m:f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == '-m':
			mode = val
		elif arg == "-f":
			file_name = val

	if mode == "generic":
		printer = PrintGenericStubs()
	else:
		print "ERROR: Invalid mode \"%s\" specified." % mode
		show_usage()

	api = gl_XML.parse_GL_API( file_name )

	printer.Print( api )
