#!/usr/bin/env python


# Helper for the getprocaddress.c test.

import sys, getopt, re
sys.path.append("../../src/mesa/glapi/" )
import gl_XML
import license


def FindTestFunctions():
	"""Scan getprocaddress.c for lines that start with "test_" to find
	extension function tests.  Return a list of names found."""
	functions = []
	f = open("getprocaddress.c")
	if not f:
		return functions
	for line in f.readlines():
		v = re.search("^test_([a-zA-Z0-9]+)", line)
		if v:
			func = v.group(1)
			functions.append(func)
	f.close
	return functions


class PrintExports(gl_XML.gl_print_base):
	def __init__(self):
		gl_XML.gl_print_base.__init__(self)

		self.name = "getprocaddress.py (from Mesa)"
		self.license = license.bsd_license_template % ( \
"""Copyright (C) 1999-2001  Brian Paul   All Rights Reserved.
(C) Copyright IBM Corporation 2004""", "BRIAN PAUL, IBM")

		self.tests = FindTestFunctions()
		self.prevCategory = ""
		return


	def printRealHeader(self):
		print """
struct name_test_pair {
   const char *name;
   GLboolean (*test)(generic_func);
};
   
static struct name_test_pair functions[] = {"""

	def printBody(self, api):
		prev_category = None
		

		for f in api.functionIterateByCategory():
			[category, num] = api.get_category_for_name( f.name )
			if category != prev_category:
				print '   { "-%s", NULL},' % category
				prev_category = category
			
			test = "NULL"
			for name in f.entry_points:
				if name in self.tests:
					test = "test_%s" % name
					break

			print '   { "gl%s", %s },' % (f.name, test)

		print ''
		print '   { NULL, NULL }'
		print '};'
		print ''
		return


if __name__ == '__main__':
	file_name = "../../src/mesa/glapi/gl_API.xml"
    
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "f:")
	except Exception,e:
		show_usage()

	for (arg,val) in args:
		if arg == "-f":
			file_name = val

	printer = PrintExports()

	api = gl_XML.parse_GL_API( file_name, gl_XML.gl_item_factory() )

	printer.Print( api )
