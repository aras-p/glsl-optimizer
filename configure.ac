dnl Process this file with autoconf to create configure.

AC_PREREQ([2.59])

dnl Versioning - scrape the version from configs/default
m4_define([mesa_version],
    [m4_esyscmd([${MAKE-make} -s -f bin/version.mk version | tr -d '\n'])])
m4_ifval(mesa_version,,
    [m4_fatal([Failed to get the Mesa version from `make -f bin/version.mk version`])])

dnl Tell the user about autoconf.html in the --help output
m4_divert_once([HELP_END], [
See docs/autoconf.html for more details on the options for Mesa.])

AC_INIT([Mesa],[mesa_version],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa])
AC_CONFIG_AUX_DIR([bin])
AC_CANONICAL_HOST

dnl Versions for external dependencies
LIBDRM_REQUIRED=2.4.3
DRI2PROTO_REQUIRED=1.99.3

dnl Check for progs
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_CHECK_PROGS([MAKE], [gmake make])
AC_PATH_PROG([MKDEP], [makedepend])
AC_PATH_PROG([SED], [sed])

dnl We need a POSIX shell for parts of the build. Assume we have one
dnl in most cases.
case "$host_os" in
solaris*)
    # Solaris /bin/sh is too old/non-POSIX compliant
    AC_PATH_PROGS(POSIX_SHELL, [ksh93 ksh sh])
    SHELL="$POSIX_SHELL"
    ;;
esac

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
esac

dnl Add flags for gcc and g++
if test "x$GCC" = xyes; then
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -std=c99 -ffast-math"

    # Work around aliasing bugs - developers should comment this out
    CFLAGS="$CFLAGS -fno-strict-aliasing"
fi
if test "x$GXX" = xyes; then
    CXXFLAGS="$CXXFLAGS -Wall"

    # Work around aliasing bugs - developers should comment this out
    CXXFLAGS="$CXXFLAGS -fno-strict-aliasing"
fi

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
if test "$enable_static" = yes; then
    LIB_EXTENSION='a'
else
    case "$host_os" in
    darwin* )
        LIB_EXTENSION='dylib' ;;
    * )
        LIB_EXTENSION='so' ;;
    esac
fi

GL_LIB_NAME='lib$(GL_LIB).'${LIB_EXTENSION}
GLU_LIB_NAME='lib$(GLU_LIB).'${LIB_EXTENSION}
GLUT_LIB_NAME='lib$(GLUT_LIB).'${LIB_EXTENSION}
GLW_LIB_NAME='lib$(GLW_LIB).'${LIB_EXTENSION}
OSMESA_LIB_NAME='lib$(OSMESA_LIB).'${LIB_EXTENSION}

GL_LIB_GLOB='lib$(GL_LIB).*'${LIB_EXTENSION}'*'
GLU_LIB_GLOB='lib$(GLU_LIB).*'${LIB_EXTENSION}'*'
GLUT_LIB_GLOB='lib$(GLUT_LIB).*'${LIB_EXTENSION}'*'
GLW_LIB_GLOB='lib$(GLW_LIB).*'${LIB_EXTENSION}'*'
OSMESA_LIB_GLOB='lib$(OSMESA_LIB).*'${LIB_EXTENSION}'*'

AC_SUBST([GL_LIB_NAME])
AC_SUBST([GLU_LIB_NAME])
AC_SUBST([GLUT_LIB_NAME])
AC_SUBST([GLW_LIB_NAME])
AC_SUBST([OSMESA_LIB_NAME])

AC_SUBST([GL_LIB_GLOB])
AC_SUBST([GLU_LIB_GLOB])
AC_SUBST([GLUT_LIB_GLOB])
AC_SUBST([GLW_LIB_GLOB])
AC_SUBST([OSMESA_LIB_GLOB])

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
        linux* | *freebsd* | dragonfly*)
            test "x$enable_64bit" = xyes && asm_arch=x86_64 || asm_arch=x86
            ;;
        esac
        ;;
    x86_64)
        case "$host_os" in
        linux* | *freebsd* | dragonfly*)
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

dnl
dnl Driver configuration. Options are xlib, dri and osmesa right now.
dnl More later: directfb, fbdev, ...
dnl
default_driver="xlib"

case "$host_os" in
linux*)
    case "$host_cpu" in
    i*86|x86_64|powerpc*|sparc*) default_driver="dri";;
    esac
    ;;
*freebsd* | dragonfly*)
    case "$host_cpu" in
    i*86|x86_64|powerpc*|sparc*) default_driver="dri";;
    esac
    ;;
esac

