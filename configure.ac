dnl Process this file with autoconf to create configure.

AC_PREREQ(2.59)

dnl Versioning
dnl Make version number available to autoconf and configure
m4_define(mesa_major, 7)
m4_define(mesa_minor, 1)
m4_define(mesa_tiny, 0)
m4_define(mesa_version, [mesa_major().mesa_minor().mesa_tiny()])

AC_INIT(Mesa, mesa_version(), mesa3d@sourceforge.net)
AC_CONFIG_AUX_DIR(bin)
AC_CANONICAL_HOST

dnl Substitute the version number into shell variables
MESA_MAJOR=mesa_major()
MESA_MINOR=mesa_minor()
MESA_TINY=mesa_tiny()
AC_SUBST(MESA_MAJOR)
AC_SUBST(MESA_MINOR)
AC_SUBST(MESA_TINY)

dnl Check for progs
AC_PROG_CPP
AC_PROG_CC
AC_PROG_CXX
AC_PATH_PROG(MAKE, make)
AC_PATH_PROG(MKDEP, makedepend)
AC_PATH_PROG(SED, sed)
PKG_PROG_PKG_CONFIG()

dnl LIB_DIR - library basename
LIB_DIR=`echo $libdir | $SED 's%.*/%%'`
AC_SUBST(LIB_DIR)

dnl Cache LDFLAGS so we can add EXTRA_LIB_PATH and restore it later
_SAVE_LDFLAGS="$LDFLAGS"
AC_ARG_VAR(EXTRA_LIB_PATH,[Extra -L paths for the linker])
AC_SUBST(EXTRA_LIB_PATH)

dnl Cache CPPFLAGS so we can add *_INCLUDES and restore it later
_SAVE_CPPFLAGS="$CPPFLAGS"
AC_ARG_VAR(X11_INCLUDES,[Extra -I paths for X11 headers])
AC_SUBST(X11_INCLUDES)

dnl Compiler macros
DEFINES=""
AC_SUBST(DEFINES)
if test "x$GCC" = xyes; then
    DEFINES="-D_POSIX_SOURCE -D_POSIX_C_SOURCE=199309L -D_BSD_SOURCE"
fi
case "$host_os" in
linux*)
    DEFINES="$DEFINES -D_SVID_SOURCE -D_GNU_SOURCE -DPTHREADS -DHAVE_POSIX_MEMALIGN"
    ;;
esac

dnl Add flags for gcc and g++
if test "x$GCC" = xyes; then
    CFLAGS="$CFLAGS -Wall -Wmissing-prototypes -std=c99 -ffast-math"
fi
if test "x$GXX" = xyes; then
    CXXFLAGS="$CXXFLAGS -Wall"
fi

dnl These should be unnecessary, but let the user set them if they want
AC_ARG_VAR(OPT_FLAGS, [Additional optimization flags for the compiler.
    Default is to use CFLAGS.])
AC_ARG_VAR(ARCH_FLAGS, [Additional architecture specific flags for the
    compiler. Default is to use CFLAGS.])
AC_SUBST(OPT_FLAGS)
AC_SUBST(ARCH_FLAGS)

dnl
dnl library names
dnl
GL_LIB_NAME='lib$(GL_LIB).so'
GLU_LIB_NAME='lib$(GLU_LIB).so'
GLUT_LIB_NAME='lib$(GLUT_LIB).so'
GLW_LIB_NAME='lib$(GLW_LIB).so'
OSMESA_LIB_NAME='lib$(OSMESA_LIB).so'
AC_SUBST(GL_LIB_NAME)
AC_SUBST(GLU_LIB_NAME)
AC_SUBST(GLUT_LIB_NAME)
AC_SUBST(GLW_LIB_NAME)
AC_SUBST(OSMESA_LIB_NAME)

dnl
dnl Driver configuration. Options are x11 (Xlib) and dri right now.
dnl More later: osmesa, directfb, fbdev, ...
dnl
AC_ARG_WITH(driver,
    [AS_HELP_STRING([--with-driver=DRIVER],
        [driver for Mesa: x11,dri @<:@default=x11@:>@])],
    mesa_driver="$withval",
    mesa_driver="x11")
dnl Check for valid option
case "x$mesa_driver" in
xx11|xdri)
    ;;
