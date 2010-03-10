# Makefile for Thomas Sondergaard's API tracer

TOP = ../../..

include $(TOP)/configs/current


OBJECTS = gltrace.o gltrace_support.o

TRACER = gltrace.so

.cc.o:
	$(CXX) -c $(INCDIRS) $(CXXFLAGS) $< -o $@


default: $(TRACER)

$(TRACER): $(OBJECTS)
	$(MKLIB) -o $(TRACER) -noprefix -cplusplus -linker '$(CXX)' \
		-ldflags '$(LDFLAGS)' $(MKLIB_OPTIONS) $(OBJECTS)

gltrace.cc: gltrace.py
	PYTHONPATH=$(TOP)/src/mesa/glapi/gen python gltrace.py -f $(TOP)/src/mesa/glapi/gen/gl_API.xml  > gltrace.cc


clean:
	rm -f $(OBJECTS)
	rm -f $(TRACER)
	rm -f *~
	rm -f gltrace.cc