AC_ARG_WITH([driver],
    [AS_HELP_STRING([--with-driver=DRIVER],
        [driver for Mesa: xlib,dri,osmesa @<:@default=dri when available, or xlib@:>@])],
    [mesa_driver="$withval"],
    [mesa_driver="$default_driver"])
dnl Check for valid option
case "x$mesa_driver" in
xxlib|xdri|xosmesa)
    ;;
*)
    AC_MSG_ERROR([Driver '$mesa_driver' is not a valid option])
    ;;
esac

dnl
dnl Driver specific build directories
dnl
SRC_DIRS="mesa glew"
GLU_DIRS="sgi"
WINDOW_SYSTEM=""
GALLIUM_DIRS="auxiliary drivers state_trackers"
GALLIUM_WINSYS_DIRS=""
GALLIUM_WINSYS_DRM_DIRS=""
GALLIUM_AUXILIARY_DIRS="draw translate cso_cache pipebuffer tgsi sct rtasm util indices"
GALLIUM_DRIVERS_DIRS="softpipe failover trace"
GALLIUM_STATE_TRACKERS_DIRS=""

case "$mesa_driver" in
xlib)
    DRIVER_DIRS="x11"
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS xlib"
    ;;
dri)
    SRC_DIRS="glx/x11 $SRC_DIRS"
    DRIVER_DIRS="dri"
    WINDOW_SYSTEM="dri"
    GALLIUM_WINSYS_DIRS="$GALLIUM_WINSYS_DIRS drm"
    ;;
osmesa)
    DRIVER_DIRS="osmesa"
    ;;
esac
AC_SUBST([SRC_DIRS])
AC_SUBST([GLU_DIRS])
AC_SUBST([DRIVER_DIRS])
AC_SUBST([WINDOW_SYSTEM])
AC_SUBST([GALLIUM_DIRS])
AC_SUBST([GALLIUM_WINSYS_DIRS])
AC_SUBST([GALLIUM_WINSYS_DRM_DIRS])
AC_SUBST([GALLIUM_DRIVERS_DIRS])
AC_SUBST([GALLIUM_AUXILIARY_DIRS])
AC_SUBST([GALLIUM_STATE_TRACKERS_DIRS])

dnl
dnl User supplied program configuration
dnl
if test -d "$srcdir/progs/demos"; then
    default_demos=yes
else
    default_demos=no
fi
AC_ARG_WITH([demos],
    [AS_HELP_STRING([--with-demos@<:@=DIRS...@:>@],
        [optional comma delimited demo directories to build
        @<:@default=auto if source available@:>@])],
    [with_demos="$withval"],
    [with_demos="$default_demos"])
if test "x$with_demos" = x; then
    with_demos=no
fi

dnl If $with_demos is yes, directories will be added as libs available
PROGRAM_DIRS=""
case "$with_demos" in
no) ;;
yes)
    # If the driver isn't osmesa, we have libGL and can build xdemos
    if test "$mesa_driver" != osmesa; then
        PROGRAM_DIRS="xdemos"
    fi
    ;;
*)
    # verify the requested demos directories exist
    demos=`IFS=,; echo $with_demos`
    for demo in $demos; do
        test -d "$srcdir/progs/$demo" || \
            AC_MSG_ERROR([Program directory '$demo' doesn't exist])
    done
    PROGRAM_DIRS="$demos"
    ;;
esac

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
if test "$x11_pkgconfig" = no; then
    AC_PATH_XTRA
fi

dnl Try to tell the user that the --x-* options are only used when
dnl pkg-config is not available. This must be right after AC_PATH_XTRA.
m4_divert_once([HELP_BEGIN],
[These options are only used when the X libraries cannot be found by the
pkg-config utility.])

dnl We need X for xlib and dri, so bomb now if it's not found
case "$mesa_driver" in
xlib|dri)
    if test "$no_x" = yes; then
        AC_MSG_ERROR([X11 development libraries needed for $mesa_driver driver])
    fi
    ;;
esac

dnl XCB - this is only used for GLX right now
AC_ARG_ENABLE([xcb],
    [AS_HELP_STRING([--enable-xcb],
        [use XCB for GLX @<:@default=disabled@:>@])],
    [enable_xcb="$enableval"],
    [enable_xcb=no])
if test "x$enable_xcb" = xyes; then
    DEFINES="$DEFINES -DUSE_XCB"
else
    enable_xcb=no
fi

dnl
dnl libGL configuration per driver
dnl
case "$mesa_driver" in
xlib)
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
    GL_LIB_DEPS="$GL_LIB_DEPS $SELINUX_LIBS -lm -lpthread"
    GL_PC_LIB_PRIV="$GL_PC_LIB_PRIV $SELINUX_LIBS -lm -lpthread"

    # if static, move the external libraries to the programs
    # and empty the libraries for libGL
    if test "$enable_static" = yes; then
        APP_LIB_DEPS="$APP_LIB_DEPS $GL_LIB_DEPS"
        GL_LIB_DEPS=""
    fi
    ;;