*)
    AC_MSG_ERROR([Driver '$mesa_driver' is not a valid option])
    ;;
esac

dnl
dnl Driver specific build directories
dnl
SRC_DIRS="mesa"
GLU_DIRS="sgi"
DRI_DIRS=""
WINDOW_SYSTEM=""
case "$mesa_driver" in
x11)
    DRIVER_DIRS="x11"
    ;;
dri)
    SRC_DIRS="glx/x11 $SRC_DIRS"
    DRIVER_DIRS="dri"
    WINDOW_SYSTEM="dri"
    ;;
esac
AC_SUBST(SRC_DIRS)
AC_SUBST(GLU_DIRS)
AC_SUBST(DRIVER_DIRS)
AC_SUBST(WINDOW_SYSTEM)

dnl
dnl Find out if X is available. The variables have_x or no_x will be
dnl set and used later in the driver setups
dnl
if test -n "$PKG_CONFIG"; then
    AC_MSG_CHECKING([pkg-config files for X11 are available])
    if $PKG_CONFIG --exists x11; then
        x11_pkgconfig=yes
        have_x=yes
        AC_MSG_RESULT(yes)
    else
        x11_pkgconfig=no
        no_x=yes
        AC_MSG_RESULT(no)
    fi
else
    x11_pkgconfig=no
fi
dnl Use the autoconf macro if no pkg-config files
if test "$x11_pkgconfig" = no; then
    AC_PATH_XTRA
fi

dnl We need X for xlib and dri, so bomb now if it's not found
case "$mesa_driver" in
x11|dri)
    if test "$no_x" = yes; then
        AC_MSG_ERROR([X11 development libraries needed for $mesa_driver driver])
    fi
    ;;
esac

dnl
dnl libGL configuration per driver
dnl
case "$mesa_driver" in
x11)
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES(X11GL, x11 xext)
        X11_INCLUDES="$X11_INCLUDES $X11GL_CFLAGS"
        GL_LIB_DEPS="$X11GL_LIBS"
    else
        # should check these...
        X11_INCLUDES="$X11_INCLUDES $X_CFLAGS"
        GL_LIB_DEPS="$X_LIBS -lX11 -lXext"
    fi
    GL_LIB_DEPS="$GL_LIB_DEPS -lm -lpthread"
    ;;
dri)
    # Check for libdrm
    PKG_CHECK_MODULES(LIBDRM, libdrm)

    # find the DRI deps for libGL
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES(DRIGL, x11 xext xxf86vm xdamage xfixes)
        X11_INCLUDES="$X11_INCLUDES $DRIGL_CFLAGS"
        GL_LIB_DEPS="$DRIGL_LIBS"
    else
        # should check these...
        X11_INCLUDES="$X11_INCLUDES $X_CFLAGS"
        GL_LIB_DEPS="$X_LIBS -lX11 -lXext -lXxf86vm -lXdamage -lXfixes"
    fi

    # need DRM libs, -lpthread, etc.
    GL_LIB_DEPS="$GL_LIB_DEPS $LIBDRM_LIBS -lm -lpthread -ldl"
    ;;
esac
AC_SUBST(GL_LIB_DEPS)

dnl
dnl More X11 setup
dnl
if test "$mesa_driver" = x11; then
    DEFINES="$DEFINES -DUSE_XSHM"
fi

dnl
dnl More DRI setup
dnl
AC_ARG_ENABLE(glx-tls,
    [AS_HELP_STRING([--enable-glx-tls],
        [enable TLS support in GLX @<:@default=no@:>@])],
        GLX_USE_TLS="$enableval",
        GLX_USE_TLS=no)
dnl Directory for DRI drivers
AC_ARG_WITH(dridriverdir,
    [AS_HELP_STRING([--with-dridriverdir=DIR],
        [directory for the DRI drivers @<:@/usr/X11R6/lib/modules/dri@:>@])],
    DRI_DRIVER_INSTALL_DIR="$withval",
    DRI_DRIVER_INSTALL_DIR='/usr/X11R6/lib/modules/dri')
AC_SUBST(DRI_DRIVER_INSTALL_DIR)
dnl Direct rendering or just indirect rendering
AC_ARG_ENABLE(driglx-direct,
    [AS_HELP_STRING([--enable-driglx-direct],
        [enable direct rendering in GLX for DRI @<:@default=yes@:>@])],
        driglx_direct="$enableval",
        driglx_direct="yes")

