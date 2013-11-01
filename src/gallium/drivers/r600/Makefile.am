AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

AM_CFLAGS = \
	$(GALLIUM_DRIVER_CFLAGS) \
	$(RADEON_CFLAGS)

AM_CXXFLAGS = \
	$(GALLIUM_DRIVER_CXXFLAGS) \
	$(RADEON_CFLAGS)

noinst_LTLIBRARIES = libr600.la

libr600_la_SOURCES = \
	$(C_SOURCES) \
	$(CXX_SOURCES)

libr600_la_LIBADD = ../radeon/libradeon.la

if NEED_RADEON_LLVM

AM_CFLAGS += \
	$(LLVM_CFLAGS) \
	-I$(top_srcdir)/src/gallium/drivers/radeon/

libr600_la_SOURCES += \
	$(LLVM_C_SOURCES)

libr600_la_LIBADD += ../radeon/libllvmradeon.la

endif

if USE_R600_LLVM_COMPILER
AM_CFLAGS += \
	-DR600_USE_LLVM
endif

if HAVE_GALLIUM_COMPUTE
AM_CFLAGS += \
	-DHAVE_OPENCL
endif
