dnl Process this file with autoconf to create configure.

AC_PREREQ([2.60])

dnl Versioning - scrape the version from configs/default
m4_define([mesa_version],
    [m4_esyscmd([${MAKE-make} -s -f bin/version.mk version | tr -d '\n' | tr -d '\r'])])
m4_ifval(mesa_version,,
    [m4_fatal([Failed to get the Mesa version from `make -f bin/version.mk version`])])

dnl Tell the user about autoconf.html in the --help output
m4_divert_once([HELP_END], [
See docs/autoconf.html for more details on the options for Mesa.])

AC_INIT([Mesa],[mesa_version],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa])
AC_CONFIG_AUX_DIR([bin])
AC_CANONICAL_HOST
AM_INIT_AUTOMAKE([foreign])

dnl Save user CFLAGS and CXXFLAGS so one can override the default ones
USER_CFLAGS="$CFLAGS"
USER_CXXFLAGS="$CXXFLAGS"

dnl Versions for external dependencies
LIBDRM_REQUIRED=2.4.24
LIBDRM_RADEON_REQUIRED=2.4.24
LIBDRM_INTEL_REQUIRED=2.4.30
LIBDRM_NOUVEAU_REQUIRED=0.6
DRI2PROTO_REQUIRED=2.6
GLPROTO_REQUIRED=1.4.14
LIBDRM_XORG_REQUIRED=2.4.24
LIBKMS_XORG_REQUIRED=1.0.0

dnl Check for progs
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_PROGS([MAKE], [gmake make])
AC_CHECK_PROGS([PYTHON2], [python2 python])
AC_PROG_SED
AC_PATH_PROG([MKDEP], [makedepend])

if test "x$MKDEP" = "x"; then
    AC_MSG_ERROR([makedepend is required to build Mesa])
fi

AC_PATH_PROG([FLEX], [flex])
test "x$FLEX" = "x" && AC_MSG_ERROR([flex is needed to build Mesa])

AC_PATH_PROG([BISON], [bison])
test "x$BISON" = "x" && AC_MSG_ERROR([bison is needed to build Mesa])

dnl Our fallback install-sh is a symlink to minstall. Use the existing
dnl configuration in that case.
AC_PROG_INSTALL
test "x$INSTALL" = "x$ac_install_sh" && INSTALL='$(MINSTALL)'

dnl We need a POSIX shell for parts of the build. Assume we have one
dnl in most cases.
case "$host_os" in
solaris*)
    # Solaris /bin/sh is too old/non-POSIX compliant
    AC_PATH_PROGS(POSIX_SHELL, [ksh93 ksh sh])
    SHELL="$POSIX_SHELL"
    ;;
esac

AC_PATH_PROG([GTESTCONFIG], [gtest-config])
if test "x$GTESTCONFIG" != "x"; then
    GTEST_CFLAGS=`gtest-config --cppflags --cxxflags`
    GTEST_LIBS=`gtest-config --ldflags --libs`
    AC_SUBST([GTEST_CFLAGS])
    AC_SUBST([GTEST_LIBS])
    HAVE_GTEST=yes
else
    HAVE_GTEST=no
fi
AM_CONDITIONAL(HAVE_GTEST, test x$HAVE_GTEST = xyes)

dnl clang is mostly GCC-compatible, but its version is much lower,
dnl so we have to check for it.
AC_MSG_CHECKING([if compiling with clang])

AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])],
[CLANG=yes], [CLANG=no])

AC_MSG_RESULT([$CLANG])

dnl If we're using GCC, make sure that it is at least version 3.3.0.  Older
dnl versions are explictly not supported.
if test "x$GCC" = xyes -a "x$CLANG" = xno; then
    AC_MSG_CHECKING([whether gcc version is sufficient])
    major=0
    minor=0

    GCC_VERSION=`$CC -dumpversion`
    if test $? -eq 0; then
        major=`echo $GCC_VERSION | cut -d. -f1`
        minor=`echo $GCC_VERSION | cut -d. -f2`
    fi

    if test $major -lt 3 -o $major -eq 3 -a $minor -lt 3 ; then
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([If using GCC, version 3.3.0 or later is required.])
    else
        AC_MSG_RESULT([yes])
    fi
fi


MKDEP_OPTIONS=-fdepend
dnl Ask gcc where it's keeping its secret headers
if test "x$GCC" = xyes; then
    for dir in include include-fixed; do
        GCC_INCLUDES=`$CC -print-file-name=$dir`
        if test "x$GCC_INCLUDES" != x && \
           test "$GCC_INCLUDES" != "$dir" && \
           test -d "$GCC_INCLUDES"; then
            MKDEP_OPTIONS="$MKDEP_OPTIONS -I$GCC_INCLUDES"
        fi
    done
fi
AC_SUBST([MKDEP_OPTIONS])

dnl Make sure the pkg-config macros are defined
m4_ifndef([PKG_PROG_PKG_CONFIG],
    [m4_fatal([Could not locate the pkg-config autoconf macros.
  These are usually located in /usr/share/aclocal/pkg.m4. If your macros
  are in a different location, try setting the environment variable
  ACLOCAL="aclocal -I/other/macro/dir" before running autoreconf.])])
PKG_PROG_PKG_CONFIG()

dnl LIB_DIR - library basename
LIB_DIR=`echo $libdir | $SED 's%.*/%%'`
AC_SUBST([LIB_DIR])

dnl Cache LDFLAGS so we can add EXTRA_LIB_PATH and restore it later
_SAVE_LDFLAGS="$LDFLAGS"
AC_ARG_VAR([EXTRA_LIB_PATH],[Extra -L paths for the linker])
AC_SUBST([EXTRA_LIB_PATH])

dnl Cache CPPFLAGS so we can add *_INCLUDES and restore it later
_SAVE_CPPFLAGS="$CPPFLAGS"
AC_ARG_VAR([X11_INCLUDES],[Extra -I paths for X11 headers])
AC_SUBST([X11_INCLUDES])

dnl Compiler macros
DEFINES=""
AC_SUBST([DEFINES])
case "$host_os" in
linux*|*-gnu*|gnu*)
    DEFINES="$DEFINES -D_GNU_SOURCE -DPTHREADS"
    ;;
solaris*)
    DEFINES="$DEFINES -DPTHREADS -DSVR4"
    ;;
cygwin*)
    DEFINES="$DEFINES -DPTHREADS"
    ;;
esac

dnl Add flags for gcc and g++
if test "x$GCC" = xyes; then
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -std=c99"
    if test "x$CLANG" = "xno"; then
       CFLAGS="$CFLAGS -ffast-math"
    fi

    # Enable -fvisibility=hidden if using a gcc that supports it
    save_CFLAGS="$CFLAGS"
    AC_MSG_CHECKING([whether $CC supports -fvisibility=hidden])
    VISIBILITY_CFLAGS="-fvisibility=hidden"
    CFLAGS="$CFLAGS $VISIBILITY_CFLAGS"
    AC_LINK_IFELSE([AC_LANG_PROGRAM()], AC_MSG_RESULT([yes]),
		   [VISIBILITY_CFLAGS=""; AC_MSG_RESULT([no])]);

    # Restore CFLAGS; VISIBILITY_CFLAGS are added to it where needed.
    CFLAGS=$save_CFLAGS

    # Work around aliasing bugs - developers should comment this out
    CFLAGS="$CFLAGS -fno-strict-aliasing"

    # gcc's builtin memcmp is slower than glibc's
    # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43052
    CFLAGS="$CFLAGS -fno-builtin-memcmp"
fi
if test "x$GXX" = xyes; then
    CXXFLAGS="$CXXFLAGS -Wall"

    # Enable -fvisibility=hidden if using a gcc that supports it
    save_CXXFLAGS="$CXXFLAGS"
    AC_MSG_CHECKING([whether $CXX supports -fvisibility=hidden])
    VISIBILITY_CXXFLAGS="-fvisibility=hidden"
    CXXFLAGS="$CXXFLAGS $VISIBILITY_CXXFLAGS"
    AC_LANG_PUSH([C++])
    AC_LINK_IFELSE([AC_LANG_PROGRAM()], AC_MSG_RESULT([yes]),
		   [VISIBILITY_CXXFLAGS="" ; AC_MSG_RESULT([no])]);
    AC_LANG_POP([C++])

    # Restore CXXFLAGS; VISIBILITY_CXXFLAGS are added to it where needed.
    CXXFLAGS=$save_CXXFLAGS

    # Work around aliasing bugs - developers should comment this out
    CXXFLAGS="$CXXFLAGS -fno-strict-aliasing"

    # gcc's builtin memcmp is slower than glibc's
    # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43052
    CXXFLAGS="$CXXFLAGS -fno-builtin-memcmp"
fi

dnl even if the compiler appears to support it, using visibility attributes isn't
dnl going to do anything useful currently on cygwin apart from emit lots of warnings
case "$host_os" in
cygwin*)
    VISIBILITY_CFLAGS=""
    VISIBILITY_CXXFLAGS=""
    ;;
esac

AC_SUBST([VISIBILITY_CFLAGS])
AC_SUBST([VISIBILITY_CXXFLAGS])

dnl These should be unnecessary, but let the user set them if they want
AC_ARG_VAR([OPT_FLAGS], [Additional optimization flags for the compiler.
    Default is to use CFLAGS.])
AC_ARG_VAR([ARCH_FLAGS], [Additional architecture specific flags for the
    compiler. Default is to use CFLAGS.])
AC_SUBST([OPT_FLAGS])
AC_SUBST([ARCH_FLAGS])

dnl
dnl Hacks to enable 32 or 64 bit build
dnl
AC_ARG_ENABLE([32-bit],
    [AS_HELP_STRING([--enable-32-bit],
        [build 32-bit libraries @<:@default=auto@:>@])],
    [enable_32bit="$enableval"],
    [enable_32bit=auto]
)
if test "x$enable_32bit" = xyes; then
    if test "x$GCC" = xyes; then
        CFLAGS="$CFLAGS -m32"
        ARCH_FLAGS="$ARCH_FLAGS -m32"
    fi
    if test "x$GXX" = xyes; then
        CXXFLAGS="$CXXFLAGS -m32"
    fi
