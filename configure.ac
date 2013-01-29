dnl Process this file with autoconf to create configure.

AC_PREREQ([2.60])

dnl Tell the user about autoconf.html in the --help output
m4_divert_once([HELP_END], [
See docs/autoconf.html for more details on the options for Mesa.])

AC_INIT([Mesa], [9.1.0],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa])
AC_CONFIG_AUX_DIR([bin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign])

dnl http://people.gnome.org/~walters/docs/build-api.txt
dnl We don't support srcdir != builddir.
echo \#buildapi-variable-no-builddir >/dev/null

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl Set internal versions
OSMESA_VERSION=8
AC_SUBST([OSMESA_VERSION])

dnl Versions for external dependencies
LIBDRM_REQUIRED=2.4.24
LIBDRM_RADEON_REQUIRED=2.4.40
LIBDRM_INTEL_REQUIRED=2.4.38
LIBDRM_NVVIEUX_REQUIRED=2.4.33
LIBDRM_NOUVEAU_REQUIRED="2.4.33 libdrm >= 2.4.41"
DRI2PROTO_REQUIRED=2.6
GLPROTO_REQUIRED=1.4.14
LIBDRM_XORG_REQUIRED=2.4.24
LIBKMS_XORG_REQUIRED=1.0.0

dnl Check for progs
AC_PROG_CPP
AC_PROG_CC
AX_PROG_CC_FOR_BUILD
AC_PROG_CXX
AX_PROG_CXX_FOR_BUILD
AM_PROG_CC_C_O
AM_PROG_AS
AC_CHECK_PROGS([MAKE], [gmake make])
AC_CHECK_PROGS([PYTHON2], [python2 python])
AX_PYTHON_MODULE([libxml2], [needed])
AC_PROG_SED
AC_PROG_MKDIR_P

LT_PREREQ([2.2])
LT_INIT([disable-static])

AX_PROG_BISON([],
              AS_IF([test ! -f "$srcdir/src/glsl/glcpp/glcpp-parse.c"],
                    [AC_MSG_ERROR([bison not found - unable to compile glcpp-parse.y])]))
AX_PROG_FLEX([],
             AS_IF([test ! -f "$srcdir/src/glsl/glcpp/glcpp-lex.c"],
                   [AC_MSG_ERROR([flex not found - unable to compile glcpp-lex.l])]))

AC_PATH_PROG([PERL], [perl])

AC_CHECK_PROG(INDENT, indent, indent, cat)
if test "x$INDENT" != "xcat"; then
    AC_SUBST(INDENT_FLAGS, '-i4 -nut -br -brs -npcs -ce -TGLubyte -TGLbyte -TBool')
fi

AC_PROG_INSTALL

dnl We need a POSIX shell for parts of the build. Assume we have one
dnl in most cases.
case "$host_os" in
solaris*)
    # Solaris /bin/sh is too old/non-POSIX compliant
    AC_PATH_PROGS(POSIX_SHELL, [ksh93 ksh sh])
    SHELL="$POSIX_SHELL"
    ;;
esac

dnl clang is mostly GCC-compatible, but its version is much lower,
dnl so we have to check for it.
AC_MSG_CHECKING([if compiling with clang])

AC_COMPILE_IFELSE(
[AC_LANG_PROGRAM([], [[
#ifndef __clang__
       not clang
#endif
]])],
[acv_mesa_CLANG=yes], [acv_mesa_CLANG=no])

AC_MSG_RESULT([$acv_mesa_CLANG])

dnl If we're using GCC, make sure that it is at least version 3.3.0.  Older
dnl versions are explictly not supported.
if test "x$GCC" = xyes -a "x$acv_mesa_CLANG" = xno; then
    AC_MSG_CHECKING([whether gcc version is sufficient])
    major=0
    minor=0

    GCC_VERSION=`$CC -dumpversion`
    if test $? -eq 0; then
        GCC_VERSION_MAJOR=`echo $GCC_VERSION | cut -d. -f1`
        GCC_VERSION_MINOR=`echo $GCC_VERSION | cut -d. -f2`
    fi

    if test $GCC_VERSION_MAJOR -lt 3 -o $GCC_VERSION_MAJOR -eq 3 -a $GCC_VERSION_MINOR -lt 3 ; then
        AC_MSG_RESULT([no])
        AC_MSG_ERROR([If using GCC, version 3.3.0 or later is required.])
    else
        AC_MSG_RESULT([yes])
    fi
fi

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

dnl Cache LDFLAGS and CPPFLAGS so we can add to them and restore later
_SAVE_LDFLAGS="$LDFLAGS"
_SAVE_CPPFLAGS="$CPPFLAGS"

dnl build host compiler macros
DEFINES_FOR_BUILD=""
AC_SUBST([DEFINES_FOR_BUILD])
case "$build_os" in
linux*|*-gnu*|gnu*)
    DEFINES_FOR_BUILD="$DEFINES_FOR_BUILD -D_GNU_SOURCE"
    ;;
solaris*)
    DEFINES_FOR_BUILD="$DEFINES_FOR_BUILD -DSVR4"
    ;;
cygwin*)
    DEFINES_FOR_BUILD="$DEFINES_FOR_BUILD"
    ;;
esac

dnl Compiler macros
DEFINES=""
AC_SUBST([DEFINES])
case "$host_os" in
linux*|*-gnu*|gnu*)
    DEFINES="$DEFINES -D_GNU_SOURCE -DHAVE_PTHREAD"
    ;;
solaris*)
    DEFINES="$DEFINES -DHAVE_PTHREAD -DSVR4"
    ;;
cygwin*)
    DEFINES="$DEFINES -DHAVE_PTHREAD"
    ;;
esac

dnl Add flags for gcc and g++
if test "x$GCC" = xyes; then
    case "$host_os" in
    cygwin*)
        CFLAGS="$CFLAGS -Wall -std=gnu99"
        ;;
    *)
        CFLAGS_FOR_BUILD="$CFLAGS_FOR_BUILD -Wall -std=c99"
        CFLAGS="$CFLAGS -Wall -std=c99"
        ;;
    esac

    # Enable -Werror=implicit-function-declaration and
    # -Werror=missing-prototypes, if available, or otherwise, just
    # -Wmissing-prototypes.  This is particularly useful to avoid
    # generating a loadable driver module that has undefined symbols.
    save_CFLAGS="$CFLAGS"
    AC_MSG_CHECKING([whether $CC supports -Werror=missing-prototypes])
    CFLAGS="$CFLAGS -Werror=implicit-function-declaration"
    CFLAGS="$CFLAGS -Werror=missing-prototypes"
    AC_LINK_IFELSE([AC_LANG_PROGRAM()],
		   AC_MSG_RESULT([yes]),
		   [CFLAGS="$save_CFLAGS -Wmissing-prototypes";
		    AC_MSG_RESULT([no])]);

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
    CFLAGS_FOR_BUILD="$CFLAGS_FOR_BUILD -fno-strict-aliasing"
    CFLAGS="$CFLAGS -fno-strict-aliasing"

    # gcc's builtin memcmp is slower than glibc's
    # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43052
    CFLAGS_FOR_BUILD="$CFLAGS_FOR_BUILD -fno-builtin-memcmp"
    CFLAGS="$CFLAGS -fno-builtin-memcmp"
fi
if test "x$GXX" = xyes; then
    CXXFLAGS_FOR_BUILD="$CXXFLAGS_FOR_BUILD -Wall"
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
    CXXFLAGS_FOR_BUILD="$CXXFLAGS_FOR_BUILD -fno-strict-aliasing"
    CXXFLAGS="$CXXFLAGS -fno-strict-aliasing"

    # gcc's builtin memcmp is slower than glibc's
    # http://gcc.gnu.org/bugzilla/show_bug.cgi?id=43052
    CXXFLAGS_FOR_BUILD="$CXXFLAGS_FOR_BUILD -fno-builtin-memcmp"
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
        CCASFLAGS="$CCASFLAGS -m32"
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

dnl Can't have static and shared libraries, default to static if user
dnl explicitly requested. If both disabled, set to static since shared
dnl was explicitly requested.
case "x$enable_static$enable_shared" in
xyesyes )
    AC_MSG_WARN([Cannot build static and shared libraries, disabling shared])
    enable_shared=no
    ;;