dri)
    # DRI must be shared, I think
    if test "$enable_static" = yes; then
        AC_MSG_ERROR([Can't use static libraries for DRI drivers])
    fi

    # Check for libdrm
    PKG_CHECK_MODULES([LIBDRM], [libdrm >= $LIBDRM_REQUIRED])
    PKG_CHECK_MODULES([DRI2PROTO], [dri2proto >= $DRI2PROTO_REQUIRED])
    GL_PC_REQ_PRIV="libdrm >= $LIBDRM_REQUIRED dri2proto >= $DRI2PROTO_REQUIRED"
    DRI_PC_REQ_PRIV="libdrm >= $LIBDRM_REQUIRED"

    # find the DRI deps for libGL
    if test "$x11_pkgconfig" = yes; then
        # add xcb modules if necessary
        dri_modules="x11 xext xxf86vm xdamage xfixes"
        if test "$enable_xcb" = yes; then
            dri_modules="$dri_modules x11-xcb xcb-glx"
        fi

        PKG_CHECK_MODULES([DRIGL], [$dri_modules])
        GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV $dri_modules"
        X11_INCLUDES="$X11_INCLUDES $DRIGL_CFLAGS"
        GL_LIB_DEPS="$DRIGL_LIBS"
    else
        # should check these...
        X11_INCLUDES="$X11_INCLUDES $X_CFLAGS"
        GL_LIB_DEPS="$X_LIBS -lX11 -lXext -lXxf86vm -lXdamage -lXfixes"
        GL_PC_LIB_PRIV="$GL_LIB_DEPS"
        GL_PC_CFLAGS="$X11_INCLUDES"

        # XCB can only be used from pkg-config
        if test "$enable_xcb" = yes; then
            PKG_CHECK_MODULES([XCB],[x11-xcb xcb-glx])
            GL_PC_REQ_PRIV="$GL_PC_REQ_PRIV x11-xcb xcb-glx"
            X11_INCLUDES="$X11_INCLUDES $XCB_CFLAGS"
            GL_LIB_DEPS="$GL_LIB_DEPS $XCB_LIBS"
        fi
    fi

    # need DRM libs, -lpthread, etc.
    GL_LIB_DEPS="$GL_LIB_DEPS $LIBDRM_LIBS -lm -lpthread $DLOPEN_LIBS"
    GL_PC_LIB_PRIV="-lm -lpthread $DLOPEN_LIBS"
    ;;
osmesa)
    # No libGL for osmesa
    GL_LIB_DEPS=""
    ;;
esac
AC_SUBST([GL_LIB_DEPS])
AC_SUBST([GL_PC_REQ_PRIV])
AC_SUBST([GL_PC_LIB_PRIV])
AC_SUBST([GL_PC_CFLAGS])
AC_SUBST([DRI_PC_REQ_PRIV])

dnl
dnl More X11 setup
dnl
if test "$mesa_driver" = xlib; then
    DEFINES="$DEFINES -DUSE_XSHM"
fi

dnl
dnl More DRI setup
dnl
AC_ARG_ENABLE([glx-tls],
    [AS_HELP_STRING([--enable-glx-tls],
        [enable TLS support in GLX @<:@default=disabled@:>@])],
    [GLX_USE_TLS="$enableval"],
    [GLX_USE_TLS=no])
dnl Directory for DRI drivers
AC_ARG_WITH([dri-driverdir],
    [AS_HELP_STRING([--with-dri-driverdir=DIR],
        [directory for the DRI drivers @<:@${libdir}/dri@:>@])],
    [DRI_DRIVER_INSTALL_DIR="$withval"],
    [DRI_DRIVER_INSTALL_DIR='${libdir}/dri'])
AC_SUBST([DRI_DRIVER_INSTALL_DIR])
dnl Direct rendering or just indirect rendering
AC_ARG_ENABLE([driglx-direct],
    [AS_HELP_STRING([--disable-driglx-direct],
        [enable direct rendering in GLX for DRI @<:@default=enabled@:>@])],
    [driglx_direct="$enableval"],
    [driglx_direct="yes"])
dnl Which drivers to build - default is chosen by platform
AC_ARG_WITH([dri-drivers],
    [AS_HELP_STRING([--with-dri-drivers@<:@=DIRS...@:>@],
        [comma delimited DRI drivers list, e.g.
        "swrast,i965,radeon,nouveau" @<:@default=auto@:>@])],
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
    DRI_DIRS="yes"
    ;;
*)
    # verify the requested driver directories exist
    dri_drivers=`IFS=', '; echo $with_dri_drivers`
    for driver in $dri_drivers; do
        test -d "$srcdir/src/mesa/drivers/dri/$driver" || \
            AC_MSG_ERROR([DRI driver directory '$driver' doesn't exist])
    done
    DRI_DIRS="$dri_drivers"
    ;;
esac

dnl Just default to no EGL for now
USING_EGL=0
AC_SUBST([USING_EGL])

dnl Set DRI_DIRS, DEFINES and LIB_DEPS
if test "$mesa_driver" = dri; then
    # Use TLS in GLX?
    if test "x$GLX_USE_TLS" = xyes; then
        DEFINES="$DEFINES -DGLX_USE_TLS -DPTHREADS"
    fi

    if test "x$USING_EGL" = x1; then
        PROGRAM_DIRS="egl"
    fi

    # Platform specific settings and drivers to build
    case "$host_os" in
    linux*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        if test "x$driglx_direct" = xyes; then
            DEFINES="$DEFINES -DGLX_DIRECT_RENDERING"
        fi
        DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING -DHAVE_ALIAS"

        case "$host_cpu" in
        x86_64)
            # ffb, gamma, and sis are missing because they have not be
            # converted to use the new interface.  i810 are missing
            # because there is no x86-64 system where they could *ever*
            # be used.
            if test "x$DRI_DIRS" = "xyes"; then
                DRI_DIRS="i915 i965 mach64 mga r128 r200 r300 radeon \
                    savage tdfx unichrome swrast"
            fi
            ;;
        powerpc*)
            # Build only the drivers for cards that exist on PowerPC.
            # At some point MGA will be added, but not yet.
            if test "x$DRI_DIRS" = "xyes"; then
                DRI_DIRS="mach64 r128 r200 r300 radeon tdfx swrast"
            fi
            ;;
        sparc*)
            # Build only the drivers for cards that exist on sparc`
            if test "x$DRI_DIRS" = "xyes"; then
                DRI_DIRS="mach64 r128 r200 r300 radeon ffb swrast"
            fi
            ;;
        esac
        ;;
    freebsd* | dragonfly*)
        DEFINES="$DEFINES -DPTHREADS -DUSE_EXTERNAL_DXTN_LIB=1"
        DEFINES="$DEFINES -DIN_DRI_DRIVER -DHAVE_ALIAS"
        DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING"
        if test "x$driglx_direct" = xyes; then
            DEFINES="$DEFINES -DGLX_DIRECT_RENDERING"
        fi
        if test "x$GXX" = xyes; then
            CXXFLAGS="$CXXFLAGS -ansi -pedantic"
        fi

        # ffb and gamma are missing because they have not been converted
        # to use the new interface.
        if test "x$DRI_DIRS" = "xyes"; then
            DRI_DIRS="i810 i915 i965 mach64 mga r128 r200 r300 radeon tdfx \
                unichrome savage sis swrast"
        fi
        ;;
    gnu*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING -DHAVE_ALIAS"
	;;
    solaris*)
        DEFINES="$DEFINES -DUSE_EXTERNAL_DXTN_LIB=1 -DIN_DRI_DRIVER"
        DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING"
        if test "x$driglx_direct" = xyes; then
            DEFINES="$DEFINES -DGLX_DIRECT_RENDERING"
        fi
        ;;
    esac

    # default drivers
    if test "x$DRI_DIRS" = "xyes"; then
        DRI_DIRS="i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
            savage sis tdfx trident unichrome ffb swrast"
    fi

    DRI_DIRS=`echo "$DRI_DIRS" | $SED 's/  */ /g'`

    # Check for expat
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

    # put all the necessary libs together
    DRI_LIB_DEPS="$SELINUX_LIBS $LIBDRM_LIBS $EXPAT_LIB -lm -lpthread $DLOPEN_LIBS"