fi
AC_ARG_ENABLE([64-bit],
    [AS_HELP_STRING([--enable-64-bit],
        [build 64-bit libraries @<:@default=auto@:>@])],
    [enable_64bit="$enableval"],
    [enable_64bit=auto]
)
if test "x$enable_64bit" = xyes; then
    if test "x$GCC" = xyes; then
        CFLAGS="$CFLAGS -m64"
    fi
    if test "x$GXX" = xyes; then
        CXXFLAGS="$CXXFLAGS -m64"
    fi
fi

dnl
dnl shared/static libraries, mimic libtool options
dnl
AC_ARG_ENABLE([static],
    [AS_HELP_STRING([--enable-static],
        [build static libraries @<:@default=disabled@:>@])],
    [enable_static="$enableval"],
    [enable_static=no]
)
case "x$enable_static" in
xyes|xno ) ;;
x ) enable_static=no ;;
* )
    AC_MSG_ERROR([Static library option '$enable_static' is not a valid])
    ;;
esac
AC_ARG_ENABLE([shared],
    [AS_HELP_STRING([--disable-shared],
        [build shared libraries @<:@default=enabled@:>@])],
    [enable_shared="$enableval"],
    [enable_shared=yes]
)
case "x$enable_shared" in
xyes|xno ) ;;
x ) enable_shared=yes ;;
* )
    AC_MSG_ERROR([Shared library option '$enable_shared' is not a valid])
    ;;
esac

dnl Can't have static and shared libraries, default to static if user
dnl explicitly requested. If both disabled, set to static since shared
dnl was explicitly requirested.
case "x$enable_static$enable_shared" in
xyesyes )
    AC_MSG_WARN([Can't build static and shared libraries, disabling shared])
    enable_shared=no
    ;;
xnono )
    AC_MSG_WARN([Can't disable both static and shared libraries, enabling static])
    enable_static=yes
    ;;
esac

dnl
dnl mklib options
dnl
AC_ARG_VAR([MKLIB_OPTIONS],[Options for the Mesa library script, mklib])
if test "$enable_static" = yes; then
    MKLIB_OPTIONS="$MKLIB_OPTIONS -static"
fi
AC_SUBST([MKLIB_OPTIONS])

dnl
dnl other compiler options
dnl
AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [use debug compiler flags and macros @<:@default=disabled@:>@])],
    [enable_debug="$enableval"],
    [enable_debug=no]
)
if test "x$enable_debug" = xyes; then
    DEFINES="$DEFINES -DDEBUG"
    if test "x$GCC" = xyes; then
        CFLAGS="$CFLAGS -g"
    fi
    if test "x$GXX" = xyes; then
        CXXFLAGS="$CXXFLAGS -g"
    fi
fi

dnl
dnl library names
dnl
LIB_PREFIX_GLOB='lib'
LIB_VERSION_SEPARATOR='.'
if test "$enable_static" = yes; then
    LIB_EXTENSION='a'
else
    case "$host_os" in
    darwin* )
        LIB_EXTENSION='dylib' ;;
    cygwin* )
        dnl prefix can be 'cyg' or 'lib'
        LIB_PREFIX_GLOB='???'
        LIB_VERSION_SEPARATOR='-'
        LIB_EXTENSION='dll' ;;
    aix* )
        LIB_EXTENSION='a' ;;
    * )
        LIB_EXTENSION='so' ;;
    esac
fi

dnl
dnl Mangled Mesa support
dnl
AC_ARG_ENABLE([mangling],
  [AS_HELP_STRING([--enable-mangling],
    [enable mangled symbols and library name @<:@default=disabled@:>@])],
  [enable_mangling="${enableval}"],
  [enable_mangling=no]
)
GL_LIB="GL"
GLU_LIB="GLU"
OSMESA_LIB="OSMesa"
if test "x${enable_mangling}" = "xyes" ; then
  DEFINES="${DEFINES} -DUSE_MGL_NAMESPACE"
  GL_LIB="MangledGL"
  GLU_LIB="MangledGLU"
  OSMESA_LIB="MangledOSMesa"
fi
AC_SUBST([GL_LIB])
AC_SUBST([GLU_LIB])
AC_SUBST([OSMESA_LIB])

dnl
dnl potentially-infringing-but-nobody-knows-for-sure stuff
dnl
AC_ARG_ENABLE([texture-float],
    [AS_HELP_STRING([--enable-texture-float],
        [enable floating-point textures and renderbuffers @<:@default=disabled@:>@])],
    [enable_texture_float="$enableval"],
    [enable_texture_float=no]
)
if test "x$enable_texture_float" = xyes; then
    AC_MSG_WARN([Floating-point textures enabled.])
    AC_MSG_WARN([Please consult docs/patents.txt with your lawyer before building Mesa.])
    DEFINES="$DEFINES -DTEXTURE_FLOAT_ENABLED"
fi

GL_LIB_NAME='lib$(GL_LIB).'${LIB_EXTENSION}
GLU_LIB_NAME='lib$(GLU_LIB).'${LIB_EXTENSION}
OSMESA_LIB_NAME='lib$(OSMESA_LIB).'${LIB_EXTENSION}
EGL_LIB_NAME='lib$(EGL_LIB).'${LIB_EXTENSION}
GLESv1_CM_LIB_NAME='lib$(GLESv1_CM_LIB).'${LIB_EXTENSION}
GLESv2_LIB_NAME='lib$(GLESv2_LIB).'${LIB_EXTENSION}
VG_LIB_NAME='lib$(VG_LIB).'${LIB_EXTENSION}
GLAPI_LIB_NAME='lib$(GLAPI_LIB).'${LIB_EXTENSION}
WAYLAND_EGL_LIB_NAME='lib$(WAYLAND_EGL_LIB).'${LIB_EXTENSION}
GBM_LIB_NAME='lib$(GBM_LIB).'${LIB_EXTENSION}

GL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLU_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLU_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
OSMESA_LIB_GLOB=${LIB_PREFIX_GLOB}'$(OSMESA_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
EGL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(EGL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
EGL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(EGL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLESv1_CM_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLESv1_CM_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLESv2_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLESv2_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
VG_LIB_GLOB=${LIB_PREFIX_GLOB}'$(VG_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLAPI_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLAPI_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
WAYLAND_EGL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(WAYLAND_EGL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GBM_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GBM_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'

AC_SUBST([GL_LIB_NAME])
AC_SUBST([GLU_LIB_NAME])
AC_SUBST([OSMESA_LIB_NAME])
AC_SUBST([EGL_LIB_NAME])
AC_SUBST([GLESv1_CM_LIB_NAME])
AC_SUBST([GLESv2_LIB_NAME])
AC_SUBST([VG_LIB_NAME])
AC_SUBST([GLAPI_LIB_NAME])
AC_SUBST([WAYLAND_EGL_LIB_NAME])
AC_SUBST([GBM_LIB_NAME])

AC_SUBST([GL_LIB_GLOB])
AC_SUBST([GLU_LIB_GLOB])
AC_SUBST([OSMESA_LIB_GLOB])
AC_SUBST([EGL_LIB_GLOB])
AC_SUBST([GLESv1_CM_LIB_GLOB])
AC_SUBST([GLESv2_LIB_GLOB])
AC_SUBST([VG_LIB_GLOB])
AC_SUBST([GLAPI_LIB_GLOB])
AC_SUBST([WAYLAND_EGL_LIB_GLOB])
AC_SUBST([GBM_LIB_GLOB])

dnl
dnl Arch/platform-specific settings
dnl
AC_ARG_ENABLE([asm],
    [AS_HELP_STRING([--disable-asm],
        [disable assembly usage @<:@default=enabled on supported plaforms@:>@])],
    [enable_asm="$enableval"],
    [enable_asm=yes]
)
asm_arch=""
ASM_FLAGS=""
MESA_ASM_SOURCES=""
GLAPI_ASM_SOURCES=""
AC_MSG_CHECKING([whether to enable assembly])
test "x$enable_asm" = xno && AC_MSG_RESULT([no])
# disable if cross compiling on x86/x86_64 since we must run gen_matypes
if test "x$enable_asm" = xyes && test "x$cross_compiling" = xyes; then
    case "$host_cpu" in
    i?86 | x86_64)
        enable_asm=no
        AC_MSG_RESULT([no, cross compiling])
        ;;
    esac
fi
# check for supported arches
if test "x$enable_asm" = xyes; then
    case "$host_cpu" in
    i?86)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd*)
            test "x$enable_64bit" = xyes && asm_arch=x86_64 || asm_arch=x86
            ;;
        esac
        ;;
    x86_64)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd*)
            test "x$enable_32bit" = xyes && asm_arch=x86 || asm_arch=x86_64
            ;;
        esac
        ;;
    powerpc)
        case "$host_os" in
        linux*)
            asm_arch=ppc
            ;;
        esac
        ;;
    sparc*)
        case "$host_os" in
        linux*)
            asm_arch=sparc
            ;;
        esac
        ;;
    esac

    case "$asm_arch" in
    x86)
        ASM_FLAGS="-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
        MESA_ASM_SOURCES='$(X86_SOURCES)'
        GLAPI_ASM_SOURCES='$(X86_API)'
        AC_MSG_RESULT([yes, x86])
        ;;
    x86_64)
        ASM_FLAGS="-DUSE_X86_64_ASM"
        MESA_ASM_SOURCES='$(X86-64_SOURCES)'
        GLAPI_ASM_SOURCES='$(X86-64_API)'
        AC_MSG_RESULT([yes, x86_64])
        ;;
    ppc)
        ASM_FLAGS="-DUSE_PPC_ASM -DUSE_VMX_ASM"
        MESA_ASM_SOURCES='$(PPC_SOURCES)'
        AC_MSG_RESULT([yes, ppc])
        ;;
    sparc)
        ASM_FLAGS="-DUSE_SPARC_ASM"
        MESA_ASM_SOURCES='$(SPARC_SOURCES)'
        GLAPI_ASM_SOURCES='$(SPARC_API)'
        AC_MSG_RESULT([yes, sparc])
        ;;
    *)
        AC_MSG_RESULT([no, platform not supported])
        ;;
    esac
fi
AC_SUBST([ASM_FLAGS])
AC_SUBST([MESA_ASM_SOURCES])
AC_SUBST([GLAPI_ASM_SOURCES])

dnl PIC code macro
MESA_PIC_FLAGS

