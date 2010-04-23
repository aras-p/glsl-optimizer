#!/usr/bin/env python

# Mesa 3-D graphics library
# Version:  7.9
#
# Copyright (C) 2010 LunarG Inc.
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
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
# DEALINGS IN THE SOFTWARE.
#
# Authors:
#    Chia-I Wu <olv@lunarg.com>

import sys
import re
from optparse import OptionParser

# number of dynamic entries
ABI_NUM_DYNAMIC_ENTRIES = 256

class ABIEntry(object):
    """Represent an ABI entry."""

    _match_c_param = re.compile(
            '^(?P<type>[\w\s*]+?)(?P<name>\w+)(\[(?P<array>\d+)\])?$')

    def __init__(self, cols, attrs):
        self._parse(cols)

        self.slot = attrs['slot']
        self.hidden = attrs['hidden']
        self.alias = attrs['alias']

    def c_prototype(self):
        return '%s %s(%s)' % (self.c_return(), self.name, self.c_params())

    def c_return(self):
        ret = self.ret
        if not ret:
            ret = 'void'

        return ret

    def c_params(self):
        """Return the parameter list used in the entry prototype."""
        c_params = []
        for t, n, a in self.params:
            sep = '' if t.endswith('*') else ' '
            arr = '[%d]' % a if a else ''
            c_params.append(t + sep + n + arr)
        if not c_params:
            c_params.append('void')

        return ", ".join(c_params)

    def c_args(self):
        """Return the argument list used in the entry invocation."""
        c_args = []
        for t, n, a in self.params:
            c_args.append(n)

        return ", ".join(c_args)

    def _parse(self, cols):
        ret = cols.pop(0)
        if ret == 'void':
            ret = None

        name = cols.pop(0)

        params = []
        if not cols:
            raise Exception(cols)
        elif len(cols) == 1 and cols[0] == 'void':
            pass
        else:
            for val in cols:
                params.append(self._parse_param(val))

        self.ret = ret
        self.name = name
        self.params = params

    def _parse_param(self, c_param):
        m = self._match_c_param.match(c_param)
        if not m:
            raise Exception('unrecognized param ' + c_param)

        c_type = m.group('type').strip()
        c_name = m.group('name')
        c_array = m.group('array')
        c_array = int(c_array) if c_array else 0

        return (c_type, c_name, c_array)

    def __str__(self):
        return self.c_prototype()

    def __cmp__(self, other):
        # compare slot, alias, and then name
        res = cmp(self.slot, other.slot)
        if not res:
            if not self.alias:
                res = -1
            elif not other.alias:
                res = 1

            if not res:
                res = cmp(self.name, other.name)

        return res

def abi_parse_line(line):
    cols = [col.strip() for col in line.split(',')]

    attrs = {
            'slot': -1,
            'hidden': False,
            'alias': None,
    }

    # extract attributes from the first column
    vals = cols[0].split(':')
    while len(vals) > 1:
        val = vals.pop(0)
        if val.startswith('slot='):
            attrs['slot'] = int(val[5:])
        elif val == 'hidden':
            attrs['hidden'] = True
        elif val.startswith('alias='):
            attrs['alias'] = val[6:]
        elif not val:
            pass
        else:
            raise Exception('unknown attribute %s' % val)
    cols[0] = vals[0]

    return (attrs, cols)