fi
AC_SUBST([DRI_DIRS])
AC_SUBST([EXPAT_INCLUDES])
AC_SUBST([DRI_LIB_DEPS])

dnl
dnl OSMesa configuration
dnl
if test "$mesa_driver" = xlib; then
    default_gl_osmesa=yes
else
    default_gl_osmesa=no
fi
AC_ARG_ENABLE([gl-osmesa],
    [AS_HELP_STRING([--enable-gl-osmesa],
        [enable OSMesa on libGL @<:@default=enabled for xlib driver@:>@])],
    [gl_osmesa="$enableval"],
    [gl_osmesa="$default_gl_osmesa"])
if test "x$gl_osmesa" = xyes; then
    if test "$mesa_driver" = osmesa; then
        AC_MSG_ERROR([libGL is not available for OSMesa driver])
    else
        DRIVER_DIRS="$DRIVER_DIRS osmesa"
    fi
fi

dnl Configure the channel bits for OSMesa (libOSMesa, libOSMesa16, ...)
AC_ARG_WITH([osmesa-bits],
    [AS_HELP_STRING([--with-osmesa-bits=BITS],
        [OSMesa channel bits and library name: 8, 16, 32 @<:@default=8@:>@])],
    [osmesa_bits="$withval"],
    [osmesa_bits=8])
