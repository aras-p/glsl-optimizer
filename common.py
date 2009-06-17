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
	'ppc' : 'ppc',
	'x86_64': 'x86_64',
}
if 'PROCESSOR_ARCHITECTURE' in os.environ:
	default_machine = os.environ['PROCESSOR_ARCHITECTURE']
else:
	default_machine = _platform.machine()
default_machine = _machine_map.get(default_machine, 'generic')

if default_platform in ('linux', 'freebsd', 'darwin'):
	default_dri = 'yes'
elif default_platform in ('winddk', 'windows', 'wince'):
	default_dri = 'no'
else:
	default_dri = 'no'


#######################################################################
# Common options

def AddOptions(opts):
	try:
		from SCons.Variables.BoolVariable import BoolVariable as BoolOption
	except ImportError:
		from SCons.Options.BoolOption import BoolOption
	try:
		from SCons.Variables.EnumVariable import EnumVariable as EnumOption
	except ImportError:
		from SCons.Options.EnumOption import EnumOption
	opts.Add(BoolOption('debug', 'debug build', 'no'))
	opts.Add(BoolOption('profile', 'profile build', 'no'))
	opts.Add(BoolOption('quiet', 'quiet command lines', 'yes'))
	opts.Add(EnumOption('machine', 'use machine-specific assembly code', default_machine,
											 allowed_values=('generic', 'ppc', 'x86', 'x86_64')))
	opts.Add(EnumOption('platform', 'target platform', default_platform,
											 allowed_values=('linux', 'cell', 'windows', 'winddk', 'wince')))
	opts.Add(EnumOption('toolchain', 'compiler toolchain', 'default',
											 allowed_values=('default', 'crossmingw', 'winsdk', 'winddk')))
	opts.Add(BoolOption('llvm', 'use LLVM', 'no'))
	opts.Add(BoolOption('dri', 'build DRI drivers', default_dri))

