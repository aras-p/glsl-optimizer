AUTOMAKE_OPTIONS = subdir-objects

include Makefile.sources
include $(top_srcdir)/src/gallium/Automake.inc

noinst_LTLIBRARIES = libgallium.la

AM_CFLAGS = \
	-I$(top_srcdir)/src/gallium/auxiliary/util \
	$(GALLIUM_CFLAGS) \
	$(VISIBILITY_CFLAGS)

AM_CXXFLAGS = $(VISIBILITY_CXXFLAGS)

libgallium_la_SOURCES = \
	$(C_SOURCES) \
	$(GENERATED_SOURCES)

if HAVE_MESA_LLVM

AM_CFLAGS += \
	$(LLVM_CFLAGS)

AM_CXXFLAGS += \
	$(GALLIUM_CFLAGS) \
	$(LLVM_CXXFLAGS)

libgallium_la_SOURCES += \
	$(GALLIVM_SOURCES) \
	$(GALLIVM_CPP_SOURCES)

endif

indices/u_indices_gen.c: $(srcdir)/indices/u_indices_gen.py
	$(AM_V_at)$(MKDIR_P) indices
	$(AM_V_GEN) $(PYTHON2) $< > $@

indices/u_unfilled_gen.c: $(srcdir)/indices/u_unfilled_gen.py
	$(AM_V_at)$(MKDIR_P) indices
	$(AM_V_GEN) $(PYTHON2) $< > $@

util/u_format_srgb.c: $(srcdir)/util/u_format_srgb.py
	$(AM_V_at)$(MKDIR_P) util
	$(AM_V_GEN) $(PYTHON2) $< > $@

util/u_format_table.c: $(srcdir)/util/u_format_table.py $(srcdir)/util/u_format_pack.py $(srcdir)/util/u_format_parse.py $(srcdir)/util/u_format.csv
	$(AM_V_at)$(MKDIR_P) util
	$(AM_V_GEN) $(PYTHON2) $(srcdir)/util/u_format_table.py $(srcdir)/util/u_format.csv > $@