def abi_parse(filename):
    """Parse a CSV file for ABI entries."""
    fp = open(filename) if filename != '-' else sys.stdin
    lines = [line.strip() for line in fp.readlines()
            if not line.startswith('#') and line.strip()]

    entry_dict = {}
    next_slot = 0
    for line in lines:
        attrs, cols = abi_parse_line(line)

        # post-process attributes
        if attrs['alias']:
            try:
                ent = entry_dict[attrs['alias']]
                slot = ent.slot
            except KeyError:
                raise Exception('failed to alias %s' % attrs['alias'])
        else:
            slot = next_slot
            next_slot += 1

        if attrs['slot'] < 0:
            attrs['slot'] = slot
        elif attrs['slot'] != slot:
            raise Exception('invalid slot in %s' % (line))

        ent = ABIEntry(cols, attrs)
        if entry_dict.has_key(ent.name):
            raise Exception('%s is duplicated' % (ent.name))
        entry_dict[ent.name] = ent

    entries = entry_dict.values()
    entries.sort()

    # sanity check
    i = 0
    for slot in xrange(next_slot):
        if entries[i].slot != slot:
            raise Exception('entries are not ordered by slots')
        if entries[i].alias:
            raise Exception('first entry of slot %d aliases %s'
                    % (slot, entries[i].alias))
        while i < len(entries) and entries[i].slot == slot:
            i += 1
    if i < len(entries):
        raise Exception('there are %d invalid entries' % (len(entries) - 1))

    return entries

def abi_dynamics():
    """Return the dynamic entries."""
    entries = []
    for i in xrange(ABI_NUM_DYNAMIC_ENTRIES):
        cols = ['void', 'dynamic%d' % (i), 'void']
        attrs = { 'slot': -1, 'hidden': False, 'alias': None }
        entries.append(ABIEntry(cols, attrs))
    return entries

