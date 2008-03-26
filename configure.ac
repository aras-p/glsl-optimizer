dnl Process this file with autoconf to create configure.

AC_PREREQ(2.59)

dnl Versioning
dnl Make version number available to autoconf and configure
m4_define(mesa_major, 7)
m4_define(mesa_minor, 1)
m4_define(mesa_tiny, 0)
m4_define(mesa_version, [mesa_major().mesa_minor().mesa_tiny()])

AC_INIT([Mesa],[mesa_version()],
    [https://bugs.freedesktop.org/enter_bug.cgi?product=Mesa])
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
AC_PATH_PROG(GMAKE, gmake, [not_found])
AC_PATH_PROG(MAKE, make)
if test "x$GMAKE" != "xnot_found"; then
	MAKE="$GMAKE"
fi
AC_PATH_PROG(MKDEP, makedepend)
AC_PATH_PROG(SED, sed)

MKDEP_OPTIONS=-fdepend
dnl Ask gcc where it's keeping its secret headers
if test "x$GCC" = xyes; then
    GCC_INCLUDES=`$CC -print-file-name=include`
    if test "x$GCC_INCLUDES" != x; then
        MKDEP_OPTIONS="$MKDEP_OPTIONS -I$GCC_INCLUDES"
    fi
fi
AC_SUBST(MKDEP_OPTIONS)

dnl Check to see if dlopen is in default libraries (like Solaris, which
dnl has it in libc), or if libdl is needed to get it.
AC_CHECK_FUNC([dlopen], [],
	AC_CHECK_LIB([dl], [dlopen], DLOPEN_LIBS="-ldl"))

dnl Make sure the pkg-config macros are defined
m4_ifdef([PKG_PROG_PKG_CONFIG],,[
    AC_MSG_ERROR([The pkg-config autoconf macros are not defined.
                  Did you run 'make configure'?])]
)
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
case "$host_os" in
linux*)
if test "x$GCC" = xyes; then
    DEFINES="$DEFINES -D_POSIX_SOURCE -D_POSIX_C_SOURCE=199309L -D_BSD_SOURCE"
fi
    DEFINES="$DEFINES -D_SVID_SOURCE -D_GNU_SOURCE -DPTHREADS -DHAVE_POSIX_MEMALIGN"
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
AC_ARG_VAR(OPT_FLAGS, [Additional optimization flags for the compiler.
    Default is to use CFLAGS.])
AC_ARG_VAR(ARCH_FLAGS, [Additional architecture specific flags for the
    compiler. Default is to use CFLAGS.])
AC_SUBST(OPT_FLAGS)
AC_SUBST(ARCH_FLAGS)

dnl
dnl Hacks to enable 32 or 64 bit build
dnl
AC_ARG_ENABLE(32-bit,
    [AS_HELP_STRING([--enable-32-bit],
        [build 32-bit libraries @<:@default=auto@:>@])],
    enable_32bit="$enableval",
    enable_32bit=auto
)
if test "x$enable_32bit" = xyes; then
    if test "x$GCC" = xyes; then
        CFLAGS="$CFLAGS -m32"
    fi
    if test "x$GXX" = xyes; then
        CXXFLAGS="$CXXFLAGS -m32"
    fi
fi
AC_ARG_ENABLE(64-bit,
    [AS_HELP_STRING([--enable-64-bit],
        [build 64-bit libraries @<:@default=auto@:>@])],
    enable_64bit="$enableval",
    enable_64bit=auto
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
AC_ARG_ENABLE(static,
    [AS_HELP_STRING([--enable-static],
        [build static libraries @<:@default=disabled@:>@])],
    enable_static="$enableval",
    enable_static=no
)
case "x$enable_static" in
xyes|xno ) ;;
x ) enable_static=no ;;
* )
    AC_MSG_ERROR([Static library option '$enable_static' is not a valid])
    ;;
