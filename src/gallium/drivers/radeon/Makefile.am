include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

LIBGALLIUM_LIBS=

noinst_LTLIBRARIES = libradeon.la

AM_CFLAGS = $(GALLIUM_CFLAGS)

libradeon_la_SOURCES = \
	$(C_SOURCES)

if NEED_RADEON_LLVM

if HAVE_GALLIUM_R600
if HAVE_GALLIUM_RADEONSI
lib_LTLIBRARIES = libllvmradeon@VERSION@.la
libllvmradeon@VERSION@_la_LDFLAGS = -Wl, -shared -avoid-version \
	$(LLVM_LDFLAGS)
LIBGALLIUM_LIBS += $(top_builddir)/src/gallium/auxiliary/libgallium.la
else
noinst_LTLIBRARIES += libllvmradeon@VERSION@.la
endif
else
noinst_LTLIBRARIES += libllvmradeon@VERSION@.la
endif

libllvmradeon@VERSION@_la_CXXFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(filter-out -DDEBUG, $(LLVM_CXXFLAGS)) \
	$(DEFINES)

libllvmradeon@VERSION@_la_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(LLVM_CFLAGS)

libllvmradeon@VERSION@_la_SOURCES = \
	$(LLVM_CPP_FILES) \
	$(LLVM_C_FILES)

libllvmradeon@VERSION@_la_LIBADD = \
	$(LIBGALLIUM_LIBS) \
	$(CLOCK_LIB) \
	$(LLVM_LIBS) \
	$(ELF_LIB)

endif
