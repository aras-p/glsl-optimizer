#!/usr/bin/env python

# (C) Copyright IBM Corporation 2004, 2005
# (C) Copyright Apple Inc. 2011
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
#    Jeremy Huddleston <jeremyhu@apple.com>
#
# Based on code ogiginally by:
#    Ian Romanick <idr@us.ibm.com>

import license
import gl_XML, glX_XML
import sys, getopt

header = """
#include <dlfcn.h>
#include <stdlib.h>
#include <stdio.h>

#include <GL/gl.h>

#include "glapi.h"
#include "glapitable.h"
#include "main/dispatch.h"

struct _glapi_table *
_glapi_create_table_from_handle(void *handle, const char *symbol_prefix) {
    struct _glapi_table *disp = calloc(1, sizeof(struct _glapi_table));
    char symboln[512];

    if(!disp)
         return NULL;
"""

footer = """
    return disp;
}
"""

body_template = """
    if(!disp->%(name)s) {
         snprintf(symboln, sizeof(symboln), "%%s%(entry_point)s", symbol_prefix);
         SET_%(name)s(disp, dlsym(handle, symboln));
    }
"""

class PrintCode(gl_XML.gl_print_base):

	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "gl_gen_table.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004, 2005
(C) Copyright Apple Inc 2011""", "BRIAN PAUL, IBM")

		return


	def get_stack_size(self, f):
		size = 0
		for p in f.parameterIterator():
			if p.is_padding:
				continue

			size += p.get_stack_size()

		return size


	def printRealHeader(self):
		print header
		return


	def printRealFooter(self):
		print footer
		return


	def printBody(self, api):
		for f in api.functionIterateByOffset():
			for entry_point in f.entry_points:
				vars = { 'entry_point' : entry_point,
				         'name' : f.name }

				print body_template % vars
		return

def show_usage():
	print "Usage: %s [-f input_file_name]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"

	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "m:f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	printer = PrintCode()

	api = gl_XML.parse_GL_API(file_name, glX_XML.glx_item_factory())
	printer.Print(api)