esac
AC_ARG_ENABLE(shared,
    [AS_HELP_STRING([--disable-shared],
        [build shared libraries @<:@default=enabled@:>@])],
    enable_shared="$enableval",
    enable_shared=yes
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
AC_ARG_VAR(MKLIB_OPTIONS,[Options for the Mesa library script, mklib])
if test "$enable_static" = yes; then
    MKLIB_OPTIONS="$MKLIB_OPTIONS -static"
fi
AC_SUBST(MKLIB_OPTIONS)

dnl
dnl other compiler options
dnl
AC_ARG_ENABLE(debug,
    [AS_HELP_STRING([--enable-debug],
        [use debug compiler flags and macros @<:@default=disabled@:>@])],
    enable_debug="$enableval",
    enable_debug=no
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
dnl These will be used near the end in the arch specific options
AC_ARG_ENABLE(asm,
    [AS_HELP_STRING([--disable-asm],
        [disable assembly usage @<:@default=enabled on supported plaforms@:>@])],
    enable_asm="$enableval",
    enable_asm=yes
)

dnl
dnl library names
dnl
if test "$enable_static" = yes; then
    GL_LIB_NAME='lib$(GL_LIB).a'
    GLU_LIB_NAME='lib$(GLU_LIB).a'
    GLUT_LIB_NAME='lib$(GLUT_LIB).a'
    GLW_LIB_NAME='lib$(GLW_LIB).a'
    OSMESA_LIB_NAME='lib$(OSMESA_LIB).a'
else
    GL_LIB_NAME='lib$(GL_LIB).so'
    GLU_LIB_NAME='lib$(GLU_LIB).so'
    GLUT_LIB_NAME='lib$(GLUT_LIB).so'
    GLW_LIB_NAME='lib$(GLW_LIB).so'
    OSMESA_LIB_NAME='lib$(OSMESA_LIB).so'
fi
AC_SUBST(GL_LIB_NAME)
AC_SUBST(GLU_LIB_NAME)
AC_SUBST(GLUT_LIB_NAME)
AC_SUBST(GLW_LIB_NAME)
AC_SUBST(OSMESA_LIB_NAME)

dnl
dnl Driver configuration. Options are xlib, dri and osmesa right now.
dnl More later: directfb, fbdev, ...
dnl
AC_ARG_WITH(driver,
    [AS_HELP_STRING([--with-driver=DRIVER],
        [driver for Mesa: xlib,dri,osmesa @<:@default=xlib@:>@])],
    mesa_driver="$withval",
    mesa_driver="xlib")
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
SRC_DIRS="mesa"
GLU_DIRS="sgi"
WINDOW_SYSTEM=""
case "$mesa_driver" in
xlib)
    DRIVER_DIRS="x11"
    ;;
dri)
    SRC_DIRS="glx/x11 $SRC_DIRS"
    DRIVER_DIRS="dri"
    WINDOW_SYSTEM="dri"
    ;;
osmesa)
    DRIVER_DIRS="osmesa"
    ;;
esac
AC_SUBST(SRC_DIRS)
AC_SUBST(GLU_DIRS)
AC_SUBST(DRIVER_DIRS)
AC_SUBST(WINDOW_SYSTEM)

dnl
dnl User supplied program configuration
dnl
if test -d "$srcdir/progs/demos"; then
    default_demos=yes
else
    default_demos=no
fi
AC_ARG_WITH(demos,
    [AS_HELP_STRING([--with-demos@<:@=DIRS...@:>@],
        [optional comma delimited demo directories to build
        @<:@default=auto if source available@:>@])],
    with_demos="$withval",
    with_demos="$default_demos")
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
xlib|dri)
    if test "$no_x" = yes; then
        AC_MSG_ERROR([X11 development libraries needed for $mesa_driver driver])
    fi
    ;;
esac

# SELinux awareness.
AC_ARG_ENABLE(selinux, AS_HELP_STRING([--enable-selinux], [Build SELinux-aware Mesa (default: disabled)]), [MESA_SELINUX=$enableval], [MESA_SELINUX=no])
if test "x$enable_selinux" = "xyes"; then
    AC_CHECK_HEADER(selinux/selinux.h,,
		    AC_MSG_ERROR([SELinux headers not found]))
    AC_CHECK_LIB(selinux,is_selinux_enabled,,
		 AC_MSG_ERROR([SELinux library not found]))
    SELINUX_LIBS="-lselinux"
    DEFINES="$DEFINES -DMESA_SELINUX"
