SUBDIRS = hash_table

AM_CFLAGS = \
	$(X11_CFLAGS) \
	$(PTHREAD_CFLAGS)
AM_CPPFLAGS = \
	-I$(top_builddir)/src/gtest/include \
	-I$(top_builddir)/src/mapi \
	-I$(top_builddir)/src/mesa \
	-I$(top_builddir)/include \
	$(API_DEFINES) $(DEFINES) $(INCLUDE_DIRS)

TESTS = main-test
check_PROGRAMS = main-test

main_test_SOURCES =			\
	enum_strings.cpp

main_test_LDADD = \
	$(top_builddir)/src/mesa/libmesa.la \
	$(top_builddir)/src/gtest/libgtest.la \
	$(PTHREAD_LIBS) \
	$(DLOPEN_LIBS)

if HAVE_SHARED_GLAPI
AM_CPPFLAGS += -DHAVE_SHARED_GLAPI

main_test_SOURCES +=			\
	dispatch_sanity.cpp

main_test_LDADD += \
	$(top_builddir)/src/mapi/shared-glapi/libglapi.la
else
main_test_SOURCES +=			\
	stubs.cpp
endif
