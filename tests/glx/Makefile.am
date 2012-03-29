INC = \
	-I$(top_builddir)/src/gtest/include \
	-I$(top_builddir)/src/mapi \
	-I$(top_builddir)/src/glx

AM_CFLAGS = $(INC) $(X11_CFLAGS)
AM_CXXFLAGS = $(INC) $(X11_CFLAGS)

if HAVE_XCB_GLX_CREATE_CONTEXT
TESTS = glx_unittest
check_PROGRAMS = glx_unittest

glx_unittest_SOURCES =			\
	clientinfo_unittest.cpp		\
        create_context_unittest.cpp	\
        fake_glx_screen.cpp

glx_unittest_LDADD = \
	$(top_builddir)/src/glx/libglx.la \
	$(top_builddir)/src/gtest/libgtest.la \
	-lpthread

endif