class ABIPrinter(object):
    """ABIEntry Printer"""

    def __init__(self, entries, options):
        self.entries = entries
        self.options = options
        self._undefs = []

    def _add_undefs(self, undefs):
        self._undefs.extend(undefs)

    def output_header(self):
        print '/* This file is automatically generated.  Do not modify. */'
        print

    def output_footer(self):
        print '/* clean up */'
        for m in self._undefs:
            print '#undef %s' % (m)

    def output_entry(self, ent):
        if ent.slot < 0:
            out_ent = 'MAPI_DYNAMIC_ENTRY(%s, %s, (%s))' % \
                    (ent.c_return(), ent.name, ent.c_params())
            out_code = ''
        else:
            if ent.alias:
                macro_ent = 'MAPI_ALIAS_ENTRY'
                macro_code = 'MAPI_ALIAS_CODE'
            else:
                macro_ent = 'MAPI_ABI_ENTRY'
                macro_code = 'MAPI_ABI_CODE'

            if ent.ret:
                macro_code += '_RETURN'
            if ent.hidden:
                macro_ent += '_HIDDEN'
                macro_code += '_HIDDEN'

            if ent.alias:
                out_ent = '%s(%s, %s, %s, (%s))' % (macro_ent,
                        ent.alias, ent.c_return(), ent.name, ent.c_params())
                out_code = '%s(%s, %s, %s, (%s))' % (macro_code,
                        ent.alias, ent.c_return(), ent.name, ent.c_args())
            else:
                out_ent = '%s(%s, %s, (%s))' % (macro_ent,
                        ent.c_return(), ent.name, ent.c_params())
                out_code = '%s(%s, %s, (%s))' % (macro_code,
                        ent.c_return(), ent.name, ent.c_args())

        print out_ent
        if out_code:
            print '   ' + out_code

    def output_entries(self, pool_offsets):
        defs = [
                # normal entries
                ('MAPI_ABI_ENTRY', '(ret, name, params)', ''),
                ('MAPI_ABI_CODE', '(ret, name, args)', ''),
                ('MAPI_ABI_CODE_RETURN', '', 'MAPI_ABI_CODE'),
                # alias entries
                ('MAPI_ALIAS_ENTRY', '(alias, ret, name, params)', ''),
                ('MAPI_ALIAS_CODE', '(alias, ret, name, args)', ''),
                ('MAPI_ALIAS_CODE_RETURN', '', 'MAPI_ALIAS_CODE'),
                # hidden normal entries
                ('MAPI_ABI_ENTRY_HIDDEN', '', 'MAPI_ABI_ENTRY'),
                ('MAPI_ABI_CODE_HIDDEN', '', 'MAPI_ABI_CODE'),
                ('MAPI_ABI_CODE_RETURN_HIDDEN', '', 'MAPI_ABI_CODE_RETURN'),
                # hidden alias entries
                ('MAPI_ALIAS_ENTRY_HIDDEN', '', 'MAPI_ALIAS_ENTRY'),
                ('MAPI_ALIAS_CODE_HIDDEN', '', 'MAPI_ALIAS_CODE'),
                ('MAPI_ALIAS_CODE_RETURN_HIDDEN', '', 'MAPI_ALIAS_CODE_RETURN'),
                # dynamic entries
                ('MAPI_DYNAMIC_ENTRY', '(ret, name, params)', ''),
        ]
        undefs = [d[0] for d in defs]

        print '#if defined(MAPI_ABI_ENTRY) || defined(MAPI_ABI_ENTRY_HIDDEN)'
        print
        for d in defs:
            print '#ifndef %s' % (d[0])
            if d[2]:
                print '#define %s%s %s' % d
            else:
                print '#define %s%s' % d[:2]

            print '#endif'
        print

        print '/* see MAPI_TMP_TABLE */'
        for ent in self.entries:
            print '#define MAPI_SLOT_%s %d' % (ent.name, ent.slot)
        print
        print '/* see MAPI_TMP_PUBLIC_STUBS */'
        for ent in self.entries:
            print '#define MAPI_POOL_%s %d' % (ent.name, pool_offsets[ent])
        print

        # define macros that generate code
        for ent in self.entries:
            self.output_entry(ent)
        print
        dynamics = abi_dynamics()
        for ent in dynamics:
            self.output_entry(ent)
        print

        for ent in self.entries:
            print '#undef MAPI_SLOT_%s' % (ent.name)
        for ent in self.entries:
            print '#undef MAPI_POOL_%s' % (ent.name)
        print
        print '#endif /* defined(MAPI_ABI_ENTRY) || defined(MAPI_ABI_ENTRY_HIDDEN) */'
        print

        self._add_undefs(undefs)

    def _get_string_pool(self):
        """Get the string pool."""
        pool = []
        offsets = {}

        count = 0
        for ent in self.entries:
            offsets[ent] = count
            pool.append(ent.name + '\\0')
            count += len(ent.name) + 1

        return (pool, offsets)

    def output_sorted_indices(self):
        entry_index_pairs = []
        for i in xrange(len(self.entries)):
            entry_index_pairs.append((self.entries[i], i))
        entry_index_pairs.sort(lambda x, y: cmp(x[0].name, y[0].name))

        print '/* see MAPI_TMP_PUBLIC_STUBS */'
        print '#ifdef MAPI_ABI_SORTED_INDICES'
        print
        print 'static const int MAPI_ABI_SORTED_INDICES[] = {'
        for ent, idx in entry_index_pairs:
            print '   %d, /* %s */' % (idx, ent.name)
        print '   -1'
        print '};'
        print
        print '#endif /* MAPI_ABI_SORTED_INDICES */'
        print

        self._add_undefs(['MAPI_ABI_SORTED_INDICES'])

    def output_defines(self):
        print '/* ABI defines */'
        print '#ifdef MAPI_ABI_DEFINES'
        print '#include "%s"' % (self.options.include)
        print '#endif /* MAPI_ABI_DEFINES */'
        print

        self._add_undefs(['MAPI_ABI_DEFINES'])

    def output(self):
        pool, pool_offsets = self._get_string_pool()

        self.output_header()
        self.output_defines()
        self.output_entries(pool_offsets)
        self.output_sorted_indices()
        self.output_footer()

def parse_args():
    parser = OptionParser(usage='usage: %prog [options] <filename>')
    parser.add_option('-i', '--include', dest='include',
            help='include the header for API defines')

    options, args = parser.parse_args()
    if not args or not options.include:
        parser.print_help()
        sys.exit(1)

    return (args[0], options)

def main():
    filename, options = parse_args()

    entries = abi_parse(filename)
    printer = ABIPrinter(entries, options)
    printer.output()

if __name__ == '__main__':
    main()
