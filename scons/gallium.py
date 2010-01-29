"""gallium

Frontend-tool for Gallium3D architecture.

"""

#
# Copyright 2008 Tungsten Graphics, Inc., Cedar Park, Texas.
# All Rights Reserved.
#
# Permission is hereby granted, free of charge, to any person obtaining a
# copy of this software and associated documentation files (the
# "Software"), to deal in the Software without restriction, including
# without limitation the rights to use, copy, modify, merge, publish,
# distribute, sub license, and/or sell copies of the Software, and to
# permit persons to whom the Software is furnished to do so, subject to
# the following conditions:
#
# The above copyright notice and this permission notice (including the
# next paragraph) shall be included in all copies or substantial portions
# of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
# OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
# MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NON-INFRINGEMENT.
# IN NO EVENT SHALL TUNGSTEN GRAPHICS AND/OR ITS SUPPLIERS BE LIABLE FOR
# ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
# TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
# SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
#


import distutils.version
import os
import os.path
import re
import subprocess

import SCons.Action
import SCons.Builder
import SCons.Scanner


def symlink(target, source, env):
    target = str(target[0])
    source = str(source[0])
    if os.path.islink(target) or os.path.exists(target):
        os.remove(target)
    os.symlink(os.path.basename(source), target)

def install(env, source, subdir):
    target_dir = os.path.join(env.Dir('#.').srcnode().abspath, env['build'], subdir)
    env.Install(target_dir, source)

def install_program(env, source):
    install(env, source, 'bin')

def install_shared_library(env, sources, version = ()):
    install_dir = os.path.join(env.Dir('#.').srcnode().abspath, env['build'])
    version = tuple(map(str, version))
    if env['SHLIBSUFFIX'] == '.dll':
        dlls = env.FindIxes(sources, 'SHLIBPREFIX', 'SHLIBSUFFIX')
        install(env, dlls, 'bin')
        libs = env.FindIxes(sources, 'LIBPREFIX', 'LIBSUFFIX')
        install(env, libs, 'lib')
    else:
        for source in sources:
            target_dir =  os.path.join(install_dir, 'lib')
            target_name = '.'.join((str(source),) + version)
            last = env.InstallAs(os.path.join(target_dir, target_name), source)
            while len(version):
                version = version[:-1]
                target_name = '.'.join((str(source),) + version)
                action = SCons.Action.Action(symlink, "$TARGET -> $SOURCE")
                last = env.Command(os.path.join(target_dir, target_name), last, action) 

def createInstallMethods(env):
    env.AddMethod(install_program, 'InstallProgram')
    env.AddMethod(install_shared_library, 'InstallSharedLibrary')


def num_jobs():
    try:
        return int(os.environ['NUMBER_OF_PROCESSORS'])
    except (ValueError, KeyError):
        pass

    try:
        return os.sysconf('SC_NPROCESSORS_ONLN')
    except (ValueError, OSError, AttributeError):
        pass

    try:
        return int(os.popen2("sysctl -n hw.ncpu")[1].read())
    except ValueError:
        pass

    return 1


