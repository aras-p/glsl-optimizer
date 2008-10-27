TARGET		= libXvMCg3dvl.so
SONAME		= libXvMCg3dvl.so.1
GALLIUMDIR	= ../gallium

OBJECTS         = block.o surface.o context.o subpicture.o attributes.o

ifeq (${DRIVER}, softpipe)
OBJECTS         += ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
endif

CFLAGS	+= -g -fPIC -Wall				\
	   -I${GALLIUMDIR}/state_trackers/g3dvl		\
	   -I${GALLIUMDIR}/winsys/g3dvl			\
	   -I${GALLIUMDIR}/include			\
	   -I${GALLIUMDIR}/auxiliary			\
	   -I${GALLIUMDIR}/drivers

ifeq (${DRIVER}, softpipe)
LDFLAGS	+= -L${GALLIUMDIR}/state_trackers/g3dvl		\
	   -L${GALLIUMDIR}/drivers/softpipe		\
	   -L${GALLIUMDIR}/auxiliary/tgsi		\
	   -L${GALLIUMDIR}/auxiliary/draw		\
	   -L${GALLIUMDIR}/auxiliary/translate		\
	   -L${GALLIUMDIR}/auxiliary/cso_cache		\
	   -L${GALLIUMDIR}/auxiliary/util		\
	   -L${GALLIUMDIR}/auxiliary/rtasm
else
LDFLAGS	+= -L${GALLIUMDIR}/state_trackers/g3dvl		\
   	   -L${GALLIUMDIR}/winsys/g3dvl/nouveau		\
   	   -L${GALLIUMDIR}/auxiliary/util
endif

ifeq (${DRIVER}, softpipe)
LIBS	+= -lg3dvl -lsoftpipe -ldraw -ltgsi -ltranslate -lrtasm -lcso_cache -lutil -lm
else
LIBS	+= -lg3dvl -lnouveau_dri -lutil
endif

#############################################

ifeq (${DRIVER}, softpipe)
.PHONY	= all clean g3dvl
else
.PHONY	= all clean g3dvl nouveau_winsys
endif

all: ${TARGET}

ifeq (${DRIVER}, softpipe)
${TARGET}: ${OBJECTS} g3dvl
	$(CC) ${LDFLAGS} -shared -Wl,-soname,${SONAME} -o $@ ${OBJECTS} ${LIBS}

g3dvl:
	cd ${GALLIUMDIR}/state_trackers/g3dvl; ${MAKE}

clean:
	cd ${GALLIUMDIR}/state_trackers/g3dvl; ${MAKE} clean
	rm -rf ${OBJECTS} ${TARGET}
else
${TARGET}: ${OBJECTS} g3dvl nouveau_winsys
	$(CC) ${LDFLAGS} -shared -Wl,-soname,${SONAME} -o $@ ${OBJECTS} ${LIBS}

g3dvl:
	cd ${GALLIUMDIR}/state_trackers/g3dvl; ${MAKE}

nouveau_winsys:
	cd ${GALLIUMDIR}/winsys/g3dvl/nouveau; ${MAKE}

clean:
	cd ${GALLIUMDIR}/state_trackers/g3dvl; ${MAKE} clean
	cd ${GALLIUMDIR}/winsys/g3dvl/nouveau; ${MAKE} clean
	rm -rf ${OBJECTS} ${TARGET}
endif
