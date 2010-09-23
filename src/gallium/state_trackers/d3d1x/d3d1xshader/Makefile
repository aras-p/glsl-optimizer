LIBNAME=d3d1xshader
CPP_SOURCES=$(wildcard src/*.cpp)
LIBRARY_INCLUDES=-Iinclude -I../d3dapi -I../w32api
PROGS=tools/fxdis
LIBS=libd3d1xshader.a

include ../Makefile.inc

include/tpf_defs.h: $(wildcard defs/*.txt)
	./gen-header.sh $^ > $@

src/tpf_text.cpp: $(wildcard defs/*.txt)
	./gen-text.sh $^ > $@