dnl Check to see if dlopen is in default libraries (like Solaris, which
dnl has it in libc), or if libdl is needed to get it.
AC_CHECK_FUNC([dlopen], [],
    [AC_CHECK_LIB([dl], [dlopen], [DLOPEN_LIBS="-ldl"])])
AC_SUBST([DLOPEN_LIBS])

dnl See if posix_memalign is available
AC_CHECK_FUNC([posix_memalign], [DEFINES="$DEFINES -DHAVE_POSIX_MEMALIGN"])

dnl SELinux awareness.
AC_ARG_ENABLE([selinux],
    [AS_HELP_STRING([--enable-selinux],
        [Build SELinux-aware Mesa @<:@default=disabled@:>@])],
    [MESA_SELINUX="$enableval"],
    [MESA_SELINUX=no])
if test "x$enable_selinux" = "xyes"; then
    AC_CHECK_HEADER([selinux/selinux.h],[],
                    [AC_MSG_ERROR([SELinux headers not found])])
    AC_CHECK_LIB([selinux],[is_selinux_enabled],[],
                 [AC_MSG_ERROR([SELinux library not found])])
    SELINUX_LIBS="-lselinux"
    DEFINES="$DEFINES -DMESA_SELINUX"
fi

dnl Options for APIs
AC_ARG_ENABLE([opengl],
    [AS_HELP_STRING([--disable-opengl],
        [disable support for standard OpenGL API @<:@default=no@:>@])],
    [enable_opengl="$enableval"],
    [enable_opengl=yes])
AC_ARG_ENABLE([gles1],
    [AS_HELP_STRING([--enable-gles1],
        [enable support for OpenGL ES 1.x API @<:@default=no@:>@])],
    [enable_gles1="$enableval"],
    [enable_gles1=no])
AC_ARG_ENABLE([gles2],
    [AS_HELP_STRING([--enable-gles2],
        [enable support for OpenGL ES 2.x API @<:@default=no@:>@])],
    [enable_gles2="$enableval"],
    [enable_gles2=no])
AC_ARG_ENABLE([openvg],
    [AS_HELP_STRING([--enable-openvg],
        [enable support for OpenVG API @<:@default=no@:>@])],
    [enable_openvg="$enableval"],
    [enable_openvg=no])

AC_ARG_ENABLE([dri],
    [AS_HELP_STRING([--enable-dri],
        [enable DRI modules @<:@default=auto@:>@])],
    [enable_dri="$enableval"],
    [enable_dri=auto])
AC_ARG_ENABLE([glx],
    [AS_HELP_STRING([--enable-glx],
        [enable GLX library @<:@default=auto@:>@])],
    [enable_glx="$enableval"],
    [enable_glx=auto])
AC_ARG_ENABLE([osmesa],
    [AS_HELP_STRING([--enable-osmesa],
        [enable OSMesa library @<:@default=auto@:>@])],
    [enable_osmesa="$enableval"],
    [enable_osmesa=auto])
AC_ARG_ENABLE([egl],
    [AS_HELP_STRING([--disable-egl],
        [disable EGL library @<:@default=enabled@:>@])],
    [enable_egl="$enableval"],
    [enable_egl=yes])

AC_ARG_ENABLE([xorg],
    [AS_HELP_STRING([--enable-xorg],
        [enable support for X.Org DDX API @<:@default=no@:>@])],
    [enable_xorg="$enableval"],
    [enable_xorg=no])
AC_ARG_ENABLE([xa],
    [AS_HELP_STRING([--enable-xa],
        [enable build of the XA X Acceleration API @<:@default=no@:>@])],
    [enable_xa="$enableval"],
    [enable_xa=no])
AC_ARG_ENABLE([d3d1x],
    [AS_HELP_STRING([--enable-d3d1x],
        [enable support for Direct3D 10 & 11 low-level API @<:@default=no@:>@])],
    [enable_d3d1x="$enableval"],
    [enable_d3d1x=no])
AC_ARG_ENABLE([gbm],
   [AS_HELP_STRING([--enable-gbm],
         [enable gbm library @<:@default=auto@:>@])],
   [enable_gbm="$enableval"],
   [enable_gbm=auto])

AC_ARG_ENABLE([xvmc],
   [AS_HELP_STRING([--enable-xvmc],
         [enable xvmc library @<:@default=auto@:>@])],
   [enable_xvmc="$enableval"],
   [enable_xvmc=auto])
AC_ARG_ENABLE([vdpau],
   [AS_HELP_STRING([--enable-vdpau],
         [enable vdpau library @<:@default=auto@:>@])],
   [enable_vdpau="$enableval"],
   [enable_vdpau=auto])
AC_ARG_ENABLE([va],
   [AS_HELP_STRING([--enable-va],
         [enable va library @<:@default=auto@:>@])],
   [enable_va="$enableval"],
   [enable_va=auto])

AC_ARG_ENABLE([xlib_glx],
    [AS_HELP_STRING([--enable-xlib-glx],
        [make GLX library Xlib-based instead of DRI-based @<:@default=disable@:>@])],
    [enable_xlib_glx="$enableval"],
    [enable_xlib_glx=auto])
AC_ARG_ENABLE([gallium_egl],
    [AS_HELP_STRING([--enable-gallium-egl],
        [enable optional EGL state tracker (not required
         for EGL support in Gallium with OpenGL and OpenGL ES)
         @<:@default=disable@:>@])],
    [enable_gallium_egl="$enableval"],
    [enable_gallium_egl=no])
AC_ARG_ENABLE([gallium_gbm],
    [AS_HELP_STRING([--enable-gallium-gbm],
        [enable optional gbm state tracker (not required for
         gbm support in Gallium)
         @<:@default=auto@:>@])],
    [enable_gallium_gbm="$enableval"],
    [enable_gallium_gbm=auto])

# Option for Gallium drivers
GALLIUM_DRIVERS_DEFAULT="r300,r600,svga,swrast"

AC_ARG_WITH([gallium-drivers],
    [AS_HELP_STRING([--with-gallium-drivers@<:@=DIRS...@:>@],
        [comma delimited Gallium drivers list, e.g.
        "i915,nouveau,r300,r600,svga,swrast"
        @<:@default=r300,r600,swrast@:>@])],
    [with_gallium_drivers="$withval"],
    [with_gallium_drivers="$GALLIUM_DRIVERS_DEFAULT"])

# Doing '--without-gallium-drivers' will set this variable to 'no'.  Clear it
# here so that the script doesn't choke on an unknown driver name later.
case "$with_gallium_drivers" in
    yes) with_gallium_drivers="$GALLIUM_DRIVERS_DEFAULT" ;;
    no) with_gallium_drivers='' ;;
esac

if test "x$enable_opengl" = xno -a \
        "x$enable_gles1" = xno -a \
        "x$enable_gles2" = xno -a \
        "x$enable_openvg" = xno -a \
        "x$enable_xorg" = xno -a \
        "x$enable_xa" = xno -a \
        "x$enable_d3d1x" = xno -a \
        "x$enable_xvmc" = xno -a \
        "x$enable_vdpau" = xno -a \
        "x$enable_va" = xno; then
    AC_MSG_ERROR([at least one API should be enabled])
fi

API_DEFINES=""
if test "x$enable_opengl" = xno; then
    API_DEFINES="$API_DEFINES -DFEATURE_GL=0"
else
    API_DEFINES="$API_DEFINES -DFEATURE_GL=1"
fi
if test "x$enable_gles1" = xyes; then
    API_DEFINES="$API_DEFINES -DFEATURE_ES1=1"
fi
if test "x$enable_gles2" = xyes; then
    API_DEFINES="$API_DEFINES -DFEATURE_ES2=1"
fi
AC_SUBST([API_DEFINES])

AC_ARG_ENABLE([shared-glapi],
    [AS_HELP_STRING([--enable-shared-glapi],
        [EXPERIMENTAL.  Enable shared glapi for OpenGL @<:@default=no@:>@])],
    [enable_shared_glapi="$enableval"],
    [enable_shared_glapi=no])

SHARED_GLAPI="0"
if test "x$enable_shared_glapi" = xyes; then
    SHARED_GLAPI="1"
fi
AC_SUBST([SHARED_GLAPI])

dnl
dnl Driver configuration. Options are xlib, dri and osmesa right now.
dnl More later: fbdev, ...
dnl
default_driver="xlib"

case "$host_os" in
linux*)
    case "$host_cpu" in
    i*86|x86_64|powerpc*|sparc*) default_driver="dri";;
    esac
    ;;
*freebsd* | dragonfly* | *netbsd*)
    case "$host_cpu" in
    i*86|x86_64|powerpc*|sparc*) default_driver="dri";;
    esac
    ;;
esac

if test "x$enable_opengl" = xno; then
    default_driver="no"
fi

AC_ARG_WITH([driver],
    [AS_HELP_STRING([--with-driver=DRIVER], [DEPRECATED])],
    [mesa_driver="$withval"],
    [mesa_driver=auto])
dnl Check for valid option
case "x$mesa_driver" in
xxlib|xdri|xosmesa|xno)
    if test "x$enable_dri" != xauto -o \
            "x$enable_glx" != xauto -o \
            "x$enable_osmesa" != xauto -o \
            "x$enable_xlib_glx" != xauto; then
        AC_MSG_ERROR([--with-driver=$mesa_driver is deprecated])
    fi
    ;;
xauto)
    mesa_driver="$default_driver"
    ;;
*)
    AC_MSG_ERROR([Driver '$mesa_driver' is not a valid option])
    ;;
esac

# map $mesa_driver to APIs
if test "x$enable_dri" = xauto; then
    case "x$mesa_driver" in
    xdri) enable_dri=yes ;;
    *)    enable_dri=no ;;
    esac
fi

if test "x$enable_glx" = xauto; then
    case "x$mesa_driver" in
    xdri|xxlib) enable_glx=yes ;;
    *)          enable_glx=no ;;
    esac
fi

if test "x$enable_osmesa" = xauto; then
    case "x$mesa_driver" in
    xxlib|xosmesa) enable_osmesa=yes ;;
    *)             enable_osmesa=no ;;
    esac
fi

if test "x$enable_xlib_glx" = xauto; then
    case "x$mesa_driver" in
    xxlib) enable_xlib_glx=yes ;;
    *)     enable_xlib_glx=no ;;
    esac