if test "$mesa_driver" != osmesa && test "x$osmesa_bits" != x8; then
    AC_MSG_WARN([Ignoring OSMesa channel bits for non-OSMesa driver])
    osmesa_bits=8
fi
case "x$osmesa_bits" in
x8)
    OSMESA_LIB=OSMesa
    ;;
x16|x32)
    OSMESA_LIB="OSMesa$osmesa_bits"
    DEFINES="$DEFINES -DCHAN_BITS=$osmesa_bits -DDEFAULT_SOFTWARE_DEPTH_BITS=31"
    ;;
*)
    AC_MSG_ERROR([OSMesa bits '$osmesa_bits' is not a valid option])
    ;;
esac
AC_SUBST([OSMESA_LIB])

case "$mesa_driver" in
osmesa)
    # only link libraries with osmesa if shared
    if test "$enable_static" = no; then
        OSMESA_LIB_DEPS="-lm -lpthread $SELINUX_LIBS $DLOPEN_LIBS"
    else
        OSMESA_LIB_DEPS=""
    fi
    OSMESA_MESA_DEPS=""
    OSMESA_PC_LIB_PRIV="-lm -lpthread $SELINUX_LIBS $DLOPEN_LIBS"
    ;;
*)
    # Link OSMesa to libGL otherwise
    OSMESA_LIB_DEPS=""
    # only link libraries with osmesa if shared
    if test "$enable_static" = no; then
        OSMESA_MESA_DEPS='-l$(GL_LIB)'
    else
        OSMESA_MESA_DEPS=""
    fi
    OSMESA_PC_REQ="gl"
    ;;
esac
OSMESA_PC_LIB_PRIV="$OSMESA_PC_LIB_PRIV"
AC_SUBST([OSMESA_LIB_DEPS])
AC_SUBST([OSMESA_MESA_DEPS])
AC_SUBST([OSMESA_PC_REQ])
AC_SUBST([OSMESA_PC_LIB_PRIV])

dnl
dnl EGL configuration
dnl
AC_ARG_ENABLE([egl],
    [AS_HELP_STRING([--disable-egl],
        [disable EGL library @<:@default=enabled@:>@])],
    [enable_egl="$enableval"],
    [enable_egl=yes])
if test "x$enable_egl" = xyes; then
    SRC_DIRS="$SRC_DIRS egl"

    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES([EGL], [x11])
        EGL_LIB_DEPS="$EGL_LIBS"
    else
        # should check these...
        EGL_LIB_DEPS="$X_LIBS -lX11"
    fi
    EGL_LIB_DEPS="$EGL_LIB_DEPS $DLOPEN_LIBS"
fi
AC_SUBST([EGL_LIB_DEPS])

dnl
dnl GLU configuration
dnl
AC_ARG_ENABLE([glu],
    [AS_HELP_STRING([--disable-glu],
        [enable OpenGL Utility library @<:@default=enabled@:>@])],
    [enable_glu="$enableval"],
    [enable_glu=yes])
if test "x$enable_glu" = xyes; then
    SRC_DIRS="$SRC_DIRS glu"

    case "$mesa_driver" in
    osmesa)
        # If GLU is available and we have libOSMesa (not 16 or 32),
        # we can build the osdemos
        if test "$with_demos" = yes && test "$osmesa_bits" = 8; then
            PROGRAM_DIRS="$PROGRAM_DIRS osdemos"
        fi

        # Link libGLU to libOSMesa instead of libGL
        GLU_LIB_DEPS=""
        GLU_PC_REQ="osmesa"
        if test "$enable_static" = no; then
            GLU_MESA_DEPS='-l$(OSMESA_LIB)'
        else
            GLU_MESA_DEPS=""
        fi
        ;;
    *)
        # If static, empty GLU_LIB_DEPS and add libs for programs to link
        GLU_PC_REQ="gl"
        GLU_PC_LIB_PRIV="-lm"
        if test "$enable_static" = no; then
            GLU_LIB_DEPS="-lm"
            GLU_MESA_DEPS='-l$(GL_LIB)'
        else
            GLU_LIB_DEPS=""
            GLU_MESA_DEPS=""
            APP_LIB_DEPS="$APP_LIB_DEPS -lstdc++"
        fi
        ;;
    esac
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

