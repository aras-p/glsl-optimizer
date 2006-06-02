# Makefile for Thomas Sondergaard's API tracer

TOP = ../../..

include $(TOP)/configs/current


OBJECTS = gltrace.o gltrace_support.o

TRACER = gltrace.so

.cc.o:
	$(CXX) -c $(INCDIRS) $(CXXFLAGS) $< -o $@


default: $(TRACER)

$(TRACER): $(OBJECTS)
	$(TOP)/bin/mklib -o $(TRACER) -noprefix -cplusplus \
		$(MKLIB_OPTIONS) $(OBJECTS)

gltrace.cc: gltrace.py
	PYTHONPATH=$(TOP)/src/mesa/glapi python gltrace.py -f $(TOP)/src/mesa/glapi/gl_API.xml  > gltrace.cc


clean:
	rm -f $(OBJECTS)
	rm -f $(TRACER)
	rm -f *~
	rm -f gltrace.cc
