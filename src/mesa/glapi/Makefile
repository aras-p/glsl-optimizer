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
	../sparc/glapi_sparc.S \
	../drivers/dri/common/extension_helper.h \
	../../glx/x11/indirect.c \
	../../glx/x11/indirect.h \
	../../glx/x11/indirect_init.c \
	../../glx/x11/indirect_size.h \
	../../glx/x11/indirect_size.c


#XORG_BASE = /home/idr/devel/graphics/Xorg/xserver/xorg
GLX_DIR   = $(XORG_BASE)/GL/glx

SERVER_OUTPUTS = $(GLX_DIR)/indirect_dispatch.c \
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

INDENT_FLAGS = -i4 -nut -br -brs -npcs -ce -T GLubyte -T GLbyte -T Bool

all: $(OUTPUTS)

server: $(SERVER_OUTPUTS)

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

../sparc/glapi_sparc.S: $(COMMON) gl_SPARC_asm.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_SPARC_asm.py > ../sparc/glapi_sparc.S


../drivers/dri/common/extension_helper.h: $(COMMON) extension_helper.py
	$(PYTHON2) $(PYTHON_FLAGS) extension_helper.py > ../drivers/dri/common/extension_helper.h

../../glx/x11/indirect.c: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m proto | indent $(INDENT_FLAGS) > ../../glx/x11/indirect.c

../../glx/x11/indirect.h: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m init_h > ../../glx/x11/indirect.h

../../glx/x11/indirect_init.c: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m init_c > ../../glx/x11/indirect_init.c

../../glx/x11/indirect_size.h: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_h --only-set -h _INDIRECT_SIZE_H_ | indent $(INDENT_FLAGS) > ../../glx/x11/indirect_size.h

../../glx/x11/indirect_size.c: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_c --only-set | indent $(INDENT_FLAGS) > ../../glx/x11/indirect_size.c

$(GLX_DIR)/indirect_dispatch.c: $(COMMON_GLX) glX_proto_recv.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_recv.py -m dispatch_c > $@

$(GLX_DIR)/indirect_dispatch_swap.c: $(COMMON_GLX) glX_proto_recv.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_recv.py -m dispatch_c -s > $@

$(GLX_DIR)/indirect_dispatch.h: $(COMMON_GLX) glX_proto_recv.py glX_API.xml
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_recv.py -m dispatch_h -f gl_and_glX_API.xml -s > $@

$(GLX_DIR)/indirect_size_get.h: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_h --only-get -h '_INDIRECT_SIZE_GET_H_' | indent $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_size_get.c: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_c | indent $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_reqsize.h: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m reqsize_h --only-get -h '_INDIRECT_SIZE_GET_H_' | indent $(INDENT_FLAGS) -l200 > $@

$(GLX_DIR)/indirect_reqsize.c: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m reqsize_c | indent $(INDENT_FLAGS) > $@

$(GLX_DIR)/indirect_table.c: $(COMMON_GLX) glX_server_table.py glX_API.xml
	$(PYTHON2) $(PYTHON_FLAGS) glX_server_table.py -f gl_and_glX_API.xml > $@

clean:
	rm -f *~ *.pyo
	rm -f $(OUTPUTS)
