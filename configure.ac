dnl Process this file with autoconf to create configure.

AC_PREREQ([2.60])

dnl Tell the user about autoconf.html in the --help output
m4_divert_once([HELP_END], [
See docs/autoconf.html for more details on the options for Mesa.])

m4_define(MESA_VERSION, m4_normalize(m4_include(VERSION)))
AC_INIT([Mesa], [MESA_VERSION],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa])
AC_CONFIG_AUX_DIR([bin])
AC_CONFIG_MACRO_DIR([m4])
AC_CANONICAL_SYSTEM
AM_INIT_AUTOMAKE([foreign])

# Support silent build rules, requires at least automake-1.11. Disable
# by either passing --disable-silent-rules to configure or passing V=1
# to make
m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])],
    [AC_SUBST([AM_DEFAULT_VERBOSITY], [1])])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

dnl Set internal versions
OSMESA_VERSION=8
AC_SUBST([OSMESA_VERSION])

dnl Versions for external dependencies
LIBDRM_REQUIRED=2.4.38
LIBDRM_RADEON_REQUIRED=2.4.56
LIBDRM_INTEL_REQUIRED=2.4.52
LIBDRM_NVVIEUX_REQUIRED=2.4.33
LIBDRM_NOUVEAU_REQUIRED="2.4.33 libdrm >= 2.4.41"
LIBDRM_FREEDRENO_REQUIRED=2.4.55
DRI2PROTO_REQUIRED=2.6
DRI3PROTO_REQUIRED=1.0
PRESENTPROTO_REQUIRED=1.0
LIBUDEV_REQUIRED=151
GLPROTO_REQUIRED=1.4.14
LIBOMXIL_BELLAGIO_REQUIRED=0.0
VDPAU_REQUIRED=0.4.1
WAYLAND_REQUIRED=1.2.0
XCB_REQUIRED=1.9.3
XCBDRI2_REQUIRED=1.8
XCBGLX_REQUIRED=1.8.1
XSHMFENCE_REQUIRED=1.1
XVMC_REQUIRED=1.0.6

dnl Check for progs
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AM_PROG_CC_C_O
AM_PROG_AS
AC_CHECK_PROGS([MAKE], [gmake make])
AC_CHECK_PROGS([PYTHON2], [python2 python])
AC_PROG_SED
AC_PROG_MKDIR_P

AC_SYS_LARGEFILE

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
GEN_ASM_OFFSETS=no
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

    if test "x$cross_compiling" = xyes; then
        GEN_ASM_OFFSETS=yes
    fi
fi

dnl Check for compiler builtins
AX_GCC_BUILTIN([__builtin_bswap32])
AX_GCC_BUILTIN([__builtin_bswap64])

AM_CONDITIONAL([GEN_ASM_OFFSETS], test "x$GEN_ASM_OFFSETS" = xyes)

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

dnl Compiler macros
DEFINES="-DUSE_EXTERNAL_DXTN_LIB=1"
AC_SUBST([DEFINES])
case "$host_os" in
linux*|*-gnu*|gnu*)
    DEFINES="$DEFINES -D_GNU_SOURCE -DHAVE_PTHREAD"
    ;;
solaris*)
    DEFINES="$DEFINES -DHAVE_PTHREAD -DSVR4"
    ;;
cygwin*)
    DEFINES="$DEFINES -D_XOPEN_SOURCE=700 -DHAVE_PTHREAD"
    ;;
esac

dnl Add flags for gcc and g++
if test "x$GCC" = xyes; then
    CFLAGS="$CFLAGS -Wall -std=c99"

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

dnl
dnl Optional flags, check for compiler support
dnl
AX_CHECK_COMPILE_FLAG([-msse4.1], [SSE41_SUPPORTED=1], [SSE41_SUPPORTED=0])
if test "x$SSE41_SUPPORTED" = x1; then
    DEFINES="$DEFINES -DUSE_SSE41"
fi
AM_CONDITIONAL([SSE41_SUPPORTED], [test x$SSE41_SUPPORTED = x1])

dnl Can't have static and shared libraries, default to static if user
dnl explicitly requested. If both disabled, set to static since shared
dnl was explicitly requested.
case "x$enable_static$enable_shared" in
xyesyes)
    AC_MSG_WARN([Cannot build static and shared libraries, disabling shared])
    enable_shared=no
    ;;
xnono)
    AC_MSG_WARN([Cannot disable both static and shared libraries, enabling shared])
    enable_shared=yes
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
    DEFINES="$DEFINES -DDEBUG"
    if test "x$GCC" = xyes; then
        if ! echo "$CFLAGS" | grep -q -e '-g'; then
            CFLAGS="$CFLAGS -g"
        fi
        if ! echo "$CFLAGS" | grep -q -e '-O'; then
            CFLAGS="$CFLAGS -O0"
        fi
    fi
    if test "x$GXX" = xyes; then
        if ! echo "$CXXFLAGS" | grep -q -e '-g'; then
            CXXFLAGS="$CXXFLAGS -g"
        fi
        if ! echo "$CXXFLAGS" | grep -q -e '-O'; then
            CXXFLAGS="$CXXFLAGS -O0"
        fi
    fi
fi

dnl
dnl Check if linker supports -Bsymbolic
dnl
save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -Wl,-Bsymbolic"
AC_MSG_CHECKING([if ld supports -Bsymbolic])
AC_LINK_IFELSE(
    [AC_LANG_SOURCE([int main() { return 0;}])],
    [AC_MSG_RESULT([yes])
        BSYMBOLIC="-Wl,-Bsymbolic";],
    [AC_MSG_RESULT([no])
        BSYMBOLIC="";])
LDFLAGS=$save_LDFLAGS

AC_SUBST([BSYMBOLIC])

dnl
dnl Check if linker supports garbage collection
dnl
save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -Wl,--gc-sections"
AC_MSG_CHECKING([whether ld supports --gc-sections])
AC_LINK_IFELSE(
    [AC_LANG_SOURCE([static char UnusedFunc() { return 5; } int main() { return 0;}])],
    [AC_MSG_RESULT([yes])
        GC_SECTIONS="-Wl,--gc-sections";],
    [AC_MSG_RESULT([no])
        GC_SECTIONS="";])
LDFLAGS=$save_LDFLAGS

AC_SUBST([GC_SECTIONS])

dnl
dnl OpenBSD does not have DT_NEEDED entries for libc by design
dnl so when these flags are passed to ld via libtool the checks will fail
dnl
case "$host_os" in
openbsd* | darwin* )
    LD_NO_UNDEFINED="" ;;
*)
    LD_NO_UNDEFINED="-Wl,--no-undefined" ;;
esac

AC_SUBST([LD_NO_UNDEFINED])

dnl
dnl Check if linker supports version scripts
dnl
AC_MSG_CHECKING([if the linker supports version-scripts])
save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -Wl,--version-script=conftest.map"
cat > conftest.map <<EOF
VERSION_1 {
     global:
         main;

     local:
         *;
};
EOF
AC_LINK_IFELSE(
    [AC_LANG_SOURCE([int main() { return 0;}])],
    [have_ld_version_script=yes;AC_MSG_RESULT(yes)],
    [have_ld_version_script=no; AC_MSG_RESULT(no)])
LDFLAGS=$save_LDFLAGS
AM_CONDITIONAL(HAVE_LD_VERSION_SCRIPT, test "$have_ld_version_script" = "yes")

dnl
dnl Check if linker supports dynamic list files
dnl
AC_MSG_CHECKING([if the linker supports --dynamic-list])
save_LDFLAGS=$LDFLAGS
LDFLAGS="$LDFLAGS -Wl,--dynamic-list=conftest.dyn"
cat > conftest.dyn <<EOF
{
	radeon_drm_winsys_create;
};
EOF
AC_LINK_IFELSE(
    [AC_LANG_SOURCE([int main() { return 0;}])],
    [have_ld_dynamic_list=yes;AC_MSG_RESULT(yes)],
    [have_ld_dynamic_list=no; AC_MSG_RESULT(no)])
LDFLAGS=$save_LDFLAGS
AM_CONDITIONAL(HAVE_LD_DYNAMIC_LIST, test "$have_ld_dynamic_list" = "yes")

dnl
dnl compatibility symlinks
dnl
case "$host_os" in
linux* )
    HAVE_COMPAT_SYMLINKS=yes ;;
* )
    HAVE_COMPAT_SYMLINKS=no ;;
esac

AM_CONDITIONAL(HAVE_COMPAT_SYMLINKS, test "x$HAVE_COMPAT_SYMLINKS" = xyes)

dnl
dnl library names
dnl
case "$host_os" in
darwin* )
    LIB_EXT='dylib' ;;
cygwin* )
    LIB_EXT='dll' ;;
aix* )
    LIB_EXT='a' ;;
* )
    LIB_EXT='so' ;;
esac

AC_SUBST([LIB_EXT])

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
AC_MSG_CHECKING([whether to enable assembly])
test "x$enable_asm" = xno && AC_MSG_RESULT([no])
# disable if cross compiling on x86/x86_64 since we must run gen_matypes
if test "x$enable_asm" = xyes && test "x$cross_compiling" = xyes; then
    case "$host_cpu" in
    i?86 | x86_64 | amd64)
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
        linux* | *freebsd* | dragonfly* | *netbsd* | openbsd* | gnu*)
            asm_arch=x86
            ;;
        esac
        ;;
    x86_64|amd64)
        case "$host_os" in
        linux* | *freebsd* | dragonfly* | *netbsd* | openbsd*)
            asm_arch=x86_64
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
        AC_MSG_RESULT([yes, x86])
        ;;
    x86_64|amd64)
        DEFINES="$DEFINES -DUSE_X86_64_ASM"
        AC_MSG_RESULT([yes, x86_64])
        ;;
    sparc)
        DEFINES="$DEFINES -DUSE_SPARC_ASM"
        AC_MSG_RESULT([yes, sparc])
        ;;
    *)
        AC_MSG_RESULT([no, platform not supported])
        ;;
    esac