dnl Just default to no EGL for now
USING_EGL=0
AC_SUBST(USING_EGL)

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
        DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING -DHAVE_ALIAS"
        if test "x$driglx_direct" = xyes; then
            DEFINES="$DEFINES -DGLX_DIRECT_RENDERING"
        fi

        case "$host_cpu" in
        i*86)
            DRI_DIRS="i810 i915tex i915 i965 mach64 mga r128 r200 r300 \
                radeon s3v savage sis tdfx trident unichrome ffb"
            ;;
        x86_64)
            DRI_DIRS="i915tex i915 i965 mach64 mga r128 r200 radeon tdfx \
                unichrome savage r300"
            ;;
        powerpc*)
            DRI_DIRS="mach64 r128 r200 r300 radeon tdfx"
            ;;
        esac
        ;;
    freebsd*)
        DEFINES="$DEFINES -DPTHREADS -DUSE_EXTERNAL_DXTN_LIB=1"
        DEFINES="$DEFINES -DIN_DRI_DRIVER -DHAVE_ALIAS"
        DEFINES="$DEFINES -DGLX_INDIRECT_RENDERING"
        if test "x$driglx_direct" = xyes; then
            DEFINES="$DEFINES -DGLX_DIRECT_RENDERING"
        fi
        if test "x$GXX" = xyes; then
            CXXFLAGS="$CXXFLAGS -ansi -pedantic"
        fi

        DRI_DIRS="i810 i915 i965 mach64 mga r128 r200 r300 radeon tdfx \
            unichrome savage sis"
        ;;
    esac
    DRI_DIRS=`echo "$DRI_DIRS" | $SED 's/  */ /g'`

    # Check for expat
    EXPAT_INCLUDES=""
    EXPAT_LIB=-lexpat
    AC_ARG_WITH(expat, AS_HELP_STRING([--with-expat=DIR],
            [expat install directory]),[
        EXPAT_INCLUDES="-I$withval/include"
        CPPFLAGS="$CPPFLAGS $EXPAT_INCLUDES"
        LDFLAGS="$LDFLAGS -L$withval/$LIB_DIR"
        EXPAT_LIB="-L$withval/$LIB_DIR -lexpat"
        ])
    AC_CHECK_HEADER(expat.h,,AC_MSG_ERROR([Expat required for DRI.]))
    AC_CHECK_LIB(expat, XML_ParserCreate,,
        AC_MSG_ERROR([Expat required for DRI.]))

    # put all the necessary libs together
    DRI_LIB_DEPS="$LIBDRM_LIBS $EXPAT_LIB -lm -lpthread -ldl"
fi
AC_SUBST(DRI_DIRS)
AC_SUBST(EXPAT_INCLUDES)
AC_SUBST(DRI_LIB_DEPS)

dnl
dnl OSMesa configuration
dnl
if test "$mesa_driver" = x11; then
    DRIVER_DIRS="$DRIVER_DIRS osmesa"
fi
OSMESA_LIB_DEPS=""
OSMESA_MESA_DEPS='-l$(GL_LIB)'
AC_SUBST(OSMESA_LIB_DEPS)
AC_SUBST(OSMESA_MESA_DEPS)

dnl
dnl GLU configuration
dnl
AC_ARG_ENABLE(glu,
    [AS_HELP_STRING([--enable-glu],
        [enable OpenGL Utility library @<:@default=yes@:>@])],
    enable_glu="$enableval",
    enable_glu=yes)
if test "x$enable_glu" = xyes; then
    SRC_DIRS="$SRC_DIRS glu"

    # If GLU is available, we can build some programs
    PROGRAM_DIRS="$PROGRAM_DIRS xdemos"

    GLU_LIB_DEPS="-lm"
    GLU_MESA_DEPS='-l$(GL_LIB)'
fi
AC_SUBST(GLU_LIB_DEPS)
AC_SUBST(GLU_MESA_DEPS)

dnl
dnl GLw configuration
dnl
AC_ARG_ENABLE(glw,
    [AS_HELP_STRING([--enable-glw],
        [enable Xt/Motif widget library @<:@default=yes@:>@])],
    enable_glw="$enableval",
    enable_glw=yes)
