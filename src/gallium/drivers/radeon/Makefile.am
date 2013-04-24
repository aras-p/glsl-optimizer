include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

LIBGALLIUM_LIBS=

noinst_LTLIBRARIES = libradeon.la

AM_CFLAGS = $(GALLIUM_CFLAGS) $(RADEON_CFLAGS)

libradeon_la_SOURCES = \
	$(C_SOURCES)

if NEED_RADEON_LLVM

libllvmradeon_la_LDFLAGS = \
	$(LLVM_LDFLAGS)

noinst_LTLIBRARIES += libllvmradeon.la

libllvmradeon_la_CXXFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(DEFINES)

libllvmradeon_la_CFLAGS = \
	$(GALLIUM_CFLAGS) \
	$(LLVM_CFLAGS)

libllvmradeon_la_SOURCES = \
	$(LLVM_CPP_FILES) \
	$(LLVM_C_FILES)

libllvmradeon_la_LIBADD = \
	$(LIBGALLIUM_LIBS) \
	$(CLOCK_LIB) \
	$(LLVM_LIBS) \
	$(ELF_LIB)

endif
