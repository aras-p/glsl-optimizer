#!/usr/bin/env python2

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


# This script is used to run api_speed against several different libGL
# libraries and compare the results.  See the show_usage function for more
# details on how to use it.


import re, os, sys, getopt

class results:
	def process_file(self, f):
		self.cycles = {}
		self.iterations = -1

		for line in f.readlines():
			m = re.match("(\d+) calls to (.{20}) required (\d+) cycles.", line)

			if self.iterations != -1 and int(m.group(1)) != self.iterations:
				raise

			# This could be done with lstrip, but the version of
			# the Python library on my system doesn't have it.
			# The installed version of Python is quite old. :(

			temp = m.group(2)
			function_name = None
			for i in range(len(temp)):
				if temp[i] != ' ':
					function_name = temp[i:]
					break

			if function_name == None:
				raise

			self.cycles[ function_name ] = int(m.group(3))
			self.iterations = int(m.group(1))


	def show_results(self):
		for name in self.cycles:
			print "%s -> %f" % (name, float(self.cycles[name]) / self.iterations)


	def compare_results(self, other):
		for name in self.cycles:
			if other.cycles.has_key(name):
				a = float(self.cycles[name])  / float(self.iterations)
				b = float(other.cycles[name]) / float(other.iterations)
				if abs( a ) < 0.000001:
				    print "a = %f, b = %f" % (a, b)
				else:
				    p = (100.0 * b / a) - 100.0
				    print "%- 20s %7.2f - %7.2f = % -6.2f (%+.1f%%)" % (name, a, b, a - b, p)
		return


def make_execution_string(lib, iterations):
	if lib == None:
		return "./api_speed %u" % (iterations)
	else:
		return "LD_PRELOAD=%s ./api_speed %u" % (lib, iterations)


def show_usage():
	print """Usage: %s [-i iterations] {library ...}

The full path to one or more libGL libraries (including the full name of the
library) can be included on the command-line.  Each library will be tested,
and the results compared.  The first library listed will be used as the
"base line" for all comparisons.""" % (sys.argv[0])
	sys.exit(1)


if __name__ == '__main__':
	try:
		(args, trail) = getopt.getopt(sys.argv[1:], "i:")
	except Exception,e:
		show_usage()

	iterations = 1000000
	try:
		for (arg,val) in args:
			if arg == "-i":
				iterations = int(val)
	except Exception,e:
		show_usage()


	# If no libraries were specifically named, just run the test against
	# the default system libGL.

	if len(trail) == 0:
		trail.append(None)


	result_array = []
	names = []

	for lib in trail:
		s = make_execution_string( lib, iterations )
		r = results()
		r.process_file( os.popen(s) )
		names.append(lib)
		result_array.append(r)


	# If the test was only run against one library, just show the results
	# of the test run.  Otherwise, compare each successive run against
	# the first run.

	if len( result_array ) == 1:
		result_array[0].show_results()
	else:
		for i in range(1, len( result_array )):
			print "%s vs. %s" % (names[0], names[i])
			result_array[0].compare_results( result_array[i] )
			print ""
