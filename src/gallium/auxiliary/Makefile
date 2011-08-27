TOP = ../../..
include $(TOP)/configs/current

LIBNAME = gallium

# get source lists
include Makefile.sources

C_SOURCES += $(GENERATED_SOURCES)

ifeq ($(MESA_LLVM),1)
C_SOURCES += \
	$(GALLIVM_SOURCES)
CPP_SOURCES += \
	$(GALLIVM_CPP_SOURCES)
endif


include ../Makefile.template


indices/u_indices_gen.c: indices/u_indices_gen.py
	$(PYTHON2) $< > $@

indices/u_unfilled_gen.c: indices/u_unfilled_gen.py
	$(PYTHON2) $< > $@

util/u_format_srgb.c: util/u_format_srgb.py
	$(PYTHON2) $< > $@

util/u_format_table.c: util/u_format_table.py util/u_format_pack.py util/u_format_parse.py util/u_format.csv
	$(PYTHON2) util/u_format_table.py util/u_format.csv > $@

util/u_half.c: util/u_half.py
	$(PYTHON2) util/u_half.py > $@
# DO NOT DELETE
