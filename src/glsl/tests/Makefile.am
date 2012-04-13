INC = \
	-I$(top_builddir)/src/gtest/include \
	-I$(top_builddir)/src/mesa \
	-I$(top_builddir)/src/mapi \
	-I$(top_builddir)/src/glsl

AM_CFLAGS =  $(INC)
AM_CXXFLAGS = $(INC)

TESTS = \
	optimization-test \
	uniform-initializer-test

check_PROGRAMS = 				\
	uniform-initializer-test

uniform_initializer_test_SOURCES =		\
	copy_constant_to_storage_tests.cpp	\
	set_uniform_initializer_tests.cpp	\
	uniform_initializer_utils.cpp

uniform_initializer_test_LDADD =		\
	$(top_builddir)/src/gtest/libgtest.la	\
	$(top_builddir)/src/glsl/libglsl.a	\
	$(top_builddir)/src/mesa/libmesa.a	\
	-lpthread