fi

dnl Check to see if dlopen is in default libraries (like Solaris, which
dnl has it in libc), or if libdl is needed to get it.
AC_CHECK_FUNC([dlopen], [DEFINES="$DEFINES -DHAVE_DLOPEN"],
    [AC_CHECK_LIB([dl], [dlopen],
       [DEFINES="$DEFINES -DHAVE_DLOPEN"; DLOPEN_LIBS="-ldl"])])
AC_SUBST([DLOPEN_LIBS])

dnl Check if that library also has dladdr
save_LIBS="$LIBS"
LIBS="$LIBS $DLOPEN_LIBS"
AC_CHECK_FUNCS([dladdr])
LIBS="$save_LIBS"

case "$host_os" in
darwin*|mingw*)
    ;;
*)
    AC_CHECK_FUNCS([clock_gettime], [CLOCK_LIB=],
                   [AC_CHECK_LIB([rt], [clock_gettime], [CLOCK_LIB=-lrt],
                                 [AC_MSG_ERROR([Could not find clock_gettime])])])
    AC_SUBST([CLOCK_LIB])
    ;;
esac

dnl See if posix_memalign is available
AC_CHECK_FUNC([posix_memalign], [DEFINES="$DEFINES -DHAVE_POSIX_MEMALIGN"])

dnl Check for pthreads
case "$host_os" in
mingw*)
    ;;
*)
    AX_PTHREAD
    ;;
esac
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
    PKG_CHECK_MODULES([SELINUX], [libselinux], [],
        [AC_CHECK_HEADER([selinux/selinux.h],[],
                         [AC_MSG_ERROR([SELinux headers not found])])
         AC_CHECK_LIB([selinux],[is_selinux_enabled],[],
                      [AC_MSG_ERROR([SELinux library not found])])
         SELINUX_LIBS="-lselinux"])
    DEFINES="$DEFINES -DMESA_SELINUX"
fi
AC_SUBST([SELINUX_CFLAGS])
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

case "$host_os" in
linux*)
    dri3_default=yes
    ;;
*)
    dri3_default=no
    ;;
esac
AC_ARG_ENABLE([dri3],
    [AS_HELP_STRING([--enable-dri3],
        [enable DRI3 @<:@default=auto@:>@])],
    [enable_dri3="$enableval"],
    [enable_dri3="$dri3_default"])
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
AC_ARG_ENABLE([gallium-osmesa],
    [AS_HELP_STRING([--enable-gallium-osmesa],
        [enable Gallium implementation of the OSMesa library @<:@default=disabled@:>@])],
    [enable_gallium_osmesa="$enableval"],
    [enable_gallium_osmesa=no])
AC_ARG_ENABLE([egl],
    [AS_HELP_STRING([--disable-egl],
        [disable EGL library @<:@default=enabled@:>@])],
    [enable_egl="$enableval"],
    [enable_egl=yes])

AC_ARG_ENABLE([xa],
    [AS_HELP_STRING([--enable-xa],
        [enable build of the XA X Acceleration API @<:@default=no@:>@])],
    [enable_xa="$enableval"],
    [enable_xa=no])
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
AC_ARG_ENABLE([omx],
   [AS_HELP_STRING([--enable-omx],
         [enable OpenMAX library @<:@default=no@:>@])],
   [enable_omx="$enableval"],
   [enable_omx=no])
AC_ARG_ENABLE([opencl],
   [AS_HELP_STRING([--enable-opencl],
         [enable OpenCL library @<:@default=no@:>@])],
   [enable_opencl="$enableval"],
   [enable_opencl=no])
AC_ARG_ENABLE([opencl_icd],
   [AS_HELP_STRING([--enable-opencl-icd],
          [Build an OpenCL ICD library to be loaded by an ICD implementation
           @<:@default=no@:>@])],
    [enable_opencl_icd="$enableval"],
    [enable_opencl_icd=no])
AC_ARG_ENABLE([xlib-glx],
    [AS_HELP_STRING([--enable-xlib-glx],
        [make GLX library Xlib-based instead of DRI-based @<:@default=disabled@:>@])],
    [enable_xlib_glx="$enableval"],
    [enable_xlib_glx=no])
AC_ARG_ENABLE([gallium-egl],
    [AS_HELP_STRING([--enable-gallium-egl],
        [enable optional EGL state tracker (not required
         for EGL support in Gallium with OpenGL and OpenGL ES)
         @<:@default=disable@:>@])],
    [enable_gallium_egl="$enableval"],
    [enable_gallium_egl=no])
AC_ARG_ENABLE([gallium-gbm],
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

AC_ARG_ENABLE([gallium-tests],
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
        "i915,ilo,nouveau,r300,r600,radeonsi,freedreno,svga,swrast,vc4"
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
        "x$enable_xa" = xno -a \
        "x$enable_xvmc" = xno -a \
        "x$enable_vdpau" = xno -a \
        "x$enable_omx" = xno -a \
        "x$enable_opencl" = xno; then
    AC_MSG_ERROR([at least one API should be enabled])
fi

# Building OpenGL ES1 and/or ES2 without OpenGL is not supported on mesa 9.0.x
if test "x$enable_opengl" = xno -a \
        "x$enable_gles1" = xyes; then
    AC_MSG_ERROR([Building OpenGL ES1 without OpenGL is not supported])
fi

if test "x$enable_opengl" = xno -a \
        "x$enable_gles2" = xyes; then
    AC_MSG_ERROR([Building OpenGL ES2 without OpenGL is not supported])
fi

AM_CONDITIONAL(HAVE_OPENGL, test "x$enable_opengl" = xyes)
AM_CONDITIONAL(HAVE_OPENGL_ES1, test "x$enable_gles1" = xyes)
AM_CONDITIONAL(HAVE_OPENGL_ES2, test "x$enable_gles2" = xyes)
AM_CONDITIONAL(NEED_OPENGL_COMMON, test "x$enable_opengl" = xyes -o \
                                        "x$enable_gles1" = xyes -o \
                                        "x$enable_gles2" = xyes)

if test "x$enable_glx" = xno; then
    AC_MSG_WARN([GLX disabled, disabling Xlib-GLX])
    enable_xlib_glx=no
fi

if test "x$enable_dri$enable_xlib_glx" = xyesyes; then
    AC_MSG_ERROR([DRI and Xlib-GLX cannot be built together])
fi

if test "x$enable_opengl$enable_xlib_glx" = xnoyes; then
    AC_MSG_ERROR([Xlib-GLX cannot be built without OpenGL])
fi

# Disable GLX if OpenGL is not enabled
if test "x$enable_glx$enable_opengl" = xyesno; then
    AC_MSG_WARN([OpenGL not enabled, disabling GLX])
    enable_glx=no
fi

# Disable GLX if DRI and Xlib-GLX are not enabled
if test "x$enable_glx" = xyes -a \
        "x$enable_dri" = xno -a \
        "x$enable_xlib_glx" = xno; then
    AC_MSG_WARN([Neither DRI nor Xlib-GLX enabled, disabling GLX])
    enable_glx=no
fi

AM_CONDITIONAL(HAVE_DRI_GLX, test "x$enable_glx" = xyes -a \
                                  "x$enable_dri" = xyes)

# Select which platform-dependent DRI code gets built
case "$host_os" in
darwin*)
    dri_platform='apple' ;;
gnu*|mingw*|cygwin*)
    dri_platform='none' ;;
*)
    dri_platform='drm' ;;
esac

AM_CONDITIONAL(HAVE_DRICOMMON, test "x$enable_dri" = xyes )
AM_CONDITIONAL(HAVE_DRISW, test "x$enable_dri" = xyes )
AM_CONDITIONAL(HAVE_DRI2, test "x$enable_dri" = xyes && test "x$dri_platform" = xdrm )
AM_CONDITIONAL(HAVE_DRI3, test "x$enable_dri3" = xyes && test "x$dri_platform" = xdrm )
AM_CONDITIONAL(HAVE_APPLEDRI, test "x$enable_dri" = xyes && test "x$dri_platform" = xapple )

AC_ARG_ENABLE([shared-glapi],
    [AS_HELP_STRING([--enable-shared-glapi],
        [Enable shared glapi for OpenGL @<:@default=yes@:>@])],
    [enable_shared_glapi="$enableval"],
    [enable_shared_glapi=yes])

case "x$enable_opengl$enable_gles1$enable_gles2" in
x*yes*yes*)
    if test "x$enable_shared_glapi" = xno; then
        AC_MSG_ERROR([shared GLAPI required when building two or more of
                      the following APIs - opengl, gles1 gles2])
    fi
    ;;
esac

# Building Xlib-GLX requires shared glapi to be disabled.
if test "x$enable_xlib_glx" = xyes; then
    AC_MSG_NOTICE([Shared GLAPI should not used with Xlib-GLX, disabling])
    enable_shared_glapi=no
fi

AM_CONDITIONAL(HAVE_SHARED_GLAPI, test "x$enable_shared_glapi" = xyes)

# Build the pipe-drivers as separate libraries/modules.
# Do not touch this unless you know what you are doing.
# XXX: Expose via configure option ?
enable_shared_pipe_drivers=no

