#!/usr/bin/python2
# -*- Mode: Python; py-indent-offset: 8 -*-

# (C) Copyright Zack Rusin 2005
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
#    Zack Rusin <zack@kde.org>

from xml.sax import saxutils
from xml.sax import make_parser
from xml.sax.handler import feature_namespaces

import license
import gl_XML
import sys, getopt

class PrintGlEnums(gl_XML.FilterGLAPISpecBase):
	name = "gl_enums.py (from Mesa)"

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2005 Brian Paul All Rights Reserved.""", "BRIAN PAUL")


	def printRealHeader(self):
		print '#include "glheader.h"'
		print '#include "enums.h"'
		print '#include "imports.h"'
		print ''
		print 'typedef struct {'
		print '   const char *c;'
		print '   int n;'
		print '} enum_elt;'
		print ''
		print 'static const enum_elt all_enums[] ='
		print '{'
		return

	def printBody(self):
		print """
};

#define Elements(x) sizeof(x)/sizeof(*x)

typedef int (*cfunc)(const void *, const void *);

/* note the extra level of indirection
 */
static int compar_name( const enum_elt **a, const enum_elt **b )
{
   return _mesa_strcmp((*a)->c, (*b)->c);
}

static int compar_nr( const enum_elt *a, const enum_elt *b )
{
   return a->n - b->n;
}


static char token_tmp[20];

const char *_mesa_lookup_enum_by_nr( int nr )
{
   enum_elt tmp, *e;

   tmp.n = nr;

   e = (enum_elt *)bsearch( &tmp, all_enums, Elements(all_enums),
			     sizeof(*all_enums), (cfunc) compar_nr );

   if (e) {
      return e->c;
   }
   else {
      /* this isn't re-entrant safe, no big deal here */
      _mesa_sprintf(token_tmp, "0x%x", nr);
      return token_tmp;
   }
}

int _mesa_lookup_enum_by_name( const char *symbol )
{
   enum_elt tmp, *e, **f;
   static const enum_elt * const index1[] = {"""

	def printRealFooter(self):
		print """   };

   if (!symbol)
      return 0;

   tmp.c = symbol;
   e = &tmp;

   f = (enum_elt **)bsearch( &e, index1, Elements(all_enums),
			    sizeof(*index1), (cfunc) compar_name );

   return f ? (*f)->n : -1;
}

"""
		return

	def printFunctions(self):
		self.enums.sort(lambda x,y: x.value - y.value)
		for enum in self.enums:
			print '   { "%s", 0x%X },' % (enum.name, enum.value)
		nameEnums = self.enums[:]
		self.printBody();
		self.enums.sort(lambda x,y: cmp(x.name, y.name))
		for enum in self.enums:
			print '      &all_enums[%d], ' % (nameEnums.index(enum))
		return


def show_usage():
	print "Usage: %s [-f input_file_name]" % sys.argv[0]
	sys.exit(1)

if __name__ == '__main__':
	file_name = "gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	dh = PrintGlEnums()

	parser = make_parser()
	parser.setFeature(feature_namespaces, 0)
	parser.setContentHandler(dh)

	f = open(file_name)

	dh.printHeader()
	parser.parse(f)
	dh.printFooter()
