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

# fail early for a common error on windows
if env['gles']:
    try:
        import libxml2
    except ImportError:
        raise SCons.Errors.UserError, "GLES requires libxml2-python to build"

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


#######################################################################
# Invoke host SConscripts 
# 
# For things that are meant to be run on the native host build machine, instead
# of the target machine.
#

# Create host environent
if env['crosscompile'] and env['platform'] != 'embedded':
    host_env = Environment(
        options = opts,
        # no tool used
        tools = [],
        toolpath = ['#scons'],
        ENV = os.environ,
    )

    # Override options
    host_env['platform'] = common.host_platform
    host_env['machine'] = common.host_machine
    host_env['toolchain'] = 'default'
    host_env['llvm'] = False

    host_env.Tool('gallium')

    host_env['hostonly'] = True
    assert host_env['crosscompile'] == False

    Export(env = host_env)

    SConscript(
        'src/SConscript',
        variant_dir = host_env['build_dir'],
        duplicate = 0, # http://www.scons.org/doc/0.97/HTML/scons-user/x2261.html
    )

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