fi

dnl
dnl libGL configuration per driver
dnl
case "$mesa_driver" in
xlib)
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES(XLIBGL, x11 xext)
        X11_INCLUDES="$X11_INCLUDES $XLIBGL_CFLAGS"
        GL_LIB_DEPS="$XLIBGL_LIBS"
    else
        # should check these...
        X11_INCLUDES="$X11_INCLUDES $X_CFLAGS"
        GL_LIB_DEPS="$X_LIBS -lX11 -lXext"
    fi
    GL_LIB_DEPS="$GL_LIB_DEPS $SELINUX_LIBS -lm -lpthread"

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
    PKG_CHECK_MODULES(LIBDRM, libdrm)
    PKG_CHECK_MODULES(DRI2PROTO, dri2proto)

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
    GL_LIB_DEPS="$GL_LIB_DEPS $LIBDRM_LIBS -lm -lpthread $DLOPEN_LIBS"
    ;;
osmesa)
    # No libGL for osmesa
    GL_LIB_DEPS=""
    ;;
esac
AC_SUBST(GL_LIB_DEPS)

dnl
dnl More X11 setup
dnl
if test "$mesa_driver" = xlib; then
    DEFINES="$DEFINES -DUSE_XSHM"
fi

dnl
dnl More DRI setup
dnl
AC_ARG_ENABLE(glx-tls,
    [AS_HELP_STRING([--enable-glx-tls],
        [enable TLS support in GLX @<:@default=disabled@:>@])],
        GLX_USE_TLS="$enableval",
        GLX_USE_TLS=no)
dnl Directory for DRI drivers
AC_ARG_WITH(dri-driverdir,
    [AS_HELP_STRING([--with-dri-driverdir=DIR],
        [directory for the DRI drivers @<:@/usr/X11R6/lib/modules/dri@:>@])],
    DRI_DRIVER_INSTALL_DIR="$withval",
    DRI_DRIVER_INSTALL_DIR='/usr/X11R6/lib/modules/dri')
AC_SUBST(DRI_DRIVER_INSTALL_DIR)
dnl Direct rendering or just indirect rendering
AC_ARG_ENABLE(driglx-direct,
    [AS_HELP_STRING([--disable-driglx-direct],
        [enable direct rendering in GLX for DRI @<:@default=enabled@:>@])],
        driglx_direct="$enableval",
        driglx_direct="yes")

dnl Which drivers to build - default is chosen by platform
AC_ARG_WITH(dri-drivers,
    [AS_HELP_STRING([--with-dri-drivers@<:@=DIRS...@:>@],
        [comma delimited DRI drivers, e.g. "i965,radeon,nouveau" @<:@default=auto@:>@])],
    with_dri_drivers="$withval",
    with_dri_drivers=yes)
if test "x$with_dri_drivers" = x; then
    with_dri_drivers=no
fi

dnl If $with_dri_drivers is yes, directories will be added through
dnl platform checks
DRI_DIRS=""
case "$with_dri_drivers" in
no|yes) ;;
*)
    # verify the requested driver directories exist
    dri_drivers=`IFS=,; echo $with_dri_drivers`
    for driver in $dri_drivers; do
        test -d "$srcdir/src/mesa/drivers/dri/$driver" || \
            AC_MSG_ERROR([DRI driver directory '$driver' doesn't exist])
    done
    DRI_DIRS="$dri_drivers"
    ;;
esac

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
        x86_64)
            # ffb, gamma, and sis are missing because they have not be
            # converted to use the new interface.  i810 are missing
            # because there is no x86-64 system where they could *ever*
            # be used.
            if test "x$DRI_DIRS" = x; then
                DRI_DIRS="i915 i965 mach64 mga r128 r200 r300 radeon \
                    savage tdfx unichrome"
            fi
            ;;
        powerpc*)
            # Build only the drivers for cards that exist on PowerPC.
            # At some point MGA will be added, but not yet.
            if test "x$DRI_DIRS" = x; then
                DRI_DIRS="mach64 r128 r200 r300 radeon tdfx"
            fi
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

        # ffb and gamma are missing because they have not been converted
        # to use the new interface.
        if test "x$DRI_DIRS" = x; then
            DRI_DIRS="i810 i915 i965 mach64 mga r128 r200 r300 radeon tdfx \
                unichrome savage sis"
        fi
        ;;
    esac

    # default drivers
    if test "x$DRI_DIRS" = x; then
        DRI_DIRS="i810 i915 i965 mach64 mga r128 r200 r300 radeon s3v \
            savage sis tdfx trident unichrome ffb"
    fi

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
    DRI_LIB_DEPS="$SELINUX_LIBS $LIBDRM_LIBS $EXPAT_LIB -lm -lpthread $DLOPEN_LIBS"
