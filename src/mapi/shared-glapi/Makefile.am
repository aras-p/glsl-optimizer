# Used by OpenGL ES or when --enable-shared-glapi is specified

SUBDIRS = . tests

TOP = $(top_srcdir)
GLAPI = $(top_srcdir)/src/mapi/glapi
include $(top_srcdir)/src/mapi/Makefile.sources

lib_LTLIBRARIES = libglapi.la
libglapi_la_SOURCES = $(MAPI_GLAPI_FILES)
libglapi_la_LIBADD = $(PTHREAD_LIBS) $(SELINUX_LIBS)
libglapi_la_LDFLAGS = \
	-no-undefined \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

include $(GLAPI)/gen/glapi_gen.mk
glapi_mapi_tmp.h : $(GLAPI)/gen/gl_and_es_API.xml $(glapi_gen_mapi_deps)
	$(call glapi_gen_mapi,$<,shared-glapi)

BUILT_SOURCES = glapi_mapi_tmp.h
CLEANFILES = $(BUILT_SOURCES)

AM_CFLAGS = $(PTHREAD_CFLAGS)
AM_CPPFLAGS =							\
	$(DEFINES)						\
	$(SELINUX_CFLAGS)					\
	-I$(top_srcdir)/include					\
	-I$(top_srcdir)/src/mapi				\
	-I$(top_builddir)/src/mapi                              \
	-DMAPI_MODE_GLAPI					\
	-DMAPI_ABI_HEADER=\"shared-glapi/glapi_mapi_tmp.h\"

include $(top_srcdir)/install-lib-links.mk