xnono )
    AC_MSG_WARN([Cannot disable both static and shared libraries, enabling static])
    enable_static=yes
    ;;
esac

AM_CONDITIONAL(BUILD_SHARED, test "x$enable_shared" = xyes)

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
    DEFINES_FOR_BUILD="$DEFINES_FOR_BUILD -DDEBUG"
    if test "x$GCC_FOR_BUILD" = xyes; then
        CFLAGS_FOR_BUILD="$CFLAGS_FOR_BUILD -g -O0"
    fi
    if test "x$GXX_FOR_BUILD" = xyes; then
        CXXFLAGS_FOR_BUILD="$CXXFLAGS_FOR_BUILD -g -O0"
    fi

    DEFINES="$DEFINES -DDEBUG"
    if test "x$GCC" = xyes; then
        CFLAGS="$CFLAGS -g -O0"
    fi
    if test "x$GXX" = xyes; then
        CXXFLAGS="$CXXFLAGS -g -O0"
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
AC_ARG_WITH([gl-lib-name],
  [AS_HELP_STRING([--with-gl-lib-name@<:@=NAME@:>@],
    [specify GL library name @<:@default=GL@:>@])],
  [GL_LIB=$withval],
  [GL_LIB=GL])
AC_ARG_WITH([osmesa-lib-name],
  [AS_HELP_STRING([--with-osmesa-lib-name@<:@=NAME@:>@],
    [specify OSMesa library name @<:@default=OSMesa@:>@])],
  [OSMESA_LIB=$withval],
  [OSMESA_LIB=OSMesa])
AS_IF([test "x$GL_LIB" = xyes], [GL_LIB=GL])
AS_IF([test "x$OSMESA_LIB" = xyes], [OSMESA_LIB=OSMesa])

dnl
dnl Mangled Mesa support
dnl
AC_ARG_ENABLE([mangling],
  [AS_HELP_STRING([--enable-mangling],
    [enable mangled symbols and library name @<:@default=disabled@:>@])],
  [enable_mangling="${enableval}"],
  [enable_mangling=no]
)
if test "x${enable_mangling}" = "xyes" ; then
  DEFINES="${DEFINES} -DUSE_MGL_NAMESPACE"
  GL_LIB="Mangled${GL_LIB}"
  OSMESA_LIB="Mangled${OSMESA_LIB}"
fi
AC_SUBST([GL_LIB])
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
OSMESA_LIB_NAME='lib$(OSMESA_LIB).'${LIB_EXTENSION}
EGL_LIB_NAME='lib$(EGL_LIB).'${LIB_EXTENSION}
GLESv1_CM_LIB_NAME='lib$(GLESv1_CM_LIB).'${LIB_EXTENSION}
GLESv2_LIB_NAME='lib$(GLESv2_LIB).'${LIB_EXTENSION}
VG_LIB_NAME='lib$(VG_LIB).'${LIB_EXTENSION}
GLAPI_LIB_NAME='lib$(GLAPI_LIB).'${LIB_EXTENSION}

GL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
EGL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(EGL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
EGL_LIB_GLOB=${LIB_PREFIX_GLOB}'$(EGL_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLESv1_CM_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLESv1_CM_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLESv2_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLESv2_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
VG_LIB_GLOB=${LIB_PREFIX_GLOB}'$(VG_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'
GLAPI_LIB_GLOB=${LIB_PREFIX_GLOB}'$(GLAPI_LIB)'${LIB_VERSION_SEPARATOR}'*'${LIB_EXTENSION}'*'

AC_SUBST([GL_LIB_NAME])
AC_SUBST([OSMESA_LIB_NAME])
AC_SUBST([EGL_LIB_NAME])
AC_SUBST([GLESv1_CM_LIB_NAME])
AC_SUBST([GLESv2_LIB_NAME])
AC_SUBST([VG_LIB_NAME])
AC_SUBST([GLAPI_LIB_NAME])

AC_SUBST([GL_LIB_GLOB])
AC_SUBST([EGL_LIB_GLOB])
AC_SUBST([GLESv1_CM_LIB_GLOB])
AC_SUBST([GLESv2_LIB_GLOB])
AC_SUBST([VG_LIB_GLOB])
AC_SUBST([GLAPI_LIB_GLOB])

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
MESA_ASM_FILES=""
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
        DEFINES="$DEFINES -DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
        MESA_ASM_FILES='$(X86_FILES)'
        AC_MSG_RESULT([yes, x86])
        ;;
    x86_64)
        DEFINES="$DEFINES -DUSE_X86_64_ASM"
        MESA_ASM_FILES='$(X86_64_FILES)'
        AC_MSG_RESULT([yes, x86_64])
        ;;
    sparc)
        DEFINES="$DEFINES -DUSE_SPARC_ASM"
        MESA_ASM_FILES='$(SPARC_FILES)'
        AC_MSG_RESULT([yes, sparc])
        ;;
    *)
        AC_MSG_RESULT([no, platform not supported])
        ;;
    esac
fi
AC_SUBST([MESA_ASM_FILES])

dnl Check to see if dlopen is in default libraries (like Solaris, which
dnl has it in libc), or if libdl is needed to get it.
AC_CHECK_FUNC([dlopen], [DEFINES="$DEFINES -DHAVE_DLOPEN"],
    [AC_CHECK_LIB([dl], [dlopen],
       [DEFINES="$DEFINES -DHAVE_DLOPEN"; DLOPEN_LIBS="-ldl"])])
AC_SUBST([DLOPEN_LIBS])

AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=],
               [AC_CHECK_LIB([rt], [clock_gettime], [CLOCK_LIB=-lrt],
                             [AC_MSG_ERROR([Couldn't find clock_gettime])])])
AC_SUBST([CLOCK_LIB])

dnl See if posix_memalign is available
AC_CHECK_FUNC([posix_memalign], [DEFINES="$DEFINES -DHAVE_POSIX_MEMALIGN"])

dnl Check for pthreads
AX_PTHREAD
dnl AX_PTHREADS leaves PTHREAD_LIBS empty for gcc and sets PTHREAD_CFLAGS
dnl to -pthread, which causes problems if we need -lpthread to appear in
dnl pkgconfig files.
test -z "$PTHREAD_LIBS" && PTHREAD_LIBS="-lpthread"

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
AC_SUBST([SELINUX_LIBS])

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
        [enable DRI modules @<:@default=enabled@:>@])],
    [enable_dri="$enableval"],
    [enable_dri=yes])
AC_ARG_ENABLE([glx],
    [AS_HELP_STRING([--enable-glx],
        [enable GLX library @<:@default=enabled@:>@])],
    [enable_glx="$enableval"],
    [enable_glx=yes])
AC_ARG_ENABLE([osmesa],
    [AS_HELP_STRING([--enable-osmesa],
        [enable OSMesa library @<:@default=disabled@:>@])],
    [enable_osmesa="$enableval"],
    [enable_osmesa=no])
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
dnl Broken, unmaintained. Don't want to see bug reports about it without patches.
enable_d3d1x=no
dnl AC_ARG_ENABLE([d3d1x],
dnl     [AS_HELP_STRING([--enable-d3d1x],
dnl         [enable support for Direct3D 10 & 11 low-level API @<:@default=no@:>@])],
dnl     [enable_d3d1x="$enableval"],
dnl     [enable_d3d1x=no])
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
AC_ARG_ENABLE([opencl],
   [AS_HELP_STRING([--enable-opencl],
         [enable OpenCL library NOTE: Enabling this option will also enable
          --with-llvm-shared-libs
          @<:@default=no@:>@])],
   [enable_opencl="$enableval" with_llvm_shared_libs="$enableval"],
   [enable_opencl=no])
AC_ARG_ENABLE([xlib_glx],
    [AS_HELP_STRING([--enable-xlib-glx],
        [make GLX library Xlib-based instead of DRI-based @<:@default=disabled@:>@])],
    [enable_xlib_glx="$enableval"],
    [enable_xlib_glx=no])
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

AC_ARG_ENABLE([r600-llvm-compiler],
    [AS_HELP_STRING([--enable-r600-llvm-compiler],
        [Enable experimental LLVM backend for graphics shaders @<:@default=disable@:>@])],
    [enable_r600_llvm="$enableval"],
    [enable_r600_llvm=no])

AC_ARG_ENABLE([gallium_tests],
    [AS_HELP_STRING([--enable-gallium-tests],
        [Enable optional Gallium tests) @<:@default=disable@:>@])],
    [enable_gallium_tests="$enableval"],
    [enable_gallium_tests=no])

# Option for Gallium drivers

# Keep this in sync with the --with-gallium-drivers help string default value
GALLIUM_DRIVERS_DEFAULT="r300,r600,svga,swrast"

AC_ARG_WITH([gallium-drivers],
    [AS_HELP_STRING([--with-gallium-drivers@<:@=DIRS...@:>@],
        [comma delimited Gallium drivers list, e.g.
        "i915,nouveau,r300,r600,radeonsi,svga,swrast"
        @<:@default=r300,r600,svga,swrast@:>@])],
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
        "x$enable_opencl" = xno; then
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

if test "x$enable_glx" = xno; then
    AC_MSG_WARN([GLX disabled, disabling Xlib-GLX])
    enable_xlib_glx=no
fi

if test "x$enable_dri$enable_xlib_glx" = xyesyes; then
    AC_MSG_ERROR([DRI and Xlib-GLX cannot be built together])
fi

# Disable GLX if DRI and Xlib-GLX are not enabled
if test "x$enable_glx" = xyes -a \
        "x$enable_dri" = xno -a \
        "x$enable_xlib_glx" = xno; then
    AC_MSG_WARN([Neither DRI nor Xlib-GLX enabled, disabling GLX])
    enable_glx=no
fi

AM_CONDITIONAL(HAVE_DRI, test "x$enable_dri" = xyes)
AM_CONDITIONAL(NEED_LIBMESA, test "x$enable_xlib_glx" = xyes -o \
                                  "x$enable_osmesa" = xyes)

AC_ARG_ENABLE([shared-glapi],
    [AS_HELP_STRING([--enable-shared-glapi],
        [Enable shared glapi for OpenGL @<:@default=yes@:>@])],
    [enable_shared_glapi="$enableval"],
    [enable_shared_glapi="$enable_dri"])

# Shared GLAPI is only useful for DRI
if test "x$enable_dri" = xno; then
    AC_MSG_NOTICE([Shared GLAPI is only useful for DRI, disabling])
    enable_shared_glapi=no
fi

if test "x$enable_shared_glapi" = xyes; then
    # libGL will use libglapi for function lookups (IN_DRI_DRIVER means to use
    # the remap table)
    DEFINES="$DEFINES -DIN_DRI_DRIVER"
    CORE_DIRS="mapi/shared-glapi"
fi
AM_CONDITIONAL(HAVE_SHARED_GLAPI, test "x$enable_shared_glapi" = xyes)

dnl
dnl Driver specific build directories
dnl
GALLIUM_DIRS="auxiliary drivers state_trackers"
GALLIUM_TARGET_DIRS=""
GALLIUM_WINSYS_DIRS="sw"
GALLIUM_DRIVERS_DIRS="galahad trace rbug noop identity"
GALLIUM_STATE_TRACKERS_DIRS=""

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
    CORE_DIRS="mapi/glapi/gen $CORE_DIRS gtest glsl mesa"
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
    DRIVER_DIRS="$DRIVER_DIRS osmesa"
fi

AC_SUBST([SRC_DIRS])
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
        AC_MSG_ERROR([Cannot use static libraries for DRI drivers])
    fi

    # not a hard requirement as swrast does not depend on it
    if test "x$have_libdrm" = xyes; then
        DRI_PC_REQ_PRIV="libdrm >= $LIBDRM_REQUIRED"
    fi
fi

dnl Find out if X is available.
PKG_CHECK_MODULES([X11], [x11], [no_x=no], [no_x=yes])

dnl Try to tell the user that the --x-* options are only used when
dnl pkg-config is not available. This must be right after AC_PATH_XTRA.
m4_divert_once([HELP_BEGIN],
[These options are only used when the X libraries cannot be found by the
pkg-config utility.])

dnl We need X for xlib and dri, so bomb now if it's not found
if test "x$enable_glx" = xyes -a "x$no_x" = xyes; then
    AC_MSG_ERROR([X11 development libraries needed for GLX])
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
        [disable direct rendering in GLX and EGL for DRI \
            @<:@default=auto@:>@])],
    [driglx_direct="$enableval"],
    [driglx_direct="$driglx_direct_default"])

dnl
dnl libGL configuration per driver
dnl
case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    # Xlib-based GLX
    PKG_CHECK_MODULES([XLIBGL], [x11 xext])
    GL_PC_REQ_PRIV="x11 xext"
    X11_INCLUDES="$X11_INCLUDES $XLIBGL_CFLAGS"
    GL_LIB_DEPS="$XLIBGL_LIBS"
    GL_LIB_DEPS="$GL_LIB_DEPS $SELINUX_LIBS -lm $PTHREAD_LIBS $DLOPEN_LIBS"
    GL_PC_LIB_PRIV="$GL_PC_LIB_PRIV $SELINUX_LIBS -lm $PTHREAD_LIBS"
    ;;
xyesno)
    # DRI-based GLX
    PKG_CHECK_MODULES([GLPROTO], [glproto >= $GLPROTO_REQUIRED])
    if test x"$driglx_direct" = xyes; then
        if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([Direct rendering requires libdrm >= $LIBDRM_REQUIRED])
        fi
        PKG_CHECK_MODULES([DRI2PROTO], [dri2proto >= $DRI2PROTO_REQUIRED])
        GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV libdrm >= $LIBDRM_REQUIRED"
    fi

    # find the DRI deps for libGL
    dri_modules="x11 xext xdamage xfixes x11-xcb xcb-glx >= 1.8.1 xcb-dri2 >= 1.8"

    # add xf86vidmode if available
    PKG_CHECK_MODULES([XF86VIDMODE], [xxf86vm], HAVE_XF86VIDMODE=yes, HAVE_XF86VIDMODE=no)
    if test "$HAVE_XF86VIDMODE" = yes ; then
        dri_modules="$dri_modules xxf86vm"
    fi

    PKG_CHECK_MODULES([DRIGL], [$dri_modules])
    GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV $dri_modules"
    X11_INCLUDES="$X11_INCLUDES $DRIGL_CFLAGS"
    GL_LIB_DEPS="$DRIGL_LIBS"

    # need DRM libs, $PTHREAD_LIBS, etc.
    GL_LIB_DEPS="$GL_LIB_DEPS $LIBDRM_LIBS -lm $PTHREAD_LIBS $DLOPEN_LIBS"
    GL_PC_LIB_PRIV="-lm $PTHREAD_LIBS $DLOPEN_LIBS"
    ;;
esac

# This is outside the case (above) so that it is invoked even for non-GLX
# builds.
AM_CONDITIONAL(HAVE_XF86VIDMODE, test "x$HAVE_XF86VIDMODE" = xyes)

GLESv1_CM_LIB_DEPS="$LIBDRM_LIBS -lm $PTHREAD_LIBS $DLOPEN_LIBS"
GLESv1_CM_PC_LIB_PRIV="-lm $PTHREAD_LIBS $DLOPEN_LIBS"
GLESv2_LIB_DEPS="$LIBDRM_LIBS -lm $PTHREAD_LIBS $DLOPEN_LIBS"
GLESv2_PC_LIB_PRIV="-lm $PTHREAD_LIBS $DLOPEN_LIBS"

AC_SUBST([X11_INCLUDES])
AC_SUBST([GL_LIB_DEPS])
AC_SUBST([GL_PC_REQ_PRIV])
AC_SUBST([GL_PC_LIB_PRIV])
AC_SUBST([GL_PC_CFLAGS])
AC_SUBST([DRI_PC_REQ_PRIV])
AC_SUBST([GLESv1_CM_LIB_DEPS])
AC_SUBST([GLESv1_CM_PC_LIB_PRIV])
AC_SUBST([GLESv2_LIB_DEPS])
AC_SUBST([GLESv2_PC_LIB_PRIV])

DRI_LIB_DEPS="\$(top_builddir)/src/mesa/libdricore/libdricore${VERSION}.la"

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

