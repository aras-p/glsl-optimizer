"""generic

Generic tool that provides a commmon ground for all platforms.

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


import os
import os.path
import re
import platform as _platform
import sys

import SCons.Action
import SCons.Builder
import SCons.Scanner


def quietCommandLines(env):
    # Quiet command lines
    # See also http://www.scons.org/wiki/HidingCommandLinesInOutput
    env['CCCOMSTR'] = "Compiling $SOURCE ..."
    env['CXXCOMSTR'] = "Compiling $SOURCE ..."
    env['ARCOMSTR'] = "Archiving $TARGET ..."
    env['RANLIBCOMSTR'] = ""
    env['LINKCOMSTR'] = "Linking $TARGET ..."


def createConvenienceLibBuilder(env):
    """This is a utility function that creates the ConvenienceLibrary
    Builder in an Environment if it is not there already.

    If it is already there, we return the existing one.

    Based on the stock StaticLibrary and SharedLibrary builders.
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

    return convenience_lib


# TODO: handle import statements with multiple modules
# TODO: handle from import statements
import_re = re.compile(r'^import\s+(\S+)$', re.M)

def python_scan(node, env, path):
    # http://www.scons.org/doc/0.98.5/HTML/scons-user/c2781.html#AEN2789
    contents = node.get_contents()
    source_dir = node.get_dir()
    imports = import_re.findall(contents)
    results = []
    for imp in imports:
        for dir in path:
            file = os.path.join(str(dir), imp.replace('.', os.sep) + '.py')
            if os.path.exists(file):
                results.append(env.File(file))
                break
            file = os.path.join(str(dir), imp.replace('.', os.sep), '__init__.py')
            if os.path.exists(file):
                results.append(env.File(file))
                break
    return results

python_scanner = SCons.Scanner.Scanner(function = python_scan, skeys = ['.py'])


def code_generate(env, script, target, source, command):
    """Method to simplify code generation via python scripts.

    http://www.scons.org/wiki/UsingCodeGenerators
    http://www.scons.org/doc/0.98.5/HTML/scons-user/c2768.html
    """

    # We're generating code using Python scripts, so we have to be
    # careful with our scons elements.  This entry represents
    # the generator file *in the source directory*.
    script_src = env.File(script).srcnode()

    # This command creates generated code *in the build directory*.
    command = command.replace('$SCRIPT', script_src.path)
    code = env.Command(target, source, command)

    # Explicitly mark that the generated code depends on the generator,
    # and on implicitly imported python modules
    path = (script_src.get_dir(),)
    deps = [script_src]
    deps += script_src.get_implicit_deps(env, python_scanner, path)
    env.Depends(code, deps)

    # Running the Python script causes .pyc files to be generated in the
    # source directory.  When we clean up, they should go too. So add side
    # effects for .pyc files
    for dep in deps:
        pyc = env.File(str(dep) + 'c')
        env.SideEffect(pyc, code)

    return code


def createCodeGenerateMethod(env):
    env.Append(SCANNERS = python_scanner)
    env.AddMethod(code_generate, 'CodeGenerate')


def symlink(target, source, env):
    target = str(target[0])
    source = str(source[0])
    if os.path.islink(target) or os.path.exists(target):
        os.remove(target)
    os.symlink(os.path.basename(source), target)

def install_shared_library(env, source, version = ()):
    source = str(source[0])
    version = tuple(map(str, version))
    target_dir =  os.path.join(env.Dir('#.').srcnode().abspath, env['build'], 'lib')
    target_name = '.'.join((str(source),) + version)
    last = env.InstallAs(os.path.join(target_dir, target_name), source)
    while len(version):
        version = version[:-1]
        target_name = '.'.join((str(source),) + version)
        action = SCons.Action.Action(symlink, "$TARGET -> $SOURCE")
        last = env.Command(os.path.join(target_dir, target_name), last, action) 

def createInstallMethods(env):
    env.AddMethod(install_shared_library, 'InstallSharedLibrary')


_platform_map = {
    'linux2': 'linux',
    'win32': 'windows',
}


_machine_map = {
	'x86': 'x86',
	'i386': 'x86',
	'i486': 'x86',
	'i586': 'x86',
	'i686': 'x86',
	'ppc': 'ppc',
	'x86_64': 'x86_64',
}


_toolchain_map = {
    'winddk': 'winddk',
    'wince': 'wcesdk',
}


