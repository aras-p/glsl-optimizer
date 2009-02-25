TARGET     = libegldrm.a
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))
GALLIUMDIR = ../..
TOP        = ../../../..

include ${TOP}/configs/current

CFLAGS := \
          -I${GALLIUMDIR}/include \
          -I${GALLIUMDIR}/auxiliary \
          -I${TOP}/src/mesa/drivers/dri/common \
          -I${TOP}/src/mesa \
          -I$(TOP)/include \
          -I$(TOP)/src/egl/main \
          ${LIBDRM_CFLAGS} \
          ${CFLAGS}

#############################################

.PHONY	= all clean

all: ${TARGET}

${TARGET}: ${OBJECTS}
	ar rcs $@ $^

clean:
	rm -rf ${OBJECTS} ${TARGET}