AS_IF([test "x$GLX_USE_TLS" = xyes -a "x$ax_pthread_ok" = xyes],
      [DEFINES="${DEFINES} -DGLX_USE_TLS -DHAVE_PTHREAD"])

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
            AC_MSG_ERROR([DRI driver directory '$driver' does not exist])
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
        DEFINES="$DEFINES -DHAVE_PTHREAD -DUSE_EXTERNAL_DXTN_LIB=1"
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
	save_LIBS="$LIBS"
        AC_CHECK_LIB([expat],[XML_ParserCreate],[],
            [AC_MSG_ERROR([Expat required for DRI.])])
	LIBS="$save_LIBS"
    fi

    # if we are building any dri driver other than swrast or using the dri state tracker ...
    if test -n "$DRI_DIRS" -a x"$DRI_DIRS" != xswrast || test "x$enable_dri" = xyes; then
        # ... libdrm is required
        if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([DRI drivers requires libdrm >= $LIBDRM_REQUIRED])
        fi
        # ... and build dricommon
        HAVE_COMMON_DRI=yes
    fi

    # put all the necessary libs together
    DRI_LIB_DEPS="$DRI_LIB_DEPS $SELINUX_LIBS $LIBDRM_LIBS $EXPAT_LIB -lm $PTHREAD_LIBS $DLOPEN_LIBS"
    GALLIUM_DRI_LIB_DEPS="$GALLIUM_DRI_LIB_DEPS $SELINUX_LIBS $LIBDRM_LIBS $EXPAT_LIB -lm $CLOCK_LIB $PTHREAD_LIBS $DLOPEN_LIBS"
fi
AM_CONDITIONAL(NEED_LIBDRICORE, test -n "$DRI_DIRS")
AC_SUBST([DRI_DIRS])
AC_SUBST([EXPAT_INCLUDES])
AC_SUBST([DRI_LIB_DEPS])
AC_SUBST([GALLIUM_DRI_LIB_DEPS])

case $DRI_DIRS in
*i915*|*i965*)
    PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])

    for d in $(echo $DRI_DIRS | sed 's/,/ /g'); do
        case $d in
        i915)
            HAVE_I915_DRI=yes;
            ;;
        i965)
            HAVE_I965_DRI=yes;
            ;;
        esac
    done

    ;;
esac

case $DRI_DIRS in
*nouveau*)
    PKG_CHECK_MODULES([NOUVEAU], [libdrm_nouveau >= $LIBDRM_NVVIEUX_REQUIRED])
    HAVE_NOUVEAU_DRI=yes;
    ;;
esac

case $DRI_DIRS in
*radeon*|*r200*)
    PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])

    for d in $(echo $DRI_DIRS | sed 's/,/ /g'); do
        case $d in
        radeon)
            HAVE_RADEON_DRI=yes;
            ;;
        r200)
            HAVE_R200_DRI=yes;
            ;;
        esac
    done

    ;;
esac

case $DRI_DIRS in
*swrast*)
    HAVE_SWRAST_DRI=yes;
    ;;
esac

AM_CONDITIONAL(HAVE_I915_DRI, test x$HAVE_I915_DRI = xyes)
AM_CONDITIONAL(HAVE_I965_DRI, test x$HAVE_I965_DRI = xyes)
AM_CONDITIONAL(HAVE_NOUVEAU_DRI, test x$HAVE_NOUVEAU_DRI = xyes)
AM_CONDITIONAL(HAVE_R200_DRI, test x$HAVE_R200_DRI = xyes)
AM_CONDITIONAL(HAVE_RADEON_DRI, test x$HAVE_RADEON_DRI = xyes)
AM_CONDITIONAL(HAVE_SWRAST_DRI, test x$HAVE_SWRAST_DRI = xyes)
AM_CONDITIONAL(HAVE_COMMON_DRI, test x$HAVE_COMMON_DRI = xyes)

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
        OSMESA_LIB_DEPS="-lm $PTHREAD_LIBS $SELINUX_LIBS $DLOPEN_LIBS"
    else
        OSMESA_LIB_DEPS=""
    fi
    OSMESA_MESA_DEPS=""
    OSMESA_PC_LIB_PRIV="-lm $PTHREAD_LIBS $SELINUX_LIBS $DLOPEN_LIBS"
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

    PKG_CHECK_MODULES([LIBUDEV], [libudev], [],
                      AC_MSG_ERROR([gbm needs udev]))

    if test "x$enable_dri" = xyes; then
        GBM_BACKEND_DIRS="$GBM_BACKEND_DIRS dri"
        if test "x$enable_shared_glapi" = xno; then
            AC_MSG_ERROR([gbm_dri requires --enable-shared-glapi])
        fi
    fi
fi
GBM_PC_REQ_PRIV="libudev"
GBM_PC_LIB_PRIV="$DLOPEN_LIBS"
AC_SUBST([GBM_PC_REQ_PRIV])
AC_SUBST([GBM_PC_LIB_PRIV])

dnl
dnl EGL configuration
dnl
EGL_CLIENT_APIS=""

if test "x$enable_egl" = xyes; then
    SRC_DIRS="$SRC_DIRS egl"
    EGL_LIB_DEPS="$DLOPEN_LIBS $SELINUX_LIBS $PTHREAD_LIBS"

    AC_CHECK_FUNC(mincore, [DEFINES="$DEFINES -DHAVE_MINCORE"])

    if test "$enable_static" != yes; then
        # build egl_glx when libGL is built
        PKG_CHECK_MODULES([LIBUDEV], [libudev > 150],
                          [have_libudev=yes],[have_libudev=no])
        if test "$have_libudev" = yes; then
            DEFINES="$DEFINES -DHAVE_LIBUDEV"
        fi

        if test "x$enable_dri" = xyes; then
	    HAVE_EGL_DRIVER_DRI2=1
	fi

    fi
fi
AC_SUBST([EGL_LIB_DEPS])

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
    enable_gallium_loader=yes
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
    VG_LIB_DEPS="$VG_LIB_DEPS $SELINUX_LIBS $PTHREAD_LIBS"
    CORE_DIRS="$CORE_DIRS mapi/vgapi"
    GALLIUM_STATE_TRACKERS_DIRS="vega $GALLIUM_STATE_TRACKERS_DIRS"
    HAVE_ST_VEGA=yes
    VG_PC_LIB_PRIV="-lm $CLOCK_LIB $PTHREAD_LIBS $DLOPEN_LIBS"
    AC_SUBST([VG_PC_LIB_PRIV])
fi
AM_CONDITIONAL(HAVE_OPENVG, test "x$enable_openvg" = xyes)

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
fi

if test "x$enable_xvmc" = xyes; then
    PKG_CHECK_MODULES([XVMC], [xvmc >= 1.0.6 x11-xcb xcb-dri2 >= 1.8])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS xvmc"
    HAVE_ST_XVMC="yes"
fi

if test "x$enable_vdpau" = xyes; then
    PKG_CHECK_MODULES([VDPAU], [vdpau >= 0.4.1 x11-xcb xcb-dri2 >= 1.8])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS vdpau"
    HAVE_ST_VDPAU="yes"
fi

dnl
dnl OpenCL configuration
dnl

