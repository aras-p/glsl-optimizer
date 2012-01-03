AM_CFLAGS = -I$(top_builddir)/src/glx -I$(top_builddir)/src/mapi \
	$(X11_CFLAGS) $(GTEST_CFLAGS)
AM_CXXFLAGS = -I$(top_builddir)/src/glx -I$(top_builddir)/src/mapi \
	$(X11_CFLAGS) $(GTEST_CFLAGS)

if HAVE_GTEST
if HAVE_XCB_GLX_CREATE_CONTEXT
TESTS = glx_unittest
check_PROGRAMS = glx_unittest

glx_unittest_SOURCES =			\
	clientinfo_unittest.cpp		\
        create_context_unittest.cpp	\
        fake_glx_screen.cpp

glx_unittest_LDADD = $(top_builddir)/src/glx/libglx.a $(GTEST_LIBS) -lgtest_main
endif
endif
