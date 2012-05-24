
TOP = ../../../..
include $(TOP)/configs/current

include Makefile.sources

LIBNAME = radeon

LIBRARY_INCLUDES = -I$(TOP)/include

TBLGEN = $(LLVM_BINDIR)/llvm-tblgen

CXXFLAGS+= $(LLVM_CXXFLAGS)

include ../../Makefile.template

CXXFLAGS := $(filter-out -DDEBUG, $(CXXFLAGS))

tablegen = $(TBLGEN) -I $(LLVM_INCLUDEDIR) $1 $2 -o $3

HAVE_LLVM_INTRINSICS = $(shell grep IntrinsicsR600.td $(LLVM_INCLUDEDIR)/llvm/Intrinsics.td)

gen: $(GENERATED_SOURCES)

SIRegisterInfo.td: SIGenRegisterInfo.pl
	$(PERL) $^ > $@

SIRegisterGetHWRegNum.inc: SIGenRegisterInfo.pl
	$(PERL) $^ $@ > /dev/null

R600Intrinsics.td: R600IntrinsicsNoOpenCL.td R600IntrinsicsOpenCL.td
ifeq ($(HAVE_LLVM_INTRINSICS),)
	cp R600IntrinsicsNoOpenCL.td R600Intrinsics.td
else
	cp R600IntrinsicsOpenCL.td R600Intrinsics.td
endif

R600RegisterInfo.td: R600GenRegisterInfo.pl
	$(PERL) $^ > $@

AMDILGenRegisterInfo.inc: *.td
	$(call tablegen, -gen-register-info, AMDIL.td, $@)

AMDILGenInstrInfo.inc: *.td
	$(call tablegen, -gen-instr-info, AMDIL.td, $@)

AMDILGenAsmWriter.inc: *.td
	$(call tablegen, -gen-asm-writer, AMDIL.td, $@)

AMDILGenDAGISel.inc: *.td
	$(call tablegen, -gen-dag-isel, AMDIL.td, $@)

AMDILGenCallingConv.inc: *.td
	$(call tablegen, -gen-callingconv, AMDIL.td, $@)

AMDILGenSubtargetInfo.inc: *.td
	$(call tablegen, -gen-subtarget, AMDIL.td, $@)

AMDILGenEDInfo.inc: *.td
	$(call tablegen, -gen-enhanced-disassembly-info, AMDIL.td, $@)

AMDILGenIntrinsics.inc: *.td
	$(call tablegen, -gen-tgt-intrinsic, AMDIL.td, $@)

AMDILGenCodeEmitter.inc: *.td
	$(call tablegen, -gen-emitter, AMDIL.td, $@)

LOADER_LIBS=$(shell llvm-config --libs bitreader asmparser)
loader: loader.o libradeon.a
	gcc -o loader -L/usr/local/lib $(LDFLAGS) loader.o libradeon.a $(LLVM_LIBS) $(LOADER_LIBS) -lpthread -ldl -lstdc++ -lm

# FIXME: Remove when this driver is converted to automake.
all: default