AC_ARG_WITH([libclc-path],
   [AS_HELP_STRING([--with-libclc-path],
         [DEPRECATED: See http://dri.freedesktop.org/wiki/GalliumCompute#How_to_Install])],
   [LIBCLC_PATH="$withval"],
   [LIBCLC_PATH=""])

if test "x$LIBCLC_PATH" != x; then
   AC_MSG_ERROR([The --with-libclc-path option has been deprecated.
                  Please review the updated build instructions for clover:
		  http://dri.freedesktop.org/wiki/GalliumCompute])
fi


AC_ARG_WITH([clang-libdir],
   [AS_HELP_STRING([--with-clang-libdir],
         [Path to Clang libraries @<:@default=llvm-config --libdir@:>@])],
   [CLANG_LIBDIR="$withval"],
   [CLANG_LIBDIR=""])

LIBCLC_INCLUDEDIR=`pkg-config --variable=includedir libclc`
LIBCLC_LIBEXECDIR=`pkg-config --variable=libexecdir libclc`
AC_SUBST([LIBCLC_INCLUDEDIR])
AC_SUBST([LIBCLC_LIBEXECDIR])

if test "x$enable_opencl" = xyes; then
    if test "x$with_gallium_drivers" = x; then
        AC_MSG_ERROR([cannot enable OpenCL without Gallium])
    fi

    if test $GCC_VERSION_MAJOR -lt 4 -o $GCC_VERSION_MAJOR -eq 4 -a $GCC_VERSION_MINOR -lt 6; then
        AC_MSG_ERROR([gcc >= 4.6 is required to build clover])
    fi

    if test "x$LIBCLC_INCLUDEDIR" == x || test "x$LIBCLC_LIBEXECDIR" == x; then
        AC_MSG_ERROR([pkg-config cannot use libclc.pc which is required to build clover])
    fi

    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS clover"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS opencl"
    enable_gallium_loader=yes
fi

if test "x$enable_gallium_gbm" = xyes || test "x$enable_opencl" = xyes; then
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS pipe-loader"
fi

dnl
dnl Gallium configuration
dnl
if test "x$with_gallium_drivers" != x; then
    SRC_DIRS="$SRC_DIRS gallium gallium/winsys gallium/targets"
fi
AM_CONDITIONAL(HAVE_GALLIUM, test "x$with_gallium_drivers" != x)

AC_SUBST([LLVM_BINDIR])
AC_SUBST([LLVM_CFLAGS])
AC_SUBST([LLVM_CPPFLAGS])
AC_SUBST([LLVM_CXXFLAGS])
AC_SUBST([LLVM_LIBDIR])
AC_SUBST([LLVM_LIBS])
AC_SUBST([LLVM_LDFLAGS])
AC_SUBST([LLVM_INCLUDEDIR])
AC_SUBST([LLVM_VERSION])
AC_SUBST([CLANG_RESOURCE_DIR])

case "x$enable_opengl$enable_gles1$enable_gles2" in
x*yes*)
    EGL_CLIENT_APIS="$EGL_CLIENT_APIS "'$(GL_LIB)'
    HAVE_OPENGL=yes
    ;;
esac
AM_CONDITIONAL(HAVE_OPENGL, test "x$HAVE_OPENGL" = xyes)

AC_SUBST([VG_LIB_DEPS])
AC_SUBST([EGL_CLIENT_APIS])

dnl
dnl EGL Platforms configuration
dnl
AC_ARG_WITH([egl-platforms],
    [AS_HELP_STRING([--with-egl-platforms@<:@=DIRS...@:>@],
        [comma delimited native platforms libEGL supports, e.g.
        "x11,drm" @<:@default=auto@:>@])],
    [with_egl_platforms="$withval"],
    [if test "x$enable_egl" = xyes; then
	with_egl_platforms="x11"
    else
	with_egl_platforms=""
    fi])

EGL_PLATFORMS=""

if test "x$with_egl_platforms" != "x" -a "x$enable_egl" != xyes; then
    AC_MSG_ERROR([cannot build egl state tracker without EGL library])
fi

# Do per-EGL platform setups and checks
egl_platforms=`IFS=', '; echo $with_egl_platforms`
for plat in $egl_platforms; do
	case "$plat" in
	fbdev|null)
		GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/$plat"
		;;

	wayland)
		PKG_CHECK_MODULES([WAYLAND], [wayland-client >= 1.0.2 wayland-server >= 1.0.2])
		GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/wayland"

                WAYLAND_PREFIX=`$PKG_CONFIG --variable=prefix wayland-client`
                AC_PATH_PROG([WAYLAND_SCANNER], [wayland-scanner],,
                             [${WAYLAND_PREFIX}/bin$PATH_SEPARATOR$PATH])
		;;

	x11)
		PKG_CHECK_MODULES([XCB_DRI2], [x11-xcb xcb-dri2 >= 1.8 xcb-xfixes])

                if test "x$enable_glx" = xyes; then
                        HAVE_EGL_DRIVER_GLX=1
                fi
		;;

	drm)
		test "x$enable_gbm" = "xno" &&
			AC_MSG_ERROR([EGL platform drm needs gbm])
		;;

	android|gdi)
		;;

	*)
		AC_MSG_ERROR([EGL platform '$plat' does not exist])
		;;
	esac

        case "$plat$have_libudev" in
                waylandno|drmno)
                    AC_MSG_ERROR([cannot build $plat platfrom without udev]) ;;
        esac
done

# libEGL wants to default to the first platform specified in
# ./configure.  parse that here.
if test "x$egl_platforms" != "x"; then
    FIRST_PLATFORM_CAPS=`echo $egl_platforms | sed 's| .*||' | tr 'a-z' 'A-Z'`
    EGL_NATIVE_PLATFORM="_EGL_PLATFORM_$FIRST_PLATFORM_CAPS"
else
    EGL_NATIVE_PLATFORM="_EGL_INVALID_PLATFORM"
fi

EGL_PLATFORMS="$egl_platforms"

AM_CONDITIONAL(HAVE_EGL_PLATFORM_X11, echo "$egl_platforms" | grep 'x11' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_EGL_PLATFORM_WAYLAND, echo "$egl_platforms" | grep 'wayland' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_EGL_PLATFORM_DRM, echo "$egl_platforms" | grep 'drm' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_EGL_PLATFORM_FBDEV, echo "$egl_platforms" | grep 'fbdev' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_EGL_PLATFORM_NULL, echo "$egl_platforms" | grep 'null' >/dev/null 2>&1)

AM_CONDITIONAL(HAVE_EGL_DRIVER_DRI2, test "x$HAVE_EGL_DRIVER_DRI2" != "x")
AM_CONDITIONAL(HAVE_EGL_DRIVER_GLX, test "x$HAVE_EGL_DRIVER_GLX" != "x")

AC_SUBST([EGL_NATIVE_PLATFORM])
AC_SUBST([EGL_PLATFORMS])
AC_SUBST([EGL_CFLAGS])

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

AC_ARG_WITH([llvm-shared-libs],
    [AS_HELP_STRING([--with-llvm-shared-libs],
        [link with LLVM shared libraries @<:@default=disabled@:>@])],
    [with_llvm_shared_libs=yes],
    [with_llvm_shared_libs=no])

AC_ARG_WITH([llvm-prefix],
    [AS_HELP_STRING([--with-llvm-prefix],
        [Prefix for LLVM installations in non-standard locations])],
    [llvm_prefix="$withval"],
    [llvm_prefix=""])


# Call this inside ` ` to get the return value.
# $1 is the llvm-config command with arguments.
strip_unwanted_llvm_flags() {
    # Use \> (marks the end of the word)
    echo `$1` | sed \
	-e 's/-DNDEBUG\>//g' \
	-e 's/-pedantic\>//g' \
	-e 's/-Wcovered-switch-default\>//g' \
	-e 's/-O.\>//g' \
	-e 's/-g\>//g' \
	-e 's/-Wall\>//g' \
	-e 's/-fomit-frame-pointer\>//g'
}


if test "x$with_gallium_drivers" = x; then
    enable_gallium_llvm=no
fi
if test "x$enable_gallium_llvm" = xauto; then
    case "$host_cpu" in
    i*86|x86_64) enable_gallium_llvm=yes;;
    esac
fi
if test "x$enable_gallium_llvm" = xyes; then
    if test "x$llvm_prefix" != x; then
        AC_PATH_PROG([LLVM_CONFIG], [llvm-config], [no], ["$llvm_prefix/bin"])
    else
        AC_PATH_PROG([LLVM_CONFIG], [llvm-config], [no])
    fi

    if test "x$LLVM_CONFIG" != xno; then
	LLVM_VERSION=`$LLVM_CONFIG --version | sed 's/svn.*//g'`
	LLVM_VERSION_INT=`echo $LLVM_VERSION | sed -e 's/\([[0-9]]\)\.\([[0-9]]\)/\10\2/g'`
        if test "x$with_llvm_shared_libs" != xyes; then
            LLVM_COMPONENTS="engine bitwriter"
            if $LLVM_CONFIG --components | grep -q '\<mcjit\>'; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} mcjit"
            fi

            if test "x$enable_opencl" = xyes; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} ipo linker instrumentation"
            fi
	fi
	LLVM_LDFLAGS=`$LLVM_CONFIG --ldflags`
	LLVM_BINDIR=`$LLVM_CONFIG --bindir`
	LLVM_CPPFLAGS=`strip_unwanted_llvm_flags "$LLVM_CONFIG --cppflags"`
	LLVM_CFLAGS=$LLVM_CPPFLAGS   # CPPFLAGS seem to be sufficient
	LLVM_CXXFLAGS=`strip_unwanted_llvm_flags "$LLVM_CONFIG --cxxflags"`
	LLVM_INCLUDEDIR=`$LLVM_CONFIG --includedir`
	LLVM_LIBDIR=`$LLVM_CONFIG --libdir`
	DEFINES="${DEFINES} -DHAVE_LLVM=0x0$LLVM_VERSION_INT"
	MESA_LLVM=1

	dnl Check for Clang interanl headers
        if test "x$enable_opencl" = xyes; then
            if test "x$CLANG_LIBDIR" = x; then
                CLANG_LIBDIR=${LLVM_LIBDIR}
            fi
            CLANG_RESOURCE_DIR=$CLANG_LIBDIR/clang/${LLVM_VERSION}
            AC_CHECK_FILE("$CLANG_RESOURCE_DIR/include/stddef.h",,
                AC_MSG_ERROR([Could not find clang internal header stddef.h in $CLANG_RESOURCE_DIR Use --with-clang-libdir to specify the correct path to the clang libraries.]))
        fi
    else
	MESA_LLVM=0
	LLVM_VERSION_INT=0
    fi
