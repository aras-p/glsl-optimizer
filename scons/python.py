"""gallium

Frontend-tool for Gallium3D architecture.

"""

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


import sys
import distutils.sysconfig
import os.path


def generate(env):
    # See http://www.scons.org/wiki/PythonExtensions
    
    if sys.platform in ['windows']:
        python_root = sys.prefix
        python_version = '%u%u' % sys.version_info[:2]
        python_include = os.path.join(python_root, 'include')
        python_libs = os.path.join(python_root, 'libs')
        python_lib = os.path.join(python_libs, 'python' + python_version + '.lib')
        
        env.Append(CPPPATH = [python_include])
        env.Append(LIBPATH = [python_libs])
        env.Append(LIBS = ['python' + python_version + '.lib'])
        env.Replace(SHLIBPREFIX = '')
        env.Replace(SHLIBSUFFIX = '.pyd')
        
        # XXX; python25_d.lib is not included in Python for windows, and 
        # we'll get missing symbols unless we undefine _DEBUG
        cppdefines = env['CPPDEFINES']
        cppdefines = [define for define in cppdefines if define != '_DEBUG']
        env.Replace(CPPDEFINES = cppdefines)
    else:
        #env.ParseConfig('python-config --cflags --ldflags --libs')
        env.AppendUnique(CPPPATH = [distutils.sysconfig.get_python_inc()])
        env.Replace(SHLIBPREFIX = '')
        env.Replace(SHLIBSUFFIX = distutils.sysconfig.get_config_vars()['SO'])

    # for debugging
    #print env.Dump()


def exists(env):
    return 1