fi

if test "x$enable_glx" = xno; then
    enable_xlib_glx=no
fi

dnl
dnl Driver specific build directories
dnl

dnl this variable will be prepended to SRC_DIRS and is not exported
CORE_DIRS=""

SRC_DIRS=""
GLU_DIRS="sgi"
GALLIUM_DIRS="auxiliary drivers state_trackers"
GALLIUM_TARGET_DIRS=""
GALLIUM_WINSYS_DIRS="sw"
GALLIUM_DRIVERS_DIRS="galahad trace rbug noop identity"
GALLIUM_STATE_TRACKERS_DIRS=""

# build shared-glapi if enabled for OpenGL or if OpenGL ES is enabled
case "x$enable_shared_glapi$enable_gles1$enable_gles2" in
x*yes*)
    CORE_DIRS="$CORE_DIRS mapi/shared-glapi"
    ;;
esac

# build glapi if OpenGL is enabled
if test "x$enable_opengl" = xyes; then
    CORE_DIRS="$CORE_DIRS mapi/glapi"
fi

# build es1api if OpenGL ES 1.x is enabled
if test "x$enable_gles1" = xyes; then
    CORE_DIRS="$CORE_DIRS mapi/es1api"
fi

# build es2api if OpenGL ES 2.x is enabled
if test "x$enable_gles2" = xyes; then
    CORE_DIRS="$CORE_DIRS mapi/es2api"
fi

# build glsl and mesa if OpenGL or OpenGL ES is enabled
case "x$enable_opengl$enable_gles1$enable_gles2" in
x*yes*)
    CORE_DIRS="$CORE_DIRS glsl mesa"
    ;;
esac

case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    DRIVER_DIRS="$DRIVER_DIRS x11"
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/xlib"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS libgl-xlib"
    GALLIUM_STATE_TRACKERS_DIRS="glx $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_WINSYS_XLIB="yes"
    ;;
xyesno)
    # DRI-based GLX
    SRC_DIRS="$SRC_DIRS glx"
    ;;
esac

if test "x$enable_dri" = xyes; then
    DRIVER_DIRS="$DRIVER_DIRS dri"

    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/dri"
    GALLIUM_STATE_TRACKERS_DIRS="dri $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_ST_DRI="yes"
fi

if test "x$enable_osmesa" = xyes; then
    # the empty space matters for osmesa... (see src/mesa/Makefile)
    if test -n "$DRIVER_DIRS"; then
        DRIVER_DIRS="$DRIVER_DIRS osmesa"
    else
        DRIVER_DIRS="osmesa"
    fi
fi

AC_SUBST([SRC_DIRS])
AC_SUBST([GLU_DIRS])
AC_SUBST([DRIVER_DIRS])
AC_SUBST([GALLIUM_DIRS])
AC_SUBST([GALLIUM_TARGET_DIRS])
AC_SUBST([GALLIUM_WINSYS_DIRS])
AC_SUBST([GALLIUM_DRIVERS_DIRS])
AC_SUBST([GALLIUM_STATE_TRACKERS_DIRS])
AC_SUBST([MESA_LLVM])

# Check for libdrm
PKG_CHECK_MODULES([LIBDRM], [libdrm >= $LIBDRM_REQUIRED],
                  [have_libdrm=yes], [have_libdrm=no])

if test "x$enable_dri" = xyes; then
    # DRI must be shared, I think
    if test "$enable_static" = yes; then
        AC_MSG_ERROR([Can't use static libraries for DRI drivers])
    fi

    # not a hard requirement as swrast does not depend on it
    if test "x$have_libdrm" = xyes; then
        DRI_PC_REQ_PRIV="libdrm >= $LIBDRM_REQUIRED"
    fi
fi

dnl
dnl Find out if X is available. The variable have_x is set if libX11 is
dnl found to mimic AC_PATH_XTRA.
dnl
if test -n "$PKG_CONFIG"; then
    AC_MSG_CHECKING([pkg-config files for X11 are available])
    PKG_CHECK_EXISTS([x11],[
        x11_pkgconfig=yes
        have_x=yes
        ],[
        x11_pkgconfig=no
    ])
    AC_MSG_RESULT([$x11_pkgconfig])
else
    x11_pkgconfig=no
fi
dnl Use the autoconf macro if no pkg-config files
if test "$x11_pkgconfig" = yes; then
    PKG_CHECK_MODULES([X11], [x11])
else
    AC_PATH_XTRA
    test -z "$X11_CFLAGS" && X11_CFLAGS="$X_CFLAGS"
    test -z "$X11_LIBS" && X11_LIBS="$X_LIBS -lX11"
    AC_SUBST([X11_CFLAGS])
    AC_SUBST([X11_LIBS])
fi

dnl Try to tell the user that the --x-* options are only used when
dnl pkg-config is not available. This must be right after AC_PATH_XTRA.
m4_divert_once([HELP_BEGIN],
[These options are only used when the X libraries cannot be found by the
pkg-config utility.])

dnl We need X for xlib and dri, so bomb now if it's not found
if test "x$enable_glx" = xyes -a "x$no_x" = xyes; then
    AC_MSG_ERROR([X11 development libraries needed for GLX])
fi

if test "x$enable_glx" = xyes; then
    DEFINES="$DEFINES -DUSE_XCB"
fi

dnl Direct rendering or just indirect rendering
case "$host_os" in
gnu*)
    dnl Disable by default on GNU/Hurd
    driglx_direct_default="no"
    ;;
cygwin*)
    dnl Disable by default on cygwin
    driglx_direct_default="no"
    ;;
*)
    driglx_direct_default="yes"
    ;;
esac
AC_ARG_ENABLE([driglx-direct],
    [AS_HELP_STRING([--disable-driglx-direct],
        [enable direct rendering in GLX and EGL for DRI \
            @<:@default=auto@:>@])],
    [driglx_direct="$enableval"],
    [driglx_direct="$driglx_direct_default"])

dnl
dnl libGL configuration per driver
dnl
case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    # Xlib-based GLX
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES([XLIBGL], [x11 xext])
        GL_PC_REQ_PRIV="x11 xext"
        X11_INCLUDES="$X11_INCLUDES $XLIBGL_CFLAGS"
        GL_LIB_DEPS="$XLIBGL_LIBS"
    else
        # should check these...
        X11_INCLUDES="$X11_INCLUDES $X_CFLAGS"
        GL_LIB_DEPS="$X_LIBS -lX11 -lXext"
        GL_PC_LIB_PRIV="$GL_LIB_DEPS"
        GL_PC_CFLAGS="$X11_INCLUDES"
    fi
    GL_LIB_DEPS="$GL_LIB_DEPS $SELINUX_LIBS -lm -lpthread $DLOPEN_LIBS"
    GL_PC_LIB_PRIV="$GL_PC_LIB_PRIV $SELINUX_LIBS -lm -lpthread"
    ;;
xyesno)
    # DRI-based GLX
    PKG_CHECK_MODULES([GLPROTO], [glproto >= $GLPROTO_REQUIRED])
    GL_PC_REQ_PRIV="glproto >= $GLPROTO_REQUIRED"
    if test x"$driglx_direct" = xyes; then
        if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([Direct rendering requires libdrm >= $LIBDRM_REQUIRED])
        fi
        PKG_CHECK_MODULES([DRI2PROTO], [dri2proto >= $DRI2PROTO_REQUIRED])
        GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV libdrm >= $LIBDRM_REQUIRED dri2proto >= $DRI2PROTO_REQUIRED"
    fi

    # find the DRI deps for libGL
    if test "$x11_pkgconfig" = yes; then
        dri_modules="x11 xext xdamage xfixes x11-xcb xcb-glx"

        # add xf86vidmode if available
        PKG_CHECK_MODULES([XF86VIDMODE], [xxf86vm], HAVE_XF86VIDMODE=yes, HAVE_XF86VIDMODE=no)
        if test "$HAVE_XF86VIDMODE" = yes ; then
            dri_modules="$dri_modules xxf86vm"
        fi

        PKG_CHECK_MODULES([DRIGL], [$dri_modules])
        GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV $dri_modules"
        X11_INCLUDES="$X11_INCLUDES $DRIGL_CFLAGS"
        GL_LIB_DEPS="$DRIGL_LIBS"
    else
        # should check these...
        X11_INCLUDES="$X11_INCLUDES $X_CFLAGS"
        if test "x$HAVE_XF86VIDMODE" == xyes; then
           GL_LIB_DEPS="$X_LIBS -lX11 -lXext -lXxf86vm -lXdamage -lXfixes"
	else
           GL_LIB_DEPS="$X_LIBS -lX11 -lXext -lXdamage -lXfixes"
	fi
        GL_PC_LIB_PRIV="$GL_LIB_DEPS"
        GL_PC_CFLAGS="$X11_INCLUDES"

        # XCB can only be used from pkg-config
        PKG_CHECK_MODULES([XCB],[x11-xcb xcb-glx])
        GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV x11-xcb xcb-glx"
        X11_INCLUDES="$X11_INCLUDES $XCB_CFLAGS"
        GL_LIB_DEPS="$GL_LIB_DEPS $XCB_LIBS"
    fi

    # Check to see if the xcb-glx library is new enough to support
    # GLX_ARB_create_context.  This bit of hackery is necessary until XCB 1.8
    # is released.
    save_CPPFLAGS="$CPPFLAGS"
    save_LDFLAGS="$LDFLAGS"
    CPPFLAGS="$CPPFLAGS $X11_INCLUDES"
    LDFLAGS="$LDFLAGS $GL_LIB_DEPS"
    AC_CHECK_LIB(xcb-glx, xcb_glx_create_context_attribs_arb_checked,
        [HAVE_XCB_GLX_CREATE_CONTEXT=yes],
        [HAVE_XCB_GLX_CREATE_CONTEXT=no])
    CPPFLAGS="$save_CPPFLAGS"
    LDFLAGS="$save_LDFLAGS"

    if test x$HAVE_XCB_GLX_CREATE_CONTEXT = xyes; then
        X11_INCLUDES="$X11_INCLUDES -DHAVE_XCB_GLX_CREATE_CONTEXT"
    fi

    # need DRM libs, -lpthread, etc.
    GL_LIB_DEPS="$GL_LIB_DEPS $LIBDRM_LIBS -lm -lpthread $DLOPEN_LIBS"
    GL_PC_LIB_PRIV="-lm -lpthread $DLOPEN_LIBS"
    ;;
