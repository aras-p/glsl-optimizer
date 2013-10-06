AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = lib@OPENCL_LIBNAME@.la

lib@OPENCL_LIBNAME@_la_LDFLAGS = \
	$(LLVM_LDFLAGS) \
	-version-number 1:0

lib@OPENCL_LIBNAME@_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader.la \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la \
	$(top_builddir)/src/gallium/state_trackers/clover/libclover.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(GALLIUM_PIPE_LOADER_LIBS) $(LIBUDEV_LIBS) \
	-ldl \
	-lclangCodeGen \
	-lclangFrontendTool \
	-lclangFrontend \
	-lclangDriver \
	-lclangSerialization \
	-lclangCodeGen \
	-lclangParse \
	-lclangSema \
	-lclangAnalysis \
	-lclangAST \
	-lclangEdit \
	-lclangLex \
	-lclangBasic \
	$(LLVM_LIBS)

lib@OPENCL_LIBNAME@_la_SOURCES =

# Force usage of a C++ linker
nodist_EXTRA_lib@OPENCL_LIBNAME@_la_SOURCES = dummy.cpp

if HAVE_CLOVER_ICD
icddir = /etc/OpenCL/vendors/
icd_DATA = mesa.icd
endif

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: lib@OPENCL_LIBNAME@.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)
	ln -f .libs/lib@OPENCL_LIBNAME@.so* $(top_builddir)/$(LIB_DIR)/