else
    MESA_LLVM=0
    LLVM_VERSION_INT=0
fi

dnl Directory for XVMC libs
AC_ARG_WITH([xvmc-libdir],
    [AS_HELP_STRING([--with-xvmc-libdir=DIR],
        [directory for the XVMC libraries @<:@default=${libdir}@:>@])],
    [XVMC_LIB_INSTALL_DIR="$withval"],
    [XVMC_LIB_INSTALL_DIR='${libdir}'])
AC_SUBST([XVMC_LIB_INSTALL_DIR])

dnl
dnl Gallium Tests
dnl
if test "x$enable_gallium_tests" = xyes; then
    SRC_DIRS="$SRC_DIRS gallium/tests/trivial gallium/tests/unit"
    enable_gallium_loader=yes
fi

dnl Directory for VDPAU libs
AC_ARG_WITH([vdpau-libdir],
    [AS_HELP_STRING([--with-vdpau-libdir=DIR],
        [directory for the VDPAU libraries @<:@default=${libdir}/vdpau@:>@])],
    [VDPAU_LIB_INSTALL_DIR="$withval"],
    [VDPAU_LIB_INSTALL_DIR='${libdir}/vdpau'])
AC_SUBST([VDPAU_LIB_INSTALL_DIR])

dnl Directory for OpenCL libs
AC_ARG_WITH([opencl-libdir],
    [AS_HELP_STRING([--with-opencl-libdir=DIR],
        [directory for the OpenCL libraries @<:@default=${libdir}/opencl@:>@])],
    [OPENCL_LIB_INSTALL_DIR="$withval"],
    [OPENCL_LIB_INSTALL_DIR='${libdir}/opencl'])
AC_SUBST([OPENCL_LIB_INSTALL_DIR])

dnl
dnl Gallium helper functions
dnl
gallium_check_st() {
    if test "x$HAVE_ST_DRI" = xyes || test "x$HAVE_ST_XORG" = xyes ||
	 test "x$HAVE_ST_XA" = xyes || test "x$HAVE_ST_XVMC" = xyes ||
	 test "x$HAVE_ST_VDPAU" = xyes; then
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
    fi
    if test "x$HAVE_ST_VDPAU" = xyes && test "x$6" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $6"
    fi
}

gallium_require_llvm() {
    if test "x$MESA_LLVM" = x0; then
        case "$host_cpu" in
        i*86|x86_64) AC_MSG_ERROR([LLVM is required to build $1 on x86 and x86_64]);;
        esac
    fi
}

gallium_require_drm_loader() {
    if test "x$enable_gallium_loader" = xyes; then
        PKG_CHECK_MODULES([LIBUDEV], [libudev], [],
                          AC_MSG_ERROR([Gallium drm loader requires libudev]))
        if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([Gallium drm loader requires libdrm >= $LIBDRM_REQUIRED])
        fi
        enable_gallium_drm_loader=yes
    fi
}

