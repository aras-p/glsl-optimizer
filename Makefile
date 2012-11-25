FLASCC=/path/to/FLASCC/sdk
FLEX=/path/to/flex

BUILD=$(PWD)/build
INSTALL=$(PWD)/install
SRCROOT=$(PWD)/

ENV=PATH="$(FLASCC)/usr/bin":"$(INSTALL)/usr/bin":"$(PATH)" CC=gcc CXX=g++ CFLAGS=-O4 CXXFLAGS=-O4

swc: check
	cd swc && $(ENV) cmake -D SDK="$(FLASCC)" .
	cd swc && $(ENV) make -j6
	mv swc/glsl2agal.swc bin/
	mv swc/glsl2agalopt bin/

example: check
	cd examples/basic && $(FLEX)/bin/mxmlc \
		-static-link-runtime-shared-libraries \
		-library-path+=../../bin/glsl2agal.swc \
		GLSLCompiler.mxml \
		-o GLSLCompiler.swf

clean:
	rm -rf swc/cmake_install.cmake swc/CMakeCache.txt swc/CMakeFiles swc/Makefile

check:
	@if [ -d $(FLASCC)/usr/bin ] ; then true ; \
	else echo "Couldn't locate FLASCC sdk directory, please invoke make with \"make FLASCC=/path/to/FLASCC/sdk ...\"" ; exit 1 ; \
	fi

	@if [ -d "$(FLEX)/bin" ] ; then true ; \
	else echo "Couldn't locate Flex sdk directory, please invoke make with \"make FLEX=/path/to/flex  ...\"" ; exit 1 ; \
	fi