fi
AC_SUBST(DRI_DIRS)
AC_SUBST(EXPAT_INCLUDES)
AC_SUBST(DRI_LIB_DEPS)

dnl
dnl OSMesa configuration
dnl
if test "$mesa_driver" = xlib; then
    default_gl_osmesa=yes
else
    default_gl_osmesa=no
fi
AC_ARG_ENABLE(gl-osmesa,
    [AS_HELP_STRING([--enable-gl-osmesa],
        [enable OSMesa on libGL @<:@default=enabled for xlib driver@:>@])],
        gl_osmesa="$enableval",
        gl_osmesa="$default_gl_osmesa")
if test "x$gl_osmesa" = xyes; then
    if test "$mesa_driver" = osmesa; then
        AC_MSG_ERROR([libGL is not available for OSMesa driver])
    else
        DRIVER_DIRS="$DRIVER_DIRS osmesa"
    fi
fi

dnl Configure the channel bits for OSMesa (libOSMesa, libOSMesa16, ...)
AC_ARG_WITH(osmesa-bits,
    [AS_HELP_STRING([--with-osmesa-bits=BITS],
        [OSMesa channel bits and library name: 8, 16, 32 @<:@default=8@:>@])],
        osmesa_bits="$withval",
        osmesa_bits=8)
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
AC_SUBST(OSMESA_LIB)

case "$mesa_driver" in
osmesa)
    # only link librararies with osmesa if shared
    if test "$enable_static" = no; then
        OSMESA_LIB_DEPS="-lm -lpthread $SELINUX_LIBS"
    else
        OSMESA_LIB_DEPS=""
    fi
    OSMESA_MESA_DEPS=""
    ;;
*)
    # Link OSMesa to libGL otherwise
    OSMESA_LIB_DEPS=""
    # only link librararies with osmesa if shared
    if test "$enable_static" = no; then
        OSMESA_MESA_DEPS='-l$(GL_LIB)'
    else
        OSMESA_MESA_DEPS=""
    fi
    ;;
esac
AC_SUBST(OSMESA_LIB_DEPS)
AC_SUBST(OSMESA_MESA_DEPS)

dnl
dnl GLU configuration
dnl
AC_ARG_ENABLE(glu,
    [AS_HELP_STRING([--disable-glu],
        [enable OpenGL Utility library @<:@default=enabled@:>@])],
    enable_glu="$enableval",
    enable_glu=yes)
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
        if test "$enable_static" = no; then
            GLU_MESA_DEPS='-l$(OSMESA_LIB)'
        else
            GLU_MESA_DEPS=""
        fi
        ;;
    *)
        # If static, empty GLU_LIB_DEPS and add libs for programs to link
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
AC_SUBST(GLU_LIB_DEPS)
AC_SUBST(GLU_MESA_DEPS)

dnl
dnl GLw configuration
dnl
AC_ARG_ENABLE(glw,
    [AS_HELP_STRING([--disable-glw],
        [enable Xt/Motif widget library @<:@default=enabled@:>@])],
    enable_glw="$enableval",
    enable_glw=yes)
