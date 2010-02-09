# This file isn't used during a normal compilation since we don't want to
# require Python in order to compile Mesa.
# Instead, when the Mesa developers update/change the API interface it's
# up to him/her to re-run this makefile and check in the newly generated files.


TOP = ../../..
include $(TOP)/configs/current

OUTPUTS = glprocs.h glapitemp.h glapioffsets.h glapitable.h glapidispatch.h \
	../main/enums.c \
	../main/remap_helper.h \
	../x86/glapi_x86.S \
	../x86-64/glapi_x86-64.S \
	../sparc/glapi_sparc.S \
	../../glx/indirect.c \
	../../glx/indirect.h \
	../../glx/indirect_init.c \
	../../glx/indirect_size.h \
	../../glx/indirect_size.c


GLX_DIR = $(XORG_BASE)/glx

SERVER_GLAPI_FILES = \
	$(GLX_DIR)/glapi.h \
	$(GLX_DIR)/glapi.c \
	$(GLX_DIR)/glapi_nop.c \
	$(GLX_DIR)/glthread.c \
	$(GLX_DIR)/glthread.h

SERVER_OUTPUTS = \
	$(GLX_DIR)/indirect_dispatch.c \
	$(GLX_DIR)/indirect_dispatch_swap.c \
	$(GLX_DIR)/indirect_dispatch.h \
	$(GLX_DIR)/indirect_reqsize.c \
	$(GLX_DIR)/indirect_reqsize.h \
	$(GLX_DIR)/indirect_size.h \
	$(GLX_DIR)/indirect_size_get.c \
	$(GLX_DIR)/indirect_size_get.h \
	$(GLX_DIR)/indirect_table.c \
	$(GLX_DIR)/glapitemp.h \
	$(GLX_DIR)/glapitable.h \
	$(GLX_DIR)/glapioffsets.h \
	$(GLX_DIR)/glapidispatch.h \
	$(GLX_DIR)/glprocs.h \
	$(SERVER_GLAPI_FILES)

API_XML = gl_API.xml \
	ARB_copy_buffer.xml \
	ARB_depth_clamp.xml \
	ARB_draw_elements_base_vertex.xml \
	ARB_framebuffer_object.xml \
	ARB_map_buffer_range.xml \
	ARB_seamless_cube_map.xml \
	ARB_sync.xml \
	ARB_vertex_array_object.xml \
	APPLE_vertex_array_object.xml \
	EXT_draw_buffers2.xml \
	EXT_framebuffer_object.xml \
	EXT_packed_depth_stencil.xml \
	EXT_provoking_vertex.xml \
	EXT_texture_array.xml \
	NV_conditional_render.xml

COMMON = gl_XML.py glX_XML.py license.py $(API_XML) typeexpr.py
COMMON_GLX = $(COMMON) glX_API.xml glX_XML.py glX_proto_common.py

all: check-xorg-source $(OUTPUTS) $(SERVER_OUTPUTS)

check-xorg-source:
	@if ! test -d $(GLX_DIR); then \
		echo "ERROR: Must specify path to xserver checkout; set XORG_BASE."; \
		exit 1; \
	fi

$(GLX_DIR)/%.c: %.c
	cp $< $@

$(GLX_DIR)/%.h: %.h
	cp $< $@

glprocs.h $(GLX_DIR)/glprocs.h: gl_procs.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapitemp.h $(GLX_DIR)/glapitemp.h: gl_apitemp.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapioffsets.h $(GLX_DIR)/glapioffsets.h: gl_offsets.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapitable.h $(GLX_DIR)/glapitable.h: gl_table.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

glapidispatch.h $(GLX_DIR)/glapidispatch.h: gl_table.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m remap_table > $@

../main/enums.c: gl_enums.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../main/remap_helper.h: remap_helper.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../x86/glapi_x86.S: gl_x86_asm.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../x86-64/glapi_x86-64.S: gl_x86-64_asm.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../sparc/glapi_sparc.S: gl_SPARC_asm.py $(COMMON)
	$(PYTHON2) $(PYTHON_FLAGS) $< > $@

../../glx/indirect.c: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m proto | $(INDENT) $(INDENT_FLAGS) > $@

../../glx/indirect.h: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m init_h > $@

../../glx/indirect_init.c: glX_proto_send.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m init_c > $@

../../glx/indirect_size.h $(GLX_DIR)/indirect_size.h: glX_proto_size.py $(COMMON_GLX)
	$(PYTHON2) $(PYTHON_FLAGS) $< -m size_h --only-set -h _INDIRECT_SIZE_H_ \
	  | $(INDENT) $(INDENT_FLAGS) > $@

../../glx/indirect_size.c: glX_proto_size.py $(COMMON_GLX)
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
