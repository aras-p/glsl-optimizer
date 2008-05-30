#!/usr/bin/env python
##########################################################################
# 
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
# 
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
# 
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
# 
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
# 
##########################################################################


import sys
import optparse
import re
import struct


__version__ = '0.1'

verbose = False


class ParseError(Exception):
	pass


class MsvcDemangler:
	# http://www.kegel.com/mangle.html

	def __init__(self, symbol):
		self._symbol = symbol
		self._pos = 0

	def lookahead(self):
		return self._symbol[self._pos]

	def consume(self):
		ret = self.lookahead()
		self._pos += 1
		return ret
	
	def match(self, c):
		if self.lookahead() != c:
			raise ParseError
		self.consume()

	def parse(self):
		self.match('?')
		name = self.parse_name()
		qualifications = self.parse_qualifications()
		return '::'.join(qualifications + [name])

	def parse_name(self):
		if self.lookahead() == '?':
			return self.consume() + self.consume()
		else:
			name = self.parse_id()
			self.match('@')
			return name

	def parse_qualifications(self):
		qualifications = []
		while self.lookahead() != '@':
			name = self.parse_id()
			qualifications.append(name)
			self.match('@')
		return qualifications

	def parse_id(self):
		s = ''
		while True:
			c = self.lookahead()
			if c.isalnum() or c in '_':
				s += c
				self.consume()
			else:
				break
		return s


def demangle(name):
	if name.startswith('_'):
		name = name[1:]
		idx = name.rfind('@')
		if idx != -1 and name[idx+1:].isdigit():
			name = name[:idx]
		return name
	if name.startswith('?'):
		demangler = MsvcDemangler(name)
		return demangler.parse()

		return name
	return name


class Profile:

	def __init__(self):
		self.symbols = []
		self.symbol_cache = {}
		self.base_addr = None
		self.functions = {}
		self.last_stamp = 0
		self.stamp_base = 0
	
	def unwrap_stamp(self, stamp):
		if stamp < self.last_stamp:
			self.stamp_base += 1 << 32
		self.last_stamp = stamp
		return self.stamp_base + stamp

	def read_map(self, mapfile):
		# See http://msdn.microsoft.com/en-us/library/k7xkk3e2.aspx
		last_addr = 0
		last_name = 0
		for line in file(mapfile, "rt"):
			fields = line.split()
			try:
				section_offset, name, addr, type, lib_object = fields
			except ValueError:
				continue
			if type != 'f':
				continue
			addr = int(addr, 16)
			name = demangle(name)
			if last_addr == addr:
				# TODO: handle collapsed functions
				#assert last_name == name
				continue
			self.symbols.append((addr, name))
			last_addr = addr
			last_name = name

		# sort symbols
		self.symbols.sort(key = lambda (addr, name): addr)

	def lookup_addr(self, addr):
		try:
			return self.symbol_cache[addr]
		except KeyError:
			pass

		tolerance = 4196
		s, e = 0, len(self.symbols)
		while s != e:
			i = (s + e)//2
			start_addr, name = self.symbols[i]
			try:
				end_addr, next_name = self.symbols[i + 1]
			except IndexError:
				end_addr = start_addr + tolerance
			if addr < start_addr:
				e = i
				continue
			if addr == end_addr:
				return next_name
			if addr > end_addr:
				s = i
				continue
			return name
		return "0x%08x" % addr

	def lookup_symbol(self, name):
		for symbol_addr, symbol_name in self.symbols:
			if name == symbol_name:
				return symbol_addr
		return 0

	def read_data(self, data):
		# TODO: compute these automatically
		caller_overhead = 672 - 2*144 # __debug_profile_reference2 - 2*__debug_profile_reference1
		callee_overhead = 144 # __debug_profile_reference1
		callee_overhead -= 48 # tolerance
		caller_overhead = callee_overhead

		fp = file(data, "rb")
		entry_format = "II"
		entry_size = struct.calcsize(entry_format)
		stack = []
		last_stamp = 0
		delta = 0
		while True:
			entry = fp.read(entry_size)
			if len(entry) < entry_size:
				break
			addr_exit, stamp = struct.unpack(entry_format, entry)
			if addr_exit == 0 and stamp == 0:
				break
			addr = addr_exit & 0xfffffffe
			exit = addr_exit & 0x00000001

			if self.base_addr is None:
				ref_addr = self.lookup_symbol('__debug_profile_reference2')
				if ref_addr:
					self.base_addr = addr - ref_addr
				else:
					self.base_addr = 0
				#print hex(self.base_addr)
			rel_addr = addr - self.base_addr
			#print hex(addr - self.base_addr)

			name = self.lookup_addr(rel_addr)
			stamp = self.unwrap_stamp(stamp)

			delta += stamp - last_stamp

			if not exit:
				if verbose >= 2:
					print "%10u >> 0x%08x" % (stamp, addr)
				if verbose:
					print "%10u >> %s" % (stamp, name)
				delta -= caller_overhead
				stack.append((name, stamp, delta))
				delta = 0
			else:
				if verbose >= 2:
					print "%10u << 0x%08x" % (stamp, addr)
				if len(stack):
					self_time = delta - callee_overhead
					entry_name, entry_stamp, delta = stack.pop()
					if entry_name != name:
						if verbose:
							print "%10u << %s" % (stamp, name)
						#assert entry_name == name
						break
					total_time = stamp - entry_stamp
					self.functions[entry_name] = self.functions.get(entry_name, 0) + self_time
					if verbose:
						print "%10u << %s %+u" % (stamp, name, self_time)
				else:
					delta = 0

			last_stamp = stamp

	def write_report(self):
		total = sum(self.functions.values())
		results = self.functions.items()
		results.sort(key = lambda (name, time): -time)
		for name, time in results:
			perc = float(time)/float(total)*100.0
			print "%6.03f %s" % (perc, name)


def main():
		parser = optparse.OptionParser(
			usage="\n\t%prog [options] [file] ...",
			version="%%prog %s" % __version__)
		parser.add_option(
			'-m', '--map', metavar='FILE',
			type="string", dest="map",
			help="map file")
		parser.add_option(
			'-b', '--base', metavar='FILE',
			type="string", dest="base",
			help="base addr")
		parser.add_option(
			'-v', '--verbose',
			action="count",
			dest="verbose", default=0,
			help="verbose output")
		(options, args) = parser.parse_args(sys.argv[1:])

		global verbose
		verbose = options.verbose

		profile = Profile()
		if options.base is not None:
			profile.base_addr = int(options.base, 16)
		if options.map is not None:
			profile.read_map(options.map)
		for arg in args:
			profile.read_data(arg)
		profile.write_report()


if __name__ == '__main__':
	main()