dnl
dnl Driver specific build directories
dnl
GALLIUM_TARGET_DIRS=""
GALLIUM_WINSYS_DIRS="sw"
GALLIUM_DRIVERS_DIRS="galahad trace rbug noop identity"
GALLIUM_STATE_TRACKERS_DIRS=""

case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/xlib"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS libgl-xlib"
    GALLIUM_STATE_TRACKERS_DIRS="glx $GALLIUM_STATE_TRACKERS_DIRS"
    NEED_WINSYS_XLIB="yes"
    ;;
esac

if test "x$enable_dri" = xyes; then
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/dri"
    GALLIUM_STATE_TRACKERS_DIRS="dri $GALLIUM_STATE_TRACKERS_DIRS"
    enable_gallium_loader="$enable_shared_pipe_drivers"
fi

if test "x$enable_gallium_osmesa" = xyes; then
    if ! echo "$with_gallium_drivers" | grep -q 'swrast'; then
        AC_MSG_ERROR([gallium_osmesa requires the gallium swrast driver])
    fi
    if test "x$enable_osmesa" = xyes; then
        AC_MSG_ERROR([Cannot enable both classic and Gallium OSMesa implementations])
    fi
    GALLIUM_STATE_TRACKERS_DIRS="osmesa $GALLIUM_STATE_TRACKERS_DIRS"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS osmesa"
fi

AC_SUBST([MESA_LLVM])

# Check for libdrm
PKG_CHECK_MODULES([LIBDRM], [libdrm >= $LIBDRM_REQUIRED],
                  [have_libdrm=yes], [have_libdrm=no])
if test "x$have_libdrm" = xyes; then
	DEFINES="$DEFINES -DHAVE_LIBDRM"
fi

case "$host_os" in
linux*)
    need_pci_id=yes ;;
*)
    need_pci_id=no ;;
esac

PKG_CHECK_MODULES([LIBUDEV], [libudev >= $LIBUDEV_REQUIRED],
                  have_libudev=yes, have_libudev=no)

AC_ARG_ENABLE([sysfs],
    [AS_HELP_STRING([--enable-sysfs],
        [enable /sys PCI identification @<:@default=disabled@:>@])],
    [have_sysfs="$enableval"],
    [have_sysfs=no]
)

if test "x$enable_dri" = xyes; then
    if test "$enable_static" = yes; then
        AC_MSG_ERROR([Cannot use static libraries for DRI drivers])
    fi

    # not a hard requirement as swrast does not depend on it
    if test "x$have_libdrm" = xyes; then
        DRI_PC_REQ_PRIV="libdrm >= $LIBDRM_REQUIRED"
    fi
fi

AC_ARG_ENABLE([driglx-direct],
    [AS_HELP_STRING([--disable-driglx-direct],
        [disable direct rendering in GLX and EGL for DRI \
            @<:@default=auto@:>@])],
    [driglx_direct="$enableval"],
    [driglx_direct="yes"])

dnl
dnl libGL configuration per driver
dnl
case "x$enable_glx$enable_xlib_glx" in
xyesyes)
    # Xlib-based GLX
    dri_modules="x11 xext xcb"
    PKG_CHECK_MODULES([XLIBGL], [$dri_modules])
    GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV $dri_modules"
    X11_INCLUDES="$X11_INCLUDES $XLIBGL_CFLAGS"
    GL_LIB_DEPS="$XLIBGL_LIBS"
    GL_LIB_DEPS="$GL_LIB_DEPS $SELINUX_LIBS -lm $PTHREAD_LIBS $DLOPEN_LIBS"
    GL_PC_LIB_PRIV="$GL_PC_LIB_PRIV $SELINUX_LIBS -lm $PTHREAD_LIBS"
    ;;
xyesno)
    # DRI-based GLX
    PKG_CHECK_MODULES([GLPROTO], [glproto >= $GLPROTO_REQUIRED])

    # find the DRI deps for libGL
    dri_modules="x11 xext xdamage xfixes x11-xcb xcb xcb-glx >= $XCBGLX_REQUIRED"

    if test x"$driglx_direct" = xyes; then
        if test x"$dri_platform" = xdrm ; then
            DEFINES="$DEFINES -DGLX_USE_DRM"
            if test "x$have_libdrm" != xyes; then
               AC_MSG_ERROR([Direct rendering requires libdrm >= $LIBDRM_REQUIRED])
            fi

            PKG_CHECK_MODULES([DRI2PROTO], [dri2proto >= $DRI2PROTO_REQUIRED])
            GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV libdrm >= $LIBDRM_REQUIRED"
            if test x"$enable_dri3" = xyes; then
               PKG_CHECK_MODULES([DRI3PROTO], [dri3proto >= $DRI3PROTO_REQUIRED])
               PKG_CHECK_MODULES([PRESENTPROTO], [presentproto >= $PRESENTPROTO_REQUIRED])
            fi

            if test x"$enable_dri" = xyes; then
               dri_modules="$dri_modules xcb-dri2 >= $XCBDRI2_REQUIRED"
            fi

            if test x"$enable_dri3" = xyes; then
               PKG_CHECK_EXISTS([xcb >= $XCB_REQUIRED], [], AC_MSG_ERROR([DRI3 requires xcb >= $XCB_REQUIRED]))
               dri_modules="$dri_modules xcb-dri3 xcb-present xcb-sync xshmfence >= $XSHMFENCE_REQUIRED"
            fi
        fi
        if test x"$dri_platform" = xapple ; then
            DEFINES="$DEFINES -DGLX_USE_APPLEGL"
        fi
    fi

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

have_pci_id=no
if test "$have_libudev" = yes; then
    DEFINES="$DEFINES -DHAVE_LIBUDEV"
    have_pci_id=yes
fi

if test "$have_sysfs" = yes; then
    DEFINES="$DEFINES -DHAVE_SYSFS"
    have_pci_id=yes
fi

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
        [comma delimited classic DRI drivers list, e.g.
        "swrast,i965,radeon" @<:@default=auto@:>@])],
    [with_dri_drivers="$withval"],
    [with_dri_drivers=auto])

if test "x$with_dri_drivers" = xauto; then
    if test "x$enable_opengl" = xyes -a "x$enable_dri" = xyes; then
        with_dri_drivers="yes"
    else
        with_dri_drivers="no"
    fi
fi
if test "x$with_dri_drivers" = xno; then
    with_dri_drivers=''
fi

dnl If $with_dri_drivers is yes, drivers will be added through
dnl platform checks. Set DEFINES and LIB_DEPS
if test "x$enable_dri" = xyes; then
    # Platform specific settings and drivers to build
    case "$host_os" in
    linux*)
        DEFINES="$DEFINES -DHAVE_ALIAS"
        if test "x$enable_dri3" = xyes; then
            DEFINES="$DEFINES -DHAVE_DRI3"
        fi

        if test "x$have_pci_id" != xyes; then
            AC_MSG_ERROR([libudev-dev or sysfs required for building DRI])
        fi

        case "$host_cpu" in
        powerpc* | sparc*)
            # Build only the drivers for cards that exist on PowerPC/sparc
            if test "x$with_dri_drivers" = "xyes"; then
                with_dri_drivers="r200 radeon swrast"
            fi
            ;;
        esac
        ;;
    *freebsd* | dragonfly* | *netbsd* | openbsd*)
        DEFINES="$DEFINES -DHAVE_PTHREAD"
        DEFINES="$DEFINES -DHAVE_ALIAS"
        ;;
    gnu*)
        DEFINES="$DEFINES -DHAVE_ALIAS"
        ;;
    cygwin*)
        if test "x$with_dri_drivers" = "xyes"; then
            with_dri_drivers="swrast"
        fi
        ;;
    darwin*)
        DEFINES="$DEFINES -DGLX_ALIAS_UNSUPPORTED"
        if test "x$with_dri_drivers" = "xyes"; then
            with_dri_drivers="swrast"
        fi
        ;;
    esac

    # default drivers
    if test "x$with_dri_drivers" = "xyes"; then
        with_dri_drivers="i915 i965 nouveau r200 radeon swrast"
    fi

    # Check for expat
    PKG_CHECK_EXISTS([expat], [have_expat=yes], [have_expat=no])
    if test "x$have_expat" = "xyes"; then
       PKG_CHECK_MODULES([EXPAT], [expat], [],
                         AC_MSG_ERROR([Expat required for DRI.]))
    else
       # expat version 2.0 and earlier do not provide expat.pc
       EXPAT_LIBS=-lexpat
    fi

    DRICOMMON_NEED_LIBDRM=no
    # If we are building any DRI driver other than swrast.
    if test -n "$with_dri_drivers"; then
        if test "x$with_dri_drivers" != xswrast; then
            # ... libdrm is required
            if test "x$have_libdrm" != xyes; then
                AC_MSG_ERROR([DRI drivers requires libdrm >= $LIBDRM_REQUIRED])
            fi
            DRICOMMON_NEED_LIBDRM=yes
        fi
    fi

    # If we're building any gallium DRI driver other than swrast
    if test -n "$with_gallium_drivers" -a "x$DRICOMMON_NEED_LIBDRM" = xno; then
        if test "x$with_gallium_drivers" != xswrast; then
            # ... build a libdrm aware dricommon
            DRICOMMON_NEED_LIBDRM=yes
        fi
    fi

    # put all the necessary libs together
    DRI_LIB_DEPS="$DRI_LIB_DEPS $SELINUX_LIBS $LIBDRM_LIBS $EXPAT_LIBS -lm $PTHREAD_LIBS $DLOPEN_LIBS"
fi

AC_SUBST([DRI_LIB_DEPS])

