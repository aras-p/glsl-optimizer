# This file isn't used during a normal compilation since we don't want to
# require Python in order to compile Mesa.
# Instead, when the Mesa developers update/change the API interface it's
# up to him/her to re-run this makefile and check in the newly generated files.


OUTPUTS = glprocs.h glapitemp.h glapioffsets.h glapitable.h glapi_x86.S 

COMMON = gl_XML.pyc license.pyc gl_API.xml

all: $(OUTPUTS)

gl_XML.pyc: gl_XML.py
	rm -f gl_XML.pyc > /dev/null
	python2 -t -O gl_XML.py

license.pyc: license.py
	rm -f license.pyc > /dev/null
	python2 -t -O license.py

glprocs.h: $(COMMON) gl_procs.py
	python2 -t gl_procs.py > glprocs.h

glapitemp.h: $(COMMON) gl_apitemp.py
	python2 -t gl_apitemp.py > glapitemp.h

glapioffsets.h: $(COMMON) gl_offsets.py
	python2 -t gl_offsets.py > glapioffsets.h

glapitable.h: $(COMMON) gl_table.py
	python2 -t gl_table.py > glapitable.h

glapi_x86.S: $(COMMON) gl_x86_asm.py
	python2 -t gl_x86_asm.py > glapi_x86.S

clean:
	rm -f *~ *.pyc
	rm -f $(OUTPUTS)