radeon_llvm_check() {
    LLVM_REQUIRED_VERSION_MAJOR="3"
    LLVM_REQUIRED_VERSION_MINOR="2"
    if test "$LLVM_VERSION_INT" -lt "${LLVM_REQUIRED_VERSION_MAJOR}0${LLVM_REQUIRED_VERSION_MINOR}"; then
        AC_MSG_ERROR([LLVM $LLVM_REQUIRED_VERSION_MAJOR.$LLVM_REQUIRED_VERSION_MINOR or newer with R600 target enabled is required.
                     To use the r600/radeonsi LLVM backend, you need to fetch the LLVM source from:
                     git://people.freedesktop.org/~tstellar/llvm master
                     and build with --enable-experimental-targets=R600])
    fi
    if test true && $LLVM_CONFIG --targets-built | grep -qv '\<R600\>' ; then
        AC_MSG_ERROR([LLVM R600 Target not enabled.  You can enable it when building the LLVM
                      sources with the --enable-experimental-targets=R600
                      configure flag])
    fi
    AC_MSG_WARN([Please ensure you use the latest llvm tree from git://people.freedesktop.org/~tstellar/llvm master before submitting a bug])
    LLVM_COMPONENTS="${LLVM_COMPONENTS} r600"
}

dnl Gallium drivers
dnl Duplicates in GALLIUM_DRIVERS_DIRS are removed by sorting it after this block
if test "x$with_gallium_drivers" != x; then
    gallium_drivers=`IFS=', '; echo $with_gallium_drivers`
    for driver in $gallium_drivers; do
        case "x$driver" in
        xsvga)
            HAVE_GALLIUM_SVGA=yes
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS svga softpipe"
            gallium_check_st "svga/drm" "dri-vmwgfx" "" "xa-vmwgfx"
            ;;
        xi915)
            HAVE_GALLIUM_I915=yes
            PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS i915 softpipe"
            if test "x$MESA_LLVM" = x1; then
                GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS llvmpipe"
            fi
            GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS i915/sw"
            gallium_check_st "i915/drm" "dri-i915" "xorg-i915"
            ;;
        xr300)
            HAVE_GALLIUM_R300=yes
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            gallium_require_llvm "Gallium R300"
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r300"
            gallium_check_st "radeon/drm" "dri-r300" "" "" "xvmc-r300" "vdpau-r300"
            ;;
        xr600)
            HAVE_GALLIUM_R600=yes
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r600"
            if test "x$enable_r600_llvm" = xyes -o "x$enable_opencl" = xyes; then
                radeon_llvm_check
                NEED_RADEON_GALLIUM=yes;
                R600_NEED_RADEON_GALLIUM=yes;
                LLVM_COMPONENTS="${LLVM_COMPONENTS} ipo bitreader asmparser"
            fi
            if test "x$enable_r600_llvm" = xyes; then
                USE_R600_LLVM_COMPILER=yes;
            fi
            gallium_check_st "radeon/drm" "dri-r600" "xorg-r600" "" "xvmc-r600" "vdpau-r600"
            ;;
        xradeonsi)
            HAVE_GALLIUM_RADEONSI=yes
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS radeonsi"
            radeon_llvm_check
	    NEED_RADEON_GALLIUM=yes;
            gallium_check_st "radeon/drm" "dri-radeonsi" "xorg-radeonsi" "" "" "vdpau-radeonsi" ""
            ;;
        xnouveau)
            HAVE_GALLIUM_NOUVEAU=yes
            PKG_CHECK_MODULES([NOUVEAU], [libdrm_nouveau >= $LIBDRM_NOUVEAU_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS nouveau nv30 nv50 nvc0"
            gallium_check_st "nouveau/drm" "dri-nouveau" "xorg-nouveau" "" "xvmc-nouveau" "vdpau-nouveau"
            ;;
        xswrast)
            HAVE_GALLIUM_SOFTPIPE=yes
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS softpipe"
            if test "x$MESA_LLVM" = x1; then
                HAVE_GALLIUM_LLVMPIPE=yes
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
            if test "x$HAVE_ST_VDPAU" = xyes ||
               test "x$HAVE_ST_XVMC" = xyes; then
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

dnl Set LLVM_LIBS - This is done after the driver configuration so
dnl that drivers can add additonal components to LLVM_COMPONENTS.
dnl Previously, gallium drivers were updating LLVM_LIBS directly
dnl by calling llvm-config --libs ${DRIVER_LLVM_COMPONENTS}, but
dnl this was causing the same libraries to be appear multiple times
dnl in LLVM_LIBS.

if test "x$MESA_LLVM" != x0; then

    LLVM_LIBS="`$LLVM_CONFIG --libs ${LLVM_COMPONENTS}`"

    if test "x$with_llvm_shared_libs" = xyes; then
        dnl We can't use $LLVM_VERSION because it has 'svn' stripped out,
        LLVM_SO_NAME=LLVM-`$LLVM_CONFIG --version`
        AC_CHECK_FILE("$LLVM_LIBDIR/lib$LLVM_SO_NAME.so", llvm_have_one_so=yes,)

        if test "x$llvm_have_one_so" = xyes; then
            dnl LLVM was built using auto*, so there is only one shared object.
            LLVM_LIBS="-l$LLVM_SO_NAME"
        else
            dnl If LLVM was built with CMake, there will be one shared object per
            dnl component.
            AC_CHECK_FILE("$LLVM_LIBDIR/libLLVMTarget.so",,
                    AC_MSG_ERROR([Could not find llvm shared libraries:
	Please make sure you have built llvm with the --enable-shared option
	and that your llvm libraries are installed in $LLVM_LIBDIR
	If you have installed your llvm libraries to a different directory you
	can use the --with-llvm-prefix= configure flag to specify this directory.
	NOTE: Mesa is attempting to use llvm shared libraries because you have
	passed one of the following options to configure:
		--with-llvm-shared-libs
		--enable-opencl
	If you do not want to build with llvm shared libraries and instead want to
	use llvm static libraries then remove these options from your configure
	invocation and reconfigure.]))

           dnl We don't need to update LLVM_LIBS in this case because the LLVM
           dnl install uses a shared object for each compoenent and we have
           dnl already added all of these objects to LLVM_LIBS.
        fi
    fi
fi

AM_CONDITIONAL(HAVE_GALLIUM_SVGA, test "x$HAVE_GALLIUM_SVGA" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_I915, test "x$HAVE_GALLIUM_I915" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_R300, test "x$HAVE_GALLIUM_R300" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_R600, test "x$HAVE_GALLIUM_R600" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_RADEONSI, test "x$HAVE_GALLIUM_RADEONSI" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_NOUVEAU, test "x$HAVE_GALLIUM_NOUVEAU" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_SOFTPIPE, test "x$HAVE_GALLIUM_SOFTPIPE" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_LLVMPIPE, test "x$HAVE_GALLIUM_LLVMPIPE" = xyes)

if test "x$enable_gallium_loader" = xyes; then
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/null"
    GALLIUM_PIPE_LOADER_DEFINES="-DHAVE_PIPE_LOADER_SW"
    GALLIUM_PIPE_LOADER_LIBS="\$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la"
    GALLIUM_PIPE_LOADER_LIBS="$GALLIUM_PIPE_LOADER_LIBS \$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la"

    if test "x$HAVE_WINSYS_XLIB" = xyes; then
        GALLIUM_PIPE_LOADER_DEFINES="$GALLIUM_PIPE_LOADER_DEFINES -DHAVE_PIPE_LOADER_XLIB"
        GALLIUM_PIPE_LOADER_LIBS="$GALLIUM_PIPE_LOADER_LIBS \$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la"
    fi

    if test "x$enable_gallium_drm_loader" = xyes; then
        GALLIUM_PIPE_LOADER_DEFINES="$GALLIUM_PIPE_LOADER_DEFINES -DHAVE_PIPE_LOADER_DRM"
        PKG_CHECK_MODULES([GALLIUM_PIPE_LOADER_XCB], [xcb xcb-dri2],
                          pipe_loader_have_xcb=yes, pipe_loader_have_xcb=no)
        if test "x$pipe_loader_have_xcb" = xyes; then
            GALLIUM_PIPE_LOADER_DEFINES="$GALLIUM_PIPE_LOADER_DEFINES -DPIPE_LOADER_HAVE_XCB"
            GALLIUM_PIPE_LOADER_LIBS="$GALLIUM_PIPE_LOADER_LIBS $GALLIUM_PIPE_LOADER_XCB_LIBS $LIBDRM_LIBS"
        fi
    fi

    AC_SUBST([GALLIUM_PIPE_LOADER_DEFINES])
    AC_SUBST([GALLIUM_PIPE_LOADER_LIBS])
fi

dnl Tell Automake which drivers to build
for driver in $GALLIUM_DRIVERS_DIRS; do
    case "x$driver" in
        xgalahad)
            HAVE_GALAHAD_GALLIUM=yes;
	    ;;
	xidentity)
	    HAVE_IDENTITY_GALLIUM=yes;
	    ;;
	xnoop)
	    HAVE_NOOP_GALLIUM=yes;
	    ;;
        *)
            GALLIUM_MAKE_DIRS="$GALLIUM_MAKE_DIRS $driver"
	    ;;
    esac
done

AM_CONDITIONAL(HAVE_GALAHAD_GALLIUM, test x$HAVE_GALAHAD_GALLIUM = xyes)
AM_CONDITIONAL(HAVE_IDENTITY_GALLIUM, test x$HAVE_IDENTITY_GALLIUM = xyes)
AM_CONDITIONAL(HAVE_NOOP_GALLIUM, test x$HAVE_NOOP_GALLIUM = xyes)
AM_CONDITIONAL(NEED_RADEON_GALLIUM, test x$NEED_RADEON_GALLIUM = xyes)
AM_CONDITIONAL(R600_NEED_RADEON_GALLIUM, test x$R600_NEED_RADEON_GALLIUM = xyes)
AM_CONDITIONAL(USE_R600_LLVM_COMPILER, test x$USE_R600_LLVM_COMPILER = xyes)
AM_CONDITIONAL(HAVE_LOADER_GALLIUM, test x$enable_gallium_loader = xyes)
AM_CONDITIONAL(HAVE_DRM_LOADER_GALLIUM, test x$enable_gallium_drm_loader = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_COMPUTE, test x$enable_opencl = xyes)
AM_CONDITIONAL(HAVE_MESA_LLVM, test x$MESA_LLVM = x1)
AM_CONDITIONAL(LLVM_NEEDS_FNORTTI, test $LLVM_VERSION_INT -ge 302)

AC_SUBST([GALLIUM_MAKE_DIRS])

AM_CONDITIONAL(NEED_LIBPROGRAM, test "x$with_gallium_drivers" != x -o \
                                     "x$enable_xlib_glx" = xyes -o \
                                     "x$enable_osmesa" = xyes)
AM_CONDITIONAL(HAVE_X11_DRIVER, echo "$DRIVER_DIRS" | grep 'x11' >/dev/null 2>&1)

AM_CONDITIONAL(HAVE_X86_ASM, echo "$DEFINES" | grep 'X86_ASM' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_X86_64_ASM, echo "$DEFINES" | grep 'X86_64_ASM' >/dev/null 2>&1)
AM_CONDITIONAL(HAVE_SPARC_ASM, echo "$DEFINES" | grep 'SPARC_ASM' >/dev/null 2>&1)

AM_CONDITIONAL(CROSS_COMPILING, test "x$cross_compiling" = xyes)

AC_SUBST([VDPAU_MAJOR], 1)
AC_SUBST([VDPAU_MINOR], 0)

AC_SUBST([XVMC_MAJOR], 1)
AC_SUBST([XVMC_MINOR], 0)

AC_SUBST([XA_MAJOR], 1)
AC_SUBST([XA_MINOR], 0)
AC_SUBST([XA_TINY], 0)
AC_SUBST([XA_VERSION], "$XA_MAJOR.$XA_MINOR.$XA_TINY")

dnl prepend CORE_DIRS to SRC_DIRS
SRC_DIRS="$CORE_DIRS $SRC_DIRS"

dnl Restore LDFLAGS and CPPFLAGS
LDFLAGS="$_SAVE_LDFLAGS"
CPPFLAGS="$_SAVE_CPPFLAGS"

dnl Add user CFLAGS and CXXFLAGS
CFLAGS="$CFLAGS $USER_CFLAGS"
CXXFLAGS="$CXXFLAGS $USER_CXXFLAGS"