DRI_DIRS=''
dnl Duplicates in DRI_DIRS are removed by sorting it at the end of this block
if test -n "$with_dri_drivers"; then
    if test "x$enable_opengl" != xyes; then
        AC_MSG_ERROR([--with-dri-drivers requires OpenGL])
    fi

    dri_drivers=`IFS=', '; echo $with_dri_drivers`
    for driver in $dri_drivers; do
        DRI_DIRS="$DRI_DIRS $driver"
        case "x$driver" in
        xi915)
            HAVE_I915_DRI=yes;
            PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
            ;;
        xi965)
            HAVE_I965_DRI=yes;
            PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
            ;;
        xnouveau)
            HAVE_NOUVEAU_DRI=yes;
            PKG_CHECK_MODULES([NOUVEAU], [libdrm_nouveau >= $LIBDRM_NVVIEUX_REQUIRED])
            ;;
        xradeon)
            HAVE_RADEON_DRI=yes;
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            ;;
        xr200)
            HAVE_R200_DRI=yes;
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            ;;
        xswrast)
            HAVE_SWRAST_DRI=yes;
            ;;
        *)
            AC_MSG_ERROR([classic DRI driver '$driver' does not exist])
            ;;
        esac
    done
    DRI_DIRS=`echo $DRI_DIRS|tr " " "\n"|sort -u|tr "\n" " "`
fi

AM_CONDITIONAL(NEED_MEGADRIVER, test -n "$DRI_DIRS")
AM_CONDITIONAL(NEED_LIBMESA, test "x$enable_xlib_glx" = xyes -o \
                                  "x$enable_osmesa" = xyes -o \
                                  -n "$DRI_DIRS")

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

if test "x$enable_osmesa" = xyes -o "x$enable_gallium_osmesa" = xyes; then
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
    if test "x$need_pci_id$have_pci_id" = xyesno; then
        AC_MSG_ERROR([gbm requires udev >= $LIBUDEV_REQUIRED or sysfs])
    fi

    if test "x$enable_dri" = xyes; then
        GBM_BACKEND_DIRS="$GBM_BACKEND_DIRS dri"
        if test "x$enable_shared_glapi" = xno; then
            AC_MSG_ERROR([gbm_dri requires --enable-shared-glapi])
        fi
    else
        # Strictly speaking libgbm does not require --enable-dri, although
        # both of its backends do. Thus one can build libgbm without any
        # backends if --disable-dri is set.
        # To avoid unnecessary complexity of checking if at least one backend
        # is available when building, just mandate --enable-dri.
        AC_MSG_ERROR([gbm requires --enable-dri])
    fi
fi
AM_CONDITIONAL(HAVE_GBM, test "x$enable_gbm" = xyes)
if test "x$need_pci_id$have_libudev" = xyesyes; then
    GBM_PC_REQ_PRIV="libudev >= $LIBUDEV_REQUIRED"
else
    GBM_PC_REQ_PRIV=""
fi
GBM_PC_LIB_PRIV="$DLOPEN_LIBS"
AC_SUBST([GBM_PC_REQ_PRIV])
AC_SUBST([GBM_PC_LIB_PRIV])

dnl
dnl EGL configuration
dnl
EGL_CLIENT_APIS=""

if test "x$enable_egl" = xyes; then
    EGL_LIB_DEPS="$DLOPEN_LIBS $SELINUX_LIBS $PTHREAD_LIBS"

    AC_CHECK_FUNC(mincore, [DEFINES="$DEFINES -DHAVE_MINCORE"])

    if test "$enable_static" != yes; then
        if test "x$enable_dri" = xyes; then
	    HAVE_EGL_DRIVER_DRI2=1
	fi

    fi
fi
AM_CONDITIONAL(HAVE_EGL, test "x$enable_egl" = xyes)
AC_SUBST([EGL_LIB_DEPS])

dnl
dnl EGL Gallium configuration
dnl
if test "x$enable_gallium_egl" = xyes; then
    if test -z "$with_gallium_drivers"; then
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
# XXX: Uncomment once converted to use static/shared pipe-drivers
#    enable_gallium_loader=$enable_shared_pipe_drivers
fi
AM_CONDITIONAL(HAVE_GALLIUM_EGL, test "x$enable_gallium_egl" = xyes)

dnl
dnl gbm Gallium configuration
dnl
if test "x$enable_gallium_gbm" = xauto; then
    case "$enable_gbm$enable_gallium_egl$enable_dri$with_egl_platforms" in
        yesyesyes*drm*)
            enable_gallium_gbm=yes ;;
         *)
            enable_gallium_gbm=no ;;
    esac
fi
if test "x$enable_gallium_gbm" = xyes; then
    if test -z "$with_gallium_drivers"; then
        AC_MSG_ERROR([cannot enable gbm_gallium without Gallium])
    fi
    if test "x$enable_gbm" = xno; then
        AC_MSG_ERROR([cannot enable gbm_gallium without gbm])
    fi

    if test "x$enable_gallium_egl" != xyes; then
        AC_MSG_ERROR([gbm_gallium is only used by egl_gallium])
    fi

    GALLIUM_STATE_TRACKERS_DIRS="gbm $GALLIUM_STATE_TRACKERS_DIRS"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS gbm"
    enable_gallium_loader=$enable_shared_pipe_drivers
fi
AM_CONDITIONAL(HAVE_GALLIUM_GBM, test "x$enable_gallium_gbm" = xyes)

dnl
dnl XA configuration
dnl
if test "x$enable_xa" = xyes; then
    if test "x$with_gallium_drivers" = xswrast; then
       AC_MSG_ERROR([
          Building xa requires at least one non swrast gallium driver.
          If you are looking to use libxatracker.so with the VMware driver,
          make sure to include svga in the gallium drivers list, apart from
          enabling XA.
          Example: ./configure --enable-xa --with-gallium-drivers=svga...])
    fi
    GALLIUM_STATE_TRACKERS_DIRS="xa $GALLIUM_STATE_TRACKERS_DIRS"
    enable_gallium_loader=$enable_shared_pipe_drivers
fi
AM_CONDITIONAL(HAVE_ST_XA, test "x$enable_xa" = xyes)

dnl
dnl OpenVG configuration
dnl
VG_LIB_DEPS=""

if test "x$enable_openvg" = xyes; then
    if test "x$enable_egl" = xno; then
        AC_MSG_ERROR([cannot enable OpenVG without EGL])
    fi
    if test -z "$with_gallium_drivers"; then
        AC_MSG_ERROR([cannot enable OpenVG without Gallium])
    fi
    if test "x$enable_gallium_egl" = xno; then
        AC_MSG_ERROR([cannot enable OpenVG without egl_gallium])
    fi

    EGL_CLIENT_APIS="$EGL_CLIENT_APIS "'$(VG_LIB)'
    VG_LIB_DEPS="$VG_LIB_DEPS $SELINUX_LIBS $PTHREAD_LIBS"
    GALLIUM_STATE_TRACKERS_DIRS="vega $GALLIUM_STATE_TRACKERS_DIRS"
    VG_PC_LIB_PRIV="-lm $CLOCK_LIB $PTHREAD_LIBS $DLOPEN_LIBS"
    AC_SUBST([VG_PC_LIB_PRIV])
fi
AM_CONDITIONAL(HAVE_OPENVG, test "x$enable_openvg" = xyes)

dnl
dnl Gallium G3DVL configuration
dnl
if test -n "$with_gallium_drivers" -a "x$with_gallium_drivers" != xswrast; then
    if test "x$enable_xvmc" = xauto; then
	PKG_CHECK_EXISTS([xvmc], [enable_xvmc=yes], [enable_xvmc=no])
    fi

    if test "x$enable_vdpau" = xauto; then
	PKG_CHECK_EXISTS([vdpau], [enable_vdpau=yes], [enable_vdpau=no])
    fi

    if test "x$enable_omx" = xauto; then
	PKG_CHECK_EXISTS([libomxil-bellagio], [enable_omx=yes], [enable_omx=no])
    fi
fi

if test "x$enable_xvmc" = xyes; then
    PKG_CHECK_MODULES([XVMC], [xvmc >= $XVMC_REQUIRED x11-xcb xcb xcb-dri2 >= $XCBDRI2_REQUIRED])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS xvmc"
    enable_gallium_loader=$enable_shared_pipe_drivers
fi
AM_CONDITIONAL(HAVE_ST_XVMC, test "x$enable_xvmc" = xyes)

if test "x$enable_vdpau" = xyes; then
    PKG_CHECK_MODULES([VDPAU], [vdpau >= $VDPAU_REQUIRED x11-xcb xcb xcb-dri2 >= $XCBDRI2_REQUIRED],
                      [VDPAU_LIBS="`$PKG_CONFIG --libs x11-xcb xcb xcb-dri2`"])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS vdpau"
    enable_gallium_loader=$enable_shared_pipe_drivers
fi
AM_CONDITIONAL(HAVE_ST_VDPAU, test "x$enable_vdpau" = xyes)

if test "x$enable_omx" = xyes; then
    PKG_CHECK_MODULES([OMX], [libomxil-bellagio >= $LIBOMXIL_BELLAGIO_REQUIRED x11-xcb xcb xcb-dri2 >= $XCBDRI2_REQUIRED])
    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS omx"
    enable_gallium_loader=$enable_shared_pipe_drivers
fi
AM_CONDITIONAL(HAVE_ST_OMX, test "x$enable_omx" = xyes)

dnl
dnl OpenCL configuration
dnl

AC_ARG_WITH([libclc-path],
   [AS_HELP_STRING([--with-libclc-path],
         [DEPRECATED: See http://dri.freedesktop.org/wiki/GalliumCompute#How_to_Install])],
   [LIBCLC_PATH="$withval"],
   [LIBCLC_PATH=''])

if test -n "$LIBCLC_PATH"; then
   AC_MSG_ERROR([The --with-libclc-path option has been deprecated.
                  Please review the updated build instructions for clover:
		  http://dri.freedesktop.org/wiki/GalliumCompute])
fi


AC_ARG_WITH([clang-libdir],
   [AS_HELP_STRING([--with-clang-libdir],
         [Path to Clang libraries @<:@default=llvm-config --libdir@:>@])],
   [CLANG_LIBDIR="$withval"],
   [CLANG_LIBDIR=''])

PKG_CHECK_EXISTS([libclc], [have_libclc=yes], [have_libclc=no])

if test "x$enable_opencl" = xyes; then
    if test -z "$with_gallium_drivers"; then
        AC_MSG_ERROR([cannot enable OpenCL without Gallium])
    fi

    if test $GCC_VERSION_MAJOR -lt 4 -o $GCC_VERSION_MAJOR -eq 4 -a $GCC_VERSION_MINOR -lt 7; then
        AC_MSG_ERROR([gcc >= 4.7 is required to build clover])
    fi

    if test "x$have_libclc" = xno; then
        AC_MSG_ERROR([pkg-config cannot find libclc.pc which is required to build clover.
                    Make sure the directory containing libclc.pc is specified in your
                    PKG_CONFIG_PATH environment variable.
                    By default libclc.pc is installed to /usr/local/share/pkgconfig/])
    else
        LIBCLC_INCLUDEDIR=`$PKG_CONFIG --variable=includedir libclc`
        LIBCLC_LIBEXECDIR=`$PKG_CONFIG --variable=libexecdir libclc`
        AC_SUBST([LIBCLC_INCLUDEDIR])
        AC_SUBST([LIBCLC_LIBEXECDIR])
    fi

    GALLIUM_STATE_TRACKERS_DIRS="$GALLIUM_STATE_TRACKERS_DIRS clover"
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS opencl"
    # XXX: Use $enable_shared_pipe_drivers once converted to use static/shared pipe-drivers
    enable_gallium_loader=yes

    if test "x$enable_opencl_icd" = xyes; then
        OPENCL_LIBNAME="MesaOpenCL"
    else
        OPENCL_LIBNAME="OpenCL"
    fi
fi
AM_CONDITIONAL(HAVE_CLOVER, test "x$enable_opencl" = xyes)
AM_CONDITIONAL(HAVE_CLOVER_ICD, test "x$enable_opencl_icd" = xyes)
AC_SUBST([OPENCL_LIBNAME])

dnl
dnl Gallium configuration
dnl
AM_CONDITIONAL(HAVE_GALLIUM, test -n "$with_gallium_drivers")

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
    ;;
esac

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

if test "x$with_egl_platforms" != "x" -a "x$enable_egl" != xyes; then
    AC_MSG_ERROR([cannot build egl state tracker without EGL library])
fi

# Do per-EGL platform setups and checks
egl_platforms=`IFS=', '; echo $with_egl_platforms`
for plat in $egl_platforms; do
	case "$plat" in
	wayland)
		PKG_CHECK_MODULES([WAYLAND], [wayland-client >= $WAYLAND_REQUIRED wayland-server >= $WAYLAND_REQUIRED])
		GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/wayland"

                WAYLAND_PREFIX=`$PKG_CONFIG --variable=prefix wayland-client`
                AC_PATH_PROG([WAYLAND_SCANNER], [wayland-scanner],,
                             [${WAYLAND_PREFIX}/bin$PATH_SEPARATOR$PATH])
		;;

	x11)
		PKG_CHECK_MODULES([XCB_DRI2], [x11-xcb xcb xcb-dri2 >= $XCBDRI2_REQUIRED xcb-xfixes])
		;;

	drm)
		test "x$enable_gbm" = "xno" &&
			AC_MSG_ERROR([EGL platform drm needs gbm])
		test "x$have_libdrm" != xyes &&
			AC_MSG_ERROR([EGL platform drm requires libdrm >= $LIBDRM_REQUIRED])
		;;

	android|fbdev|gdi|null)
		;;

	*)
		AC_MSG_ERROR([EGL platform '$plat' does not exist])
		;;
	esac

        case "$plat$need_pci_id$have_pci_id" in
                waylandyesno|drmyesno)
                    AC_MSG_ERROR([cannot build $plat platform without udev >= $LIBUDEV_REQUIRED or sysfs]) ;;
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

if echo "$egl_platforms" | grep -q 'x11'; then
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/xlib"
    NEED_WINSYS_XLIB=yes
fi
AM_CONDITIONAL(HAVE_EGL_PLATFORM_X11, echo "$egl_platforms" | grep -q 'x11')
AM_CONDITIONAL(HAVE_EGL_PLATFORM_WAYLAND, echo "$egl_platforms" | grep -q 'wayland')
AM_CONDITIONAL(HAVE_EGL_PLATFORM_DRM, echo "$egl_platforms" | grep -q 'drm')
AM_CONDITIONAL(HAVE_EGL_PLATFORM_FBDEV, echo "$egl_platforms" | grep -q 'fbdev')
AM_CONDITIONAL(HAVE_EGL_PLATFORM_NULL, echo "$egl_platforms" | grep -q 'null')

AM_CONDITIONAL(HAVE_EGL_DRIVER_DRI2, test "x$HAVE_EGL_DRIVER_DRI2" != "x")

AC_SUBST([EGL_NATIVE_PLATFORM])
AC_SUBST([EGL_CFLAGS])

# If we don't have the X11 platform, set this define so we don't try to include
# the X11 headers.
if ! echo "$egl_platforms" | grep -q 'x11'; then
    DEFINES="$DEFINES -DMESA_EGL_NO_X11_HEADERS"
    GL_PC_CFLAGS="$GL_PC_CFLAGS -DMESA_EGL_NO_X11_HEADERS"
fi

AC_ARG_WITH([egl-driver-dir],
    [AS_HELP_STRING([--with-egl-driver-dir=DIR],
                    [directory for EGL drivers @<:@default=${libdir}/egl@:>@])],
    [EGL_DRIVER_INSTALL_DIR="$withval"],
    [EGL_DRIVER_INSTALL_DIR='${libdir}/egl'])
AC_SUBST([EGL_DRIVER_INSTALL_DIR])

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

AC_ARG_ENABLE([llvm-shared-libs],
    [AS_HELP_STRING([--enable-llvm-shared-libs],
        [link with LLVM shared libraries @<:@default=enabled@:>@])],
    [enable_llvm_shared_libs="$enableval"],
    [enable_llvm_shared_libs=yes])

AC_ARG_WITH([llvm-prefix],
    [AS_HELP_STRING([--with-llvm-prefix],
        [Prefix for LLVM installations in non-standard locations])],
    [llvm_prefix="$withval"],
    [llvm_prefix=''])


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
	-e 's/-Wcast-qual\>//g' \
	-e 's/-Woverloaded-virtual\>//g' \
	-e 's/-fcolor-diagnostics\>//g' \
	-e 's/-fdata-sections\>//g' \
	-e 's/-ffunction-sections\>//g' \
	-e 's/-fno-exceptions\>//g' \
	-e 's/-fomit-frame-pointer\>//g' \
	-e 's/-fvisibility-inlines-hidden\>//g' \
	-e 's/-fPIC\>//g' \
	-e 's/-fstack-protector-strong\>//g'
}


if test -z "$with_gallium_drivers"; then
    enable_gallium_llvm=no
fi
if test "x$enable_gallium_llvm" = xauto; then
    case "$host_cpu" in
    i*86|x86_64|amd64) enable_gallium_llvm=yes;;
    esac
