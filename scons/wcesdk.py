"""wcesdk

Tool-specific initialization for Microsoft Window CE SDKs.

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

def get_wce500_paths(env):
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

def get_wce600_root(env):
    try:
        return os.environ['_WINCEROOT']
    except KeyError:
        pass

    if SCons.Util.can_read_reg:
        key = r'SOFTWARE\Microsoft\Platform Builder\6.00\Directories\OS Install Dir'
        try:
            path, t = SCons.Util.RegGetValue(SCons.Util.HKEY_LOCAL_MACHINE, key)
        except SCons.Util.RegError:
            pass
        else:
            return path

    default_path = os.path.join(r'C:\WINCE600', version)
    if os.path.exists(default_path):
        return default_path
    
    return None 

def get_wce600_paths(env):
    """Return a 3-tuple of (INCLUDE, LIB, PATH) as the values
    of those three environment variables that should be set
    in order to execute the MSVC tools properly."""
    
    exe_paths = []
    lib_paths = []
    include_paths = []

    # See also C:\WINCE600\public\common\oak\misc\wince.bat

    wince_root = get_wce600_root(env)
    if wince_root is None:
        raise SCons.Errors.InternalError, "Windows CE 6.0 SDK not found"
    
    os_version = os.environ.get('_WINCEOSVER', '600')
    platform_root = os.environ.get('_PLATFORMROOT', os.path.join(wince_root, 'platform'))
    sdk_root = os.environ.get('_SDKROOT' ,os.path.join(wince_root, 'sdk'))

    platform_root = os.environ.get('_PLATFORMROOT', os.path.join(wince_root, 'platform'))
    sdk_root = os.environ.get('_SDKROOT' ,os.path.join(wince_root, 'sdk'))

    host_cpu = os.environ.get('_HOSTCPUTYPE', 'i386')
    target_cpu = os.environ.get('_TGTCPU', 'x86')

    if env['debug']:
        build = 'debug'
    else:
        build = 'retail'

    try:
        project_root = os.environ['_PROJECTROOT']
    except KeyError:
        # No project root defined -- use the common stuff instead
        project_root = os.path.join(wince_root, 'public', 'common')

    exe_paths.append( os.path.join(sdk_root, 'bin', host_cpu) )
    exe_paths.append( os.path.join(sdk_root, 'bin', host_cpu, target_cpu) )
    exe_paths.append( os.path.join(wince_root, 'common', 'oak', 'bin', host_cpu) )
    exe_paths.append( os.path.join(wince_root, 'common', 'oak', 'misc') )

    include_paths.append( os.path.join(project_root, 'sdk', 'inc') )
    include_paths.append( os.path.join(project_root, 'oak', 'inc') )
    include_paths.append( os.path.join(project_root, 'ddk', 'inc') )
    include_paths.append( os.path.join(sdk_root, 'CE', 'inc') )
    
    lib_paths.append( os.path.join(project_root, 'sdk', 'lib', target_cpu, build) )
    lib_paths.append( os.path.join(project_root, 'oak', 'lib', target_cpu, build) )
    lib_paths.append( os.path.join(project_root, 'ddk', 'lib', target_cpu, build) )

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
        include_path, lib_path, exe_path = get_wce600_paths(env)

        env.PrependENVPath('INCLUDE', include_path)
        env.PrependENVPath('LIB', lib_path)
        env.PrependENVPath('PATH', exe_path)
    except (SCons.Util.RegError, SCons.Errors.InternalError):
        pass

def exists(env):
    return get_wce600_root(env) is not None

# vim:set ts=4 sw=4 et:
