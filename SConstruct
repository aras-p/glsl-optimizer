#######################################################################
# Top-level SConstruct

import os
import os.path
import sys


#######################################################################
# Configuration options
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

platform_map = {
	'linux2': 'linux',
	'win32': 'winddk',
}

default_platform = platform_map.get(sys.platform, sys.platform)

if default_platform in ('linux', 'freebsd', 'darwin'):
	default_statetrackers = 'mesa'
	default_drivers = 'softpipe,failover,i915simple,i965simple'
	default_winsys = 'xlib'
	default_dri = 'yes'
elif default_platform in ('winddk',):
	default_statetrackers = 'none'
	default_drivers = 'softpipe,i915simple'
	default_winsys = 'none'
	default_dri = 'no'
else:
	default_drivers = 'all'
	default_winsys = 'all'
	default_dri = 'no'


# TODO: auto-detect defaults
opts = Options('config.py')
opts.Add(BoolOption('debug', 'build debug version', False))
opts.Add(EnumOption('machine', 'use machine-specific assembly code', 'x86',
                     allowed_values=('generic', 'x86', 'x86-64')))
opts.Add(EnumOption('platform', 'target platform', default_platform,
                     allowed_values=('linux', 'cell', 'winddk')))
opts.Add(ListOption('statetrackers', 'state_trackers to build', default_statetrackers,
                     [
                     	'mesa', 
                     ],
                     ))
opts.Add(ListOption('drivers', 'pipe drivers to build', default_drivers,
                     [
                     	'softpipe', 
                     	'failover', 
                     	'i915simple', 
                     	'i965simple', 
                     	'cell',
                     ],
                     ))
opts.Add(ListOption('winsys', 'winsys drivers to build', default_winsys,
                     [
                     	'xlib', 
                     	'intel',
                     ],
                     ))
opts.Add(BoolOption('llvm', 'use LLVM', 'no'))
opts.Add(BoolOption('dri', 'build DRI drivers', default_dri))

env = Environment(
	options = opts, 
	ENV = os.environ)
Help(opts.GenerateHelpText(env))

# for debugging
#print env.Dump()

# replicate options values in local variables
debug = env['debug']
dri = env['dri']
llvm = env['llvm']
machine = env['machine']
platform = env['platform']

# derived options
x86 = machine == 'x86'
gcc = platform in ('linux', 'freebsd', 'darwin')
msvc = platform in ('win32', 'winddk')

Export([
	'debug', 
	'x86', 
	'dri', 
	'llvm',
	'platform',
	'gcc',
	'msvc',
])


#######################################################################
# Environment setup
#
# TODO: put the compiler specific settings in separate files
# TODO: auto-detect as much as possible


if platform == 'winddk':
	import ntpath
	escape = env['ESCAPE']
	env.Tool('winddk', '.')
	if 'BASEDIR' in os.environ:
		WINDDK = os.environ['BASEDIR']
	else:
		WINDDK = "C:\\WINDDK\\3790.1830"
	# NOTE: We need this elaborate construct to get the absolute paths and
	# forward slashes to msvc unharmed when cross compiling from posix platforms 
	#env.Append(CPPFLAGS = [
	#	escape('/I' + ntpath.join(WINDDK, 'inc\\wxp')),
	#	escape('/I' + ntpath.join(WINDDK, 'inc\\ddk\\wxp')),
	#	escape('/I' + ntpath.join(WINDDK, 'inc\\ddk\\wdm\\wxp')),
	#	escape('/I' + ntpath.join(WINDDK, 'inc\\crt')),
	#])
	
	env.Append(CFLAGS = '/W3')
	if debug:
		env.Append(CPPDEFINES = [
			('DBG', '1'),
			('DEBUG', '1'),
			('_DEBUG', '1'),
		])
		env.Append(CFLAGS = '/Od /Zi')
		env.Append(CXXFLAGS = '/Od /Zi')
			

# Optimization flags
if gcc:
	if debug:
		env.Append(CFLAGS = '-O0 -g3')
		env.Append(CXXFLAGS = '-O0 -g3')
	else:
		env.Append(CFLAGS = '-O3 -g3')
		env.Append(CXXFLAGS = '-O3 -g3')

	env.Append(CFLAGS = '-Wall -Wmissing-prototypes -Wno-long-long -ffast-math -pedantic')
	env.Append(CXXFLAGS = '-Wall -pedantic')
	
	# Be nice to Eclipse
	env.Append(CFLAGS = '-fmessage-length=0')
	env.Append(CXXFLAGS = '-fmessage-length=0')


# Defines
if debug:
	env.Append(CPPDEFINES = ['DEBUG'])
else:
	env.Append(CPPDEFINES = ['NDEBUG'])


# Includes
env.Append(CPPPATH = [
	'#/include',
	'#/src/gallium/include',
	'#/src/gallium/auxiliary',
	'#/src/gallium/drivers',
])


# x86 assembly
if x86:
	env.Append(CPPDEFINES = [
		'USE_X86_ASM', 
		'USE_MMX_ASM',
		'USE_3DNOW_ASM',
		'USE_SSE_ASM',
	])
	if gcc:	
		env.Append(CFLAGS = '-m32')
		env.Append(CXXFLAGS = '-m32')


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
	env.ParseConfig('llvm-config --cflags --ldflags --libs')
	env.Append(CPPDEFINES = ['MESA_LLVM'])
	env.Append(CXXFLAGS = ['-Wno-long-long'])
	

# libGL
if platform not in ('winddk',):
	env.Append(LIBS = [
		'X11',
		'Xext',
		'Xxf86vm',
		'Xdamage',
		'Xfixes',
	])

Export('env')


#######################################################################
# Convenience Library Builder
# based on the stock StaticLibrary and SharedLibrary builders

def createConvenienceLibBuilder(env):
    """This is a utility function that creates the ConvenienceLibrary
    Builder in an Environment if it is not there already.

    If it is already there, we return the existing one.
    """

    try:
        convenience_lib = env['BUILDERS']['ConvenienceLibrary']
    except KeyError:
        action_list = [ Action("$ARCOM", "$ARCOMSTR") ]
        if env.Detect('ranlib'):
            ranlib_action = Action("$RANLIBCOM", "$RANLIBCOMSTR")
            action_list.append(ranlib_action)

        convenience_lib = Builder(action = action_list,
                                  emitter = '$LIBEMITTER',
                                  prefix = '$LIBPREFIX',
                                  suffix = '$LIBSUFFIX',
                                  src_suffix = '$SHOBJSUFFIX',
                                  src_builder = 'SharedObject')
        env['BUILDERS']['ConvenienceLibrary'] = convenience_lib
        env['BUILDERS']['Library'] = convenience_lib

    return convenience_lib

createConvenienceLibBuilder(env)


#######################################################################
# Invoke SConscripts

# Put build output in a separate dir, which depends on the current configuration
# See also http://www.scons.org/wiki/AdvancedBuildExample
build_topdir = 'build'
build_subdir = platform
if dri:
	build_subdir += "-dri"
if llvm:
	build_subdir += "-llvm"
if x86:
	build_subdir += "-x86"
if debug:
	build_subdir += "-debug"
build_dir = os.path.join(build_topdir, build_subdir)

# TODO: Build several variants at the same time?
# http://www.scons.org/wiki/SimultaneousVariantBuilds

SConscript(
	'src/SConscript',
	build_dir = build_dir,
	duplicate = 0 # http://www.scons.org/doc/0.97/HTML/scons-user/x2261.html
)
