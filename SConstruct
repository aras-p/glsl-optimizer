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

# XXX: This creates a many problems as it saves...
#opts.Save('config.py', env)

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

with open("VERSION") as f:
  mesa_version = f.read().strip()
env.Append(CPPDEFINES = [
    ('PACKAGE_VERSION', '\\"%s\\"' % mesa_version),
    ('PACKAGE_BUGREPORT', '\\"https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa\\"'),
])

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

# for debugging
#print env.Dump()


#######################################################################
# Invoke host SConscripts 
# 
# For things that are meant to be run on the native host build machine, instead
# of the target machine.
#

# Create host environent
if env['crosscompile'] and not env['embedded']:
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

    target_env = env
    env = host_env
    Export('env')

    SConscript(
        'src/SConscript',
        variant_dir = host_env['build_dir'],
        duplicate = 0, # http://www.scons.org/doc/0.97/HTML/scons-user/x2261.html
    )

    env = target_env

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


########################################################################
# List all aliases

try:
    from SCons.Node.Alias import default_ans
except ImportError:
    pass
else:
    aliases = default_ans.keys()
    aliases.sort()
    env.Help('\n')
    env.Help('Recognized targets:\n')
    for alias in aliases:
        env.Help('    %s\n' % alias)
