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
import SCons.Util

import common

#######################################################################
# Configuration options

default_statetrackers = 'mesa'
default_targets = 'graw-null'

if common.default_platform in ('linux', 'freebsd', 'darwin'):
	default_drivers = 'softpipe,galahad,failover,svga,i915,i965,trace,identity,llvmpipe'
	default_winsys = 'xlib'
elif common.default_platform in ('winddk',):
	default_drivers = 'softpipe,svga,i915,i965,trace,identity'
	default_winsys = 'all'
elif common.default_platform in ('embedded',):
	default_drivers = 'softpipe,llvmpipe'
	default_winsys = 'xlib'
else:
	default_drivers = 'all'
	default_winsys = 'all'

opts = Variables('config.py')
common.AddOptions(opts)
opts.Add(ListVariable('statetrackers', 'state trackers to build', default_statetrackers,
                     ['mesa', 'python', 'xorg', 'egl']))
opts.Add(ListVariable('drivers', 'pipe drivers to build', default_drivers,
                     ['softpipe', 'galahad', 'failover', 'svga', 'i915', 'i965', 'trace', 'r300', 'r600', 'identity', 'llvmpipe', 'nouveau', 'nv50', 'nvfx']))
opts.Add(ListVariable('winsys', 'winsys drivers to build', default_winsys,
                     ['xlib', 'vmware', 'i915', 'i965', 'gdi', 'radeon', 'r600', 'graw-xlib']))

opts.Add(ListVariable('targets', 'driver targets to build', default_targets,
		      ['dri-i915',
		       'dri-i965',
		       'dri-nouveau',
		       'dri-radeong',
		       'dri-swrast',
		       'dri-vmwgfx',
		       'egl-i915',
		       'egl-i965',
		       'egl-nouveau',
		       'egl-radeon',
		       'egl-swrast',
		       'egl-vmwgfx',
		       'graw-xlib',
		       'graw-null',
		       'libgl-gdi',
		       'libgl-xlib',
		       'xorg-i915',
		       'xorg-i965',
		       'xorg-nouveau',
		       'xorg-radeon',
		       'xorg-vmwgfx']))

opts.Add(EnumVariable('MSVS_VERSION', 'MS Visual C++ version', None, allowed_values=('7.1', '8.0', '9.0')))

env = Environment(
	options = opts,
	tools = ['gallium'],
	toolpath = ['#scons'],	
	ENV = os.environ,
)

if os.environ.has_key('CC'):
	env['CC'] = os.environ['CC']
if os.environ.has_key('CFLAGS'):
	env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
if os.environ.has_key('CXX'):
	env['CXX'] = os.environ['CXX']
if os.environ.has_key('CXXFLAGS'):
	env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
if os.environ.has_key('LDFLAGS'):
	env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

Help(opts.GenerateHelpText(env))

# replicate options values in local variables
debug = env['debug']
dri = env['dri']
machine = env['machine']
platform = env['platform']

# derived options
x86 = machine == 'x86'
ppc = machine == 'ppc'
gcc = platform in ('linux', 'freebsd', 'darwin', 'embedded')
msvc = platform in ('windows', 'winddk')

Export([
	'debug', 
	'x86', 
	'ppc', 
	'dri', 
	'platform',
	'gcc',
	'msvc',
])


#######################################################################
# Environment setup

# Always build trace, rbug, identity, softpipe, and llvmpipe (where possible)
if 'trace' not in env['drivers']:
    env['drivers'].append('trace')
if 'rbug' not in env['drivers']:
    env['drivers'].append('rbug')
if 'galahad' not in env['drivers']:
    env['drivers'].append('galahad')
if 'identity' not in env['drivers']:
    env['drivers'].append('identity')
if 'softpipe' not in env['drivers']:
    env['drivers'].append('softpipe')
if env['llvm'] and 'llvmpipe' not in env['drivers']:
    env['drivers'].append('llvmpipe')
if 'sw' not in env['drivers']:
    env['drivers'].append('sw')

# Includes
env.Prepend(CPPPATH = [
	'#/include',
])
env.Append(CPPPATH = [
	'#/src/gallium/include',
	'#/src/gallium/auxiliary',
	'#/src/gallium/drivers',
	'#/src/gallium/winsys',
])

if env['msvc']:
    env.Append(CPPPATH = ['#include/c99'])

# Embedded
if platform == 'embedded':
	env.Append(CPPDEFINES = [
		'_POSIX_SOURCE',
		('_POSIX_C_SOURCE', '199309L'), 
		'_SVID_SOURCE',
		'_BSD_SOURCE', 
		'_GNU_SOURCE',
		
		'PTHREADS',
	])
	env.Append(LIBS = [
		'm',
		'pthread',
		'dl',
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
	if gcc:
		env.Append(CFLAGS = ['-fvisibility=hidden'])
	if platform == 'darwin':
		env.Append(CPPDEFINES = ['_DARWIN_C_SOURCE'])
	env.Append(LIBS = [
		'm',
		'pthread',
		'dl',
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
	variant_dir = env['build_dir'],
	duplicate = 0 # http://www.scons.org/doc/0.97/HTML/scons-user/x2261.html
)

env.Default('src')