fi
if test "x$enable_gallium_llvm" = xyes; then
    if test -n "$llvm_prefix"; then
        AC_PATH_TOOL([LLVM_CONFIG], [llvm-config], [no], ["$llvm_prefix/bin"])
    else
        AC_PATH_TOOL([LLVM_CONFIG], [llvm-config], [no])
    fi

    if test "x$LLVM_CONFIG" != xno; then
        LLVM_VERSION=`$LLVM_CONFIG --version | grep -o '^[[0-9.]]\+'`
        LLVM_LDFLAGS=`$LLVM_CONFIG --ldflags`
        LLVM_BINDIR=`$LLVM_CONFIG --bindir`
        LLVM_CPPFLAGS=`strip_unwanted_llvm_flags "$LLVM_CONFIG --cppflags"`
        LLVM_CFLAGS=$LLVM_CPPFLAGS   # CPPFLAGS seem to be sufficient
        LLVM_CXXFLAGS=`strip_unwanted_llvm_flags "$LLVM_CONFIG --cxxflags"`
        LLVM_INCLUDEDIR=`$LLVM_CONFIG --includedir`
        LLVM_LIBDIR=`$LLVM_CONFIG --libdir`

        AC_COMPUTE_INT([LLVM_VERSION_MAJOR], [LLVM_VERSION_MAJOR],
            [#include "${LLVM_INCLUDEDIR}/llvm/Config/llvm-config.h"])
        AC_COMPUTE_INT([LLVM_VERSION_MINOR], [LLVM_VERSION_MINOR],
            [#include "${LLVM_INCLUDEDIR}/llvm/Config/llvm-config.h"])

        dnl In LLVM 3.4.1 patch level was defined in config.h and not
        dnl llvm-config.h
        AC_COMPUTE_INT([LLVM_VERSION_PATCH], [LLVM_VERSION_PATCH],
            [#include "${LLVM_INCLUDEDIR}/llvm/Config/config.h"],
            LLVM_VERSION_PATCH=0) dnl Default if LLVM_VERSION_PATCH not found

        if test -n "${LLVM_VERSION_MAJOR}"; then
            LLVM_VERSION_INT="${LLVM_VERSION_MAJOR}0${LLVM_VERSION_MINOR}"
        else
            LLVM_VERSION_INT=`echo $LLVM_VERSION | sed -e 's/\([[0-9]]\)\.\([[0-9]]\)/\10\2/g'`
        fi

        LLVM_REQUIRED_VERSION_MAJOR="3"
        LLVM_REQUIRED_VERSION_MINOR="1"
        if test "$LLVM_VERSION_INT" -lt "${LLVM_REQUIRED_VERSION_MAJOR}0${LLVM_REQUIRED_VERSION_MINOR}"; then
            AC_MSG_ERROR([LLVM $LLVM_REQUIRED_VERSION_MAJOR.$LLVM_REQUIRED_VERSION_MINOR or newer is required])
        fi

        LLVM_COMPONENTS="engine bitwriter"
        if $LLVM_CONFIG --components | grep -qw 'mcjit'; then
            LLVM_COMPONENTS="${LLVM_COMPONENTS} mcjit"
        fi

        if test "x$enable_opencl" = xyes; then
            LLVM_COMPONENTS="${LLVM_COMPONENTS} ipo linker instrumentation"
            # LLVM 3.3 >= 177971 requires IRReader
            if $LLVM_CONFIG --components | grep -qw 'irreader'; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} irreader"
            fi
            # LLVM 3.4 requires Option
            if $LLVM_CONFIG --components | grep -qw 'option'; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} option"
            fi
            # Current OpenCL/Clover and LLVM 3.5 require ObjCARCOpts and ProfileData
            if $LLVM_CONFIG --components | grep -qw 'objcarcopts'; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} objcarcopts"
            fi
            if $LLVM_CONFIG --components | grep -qw 'profiledata'; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} profiledata"
            fi
        fi
        DEFINES="${DEFINES} -DHAVE_LLVM=0x0$LLVM_VERSION_INT -DLLVM_VERSION_PATCH=$LLVM_VERSION_PATCH"
        MESA_LLVM=1

        dnl Check for Clang internal headers
        if test "x$enable_opencl" = xyes; then
            if test -z "$CLANG_LIBDIR"; then
                CLANG_LIBDIR=${LLVM_LIBDIR}
            fi
            CLANG_RESOURCE_DIR=$CLANG_LIBDIR/clang/${LLVM_VERSION}
            AS_IF([test ! -f "$CLANG_RESOURCE_DIR/include/stddef.h"],
                [AC_MSG_ERROR([Could not find clang internal header stddef.h in $CLANG_RESOURCE_DIR Use --with-clang-libdir to specify the correct path to the clang libraries.])])
        fi
    else
        MESA_LLVM=0
        LLVM_VERSION_INT=0
    fi
else
    MESA_LLVM=0
    LLVM_VERSION_INT=0

    if test "x$enable_opencl" = xyes; then
        AC_MSG_ERROR([cannot enable OpenCL without LLVM])
    fi
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
    # XXX: Use $enable_shared_pipe_drivers once converted to use static/shared pipe-drivers
    enable_gallium_loader=yes
fi
AM_CONDITIONAL(HAVE_GALLIUM_TESTS, test "x$enable_gallium_tests" = xyes)

if test "x$enable_gallium_loader" = xyes; then
    GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS pipe-loader"
fi
AM_CONDITIONAL(NEED_GALLIUM_LOADER, test "x$enable_gallium_loader" = xyes)

dnl Directory for VDPAU libs
AC_ARG_WITH([vdpau-libdir],
    [AS_HELP_STRING([--with-vdpau-libdir=DIR],
        [directory for the VDPAU libraries @<:@default=${libdir}/vdpau@:>@])],
    [VDPAU_LIB_INSTALL_DIR="$withval"],
    [VDPAU_LIB_INSTALL_DIR='${libdir}/vdpau'])
AC_SUBST([VDPAU_LIB_INSTALL_DIR])

OMX_LIB_INSTALL_DIR_DEFAULT=''
if test "x$enable_omx" = xyes; then
    OMX_LIB_INSTALL_DIR_DEFAULT=`$PKG_CONFIG --variable=pluginsdir libomxil-bellagio`
fi

AC_ARG_WITH([omx-libdir],
    [AS_HELP_STRING([--with-omx-libdir=DIR],
        [directory for the OMX libraries])],
    [OMX_LIB_INSTALL_DIR="$withval"],
    [OMX_LIB_INSTALL_DIR="$OMX_LIB_INSTALL_DIR_DEFAULT"])
AC_SUBST([OMX_LIB_INSTALL_DIR])

dnl Directory for OpenCL libs
AC_ARG_WITH([opencl-libdir],
    [AS_HELP_STRING([--with-opencl-libdir=DIR],
        [directory for auxiliary libraries used by the OpenCL implementation @<:@default=${libdir}/opencl@:>@])],
    [OPENCL_LIB_INSTALL_DIR="$withval"],
    [OPENCL_LIB_INSTALL_DIR='${libdir}/opencl'])
AC_SUBST([OPENCL_LIB_INSTALL_DIR])

dnl
dnl Gallium helper functions
dnl
gallium_check_st() {
    if test "x$have_libdrm" != xyes; then
       AC_MSG_ERROR([$1 requires libdrm >= $LIBDRM_REQUIRED])
    fi
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS $2"

    if test "x$enable_dri" = xyes && test -n "$3"; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $3"
    fi
    if test "x$enable_xa" = xyes && test -n "$4"; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $4"
    fi
    if test "x$enable_xvmc" = xyes && test -n "$5"; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $5"
    fi
    if test "x$enable_vdpau" = xyes && test -n "$6"; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $6"
    fi
    if test "x$enable_omx" = xyes && test "x$7" != x; then
         GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS $7"
    fi
}

gallium_require_llvm() {
    if test "x$MESA_LLVM" = x0; then
        case "$host" in *gnux32) return;; esac
        case "$host_cpu" in
        i*86|x86_64|amd64) AC_MSG_ERROR([LLVM is required to build $1 on x86 and x86_64]);;
        esac
    fi
}

gallium_require_drm_loader() {
    if test "x$enable_gallium_loader" = xyes; then
        if test "x$need_pci_id$have_pci_id" = xyesno; then
            AC_MSG_ERROR([Gallium drm loader requires libudev >= $LIBUDEV_REQUIRED or sysfs])
        fi
        if test "x$have_libdrm" != xyes; then
            AC_MSG_ERROR([Gallium drm loader requires libdrm >= $LIBDRM_REQUIRED])
        fi
        enable_gallium_drm_loader=yes
    fi
}

require_egl_drm() {
    case "$with_egl_platforms" in
        *drm*)
            ;;
         *)
            AC_MSG_ERROR([--with-egl-platforms=drm is required to build the $1 driver.])
            ;;
    esac
    if test "x$enable_gbm" != xyes; then
            AC_MSG_ERROR([--enable-gbm is required to build the $1 driver.])
    fi
}

radeon_llvm_check() {
    if test "x$enable_gallium_llvm" != "xyes"; then
        AC_MSG_ERROR([--enable-gallium-llvm is required when building $1])
    fi
    LLVM_REQUIRED_VERSION_MAJOR="3"
    LLVM_REQUIRED_VERSION_MINOR="4"
    LLVM_REQUIRED_VERSION_PATCH="2"
    if test "${LLVM_VERSION_INT}${LLVM_VERSION_PATCH}" -lt "${LLVM_REQUIRED_VERSION_MAJOR}0${LLVM_REQUIRED_VERSION_MINOR}${LLVM_REQUIRED_VERSION_PATCH}"; then
        AC_MSG_ERROR([LLVM $LLVM_REQUIRED_VERSION_MAJOR.$LLVM_REQUIRED_VERSION_MINOR.$LLVM_REQUIRED_VERSION_PATCH or newer is required for $1])
    fi
    if test true && $LLVM_CONFIG --targets-built | grep -qvw 'R600' ; then
        AC_MSG_ERROR([LLVM R600 Target not enabled.  You can enable it when building the LLVM
                      sources with the --enable-experimental-targets=R600
                      configure flag])
    fi
    LLVM_COMPONENTS="${LLVM_COMPONENTS} r600 bitreader ipo"
    NEED_RADEON_LLVM=yes
    AC_CHECK_LIB([elf], [elf_memory], [ELF_LIB=-lelf],
                                 [AC_MSG_ERROR([$1 requires libelf when using LLVM])])
}

