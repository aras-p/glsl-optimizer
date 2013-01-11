include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

if HAVE_GALLIUM_R600
if HAVE_GALLIUM_RADEONSI
lib_LTLIBRARIES = libllvmradeon@VERSION@.la
libllvmradeon@VERSION@_la_LDFLAGS = -Wl, -shared -avoid-version \
	$(LLVM_LDFLAGS)
else
noinst_LTLIBRARIES = libllvmradeon@VERSION@.la
endif
else
noinst_LTLIBRARIES = libllvmradeon@VERSION@.la
endif

AM_CXXFLAGS = \
	$(filter-out -DDEBUG, $(LLVM_CXXFLAGS)) \
	$(DEFINES)

AM_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(LLVM_CFLAGS)

libllvmradeon@VERSION@_la_SOURCES = \
	$(CPP_FILES) \
	$(C_FILES)

libllvmradeon@VERSION@_la_LIBADD = \
	$(CLOCK_LIB) \
	$(LLVM_LIBS)