esac

# This is outside the case (above) so that it is invoked even for non-GLX
# builds.
AM_CONDITIONAL(HAVE_XCB_GLX_CREATE_CONTEXT,
    test x$HAVE_XCB_GLX_CREATE_CONTEXT = xyes)

GLESv1_CM_LIB_DEPS="$LIBDRM_LIBS -lm -lpthread $DLOPEN_LIBS"
GLESv1_CM_PC_LIB_PRIV="-lm -lpthread $DLOPEN_LIBS"
GLESv2_LIB_DEPS="$LIBDRM_LIBS -lm -lpthread $DLOPEN_LIBS"
GLESv2_PC_LIB_PRIV="-lm -lpthread $DLOPEN_LIBS"

AC_SUBST([GL_LIB_DEPS])
AC_SUBST([GL_PC_REQ_PRIV])
AC_SUBST([GL_PC_LIB_PRIV])
AC_SUBST([GL_PC_CFLAGS])
AC_SUBST([DRI_PC_REQ_PRIV])
AC_SUBST([GLESv1_CM_LIB_DEPS])
AC_SUBST([GLESv1_CM_PC_LIB_PRIV])
AC_SUBST([GLESv2_LIB_DEPS])
AC_SUBST([GLESv2_PC_LIB_PRIV])

GLAPI_LIB_DEPS="-lpthread $SELINUX_LIBS"
AC_SUBST([GLAPI_LIB_DEPS])


dnl Setup default DRI CFLAGS
DRI_CFLAGS='$(CFLAGS)'
DRI_CXXFLAGS='$(CXXFLAGS)'
DRI_LIB_DEPS='$(TOP)/src/mesa/libmesa.a'
MESA_MODULES='$(TOP)/src/mesa/libmesa.a'

AC_ARG_ENABLE([shared-dricore],
    [AS_HELP_STRING([--enable-shared-dricore],
        [link DRI modules with shared core DRI routines @<:@default=disabled@:>@])],
    [enable_dricore="$enableval"],
    [enable_dricore=no])
if test "x$enable_dri" = xyes ; then
   if test "$enable_dricore" = yes ; then
      if test "$GCC$GXX" != yesyes ; then
      	 AC_MSG_WARN([Shared dricore requires GCC-compatible rpath handling.  Disabling shared dricore])
	 enable_dricore=no
      else
	 DRICORE_GLSL_LIBS='$(TOP)/$(LIB_DIR)/libglsl.so'
	 DRICORE_LIBS='$(TOP)/$(LIB_DIR)/libdricore.so'
	 DRICORE_LIB_DEPS='-L$(TOP)/$(LIB_DIR) -Wl,-R$(DRI_DRIVER_INSTALL_DIR) -lglsl'
      	 DRI_LIB_DEPS='-L$(TOP)/$(LIB_DIR) -Wl,-R$(DRI_DRIVER_INSTALL_DIR) -ldricore -lglsl'
      	 DRI_CFLAGS='$(CFLAGS_NOVISIBILITY) -DUSE_DRICORE'
      	 DRI_CXXFLAGS='$(CXXFLAGS_NOVISIBILITY) -DUSE_DRICORE'
      	 MESA_MODULES='$(DRICORE_LIBS) $(DRICORE_GLSL_LIBS)'
      fi
   fi
fi
AC_SUBST([DRICORE_LIBS])
AC_SUBST([DRICORE_GLSL_LIBS])
AC_SUBST([DRICORE_LIB_DEPS])
AC_SUBST([DRI_CXXFLAGS])
AC_SUBST([DRI_CFLAGS])
AC_SUBST([MESA_MODULES])

AC_SUBST([HAVE_XF86VIDMODE])

dnl
dnl More GLX setup
dnl
case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    DEFINES="$DEFINES -DUSE_XSHM"
    ;;
xyesno)
    DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING"
    if test "x$driglx_direct" = xyes; then
        DEFINES="$DEFINES -DGLX_DIRECT_RENDERING"
    fi
    ;;
esac

dnl
dnl TLS detection
dnl

AC_ARG_ENABLE([glx-tls],
    [AS_HELP_STRING([--enable-glx-tls],
        [enable TLS support in GLX @<:@default=disabled@:>@])],
    [GLX_USE_TLS="$enableval"],
    [GLX_USE_TLS=no])
AC_SUBST(GLX_TLS, ${GLX_USE_TLS})

AS_IF([test "x$GLX_USE_TLS" = xyes],
      [DEFINES="${DEFINES} -DGLX_USE_TLS -DPTHREADS"])

dnl
dnl More DRI setup
dnl
dnl Directory for DRI drivers
AC_ARG_WITH([dri-driverdir],
    [AS_HELP_STRING([--with-dri-driverdir=DIR],
        [directory for the DRI drivers @<:@${libdir}/dri@:>@])],
    [DRI_DRIVER_INSTALL_DIR="$withval"],
    [DRI_DRIVER_INSTALL_DIR='${libdir}/dri'])
AC_SUBST([DRI_DRIVER_INSTALL_DIR])
dnl Extra search path for DRI drivers
AC_ARG_WITH([dri-searchpath],
    [AS_HELP_STRING([--with-dri-searchpath=DIRS...],
        [semicolon delimited DRI driver search directories @<:@${libdir}/dri@:>@])],
    [DRI_DRIVER_SEARCH_DIR="$withval"],
    [DRI_DRIVER_SEARCH_DIR='${DRI_DRIVER_INSTALL_DIR}'])
AC_SUBST([DRI_DRIVER_SEARCH_DIR])
dnl Which drivers to build - default is chosen by platform
AC_ARG_WITH([dri-drivers],
    [AS_HELP_STRING([--with-dri-drivers@<:@=DIRS...@:>@],
        [comma delimited DRI drivers list, e.g.
        "swrast,i965,radeon" @<:@default=auto@:>@])],
    [with_dri_drivers="$withval"],
    [with_dri_drivers=yes])
if test "x$with_dri_drivers" = x; then
    with_dri_drivers=no
fi

dnl If $with_dri_drivers is yes, directories will be added through
dnl platform checks
DRI_DIRS=""
case "$with_dri_drivers" in
no) ;;
yes)
    # classic DRI drivers require FEATURE_GL to build
    if test "x$enable_opengl" = xyes; then
        DRI_DIRS="yes"
    fi
    ;;
*)
    # verify the requested driver directories exist
    dri_drivers=`IFS=', '; echo $with_dri_drivers`
    for driver in $dri_drivers; do
        test -d "$srcdir/src/mesa/drivers/dri/$driver" || \
            AC_MSG_ERROR([DRI driver directory '$driver' doesn't exist])
    done
    DRI_DIRS="$dri_drivers"
    if test -n "$DRI_DIRS" -a "x$enable_opengl" != xyes; then
        AC_MSG_ERROR([--with-dri-drivers requires OpenGL])
    fi
    ;;
esac

dnl Set DRI_DIRS, DEFINES and LIB_DEPS
if test "x$enable_dri" = xyes; then
    # Platform specific settings and drivers to build
    case "$host_os" in
    linux*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        DEFINES="$DEFINES -DHAVE_ALIAS"

        case "$host_cpu" in
        x86_64)
            if test "x$DRI_DIRS" = "xyes"; then
                DRI_DIRS="i915 i965 nouveau r200 radeon swrast"
            fi
            ;;
        powerpc*)
            # Build only the drivers for cards that exist on PowerPC.
            if test "x$DRI_DIRS" = "xyes"; then
                DRI_DIRS="r200 radeon swrast"
            fi
            ;;
        sparc*)
            # Build only the drivers for cards that exist on sparc
            if test "x$DRI_DIRS" = "xyes"; then
                DRI_DIRS="r200 radeon swrast"
            fi
            ;;
        esac
        ;;
    freebsd* | dragonfly* | *netbsd*)
        DEFINES="$DEFINES -DPTHREADS -DUSE_EXTERNAL_DXTN_LIB=1"
        DEFINES="$DEFINES -DIN_DRI_DRIVER -DHAVE_ALIAS"

        if test "x$DRI_DIRS" = "xyes"; then
            DRI_DIRS="i915 i965 nouveau r200 radeon swrast"
        fi
        ;;
    gnu*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        DEFINES="$DEFINES -DHAVE_ALIAS"
	;;
    solaris*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        ;;
    cygwin*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        if test "x$DRI_DIRS" = "xyes"; then
            DRI_DIRS="swrast"
        fi
        ;;
    esac

    # default drivers
    if test "x$DRI_DIRS" = "xyes"; then
        DRI_DIRS="i915 i965 nouveau r200 radeon swrast"
    fi

    DRI_DIRS=`echo "$DRI_DIRS" | $SED 's/  */ /g'`

    # Check for expat
    if test "x$enable_dri" = xyes; then
        EXPAT_INCLUDES=""
        EXPAT_LIB=-lexpat
        AC_ARG_WITH([expat],
            [AS_HELP_STRING([--with-expat=DIR],
                [expat install directory])],[
            EXPAT_INCLUDES="-I$withval/include"
            CPPFLAGS="$CPPFLAGS $EXPAT_INCLUDES"
            LDFLAGS="$LDFLAGS -L$withval/$LIB_DIR"
            EXPAT_LIB="-L$withval/$LIB_DIR -lexpat"
            ])
        AC_CHECK_HEADER([expat.h],[],[AC_MSG_ERROR([Expat required for DRI.])])
        AC_CHECK_LIB([expat],[XML_ParserCreate],[],
            [AC_MSG_ERROR([Expat required for DRI.])])
    fi

    # libdrm is required for all except swrast
    if test -n "$DRI_DIRS" -a x"$DRI_DIRS" != xswrast; then
        if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([DRI drivers requires libdrm >= $LIBDRM_REQUIRED])
        fi
    fi

    # put all the necessary libs together, including possibly libdricore
    DRI_LIB_DEPS="$DRI_LIB_DEPS $SELINUX_LIBS $LIBDRM_LIBS $EXPAT_LIB -lm -lpthread $DLOPEN_LIBS"
