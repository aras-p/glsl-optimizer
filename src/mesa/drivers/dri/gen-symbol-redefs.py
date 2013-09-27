#!/usr/bin/env python
# -*- coding: utf-8 -*-

# Copyright © 2013 Intel Corporation
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the "Software"),
# to deal in the Software without restriction, including without limitation
# the rights to use, copy, modify, merge, publish, distribute, sublicense,
# and/or sell copies of the Software, and to permit persons to whom the
# Software is furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice (including the next
# paragraph) shall be included in all copies or substantial portions of the
# Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
# THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
# FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
# IN THE SOFTWARE.

import sys
import argparse
import re
import subprocess

# Example usages:
# ./gen-symbol-redefs.py i915/.libs/libi915_dri.a old_ i915 i830
# ./gen-symbol-redefs.py r200/.libs/libr200_dri.a r200_ r200

argparser = argparse.ArgumentParser(description="Generates #defines to hide driver global symbols outside of a driver's namespace.")
argparser.add_argument("file",
                    metavar = 'file',
                    help='libdrivername.a file to read')
argparser.add_argument("newprefix",
                    metavar = 'newprefix',
                    help='New prefix to give non-driver global symbols')
argparser.add_argument('prefixes',
                       metavar='prefix',
                       nargs='*',
                       help='driver-specific prefixes')
args = argparser.parse_args()

stdout = subprocess.check_output(['nm', args.file])

for line in stdout.splitlines():
    m = re.match("[0-9a-z]+ [BT] (.*)", line)
    if not m:
        continue

    symbol = m.group(1)

    has_good_prefix = re.match(args.newprefix, symbol) != None
    for prefix in args.prefixes:
        if re.match(prefix, symbol):
            has_good_prefix = True
            break
    if has_good_prefix:
        continue

    # This is the single public entrypoint.
    if re.match("__driDriverGetExtensions", symbol):
        continue

    print '#define {0:35} {1}{0}'.format(symbol, args.newprefix)
