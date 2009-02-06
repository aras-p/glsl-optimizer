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
		   -L${GALLIUMDIR}/auxiliary/draw		\
		   -L${GALLIUMDIR}/auxiliary/tgsi		\
		   -L${GALLIUMDIR}/auxiliary/translate		\
		   -L${GALLIUMDIR}/auxiliary/rtasm		\
		   -L${GALLIUMDIR}/auxiliary/cso_cache		\
		   -L${GALLIUMDIR}/drivers/nv04			\
		   -L${GALLIUMDIR}/drivers/nv10			\
		   -L${GALLIUMDIR}/drivers/nv20			\
		   -L${GALLIUMDIR}/drivers/nv30			\
		   -L${GALLIUMDIR}/drivers/nv40			\
		   -L${GALLIUMDIR}/drivers/nv50

LIBS		+= -lnouveaudrm -ldriclient -ldrm_nouveau -ldrm -lnv04 -lnv10 -lnv20 -lnv30 -lnv40 -lnv50 -ldraw -ltgsi -ltranslate -lrtasm -lcso_cache -lm

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