fi
AC_SUBST([DRI_DIRS])
AC_SUBST([EXPAT_INCLUDES])
AC_SUBST([DRI_LIB_DEPS])

case $DRI_DIRS in
*i915*|*i965*)
    PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
    ;;
esac

case $DRI_DIRS in
*nouveau*)
    PKG_CHECK_MODULES([NOUVEAU], [libdrm_nouveau >= $LIBDRM_NOUVEAU_REQUIRED])
    ;;
esac

case $DRI_DIRS in
*radeon*|*r200*)
    PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
    ;;
esac


dnl
dnl OSMesa configuration
dnl

dnl Configure the channel bits for OSMesa (libOSMesa, libOSMesa16, ...)
AC_ARG_WITH([osmesa-bits],
    [AS_HELP_STRING([--with-osmesa-bits=BITS],
        [OSMesa channel bits and library name: 8, 16, 32 @<:@default=8@:>@])],
    [osmesa_bits="$withval"],
    [osmesa_bits=8])
if test "x$osmesa_bits" != x8; then
    if test "x$enable_dri" = xyes -o "x$enable_glx" = xyes; then
        AC_MSG_WARN([Ignoring OSMesa channel bits because of non-OSMesa driver])
        osmesa_bits=8
    fi
fi
case "x$osmesa_bits" in
x8)
    OSMESA_LIB="${OSMESA_LIB}"
    ;;
x16|x32)
    OSMESA_LIB="${OSMESA_LIB}$osmesa_bits"
    DEFINES="$DEFINES -DCHAN_BITS=$osmesa_bits -DDEFAULT_SOFTWARE_DEPTH_BITS=31"
    ;;
*)
    AC_MSG_ERROR([OSMesa bits '$osmesa_bits' is not a valid option])
    ;;
esac

if test "x$enable_osmesa" = xyes; then
    # only link libraries with osmesa if shared
    if test "$enable_static" = no; then
        OSMESA_LIB_DEPS="-lm -lpthread $SELINUX_LIBS $DLOPEN_LIBS"
    else
        OSMESA_LIB_DEPS=""
    fi
    OSMESA_MESA_DEPS=""
    OSMESA_PC_LIB_PRIV="-lm -lpthread $SELINUX_LIBS $DLOPEN_LIBS"
fi
AC_SUBST([OSMESA_LIB_DEPS])
AC_SUBST([OSMESA_MESA_DEPS])
AC_SUBST([OSMESA_PC_REQ])
AC_SUBST([OSMESA_PC_LIB_PRIV])

dnl
dnl gbm configuration
dnl
if test "x$enable_gbm" = xauto; then
    case "$with_egl_platforms" in
        *drm*)
            enable_gbm=yes ;;
         *)
            enable_gbm=no ;;
    esac
fi
if test "x$enable_gbm" = xyes; then
    SRC_DIRS="$SRC_DIRS gbm"
    GBM_BACKEND_DIRS=""

    PKG_CHECK_MODULES([LIBUDEV], [libudev], [],
                      AC_MSG_ERROR([gbm needs udev]))
    GBM_LIB_DEPS="$DLOPEN_LIBS $LIBUDEV_LIBS"

    if test "x$enable_dri" = xyes; then
        GBM_BACKEND_DIRS="$GBM_BACKEND_DIRS dri"
        if test "$SHARED_GLAPI" -eq 0; then
            AC_MSG_ERROR([gbm_dri requires --enable-shared-glapi])
        fi
    fi
fi
AC_SUBST([GBM_LIB_DEPS])
AC_SUBST([GBM_BACKEND_DIRS])
GBM_PC_REQ_PRIV="libudev"
GBM_PC_LIB_PRIV="$DLOPEN_LIBS"
GBM_PC_CFLAGS=
AC_SUBST([GBM_PC_REQ_PRIV])
AC_SUBST([GBM_PC_LIB_PRIV])
AC_SUBST([GBM_PC_CFLAGS])

dnl
dnl EGL configuration
dnl
EGL_CLIENT_APIS=""

if test "x$enable_egl" = xyes; then
    SRC_DIRS="$SRC_DIRS egl"
    EGL_LIB_DEPS="$DLOPEN_LIBS $SELINUX_LIBS -lpthread"
    EGL_DRIVERS_DIRS=""

    AC_CHECK_FUNC(mincore, [DEFINES="$DEFINES -DHAVE_MINCORE"])

    if test "$enable_static" != yes; then
        # build egl_glx when libGL is built
        if test "x$enable_glx" = xyes; then
            EGL_DRIVERS_DIRS="glx"
        fi

        PKG_CHECK_MODULES([LIBUDEV], [libudev > 150],
                          [have_libudev=yes],[have_libudev=no])
        if test "$have_libudev" = yes; then
            DEFINES="$DEFINES -DHAVE_LIBUDEV"
        fi
        if test "x$enable_dri" = xyes; then
            # build egl_dri2 when xcb-dri2 is available
            PKG_CHECK_MODULES([XCB_DRI2], [x11-xcb xcb-dri2 xcb-xfixes],
            		  [have_xcb_dri2=yes],[have_xcb_dri2=no])
            
            if test "$have_xcb_dri2" = yes; then
                EGL_DRIVER_DRI2=dri2
                DEFINES="$DEFINES -DHAVE_XCB_DRI2"
                # workaround a bug in xcb-dri2 generated by xcb-proto 1.6
                AC_CHECK_LIB(xcb-dri2, xcb_dri2_connect_alignment_pad, [],
                          [DEFINES="$DEFINES -DXCB_DRI2_CONNECT_DEVICE_NAME_BROKEN"])
            fi
	fi

        EGL_DRIVERS_DIRS="$EGL_DRIVERS_DIRS $EGL_DRIVER_DRI2"
    fi
fi
AC_SUBST([EGL_LIB_DEPS])
AC_SUBST([EGL_DRIVERS_DIRS])

dnl
dnl EGL Gallium configuration
dnl
if test "x$enable_gallium_egl" = xyes; then
    if test "x$with_gallium_drivers" = x; then
        AC_MSG_ERROR([cannot enable egl_gallium without Gallium])
    fi
    if test "x$enable_egl" = xno; then
        AC_MSG_ERROR([cannot enable egl_gallium without EGL])
    fi
    if test "x$have_libdrm" != xyes; then
        AC_MSG_ERROR([egl_gallium requires libdrm >= $LIBDRM_REQUIRED])
    fi

    GALLIUM_STATE_TRACKERS_DIRS="egl $GALLIUM_STATE_TRACKERS_DIRS"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS egl-static"
    HAVE_ST_EGL="yes"
fi

dnl
dnl gbm Gallium configuration
dnl
if test "x$enable_gallium_gbm" = xauto; then
    case "$enable_gbm$HAVE_ST_EGL$enable_dri$with_egl_platforms" in
        yesyesyes*drm*)
            enable_gallium_gbm=yes ;;
         *)
            enable_gallium_gbm=no ;;
    esac
fi
if test "x$enable_gallium_gbm" = xyes; then
    if test "x$with_gallium_drivers" = x; then
        AC_MSG_ERROR([cannot enable gbm_gallium without Gallium])
    fi
    if test "x$enable_gbm" = xno; then
        AC_MSG_ERROR([cannot enable gbm_gallium without gbm])
    fi
    # gbm_gallium abuses DRI_LIB_DEPS to link.  Make sure it is set.
    if test "x$enable_dri" = xno; then
        AC_MSG_ERROR([gbm_gallium requires --enable-dri to build])
    fi

    GALLIUM_STATE_TRACKERS_DIRS="gbm $GALLIUM_STATE_TRACKERS_DIRS"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS gbm"
    HAVE_ST_GBM="yes"
fi

dnl
dnl X.Org DDX configuration
dnl
if test "x$enable_xorg" = xyes; then
    PKG_CHECK_MODULES([XORG], [xorg-server >= 1.6.0])
    PKG_CHECK_MODULES([LIBDRM_XORG], [libdrm >= $LIBDRM_XORG_REQUIRED])
    PKG_CHECK_MODULES([LIBKMS_XORG], [libkms >= $LIBKMS_XORG_REQUIRED])
    PKG_CHECK_MODULES(XEXT, [xextproto >= 7.0.99.1],
        HAVE_XEXTPROTO_71="yes"; DEFINES="$DEFINES -DHAVE_XEXTPROTO_71",
        HAVE_XEXTPROTO_71="no")
    GALLIUM_STATE_TRACKERS_DIRS="xorg $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_ST_XORG=yes
fi

dnl
dnl XA configuration
dnl
if test "x$enable_xa" = xyes; then
AC_PROG_AWK
AC_PROG_GREP
AC_CHECK_PROG(NM, nm, "nm")
if test "x$AWK" = x || test "x$GREP" = x || test "x$NM" = x; then
AC_MSG_WARN([Missing one of nm, grep or awk. Disabling xa.])
enable_xa=no
fi
fi
if test "x$enable_xa" = xyes; then
    GALLIUM_STATE_TRACKERS_DIRS="xa $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_ST_XA=yes
    AC_SUBST(AWK)
    AC_SUBST(GREP)
    AC_SUBST(NM)
fi

dnl
dnl OpenVG configuration
dnl
VG_LIB_DEPS=""

if test "x$enable_openvg" = xyes; then
    if test "x$enable_egl" = xno; then
        AC_MSG_ERROR([cannot enable OpenVG without EGL])
    fi
    if test "x$with_gallium_drivers" = x; then
        AC_MSG_ERROR([cannot enable OpenVG without Gallium])
    fi
    if test "x$enable_gallium_egl" = xno; then
        AC_MSG_ERROR([cannot enable OpenVG without egl_gallium])
    fi

    EGL_CLIENT_APIS="$EGL_CLIENT_APIS "'$(VG_LIB)'
    VG_LIB_DEPS="$VG_LIB_DEPS $SELINUX_LIBS -lpthread"
    CORE_DIRS="$CORE_DIRS mapi/vgapi"
    GALLIUM_STATE_TRACKERS_DIRS="vega $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_ST_VEGA=yes
fi

dnl
dnl D3D1X configuration
dnl

if test "x$enable_d3d1x" = xyes; then
    if test "x$with_gallium_drivers" = x; then
        AC_MSG_ERROR([cannot enable D3D1X without Gallium])
    fi

    GALLIUM_STATE_TRACKERS_DIRS="d3d1x $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_ST_D3D1X=yes