dnl
dnl GLw configuration
dnl
AC_ARG_ENABLE([glw],
    [AS_HELP_STRING([--disable-glw],
        [enable Xt/Motif widget library @<:@default=enabled@:>@])],
    [enable_glw="$enableval"],
    [enable_glw=yes])
dnl Don't build GLw on osmesa
if test "x$enable_glw" = xyes && test "$mesa_driver" = osmesa; then
    AC_MSG_WARN([Disabling GLw since the driver is OSMesa])
    enable_glw=no
fi
AC_ARG_ENABLE([motif],
    [AS_HELP_STRING([--enable-motif],
        [use Motif widgets in GLw @<:@default=disabled@:>@])],
    [enable_motif="$enableval"],
    [enable_motif=no])

if test "x$enable_glw" = xyes; then
    SRC_DIRS="$SRC_DIRS glw"
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES([GLW],[x11 xt])
        GLW_PC_REQ_PRIV="x11 xt"
        GLW_LIB_DEPS="$GLW_LIBS"
    else
        # should check these...
        GLW_LIB_DEPS="$X_LIBS -lXt -lX11"
        GLW_PC_LIB_PRIV="$GLW_LIB_DEPS"
        GLW_PC_CFLAGS="$X11_INCLUDES"
    fi

    GLW_SOURCES="GLwDrawA.c"
    MOTIF_CFLAGS=
    if test "x$enable_motif" = xyes; then
        GLW_SOURCES="$GLW_SOURCES GLwMDrawA.c"
        AC_PATH_PROG([MOTIF_CONFIG], [motif-config], [no])
        if test "x$MOTIF_CONFIG" != xno; then
            MOTIF_CFLAGS=`$MOTIF_CONFIG --cflags`
            MOTIF_LIBS=`$MOTIF_CONFIG --libs`
        else
            AC_CHECK_HEADER([Xm/PrimitiveP.h], [],
                [AC_MSG_ERROR([Can't locate Motif headers])])
            AC_CHECK_LIB([Xm], [XmGetPixmap], [MOTIF_LIBS="-lXm"],
                [AC_MSG_ERROR([Can't locate Motif Xm library])])
        fi
        # MOTIF_LIBS is prepended to GLW_LIB_DEPS since Xm needs Xt/X11
        GLW_LIB_DEPS="$MOTIF_LIBS $GLW_LIB_DEPS"
        GLW_PC_LIB_PRIV="$MOTIF_LIBS $GLW_PC_LIB_PRIV"
        GLW_PC_CFLAGS="$MOTIF_CFLAGS $GLW_PC_CFLAGS"
    fi

    # If static, empty GLW_LIB_DEPS and add libs for programs to link
    GLW_PC_LIB_PRIV="$GLW_PC_LIB_PRIV"
    if test "$enable_static" = no; then
        GLW_MESA_DEPS='-l$(GL_LIB)'
        GLW_LIB_DEPS="$GLW_LIB_DEPS"
    else
        APP_LIB_DEPS="$APP_LIB_DEPS $GLW_LIB_DEPS"
        GLW_LIB_DEPS=""
        GLW_MESA_DEPS=""
    fi
fi
AC_SUBST([GLW_LIB_DEPS])
AC_SUBST([GLW_MESA_DEPS])
AC_SUBST([GLW_SOURCES])
AC_SUBST([MOTIF_CFLAGS])
AC_SUBST([GLW_PC_REQ_PRIV])
AC_SUBST([GLW_PC_LIB_PRIV])
AC_SUBST([GLW_PC_CFLAGS])

dnl
dnl GLUT configuration
dnl
if test -f "$srcdir/include/GL/glut.h"; then
    default_glut=yes
else
    default_glut=no
fi
AC_ARG_ENABLE([glut],
    [AS_HELP_STRING([--disable-glut],
        [enable GLUT library @<:@default=enabled if source available@:>@])],
    [enable_glut="$enableval"],
    [enable_glut="$default_glut"])

dnl Can't build glut if GLU not available
if test "x$enable_glu$enable_glut" = xnoyes; then
    AC_MSG_WARN([Disabling glut since GLU is disabled])
    enable_glut=no
fi
dnl Don't build glut on osmesa
if test "x$enable_glut" = xyes && test "$mesa_driver" = osmesa; then
    AC_MSG_WARN([Disabling glut since the driver is OSMesa])
    enable_glut=no
fi

if test "x$enable_glut" = xyes; then
    SRC_DIRS="$SRC_DIRS glut/glx"
    GLUT_CFLAGS=""
    if test "x$GCC" = xyes; then
        GLUT_CFLAGS="-fexceptions"
    fi
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES([GLUT],[x11 xmu xi])
        GLUT_PC_REQ_PRIV="x11 xmu xi"
        GLUT_LIB_DEPS="$GLUT_LIBS"
    else
        # should check these...
        GLUT_LIB_DEPS="$X_LIBS -lX11 -lXmu -lXi"
        GLUT_PC_LIB_PRIV="$GLUT_LIB_DEPS"
        GLUT_PC_CFLAGS="$X11_INCLUDES"
    fi
    GLUT_LIB_DEPS="$GLUT_LIB_DEPS -lm"
    GLUT_PC_LIB_PRIV="$GLUT_PC_LIB_PRIV -lm"

    # If glut is available, we can build most programs
    if test "$with_demos" = yes; then
        PROGRAM_DIRS="$PROGRAM_DIRS demos redbook samples glsl"
    fi

    # If static, empty GLUT_LIB_DEPS and add libs for programs to link
    if test "$enable_static" = no; then
        GLUT_MESA_DEPS='-l$(GLU_LIB) -l$(GL_LIB)'
    else
        APP_LIB_DEPS="$APP_LIB_DEPS $GLUT_LIB_DEPS"
        GLUT_LIB_DEPS=""
        GLUT_MESA_DEPS=""
    fi
fi
AC_SUBST([GLUT_LIB_DEPS])
AC_SUBST([GLUT_MESA_DEPS])
AC_SUBST([GLUT_CFLAGS])
AC_SUBST([GLUT_PC_REQ_PRIV])
AC_SUBST([GLUT_PC_LIB_PRIV])
AC_SUBST([GLUT_PC_CFLAGS])

dnl
dnl Program library dependencies
dnl    Only libm is added here if necessary as the libraries should
dnl    be pulled in by the linker
dnl
if test "x$APP_LIB_DEPS" = x; then
    case "$host_os" in
    solaris*)
        APP_LIB_DEPS="-lX11 -lsocket -lnsl -lm"
        ;;
    *)
        APP_LIB_DEPS="-lm"
        ;;
    esac
fi
AC_SUBST([APP_LIB_DEPS])
AC_SUBST([PROGRAM_DIRS])

dnl
dnl Gallium configuration
dnl
AC_ARG_ENABLE([gallium],
    [AS_HELP_STRING([--disable-gallium],
        [build gallium @<:@default=enabled@:>@])],
    [enable_gallium="$enableval"],
    [enable_gallium=yes])
if test "x$enable_gallium" = xyes; then
    SRC_DIRS="$SRC_DIRS gallium gallium/winsys"
fi

dnl
dnl Gallium state trackers configuration
dnl
AC_ARG_WITH([state-trackers],
    [AS_HELP_STRING([--with-state-trackers@<:@=DIRS...@:>@],
        [comma delimited state_trackers list, e.g.
        "egl,glx" @<:@default=auto@:>@])],
    [with_state_trackers="$withval"],
    [with_state_trackers=yes])

case "$with_state_trackers" in
no)
    GALLIUM_STATE_TRACKERS_DIRS=""
    ;;
yes)
    # look at what else is built
    case "$mesa_driver" in
    xlib)
        GALLIUM_STATE_TRACKERS_DIRS=glx
        ;;
    dri)
        test "x$enable_egl" = xyes && GALLIUM_STATE_TRACKERS_DIRS=egl
        ;;
    esac
    ;;
*)
    # verify the requested state tracker exist
    state_trackers=`IFS=', '; echo $with_state_trackers`
    for tracker in $state_trackers; do
        test -d "$srcdir/src/gallium/state_trackers/$tracker" || \
            AC_MSG_ERROR([state tracker '$tracker' doesn't exist])

        if test "$tracker" = egl && test "x$enable_egl" != xyes; then
            AC_MSG_ERROR([cannot build egl state tracker without EGL library])
        fi
    done
    GALLIUM_STATE_TRACKERS_DIRS="$state_trackers"
    ;;
esac

AC_ARG_WITH([xorg-driver-dir],
    [AS_HELP_STRING([--with-xorg-driver-dir=DIR],
                    [Default xorg driver directory[[default=${libdir}/xorg/modules/drivers]]])],
    [XORG_DRIVER_INSTALL_DIR="$withval"],
    [XORG_DRIVER_INSTALL_DIR="${libdir}/xorg/modules/drivers"])
AC_SUBST([XORG_DRIVER_INSTALL_DIR])

dnl
dnl Gallium Intel configuration
dnl
AC_ARG_ENABLE([gallium-intel],
    [AS_HELP_STRING([--disable-gallium-intel],
        [build gallium intel @<:@default=enabled@:>@])],
    [enable_gallium_intel="$enableval"],
    [enable_gallium_intel=yes])
if test "x$enable_gallium_intel" = xyes; then
    GALLIUM_WINSYS_DRM_DIRS="$GALLIUM_WINSYS_DRM_DIRS intel"
    GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS i915simple"
fi

dnl
dnl Gallium Radeon configuration
dnl
AC_ARG_ENABLE([gallium-radeon],
    [AS_HELP_STRING([--enable-gallium-radeon],
        [build gallium radeon @<:@default=disabled@:>@])],
    [enable_gallium_radeon="$enableval"],
    [enable_gallium_radeon=no])
if test "x$enable_gallium_radeon" = xyes; then
    GALLIUM_WINSYS_DRM_DIRS="$GALLIUM_WINSYS_DRM_DIRS radeon"
    GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS r300"
fi

dnl
dnl Gallium Radeon configuration
dnl
AC_ARG_ENABLE([gallium-nouveau],
    [AS_HELP_STRING([--enable-gallium-nouveau],
        [build gallium nouveau @<:@default=disabled@:>@])],
    [enable_gallium_nouveau="$enableval"],
    [enable_gallium_nouveau=no])
if test "x$enable_gallium_nouveau" = xyes; then
    GALLIUM_WINSYS_DRM_DIRS="$GALLIUM_WINSYS_DRM_DIRS nouveau"
    GALLIUM_DRIVERS_DIRS="$GALLIUM_DRIVERS_DIRS nv04 nv10 nv20 nv30 nv40 nv50"
fi


dnl Restore LDFLAGS and CPPFLAGS
LDFLAGS="$_SAVE_LDFLAGS"
CPPFLAGS="$_SAVE_CPPFLAGS"

dnl Substitute the config
AC_CONFIG_FILES([configs/autoconf])

dnl Replace the configs/current symlink
AC_CONFIG_COMMANDS([configs],[
if test -f configs/current || test -L configs/current; then
    rm -f configs/current
fi
ln -s autoconf configs/current
])

AC_OUTPUT

dnl
dnl Output some configuration info for the user
dnl
echo ""
echo "        prefix:          $prefix"
echo "        exec_prefix:     $exec_prefix"
echo "        libdir:          $libdir"
echo "        includedir:      $includedir"

dnl Driver info
echo ""
echo "        Driver:          $mesa_driver"
if echo "$DRIVER_DIRS" | grep 'osmesa' >/dev/null 2>&1; then
    echo "        OSMesa:          lib$OSMESA_LIB"
else
    echo "        OSMesa:          no"
fi
if test "$mesa_driver" = dri; then
    # cleanup the drivers var
    dri_dirs=`echo $DRI_DIRS | $SED 's/^ *//;s/  */ /;s/ *$//'`
if test "x$DRI_DIRS" = x; then
    echo "        DRI drivers:     no"
else
    echo "        DRI drivers:     $dri_dirs"
fi
    echo "        DRI driver dir:  $DRI_DRIVER_INSTALL_DIR"
fi
echo "        Use XCB:         $enable_xcb"

echo ""
if echo "$SRC_DIRS" | grep 'gallium' >/dev/null 2>&1; then
    echo "        Gallium:         yes"
    echo "        Gallium dirs:    $GALLIUM_DIRS"
    echo "        Winsys dirs:     $GALLIUM_WINSYS_DIRS"
    echo "        Winsys drm dirs:$GALLIUM_WINSYS_DRM_DIRS"
    echo "        Auxiliary dirs:  $GALLIUM_AUXILIARY_DIRS"
    echo "        Driver dirs:     $GALLIUM_DRIVERS_DIRS"
    echo "        Trackers dirs:   $GALLIUM_STATE_TRACKERS_DIRS"
else
    echo "        Gallium:         no"
fi

dnl Libraries
echo ""
echo "        Shared libs:     $enable_shared"
echo "        Static libs:     $enable_static"
echo "        EGL:             $enable_egl"
echo "        GLU:             $enable_glu"
echo "        GLw:             $enable_glw (Motif: $enable_motif)"
echo "        glut:            $enable_glut"

dnl Programs
# cleanup the programs var for display
program_dirs=`echo $PROGRAM_DIRS | $SED 's/^ *//;s/  */ /;s/ *$//'`
if test "x$program_dirs" = x; then
    echo "        Demos:           no"
else
    echo "        Demos:           $program_dirs"
fi

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
echo "        Run '${MAKE-make}' to build Mesa"
echo ""
