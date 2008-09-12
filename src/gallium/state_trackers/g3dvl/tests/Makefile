GALLIUMDIR	= ../../..

CFLAGS	+= -g -Wall -Werror				\
	   -I${GALLIUMDIR}/state_trackers/g3dvl		\
	   -I${GALLIUMDIR}/winsys/g3dvl			\
	   -I${GALLIUMDIR}/include			\
	   -I${GALLIUMDIR}/auxiliary			\
	   -I${GALLIUMDIR}/drivers
LDFLAGS	+= -L${GALLIUMDIR}/state_trackers/g3dvl		\
	   -L${GALLIUMDIR}/drivers/softpipe		\
	   -L${GALLIUMDIR}/auxiliary/tgsi		\
	   -L${GALLIUMDIR}/auxiliary/draw		\
	   -L${GALLIUMDIR}/auxiliary/util		\
	   -L${GALLIUMDIR}/auxiliary/translate		\
	   -L${GALLIUMDIR}/auxiliary/cso_cache		\
	   -L${GALLIUMDIR}/auxiliary/rtasm
LIBS	+= -lg3dvl -lsoftpipe -ldraw -ltgsi -ltranslate -lrtasm -lcso_cache -lutil -lX11 -lm

#############################################

.PHONY	= all clean

all: test_context test_surface test_i_rendering test_p_rendering test_pf_rendering test_b_rendering

test_context: test_context.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_surface: test_surface.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_i_rendering: test_i_rendering.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_p_rendering: test_p_rendering.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_pf_rendering: test_pf_rendering.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_b_rendering: test_b_rendering.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

clean:
	rm -rf *.o test_context test_surface test_i_rendering test_p_rendering test_pf_rendering test_b_rendering

