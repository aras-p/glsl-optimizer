# ===========================================================================
#   http://www.gnu.org/software/autoconf-archive/ax_prog_cxx_for_build.html
# ===========================================================================
#
# SYNOPSIS
#
#   AX_PROG_CXX_FOR_BUILD
#
# DESCRIPTION
#
#   This macro searches for a C++ compiler that generates native executables,
#   that is a C++ compiler that surely is not a cross-compiler. This can be
#   useful if you have to generate source code at compile-time like for
#   example GCC does.
#
#   The macro sets the CXX_FOR_BUILD and CXXCPP_FOR_BUILD macros to anything
#   needed to compile or link (CXX_FOR_BUILD) and preprocess (CXXCPP_FOR_BUILD).
#   The value of these variables can be overridden by the user by specifying
#   a compiler with an environment variable (like you do for standard CXX).
#
# LICENSE
#
#   Copyright (c) 2008 Paolo Bonzini <bonzini@gnu.org>
#   Copyright (c) 2012 Avionic Design GmbH
#
#   Based on the AX_PROG_CC_FOR_BUILD macro by Paolo Bonzini.
#
#   Copying and distribution of this file, with or without modification, are
#   permitted in any medium without royalty provided the copyright notice
#   and this notice are preserved. This file is offered as-is, without any
#   warranty.

#serial 5

AU_ALIAS([AC_PROG_CXX_FOR_BUILD], [AX_PROG_CXX_FOR_BUILD])
AC_DEFUN([AX_PROG_CXX_FOR_BUILD], [dnl
AC_REQUIRE([AX_PROG_CC_FOR_BUILD])dnl
AC_REQUIRE([AC_PROG_CXX])dnl
AC_REQUIRE([AC_PROG_CXXCPP])dnl
AC_REQUIRE([AC_CANONICAL_SYSTEM])dnl
dnl
pushdef([AC_TRY_COMPILER], [
cat > conftest.$ac_ext << EOF
#line __oline__ "configure"
#include "confdefs.h"
[$1]
EOF
# If we can't run a trivial program, we are probably using a cross
compiler.
# Fail miserably.
if AC_TRY_EVAL(ac_link) && test -s conftest${ac_exeext} && (./conftest;
exit) 2>/dev/null; then
  [$2]=yes
else
  echo "configure: failed program was:" >&AC_FD_CC
  cat conftest.$ac_ext >&AC_FD_CC
  [$2]=no
fi
[$3]=no
rm -fr conftest*])dnl

dnl Use the standard macros, but make them use other variable names
dnl
pushdef([cross_compiling], [#])dnl
pushdef([ac_cv_prog_CXXCPP], ac_cv_build_prog_CXXCPP)dnl
pushdef([ac_cv_prog_gxx], ac_cv_build_prog_gxx)dnl
pushdef([ac_cv_prog_cxx_works], ac_cv_build_prog_cxx_works)dnl
pushdef([ac_cv_prog_cxx_cross], ac_cv_build_prog_cxx_cross)dnl
pushdef([ac_cv_prog_cxx_g], ac_cv_build_prog_cxx_g)dnl
pushdef([CXX], CXX_FOR_BUILD)dnl
pushdef([CXXCPP], CXXCPP_FOR_BUILD)dnl
pushdef([CXXFLAGS], CXXFLAGS_FOR_BUILD)dnl
pushdef([CXXCPPFLAGS], CXXCPPFLAGS_FOR_BUILD)dnl
pushdef([host], build)dnl
pushdef([host_alias], build_alias)dnl
pushdef([host_cpu], build_cpu)dnl
pushdef([host_vendor], build_vendor)dnl
pushdef([host_os], build_os)dnl
pushdef([ac_cv_host], ac_cv_build)dnl
pushdef([ac_cv_host_alias], ac_cv_build_alias)dnl
pushdef([ac_cv_host_cpu], ac_cv_build_cpu)dnl
pushdef([ac_cv_host_vendor], ac_cv_build_vendor)dnl
pushdef([ac_cv_host_os], ac_cv_build_os)dnl
pushdef([ac_cxxcpp], ac_build_cxxcpp)dnl
pushdef([ac_compile], ac_build_compile)dnl
pushdef([ac_link], ac_build_link)dnl
pushdef([ac_tool_prefix], [#])dnl

AC_PROG_CXX
AC_PROG_CXXCPP

dnl Restore the old definitions
dnl
popdef([AC_TRY_COMPILER])dnl
popdef([ac_tool_prefix])dnl
popdef([ac_link])dnl
popdef([ac_compile])dnl
popdef([ac_cxxcpp])dnl
popdef([ac_cv_host_os])dnl
popdef([ac_cv_host_vendor])dnl
popdef([ac_cv_host_cpu])dnl
popdef([ac_cv_host_alias])dnl
popdef([ac_cv_host])dnl
popdef([host_os])dnl
popdef([host_vendor])dnl
popdef([host_cpu])dnl
popdef([host_alias])dnl
popdef([host])dnl
popdef([CXXCPPFLAGS])dnl
popdef([CXXFLAGS])dnl
popdef([CXXCPP])dnl
popdef([CXX])dnl
popdef([ac_cv_prog_cxx_g])dnl
popdef([ac_cv_prog_cxx_works])dnl
popdef([ac_cv_prog_cxx_cross])dnl
popdef([ac_cv_prog_gxx])dnl
popdef([cross_compiling])dnl

dnl Finally, set Makefile variables
dnl
AC_SUBST([CXXFLAGS_FOR_BUILD])dnl
AC_SUBST([CXXCPPFLAGS_FOR_BUILD])dnl
])