dnl Substitute the config
AC_CONFIG_FILES([Makefile
		src/Makefile
		src/egl/Makefile
		src/egl/drivers/Makefile
		src/egl/drivers/dri2/Makefile
		src/egl/drivers/glx/Makefile
		src/egl/main/Makefile
		src/egl/main/egl.pc
		src/egl/wayland/Makefile
		src/egl/wayland/wayland-drm/Makefile
		src/egl/wayland/wayland-egl/Makefile
		src/egl/wayland/wayland-egl/wayland-egl.pc
		src/gallium/Makefile
		src/gallium/auxiliary/Makefile
		src/gallium/auxiliary/pipe-loader/Makefile
		src/gallium/drivers/Makefile
		src/gallium/drivers/i915/Makefile
		src/gallium/drivers/llvmpipe/Makefile
		src/gallium/drivers/nouveau/Makefile
		src/gallium/drivers/nv30/Makefile
		src/gallium/drivers/nv50/Makefile
		src/gallium/drivers/nvc0/Makefile
		src/gallium/drivers/r300/Makefile
		src/gallium/drivers/r600/Makefile
		src/gallium/drivers/radeon/Makefile
		src/gallium/drivers/radeonsi/Makefile
		src/gallium/drivers/rbug/Makefile
		src/gallium/drivers/softpipe/Makefile
		src/gallium/drivers/svga/Makefile
		src/gallium/drivers/trace/Makefile
		src/gallium/state_trackers/Makefile
		src/gallium/state_trackers/clover/Makefile
		src/gallium/state_trackers/dri/Makefile
		src/gallium/state_trackers/dri/drm/Makefile
		src/gallium/state_trackers/dri/sw/Makefile
		src/gallium/state_trackers/egl/Makefile
		src/gallium/state_trackers/gbm/Makefile
		src/gallium/state_trackers/glx/Makefile
		src/gallium/state_trackers/vdpau/Makefile
		src/gallium/state_trackers/vega/Makefile
		src/gallium/state_trackers/xa/Makefile
		src/gallium/state_trackers/xorg/Makefile
		src/gallium/state_trackers/xvmc/Makefile
		src/gallium/targets/Makefile
		src/gallium/targets/dri-i915/Makefile
		src/gallium/targets/dri-nouveau/Makefile
		src/gallium/targets/dri-r300/Makefile
		src/gallium/targets/dri-r600/Makefile
		src/gallium/targets/dri-radeonsi/Makefile
		src/gallium/targets/dri-swrast/Makefile
		src/gallium/targets/dri-vmwgfx/Makefile
		src/gallium/targets/egl-static/Makefile
		src/gallium/targets/gbm/Makefile
		src/gallium/targets/opencl/Makefile
		src/gallium/targets/pipe-loader/Makefile
		src/gallium/targets/libgl-xlib/Makefile
		src/gallium/targets/vdpau-nouveau/Makefile
		src/gallium/targets/vdpau-r300/Makefile
		src/gallium/targets/vdpau-r600/Makefile
		src/gallium/targets/vdpau-radeonsi/Makefile
		src/gallium/targets/vdpau-softpipe/Makefile
		src/gallium/targets/xa-vmwgfx/Makefile
		src/gallium/targets/xa-vmwgfx/xatracker.pc
		src/gallium/targets/xorg-i915/Makefile
		src/gallium/targets/xorg-nouveau/Makefile
		src/gallium/targets/xorg-r600/Makefile
		src/gallium/targets/xorg-radeonsi/Makefile
		src/gallium/targets/xvmc-nouveau/Makefile
		src/gallium/targets/xvmc-r300/Makefile
		src/gallium/targets/xvmc-r600/Makefile
		src/gallium/targets/xvmc-softpipe/Makefile
		src/gallium/tests/trivial/Makefile
		src/gallium/tests/unit/Makefile
		src/gallium/winsys/Makefile
		src/gallium/winsys/i915/drm/Makefile
		src/gallium/winsys/i915/sw/Makefile
		src/gallium/winsys/nouveau/drm/Makefile
		src/gallium/winsys/radeon/drm/Makefile
		src/gallium/winsys/svga/drm/Makefile
		src/gallium/winsys/sw/Makefile
		src/gallium/winsys/sw/dri/Makefile
		src/gallium/winsys/sw/fbdev/Makefile
		src/gallium/winsys/sw/null/Makefile
		src/gallium/winsys/sw/wayland/Makefile
		src/gallium/winsys/sw/wrapper/Makefile
		src/gallium/winsys/sw/xlib/Makefile
		src/gbm/Makefile
		src/gbm/main/gbm.pc
		src/glsl/Makefile
		src/glsl/builtin_compiler/Makefile
		src/glx/Makefile
		src/glx/tests/Makefile
		src/gtest/Makefile
		src/mapi/es1api/Makefile
		src/mapi/es1api/glesv1_cm.pc
		src/mapi/es2api/Makefile
		src/mapi/es2api/glesv2.pc
		src/mapi/glapi/Makefile
		src/mapi/glapi/gen/Makefile
		src/mapi/glapi/tests/Makefile
		src/mapi/shared-glapi/Makefile
		src/mapi/shared-glapi/tests/Makefile
		src/mapi/vgapi/Makefile
		src/mapi/vgapi/vg.pc
		src/mesa/Makefile
		src/mesa/gl.pc
		src/mesa/drivers/Makefile
		src/mesa/drivers/dri/dri.pc
		src/mesa/drivers/dri/common/Makefile
		src/mesa/drivers/dri/common/xmlpool/Makefile
		src/mesa/drivers/dri/i915/Makefile
		src/mesa/drivers/dri/i965/Makefile
		src/mesa/drivers/dri/Makefile
		src/mesa/drivers/dri/nouveau/Makefile
		src/mesa/drivers/dri/r200/Makefile
		src/mesa/drivers/dri/radeon/Makefile
		src/mesa/drivers/dri/swrast/Makefile
		src/mesa/drivers/osmesa/Makefile
		src/mesa/drivers/osmesa/osmesa.pc
		src/mesa/drivers/x11/Makefile
		src/mesa/libdricore/Makefile
		src/mesa/main/tests/Makefile
		src/mesa/main/tests/hash_table/Makefile
		src/mesa/program/Makefile
		src/mesa/x86-64/Makefile
		src/mesa/x86/Makefile])

dnl Sort the dirs alphabetically
GALLIUM_TARGET_DIRS=`echo $GALLIUM_TARGET_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
GALLIUM_WINSYS_DIRS=`echo $GALLIUM_WINSYS_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
GALLIUM_DRIVERS_DIRS=`echo $GALLIUM_DRIVERS_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
GALLIUM_MAKE_DIRS=`echo $GALLIUM_MAKE_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
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

dnl EGL
echo ""
echo "        EGL:             $enable_egl"
if test "$enable_egl" = yes; then
    echo "        EGL platforms:   $EGL_PLATFORMS"

    egl_drivers=""
    if test "x$HAVE_EGL_DRIVER_GLX" != "x"; then
        egl_drivers="$egl_drivers builtin:egl_glx"
    fi
    if test "x$HAVE_EGL_DRIVER_DRI2" != "x"; then
        egl_drivers="$egl_drivers builtin:egl_dri2"
    fi

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
echo "        Shared-glapi:    $enable_shared_glapi"

dnl Compiler options
# cleanup the CFLAGS/CXXFLAGS/DEFINES vars
cflags=`echo $CFLAGS | \
    $SED 's/^ *//;s/  */ /;s/ *$//'`
cxxflags=`echo $CXXFLAGS | \
    $SED 's/^ *//;s/  */ /;s/ *$//'`
defines=`echo $DEFINES | $SED 's/^ *//;s/  */ /;s/ *$//'`
echo ""
echo "        CFLAGS:          $cflags"
echo "        CXXFLAGS:        $cxxflags"
echo "        Macros:          $defines"
echo ""
if test "x$MESA_LLVM" = x1; then
    echo "        LLVM_CFLAGS:     $LLVM_CFLAGS"
    echo "        LLVM_CXXFLAGS:   $LLVM_CXXFLAGS"
    echo "        LLVM_CPPFLAGS:   $LLVM_CPPFLAGS"
    echo ""
fi
echo "        PYTHON2:         $PYTHON2"

echo ""
echo "        Run '${MAKE-make}' to build Mesa"
echo ""
