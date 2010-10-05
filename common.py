#######################################################################
# Common SCons code

import os
import os.path
import re
import subprocess
import sys
import platform as _platform


#######################################################################
# Defaults

_platform_map = {
	'linux2': 'linux',
	'win32': 'windows',
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


# find default_machine value
if 'PROCESSOR_ARCHITECTURE' in os.environ:
	default_machine = os.environ['PROCESSOR_ARCHITECTURE']
else:
	default_machine = _platform.machine()
default_machine = _machine_map.get(default_machine, 'generic')


# find default_llvm value
if 'LLVM' in os.environ:
    default_llvm = 'yes'
else:
    # Search sys.argv[] for a "platform=foo" argument since we don't have
    # an 'env' variable at this point.
    platform = default_platform
    pattern = re.compile("(platform=)(.*)")
    for arg in sys.argv:
        m = pattern.match(arg)
        if m:
            platform = m.group(2)

    default_llvm = 'no'
    try:
        if platform != 'windows' and subprocess.call(['llvm-config', '--version'], stdout=subprocess.PIPE) == 0:
            default_llvm = 'yes'
    except:
        pass


# find default_dri value
if default_platform in ('linux', 'freebsd'):
	default_dri = 'yes'
elif default_platform in ('winddk', 'windows', 'wince', 'darwin'):
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
	opts.Add(EnumOption('build', 'build type', 'debug',
	                  allowed_values=('debug', 'checked', 'profile', 'release')))
	opts.Add(BoolOption('quiet', 'quiet command lines', 'yes'))
	opts.Add(EnumOption('machine', 'use machine-specific assembly code', default_machine,
											 allowed_values=('generic', 'ppc', 'x86', 'x86_64')))
	opts.Add(EnumOption('platform', 'target platform', default_platform,
											 allowed_values=('linux', 'cell', 'windows', 'winddk', 'wince', 'darwin', 'embedded', 'cygwin', 'sunos5', 'freebsd8')))
	opts.Add('toolchain', 'compiler toolchain', 'default')
	opts.Add(BoolOption('llvm', 'use LLVM', default_llvm))
	opts.Add(BoolOption('dri', 'build DRI drivers', default_dri))
	opts.Add(BoolOption('debug', 'DEPRECATED: debug build', 'yes'))
	opts.Add(BoolOption('profile', 'DEPRECATED: profile build', 'no'))
