TARGET     = libegldrm.a
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))
GALLIUMDIR = ../..
TOP        = ../../../..

include ${TOP}/configs/current

CFLAGS += -g -Wall -Werror-implicit-function-declaration -fPIC \
          -I${GALLIUMDIR}/include \
          -I${GALLIUMDIR}/auxiliary \
          -I${TOP}/src/mesa/drivers/dri/common \
          -I${TOP}/src/mesa \
          -I$(TOP)/include \
          -I$(TOP)/src/egl/main \
          $(shell pkg-config --cflags pixman-1 xorg-server) \
          ${LIBDRM_CFLAGS}

#############################################

.PHONY	= all clean

all: ${TARGET}

${TARGET}: ${OBJECTS}
	ar rcs $@ $^

clean:
	rm -rf ${OBJECTS} ${TARGET}
