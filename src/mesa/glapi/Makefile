# This file isn't used during a normal compilation since we don't want to
# require Python in order to compile Mesa.
# Instead, when the Mesa developers update/change the API interface it's
# up to him/her to re-run this makefile and check in the newly generated files.


OUTPUTS = glprocs.h glapitemp.h glapioffsets.h glapitable.h \
	../x86/glapi_x86.S \
	../../glx/x11/indirect.c \
	../../glx/x11/indirect.h \
	../../glx/x11/indirect_init.c \
	../../glx/x11/indirect_size.h \
	../../glx/x11/indirect_size.c

COMMON = gl_XML.pyo license.pyo gl_API.xml
COMMON_GLX = $(COMMON) glX_XML.pyo

PYTHON2 = python
PYTHON_FLAGS = -t -O -O

all: $(OUTPUTS)

gl_XML.pyo: gl_XML.py
	rm -f gl_XML.pyo > /dev/null
	$(PYTHON2) $(PYTHON_FLAGS) gl_XML.py

glX_XML.pyo: glX_XML.py $(COMMON)
	rm -f glX_XML.pyo > /dev/null
	$(PYTHON2) $(PYTHON_FLAGS) glX_XML.py

license.pyo: license.py
	rm -f license.pyo > /dev/null
	$(PYTHON2) $(PYTHON_FLAGS) license.py

glprocs.h: $(COMMON) gl_procs.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_procs.py > glprocs.h

glapitemp.h: $(COMMON) gl_apitemp.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_apitemp.py > glapitemp.h

glapioffsets.h: $(COMMON) gl_offsets.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_offsets.py > glapioffsets.h

glapitable.h: $(COMMON) gl_table.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_table.py > glapitable.h

../x86/glapi_x86.S: $(COMMON) gl_x86_asm.py
	$(PYTHON2) $(PYTHON_FLAGS) gl_x86_asm.py > ../x86/glapi_x86.S

../../glx/x11/indirect.c: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m proto > ../../glx/x11/indirect.c

../../glx/x11/indirect.h: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m init_h > ../../glx/x11/indirect.h

../../glx/x11/indirect_init.c: $(COMMON_GLX) glX_proto_send.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_send.py -m init_c > ../../glx/x11/indirect_init.c

../../glx/x11/indirect_size.h: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_h > ../../glx/x11/indirect_size.h

../../glx/x11/indirect_size.c: $(COMMON_GLX) glX_proto_size.py
	$(PYTHON2) $(PYTHON_FLAGS) glX_proto_size.py -m size_c > ../../glx/x11/indirect_size.c

clean:
	rm -f *~ *.pyo
	rm -f $(OUTPUTS)
