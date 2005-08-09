# This file isn't used during a normal compilation since we don't want to
# require Python in order to compile Mesa.
# Instead, when the Mesa developers update/change the API interface it's
# up to him/her to re-run this makefile and check in the newly generated files.


TOP = ../../..
include $(TOP)/configs/current

OUTPUTS = glprocs.h glapitemp.h glapioffsets.h glapitable.h dispatch.h \
	../main/enums.c \
	../x86/glapi_x86.S \
	../x86-64/glapi_x86-64.S \
	../drivers/dri/common/extension_helper.h \
	../../glx/x11/indirect.c \
	../../glx/x11/indirect.h \
	../../glx/x11/indirect_init.c \
	../../glx/x11/indirect_size.h \
	../../glx/x11/indirect_size.c

COMMON = gl_XML.py license.py gl_API.xml typeexpr.py
COMMON_GLX = $(COMMON) glX_XML.py glX_proto_common.py

all: $(OUTPUTS)

glprocs.h: $(COMMON) gl_procs.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_procs.py > glprocs.h

glapitemp.h: $(COMMON) gl_apitemp.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_apitemp.py > glapitemp.h

glapioffsets.h: $(COMMON) gl_offsets.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_offsets.py > glapioffsets.h

glapitable.h: $(COMMON) gl_table.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_table.py > glapitable.h

dispatch.h: $(COMMON) gl_table.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_table.py -m remap_table > dispatch.h

../main/enums.c: $(COMMON) gl_enums.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_enums.py > ../main/enums.c

../x86/glapi_x86.S: $(COMMON) gl_x86_asm.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_x86_asm.py > ../x86/glapi_x86.S

../x86-64/glapi_x86-64.S: $(COMMON) gl_x86-64_asm.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_x86-64_asm.py > ../x86-64/glapi_x86-64.S

../drivers/dri/common/extension_helper.h: $(COMMON) extension_helper.py
	$(PYTHON2) $(PYTHON_FLAGS) extension_helper.py > ../drivers/dri/common/extension_helper.h

../../glx/x11/indirect.c: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m proto > ../../glx/x11/indirect.c

../../glx/x11/indirect.h: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m init_h > ../../glx/x11/indirect.h

../../glx/x11/indirect_init.c: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m init_c > ../../glx/x11/indirect_init.c

../../glx/x11/indirect_size.h: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_h --only-set -h _INDIRECT_SIZE_H_ > ../../glx/x11/indirect_size.h

../../glx/x11/indirect_size.c: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_c --only-set > ../../glx/x11/indirect_size.c

clean:
	rm -f *~ *.pyo
	rm -f $(OUTPUTS)
