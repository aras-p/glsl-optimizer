TARGET		= libnouveau_dri.so
GALLIUMDIR	= ../../..
DRMDIR		?= /usr
DRIDIR		= ../../../../driclient

OBJECTS		= nouveau_screen_vl.o nouveau_context_vl.o nouveau_swapbuffers.o

CFLAGS		+= -g -Wall -Werror=implicit-function-declaration -fPIC	\
		   -I${GALLIUMDIR}/include				\
		   -I${GALLIUMDIR}/winsys/g3dvl				\
		   -I${GALLIUMDIR}/winsys/drm/nouveau			\
		   -I${DRMDIR}/include					\
		   -I${DRMDIR}/include/drm				\
		   -I${DRMDIR}/include/nouveau				\
		   -I${GALLIUMDIR}/drivers				\
		   -I${GALLIUMDIR}/auxiliary				\
		   -I${DRIDIR}/include

LDFLAGS		+= -L${DRMDIR}/lib				\
		   -L${DRIDIR}/lib				\
		   -L${GALLIUMDIR}/winsys/drm/nouveau/common	\
		   -L${GALLIUMDIR}/auxiliary			\
		   -L${GALLIUMDIR}/drivers/nv30			\
		   -L${GALLIUMDIR}/drivers/nv40			\
		   -L${GALLIUMDIR}/drivers/nv50

LIBS		+= -lnouveaudrm -ldriclient -ldrm_nouveau -ldrm -lnv30 -lnv40 -lnv50 -lgallium -lm

#############################################

.PHONY	= all clean libdriclient

all: ${TARGET}

${TARGET}: ${OBJECTS} libdriclient
	$(CC) ${LDFLAGS} -shared -o $@ ${OBJECTS} ${LIBS}

libdriclient:
	cd ${DRIDIR}/src; ${MAKE}

clean:
	cd ${DRIDIR}/src; ${MAKE} clean
	rm -rf ${OBJECTS} ${TARGET}
