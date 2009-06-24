#######################################################################
# Top-level SConstruct
#
# For example, invoke scons as 
#
#   scons debug=1 dri=0 machine=x86
#
# to set configuration variables. Or you can write those options to a file
# named config.py:
#
#   # config.py
#   debug=1
#   dri=0
#   machine='x86'
# 
# Invoke
#
#   scons -h
#
# to get the full list of options. See scons manpage for more info.
#  

import os
import os.path
import sys

import common

#######################################################################
# Configuration options

default_statetrackers = 'mesa'

if common.default_platform in ('linux', 'freebsd', 'darwin'):
	default_drivers = 'softpipe,failover,i915simple,trace,identity'
	default_winsys = 'xlib'
elif common.default_platform in ('winddk',):
	default_drivers = 'softpipe,i915simple,trace,identity'
	default_winsys = 'all'
else:
	default_drivers = 'all'
	default_winsys = 'all'

opts = Variables('config.py')
common.AddOptions(opts)
opts.Add(ListVariable('statetrackers', 'state trackers to build', default_statetrackers,
                     ['mesa', 'python']))
opts.Add(ListVariable('drivers', 'pipe drivers to build', default_drivers,
                     ['softpipe', 'failover', 'i915simple', 'i965simple', 'cell', 'trace', 'r300', 'identity']))
opts.Add(ListVariable('winsys', 'winsys drivers to build', default_winsys,
                     ['xlib', 'intel', 'gdi', 'radeon']))

opts.Add(EnumVariable('MSVS_VERSION', 'MS Visual C++ version', None, allowed_values=('7.1', '8.0', '9.0')))

env = Environment(
	options = opts,
	tools = ['gallium'],
	toolpath = ['#scons'],	
	ENV = os.environ,
)

Help(opts.GenerateHelpText(env))

# replicate options values in local variables
debug = env['debug']
dri = env['dri']
llvm = env['llvm']
machine = env['machine']
platform = env['platform']

# derived options
x86 = machine == 'x86'
ppc = machine == 'ppc'
gcc = platform in ('linux', 'freebsd', 'darwin')
msvc = platform in ('windows', 'winddk')

Export([
	'debug', 
	'x86', 
	'ppc', 
	'dri', 
	'llvm',
	'platform',
	'gcc',
	'msvc',
])


#######################################################################
# Environment setup

# Includes
env.Append(CPPPATH = [
	'#/include',
	'#/src/gallium/include',
	'#/src/gallium/auxiliary',
	'#/src/gallium/drivers',
])


# Posix
if platform in ('posix', 'linux', 'freebsd', 'darwin'):
	env.Append(CPPDEFINES = [
		'_POSIX_SOURCE',
		('_POSIX_C_SOURCE', '199309L'), 
		'_SVID_SOURCE',
		'_BSD_SOURCE', 
		'_GNU_SOURCE',
		
		'PTHREADS',
		'HAVE_POSIX_MEMALIGN',
	])
	env.Append(CPPPATH = ['/usr/X11R6/include'])
	env.Append(LIBPATH = ['/usr/X11R6/lib'])
	env.Append(LIBS = [
		'm',
		'pthread',
		'expat',
		'dl',
	])


# DRI
if dri:
	env.ParseConfig('pkg-config --cflags --libs libdrm')
	env.Append(CPPDEFINES = [
		('USE_EXTERNAL_DXTN_LIB', '1'), 
		'IN_DRI_DRIVER',
		'GLX_DIRECT_RENDERING',
		'GLX_INDIRECT_RENDERING',
	])

# LLVM
if llvm:
	# See also http://www.scons.org/wiki/UsingPkgConfig
	env.ParseConfig('llvm-config --cflags --ldflags --libs backend bitreader engine instrumentation interpreter ipo')
	env.Append(CPPDEFINES = ['MESA_LLVM'])
        # Force C++ linkage
	env['LINK'] = env['CXX']

# libGL
if platform in ('linux', 'freebsd', 'darwin'):
	env.Append(LIBS = [
		'X11',
		'Xext',
		'Xxf86vm',
		'Xdamage',
		'Xfixes',
	])

# for debugging
#print env.Dump()

Export('env')


#######################################################################
# Invoke SConscripts

# TODO: Build several variants at the same time?
# http://www.scons.org/wiki/SimultaneousVariantBuilds

SConscript(
	'src/SConscript',
	variant_dir = env['build'],
	duplicate = 0 # http://www.scons.org/doc/0.97/HTML/scons-user/x2261.html
)
