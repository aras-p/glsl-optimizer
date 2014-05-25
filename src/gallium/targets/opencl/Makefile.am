AUTOMAKE_OPTIONS = subdir-objects

lib_LTLIBRARIES = lib@OPENCL_LIBNAME@.la

lib@OPENCL_LIBNAME@_la_LDFLAGS = \
	$(LLVM_LDFLAGS) \
	-no-undefined \
	-version-number 1:0 \
	$(GC_SECTIONS) \
	$(LD_NO_UNDEFINED)

if HAVE_LD_VERSION_SCRIPT
lib@OPENCL_LIBNAME@_la_LDFLAGS += \
	-Wl,--version-script=$(top_srcdir)/src/gallium/targets/opencl/opencl.sym
endif

lib@OPENCL_LIBNAME@_la_LIBADD = \
	$(top_builddir)/src/gallium/auxiliary/pipe-loader/libpipe_loader_client.la \
	$(top_builddir)/src/gallium/winsys/sw/null/libws_null.la \
	$(top_builddir)/src/gallium/state_trackers/clover/libclover.la \
	$(top_builddir)/src/gallium/auxiliary/libgallium.la \
	$(GALLIUM_PIPE_LOADER_CLIENT_LIBS) \
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

if HAVE_DRISW
lib@OPENCL_LIBNAME@_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/sw/dri/libswdri.la
endif

if NEED_WINSYS_XLIB
lib@OPENCL_LIBNAME@_la_LIBADD += \
	$(top_builddir)/src/gallium/winsys/sw/xlib/libws_xlib.la \
	-lX11 -lXext -lXfixes \
	$(LIBDRM_LIBS)
endif

nodist_EXTRA_lib@OPENCL_LIBNAME@_la_SOURCES = dummy.cpp
lib@OPENCL_LIBNAME@_la_SOURCES =


if HAVE_CLOVER_ICD
icddir = /etc/OpenCL/vendors/
icd_DATA = mesa.icd
endif

include $(top_srcdir)/install-gallium-links.mk
