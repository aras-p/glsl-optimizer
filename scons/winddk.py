"""winddk

Tool-specific initialization for Microsoft Windows DDK.

"""

#
# Copyright (c) 2001-2007 The SCons Foundation
# Copyright (c) 2008 Tungsten Graphics, Inc.
#
# Permission is hereby granted, free of charge, to any person obtaining
# a copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sublicense, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice shall be included
# in all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY
# KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE
# WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
# NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
# LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
# OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
# WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#

import os.path
import re
import string

import SCons.Action
import SCons.Builder
import SCons.Errors
import SCons.Platform.win32
import SCons.Tool
import SCons.Util
import SCons.Warnings

import msvc_sa
import mslib_sa
import mslink_sa

versions = [
    '6001.18002',
    '3790.1830',
]

def cpu_bin(target_cpu):
    if target_cpu == 'i386':
        return 'x86'
    else:
        return target_cpu

def get_winddk_root(env, version):
    default_path = os.path.join(r'C:\WINDDK', version)
    if os.path.exists(default_path):
        return default_path
    return None 

def get_winddk_paths(env, version, root):
    version_major, version_minor = map(int, version.split('.'))
    
    if version_major >= 6000:
        target_os = 'wlh'
    else:
        target_os = 'wxp'

    if env['machine'] in ('generic', 'x86'):
        target_cpu = 'i386'
    elif env['machine'] == 'x86_64':
        target_cpu = 'amd64'
    else:
        raise SCons.Errors.InternalError, "Unsupported target machine"

    if version_major >= 6000:
        # TODO: take in consideration the host cpu
        bin_dir = os.path.join(root, 'bin', 'x86', cpu_bin(target_cpu))
    else:
        if target_cpu == 'i386':
            bin_dir = os.path.join(root, 'bin', 'x86')
        else:
            # TODO: take in consideration the host cpu
            bin_dir = os.path.join(root, 'bin', 'win64', 'x86', cpu_bin(target_cpu))

    env.PrependENVPath('PATH', [bin_dir])
    
    crt_inc_dir = os.path.join(root, 'inc', 'crt')
    if version_major >= 6000:
        sdk_inc_dir = os.path.join(root, 'inc', 'api')
        ddk_inc_dir = os.path.join(root, 'inc', 'ddk')
        wdm_inc_dir = os.path.join(root, 'inc', 'ddk')
    else:
        ddk_inc_dir = os.path.join(root, 'inc', 'ddk', target_os)
        sdk_inc_dir = os.path.join(root, 'inc', target_os)
        wdm_inc_dir = os.path.join(root, 'inc', 'ddk', 'wdm', target_os)

    env.PrependENVPath('INCLUDE', [
        wdm_inc_dir,
        ddk_inc_dir,
        crt_inc_dir,
        sdk_inc_dir,
    ])

    env.PrependENVPath('LIB', [
        os.path.join(root, 'lib', 'crt', target_cpu),
        os.path.join(root, 'lib', target_os, target_cpu),
    ])

def generate(env):
    if not env.has_key('ENV'):
        env['ENV'] = {}

    for version in versions:
        root = get_winddk_root(env, version)
        if root is not None:
            get_winddk_paths(env, version, root)
            break

    msvc_sa.generate(env)
    mslib_sa.generate(env)
    mslink_sa.generate(env)

def exists(env):
    for version in versions:
        if get_winddk_root(env, version) is not None:
            return True
    return False

# vim:set ts=4 sw=4 et:
