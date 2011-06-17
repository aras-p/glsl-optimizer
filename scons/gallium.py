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
import platform as _platform

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
    target_dir = os.path.join(env.Dir('#.').srcnode().abspath, env['build_dir'], subdir)
    return env.Install(target_dir, source)

def install_program(env, source):
    return install(env, source, 'bin')

def install_shared_library(env, sources, version = ()):
    targets = []
    install_dir = os.path.join(env.Dir('#.').srcnode().abspath, env['build_dir'])
    version = tuple(map(str, version))
    if env['SHLIBSUFFIX'] == '.dll':
        dlls = env.FindIxes(sources, 'SHLIBPREFIX', 'SHLIBSUFFIX')
        targets += install(env, dlls, 'bin')
        libs = env.FindIxes(sources, 'LIBPREFIX', 'LIBSUFFIX')
        targets += install(env, libs, 'lib')
    else:
        for source in sources:
            target_dir =  os.path.join(install_dir, 'lib')
            target_name = '.'.join((str(source),) + version)
            last = env.InstallAs(os.path.join(target_dir, target_name), source)
            targets += last
            while len(version):
                version = version[:-1]
                target_name = '.'.join((str(source),) + version)
                action = SCons.Action.Action(symlink, "  Symlinking $TARGET ...")
                last = env.Command(os.path.join(target_dir, target_name), last, action) 
                targets += last
    return targets


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


def pkg_config_modules(env, name, modules):
    '''Simple wrapper for pkg-config.'''

    env[name] = False

    if env['platform'] == 'windows':
        return

    if not env.Detect('pkg-config'):
        return

    if subprocess.call(["pkg-config", "--exists", ' '.join(modules)]) != 0:
        return

    # Put -I and -L flags directly into the environment, as these don't affect
    # the compilation of targets that do not use them
    try:
        env.ParseConfig('pkg-config --cflags-only-I --libs-only-L ' + ' '.join(modules))
    except OSError:
        return

    # Other flags may affect the compilation of unrelated targets, so store
    # them with a prefix, (e.g., XXX_CFLAGS, XXX_LIBS, etc)
    try:
        flags = env.ParseFlags('!pkg-config --cflags-only-other --libs-only-l --libs-only-other ' + ' '.join(modules))
    except OSError:
        return
    prefix = name.upper() + '_'
    for flag_name, flag_value in flags.iteritems():
        env[prefix + flag_name] = flag_value

    env[name] = True



def generate(env):
    """Common environment generation code"""

    # Tell tools which machine to compile for
    env['TARGET_ARCH'] = env['machine']
    env['MSVS_ARCH'] = env['machine']

    # Toolchain
    platform = env['platform']
    if env['toolchain'] == 'default':
        if platform == 'winddk':
            env['toolchain'] = 'winddk'
        elif platform == 'wince':
            env['toolchain'] = 'wcesdk'
    env.Tool(env['toolchain'])

    # Allow override compiler and specify additional flags from environment
    if os.environ.has_key('CC'):
        env['CC'] = os.environ['CC']
        # Update CCVERSION to match
        pipe = SCons.Action._subproc(env, [env['CC'], '--version'],
                                     stdin = 'devnull',
                                     stderr = 'devnull',
                                     stdout = subprocess.PIPE)
        if pipe.wait() == 0:
            line = pipe.stdout.readline()
            match = re.search(r'[0-9]+(\.[0-9]+)+', line)
            if match:
                env['CCVERSION'] = match.group(0)
    if os.environ.has_key('CFLAGS'):
        env['CCFLAGS'] += SCons.Util.CLVar(os.environ['CFLAGS'])
    if os.environ.has_key('CXX'):
        env['CXX'] = os.environ['CXX']
    if os.environ.has_key('CXXFLAGS'):
        env['CXXFLAGS'] += SCons.Util.CLVar(os.environ['CXXFLAGS'])
    if os.environ.has_key('LDFLAGS'):
        env['LINKFLAGS'] += SCons.Util.CLVar(os.environ['LDFLAGS'])

    env['gcc'] = 'gcc' in os.path.basename(env['CC']).split('-')
    env['msvc'] = env['CC'] == 'cl'

    if env['msvc'] and env['toolchain'] == 'default' and env['machine'] == 'x86_64':
        # MSVC x64 support is broken in earlier versions of scons
        env.EnsurePythonVersion(2, 0)

    # shortcuts
    machine = env['machine']
    platform = env['platform']
    x86 = env['machine'] == 'x86'
    ppc = env['machine'] == 'ppc'
    gcc = env['gcc']
    msvc = env['msvc']

    # Determine whether we are cross compiling; in particular, whether we need
    # to compile code generators with a different compiler as the target code.
    host_platform = _platform.system().lower()
    if host_platform.startswith('cygwin'):
        host_platform = 'cygwin'
    host_machine = os.environ.get('PROCESSOR_ARCHITEW6432', os.environ.get('PROCESSOR_ARCHITECTURE', _platform.machine()))
    host_machine = {
        'x86': 'x86',
        'i386': 'x86',
        'i486': 'x86',
        'i586': 'x86',
        'i686': 'x86',
        'ppc' : 'ppc',
        'AMD64': 'x86_64',
        'x86_64': 'x86_64',
    }.get(host_machine, 'generic')
    env['crosscompile'] = platform != host_platform
    if machine == 'x86_64' and host_machine != 'x86_64':
        env['crosscompile'] = True
    env['hostonly'] = False

    # Backwards compatability with the debug= profile= options
    if env['build'] == 'debug':
        if not env['debug']:
            print 'scons: warning: debug option is deprecated and will be removed eventually; use instead'
            print
            print ' scons build=release'
            print
            env['build'] = 'release'
        if env['profile']:
            print 'scons: warning: profile option is deprecated and will be removed eventually; use instead'
            print
            print ' scons build=profile'
            print
            env['build'] = 'profile'
    if False:
        # Enforce SConscripts to use the new build variable
        env.popitem('debug')
        env.popitem('profile')
    else:
        # Backwards portability with older sconscripts
        if env['build'] in ('debug', 'checked'):
            env['debug'] = True
            env['profile'] = False
        if env['build'] == 'profile':
            env['debug'] = False
            env['profile'] = True
        if env['build'] == 'release':
            env['debug'] = False
            env['profile'] = False

    # Put build output in a separate dir, which depends on the current
    # configuration. See also http://www.scons.org/wiki/AdvancedBuildExample
    build_topdir = 'build'
    build_subdir = env['platform']
    if env['embedded']:
        build_subdir =  'embedded-' + build_subdir
    if env['machine'] != 'generic':
        build_subdir += '-' + env['machine']
    if env['build'] != 'release':
        build_subdir += '-' +  env['build']
    build_dir = os.path.join(build_topdir, build_subdir)
    # Place the .sconsign file in the build dir too, to avoid issues with
    # different scons versions building the same source file
    env['build_dir'] = build_dir
    env.SConsignFile(os.path.join(build_dir, '.sconsign'))
    if 'SCONS_CACHE_DIR' in os.environ:
        print 'scons: Using build cache in %s.' % (os.environ['SCONS_CACHE_DIR'],)
        env.CacheDir(os.environ['SCONS_CACHE_DIR'])
    env['CONFIGUREDIR'] = os.path.join(build_dir, 'conf')
    env['CONFIGURELOG'] = os.path.join(os.path.abspath(build_dir), 'config.log')

    # Parallel build
    if env.GetOption('num_jobs') <= 1:
        env.SetOption('num_jobs', num_jobs())

    env.Decider('MD5-timestamp')
    env.SetOption('max_drift', 60)

    # C preprocessor options
    cppdefines = []
    if env['build'] in ('debug', 'checked'):
        cppdefines += ['DEBUG']
    else:
        cppdefines += ['NDEBUG']
    if env['build'] == 'profile':
        cppdefines += ['PROFILE']
    if env['platform'] in ('posix', 'linux', 'freebsd', 'darwin'):
        cppdefines += [
            '_POSIX_SOURCE',
            ('_POSIX_C_SOURCE', '199309L'),
            '_SVID_SOURCE',
            '_BSD_SOURCE',
            '_GNU_SOURCE',
            'PTHREADS',
            'HAVE_POSIX_MEMALIGN',
        ]
    if env['platform'] == 'darwin':
        cppdefines += ['_DARWIN_C_SOURCE']
    if platform == 'windows':
        cppdefines += [
            'WIN32',
            '_WINDOWS',
            #'_UNICODE',
            #'UNICODE',
            # http://msdn.microsoft.com/en-us/library/aa383745.aspx
            ('_WIN32_WINNT', '0x0601'),
            ('WINVER', '0x0601'),
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
        if env['build'] in ('debug', 'checked'):
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
        if env['build'] in ('debug', 'checked'):
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
    if env['embedded']:
        cppdefines += ['PIPE_SUBSYSTEM_EMBEDDED']
    env.Append(CPPDEFINES = cppdefines)

    # C compiler options
    cflags = [] # C
    cxxflags = [] # C++
    ccflags = [] # C & C++
    if gcc:
        ccversion = env['CCVERSION']
        if env['build'] == 'debug':
            ccflags += ['-O0']
        elif ccversion.startswith('4.2.'):
            # gcc 4.2.x optimizer is broken
            print "warning: gcc 4.2.x optimizer is broken -- disabling optimizations"
            ccflags += ['-O0']
        else:
            ccflags += ['-O3']
        ccflags += ['-g3']
        if env['build'] in ('checked', 'profile'):
            # See http://code.google.com/p/jrfonseca/wiki/Gprof2Dot#Which_options_should_I_pass_to_gcc_when_compiling_for_profiling?
            ccflags += [
                '-fno-omit-frame-pointer',
                '-fno-optimize-sibling-calls',
            ]
        if env['machine'] == 'x86':
            ccflags += [
                '-m32',
                #'-march=pentium4',
            ]
            if distutils.version.LooseVersion(ccversion) >= distutils.version.LooseVersion('4.2') \
               and (platform != 'windows' or env['build'] == 'debug' or True):
                # NOTE: We need to ensure stack is realigned given that we
                # produce shared objects, and have no control over the stack
                # alignment policy of the application. Therefore we need
                # -mstackrealign ore -mincoming-stack-boundary=2.
                #
                # XXX: -O and -mstackrealign causes stack corruption on MinGW
                #
                # XXX: We could have SSE without -mstackrealign if we always used
                # __attribute__((force_align_arg_pointer)), but that's not
                # always the case.
                ccflags += [
                    '-mstackrealign', # ensure stack is aligned
                    '-mmmx', '-msse', '-msse2', # enable SIMD intrinsics
                    #'-mfpmath=sse',
                ]
            if platform in ['windows', 'darwin']:
                # Workaround http://gcc.gnu.org/bugzilla/show_bug.cgi?id=37216
                ccflags += ['-fno-common']
        if env['machine'] == 'x86_64':
            ccflags += ['-m64']
            if platform == 'darwin':
                ccflags += ['-fno-common']
        if env['platform'] != 'windows':
            ccflags += ['-fvisibility=hidden']
        # See also:
        # - http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
        ccflags += [
            '-Wall',
            '-Wno-long-long',
            '-ffast-math',
            '-fmessage-length=0', # be nice to Eclipse
        ]
        cflags += [
            '-Wmissing-prototypes',
            '-std=gnu99',
        ]
        if distutils.version.LooseVersion(ccversion) >= distutils.version.LooseVersion('4.0'):
            ccflags += [
                '-Wmissing-field-initializers',
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
        if env['build'] == 'debug':
            ccflags += [
              '/Od', # disable optimizations
              '/Oi', # enable intrinsic functions
              '/Oy-', # disable frame pointer omission
            ]
        else:
            ccflags += [
                '/O2', # optimize for speed
            ]
        if env['build'] == 'release':
            ccflags += [
                '/GL', # enable whole program optimization
            ]
        else:
            ccflags += [
                '/GL-', # disable whole program optimization
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
        if env['build'] in ('debug', 'checked'):
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
        if env['build'] == 'release':
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
        if env['build'] != 'release':
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

    # We have C++ in several libraries, so always link with the C++ compiler
    if env['gcc']:
        env['LINK'] = env['CXX']

    # Default libs
    libs = []
    if env['platform'] in ('posix', 'linux', 'freebsd', 'darwin'):
        libs += ['m', 'pthread', 'dl']
    env.Append(LIBS = libs)

    # Load tools
    env.Tool('lex')
    env.Tool('yacc')
    if env['llvm']:
        env.Tool('llvm')
    
    pkg_config_modules(env, 'x11', ['x11', 'xext'])
    pkg_config_modules(env, 'drm', ['libdrm'])
    pkg_config_modules(env, 'drm_intel', ['libdrm_intel'])
    pkg_config_modules(env, 'drm_radeon', ['libdrm_radeon'])
    pkg_config_modules(env, 'xorg', ['xorg-server'])
    pkg_config_modules(env, 'kms', ['libkms'])

    env['dri'] = env['x11'] and env['drm']

    # Custom builders and methods
    env.Tool('custom')
    createInstallMethods(env)

    # for debugging
    #print env.Dump()


def exists(env):
    return 1
