"""evc

Tool-specific initialization for Microsoft eMbedded Visual C++.

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

def get_evc_paths(env, version=None):
    """Return a 3-tuple of (INCLUDE, LIB, PATH) as the values
    of those three environment variables that should be set
    in order to execute the MSVC tools properly."""
    
    exe_paths = []
    lib_paths = []
    include_paths = []

    # mymic the batch files located in Microsoft eMbedded C++ 4.0\EVC\WCExxx\BIN
    os_version = os.environ.get('OSVERSION', 'WCE500')
    platform = os.environ.get('PLATFORM', 'STANDARDSDK_500')
    wce_root = os.environ.get('WCEROOT', 'C:\\Program Files\\Microsoft eMbedded C++ 4.0')
    sdk_root = os.environ.get('SDKROOT', 'C:\\Windows CE Tools')

    target_cpu = 'x86'
    cfg = 'none'

    exe_paths.append( os.path.join(wce_root, 'COMMON', 'EVC', 'bin') )
    exe_paths.append( os.path.join(wce_root, 'EVC', os_version, 'bin') )
    include_paths.append( os.path.join(sdk_root, os_version, platform, 'include', target_cpu) )
    include_paths.append( os.path.join(sdk_root, os_version, platform, 'MFC', 'include') )
    include_paths.append( os.path.join(sdk_root, os_version, platform, 'ATL', 'include') )
    lib_paths.append( os.path.join(sdk_root, os_version, platform, 'lib', target_cpu) )
    lib_paths.append( os.path.join(sdk_root, os_version, platform, 'MFC', 'lib', target_cpu) )
    lib_paths.append( os.path.join(sdk_root, os_version, platform, 'ATL', 'lib', target_cpu) )
    
    include_path = string.join( include_paths, os.pathsep )
    lib_path = string.join(lib_paths, os.pathsep )
    exe_path = string.join(exe_paths, os.pathsep )
    return (include_path, lib_path, exe_path)

def generate(env):

    msvc_sa.generate(env)
    mslib_sa.generate(env)
    mslink_sa.generate(env)

    if not env.has_key('ENV'):
        env['ENV'] = {}
    
    try:
        include_path, lib_path, exe_path = get_evc_paths(env)

        # since other tools can set these, we just make sure that the
        # relevant stuff from WINDDK is in there somewhere.
        env.PrependENVPath('INCLUDE', include_path)
        env.PrependENVPath('LIB', lib_path)
        env.PrependENVPath('PATH', exe_path)
    except (SCons.Util.RegError, SCons.Errors.InternalError):
        pass

def exists(env):
    if not msvc_sa.exits(env):
        return 0
    if not mslib_sa.exits(env):
        return 0
    if not mslink_sa.exits(env):
        return 0
    return 1

# vim:set ts=4 sw=4 et:
