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
elif default_platform in ('winddk', 'windows'):
	default_dri = 'no'
else:
	default_dri = 'no'


#######################################################################
# Common options

def AddOptions(opts):
	try:
		from SCons.Options.BoolOption import BoolOption
	except ImportError:
		from SCons.Variables.BoolVariable import BoolVariable as BoolOption
	try:
		from SCons.Options.EnumOption import EnumOption
	except ImportError:
		from SCons.Variables.EnumVariable import EnumVariable as EnumOption
	opts.Add(BoolOption('debug', 'build debug version', 'no'))
	#opts.Add(BoolOption('quiet', 'quiet command lines', 'no'))
	opts.Add(EnumOption('machine', 'use machine-specific assembly code', default_machine,
											 allowed_values=('generic', 'x86', 'x86_64')))
	opts.Add(EnumOption('platform', 'target platform', default_platform,
											 allowed_values=('linux', 'cell', 'windows', 'winddk')))
	opts.Add(BoolOption('llvm', 'use LLVM', 'no'))
	opts.Add(BoolOption('dri', 'build DRI drivers', default_dri))


#######################################################################
# Quiet command lines
#
# See also http://www.scons.org/wiki/HidingCommandLinesInOutput

def quietCommandLines(env):
	env['CCCOMSTR'] = "Compiling $SOURCE ..."
	env['CXXCOMSTR'] = "Compiling $SOURCE ..."
	env['ARCOMSTR'] = "Archiving $TARGET ..."
	env['RANLIBCOMSTR'] = ""
	env['LINKCOMSTR'] = "Linking $TARGET ..."


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


#######################################################################
# Common environment generation code

def generate(env):
	# FIXME: this is already too late
	#if env.get('quiet', False):
	#	quietCommandLines(env)
	
	# shortcuts
	debug = env['debug']
	machine = env['machine']
	platform = env['platform']
	x86 = env['machine'] == 'x86'
	gcc = env['platform'] in ('linux', 'freebsd', 'darwin')
	msvc = env['platform'] in ('windows', 'winddk')

	# C preprocessor options
	cppdefines = []
	if debug:
		cppdefines += ['DEBUG']
	else:
		cppdefines += ['NDEBUG']
	if platform == 'windows':
		cppdefines += [
			'WIN32', 
			'_WINDOWS', 
			'_UNICODE',
			'UNICODE',
			# http://msdn2.microsoft.com/en-us/library/6dwk3a1z.aspx,
			'WIN32_LEAN_AND_MEAN',
			'VC_EXTRALEAN', 
			'_CRT_SECURE_NO_DEPRECATE',
		]
		if debug:
			cppdefines += ['_DEBUG']
	if platform == 'winddk':
		# Mimic WINDDK's builtin flags. See also:
		# - WINDDK's bin/makefile.new i386mk.inc for more info.
		# - buildchk_wxp_x86.log files, generated by the WINDDK's build
		# - http://alter.org.ua/docs/nt_kernel/vc8_proj/
		cppdefines += [
			('_X86_', '1'), 
			('i386', '1'), 
			'STD_CALL', 
			('CONDITION_HANDLING', '1'),
			('NT_INST', '0'), 
			('WIN32', '100'),
			('_NT1X_', '100'),
			('WINNT', '1'),
			('_WIN32_WINNT', '0x0501'), # minimum required OS version
			('WINVER', '0x0501'),
			('_WIN32_IE', '0x0603'),
			('WIN32_LEAN_AND_MEAN', '1'),
			('DEVL', '1'),
			('__BUILDMACHINE__', 'WinDDK'),
			('FPO', '0'),
		]
		if debug:
			cppdefines += [('DBG', 1)]
	if platform == 'windows':
		cppdefines += ['PIPE_SUBSYSTEM_USER']
	if platform == 'winddk':
		cppdefines += ['PIPE_SUBSYSTEM_KERNEL']
	env.Append(CPPDEFINES = cppdefines)

	# C compiler options
	cflags = []
	if gcc:
		if debug:
			cflags += ['-O0', '-g3']
		else:
			cflags += ['-O3', '-g3']
		cflags += [
			'-Wall', 
			'-Wmissing-prototypes',
			'-Wno-long-long',
			'-ffast-math',
			'-pedantic',
			'-fmessage-length=0', # be nice to Eclipse 
		]
	if msvc:
		# See also:
		# - http://msdn2.microsoft.com/en-us/library/y0zzbyt4.aspx
		# - cl /?
		if debug:
			cflags += [
			  '/Od', # disable optimizations
			  '/Oi', # enable intrinsic functions
			  '/Oy-', # disable frame pointer omission
			]
		else:
			cflags += [
			  '/Ox', # maximum optimizations
			  '/Oi', # enable intrinsic functions
			  '/Os', # favor code space
			]
		if platform == 'windows':
			cflags += [
				# TODO
				#'/Wp64', # enable 64 bit porting warnings
			]
		if platform == 'winddk':
			cflags += [
				'/Zl', # omit default library name in .OBJ
				'/Zp8', # 8bytes struct member alignment
				'/Gy', # separate functions for linker
				'/Gm-', # disable minimal rebuild
				'/W3', # warning level
				'/WX', # treat warnings as errors
				'/Gz', # __stdcall Calling convention
				'/GX-', # disable C++ EH
				'/GR-', # disable C++ RTTI
				'/GF', # enable read-only string pooling
				'/G6', # optimize for PPro, P-II, P-III
				'/Ze', # enable extensions
				'/Gi-', # disable incremental compilation
				'/QIfdiv-', # disable Pentium FDIV fix
				'/hotpatch', # prepares an image for hotpatching.
				#'/Z7', #enable old-style debug info
			]
		# Put debugging information in a separate .pdb file for each object file as
		# descrived in the scons manpage
		env['CCPDBFLAGS'] = '/Zi /Fd${TARGET}.pdb'
	env.Append(CFLAGS = cflags)
	env.Append(CXXFLAGS = cflags)

	# Linker options
	if platform == 'winddk':
		# See also:
		# - http://msdn2.microsoft.com/en-us/library/y0zzbyt4.aspx
		env.Append(LINKFLAGS = [
			'/merge:_PAGE=PAGE',
			'/merge:_TEXT=.text',
			'/section:INIT,d',
			'/opt:ref',
			'/opt:icf',
			'/ignore:4198,4010,4037,4039,4065,4070,4078,4087,4089,4221',
			'/incremental:no',
			'/fullbuild',
			'/release',
			'/nodefaultlib',
			'/wx',
			'/debug',
			'/debugtype:cv',
			'/version:5.1',
			'/osversion:5.1',
			'/functionpadmin:5',
			'/safeseh',
			'/pdbcompress',
			'/stack:0x40000,0x1000',
			'/driver', 
			'/align:0x80',
			'/subsystem:native,5.01',
			'/base:0x10000',
			
			'/entry:DrvEnableDriver',
		])


	createConvenienceLibBuilder(env)
	
	
	# for debugging
	#print env.Dump()