fi

dnl
dnl Gallium G3DVL configuration
dnl
AC_ARG_ENABLE([gallium-g3dvl],
    [AS_HELP_STRING([--enable-gallium-g3dvl],
        [build gallium g3dvl @<:@default=disabled@:>@])],
    [enable_gallium_g3dvl="$enableval"],
    [enable_gallium_g3dvl=no])
if test "x$enable_gallium_g3dvl" = xyes; then
    if test "x$with_gallium_drivers" = x; then
        AC_MSG_ERROR([cannot enable G3DVL without Gallium])
    fi

    if test "x$enable_xvmc" = xauto; then
	PKG_CHECK_EXISTS([xvmc], [enable_xvmc=yes], [enable_xvmc=no])
    fi

    if test "x$enable_vdpau" = xauto; then
	PKG_CHECK_EXISTS([vdpau], [enable_vdpau=yes], [enable_vdpau=no])
    fi

    if test "x$enable_va" = xauto; then
	#don't enable vaapi state tracker even if package exists
	#PKG_CHECK_EXISTS([libva], [enable_vdpau=yes], [enable_vdpau=no])
        enable_va=no
    fi
fi

if test "x$enable_xvmc" = xyes; then
    PKG_CHECK_MODULES([XVMC], [xvmc >= 1.0.6 xorg-server])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS xorg/xvmc"
    HAVE_ST_XVMC="yes"
fi

if test "x$enable_vdpau" = xyes; then
    PKG_CHECK_MODULES([VDPAU], [vdpau >= 0.4.1])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS vdpau"
    HAVE_ST_VDPAU="yes"
fi

if test "x$enable_va" = xyes; then
    PKG_CHECK_MODULES([LIBVA], [libva = 0.31.1])
    AC_MSG_WARN([vaapi state tracker currently unmaintained])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS va"
    HAVE_ST_VA="yes"
fi

dnl
dnl GLU configuration
dnl
AC_ARG_ENABLE([glu],
    [AS_HELP_STRING([--disable-glu],
        [enable OpenGL Utility library @<:@default=enabled@:>@])],
    [enable_glu="$enableval"],
    [enable_glu=yes])

if test "x$enable_glu" = xyes; then
    if test "x$enable_glx" = xno -a "x$enable_osmesa" = xno; then
        AC_MSG_NOTICE([Disabling GLU since there is no OpenGL driver])
        enable_glu=no
    fi
fi

if test "x$enable_glu" = xyes; then
    SRC_DIRS="$SRC_DIRS glu"

    if test "x$enable_glx" = xno; then
        # Link libGLU to libOSMesa instead of libGL
        GLU_LIB_DEPS=""
        GLU_PC_REQ="osmesa"
        if test "$enable_static" = no; then
            GLU_MESA_DEPS='-l$(OSMESA_LIB)'
        else
            GLU_MESA_DEPS=""
        fi
    else
        # If static, empty GLU_LIB_DEPS and add libs for programs to link
        GLU_PC_REQ="gl"
        GLU_PC_LIB_PRIV="-lm"
        if test "$enable_static" = no; then
            GLU_LIB_DEPS="-lm"
            GLU_MESA_DEPS='-l$(GL_LIB)'
        else
            GLU_LIB_DEPS=""
            GLU_MESA_DEPS=""
        fi
    fi
fi
if test "$enable_static" = no; then
    GLU_LIB_DEPS="$GLU_LIB_DEPS $OS_CPLUSPLUS_LIBS"
fi
GLU_PC_LIB_PRIV="$GLU_PC_LIB_PRIV $OS_CPLUSPLUS_LIBS"
AC_SUBST([GLU_LIB_DEPS])
AC_SUBST([GLU_MESA_DEPS])
AC_SUBST([GLU_PC_REQ])
AC_SUBST([GLU_PC_REQ_PRIV])
AC_SUBST([GLU_PC_LIB_PRIV])
AC_SUBST([GLU_PC_CFLAGS])

AC_SUBST([PROGRAM_DIRS])

dnl
dnl Gallium configuration
dnl
if test "x$with_gallium_drivers" != x; then
    SRC_DIRS="$SRC_DIRS gallium gallium/winsys gallium/targets"
fi

AC_SUBST([LLVM_CFLAGS])
AC_SUBST([LLVM_LIBS])
AC_SUBST([LLVM_LDFLAGS])
AC_SUBST([LLVM_VERSION])

case "x$enable_opengl$enable_gles1$enable_gles2" in
x*yes*)
    EGL_CLIENT_APIS="$EGL_CLIENT_APIS "'$(GL_LIB)'
    ;;
esac

AC_SUBST([VG_LIB_DEPS])
AC_SUBST([EGL_CLIENT_APIS])

AC_ARG_WITH([egl-platforms],
    [AS_HELP_STRING([--with-egl-platforms@<:@=DIRS...@:>@],
        [comma delimited native platforms libEGL supports, e.g.
        "x11,drm" @<:@default=auto@:>@])],
    [with_egl_platforms="$withval"],
    [with_egl_platforms=yes])

EGL_PLATFORMS=""
WAYLAND_EGL_LIB_DEPS=""

case "$with_egl_platforms" in
yes)
    if test "x$enable_egl" = xyes; then
        EGL_PLATFORMS="x11"
    fi
    ;;
*)
    if test "x$enable_egl" != xyes; then
        AC_MSG_ERROR([cannot build egl state tracker without EGL library])
    fi
    # verify the requested driver directories exist
    egl_platforms=`IFS=', '; echo $with_egl_platforms`
    for plat in $egl_platforms; do
        test -d "$srcdir/src/gallium/state_trackers/egl/$plat" || \
            AC_MSG_ERROR([EGL platform '$plat' doesn't exist])
        if test "$plat" = "fbdev"; then
                GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/fbdev"
        fi
        if test "$plat" = "null"; then
                GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/null"
        fi
	if test "$plat" = "wayland"; then
		PKG_CHECK_MODULES([WAYLAND], [wayland-client wayland-server],, \
				  [AC_MSG_ERROR([cannot find libwayland-client])])
		WAYLAND_EGL_LIB_DEPS="$WAYLAND_LIBS $LIBDRM_LIBS"
                GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/wayland"
	fi
        if test "$plat" = "drm" && test "x$enable_gbm" = "xno"; then
                AC_MSG_ERROR([EGL platform drm needs gbm])
        fi
        case "$plat$have_libudev" in
                waylandno|drmno)
                    AC_MSG_ERROR([cannot build $plat platfrom without udev]) ;;
        esac
    done
    EGL_PLATFORMS="$egl_platforms"
    ;;
esac
AC_SUBST([EGL_PLATFORMS])

AC_SUBST([WAYLAND_EGL_LIB_DEPS])
WAYLAND_EGL_PC_REQ_PRIV="wayland-client libdrm"
WAYLAND_EGL_PC_LIB_PRIV=
WAYLAND_EGL_PC_CFLAGS=

AC_SUBST([WAYLAND_EGL_PC_REQ_PRIV])
AC_SUBST([WAYLAND_EGL_PC_LIB_PRIV])
AC_SUBST([WAYLAND_EGL_PC_CFLAGS])


AC_ARG_WITH([egl-driver-dir],
    [AS_HELP_STRING([--with-egl-driver-dir=DIR],
                    [directory for EGL drivers [[default=${libdir}/egl]]])],
    [EGL_DRIVER_INSTALL_DIR="$withval"],
    [EGL_DRIVER_INSTALL_DIR='${libdir}/egl'])
AC_SUBST([EGL_DRIVER_INSTALL_DIR])

AC_ARG_WITH([xorg-driver-dir],
    [AS_HELP_STRING([--with-xorg-driver-dir=DIR],
                    [Default xorg driver directory[[default=${libdir}/xorg/modules/drivers]]])],
    [XORG_DRIVER_INSTALL_DIR="$withval"],
    [XORG_DRIVER_INSTALL_DIR="${libdir}/xorg/modules/drivers"])
AC_SUBST([XORG_DRIVER_INSTALL_DIR])

AC_ARG_WITH([max-width],
    [AS_HELP_STRING([--with-max-width=N],
                    [Maximum framebuffer width (4096)])],
    [DEFINES="${DEFINES} -DMAX_WIDTH=${withval}";
     AS_IF([test "${withval}" -gt "4096"],
           [AC_MSG_WARN([Large framebuffer: see s_tritemp.h comments.])])]
)
AC_ARG_WITH([max-height],
    [AS_HELP_STRING([--with-max-height=N],
                    [Maximum framebuffer height (4096)])],
    [DEFINES="${DEFINES} -DMAX_HEIGHT=${withval}";
     AS_IF([test "${withval}" -gt "4096"],
           [AC_MSG_WARN([Large framebuffer: see s_tritemp.h comments.])])]
)

dnl
dnl Gallium LLVM
dnl
AC_ARG_ENABLE([gallium-llvm],
    [AS_HELP_STRING([--enable-gallium-llvm],
        [build gallium LLVM support @<:@default=enabled on x86/x86_64@:>@])],
    [enable_gallium_llvm="$enableval"],
    [enable_gallium_llvm=auto])
if test "x$with_gallium_drivers" = x; then
    enable_gallium_llvm=no
fi
if test "x$enable_gallium_llvm" = xauto; then
    case "$host_cpu" in
    i*86|x86_64) enable_gallium_llvm=yes;;
    esac
fi
if test "x$enable_gallium_llvm" = xyes; then
    AC_PATH_PROG([LLVM_CONFIG], [llvm-config], [no])

    if test "x$LLVM_CONFIG" != xno; then
	LLVM_VERSION=`$LLVM_CONFIG --version | sed 's/svn.*//g'`
	LLVM_CFLAGS=`$LLVM_CONFIG --cppflags|sed -e 's/-DNDEBUG\>//g' -e 's/-pedantic//g'`
	LLVM_LIBS="`$LLVM_CONFIG --libs`"

	LLVM_LDFLAGS=`$LLVM_CONFIG --ldflags`
	DEFINES="$DEFINES -D__STDC_CONSTANT_MACROS"
	MESA_LLVM=1
    else
	MESA_LLVM=0
    fi
else
    MESA_LLVM=0
