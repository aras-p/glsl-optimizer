#!/usr/bin/env python

# $Id: glsparcasm.py,v 1.3 2001/06/06 22:55:28 davem69 Exp $

# Mesa 3-D graphics library
# Version:  3.5
# 
# Copyright (C) 1999-2000  Brian Paul   All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
# 
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# BRIAN PAUL BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN
# AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
# CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.


# Generate the glapi_sparc.S assembly language file.
#
# Usage:
#    glsparcasm.py >glapi_sparc.S
#
# Dependencies:
#    The gl.spec file from the SI must be in the current directory.
#
# Brian Paul      11 May 2000
# David S. Miller  4 Jun 2001

import string
import re


def PrintHead():
	print '/* DO NOT EDIT - This file generated automatically with glsparcasm.py script */'
	print '#include "glapioffsets.h"'
	print ''
	print '#define GL_PREFIX(n) gl##n'
	print '#define GLOBL_FN(x) .globl x ; .type x,@function'
	print ''
	print '/* The _glapi_Dispatch symbol addresses get relocated into the'
	print ' * sethi/or instruction sequences below at library init time.'
	print ' */'
	print ''
	print ''
	print '.text'
	print '.align 32'
	print '.globl __glapi_sparc_icache_flush'
	print '__glapi_sparc_icache_flush: /* %o0 = insn_addr */'
	print '\tflush\t%o0'
	print '\tretl'
	print '\t nop'
	print ''
	print '.data'
	print '.align 64'
	print ''
	print '.globl _mesa_sparc_glapi_begin'
	print '.type _mesa_sparc_glapi_begin,@function'
	print '_mesa_sparc_glapi_begin:'
	print ''
	return
#endif

def PrintTail():
	print '\t nop'
	print ''
	print '.globl _mesa_sparc_glapi_end'
	print '.type _mesa_sparc_glapi_end,@function'
	print '_mesa_sparc_glapi_end:'
	print ''
#endif


def GenerateDispatchCode(name, offset):
	print ''
	print "GLOBL_FN(GL_PREFIX(%s))" % (name)
	print "GL_PREFIX(%s):" % (name)
	print '#ifdef __sparc_v9__'
	print '\tsethi\t%hi(0x00000000), %g2'
	print '\tsethi\t%hi(0x00000000), %g1'
	print '\tor\t%g2, %lo(0x00000000), %g2'
	print '\tor\t%g1, %lo(0x00000000), %g1'
	print '\tsllx\t%g2, 32, %g2'
	print '\tldx\t[%g1 + %g2], %g1'
	print "\tsethi\t%%hi(8 * _gloffset_%s), %%g2" % (offset)
	print "\tor\t%%g2, %%lo(8 * _gloffset_%s), %%g2" % (offset)
	print '\tldx\t[%g1 + %g2], %g3'
	print '#else'
	print '\tsethi\t%hi(0x00000000), %g1'
	print '\tld\t[%g1 + %lo(0x00000000)], %g1'
	print "\tld\t[%%g1 + (4 * _gloffset_%s)], %%g3" % (offset)
	print '#endif'
	print '\tjmpl\t%g3, %g0'
#enddef


def FindAlias(list, funcName):
	for i in range(0, len(list)):
		entry = list[i]
		if entry[0] == funcName:
			return entry[1]
		#endif
	#endfor
	return ''
#enddef



def PrintDefines():
	functionPattern = re.compile('^[a-zA-Z0-9]+\(')
	functionNamePattern = re.compile('^[a-zA-Z0-9]+')

	funcName = ''
	functions = [ ]

	f = open('gl.spec')
	for line in f.readlines():

		m = functionPattern.match(line)
		if m:
			# extract funcName
			n = functionNamePattern.findall(line)
			funcName = n[0]

		m = string.split(line)
		if len(m) > 1:
			if m[0] == 'param':
				paramName = m[1]
			if m[0] == 'offset':
				if m[1] == '?':
					#print 'WARNING skipping', funcName
					noop = 0
				else:
					entry = [ funcName, funcName ]
					functions.append(entry)
				#endif
			elif m[0] == 'alias':
				aliasedName = FindAlias(functions, m[1])
				if aliasedName:
					entry = [ funcName, aliasedName ]
					functions.append(entry)
				else:
					print 'WARNING: alias to unknown function:', aliasedName
				#endif
			#endif
		#endif
	#endfor

	# Now generate the assembly dispatch code
	for i in range(0, len(functions)):
		entry = functions[i]
		GenerateDispatchCode( entry[0], entry[1] )

#enddef



PrintHead()
PrintDefines()
PrintTail()
