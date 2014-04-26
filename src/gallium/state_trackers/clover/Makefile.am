AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources

AM_CPPFLAGS = \
	$(GALLIUM_PIPE_LOADER_DEFINES) \
	-DPIPE_SEARCH_DIR=\"$(libdir)/gallium-pipe\" \
	-I$(top_srcdir)/include \
	-I$(top_srcdir)/src/gallium/include \
	-I$(top_srcdir)/src/gallium/drivers \
	-I$(top_srcdir)/src/gallium/auxiliary \
	-I$(top_srcdir)/src/gallium/winsys \
	-I$(srcdir)

if HAVE_CLOVER_ICD
AM_CPPFLAGS += -DHAVE_CLOVER_ICD
else
# Only install the headers if we are building a stand-alone implementation
cldir = $(includedir)/CL
cl_HEADERS = \
	$(top_srcdir)/include/CL/cl.h \
	$(top_srcdir)/include/CL/cl.hpp \
	$(top_srcdir)/include/CL/cl_d3d10.h \
	$(top_srcdir)/include/CL/cl_d3d11.h \
	$(top_srcdir)/include/CL/cl_dx9_media_sharing.h \
	$(top_srcdir)/include/CL/cl_egl.h \
	$(top_srcdir)/include/CL/cl_ext.h \
	$(top_srcdir)/include/CL/cl_gl.h \
	$(top_srcdir)/include/CL/cl_gl_ext.h \
	$(top_srcdir)/include/CL/cl_platform.h \
	$(top_srcdir)/include/CL/opencl.h
endif

noinst_LTLIBRARIES = libclover.la libcltgsi.la libclllvm.la

libcltgsi_la_CXXFLAGS = \
	-std=c++0x \
	$(VISIBILITY_CXXFLAGS)

libcltgsi_la_SOURCES = \
	tgsi/compiler.cpp

libclllvm_la_CXXFLAGS = \
	$(VISIBILITY_CXXFLAGS) \
	$(LLVM_CXXFLAGS) \
	$(DEFINES) \
	-DLIBCLC_INCLUDEDIR=\"$(LIBCLC_INCLUDEDIR)/\" \
	-DLIBCLC_LIBEXECDIR=\"$(LIBCLC_LIBEXECDIR)/\" \
	-DCLANG_RESOURCE_DIR=\"$(CLANG_RESOURCE_DIR)\"

libclllvm_la_SOURCES = \
	llvm/invocation.cpp

libclover_la_CXXFLAGS = \
	-std=c++11 \
	$(VISIBILITY_CXXFLAGS)

libclover_la_LIBADD = \
	libcltgsi.la libclllvm.la

libclover_la_SOURCES = $(CPP_SOURCES)
