
TOP = ../../../..
include $(TOP)/configs/current

include Makefile.sources

LIBNAME = radeon

LIBRARY_INCLUDES = -I$(TOP)/include

CXXFLAGS+= $(LLVM_CXXFLAGS)

include ../../Makefile.template

CXXFLAGS := $(filter-out -DDEBUG, $(CXXFLAGS))
