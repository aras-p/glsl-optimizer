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

COMMON = gl_XML.pyc license.pyc gl_API.xml
PYTHON2 = python

all: $(OUTPUTS)

gl_XML.pyc: gl_XML.py
	rm -f gl_XML.pyc > /dev/null
	$(PYTHON2) -t -O gl_XML.py

license.pyc: license.py
	rm -f license.pyc > /dev/null
	$(PYTHON2) -t -O license.py

glprocs.h: $(COMMON) gl_procs.py
	$(PYTHON2) -t gl_procs.py > glprocs.h

glapitemp.h: $(COMMON) gl_apitemp.py
	$(PYTHON2) -t gl_apitemp.py > glapitemp.h

glapioffsets.h: $(COMMON) gl_offsets.py
	$(PYTHON2) -t gl_offsets.py > glapioffsets.h

glapitable.h: $(COMMON) gl_table.py
	$(PYTHON2) -t gl_table.py > glapitable.h

../x86/glapi_x86.S: $(COMMON) gl_x86_asm.py
	$(PYTHON2) -t gl_x86_asm.py > ../x86/glapi_x86.S

../../glx/x11/indirect.c: $(COMMON) glX_proto_send.py
	$(PYTHON2) -t glX_proto_send.py -m proto > ../../glx/x11/indirect.c

../../glx/x11/indirect.h: $(COMMON) glX_proto_send.py
	$(PYTHON2) -t glX_proto_send.py -m init_h > ../../glx/x11/indirect.h

../../glx/x11/indirect_init.c: $(COMMON) glX_proto_send.py
	$(PYTHON2) -t glX_proto_send.py -m init_c > ../../glx/x11/indirect_init.c

../../glx/x11/indirect_size.h: $(COMMON) glX_proto_send.py
	$(PYTHON2) -t glX_proto_send.py -m size_h > ../../glx/x11/indirect_size.h

../../glx/x11/indirect_size.c: $(COMMON) glX_proto_send.py
	$(PYTHON2) -t glX_proto_send.py -m size_c > ../../glx/x11/indirect_size.c

clean:
	rm -f *~ *.pyc
	rm -f $(OUTPUTS)