dnl Duplicates in GALLIUM_DRIVERS_DIRS are removed by sorting it after this block
if test -n "$with_gallium_drivers"; then
    gallium_drivers=`IFS=', '; echo $with_gallium_drivers`
    for driver in $gallium_drivers; do
        case "x$driver" in
        xsvga)
            HAVE_GALLIUM_SVGA=yes
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS svga softpipe"
            gallium_require_drm_loader
            gallium_check_st "svga" "svga/drm" "dri/vmwgfx" "xa/vmwgfx"
            ;;
        xi915)
            HAVE_GALLIUM_I915=yes
            PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS i915"
            gallium_check_st "Gallium i915" "i915/drm" "dri/i915" "xa/i915"
            ;;
        xilo)
            HAVE_GALLIUM_ILO=yes
            PKG_CHECK_MODULES([INTEL], [libdrm_intel >= $LIBDRM_INTEL_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS ilo"
            gallium_check_st "Gallium i965/ilo" "intel/drm" "dri/ilo" "xa/ilo"
            ;;
        xr300)
            HAVE_GALLIUM_R300=yes
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            gallium_require_drm_loader
            gallium_require_llvm "Gallium R300"
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r300"
            gallium_check_st "Gallium R300" "radeon/drm" "dri/r300"
            ;;
        xr600)
            HAVE_GALLIUM_R600=yes
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r600"
            if test "x$enable_r600_llvm" = xyes -o "x$enable_opencl" = xyes; then
                radeon_llvm_check "r600g"
                LLVM_COMPONENTS="${LLVM_COMPONENTS} bitreader asmparser"
            fi
            if test "x$enable_r600_llvm" = xyes; then
                USE_R600_LLVM_COMPILER=yes;
            fi
            if test "x$enable_opencl" = xyes; then
                LLVM_COMPONENTS="${LLVM_COMPONENTS} bitreader asmparser"
            fi
            gallium_check_st "Gallium R600" "radeon/drm" "dri/r600" "" "xvmc/r600" "vdpau/r600" "omx/r600"
            ;;
        xradeonsi)
            HAVE_GALLIUM_RADEONSI=yes
            PKG_CHECK_MODULES([RADEON], [libdrm_radeon >= $LIBDRM_RADEON_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS radeonsi"
            radeon_llvm_check "radeonsi"
            require_egl_drm "radeonsi"
            gallium_check_st "radeonsi" "radeon/drm" "dri/radeonsi" "" "" "vdpau/radeonsi" "omx/radeonsi"
            ;;
        xnouveau)
            HAVE_GALLIUM_NOUVEAU=yes
            PKG_CHECK_MODULES([NOUVEAU], [libdrm_nouveau >= $LIBDRM_NOUVEAU_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS nouveau"
            gallium_check_st "nouveau" "nouveau/drm" "dri/nouveau" "xa/nouveau" "xvmc/nouveau" "vdpau/nouveau" "omx/nouveau"
            ;;
        xfreedreno)
            HAVE_GALLIUM_FREEDRENO=yes
            PKG_CHECK_MODULES([FREEDRENO], [libdrm_freedreno >= $LIBDRM_FREEDRENO_REQUIRED])
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS freedreno"
            gallium_check_st "freedreno" "freedreno/drm" "dri/freedreno" "xa/freedreno" "" ""
            ;;
        xswrast)
            HAVE_GALLIUM_SOFTPIPE=yes
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS softpipe"
            if test "x$MESA_LLVM" = x1; then
                HAVE_GALLIUM_LLVMPIPE=yes
                GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS llvmpipe"
            fi

            if test "x$enable_dri" = xyes; then
                GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS dri/swrast"
            fi

            if test "x$have_libdrm" = xyes; then
                GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/kms-dri"
                GALLIUM_TARGET_DIRS="$GALLIUM_TARGET_DIRS dri/kms-swrast"
            fi
            ;;
        xvc4)
            HAVE_GALLIUM_VC4=yes
            gallium_require_drm_loader
            GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS vc4"
            gallium_check_st "vc4" "vc4/drm" "dri-vc4" "" "" ""

            case "$host_cpu" in
                i?86 | x86_64 | amd64)
                USE_VC4_SIMULATOR=yes
                ;;
            esac
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

    if test "x$enable_llvm_shared_libs" = xyes; then
        dnl We can't use $LLVM_VERSION because it has 'svn' stripped out,
        LLVM_SO_NAME=LLVM-`$LLVM_CONFIG --version`
        AS_IF([test -f "$LLVM_LIBDIR/lib$LLVM_SO_NAME.so"], [llvm_have_one_so=yes])

        if test "x$llvm_have_one_so" = xyes; then
            dnl LLVM was built using auto*, so there is only one shared object.
            LLVM_LIBS="-l$LLVM_SO_NAME"
        else
            dnl If LLVM was built with CMake, there will be one shared object per
            dnl component.
            AS_IF([test ! -f "$LLVM_LIBDIR/libLLVMTarget.so"],
                    [AC_MSG_ERROR([Could not find llvm shared libraries:
	Please make sure you have built llvm with the --enable-shared option
	and that your llvm libraries are installed in $LLVM_LIBDIR
	If you have installed your llvm libraries to a different directory you
	can use the --with-llvm-prefix= configure flag to specify this directory.
	NOTE: Mesa is attempting to use llvm shared libraries by default.
	If you do not want to build with llvm shared libraries and instead want to
	use llvm static libraries then add --disable-llvm-shared-libs to your configure
	invocation and rebuild.])])

           dnl We don't need to update LLVM_LIBS in this case because the LLVM
           dnl install uses a shared object for each compoenent and we have
           dnl already added all of these objects to LLVM_LIBS.
        fi
    else
        AC_MSG_WARN([Building mesa with staticly linked LLVM may cause compilation issues])
    fi
fi

AM_CONDITIONAL(HAVE_GALLIUM_SVGA, test "x$HAVE_GALLIUM_SVGA" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_I915, test "x$HAVE_GALLIUM_I915" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_ILO, test "x$HAVE_GALLIUM_ILO" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_R300, test "x$HAVE_GALLIUM_R300" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_R600, test "x$HAVE_GALLIUM_R600" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_RADEONSI, test "x$HAVE_GALLIUM_RADEONSI" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_RADEON_COMMON, test "x$HAVE_GALLIUM_R600" = xyes -o \
                                                "x$HAVE_GALLIUM_RADEONSI" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_NOUVEAU, test "x$HAVE_GALLIUM_NOUVEAU" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_FREEDRENO, test "x$HAVE_GALLIUM_FREEDRENO" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_SOFTPIPE, test "x$HAVE_GALLIUM_SOFTPIPE" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_LLVMPIPE, test "x$HAVE_GALLIUM_LLVMPIPE" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_VC4, test "x$HAVE_GALLIUM_VC4" = xyes)

AM_CONDITIONAL(NEED_GALLIUM_SOFTPIPE_DRIVER, test "x$HAVE_GALLIUM_SVGA" = xyes -o \
                                                  "x$HAVE_GALLIUM_SOFTPIPE" = xyes)
AM_CONDITIONAL(NEED_GALLIUM_LLVMPIPE_DRIVER, test "x$HAVE_GALLIUM_SOFTPIPE" = xyes \
                                                  && test "x$MESA_LLVM" = x1)

AM_CONDITIONAL(HAVE_GALLIUM_STATIC_TARGETS, test "x$enable_shared_pipe_drivers" = xno)

# NOTE: anything using xcb or other client side libs ends up in separate
#       _CLIENT variables.  The pipe loader is built in two variants,
#       one that is standalone and does not link any x client libs (for
#       use by XA tracker in particular, but could be used in any case
#       where communication with xserver is not desired).
if test "x$enable_gallium_loader" = xyes; then
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS sw/null"

    if test "x$NEED_WINSYS_XLIB" = xyes; then
        GALLIUM_PIPE_LOADER_DEFINES="$GALLIUM_PIPE_LOADER_DEFINES -DHAVE_PIPE_LOADER_XLIB"
    fi

    if test "x$enable_dri" = xyes; then
        GALLIUM_PIPE_LOADER_DEFINES="$GALLIUM_PIPE_LOADER_DEFINES -DHAVE_PIPE_LOADER_DRI"
    fi

    if test "x$enable_gallium_drm_loader" = xyes; then
        GALLIUM_PIPE_LOADER_DEFINES="$GALLIUM_PIPE_LOADER_DEFINES -DHAVE_PIPE_LOADER_DRM"
        PKG_CHECK_MODULES([GALLIUM_PIPE_LOADER_XCB], [xcb xcb-dri2],
                          pipe_loader_have_xcb=yes, pipe_loader_have_xcb=no)
        if test "x$pipe_loader_have_xcb" = xyes; then
            GALLIUM_PIPE_LOADER_CLIENT_DEFINES="$GALLIUM_PIPE_LOADER_CLIENT_DEFINES -DHAVE_PIPE_LOADER_XCB"
            GALLIUM_PIPE_LOADER_CLIENT_LIBS="$GALLIUM_PIPE_LOADER_CLIENT_LIBS $GALLIUM_PIPE_LOADER_XCB_LIBS $LIBDRM_LIBS"
        fi
    fi

    GALLIUM_PIPE_LOADER_CLIENT_DEFINES="$GALLIUM_PIPE_LOADER_CLIENT_DEFINES $GALLIUM_PIPE_LOADER_DEFINES"
    GALLIUM_PIPE_LOADER_CLIENT_LIBS="$GALLIUM_PIPE_LOADER_CLIENT_LIBS $GALLIUM_PIPE_LOADER_LIBS"

    AC_SUBST([GALLIUM_PIPE_LOADER_DEFINES])
    AC_SUBST([GALLIUM_PIPE_LOADER_LIBS])
    AC_SUBST([GALLIUM_PIPE_LOADER_CLIENT_DEFINES])
    AC_SUBST([GALLIUM_PIPE_LOADER_CLIENT_LIBS])
fi

AM_CONDITIONAL(HAVE_I915_DRI, test x$HAVE_I915_DRI = xyes)
AM_CONDITIONAL(HAVE_I965_DRI, test x$HAVE_I965_DRI = xyes)
AM_CONDITIONAL(HAVE_NOUVEAU_DRI, test x$HAVE_NOUVEAU_DRI = xyes)
AM_CONDITIONAL(HAVE_R200_DRI, test x$HAVE_R200_DRI = xyes)
AM_CONDITIONAL(HAVE_RADEON_DRI, test x$HAVE_RADEON_DRI = xyes)
AM_CONDITIONAL(HAVE_SWRAST_DRI, test x$HAVE_SWRAST_DRI = xyes)