dnl Don't build GLw on osmesa
if test "x$enable_glw" = xyes && test "$mesa_driver" = osmesa; then
    AC_MSG_WARN([Disabling GLw since the driver is OSMesa])
    enable_glw=no
fi
if test "x$enable_glw" = xyes; then
    SRC_DIRS="$SRC_DIRS glw"
    if test "$x11_pkgconfig" = yes; then
        PKG_CHECK_MODULES(GLW, x11 xt)
        GLW_LIB_DEPS="$GLW_LIBS"
    else
        # should check these...
        GLW_LIB_DEPS="$X_LIBS -lX11 -lXt"
    fi

    # If static, empty GLW_LIB_DEPS and add libs for programs to link
    if test "$enable_static" = no; then
        GLW_MESA_DEPS='-l$(GL_LIB)'
    else
        APP_LIB_DEPS="$APP_LIB_DEPS $GLW_LIB_DEPS"
        GLW_LIB_DEPS=""
        GLW_MESA_DEPS=""
    fi
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
    [AS_HELP_STRING([--disable-glut],
        [enable GLUT library @<:@default=enabled if source available@:>@])],
    enable_glut="$enableval",
    enable_glut="$default_glut")

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
        PKG_CHECK_MODULES(GLUT, x11 xmu xi)
        GLUT_LIB_DEPS="$GLUT_LIBS"
    else
        # should check these...
        GLUT_LIB_DEPS="$X_LIBS -lX11 -lXmu -lXi"
    fi
    GLUT_LIB_DEPS="$GLUT_LIB_DEPS -lm"

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
        if test "x$enable_asm" = xyes; then
            ASM_FLAGS="-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
            ASM_SOURCES='$(X86_SOURCES)'
            ASM_API='$(X86_API)'
        fi
        ;;
    x86_64)
        if test "x$enable_asm" = xyes; then
            ASM_FLAGS="-DUSE_X86_64_ASM"
            ASM_SOURCES='$(X86-64_SOURCES)'
            ASM_API='$(X86-64_API)'
        fi
        ;;
    powerpc)
        if test "x$enable_asm" = xyes; then
            ASM_FLAGS="-DUSE_PPC_ASM -DUSE_VMX_ASM"
            ASM_SOURCES='$(PPC_SOURCES)'
        fi
        ;;
    esac
    ;;
freebsd*)
    PIC_FLAGS="-fPIC"
    case "$host_cpu" in
    i*86)
        PIC_FLAGS=""
        if test "x$enable_asm" = xyes; then
            ASM_FLAGS="-DUSE_X86_ASM -DUSE_MMX_ASM -DUSE_3DNOW_ASM -DUSE_SSE_ASM"
            ASM_SOURCES='$(X86_SOURCES)'
            ASM_API='$(X86_API)'
        fi
        ;;
    x86_64)
        if test "x$enable_asm" = xyes; then
            ASM_FLAGS="-DUSE_X86_64_ASM"
            ASM_SOURCES='$(X86-64_SOURCES)'
            ASM_API='$(X86-64_API)'
        fi
        ;;
    esac
    ;;
esac

dnl Restore LDFLAGS and CPPFLAGS
LDFLAGS="$_SAVE_LDFLAGS"
CPPFLAGS="$_SAVE_CPPFLAGS"

dnl Substitute the config
AC_CONFIG_FILES([configs/autoconf])
AC_OUTPUT

dnl Replace the configs/current symlink
if test -f configs/current || test -L configs/current; then
    rm -f configs/current
fi
ln -s autoconf configs/current

dnl
dnl Output some configuration info for the user
dnl
echo ""
echo "        prefix:          $prefix"
echo "        exec_prefix:     $exec_prefix"
echo "        libdir:          $libdir"

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
    echo "        DRI drivers:     $dri_dirs"
    echo "        DRI driver dir:  $DRI_DRIVER_INSTALL_DIR"
fi

dnl Libraries
echo ""
echo "        Shared libs:     $enable_shared"
echo "        Static libs:     $enable_static"
echo "        GLU:             $enable_glu"
echo "        GLw:             $enable_glw"
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
echo "        Run 'make' to build Mesa"
echo ""
