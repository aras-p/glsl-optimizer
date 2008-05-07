# This file isn't used during a normal compilation since we don't want to
# require Python in order to compile Mesa.
# Instead, when the Mesa developers update/change the API interface it's
# up to him/her to re-run this makefile and check in the newly generated files.


TOP = ../../..
include $(TOP)/configs/current

GLX_DIR = ../../glx/x11

OUTPUTS = glprocs.h glapitemp.h glapioffsets.h glapitable.h dispatch.h \
	../main/enums.c \
	../x86/glapi_x86.S \
	../x86-64/glapi_x86-64.S \
	../sparc/glapi_sparc.S \
	../drivers/dri/common/extension_helper.h \
	$(GLX_DIR)/indirect.c \
	$(GLX_DIR)/indirect.h \
	$(GLX_DIR)/indirect_init.c \
	$(GLX_DIR)/indirect_size.h \
	$(GLX_DIR)/indirect_size.c \
	$(GLX_DIR)/indirect_dispatch.c \
	$(GLX_DIR)/indirect_dispatch_swap.c \
	$(GLX_DIR)/indirect_dispatch.h \
	$(GLX_DIR)/indirect_reqsize.c \
	$(GLX_DIR)/indirect_reqsize.h \
	$(GLX_DIR)/indirect_size_get.c \
	$(GLX_DIR)/indirect_size_get.h \
	$(GLX_DIR)/indirect_table.c

API_XML = gl_API.xml \
	EXT_framebuffer_object.xml \
	APPLE_vertex_array_object.xml

COMMON = gl_XML.py glX_XML.py license.py $(API_XML) typeexpr.py
COMMON_GLX = $(COMMON) glX_API.xml glX_XML.py glX_proto_common.py

all: $(OUTPUTS)

glprocs.h: gl_procs.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapitemp.h: gl_apitemp.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapioffsets.h: gl_offsets.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapitable.h: gl_table.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

dispatch.h: gl_table.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m remap_table > $@

../main/enums.c: gl_enums.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../x86/glapi_x86.S: gl_x86_asm.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../x86-64/glapi_x86-64.S: gl_x86-64_asm.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../sparc/glapi_sparc.S: gl_SPARC_asm.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@


../drivers/dri/common/extension_helper.h: extension_helper.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

$(GLX_DIR)/indirect.c: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m proto | $(INDENT) $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect.h: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m init_h > $@

$(GLX_DIR)/indirect_init.c: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m init_c > $@

$(GLX_DIR)/indirect_size.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m size_h --only-set -h _INDIRECT_SIZE_H_ \
	  | $(INDENT) $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_size.c: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m size_c --only-set \
	  | $(INDENT) $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_dispatch.c: glX_proto_recv.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m dispatch_c > $@

$(GLX_DIR)/indirect_dispatch_swap.c: glX_proto_recv.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m dispatch_c -s > $@

$(GLX_DIR)/indirect_dispatch.h: glX_proto_recv.py gl_and_glX_API.xml $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m dispatch_h -f gl_and_glX_API.xml -s > $@

$(GLX_DIR)/indirect_size_get.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m size_h --only-get -h '_INDIRECT_SIZE_GET_H_' \
	  | $(INDENT) $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_size_get.c: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m size_c | $(INDENT) $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_reqsize.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m reqsize_h --only-get -h '_INDIRECT_SIZE_GET_H_' \
	  | $(INDENT) $(INDENT_FLAGS) -l200 > $@

$(GLX_DIR)/indirect_reqsize.c: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m reqsize_c | $(INDENT) $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_table.c: glX_server_table.py gl_and_glX_API.xml $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -f gl_and_glX_API.xml > $@

clean:
	-rm -f *~ *.pyo
	-rm -f $(OUTPUTS)
