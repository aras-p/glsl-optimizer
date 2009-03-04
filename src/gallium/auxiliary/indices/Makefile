TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = indices

C_SOURCES = \
	u_indices_gen.c \
	u_unfilled_gen.c 

include ../../Makefile.template

u_indices_gen.c: u_indices_gen.py
	python $< > $@

u_unfilled_gen.c: u_unfilled_gen.py
	python $< > $@
