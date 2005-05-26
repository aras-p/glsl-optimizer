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

import license
import gl_XML
import sys, getopt

class PrintGlEnums(gl_XML.FilterGLAPISpecBase):

	def __init__(self):
		gl_XML.FilterGLAPISpecBase.__init__(self)

		self.name = "gl_enums.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2005 Brian Paul All Rights Reserved.""", "BRIAN PAUL")
		self.enum_table = {}


	def printRealHeader(self):
		print '#include "glheader.h"'
		print '#include "enums.h"'
		print '#include "imports.h"'
		print ''
		print 'typedef struct {'
		print '   size_t offset;'
		print '   int n;'
		print '} enum_elt;'
		print ''
		return

	def printBody(self):
		print """
#define Elements(x) sizeof(x)/sizeof(*x)

typedef int (*cfunc)(const void *, const void *);

/**
 * Compare a key name to an element in the \c all_enums array.
 *
 * \c bsearch always passes the key as the first parameter and the pointer
 * to the array element as the second parameter.  We can elimiate some
 * extra work by taking advantage of that fact.
 *
 * \param a  Pointer to the desired enum name.
 * \param b  Pointer to an element of the \c all_enums array.
 */
static int compar_name( const char *a, const enum_elt *b )
{
   return _mesa_strcmp( a, & enum_string_table[ b->offset ] );
}

/**
 * Compare a key enum value to an element in the \c all_enums array.
 *
 * \c bsearch always passes the key as the first parameter and the pointer
 * to the array element as the second parameter.  We can elimiate some
 * extra work by taking advantage of that fact.
 *
 * \param a  Pointer to the desired enum name.
 * \param b  Pointer to an index into the \c all_enums array.
 */
static int compar_nr( const int *a, const unsigned *b )
{
   return a[0] - all_enums[*b].n;
}


static char token_tmp[20];

const char *_mesa_lookup_enum_by_nr( int nr )
{
   unsigned * i;

   i = (unsigned *)bsearch( & nr, reduced_enums, Elements(reduced_enums),
                            sizeof(reduced_enums[0]), (cfunc) compar_nr );

   if ( i != NULL ) {
      return & enum_string_table[ all_enums[ *i ].offset ];
   }
   else {
      /* this is not re-entrant safe, no big deal here */
      _mesa_sprintf(token_tmp, "0x%x", nr);
      return token_tmp;
   }
}

int _mesa_lookup_enum_by_name( const char *symbol )
{
   enum_elt * f = NULL;

   if ( symbol != NULL ) {
      f = (enum_elt *)bsearch( symbol, all_enums, Elements(all_enums),
			       sizeof( enum_elt ), (cfunc) compar_name );
   }

   return (f != NULL) ? f->n : -1;
}

"""
		return

	def printFunctions(self):
		keys = self.enum_table.keys()
		keys.sort()

		name_table = []
		enum_table = {}

		for enum in keys:
			low_pri = 9
			for [name, pri] in self.enum_table[ enum ]:
				name_table.append( [name, enum] )

				if pri < low_pri:
					low_pri = pri
					enum_table[enum] = name
						

		name_table.sort()

		string_offsets = {}
		i = 0;
		print 'static const char enum_string_table[] = '
		for [name, enum] in name_table:
			print '   "%s\\0"' % (name)
			string_offsets[ name ] = i
			i += len(name) + 1

		print '   ;'
		print ''


		print 'static const enum_elt all_enums[%u] =' % (len(name_table))
		print '{'
		for [name, enum] in name_table:
			print '   { % 5u, 0x%08X }, /* %s */' % (string_offsets[name], enum, name)
		print '};'
		print ''

		print 'static const unsigned reduced_enums[%u] =' % (len(keys))
		print '{'
		for enum in keys:
			name = enum_table[ enum ]
			if [name, enum] not in name_table:
				print '      /* Error! %s, 0x%04x */ 0,' % (name, enum)
			else:
				i = name_table.index( [name, enum] )

				print '      % 4u, /* %s */' % (i, name)
		print '};'


		self.printBody();
		return


	def append(self, object_type, obj):
		if object_type == "enum":
			if obj.value not in self.enum_table:
				self.enum_table[ obj.value ] = []


			# Prevent duplicate names in the enum table.

			found_it = 0
			for [n, junk] in self.enum_table[ obj.value ]:
				if n == obj.name:
					found_it = 1
					break

			if not found_it:

				# Calculate a "priority" for this enum name.
				# When we lookup an enum by number, there may
				# be many possible names, but only one can be
				# returned.  The priority is used by this
				# script to select which name is the "best".
				#
				# Highest precedence is given to "core" name.
				# ARB extension names have the next highest,
				# followed by EXT extension names.  Vendor
				# extension names are the lowest.

				if obj.category.startswith( "GL_VERSION_" ):
					priority = 0
				elif obj.category.startswith( "GL_ARB_" ):
					priority = 1
				elif obj.category.startswith( "GL_EXT_" ):
					priority = 2
				else:
					priority = 3

				self.enum_table[ obj.value ].append( [obj.name, priority] )

		else:
			gl_XML.FilterGLAPISpecBase.append(self, object_type, obj)


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
	gl_XML.parse_GL_API( dh, file_name )
