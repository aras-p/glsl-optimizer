#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

AC_PREREQ(2.61)
AC_INIT(glsl, XXXXX, idr@freedesktop.org, glsl)
AC_CONFIG_SRCDIR([Makefile.am])
AM_CONFIG_HEADER([config.h])
AC_CONFIG_FILES([glcpp/Makefile])

AM_INIT_AUTOMAKE
LT_INIT

AM_MAINTAINER_MODE

# Checks for programs.
AC_PROG_CXX
AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_YACC
AC_PROG_LEX

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

# Checks for libraries.

# Checks for header files.

# Checks for typedefs, structures, and compiler characteristics.

# Checks for library functions.
AC_HEADER_STDC

AH_TOP([#ifndef GLSL_CONFIG_H
#define GLSL_CONFIG_H])
AH_BOTTOM([#endif /* GLSL_CONFIG_H */])

PKG_CHECK_MODULES([talloc], [talloc >= 2.0])

AC_ARG_ENABLE([debug],
    [AS_HELP_STRING([--enable-debug],
        [use debug compiler flags and macros @<:@default=disabled@:>@])],
    [enable_debug="$enableval"],
    [enable_debug=no]
)
if test "x$enable_debug" = xyes; then
    DEFINES="$DEFINES -DDEBUG"
    if test "x$GCC" = xyes; then
	# Remove any -g or -O flags from the command line
	CFLAGS=[`echo $CFLAGS | sed 's/-g[^ \t]*[ \t]*//g;s/-O[^ \t]*[ \t]*//g'`]
        CFLAGS="$CFLAGS -O0 -ggdb3 -fstack-protector -D_FORTIFY_SOURCE=2"
    fi
    if test "x$GXX" = xyes; then
	# Remove any -g flags from the command line
	CXXFLAGS=[`echo $CXXFLAGS | sed 's/-g[^ \t]*[ \t]*//g;s/-O[^ \t]*[ \t]*//g'`]
        CXXFLAGS="$CXXFLAGS -O0 -ggdb3 -fstack-protector -D_FORTIFY_SOURCE=2"
    fi
fi

if test "x$GXX" = xyes ; then
	WARN="-Wall -Wextra -Wunsafe-loop-optimizations -Wstack-protector"
else
	WARN=""
fi

CFLAGS="$CFLAGS $WARN"
CXXFLAGS="$CXXFLAGS $WARN"
YFLAGS="-d -v"

AC_OUTPUT([Makefile])