if test "x$enable_glw" = xyes; then
    SRC_DIRS="$SRC_DIRS glw"
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES(GLW, x11 xt)
        GLW_LIB_DEPS="$GLW_LIBS"
    else
        # should check these...
        GLW_LIB_DEPS="$X_LIBS -lX11 -lXt"
    fi

    GLW_MESA_DEPS='-l$(GL_LIB)'
fi
AC_SUBST(GLW_LIB_DEPS)
AC_SUBST(GLW_MESA_DEPS)

dnl
dnl GLUT configuration
dnl
if test -f "$srcdir/include/GL/glut.h"; then
    default_glut=yes
else
    default_glut=no
fi
AC_ARG_ENABLE(glut,
    [AS_HELP_STRING([--enable-glut],
        [enable GLUT library @<:@default=yes if source available@:>@])],
    enable_glut="$enableval",
    enable_glut="$default_glut")

dnl Can't build glut if GLU not available
if test "x$enable_glu$enable_glut" = xnoyes; then
    AC_MSG_WARN([Disabling glut since GLU is disabled])
    enable_glut=no
fi
if test "x$enable_glut" = xyes; then
    SRC_DIRS="$SRC_DIRS glut/glx"
    GLUT_CFLAGS=""
    if test "x$GCC" = xyes; then
        GLUT_CFLAGS="-fexceptions"
    fi
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES(GLUT, x11 xmu xt xi)
        GLUT_LIB_DEPS="$GLUT_LIBS"
    else
        # should check these...
        GLUT_LIB_DEPS="$X_LIBS -lX11 -lXmu -lXt -lXi"
    fi
    GLUT_LIB_DEPS="$GLUT_LIB_DEPS -lm"

    # If glut is available, we can build most programs
    PROGRAM_DIRS="$PROGRAM_DIRS demos redbook samples glsl"

    GLUT_MESA_DEPS='-l$(GLU_LIB) -l$(GL_LIB)'
fi
AC_SUBST(GLUT_LIB_DEPS)
AC_SUBST(GLUT_MESA_DEPS)
AC_SUBST(GLUT_CFLAGS)

dnl
dnl Program library dependencies
dnl    Only libm is added here if necessary as the libraries should
dnl    be pulled in by the linker
dnl
if test "x$APP_LIB_DEPS" = x; then
    APP_LIB_DEPS="-lm"
fi
AC_SUBST(APP_LIB_DEPS)
AC_SUBST(PROGRAM_DIRS)

dnl Arch/platform-specific settings
PIC_FLAGS=""
ASM_FLAGS=""
ASM_SOURCES=""
ASM_API=""
AC_SUBST(PIC_FLAGS)
AC_SUBST(ASM_FLAGS)
AC_SUBST(ASM_SOURCES)
AC_SUBST(ASM_API)
case "$host_os" in
linux*)
    PIC_FLAGS="-fPIC"
    case "$host_cpu" in
    i*86)
        ASM_FLAGS="-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
        ASM_SOURCES='$(X86_SOURCES)'
        ASM_API='$(X86_API)'
        ;;
    x86_64)
        ASM_FLAGS="-DUSE_X86_64_ASM"
        ASM_SOURCES='$(X86-64_SOURCES)'
        ASM_API='$(X86-64_API)'
        ;;
    powerpc)
        ASM_FLAGS="-DUSE_PPC_ASM -DUSE_VMX_ASM"
        ASM_SOURCES='$(PPC_SOURCES)'
        ;;
    esac
    ;;
freebsd*)
    PIC_FLAGS="-fPIC"
    case "$host_os" in
    i*86)
        PIC_FLAGS=""
        ASM_FLAGS="-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
        ASM_SOURCES='$(X86_SOURCES)'
        ASM_API='$(X86_API)'
        ;;
    x86_64)
        ASM_FLAGS="-DUSE_X86_64_ASM"
        ASM_SOURCES='$(X86-64_SOURCES)'
        ASM_API='$(X86-64_API)'
        ;;
    esac
    ;;
esac

dnl Restore LDFLAGS and CPPFLAGS
LDFLAGS="$_SAVE_LDFLAGS"
CPPFLAGS="$_SAVE_CPPFLAGS"

dnl Substitute the config
AC_OUTPUT([configs/autoconf])

echo ""
echo "        Run 'make autoconf' to build Mesa"
echo ""
