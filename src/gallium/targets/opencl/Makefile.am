AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = libOpenCL.la

libOpenCL_la_LDFLAGS = \
	$(LLVM_LDFLAGS) \
	-version-number 1:0

libOpenCL_la_LIBADD = \
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


libOpenCL_la_SOURCES =

# Force usage of a C++ linker
nodist_EXTRA_libOpenCL_la_SOURCES = dummy.cpp

# Provide compatibility with scripts for the old Mesa build system for
# a while by putting a link to the driver into /lib of the build tree.
all-local: libOpenCL.la
	$(MKDIR_P) $(top_builddir)/$(LIB_DIR)
	ln -f .libs/libOpenCL.so* $(top_builddir)/$(LIB_DIR)/