def generate(env):
    """Common environment generation code"""

    # Toolchain
    platform = env['platform']
    if env['toolchain'] == 'default':
        if platform == 'winddk':
            env['toolchain'] = 'winddk'
        elif platform == 'wince':
            env['toolchain'] = 'wcesdk'
    env.Tool(env['toolchain'])

    if os.environ.has_key('CC'):
        env['CC'] = os.environ['CC']

    env['gcc'] = 'gcc' in os.path.basename(env['CC']).split('-')
    env['msvc'] = env['CC'] == 'cl'

    # shortcuts
    debug = env['debug']
    machine = env['machine']
    platform = env['platform']
    x86 = env['machine'] == 'x86'
    ppc = env['machine'] == 'ppc'
    gcc = env['gcc']
    msvc = env['msvc']

    # Put build output in a separate dir, which depends on the current
    # configuration. See also http://www.scons.org/wiki/AdvancedBuildExample
    build_topdir = 'build'
    build_subdir = env['platform']
    if env['llvm']:
        build_subdir += "-llvm"
    if env['machine'] != 'generic':
        build_subdir += '-' + env['machine']
    if env['debug']:
        build_subdir += "-debug"
    if env['profile']:
        build_subdir += "-profile"
    build_dir = os.path.join(build_topdir, build_subdir)
    # Place the .sconsign file in the build dir too, to avoid issues with
    # different scons versions building the same source file
    env['build'] = build_dir
    env.SConsignFile(os.path.join(build_dir, '.sconsign'))
    env.CacheDir('build/cache')
    env['CONFIGUREDIR'] = os.path.join(build_dir, 'conf')
    env['CONFIGURELOG'] = os.path.join(os.path.abspath(build_dir), 'config.log')

    # Parallel build
    if env.GetOption('num_jobs') <= 1:
        env.SetOption('num_jobs', num_jobs())

    # C preprocessor options
    cppdefines = []
    if debug:
        cppdefines += ['DEBUG']
    else:
        cppdefines += ['NDEBUG']
    if env['profile']:
        cppdefines += ['PROFILE']
    if platform == 'windows':
        cppdefines += [
            'WIN32',
            '_WINDOWS',
            #'_UNICODE',
            #'UNICODE',
            ('_WIN32_WINNT', '0x0501'), # minimum required OS version
            ('WINVER', '0x0501'),
        ]
        if msvc and env['toolchain'] != 'winddk':
            cppdefines += [
                'VC_EXTRALEAN',
                '_USE_MATH_DEFINES',
                '_CRT_SECURE_NO_WARNINGS',
                '_CRT_SECURE_NO_DEPRECATE',
                '_SCL_SECURE_NO_WARNINGS',
                '_SCL_SECURE_NO_DEPRECATE',
            ]
        if debug:
            cppdefines += ['_DEBUG']
    if env['toolchain'] == 'winddk':
        # Mimic WINDDK's builtin flags. See also:
        # - WINDDK's bin/makefile.new i386mk.inc for more info.
        # - buildchk_wxp_x86.log files, generated by the WINDDK's build
        # - http://alter.org.ua/docs/nt_kernel/vc8_proj/
        if machine == 'x86':
            cppdefines += ['_X86_', 'i386']
        if machine == 'x86_64':
            cppdefines += ['_AMD64_', 'AMD64']
    if platform == 'winddk':
        cppdefines += [
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
    if platform == 'wince':
        cppdefines += [
            '_CRT_SECURE_NO_DEPRECATE',
            '_USE_32BIT_TIME_T',
            'UNICODE',
            '_UNICODE',
            ('UNDER_CE', '600'),
            ('_WIN32_WCE', '0x600'),
            'WINCEOEM',
            'WINCEINTERNAL',
            'WIN32',
            'STRICT',
            'x86',
            '_X86_',
            'INTERNATIONAL',
            ('INTLMSG_CODEPAGE', '1252'),
        ]
    if platform == 'windows':
        cppdefines += ['PIPE_SUBSYSTEM_WINDOWS_USER']
    if platform == 'winddk':
        cppdefines += ['PIPE_SUBSYSTEM_WINDOWS_DISPLAY']
    if platform == 'wince':
        cppdefines += ['PIPE_SUBSYSTEM_WINDOWS_CE']
        cppdefines += ['PIPE_SUBSYSTEM_WINDOWS_CE_OGL']
    if platform == 'embedded':
        cppdefines += ['PIPE_SUBSYSTEM_EMBEDDED']
    env.Append(CPPDEFINES = cppdefines)

    # C compiler options
    cflags = [] # C
    cxxflags = [] # C++
    ccflags = [] # C & C++
    if gcc:
        ccversion = ''
        pipe = SCons.Action._subproc(env, [env['CC'], '--version'],
                                     stdin = 'devnull',
                                     stderr = 'devnull',
                                     stdout = subprocess.PIPE)
        if pipe.wait() == 0:
            line = pipe.stdout.readline()
            match = re.search(r'[0-9]+(\.[0-9]+)+', line)
            if match:
            	ccversion = match.group(0)
        if debug:
            ccflags += ['-O0', '-g3']
        elif ccversion.startswith('4.2.'):
            # gcc 4.2.x optimizer is broken
            print "warning: gcc 4.2.x optimizer is broken -- disabling optimizations"
            ccflags += ['-O0', '-g3']
        else:
            ccflags += ['-O3', '-g3']
        if env['profile']:
            # See http://code.google.com/p/jrfonseca/wiki/Gprof2Dot#Which_options_should_I_pass_to_gcc_when_compiling_for_profiling?
            ccflags += [
                '-fno-omit-frame-pointer',
                '-fno-optimize-sibling-calls',
            ]
        if env['machine'] == 'x86':
            ccflags += [
                '-m32',
                #'-march=pentium4',
                #'-mfpmath=sse',
            ]
            if platform != 'windows':
                # XXX: -mstackrealign causes stack corruption on MinGW. Ditto
                # for -mincoming-stack-boundary=2.  Still enable it on other
                # platforms for now, but we can't rely on it for cross platform
                # code. We have to use __attribute__((force_align_arg_pointer))
                # instead.
                ccflags += [
                    '-mmmx', '-msse', '-msse2', # enable SIMD intrinsics
                ]
        	if distutils.version.LooseVersion(ccversion) >= distutils.version.LooseVersion('4.2'):
		    ccflags += [
                    	'-mstackrealign', # ensure stack is aligned
		    ]
        if env['machine'] == 'x86_64':
            ccflags += ['-m64']
        # See also:
        # - http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
        ccflags += [
            '-Wall',
            '-Wmissing-field-initializers',
            '-Wno-long-long',
            '-ffast-math',
            '-fmessage-length=0', # be nice to Eclipse
        ]
        cflags += [
            '-Wmissing-prototypes',
            '-std=gnu99',
        ]
        if distutils.version.LooseVersion(ccversion) >= distutils.version.LooseVersion('4.2'):
	    ccflags += [
            	'-Werror=pointer-arith',
	    ]
	    cflags += [
            	'-Werror=declaration-after-statement',
	    ]
    if msvc:
        # See also:
        # - http://msdn.microsoft.com/en-us/library/19z1t1wy.aspx
        # - cl /?
        if debug:
            ccflags += [
              '/Od', # disable optimizations
              '/Oi', # enable intrinsic functions
              '/Oy-', # disable frame pointer omission
              '/GL-', # disable whole program optimization
            ]
        else:
            ccflags += [
                '/O2', # optimize for speed
                '/GL', # enable whole program optimization
            ]
        ccflags += [
            '/fp:fast', # fast floating point 
            '/W3', # warning level
            #'/Wp64', # enable 64 bit porting warnings
        ]
        if env['machine'] == 'x86':
            ccflags += [
                #'/arch:SSE2', # use the SSE2 instructions
            ]
        if platform == 'windows':
            ccflags += [
                # TODO
            ]
        if platform == 'winddk':
            ccflags += [
                '/Zl', # omit default library name in .OBJ
                '/Zp8', # 8bytes struct member alignment
                '/Gy', # separate functions for linker
                '/Gm-', # disable minimal rebuild
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
        if platform == 'wince':
            # See also C:\WINCE600\public\common\oak\misc\makefile.def
            ccflags += [
                '/Zl', # omit default library name in .OBJ
                '/GF', # enable read-only string pooling
                '/GR-', # disable C++ RTTI
                '/GS', # enable security checks
                # Allow disabling language conformance to maintain backward compat
                #'/Zc:wchar_t-', # don't force wchar_t as native type, instead of typedef
                #'/Zc:forScope-', # don't enforce Standard C++ for scoping rules
                #'/wd4867',
                #'/wd4430',
                #'/MT',
                #'/U_MT',
            ]
        # Automatic pdb generation
        # See http://scons.tigris.org/issues/show_bug.cgi?id=1656
        env.EnsureSConsVersion(0, 98, 0)
        env['PDB'] = '${TARGET.base}.pdb'
    env.Append(CCFLAGS = ccflags)
    env.Append(CFLAGS = cflags)
    env.Append(CXXFLAGS = cxxflags)

    if env['platform'] == 'windows' and msvc:
        # Choose the appropriate MSVC CRT
        # http://msdn.microsoft.com/en-us/library/2kzt1wy3.aspx
        if env['debug']:
            env.Append(CCFLAGS = ['/MTd'])
            env.Append(SHCCFLAGS = ['/LDd'])
        else:
            env.Append(CCFLAGS = ['/MT'])
            env.Append(SHCCFLAGS = ['/LD'])
    
    # Assembler options
    if gcc:
        if env['machine'] == 'x86':
            env.Append(ASFLAGS = ['-m32'])
        if env['machine'] == 'x86_64':
            env.Append(ASFLAGS = ['-m64'])

    # Linker options
    linkflags = []
    shlinkflags = []
    if gcc:
        if env['machine'] == 'x86':
            linkflags += ['-m32']
        if env['machine'] == 'x86_64':
            linkflags += ['-m64']
        if env['platform'] not in ('darwin'):
            shlinkflags += [
                '-Wl,-Bsymbolic',
            ]
        # Handle circular dependencies in the libraries
        if env['platform'] in ('darwin'):
            pass
        else:
            env['_LIBFLAGS'] = '-Wl,--start-group ' + env['_LIBFLAGS'] + ' -Wl,--end-group'
    if msvc:
        if not env['debug']:
            # enable Link-time Code Generation
            linkflags += ['/LTCG']
            env.Append(ARFLAGS = ['/LTCG'])
    if platform == 'windows' and msvc:
        # See also:
        # - http://msdn2.microsoft.com/en-us/library/y0zzbyt4.aspx
        linkflags += [
            '/fixed:no',
            '/incremental:no',
        ]
    if platform == 'winddk':
        linkflags += [
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
        ]
        if env['debug'] or env['profile']:
            linkflags += [
                '/MAP', # http://msdn.microsoft.com/en-us/library/k7xkk3e2.aspx
            ]
    if platform == 'wince':
        linkflags += [
            '/nodefaultlib',
            #'/incremental:no',
            #'/fullbuild',
            '/entry:_DllMainCRTStartup',
        ]
    env.Append(LINKFLAGS = linkflags)
    env.Append(SHLINKFLAGS = shlinkflags)

    # Default libs
    env.Append(LIBS = [])

    # Custom builders and methods
    env.Tool('custom')
    createInstallMethods(env)

    # for debugging
    #print env.Dump()


def exists(env):
    return 1
