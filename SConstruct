#######################################################################
# Top-level SConstruct
#
# For example, invoke scons as 
#
#   scons build=debug llvm=yes machine=x86
#
# to set configuration variables. Or you can write those options to a file
# named config.py:
#
#   # config.py
#   build='debug'
#   llvm=True
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

opts = Variables('config.py')
common.AddOptions(opts)
opts.Add(EnumVariable('MSVS_VERSION', 'MS Visual C++ version', None, allowed_values=('7.1', '8.0', '9.0')))

env = Environment(
	options = opts,
	tools = ['gallium'],
	toolpath = ['#scons'],	
	ENV = os.environ,
)

# Backwards compatability with old target configuration variable
try:
    targets = ARGUMENTS['targets']
except KeyError:
    pass
else:
    targets = targets.split(',')
    print 'scons: warning: targets option is deprecated; pass the targets on their own such as'
    print
    print '  scons %s' % ' '.join(targets)
    print 
    COMMAND_LINE_TARGETS.append(targets)


Help(opts.GenerateHelpText(env))


#######################################################################
# Environment setup

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
if env['platform'] == 'embedded':
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
if env['platform'] in ('posix', 'linux', 'freebsd', 'darwin'):
	env.Append(CPPDEFINES = [
		'_POSIX_SOURCE',
		('_POSIX_C_SOURCE', '199309L'), 
		'_SVID_SOURCE',
		'_BSD_SOURCE', 
		'_GNU_SOURCE',
		'PTHREADS',
		'HAVE_POSIX_MEMALIGN',
	])
	if env['gcc']:
		env.Append(CFLAGS = ['-fvisibility=hidden'])
	if env['platform'] == 'darwin':
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

