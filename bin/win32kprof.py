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

from gprof2dot import Call, Function, Profile
from gprof2dot import CALLS, SAMPLES, TIME, TIME_RATIO, TOTAL_TIME, TOTAL_TIME_RATIO
from gprof2dot import DotWriter, TEMPERATURE_COLORMAP


__version__ = '0.1'


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


class Reader:

    def __init__(self):
        self.symbols = []
        self.symbol_cache = {}
        self.base_addr = None
    
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
            section, offset = section_offset.split(':')
            addr = int(offset, 16)
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
                return next_name, addr - start_addr
            if addr > end_addr:
                s = i
                continue
            return name, addr - start_addr
        raise ValueError

    def lookup_symbol(self, name):
        for symbol_addr, symbol_name in self.symbols:
            if name == symbol_name:
                return symbol_addr
        return 0

    def read_data(self, data):
        profile = Profile()

        fp = file(data, "rb")
        entry_format = "IIII"
        entry_size = struct.calcsize(entry_format)
        caller = None
        caller_stack = []
        while True:
            entry = fp.read(entry_size)
            if len(entry) < entry_size:
                break
            caller_addr, callee_addr, samples_lo, samples_hi = struct.unpack(entry_format, entry)
            if caller_addr == 0 and callee_addr == 0:
                continue

            if self.base_addr is None:
                ref_addr = self.lookup_symbol('___debug_profile_reference@0')
                if ref_addr:
                    self.base_addr = (caller_addr - ref_addr) & ~(options.align - 1)
                else:
                    self.base_addr = 0
                sys.stderr.write('Base addr: %08x\n' % self.base_addr)

            samples = (samples_hi << 32) | samples_lo
            
            try:
                caller_raddr = caller_addr - self.base_addr
                caller_sym, caller_ofs = self.lookup_addr(caller_raddr)

                try:
                    caller = profile.functions[caller_sym]
                except KeyError:
                    caller_name = demangle(caller_sym)
                    caller = Function(caller_sym, caller_name)
                    profile.add_function(caller)
                    caller[CALLS] = 0
                    caller[SAMPLES] = 0
            except ValueError:
                caller = None

            if not callee_addr:
                if caller:
                    caller[SAMPLES] += samples
            else:
                callee_raddr = callee_addr - self.base_addr
                callee_sym, callee_ofs = self.lookup_addr(callee_raddr)

                try:
                    callee = profile.functions[callee_sym]
                except KeyError:
                    callee_name = demangle(callee_sym)
                    callee = Function(callee_sym, callee_name)
                    profile.add_function(callee)
                    callee[CALLS] = samples
                    callee[SAMPLES] = 0
                else:
                    callee[CALLS] += samples

                if caller is not None:
                    try:
                        call = caller.calls[callee.id]
                    except KeyError:
                        call = Call(callee.id)
                        call[CALLS] = samples
                        caller.add_call(call)
                    else:
                        call[CALLS] += samples
            
            if options.verbose:
                if not callee_addr:
                    sys.stderr.write('%s+%u: %u\n' % (caller_sym, caller_ofs, samples))
                else:
                    sys.stderr.write('%s+%u -> %s+%u: %u\n' % (caller_sym, caller_ofs, callee_sym, callee_ofs, samples))

        # compute derived data
        profile.validate()
        profile.find_cycles()
        profile.aggregate(SAMPLES)
        profile.ratio(TIME_RATIO, SAMPLES)
        profile.call_ratios(CALLS)
        profile.integrate(TOTAL_TIME_RATIO, TIME_RATIO)

        return profile


def main():
    parser = optparse.OptionParser(
        usage="\n\t%prog [options] [file] ...",
        version="%%prog %s" % __version__)
    parser.add_option(
        '-a', '--align', metavar='NUMBER',
        type="int", dest="align", default=16,
        help="section alignment")
    parser.add_option(
        '-m', '--map', metavar='FILE',
        type="string", dest="map",
        help="map file")
    parser.add_option(
        '-b', '--base', metavar='FILE',
        type="string", dest="base",
        help="base addr")
    parser.add_option(
        '-n', '--node-thres', metavar='PERCENTAGE',
        type="float", dest="node_thres", default=0.5,
        help="eliminate nodes below this threshold [default: %default]")
    parser.add_option(
        '-e', '--edge-thres', metavar='PERCENTAGE',
        type="float", dest="edge_thres", default=0.1,
        help="eliminate edges below this threshold [default: %default]")
    parser.add_option(
        '-v', '--verbose',
        action="count",
        dest="verbose", default=0,
        help="verbose output")

    global options
    (options, args) = parser.parse_args(sys.argv[1:])

    reader = Reader()
    if options.base is not None:
        reader.base_addr = int(options.base, 16)
    if options.map is not None:
        reader.read_map(options.map)
    for arg in args:
        profile = reader.read_data(arg)
        profile.prune(options.node_thres/100.0, options.edge_thres/100.0)
        output = sys.stdout
        dot = DotWriter(output)
        colormap = TEMPERATURE_COLORMAP
        dot.graph(profile, colormap)


if __name__ == '__main__':
    main()

