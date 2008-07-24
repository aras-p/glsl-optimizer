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

def get_winddk_root(env):
    try:
        return os.environ['BASEDIR']
    except KeyError:
        pass

    version = "3790.1830"
    
    if SCons.Util.can_read_reg:
        key = r'SOFTWARE\Microsoft\WINDDK\%s\LFNDirectory' % version
        try:
            path, t = SCons.Util.RegGetValue(SCons.Util.HKEY_LOCAL_MACHINE, key)
        except SCons.Util.RegError:
            pass
        else:
            return path

    default_path = os.path.join(r'C:\WINDDK', version)
    if os.path.exists(default_path):
        return default_path
    
    return None 

def get_winddk_paths(env):
    """Return a 3-tuple of (INCLUDE, LIB, PATH) as the values
    of those three environment variables that should be set
    in order to execute the MSVC tools properly."""
    
    WINDDKdir = None
    exe_paths = []
    lib_paths = []
    include_paths = []

    WINDDKdir = get_winddk_root(env)
    if WINDDKdir is None:
        raise SCons.Errors.InternalError, "WINDDK not found"

    exe_paths.append( os.path.join(WINDDKdir, 'bin') )
    exe_paths.append( os.path.join(WINDDKdir, 'bin', 'x86') )
    include_paths.append( os.path.join(WINDDKdir, 'inc', 'wxp') )
    lib_paths.append( os.path.join(WINDDKdir, 'lib') )

    target_os = 'wxp'
    target_cpu = 'i386'
    
    env['SDK_INC_PATH'] = os.path.join(WINDDKdir, 'inc', target_os) 
    env['CRT_INC_PATH'] = os.path.join(WINDDKdir, 'inc', 'crt') 
    env['DDK_INC_PATH'] = os.path.join(WINDDKdir, 'inc', 'ddk', target_os) 
    env['WDM_INC_PATH'] = os.path.join(WINDDKdir, 'inc', 'ddk', 'wdm', target_os) 

    env['SDK_LIB_PATH'] = os.path.join(WINDDKdir, 'lib', target_os, target_cpu) 
    env['CRT_LIB_PATH'] = os.path.join(WINDDKdir, 'lib', 'crt', target_cpu) 
    env['DDK_LIB_PATH'] = os.path.join(WINDDKdir, 'lib', target_os, target_cpu)
    env['WDM_LIB_PATH'] = os.path.join(WINDDKdir, 'lib', target_os, target_cpu)
                                     
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
        include_path, lib_path, exe_path = get_winddk_paths(env)

        # since other tools can set these, we just make sure that the
        # relevant stuff from WINDDK is in there somewhere.
        env.PrependENVPath('INCLUDE', include_path)
        env.PrependENVPath('LIB', lib_path)
        env.PrependENVPath('PATH', exe_path)
    except (SCons.Util.RegError, SCons.Errors.InternalError):
        pass

def exists(env):
    return get_winddk_root(env) is not None

# vim:set ts=4 sw=4 et:
