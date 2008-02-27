#######################################################################
# Common SCons code

import os
import os.path
import sys
import platform as _platform


#######################################################################
# Defaults

_platform_map = {
	'linux2': 'linux',
	'win32': 'winddk',
}

default_platform = sys.platform
default_platform = _platform_map.get(default_platform, default_platform)

_machine_map = {
	'x86': 'x86',
	'i386': 'x86',
	'i486': 'x86',
	'i586': 'x86',
	'i686': 'x86',
	'x86_64': 'x86_64',
}
if 'PROCESSOR_ARCHITECTURE' in os.environ:
	default_machine = os.environ['PROCESSOR_ARCHITECTURE']
else:
	default_machine = _platform.machine()
default_machine = _machine_map.get(default_machine, 'generic')

if default_platform in ('linux', 'freebsd', 'darwin'):
	default_dri = 'yes'
elif default_platform in ('winddk',):
	default_dri = 'no'
else:
	default_dri = 'no'


#######################################################################
# Common options

def Options():
	from SCons.Options import Options
	from SCons.Options.BoolOption import BoolOption
	from SCons.Options.EnumOption import EnumOption
	opts = Options('config.py')
	opts.Add(BoolOption('debug', 'build debug version', 'no'))
	opts.Add(EnumOption('machine', 'use machine-specific assembly code', default_machine,
											 allowed_values=('generic', 'x86', 'x86_64')))
	opts.Add(EnumOption('platform', 'target platform', default_platform,
											 allowed_values=('linux', 'cell', 'winddk')))
	opts.Add(BoolOption('llvm', 'use LLVM', 'no'))
	opts.Add(BoolOption('dri', 'build DRI drivers', default_dri))
	return opts


#######################################################################
# Convenience Library Builder
# based on the stock StaticLibrary and SharedLibrary builders

import SCons.Action
import SCons.Builder

def createConvenienceLibBuilder(env):
    """This is a utility function that creates the ConvenienceLibrary
    Builder in an Environment if it is not there already.

    If it is already there, we return the existing one.
    """

    try:
        convenience_lib = env['BUILDERS']['ConvenienceLibrary']
    except KeyError:
        action_list = [ SCons.Action.Action("$ARCOM", "$ARCOMSTR") ]
        if env.Detect('ranlib'):
            ranlib_action = SCons.Action.Action("$RANLIBCOM", "$RANLIBCOMSTR")
            action_list.append(ranlib_action)

        convenience_lib = SCons.Builder.Builder(action = action_list,
                                  emitter = '$LIBEMITTER',
                                  prefix = '$LIBPREFIX',
                                  suffix = '$LIBSUFFIX',
                                  src_suffix = '$SHOBJSUFFIX',
                                  src_builder = 'SharedObject')
        env['BUILDERS']['ConvenienceLibrary'] = convenience_lib
        env['BUILDERS']['Library'] = convenience_lib

    return convenience_lib


#######################################################################
# Build

def make_build_dir(env):
	# Put build output in a separate dir, which depends on the current configuration
	# See also http://www.scons.org/wiki/AdvancedBuildExample
	build_topdir = 'build'
	build_subdir = env['platform']
	if env['dri']:
		build_subdir += "-dri"
	if env['llvm']:
		build_subdir += "-llvm"
	if env['machine'] != 'generic':
		build_subdir += '-' + env['machine']
	if env['debug']:
		build_subdir += "-debug"
	build_dir = os.path.join(build_topdir, build_subdir)
	# Place the .sconsign file on the builddir too, to avoid issues with different scons
	# versions building the same source file
	env.SConsignFile(os.path.join(build_dir, '.sconsign'))
	return build_dir