fi

dnl Directory for VDPAU libs
AC_ARG_WITH([vdpau-libdir],
    [AS_HELP_STRING([--with-vdpau-libdir=DIR],
        [directory for the VDPAU libraries @<:@default=${libdir}/vdpau@:>@])],
    [VDPAU_LIB_INSTALL_DIR="$withval"],
    [VDPAU_LIB_INSTALL_DIR='${libdir}/vdpau'])
AC_SUBST([VDPAU_LIB_INSTALL_DIR])

dnl Directory for VA libs
AC_ARG_WITH([va-libdir],
    [AS_HELP_STRING([--with-va-libdir=DIR],
        [directory for the VA libraries @<:@default=${libdir}/va@:>@])],
    [VA_LIB_INSTALL_DIR="$withval"],
    [VA_LIB_INSTALL_DIR='${libdir}/va'])
AC_SUBST([VA_LIB_INSTALL_DIR])

dnl
dnl Gallium helper functions
dnl
gallium_check_st() {
    if test "x$HAVE_ST_DRI" = xyes || test "x$HAVE_ST_XORG" = xyes ||
	 test "x$HAVE_ST_XA" = xyes || test "x$HAVE_ST_XVMC" = xyes ||
	 test "x$HAVE_ST_VDPAU" = xyes || test "x$HAVE_ST_VA" = xyes; then
         if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([DRI or Xorg DDX requires libdrm >= $LIBDRM_REQUIRED])
         fi
         GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS $1"
    fi
    if test "x$HAVE_ST_DRI" = xyes && test "x$2" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $2"
    fi
    if test "x$HAVE_ST_XORG" = xyes && test "x$3" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $3"
    fi
    if test "x$HAVE_ST_XA" = xyes && test "x$4" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $4"
    fi
    if test "x$HAVE_ST_XVMC" = xyes && test "x$5" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $5"
         NEED_G3DVL_DRI="yes"
    fi
    if test "x$HAVE_ST_VDPAU" = xyes && test "x$6" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $6"
         NEED_G3DVL_DRI="yes"
    fi
    if test "x$HAVE_ST_VA" = xyes && test "x$7" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $7"
         NEED_G3DVL_DRI="yes"
    fi
}

gallium_require_llvm() {
    if test "x$MESA_LLVM" = x0; then
        case "$host_cpu" in
        i*86|x86_64) AC_MSG_ERROR([LLVM is required to build $1 on x86 and x86_64]);;
        esac
    fi
}

dnl Gallium drivers
dnl Duplicates in GALLIUM_DRIVERS_DIRS are removed by sorting it after this block
if test "x$with_gallium_drivers" != x; then
    gallium_drivers=`IFS=', '; echo $with_gallium_drivers`
    for driver in $gallium_drivers; do
        case "x$driver" in
        xsvga)
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS svga softpipe"
            gallium_check_st "svga/drm" "dri-vmwgfx" "" "xa-vmwgfx"
            ;;
        xi915)
            PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS i915 softpipe"
            if test "x$MESA_LLVM" = x1; then
                GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS llvmpipe"
            fi
            GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS i915/sw"
            gallium_check_st "i915/drm" "dri-i915" "xorg-i915"
            ;;
        xr300)
            gallium_require_llvm "Gallium R300"
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r300"
            gallium_check_st "radeon/drm" "dri-r300" "xorg-r300" "" "xvmc-r300" "vdpau-r300" "va-r300"
            ;;
        xr600)
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r600"
            gallium_check_st "radeon/drm" "dri-r600" "xorg-r600" "" "xvmc-r600" "vdpau-r600" "va-r600"
            ;;
        xnouveau)
            PKG_CHECK_MODULES([NOUVEAU], [libdrm_nouveau >= $LIBDRM_NOUVEAU_REQUIRED])
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS nouveau nvfx nv50 nvc0"
            gallium_check_st "nouveau/drm" "dri-nouveau" "xorg-nouveau" "" "xvmc-nouveau" "vdpau-nouveau"
            ;;
        xswrast)
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS softpipe"
            if test "x$MESA_LLVM" = x1; then
                GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS llvmpipe"
            fi

            if test "x$HAVE_ST_DRI" = xyes; then
                GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS dri-swrast"
            fi
            if test "x$HAVE_ST_VDPAU" = xyes; then
                GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS vdpau-softpipe"
            fi
            if test "x$HAVE_ST_XVMC" = xyes; then
                GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS xvmc-softpipe"
            fi
            if test "x$HAVE_ST_VA" = xyes; then
                GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS va-softpipe"
            fi
            if test "x$HAVE_ST_VDPAU" = xyes ||
               test "x$HAVE_ST_XVMC" = xyes ||
               test "x$HAVE_ST_VA" = xyes; then
               if test "x$HAVE_WINSYS_XLIB" != xyes; then
                  GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/xlib"
               fi
            fi
            ;;
        *)
            AC_MSG_ERROR([Unknown Gallium driver: $driver])
            ;;
        esac
    done
fi

if test "x$NEED_G3DVL_DRI" = xyes; then
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS g3dvl/dri"
fi

dnl prepend CORE_DIRS to SRC_DIRS
SRC_DIRS="$CORE_DIRS $SRC_DIRS"

dnl Restore LDFLAGS and CPPFLAGS
LDFLAGS="$_SAVE_LDFLAGS"
CPPFLAGS="$_SAVE_CPPFLAGS"

dnl Add user CFLAGS and CXXFLAGS
CFLAGS="$CFLAGS $USER_CFLAGS"
CXXFLAGS="$CXXFLAGS $USER_CXXFLAGS"

dnl Substitute the config
AC_CONFIG_FILES([configs/autoconf
		tests/Makefile
		tests/glx/Makefile])

dnl Replace the configs/current symlink
AC_CONFIG_COMMANDS([configs],[
if test -f configs/current || test -L configs/current; then
    rm -f configs/current
fi
ln -s autoconf configs/current
])

dnl Sort the dirs alphabetically
GALLIUM_TARGET_DIRS=`echo $GALLIUM_TARGET_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
GALLIUM_WINSYS_DIRS=`echo $GALLIUM_WINSYS_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
GALLIUM_DRIVERS_DIRS=`echo $GALLIUM_DRIVERS_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
GALLIUM_STATE_TRACKERS_DIRS=`echo $GALLIUM_STATE_TRACKERS_DIRS|tr " " "\n"|sort -u|tr "\n" " "`

AC_OUTPUT

dnl
dnl Output some configuration info for the user
dnl
echo ""
echo "        prefix:          $prefix"
echo "        exec_prefix:     $exec_prefix"
echo "        libdir:          $libdir"
echo "        includedir:      $includedir"

dnl API info
echo ""
echo "        OpenGL:          $enable_opengl (ES1: $enable_gles1 ES2: $enable_gles2)"
echo "        OpenVG:          $enable_openvg"

dnl Driver info
echo ""
if test "x$enable_osmesa" != xno; then
        echo "        OSMesa:          lib$OSMESA_LIB"
else
        echo "        OSMesa:          no"
fi

if test "x$enable_dri" != xno; then
        # cleanup the drivers var
        dri_dirs=`echo $DRI_DIRS | $SED 's/^ *//;s/  */ /;s/ *$//'`
        if test "x$DRI_DIRS" = x; then
            echo "        DRI drivers:     no"
        else
            echo "        DRI drivers:     $dri_dirs"
        fi
        echo "        DRI driver dir:  $DRI_DRIVER_INSTALL_DIR"
        echo "        Shared dricore:  $enable_dricore"
fi

case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    echo "        GLX:             Xlib-based"
    ;;
xyesno)
    echo "        GLX:             DRI-based"
    ;;
*)
    echo "        GLX:             $enable_glx"
    ;;
esac

echo ""
echo "        GLU:             $enable_glu"

dnl EGL
echo ""
echo "        EGL:             $enable_egl"
if test "$enable_egl" = yes; then
    echo "        EGL platforms:   $EGL_PLATFORMS"

    egl_drivers=""
    for d in $EGL_DRIVERS_DIRS; do
        egl_drivers="$egl_drivers builtin:egl_$d"
    done

    if test "x$HAVE_ST_EGL" = xyes; then
        echo "        EGL drivers:    ${egl_drivers} egl_gallium"
        echo "        EGL Gallium STs:$EGL_CLIENT_APIS"
    else
        echo "        EGL drivers:    $egl_drivers"
    fi
fi

echo ""
if test "x$MESA_LLVM" = x1; then
    echo "        llvm:            yes"
    echo "        llvm-config:     $LLVM_CONFIG"
    echo "        llvm-version:    $LLVM_VERSION"
else
    echo "        llvm:            no"
fi

echo ""
if echo "$SRC_DIRS" | grep 'gallium' >/dev/null 2>&1; then
    echo "        Gallium:         yes"
    echo "        Gallium dirs:    $GALLIUM_DIRS"
    echo "        Target dirs:     $GALLIUM_TARGET_DIRS"
    echo "        Winsys dirs:     $GALLIUM_WINSYS_DIRS"
    echo "        Driver dirs:     $GALLIUM_DRIVERS_DIRS"
    echo "        Trackers dirs:   $GALLIUM_STATE_TRACKERS_DIRS"
else
    echo "        Gallium:         no"
fi


dnl Libraries
echo ""
echo "        Shared libs:     $enable_shared"
echo "        Static libs:     $enable_static"

dnl Compiler options
# cleanup the CFLAGS/CXXFLAGS/DEFINES vars
cflags=`echo $CFLAGS $OPT_FLAGS $PIC_FLAGS $ARCH_FLAGS | \
    $SED 's/^ *//;s/  */ /;s/ *$//'`
cxxflags=`echo $CXXFLAGS $OPT_FLAGS $PIC_FLAGS $ARCH_FLAGS | \
    $SED 's/^ *//;s/  */ /;s/ *$//'`
defines=`echo $DEFINES $ASM_FLAGS | $SED 's/^ *//;s/  */ /;s/ *$//'`
echo ""
echo "        CFLAGS:          $cflags"
echo "        CXXFLAGS:        $cxxflags"
echo "        Macros:          $defines"
echo ""
echo "        PYTHON2:         $PYTHON2"

echo ""
echo "        Run '${MAKE-make}' to build Mesa"
echo ""
