TARGET		= libnouveau_dri.so
GALLIUMDIR	= ../../..
DRMDIR		?= /usr
DRIDIR		= ../../../../driclient

OBJECTS		= nouveau_bo.o nouveau_fence.o nouveau_swapbuffers.o nouveau_channel.o		\
		  nouveau_grobj.o nouveau_context.o nouveau_winsys.o nouveau_lock.o		\
		  nouveau_winsys_pipe.o nouveau_device.o nouveau_notifier.o nouveau_dma.o	\
		  nouveau_pushbuf.o nouveau_resource.o nouveau_screen.o nv04_surface.o		\
		  nv50_surface.o #nouveau_winsys_softpipe.o

CFLAGS		+= -g -Wall -fPIC			\
		   -I${GALLIUMDIR}/include		\
		   -I${GALLIUMDIR}/winsys/g3dvl		\
		   -I${DRMDIR}/include			\
		   -I${DRMDIR}/include/drm		\
		   -I${GALLIUMDIR}/drivers		\
		   -I${GALLIUMDIR}/auxiliary		\
		   -I${DRIDIR}/include

LDFLAGS		+= -L${DRMDIR}/lib			\
		   -L${DRIDIR}/lib			\
		   -L${GALLIUMDIR}/auxiliary/draw	\
		   -L${GALLIUMDIR}/auxiliary/tgsi	\
		   -L${GALLIUMDIR}/auxiliary/translate	\
		   -L${GALLIUMDIR}/auxiliary/rtasm	\
		   -L${GALLIUMDIR}/auxiliary/cso_cache	\
		   -L${GALLIUMDIR}/drivers/nv10		\
		   -L${GALLIUMDIR}/drivers/nv20		\
		   -L${GALLIUMDIR}/drivers/nv30		\
		   -L${GALLIUMDIR}/drivers/nv40		\
		   -L${GALLIUMDIR}/drivers/nv50

LIBS		+= -ldriclient -ldrm -lnv10 -lnv20 -lnv30 -lnv40 -lnv50 -ldraw -ltgsi -ltranslate -lrtasm -lcso_cache -lm

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