_bool_map = {
    'y': 1, 
    'yes': 1,
    't': 1, 
    'true': 1, 
    '1': 1,
    'on': 1,
    'all': 1, 
    'n': 0, 
    'no': 0, 
    'f': 0, 
    'false': 0, 
    '0': 0,
    'off': 0,
    'none': 0,
}


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

    from SCons.Script import ARGUMENTS

    # FIXME: this is already too late
    #if env.get('quiet', False):
    #    quietCommandLines(env)


    # Platform
    try:
        env['platform'] = ARGUMENTS['platform']
    except KeyError:
        env['platform'] = _platform_map.get(sys.platform, sys.platform)

    # Machine
    try:
        env['machine'] = ARGUMENTS['machine']
    except KeyError:
        env['machine'] = _machine_map.get(os.environ.get('PROCESSOR_ARCHITECTURE', _platform.machine()), 'generic')

    # Toolchain
    try:
        env['toolchain'] = ARGUMENTS['toolchain']
    except KeyError:
        if env['platform'] in ('windows', 'winddk', 'wince') and sys.platform != 'win32':
            env['toolchain'] = 'crossmingw'
        else:
            env['toolchain'] = _toolchain_map.get(env['platform'], 'default')
    if env['toolchain'] == 'crossmingw' and env['machine'] not in ('generic', 'x86'):
            env['machine'] = 'x86'

    try:
        env['MSVS_VERSION'] = ARGUMENTS['MSVS_VERSION']
    except KeyError:
        pass

    # Build type
    env['debug'] = _bool_map[ARGUMENTS.get('debug', 'yes')]
    env['profile'] = _bool_map[ARGUMENTS.get('profile', 'no')]

    # Put build output in a separate dir, which depends on the current
    # configuration. See also http://www.scons.org/wiki/AdvancedBuildExample
    try:
        env['build'] = ARGUMENTS['build']
    except KeyError:
        build_topdir = 'build'
        build_subdir = env['platform']
        if env['machine'] != 'generic':
            build_subdir += '-' + env['machine']
        if env['debug']:
            build_subdir += "-debug"
        if env['profile']:
            build_subdir += "-profile"
        env['build'] = os.path.join(build_topdir, build_subdir)
    # Place the .sconsign file in the build dir too, to avoid issues with
    # different scons versions building the same source file
    env.SConsignFile(os.path.join(env['build'], '.sconsign'))

    # Parallel build
    if env.GetOption('num_jobs') <= 1:
        env.SetOption('num_jobs', num_jobs())

    # Summary
    print
    print '  platform=%s' % env['platform']
    print '  machine=%s' % env['machine']
    print '  toolchain=%s' % env['toolchain']
    print '  debug=%s' % ['no', 'yes'][env['debug']]
    print '  profile=%s' % ['no', 'yes'][env['profile']]
    print '  build=%s' % env['build']
    print '  %s jobs' % env.GetOption('num_jobs')
    print

    # Load tool chain
    env.Tool(env['toolchain'])

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
            # http://msdn2.microsoft.com/en-us/library/6dwk3a1z.aspx,
            #'WIN32_LEAN_AND_MEAN',
        ]
        if msvc:
            cppdefines += [
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
    env.Append(CPPDEFINES = cppdefines)

    # C preprocessor includes
    if platform == 'winddk':
        env.Append(CPPPATH = [
            env['SDK_INC_PATH'],
            env['DDK_INC_PATH'],
            env['WDM_INC_PATH'],
            env['CRT_INC_PATH'],
        ])

    # C compiler options
    cflags = [] # C
    cxxflags = [] # C++
    ccflags = [] # C & C++
    if gcc:
        if debug:
            ccflags += ['-O0', '-g3']
        elif env['toolchain'] == 'crossmingw':
            ccflags += ['-O0', '-g3'] # mingw 4.2.1 optimizer is broken
        else:
            ccflags += ['-O3', '-g0']
        if env['profile']:
            ccflags += ['-pg']
        if env['machine'] == 'x86':
            ccflags += [
                '-m32',
                #'-march=pentium4',
                '-mmmx', '-msse', '-msse2', # enable SIMD intrinsics
                #'-mfpmath=sse',
            ]
        if env['machine'] == 'x86_64':
            ccflags += ['-m64']
        # See also:
        # - http://gcc.gnu.org/onlinedocs/gcc/Warning-Options.html
        ccflags += [
            '-Werror=declaration-after-statement',
            '-Wall',
            '-Wmissing-prototypes',
            '-Wmissing-field-initializers',
            '-Wpointer-arith',
            '-Wno-long-long',
            '-ffast-math',
            '-std=gnu99',
            '-fmessage-length=0', # be nice to Eclipse
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
              '/Ox', # maximum optimizations
              '/Oi', # enable intrinsic functions
              '/Ot', # favor code speed
              #'/fp:fast', # fast floating point 
            ]
        if env['profile']:
            ccflags += [
                '/Gh', # enable _penter hook function
                '/GH', # enable _pexit hook function
            ]
        ccflags += [
            '/W3', # warning level
            #'/Wp64', # enable 64 bit porting warnings
        ]
        if env['machine'] == 'x86':
            ccflags += [
                #'/QIfist', # Suppress _ftol
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
    if gcc:
        if env['machine'] == 'x86':
            linkflags += ['-m32']
        if env['machine'] == 'x86_64':
            linkflags += ['-m64']
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

    # Default libs
    env.Append(LIBS = [])

    # Custom builders and methods
    createConvenienceLibBuilder(env)
    createCodeGenerateMethod(env)
    createInstallMethods(env)

    # for debugging
    #print env.Dump()


def exists(env):
    return 1
