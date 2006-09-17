# src/mesa/drivers/dri/nouveau/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = nouveau_dri.so

MINIGLX_SOURCES = 

DRIVER_SOURCES = \
	nouveau_context.c        \
	nouveau_driver.c         \
	nouveau_fifo.c           \
	nouveau_ioctl.c          \
	nouveau_lock.c           \
	nouveau_span.c           \
	nouveau_state.c		 \
	nouveau_tex.c            \
	nouveau_swtcl.c          \
	nv10_swtcl.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

ASM_SOURCES = 


include ../Makefile.template

symlinks:
