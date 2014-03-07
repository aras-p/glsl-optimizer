include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc


AM_CFLAGS = \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libradeon.la

libradeon_la_SOURCES = \
	$(C_SOURCES)

if NEED_RADEON_LLVM

noinst_LTLIBRARIES += libllvmradeon.la

libllvmradeon_la_CFLAGS = \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(RADEON_CFLAGS) \
	$(LLVM_CFLAGS)

libllvmradeon_la_SOURCES = \
	$(LLVM_C_FILES)

libllvmradeon_la_LIBADD = \
	$(CLOCK_LIB) \
	$(LLVM_LIBS) \
	$(ELF_LIB)

libllvmradeon_la_LDFLAGS = \
	$(LLVM_LDFLAGS)

endif
