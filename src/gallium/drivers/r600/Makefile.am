include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libr600.la

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/include \
	$(GALLIUM_CFLAGS) \
	$(RADEON_CFLAGS) \
	$(VISIBILITY_CFLAGS)

libr600_la_SOURCES = \
	$(C_SOURCES)

if R600_NEED_RADEON_GALLIUM

libr600_la_SOURCES += \
	$(LLVM_C_SOURCES) \
	$(LLVM_CXX_SOURCES)

libr600_la_LIBADD = ../radeon/libllvmradeon@VERSION@.la

AM_CFLAGS += \
	$(LLVM_CFLAGS) \
	-I$(top_srcdir)/src/gallium/drivers/radeon/

AM_CXXFLAGS= \
	$(LLVM_CXXFLAGS)
endif

if USE_R600_LLVM_COMPILER
AM_CFLAGS += \
	-DR600_USE_LLVM
endif

if HAVE_GALLIUM_COMPUTE
AM_CFLAGS += \
	-DHAVE_OPENCL
endif