AM_CONDITIONAL(NEED_RADEON_DRM_WINSYS, test "x$HAVE_GALLIUM_R300" = xyes -o \
                                            "x$HAVE_GALLIUM_R600" = xyes -o \
                                            "x$HAVE_GALLIUM_RADEONSI" = xyes)
AM_CONDITIONAL(NEED_WINSYS_XLIB, test "x$NEED_WINSYS_XLIB" = xyes)
AM_CONDITIONAL(NEED_RADEON_LLVM, test x$NEED_RADEON_LLVM = xyes)
AM_CONDITIONAL(USE_R600_LLVM_COMPILER, test x$USE_R600_LLVM_COMPILER = xyes)
AM_CONDITIONAL(HAVE_LOADER_GALLIUM, test x$enable_gallium_loader = xyes)
AM_CONDITIONAL(HAVE_DRM_LOADER_GALLIUM, test x$enable_gallium_drm_loader = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_COMPUTE, test x$enable_opencl = xyes)
AM_CONDITIONAL(HAVE_MESA_LLVM, test x$MESA_LLVM = x1)
AM_CONDITIONAL(USE_VC4_SIMULATOR, test x$USE_VC4_SIMULATOR = xyes)
if test "x$USE_VC4_SIMULATOR" = xyes -a "x$HAVE_GALLIUM_ILO" = xyes; then
    AC_MSG_ERROR([VC4 simulator on x86 replaces i965 driver build, so ilo must be disabled.])
fi

AC_SUBST([ELF_LIB])

AM_CONDITIONAL(DRICOMMON_NEED_LIBDRM, test "x$DRICOMMON_NEED_LIBDRM" = xyes)
AM_CONDITIONAL(HAVE_LIBDRM, test "x$have_libdrm" = xyes)
AM_CONDITIONAL(HAVE_X11_DRIVER, test "x$enable_xlib_glx" = xyes)
AM_CONDITIONAL(HAVE_OSMESA, test "x$enable_osmesa" = xyes)
AM_CONDITIONAL(HAVE_GALLIUM_OSMESA, test "x$enable_gallium_osmesa" = xyes)

AM_CONDITIONAL(HAVE_X86_ASM, test "x$asm_arch" = xx86 -o "x$asm_arch" = xx86_64)
AM_CONDITIONAL(HAVE_X86_64_ASM, test "x$asm_arch" = xx86_64)
AM_CONDITIONAL(HAVE_SPARC_ASM, test "x$asm_arch" = xsparc)

AC_SUBST([VDPAU_MAJOR], 1)
AC_SUBST([VDPAU_MINOR], 0)

AC_SUBST([XVMC_MAJOR], 1)
AC_SUBST([XVMC_MINOR], 0)

XA_HEADER="$srcdir/src/gallium/state_trackers/xa/xa_tracker.h"
XA_MAJOR=`grep "#define XA_TRACKER_VERSION_MAJOR" $XA_HEADER | $SED 's/^#define XA_TRACKER_VERSION_MAJOR //'`
XA_MINOR=`grep "#define XA_TRACKER_VERSION_MINOR" $XA_HEADER | $SED 's/^#define XA_TRACKER_VERSION_MINOR //'`
XA_TINY=`grep "#define XA_TRACKER_VERSION_PATCH" $XA_HEADER | $SED 's/^#define XA_TRACKER_VERSION_PATCH //'`

AC_SUBST([XA_MAJOR], $XA_MAJOR)
AC_SUBST([XA_MINOR], $XA_MINOR)
AC_SUBST([XA_TINY], $XA_TINY)
AC_SUBST([XA_VERSION], "$XA_MAJOR.$XA_MINOR.$XA_TINY")

dnl Restore LDFLAGS and CPPFLAGS
LDFLAGS="$_SAVE_LDFLAGS"
CPPFLAGS="$_SAVE_CPPFLAGS"

dnl Suppress clang's warnings about unused CFLAGS and CXXFLAGS
if test "x$acv_mesa_CLANG" = xyes; then
    CFLAGS="$CFLAGS -Qunused-arguments"
    CXXFLAGS="$CXXFLAGS -Qunused-arguments"
fi

dnl Add user CFLAGS and CXXFLAGS
CFLAGS="$CFLAGS $USER_CFLAGS"
CXXFLAGS="$CXXFLAGS $USER_CXXFLAGS"

dnl Substitute the config
AC_CONFIG_FILES([Makefile
		src/Makefile
		src/egl/drivers/dri2/Makefile
		src/egl/main/Makefile
		src/egl/main/egl.pc
		src/egl/wayland/Makefile
		src/egl/wayland/wayland-drm/Makefile
		src/egl/wayland/wayland-egl/Makefile
		src/egl/wayland/wayland-egl/wayland-egl.pc
		src/gallium/Makefile
		src/gallium/auxiliary/Makefile
		src/gallium/auxiliary/pipe-loader/Makefile
		src/gallium/drivers/freedreno/Makefile
		src/gallium/drivers/galahad/Makefile
		src/gallium/drivers/i915/Makefile
		src/gallium/drivers/identity/Makefile
		src/gallium/drivers/ilo/Makefile
		src/gallium/drivers/llvmpipe/Makefile
		src/gallium/drivers/noop/Makefile
		src/gallium/drivers/nouveau/Makefile
		src/gallium/drivers/r300/Makefile
		src/gallium/drivers/r600/Makefile
		src/gallium/drivers/radeon/Makefile
		src/gallium/drivers/radeonsi/Makefile
		src/gallium/drivers/rbug/Makefile
		src/gallium/drivers/softpipe/Makefile
		src/gallium/drivers/svga/Makefile
		src/gallium/drivers/trace/Makefile
		src/gallium/drivers/vc4/Makefile
		src/gallium/state_trackers/clover/Makefile
		src/gallium/state_trackers/dri/Makefile
		src/gallium/state_trackers/egl/Makefile
		src/gallium/state_trackers/gbm/Makefile
		src/gallium/state_trackers/glx/xlib/Makefile
		src/gallium/state_trackers/omx/Makefile
		src/gallium/state_trackers/osmesa/Makefile
		src/gallium/state_trackers/vdpau/Makefile
		src/gallium/state_trackers/vega/Makefile
		src/gallium/state_trackers/xa/Makefile
		src/gallium/state_trackers/xvmc/Makefile
		src/gallium/targets/dri/Makefile
		src/gallium/targets/egl-static/Makefile
		src/gallium/targets/gbm/Makefile
		src/gallium/targets/libgl-xlib/Makefile
		src/gallium/targets/omx/Makefile
		src/gallium/targets/opencl/Makefile
		src/gallium/targets/osmesa/Makefile
		src/gallium/targets/osmesa/osmesa.pc
		src/gallium/targets/pipe-loader/Makefile
		src/gallium/targets/vdpau/Makefile
		src/gallium/targets/xa/Makefile
		src/gallium/targets/xa/xatracker.pc
		src/gallium/targets/xvmc/Makefile
		src/gallium/tests/trivial/Makefile
		src/gallium/tests/unit/Makefile
		src/gallium/winsys/freedreno/drm/Makefile
		src/gallium/winsys/i915/drm/Makefile
		src/gallium/winsys/intel/drm/Makefile
		src/gallium/winsys/nouveau/drm/Makefile
		src/gallium/winsys/radeon/drm/Makefile
		src/gallium/winsys/svga/drm/Makefile
		src/gallium/winsys/sw/dri/Makefile
		src/gallium/winsys/sw/fbdev/Makefile
		src/gallium/winsys/sw/kms-dri/Makefile
		src/gallium/winsys/sw/null/Makefile
		src/gallium/winsys/sw/wayland/Makefile
		src/gallium/winsys/sw/wrapper/Makefile
		src/gallium/winsys/sw/xlib/Makefile
		src/gallium/winsys/vc4/drm/Makefile
		src/gbm/Makefile
		src/gbm/main/gbm.pc
		src/glsl/Makefile
		src/glx/Makefile
		src/glx/apple/Makefile
		src/glx/tests/Makefile
		src/gtest/Makefile
		src/loader/Makefile
		src/mapi/Makefile
		src/mapi/es1api/glesv1_cm.pc
		src/mapi/es2api/glesv2.pc
		src/mapi/glapi/gen/Makefile
		src/mapi/vgapi/Makefile
		src/mapi/vgapi/vg.pc
		src/mesa/Makefile
		src/mesa/gl.pc
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
		src/mesa/main/tests/Makefile
		src/util/Makefile
		src/util/tests/hash_table/Makefile])

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
case "x$enable_osmesa$enable_gallium_osmesa" in
xnoyes)
        echo "        OSMesa:          lib$OSMESA_LIB (Gallium)"
        ;;
xyesno)
        echo "        OSMesa:          lib$OSMESA_LIB"
        ;;
xnono)
        echo "        OSMesa:          no"
        ;;
esac

echo ""
if test "x$enable_dri" != xno; then
        echo "        DRI platform:    $dri_platform"
        if test -z "$DRI_DIRS"; then
            echo "        DRI drivers:     no"
        else
            echo "        DRI drivers:     $DRI_DIRS"
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
    echo "        EGL platforms:   $egl_platforms"

    egl_drivers=""
    if test "x$HAVE_EGL_DRIVER_DRI2" != "x"; then
        egl_drivers="$egl_drivers builtin:egl_dri2"
    fi

    if test "x$enable_gallium_egl" = xyes; then
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
if test -n "$with_gallium_drivers"; then
    echo "        Gallium:         yes"
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
